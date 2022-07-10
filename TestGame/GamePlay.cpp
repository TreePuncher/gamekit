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


using CellCoord = int3;
using ChunkCoord = int3;

int64_t GetChunkIndex(ChunkCoord coord)
{
    return coord[2] * 2048 * 2048 + coord[1] * 2048 + coord[1];
}

struct SparseMap;

struct MapCordHasher
{
    uint64_t operator ()(const CellCoord& in) noexcept
    {
        return -1;
    }
};

using CellId = uint8_t;

enum CellIds : CellId
{
    Floor,
    Ramp,
    Wall,
    Count,
    Super,
    Error
};

constexpr float CellWeights[] = { 1, 1, 1 };

template<size_t ... ints>
consteval auto _GetCellIdsCollectionHelper(std::integer_sequence<size_t, ints...> int_seq)
{
    return std::array<CellId, (size_t)CellIds::Count>{ ints... };
}

template<size_t ... ints>
consteval auto _GetCellIdsProbabilitiesHelper(std::integer_sequence<size_t, ints...> int_seq, const float total)
{
    return std::array<float, (size_t)CellIds::Count>{ ((CellWeights[ints] / total), ...) };
}

constinit const auto SuperSet   = []() { return _GetCellIdsCollectionHelper(std::make_index_sequence<(size_t)CellIds::Count>{});  }();

constinit std::array<float, (size_t)CellIds::Count> CellProbabilies =
                                                        []() -> std::array<float, (size_t)CellIds::Count>
                                                        {
                                                            const float sum = std::accumulate(std::begin(CellWeights), std::end(CellWeights), 0.0f);

                                                            return _GetCellIdsProbabilitiesHelper(std::make_index_sequence<(size_t)CellIds::Count>{}, sum);
                                                        }();


struct iConstraint
{
    virtual bool operator () (const SparseMap& map, CellCoord xyz, CellId id) const noexcept = 0;
};

using ConstraintTable = Vector<std::unique_ptr<iConstraint>>;

int64_t GetTileID(const ChunkCoord cord) noexcept
{
    constexpr auto r_max = std::numeric_limits<int16_t>::max();

    return cord[0] + cord[1] * r_max + cord[2] * r_max * r_max;
}


struct MapChunk
{
    ChunkCoord  coord;
    bool        active = true;
    CellId      cells[8*8*8]; //8x8

    CellId& operator[] (const int3 localCoord) noexcept
    {
        const auto idx = abs(localCoord[0]) % 8 + abs(localCoord[1]) % 8 * 8 + abs(localCoord[2]) % 8 * 64;

        return cells[idx];
    }

    CellId operator[] (const int3 localCoord) const noexcept
    {
        const auto idx = abs(localCoord[0]) % 8 + abs(localCoord[1]) % 8 * 8 + abs(localCoord[2]) % 8 * 64;
        return cells[idx];
    }

    struct iterator
    {
        CellId*     cells;
        int         flatIdx;
        ChunkCoord  coord;

		struct _Pair
		{
			CellId& cell;
			int3    flatIdx;
		};

        _Pair operator * ()
        {
            return { cells[flatIdx], coord * 8 + int3{ flatIdx % 8, (flatIdx / 8) % 8,  (flatIdx / 64) } };
        }

        void operator ++ ()
        {
            flatIdx++;
        }

        bool operator != (const iterator& rhs) const
        {
            return !(cells == rhs.cells && flatIdx == rhs.flatIdx);
        }
    };

    iterator begin()
    {
        if (active)
            return iterator{ cells, 0, coord };
        else return end();
    }

    iterator end()
    {
        return iterator{ cells, 8 * 8 * 8, coord };
    }

    operator int64_t () const { return GetTileID(coord); }
};


struct ChunkedChunkMap
{
    ChunkedChunkMap(iAllocator& allocator) :
        chunkIDs { allocator },
        chunks   { allocator } {}

    auto begin()    { return chunks.begin(); }
    auto end()      { return chunks.end(); }

    void InsertBlock(const int3 xyz, const int3 begin = { 0, 0, 0 })
    {
		const auto offset = xyz / 2;

        for (int x = 0; x < xyz[0]; x++)
            for (int y = 0; y < xyz[1]; y++)
                for (int z = 0; z < xyz[2]; z++)
                {
                    const auto chunkIdx = chunks.emplace_back(begin + int3{ x, y, z } - offset);

					for (auto&& [cell, cellIdx] : chunks[chunkIdx])
						cell = CellIds::Super;

                    chunks[chunkIdx].active = false;
                }

        std::ranges::sort(chunks);
        chunkIDs.resize(chunks.size());
        for (size_t I = 0; I < chunks.size(); I++)
            chunkIDs[I] = chunks[I];
    }

    std::optional<std::reference_wrapper<MapChunk>> operator [](ChunkCoord coord)
    {
        const auto idx = GetChunkIndex(coord);

        auto res = std::ranges::lower_bound(chunkIDs, idx);

        if (res != chunkIDs.end())
        {
            auto idx = std::distance(chunkIDs.begin(), res);
            if (chunks[idx].coord == coord)
                return { chunks[idx] };
        }

        return {};
    }

    std::optional<std::reference_wrapper<const MapChunk>> operator [](ChunkCoord coord) const
    {
        const auto idx = GetChunkIndex(coord);

        auto res = std::ranges::lower_bound(chunks, idx);

        if (res != chunks.end() && res->coord == coord)
            return { *res };
        else
            return {};
    }

    Vector<int64_t>     chunkIDs;
    Vector<MapChunk>    chunks;
};


struct SparseMap
{
    ChunkedChunkMap chunks;

    auto operator [](CellCoord cord) const
    {
        auto chunkID = cord / (8);
        if (auto res = chunks[chunkID]; res)
            return res.value().get()[cord - (chunkID * 8)];

        return (CellId)CellIds::Error;
    }

    auto begin()    { return chunks.begin(); }
    auto end()      { return chunks.end(); }

    struct NeighborIterator
    {
        NeighborIterator(const SparseMap& IN_map, CellCoord coord, int IN_idx = 0) :
            centralCord { coord  },
            map         { IN_map },
            idx         { IN_idx } {}

        auto operator * ()
        {
            static const int3 offsets[] =
            {
                {  0, -1,  0 },
                { -1,  0,  0 },
                {  1,  0,  0 },
                {  0, -1,  0 },
                {  0,  0,  1 },
                {  0,  0, -1 },
            };

            const auto neighborCoord = offsets[idx] + centralCord;
            return std::make_pair(map[neighborCoord], neighborCoord);
        }

        NeighborIterator begin()    { return { map, centralCord, 0 }; }
        NeighborIterator end()      { return { map, centralCord, 6 }; }

        void operator ++ () noexcept { idx++; }

        bool operator != (const NeighborIterator& rhs) const
        {
            return !(rhs.centralCord == centralCord && rhs.idx == idx);
        }

        const SparseMap&    map;
        CellCoord           centralCord;
        int                 idx = 0;
    };

    auto GetChunk(uint3 xyz)
    {
        auto chunkID = (xyz & (-1 + 15)) / 8;

        return chunks[chunkID];
    }

    static_vector<CellId> GetSuperSet(this auto& map, const ConstraintTable& constraints, const CellCoord xyz)
    {
        static_vector<CellId> out;

        for (const auto cellType : SuperSet)
        {
            for (const auto& constraint : constraints)
                if ((*constraint)(map, xyz, cellType))
                {
                    out.push_back(cellType);
                    break;
                }
        }

        return out;
    }

    void SetCell(const CellCoord XYZ, const CellId ID)
    {
        auto chunkID = (XYZ & (-1 + 15)) / 8;

        if (auto res = chunks[chunkID]; res)
        {
            auto& chunk = res.value().get();
            chunk[XYZ % 8] = ID;
            chunk.active = true;
        }
    }

    float CalculateCellEntropy(this const auto& map, const CellCoord coord, const static_vector<CellId>& superSet)
    {
        const auto sum = [&]()
            {
                float s = 0;
                for(auto e : superSet)
                    s += CellWeights[e];

                return s;
            }();

        float entropy = 0.0f;
        for (auto& cell : superSet)
            entropy += -log(CellWeights[cell] / sum) * CellWeights[cell];

        return entropy;
    }

    void Generate(this SparseMap& map, const ConstraintTable& constraints, const uint3 WHD, iAllocator& tempMemory)
    {
        const auto XYZ = Max(WHD, uint3{ 8, 8, 8 }) / 8;

        map.chunks.InsertBlock(XYZ, XYZ / -2);
        map.SetCell({ 0, 0, 0 }, CellIds::Floor);

        bool continueGeneration;

        struct EntropyPair
        {
            float       entropy;
            CellCoord   cellIdx;

            operator float() const { return entropy; }
        };

        Vector<EntropyPair> entropySet{ tempMemory };
        entropySet.reserve(4096);

        do
        {
            continueGeneration = false;

            for (auto& chunk : map.chunks)
            {
                if (chunk.active)
                {
                    for (auto [cell, cellCoord] : chunk)
                    {
                        if (cell == CellIds::Super)
                            for (auto [neighborCell, _] : NeighborIterator{ map, cellCoord })
                            {
                                if (neighborCell != CellIds::Super && neighborCell != CellIds::Error)
                                {
                                    continueGeneration |= true;

                                    entropySet.emplace_back(
                                        map.CalculateCellEntropy(cellCoord, map.GetSuperSet(constraints, cellCoord)),
                                        cellCoord);

                                    break;
                                }
                            }
                    }
                }
            }


            if (entropySet.size())
            {
                std::ranges::partial_sort(entropySet, entropySet.begin() + 1);

                auto entity = entropySet.front();
                const auto superSet = map.GetSuperSet(constraints, entity.cellIdx);
                if (superSet.size())
                {
                    auto tile = superSet[rand() % superSet.size()];
                    map.SetCell(entity.cellIdx, tile);
                }
                else
                    DebugBreak();
            }

            entropySet.clear();
        } while (continueGeneration);
    }
};


/************************************************************************************************/

class WallConstraint : public iConstraint
{
    virtual bool operator () (const SparseMap& map, CellCoord xyz, CellId ID) const noexcept override
    {
        // wall tiles need at least one neighboring floor tile
        if (ID != CellIds::Wall)
            return false;

        for (auto&& [cell, cellId] : SparseMap::NeighborIterator{ map, xyz })
        {
            if (cell == CellIds::Floor)
                return true;
        }

        return false;
    }
};

class FloorConstraint : public iConstraint
{
    virtual bool operator () (const SparseMap& map, CellCoord xyz, CellId ID) const noexcept override
    {
        // Floors tiles need at least one neighboring floor tile or ramp tile
        if (ID != CellIds::Floor)
            return false;

        for (auto&& [cell, cellId] : SparseMap::NeighborIterator{ map, xyz })
        {
            switch(cell)
                case CellIds::Floor:
                case CellIds::Ramp:
                    return true;
        }

        return false;
    }
};

class RampConstraint : public iConstraint
{
    virtual bool operator () (const SparseMap& map, CellCoord coord, CellId id) const noexcept override
    {
        if (id != CellIds::Ramp)
            return false;

        bool wallNeighbor = false;

        for (auto&& [cell, cellId] : SparseMap::NeighborIterator{ map, coord })
        {
            // Ramps must be next to a wall, but not next to another ramp
            switch (cell)
            {
            case CellIds::Wall:
                wallNeighbor = true; break;
            case CellIds::Ramp:
                return false;
            }
        }

        return wallNeighbor;
    }
};

SparseMap GenerateWorld(GameWorld& world, iAllocator& temp)
{
    // Step 1. Generate world
    SparseMap map{ temp };
	ConstraintTable constraints{ temp };
    constraints.emplace_back(std::make_unique<FloorConstraint>());
    constraints.emplace_back(std::make_unique<WallConstraint>());
    constraints.emplace_back(std::make_unique<RampConstraint>());

    map.Generate(constraints, int3{ 256, 256, 1 }, temp);

    auto chunk = map.GetChunk({0, 0, 0});
    if (chunk)
    {
        auto& c = chunk.value().get();
        c.cells;
        int x = 0;
    }
    // Step 2. Translate map cells -> game world
    // ...

    return map;
}


/************************************************************************************************/


void CreateMultiplayerScene(GameWorld& world, iAllocator& tempAllocator)
{
    //static const GUID_t sceneID = 1234;
    //world.LoadScene(sceneID);

    GenerateWorld(world, tempAllocator);

    auto& physics       = PhysXComponent::GetComponent();
    auto& floorCollider = world.objectPool.Allocate();
    auto floorShape     = physics.CreateCubeShape({ 200, 1, 200 });
    
    auto& staticBody    = floorCollider.AddView<StaticBodyView>(world.layer, float3{ 0, -1.0f, 0 } );
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
    
    if (auto gameObject = FindPlayer(playerState.player); gameObject)
    {
        Apply(
            *gameObject,
            [&](PlayerView& player)
            {
                for (const auto event_ : playerState.inputState.events)
                {
                    switch (event_)
                    {
                    case PlayerInputState::Event::Action1:
                    case PlayerInputState::Event::Action2:
                    case PlayerInputState::Event::Action3:
                    case PlayerInputState::Event::Action4:
                    {
                        player->Action1();
                    }   break;
                    default:
                        break;
                    }
                }
            });
    }
}


/************************************************************************************************/


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
