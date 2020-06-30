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

#include "MultiplayerState.h"
#include "Packets.h"
#include "LobbyGUI.h"


/************************************************************************************************/


class GameClientState;


struct ClientGameDescription
{
	short		port			= 1337;
	const char*	serverAddress	= "127.0.0.1"; // no place like home
	const char* name			= nullptr;
};


struct RemotePlayer
{
	MultiplayerPlayerID_t	id;
	char					name[32];
};


class ClientLobbyState : public FlexKit::FrameworkState
{
public:
	ClientLobbyState(
		GameFramework&		IN_framework,
        BaseState&          IN_base,
		GameClientState&	IN_client,
		NetworkState&		IN_network,
		const char*			IN_localPlayerName);


	~ClientLobbyState();


	void Update			(EngineCore& core, UpdateDispatcher& Dispatcher, double dT) final override;
	void Draw			(EngineCore& core, UpdateDispatcher& Dispatcher, double dT, FrameGraph&) final override;
	void PostDrawUpdate	(EngineCore& core, UpdateDispatcher& Dispatcher, double dT, FrameGraph&) final override;

	bool EventHandler	(Event evt) final override;

    BaseState&              base;
	size_t					refreshCounter;
	Vector<PacketHandler*>	packetHandlers;
	NetworkState&			network;
	GameClientState&		client;
	MultiplayerLobbyScreen	screen;

	bool					ready;
	const char*				localPlayerName;
};


/************************************************************************************************/


class GameClientState : public FlexKit::FrameworkState
{
public:
	GameClientState(
		GameFramework&          IN_framework,
		BaseState&				IN_base,
		NetworkState&			IN_network,
		ClientGameDescription	IN_desc = ClientGameDescription{}) :
			FrameworkState		{ IN_framework							},
			base				{ IN_base								},
			network				{ IN_network							},
			packetHandlers		{ IN_framework.core.GetBlockMemory()	},
			remotePlayers		{ IN_framework.core.GetBlockMemory()	}
	{
		char Address[256];
		if (!IN_desc.name)
		{
			std::cout << "Please Enter Name: \n";
			std::cin >> localName;
		}
		else 
			strncpy(localName, IN_desc.name, strnlen(IN_desc.name, 32) + 1);

		if (!IN_desc.serverAddress)
		{
			std::cout << "Please Address: \n";
			std::cin >> Address;
		}

		std::cout << "Connecting now\n";
        network.Accepted =
            [&](ConnectionHandle IN_server)
            {
                server = IN_server;
                JoinLobby();
            };

        network.Connect("127.0.0.1", 1337);
	}


	~GameClientState()
	{
        network.CloseConnection(server);

        Sleep(1000); // IDK, does it need time to send the packet?

		for (auto handler : packetHandlers)
			framework.core.GetBlockMemory().free(handler);
	}


    void StartGame()
    {
        FK_LOG_INFO("Recieved game start from host. Starting Game!");

        struct LoadState
        {
            LoadState(iAllocator* allocator) : handler{ allocator } 
            {
                handler.emplace_back(
                    CreatePacketHandler(
                        BeginGame,
                        [&]( UserPacketHeader*   header,
                            Packet*             packet,
                            NetworkState*       network)
                        {
                            FK_ASSERT(header  != nullptr);
                            FK_ASSERT(packet  != nullptr);
                            FK_ASSERT(network != nullptr);

                            beginLoad = true;
                        }, allocator));
            }

            PacketHandlerVector handler;
            bool                beginLoad   = false;

            operator PacketHandlerVector& ()    { return handler;   }
            operator bool()                     { return beginLoad; }
        };

        auto& allocator     = framework.core.GetBlockMemory();
        auto& waiting       = allocator.allocate<LoadState>(allocator);

        auto OnCompletion   = [&](auto& core, auto& Dispatcher, double dT)
        {
            auto& gameState     = framework.PushState<GameState>(base);
            auto& localState    = framework.PushState<LocalPlayerState>(base, gameState);

            // if we don't update, these states will miss their first frame update
            gameState.Update(core, Dispatcher, dT);
            localState.Update(core, Dispatcher, dT);

            network.PopHandler();
            framework.core.GetBlockMemory().release_allocation(waiting);
        };

        auto OnUpdate = [&](auto& core, auto& dispatcher, double dt) -> bool
        {
            return waiting;
        };

        framework.PopState(); // pop lobby state
        network.PushHandler(waiting);

		auto& gameState     = framework.PushState<GameState>(base);
        /*
        auto& localState    =
            framework.PushState
                <GameLoadSceneState<decltype(OnCompletion),decltype(OnUpdate)>>
                (base, gameState.scene, "MultiplayerTestLevel", OnCompletion, OnUpdate);
                */
	}


    void JoinLobby()
    {
		framework.PushState<ClientLobbyState>(base, *this, network, localName);
    }

	void ServerLost()
	{
		FK_LOG_INFO("Server lost, quitting!");
		framework.quit = true;
	}


	bool Update(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT)
	{
		// Handle Packets

		return true;
	}

	Vector<RemotePlayer>	remotePlayers;
	Vector<PacketHandler*>	packetHandlers;
	NetworkState&			network;
	BaseState&				base;
    ConnectionHandle        server;

	MultiplayerPlayerID_t	localID;
	char					localName[32];
};


/************************************************************************************************/
