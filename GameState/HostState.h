#ifndef PLAYHOST_H
#define PLAYHOST_H

/**********************************************************************

Copyright (c) 2017 Robert May

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


#include <WindowsIncludes.h>
#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "RakNetTypes.h"
#include "BitStream.h"

#include "Gameplay.h"
#include "..\Application\GameMemory.h"
#include "..\Application\GameFramework.h"
#include "..\coreutilities\type.h"
#include <inttypes.h>

/*
TODO's
*/


struct LocalPlayer
{
	FlexKit::EntityHandle Model;
};

enum NetCommands : unsigned char
{
	STARTINDEX = ID_USER_PACKET_ENUM,
	eREQUESTNAME,
	eNAMEREPLY,
	eSENDCLIENTINFO,
	eGAMEREADYUP,
	eGAMESETINFO,
	eGAMESTARTING,
	eGAMEENDED,
	eINCOMINGSTRUCT,
};

const unsigned short gServerPort = 1366;
const size_t MaxNameSize = 64;

struct PacketBase
{
	Type_t ID;
	size_t PacketSize;// Should I do a CRC?

	operator char* () { return (char*)this; }
};

struct PacketUtility
{
	char		Command;
	PacketBase	P;

	//operator char* () { return (char*)this; }
};

struct PlayerReadyState
{
	uint8_t Ready;
};

struct GetNamePacket;

struct GetNamePacket : PacketBase
{
	GetNamePacket( char* str)
	{
		ID = GetTypeGUID(GetNamePacket);
		PacketSize = sizeof(GetNamePacket);
		strncpy(Name, str, MaxNameSize-1);
	}

	char Name[MaxNameSize];
};

struct GameInfoPacket : PacketBase
{
	GameInfoPacket(size_t playercount, char* level) : PlayerCount(playercount)
	{
		ID = GetTypeGUID(GameInfoPacket);
		strncpy(LevelName, level, sizeof(LevelName));
		PacketSize = sizeof(GameInfoPacket) + sizeof(PlayerID_t) * playercount;
	}

	size_t		PlayerCount;
	char		LevelName[128];

	PlayerID_t	PlayerIDs[];
};

struct GameModePacket : PacketBase
{
	GameModePacket(ClientMode mode) : Mode(mode)
	{
		ID = GetTypeGUID(GameModePacket);
	}

	ClientMode Mode;
};

struct ConnectionAccepted
{
	size_t PlayerID;
};

struct PlayerInputPacket : PacketBase
{
	PlayerInputPacket(InputFrame input) : Input(input)
	{
		ID = GetTypeGUID(PlayerInputPacket);
	}
	InputFrame	Input;
};


struct PlayerInfoPacket : PacketBase
{
	PlayerInfoPacket(size_t playerID, Quaternion Q, float3 xyz) : PlayerID(playerID), POS(xyz), R(Q)
	{
		ID			= GetTypeGUID(_PlayerInfoPacket_);
		PacketSize	= sizeof(PlayerInfoPacket);
	}

	size_t		PlayerID;
	float3		POS;
	Quaternion	R;
};


struct SetPlayerInfoPacket : PacketBase
{
	SetPlayerInfoPacket(float3 xyz, Quaternion Q) : POS(xyz), R(Q)
	{
		ID = GetTypeGUID(SetPlayerInfoPacket);
		PacketSize = sizeof(SetPlayerInfoPacket);
	}

	size_t		PlayerID;
	float3		POS;
	Quaternion	R;
};


struct PlayerClientStateInfoUpdate: PacketBase
{
	PlayerClientStateInfoUpdate(PlayerStateFrame state) : State(state)
	{
		ID			= GetTypeGUID(PlayerClientStateInfoUpdate);
		PacketSize	= sizeof(PlayerClientStateInfoUpdate);
	}

	PlayerStateFrame State;
};

struct HostState : public SubState
{
	RakNet::RakPeerInterface*	Peer;
	size_t						PlayerCount;
	size_t						MinPlayerCount;

	double						T;

	ServerMode ServerMode;

	struct Connection
	{
		RakNet::SystemAddress Addr;
		PlayerID_t			  ID;
	};

	struct PlayerName {
		char Name[MaxNameSize];
	};

	PhysicsComponentSystem	Scene;
	//GameplayComponentSystem Game;

	FlexKit::static_vector<Connection>	OpenConnections;
	FlexKit::static_vector<bool>		PlayerReadyState;
	FlexKit::static_vector<PlayerName>	Names;
	FlexKit::static_vector<ClientMode>	ClientModes;
};

size_t GetPlayerIndex(HostState* Host, RakNet::SystemAddress Addr);

HostState* CreateHostState(EngineMemory* Engine, GameFramework* Framework);


#endif
