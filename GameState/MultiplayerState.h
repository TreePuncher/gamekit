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
#include <steam\isteamnetworkingsockets.h>
#include <steam\isteamnetworkingutils.h>

using FlexKit::EngineCore;
using FlexKit::UpdateDispatcher;
using FlexKit::GameFramework;


/************************************************************************************************/
 

typedef size_t PacketID_t;


enum EBasePacketIDs : unsigned char
{
	EBP_USERPACKET,
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
	virtual void HandlePacket(UserPacketHeader* incomingPacket, void* packet, NetworkState* network) = 0;
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
			void*				packet, 
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
		GameFramework*	IN_framework, 
		BaseState*		IN_base) :
			FrameworkState	{ IN_framework },
			handlerStack	{ IN_framework->core->GetBlockMemory() }
	{
	}


	~NetworkState()
	{
	}


	/************************************************************************************************/


    void Connect()
    {

    }


    /************************************************************************************************/


	bool Update(FlexKit::EngineCore* Engine, FlexKit::UpdateDispatcher& Dispatcher, double dT) override
	{
		// Handle incoming Packets
		return true;
	}


	/************************************************************************************************/


	void SendPacket(UserPacketHeader* packet)
	{
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

	unsigned short						serverPort;
    ISteamNetworkingSockets*            steamSockets;
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
