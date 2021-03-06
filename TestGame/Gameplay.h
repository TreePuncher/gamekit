#pragma once


/************************************************************************************************/


#include "containers.h"
#include "Components.h"
#include "Events.h"
#include "MathUtils.h"
#include "GameFramework.h"
#include "FrameGraph.h"
#include "Particles.h"
#include "physicsutilities.h"

#include <regex>


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


using FlexKit::Event;
using FlexKit::FrameGraph;
using FlexKit::GameFramework;

using FlexKit::iAllocator;
using FlexKit::CameraHandle;
using FlexKit::ConstantBufferHandle;
using FlexKit::Handle_t;
using FlexKit::VertexBufferHandle;
using FlexKit::ResourceHandle;
using FlexKit::static_vector;

using FlexKit::CameraView;

using FlexKit::int2;
using FlexKit::uint2;
using FlexKit::float2;
using FlexKit::float3;
using FlexKit::float4;
using FlexKit::Quaternion;



/************************************************************************************************/


enum PLAYER_INPUT_EVENTS : int64_t
{
	PLAYER_UP				= GetCRCGUID(PLAYER_UP),
	PLAYER_LEFT				= GetCRCGUID(PLAYER_LEFT),
	PLAYER_DOWN				= GetCRCGUID(PLAYER_DOWN),
	PLAYER_RIGHT			= GetCRCGUID(PLAYER_RIGHT),

	PLAYER_ACTION1			= GetTypeGUID(PLAYER_ACTION1),
	PLAYER_ACTION2			= GetTypeGUID(PLAYER_ACTION2),
	PLAYER_ACTION3			= GetTypeGUID(PLAYER_ACTION3),
	PLAYER_ACTION4			= GetTypeGUID(PLAYER_ACTION4),

	PLAYER_HOLD				= GetCRCGUID(PLAYER_HOLD),

	PLAYER_ROTATE_LEFT		= GetCRCGUID(PLAYER_ROTATE_LEFT),
	PLAYER_ROTATE_RIGHT		= GetCRCGUID(PLAYER_ROTATE_RIGHT),

	DEBUG_PLAYER_UP			= GetCRCGUID(DEBUG_PLAYER_UP),
	DEBUG_PLAYER_LEFT		= GetCRCGUID(DEBUG_PLAYER_LEFT),
	DEBUG_PLAYER_DOWN		= GetCRCGUID(DEBUG_PLAYER_DOWN),
	DEBUG_PLAYER_RIGHT		= GetCRCGUID(DEBUG_PLAYER_RIGHT),
	DEBUG_PLAYER_ACTION1	= GetCRCGUID(DEBUG_PLAYER_ACTION1),
	DEBUG_PLAYER_HOLD		= GetCRCGUID(DEBUG_PLAYER_HOLD),

	PLAYER_UNKNOWN,
};

enum class PlayerEvents
{
    PlayerDeath,
    PlayerHit,
};

enum DEBUG_EVENTS : int64_t
{
	TOGGLE_DEBUG_CAMERA  = GetCRCGUID(TOGGLE_DEBUG_CAMERA),
	TOGGLE_DEBUG_OVERLAY = GetCRCGUID(TOGGLE_DEBUG_OVERLAY)
};


/************************************************************************************************/


using CardTypeID_t = uint32_t;

class GameWorld;

struct CardInterface
{
    CardTypeID_t  CardID                = (CardTypeID_t)-1;
    const char*   cardName              = "!!!!!";
    const char*   description           = "!!!!!";

    uint          requiredPowerLevel    = 0;

    std::function<void (GameWorld& world, GameObject& player)> OnCast   = [](GameWorld& world, GameObject& player){};
    std::function<void (GameWorld& world, GameObject& player)> OnHit    = [](GameWorld& world, GameObject& player){};
};


struct PowerCard : public CardInterface
{
    PowerCard();

    static CardTypeID_t ID() { return GetTypeGUID(PowerCard); };
};


struct FireBall : public CardInterface
{
    FireBall();

    static CardTypeID_t ID() { return GetTypeGUID(FireBall); };
};


struct PlayerDesc
{
    MultiplayerPlayerID_t   player;
    PhysXSceneHandle        pscene;
    GraphicScene&           gscene;  

    float h = 1.0f;
    float r = 1.0f;
};


struct SpellData
{
    GameObject*             gameObject;

    uint32_t                caster;
    CardInterface           card;

    float                   life;
    float                   duration;

    float3                  position;
    float3                  velocity;
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
    MultiplayerPlayerID_t       player;
    float3                      pos;
    float3                      velocity;
    float3                      forwardVector;
    Quaternion                  orientation;

    PlayerInputState            inputState;
    ThirdPersonCameraFrameState cameraState;
};


struct PlayerState
{
    GameObject*             gameObject;
    MultiplayerPlayerID_t   playerID;
    float                   playerHealth    = 100.0f;
    uint32_t                maxCastingLevel = 1;
    float                   mana            = 0.0f;

    float3                  forward;
    float3                  position;

    void ApplyDamage(float damage)
    {
        playerHealth -= damage;

        if (playerHealth > 0.0f)
        {
            FlexKit::Event evt;
            evt.InputSource       = Event::InputType::Local;
            evt.mType             = Event::EventType::iObject;
            evt.mData1.mINT[0]    = (int)PlayerEvents::PlayerHit;

            playerEvents.push_back(evt);
        }
        else if(playerHealth <= 0.0f)
        {
            FlexKit::Event evt;
            evt.InputSource       = Event::InputType::Local;
            evt.mType             = Event::EventType::iObject;
            evt.mData1.mINT[0]    = (int)PlayerEvents::PlayerDeath;

            playerEvents.push_back(evt);
        }
    }

    static_vector<FlexKit::Event> playerEvents;
};

inline static const ComponentID PlayerComponentID = GetTypeGUID(PlayerGameComponentID);

using PlayerHandle      = Handle_t<32, PlayerComponentID>;
using PlayerComponent   = BasicComponent_t<PlayerState, PlayerHandle, PlayerComponentID>;
using PlayerView        = PlayerComponent::View;


struct LocalPlayerData
{
    PlayerHandle                        playerGameState;
    MultiplayerPlayerID_t               playerID;
    CircularBuffer<PlayerFrameState>    inputHistory;
    //Vector<iNetEvent>                   pendingEvent;
};

struct RemotePlayerData
{
    GameObject*             gameObject;
    PlayerHandle            playerGameState;
    ConnectionHandle        connection;
    MultiplayerPlayerID_t   playerID;

    void Update(PlayerFrameState state)
    {
        SetControllerPosition(*gameObject, state.pos);
        SetControllerOrientation(*gameObject, state.orientation);

        Apply(
            *gameObject,
            [&](PlayerView& player)
            {
                player->forward     = state.forwardVector;
                player->position    = GetControllerPosition(*gameObject);
            });
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

using RemotePlayerHandle     = Handle_t<32, RemotePlayerComponentID>; // this is a handle to an instance
using RemotePlayerComponent  = BasicComponent_t<RemotePlayerData, RemotePlayerHandle, RemotePlayerComponentID>; // This defines a new component
using RemotePlayerView       = RemotePlayerComponent::View; // This defines an interface to access data in the component in a easy manner 

inline static const ComponentID SpellComponentID = GetTypeGUID(SpellComponent);

using SpellHandle       = Handle_t<32, SpellComponentID>;
using SpellComponent    = BasicComponent_t<SpellData, SpellHandle, SpellComponentID>;
using SpellView         = SpellComponent::View;


class GameWorld
{
public:
    GameWorld(EngineCore& IN_core) :
        allocator       { static_cast<iAllocator&>(IN_core.GetBlockMemory()) },
        core            { IN_core },
        renderSystem    { IN_core.RenderSystem},
        objectPool      { IN_core.GetBlockMemory(), 8096 },
        pscene          { PhysXComponent::GetComponent().CreateScene() },
        gscene          { IN_core.GetBlockMemory() },
        cubeShape       { PhysXComponent::GetComponent().CreateCubeShape({ 0.5f, 0.5f, 0.5f}) } {}

    ~GameWorld()
    {
        gscene.ClearScene();
    }



    GameObject& AddLocalPlayer(MultiplayerPlayerID_t multiplayerID);
    GameObject& AddRemotePlayer(MultiplayerPlayerID_t playerID, ConnectionHandle connection = InvalidHandle_t);
    void        AddCube(float3 POS);

    GameObject& CreatePlayer(const PlayerDesc& desc);
    GameObject& CreateSpell(SpellData initial, float3 initialPosition, float3 initialVelocity);
    bool        LoadScene(GUID_t assetID);

    void        UpdatePlayer        (const PlayerFrameState& playerState, const double);
    void        UpdateRemotePlayer  (const PlayerFrameState& playerState, const double);
    UpdateTask& UpdateSpells(UpdateDispatcher& dispathcer, ObjectPool<GameObject>& objectPool, const double dt);


    struct GameEvent
    {

    };


    Vector<GameEvent>       pendingEvents;

    const GUID_t            playerModel = 7896;
    const GUID_t            cube1X1X1   = 7895;
    const GUID_t            spellModel  = 7894;
    const PxShapeHandle     cubeShape;

    EngineCore&             core;
    RenderSystem&           renderSystem;

    GraphicScene            gscene;
    PhysXSceneHandle        pscene;

    ObjectPool<GameObject>  objectPool;
    iAllocator&             allocator;
};


/************************************************************************************************/


void CreateMultiplayerScene(GameWorld&);

PlayerFrameState    GetPlayerFrameState (GameObject& gameObject);
RemotePlayerData*   FindRemotePlayer    (MultiplayerPlayerID_t ID);
GameObject*         FindPlayer          (MultiplayerPlayerID_t ID);


/************************************************************************************************/


/**********************************************************************

Copyright (c) 2021 Robert May

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

