#pragma once


#include <vector>
#include "type.h"
#include "..\FlexKitResourceCompiler\SceneResource.h"
#include "..\coreutilities\Components.h"


/************************************************************************************************/


constexpr uint32_t CSGComponentID = GetTypeGUID(CSGComponentID);


struct Triangle
{
    FlexKit::float3 position[3];

    void Serialize(auto& ar)
    {
        ar& position[0];
        ar& position[1];
        ar& position[2];
    }

            FlexKit::float3& operator [] (size_t idx) noexcept;
    const   FlexKit::float3& operator [] (size_t idx) const noexcept;

    Triangle        Offset(FlexKit::float3) const noexcept;
    FlexKit::AABB   GetAABB() const noexcept;
    const float3    Normal() const noexcept;
    const float3    TriPoint() const noexcept;
    const float3    TriPoint(const FlexKit::float3 BaryCentricPoint) const noexcept;
};

struct CSGShape
{
    struct wVertex
    {
        float3   point;
    };

    struct wEdge // Half Edge, directed structure
    {
        uint32_t vertices[2];
        uint32_t oppositeNeighbor;
        uint32_t next;
        uint32_t prev;
        uint32_t face;
    };

    struct wFace
    {
        uint32_t                edgeStart;
        std::vector<uint32_t>   polys;
    };

    using wEdgeList = std::vector<uint32_t>;

    std::vector<wVertex>    wVertices;
    std::vector<wEdgeList>  wVerticeEdges;
    std::vector<wEdge>      wEdges;
    std::vector<wFace>      wFaces;

    std::vector<Triangle>   tris; // Generated from the wing mesh representation

    uint32_t FindOpposingEdge(uint32_t V1, uint32_t V2) const noexcept;

    uint32_t AddVertex    (FlexKit::float3 point);
    uint32_t AddEdge      (uint32_t V1, uint32_t V2, uint32_t owningFace);
    uint32_t AddTri       (uint32_t V1, uint32_t V2, uint32_t V3);
    uint32_t AddPolygon   (uint32_t* tri_start, uint32_t* tri_end);

    uint32_t _AddEdge   ();
    uint32_t _AddVertex ();


    struct _SplitEdgeRes
    {
        uint32_t lowerEdge;
        uint32_t upperEdge;
    };

    uint32_t    _SplitEdge                 (const uint32_t edgeId,     const uint32_t vertexIdx);
    void        _RemoveVertexEdgeNeighbor  (const uint32_t vertexIdx,  const uint32_t edgeIdx);

    FlexKit::LineSegment    GetEdgeSegment  (uint32_t edgeId) const;
    Triangle                GetTri          (uint32_t triId) const;
    uint32_t                GetVertexFromFaceLocalIdx(uint32_t faceIdx, uint32_t vertexIdx) const;

    uint32_t    NextEdge(const uint32_t edgeIdx) const;

    void        SplitTri        (const uint32_t triId, const float3 BaryCentricPoint = float3{ 1.0f/3.0f });
    void        SplitEdge       (const uint32_t edgeId, const float U = 0.5f);
    void        RotateEdgeCCW   (const uint32_t edgeId);

    void Build();

    FlexKit::AABB                   GetAABB() const noexcept;
    FlexKit::AABB                   GetAABB(const float3 pos) const noexcept;
    const std::vector<Triangle>&    GetTris() const noexcept;
    FlexKit::BoundingSphere         GetBoundingVolume() const noexcept;

    struct RayCast_result
    {
        float3 hitLocation; // barycentric intersection cordinate
        float  distance;
        size_t triangleIdx;
    };

    std::optional<RayCast_result>   RayCast(const FlexKit::Ray& r) const noexcept;


    bool dirty = false;


    void Serialize(auto& ar)
    {
        ar& tris;
    }
};


/************************************************************************************************/


enum class CSG_OP
{
    CSG_ADD,
    CSG_SUB,
    CSG_INTERSECTION,
};


struct CSGBrush
{
    CSGShape                    shape;
    CSG_OP                      op;
    bool                        dirty = false;

    FlexKit::float3             position;
    FlexKit::Quaternion         orientation;
    FlexKit::float3             scale;

    std::shared_ptr<CSGBrush>   left;
    std::shared_ptr<CSGBrush>   right;

    struct TrianglePair
    {
        Triangle A;
        Triangle B;
    };

    struct RayCast_result
    {
        CSGShape*       shape;
        size_t          triIdx;
        float           distance;
        FlexKit::float3 BaryCentricResult;
    };

    std::optional<RayCast_result> RayCast(const FlexKit::Ray& r) const noexcept;

    std::vector<TrianglePair> intersections;

    bool                        IsLeaf() const noexcept;
    FlexKit::AABB               GetAABB() const noexcept;
    void                        Rebuild() noexcept;

    void Serialize(auto& ar)
    {
        ar& shape;
        ar& op;

        ar& position;
        ar& orientation;
        ar& scale;

        ar& left;
        ar& right;
    }
};


/************************************************************************************************/


class EditorComponentCSG :
    public FlexKit::Serializable<EditorComponentCSG, FlexKit::EntityComponent, CSGComponentID>
{
public:
    void Serialize(auto& ar)
    {
        EntityComponent::Serialize(ar);

        ar& brushes;
    }

    FlexKit::Blob GetBlob() override
    {
        return {};
    }

    std::vector<CSGBrush>   brushes;
};


/************************************************************************************************/


struct CSGComponentData
{
    std::vector<CSGBrush>   brushes;
    int32_t                 selectedBrush = -1;
    int                     debugVal1 = 0;
};


using CSGHandle     = FlexKit::Handle_t<32, CSGComponentID>;
using CSGComponent  = FlexKit::BasicComponent_t<CSGComponentData, CSGHandle, CSGComponentID>;
using CSGView       = CSGComponent::View;


/************************************************************************************************/


class EditorViewport;

struct Vertex
{
    FlexKit::float4 Position;
    FlexKit::float4 Color;
    FlexKit::float2 UV;
};


void                        RegisterComponentInspector(EditorViewport& viewport);
static_vector<Vertex, 24>   CreateWireframeCube(const float halfW);


/************************************************************************************************/
