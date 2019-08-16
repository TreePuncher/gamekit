/**********************************************************************

Copyright (c) 2018 Robert May

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


#include "Client.h"
#include "..\coreutilities\MathUtils.h"


/************************************************************************************************/


ClientLobbyState::ClientLobbyState(
	FlexKit::GameFramework* IN_framework,
	GameClientState*		IN_client,
	NetworkState*			IN_network, 
	const char*				IN_localPlayerName) :
		FrameworkState	{ IN_framework },

		client			{ *IN_client			},
		localPlayerName { IN_localPlayerName	},
		network			{ *IN_network			},
		ready			{ false					},
		refreshCounter	{ 0						},
		packetHandlers	{ IN_framework->core->GetBlockMemory()										},
		screen			{ IN_framework->core->GetBlockMemory(), IN_framework->DefaultAssets.Font	}
{
	packetHandlers.push_back(
		CreatePacketHandler(
			LobbyMessage, 
			[&](auto, auto, auto)
			{
				std::cout << "Message Received!\n";
			}, 
			IN_framework->core->GetBlockMemory()));


	/*
	packetHandlers.push_back(
		CreatePacketHandler(
			ClientDataRequest,
			[&](UserPacketHeader* incomingPacket, RakNet::Packet* P, NetworkState* network)
			{
				auto				request = reinterpret_cast<RequestClientDataPacket*>(incomingPacket);
				ClientDataPacket	responsePacket(request->playerID, localPlayerName);

				client.localID = request->playerID;

				FK_LOG_INFO("Sending Client Info");
				std::cout << "playerID set to: " << request->playerID << "\n";
				network->SendPacket(responsePacket.GetRawPacket(), P->systemAddress);
			},
			IN_framework->core->GetBlockMemory()));


	packetHandlers.push_back(
		CreatePacketHandler(
			RequestPlayerListResponse,
			[&](UserPacketHeader* incomingPacket, RakNet::Packet* P, NetworkState* network)
			{
				FK_LOG_INFO("Player List Received");

				screen.ClearRows();
				client.remotePlayers.clear();

				auto playerList				= reinterpret_cast<PlayerListPacket*>(incomingPacket);
				const size_t playerCount	= FlexKit::max(FlexKit::min(playerList->playerCount, 3u), 0u);

				screen.CreateRow		(client.localID);
				screen.SetPlayerName	(client.localID, localPlayerName);
				screen.SetPlayerReady	(client.localID, ready);

				for (size_t idx = 0; idx < playerCount; ++idx)
				{
					auto nameStr	= playerList->Players[idx].playerName;
					auto id			= playerList->Players[idx].playerID;
					auto ready		= playerList->Players[idx].ready;

					client.remotePlayers.push_back({ playerList->Players[idx].playerID });
					strncpy(client.remotePlayers.back().name, nameStr, sizeof(RemotePlayer::name));

					screen.CreateRow		(id);
					screen.SetPlayerName	(id, client.remotePlayers.back().name);
					screen.SetPlayerReady	(id, ready);
				}
			},
			IN_framework->core->GetBlockMemory()));

		packetHandlers.push_back(
		CreatePacketHandler(
			StartGame,
			[&](UserPacketHeader* incomingPacket, RakNet::Packet* P, NetworkState* network)
			{
				FK_LOG_INFO("Starting Game");
				client.StartGame();
			},
			IN_framework->core->GetBlockMemory()));

	network.PushHandler(&packetHandlers);
	*/
}


/************************************************************************************************/


ClientLobbyState::~ClientLobbyState()
{
	network.PopHandler();

	for (auto handler : packetHandlers)
		framework->core->GetBlockMemory().free(handler);
}


/************************************************************************************************/


bool ClientLobbyState::EventHandler(FlexKit::Event evt)
{
	if (evt.InputSource == FlexKit::Event::InputType::Keyboard)
	{
		if (evt.Action == FlexKit::Event::InputAction::Release)
		{
			switch (evt.mData1.mKC[0])
			{
			case FlexKit::KC_R:
			{
				ready = !ready;
				
				ClientReady packet(client.localID, ready);
				FK_ASSERT(0);
				//network.SendPacket(packet.GetRawPacket(), client.ServerAddress);
			}
			}
		}
	}
	return true;
}


/************************************************************************************************/


bool ClientLobbyState::Update(FlexKit::EngineCore* Engine, FlexKit::UpdateDispatcher& Dispatcher, double dT)
{
	if (refreshCounter++ > 10)
	{
		refreshCounter = 0;

		RequestPlayerListPacket packet{ client.localID };
		FK_ASSERT(0);
		//client.network.SendPacket(packet.GetRawPacket(), client.ServerAddress);
	}

	return true;
}


/************************************************************************************************/


bool ClientLobbyState::Draw(FlexKit::EngineCore* core, FlexKit::UpdateDispatcher& dispatcher, double dT, FlexKit::FrameGraph& frameGraph)
{
	auto currentRenderTarget = GetCurrentBackBuffer(&core->Window);

	ClearVertexBuffer	(frameGraph, client.base.vertexBuffer);
	ClearVertexBuffer	(frameGraph, client.base.textBuffer);
	ClearBackBuffer		(frameGraph, currentRenderTarget);

	LobbyScreenDrawDesc Desc;
	Desc.allocator			= core->GetTempMemory();
	Desc.constantBuffer		= client.base.constantBuffer;
	Desc.vertexBuffer		= client.base.vertexBuffer;
	Desc.textBuffer			= client.base.textBuffer;
	Desc.renderTarget		= currentRenderTarget;
	screen.Draw(Desc, dispatcher, frameGraph);

	FlexKit::DrawMouseCursor(
		framework->MouseState.NormalizedScreenCord,
		{ 0.05f, 0.05f },
		client.base.vertexBuffer,
		client.base.constantBuffer,
		currentRenderTarget,
		core->GetTempMemory(),
		&frameGraph);

	return true;
}


/************************************************************************************************/


bool ClientLobbyState::PostDrawUpdate(FlexKit::EngineCore* core, FlexKit::UpdateDispatcher& dispatcher, double dT, FlexKit::FrameGraph& frameGraph)
{
	if (framework->drawDebugStats)
		framework->DrawDebugHUD(dT, client.base.textBuffer, frameGraph);

	PresentBackBuffer(frameGraph, &core->Window);
	return true;
}


/************************************************************************************************/