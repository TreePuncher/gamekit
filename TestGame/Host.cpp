#include "Host.h"
#include <random>
#include <limits>


using FlexKit::GameFramework;

/************************************************************************************************/


MultiplayerPlayerID_t GeneratePlayerID()
{
	std::random_device generator;
	std::uniform_int_distribution<MultiplayerPlayerID_t> distribution(
		std::numeric_limits<MultiplayerPlayerID_t>::min(),
		std::numeric_limits<MultiplayerPlayerID_t>::max());

	return distribution(generator);
}


/************************************************************************************************/


void GameHostLobbyState::Update(EngineCore& core, UpdateDispatcher& Dispatcher, double dT)
{
	for (auto player : playerLobbyState)
	{
		auto playerHostState = host.GetPlayer(player.ID);
		if (playerHostState->state == Lobby)
		{
			screen.SetPlayerName(player.ID, playerHostState->name);
			screen.SetPlayerReady(player.ID, player.Ready);
		}
	}

	FlexKit::WindowInput windowInput;
	windowInput.CursorWH                = { 0.05f, 0.05f };
	//windowInput.MousePosition           = framework.MouseState.NormalizedScreenCord;
	//windowInput.LeftMouseButtonPressed  = framework.MouseState.LMB_Pressed;

	screen.Update(dT, windowInput, base.renderWindow.GetPixelSize(), core.GetTempMemory());
}


/************************************************************************************************/


void GameHostLobbyState::Draw(EngineCore& core, UpdateDispatcher& Dispatcher, double dT, FrameGraph& frameGraph)
{
	auto currentRenderTarget = base.renderWindow.GetBackBuffer();

    frameGraph.Resources.AddBackBuffer(base.renderWindow.GetBackBuffer());

	ClearVertexBuffer	(frameGraph, host.base.vertexBuffer);
	ClearBackBuffer		(frameGraph, currentRenderTarget, { 0.0f, 0.0f, 0.0f, 0.0f });

	LobbyScreenDrawDesc Desc;
	Desc.allocator			= core.GetTempMemory();
	Desc.constantBuffer		= host.base.constantBuffer;
	Desc.vertexBuffer		= host.base.vertexBuffer;
	Desc.renderTarget		= currentRenderTarget;
	screen.Draw(Desc, Dispatcher, frameGraph);

    /*
	DrawMouseCursor(
		framework.MouseState.NormalizedScreenCord,
		{ 0.05f, 0.05f },
		host.base.vertexBuffer,
		host.base.constantBuffer,
		currentRenderTarget,
		core.GetTempMemory(),
		&frameGraph);
    */

    if (framework.drawDebugStats)
        framework.DrawDebugHUD(dT, host.base.vertexBuffer, base.renderWindow.GetBackBuffer(), frameGraph);

    PresentBackBuffer(frameGraph, base.renderWindow);
}


/************************************************************************************************/


void GameHostLobbyState::PostDrawUpdate(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
{
    base.PostDrawUpdate(core, dispatcher, dT);
}


/************************************************************************************************/


bool GameHostLobbyState::EventHandler(Event evt)
{
	if (evt.InputSource == Event::Keyboard &&
		evt.Action == Event::Pressed)
	{
		switch ((DEBUG_EVENTS)evt.mData1.mINT[0])
		{
		case FlexKit::KC_SPACE:
		{
			host.BeginGame();
            return true;
		}	break;
		default:
			break;
		}
	}

    return false;
}


/************************************************************************************************/


GameHostLobbyState::GameHostLobbyState(
	GameFramework&	IN_framework,
    BaseState&      IN_base,
	GameHostState&	IN_host) :
		FrameworkState		{ IN_framework																}, 
		packetHandlers		{ IN_framework.core.GetBlockMemory()										},
        base                { IN_base                                                                   },
		host				{ IN_host																	},
		playerLobbyState	{ IN_framework.core.GetBlockMemory()										},
		screen				{ IN_framework.core.GetBlockMemory(), IN_framework.DefaultAssets.Font	}
{

    host.network.HandleNewConnection =
        [&](auto connection)
        {
	        HandleNewConnection(connection);
        };


    host.network.HandleDisconnection =
        [&](auto connection)
        {
            HandleDisconnection(connection);
        };

	packetHandlers.push_back(
		CreatePacketHandler(
			ClientDataRequestResponse,
			[&](UserPacketHeader* packetContents, Packet* incomingPacket, NetworkState* network)
			{
				ClientDataPacket* ClientData = (ClientDataPacket*)packetContents;

				if (auto player = host.GetPlayer(ClientData->playerID); player)
				{
					player->state = Lobby;

					host.SetPlayerName(
						ClientData->playerID, 
						ClientData->playerName, 
						ClientData->playerNameLength);

					FK_LOG_9("Player: %s joined game", ClientData->playerName);
				}
			}, 
			IN_framework.core.GetBlockMemory()));


	packetHandlers.push_back(
		CreatePacketHandler(
			ClientReadyEvent,
			[&](UserPacketHeader* packetContents, Packet* incomingPacket, NetworkState* network)
			{
				FK_LOG_INFO("ready packet recieved!");

				ClientReady* readyPacket = (ClientReady*)packetContents;
				auto playerState = host.GetPlayer(readyPacket->playerID);

				if (playerState)
				{
					for (auto& playerLobby : playerLobbyState)
					{
						if (playerLobby.ID == readyPacket->playerID)
						{
							playerLobby.Ready = readyPacket->ready;

#ifdef _DEBUG
							if (readyPacket->ready)
								FK_LOG_9("Player: %s is now ready", playerState->name);
							else
								FK_LOG_9("Player: %s is now not ready", playerState->name);
#endif
						}
					}
				}
			}, 
			IN_framework.core.GetBlockMemory()));


	packetHandlers.push_back(
		CreatePacketHandler(
			RequestPlayerList,
			[&](UserPacketHeader* packetContents, Packet* incomingPacket, NetworkState* network)
			{
				FK_LOG_9("Player list requested");

				auto request				= (RequestPlayerListPacket*)(packetContents);
				auto packetSize				= PlayerListPacket::GetPacketSize(host.players.size() - 1);
				auto packetBuffer			= framework.core.GetTempMemory()._aligned_malloc(packetSize);
                PlayerListPacket& newPacket = *new(packetBuffer) PlayerListPacket{ request->playerID, host.players.size() - 1 };

				size_t idx = 0;
				for (auto& playerState : host.players)
				{
					if (playerState.ID == newPacket.playerID)
						continue;

					newPacket.Players[idx].playerID	= playerState.ID;

					if (playerState.local)
						newPacket.Players[idx].ready = localHostReady;
					else
					{
						auto [player, found] = GetPlayerLobbyState(playerState.ID);
						if(found)
							newPacket.Players[idx].ready = player.Ready;
					}

					strncpy(
						newPacket.Players[idx].playerName, 
						playerState.name,
						sizeof(PlayerListPacket::entry::playerName));

					idx++;
				}

			    host.network.Send(newPacket, incomingPacket->sender);
			}, 
			IN_framework.core.GetBlockMemory()));


	host.network.PushHandler(packetHandlers);
    screen.ClearRows();
}


/************************************************************************************************/


GameHostLobbyState::~GameHostLobbyState()
{
	host.network.PopHandler();
	for (auto handler : packetHandlers)
		framework.core.GetBlockMemory().free(handler);
}


/************************************************************************************************/


void GameHostLobbyState::AddLocalPlayer(const MultiplayerPlayerID_t ID)
{
	screen.CreateRow(ID);

	const auto localPlayer	= host.GetPlayer(ID);
	const auto nameLength	= strnlen(localPlayer->name, 32);

	host.SetState(ID, Lobby);
	screen.SetPlayerName(ID, localPlayer->name);
}


/************************************************************************************************/


void GameHostLobbyState::HandleNewConnection(const ConnectionHandle handle)
{
	auto newID = host.GetNewID();

	RequestClientDataPacket packet(newID);

	host.players.push_back({
					false,
					Joining,
					newID,
					handle,
					""});

	screen.CreateRow(newID);

	playerLobbyState.push_back({	
					newID, 
					false});
    
    host.network.Send(packet.header, handle);
}


/************************************************************************************************/


void GameHostLobbyState::HandleDisconnection(const ConnectionHandle handle)
{
    screen.ClearRows();

    auto disconnectedPlayer = host.GetPlayer(handle);

    playerLobbyState.remove_stable(
        find(   playerLobbyState,
                [&](PlayerLobbyEntry& e) -> bool { return e.ID == disconnectedPlayer->ID; }));
    
    host.RemovePlayer(disconnectedPlayer->ID);

    // Re-add LocalPlayer
    auto hostPlayer         = host.hostPlayer;
    const auto localPlayer  = host.GetPlayer(hostPlayer);
    screen.CreateRow(hostPlayer);
    screen.SetPlayerName(hostPlayer, localPlayer->name);
    
    // Other Players
    for (auto player : host.players)
        screen.CreateRow(player.ID);
}


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
