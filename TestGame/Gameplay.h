#pragma once


/************************************************************************************************/


#include "containers.h"
#include "Components.h"
#include "Events.h"
#include "MathUtils.h"
#include "GameFramework.h"
#include "FrameGraph.h"
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


enum PLAYER_EVENTS : int64_t
{
	PLAYER_UP				= GetCRCGUID(PLAYER_UP),
	PLAYER_LEFT				= GetCRCGUID(PLAYER_LEFT),
	PLAYER_DOWN				= GetCRCGUID(PLAYER_DOWN),
	PLAYER_RIGHT			= GetCRCGUID(PLAYER_RIGHT),

	PLAYER_ACTION1			= GetTypeGUID(A123CTION11),
	PLAYER_ACTION2			= GetTypeGUID(AC3TION12),
	PLAYER_ACTION3			= GetTypeGUID(A11123CTION13),
	PLAYER_ACTION4			= GetTypeGUID(AC123TION11),

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


enum DEBUG_EVENTS : int64_t
{
	TOGGLE_DEBUG_CAMERA  = GetCRCGUID(TOGGLE_DEBUG_CAMERA),
	TOGGLE_DEBUG_OVERLAY = GetCRCGUID(TOGGLE_DEBUG_OVERLAY)
};


/************************************************************************************************/


using CardTypeID_t = uint32_t;

struct CardInterface
{
    CardTypeID_t  CardID      = (CardTypeID_t)-1;
    const char*   cardName    = "!!!!!";
    const char*   description = "!!!!!";

    uint          requiredPowerLevel = 0;
};


struct PowerCard : public CardInterface
{
    PowerCard() : CardInterface{
        GetTypeGUID(PowerCard),
        "PowerCard",
        "Use to increase current level, allows for more powerful spell casting" } {}
};


struct FireBall : public CardInterface
{
    FireBall() : CardInterface{
        ID(),
        "FireBall",
        "Throws a small ball a fire, burns on contact.\n"
        "Required casting level is 1" } {}

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


inline static const ComponentID SpellComponentID  = GetTypeGUID(SpellComponent);

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


struct LocalPlayerData
{
    MultiplayerPlayerID_t               player;
    CircularBuffer<PlayerFrameState>    inputHistory;
    //Vector<iNetEvent>                   pendingEvent;
};


struct RemotePlayerData
{
    GameObject*             gameObject;
    ConnectionHandle        connection;
    MultiplayerPlayerID_t   ID;

    void Update(PlayerFrameState state)
    {
        SetControllerPosition(*gameObject, state.pos);
        SetControllerOrientation(*gameObject, state.orientation);
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

using SpellHandle       = Handle_t<32, SpellComponentID>;
using SpellComponent    = BasicComponent_t<SpellData, SpellHandle, SpellComponentID>;
using SpellView         = SpellComponent::View;

GameObject& CreatePlayer(const PlayerDesc& desc, RenderSystem& renderSystem, iAllocator& allocator);

class GameWorld
{
public:
    GameWorld(EngineCore& IN_core) :
        core            { IN_core },
        renderSystem    { IN_core.RenderSystem},
        objectPool      { IN_core.GetBlockMemory(), 8096 },
        pscene          { PhysXComponent::GetComponent().CreateScene() },
        gscene          { IN_core.GetBlockMemory() },
        allocator       { static_cast<iAllocator&>(IN_core.GetBlockMemory()) },
        cubeShape       { PhysXComponent::GetComponent().CreateCubeShape({ 0.5f, 0.5f, 0.5f}) } {}

    ~GameWorld()
    {
        gscene.ClearScene();
    }


    GameObject& AddLocalPlayer(MultiplayerPlayerID_t multiplayerID)
    {
        return CreatePlayer(
                    PlayerDesc{
                        .player = multiplayerID,
                        .pscene = pscene,
                        .gscene = gscene,
                        .h      = 0.5f,
                        .r      = 0.5f
                    },
                    renderSystem,
                    allocator);
    }

    GameObject& AddRemotePlayer(MultiplayerPlayerID_t playerID)
    {
        auto& gameObject = objectPool.Allocate();

        gameObject.AddView<RemotePlayerView>(RemotePlayerData{ &gameObject, InvalidHandle_t, playerID });

        auto [triMesh, loaded] = FindMesh(playerModel);

        if (!loaded)
            triMesh = LoadTriMeshIntoTable(renderSystem, renderSystem.GetImmediateUploadQueue(), playerModel);

        auto& characterController = gameObject.AddView<CharacterControllerView>(pscene, float3{0, 0, 0}, GetZeroedNode(), 1.0f, 1.0f);
        gameObject.AddView<SceneNodeView<>>(characterController.GetNode());
        gameObject.AddView<DrawableView>(triMesh, characterController.GetNode());

        gscene.AddGameObject(gameObject, GetSceneNode(gameObject));

        SetBoundingSphereFromMesh(gameObject);

        return gameObject;
    }

    void AddCube(float3 POS)
    {
        auto [triMesh, loaded]  = FindMesh(cube1X1X1);

        auto& gameObject = objectPool.Allocate();

        if (!loaded)
            triMesh = LoadTriMeshIntoTable(renderSystem, renderSystem.GetImmediateUploadQueue(), cube1X1X1);

        gameObject.AddView<RigidBodyView>(cubeShape, pscene, POS);
        gameObject.AddView<SceneNodeView<>>(GetRigidBodyNode(gameObject));
        gameObject.AddView<DrawableView>(triMesh, GetSceneNode(gameObject));

        gscene.AddGameObject(gameObject, GetSceneNode(gameObject));

        SetBoundingSphereFromMesh(gameObject);
    }

    GameObject& CreateSpell(SpellData initial, float3 initialPosition, float3 initialVelocity)
    {
        auto& gameObject    = objectPool.Allocate();
        auto& spellInstance = gameObject.AddView<SpellView>();
        auto& spell         = spellInstance.GetData();


        auto [triMesh, loaded] = FindMesh(spellModel);

        if (!loaded)
            triMesh = LoadTriMeshIntoTable(renderSystem, renderSystem.GetImmediateUploadQueue(), spellModel);

        gameObject.AddView<SceneNodeView<>>();
        gameObject.AddView<DrawableView>(triMesh, GetSceneNode(gameObject));

        SetWorldPosition(gameObject, initialPosition);

        gscene.AddGameObject(gameObject, GetSceneNode(gameObject));

        SetBoundingSphereFromMesh(gameObject);

        spell.caster        = initial.caster;
        spell.card          = initial.card;
        spell.duration      = initial.duration;
        spell.life          = initial.life;
        spell.gameObject    = &gameObject;

        spell.position      = initialPosition;
        spell.velocity      = initialVelocity;

        return gameObject;
    }

    bool LoadScene(GUID_t assetID)
    {
        auto res = FlexKit::LoadScene(core, gscene, assetID);

        auto& physics       = PhysXComponent::GetComponent();
        auto& visibility    = SceneVisibilityComponent::GetComponent();

        static std::regex pattern{"Cube"};
        for (auto& entity : gscene.sceneEntities)
        {
            auto& go    = *visibility[entity].entity;
            auto id     = GetStringID(go);

            if (id && std::regex_search(id, pattern))
            {
                auto meshHandle = GetTriMesh(go);
                auto mesh       = GetMeshResource(meshHandle);

                AABB aabb{
                    .Min = mesh->Info.Min,
                    .Max = mesh->Info.Max };

                const auto midPoint = aabb.MidPoint();
                const auto dim      = aabb.Dim() / 2.0f;
                const auto pos      = GetWorldPosition(go);
                auto q              = GetOrientation(go);


                PxShapeHandle shape = physics.CreateCubeShape(dim);

                go.AddView<StaticBodyView>(pscene, shape, pos, q);
            }
        }

        return res;
    }

    void UpdatePlayer(const PlayerFrameState& playerState);


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


UpdateTask& UpdateSpells(UpdateDispatcher& dispathcer, ObjectPool<GameObject>& objectPool, const double dt);


/************************************************************************************************/


void CreateMultiplayerScene(GameWorld&);

PlayerFrameState    GetPlayerFrameState (GameObject& gameObject);
RemotePlayerData*   FindPlayer          (MultiplayerPlayerID_t ID);


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

