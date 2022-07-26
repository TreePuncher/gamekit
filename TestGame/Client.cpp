#include "Client.h"
#include "MathUtils.h"


/************************************************************************************************/


ClientWorldStateMangager::ClientWorldStateMangager(ConnectionHandle IN_server, MultiplayerPlayerID_t IN_playerID, NetworkState& IN_net, BaseState& IN_base) :
	net                     { IN_net    },
	base                    { IN_base   },

	ID                      { IN_playerID   },
	server                  { IN_server     },

	pendingRemoteUpdates    { IN_base.framework.core.GetBlockMemory() },

	gadgetComponent         { IN_base.framework.core.GetBlockMemory() },
	playerComponent         { IN_base.framework.core.GetBlockMemory() },
	localPlayerComponent    { IN_base.framework.core.GetBlockMemory() },
	remotePlayerComponent   { IN_base.framework.core.GetBlockMemory() },

	eventMap                { IN_base.framework.core.GetBlockMemory() },
	packetHandlers          { IN_base.framework.core.GetBlockMemory() },

	world                   { IN_base.framework.core },
	localPlayer             { world.AddLocalPlayer(IN_playerID) }
{
	auto& allocator = IN_base.framework.core.GetBlockMemory();

	packetHandlers.push_back(
		CreatePacketHandler(
			PlayerUpdate,
			[&](UserPacketHeader* header, Packet* packet, NetworkState* network)
			{
				auto* updatePacket = reinterpret_cast<PlayerUpdatePacket*>(header);

				pendingRemoteUpdates.push_back(updatePacket->state);
			},
			allocator));

	net.PushHandler(packetHandlers);

	eventMap.MapKeyToEvent(KEYCODES::KC_W, TPC_MoveForward);
	eventMap.MapKeyToEvent(KEYCODES::KC_S, TPC_MoveBackward);

	eventMap.MapKeyToEvent(KEYCODES::KC_A, TPC_MoveLeft);
	eventMap.MapKeyToEvent(KEYCODES::KC_D, TPC_MoveRight);
	eventMap.MapKeyToEvent(KEYCODES::KC_Q, TPC_MoveDown);
	eventMap.MapKeyToEvent(KEYCODES::KC_E, TPC_MoveUp);

	eventMap.MapKeyToEvent(KEYCODES::KC_1, PLAYER_ACTION1);
	eventMap.MapKeyToEvent(KEYCODES::KC_2, PLAYER_ACTION2);
	eventMap.MapKeyToEvent(KEYCODES::KC_3, PLAYER_ACTION3);
	eventMap.MapKeyToEvent(KEYCODES::KC_4, PLAYER_ACTION4);

	localPlayer.AddView<LocalPlayerView>();

	static const GUID_t sceneID = 1234;

	auto& visibility = SceneVisibilityComponent::GetComponent();

	SetControllerPosition(localPlayer, { 30, 10, -30 });

	const auto worldAssets = LoadBasicTiles();
	CreateMultiplayerScene(world, worldAssets, IN_base.framework.core.GetBlockMemory(), IN_base.framework.core.GetTempMemory());
}


/************************************************************************************************/


ClientWorldStateMangager::~ClientWorldStateMangager(){}


/************************************************************************************************/


void ClientWorldStateMangager::AddRemotePlayer(MultiplayerPlayerID_t playerID)
{
	world.AddRemotePlayer(playerID);
}


/************************************************************************************************/



WorldStateUpdate ClientWorldStateMangager::Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
{
	ProfileFunction();

	net.Update(core, dispatcher, dT);

	struct _ { };

	WorldStateUpdate out;

	auto& worldUpdate =
		dispatcher.Add<_>(
			[](UpdateDispatcher::UpdateBuilder& Builder, auto& data)
			{
			},
			[&, dT = dT](auto& data, iAllocator& threadAllocator)
			{
				ProfileFunction();

				fixedUpdate(dT,
					[this, dT = dT](auto dT)
					{
						ProfileFunction();

						// Send Player Updates
						PlayerFrameState localState = GetPlayerFrameState(localPlayer);
						currentInputState.mousedXY  = base.renderWindow.mouseState.Normalized_dPos;
						auto leftStick  = base.gamepads.GetLeftJoyStick();
						auto rightStick = base.gamepads.GetRightJoyStick();

						currentInputState.X = leftStick.x;
						currentInputState.Y = leftStick.y;

						localState.inputState       = currentInputState;

						UpdateLocalPlayer(localPlayer, currentInputState, dT);
						world.UpdatePlayer(localState, dT);

						for (auto& playerUpdate : pendingRemoteUpdates)
							world.UpdateRemotePlayer(playerUpdate, dT);

						// Process Player events
						for (auto& player : playerComponent)
						{
							for (auto& evt : player.componentData.playerEvents)
							{
								switch (evt.mData1.mINT[0])
								{
								case (int)PlayerEvents::PlayerDeath:
								{
									static_vector<Event> events{ evt };
									BroadcastEvent(events);

									gameEventHandler(evt); // broadcast evt globally
								}   break;
								}
							}

							player.componentData.playerEvents.clear();
						}

						SendFrameState(ID, localState, server);

						pendingRemoteUpdates.clear();
						currentInputState.events.clear();
					});
			}
		);

	auto& physicsUpdate     = base.physics.Update(dispatcher, dT);
	auto& spellUpdate       = world.UpdateGadgets(dispatcher, world.objectPool, dT);
	auto& animationUpdate   = UpdateAnimations(dispatcher, dT);

	spellUpdate.AddInput(worldUpdate);
	physicsUpdate.AddInput(worldUpdate);
	physicsUpdate.AddInput(spellUpdate);

	out.update = &physicsUpdate;

	return out;
}


/************************************************************************************************/


void ClientWorldStateMangager::SendFrameState(const MultiplayerPlayerID_t ID, const PlayerFrameState& state, const ConnectionHandle connection)
{
	PlayerUpdatePacket packet;
	packet.playerID = ID;
	packet.state    = state;

	net.Send(packet.header, connection);
}


/************************************************************************************************/


bool ClientWorldStateMangager::EventHandler(Event evt)
{
	ProfileFunction();

	return eventMap.Handle(evt,
			[&](auto& evt) -> bool
			{
				return HandleEvents(currentInputState, evt);
			});
}


/************************************************************************************************/


Scene& ClientWorldStateMangager::GetScene()
{
	return world.scene;
}


/************************************************************************************************/


LayerHandle ClientWorldStateMangager::GetLayer()
{
	return world.layer;
}


/************************************************************************************************/


CameraHandle ClientWorldStateMangager::GetActiveCamera() const 
{
	return GetCameraControllerCamera(localPlayer);
}


/************************************************************************************************/


ClientState::ClientState(
	GameFramework&          framework,
	MultiplayerPlayerID_t   clientID,
	ConnectionHandle        server,
	BaseState&              IN_base,
	NetworkState&           IN_net) :
			FrameworkState  { framework },
			clientID        { clientID  },
			server          { server    },
			net             { IN_net    },
			base            { IN_base   },
			packetHandlers  { framework.core.GetBlockMemory() }
{
	packetHandlers.push_back(
		CreatePacketHandler(
			LobbyMessage,
			[&](UserPacketHeader* header, Packet* packet, NetworkState* network)
			{
				LobbyMessagePacket* pckt = reinterpret_cast<LobbyMessagePacket*>(header);
				std::string msg = pckt->message;

				OnMessageRecieved(msg);
			},
			framework.core.GetBlockMemory()
		));

	packetHandlers.push_back(
		CreatePacketHandler(
			PlayerJoin,
			[&](UserPacketHeader* header, Packet* packet, NetworkState* network)
			{
				auto pckt               = reinterpret_cast<PlayerJoinEventPacket*>(header);
				std::string playerName  = pckt->PlayerName;

				Player p = {
					.name   = playerName,
					.ID     = pckt->playerID,
				};

				peers.push_back(p);
				OnPlayerJoin(peers.back());
			},
			framework.core.GetBlockMemory()
		));

	packetHandlers.push_back(
		CreatePacketHandler(
			BeginGame,
			[&](UserPacketHeader* header, Packet* packet, NetworkState* network)
			{
				OnGameStart();
			},
			framework.core.GetBlockMemory()
		));

	packetHandlers.push_back(
		CreatePacketHandler(
			RequestPlayerListResponse,
			[&](UserPacketHeader* header, Packet* packet, NetworkState* network)
			{
				FK_LOG_INFO("Player List Recieved!");

				auto playerList = static_cast<PlayerListPacket*>(header);

				const auto playerCount = playerList->playerCount;
				for (uint32_t I = 0; I < playerCount; ++I)
				{
					Player player;
					player.name = playerList->Players[I].name;
					player.ID   = playerList->Players[I].ID;

					if(player.ID != clientID)
						peers.push_back(player);
				}
				
			},
			framework.core.GetBlockMemory()
				));

	net.PushHandler(packetHandlers);
}


/************************************************************************************************/


UpdateTask* ClientState::Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
{
	return net.Update(core, dispatcher, dT);
}


/************************************************************************************************/


void ClientState::SendChatMessage(std::string msg)
{
	LobbyMessagePacket packet(clientID, msg);

	net.Send(packet.Header, server);
}


/************************************************************************************************/


void ClientState::RequestPlayerList()
{
	RequestPlayerListPacket packet{ clientID };

	net.Send(packet.Header, server);
}


/************************************************************************************************/


void ClientState::SendGadgetUpdate(std::vector<GadgetInterface*>& spellbook)
{
	SpellBookUpdatePacket packet{ clientID };

	for (size_t I = 0; I < spellbook.size(); ++I)
		packet.spellBook[I] = spellbook[I]->CardID;

	net.Send(packet.header, server);
}


/************************************************************************************************/


UpdateTask* ClientState::Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{
	auto renderTarget = base.renderWindow.GetBackBuffer();

	ClearVertexBuffer(frameGraph, base.vertexBuffer);
	ClearBackBuffer(frameGraph, renderTarget, 0.0f);

	auto reserveVB = CreateVertexBufferReserveObject(base.vertexBuffer, core.RenderSystem, core.GetTempMemory());
	auto reserveCB = CreateConstantBufferReserveObject(base.constantBuffer, core.RenderSystem, core.GetTempMemory());

	ImGui::Render();
	base.debugUI.DrawImGui(dT, dispatcher, frameGraph, reserveVB, reserveCB, renderTarget);

	FlexKit::PresentBackBuffer(frameGraph, renderTarget);

	return nullptr;
}


/************************************************************************************************/


void ClientState::PostDrawUpdate(EngineCore&, double dT)
{

}


/************************************************************************************************/


void PushClientState(const MultiplayerPlayerID_t playerID, const ConnectionHandle server, BaseState& base, NetworkState& net)
{
	auto& framework = base.framework;
	auto& client    = framework.PushState<ClientState>(playerID, server, base, net);
	auto& lobby     = framework.PushState<LobbyState>(base, net);

	client.OnMessageRecieved    = [&](std::string msg) { lobby.MessageRecieved(msg); };
	client.OnPlayerJoin         =
		[&](ClientState::Player& player)
		{
			LobbyState::Player lobbyPlayer = {
				.Name   = player.name,
				.ID     = player.ID,
			};

			lobby.players.push_back(lobbyPlayer);
			lobby.chatHistory += "Player Joined\n";
		};

	client.OnGameStart =
		[&]()
		{
			framework.PopState();

			AddAssetFile("assets\\multiplayerAssets.gameres");

			auto& worldState = framework.core.GetBlockMemory().allocate<ClientWorldStateMangager>(client.server, client.clientID, net, base);
			auto& localState = framework.PushState<LocalGameState>(worldState, base);

			net.HandleDisconnection = [&](ConnectionHandle connection) { framework.quit = true; };

			for (auto& player : client.peers)
				if(player.ID != client.clientID)
					worldState.AddRemotePlayer(player.ID);
		};

	lobby.OnSendMessage         = [&](std::string msg) { client.SendChatMessage(msg); };
	lobby.GetPlayer             =
		[&](uint idx) -> LobbyState::Player
		{
			auto& peer = client.peers[idx];
						
			LobbyState::Player out{
				.Name   = peer.name,
				.ID     = peer.ID,
			};

			return out;
		};

	lobby.GetPlayerCount        = [&](){ return (uint)client.peers.size(); };

	client.RequestPlayerList();
}


/************************************************************************************************/
