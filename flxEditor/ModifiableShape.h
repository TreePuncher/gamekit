#pragma once

#include "MathUtils.h"
#include "intersection.h"
#include <stdint.h>

/************************************************************************************************/


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

    Triangle                Offset(FlexKit::float3) const noexcept;
    FlexKit::AABB           GetAABB()               const noexcept;
    const FlexKit::float3   Normal()                const noexcept;
    const FlexKit::float3   TriPoint()              const noexcept;
    const FlexKit::float3   TriPoint(const FlexKit::float3 BaryCentricPoint) const noexcept;
};


/************************************************************************************************/


struct ModifiableShape
{
    struct wVertex
    {
        FlexKit::float3 point;

        void Serialize(auto& ar)
        {
            ar& point;
        }
    };

    struct wEdge // Half Edge, directed structure
    {
        uint32_t vertices[2];
        uint32_t oppositeNeighbor;
        uint32_t next;
        uint32_t prev;
        uint32_t face;
    };

    struct SubFace
    {
        uint32_t vertices[3];
        uint32_t edges[3];
    };

    struct wFace
    {
        uint32_t                edgeStart;
        std::vector<uint32_t>   polys;

        auto begin(const ModifiableShape* shape) const
        {
            return ConstFaceIterator{ shape, edgeStart };
        }

        auto end(const ModifiableShape* shape) const
        {
            auto end = ConstFaceIterator{ shape, edgeStart };
            end--;

            return end;
        }

        auto begin  (ModifiableShape* shape) { return FaceIterator{ shape, edgeStart }; }
        auto end    (ModifiableShape* shape)   { auto end = FaceIterator{ shape, edgeStart }; end--; return end; }

        void Serialize(auto& ar)
        {
            ar& edgeStart;
            ar& polys;
        }
    };

    class ConstFaceIterator
    {
    public:
        ConstFaceIterator(const ModifiableShape* IN_shape, uint32_t IN_end) noexcept
            : shape     { IN_shape }
            , endIdx    { IN_end }
            , current   { IN_end }
            , itr       { 0 } {}

        const ModifiableShape* shape;
        uint32_t    endIdx;
        uint32_t    current;
        int32_t     itr;

        FlexKit::float3 GetPoint(uint32_t vertex) const;
        bool            end() noexcept;
        bool            operator == (const ConstFaceIterator& rhs) const noexcept;
        const   wEdge*  operator -> () noexcept;

        ConstFaceIterator   operator +  (int rhs) const noexcept;
        ConstFaceIterator&  operator += (int rhs) noexcept;
        ConstFaceIterator   operator -  (int rhs) const noexcept;
        ConstFaceIterator&  operator -= (int rhs) noexcept;

        void Next() noexcept;
        void Prev() noexcept;

        ConstFaceIterator& operator ++(int) noexcept;
        ConstFaceIterator& operator --(int) noexcept;
    };

    class FaceIterator
    {
    public:
        FaceIterator(ModifiableShape* IN_shape, uint32_t IN_end) noexcept
            : shape     { IN_shape }
            , endIdx    { IN_end }
            , current   { IN_end }
            , itr       { 0 } {}

        ModifiableShape*     shape;
        uint32_t      endIdx;
        uint32_t      current;
        uint32_t      itr;

        FlexKit::float3 GetPoint(uint32_t vertex) const;
        bool            end() noexcept;

        bool            operator == (const FaceIterator& rhs) const noexcept;
        const wEdge*    operator -> ()              noexcept;
        FaceIterator    operator +  (int rhs) const noexcept;
        FaceIterator&   operator += (int rhs)       noexcept;
        FaceIterator    operator -  (int rhs) const noexcept;
        FaceIterator&   operator -= (int rhs)       noexcept;


        void Next() noexcept;
        void Prev() noexcept;


        FaceIterator& operator ++(int) noexcept;
        FaceIterator& operator --(int) noexcept;

    };

    using wEdgeList = std::vector<uint32_t>;

    std::vector<wVertex>    wVertices;
    std::vector<wEdgeList>  wVerticeEdges;
    std::vector<wEdge>      wEdges;
    std::vector<wFace>      wFaces;

    std::vector<Triangle>   tris; // Generated from the wing mesh representation

    std::vector<uint32_t> freeEdges;
    std::vector<uint32_t> freeFaces;


    uint32_t FindOpposingEdge(uint32_t V1, uint32_t V2) const noexcept;

    uint32_t AddVertex    (FlexKit::float3 point);
    uint32_t AddEdge      (uint32_t V1, uint32_t V2, uint32_t owningFace);
    uint32_t AddTri       (uint32_t V1, uint32_t V2, uint32_t V3);
    uint32_t AddPolygon   (const uint32_t* tri_start, const uint32_t* tri_end);

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
    std::vector<Triangle>   GetFaceGeometry (uint32_t faceIdx) const;
    std::vector<uint32_t>   GetFaceIndices  (uint32_t faceIdx) const;
    FlexKit::float3         GetFaceNormal   (uint32_t faceIdx) const;

    SubFace                 GetSubFace                  (uint32_t faceIdx, uint32_t faceSubIdx) const;
    uint32_t                GetVertexFromFaceLocalIdx   (uint32_t faceIdx, uint32_t faceSubIdx, uint32_t vertexIdx) const;
    std::vector<uint32_t>   GetFaceVertices             (uint32_t faceIdx) const;
    FlexKit::float3         GetFaceCenterPoint          (uint32_t faceIdx) const;

    uint32_t    NextEdge(const uint32_t edgeIdx) const;

    uint32_t    DuplicateFace(const uint32_t triId);
    void        RemoveFace(const uint32_t triId);

    void        SplitTri        (const uint32_t triId, const FlexKit::float3 BaryCentricPoint = FlexKit::float3{ 1.0f/3.0f });
    void        SplitEdge       (const uint32_t edgeId, const float U = 0.5f);
    void        RotateEdgeCCW   (const uint32_t edgeId);


    FlexKit::AABB                   GetAABB() const noexcept;
    FlexKit::AABB                   GetAABB(const FlexKit::float3 pos) const noexcept;
    const std::vector<Triangle>&    GetTris() const noexcept;
    FlexKit::BoundingSphere         GetBoundingVolume() const noexcept;

    struct IndexedMesh
    {
        std::vector<uint32_t>           indices;
        std::vector<FlexKit::float3>    points;
    };

    void Build();

    IndexedMesh CreateIndexedMesh() const;

    struct RayCast_result
    {
        FlexKit::float3 hitLocation; // barycentric intersection cordinate
        float           distance;
        uint32_t        faceIdx;
        uint32_t        faceSubIdx;
    };

    std::optional<RayCast_result>   RayCast(const FlexKit::Ray& r) const noexcept;


    bool dirty = false;


    void Serialize(auto& ar)
    {
        ar& wVertices;
        ar& wVerticeEdges;
        ar& wEdges;
        ar& wFaces;

        ar& freeEdges;
        ar& freeFaces;

        if(ar.Loading())
            Build();
    }
};


using FaceIterator      = ModifiableShape::FaceIterator;
using ConstFaceIterator = ModifiableShape::ConstFaceIterator;


/************************************************************************************************/
