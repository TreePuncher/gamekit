/**********************************************************************

Copyright (c) 2015 - 2017 Robert May

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

#ifndef NETWORKUTILITIES_H
#define NETWORKUTILITIES_H

#if USING(RACKNET)

#include <WindowsIncludes.h>
#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "RakNetTypes.h"
#include "BitStream.h"

#include "..\buildsettings.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\memoryutilities.h"

#include <iostream>

using FlexKit::RingBuffer;
/************************************************************************************************/

struct ClientConnectingState
{
	double WaitTime; // Connection Attempt Started
};

/************************************************************************************************/

struct ClientGameInProgress
{
};

/************************************************************************************************/

struct ServerStateGameInPorgess
{
	double T;
};

/************************************************************************************************/

enum PlayerState
{
	NOT_READY = 0,
	READY
};

/************************************************************************************************/


struct PlayerConnection
{
	size_t									PlayerID;
	RakNet::SystemAddress					Address;
	char									Name[32];
};

typedef FlexKit::static_vector<PlayerConnection>	ConnectionList;
typedef size_t										ConnectionID;


/************************************************************************************************/

struct Network
{
	RakNet::RakPeerInterface*	Peer;
	
	struct sClientState
	{
		enum
		{
			ClientDisconnected,
			ClientEstablishingConnection,
			ClientInGame,
			ClientInLobby,
			ClientTextMessage
		}CurrentState;

		union
		{
			ClientConnectingState	ClientConnecting;
			ClientGameInProgress	GameInProgress;
		}StateData;

		char					PlayerName[32];
		RakNet::AddressOrGUID	ServerAddress;

		ConnectionList			OtherPlayers;

		FlexKit::StackAllocator	Allocator;
		FlexKit::byte _p[1024];
	}*ClientState;

	struct sServerState
	{
		enum
		{
			WaitingForPlayers,
			GamePlaying
		}CurrentState;

		union
		{
			ServerStateGameInPorgess GameInProgress;
		}StateData;
		size_t					PlayerCount;
		FlexKit::StackAllocator	Allocator;
		FlexKit::byte _p[1024];
	}*ServerState;

	bool			isServer;
	unsigned short  ServerPort;
};

/************************************************************************************************/

void CleanupNetwork		(Network* Net);
void InitiateNetwork	(Network* Net, FlexKit::StackAllocator* alloc, bool isServer, unsigned short ServerPort);

/************************************************************************************************/

#endif
#endif
