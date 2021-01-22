#ifndef MULTIPLAYER_STATE_INCLUDED
#define MULTIPLAYER_STATE_INCLUDED


#include "BaseState.h"

#include "containers.h"
#include "EngineCore.h"
#include "GameFramework.h"
#include "memoryutilities.h"
#include "Components.h"
#include "GuiUtilities.h"

#include <functional>
#include <slikenet/peerinterface.h>
#include <slikenet/statistics.h>
#include <slikenet/MessageIdentifiers.h>
#include <slikenet/types.h>



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

        rhs.data        = nullptr;
        rhs.sender      = InvalidHandle_t;
        rhs.allocator   = nullptr;
        rhs.dataSize    = 0;
    }


	Packet& operator = (Packet&& rhs) noexcept// Move
	{
		if (data)
			Release();

		data        = rhs.data;
		sender      = rhs.sender;
		allocator   = rhs.allocator;

        rhs.data        = nullptr;
        rhs.sender      = InvalidHandle_t;
        rhs.allocator   = nullptr;
        rhs.dataSize    = 0;

		return *this;
	}

	void Release()
	{
		if (data)
			allocator->free(data);

        data            = nullptr;
        sender          = InvalidHandle_t;
        allocator       = nullptr;
        dataSize        = 0;
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

    SLNet::RakPeerInterface& raknet = *SLNet::RakPeerInterface::GetInstance();

	struct openSocket
	{
		uint64_t state      = 0;
		uint64_t latency    = 0;

        SLNet::SystemAddress address;
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


    ~NetworkState();

	/************************************************************************************************/


    void Startup(short port);
    void Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT) final override;
    void Broadcast(const UserPacketHeader& packet);


    void Send           (const UserPacketHeader& packet, ConnectionHandle destination);
    void PushHandler    (Vector<PacketHandler*>& handler);
    void PopHandler     ();

    void Connect            (const char* address, uint16_t port);
    void CloseConnection    (ConnectionHandle handle);


    ConnectionHandle    FindConnectionHandle(SLNet::SystemAddress address);
    openSocket          GetConnection(ConnectionHandle handle);

    void RemoveConnectionHandle(SLNet::SystemAddress address);
    void SystemInError();


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
