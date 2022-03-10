#include "PCH.h"
#include "CSGComponent.h"
#include "ViewportScene.h"
#include "EditorInspectorView.h"
#include "qlistwidget.h"
#include "EditorViewport.h"
#include <QtWidgets\qlistwidget.h>
#include <ImGuizmo.h>


/************************************************************************************************/


uint32_t CSGShape::AddVertex(FlexKit::float3 point)
{
    const auto idx = wVertices.size();
    wVertices.emplace_back(point);

    return idx;
}


/************************************************************************************************/


uint32_t CSGShape::QueryEdge(uint32_t V1, uint32_t V2) const
{
    for (size_t idx = 0; idx < wEdges.size(); ++idx)
    {
        auto& halfEdge = wEdges[idx];

        if (halfEdge.vertices[0] == V2 && halfEdge.vertices[1] == V1)
            return idx;
    }

    return -1u;
}


uint32_t CSGShape::AddEdge(uint32_t V1, uint32_t V2)
{
    const auto idx = wEdges.size();
    wVertices[V1].edges.push_back(idx);
    wVertices[V2].edges.push_back(idx);

    wVerticeEdges.emplace_back();
    wVerticeEdges.emplace_back();

    const uint32_t opposingEdge = QueryEdge(V1, V2);
    if(opposingEdge != -1u)
        wEdges[opposingEdge].oppositeNeighbor = wEdges.size();

    wEdges.emplace_back(wEdge{ { V1, V2 }, opposingEdge, -1u, -1u });

    return idx;
}


/************************************************************************************************/


uint32_t CSGShape::AddTri(uint32_t V1, uint32_t V2, uint32_t V3)
{
    const auto idx = wFaces.size();

    uint32_t edges[3] = { -1u, -1u, -1u };

    auto E1 = AddEdge(V1, V2);
    auto E2 = AddEdge(V2, V3);
    auto E3 = AddEdge(V3, V1);

    wEdges[E1].next = E2;
    wEdges[E2].next = E3;
    wEdges[E3].next = E1;

    wEdges[E1].previous = E3;
    wEdges[E2].previous = E2;
    wEdges[E3].previous = E1;

    wFaces.emplace_back(wFace{ E1, {} });

    return idx;
}


/************************************************************************************************/


uint32_t CSGShape::AddPolygon(uint32_t* tri_start, uint32_t* tri_end)
{
    // TODO
    return -1;
}

Triangle CSGShape::GetTri(uint32_t polyId) const
{
    const auto& face = wFaces[polyId];

    const auto E1 = face.edgeStart;
    const auto E2 = wEdges[E1].next;
    const auto E3 = wEdges[E2].next;

    auto P1 = wVertices[wEdges[E1].vertices[0]].point;
    auto P2 = wVertices[wEdges[E2].vertices[0]].point;
    auto P3 = wVertices[wEdges[E3].vertices[0]].point;

    return Triangle{ P1, P2, P3 };
}


void CSGShape::Build()
{
    tris.clear();

    for (size_t faceIdx = 0; faceIdx < wFaces.size(); faceIdx++)
        tris.emplace_back(GetTri(faceIdx));
}


/************************************************************************************************/


static_vector<Vertex, 24> CreateWireframeCube(const float halfW)
{
    using FlexKit::float2;
    using FlexKit::float3;
    using FlexKit::float4;


    const static_vector<Vertex, 24> vertices = {
        // Top
        { float4{ -halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        { float4{ -halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        { float4{ -halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{ -halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },


        { float4{  halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        // Bottom
        { float4{ -halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        { float4{ -halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        { float4{ -halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{ -halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        { float4{  halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        // Sides
        { float4{  halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        { float4{  halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },


        { float4{ -halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{ -halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },


        { float4{ -halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{ -halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
    };

    return vertices;
}


std::vector<Vertex> CreateWireframeCube2(const float halfW)
{
    using FlexKit::float2;
    using FlexKit::float3;
    using FlexKit::float4;

    const std::vector<Vertex> vertices = {
        // Top
        { float4{ -halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        { float4{ -halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        { float4{ -halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{ -halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },


        { float4{  halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        // Bottom
        { float4{ -halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        { float4{ -halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        { float4{ -halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{ -halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        { float4{  halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        // Sides
        { float4{  halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        { float4{  halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },


        { float4{ -halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{ -halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },


        { float4{ -halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{ -halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
    };

    return vertices;
}


/************************************************************************************************/


Triangle Triangle::Offset(FlexKit::float3 offset) const noexcept
{
    Triangle tri_out;

    for (size_t I = 0; I < 3; I++)
        tri_out[I] = position[I] + offset;

    return tri_out;
}


FlexKit::float3& Triangle::operator [] (size_t idx) noexcept
{
    return position[idx];
}


const FlexKit::float3& Triangle::operator [] (size_t idx) const noexcept
{
    return position[idx];
}


FlexKit::AABB Triangle::GetAABB() const noexcept
{
    FlexKit::AABB aabb;
    aabb += position[0];
    aabb += position[1];
    aabb += position[2];

    return aabb;
}


const float3 Triangle::Normal() const noexcept
{
    return FlexKit::TripleProduct(position[0], position[1], position[2]);
}


const float3 Triangle::TriPoint() const noexcept
{
    return (position[0] + position[1] + position[2]) / 3;
}


FlexKit::AABB CSGShape::GetAABB() const noexcept
{
    FlexKit::AABB aabb;

    for (const auto& v : tris)
    {
        aabb += v.position[0];
        aabb += v.position[1];
        aabb += v.position[2];
    }

    return aabb;
}


FlexKit::AABB CSGShape::GetAABB(const float3 pos) const noexcept
{
    FlexKit::AABB aabb;

    for (const auto& v : tris)
    {
        aabb += v.position[0];
        aabb += v.position[1];
        aabb += v.position[2];
    }
    
    return aabb.Offset(pos);
}


/*
CSGShape CreateCubeCSGShape() noexcept
{
    CSGShape shape;

    const float3 dimensions = float3{ 1.0f, 1.0f, 1.0f } / 2.0f;
    // Top
    shape.tris.push_back(
        {   float3{ dimensions.x, dimensions.y, dimensions.z },
            float3{-dimensions.x, dimensions.y,-dimensions.z },
            float3{-dimensions.x, dimensions.y, dimensions.z },
        });
    shape.tris.push_back(
        {   float3{ dimensions.x, dimensions.y, dimensions.z },
            float3{ dimensions.x, dimensions.y,-dimensions.z },
            float3{-dimensions.x, dimensions.y,-dimensions.z },
        });

    // Bottom
    shape.tris.push_back(
        {   float3{  dimensions.x,-dimensions.y, dimensions.z },
            float3{ -dimensions.x,-dimensions.y, dimensions.z },
            float3{ -dimensions.x,-dimensions.y,-dimensions.z },
        });
    shape.tris.push_back(
        {   float3{  dimensions.x,-dimensions.y, dimensions.z },
            float3{ -dimensions.x,-dimensions.y,-dimensions.z },
            float3{  dimensions.x,-dimensions.y,-dimensions.z },
        });

    // Right
    shape.tris.push_back(
        {   float3{-dimensions.x, dimensions.y, dimensions.z },
            float3{-dimensions.x, dimensions.y,-dimensions.z },
            float3{-dimensions.x,-dimensions.y,-dimensions.z }
        });
    shape.tris.push_back(
        {   float3{ -dimensions.x, dimensions.y, dimensions.z },
            float3{ -dimensions.x,-dimensions.y,-dimensions.z },
            float3{ -dimensions.x,-dimensions.y, dimensions.z }
        });

    // Left
    shape.tris.push_back(
        {   float3{ dimensions.x, dimensions.y, dimensions.z },
            float3{ dimensions.x,-dimensions.y,-dimensions.z },
            float3{ dimensions.x, dimensions.y,-dimensions.z }
        });
    shape.tris.push_back(
        {   float3{ dimensions.x, dimensions.y, dimensions.z },
            float3{ dimensions.x,-dimensions.y, dimensions.z },
            float3{ dimensions.x,-dimensions.y,-dimensions.z }
        });

    // Front
    shape.tris.push_back(
        {   float3{ dimensions.x, dimensions.y, -dimensions.z },
            float3{ dimensions.x,-dimensions.y, -dimensions.z },
            float3{-dimensions.x,-dimensions.y, -dimensions.z }
        }
    );
    shape.tris.push_back(
        {   float3{ dimensions.x, dimensions.y, -dimensions.z },
            float3{-dimensions.x,-dimensions.y, -dimensions.z },
            float3{-dimensions.x, dimensions.y, -dimensions.z }
        });

    // Back
    shape.tris.push_back(
        {   float3{  dimensions.x,-dimensions.y, dimensions.z },
            float3{  dimensions.x, dimensions.y, dimensions.z },
            float3{ -dimensions.x,-dimensions.y, dimensions.z }
        }
    );
    shape.tris.push_back(
        {   float3{ -dimensions.x,-dimensions.y, dimensions.z },
            float3{  dimensions.x, dimensions.y, dimensions.z },
            float3{ -dimensions.x, dimensions.y, dimensions.z }
        });

    return shape;
}
*/


CSGShape CreateCubeCSGShape() noexcept
{
    CSGShape cubeShape;

    const uint32_t V1 = cubeShape.AddVertex({ -1,  1,  1 }); 
    const uint32_t V2 = cubeShape.AddVertex({  1,  1,  1 }); 
    const uint32_t V3 = cubeShape.AddVertex({  1,  1, -1 }); 
    const uint32_t V4 = cubeShape.AddVertex({ -1,  1, -1 }); 

    const uint32_t V5 = cubeShape.AddVertex({ -1, -1,  1 });
    const uint32_t V6 = cubeShape.AddVertex({  1, -1,  1 });
    const uint32_t V7 = cubeShape.AddVertex({  1, -1, -1 });
    const uint32_t V8 = cubeShape.AddVertex({ -1, -1, -1 });


    // Top
    const uint32_t T1 = cubeShape.AddTri(V1, V2, V3);
    const uint32_t T2 = cubeShape.AddTri(V1, V3, V4);

    // Right
    const uint32_t T3 = cubeShape.AddTri(V3, V2, V6);
    const uint32_t T4 = cubeShape.AddTri(V3, V6, V7);

    cubeShape.Build();

    return cubeShape;
}


/************************************************************************************************/


bool CSGBrush::IsLeaf() const noexcept
{
    return (left == nullptr && right == nullptr);
}


FlexKit::AABB CSGBrush::GetAABB() const noexcept
{
    return shape.GetAABB(position);
}


/************************************************************************************************/


struct IntersectionPoints
{
    float3 A;
    float3 B;
};


float ProjectPointOntoRay(const FlexKit::float3& vector, const FlexKit::Ray& r) noexcept
{
    return (vector - r.O).dot(r.D);
}


bool Intersects(const float2 A_0, const float2 A_1, const float2 B_0, const float2 B_1, float2& intersection) noexcept
{
    const auto S0 = (A_1 - A_0);
    const auto S1 = (B_1 - B_0);

    if ((S1.y / S1.x - S0.y / S0.x) < 0.000001f)
        return false; // Parallel

    const float C = S0.x * S1.y - S1.y + S1.x;
    const float A = A_0.x * A_1.y - A_0.y * A_1.x;
    const float B = B_0.x * B_1.y - B_0.y * B_1.x;

    const float x = A * S1.x - B * S0.x;
    const float y = A * S1.y - B * S0.y;

    intersection = { x, y };

    return( x >= FlexKit::Min(A_0.x, A_1.x) &&
            x <= FlexKit::Max(A_0.x, A_1.x) &&
            y >= FlexKit::Min(A_0.y, A_1.y) &&
            y <= FlexKit::Max(A_0.y, A_1.y));
}


bool Intersects(const float2 A_0, const float2 A_1, const float2 B_0, const float2 B_1) noexcept
{
    float2 _;
    return Intersects(A_0, A_1, B_0, B_1, _);
}


bool Intersects(const Triangle& A, const Triangle& B) noexcept
{
    const auto A_n = A.Normal();
    const auto A_p = A.TriPoint();
    const auto B_n = B.Normal();
    const auto B_p = B.TriPoint();

    bool infront    = false;
    bool behind     = false;

    float3 distances_A;
    float3 distances_B;

    for (size_t idx = 0; idx < 3; ++idx)
    {
        const auto t1       = B[idx] - A_p;
        const auto dp1      = t1.dot(A_n);
        const auto t2       = A[idx] - B_p;
        const auto dp2      = t2.dot(B_n);

        distances_B[idx] = dp1;
        distances_A[idx] = dp2;
    }

    infront = (distances_B[0] > 0.0f) | (distances_B[1] > 0.0f) | (distances_B[2] > 0.0);
    behind  = (distances_B[0] < 0.0f) | (distances_B[1] < 0.0f) | (distances_B[2] < 0.0);

    if(!(infront && behind))
        return false;

    infront = false;
    behind  = false;

    infront = distances_A[0] > 0.0f | distances_A[1] > 0.0f | distances_A[2] > 0.0;
    behind  = distances_A[0] < 0.0f | distances_A[1] < 0.0f | distances_A[2] < 0.0;

    if (!(infront && behind))
        return false;

    auto M_a = distances_A.Max();
    auto M_b = distances_B.Max();

    if (M_a < 0.00001f || M_b < 0.00001f)
    {   // Coplanar
        const float3 n = FlexKit::SSE_ABS(A_n);
        int i0, i1;

        if (n[0] > n[1])
        {
            if (n[0] > n[2])
            {
                i0 = 1;
                i1 = 2;
            }
            else
            {
                i0 = 0;
                i1 = 1;
            }
        }
        else
        {
            if (n[2] > n[1])
            {
                i0 = 0;
                i1 = 1;
            }
            else
            {
                i0 = 0;
                i1 = 2;
            }
        }

        const float2 XY_A[3] =
        {
            { A[0][i0], A[0][i1] },
            { A[1][i0], A[1][i1] },
            { A[2][i0], A[2][i1] }
        };

        const float2 XY_B[3] =
        {
            { B[0][i0], B[0][i1] },
            { B[1][i0], B[1][i1] },
            { B[2][i0], B[2][i1] }
        };

        // Edge to edge intersection testing

        bool res = false;
        for (size_t idx = 0; idx < 3; idx++)
        {
            for (size_t idx2 = 0; idx2 < 3; idx2++)
            {
                res |= Intersects(
                    XY_A[(idx + idx2 + 0) % 3],
                    XY_A[(idx + idx2 + 1) % 3],
                    XY_B[(idx + idx2 + 0) % 3],
                    XY_B[(idx + idx2 + 1) % 3]);
            }
        }


        if (!res)
        {
            const auto P1 = XY_B[0];

            const auto sign = [](auto& p1, auto& p2, auto& p3)
            {
                return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
            };

            const float S0 = sign(P1, XY_A[0], XY_A[1]);
            const float S1 = sign(P1, XY_A[1], XY_A[2]);
            const float S2 = sign(P1, XY_A[2], XY_A[0]);

            const bool neg = S0 < 0.0f || S1 < 0.0f || S2 < 0.0f;
            const bool pos = S0 > 0.0f || S1 > 0.0f || S2 > 0.0f;

            return !(neg && pos);
        }
        else
            return res;
    }
    else
    {   // Non-Planar
        const auto L_n = A_n.cross(B_n);
        const FlexKit::Plane p{ B_n, B_p };

        const FlexKit::Ray r[] = {
            { (A[1] - A[0]).normal(), A[0] },
            { (A[2] - A[1]).normal(), A[1] },
            { (A[0] - A[2]).normal(), A[2] } };

        const auto w0 = p.o - r[0].O;
        const auto w1 = p.o - r[1].O;
        const auto w2 = p.o - r[2].O;

        const float3 W_dp = { w0.dot(p.n),      w1.dot(p.n),    w2.dot(p.n) };
        const float3 V_dp = { r[0].D.dot(p.n),  r[1].D.dot(p.n), r[2].D.dot(p.n) };

        const float3 i = W_dp / V_dp;

        /*
        const float i[] =
        {
            FlexKit::Intersects(r[0], p),
            FlexKit::Intersects(r[1], p),
            FlexKit::Intersects(r[2], p)
        };
        */
        size_t idx = 0;
        float d = INFINITY;

        for (size_t I = 0; I < 3; I++)
        {
            //if (!std::isnan(i[I]) && !std::isinf(i[I]))
            {
                if (i[I] < d)
                {
                    idx = I;
                    d   = i[I];
                }
            }
        }

        const FlexKit::Ray L{ L_n, r[idx].R(i[idx]) };

        const float3 A_L = {
            ProjectPointOntoRay(A[0], L),
            ProjectPointOntoRay(A[1], L),
            ProjectPointOntoRay(A[2], L),
        };

        const float3 B_L = {
            ProjectPointOntoRay(B[0], L),
            ProjectPointOntoRay(B[1], L),
            ProjectPointOntoRay(B[2], L),
        };

        const float a_min = A_L.Min();
        const float a_max = A_L.Max();

        const float b_min = B_L.Min();
        const float b_max = B_L.Max();

        return (a_min < b_max) & (b_min < a_max);
    }
}


/************************************************************************************************/


void CSGBrush::Rebuild() noexcept
{
    if (left == nullptr && right == nullptr)
        return;

    intersections.clear();
    shape.tris.clear();

    auto LHS_tris = left->shape.GetTris();
    auto RHS_tris = right->shape.GetTris();

    struct IntersectionPair
    {
        Triangle* A;
        Triangle* B;
    };

    std::vector<IntersectionPair>   intersectionPairs;
    std::vector<Triangle*>          intersectSetA;
    std::vector<Triangle*>          intersectSetB;
    std::vector<Triangle*>          finalSet;

    {
        const auto AABB_A = left->GetAABB();

        for (auto& B : RHS_tris)
        {
            auto AABB_B = B.GetAABB().Offset(right->position);

            if (!FlexKit::Intersects(AABB_A, AABB_B))
            {
                shape.tris.push_back(B.Offset(right->position - position));
            }
            else
                intersectSetB.push_back(&B);
        }
    }

    {
        const auto AABB_B = right->GetAABB();

        for (auto& A : LHS_tris)
        {
            auto AABB_A = A.GetAABB().Offset(left->position);

            if (!FlexKit::Intersects(AABB_A, AABB_B))
            {
                shape.tris.push_back(A.Offset(left->position - position));
            }
            else
                intersectSetA.push_back(&A);
        }
    }


    for (auto& A : intersectSetA)
    {
        for (auto& B : intersectSetB)
        {
            if (Intersects(A->Offset(left->position), B->Offset(right->position)))
            {
                intersectionPairs.emplace_back(A, B);

                auto A_temp = A->Offset(left->position - position);
                auto B_temp = B->Offset(right->position - position);
                intersections.emplace_back(A_temp, B_temp);
            }
            else
            {
                //shape.tris.push_back(A->Offset(left->position - position));
                //shape.tris.push_back(B->Offset(right->position - position));
            }
        }
    }


    std::sort(finalSet.begin(), finalSet.end());
    finalSet.erase(std::unique(finalSet.begin(), finalSet.end()), finalSet.end());

    //for (auto triangle : finalSet)
    //    shape.tris.push_back(triangle->Offset(-position));
}


/************************************************************************************************/


struct RayTriIntersection_Res
{
    bool    res;
    float   d;
    float3  cord_bc;

    operator bool() const noexcept { return res; }
};


/************************************************************************************************/


RayTriIntersection_Res Intersects(const FlexKit::Ray& r, const Triangle& tri) noexcept
{
    static const float epsilon = 0.000001f;

    const auto edge1    = tri[1] - tri[0];
    const auto edge2    = tri[2] - tri[0];
    const auto h        = r.D.cross(edge2);
    const auto a        = edge1.dot(h);

    if (abs(a) < epsilon)
        return { false };

    const auto f = 1.0f / a;
    const auto s = r.O - tri[0];
    const auto u = f * s.dot(h);

    if (u < 0.0f || u > 1.0f)
        return { false };

    const auto q = s.cross(edge1);
    const auto v = f * r.D.dot(q);

    if (v < 0.0f || u + v > 1.0f)
        return { false };

    const auto t = f * edge2.dot(q);

    if (t > epsilon)
    {
        const auto w = tri[0] - r.O;
        const auto n = edge1.cross(edge2).normal();
        const auto d = w.dot(n) / r.D.dot(n);

        return { true, d, { u, v, 1.0f - u - v } };
    }
    else
        return { false };
}


/************************************************************************************************/


std::optional<CSGShape::RayCast_result> CSGShape::RayCast(const FlexKit::Ray& r) const noexcept
{
    if (!Intersects(r, GetAABB()))
        return {};

    float           d            = INFINITY;
    size_t          triangleIdx  = 0;
    FlexKit::float3 hitLocation;

    for (size_t triIdx = 0; triIdx < wFaces.size(); triIdx++)
    {
        const auto tri = GetTri(triIdx);
        auto [hit, distance, hit_cord] = Intersects(r, tri);
        if (hit && distance < d)
        {
            d           = distance;
            triangleIdx = triIdx;
            hitLocation = hit_cord;
        }
    }

    return RayCast_result{
        .hitLocation    = hitLocation,
        .distance       = d,
        .triangleIdx    = triangleIdx
    };
}


/************************************************************************************************/


std::optional<CSGBrush::RayCast_result> CSGBrush::RayCast(const FlexKit::Ray& r) const noexcept
{
    float                       d     = INFINITY;
    std::shared_ptr<CSGBrush>   brush;

    if (left)
    {
        if (auto res = FlexKit::Intersects(r, left->GetAABB()); res)
        {
            d       = res.value();
            brush   = left;
        }
    }

    if (right)
    {
        if (auto res = FlexKit::Intersects(r, right->GetAABB()); res)
        {
            if (res.value() < d)
                brush = right;
        }
    }

    if (brush)
        return brush->RayCast(r);
    else
    {
        auto localR = r;
        localR.O -= position;

        auto res = shape.RayCast(localR);

        if (res)
        {
            return RayCast_result{
                .shape              = const_cast<CSGShape*>(&shape),
                .triIdx             = res->triangleIdx,
                .distance           = res->distance,
                .BaryCentricResult  = res->hitLocation,
            };
        }
        else
            return {};
    }
}


/************************************************************************************************/


constexpr ViewportModeID CSGEditModeID = GetTypeGUID(CSGEditMode);


class CSGEditMode : public IEditorViewportMode
{
public:
    CSGEditMode(FlexKit::ImGUIIntegrator& IN_hud, CSGView& IN_selection, EditorViewport& IN_viewport) :
        hud         { IN_hud        },
        selection   { IN_selection  },
        viewport    { IN_viewport   } {}

    ViewportModeID GetModeID() const override { return CSGEditModeID; };

    enum class Mode : int
    {
        Selection,
        Manipulator
    }mode = Mode::Selection;


    void DrawImguI() final
    {
        DrawCSGMenu();

        switch (mode)
        {
        case CSGEditMode::Mode::Selection:
            DrawSelectionDebugUI();
            break;
        case CSGEditMode::Mode::Manipulator:
            DrawObjectManipulatorWidget();
            break;
        default:
            break;
        }
    }

    void DrawCSGMenu()
    {
        if (ImGui::Begin("Edit CSG Menu", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
        {
            ImGui::SetWindowPos({ 0, 0 });
            ImGui::SetWindowSize({ 400, 400 });

            CSG_OP op = CSG_OP::CSG_ADD;

            static const char* modeStr[] =
            {
                "Add",
                "Sub",
                "Intersection"
            };

            if (ImGui::BeginCombo("OP", modeStr[(int)op]))
            {
                if (ImGui::Selectable(modeStr[0]))
                    op = CSG_OP::CSG_ADD;
                if (ImGui::Selectable(modeStr[1]))
                    op = CSG_OP::CSG_SUB;
                if (ImGui::Selectable(modeStr[2]))
                    op = CSG_OP::CSG_INTERSECTION;

                ImGui::EndCombo();
            }

            static const char* selectionStr[] =
            {
                "Vertex",
                "Edge",
                "Triangle",
                "Polygon",
                "Disabled",
            };

            if (ImGui::BeginCombo("Selection", selectionStr[(int)selectionContext.mode]))
            {
                if (ImGui::Selectable(selectionStr[0]))
                    selectionContext.mode = SelectionPrimitive::Vertex;
                if (ImGui::Selectable(selectionStr[1]))
                    selectionContext.mode = SelectionPrimitive::Edge;
                if (ImGui::Selectable(selectionStr[2]))
                    selectionContext.mode = SelectionPrimitive::Triangle;
                if (ImGui::Selectable(selectionStr[3]))
                    selectionContext.mode = SelectionPrimitive::Polygon;
                if (ImGui::Selectable(selectionStr[4]))
                    selectionContext.mode = SelectionPrimitive::Disabled;

                ImGui::EndCombo();
            }

            if (ImGui::Button("Create Square"))
            {
                auto& csg = selection.GetData();
                CSGBrush newBrush;
                newBrush.op         = op;
                newBrush.shape      = CreateCubeCSGShape();
                newBrush.position   = FlexKit::float3{ 0, 0, 0 };
                newBrush.dirty      = true;

                csg.brushes.push_back(newBrush);
                csg.selectedBrush = -1;
            }

            const auto selectedIdx = selection.GetData().selectedBrush;

            if (selectedIdx != -1)
            {
                auto& brushes   = selection.GetData().brushes;
                auto& brush     = brushes[selectedIdx];

                ImGui::Text(fmt::format("Tri count: {}", brush.shape.tris.size()).c_str());
            }
        }
        ImGui::End();
    }

    void DrawSelectionDebugUI()
    {
        if (selectionContext.selectedPrimitives.size())
        {
            if (ImGui::Begin("Selection Debug", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
            {
                ImGui::SetWindowPos({ 0, ImGui::GetCursorPosY() });
                ImGui::SetWindowSize({ 400, 400 });

                ImGui::Text(fmt::format("Selection Count: {}", selectionContext.selectedPrimitives.size()).c_str());

                for (size_t I = 0; I < selectionContext.selectedPrimitives.size(); I++)
                {
                    auto& primitive = selectionContext.selectedPrimitives[I];
                    ImGui::Text(fmt::format("Selection: {}, IntersectionPoint, [{}, {}, {}]",
                        I,
                        primitive.BaryCentricResult[0],
                        primitive.BaryCentricResult[1],
                        primitive.BaryCentricResult[2]).c_str());
                }

            }
            ImGui::End();
        }
    }

    void DrawObjectManipulatorWidget() const
    {
        const auto selectedIdx  = selection.GetData().selectedBrush;
        if (selectedIdx == -1)
            return;

        auto& brushes   = selection.GetData().brushes;
        auto& brush     = brushes[selectedIdx];


        if (brushes.size() < selectedIdx)
            return;

        ImGuiIO& io = ImGui::GetIO();
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

        const auto camera = viewport.GetViewportCamera();
        FlexKit::CameraComponent::GetComponent().GetCamera(camera).UpdateMatrices();
        auto& cameraData = FlexKit::CameraComponent::GetComponent().GetCamera(camera);


                FlexKit::float4x4 m             = FlexKit::TranslationMatrix(brushes[selectedIdx].position);
                FlexKit::float4x4 delta         = FlexKit::float4x4::Identity();

        const   FlexKit::float4x4 view          = cameraData.View.Transpose();
        const   FlexKit::float4x4 projection    = cameraData.Proj;

        if (ImGuizmo::Manipulate(view, projection, operation, space, m, delta))
        {
            FlexKit::float3 pos;
            FlexKit::float3 rotation;
            FlexKit::float3 scale;
            ImGuizmo::DecomposeMatrixToComponents(m, pos, rotation, scale);

            brushes[selectedIdx].position   = pos;

            auto itr = brushes.begin();
            for (;itr != brushes.end();)
            {
                auto& brush_b = *itr;
                if (&brush_b != &brush && FlexKit::Intersects(brush.GetAABB(), brush_b.GetAABB()))
                {
                    auto lhsBrush = std::make_shared<CSGBrush>();
                    auto rhsBrush = std::make_shared<CSGBrush>();

                    *lhsBrush = brush;
                    *rhsBrush = brush_b;

                    brush.left  = lhsBrush;
                    brush.right = rhsBrush;

                    brush.Rebuild();

                    itr = brushes.erase(itr);
                }
                else
                    itr++;
            }
        }
    }

    void Draw(FlexKit::UpdateDispatcher& dispatcher, FlexKit::FrameGraph& frameGraph, TemporaryBuffers& temps, FlexKit::ResourceHandle renderTarget, FlexKit::ResourceHandle depthBuffer) final
    {

        struct DrawHUD
        {
            FlexKit::ReserveVertexBufferFunction    ReserveVertexBuffer;
            FlexKit::ReserveConstantBufferFunction  ReserveConstantBuffer;
            FlexKit::FrameResourceHandle            renderTarget;
            FlexKit::FrameResourceHandle            depthBuffer;
        };


        if(selection.GetData().brushes.size())
        {
            frameGraph.AddNode<DrawHUD>(
                DrawHUD{
                    temps.ReserveVertexBuffer,
                    temps.ReserveConstantBuffer
                },
                [&](FlexKit::FrameGraphNodeBuilder& builder, DrawHUD& data)
                {
                    data.renderTarget   = builder.RenderTarget(renderTarget);
                    data.depthBuffer    = builder.DepthTarget(depthBuffer);
                },
                [&](DrawHUD& data, FlexKit::ResourceHandler& resources, FlexKit::Context& ctx, auto& allocator)
                {
                    ctx.BeginEvent_DEBUG("Draw CSG HUD");

                    const auto& brushes = selection.GetData().brushes;

                    // Setup state
                    FlexKit::DescriptorHeap descHeap;
			        descHeap.Init(
				        ctx,
				        resources.renderSystem().Library.RS6CBVs4SRVs.GetDescHeap(0),
				        &allocator);
			        descHeap.NullFill(ctx);

			        ctx.SetRootSignature(resources.renderSystem().Library.RS6CBVs4SRVs);
			        ctx.SetPipelineState(resources.GetPipelineState(FlexKit::DRAW_TRI3D_PSO));

			        ctx.SetScissorAndViewports({ resources.GetRenderTarget(data.renderTarget) });
                    ctx.SetRenderTargets({ resources.GetRenderTarget(data.renderTarget) }, true, resources.GetRenderTarget(data.depthBuffer));

			        ctx.SetPrimitiveTopology(FlexKit::EInputTopology::EIT_TRIANGLE);

			        ctx.SetGraphicsDescriptorTable(0, descHeap);

			        ctx.NullGraphicsConstantBufferView(3);
			        ctx.NullGraphicsConstantBufferView(4);
			        ctx.NullGraphicsConstantBufferView(5);
			        ctx.NullGraphicsConstantBufferView(6);


                    struct DrawConstants
                    {
                        FlexKit::float4     unused1;
                        FlexKit::float4     unused2;
                        FlexKit::float4x4   transform;
                    };

                    auto constantBuffer = data.ReserveConstantBuffer(
                        FlexKit::AlignedSize<FlexKit::Camera::ConstantBuffer>() +
                        FlexKit::AlignedSize<DrawConstants>() * brushes.size());

                    FlexKit::ConstantBufferDataSet cameraConstants{ FlexKit::GetCameraConstants(viewport.GetViewportCamera()), constantBuffer };
                    ctx.SetGraphicsConstantBufferView(1, cameraConstants);

                    const size_t selectedNodeIdx    = selection.GetData().selectedBrush;
                    FlexKit::VBPushBuffer VBBuffer  = data.ReserveVertexBuffer(sizeof(Vertex) * 1024 * brushes.size());

                    size_t idx = 0;
                    for (const auto& brush : brushes)
                    {
                        FlexKit::float4 Color = (idx == selectedNodeIdx) ? FlexKit::float4{ 1, 1, 1, 1.0f } : FlexKit::float4{ 0.25f, 0.25f, 0.25f, 1.0f };

                        const auto aabb     = brush.GetAABB();
                        const float r       = aabb.Dim()[aabb.LongestAxis()];
                        const auto offset   = aabb.MidPoint();

                        std::vector<Vertex> verts;

                        for (auto& tri : brush.shape.tris)
                        {
                            Vertex v;
                            v.Color     = Color;
                            v.UV        = FlexKit::float2(1, 1);

                            for (size_t idx = 0; idx < 3; idx++)
                            {
                                v.Position = FlexKit::float4(tri[idx], 1);
                                verts.emplace_back(v);
                            }
                            /*
                            v.Position = FlexKit::float4(tri[0], 1);
                            verts.emplace_back(v);
                            v.Position = FlexKit::float4(tri[1], 1);
                            verts.emplace_back(v);
                            v.Position = FlexKit::float4(tri[1], 1);
                            verts.emplace_back(v);
                            v.Position = FlexKit::float4(tri[2], 1);
                            verts.emplace_back(v);
                            v.Position = FlexKit::float4(tri[2], 1);
                            verts.emplace_back(v);
                            v.Position = FlexKit::float4(tri[0], 1);
                            verts.emplace_back(v);
                            */
                        }

                        for (auto& intersection : brush.intersections)
                        {
                            Vertex v;
                            v.Color     = FlexKit::float4(1, 0, 0, 1);
                            v.UV        = FlexKit::float2(1, 1);

                            for (size_t idx = 0; idx < 3; idx++)
                            {
                                v.Position = FlexKit::float4(intersection.A[idx], 1);
                                verts.emplace_back(v);
                            }
                            for (size_t idx = 0; idx < 3; idx++)
                            {
                                v.Position = FlexKit::float4(intersection.B[idx], 1);
                                verts.emplace_back(v);
                            }
                            /*
                            v.Position = FlexKit::float4(intersection.A[0], 1);
                            verts.emplace_back(v);
                            v.Position = FlexKit::float4(intersection.A[1], 1);
                            verts.emplace_back(v);
                            v.Position = FlexKit::float4(intersection.A[1], 1);
                            verts.emplace_back(v);
                            v.Position = FlexKit::float4(intersection.A[2], 1);
                            verts.emplace_back(v);
                            v.Position = FlexKit::float4(intersection.A[2], 1);
                            verts.emplace_back(v);
                            v.Position = FlexKit::float4(intersection.A[0], 1);
                            verts.emplace_back(v);


                            v.Position = FlexKit::float4(intersection.B[0], 1);
                            verts.emplace_back(v);
                            v.Position = FlexKit::float4(intersection.B[1], 1);
                            verts.emplace_back(v);
                            v.Position = FlexKit::float4(intersection.B[1], 1);
                            verts.emplace_back(v);
                            v.Position = FlexKit::float4(intersection.B[2], 1);
                            verts.emplace_back(v);
                            v.Position = FlexKit::float4(intersection.B[2], 1);
                            verts.emplace_back(v);
                            v.Position = FlexKit::float4(intersection.B[0], 1);
                            verts.emplace_back(v);
                            */
                        }

                        selectionContext.GetSelectionUIGeometry(verts);

                        const FlexKit::VertexBufferDataSet vbDataSet{
                            verts,
                            VBBuffer };

                        ctx.SetVertexBuffers({ vbDataSet });

                        const DrawConstants CB_Data = {
                            .unused1    = FlexKit::float4{ 1, 1, 1, 1 },
                            .unused2    = FlexKit::float4{ 1, 1, 1, 1 },
                            .transform  = FlexKit::TranslationMatrix(brush.position)
                        };

                        const FlexKit::ConstantBufferDataSet constants{ CB_Data, constantBuffer };

                        ctx.SetGraphicsConstantBufferView(2, constants);

                        ctx.Draw(verts.size());

                        ++idx;

                    }

                    ctx.EndEvent_DEBUG();
                }
            );
        }
    }

    void mousePressEvent(QMouseEvent* evt) final 
    {
        if(ImGui::IsAnyItemHovered() || mode == CSGEditMode::Mode::Manipulator)
        {
            if (evt->button() == Qt::MouseButton::LeftButton)
            {
                previousMousePosition = FlexKit::int2{ -160000, -160000 };

                FlexKit::Event mouseEvent;
                mouseEvent.InputSource  = FlexKit::Event::Mouse;
                mouseEvent.Action       = FlexKit::Event::Pressed;
                mouseEvent.mType        = FlexKit::Event::Input;

                mouseEvent.mData1.mKC[0] = FlexKit::KC_MOUSELEFT;
                hud.HandleInput(mouseEvent);
            }
            else if (evt->button() == Qt::MouseButton::RightButton)
            {
                previousMousePosition = FlexKit::int2{ -160000, -160000 };

                FlexKit::Event mouseEvent;
                mouseEvent.InputSource  = FlexKit::Event::Mouse;
                mouseEvent.Action       = FlexKit::Event::Pressed;
                mouseEvent.mType        = FlexKit::Event::Input;

                mouseEvent.mData1.mKC[0] = FlexKit::KC_MOUSERIGHT;
                hud.HandleInput(mouseEvent);
            }
        }
        else
        {
            switch (mode)
            {
            case CSGEditMode::Mode::Selection:
            {
                switch (selectionContext.mode)
                {
                case SelectionPrimitive::Triangle:
                {
                    size_t selectedIdx = -1;
                    float  minDistance = 10000.0f;
                    const auto r = viewport.GetMouseRay();

                    for (const auto& brush : selection.GetData().brushes)
                    {
                        const auto AABB = brush.GetAABB();
                        auto res = FlexKit::Intersects(r, AABB);
                        if (res && res.value() < minDistance && res.value() > 0.0f)
                        {
                            if (auto res = brush.RayCast(r); res)
                            {
                                if (res->distance < minDistance)
                                {
                                    selectionContext.selectedPrimitives.clear();
                                    selectionContext.selectedPrimitives.push_back(*res);
                                    selectionContext.brush = const_cast<CSGBrush*>(&brush);
                                    selectionContext.shape = const_cast<CSGShape*>(&brush.shape);
                                }
                            }
                        }
                    }
                    return;
                }   break;
                default:
                    break;
                }

            }   break;
            case CSGEditMode::Mode::Manipulator:
                break;
            default:
                break;
            }
        }
    }

    void mouseReleaseEvent(QMouseEvent* evt) final 
    {
        if (ImGui::IsAnyItemHovered() || mode == CSGEditMode::Mode::Manipulator)
        {
            if (evt->button() == Qt::MouseButton::LeftButton)
            {
                previousMousePosition = FlexKit::int2{ -160000, -160000 };

                FlexKit::Event mouseEvent;
                mouseEvent.InputSource  = FlexKit::Event::Mouse;
                mouseEvent.Action       = FlexKit::Event::Release;
                mouseEvent.mType        = FlexKit::Event::Input;

                mouseEvent.mData1.mKC[0] = FlexKit::KC_MOUSELEFT;
                hud.HandleInput(mouseEvent);
            }
            else if (evt->button() == Qt::MouseButton::RightButton)
            {
                previousMousePosition = FlexKit::int2{ -160000, -160000 };

                FlexKit::Event mouseEvent;
                mouseEvent.InputSource  = FlexKit::Event::Mouse;
                mouseEvent.Action       = FlexKit::Event::Release;
                mouseEvent.mType        = FlexKit::Event::Input;

                mouseEvent.mData1.mKC[0] = FlexKit::KC_MOUSERIGHT;
                hud.HandleInput(mouseEvent);
            }
        }
        else
        {
            switch (mode)
            {
            case CSGEditMode::Mode::Selection:
            {
            }   break;
            case CSGEditMode::Mode::Manipulator:
                break;
            default:
                break;
            }
        }
    }

    void keyPressEvent(QKeyEvent* evt) final
    {
        switch (evt->key())
        {
        case Qt::Key_W:
            operation   = ImGuizmo::OPERATION::TRANSLATE;
            mode        = Mode::Manipulator;
            break;
        case Qt::Key_E:
            operation   = ImGuizmo::OPERATION::SCALE;
            mode        = Mode::Manipulator;
            break;
        case Qt::Key_R:
            operation   = ImGuizmo::OPERATION::ROTATE;
            mode        = Mode::Manipulator;
            break;
        case Qt::Key_T:
        {
            if(mode == Mode::Manipulator)
                space = space == ImGuizmo::MODE::WORLD ? ImGuizmo::MODE::LOCAL: ImGuizmo::MODE::WORLD;
        }   break;
        default:
            break;
        }
    }

    enum class SelectionPrimitive
    {
        Vertex,
        Edge,
        Triangle,
        Polygon,
        Disabled
    };

    struct CSGSelectionContext
    {
        SelectionPrimitive mode = SelectionPrimitive::Disabled;

        CSGBrush*   brush   = nullptr;
        CSGShape*   shape   = nullptr;

        std::vector<CSGBrush::RayCast_result> selectedPrimitives;

        void GetSelectionUIGeometry(std::vector<Vertex>& verts) const
        {
            if (!shape)
                return;

            switch (mode)
            {
            case SelectionPrimitive::Vertex:
            {
            }   break;
            case SelectionPrimitive::Edge:
            {
            }   break;
            case SelectionPrimitive::Triangle:
            case SelectionPrimitive::Polygon:
            {
                for (auto& primitive : selectedPrimitives)
                {
                    auto tri = primitive.shape->GetTri(primitive.triIdx).Offset(brush->position);
                    tri = tri.Offset(tri.Normal() * 0.01f);

                    Vertex v;
                    v.Color = FlexKit::float4(1, 0, 0, 1);
                    v.UV    = FlexKit::float2(1, 1);

                    for (size_t idx = 0; idx < 3; idx++)
                    {
                        v.Position = FlexKit::float4(tri[idx], 1);
                        verts.emplace_back(v);
                    }
                }
            }   break;
            }
        }
    }selectionContext;

    ImGuizmo::MODE      space       = ImGuizmo::MODE::WORLD;
    ImGuizmo::OPERATION operation   = ImGuizmo::OPERATION::TRANSLATE;

    EditorViewport&             viewport;
    CSGView&                    selection;
    FlexKit::ImGUIIntegrator&   hud;
    FlexKit::int2               previousMousePosition = FlexKit::int2{ -160000, -160000 };
};


/************************************************************************************************/


void UpdateCSGComponent(CSGComponentData& brushComponent)
{
    auto itr1 = brushComponent.brushes.begin();
    auto itr2 = brushComponent.brushes.begin();

    for (; itr1 != brushComponent.brushes.end();)
    {
        for (; itr2 != brushComponent.brushes.end();)
        {
            auto& brush_b = *itr2;
            if (itr2 != itr1 && FlexKit::Intersects(itr1->GetAABB(), itr2->GetAABB()))
            {
                auto lhsBrush = std::make_shared<CSGBrush>();
                auto rhsBrush = std::make_shared<CSGBrush>();

                *lhsBrush = *itr1;
                *rhsBrush = *itr2;

                itr1->left = lhsBrush;
                itr1->right = rhsBrush;

                itr1->Rebuild();

                itr2 = brushComponent.brushes.erase(itr2);
            }
            else
                itr2++;
        }
    }
}


/************************************************************************************************/


class CSGInspector : public IComponentInspector
{
public:
    CSGInspector(EditorViewport& IN_viewport) :
        viewport{ IN_viewport } {}

    FlexKit::ComponentID ComponentID() override
    {
        return CSGComponentID;
    }

    void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::ComponentViewBase& view) override
    {
        CSGView& csgView = static_cast<CSGView&>(view);

        panelCtx.PushVerticalLayout("CSG", true);

        panelCtx.AddButton(
            "Edit",
            [&]
            {
                viewport.PushMode(std::make_shared<CSGEditMode>(viewport.GetHUD(), csgView, viewport));
            });

        panelCtx.AddButton(
            "Test",
            [&]
            {
                auto& csg = csgView.GetData();
                CSGBrush brush1;
                brush1.op       = CSG_OP::CSG_ADD;
                brush1.shape    = CreateCubeCSGShape();
                brush1.position = FlexKit::float3{ 0.5f, 0.5f, 0.5f };
                brush1.dirty    = true;

                CSGBrush brush2;
                brush2.op       = CSG_OP::CSG_ADD;
                brush2.shape    = CreateCubeCSGShape();
                brush2.position = FlexKit::float3{ 0.5f, 0.5f, 0.5f };
                brush2.dirty    = true;

                csg.brushes.push_back(brush1);
                csg.brushes.push_back(brush2);

                UpdateCSGComponent(csg);

            });

        panelCtx.AddList(
            [&]() -> size_t
            {    // Update Size
                return csgView.GetData().brushes.size();
            }, 
            [&](auto idx, QListWidgetItem* item)
            {   // Update Contents
                auto op = csgView.GetData().brushes[idx].op;

                std::string label = op == CSG_OP::CSG_ADD ? "Add" :
                                    op == CSG_OP::CSG_SUB ? "Sub" : "";

                item->setData(Qt::DisplayRole, label.c_str());
            },             
            [&](QListWidget* listWidget)
            {   // On Event
                listWidget->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
                auto idx = listWidget->indexFromItem(listWidget->currentItem()).row();
                csgView.GetData().selectedBrush = idx;
            }              
        );

        panelCtx.Pop();
        panelCtx.Pop();
    }

    EditorViewport& viewport;
};


/************************************************************************************************/


struct CSGComponentFactory : public IComponentFactory
{
    void Construct(ViewportGameObject& viewportObject, ViewportScene& scene)
    {
        viewportObject.gameObject.AddView<CSGView>();
    }

    inline static const std::string name = "CSG";

    const std::string& ComponentName() const noexcept { return name; }

    static bool Register()
    {
        EditorInspectorView::AddComponentFactory(std::make_unique<CSGComponentFactory>());

        return true;
    }

    static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
    {
        auto& editorCSGComponent = static_cast<EditorComponentCSG&>(component);

    }

    inline static IEntityComponentRuntimeUpdater::RegisterConstructorHelper<CSGComponentFactory, CSGComponentID> _register;

    inline static bool _registered = Register();
};


/************************************************************************************************/


const std::vector<Triangle>& CSGShape::GetTris() const noexcept
{
    return tris;
}

FlexKit::BoundingSphere CSGShape::GetBoundingVolume() const noexcept
{
    return { 0, 0, 0, 1 };
}


/************************************************************************************************/


void RegisterComponentInspector(EditorViewport& viewport)
{
    EditorInspectorView::AddComponentInspector<CSGInspector>(viewport);
}


/************************************************************************************************/
