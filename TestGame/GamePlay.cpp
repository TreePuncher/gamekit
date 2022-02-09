#include "Gameplay.h"
#include "containers.h"
#include <algorithm>


using namespace FlexKit;


/************************************************************************************************/


PowerCard::PowerCard() : CardInterface{
        ID(),
        "PowerCard",
        "Use to increase current level, allows for more powerful spell casting",
        0 }
{
    OnCast =
        [](GameWorld& world, GameObject& player)
        {
            Apply(player,
                [](PlayerView& player)
                {
                    player.GetData().maxCastingLevel++;
                });
        };
}

FireBall::FireBall() : CardInterface{
        ID(),
        "FireBall",
        "Throws a small ball a fire, burns on contact.\n"
        "Required casting level is 1" }
{
    OnCast =
        [&](GameWorld& world, GameObject& playerGameObject)
        {
            Apply(playerGameObject,
                [&](PlayerView& player)
                {
                    if (player->mana < 1 && false)
                    {
                        FK_LOG_INFO("Cast Failed: Not enough managa!");
                        return;
                    }

                    player->mana -= 1;

                    SpellData initial;
                    initial.card     = FireBall();
                    initial.caster   = player->playerID;
                    initial.duration = 3.0f;
                    initial.life     = 0.0f;

                    auto velocity    = player->forward * 50.0f;
                    auto position    = player->position + player->forward * 2.0f;

                    std::cout << "Particle Created at: " << position << " with Velocity: " << velocity << "\n";

                    world.CreateSpell(initial, position, velocity);
                });
        };

    OnHit =
        [](GameWorld& world, GameObject& player)
        {
            Apply(player,
                [](PlayerView& player)
                {
                    player->ApplyDamage(100);
                });
        };
}


/************************************************************************************************/


GameObject& GameWorld::CreatePlayer(const PlayerDesc& desc)
{
    GameObject& gameObject = objectPool.Allocate();
    CreateThirdPersonCameraController(gameObject, desc.layer, allocator, desc.r, desc.h);

    auto [triMesh, loaded] = FindMesh(playerModel);

    if (!loaded)
        triMesh = LoadTriMeshIntoTable(renderSystem.GetImmediateUploadQueue(), playerModel);

    auto& player        = gameObject.AddView<PlayerView>(
        PlayerState{
            .gameObject         = &gameObject,
            .playerID           = desc.player,
            .playerHealth       = 100.0f,
            .maxCastingLevel    = 1
        });

    auto& localplayer   = gameObject.AddView<LocalPlayerView>(
        LocalPlayerData{
            .playerGameState    = player.handle,
            .playerID           = desc.player,
        });

    localplayer.GetData().playerID      = desc.player;

    auto node = GetCameraControllerNode(gameObject);

    gameObject.AddView<SceneNodeView<>>(node);
    gameObject.AddView<BrushView>(triMesh, node);

    auto& materials     = MaterialComponent::GetComponent();
    auto material       = materials.CreateMaterial();
    materials.Add2Pass(material, PassHandle{ GetCRCGUID(PBR_CLUSTERED_DEFERRED) });
    auto& materialView  = gameObject.AddView<MaterialComponentView>(material);
    SetMaterialHandle(gameObject, material);

    materialView.SetProperty(GetCRCGUID(PBR_ALBEDO), float4{ 1, 0, 0, 0.5f });

    desc.scene.AddGameObject(gameObject, node);

    return gameObject;
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
            .playerHealth       = 100.0f,
            .maxCastingLevel    = 1 });

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
    gameObject.AddView<BrushView>(triMesh, characterController.GetNode());

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

    gameObject.AddView<RigidBodyView>(cubeShape, layer, POS);
    gameObject.AddView<SceneNodeView<>>(GetRigidBodyNode(gameObject));
    gameObject.AddView<BrushView>(triMesh, GetSceneNode(gameObject));

    scene.AddGameObject(gameObject, GetSceneNode(gameObject));

    SetBoundingSphereFromMesh(gameObject);
}


/************************************************************************************************/


GameObject& GameWorld::CreateSpell(SpellData initial, float3 initialPosition, float3 initialVelocity)
{
    auto& gameObject    = objectPool.Allocate();
    auto& spellInstance = gameObject.AddView<SpellView>();
    auto& spell         = spellInstance.GetData();


    auto [triMesh, loaded] = FindMesh(spellModel);

    if (!loaded)
        triMesh = LoadTriMeshIntoTable(renderSystem.GetImmediateUploadQueue(), spellModel);

    auto& materials = MaterialComponent::GetComponent();

    const static auto materialClass = [&]()
        {
            auto particleMaterial   = materials.CreateMaterial();

            materials.Add2Pass(particleMaterial, PassHandle{ GetCRCGUID(OIT_MCGUIRE) });

            materials.SetProperty(particleMaterial, GetCRCGUID(PBR_ALBEDO), float4{ 0.5f, 0.0f, 0.5f, 0.5f });
            materials.SetProperty(particleMaterial, GetCRCGUID(PBR_SPECULAR), float4{ 0.9f, 0.9f, 0.9f, 0.5f });

            materials.AddRef(particleMaterial);

            return particleMaterial;
        }();


    auto particleMaterial = materials.CreateMaterial(materialClass);

    gameObject.AddView<SceneNodeView<>>();
    gameObject.AddView<BrushView>(triMesh, GetSceneNode(gameObject));
    gameObject.AddView<MaterialComponentView>(particleMaterial);

    SetMaterialHandle(gameObject, particleMaterial);

    SetWorldPosition(gameObject, initialPosition);

    scene.AddGameObject(gameObject, GetSceneNode(gameObject));

    SetVisable(gameObject, true);
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


/************************************************************************************************/


bool GameWorld::LoadScene(GUID_t assetID)
{
    auto res = FlexKit::LoadScene(core, scene, assetID);

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

            AABB aabb{
                .Min = mesh->Info.Min,
                .Max = mesh->Info.Max };

            const auto midPoint = aabb.MidPoint();
            const auto dim      = aabb.Dim() / 2.0f;
            const auto pos      = GetWorldPosition(go);
            auto q              = GetOrientation(go);


            PxShapeHandle shape = physics.CreateCubeShape(dim);

            go.AddView<StaticBodyView>(layer, shape, pos, q);
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
    
    floorCollider.AddView<StaticBodyView>(world.layer, floorShape, float3{ 0, -1.0f, 0 });
}


/************************************************************************************************/


void GameWorld::UpdatePlayer(const PlayerFrameState& playerState, const double dT)
{
    auto gameObject = FindPlayer(playerState.player);

    Apply(
        *gameObject,
        [&](PlayerView& player)
        {
            player->mana = Min(player->mana + dT, player->maxCastingLevel);
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
                FireBall fireball;

                fireball.OnCast(*this, *gameObject);
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


UpdateTask& GameWorld::UpdateSpells(FlexKit::UpdateDispatcher& dispathcer, ObjectPool<GameObject>& objectPool, const double dt)
{
    struct _ {};
    return dispathcer.Add<_>(
        [&](auto& builder, auto& _)
        {
        },
        [=, &objectPool = objectPool](auto& _, auto& allocator)
        {
            ProfileFunction();

            auto& spellComponent = SpellComponent::GetComponent();

            Vector<GameObject*> freeList{ &allocator };

            auto scene = PhysXComponent::GetComponent().GetLayer_ref(layer).scene;

            for (auto& spellInstance : spellComponent)
            {
                auto& spell = spellInstance.componentData;

                spell.life      += dt;
                spell.position  += spell.velocity * dt;

                OverlappCallback callback(
                    [&](const physx::PxOverlapHit* buffer, physx::PxU32 nbHits) -> bool
                    {
                        for (size_t I = 0; I < nbHits; I++)
                        {
                            if (buffer[I].actor && buffer[I].actor->userData) {

                                auto& gameObject = *reinterpret_cast<GameObject*>(buffer[I].actor->userData);

                                if (gameObject.hasView(PlayerView::GetComponentID()))
                                    spell.card.OnHit(*this, gameObject);
                            }
                        }

                        return false;
                    },
                    []() {}
                );
                auto boundingSphere = GetBoundingSphereFromMesh(*spell.gameObject);

                physx::PxSphereGeometry geo     { 1 };
                physx::PxTransform      pose    { Float3TopxVec3(spell.position) };

                if (spell.life > spell.duration || scene->overlap(geo, pose, callback))
                    freeList.push_back(spell.gameObject);
                else
                    SetWorldPosition(*spell.gameObject, spell.position);
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
