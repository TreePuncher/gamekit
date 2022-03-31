#ifndef MULTIPLAYERGAMESTATE_H
#define MULTIPLAYERGAMESTATE_H


#include "GameFramework.h"
#include "containers.h"
#include "Components.h"
#include "EngineCore.h"
#include "ResourceHandles.h"
#include "GameAssets.h"
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
    GameOver                    = GetCRCGUID(GameOver),
    PlayerJoin                  = GetCRCGUID(PlayerJoin),
	RequestPlayerList			= GetCRCGUID(RequestPlayerList),
	RequestPlayerListResponse	= GetCRCGUID(PlayerList),
    PlayerUpdate                = GetCRCGUID(PlayerUpdate),
    GameEvent                   = GetCRCGUID(GameEvent)
};


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
    UpdateTask*             update = nullptr;
    Vector<UpdateTask*>     drawTasks;
};


class WorldStateMangagerInterface
{
public:
    virtual ~WorldStateMangagerInterface() {}

    virtual WorldStateUpdate    Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT) = 0;
    virtual Vector<UpdateTask*> DrawTasks(EngineCore& core, UpdateDispatcher& dispatcher, double dT) = 0;

    virtual bool                EventHandler(Event evt) = 0;
    virtual CameraHandle        GetActiveCamera() const = 0;
    virtual Scene&              GetScene() = 0;

    virtual GameObject&         CreateGameObject() = 0;

    using GameEventHandler      = std::function<void (Event evt)>;
    virtual void                SetOnGameEventRecieved(GameEventHandler) = 0;
};

struct PlayerUpdatePacket
{
    UserPacketHeader        header{ sizeof(PlayerUpdatePacket), { PlayerUpdate } };

    MultiplayerPlayerID_t   playerID{ (uint64_t)-1 };
    PlayerFrameState        state{};
};


struct GameEventPacket
{
    UserPacketHeader        header{ sizeof(GameEventPacket), { GameEvent } };

    static_vector<Event>    events;
};


void UpdateLocalPlayer(GameObject& player, const PlayerInputState& currentInputState, double dT);
bool HandleEvents(PlayerInputState& keyState, Event evt);


/************************************************************************************************/



class LocalGameState : public FrameworkState
{
public:
    LocalGameState(GameFramework& IN_framework, WorldStateMangagerInterface& IN_worldState, BaseState& IN_base);
    ~LocalGameState();


    UpdateTask* Update  (EngineCore& core, UpdateDispatcher& dispatcher, double dT);
    UpdateTask* Draw    (UpdateTask* updateTask, EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph);

    struct particle_data
    {
        particle_data() = default;

        particle_data(InitialParticleProperties properties) :
            position    { properties.position },
            velocity    { properties.velocity },
            maxAge      { properties.lifespan }{}

        float3  position;
        float3  velocity;
        float   age     = 0.0f;
        float   maxAge  = 0.0f;

        static std::optional<particle_data> Update(particle_data particle, double dT)
        {
            if (particle.age + dT < particle.maxAge)
            {
                particle_data updated;
                updated.age         = particle.age + dT;
                updated.maxAge      = particle.maxAge;
                updated.position    = particle.position + particle.velocity * dT;
                updated.velocity    = particle.velocity;

                return updated;
            }
            else
                return {};
        }
    };

    ParticleSystem<particle_data>   testParticleSystem;
    ParticleEmitterComponent        emitters;

    void PostDrawUpdate (EngineCore& core, double dT) override;
    bool EventHandler   (Event evt) override;

    void OnGameEnd();

    RunOnceQueue                    runOnceDrawEvents;

    //Animation*                      testAnimationResource = nullptr;
    BaseState&                      base;
    WorldStateMangagerInterface&    worldState;

    bool                            mode        = false;
    bool                            move        = false;
    GameObject*                     pointLight1 = nullptr;
    GameObject*                     smolina     = nullptr;

    //GameObject&                     testAnimation;
    //GameObject&                     particleEmitter;
    //GameObject&                     IKTarget;

    //TriMeshHandle                   playerCharacterModel;
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
            base.depthBuffer.Get()
        };

        frameGraph.Resources.AddBackBuffer(targets.RenderTarget);
        frameGraph.Resources.AddDepthBuffer(targets.DepthTarget);

        ClearVertexBuffer   (frameGraph, base.vertexBuffer);
        ClearBackBuffer     (frameGraph, targets.RenderTarget, 0.0f);
        ClearDepthBuffer    (frameGraph, targets.DepthTarget, 1.0f);

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

Copyright (c) 2022 Robert May

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
