#ifndef MULTIPLAYER_STATE_INCLUDED
#define MULTIPLAYER_STATE_INCLUDED

#include <WindowsIncludes.h>
#include <functional>

#include "MultiplayerGameState.h"
#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "RakNetTypes.h"
#include "BitStream.h"
#include "BaseState.h"

#include "..\coreutilities\containers.h"
#include "..\coreutilities\EngineCore.h"
#include "..\coreutilities\GameFramework.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\coreutilities\Components.h"
#include "..\graphicsutilities\GuiUtilities.h"


#ifdef _DEBUG
#pragma comment(lib, "RakNet_VS2008_LibStatic_Debug_x64.lib")
#else
#pragma comment(lib, "RakNet_VS2008_DLL_Release_x64.lib")
#endif


using FlexKit::EngineCore;
using FlexKit::UpdateDispatcher;


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
		EBasePacketIDs	{EBP_USERPACKET},
		packetSize		{IN_size},
		id				{IN_id}{}

	PacketID_t GetID() 
	{ 
		return id;
	}

	unsigned char		EBasePacketIDs;
	const size_t		packetSize;
	const PacketID_t	id;
};


/************************************************************************************************/


class Packet
{
public:
	virtual UserPacketHeader* GetRawPacket() = 0;
};


class NetworkState;

class PacketHandler
{
public:
	PacketHandler(PacketID_t IN_id) :
		packetTypeID{ IN_id } {}

	const PacketID_t packetTypeID;

	virtual ~PacketHandler() {};
	virtual void HandlePacket(UserPacketHeader* incomingPacket, RakNet::Packet*, NetworkState* network) = 0;
};


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
			RakNet::Packet*		packet, 
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
public:
	NetworkState(
		FlexKit::GameFramework* IN_framework, 
		BaseState*	IN_base) :
			FrameworkState	{ IN_framework },
			handlerStack	{ IN_framework->core->GetBlockMemory() }
	{
		localPeer = RakNet::RakPeerInterface::GetInstance();
		localPeer->SetMaximumIncomingConnections(16);
	}


	~NetworkState()
	{
		RakNet::RakPeerInterface::DestroyInstance(localPeer);
	}


	/************************************************************************************************/


	bool Update(FlexKit::EngineCore* Engine, FlexKit::UpdateDispatcher& Dispatcher, double dT) override
	{
		// Handle incoming Packets

		RakNet::Packet *packet;
		for (
			packet = localPeer->Receive(); 
			packet; localPeer->DeallocatePacket(packet), packet = localPeer->Receive())
		{
			FK_LOG_9("Packet Recieved!");
			switch (packet->data[0])
			{
			case DefaultMessageIDTypes::ID_NEW_INCOMING_CONNECTION:
				FK_LOG_INFO("New Connection Incoming");

				if (NewConnectionHandler)
					NewConnectionHandler(packet);

				break;
			case DefaultMessageIDTypes::ID_CONNECTION_REQUEST_ACCEPTED:
				FK_LOG_9("Connection Accepted");

				if (ConnectionAcceptedHandler)
					ConnectionAcceptedHandler(packet);

				break;
			case DefaultMessageIDTypes::ID_CONNECTION_LOST:
				FK_LOG_9("Disconnect Detected");

				if (DisconnectHandler)
					DisconnectHandler(packet);

				break;
			case EBP_USERPACKET:
			{
				UserPacketHeader* rp = reinterpret_cast<UserPacketHeader*>(packet->data);
				if (handlerStack.size())
				{
					const auto packetTypeID = rp->GetID();
					for (auto handler : *handlerStack.back())
					{
						if (handler->packetTypeID == packetTypeID)
							handler->HandlePacket(rp, packet, this);
					}
				}
			}
			}
		}

		return true;
	}


	/************************************************************************************************/


	void SendPacket(UserPacketHeader* packet, RakNet::SystemAddress& addr)
	{
		localPeer->Send(
			(const char*)packet,
			packet->packetSize,
			PacketPriority::MEDIUM_PRIORITY,
			PacketReliability::UNRELIABLE_SEQUENCED,
			0,
			addr, 
			false,
			false);
	}


	/************************************************************************************************/


	void PushHandler(Vector<PacketHandler*>* handler)
	{
		handlerStack.push_back(handler);
	}


	/************************************************************************************************/


	void PopHandler()
	{
		handlerStack.pop_back();
	}


	/************************************************************************************************/


	std::function<void (RakNet::Packet*)> NewConnectionHandler;
	std::function<void (RakNet::Packet*)> ConnectionAcceptedHandler;
	std::function<void (RakNet::Packet*)> DisconnectHandler;

	unsigned short						serverPort;
	RakNet::RakPeerInterface*			localPeer;
	Vector<Vector<PacketHandler*>*>		handlerStack;
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