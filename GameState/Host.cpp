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

#include "Host.h"
#include <random>
#include <limits>

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


bool GameHostLobbyState::Update(EngineCore* core, UpdateDispatcher& Dispatcher, double dT)
{
	for (auto player : playerLobbyState)
	{
		auto playerHostState = host->GetPlayer(player.ID);
		if (playerHostState->State == Lobby)
		{
			screen.SetPlayerName(player.ID, playerHostState->Name);
			screen.SetPlayerReady(player.ID, player.Ready);
		}
	}

	FlexKit::WindowInput windowInput;
	windowInput.CursorWH = { 0.05f, 0.05f };
	windowInput.MousePosition = Framework->MouseState.NormalizedScreenCord;
	windowInput.LeftMouseButtonPressed = Framework->MouseState.LMB_Pressed;

	screen.Update(dT, windowInput, GetPixelSize(&core->Window), core->GetTempMemory());

	return true;
}


/************************************************************************************************/


bool GameHostLobbyState::Draw(EngineCore* core, UpdateDispatcher& Dispatcher, double dT, FrameGraph& frameGraph)
{
	auto currentRenderTarget = GetCurrentBackBuffer(&core->Window);

	ClearVertexBuffer	(frameGraph, host->base->vertexBuffer);
	ClearVertexBuffer	(frameGraph, host->base->textBuffer);
	ClearBackBuffer		(frameGraph, { 0.0f, 0.0f, 0.0f, 0.0f });

	LobbyScreenDrawDesc Desc;
	Desc.allocator			= core->GetTempMemory();
	Desc.constantBuffer		= host->base->constantBuffer;
	Desc.vertexBuffer		= host->base->vertexBuffer;
	Desc.textBuffer			= host->base->textBuffer;
	Desc.renderTarget		= currentRenderTarget;
	screen.Draw(Desc, Dispatcher, frameGraph);

	FlexKit::DrawMouseCursor(
		Framework->MouseState.NormalizedScreenCord,
		{ 0.05f, 0.05f },
		host->base->vertexBuffer,
		host->base->constantBuffer,
		currentRenderTarget,
		core->GetTempMemory(),
		&frameGraph);

	return true;
}


/************************************************************************************************/


bool GameHostLobbyState::PostDrawUpdate(EngineCore* Core, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph)
{
	if (Framework->DrawDebugStats)
		Framework->DrawDebugHUD(dT, host->base->textBuffer, Graph);

	PresentBackBuffer(Graph, &Core->Window);
	return true;
}


/************************************************************************************************/


GameHostLobbyState::GameHostLobbyState(
	FlexKit::GameFramework* IN_framework,
	GameHostState*			IN_host) :
		FrameworkState		{IN_framework}, 
		packetHandlers		{IN_framework->Core->GetBlockMemory()},
		host				{IN_host},
		playerLobbyState	{IN_framework->Core->GetBlockMemory()},
		screen				{IN_framework->Core->GetBlockMemory(), IN_framework->DefaultAssets.Font}
{
	IN_host->network->NewConnectionHandler = [this](RakNet::Packet* packet) {
		HandleNewConnection(packet); 
	};

	packetHandlers.push_back(
		CreatePacketHandler(
			ClientDataRequestResponse,
			[&](UserPacketHeader* packetContents, RakNet::Packet* incomingPacket, NetworkState* network)
			{
				ClientDataPacket* ClientData = (ClientDataPacket*)packetContents;

				if (host->GetPlayer(ClientData->playerID))
				{
					host->SetState(ClientData->playerID, Lobby);
					host->SetPlayerName(
						ClientData->playerID, 
						ClientData->playerName, 
						ClientData->playerNameLength);

					FK_LOG_9("Player: %s joined game", ClientData->playerName);
				}
			}, 
			IN_framework->Core->GetBlockMemory()));


	packetHandlers.push_back(
		CreatePacketHandler(
			ClientReadyEvent,
			[&](UserPacketHeader* packetContents, RakNet::Packet* incomingPacket, NetworkState* network)
			{
				FK_LOG_INFO("ready packet recieved!");

				ClientReady* readyPacket = (ClientReady*)packetContents;
				auto playerState = host->GetPlayer(readyPacket->playerID);

				if (playerState)
				{
					for (auto& playerLobby : playerLobbyState)
					{
						if (playerLobby.ID == readyPacket->playerID)
						{
							playerLobby.Ready = readyPacket->ready;

#ifdef _DEBUG
							if (readyPacket->ready)
								FK_LOG_9("Player: %s is now ready", playerState->Name);
							else
								FK_LOG_9("Player: %s is now not ready", playerState->Name);
#endif
						}
					}
				}
			}, 
			IN_framework->Core->GetBlockMemory()));

	packetHandlers.push_back(
		CreatePacketHandler(
			RequestPlayerList,
			[&](UserPacketHeader* packetContents, RakNet::Packet* incomingPacket, NetworkState* network)
			{
				FK_LOG_9("Player list requested");

				auto request				= (RequestPlayerListPacket*)(packetContents);
				auto packetSize				= PlayerListPacket::GetPacketSize(playerLobbyState.size() - 1);
				auto packetBuffer			= Framework->Core->GetTempMemory()._aligned_malloc(packetSize);
				PlayerListPacket* newPacket = new(packetBuffer)	PlayerListPacket(request->playerID, playerLobbyState.size() - 1);

				size_t idx = 0;
				for (auto& playerState : playerLobbyState)
				{
					if (playerState.ID == newPacket->playerID)
						continue;

					auto player = host->GetPlayer(playerState.ID);

					newPacket->Players[idx].playerID	= playerState.ID;
					newPacket->Players[idx].ready		= playerState.Ready;

					if (player)
						strncpy(
							newPacket->Players[idx].playerName, 
							player->Name, 
							sizeof(PlayerListPacket::entry::playerName));

					idx++;
				}

				host->network->SendPacket(newPacket->GetRawPacket(), incomingPacket->systemAddress);
			}, 
			IN_framework->Core->GetBlockMemory()));

	host->network->PushHandler(&packetHandlers);
}


/************************************************************************************************/


GameHostLobbyState::~GameHostLobbyState()
{
	host->network->PopHandler();
	for (auto handler : packetHandlers)
		Framework->Core->GetBlockMemory().free(handler);
}


/************************************************************************************************/


void GameHostLobbyState::HandleNewConnection(RakNet::Packet* packet)
{
	auto newID = host->GetNewID();

	RequestClientDataPacket Packet(newID);

	host->players.push_back({
					false,
					Joining,
					newID,
					packet->systemAddress,
					""});

	screen.CreateRow(newID);

	playerLobbyState.push_back({	
					newID, 
					false});

	host->network->SendPacket(Packet.GetRawPacket(), packet->systemAddress);
}


/************************************************************************************************/