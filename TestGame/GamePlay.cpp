#include "Gameplay.h"
#include "containers.h"
#include "SceneLoadingContext.h"
#include "KeyValueIds.h"

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


GameWorld::GameWorld(EngineCore& IN_core, bool debug) :
    allocator       { static_cast<iAllocator&>(IN_core.GetBlockMemory()) },
    core            { IN_core },
    renderSystem    { IN_core.RenderSystem},
    objectPool      { IN_core.GetBlockMemory(), 1024 * 16 },
    enemy1Component { IN_core.GetBlockMemory() },
    layer           { PhysXComponent::GetComponent().CreateLayer(debug) },
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

using CellState_t = uint8_t;

enum CellStates : CellState_t
{
    Floor,
    Ramp,
    Corner,
    Wall,
    Count,
    Super,
    Error
};

static_assert(CellStates::Count < sizeof(CellState_t) * 8, "Increase CellState_t ByteSize");

constexpr uint8_t GetIdBit(const CellState_t id)
{
    return 1 << id;
}

constexpr float CellWeights[] = { 1, 1, 1, 1 };

template<size_t ... ints>
consteval auto _GetCellState_tsCollectionHelper(std::integer_sequence<size_t, ints...> int_seq)
{
    return std::array<CellState_t, (size_t)CellStates::Count>{ ints... };
}

template<size_t ... ints>
consteval auto _GetCellState_tsProbabilitiesHelper(std::integer_sequence<size_t, ints...> int_seq, const float total)
{
    return std::array<float, (size_t)CellStates::Count>{ ((CellWeights[ints] / total), ...) };
}

constinit const auto SuperSet = []() { return _GetCellState_tsCollectionHelper(std::make_index_sequence<(size_t)CellStates::Count>{});  }();

constinit std::array<float, (size_t)CellStates::Count> CellProbabilies =
                                                        []() -> std::array<float, (size_t)CellStates::Count>
                                                        {
                                                            const float sum = std::accumulate(std::begin(CellWeights), std::end(CellWeights), 0.0f);

                                                            return _GetCellState_tsProbabilitiesHelper(std::make_index_sequence<(size_t)CellStates::Count>{}, sum);
                                                        }();

consteval CellState_t FullSet()
{
    uint8_t set = 0;
    for (uint8_t I = 0; I < CellStates::Count; I++)
        set |= GetIdBit(I);

    return set;
}



struct iConstraint
{
    virtual bool Constraint (const   SparseMap& map,const CellCoord coord, const CellState_t ID) const noexcept = 0;
    virtual bool Apply      (        SparseMap& map,const CellCoord coord)                  const noexcept = 0;
};

using ConstraintTable = static_vector<std::unique_ptr<iConstraint>, 16>[CellStates::Count];

int64_t GetTileID(const ChunkCoord cord) noexcept
{
    constexpr auto r_max = std::numeric_limits<int16_t>::max();

    return cord[0] + cord[1] * r_max + cord[2] * r_max * r_max;
}


struct MapChunk
{
    ChunkCoord  coord;
    bool        active = true;
    CellState_t      cells[8*8*8]; //8x8

    CellState_t& operator[] (const int3 localCoord) noexcept
    {
        const auto idx = abs(localCoord[0]) % 8 + abs(localCoord[1]) % 8 * 8 + abs(localCoord[2]) % 8 * 64;

        return cells[idx];
    }

    CellState_t operator[] (const int3 localCoord) const noexcept
    {
        const auto idx = abs(localCoord[0]) % 8 + abs(localCoord[1]) % 8 * 8 + abs(localCoord[2]) % 8 * 64;
        return cells[idx];
    }

    struct iterator
    {
        CellState_t*     cells;
        int         flatIdx;
        ChunkCoord  coord;

		struct _Pair
		{
			CellState_t& cell;
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


/************************************************************************************************/


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
						cell = FullSet();

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


/************************************************************************************************/


struct SparseMap
{
    ChunkedChunkMap chunks;

    auto operator [](CellCoord cord) const
    {
        auto chunkID = cord / (8);
        if (auto res = chunks[chunkID]; res)
            return res.value().get()[cord - (chunkID * 8)];

        return (CellState_t)CellStates::Error;
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


    struct Neighbor
    {
        uint8_t c;
        int3    xyz;

        operator uint8_t& () { return c; }
    };

    static CircularBuffer<Neighbor, 8> GetNeighborRing(const SparseMap& map, CellCoord coord)
    {
        CircularBuffer<Neighbor, 8> buffer;
        for (auto&& [c, xyz] : NeighborIterator{map, coord})
            buffer.push_back({ c, xyz });

        return buffer;
    }


    auto GetChunk(uint3 xyz)
    {
        auto chunkID = (xyz & (-1 + 15)) / 8;

        return chunks[chunkID];
    }

    static_vector<CellState_t> GetSuperState(this const SparseMap& map, const CellCoord xyz)
    {
        auto bits = map[xyz];
        static_vector<CellState_t> set;
        for (size_t I = 0; I < CellStates::Count; I++)
        {
            if (GetIdBit((CellState_t)I) & bits)
                set.push_back((CellState_t)I);
        }

        return set;
    }

    static_vector<CellState_t> CalculateSuperState(this const SparseMap& map, const ConstraintTable& constraints, const CellCoord xyz)
    {
        static_vector<CellState_t> out;

        for (const auto cellType : SuperSet)
        {
            if (constraints[cellType].size())
            {
                bool valid = true;
                for (const auto& constraint : constraints[cellType])
                {
                    valid &= constraint->Constraint(map, xyz, cellType);
                    if(!valid)
                        break;
                }

                if(valid)
                    out.push_back(cellType);
            }
        }

        return out;
    }

    void UpdateNeighboringSuperStates(this auto& map, const ConstraintTable& constraints, const CellCoord cellCoord)
    {
        for (auto [neighborCell, neighborCellState_tx] : NeighborIterator{ map, cellCoord })
        {
            if (__popcnt(neighborCell) == 1)
                continue;

            auto set = map.CalculateSuperState(constraints, neighborCellState_tx);

            if (set.size() == 1)
            {
                map.SetCell(neighborCellState_tx, GetIdBit(set.front()));
                //map.UpdateNeighboringSuperStates(constraints, neighborCellState_tx);
            }
            else
            {
                CellState_t bitSet = 0;

                for (auto&& it : set)
                    bitSet |= GetIdBit(it);

                map.SetCell(neighborCellState_tx, bitSet);
            }
        }
    }

    void SetCell(const CellCoord XYZ, const CellState_t ID)
    {
        auto chunkID = (XYZ & (-1 + 15)) / 8;

        if (auto res = chunks[chunkID]; res)
        {
            auto& chunk = res.value().get();
            chunk[XYZ % 8] = ID;
            chunk.active = true;
        }
    }

	
	void RemoveBits(const CellCoord XYZ, const CellState_t ID)
	{
		auto chunkID = (XYZ & (-1 + 15)) / 8;

		if (auto res = chunks[chunkID]; res)
		{
			auto& chunk			= res.value().get();
			auto currentState	= chunk[XYZ % 8];
			auto newState		= currentState & ~ID;
			chunk[XYZ % 8]		= newState;

			chunk.active = true;
		}
	}


    float CalculateCellEntropy(this const auto& map, const CellCoord coord, const static_vector<CellState_t>& superSet)
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
        map.SetCell({ 0, 0, 0 }, GetIdBit(CellStates::Floor));
        map.UpdateNeighboringSuperStates(constraints, {0, 0, 0});


        struct EntropyPair
        {
            float       entropy;
            CellCoord   cellIdx;

            operator float() const { return entropy; }
        };

        Vector<EntropyPair> entropySet{ tempMemory };
        entropySet.reserve(4096);

        bool continueGeneration;
        
        do
        {
            continueGeneration = false;

            for (auto& chunk : map.chunks)
            {
                if (chunk.active)
                {
                    for (auto [cell, cellCoord] : chunk)
                    {
                        auto res = __popcnt(cell);
                        if (__popcnt(cell) > 1)
                        {
                            continueGeneration |= true;

                            const auto super = map.GetSuperState(cellCoord);

                            CellState_t bitSet = 0;

                            for (auto&& it : super)
                                bitSet |= GetIdBit(it);

                            map.SetCell(cellCoord, bitSet);

                            entropySet.emplace_back(
                                map.CalculateCellEntropy(cellCoord, super),
                                cellCoord);

                            if (!super.size())
                                DebugBreak();
                        }
                    }
                }
            }


            if (entropySet.size())
            {
                std::ranges::partial_sort(entropySet, entropySet.begin() + 1);

                auto entity = entropySet.front();
                const auto superSet = map.GetSuperState(entity.cellIdx);
                
                if (superSet.size() == 1)
                {
                    for (auto& constraint : constraints[superSet.front()])
                        constraint->Apply(map, entity.cellIdx);

                    map.UpdateNeighboringSuperStates(constraints, entity.cellIdx);
                }
                else if (superSet.size() > 1)
                {
                    auto cellId = superSet[rand() % superSet.size()];

                    for (auto& constraint : constraints[cellId])
                        constraint->Apply(map, entity.cellIdx);

                    map.UpdateNeighboringSuperStates(constraints, entity.cellIdx);
                }
                else
                    DebugBreak();
            }

            entropySet.clear();

            if (0)
            {
                std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";

                if (auto chunk = map.GetChunk({ 0, 0, 0 }); chunk)
                {
                    auto& chunk_ref = chunk.value().get();

                    int x = 0;
                    int y = 0;
                    for (auto [c, _] : chunk_ref)
                    {
                        if(c & GetIdBit(CellStates::Floor))
                            std::cout << "_";
                        if(c & GetIdBit(CellStates::Ramp))
                            std::cout << "R";
                        if(c & GetIdBit(CellStates::Wall))
                            std::cout << "X";
                        if(c & GetIdBit(CellStates::Corner))
                            std::cout << "C";

                        auto res = __popcnt(c);
                        for(size_t i = 0; i < CellStates::Count - res; i++)
                            std::cout << " ";

                        if (++x >= 8)
                        {
                            std::cout << "\n";

                            y++;
                            x = 0;
                            if (y > 8)
                                break;
                        }
                    }
                }
            }
        } while (continueGeneration);
    }
};


/************************************************************************************************/


struct WallConstraint : iConstraint
{
    bool Constraint(const SparseMap& map, const CellCoord xyz, const CellState_t ID) const noexcept final
    {   // wall tiles need at least one neighboring floor tile
        for (auto&& [cell, cellId] : SparseMap::NeighborIterator{ map, xyz })
        {
            if (cell & GetIdBit(CellStates::Floor))
                return true;
        }

        return false;
    }

    bool Apply(SparseMap& map, const CellCoord coord) const noexcept final
    {
		map.SetCell(coord, GetIdBit(CellStates::Wall));

        //for (auto&& [cell, cellId] : SparseMap::NeighborIterator{ map, coord })
        //    map.RemoveBits(cellId, GetIdBit(CellStates::Ramp));

        return true;
    }
};


struct CornerConstraint : iConstraint
{
    bool Constraint(const SparseMap& map, const CellCoord coord, const CellState_t ID) const noexcept final
    {
        auto neighbors = SparseMap::GetNeighborRing(map, coord);
        size_t neighborCount = 0;

        for (auto&& [c, _] : neighbors)
        {
            if (c == GetIdBit(CellStates::Ramp))
                return false;

            neighborCount += c & GetIdBit(CellStates::Corner);
        }

        auto cornerBit = GetIdBit(CellStates::Corner);
        if (neighborCount == 2)
        {
            return
                ((neighbors[0] & cornerBit && neighbors[2] & cornerBit) ||
                    (neighbors[2] & cornerBit && neighbors[4] & cornerBit) ||
                    (neighbors[4] & cornerBit && neighbors[6] & cornerBit) ||
                    (neighbors[6] & cornerBit && neighbors[0] & cornerBit)) != 0;
        }
        else return false;
    }

    bool Apply(SparseMap& map, const CellCoord coord) const noexcept final
    {
		auto neighbors = SparseMap::GetNeighborRing(map, coord);
        size_t neighborCount = 0;

        for (auto& c : neighbors)
        {
            if (c == GetIdBit(CellStates::Ramp))
                return false;

            neighborCount += c & GetIdBit(CellStates::Corner);
        }

        auto cornerBit = GetIdBit(CellStates::Corner);
        if (neighborCount == 2)
        {
			if (neighbors[0] & cornerBit && neighbors[2] & cornerBit)
			{
				map.SetCell(coord + int2{ 0, 0 }, GetIdBit(CellStates::Corner));
				map.SetCell(coord + int2{ 0, 1 }, GetIdBit(CellStates::Wall));
				map.SetCell(coord + int2{ 1, 0 }, GetIdBit(CellStates::Wall));
			}
			else if (neighbors[2] & cornerBit && neighbors[4] & cornerBit)
			{
				map.SetCell(coord + int2{ 0, 0 }, GetIdBit(CellStates::Corner));
				map.SetCell(coord + int2{ 1, 0 }, GetIdBit(CellStates::Wall));
				map.SetCell(coord + int2{ 0,-1 }, GetIdBit(CellStates::Wall));
			}
			else if (neighbors[4] & cornerBit && neighbors[6] & cornerBit)
			{
				map.SetCell(coord + int2{ 0,  0 }, GetIdBit(CellStates::Corner));
				map.SetCell(coord + int2{ 0, -1 }, GetIdBit(CellStates::Wall));
				map.SetCell(coord + int2{ -1, 0 }, GetIdBit(CellStates::Wall));
			}
			else if (neighbors[6] & cornerBit && neighbors[0] & cornerBit)
			{
				map.SetCell(coord + int2{  0,  0 }, GetIdBit(CellStates::Corner));
				map.SetCell(coord + int2{ -1,  0 }, GetIdBit(CellStates::Wall));
				map.SetCell(coord + int2{  0,  1 }, GetIdBit(CellStates::Wall));
			}

			return true;
        }
        else return false;
    }
};


struct FloorConstraint : public iConstraint
{
    bool Constraint(const SparseMap& map, const CellCoord xyz, const CellState_t coord)  const noexcept final
    {
        return true;
    }

    bool Apply(SparseMap& map, const CellCoord coord) const noexcept final
    {
		map.SetCell(coord, GetIdBit(CellStates::Floor));

        return true;
    }
};


struct RampConstraint : public iConstraint
{
    bool Constraint(const SparseMap& map, const CellCoord coord, const CellState_t ID) const noexcept final
    {   // Rules
        // must be next to a 3 wall segment,
        // not next to another ramp
        auto neighbors = SparseMap::GetNeighborRing(map, coord);

        int wallCount = 0;
        for (auto&& cell : neighbors)
        {
            if(cell == GetIdBit(CellStates::Ramp))
                return false;

            if (cell == GetIdBit(CellStates::Corner))
                return false;

            wallCount += (cell & GetIdBit(CellStates::Wall)) != 0;
        }

		if (wallCount > 1)
		{
			int i = 0;
			for (; i < neighbors.size(); i++)
				if (neighbors[i] & GetIdBit(CellStates::Wall))
					break;

			return	neighbors[i + 0] & GetIdBit(CellStates::Wall) &&
					neighbors[i + 1] & GetIdBit(CellStates::Wall) &&
					neighbors[i + 2] & GetIdBit(CellStates::Wall);
		}
        //    return ((neighbors[0] & GetIdBit(CellStates::Wall)) |
        //            (neighbors[2] & GetIdBit(CellStates::Wall)) |
        //            (neighbors[4] & GetIdBit(CellStates::Wall)) |
        //            (neighbors[6] & GetIdBit(CellStates::Wall)));
        else return false;
    }

    bool Apply(SparseMap& map, const CellCoord coord) const noexcept final
    {
        map.SetCell(coord, GetIdBit(CellStates::Ramp));

        auto neighbors = SparseMap::GetNeighborRing(map, coord);
        for (auto&& [cell, cellId] : neighbors)
            map.RemoveBits(cellId, GetIdBit(CellStates::Ramp));

        for (int i = 0; i < 8; i+= 2)
        {
            auto&& [cell, cellId] = neighbors[i];
            if (cell & GetIdBit(CellStates::Wall))
            {
                map.SetCell(neighbors[i + 4].xyz, GetIdBit(CellStates::Floor));
                break;
            }
        }

        return true;
    }
};


/************************************************************************************************/


std::optional<std::reference_wrapper<GameObject>> AddWorldObject(GameWorld& world, AssetHandle handle, iAllocator& allocator)
{
    static const auto material = [] {
        auto& materials = MaterialComponent::GetComponent();
        auto material       = materials.CreateMaterial();
        materials.Add2Pass(material, ShadowMapPassID);
        materials.Add2Pass(material, GBufferPassID);

        return material;
    }();

    
    auto& object = world.objectPool.Allocate();
    static_vector<KeyValuePair> values = {
        { PhysicsLayerKID, &world.layer }
    };

    if (!LoadPrefab(object, handle, allocator, values))
        return {};

    auto n = GetSceneNode(object);
    auto m = GetStaticBodyNode(object);

    /*
    object.AddView<SceneNodeView<>>();
    object.AddView<BrushView>(mesh);
    */

    object.AddView<MaterialView>(material);
    //SetScale(object, float3{ 5.0f, 5.0f, 5.0f });

    world.scene.AddGameObject(object);
    SetBoundingSphereFromMesh(object);

    return object;
}


void TranslateChunk(MapChunk& chunk, SparseMap& map, GameWorld& world, const WorldAssets& assets, iAllocator& allocator)
{
    for (auto [c, cellXYZ] : chunk)
    {
        // For now I'm only doing the first layer
        if (cellXYZ[2] > 0)
            continue;

        const float scale = 2.0f;

        switch (c)
        {
            case GetIdBit(CellStates::Floor):
            {
                if(auto floorObject = AddWorldObject(world, assets.floor, allocator); floorObject)
                    StaticBodySetWorldPosition(*floorObject,
                        float3{
                            cellXYZ[0] * scale,
                            cellXYZ[2] * scale,
                            cellXYZ[1] * scale });
            }   break;
            case GetIdBit(CellStates::Ramp):
            {
                if (auto rampObject = AddWorldObject(world, assets.ramp, allocator); rampObject)
                {
                    auto neighbors = SparseMap::GetNeighborRing(map, cellXYZ);

                    for (int i = 0; i < 8; i += 2)
                    {
                        auto&& [cell, cellId] = neighbors[i];
                        if (cell == GetIdBit(CellStates::Wall) && neighbors[i + 4] == GetIdBit(CellStates::Floor))
                        {
                            Yaw(*rampObject, pi / 2 * -i);
                            break;
                        }
                    }

                    StaticBodySetWorldPosition(*rampObject,
                        float3{
                            cellXYZ[0] * scale,
                            cellXYZ[2] * scale,
                            cellXYZ[1] * scale });
                }
            }   break;
            case GetIdBit(CellStates::Wall):
            {
                if (auto wallObject = AddWorldObject(world, assets.wallXSegment, allocator); wallObject)
                {
                    StaticBodySetWorldPosition(*wallObject,
                        float3{
                            cellXYZ[0] * scale,
                            cellXYZ[2] * scale,
                            cellXYZ[1] * scale });
                }
            }   break;
            case GetIdBit(CellStates::Corner):
            {
                if (auto cornerObject = AddWorldObject(world, assets.wallXSegment, allocator); cornerObject)
                {
                    StaticBodySetWorldPosition(*cornerObject,
                        float3{
                            cellXYZ[0] * scale,
                            cellXYZ[2] * scale,
                            cellXYZ[1] * scale });
                }
            }   break;
        }
    }
}

SparseMap GenerateWorld(GameWorld& world, const WorldAssets& assets, iAllocator& temp)
{
    // Step 1. Generate world
    SparseMap map{ temp };
	ConstraintTable constraints;
    constraints[CellStates::Corner].emplace_back(std::make_unique<CornerConstraint>());
    constraints[CellStates::Floor].emplace_back(std::make_unique<FloorConstraint>());
    constraints[CellStates::Wall].emplace_back(std::make_unique<WallConstraint>());
    constraints[CellStates::Ramp].emplace_back(std::make_unique<RampConstraint>());

    map.Generate(constraints, int3{ 256, 256, 1 }, temp);

    std::cout << "Legend:\n";
    std::cout << "_ = Space\n";
    std::cout << "R = Ramp\n";
    std::cout << "X = Wall\n";
    std::cout << "C = Corner\n";

    auto chunk = map.GetChunk({0, 0, 0});
    if (chunk)
    {
        auto& chunk_ref = chunk.value().get();

        int x = 0;
        int y = 0;
        for (auto [c, _] : chunk_ref)
        {
            if      (c & GetIdBit(CellStates::Floor))
                    std::cout << "_ ";
            else if (c & GetIdBit(CellStates::Ramp))
                    std::cout << "R ";
            else if (c & GetIdBit(CellStates::Wall))
                    std::cout << "X ";
            if (c & GetIdBit(CellStates::Corner))
                std::cout << "C ";

            if (++x >= 8)
            {
                std::cout << "\n";

                y++;
                x = 0;
                if (y > 8)
                    break;
            }
        }
    }


    // Step 2. Translate map cells -> game world
    if (chunk)
        TranslateChunk(*chunk, map, world, assets, temp);

    return map;
}


/************************************************************************************************/


WorldAssets LoadBasicTiles()
{
    AddAssetFile(R"(assets\basicTiles.gameres)");
    WorldAssets out;
    out.cornerSegment   = 10870;
    out.wallEndSegment  = 20850;
    out.wallXSegment    = 200;
    out.wallYSegment    = 13491;
    out.wallISegment    = 27187;

    out.floor   = 29638;
    out.ramp    = 23581;

    return out;
}


void CreateMultiplayerScene(GameWorld& world, const WorldAssets& assets, iAllocator& allocator, iAllocator& tempAllocator)
{
#if 0
    static const GUID_t sceneID = 1234;
    world.LoadScene(sceneID);
#else
    static_vector<KeyValuePair> values;
    values.emplace_back(PhysicsLayerKID, &world.layer);

    auto& prefabObject = world.objectPool.Allocate();
    FlexKit::LoadPrefab(prefabObject, "PrefabGameObject", tempAllocator, {values});

    GenerateWorld(world, assets, tempAllocator);
#endif

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

    for (int I = 0; I < 0; I++)
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
