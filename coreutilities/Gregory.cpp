#include "pch.h"
#include "Gregory.h"
#include "MathUtils.h"
#include <fp16.h>

namespace FlexKit
{   /************************************************************************************************/


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


    /************************************************************************************************/


    Patch::ControlPointWeights::ControlPointWeights()
    {
        memset(indices, 0xff, sizeof(indices));
    }


    /************************************************************************************************/


    void Patch::ControlPointWeights::Scale(float s)
    {
        for (uint32_t I = 0; I < 32; I++)
            if (indices[I] != 0xffffffff)
                weights[I] *= s;
            else
                return;
    }


    /************************************************************************************************/


    void Patch::ControlPointWeights::AddWeight(uint32_t idx, float w)
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


    /************************************************************************************************/


    void Patch::ControlPointWeights::ScaleWeight(uint32_t idx, float s)
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


    float Patch::ControlPointWeights::WeightSum() const
    {
        float w     = 0.0f;
        size_t I    = 0;

        for (; I < 32; I++)
        {
            if (indices[I] != 0xffffffff)
                w += weights[I];
            else
                break;
        }

        return w;
    }



    /************************************************************************************************/


    Patch::ControlPointWeights& Patch::ControlPointWeights::operator += (const ControlPointWeights& rhs)
    {
        for (size_t I = 0; I < 32; I++)
            if(rhs.indices[I] != 0xffffffff && rhs.weights[I] != 0.0f)
                AddWeight(rhs.indices[I], rhs.weights[I]);

        return *this;
    }


    /************************************************************************************************/


    uint Pack(uint4 points)
    {
        return  ((points[0] & 0xff) << 0) |
            ((points[1] & 0xff) << 8) |
            ((points[2] & 0xff) << 16) |
            ((points[3] & 0xff) << 24);
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

    
        auto vertexView = shape.wFaces[p.C_0].VertexView(shape);

        auto temp1 = vertexView[0];
        auto temp2 = vertexView[1];
        auto temp3 = vertexView[2];
        auto temp4 = vertexView[3];

        const auto v1 = Max(shape.GetVertexValence(vertexView[0]), 4);
        const auto v2 = Max(shape.GetVertexValence(vertexView[1]), 4);
        const auto v3 = Max(shape.GetVertexValence(vertexView[2]), 4);
        const auto v4 = Max(shape.GetVertexValence(vertexView[3]), 4);

        patch.Pn = Pack({ v1, v2, v3, v4 });

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


    /************************************************************************************************/


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


    /************************************************************************************************/

    
    float3 GetPoint_P0(Patch& patch, ModifiableShape& shape)
    {
        return ApplyWeights(patch, shape, p0);
    }


    /************************************************************************************************/


    float3 GetPoint_P1(Patch& patch, ModifiableShape& shape)
    {
        return ApplyWeights(patch, shape, p1);
    }


    /************************************************************************************************/



    float3 GetPoint_P2(Patch& patch, ModifiableShape& shape)
    {
        return ApplyWeights(patch, shape, p2);
    }


    /************************************************************************************************/


    float3 GetPoint_P3(Patch& patch, ModifiableShape& shape)
    {
        return ApplyWeights(patch, shape, p3);
    }


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
        const float3 P = GetPoint_P2(patch, shape);
        const float3 Q = ApplyWeights(patch, shape, e2Minus);
        const float3 e2_Minus = P + 2.0f / 3.0f * Q;

        return e2_Minus;
    }


    /************************************************************************************************/


    float3 GetPoint_E3_Plus(Patch& patch, ModifiableShape& shape)
    {
        const float3 P = GetPoint_P3(patch, shape);
        const float3 Q = ApplyWeights(patch, shape, e3Plus);
        const float3 e3_Plus = P + 2.0f / 3.0f * Q;

        return e3_Plus;
    }


    /************************************************************************************************/


    float3 GetPoint_E3_Minus(Patch& patch, ModifiableShape& shape)
    {
        const float3 P = GetPoint_P3(patch, shape);
        const float3 Q = ApplyWeights(patch, shape, e3Minus);
        const float3 e3_Minus = P + 2.0f / 3.0f * Q;

        return e3_Minus;
    }


    /************************************************************************************************/


    PatchGroups ClassifyPatches(ModifiableShape& shape)
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
                    const auto twin = shape.wEdges[edges[I]].twin;

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
        auto vertexEdgeView = vertexView.EdgeView();

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


    Patch::ControlPointWeights CalculateQuadControlPoint_Rn(auto& vertexEdgeView, auto& faceView, ModifiableShape& shape, bool flipDirection = false)
    {
        auto el = vertexEdgeView[1];
        auto e0 = vertexEdgeView[0];
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

        auto f = flipDirection ? -1 : 1;

        AddMidEdgeWeight(el.A_idx, f * 1.0f / 3.0f);
        AddMidEdgeWeight(el.B_idx, f * 1.0f / 3.0f);
        AddMidEdgeWeight(er.A_idx, f * -1.0f / 3.0f);
        AddMidEdgeWeight(er.B_idx, f * -1.0f / 3.0f);

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

        AddFaceWeights(f0.faceIdx, f * 2.0f / 3.0f);
        AddFaceWeights(fr.faceIdx, f * -2.0f / 3.0f);


        return controlPoint;
    }


    /************************************************************************************************/


    void CalculateQuadControlPoint_R0_Plus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape, int edge = 0)
    {
        auto vertexView     = shape.wFaces[quadPatchIdx].VertexView(shape, edge);
        auto faceView       = vertexView.FaceView(0);
        auto vertexEdgeView = vertexView.EdgeView(0);

        auto controlPoint           = CalculateQuadControlPoint_Rn(vertexEdgeView, faceView, shape);
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
        auto faceView       = vertexView.FaceView(1);
        auto vertexEdgeView = vertexView.EdgeView(1);

        auto controlPoint           = CalculateQuadControlPoint_Rn(vertexEdgeView, faceView, shape, true);
        patch.controlPoints[r0Minus] = controlPoint;

        const auto a = faceView[0].faceIdx;
        const auto b = faceView[-1].faceIdx;

        const auto points_B = shape.GetFaceVertices(b);

        patch.C_idx[1] = a;

        const auto P = ApplyWeights(patch, shape, r0Minus);
        int c = 0;
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
        auto faceView       = vertexView.FaceView(0);
        auto vertexEdgeView = vertexView.EdgeView(0);

        auto controlPoint           = CalculateQuadControlPoint_Rn(vertexEdgeView, faceView, shape);
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
        auto faceView       = vertexView.FaceView(1);
        auto vertexEdgeView = vertexView.EdgeView(1);

        auto controlPoint               = CalculateQuadControlPoint_Rn(vertexEdgeView, faceView, shape, true);
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
        auto faceView       = vertexView.FaceView(0);
        auto vertexEdgeView = vertexView.EdgeView(0);

        auto controlPoint           = CalculateQuadControlPoint_Rn(vertexEdgeView, faceView, shape);
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
        auto faceView       = vertexView.FaceView(1);
        auto vertexEdgeView = vertexView.EdgeView(1);

        auto controlPoint               = CalculateQuadControlPoint_Rn(vertexEdgeView, faceView, shape, true);
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
        auto faceView       = vertexView.FaceView(0);
        auto vertexEdgeView = vertexView.EdgeView(0);

        auto controlPoint           = CalculateQuadControlPoint_Rn(vertexEdgeView, faceView, shape);
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


    void CalculateQuadControlPoint_R3_Minus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape, int edgeOffset = 3)
    {
        auto vertexView     = shape.wFaces[quadPatchIdx].VertexView(shape, edgeOffset);
        auto faceView       = vertexView.FaceView(1);
        auto vertexEdgeView = vertexView.EdgeView(1);

        auto controlPoint               = CalculateQuadControlPoint_Rn(vertexEdgeView, faceView, shape, true);
        patch.controlPoints[r3Minus]    = controlPoint;

        const auto a          = faceView[0].faceIdx;
        const auto b          = faceView[-1].faceIdx;
        const auto points_A   = shape.GetFaceVertices(a);

        patch.C_idx[7] = b;
    }


    /************************************************************************************************/


    void CalculateQuadControlPoint_R3_Minus_2(uint32_t edgeIdx, Patch& patch, ModifiableShape& shape, int edge = 3)
    {
        const auto twin         = shape.GetEdgeTwin(edgeIdx);
        const auto ln           = shape.GetEdgeLeftNeighbor(twin);
        const auto rn           = shape.GetEdgeRightNeighbor(twin);
        const auto face1        = shape.GetEdgeOwningFace(twin);
        const auto face0        = shape.GetEdgeOwningFace(edgeIdx);
        const auto& verts1      = shape.wEdges[ln].vertices;
        const auto& verts2      = shape.wEdges[rn].vertices;
        const auto faceVerts0   = shape.GetFaceVertices(face0);
        const auto faceVerts1   = shape.GetFaceVertices(face1);

        Patch::ControlPointWeights controlPoint;
        controlPoint.AddWeight(verts1[0],-0.5f * 1.0f/3.0f);
        controlPoint.AddWeight(verts1[1],-0.5f * 1.0f/3.0f);

        controlPoint.AddWeight(verts2[0], 0.5f * 1.0f/3.0f);
        controlPoint.AddWeight(verts2[1], 0.5f * 1.0f/3.0f);

        for (auto v : faceVerts0)
            controlPoint.AddWeight(v,  0.25f * 2.0f / 3.0f);

        for (auto v : faceVerts1)
            controlPoint.AddWeight(v, -0.25f * 2.0f / 3.0f);

        auto w_s = controlPoint.WeightSum();
        auto t = ApplyWeights(controlPoint, shape);

        patch.controlPoints[r3Minus] = controlPoint;

        //*2.0f / 3.0f)
        //* -2.0f / 3.0f;
        int x = 0;
    }

    void CalculateQuadControlPoint_R(uint32_t edgeIdx, Patch& patch, ModifiableShape& shape, uint32_t controlPointIdx)
    {
        if (shape.GetEdgeTwin(edgeIdx) == 0xffffffff)
            edgeIdx = shape.wEdges[edgeIdx].prev;

        const auto twin         = shape.GetEdgeTwin(edgeIdx);
        const auto ln           = shape.GetEdgeLeftNeighbor(twin);
        const auto rn           = shape.GetEdgeRightNeighbor(twin);
        const auto face1        = shape.GetEdgeOwningFace(twin);
        const auto face0        = shape.GetEdgeOwningFace(edgeIdx);
        const auto& verts1      = shape.wEdges[ln].vertices;
        const auto& verts2      = shape.wEdges[rn].vertices;
        const auto faceVerts0   = shape.GetFaceVertices(face0);
        const auto faceVerts1   = shape.GetFaceVertices(face1);

        Patch::ControlPointWeights controlPoint;
        controlPoint.AddWeight(verts1[0],-0.5f * 1.0f/3.0f);
        controlPoint.AddWeight(verts1[1],-0.5f * 1.0f/3.0f);

        controlPoint.AddWeight(verts2[0], 0.5f * 1.0f/3.0f);
        controlPoint.AddWeight(verts2[1], 0.5f * 1.0f/3.0f);

        for (auto v : faceVerts0)
            controlPoint.AddWeight(v,  0.25f * 2.0f / 3.0f);

        for (auto v : faceVerts1)
            controlPoint.AddWeight(v, -0.25f * 2.0f / 3.0f);

        auto w_s = controlPoint.WeightSum();
        auto t = ApplyWeights(controlPoint, shape);

        patch.controlPoints[controlPointIdx] = controlPoint;
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


    /************************************************************************************************/


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


    /************************************************************************************************/


    void CalculateTJunctionEdge(Patch& patch, uint32_t edge, uint32_t target, uint32_t point, bool Minus, ModifiableShape& shape)
    {
        auto& hEdge = shape.wEdges[edge];

        const auto v0       = shape.wEdges[edge].vertices[Minus ? 1 : 0];
        const auto v1       = shape.wEdges[edge].vertices[Minus ? 0 : 1];
        const auto& edges   = shape.wVerticeEdges[v0];

        Patch::ControlPointWeights controlPoint;// = patch.controlPoints[point];

        if (hEdge.twin != 0xffffffff)
        {   // Interner Edge
            const auto f1 = shape.wEdges[edge].face;
            const auto f2 = shape.wEdges[hEdge.twin].face;

            const auto f1_verts = shape.GetFaceVertices(f1);
            const auto f2_verts = shape.GetFaceVertices(f2);

            {
                auto w = f1_verts.size() == 2 ? (2.0f / 12.0f) : (1.0f / 12.0f);
                for (auto v : f1_verts)
                    if(v != v0 && v!= v1)
                        controlPoint.AddWeight(v, w);
            }

            {
                auto w = f2_verts.size() == 2 ? (2.0f / 12.0f) : (1.0f / 12.0f);
                for (auto v : f2_verts)
                    if (v != v0 && v != v1)
                        controlPoint.AddWeight(v, w);
            }


            controlPoint.AddWeight(v0, (2.0f / 3.0f) * (2.0f / 3.0f));
            controlPoint.AddWeight(v1, (2.0f / 3.0f) * (1.0f / 3.0f));

            auto w_t = controlPoint.WeightSum();
            int x = 0;
        }
        else
        {   // exterior Edge
            static_vector<uint32_t, 2> boundaryEdges;
            for (size_t I = 0; I < 4; I++)
            {
                const auto e    = edges[I];
                const auto twin = shape.wEdges[edges[I]].twin;

                if (twin == 0xffffffff)
                    boundaryEdges.push_back(e);
            }


            uint32_t verts[] = {
                shape.wEdges[boundaryEdges[0]].vertices[0],
                shape.wEdges[boundaryEdges[0]].vertices[1],
                shape.wEdges[boundaryEdges[1]].vertices[0],
                shape.wEdges[boundaryEdges[1]].vertices[1] };

            auto end = std::remove_if(verts, verts + 4, [&](auto v) { return v == v0; });

            if (verts[0] != v1)
            {
                controlPoint.AddWeight(verts[1], 2.0f / 3.0f);
                controlPoint.AddWeight(verts[0], 1.0f / 3.0f);
            }
            else
            {
                controlPoint.AddWeight(verts[0], 2.0f / 3.0f);
                controlPoint.AddWeight(verts[1], 1.0f / 3.0f);
            }
        }

        patch.controlPoints[target] = controlPoint;
        auto f = ApplyWeights(controlPoint, shape);
        int x = 0;
    }


    /************************************************************************************************/


    void CalculateQuadFacePointF0Plus(uint32_t edge, Patch& patch, ModifiableShape& shape)
    {
        Patch::ControlPointWeights r;

        const auto adjEdge  = edge;
        const auto prevEdge = shape.wEdges[adjEdge].prev;
        const auto opposite = shape.wEdges[shape.wEdges[edge].next].next;

        const auto p0 = shape.wEdges[edge].vertices[0];
        const auto v0 = shape.wEdges[adjEdge].vertices[0];
        const auto v1 = shape.wEdges[adjEdge].vertices[1];

        const auto v2 = shape.wEdges[opposite].vertices[1];
        const auto v3 = shape.wEdges[opposite].vertices[0];

        const auto face     = shape.wEdges[edge].face;
        const auto verts    = shape.GetFaceVertices(face);

        r.AddWeight(v0, -1.0f);
        r.AddWeight(v2,  1.0f);

        r.AddWeight(v1, -1.0f);
        r.AddWeight(v3,  1.0f);

        r.Scale(0.5f);

        patch.controlPoints[r0Plus] = r;
    }


    
    /************************************************************************************************/


    void CalculateQuadFacePointF0Minus(uint32_t edge, Patch& patch, ModifiableShape& shape)
    {
        Patch::ControlPointWeights r;

        const auto adjEdge  = edge;
        const auto prevEdge = shape.wEdges[adjEdge].prev;
        const auto opposite = shape.wEdges[shape.wEdges[adjEdge].next].next;

        const auto v0 = shape.wEdges[adjEdge].vertices[0];
        const auto v1 = shape.wEdges[adjEdge].vertices[1];

        const auto v2 = shape.wEdges[opposite].vertices[1];
        const auto v3 = shape.wEdges[opposite].vertices[0];

        r.AddWeight(v0, -1.0f);
        r.AddWeight(v1,  1.0f);

        r.AddWeight(v2, -1.0f);
        r.AddWeight(v3,  1.0f);

        r.Scale(0.5f);

        patch.controlPoints[r0Minus] = r;
    }


    /************************************************************************************************/


    void CalculateQuadFacePointF1Minus(uint32_t edge, Patch& patch, ModifiableShape& shape)
    {
        Patch::ControlPointWeights r;

        const auto adjEdge  = edge;
        const auto prevEdge = shape.wEdges[adjEdge].prev;
        const auto opposite = shape.wEdges[shape.wEdges[adjEdge].next].next;

        const auto v0 = shape.wEdges[adjEdge].vertices[0];
        const auto v1 = shape.wEdges[adjEdge].vertices[1];

        const auto v2 = shape.wEdges[opposite].vertices[1];
        const auto v3 = shape.wEdges[opposite].vertices[0];

        r.AddWeight(v0, -1.0f);
        r.AddWeight(v1,  1.0f);

        r.AddWeight(v2, -1.0f);
        r.AddWeight(v3,  1.0f);

        r.Scale(0.5f);

        patch.controlPoints[r1Minus] = r;
    }


    /************************************************************************************************/


    void CalculateQuadFacePointF1Plus(uint32_t edge, Patch& patch, ModifiableShape& shape)
    {
        Patch::ControlPointWeights r;

        const auto adjEdge  = shape.wEdges[edge].next;
        const auto prevEdge = shape.wEdges[adjEdge].prev;
        const auto opposite = shape.wEdges[shape.wEdges[adjEdge].next].next;

        const auto v0 = shape.wEdges[adjEdge].vertices[0];
        const auto v1 = shape.wEdges[adjEdge].vertices[1];

        const auto v2 = shape.wEdges[opposite].vertices[1];
        const auto v3 = shape.wEdges[opposite].vertices[0];

        r.AddWeight(v0, -1.0f);
        r.AddWeight(v1,  1.0f);

        r.AddWeight(v2, -1.0f);
        r.AddWeight(v3,  1.0f);

        r.Scale(0.5f);

        patch.controlPoints[r1Plus] = r;
    }


    /************************************************************************************************/


    void CalculateQuadFacePointF2Minus(uint32_t edge, Patch& patch, ModifiableShape& shape)
    {
        Patch::ControlPointWeights r;

        const auto adjEdge  = edge;
        const auto prevEdge = shape.wEdges[adjEdge].prev;
        const auto opposite = shape.wEdges[shape.wEdges[adjEdge].next].next;

        const auto v0 = shape.wEdges[adjEdge].vertices[0];
        const auto v1 = shape.wEdges[adjEdge].vertices[1];

        const auto v2 = shape.wEdges[opposite].vertices[1];
        const auto v3 = shape.wEdges[opposite].vertices[0];

        r.AddWeight(v0, -1.0f);
        r.AddWeight(v1,  1.0f);

        r.AddWeight(v2, -1.0f);
        r.AddWeight(v3,  1.0f);

        r.Scale(0.5f);

        patch.controlPoints[r2Minus] = r;
    }


    /************************************************************************************************/


    void CalculateQuadFacePointF2Plus(uint32_t edge, Patch& patch, ModifiableShape& shape)
    {
        Patch::ControlPointWeights r;

        const auto adjEdge  = shape.wEdges[edge].next;
        const auto prevEdge = shape.wEdges[adjEdge].prev;
        const auto opposite = shape.wEdges[shape.wEdges[adjEdge].next].next;

        const auto v0 = shape.wEdges[adjEdge].vertices[0];
        const auto v1 = shape.wEdges[adjEdge].vertices[1];

        const auto v2 = shape.wEdges[opposite].vertices[1];
        const auto v3 = shape.wEdges[opposite].vertices[0];

        r.AddWeight(v0, -1.0f);
        r.AddWeight(v1,  1.0f);

        r.AddWeight(v2, -1.0f);
        r.AddWeight(v3,  1.0f);

        r.Scale(0.5f);

        patch.controlPoints[r2Plus] = r;
    }


    /************************************************************************************************/


    void CalculateQuadFacePointFn(uint32_t edge, Patch& patch, ModifiableShape& shape, uint32_t controlPoint, float f = 1.0f)
    {
        Patch::ControlPointWeights r;

        const auto adjEdge  = edge;
        const auto prevEdge = shape.wEdges[adjEdge].prev;
        const auto opposite = shape.wEdges[shape.wEdges[adjEdge].next].next;

        const auto v0 = shape.wEdges[adjEdge].vertices[0];
        const auto v1 = shape.wEdges[adjEdge].vertices[1];

        const auto v2 = shape.wEdges[opposite].vertices[1];
        const auto v3 = shape.wEdges[opposite].vertices[0];

        r.AddWeight(v0, -1.0f);
        r.AddWeight(v1,  1.0f);

        r.AddWeight(v2, -1.0f);
        r.AddWeight(v3,  1.0f);

        r.Scale(0.5f * f);

        patch.controlPoints[controlPoint] = r;
    }

    
    /************************************************************************************************/


    void CalculateQuadFacePointF3Plus(uint32_t edge, Patch& patch, ModifiableShape& shape)
    {
        Patch::ControlPointWeights r;

        const auto adjEdge  = shape.wEdges[edge].next;
        const auto prevEdge = shape.wEdges[adjEdge].prev;
        const auto opposite = shape.wEdges[shape.wEdges[adjEdge].next].next;

        const auto v0 = shape.wEdges[adjEdge].vertices[0];
        const auto v1 = shape.wEdges[adjEdge].vertices[1];

        const auto v2 = shape.wEdges[opposite].vertices[1];
        const auto v3 = shape.wEdges[opposite].vertices[0];

        r.AddWeight(v0, -1.0f);
        r.AddWeight(v1,  1.0f);

        r.AddWeight(v2, -1.0f);
        r.AddWeight(v3,  1.0f);

        r.Scale(0.5f);


        patch.controlPoints[r3Plus] = r;

        auto t = ApplyWeights(r, shape);
        int c = 0;
    }


    /************************************************************************************************/


    Patch BuildQuadBoundryPatch(uint32_t edgePatch, ModifiableShape& shape)
    {
        constexpr bool Plus  = false;
        constexpr bool Minus = true;

        Patch patch;

        CalculateQuadCornerPoints(edgePatch, patch, shape);
        patch.C_0 = edgePatch; 
        auto edgeView = shape.wFaces[edgePatch].EdgeView(shape);


        {   // Edge 0
            auto edge = edgeView[0];

            // V0
            switch (shape.wVerticeEdges[edge->vertices[0]].size())
            {
            case 2: // Corner
            {
                CalculateCornerEdge(patch, edge, e0Plus, Plus, shape);
                CalculateQuadFacePointF0Plus(edge, patch, shape);
                CalculateQuadFacePointF0Minus(edge, patch, shape);
            }   break;
            case 4: // T-Junction
            {
                CalculateTJunctionEdge(patch, edge, e0Plus, p0, Plus, shape);

                if (shape.GetEdgeTwin(edge.current) == 0xffffffff)
                {
                    CalculateQuadFacePointF0Plus(edge, patch, shape);
                    CalculateQuadControlPoint_R(edge, patch, shape, r0Minus);
                }
                else
                {
                    CalculateQuadControlPoint_R0_Plus(edgePatch, patch, shape);
                    CalculateQuadFacePointF0Minus(edge, patch, shape);
                }
            }   break;
            default:
            {
                CalculateQuadEdgePoint_E0_Plus(edgePatch, patch, shape);
                CalculateQuadEdgePoint_E0_Minus(edgePatch, patch, shape);
                CalculateQuadControlPoint_R0_Minus(edgePatch, patch, shape);
                CalculateQuadControlPoint_R0_Plus(edgePatch, patch, shape);
            }   break;
            }

            // V1
            switch (shape.wVerticeEdges[edge->vertices[1]].size())
            {
            case 2:
                CalculateCornerEdge(patch, edge, e1Minus, Minus, shape);
                break;
            case 4: // T-Junction case
                CalculateTJunctionEdge(patch, edge, e1Minus, p1, Minus, shape);
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
                CalculateQuadFacePointF1Plus(edge, patch, shape);
                CalculateQuadFacePointF1Minus(edge, patch, shape);
            }   break;
            case 4: // T-Junction
            {
                CalculateTJunctionEdge(patch, edge, e1Plus, p1, Plus, shape);

                if (shape.GetEdgeTwin(edge.current) == 0xffffffff)
                {
                    CalculateQuadFacePointF1Plus(edge, patch, shape);
                    CalculateQuadControlPoint_R(edge, patch, shape, r1Minus);
                }
                else
                {
                    CalculateQuadControlPoint_R1_Plus(edgePatch, patch, shape);
                    CalculateQuadFacePointF1Minus(edge, patch, shape);
                }
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
                CalculateTJunctionEdge(patch, edge, e2Minus, p2, Minus, shape);
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
                CalculateQuadFacePointF2Plus(edge, patch, shape);
                CalculateQuadFacePointF2Minus(edge, patch, shape);
            }   break;
            case 4: // T-Junction
            {
                CalculateTJunctionEdge(patch, edge, e2Plus, p2, Plus, shape);

                if (shape.GetEdgeTwin(edge.current) == 0xffffffff)
                {
                    CalculateQuadFacePointFn(edgeView[1], patch, shape, r2Plus, -1.0f);
                    CalculateQuadControlPoint_R(edge, patch, shape, r2Minus);
                }
                else
                {
                    CalculateQuadControlPoint_R2_Plus(edgePatch, patch, shape);
                    CalculateQuadFacePointF2Minus(edge, patch, shape);
                }
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
                CalculateTJunctionEdge(patch, edge, e3Minus, p3, Minus, shape);
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
                CalculateQuadFacePointFn(edgeView[2], patch, shape, r3Plus, -1);
                CalculateQuadFacePointFn(edge, patch, shape, r3Minus);
            }   break;
            case 4: // T-Junction
            {
                CalculateTJunctionEdge(patch, edge, e3Plus, p3, Plus, shape);

                if (shape.GetEdgeTwin(edge.current) == 0xffffffff)
                {
                    CalculateQuadFacePointFn(edgeView[2], patch, shape, r3Plus, -1.0f);
                    CalculateQuadControlPoint_R(edge, patch, shape, r3Minus);
                }
                else
                {
                    CalculateQuadControlPoint_R3_Plus(edgePatch, patch, shape);
                    CalculateQuadFacePointFn(edge, patch, shape, r3Minus);
                }
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
                CalculateTJunctionEdge(patch, edge, e0Minus, p0, Minus, shape);
                break;
            default:
                break;
            }
        }

        return patch;
    }

    
    /************************************************************************************************/


    std::vector<Patch> CreateGregoryPatches(PatchGroups& classifiedPatches, ModifiableShape& shape)
    {
        std::vector<Patch> patches;

        for (auto& quadPatch : classifiedPatches.QuadPatches)
        {
            const Patch regularPatch = BuidRegularPatch(quadPatch, shape);
            patches.push_back(regularPatch);
        }

        for (auto& borderPatch : classifiedPatches.edgeQuadPatches)
            patches.push_back(BuildQuadBoundryPatch(borderPatch, shape));

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


}   /************************************************************************************************/
