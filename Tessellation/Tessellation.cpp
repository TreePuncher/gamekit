#include "pch.h"

constexpr FlexKit::PSOHandle ACCQuad                = FlexKit::PSOHandle(GetTypeGUID(ACCQuad));
constexpr FlexKit::PSOHandle ACCDebugNormals        = FlexKit::PSOHandle(GetTypeGUID(ACCDebugNormals));
constexpr FlexKit::PSOHandle ACCDebugControlpoints  = FlexKit::PSOHandle(GetTypeGUID(ACCDebugControlpoints));

using namespace FlexKit;

enum GregoryQuadPatchPoint
{
    p0      = 0,
    p1      = 1,
    p2      = 2,
    p3      = 3,

    e0Minus = 4,
    e0Plus  = 5,
    e1Minus = 6,
    e1Plus  = 7,
    e2Minus = 8,
    e2Plus  = 9,
    e3Minus = 10,
    e3Plus  = 11,

    r0Minus = 12,
    r0Plus  = 13,

    r1Minus = 14,
    r1Plus  = 15,

    r2Minus = 16,
    r2Plus  = 17,

    r3Minus = 18,
    r3Plus  = 19
};


struct GPUVertex
{
    float P[3];
};


float Lambda(float N)
{
    const float T = cos((2 * pi) / N);

    return
        (1.0f / 16.0f) *
        (5.0f
            + T
            + cos(pi / N) * sqrt(18 + 2 * T));
}


float Sigma(float N)
{
    const float T = cos(pi / N);
    const float TT = T * T;
    return 1.0f / sqrt(4.0f + TT);
}


struct QuadPatch
{
    uint4 vertexIds;
};

struct VertexNeighorList
{
    static_vector<uint32_t> patches;
};


/************************************************************************************************/


struct PatchGroups
{
    std::vector<uint32_t> QuadPatches;
    std::vector<uint32_t> TriPatches;

    std::vector<uint32_t> irregularTri;
    std::vector<uint32_t> irregularQuad;

    std::vector<uint32_t> edgeQuadPatches;
    std::vector<uint32_t> edgeTriPatches;
};


/************************************************************************************************/


struct Patch
{
    Patch()
    {
        memset(inputVertices, 0xff, sizeof(inputVertices));
    }

    struct ControlPointWeights
    {
        ControlPointWeights() { memset(indices, 0xff, sizeof(indices)); }

        void PushWeight(uint32_t index, float w)
        {

        }

        float       weights[32] = { 0.0f };
        uint32_t    indices[32];

        void AddWeight(uint32_t idx, float w)
        {
            size_t I = 0;
            for (; I < 32; I++)
            {
                if (indices[I] == idx)
                {
                    weights[I] += w;
                    return;
                }

                if (indices[I] == 0xffffffff)
                {
                    indices[I] = idx;
                    weights[I] = w;
                    return;
                }
            }
        }

        void ScaleWeight(uint32_t idx, float s)
        {
            size_t I = 0;
            for (; I < 32; I++)
            {
                if (indices[I] == idx)
                {
                    weights[I] *= s;
                    return;
                }
            }
        }

        ControlPointWeights& operator += (const ControlPointWeights& rhs)
        {
            for (size_t I = 0; I < 32; I++)
                if(rhs.indices[I] != 0xffffffff && rhs.weights[I] != 0.0f)
                    AddWeight(rhs.indices[I], rhs.weights[I]);
        }
    } controlPoints[20];

    uint32_t C_0;
    uint32_t C_idx[8];


    uint32_t inputVertices[32];
    uint32_t vertCount = 0;
};


struct GPUPatch
{
    struct controlPoint
    {
        uint16_t weights[32];
    } controlPoints[20];

    uint C0;
    uint Cn[4];
};


/************************************************************************************************/


uint Pack(uint4 points)
{
    return  ((points[0] & 0xff) << 0) |
            ((points[1] & 0xff) << 8) |
            ((points[2] & 0xff) << 16) |
            ((points[3] & 0xff) << 24);
}


/************************************************************************************************/


uint4 MapIndexToLocal(const auto& verts, const auto& localVerts)
{
    uint4 patchLocalIndexes{ 0xff, 0xff, 0xff, 0xff };

    for (size_t J = 0; J < 4; J++)
    {
        for (uint32_t I = 0; I < 32; I++)
        {
            if (localVerts[I] == verts[J])
            {
                patchLocalIndexes[J] = I;
                break;
            }
        }
    }

    return patchLocalIndexes;
}


/************************************************************************************************/


uint32_t MapIndexToLocal(const uint32_t idx, const auto& localVerts)
{
    for (uint32_t I = 0; I < 32; I++)
        if (localVerts[I] == idx)
            return I;

    return -1;
}


/************************************************************************************************/




void CreateGPUPatch(const Patch& p, ModifiableShape& shape, Vector<GPUPatch>& patches, Vector<uint32_t>& indices)
{
    GPUPatch patch;
    uint32_t patchIndexes[32];

    for (size_t J = 0; J < 32; J++)
    {
        const auto idx = p.inputVertices[J];
        patchIndexes[J] = (idx != 0xffffffff) ? p.inputVertices[J] : p.inputVertices[0];
    }

    for (size_t I = 0; I < 20; I++)
    {
        const auto& cp = p.controlPoints[I];

        for (size_t J = 0; J < 32; J++)
            patch.controlPoints[I].weights[J] = fp16_ieee_from_fp32_value(0);

        for (size_t J = 0; J < 32; J++)
        {
            const uint32_t localIdx = MapIndexToLocal(cp.indices[J], patchIndexes);

            if (cp.indices[J] != 0xffffffff)
                patch.controlPoints[I].weights[localIdx] = fp16_ieee_from_fp32_value(cp.weights[J]);
        }
    }

    /*
    patch.C0    = Pack(MapIndexToLocal(shape.GetFaceVertices(p.C_0),      p.inputVertices));
    patch.Cn[0] = Pack(MapIndexToLocal(shape.GetFaceVertices(p.C_idx[0]), p.inputVertices));
    patch.Cn[1] = Pack(MapIndexToLocal(shape.GetFaceVertices(p.C_idx[2]), p.inputVertices));
    patch.Cn[2] = Pack(MapIndexToLocal(shape.GetFaceVertices(p.C_idx[4]), p.inputVertices));
    patch.Cn[3] = Pack(MapIndexToLocal(shape.GetFaceVertices(p.C_idx[6]), p.inputVertices));
    */

    patches.push_back(patch);

    for (auto I : p.inputVertices)
        indices.push_back(I);
}

/************************************************************************************************/


float3 ApplyWeights(Patch& patch, ModifiableShape& shape, uint32_t Idx)
{
    auto& indices = patch.controlPoints[Idx].indices;
    auto& weights = patch.controlPoints[Idx].weights;

    float3 P{ 0, 0, 0 };

    for (size_t I = 0; I < 32; I++)
    {
        auto index = indices[I];

        if (index != 0xffffffff) {
            const float w = weights[I];
            P += shape.wVertices[index].point * w;
        }
    }

    return P;
}

float3 ApplyWeights(const Patch::ControlPointWeights& CP, const ModifiableShape& shape)
{
    const auto& indices = CP.indices;
    const auto& weights = CP.weights;

    float3 P{ 0, 0, 0 };

    for (size_t I = 0; I < 32; I++)
    {
        auto index = indices[I];

        if (index != 0xffffffff) {
            const float w = weights[I];
            P += shape.wVertices[index].point * w;
        }
    }

    return P;
}

float3 GetPoint_P0(Patch& patch, ModifiableShape& shape) { return ApplyWeights(patch, shape, p0); }
float3 GetPoint_P1(Patch& patch, ModifiableShape& shape) { return ApplyWeights(patch, shape, p1); }
float3 GetPoint_P2(Patch& patch, ModifiableShape& shape) { return ApplyWeights(patch, shape, p2); }
float3 GetPoint_P3(Patch& patch, ModifiableShape& shape) { return ApplyWeights(patch, shape, p3); }


/************************************************************************************************/


float3 GetPoint_E0_Plus(Patch& patch, ModifiableShape& shape)
{
    const float3 P = GetPoint_P0(patch, shape);
    const float3 Q = ApplyWeights(patch, shape, e0Plus);
    const float3 e0_Plus = P + 2.0f / 3.0f * Q;

    return e0_Plus;
}


/************************************************************************************************/


float3 GetPoint_E0_Minus(Patch& patch, ModifiableShape& shape)
{
    const float3 P = GetPoint_P0(patch, shape);
    const float3 Q = ApplyWeights(patch, shape, e0Minus);
    const float3 e0_Minus = P + 2.0f / 3.0f * Q;

    return e0_Minus;
}


/************************************************************************************************/


float3 GetPoint_E1_Plus(Patch& patch, ModifiableShape& shape)
{
    const float3 P = GetPoint_P1(patch, shape);
    const float3 Q = ApplyWeights(patch, shape, e1Plus);
    const float3 e1_Plus = P + 2.0f / 3.0f * Q;

    return e1_Plus;
}


/************************************************************************************************/


float3 GetPoint_E1_Minus(Patch& patch, ModifiableShape& shape)
{
    const float3 P = GetPoint_P1(patch, shape);
    const float3 Q = ApplyWeights(patch, shape, e1Minus);
    const float3 e1_Minus = P + 2.0f / 3.0f * Q;

    return e1_Minus;
}


/************************************************************************************************/


float3 GetPoint_E2_Plus(Patch& patch, ModifiableShape& shape)
{
    const float3 P = GetPoint_P2(patch, shape);
    const float3 Q = ApplyWeights(patch, shape, e2Plus);
    const float3 e2_Plus = P + 2.0f / 3.0f * Q;

    return e2_Plus;
}


/************************************************************************************************/


float3 GetPoint_E2_Minus(Patch& patch, ModifiableShape& shape)
{
    const float3 P          = GetPoint_P2(patch, shape);
    const float3 Q          = ApplyWeights(patch, shape, e2Minus);
    const float3 e2_Minus   = P + 2.0f / 3.0f * Q;

    return e2_Minus;
}


/************************************************************************************************/


float3 GetPoint_E3_Plus(Patch& patch, ModifiableShape& shape)
{
    const float3 P          = GetPoint_P3(patch, shape);
    const float3 Q          = ApplyWeights(patch, shape, e3Plus);
    const float3 e3_Plus    = P + 2.0f / 3.0f * Q;

    return e3_Plus;
}


/************************************************************************************************/


float3 GetPoint_E3_Minus(Patch& patch, ModifiableShape& shape)
{
    const float3 P          = GetPoint_P3(patch, shape);
    const float3 Q          = ApplyWeights(patch, shape, e3Minus);
    const float3 e3_Minus   = P + 2.0f / 3.0f * Q;

    return e3_Minus;
}


/************************************************************************************************/


// First Classify patches
auto ClassifyPatches(ModifiableShape& shape)
{
    PatchGroups groups;

    for (uint32_t I = 0; I < shape.wFaces.size(); I++)
    {
        auto& patch = shape.wFaces[I];
        auto edgeCount = patch.GetEdgeCount(shape);

        switch (edgeCount)
        {
        case 3: // Triangle
        {
            [&]
            {
                auto itr = patch.VertexView(shape).begin();
                auto end = patch.VertexView(shape).end();

                for (; itr != end; itr++)
                {
                    if (itr.Edges().size() == 2)
                    {
                        groups.TriPatches.push_back(I);
                        return;
                    }
                    else if (itr.Edges().size() != 3)
                    {
                        groups.irregularTri.push_back(I);
                        return;
                    }
                }

                groups.TriPatches.push_back(I);
            }();
        }   break;
        case 4: // Quad:
        {
            [&]
            {
                auto itr = patch.VertexView(shape).begin();
                auto end = patch.VertexView(shape).end();

                for (; itr != end; itr++)
                {
                    const auto vertexValence = itr.Edges().size() / 2;
                    if (itr.IsEdge())
                    {
                        groups.edgeQuadPatches.push_back(I);
                        return;
                    }
                    else if (vertexValence != 4)
                    {
                        groups.irregularQuad.push_back(I);
                        return;
                    }
                }

                groups.QuadPatches.push_back(I);
            }();
        }   break;
        }
    }

    return groups;
};


/************************************************************************************************/


void CalculateQuadCornerPoints(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape)
{
    uint32_t B = 0; // ControlPoint Counter
    auto vertexView = shape.wFaces[quadPatchIdx].VertexView(shape);

    for (auto [point, pointIdx, edges, edgeView] : vertexView)
    {
        size_t A = 0; // Weight counter
        Patch::ControlPointWeights controlPoint;
        switch(edges.size())
        {
        case 2: // Corner
        {
            auto idx = A++;
            controlPoint.weights[idx] = 1.0f;
            controlPoint.indices[idx] = pointIdx;
        }   break;
        case 4: // T-Junction
        {
            static_vector<uint32_t, 2> boundaryEdges;

            for (size_t I = 0; I < 4; I++)
            {
                const auto e = edges[I];
                const auto twin = shape.wEdges[edges[I]].oppositeNeighbor;

                if (twin == 0xffffffff)
                    boundaryEdges.push_back(e);
            }

            controlPoint.weights[A] = 4.0f / 6.0f;
            controlPoint.indices[A++] = pointIdx;

            const uint32_t verts[] = {
                shape.wEdges[boundaryEdges[0]].vertices[0],
                shape.wEdges[boundaryEdges[0]].vertices[1],
                shape.wEdges[boundaryEdges[1]].vertices[0],
                shape.wEdges[boundaryEdges[1]].vertices[1] };


            for (auto v : verts)
            {
                if (v != pointIdx)
                {
                    controlPoint.weights[A]     = 1.0f / 6.0f;
                    controlPoint.indices[A++]   = v;
                }
            }
        }   break;
        default:
            const auto n = edges.size() / 2;

            auto idx                = A++;
            controlPoint.weights[idx] = (n - 3.0f) / (n + 5.0f);
            controlPoint.indices[idx] = pointIdx;

            const auto w = (1.0f / 2.0f) * 4.0f / (n * (n + 5u));

            for (const auto& edge : edgeView)
            {
                auto AddMidEdgeWeight = [&](auto v_Idx)
                {
                    for (size_t I = 0; I < A; I++)
                    {
                        if (controlPoint.indices[I] != v_Idx)
                            continue;

                        controlPoint.weights[I] += w;
                        return;
                    }

                    const auto idx = A++;
                    controlPoint.weights[idx] += 4.0f / (n * (n + 5)) * 0.5f;
                    controlPoint.indices[idx] = v_Idx;
                };

                AddMidEdgeWeight(edge.A_idx);
                AddMidEdgeWeight(edge.B_idx);
            }

            std::vector<uint32_t> faceList;

            for (size_t edge : edges)
                faceList.push_back(shape.wEdges[edge].face);

            std::sort(faceList.begin(), faceList.end());
            faceList.erase(std::unique(faceList.begin(), faceList.end()), faceList.end());

            for (auto& face : faceList)
            {
                const auto vertices     = shape.GetFaceVertices(face);
                const auto vertexCount  = vertices.size();
                const auto w            = (1.0f / vertices.size()) * 4.0f / (n * (n + 5u));

                for (auto v_Idx : vertices)
                [&]
                {
                    for (size_t I = 0; I < A; I++)
                    {
                        if (controlPoint.indices[I] != v_Idx)
                            continue;

                        controlPoint.weights[I] += w;
                        return;
                    }

                    const auto idx = A++;
                    controlPoint.indices[idx] = v_Idx;
                    controlPoint.weights[idx] = w;
                }();
            }
        }
        patch.controlPoints[B++] = controlPoint;
    }
}


/************************************************************************************************/


void CalculateQuadEdgeControlPoint(auto& vertexEdgeView, auto& faceView, auto& shape, const auto N, Patch::ControlPointWeights& outControlPoint)
{
    const auto S = Sigma(N);
    const auto L = Lambda(N);
    size_t A = 0;

    Patch::ControlPointWeights controlPoint = outControlPoint;
    for (auto& w : controlPoint.weights)
        w = 0.0f;

    // Calculate Mid Edge Points
    // M_i(1 - THETA * cos(pi / n)) * cos((2PI*I) / N)
    size_t I_1 = 0;
    for (const auto& edge : vertexEdgeView)
    {
        const float w = (1.0f - S * std::cos(pi / N)) *
                        cos((2.0f * pi * I_1) / N) *
                        (1.0f / 2.0f);

        auto AddMidEdgeWeight = [&](auto v_Idx)
        {
            for (size_t I = 0; I < A; I++)
            {
                A = Max(A, I);

                if (controlPoint.indices[I] != v_Idx)
                    continue;

                controlPoint.weights[I] += w;
                return;
            }

            const auto idx = A++;
            controlPoint.weights[idx] = w;
            controlPoint.indices[idx] = v_Idx;
        };

        AddMidEdgeWeight(edge.A_idx);
        AddMidEdgeWeight(edge.B_idx);

        I_1++;
    }

    // Calculate Face center weights
    size_t I_2 = 0;
    for (auto [face, faceIdx] : faceView)
    {
        const auto& vertices     = shape.GetFaceVertices(faceIdx);
        const auto  vertexCount  = vertices.size();
        const auto  w            = 2.0f * S * cos((2.0f * (float)pi * I_2 + (float)pi) / N) / vertexCount;

        for (auto v_Idx : vertices)
        [&]
        {
            for (size_t I = 0; controlPoint.indices[I] != 0xffffffff; I++)
            {
                A = Max(A, I);
                if (controlPoint.indices[I] != v_Idx)
                    continue;

                controlPoint.weights[I] += w;
                return;
            }

            const auto idx = A++;
            controlPoint.indices[idx] = v_Idx;
            controlPoint.weights[idx] = w;
        }();

        I_2++;
    }

    for (auto& w : controlPoint.weights)
        w *= (2.0f / 3.0f) * L * (2.0f / N);

    for (size_t I = 0; I < 32; I++)
        outControlPoint.AddWeight(controlPoint.indices[I], controlPoint.weights[I]);
}


/************************************************************************************************/


void CalculateQuadEdgePoint_E0_Plus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape)
{
    auto vertexView = shape.wFaces[quadPatchIdx].VertexView(shape);
    auto edgeView   = shape.wFaces[quadPatchIdx].EdgeView(shape);
    auto E0         = edgeView[0];

    const auto v_Idx    = E0->vertices[0];
    const auto N        = shape.wVerticeEdges[v_Idx].size() / 2.0f;

    auto faceView       = vertexView.FaceView();
    auto vertexEdgeView = vertexView.EdgeView(0);

    Patch::ControlPointWeights controlPoint = patch.controlPoints[p0];
    CalculateQuadEdgeControlPoint(vertexEdgeView, faceView, shape, N, controlPoint);

    patch.controlPoints[e0Plus] = controlPoint;

    auto E0_P = GetPoint_E0_Plus(patch, shape);
}


/************************************************************************************************/


void CalculateQuadEdgePoint_E0_Minus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape)
{
    auto vertexView = shape.wFaces[quadPatchIdx].VertexView(shape);
    auto edgeView   = shape.wFaces[quadPatchIdx].EdgeView(shape);
    auto E0         = edgeView[0];

    const auto v_Idx    = E0->vertices[0];
    const auto N        = shape.wVerticeEdges[v_Idx].size() / 2.0f;

    auto faceView       = vertexView.FaceView(1);
    auto vertexEdgeView = vertexView.EdgeView(1);

    Patch::ControlPointWeights controlPoint = patch.controlPoints[p0];
    CalculateQuadEdgeControlPoint(vertexEdgeView, faceView, shape, N, controlPoint);

    patch.controlPoints[e0Minus] = controlPoint;

    auto E0_P = GetPoint_E0_Minus(patch, shape);
}


/************************************************************************************************/


void CalculateQuadEdgePoint_E1_Plus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape)
{
    auto vertexView = shape.wFaces[quadPatchIdx].VertexView(shape, 1);
    auto edgeView   = shape.wFaces[quadPatchIdx].EdgeView(shape);
    auto E1         = edgeView[1];

    const auto v_Idx = E1->vertices[0];
    const auto N     = shape.wVerticeEdges[v_Idx].size() / 2.0f;

    auto faceView       = vertexView.FaceView();
    auto vertexEdgeView = vertexView.EdgeView();

    Patch::ControlPointWeights controlPoint = patch.controlPoints[p1];
    CalculateQuadEdgeControlPoint(vertexEdgeView, faceView, shape, N, controlPoint);

    patch.controlPoints[e1Plus] = controlPoint;

    auto P1     = GetPoint_P1(patch, shape);
    auto E1_P   = GetPoint_E1_Plus(patch, shape);
}


/************************************************************************************************/


void CalculateQuadEdgePoint_E1_Minus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape)
{
    auto vertexView = shape.wFaces[quadPatchIdx].VertexView(shape, 1);
    auto edgeView   = shape.wFaces[quadPatchIdx].EdgeView(shape);
    auto E1         = edgeView[1];

    const auto v_Idx    = E1->vertices[0];
    const auto N        = shape.wVerticeEdges[v_Idx].size() / 2.0f;

    auto faceView       = vertexView.FaceView(1);
    auto vertexEdgeView = vertexView.EdgeView(1);

    Patch::ControlPointWeights controlPoint = patch.controlPoints[p1];
    CalculateQuadEdgeControlPoint(vertexEdgeView, faceView, shape, N, controlPoint);

    patch.controlPoints[e1Minus] = controlPoint;

    auto E1_P   = GetPoint_E1_Minus(patch, shape);   
}


/************************************************************************************************/


void CalculateQuadEdgePoint_E2_Plus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape)
{
    auto vertexView = shape.wFaces[quadPatchIdx].VertexView(shape, 2);
    auto edgeView   = shape.wFaces[quadPatchIdx].EdgeView(shape);
    auto E2         = edgeView[2];

    const auto v_Idx    = E2->vertices[0];
    const auto N        = shape.wVerticeEdges[v_Idx].size() / 2.0f;

    auto faceView       = vertexView.FaceView(0);
    auto vertexEdgeView = vertexView.EdgeView(0);

    Patch::ControlPointWeights controlPoint = patch.controlPoints[p2];
    CalculateQuadEdgeControlPoint(vertexEdgeView, faceView, shape, N, controlPoint);

    patch.controlPoints[e2Plus] = controlPoint;

    auto E2_P   = GetPoint_E2_Plus(patch, shape);   
}


/************************************************************************************************/


void CalculateQuadEdgePoint_E2_Minus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape)
{
    auto vertexView = shape.wFaces[quadPatchIdx].VertexView(shape, 2);
    auto edgeView   = shape.wFaces[quadPatchIdx].EdgeView(shape);
    auto E2         = edgeView[2];

    const auto v_Idx    = E2->vertices[0];
    const auto N        = shape.wVerticeEdges[v_Idx].size() / 2.0f;

    auto faceView       = vertexView.FaceView(1);
    auto vertexEdgeView = vertexView.EdgeView(1);

    Patch::ControlPointWeights controlPoint = patch.controlPoints[p2];
    CalculateQuadEdgeControlPoint(vertexEdgeView, faceView, shape, N, controlPoint);

    patch.controlPoints[e2Minus] = controlPoint;

    auto E2_P = GetPoint_E2_Minus(patch, shape);
}


/************************************************************************************************/


void CalculateQuadEdgePoint_E3_Plus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape)
{
    auto vertexView = shape.wFaces[quadPatchIdx].VertexView(shape, 3);
    auto edgeView   = shape.wFaces[quadPatchIdx].EdgeView(shape);
    auto E2         = edgeView[3];

    const auto v_Idx    = E2->vertices[0];
    const auto N        = shape.wVerticeEdges[v_Idx].size() / 2.0f;

    auto faceView       = vertexView.FaceView(0);
    auto vertexEdgeView = vertexView.EdgeView(0);

    Patch::ControlPointWeights controlPoint = patch.controlPoints[p3];
    CalculateQuadEdgeControlPoint(vertexEdgeView, faceView, shape, N, controlPoint);

    patch.controlPoints[e3Plus] = controlPoint;

    auto E2_P = GetPoint_E3_Plus(patch, shape);
}


/************************************************************************************************/


void CalculateQuadEdgePoint_E3_Minus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape)
{
    auto vertexView = shape.wFaces[quadPatchIdx].VertexView(shape, 3);
    auto edgeView   = shape.wFaces[quadPatchIdx].EdgeView(shape);
    auto E2         = edgeView[3];

    const auto v_Idx    = E2->vertices[0];
    const auto N        = shape.wVerticeEdges[v_Idx].size() / 2.0f;

    auto faceView       = vertexView.FaceView(1);
    auto vertexEdgeView = vertexView.EdgeView(1);

    Patch::ControlPointWeights controlPoint = patch.controlPoints[p3];
    CalculateQuadEdgeControlPoint(vertexEdgeView, faceView, shape, N, controlPoint);

    patch.controlPoints[e3Minus] = controlPoint;

    auto E2_M = GetPoint_E3_Minus(patch, shape);
}


/************************************************************************************************/


Patch::ControlPointWeights CalculateQuadControlPoint_Rn(auto& vertexEdgeView, auto& edgeView, auto& faceView, ModifiableShape& shape, bool flipDirection = false)
{
    auto el = vertexEdgeView[1];
    auto er = vertexEdgeView[-1];

    auto f0 = faceView[0];
    auto fr = faceView[-1];

    Patch::ControlPointWeights controlPoint;

    size_t A = 0;

    auto AddMidEdgeWeight = [&](auto v_Idx, float factor)
    {
        const float w = 0.5f * factor;

        for (size_t I = 0; I < A; I++)
        {
            A = Max(A, I);

            if (controlPoint.indices[I] != v_Idx)
                continue;

            controlPoint.weights[I] += w;
            return;
        }

        const auto idx = A++;
        controlPoint.weights[idx] = w;
        controlPoint.indices[idx] = v_Idx;
    };

    if (!flipDirection)
    {
        AddMidEdgeWeight(el.A_idx, 1.0f / 3.0f);
        AddMidEdgeWeight(el.B_idx, 1.0f / 3.0f);
        AddMidEdgeWeight(er.A_idx, -1.0f / 3.0f);
        AddMidEdgeWeight(er.B_idx, -1.0f / 3.0f);
    }
    else
    {
        AddMidEdgeWeight(el.A_idx, -1.0f / 3.0f);
        AddMidEdgeWeight(el.B_idx, -1.0f / 3.0f);
        AddMidEdgeWeight(er.A_idx, 1.0f / 3.0f);
        AddMidEdgeWeight(er.B_idx, 1.0f / 3.0f);
    }

    auto AddFaceWeights = [&](uint32_t faceIdx, float factor)
    {
        const auto& vertices = shape.GetFaceVertices(faceIdx);
        const auto  vertexCount = vertices.size();
        const auto  w = 0.25f * factor;

        for (auto v_Idx : vertices)
            [&]
        {
            for (size_t I = 0; controlPoint.indices[I] != 0xffffffff; I++)
            {
                A = Max(A, I);
                if (controlPoint.indices[I] != v_Idx)
                    continue;

                controlPoint.weights[I] += w;
                return;
            }

            const auto idx = A++;
            controlPoint.indices[idx] = v_Idx;
            controlPoint.weights[idx] = w;
        }();
    };


    if (!flipDirection)
    {
        AddFaceWeights(f0.faceIdx, 2.0f / 3.0f);
        AddFaceWeights(fr.faceIdx, -2.0f / 3.0f);
    }
    else
    {
        AddFaceWeights(f0.faceIdx, -2.0f / 3.0f);
        AddFaceWeights(fr.faceIdx, 2.0f / 3.0f);
    }

    return controlPoint;
}


/************************************************************************************************/


void CalculateQuadControlPoint_R0_Plus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape)
{
    auto vertexView     = shape.wFaces[quadPatchIdx].VertexView(shape, 0);
    auto edgeView       = shape.wFaces[quadPatchIdx].EdgeView(shape);
    auto faceView       = vertexView.FaceView(0);
    auto vertexEdgeView = vertexView.EdgeView(0);

    auto controlPoint           = CalculateQuadControlPoint_Rn(vertexEdgeView, edgeView, faceView, shape);
    patch.controlPoints[r0Plus] = controlPoint;

    const auto a = faceView[0].faceIdx;
    const auto b = faceView[-1].faceIdx;

    const auto points_A = shape.GetFaceVertices(a);
    patch.C_0 = a;
    const auto points_B = shape.GetFaceVertices(b);

    patch.C_idx[0] = b;

    const auto r    = ApplyWeights(patch, shape, r0Plus);
    const auto C0   = shape.GetFaceCenterPoint(a);
    const auto C1   = shape.GetFaceCenterPoint(b);

    const auto eP   = GetPoint_E0_Plus(patch, shape);
    const auto eM   = GetPoint_E1_Minus(patch, shape);
    const auto P    = ApplyWeights(patch, shape, p0);

    const auto f    = (C1 * P + (float3(4.0f) - (2.0f * C0) - C1) * eP + (2 * C0 * eM) + r) / 4.0f;
}


/************************************************************************************************/


void CalculateQuadControlPoint_R0_Minus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape)
{
    auto vertexView     = shape.wFaces[quadPatchIdx].VertexView(shape, 0);
    auto edgeView       = shape.wFaces[quadPatchIdx].EdgeView(shape);
    auto faceView       = vertexView.FaceView(1);
    auto vertexEdgeView = vertexView.EdgeView(1);

    auto controlPoint           = CalculateQuadControlPoint_Rn(vertexEdgeView, edgeView, faceView, shape, true);
    patch.controlPoints[r0Minus] = controlPoint;

    const auto a = faceView[0].faceIdx;
    const auto b = faceView[-1].faceIdx;

    const auto points_B = shape.GetFaceVertices(b);

    patch.C_idx[1] = a;

    /*
    const auto r    = ApplyWeights(patch, shape, r0Minus);
    const auto C0   = shape.GetFaceCenterPoint(a);
    const auto C1   = shape.GetFaceCenterPoint(b);

    const auto eM   = GetPoint_E0_Minus(patch, shape);    // E1
    const auto eP   = GetPoint_E3_Plus(patch, shape);     // E2
    const auto P    = ApplyWeights(patch, shape, p0);

    const auto f    = (C1 * P + (float3(4.0f) - (2.0f * C0) - C1) * eM + (2 * C0 * eP) + r) / 4.0f;
    */
}


/************************************************************************************************/


void CalculateQuadControlPoint_R1_Plus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape)
{
    auto vertexView     = shape.wFaces[quadPatchIdx].VertexView(shape, 1);
    auto edgeView       = shape.wFaces[quadPatchIdx].EdgeView(shape);
    auto faceView       = vertexView.FaceView(0);
    auto vertexEdgeView = vertexView.EdgeView(0);

    auto controlPoint           = CalculateQuadControlPoint_Rn(vertexEdgeView, edgeView, faceView, shape);
    patch.controlPoints[r1Plus] = controlPoint;

    const auto b            = faceView[-1].faceIdx;
    const auto points_B     = shape.GetFaceVertices(b);

    patch.C_idx[2] = b;


    /*
    const auto a    = faceView[0].faceIdx;

    const auto r    = ApplyWeights(patch, shape, r1Plus);
    const auto C0   = shape.GetFaceCenterPoint(a);
    const auto C1   = shape.GetFaceCenterPoint(b);

    const auto eP   = GetPoint_E1_Plus(patch, shape);
    const auto eM   = GetPoint_E2_Minus(patch, shape);
    const auto P    = ApplyWeights(patch, shape, p1);

    const auto f    = (C1 * P + (float3(4.0f) - (2.0f * C0) - C1) * eP + (2 * C0 * eM) + r) / 4.0f;
    */
}


/************************************************************************************************/


void CalculateQuadControlPoint_R1_Minus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape)
{
    auto vertexView     = shape.wFaces[quadPatchIdx].VertexView(shape, 1);
    auto edgeView       = shape.wFaces[quadPatchIdx].EdgeView(shape);
    auto faceView       = vertexView.FaceView(1);
    auto vertexEdgeView = vertexView.EdgeView(1);

    auto controlPoint               = CalculateQuadControlPoint_Rn(vertexEdgeView, edgeView, faceView, shape, true);
    patch.controlPoints[r1Minus]    = controlPoint;

    const auto a        = faceView[0].faceIdx;
    const auto b        = faceView[-1].faceIdx;
    const auto points_B = shape.GetFaceVertices(b);

    patch.C_idx[3] = a;

    /*
    auto a = faceView[-1].faceIdx;
    auto b = faceView[0].faceIdx;

    const auto r    = ApplyWeights(patch, shape, r1Minus);
    const auto C0   = shape.GetFaceCenterPoint(a);
    const auto C1   = shape.GetFaceCenterPoint(b);

    const auto eP   = GetPoint_E1_Minus(patch, shape);
    const auto eM   = GetPoint_E0_Plus(patch, shape);
    const auto P    = ApplyWeights(patch, shape, p1);

    const auto f    = (C1 * P + (float3(4.0f) - (2.0f * C0) - C1) * eP + (2 * C0 * eM) + r) / 4.0f;
    */
}


/************************************************************************************************/


void CalculateQuadControlPoint_R2_Plus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape)
{
    auto vertexView     = shape.wFaces[quadPatchIdx].VertexView(shape, 2);
    auto edgeView       = shape.wFaces[quadPatchIdx].EdgeView(shape);
    auto faceView       = vertexView.FaceView(0);
    auto vertexEdgeView = vertexView.EdgeView(0);

    auto controlPoint           = CalculateQuadControlPoint_Rn(vertexEdgeView, edgeView, faceView, shape);
    patch.controlPoints[r2Plus] = controlPoint;

    const auto b            = faceView[-1].faceIdx;
    const auto points_B     = shape.GetFaceVertices(b);

    patch.C_idx[4] = b;

    /*
    const auto a = faceView[0].faceIdx;

    const auto r    = ApplyWeights(patch, shape, r2Plus);
    const auto C0   = shape.GetFaceCenterPoint(a);
    const auto C1   = shape.GetFaceCenterPoint(b);

    const auto eP   = GetPoint_E2_Plus(patch, shape);
    const auto eM   = GetPoint_E3_Minus(patch, shape);
    const auto P    = ApplyWeights(patch, shape, p2);

    auto temp1 = patch.controlPoints[e3Minus];
    auto temp2 = patch.controlPoints[e2Plus];

    auto temp3 = ApplyWeights(temp1, shape);
    auto temp4 = ApplyWeights(temp2, shape);

    auto temp5 = C0 * eM;
    auto temp6 = C1 * P;

    const auto f = (C1 * P + (float3(4.0f) - (2.0f * C0) - C1) * eP + (2 * C0 * eM) + r) / 4.0f;
    */
}


/************************************************************************************************/


void CalculateQuadControlPoint_R2_Minus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape)
{
    auto vertexView     = shape.wFaces[quadPatchIdx].VertexView(shape, 2);
    auto edgeView       = shape.wFaces[quadPatchIdx].EdgeView(shape);
    auto faceView       = vertexView.FaceView(1);
    auto vertexEdgeView = vertexView.EdgeView(1);

    auto controlPoint               = CalculateQuadControlPoint_Rn(vertexEdgeView, edgeView, faceView, shape, true);
    patch.controlPoints[r2Minus]    = controlPoint;

    const auto a          = faceView[-1].faceIdx;
    const auto b          = faceView[0].faceIdx;
    const auto points_B   = shape.GetFaceVertices(b);

    patch.C_idx[5] = a;

    /*
    const auto r    = ApplyWeights(patch, shape, r2Minus);
    const auto C0   = shape.GetFaceCenterPoint(a);
    const auto C1   = shape.GetFaceCenterPoint(b);

    const auto eP   = GetPoint_E2_Minus(patch, shape);
    const auto eM   = GetPoint_E1_Plus(patch, shape);
    const auto P    = ApplyWeights(patch, shape, p2);

    const auto f    = (C1 * P + (float3(4.0f) - (2.0f * C0) - C1) * eP + (2 * C0 * eM) + r) / 4.0f;
    */
}

/************************************************************************************************/


void CalculateQuadControlPoint_R3_Plus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape)
{
    auto vertexView     = shape.wFaces[quadPatchIdx].VertexView(shape, 3);
    auto edgeView       = shape.wFaces[quadPatchIdx].EdgeView(shape);
    auto faceView       = vertexView.FaceView(0);
    auto vertexEdgeView = vertexView.EdgeView(0);

    auto controlPoint           = CalculateQuadControlPoint_Rn(vertexEdgeView, edgeView, faceView, shape);
    patch.controlPoints[r3Plus] = controlPoint;

    const auto a        = faceView[0].faceIdx;
    const auto b        = faceView[-1].faceIdx;
    const auto points_B = shape.GetFaceVertices(b);

    patch.C_idx[6] = b;

    /*
    const auto a    = faceView[0].faceIdx;
    const auto r    = ApplyWeights(patch, shape, r3Plus);
    const auto C0   = shape.GetFaceCenterPoint(a);
    const auto C1   = shape.GetFaceCenterPoint(b);

    const auto eP   = GetPoint_E3_Plus(patch, shape);
    const auto eM   = GetPoint_E0_Minus(patch, shape);
    const auto P    = ApplyWeights(patch, shape, p3);

    const auto f    = (C1 * P + (float3(4.0f) - (2.0f * C0) - C1) * eP + (2 * C0 * eM) + r) / 4.0f;
    */
}


/************************************************************************************************/


void CalculateQuadControlPoint_R3_Minus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape)
{
    auto vertexView     = shape.wFaces[quadPatchIdx].VertexView(shape, 3);
    auto edgeView       = shape.wFaces[quadPatchIdx].EdgeView(shape);
    auto faceView       = vertexView.FaceView(1);
    auto vertexEdgeView = vertexView.EdgeView(1);

    auto controlPoint               = CalculateQuadControlPoint_Rn(vertexEdgeView, edgeView, faceView, shape, true);
    patch.controlPoints[r3Minus]    = controlPoint;

    const auto a          = faceView[0].faceIdx;
    const auto b          = faceView[-1].faceIdx;
    const auto points_A   = shape.GetFaceVertices(a);

    patch.C_idx[7] = b;

    /*
    auto b = faceView[0].faceIdx;

    const auto r    = ApplyWeights(patch, shape, r2Minus);
    const auto C0   = shape.GetFaceCenterPoint(a);
    const auto C1   = shape.GetFaceCenterPoint(b);

    const auto eP   = GetPoint_E3_Minus(patch, shape);
    const auto eM   = GetPoint_E2_Plus(patch, shape);
    const auto P    = ApplyWeights(patch, shape, p3);

    const auto f    = (C1 * P + (float3(4.0f) - (2.0f * C0) - C1) * eP + (2 * C0 * eM) + r) / 4.0f;
    */
}

/************************************************************************************************/


Patch BuidRegularPatch(uint32_t quadPatch, ModifiableShape& shape)
{
    Patch patch;

    CalculateQuadCornerPoints(quadPatch, patch, shape);
    CalculateQuadEdgePoint_E0_Plus(quadPatch, patch, shape);
    CalculateQuadEdgePoint_E0_Minus(quadPatch, patch, shape);

    CalculateQuadEdgePoint_E1_Plus(quadPatch, patch, shape);
    CalculateQuadEdgePoint_E1_Minus(quadPatch, patch, shape);

    CalculateQuadEdgePoint_E2_Plus(quadPatch, patch, shape);
    CalculateQuadEdgePoint_E2_Minus(quadPatch, patch, shape);

    CalculateQuadEdgePoint_E3_Plus(quadPatch, patch, shape);
    CalculateQuadEdgePoint_E3_Minus(quadPatch, patch, shape);

    CalculateQuadControlPoint_R0_Plus(quadPatch, patch, shape);
    CalculateQuadControlPoint_R0_Minus(quadPatch, patch, shape);

    CalculateQuadControlPoint_R1_Plus(quadPatch, patch, shape);
    CalculateQuadControlPoint_R1_Minus(quadPatch, patch, shape);

    CalculateQuadControlPoint_R2_Plus(quadPatch, patch, shape);
    CalculateQuadControlPoint_R2_Minus(quadPatch, patch, shape);

    CalculateQuadControlPoint_R3_Plus(quadPatch, patch, shape);
    CalculateQuadControlPoint_R3_Minus(quadPatch, patch, shape);

    return patch;
}


void CalculateCornerEdge(Patch& patch, uint32_t edge, uint32_t target, bool Minus, ModifiableShape& shape)
{
    const uint32_t v0 = shape.wEdges[edge].vertices[0];
    const uint32_t v1 = shape.wEdges[edge].vertices[1];

    size_t idx0 = 0;
    size_t idx1 = 0;
    for (; idx0 < 32 && patch.controlPoints[target].weights[idx0] != v0; idx0++);
    for (; idx1 < 32 && patch.controlPoints[target].weights[idx1] != v1; idx1++);


    if (Minus)
    {
        patch.controlPoints[target].AddWeight(v0, 1.0f / 3.0f);
        patch.controlPoints[target].AddWeight(v1, 2.0f / 3.0f);
    }
    else
    {
        patch.controlPoints[target].AddWeight(v0, 2.0f / 3.0f);
        patch.controlPoints[target].AddWeight(v1, 1.0f / 3.0f);
    }
}

void CalculateTJunction(Patch& patch, uint32_t edge, uint32_t target, uint32_t point, bool Minus, ModifiableShape& shape)
{
    auto& hEdge = shape.wEdges[edge];

    const auto v0       = shape.wEdges[edge].vertices[Minus ? 1 : 0];
    const auto v1       = shape.wEdges[edge].vertices[Minus ? 0 : 1];
    const auto& edges   = shape.wVerticeEdges[v0];

    Patch::ControlPointWeights controlPoint;// = patch.controlPoints[point];

    if (hEdge.oppositeNeighbor != 0xffffffff)
    {   // Interner Edge
        const auto f1 = shape.wEdges[edge].face;
        const auto f2 = shape.wEdges[hEdge.oppositeNeighbor].face;

        const auto f1_verts = shape.GetFaceVertices(f1);
        const auto f2_verts = shape.GetFaceVertices(f2);

        {
            auto w = f1_verts.size() == 4 ? (1.0f / 12.0f) : (2.0f / 12.0f);
            for (auto v : f1_verts)
                if(v != v0 && v!= v1)
                    controlPoint.AddWeight(v, w);
        }

        {
            auto w = f2_verts.size() == 4 ? (1.0f / 12.0f) : (2.0f / 12.0f);
            for (auto v : f2_verts)
                if (v != v0 && v != v1)
                    controlPoint.AddWeight(v, w);
        }

        controlPoint.AddWeight(v0, (2 / 3.0f) * (2.0f / 3.0f));
        controlPoint.AddWeight(v1, (2 / 3.0f) * (1.0f / 3.0f));
    }
    else
    {   // exterior Edge
        static_vector<uint32_t, 2> boundaryEdges;
        for (size_t I = 0; I < 4; I++)
        {
            const auto e    = edges[I];
            const auto twin = shape.wEdges[edges[I]].oppositeNeighbor;

            if (twin == 0xffffffff)
                boundaryEdges.push_back(e);
        }


        uint32_t verts[] = {
            shape.wEdges[boundaryEdges[0]].vertices[0],
            shape.wEdges[boundaryEdges[0]].vertices[1],
            shape.wEdges[boundaryEdges[1]].vertices[0],
            shape.wEdges[boundaryEdges[1]].vertices[1] };

        std::remove_if(verts, verts + 4, [&](auto v) { return v == v0; });

        if (verts[0] != v1)
        {
            controlPoint.AddWeight(verts[1], 2.0f / 3.0f);
            controlPoint.AddWeight(verts[0], 1.0f / 3.0f);
        }
        else
        {
            controlPoint.AddWeight(verts[0], 2.0f / 3.0f);
            controlPoint.AddWeight(verts[1], 1.0f / 3.0);
        }
    }

    patch.controlPoints[target] = controlPoint;
}


Patch BuildQuadEdgePatch(uint32_t edgePatch, ModifiableShape& shape)
{
    static const bool Plus  = false;
    static const bool Minus = true;


    Patch patch;

    CalculateQuadCornerPoints(edgePatch, patch, shape);

    auto edgeView = shape.wFaces[edgePatch].EdgeView(shape);

    {   // Edge 0
        auto edge = edgeView[0];

        // V0
        switch (shape.wVerticeEdges[edge->vertices[0]].size())
        {
        case 2: // Corner
        {
            CalculateCornerEdge(patch, edge, e0Plus, Plus, shape);
        }   break;
        case 4: // T-Junction
        {
            CalculateTJunction(patch, edge, e0Plus, p0, Plus, shape);
        }   break;
        default:
        {
            CalculateQuadEdgePoint_E0_Plus(edgePatch, patch, shape);
            CalculateQuadEdgePoint_E0_Minus(edgePatch, patch, shape);
            CalculateQuadControlPoint_R0_Minus(edgePatch, patch, shape);
            CalculateQuadControlPoint_R0_Plus(edgePatch, patch, shape);
        }   break;
        {
            int x = 0;
        }   break;
        }

        // V1
        switch (shape.wVerticeEdges[edge->vertices[1]].size())
        {
        case 2:
            CalculateCornerEdge(patch, edge, e1Minus, Minus, shape);
            break;
        case 4:
            CalculateTJunction(patch, edge, e1Minus, p1, Minus, shape);
            break;
        default:
            break;
        }
    }

    {   // Edge 1
        auto edge = edgeView[1];

        //V0
        switch (shape.wVerticeEdges[edge->vertices[0]].size())
        {
        case 2: // Corner
        {
            CalculateCornerEdge(patch, edge, e1Plus, Plus, shape);
        }   break;
        case 4: // T-Junction
        {
            CalculateTJunction(patch, edge, e1Plus, p1, Plus, shape);
        }   break;
        default:
        {
            CalculateQuadEdgePoint_E1_Plus(edgePatch, patch, shape);
            CalculateQuadEdgePoint_E1_Minus(edgePatch, patch, shape);
            CalculateQuadControlPoint_R1_Minus(edgePatch, patch, shape);
            CalculateQuadControlPoint_R1_Plus(edgePatch, patch, shape);
        }   break;
        }


        // V1
        switch (shape.wVerticeEdges[edge->vertices[1]].size())
        {
        case 2:
            CalculateCornerEdge(patch, edge, e2Minus, Minus, shape);
            break;
        case 4:
            CalculateTJunction(patch, edge, e2Minus, p2, Minus, shape);
            break;
        default:
            break;
        }
    }

    {   // Edge 2
        auto edge = edgeView[2];
        auto temp2 = shape.wVerticeEdges[edge->vertices[1]].size();

        switch (shape.wVerticeEdges[edge->vertices[0]].size())
        {
        case 2: // Corner
        {
            CalculateCornerEdge(patch, edge, e2Plus, Plus, shape);
        }   break;
        case 4: // T-Junction
        {
            CalculateTJunction(patch, edge, e2Plus, p2, Plus, shape);
        }   break;
        default:
        {
            CalculateQuadEdgePoint_E2_Plus(edgePatch, patch, shape);
            CalculateQuadEdgePoint_E2_Minus(edgePatch, patch, shape);
            CalculateQuadControlPoint_R2_Minus(edgePatch, patch, shape);
            CalculateQuadControlPoint_R2_Plus(edgePatch, patch, shape);
        }   break;
        }

        // V1
        switch (shape.wVerticeEdges[edge->vertices[1]].size())
        {
        case 2:
            CalculateCornerEdge(patch, edge, e3Minus, Minus, shape);
            break;
        case 4:
            CalculateTJunction(patch, edge, e3Minus, p3, Minus, shape);
            break;
        default:
            break;
        }
    }

    {   // Edge 3
        auto edge = edgeView[3];

        auto v2 = shape.wVerticeEdges[edge->vertices[1]].size();

        switch (shape.wVerticeEdges[edge->vertices[0]].size())
        {
        case 2: // Corner
        {
            CalculateCornerEdge(patch, edge, e3Plus, Plus, shape);
        }   break;
        case 4: // T-Junction
        {
            CalculateTJunction(patch, edge, e3Plus, p3, Plus, shape);
        }   break;
        default:
        {
            CalculateQuadEdgePoint_E3_Plus(edgePatch, patch, shape);
            CalculateQuadEdgePoint_E3_Minus(edgePatch, patch, shape);
            CalculateQuadControlPoint_R3_Minus(edgePatch, patch, shape);
            CalculateQuadControlPoint_R3_Plus(edgePatch, patch, shape);
        }   break;
        }

        // V1
        switch (shape.wVerticeEdges[edge->vertices[1]].size())
        {
        case 2:
            CalculateCornerEdge(patch, edge, e0Minus, Minus, shape);
            break;
        case 4:
            CalculateTJunction(patch, edge, e0Minus, p0, Minus, shape);
            break;
        default:
            break;
        }
    }

    return patch;
}


auto CalculateControlPointWeights(PatchGroups& classifiedPatches, ModifiableShape& shape)
{
    std::vector<Patch> patches;

    for (auto& quadPatch : classifiedPatches.QuadPatches)
    {
        const Patch regularPatch = BuidRegularPatch(quadPatch, shape);

        patches.push_back(regularPatch);
    }

    for (auto& borderPatch : classifiedPatches.edgeQuadPatches)
    {
        const Patch edgePatch = BuildQuadEdgePatch(borderPatch, shape);
        patches.push_back(edgePatch);
    }


    for (auto& patch : patches)
    {
        std::vector<uint32_t> verticies;

        for (const auto& controlPoint : patch.controlPoints)
            for (const auto idx : controlPoint.indices)
                verticies.push_back(idx);

        std::sort(verticies.begin(), verticies.end());
        verticies.erase(std::unique(verticies.begin(), verticies.end()), verticies.end());

        while (verticies.back() == 0xffffffff)
            verticies.pop_back();

        while (verticies.size() < 32)
            verticies.push_back(verticies.front());

        for (const auto idx : verticies)
            patch.inputVertices[patch.vertCount++] = idx;
    }

    return patches;
};


/************************************************************************************************/


class TessellationTest : public FlexKit::FrameworkState
{
public:
    TessellationTest(FlexKit::GameFramework& IN_framework, int test) :
        FrameworkState{ IN_framework },
        renderWindow    {
            std::get<0>(CreateWin32RenderWindow(
                IN_framework.GetRenderSystem(),
                FlexKit::DefaultWindowDesc(uint2{ 1920, 1080 }))) },
        rootSig         { IN_framework.core.GetBlockMemory() },
        vertexBuffer    { IN_framework.GetRenderSystem().CreateVertexBuffer(MEGABYTE, false) },
        constantBuffer  { IN_framework.GetRenderSystem().CreateConstantBuffer(MEGABYTE, false) },
        cameras         { IN_framework.core.GetBlockMemory() },
        depthBuffer     { IN_framework.GetRenderSystem().CreateDepthBuffer(uint2{ 1920, 1080 }, true) },
        debug1Buffer    { IN_framework.GetRenderSystem().CreateUAVBufferResource(MEGABYTE, false) },
        debug2Buffer    { IN_framework.GetRenderSystem().CreateUAVBufferResource(MEGABYTE, false) },
        indices         { IN_framework.core.GetBlockMemory() },
        orbitCamera     { gameObject, CameraComponent::GetComponent().CreateCamera(), 10.0f },
        nodes           {}
    {
        patches     = PreComputePatches(IN_framework.core.GetBlockMemory() );
        vertices    = GetGregoryVertexBuffer(shape, framework.core.GetBlockMemory());
        patchBuffer = MoveBufferToDevice(framework.GetRenderSystem(), (const char*)patches.data(), patches.ByteSize());

        rootSig.SetParameterAsSRV(0, 0, 0);
        rootSig.SetParameterAsCBV(1, 0, 0, PIPELINE_DEST_ALL);
        rootSig.SetParameterAsUINT(2, 16, 1, 0, PIPELINE_DEST_ALL);
        rootSig.SetParameterAsUINT(3, 1, 2, 0, PIPELINE_DEST_ALL);
        rootSig.SetParameterAsUAV(4, 0, 0, PIPELINE_DEST_ALL);
        rootSig.SetParameterAsUAV(5, 1, 0, PIPELINE_DEST_ALL);
        rootSig.SetParameterAsUAV(6, 2, 0, PIPELINE_DEST_ALL);

        rootSig.AllowIA = true;
        rootSig.AllowSO = false;
        FK_ASSERT(rootSig.Build(IN_framework.GetRenderSystem(), IN_framework.core.GetTempMemory()));

        framework.GetRenderSystem().RegisterPSOLoader(ACCQuad,                  { &rootSig, [&](auto* renderSystem) { return LoadPSO(renderSystem); } });
        framework.GetRenderSystem().RegisterPSOLoader(ACCDebugControlpoints,    { &rootSig, [&](auto* renderSystem) { return LoadPSO2(renderSystem); } });
        framework.GetRenderSystem().RegisterPSOLoader(ACCDebugNormals,          { &rootSig, [&](auto* renderSystem) { return LoadPSO3(renderSystem); } });

        FlexKit::EventNotifier<>::Subscriber sub;
        sub.Notify  = &FlexKit::EventsWrapper;
        sub._ptr    = &framework;

        renderWindow.Handler->Subscribe(sub);
        renderWindow.SetWindowTitle("[Tessellation Test]");
        renderWindow.EnableCaptureMouse(true);
        //activeCamera    = CameraComponent::GetComponent().CreateCamera((float)pi/3.0f, 1920.0f / 1080.0f);
        node            = GetZeroedNode();
        orbitCamera.SetCameraAspectRatio(1920.0f / 1080.0f);
        orbitCamera.SetCameraFOV((float)pi / 3.0f);
        //TranslateWorld(node, float3( 0, -3, -20.0f ));
        //Pitch(node, pi / 6);

        //Yaw(node, pi);
        //Pitch(node, pi / 2);
    }


    /************************************************************************************************/


    static_vector<uint2> FindSharedEdges(QuadPatch& quadPatch, uint32_t vertexIdx)
    {
        static_vector<uint2> out;

        for(size_t I = 0; I < 4; I++)
        if (quadPatch.vertexIds[I] == vertexIdx)
        {
            out.emplace_back(quadPatch.vertexIds[I], quadPatch.vertexIds[(I + 1) % 4]);
            out.emplace_back(quadPatch.vertexIds[(I + 3) % 4], quadPatch.vertexIds[I]);
        }

        return out;
    }


    /************************************************************************************************/

    // V20 ------ V21 ----- V22 ------- V23 ------ V24
    // |          |          |           |          |
    // |          |          |           |          |
    // |   P12    |    p13   |    p14    |    p15   |
    // |          |          |           |          |
    // |          |          |           |          |
    // V7 ------- V6 ------- V11 ------ V15 ------ V19
    // |          |          |           |          |
    // |          |          |           |          |
    // |   P2     |    p5    |    p8     |    p11   |
    // |          |          |           |          |
    // |          |          |           |          |
    // V5 ------- V4 ------- V10 ------ V14 ------ V18
    // |          |          |           |          |
    // |          |          |           |          |
    // |   p1     |    p4    |    p7     |    p10   |
    // |          |          |           |          |
    // |          |          |           |          |
    // V3 ------- V2 ------- V9 ------- V13 ------ V17
    // |          |          |           |          |
    // |          |          |           |          |
    // |   p0     |    p3    |    p6     |    p9    |
    // |          |          |           |          |
    // |          |          |           |          |
    // V0 ------- V1 ------- V8 ------- V12 ------ V16


    /************************************************************************************************/


    Vector<GPUPatch> PreComputePatches(iAllocator& allocator)
    {
        // Patch 1
        shape.AddVertex(float3{ 0.0f, 0.0f, 0.0f }); // 0
        shape.AddVertex(float3{ 1.0f, 0.0f, 0.0f }); // 1
        shape.AddVertex(float3{ 1.0f, 1.0f, 1.0f }); // 2
        shape.AddVertex(float3{ 0.0f, 0.0f, 1.0f }); // 3

        shape.AddVertex(float3{ 1.0f, 1.0f, 2.0f }); // 4
        shape.AddVertex(float3{ 0.0f, 0.0f, 2.0f }); // 5
        shape.AddVertex(float3{ 1.0f, 0.0f, 3.0f }); // 6
        shape.AddVertex(float3{ 0.0f, 0.0f, 3.0f }); // 7

        shape.AddVertex(float3{ 2.0f, 0.0f, 0.0f }); // 8
        shape.AddVertex(float3{ 2.0f, 1.0f, 1.0f }); // 9
        shape.AddVertex(float3{ 2.0f, 1.0f, 2.0f }); // 10
        shape.AddVertex(float3{ 2.0f, 0.0f, 3.0f }); // 11

        shape.AddVertex(float3{ 3.0f, 0.0f, 0.0f }); // 12
        shape.AddVertex(float3{ 3.0f, 0.0f, 1.0f }); // 13
        shape.AddVertex(float3{ 3.0f, 0.0f, 2.0f }); // 14
        shape.AddVertex(float3{ 3.0f, 0.0f, 3.0f }); // 15

        shape.AddVertex(float3{ 4.0f, 0.0f, 0.0f }); // 16
        shape.AddVertex(float3{ 4.0f, 0.0f, 1.0f }); // 17
        shape.AddVertex(float3{ 4.0f, 0.0f, 2.0f }); // 18
        shape.AddVertex(float3{ 4.0f, 0.0f, 3.0f }); // 19

        shape.AddVertex(float3{ 0.0f, 0.0f, 4.0f }); // 20
        shape.AddVertex(float3{ 1.0f, 0.0f, 4.0f }); // 21
        shape.AddVertex(float3{ 2.0f, 0.0f, 4.0f }); // 22
        shape.AddVertex(float3{ 3.0f, 0.0f, 4.0f }); // 23
        shape.AddVertex(float3{ 4.0f, 0.0f, 4.0f }); // 24

        const uint32_t patch0[] = { 0, 1, 2, 3 };
        const uint32_t patch1[] = { 3, 2, 4, 5 };
        const uint32_t patch2[] = { 5, 4, 6, 7 };
        const uint32_t patch3[] = { 1, 8, 9, 2 };
        const uint32_t patch4[] = { 2, 9, 10, 4 };
        const uint32_t patch5[] = { 4, 10, 11, 6 };
        const uint32_t patch6[] = { 8, 12, 13, 9 };
        const uint32_t patch7[] = { 9, 13, 14, 10 };
        const uint32_t patch8[] = { 10, 14, 15, 11 };

        const uint32_t patch9[] = { 12, 16, 17, 13 };
        const uint32_t patch10[] = { 13, 17, 18, 14 };
        const uint32_t patch11[] = { 14, 18, 19, 15 };

        const uint32_t patch12[] = { 7, 6, 21, 20 };
        const uint32_t patch13[] = { 6, 11, 22, 21 };
        const uint32_t patch14[] = { 11, 15, 23, 22 };
        const uint32_t patch15[] = { 15, 19, 24, 23 };

        shape.AddPolygon(patch0, patch0 + 4);
        shape.AddPolygon(patch1, patch1 + 4);
        shape.AddPolygon(patch2, patch2 + 4);
        shape.AddPolygon(patch3, patch3 + 4);
        shape.AddPolygon(patch4, patch4 + 4);
        shape.AddPolygon(patch5, patch5 + 4);
        shape.AddPolygon(patch6, patch6 + 4);
        shape.AddPolygon(patch7, patch7 + 4);
        shape.AddPolygon(patch8, patch8 + 4);
        shape.AddPolygon(patch9, patch9 + 4);
        shape.AddPolygon(patch10, patch10 + 4);
        shape.AddPolygon(patch11, patch11 + 4);

        shape.AddPolygon(patch12, patch12 + 4);
        shape.AddPolygon(patch13, patch13 + 4);
        shape.AddPolygon(patch14, patch14 + 4);
        shape.AddPolygon(patch15, patch15 + 4);

        auto classifiedPatches  = ClassifyPatches(shape);
        auto patches            = CalculateControlPointWeights(classifiedPatches, shape);

        Vector<GPUPatch> GPUControlPoints{ allocator };

        for (auto& patch : patches)
            CreateGPUPatch(patch, shape, GPUControlPoints, indices);

        return GPUControlPoints;
    }


    /************************************************************************************************/


    Vector<GPUVertex> GetGregoryVertexBuffer(ModifiableShape& shape, iAllocator& allocator)
    {
        Vector<GPUVertex> vertices{ allocator };

        for (const auto& vertex : shape.wVertices)
        {
            GPUVertex p;
            memcpy(&p, &vertex.point, sizeof(p));
            vertices.push_back(p);
        }

        return vertices;
    }


    /************************************************************************************************/


    void DrawPatch(UpdateDispatcher& dispatcher, FlexKit::FrameGraph& frameGraph)
    {
        struct DrawPatch
        {
            FrameResourceHandle renderTarget;
            FrameResourceHandle depthTarget;
            FrameResourceHandle debug1Buffer;
            FrameResourceHandle debug2Buffer;
        };

        auto& cameraUpdate = CameraComponent::GetComponent().QueueCameraUpdate(dispatcher);

        frameGraph.AddNode<DrawPatch>(
            DrawPatch{},
            [&](FrameGraphNodeBuilder& builder, DrawPatch& draw)
            {
                builder.AddDataDependency(cameraUpdate);
                draw.renderTarget   = builder.RenderTarget(renderWindow.GetBackBuffer());
                draw.depthTarget    = builder.DepthTarget(depthBuffer);
                draw.debug1Buffer    = builder.UnorderedAccess(debug1Buffer);
                draw.debug2Buffer    = builder.UnorderedAccess(debug2Buffer);
            },
            [&](DrawPatch& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
            {
                VBPushBuffer Vbuffer{ vertexBuffer, 8196, *ctx.renderSystem };
                CBPushBuffer Cbuffer{ constantBuffer, 1024 * 16, *ctx.renderSystem };

                const VertexBufferDataSet VB{ vertices, Vbuffer};
                const VertexBufferDataSet IB{ indices, Vbuffer};

                const float4x4 WT       = GetWT(node).Transpose();
                const auto constants    = GetCameraConstants(orbitCamera.camera);
                
                const ConstantBufferDataSet CB{ constants, Cbuffer };

                ctx.ClearDepthBuffer(depthBuffer, 1.0f);
                ctx.ClearUAVBufferRange(resources.UAV(data.debug1Buffer, ctx), 0, 64);
                ctx.ClearUAVBufferRange(resources.UAV(data.debug2Buffer, ctx), 0, 64);
                ctx.AddUAVBarrier(resources.UAV(data.debug1Buffer, ctx));
                ctx.AddUAVBarrier(resources.UAV(data.debug2Buffer, ctx));

                ctx.SetRootSignature(rootSig);
                ctx.SetPipelineState(resources.GetPipelineState(ACCQuad));
                ctx.SetPrimitiveTopology(EInputTopology::EIT_PATCH_CP_32);
                ctx.SetVertexBuffers({ VB });
                ctx.SetIndexBuffer(IB);
                ctx.SetScissorAndViewports({ resources.GetRenderTarget(data.renderTarget) });
                ctx.SetRenderTargets({ resources.GetRenderTarget(data.renderTarget) }, true, depthBuffer);

                const float expansionRate = 16.0f;// 1 + cos(t) * 63.0f / 2 + 63.0f / 2.0f;

                ctx.SetGraphicsShaderResourceView(0, patchBuffer);
                ctx.SetGraphicsConstantBufferView(1, CB);
                ctx.SetGraphicsConstantValue(2, 16, &WT);
                ctx.SetGraphicsConstantValue(3, 1, &expansionRate);
                ctx.SetGraphicsUnorderedAccessView(4, resources.UAV(data.debug1Buffer, ctx), 0);
                ctx.SetGraphicsUnorderedAccessView(5, resources.UAV(data.debug1Buffer, ctx), 64);
                ctx.SetGraphicsUnorderedAccessView(6, resources.UAV(data.debug2Buffer, ctx), 0);

                ctx.DrawIndexed(indices.size());
                //ctx.AddUAVBarrier(resources.UAV(data.debug1Buffer, ctx));
                //ctx.AddUAVBarrier(resources.UAV(data.debug2Buffer, ctx));

                const auto devicePointer1 = resources.GetDevicePointer(resources.VertexBuffer(data.debug2Buffer, ctx));
                ctx.SetPipelineState(resources.GetPipelineState(ACCDebugControlpoints));
                ctx.SetPrimitiveTopology(EInputTopology::EIT_POINT);
                ctx.SetVertexBuffers2({ D3D12_VERTEX_BUFFER_VIEW{ devicePointer1, MEGABYTE, 12 } });

                ctx.Draw(20 * indices.size() / 32);

                const auto devicePointer2 = resources.GetDevicePointer(resources.VertexBuffer(data.debug1Buffer, ctx));
                ctx.SetPipelineState(resources.GetPipelineState(ACCDebugNormals));
                ctx.SetPrimitiveTopology(EInputTopology::EIT_LINE);
                ctx.SetVertexBuffers2({ D3D12_VERTEX_BUFFER_VIEW{ devicePointer2 + 64, MEGABYTE - 64, 12 } });

                ctx.Draw(16 * 16 * 9 * 16);
            });
    }


    /************************************************************************************************/


    FlexKit::UpdateTask* Draw(FlexKit::UpdateTask* updateTask, FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT, FlexKit::FrameGraph& frameGraph)
    {
        ClearBackBuffer(frameGraph, renderWindow.GetBackBuffer(), { 0.0f, 0.0f, 0.0f, 1.0f });

        renderWindow.UpdateCapturedMouseInput(dT);

        auto& orbitCameraUpdate = QueueOrbitCameraUpdateTask(dispatcher, orbitCamera, renderWindow.mouseState, dT);
        auto& transformUpdate   = QueueTransformUpdateTask(dispatcher);
        auto& cameraUpdate      = CameraComponent::GetComponent().QueueCameraUpdate(dispatcher);

        frameGraph.dataDependencies.push_back(&orbitCameraUpdate);

        DrawPatch(dispatcher, frameGraph);

        PresentBackBuffer(frameGraph, renderWindow.GetBackBuffer());

        t += dT;

        return nullptr;
    }


    /************************************************************************************************/


    FlexKit::UpdateTask* Update(FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT)
    { 
        FlexKit::UpdateInput();
        renderWindow.UpdateCapturedMouseInput(dT);

        return nullptr;
    }


    /************************************************************************************************/


    void PostDrawUpdate(FlexKit::EngineCore& core, double dT) override
    {
        renderWindow.Present(core.vSync ? 1 : 0, 0);
    }


    /************************************************************************************************/
    

    bool EventHandler(FlexKit::Event evt) override
    {
        switch (evt.InputSource)
        {
        case FlexKit::Event::E_SystemEvent:
            if (evt.Action == FlexKit::Event::InputAction::Exit)
            {
                framework.quit = true;
                return true;
            }
            break;
        case FlexKit::Event::InputType::Keyboard:
        {
            switch (evt.mData1.mKC[0])
            {
            case KC_W:
            case KC_A:
            case KC_S:
            case KC_D:
            case KC_Q:
            case KC_E:
                orbitCamera.HandleEvent(evt);
                return true;
                break;
            case KC_M:
                if(evt.Action == Event::Release)
                    renderWindow.ToggleMouseCapture();
                break;
            }

            if (evt.mType == Event::EventType::Input && evt.Action == Event::Release && evt.mData1.mKC[0] == KC_R)
            {
                framework.GetRenderSystem().QueuePSOLoad(ACCQuad);
                framework.GetRenderSystem().QueuePSOLoad(ACCDebugControlpoints);
                framework.GetRenderSystem().QueuePSOLoad(ACCDebugNormals);
            }
        }   break;
        };

        return false;
    }


    /************************************************************************************************/


    ID3D12PipelineState* LoadPSO(FlexKit::RenderSystem* renderSystem)
    {
        auto ACC_VShader = renderSystem->LoadShader("VS_Main", "vs_6_2", "assets\\shaders\\ACCQuad.hlsl", true);
        auto ACC_DShader = renderSystem->LoadShader("DS_Main", "ds_6_2", "assets\\shaders\\ACCQuad.hlsl", true);
		auto ACC_HShader = renderSystem->LoadShader("HS_Main", "hs_6_2", "assets\\shaders\\ACCQuad.hlsl", true);
        auto ACC_PShader = renderSystem->LoadShader("PS_Main", "ps_6_2", "assets\\shaders\\ACCQuad.hlsl", true);


		/*
		typedef struct D3D12_INPUT_ELEMENT_DESC
		{
		LPCSTR SemanticName;
		UINT SemanticIndex;
		DXGI_FORMAT Format;
		UINT InputSlot;
		UINT AlignedByteOffset;
		D3D12_INPUT_CLASSIFICATION InputSlotClass;
		UINT InstanceDataStepRate;
		} 	D3D12_INPUT_ELEMENT_DESC;
		*/

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//			{ "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//			{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        Rast_Desc.CullMode              = D3D12_CULL_MODE_NONE;
        //Rast_Desc.FillMode              = D3D12_FILL_MODE_WIREFRAME;
        //Rast_Desc.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = rootSig;
			PSO_Desc.VS                    = ACC_VShader;
			PSO_Desc.DS                    = ACC_DShader;
			PSO_Desc.HS                    = ACC_HShader;
			PSO_Desc.PS                    = ACC_PShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
			PSO_Desc.NumRenderTargets      = 1;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // renderTarget
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.DepthStencilState     = Depth_Desc;
			PSO_Desc.InputLayout           = { InputElements, 1 };
			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = renderSystem->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* LoadPSO2(FlexKit::RenderSystem* renderSystem)
    {
        auto Debug_VShader = renderSystem->LoadShader("VS_Main", "vs_6_2", "assets\\shaders\\ACCDebug.hlsl", true);
        auto Debug_GShader = renderSystem->LoadShader("GS_Main", "gs_6_0", "assets\\shaders\\ACCDebug.hlsl");
        auto Debug_PShader = renderSystem->LoadShader("PS_Main", "ps_6_2", "assets\\shaders\\ACCDebug.hlsl", true);

		/*
		typedef struct D3D12_INPUT_ELEMENT_DESC
		{
		LPCSTR SemanticName;
		UINT SemanticIndex;
		DXGI_FORMAT Format;
		UINT InputSlot;
		UINT AlignedByteOffset;
		D3D12_INPUT_CLASSIFICATION InputSlotClass;
		UINT InstanceDataStepRate;
		} 	D3D12_INPUT_ELEMENT_DESC;
		*/

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        Rast_Desc.CullMode              = D3D12_CULL_MODE_NONE;
        //Rast_Desc.FillMode              = D3D12_FILL_MODE_WIREFRAME;
        //Rast_Desc.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature         = rootSig;
			PSO_Desc.VS                     = Debug_VShader;
			PSO_Desc.GS                     = Debug_GShader;
			PSO_Desc.PS                     = Debug_PShader;
			PSO_Desc.RasterizerState        = Rast_Desc;
			PSO_Desc.BlendState             = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask             = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			PSO_Desc.NumRenderTargets       = 1;
			PSO_Desc.RTVFormats[0]          = DXGI_FORMAT_R16G16B16A16_FLOAT; // renderTarget
			PSO_Desc.SampleDesc.Count       = 1;
			PSO_Desc.SampleDesc.Quality     = 0;
			PSO_Desc.DSVFormat              = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.DepthStencilState      = Depth_Desc;
			PSO_Desc.InputLayout            = { InputElements, 1 };
			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = renderSystem->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
    }


    ID3D12PipelineState* LoadPSO3(FlexKit::RenderSystem* renderSystem)
    {
        auto Debug_VShader = renderSystem->LoadShader("VS2_Main", "vs_6_2", "assets\\shaders\\ACCDebug.hlsl", true);
        auto Debug_PShader = renderSystem->LoadShader("PS2_Main", "ps_6_2",  "assets\\shaders\\ACCDebug.hlsl", true);

		/*
		typedef struct D3D12_INPUT_ELEMENT_DESC
		{
		LPCSTR SemanticName;
		UINT SemanticIndex;
		DXGI_FORMAT Format;
		UINT InputSlot;
		UINT AlignedByteOffset;
		D3D12_INPUT_CLASSIFICATION InputSlotClass;
		UINT InstanceDataStepRate;
		} 	D3D12_INPUT_ELEMENT_DESC;
		*/

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        Rast_Desc.CullMode              = D3D12_CULL_MODE_NONE;
        //Rast_Desc.FillMode              = D3D12_FILL_MODE_WIREFRAME;
        //Rast_Desc.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature         = rootSig;
			PSO_Desc.VS                     = Debug_VShader;
			PSO_Desc.PS                     = Debug_PShader;
			PSO_Desc.RasterizerState        = Rast_Desc;
			PSO_Desc.BlendState             = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask             = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			PSO_Desc.NumRenderTargets       = 1;
			PSO_Desc.RTVFormats[0]          = DXGI_FORMAT_R16G16B16A16_FLOAT; // renderTarget
			PSO_Desc.SampleDesc.Count       = 1;
			PSO_Desc.SampleDesc.Quality     = 0;
			PSO_Desc.DSVFormat              = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.DepthStencilState      = Depth_Desc;
			PSO_Desc.InputLayout            = { InputElements, 1 };
			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = renderSystem->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
    }

    /************************************************************************************************/


    ModifiableShape         shape;
    Vector<GPUVertex>       vertices;
    Vector<uint32_t>        indices;
    Vector<GPUPatch>        patches;
    NodeHandle              node;

    
    ResourceHandle          patchBuffer;
    VertexBufferHandle      vertexBuffer;
    ConstantBufferHandle    constantBuffer;
    RootSignature           rootSig;
    Win32RenderWindow       renderWindow;
    ResourceHandle          depthBuffer;
    ResourceHandle          debug1Buffer;
    ResourceHandle          debug2Buffer;

    CameraComponent         cameras;
    SceneNodeComponent      nodes;

    double                  t = 0.0f;

    bool forward = false;
    bool backward = false;

    MouseInputState         mouseState;
    GameObject              gameObject;
    OrbitCameraBehavior     orbitCamera;
};


/************************************************************************************************/


int main()
{
    try
    {
        auto* allocator = FlexKit::CreateEngineMemory();
        EXITSCOPE(ReleaseEngineMemory(allocator));

        FlexKit::FKApplication app{ allocator, FlexKit::Max(std::thread::hardware_concurrency() / 2, 1u) - 1 };

        auto& state = app.PushState<TessellationTest>(0);

        app.GetCore().FPSLimit  = 90;
        app.GetCore().FrameLock = true;
        app.GetCore().vSync     = true;
        app.Run();
    }
    catch (...) { }

    return 0;
}


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
