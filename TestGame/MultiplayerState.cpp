#include "MultiplayerState.h"

namespace SLNet{
    const SystemAddress UNASSIGNED_SYSTEM_ADDRESS;
    const RakNetGUID    UNASSIGNED_RAKNET_GUID(-1);
}

NetworkState::~NetworkState()
{
	while (incomingPackets.size())
	{
		auto packet = incomingPackets.pop_front();
		packet.Release();
	}

    SLNet::RakPeerInterface::DestroyInstance(&raknet);
}


/************************************************************************************************/

void NetworkState::Startup(short port)
{
    raknet.InitializeSecurity(0, 0, false);
    SLNet::SocketDescriptor socketDescriptor;
    socketDescriptor.port = port;

    const auto res = raknet.Startup(10, &socketDescriptor, 1);
    FK_ASSERT(res, "Failed to startup Raknet!");

    raknet.SetOccasionalPing(true);
    raknet.SetUnreliableTimeout(1000);
}


/************************************************************************************************/


UpdateTask* NetworkState::Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
{
	// Recieve Packets
	for(auto packet = raknet.Receive(); packet != nullptr; raknet.DeallocatePacket(packet), packet = raknet.Receive())
	{
        //FK_LOG_INFO("Packet Recieved!");

		switch(packet->data[0])
		{
        case ID_CONNECTION_REQUEST_ACCEPTED:
        {
            std::random_device random;
            openConnections.push_back(openSocket{
                                        0,
                                        0,
                                        packet->systemAddress,
                                        ConnectionHandle{ random() } });

            if(Accepted)
                Accepted(openConnections.back().handle);
        }   break;
        case ID_CONNECTION_LOST:
		case ID_DISCONNECTION_NOTIFICATION:
		{
            FK_LOG_INFO("Detected disconnection!");

            auto handle = FindConnectionHandle(packet->systemAddress);
            if (handle == InvalidHandle_t) {
                SystemInError();
                return nullptr;
            }

            HandleDisconnection(handle);
		}   break;
		case ID_NEW_INCOMING_CONNECTION:
		{
            FK_LOG_INFO("New incoming connection!");

			std::random_device random;
            const uint32_t id = random();
            openConnections.push_back(openSocket{
                                        0,
                                        0,
                                        packet->systemAddress,
                                        ConnectionHandle{ id }});

            raknet.Ping(packet->systemAddress);

            if(HandleNewConnection)
                HandleNewConnection(openConnections.back().handle);
		}   break;
		case EBP_USERPACKET:
		{   // Process packets in a deferred manner
			auto sender = FindConnectionHandle(packet->systemAddress);

			if (sender == InvalidHandle_t)
				continue;

			auto buffer = core.GetBlockMemory().malloc(packet->length);
			memcpy(buffer, packet->data, packet->length);

			PushIncomingPacket(Packet{ buffer, packet->length, sender, core.GetBlockMemory() });
		}   break;
		default:
		{
            FK_LOG_INFO("Unrecognized packet, dropped!  Packet ID: $u", uint32_t(packet->data[0]));

		}   break;
		}
	}

	// Handle game Packets
	while (incomingPackets.size())
	{
		auto                packet = incomingPackets.pop_front();
		UserPacketHeader*   header = reinterpret_cast<UserPacketHeader*>(packet.data);

		const auto packetID = header->GetID();
		for (auto handler : *handlerStack.back())
		{
			if (handler->packetTypeID == packetID)
				handler->HandlePacket(header, &packet, this);
		}
	}


    if (timer > 1.0f)
    {
        for (auto& socket : openConnections) {
            socket.latency = raknet.GetLastPing(socket.address);
            raknet.Ping(socket.address);// ping hosts every second to get latencies
        }
    }
    else
        timer += dT;

    return nullptr;
}


/************************************************************************************************/


void NetworkState::Broadcast(const UserPacketHeader& packet)
{
    for (auto socket : openConnections)
        raknet.Send((const char*)&packet, (int)packet.packetSize, PacketPriority::MEDIUM_PRIORITY, PacketReliability::UNRELIABLE, 0, socket.address, false, 0);
}


/************************************************************************************************/


void NetworkState::Send(const UserPacketHeader& packet, ConnectionHandle destination)
{
    if (destination == InvalidHandle_t)
        return;

    auto socket = GetConnection(destination);

    raknet.Send((const char*)&packet, (int)packet.packetSize, PacketPriority::MEDIUM_PRIORITY, PacketReliability::UNRELIABLE, 0, socket.address, false, 0);
}


/************************************************************************************************/


void NetworkState::PushHandler(Vector<PacketHandler*>& handler)
{
    handlerStack.push_back(&handler);
}


/************************************************************************************************/


void NetworkState::PopHandler()
{
    handlerStack.pop_back();
}


/************************************************************************************************/


void NetworkState::Connect(const char* address, uint16_t port)
{
    auto socketDesc = SLNet::SocketDescriptor();

    raknet.Startup(16, &socketDesc, 1);

    auto res = raknet.Connect("127.0.0.1", port, nullptr, 0);
}


/************************************************************************************************/


void NetworkState::Disconnect()
{
    openConnections.clear();
    raknet.Shutdown(0);
}


/************************************************************************************************/


void NetworkState::CloseConnection(ConnectionHandle handle)
{
    auto socket = GetConnection(handle);
    raknet.CloseConnection(socket.address, true);
}


/************************************************************************************************/


ConnectionHandle NetworkState::FindConnectionHandle(SLNet::SystemAddress address)
{
    for (size_t i = 0; i < openConnections.size(); i++)
        if (openConnections[i].address == address)
            return openConnections[i].handle;

    return InvalidHandle_t;
}


/************************************************************************************************/


NetworkState::openSocket NetworkState::GetConnection(ConnectionHandle handle)
{
    for (size_t i = 0; i < openConnections.size(); i++)
        if (openConnections[i].handle == handle)
            return openConnections[i];

    return openSocket{};
}


/************************************************************************************************/


void NetworkState::RemoveConnectionHandle(SLNet::SystemAddress address)
{
    for (size_t i = 0; i < openConnections.size(); i++)
        if (openConnections[i].address == address)
        {
            openConnections[i] = openConnections.back();
            openConnections.pop_back();
        }
}


/************************************************************************************************/


void NetworkState::SystemInError()
{
    framework.core.End = true;
}


