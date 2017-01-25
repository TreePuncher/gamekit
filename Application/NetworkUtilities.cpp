/**********************************************************************

Copyright (c) 2015 - 2016 Robert May

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

#include "stdafx.h"
#include "NetworkUtilities.h"

#if USING(RACKNET)


/************************************************************************************************/


void CleanupNetwork(Network* Net)
{
	if (Net->isServer)
		Net->Peer->Shutdown(0);

	RakNet::RakPeerInterface::ReleaseInstance(Net->Peer);
}

/************************************************************************************************/

void InitiateNetwork(Network* Net, FlexKit::StackAllocator* alloc, bool isServer, unsigned short ServerPort)
{
	Net->Peer = RakNet::RakPeerInterface::GetInstance();

	if(isServer)
	{
		Net->ServerState =&alloc->allocate<Network::sServerState>();
		Net->ServerState->Allocator.Init(Net->ServerState->_p, sizeof(Net->ServerState->_p));

		RakNet::SocketDescriptor sd(ServerPort,0);
		auto res = Net->Peer->Startup(5, &sd, 1);
		Net->Peer->SetMaximumIncomingConnections(16);
		Net->ServerState->CurrentState = Network::sServerState::WaitingForPlayers;
	}
	else
	{
		char str[512];
		Net->ServerPort  = ServerPort;
		Net->ClientState =&alloc->allocate<Network::sClientState>();
		Net->ClientState->Allocator.Init(Net->ClientState->_p, sizeof(Net->ClientState->_p));

		RakNet::SocketDescriptor desc;
		memset(str, 0, sizeof(str));
		std::cout << "Enter Server Address\n";
		std::cin >> str; 
		Net->Peer->Startup(1, &desc, 1);
		auto res = Net->Peer->Connect(str, Net->ServerPort, nullptr, 0 );
		Net->ClientState->CurrentState = Network::sClientState::ClientEstablishingConnection;
	}
}

#endif