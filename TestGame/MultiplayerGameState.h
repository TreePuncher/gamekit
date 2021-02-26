#ifndef MULTIPLAYERGAMESTATE_H
#define MULTIPLAYERGAMESTATE_H


#include "GameFramework.h"
#include "containers.h"
#include "Components.h"
#include "EngineCore.h"
#include "ResourceHandles.h"

#include "BaseState.h"
#include "Gameplay.h"
#include "MultiplayerState.h"


/************************************************************************************************/


enum LobbyPacketIDs : PacketID_t
{
	_LobbyPacketID_Begin		= UserPacketIDCount,
	LoadGame					= GetCRCGUID(LoadGame),
    ClientGameLoaded            = GetCRCGUID(ClientGameLoaded),
    BeginGame                   = GetCRCGUID(BeginGame),
    PlayerJoin                  = GetCRCGUID(PlayerJoin),
	RequestPlayerList			= GetCRCGUID(RequestPlayerList),
	RequestPlayerListResponse	= GetCRCGUID(PlayerList),
    PlayerUpdate                = GetCRCGUID(PlayerUpdate),
};


/************************************************************************************************/


typedef uint64_t MultiplayerPlayerID_t;


inline MultiplayerPlayerID_t GeneratePlayerID()
{
    std::random_device generator;
    std::uniform_int_distribution<MultiplayerPlayerID_t> distribution(
        std::numeric_limits<MultiplayerPlayerID_t>::min(),
        std::numeric_limits<MultiplayerPlayerID_t>::max());

    return distribution(generator);
}


/************************************************************************************************/


struct GameInfo
{
    std::string name;
    std::string lobbyName;

    ConnectionHandle connectionHandle = InvalidHandle_t;
};


/************************************************************************************************/


struct SpellBookUpdatePacket
{
    SpellBookUpdatePacket(MultiplayerPlayerID_t IN_id) :
        header{ sizeof(SpellBookUpdatePacket), SpellbookUpdate },
        ID{ IN_id } {}

    UserPacketHeader        header;
    MultiplayerPlayerID_t   ID = 0;
    CardTypeID_t            spellBook[50] = {};
};


/************************************************************************************************/


struct WorldStateUpdate
{
    UpdateTask* update = nullptr;
};


class WorldStateMangagerInterface
{
public:
    virtual ~WorldStateMangagerInterface() {}

    virtual WorldStateUpdate    Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT) = 0;
    virtual bool                EventHandler(Event evt) = 0;
    virtual CameraHandle        GetActiveCamera() const = 0;
    virtual GraphicScene&       GetScene() = 0;
};


struct PlayerInputState
{
    float forward   = 0;
    float backward  = 0;
    float left      = 0;
    float right     = 0;
    float up        = 0;
    float down      = 0;

    enum class Event
    {
        Action1,
        Action2,
        Action3,
        Action4,
    };

    static_vector<Event> events;

    FlexKit::float2 mousedXY = { 0, 0 };
};


struct PlayerFrameState
{
    float3                      pos;
    float3                      velocity;
    Quaternion                  orientation;

    PlayerInputState            inputState;
    ThirdPersonCameraFrameState cameraState;
};


struct PlayerUpdatePacket
{
    UserPacketHeader        header{ sizeof(PlayerUpdatePacket), { PlayerUpdate } };

    MultiplayerPlayerID_t   playerID{ (uint64_t)-1 };
    PlayerFrameState        state{};
};


void UpdatePlayerState(GameObject& player, const PlayerInputState& currentInputState, double dT);
bool HandleEvents(PlayerInputState& keyState, Event evt);


enum class NetEventType
{

};

struct iNetEvent
{

};


struct LocalPlayerData
{
    CircularBuffer<PlayerFrameState>    inputHistory;
    Vector<iNetEvent>                   pendingEvent;
};


struct RemotePlayerData
{
    GameObject*             gameObject;
    ConnectionHandle        connection;
    MultiplayerPlayerID_t   ID;

    void Update(PlayerFrameState state)
    {
        SetWorldPosition(*gameObject, state.pos);
        SetOrientation(*gameObject, state.orientation);
    }

    PlayerFrameState    GetFrameState() const
    {
        PlayerFrameState out
        {
            .pos            = GetWorldPosition(*gameObject),
            .velocity       = { 0, 0, 0 },
            .orientation    = GetOrientation(*gameObject),
        };

        return out;
    }
};


inline static const ComponentID LocalPlayerComponentID  = GetTypeGUID(LocalPlayerData);

using LocalPlayerHandle     = Handle_t<32, LocalPlayerComponentID>;
using LocalPlayerComponent  = BasicComponent_t<LocalPlayerData, LocalPlayerHandle, LocalPlayerComponentID>;
using LocalPlayerView       = LocalPlayerComponent::View;

inline static const ComponentID RemotePlayerComponentID  = GetTypeGUID(RemotePlayerData);

using RemotePlayerHandle     = Handle_t<32, RemotePlayerComponentID>;
using RemotePlayerComponent  = BasicComponent_t<RemotePlayerData, RemotePlayerHandle, RemotePlayerComponentID>;
using RemotePlayerView       = RemotePlayerComponent::View;


/************************************************************************************************/


PlayerFrameState    GetPlayerFrameState(GameObject& gameObject);
RemotePlayerData*   FindPlayer(MultiplayerPlayerID_t ID, RemotePlayerComponent& players);


/************************************************************************************************/


class LocalGameState : public FrameworkState
{
public:
    LocalGameState(GameFramework& IN_framework, WorldStateMangagerInterface& IN_worldState, BaseState& IN_base);
    ~LocalGameState();


    UpdateTask* Update  (EngineCore& core, UpdateDispatcher& dispatcher, double dT);
    UpdateTask* Draw    (UpdateTask* updateTask, EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph);


    void PostDrawUpdate (EngineCore& core, double dT) override;
    bool EventHandler   (Event evt) override;

    BaseState&                      base;
    WorldStateMangagerInterface&    worldState;
};


/************************************************************************************************/


struct GameLoadScreenStateDefaultAction
{
    void operator()(EngineCore& core, UpdateDispatcher& dispatcher, double dT) {}
};


struct GameLoadScreenStateDefaultDraw
{
    void operator()(EngineCore& core, UpdateDispatcher& dispatcher, FrameGraph& frameGraph, double dT)
    {
    }
};



// Loads Scene and sends server notification on completion
template<
    typename TY_OnUpdate    = GameLoadScreenStateDefaultAction,
    typename TY_OnDraw      = GameLoadScreenStateDefaultDraw>
class GameLoadScreenState : public FrameworkState
{
public:
    GameLoadScreenState(
        GameFramework&  IN_framework,
        BaseState&      IN_base,
        TY_OnUpdate     IN_OnUpdate     = GameLoadScreenStateDefaultAction{},
        TY_OnDraw       IN_OnDraw       = GameLoadScreenStateDefaultDraw{}) :
            FrameworkState  { IN_framework      },
            base            { IN_base           },
            OnUpdate        { IN_OnUpdate       },
            OnDraw          { IN_OnDraw         } {}


    /************************************************************************************************/


    virtual ~GameLoadScreenState() override {}


    /************************************************************************************************/


    void Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT) final override
    {
        OnUpdate(core, dispatcher, dT);
    }


    void Draw(EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph) final override
    {
        WorldRender_Targets targets = {
            base.renderWindow.GetBackBuffer(),
            base.depthBuffer
        };

        frameGraph.Resources.AddBackBuffer(base.renderWindow.GetBackBuffer());
        frameGraph.Resources.AddDepthBuffer(base.depthBuffer);

        ClearVertexBuffer   (frameGraph, base.vertexBuffer);
        ClearBackBuffer     (frameGraph, targets.RenderTarget, 0.0f);
        ClearDepthBuffer    (frameGraph, base.depthBuffer, 1.0f);

        OnDraw(core, dispatcher, frameGraph, dT);

        PrintTextFormatting Format = PrintTextFormatting::DefaultParams();
        Format.Scale = { 1.0f, 1.0f };

        DrawSprite_Text(
            "Loading...",
            frameGraph,
            *framework.DefaultAssets.Font,
            base.vertexBuffer,
            base.renderWindow.GetBackBuffer(),
            core.GetTempMemory(),
            Format);

        framework.DrawDebugHUD(dT,  base.vertexBuffer, base.renderWindow, frameGraph);

        PresentBackBuffer(frameGraph, base.renderWindow);
    }

    void PostDrawUpdate(EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
    {
        base.PostDrawUpdate(core, dispatcher, dT, frameGraph);
    }



    /************************************************************************************************/

private:

    BaseState&    base;
    TY_OnUpdate   OnUpdate;
    TY_OnDraw     OnDraw;
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
