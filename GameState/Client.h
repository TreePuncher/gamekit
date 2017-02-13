#ifndef CLIENT_H
#define CLIENT_H

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

#include "BaseState.h"
#include "Gameplay.h"
#include "..\Application\GameMemory.h"


/*
TODO's
*/

struct PlayerImposters
{
	FlexKit::EntityHandle	Graphics;
	size_t					PlayerID;
};



struct ClientState : public SubState
{
	RakNet::RakPeerInterface*	Peer;
	RakNet::SystemAddress		ServerAddr;

	DynArray<PlayerID_t>	PlayerIds;
	GUID_t					Scene;

	size_t ID;
	size_t PlayerCount;

	enum
	{
		JOINING,
		JOINED
	}CurrentState;

	char ClientName[256];
};

struct ClientPlayState : public SubState
{
	ClientState* ClientState;

	ClientMode	Mode;

	PlayerInputState	LocalInput;
	Player				LocalPlayer;
	float2				MouseInput;

	double				T;
	double				T_TillNextPositionUpdate; 
	size_t				FrameCount;

	FlexKit::DynArray<PlayerImposters> Imposters;// Cause hes an imposter, trying to steal all the client info
};

ClientState*		CreateClientState		(EngineMemory* Engine, BaseState* Base);
ClientPlayState*	CreateClientPlayState	(EngineMemory* Engine, BaseState* Base, ClientState* Peer);


#endif
