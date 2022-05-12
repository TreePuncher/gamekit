#pragma once

#include "MathUtils.h"
#include "intersection.h"
#include <stdint.h>


namespace FlexKit
{   /************************************************************************************************/


struct Triangle
{
    float3 position[3];

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


struct RayTriIntersection_Res
{
    bool    res;
    float   d;
    float3  cord_bc;

    operator bool() const noexcept { return res; }
};

RayTriIntersection_Res Intersects(const FlexKit::Ray& r, const FlexKit::Triangle& tri) noexcept;


/************************************************************************************************/


struct EdgeSegment
{
    float3 A;
    float3 AColour;
    float3 B;
    float3 BColour;
};


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

    using wEdgeList = std::vector<uint32_t>;

    struct wFace
    {
        uint32_t                edgeStart;
        std::vector<uint32_t>   polys;


        struct _EdgeView
        {
            ModifiableShape*    shape;
            uint32_t            edgeStart;

            auto begin  () { return FaceIterator{ shape, edgeStart }; }
            auto end    () { auto end = FaceIterator{ shape, edgeStart }; end--; return end; }

            uint32_t Twin() const
            {
                shape->wEdges[edgeStart].oppositeNeighbor;
            }

            auto operator [] (int32_t idx)
            {
                return (begin() + idx);
            }
        };

        
        struct _EdgeViewConst
        {
            const ModifiableShape*  shape;
            uint32_t                edgeStart;

            auto begin  () const { return ConstFaceIterator{ shape, edgeStart }; }
            auto end    () const { auto end = ConstFaceIterator{ shape, edgeStart }; end--; return end; }
        };


        struct _NeighborFaceIterator
        {
            ModifiableShape*  shape;
            uint32_t          currentEdge;
            uint32_t          itr;

            struct Res
            {
                wFace&      face;
                uint32_t    faceIdx;
            };

            wFace* operator -> () noexcept;

            Res operator * () noexcept
            {
                auto faceIdx = shape->wEdges[currentEdge].face;
                return Res{ shape->wFaces[faceIdx], faceIdx };
            }

            bool operator == (_NeighborFaceIterator& rhs) noexcept;

            void Next() noexcept;
            void Prev() noexcept;

            _NeighborFaceIterator  operator +(int n) noexcept;

            _NeighborFaceIterator  operator ++() noexcept;
            _NeighborFaceIterator  operator --() noexcept;

            _NeighborFaceIterator& operator ++(int) noexcept;
            _NeighborFaceIterator& operator --(int) noexcept;
        };

        struct _FaceView
        {
            ModifiableShape* shape;
            uint32_t         edgeStart;
            uint32_t         offset;

            auto begin()    const
            {
                _NeighborFaceIterator itr{ shape, edgeStart };

                for(size_t I = 0; I < offset; I++)
                    itr++;

                return itr;
            }

            auto end()      const
            {
                auto start  = begin();
                auto end    = start;
                end++;

                while (end.currentEdge != 0xffffffff && start.currentEdge != end.currentEdge)
                    end++;

                return end;
            }


            auto operator [](int n)
            {
                auto itr = begin() + n;
                return *itr;
            }
        };


        struct _NeighborEdgeView
        {
            ModifiableShape*    shape;
            uint32_t            edge;
            int32_t             offset;

            struct Iterator
            {
                ModifiableShape* shape;
                uint32_t         current;
                uint32_t         itr;

                struct Res
                {
                    wVertex A;
                    wVertex B;

                    uint32_t A_idx;
                    uint32_t B_idx;
                };

                Res operator * ()
                {
                    auto& edge = shape->wEdges[current];

                    return {
                        .A = shape->wVertices[edge.vertices[0]].point,
                        .B = shape->wVertices[edge.vertices[1]].point,

                        .A_idx = edge.vertices[0],
                        .B_idx = edge.vertices[1]
                    };
                }

                void operator -- () { Prev(); }
                void operator ++ () { Next(); }

                void operator -- (int) { Prev(); }
                void operator ++ (int) { Next(); }

                bool operator == (const Iterator& rhs) const { return itr == rhs.itr; }

                operator bool() { return current != 0xffffffff; }

                uint32_t Twin() const { return shape->wEdges[current].oppositeNeighbor; }

                void Next()
                {
                    if (current == 0xffffffff)
                        return;

                    itr++;
                    auto prev  = shape->wEdges[current].prev;
                    current    = shape->wEdges[prev].oppositeNeighbor;
                }

                void Prev()
                {
                    if (current == 0xffffffff)
                        return;

                    itr--;
                    auto twin   = Twin();
                    current     = shape->wEdges[twin].next;
                }
            };

            auto begin()
            {
                Iterator itr{ shape, edge };

                bool forward = offset >= 0;
                for (size_t I = 0; I < abs(offset) && itr.Twin() != 0xffffffff; I++)
                    if (forward)
                        itr++;
                    else
                        itr--;

                return itr;
            }

            auto end()
            {
                auto start  = begin();
                auto end    = start;

                end++;

                while (end && end.Twin() != 0xffffffff && end.current != start.current)
                    ++end;

                return end;
            }

            auto operator [](int32_t i)
            {
                auto itr = begin();
                auto n = abs(i);

                if (i > 0)
                {
                    while(i--)
                        itr++;
                }
                else
                    while (i++ < 0)
                        itr--;

                return *itr;
            }
        };


        struct VertexIterator
        {
            ModifiableShape*    shape;
            uint32_t            current;
            uint32_t            itr;

            wEdgeList&  Edges()         { return shape->wVerticeEdges[shape->wEdges[current].vertices[0]]; }
            bool        IsEdge() const  { return shape->IsEdgeVertex(shape->wEdges[current].vertices[0]); }

            _NeighborEdgeView EdgeView()
            {
                return { shape, current };
            }

            _FaceView FaceView()
            {
                auto v = shape->wVerticeEdges[shape->wEdges[current].vertices[0]].front();
                return { shape, v };
            }

            bool operator == (const VertexIterator& rhs) const { return itr == rhs.itr; }

            void Next()
            {
                itr++;
                current = shape->wEdges[current].next;
            }

            void Prev()
            {
                itr--;
                current = shape->wEdges[current].prev;
            }

            struct res
            {
                const wVertex&      vertex;
                const uint32_t      vertexIdx;
                const wEdgeList&    edgeList;
                _NeighborEdgeView   edgeView;
            };

            res operator * ()
            {
                auto vertexIdx = shape->wEdges[current].vertices[0];

                return res{ shape->wVertices[vertexIdx], vertexIdx, shape->wVerticeEdges[vertexIdx], EdgeView()};
            }

            wVertex* operator -> () { return &shape->wVertices[shape->wEdges[current].vertices[0]]; }

            void operator -- () { Prev(); }
            void operator ++ () { Next(); }

            void operator -- (int) { Prev(); }
            void operator ++ (int) { Next(); }
        };


        struct ConstVertexIterator
        {
            const ModifiableShape*  shape;
            uint32_t                current;
            uint32_t                itr;

            struct res
            {
                const wVertex& vertex;
                const wEdgeList& edges;
            };

            res operator * ()
            {
                const auto vertexIdx = shape->wEdges[current].vertices[0];

                return res{ shape->wVertices[vertexIdx], shape->wVerticeEdges[vertexIdx] };
            }

            const wVertex* operator -> () { return &shape->wVertices[shape->wEdges[current].vertices[0]]; }

            void Next() 
            {
                itr++;
                current = shape->wEdges[current].next;
            }

            void Prev()
            {
                itr--;
                current = shape->wEdges[current].prev;
            }

            void operator -- (int) { Prev(); }
            void operator ++ (int) { Next(); }
        };


        struct _VertexView
        {
            ModifiableShape*    shape;
            wFace*              face;
            uint32_t            edgeStart;

            auto& Edges()
            {
                return shape->wVerticeEdges[shape->wEdges[edgeStart].vertices[0]];
            }

            _NeighborEdgeView EdgeView(int32_t offset = 0, int32_t edgeOffset = 0)
            {
                auto edge = edgeStart;
                for(size_t I = 0; I < edgeOffset; I++)
                    edge = shape->wEdges[edge].next;

                return { shape, edge, offset };
            }

            _FaceView FaceView(uint32_t offset = 0)
            {
                return { shape, edgeStart, offset };
            }

            bool IsExteriorCorner() const
            {
                auto& edges = shape->wVerticeEdges[shape->wEdges[edgeStart].vertices[0]];
                return edges.size() == 2;
            }


            bool IsInteriorCorner() const
            {
                auto& edges = shape->wVerticeEdges[shape->wEdges[edgeStart].vertices[0]];

                if (edges.size() > 2)
                {
                    for (auto& edge : shape->wEdges)
                        if (edge.oppositeNeighbor = 0xffffffff)
                            return true;
                }
                else
                    return false;
            }


            bool IsEdge() const
            {
                return shape->wVerticeEdges[shape->wEdges[edgeStart].vertices[0]].size() == 3;
            }


            auto begin()    { return VertexIterator{ shape, edgeStart, 0 }; }
            auto end()      { return VertexIterator{ shape, edgeStart, (uint32_t)face->GetEdgeCount(*shape) }; }
        };


        struct _VertexViewConst
        {
            const ModifiableShape*  shape;
            uint32_t                edgeStart;

            bool IsCorner() const
            {
                return shape->wVerticeEdges[shape->wEdges[edgeStart].vertices[0]].size() == 2;
            }

            auto begin()    const { return ConstVertexIterator{ shape, edgeStart }; }
            auto end()      const { auto end = ConstVertexIterator{ shape, edgeStart }; end--; return end; }
        };


        auto EdgeView    (ModifiableShape& shape, uint32_t offset = 0)
        {
            auto edge = edgeStart;
            for (size_t I = 0; I < offset; I++)
                edge = shape.wEdges[edge].next;

            return _EdgeView{ &shape, edge };
        }

        auto VertexView  (ModifiableShape& shape, uint32_t offset = 0)
        {
            auto edge = edgeStart;
            for(size_t I = 0; I < offset; I++)
                edge = shape.wEdges[edge].next;

            return _VertexView{ &shape, this, edge };
        }

        auto EdgeView    (const ModifiableShape& shape) const { return _EdgeViewConst    { &shape, edgeStart }; }
        auto VertexView  (const ModifiableShape& shape) const { return _VertexViewConst  { &shape, edgeStart }; }

        size_t GetEdgeCount(ModifiableShape& shape) const;

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

        const ModifiableShape*  shape;
        uint32_t                endIdx;
        uint32_t                current;
        int32_t                 itr;

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

        ModifiableShape*    shape;
        uint32_t            endIdx;
        uint32_t            current;
        uint32_t            itr;

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
        operator uint32_t () { return current; }


        FaceIterator& operator ++(int) noexcept;
        FaceIterator& operator --(int) noexcept;

    };

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

    float3                  GetEdgeMidPoint (uint32_t edgeId) const;
    EdgeSegment             GetEdgeSegment  (uint32_t edgeId) const;

    std::vector<Triangle>   GetFaceGeometry (uint32_t faceIdx) const;
    std::vector<uint32_t>   GetFaceIndices  (uint32_t faceIdx) const;
    float3                  GetFaceNormal   (uint32_t faceIdx) const;

    SubFace                 GetSubFace                  (uint32_t faceIdx, uint32_t faceSubIdx) const;
    uint32_t                GetVertexFromFaceLocalIdx   (uint32_t faceIdx, uint32_t faceSubIdx, uint32_t vertexIdx) const;
    std::vector<uint32_t>   GetFaceVertices             (uint32_t faceIdx) const;
    float3                  GetFaceCenterPoint          (uint32_t faceIdx) const;

    bool        IsEdgeVertex(const uint32_t vertexIdx) const;

    uint32_t    NextEdge(const uint32_t edgeIdx) const;

    uint32_t    DuplicateFace(const uint32_t triId);
    void        RemoveFace(const uint32_t triId);

    void        SplitTri        (const uint32_t triId, const float3 BaryCentricPoint = float3{ 1.0f/3.0f });
    void        SplitEdge       (const uint32_t edgeId, const float U = 0.5f);
    void        RotateEdgeCCW   (const uint32_t edgeId);


    AABB                            GetAABB() const noexcept;
    AABB                            GetAABB(const float3 pos) const noexcept;
    const std::vector<Triangle>&    GetTris() const noexcept;
    BoundingSphere                  GetBoundingVolume() const noexcept;

    struct IndexedMesh
    {
        std::vector<uint32_t>  indices;
        std::vector<float3>    points;
    };

    void Build();

    IndexedMesh CreateIndexedMesh() const;

    struct RayCast_result
    {
        float3      hitLocation; // barycentric intersection cordinate
        float       distance;
        uint32_t    faceIdx;
        uint32_t    faceSubIdx;
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


}   /************************************************************************************************/


/**********************************************************************

Copyright (c) 2015 - 2022 Robert May

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
