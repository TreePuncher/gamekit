#include "Gameplay.h"
#include "containers.h"
#include "SceneLoadingContext.h"
#include <algorithm>

using namespace FlexKit;


/************************************************************************************************/


FlashLight::FlashLight() : GadgetInterface{
        ID(),
        "Flashlight",
        "Banish the darkness.",
        0 }
{
    OnActivate =
        [](GameWorld& world, GameObject& player)
        {
        };
}


/************************************************************************************************/


void GameWorld::SpawnEnemy_1(const Enemy_1_Desc& desc)
{
    auto& enemy = objectPool.Allocate();

    //if (LoadPrefab(enemy, "enemy1prefab", core.GetBlockMemory()))
    {
        CreateEnemy1(enemy, desc);

        static const auto material = []() {
            auto& materials = MaterialComponent::GetComponent();
            auto material   = materials.CreateMaterial();
            materials.Add2Pass(material, ShadowMapPassID);
            materials.Add2Pass(material, GBufferPassID);

            return material;
        }();

        scene.AddGameObject(enemy, GetSceneNode(enemy));

        SetBoundingSphereFromMesh(enemy);
        SetMaterialHandle(enemy, material);
    }
    //else
    //        objectPool.Release(enemy);
}


/************************************************************************************************/


GameObject& GameWorld::CreatePlayer(const PlayerDesc& desc)
{
    GameObject& gameObject = objectPool.Allocate();
    CreateThirdPersonCameraController(gameObject, desc.layer, allocator, desc.r, desc.h);

    //auto [triMesh, loaded] = FindMesh(playerModel);

    //if (!loaded)
    //    triMesh = LoadTriMeshIntoTable(renderSystem.GetImmediateUploadQueue(), playerModel);

    auto& player        = gameObject.AddView<PlayerView>(
        PlayerState{
            .gameObject         = &gameObject,
            .playerID           = desc.player,
        });

    auto& localplayer   = gameObject.AddView<LocalPlayerView>(
        LocalPlayerData{
            .playerGameState    = player.handle,
            .playerID           = desc.player,
        });

    localplayer.GetData().playerID      = desc.player;

    auto node = GetCameraControllerNode(gameObject);

    gameObject.AddView<SceneNodeView<>>(node);
    //gameObject.AddView<BrushView>(triMesh, node);

    //auto& materials     = MaterialComponent::GetComponent();
    //auto material       = materials.CreateMaterial();
    //materials.Add2Pass(material, PassHandle{ GetCRCGUID(PBR_CLUSTERED_DEFERRED) });
    //auto& materialView  = gameObject.AddView<MaterialComponentView>(material);
    //SetMaterialHandle(gameObject, material);

    //materialView.SetProperty(GetCRCGUID(PBR_ALBEDO), float4{ 1, 0, 0, 0.5f });

    static bool s_addAssets = []()
    {
        //FlexKit::AddAssetFile("assets\\testprefab.gameres");
        FlexKit::AddAssetFile("assets\\demonGirl.gameres");
        return true;
    }();


    auto [triMesh, loaded] = FindMesh(9021);

    if (!loaded)
        triMesh = LoadTriMeshIntoTable(renderSystem.GetImmediateUploadQueue(), 9021);

    auto& materials = MaterialComponent::GetComponent();
    auto material   = materials.CreateMaterial();
    materials.Add2Pass(material, ShadowMapPassID);
    materials.Add2Pass(material, GBufferPassID);

    auto& prefab    = objectPool.Allocate();
    auto& nodeView  = prefab.AddView<SceneNodeView<>>();
    auto& brush     = prefab.AddView<BrushView>(triMesh);

    nodeView.Scale(float3{ 3 });
    EnableScale(prefab, true);
    brush.SetMaterial(material);
    scene.AddGameObject(prefab, GetSceneNode(prefab));

    SetBoundingSphereFromMesh(prefab);
    SetParentNode(node, nodeView);


    /*
    if (LoadPrefab(prefab, 123456, allocator))
    {
        auto& materials = MaterialComponent::GetComponent();
        auto material   = materials.CreateMaterial();
        materials.Add2Pass(material, ShadowMapAnimatedPassID);
        materials.Add2Pass(material, GBufferAnimatedPassID);

        prefab.AddView<MaterialView>(material);
        prefab.AddView<StringIDView>("wiggle", 6);

        scene.AddGameObject(prefab, GetSceneNode(prefab));

        auto& animator  = *GetAnimator(prefab);
        //auto input      = animator.GetInputValue(0);
        //(*input)->x     = 0.0f * pi;

        SetBoundingSphereFromMesh(prefab);

        SetParentNode(node, GetSceneNode(prefab));
    }
    else
        objectPool.Release(prefab);
    */

    return gameObject;
}


/************************************************************************************************/


GameWorld::GameWorld(EngineCore& IN_core) :
    allocator       { static_cast<iAllocator&>(IN_core.GetBlockMemory()) },
    core            { IN_core },
    renderSystem    { IN_core.RenderSystem},
    objectPool      { IN_core.GetBlockMemory(), 1024 * 16 },
    enemy1Component { IN_core.GetBlockMemory() },
    layer           { PhysXComponent::GetComponent().CreateLayer() },
    scene           { IN_core.GetBlockMemory() },
    cubeShape       { PhysXComponent::GetComponent().CreateCubeShape({ 0.5f, 0.5f, 0.5f}) }
{
    AddAssetFile("assets\\enemy1.gameres");
}


/************************************************************************************************/


GameWorld::~GameWorld()
{
    Release();
}


/************************************************************************************************/


void GameWorld::Release()
{
    scene.ClearScene();

    objectPool.~ObjectPool();
}


/************************************************************************************************/


GameObject& GameWorld::AddLocalPlayer(MultiplayerPlayerID_t multiplayerID)
{
    return CreatePlayer(
                PlayerDesc{
                    .player = multiplayerID,
                    .layer  = layer,
                    .scene  = scene,
                    .h      = 0.5f,
                    .r      = 0.5f
                });
}


/************************************************************************************************/


GameObject& GameWorld::AddRemotePlayer(MultiplayerPlayerID_t playerID, ConnectionHandle connection)
{
    auto& gameObject = objectPool.Allocate();

    auto& player    = gameObject.AddView<PlayerView>(
        PlayerState{
            .gameObject         = &gameObject,
            .playerID           = playerID,
        });

    auto& dummy     = gameObject.AddView<RemotePlayerView>(
        RemotePlayerData{
            .gameObject         = &gameObject,
            .playerGameState    = player.handle,
            .connection         = connection,
            .playerID           = playerID });

    auto [triMesh, loaded] = FindMesh(playerModel);

    if (!loaded)
        triMesh = LoadTriMeshIntoTable(renderSystem.GetImmediateUploadQueue(), playerModel);

    auto& characterController = gameObject.AddView<CharacterControllerView>(layer, float3{0, 0, 0}, GetZeroedNode(), 1.0f, 1.0f);
    gameObject.AddView<SceneNodeView<>>(characterController.GetNode());
    gameObject.AddView<BrushView>(triMesh);

    scene.AddGameObject(gameObject, GetSceneNode(gameObject));

    SetBoundingSphereFromMesh(gameObject);

    return gameObject;
}


/************************************************************************************************/


void GameWorld::AddCube(float3 POS)
{
    auto [triMesh, loaded]  = FindMesh(cube1X1X1);

    auto& gameObject = objectPool.Allocate();

    if (!loaded)
        triMesh = LoadTriMeshIntoTable(renderSystem.GetImmediateUploadQueue(), cube1X1X1);

    auto& rigidBody = gameObject.AddView<RigidBodyView>(layer, POS);
    auto& sceneNode = gameObject.AddView<SceneNodeView<>>(GetRigidBodyNode(gameObject));
    auto& brushView = gameObject.AddView<BrushView>(triMesh);

    rigidBody.AddShape(cubeShape);

    scene.AddGameObject(gameObject, GetSceneNode(gameObject));

    SetBoundingSphereFromMesh(gameObject);
}


/************************************************************************************************/


bool GameWorld::LoadScene(GUID_t assetID)
{
    SceneLoadingContext ctx
    {
        .scene = scene,
        .layer = layer,
        .nodes = Vector<NodeHandle>(core.GetTempMemory())
    };

    auto res = FlexKit::LoadScene(core, ctx, assetID);

    auto& physics       = PhysXComponent::GetComponent();
    auto& visibility    = SceneVisibilityComponent::GetComponent();

    static const std::regex pattern{"Cube"};

    for (auto& entity : scene.sceneEntities)
    {
        auto& go    = *visibility[entity].entity;
        auto id     = GetStringID(go);

        if (id && std::regex_search(id, pattern))
        {
            auto meshHandle = GetTriMesh(go);
            auto mesh       = GetMeshResource(meshHandle);

            const AABB aabb{
                .Min = mesh->Info.Min,
                .Max = mesh->Info.Max };

            const auto midPoint = aabb.MidPoint();
            const auto dim      = aabb.Dim() / 2.0f;
            const auto pos      = GetWorldPosition(go);
            auto q              = GetOrientation(go);


            auto shape = physics.CreateCubeShape(dim);

            auto& staticBodyView = go.AddView<StaticBodyView>(layer, pos, q);
            staticBodyView.AddShape(shape);
        }
    }

    return res;
}


/************************************************************************************************/


void CreateMultiplayerScene(GameWorld& world)
{
    static const GUID_t sceneID = 1234;
    world.LoadScene(sceneID);

    auto& physics       = PhysXComponent::GetComponent();
    auto& floorCollider = world.objectPool.Allocate();
    auto floorShape     = physics.CreateCubeShape({ 200, 1, 200 });
    
    auto& staticBody = floorCollider.AddView<StaticBodyView>(world.layer, float3{ 0, -1.0f, 0 });
    staticBody.AddShape(floorShape);


    auto& layer = PhysXComponent::GetComponent().GetLayer_ref(world.layer);
    layer.AddUpdateCallback(
        [&](WorkBarrier& barrier, iAllocator& allocator, double dT)
        {
            CommitEnemyMoves(dT);
        });

    for (int I = 0; I < 100; I++)
    {
        const int spawnArea = 40;

        Enemy_1_Desc enemy1
        {
            .initialPos = float3(rand() % spawnArea - spawnArea / 2, 0.0f, rand() % spawnArea - spawnArea / 2),
            .layer      = world.layer
        };

        world.SpawnEnemy_1(enemy1);
    }
}


/************************************************************************************************/


void GameWorld::UpdatePlayer(const PlayerFrameState& playerState, const double dT)
{
    auto gameObject = FindPlayer(playerState.player);

    Apply(
        *gameObject,
        [&](PlayerView& player)
        {
        });

    if (gameObject) {
        for (const auto event_ : playerState.inputState.events)
        {
            switch (event_)
            {
            case PlayerInputState::Event::Action1:
            case PlayerInputState::Event::Action2:
            case PlayerInputState::Event::Action3:
            case PlayerInputState::Event::Action4:
            {
            }   break;
            default:
                break;
            }
        }
    }
}


void GameWorld::UpdateRemotePlayer(const PlayerFrameState& playerState, const double dT)
{
    auto playerView = FindRemotePlayer(playerState.player);

    if (playerView) {
        playerView->Update(playerState);

        UpdatePlayer(playerState, dT);
    }
}

/************************************************************************************************/


template<typename TY_Touch, typename TY_Final>
class OverlappCallback : public physx::PxOverlapCallback
{
public:
    OverlappCallback(TY_Touch IN_touch, TY_Final IN_final) :
        physx::PxOverlapCallback    { hits, sizeof(hits) / sizeof(hits[0]) },
        fnTouch                     { IN_touch },
        fnFinal                     { IN_final } {}

    ~OverlappCallback() final {}

    physx::PxAgain processTouches(const physx::PxOverlapHit* buffer, physx::PxU32 nbHits) final
    {
        return fnTouch(buffer, nbHits);
    }

    void finalizeQuery() final
    {
        fnFinal();
    }

    TY_Touch    fnTouch;
    TY_Final    fnFinal;

    physx::PxOverlapHit hits[64];
};


UpdateTask& GameWorld::UpdateGadgets(FlexKit::UpdateDispatcher& dispathcer, ObjectPool<GameObject>& objectPool, const double dt)
{
    struct _ {};

    return dispathcer.Add<_>(
        [&](auto& builder, auto& _)
        {
        },
        [=, &objectPool = objectPool](auto& _, auto& allocator)
        {
            ProfileFunction();

            auto& gadgetComponent = GadgetComponent::GetComponent();

            Vector<GameObject*> freeList{ &allocator };

            auto scene = PhysXComponent::GetComponent().GetLayer_ref(layer).scene;

            for (auto& gadgetInstance : gadgetComponent)
            {
                auto& gadget = gadgetInstance.componentData;
            }

            for (auto object : freeList)
                objectPool.Release(*object);
        });

}


/************************************************************************************************/


PlayerFrameState GetPlayerFrameState(GameObject& gameObject)
{
    PlayerFrameState out;

    Apply(
        gameObject,
        [&](LocalPlayerView& view)
        {
            const float3        pos = GetCameraControllerModelPosition(gameObject);
            const Quaternion    q   = GetCameraControllerModelOrientation(gameObject);

            out.player          = view.GetData().playerID;
            out.pos             = pos;
            out.orientation     = q;
            out.forwardVector   = (float3{ 1.0f, 0.0f, 1.0f } * GetCameraControllerForwardVector(gameObject)).normal();
        });

    return out;
}


/************************************************************************************************/


RemotePlayerData* FindRemotePlayer(MultiplayerPlayerID_t ID)
{
    auto& players = RemotePlayerComponent::GetComponent();

    for (auto& player : players)
    {
        if (player.componentData.playerID == ID)
            return &players[player.handle];
    }

    return nullptr;
}

GameObject* FindPlayer(MultiplayerPlayerID_t ID)
{
    auto& players = PlayerComponent::GetComponent();

    for (auto& player : players)
    {
        if (player.componentData.playerID == ID)
            return players[player.handle].gameObject;
    }

    return nullptr;
}


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
