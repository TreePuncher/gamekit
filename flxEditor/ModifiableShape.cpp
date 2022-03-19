#include "PCH.h"
#include "ModifiableShape.h"


using FlexKit::float2;
using FlexKit::float3;
using FlexKit::float4;


/************************************************************************************************/


FlexKit::float3 ModifiableShape::ConstFaceIterator::GetPoint(uint32_t vertex) const
{
    return shape->wVertices[shape->wEdges[current].vertices[vertex]].point;
}


/************************************************************************************************/


bool ModifiableShape::ConstFaceIterator::end() noexcept
{
    return !(itr == 0 || current != endIdx);
}


/************************************************************************************************/


bool ModifiableShape::ConstFaceIterator::operator == (const ConstFaceIterator& rhs) const noexcept
{
    return current == rhs.current && shape == rhs.shape;
}


/************************************************************************************************/


const ModifiableShape::wEdge* ModifiableShape::ConstFaceIterator::operator -> () noexcept
{
    return &shape->wEdges[current];
}


/************************************************************************************************/


ModifiableShape::ConstFaceIterator ModifiableShape::ConstFaceIterator::operator + (int rhs) const noexcept
{
    ModifiableShape::ConstFaceIterator out{ shape, endIdx };

    for(size_t itr = 0; itr < rhs; itr++)
        out++;

    return out;
}


/************************************************************************************************/


ModifiableShape::ConstFaceIterator& ModifiableShape::ConstFaceIterator::operator += (int rhs) noexcept
{
    for (size_t itr = 0; itr < rhs; itr++)
        Next();

    return *this;
}


/************************************************************************************************/


ModifiableShape::ConstFaceIterator ModifiableShape::ConstFaceIterator::operator - (int rhs) const noexcept
{
    ModifiableShape::ConstFaceIterator out{ shape, endIdx };

    for (size_t itr = 0; itr < rhs; itr++)
        out--;

    return out;
}


/************************************************************************************************/


ModifiableShape::ConstFaceIterator& ModifiableShape::ConstFaceIterator::operator -= (int rhs) noexcept
{
    for (size_t itr = 0; itr < rhs; itr++)
        Prev();

    return *this;
}


/************************************************************************************************/


void ModifiableShape::ConstFaceIterator::Next() noexcept
{
    itr++;
    current = shape->wEdges[current].next;
}


/************************************************************************************************/


void ModifiableShape::ConstFaceIterator::Prev() noexcept
{
    itr--;
    current = shape->wEdges[current].prev;
}


/************************************************************************************************/


ModifiableShape::ConstFaceIterator& ModifiableShape::ConstFaceIterator::operator ++(int) noexcept
{
    Next();
    return *this;
}


/************************************************************************************************/


ModifiableShape::ConstFaceIterator& ModifiableShape::ConstFaceIterator::operator --(int) noexcept
{
    Prev();
    return *this;
}


/************************************************************************************************/


float3 ModifiableShape::FaceIterator::GetPoint(uint32_t vertex) const
{
    return shape->wVertices[vertex].point;
}


/************************************************************************************************/


bool ModifiableShape::FaceIterator::end() noexcept
{
    return !(itr == 0 || current != endIdx);
}


/************************************************************************************************/


bool ModifiableShape::FaceIterator::operator == (const FaceIterator& rhs) const noexcept
{
    return current == rhs.current && shape == rhs.shape;
}


/************************************************************************************************/


const ModifiableShape::wEdge* ModifiableShape::FaceIterator::operator -> () noexcept
{
    return &shape->wEdges[current];
}


/************************************************************************************************/


ModifiableShape::FaceIterator ModifiableShape::FaceIterator::operator + (int rhs) const noexcept
{
    FaceIterator out{ shape, endIdx };

    for(size_t itr = 0; itr < rhs; itr++)
        out++;

    return out;
}


/************************************************************************************************/


ModifiableShape::FaceIterator& ModifiableShape::FaceIterator::operator += (int rhs) noexcept
{
    for (size_t itr = 0; itr < rhs; itr++)
        Next();

    return *this;
}


/************************************************************************************************/


ModifiableShape::FaceIterator ModifiableShape::FaceIterator::operator - (int rhs) const noexcept
{
    FaceIterator out{ shape, endIdx };

    for (size_t itr = 0; itr < rhs; itr++)
        out--;

    return out;
}


/************************************************************************************************/


ModifiableShape::FaceIterator& ModifiableShape::FaceIterator::operator -= (int rhs) noexcept
{
    for (size_t itr = 0; itr < rhs; itr++)
        Prev();

    return *this;
}


/************************************************************************************************/


void ModifiableShape::FaceIterator::Next() noexcept
{
    itr++;
    current = shape->wEdges[current].next;
}


/************************************************************************************************/


void ModifiableShape::FaceIterator::Prev() noexcept
{
    itr--;
    current = shape->wEdges[current].prev;
}


/************************************************************************************************/


ModifiableShape::FaceIterator& ModifiableShape::FaceIterator::operator ++(int) noexcept
{
    Next();
    return *this;
}


/************************************************************************************************/


ModifiableShape::FaceIterator& ModifiableShape::FaceIterator::operator --(int) noexcept
{
    Prev();
    return *this;
}


/************************************************************************************************/


uint32_t ModifiableShape::AddVertex(FlexKit::float3 point)
{
    const auto idx = wVertices.size();
    wVertices.emplace_back(point);
    wVerticeEdges.emplace_back();

    return idx;
}


/************************************************************************************************/


uint32_t ModifiableShape::FindOpposingEdge(uint32_t V1, uint32_t V2) const noexcept
{
    for (auto idx : wVerticeEdges[V1])
    {
        auto& halfEdge = wEdges[idx];

        if (halfEdge.vertices[0] == V2 && halfEdge.vertices[1] == V1)
            return idx;
    }

    for (auto idx : wVerticeEdges[V2])
    {
        auto& halfEdge = wEdges[idx];

        if (halfEdge.vertices[0] == V2 && halfEdge.vertices[1] == V1)
            return idx;
    }

    return -1u;
}


uint32_t ModifiableShape::AddEdge(uint32_t V1, uint32_t V2, uint32_t owningFace)
{
    const auto idx              = wEdges.size();
    const uint32_t opposingEdge = FindOpposingEdge(V1, V2);

    if(opposingEdge != -1u)
        wEdges[opposingEdge].oppositeNeighbor = wEdges.size();

    wVerticeEdges[V1].push_back(idx);
    wVerticeEdges[V2].push_back(idx);

    wEdges.emplace_back(wEdge{ { V1, V2 }, opposingEdge, -1u, -1u, owningFace });

    return idx;
}


/************************************************************************************************/


uint32_t ModifiableShape::AddTri(uint32_t V1, uint32_t V2, uint32_t V3)
{
    const auto idx = wFaces.size();

    auto E1 = AddEdge(V1, V2, idx);
    auto E2 = AddEdge(V2, V3, idx);
    auto E3 = AddEdge(V3, V1, idx);

    wEdges[E1].next = E2;
    wEdges[E2].next = E3;
    wEdges[E3].next = E1;

    wEdges[E1].prev = E3;
    wEdges[E2].prev = E2;
    wEdges[E3].prev = E1;

    wFaces.emplace_back(wFace{ E1, {} });

    return idx;
}


/************************************************************************************************/


uint32_t ModifiableShape::_AddEdge()
{
    const uint32_t idx = wEdges.size();
    wEdges.emplace_back(wEdge{ { -1u, -1u }, -1u, -1u });

    return idx;
}


/************************************************************************************************/


uint32_t ModifiableShape::_AddVertex()
{
    const uint32_t idx = wVertices.size();
    wVertices.emplace_back();
    wVerticeEdges.emplace_back();

    return idx;
}


/************************************************************************************************/


uint32_t ModifiableShape::AddPolygon(const uint32_t* vertexStart, const uint32_t* vertexEnd)
{
    uint32_t firstEdge      = -1;
    uint32_t previousEdge   = -1;
    const auto faceIdx = wFaces.size();

    for (auto itr = vertexStart; itr != vertexEnd; itr++)
    {
        uint32_t edge = -1u;
        if (itr + 1 != vertexEnd)
        {
            edge = AddEdge(itr[0], itr[1], faceIdx);
            wEdges[edge].prev = previousEdge;

            if (previousEdge != -1)
                wEdges[previousEdge].next = edge;
            else
                firstEdge = edge;
        }
        else
        {
            edge = AddEdge(
                wEdges[previousEdge].vertices[1],
                wEdges[firstEdge].vertices[0],
                faceIdx);

            wEdges[edge].prev           = previousEdge;
            wEdges[edge].next           = firstEdge;

            wEdges[firstEdge].prev      = edge;
            wEdges[previousEdge].next   = edge;
        }

        previousEdge = edge;
    }


    wFaces.emplace_back(wFace{ firstEdge, {} });

    return faceIdx;
}


/************************************************************************************************/


FlexKit::LineSegment ModifiableShape::GetEdgeSegment(uint32_t edgeId) const
{
    const auto vertices = wEdges[edgeId].vertices;

    return {
        .A = wVertices[vertices[0]].point,
        .B = wVertices[vertices[1]].point,
    };
}


/************************************************************************************************/


std::vector<Triangle> ModifiableShape::GetFaceGeometry(uint32_t faceIdx) const
{
    auto indices = GetFaceIndices(faceIdx);
    std::vector<Triangle> geometry;

    for(auto itr = indices.begin(); itr < indices.end(); itr += 3)
    {
        auto P1 = wVertices[*(itr + 0)].point;
        auto P2 = wVertices[*(itr + 1)].point;
        auto P3 = wVertices[*(itr + 2)].point;

        geometry.push_back(
            Triangle{
                P1,
                P2,
                P3
            });
    }

    return geometry;
}


/************************************************************************************************/


std::vector<uint32_t> ModifiableShape::GetFaceIndices(uint32_t faceIdx) const
{
    std::vector<uint32_t> indices;

    const auto& face = wFaces[faceIdx];

    if (face.edgeStart == -1)
        return {};

    ConstFaceIterator itr = face.begin(this);
    ConstFaceIterator end = face.end(this);

    const auto E1   = itr.current;
    auto P1         = wEdges[E1].vertices[0];

    itr++;

    while(itr != end)
    {
        const auto E2 = itr.current;
        const auto E3 = itr->next;

        auto P2 = wEdges[E2].vertices[0];
        auto P3 = wEdges[E3].vertices[0];

        indices.push_back(P1);
        indices.push_back(P2);
        indices.push_back(P3);
        itr++;
    }

    return indices;
}


/************************************************************************************************/


FlexKit::float3 ModifiableShape::GetFaceNormal(uint32_t faceIdx) const
{
    const auto& face = wFaces[faceIdx];

    const auto E1   = face.edgeStart;
    auto P1         = wVertices[wEdges[E1].vertices[0]].point;

    ConstFaceIterator itr{ this, face.edgeStart };
    ConstFaceIterator end{ this, face.edgeStart };

    end = end - 2;

    FlexKit::float3 normal{ 0 };

    while(itr != end)
    {
        const auto E2 = itr->next;
        const auto E3 = wEdges[E2].next;

        auto P2 = wVertices[wEdges[E2].vertices[0]].point;
        auto P3 = wVertices[wEdges[E3].vertices[0]].point;

        Triangle T1{ P1, P2, P3 };
        normal += T1.Normal();

        itr++;
    }

    return normal.normal();
}


/************************************************************************************************/


ModifiableShape::SubFace ModifiableShape::GetSubFace(uint32_t faceIdx, uint32_t faceSubIdx) const
{
    const auto& face = wFaces[faceIdx];

    const auto E1   = face.edgeStart;
    auto P1         = wEdges[E1].vertices[0];

    ConstFaceIterator itr{ this, face.edgeStart };
    
    itr += faceSubIdx;
    const auto E2 = itr->next;
    const auto E3 = wEdges[E2].next;

    const auto P2 = wEdges[E2].vertices[0];
    const auto P3 = wEdges[E3].vertices[0];

    return { { P1, P2, P3 }, { E1, E2, E3 } };
}


uint32_t ModifiableShape::GetVertexFromFaceLocalIdx(uint32_t faceIdx, uint32_t faceSubIdx, uint32_t vertexIdx) const
{
    const auto& face = wFaces[faceIdx];

    const auto E1   = face.edgeStart;
    auto P1         = wEdges[E1].vertices[0];

    ConstFaceIterator itr{ this, face.edgeStart };
    
    itr += faceSubIdx;
    const auto E2 = itr->next;
    const auto E3 = wEdges[E2].next;

    uint32_t vertices[3] =
    {
        P1,
        vertices[1] = wEdges[E2].vertices[0],
        vertices[2] = wEdges[E3].vertices[0],
    };

    return vertices[vertexIdx];
}


/************************************************************************************************/


std::vector<uint32_t> ModifiableShape::GetFaceVertices(uint32_t faceIdx) const
{

    const auto start = wFaces[faceIdx].edgeStart;
    auto edge = start;

    std::vector<uint32_t> out;
    for (size_t I = 0; (I == 0 || edge != start); ++I)
    {
        edge = wEdges[edge].next;
        out.push_back(wEdges[edge].vertices[0]);
    }

    return out;
}


/************************************************************************************************/


FlexKit::float3 ModifiableShape::GetFaceCenterPoint(uint32_t faceIdx) const
{
    FlexKit::float3 point{ 0 };
    size_t vertexCount = 0;

    
    ConstFaceIterator itr{ this, wFaces[faceIdx].edgeStart };
    while (!itr.end())
    {
        point += itr.GetPoint(0);
        vertexCount++;
        itr++;
    }

    return point / vertexCount;
}


/************************************************************************************************/


uint32_t ModifiableShape::NextEdge(const uint32_t edgeIdx) const
{
    return wEdges[edgeIdx].next;
}


/************************************************************************************************/


void ModifiableShape::_RemoveVertexEdgeNeighbor(const uint32_t vertexIdx, const uint32_t edgeIdx)
{
    auto& vertexNeighbor = wVerticeEdges[vertexIdx];

    vertexNeighbor.erase(
        std::remove_if(
            vertexNeighbor.begin(),
            vertexNeighbor.end(),
            [&](auto& lhs) { return lhs == edgeIdx; }),
        vertexNeighbor.end());
}


/************************************************************************************************/


uint32_t ModifiableShape::DuplicateFace(const uint32_t faceIdx)
{
    std::vector<uint32_t> vertices;

    auto& face = wFaces[faceIdx];

    for(FaceIterator itr{ this, face.edgeStart }; !itr.end(); itr++)
        vertices.push_back(itr->vertices[0]);

    std::vector<uint32_t> newVertices;
    for (auto vidx : vertices)
    {
        auto newVertex = AddVertex(wVertices[vidx].point);
        newVertices.push_back(newVertex);
    }

    return AddPolygon(newVertices.data(), newVertices.data() + newVertices.size());
}


/************************************************************************************************/


void ModifiableShape::RemoveFace(const uint32_t faceIdx)
{
    freeFaces.push_back(faceIdx);

    auto itr = wFaces[faceIdx].begin(this);

    for (; !itr.end(); itr++)
    {
        auto neighbor = wEdges[itr.current].oppositeNeighbor;
        if(neighbor != -1u)
            wEdges[neighbor].oppositeNeighbor = -1u;

        freeEdges.push_back(itr.current);
    }

    wFaces[faceIdx].edgeStart = -1;
}


/************************************************************************************************/


void ModifiableShape::SplitTri(uint32_t triId, const float3 BaryCentricPoint)
{
    const auto geometry = GetFaceGeometry(triId);
    const auto newPoint = geometry.front().TriPoint(BaryCentricPoint);

    const auto E0 = wFaces[triId].edgeStart;
    const auto E1 = wEdges[E0].next;
    const auto E2 = wEdges[E1].next;

    const auto V0 = wEdges[E0].vertices[0];
    const auto V1 = wEdges[E1].vertices[0];
    const auto V2 = wEdges[E2].vertices[0];
    const auto V3 = AddVertex(newPoint);

    const auto E3 = (uint32_t)wEdges.size();    wEdges.emplace_back();
    const auto E4 = E3 + 1;                     wEdges.emplace_back();
    const auto E5 = E3 + 2;                     wEdges.emplace_back();
    const auto E6 = E3 + 3;                     wEdges.emplace_back();
    const auto E7 = E3 + 4;                     wEdges.emplace_back();
    const auto E8 = E3 + 5;                     wEdges.emplace_back();

    wEdges[E0].next = E5;
    wEdges[E1].next = E8;
    wEdges[E2].next = E3;

    wEdges[E3].next         = E4;
    wEdges[E3].vertices[0]  = V0;
    wEdges[E3].vertices[1]  = V3;

    wEdges[E4].prev         = E3;
    wEdges[E4].next         = E2;
    wEdges[E4].vertices[0]  = V3;
    wEdges[E4].vertices[1]  = V2;

    wEdges[E5].prev         = E0;
    wEdges[E5].next         = E6;
    wEdges[E5].vertices[0]  = V1;
    wEdges[E5].vertices[1]  = V3;

    wEdges[E6].prev         = E5;
    wEdges[E6].next         = E0;
    wEdges[E6].vertices[0]  = V3;
    wEdges[E6].vertices[1]  = V0;

    wEdges[E7].prev         = E8;
    wEdges[E7].next         = E1;
    wEdges[E7].vertices[0]  = V3;
    wEdges[E7].vertices[1]  = V1;

    wEdges[E8].prev         = E1;
    wEdges[E8].next         = E7;
    wEdges[E8].vertices[0]  = V2;
    wEdges[E8].vertices[1]  = V3;

    wEdges[E3].oppositeNeighbor = E6;
    wEdges[E4].oppositeNeighbor = E8;
    wEdges[E5].oppositeNeighbor = E7;
    wEdges[E6].oppositeNeighbor = E3;
    wEdges[E7].oppositeNeighbor = E5;
    wEdges[E8].oppositeNeighbor = E4;

    wVerticeEdges[V0].push_back(E3);

    wVerticeEdges[V1].push_back(E5);
    wVerticeEdges[V1].push_back(E7);

    wVerticeEdges[V2].push_back(E4);
    wVerticeEdges[V2].push_back(E8);

    wVerticeEdges[V3].push_back(E3);
    wVerticeEdges[V3].push_back(E4);
    wVerticeEdges[V3].push_back(E5);
    wVerticeEdges[V3].push_back(E6);
    wVerticeEdges[V3].push_back(E7);
    wVerticeEdges[V3].push_back(E8);

    wFaces.push_back({E1});
    wFaces.push_back({E2});

    Build();
}


/************************************************************************************************/


uint32_t ModifiableShape::_SplitEdge(const uint32_t edgeId, const uint32_t V4)
{
    const uint32_t E1 = edgeId;
    const uint32_t E2 = wEdges[E1].next;
    const uint32_t E3 = wEdges[E2].next;
    const uint32_t E4 = _AddEdge();
    const uint32_t E5 = _AddEdge();
    const uint32_t E6 = _AddEdge();

    const uint32_t V1 = wEdges[E1].vertices[0];
    const uint32_t V2 = wEdges[E2].vertices[0];
    const uint32_t V3 = wEdges[E3].vertices[0];

    const uint32_t oldFaceIdx = wEdges[E1].face;
    const uint32_t newFaceIdx = wFaces.size();

    // Lower triangle
    // { V1, V4, V3 }
    _RemoveVertexEdgeNeighbor(wEdges[E1].vertices[1], E1);
    wEdges[E1].vertices[1]  = V4; // { V1, V4 }
    wEdges[E1].next         = E4;

    wEdges[E3].prev         = E4;

    wEdges[E4].vertices[0]      = V4;
    wEdges[E4].vertices[1]      = V3;
    wEdges[E4].prev             = E1;
    wEdges[E4].next             = E3;
    wEdges[E4].face             = oldFaceIdx;
    wEdges[E4].oppositeNeighbor = E5;

    wFaces[oldFaceIdx].edgeStart = E1;

    // Upper triangle
    // From Split point to top of triangle
    wEdges[E2].prev = E6;
    wEdges[E2].next = E5;
    wEdges[E2].face = newFaceIdx;

    wEdges[E5].vertices[0]      = V3;
    wEdges[E5].vertices[1]      = V4;
    wEdges[E5].prev             = E2;
    wEdges[E5].next             = E6;
    wEdges[E5].oppositeNeighbor = E4;
    wEdges[E5].face             = newFaceIdx;


    wEdges[E6].vertices[0]      = V4;
    wEdges[E6].vertices[1]      = V2;
    wEdges[E6].prev             = E5;
    wEdges[E6].next             = E2;
    wEdges[E6].face             = newFaceIdx;

    wVerticeEdges[V4].push_back(E1);
    wVerticeEdges[V4].push_back(E4);
    wVerticeEdges[V4].push_back(E5);
    wVerticeEdges[V4].push_back(E6);

    wFaces.push_back(wFace{ E2, {} });

    return E6;
}


/************************************************************************************************/


void ModifiableShape::SplitEdge(const uint32_t edgeIdx, const float U)
{
    const uint32_t V1   = wEdges[edgeIdx].vertices[0];
    const uint32_t V2   = wEdges[edgeIdx].vertices[1];
    const float3 A      = wVertices[V1].point;
    const float3 B      = wVertices[V2].point;
    const float3 C      = FlexKit::Lerp(A, B, U);
    const uint32_t V4   = _AddVertex();
    wVertices[V4].point = C;

    const auto E1 = _SplitEdge(edgeIdx, V4);

    if (auto neighbor = wEdges[edgeIdx].oppositeNeighbor; neighbor != -1u)
    {
        auto E2 = _SplitEdge(neighbor, V4);

        wEdges[edgeIdx].oppositeNeighbor    = E2;
        wEdges[neighbor].oppositeNeighbor   = E1;
        wEdges[E1].oppositeNeighbor         = neighbor;
        wEdges[E2].oppositeNeighbor         = edgeIdx;
    }
}


/************************************************************************************************/


void ModifiableShape::RotateEdgeCCW(const uint32_t E1)
{
    if (wEdges[E1].oppositeNeighbor == -1)
        return;

    const auto E2 = wEdges[E1].next;
    const auto E3 = wEdges[E2].next;
    const auto E4 = wEdges[E1].oppositeNeighbor;
    const auto E5 = wEdges[E4].next;
    const auto E6 = wEdges[E5].next;

    const auto V1 = wEdges[E1].vertices[0];
    const auto V2 = wEdges[E2].vertices[0];
    const auto V3 = wEdges[E3].vertices[0];
    const auto V4 = wEdges[E6].vertices[0];

    const auto P1 = wEdges[E1].face;
    const auto P2 = wEdges[E4].face;
    
    _RemoveVertexEdgeNeighbor(wEdges[E1].vertices[0], E1);
    _RemoveVertexEdgeNeighbor(wEdges[E1].vertices[1], E1);
    wEdges[E1].vertices[0]  = V4;
    wEdges[E1].vertices[1]  = V3;
    wEdges[E1].next         = E3;
    wEdges[E1].prev         = E5;
    wVerticeEdges[V2].push_back(E1);
    wVerticeEdges[V4].push_back(E1);

    wEdges[E3].prev = E1;
    wEdges[E3].next = E5;
    wEdges[E3].face = P1;

    wEdges[E5].prev = E5;
    wEdges[E5].next = E1;
    wEdges[E5].face = P1;

    _RemoveVertexEdgeNeighbor(wEdges[E4].vertices[0], E4);
    _RemoveVertexEdgeNeighbor(wEdges[E4].vertices[1], E4);
    wEdges[E4].vertices[0]  = V3;
    wEdges[E4].vertices[1]  = V4;
    wEdges[E4].next         = E6;
    wEdges[E4].prev         = E2;
    wVerticeEdges[V2].push_back(E4);
    wVerticeEdges[V4].push_back(E4);

    wEdges[E6].prev = E4;
    wEdges[E6].next = E2;
    wEdges[E6].face = P2;

    wEdges[E2].prev = E6;
    wEdges[E2].next = E4;
    wEdges[E2].face = P2;

    wFaces[P1].edgeStart = E1;
    wFaces[P2].edgeStart = E4;
}


/************************************************************************************************/


void ModifiableShape::Build()
{
    tris.clear();

    for (size_t faceIdx = 0; faceIdx < wFaces.size(); faceIdx++)
    {
        auto geometry = GetFaceGeometry(faceIdx);

        for(auto t : geometry)
            tris.emplace_back(t);
    }
}


/************************************************************************************************/


ModifiableShape::IndexedMesh ModifiableShape::CreateIndexedMesh() const
{
    IndexedMesh out;

    for (size_t faceIdx = 0; faceIdx < wFaces.size(); faceIdx++)
    {
        auto geometry = GetFaceIndices(faceIdx);

        for (auto t : geometry)
            out.indices.emplace_back(t);
    }

    for (auto v : wVertices)
        out.points.push_back(v.point);

    return out;
}


/************************************************************************************************/


const std::vector<Triangle>& ModifiableShape::GetTris() const noexcept
{
    return tris;
}


/************************************************************************************************/


FlexKit::BoundingSphere ModifiableShape::GetBoundingVolume() const noexcept
{
    return { 0, 0, 0, 1 };
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
