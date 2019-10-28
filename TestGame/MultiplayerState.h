#ifndef MULTIPLAYER_STATE_INCLUDED
#define MULTIPLAYER_STATE_INCLUDED


#include "MultiplayerGameState.h"
#include "BaseState.h"

#include "..\coreutilities\containers.h"
#include "..\coreutilities\EngineCore.h"
#include "..\coreutilities\GameFramework.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\coreutilities\Components.h"
#include "..\graphicsutilities\GuiUtilities.h"

#include <functional>
#include <raknet/Source/RakPeerInterface.h>
#include <raknet/Source/RakNetStatistics.h>
#include <raknet/Source/MessageIdentifiers.h>


#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "user32.lib")

#ifdef _DEBUG
#pragma comment(lib, "RakNet_VS2008_LibStatic_Debug_x64.lib")
#else
#pragma comment(lib, "RakNet_VS2008_LibStatic_Release_x64.lib")
#endif


using FlexKit::EngineCore;
using FlexKit::UpdateDispatcher;
using FlexKit::GameFramework;


/************************************************************************************************/


using ConnectionHandle = Handle_t<32, GetTypeGUID(ConnectionHandle)>;


/************************************************************************************************/

// Manages the lifespan of the packet
class Packet
{
public:
	Packet(void* IN_data = nullptr, size_t IN_size = 0, ConnectionHandle IN_sender = InvalidHandle_t, iAllocator* IN_allocator = nullptr) :
		dataSize    { IN_size       },
		data        { IN_data       },
		sender      { IN_sender     },
		allocator   { IN_allocator  } {}


	~Packet() { Release(); }

	// No Copy!
	Packet(const Packet&)                   = delete;
	const Packet operator = (const Packet&) = delete;


	Packet(Packet&& rhs) noexcept
	{
		if (data)
			Release();

		data        = rhs.data;
		sender      = rhs.sender;
		allocator   = rhs.allocator;

		rhs.Clear();
	}


	Packet& operator = (Packet&& rhs) noexcept// Move
	{
		if (data)
			Release();

		data        = rhs.data;
		sender      = rhs.sender;
		allocator   = rhs.allocator;

		rhs.Clear();

		return *this;
	}

	void Release()
	{
		if (data)
		{
			allocator->free(data);
			Clear();
		}
	}


	void Clear()
	{
		data        = nullptr;
		sender      = InvalidHandle_t;
		allocator   = nullptr;
	}


	static Packet CopyCreate(void* data, size_t data_size, ConnectionHandle sender, iAllocator* allocator)
	{
		auto buffer = (char*)allocator->malloc(data_size);
		memcpy(buffer, data, data_size);

		return { data, data_size, sender, allocator };
	}

	void*               data        = nullptr;
	size_t              dataSize    = 0;
	ConnectionHandle    sender      = InvalidHandle_t;
	iAllocator*         allocator   = nullptr;
};


/************************************************************************************************/
 

typedef size_t PacketID_t;


enum EBasePacketIDs : unsigned char
{
	EBP_USERPACKET = DefaultMessageIDTypes::ID_USER_PACKET_ENUM,
	EBP_COUNT,
};


class UserPacketHeader
{
public:
	UserPacketHeader(const size_t IN_size, PacketID_t IN_id) :
		EBasePacketIDs	{ EBP_USERPACKET	},
		packetSize		{ IN_size			},
		id				{ IN_id				} {}

	PacketID_t GetID() 
	{ 
		return id;
	}

	unsigned char		EBasePacketIDs;
	const size_t		packetSize;
	const PacketID_t	id;
};


/************************************************************************************************/


class NetworkState;

class PacketHandler
{
public:
	PacketHandler(PacketID_t IN_id) :
		packetTypeID{ IN_id } {}

	const PacketID_t packetTypeID;

	virtual ~PacketHandler() {};
	virtual void HandlePacket(UserPacketHeader* incomingPacket, Packet* packet, NetworkState* network) = 0;
};

using PacketHandlerVector = FlexKit::Vector<PacketHandler*>;


/************************************************************************************************/


template<typename FN_TY>
class LambdaPacketHandler : public PacketHandler
{
public:
	LambdaPacketHandler(
		PacketID_t IN_id, 
		FN_TY&& IN_FN) :
			PacketHandler	{IN_id}, 
			_FN				{std::move(IN_FN)}{}

	void HandlePacket(
			UserPacketHeader*	header, 
			Packet*				packet, 
			NetworkState*		network) override
	{
		_FN(header, packet, network);
	}

	FN_TY _FN;
};

template<typename FN_TY>
LambdaPacketHandler<FN_TY>* CreatePacketHandler(PacketID_t IN_id, FN_TY&& FN, FlexKit::iAllocator* allocator)
{
	return &allocator->allocate_aligned<LambdaPacketHandler<FN_TY>>(IN_id, std::move(FN));
}


/************************************************************************************************/



class NetworkState : public FlexKit::FrameworkState
{
protected:

	RakNet::RakPeerInterface& raknet = *RakNet::RakPeerInterface::GetInstance();

	struct openSocket
	{
		uint64_t state      = 0;
		uint64_t latency    = 0;

		RakNet::SystemAddress address;
		ConnectionHandle    handle;
	};


	void PushIncomingPacket(Packet&& packet)
	{
	   incomingPackets.emplace_back(std::move(packet));
	}


public:
	NetworkState(
		GameFramework&	IN_framework, 
		BaseState&		IN_base) :
			FrameworkState	{ IN_framework                        },
			handlerStack	{ IN_framework.core.GetBlockMemory()  },
			incomingPackets { IN_framework.core.GetBlockMemory()  },
			openConnections { IN_framework.core.GetBlockMemory()  }
	{
        raknet.SetMaximumIncomingConnections(16);
    }


	~NetworkState()
	{
		while (incomingPackets.size())
		{
			auto packet = incomingPackets.pop_front();
			packet.Release();
		}

        RakNet::RakPeerInterface::DestroyInstance(&raknet);
	}


	/************************************************************************************************/

    void Startup(short port)
    {
        raknet.InitializeSecurity(0, 0, false);
        RakNet::SocketDescriptor socketDescriptor;
        socketDescriptor.port = port;

        const auto res = raknet.Startup(10, &socketDescriptor, 1);
        FK_ASSERT(res, "Failed to startup Raknet!");

        raknet.SetOccasionalPing(true);
        raknet.SetUnreliableTimeout(1000);
    }


	void Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT) final override
	{
		// Recieve Packets
		for(auto packet = raknet.Receive(); packet != nullptr; raknet.DeallocatePacket(packet), packet = raknet.Receive())
		{
            FK_LOG_INFO("Packet Recieved!");

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

                Accepted(openConnections.back().handle);
            }   break;
            case ID_CONNECTION_LOST:
			case ID_DISCONNECTION_NOTIFICATION:
			{
                FK_LOG_INFO("Detected disconnection!");

                auto handle = FindConnectionHandle(packet->systemAddress);
                if (handle == InvalidHandle_t) {
                    SystemInError();
                    return;
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
                FK_LOG_INFO("Unrecognized packet, dropped!");

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
	}


	/************************************************************************************************/


    void Broadcast(UserPacketHeader& packet)
    {
        for (auto socket : openConnections)
            raknet.Send((const char*)& packet, packet.packetSize, PacketPriority::MEDIUM_PRIORITY, PacketReliability::UNRELIABLE, 0, socket.address, false, 0);
    }


    /************************************************************************************************/


	void Send(UserPacketHeader& packet, ConnectionHandle destination)
	{
        auto socket = GetConnection(destination);

        raknet.Send((const char*)&packet, packet.packetSize, PacketPriority::MEDIUM_PRIORITY, PacketReliability::UNRELIABLE, 0, socket.address, false, 0);
	}


	/************************************************************************************************/


	void PushHandler(Vector<PacketHandler*>& handler)
	{
		handlerStack.push_back(&handler);
	}


	/************************************************************************************************/


	void PopHandler()
	{
		handlerStack.pop_back();
	}


	/************************************************************************************************/


	void Connect(const char* address, uint16_t port)
	{
        raknet.Startup(16, &RakNet::SocketDescriptor(), 1);

        auto res = raknet.Connect("127.0.0.1", port, nullptr, 0);
	}


    void CloseConnection(ConnectionHandle handle)
    {
        auto socket = GetConnection(handle);
        raknet.CloseConnection(socket.address, true);
    }


    /************************************************************************************************/


	ConnectionHandle FindConnectionHandle(RakNet::SystemAddress address)
	{
		for (size_t i = 0; i < openConnections.size(); i++)
			if (openConnections[i].address == address)
				return openConnections[i].handle;

		return InvalidHandle_t;
	}


    /************************************************************************************************/


    openSocket GetConnection(ConnectionHandle handle)
    {
        for (size_t i = 0; i < openConnections.size(); i++)
            if (openConnections[i].handle == handle)
                return openConnections[i];

        return openSocket{};
    }


	/************************************************************************************************/


	void RemoveConnectionHandle(RakNet::SystemAddress address)
	{
		for (size_t i = 0; i < openConnections.size(); i++)
			if (openConnections[i].address == address)
			{
				openConnections[i] = openConnections.back();
				openConnections.pop_back();
			}
	}


    /************************************************************************************************/


    void SystemInError()
    {
        framework.core.End = true;
    }


    /************************************************************************************************/


	SL_list<Packet>                     incomingPackets;
	Vector<Vector<PacketHandler*>*>		handlerStack;
	Vector<openSocket>		            openConnections;

    std::function<void (ConnectionHandle)> Accepted;
    std::function<void (ConnectionHandle)> HandleNewConnection;
    std::function<void (ConnectionHandle)> HandleDisconnection;

    float                               timer = 0;
	uint16_t                            port;
	bool                                running;
};


/************************************************************************************************/


struct GameDescription
{
	size_t MaxPlayerCount	= 4;
	short  Port				= 1337;
};


/************************************************************************************************/


enum UserPacketIDs : PacketID_t
{
	ClientDataRequest,
	ClientDataRequestResponse,
	ClientDisconnect,
	ClientReadyEvent,
	LobbyMessage,
	UserPacketIDCount,
};


/**********************************************************************

Copyright (c) 2019 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/

#endif
