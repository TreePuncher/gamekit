#include "pch.h"

constexpr FlexKit::PSOHandle ACC = FlexKit::PSOHandle(GetTypeGUID(ACC));

using namespace FlexKit;

enum GregoryQuadPatchPoint
{
    p0 = 0,
    p1 = 1,
    p2 = 2,
    p3 = 3,

    e0Minus = 4,
    e0Plus  = 5,
    e1Minus = 6,
    e1Plus  = 7,
    e2Minus = 8,
    e2Plus  = 9,
    e3Minus = 10,
    e3Plus  = 11,

    f0Minus = 12,
    f0Plus  = 13,

    f1Minus = 14,
    f1Plus  = 15,

    f2Minus = 16,
    f2Plus  = 17,

    f3Minus = 18,
    f3Plus  = 19
};


struct ControlPoint
{
    float3 P;
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

    std::vector<uint32_t> edgeQuad;
    std::vector<uint32_t> edgeTri;
};

struct Patch
{
    struct ControlPointWeights
    {
        ControlPointWeights() { memset(indices, 0xff, sizeof(indices)); }

        float       weights[32] = { 0.0f };
        uint32_t    indices[32];
    } controlPoints[20];

    uint32_t inputVertices[32];
    uint32_t vertCount = 0;
};


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
    const float3 P = GetPoint_P2(patch, shape);
    const float3 Q = ApplyWeights(patch, shape, e2Minus);
    const float3 e2_Minus = P + 2.0f / 3.0f * Q;

    return e2_Minus;
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
                        groups.edgeQuad.push_back(I);
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
    size_t B = 0; // ControlPoint Counter
    auto vertexView = shape.wFaces[quadPatchIdx].VertexView(shape);

    for (auto [point, pointIdx, edges, edgeView] : vertexView)
    {
        size_t A = 0; // Weight counter
        Patch::ControlPointWeights controlPoint;
        if (!vertexView.IsExteriorCorner())
        {
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
        else
        {
            controlPoint.indices[0] = pointIdx;
            controlPoint.weights[0] = 1.0f;
        }

        float f = 0.0f;
        for (auto& w : controlPoint.weights)
            f += w;

        FK_ASSERT(fabs(f - 1.0f) < 0.001f, "Calculation error!");

        float3 p{ 0 };
        for (size_t I = 0; I < 32; I++)
            if (controlPoint.indices[I] != 0xffffffff)
                p += controlPoint.weights[I] * shape.wVertices[controlPoint.indices[I]].point;

        patch.controlPoints[B++] = controlPoint;
        auto P_n = ApplyWeights(patch, shape, B - 1);
        int x = 0;
    }
}


/************************************************************************************************/


Patch::ControlPointWeights CalculateQuadEdgeControlPoint(auto& vertexEdgeView, auto& faceView, auto& shape, const auto N)
{
    Patch::ControlPointWeights controlPoint;

    const auto S = Sigma(N);
    const auto L = Lambda(N);
    size_t A = 0;

    // Calculate Mid Edge Points
    // M_i(1 - THETA * cos(pi / n)) * cos((2PI*I) / N)
    size_t I_1 = 0;
    for (const auto& edge : vertexEdgeView)
    {
        const float w = (1.0f - S * std::cos(pi / N)) *
                        cos((2.0f * pi * I_1) / N) *
                        (1.0f / 2.0f) *
                        (2.0f / (N*N));

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
        w *= L;

    return controlPoint;
}


/************************************************************************************************/


void CalculateQuadEdgePoint_E0_Plus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape)
{
    auto vertexView = shape.wFaces[quadPatchIdx].VertexView(shape);
    auto edgeView   = shape.wFaces[quadPatchIdx].EdgeView(shape);
    auto faceView   = vertexView.FaceView();
    auto E0         = edgeView[0];

    const auto v_Idx    = E0->vertices[0];
    const auto N        = shape.wVerticeEdges[v_Idx].size() / 2.0f;

    auto vertexEdgeView = vertexView.EdgeView(1);
    auto controlPoint = CalculateQuadEdgeControlPoint(vertexEdgeView, faceView, shape, N);

    patch.controlPoints[e0Plus] = controlPoint;

    auto E0_P = GetPoint_E0_Plus(patch, shape);
}


/************************************************************************************************/


void CalculateQuadEdgePoint_E0_Minus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape)
{
    auto vertexView = shape.wFaces[quadPatchIdx].VertexView(shape);
    auto edgeView   = shape.wFaces[quadPatchIdx].EdgeView(shape);
    auto faceView   = vertexView.FaceView(1);
    auto E3         = edgeView[3];

    const auto v0_Idx     = E3->vertices[0];
    const auto N          = shape.wVerticeEdges[v0_Idx].size() / 2.0f;

    auto vertexEdgeView = vertexView.EdgeView(2);
    auto controlPoint = CalculateQuadEdgeControlPoint(vertexEdgeView, faceView, shape, N);

    patch.controlPoints[e0Minus] = controlPoint;

    auto E0_P = GetPoint_E0_Minus(patch, shape);
}


/************************************************************************************************/


void CalculateQuadEdgePoint_E1_Plus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape)
{
    auto vertexView = shape.wFaces[quadPatchIdx].VertexView(shape, 1);
    auto edgeView   = shape.wFaces[quadPatchIdx].EdgeView(shape);
    auto faceView   = vertexView.FaceView();
    auto E1         = edgeView[1];

    const auto v1_Idx     = E1->vertices[0];
    const auto N          = shape.wVerticeEdges[v1_Idx].size() / 2.0f;

    auto vertexEdgeView = vertexView.EdgeView(2, 1);
    auto controlPoint = CalculateQuadEdgeControlPoint(vertexEdgeView, faceView, shape, N);

    patch.controlPoints[e1Plus] = controlPoint;

    auto P1     = GetPoint_P1(patch, shape);
    auto E1_P   = GetPoint_E1_Plus(patch, shape);
}


/************************************************************************************************/


void CalculateQuadEdgePoint_E1_Minus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape)
{
    auto vertexView = shape.wFaces[quadPatchIdx].VertexView(shape, 1);
    auto edgeView   = shape.wFaces[quadPatchIdx].EdgeView(shape);
    auto faceView   = vertexView.FaceView(1);
    auto E1         = edgeView[1];

    const auto v1_Idx     = E1->vertices[0];
    const auto N          = shape.wVerticeEdges[v1_Idx].size() / 2.0f;

    auto vertexEdgeView = vertexView.EdgeView(3, 0);
    auto controlPoint = CalculateQuadEdgeControlPoint(vertexEdgeView, faceView, shape, N);

    patch.controlPoints[e1Minus] = controlPoint;

    auto E1_P   = GetPoint_E1_Minus(patch, shape);   
}


/************************************************************************************************/


void CalculateQuadEdgePoint_E2_Plus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape)
{
    auto vertexView = shape.wFaces[quadPatchIdx].VertexView(shape, 2);
    auto edgeView   = shape.wFaces[quadPatchIdx].EdgeView(shape);
    auto faceView   = vertexView.FaceView(0);
    auto E2         = edgeView[2];

    const auto v1_Idx     = E2->vertices[0];
    const auto v2_Idx     = E2->vertices[1];
    const auto N          = shape.wVerticeEdges[v1_Idx].size() / 2.0f;

    auto vertexEdgeView = vertexView.EdgeView(0, 0);
    auto controlPoint = CalculateQuadEdgeControlPoint(vertexEdgeView, faceView, shape, N);

    patch.controlPoints[e2Plus] = controlPoint;

    auto E2_P   = GetPoint_E2_Plus(patch, shape);   
}


/************************************************************************************************/


void CalculateQuadEdgePoint_E2_Minus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape)
{
    auto vertexView = shape.wFaces[quadPatchIdx].VertexView(shape, 2);
    auto edgeView = shape.wFaces[quadPatchIdx].EdgeView(shape);
    auto faceView = vertexView.FaceView(0);
    auto E2 = edgeView[2];

    const auto v1_Idx = E2->vertices[0];
    const auto v2_Idx = E2->vertices[1];
    const auto N = shape.wVerticeEdges[v1_Idx].size() / 2.0f;

    auto vertexEdgeView = vertexView.EdgeView(0, 0);
    auto controlPoint = CalculateQuadEdgeControlPoint(vertexEdgeView, faceView, shape, N);

    patch.controlPoints[e2Minus] = controlPoint;

    auto E2_P = GetPoint_E2_Minus(patch, shape);
}

void CalculateQuadEdgePoint_E3_Plus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape) {}
void CalculateQuadEdgePoint_E3_Minus(uint32_t quadPatchIdx, Patch& patch, ModifiableShape& shape) {}


/************************************************************************************************/


auto CalculateControlPointWeights(PatchGroups& classifiedPatches, ModifiableShape& shape)
{
    std::vector<Patch> patches;

    for (auto& quadPatch : classifiedPatches.QuadPatches)
    {
        Patch newPatch;

        CalculateQuadCornerPoints(quadPatch, newPatch, shape);
        CalculateQuadEdgePoint_E0_Plus(quadPatch, newPatch, shape);
        CalculateQuadEdgePoint_E0_Minus(quadPatch, newPatch, shape);
        CalculateQuadEdgePoint_E1_Plus(quadPatch, newPatch, shape);
        CalculateQuadEdgePoint_E1_Minus(quadPatch, newPatch, shape);
        CalculateQuadEdgePoint_E2_Plus(quadPatch, newPatch, shape);
        CalculateQuadEdgePoint_E2_Minus(quadPatch, newPatch, shape);
        CalculateQuadEdgePoint_E3_Plus(quadPatch, newPatch, shape);
        CalculateQuadEdgePoint_E3_Minus(quadPatch, newPatch, shape);

        patches.push_back(newPatch);
    }

    for (auto& patch : patches)
    {
        std::vector<uint32_t> verticies;

        for (const auto& controlPoint : patch.controlPoints)
            for (const auto idx : controlPoint.indices)
                verticies.push_back(idx);

        std::sort(verticies.begin(), verticies.end());
        verticies.erase(std::unique(verticies.begin(), verticies.end()), verticies.end());

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
                FlexKit::DefaultWindowDesc({ 1920, 1080 }))) },
        rootSig         { IN_framework.core.GetBlockMemory() },
        vertexBuffer    { IN_framework.GetRenderSystem().CreateVertexBuffer(MEGABYTE, false) },

        controlWeights  { IN_framework.core.GetBlockMemory() }
    {
        PreComputePatches();

        rootSig.SetParameterAsSRV(0, 0, 0);
        rootSig.AllowIA = true;
        rootSig.AllowSO = false;
        FK_ASSERT(rootSig.Build(IN_framework.GetRenderSystem(), IN_framework.core.GetTempMemory()));

        framework.GetRenderSystem().RegisterPSOLoader(ACC, { &rootSig, [&](auto* renderSystem) { return LoadPSO(renderSystem); } });


        FlexKit::EventNotifier<>::Subscriber sub;
        sub.Notify  = &FlexKit::EventsWrapper;
        sub._ptr    = &framework;

        renderWindow.Handler->Subscribe(sub);
        renderWindow.SetWindowTitle("[Tessellation Test]");
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


    // V7 ------- V6 ------- V11 ------ V15
    // |          |          |           |
    // |          |          |           |
    // |   P2     |    p5    |    p8     |
    // |          |          |           |
    // |          |          |           |
    // V5 ------- V4 ------- V10 ------ V14
    // |          |          |           |
    // |          |          |           |
    // |   p1     |    p4    |    p7     |
    // |          |          |           |
    // |          |          |           |
    // V3 ------- V2 ------- V9 ------- V13
    // |          |          |           |
    // |          |          |           |
    // |   p0     |    p3    |    p6     |
    // |          |          |           |
    // |          |          |           |
    // V0 ------- V1 ------- V8 ------- V12


    /************************************************************************************************/


    void PreComputePatches()
    {
        // Patch 1
        shape.AddVertex(float3{ 0.0f, 0.0f, 0.0f }); // 0
        shape.AddVertex(float3{ 1.0f, 0.0f, 0.0f }); // 1
        shape.AddVertex(float3{ 1.0f, 1.0f, 1.0f }); // 2
        shape.AddVertex(float3{ 0.0f, 1.0f, 0.0f }); // 3

        shape.AddVertex(float3{ 1.0f, 2.00f, 1.0f }); // 4
        shape.AddVertex(float3{ 0.0f, 2.00f, 0.0f }); // 5
        shape.AddVertex(float3{ 1.0f, 3.00f, 0.0f }); // 6
        shape.AddVertex(float3{ 0.0f, 3.00f, 0.0f }); // 7

        shape.AddVertex(float3{ 2.0f, 0.0f,  0.0f }); // 8
        shape.AddVertex(float3{ 2.0f, 1.0f,  1.0f }); // 9
        shape.AddVertex(float3{ 2.0f, 2.0f,  1.0f }); // 10
        shape.AddVertex(float3{ 2.0f, 3.0f,  0.0f }); // 11

        shape.AddVertex(float3{ 3.0f, 0.0f,  0.0f }); // 12
        shape.AddVertex(float3{ 3.0f, 1.0f,  0.0f }); // 13
        shape.AddVertex(float3{ 3.0f, 2.0f,  0.0f }); // 14
        shape.AddVertex(float3{ 3.0f, 3.0f,  0.0f }); // 15

        uint32_t patch1[] = { 0, 1, 2, 3 };
        uint32_t patch2[] = { 3, 2, 4, 5 };
        uint32_t patch3[] = { 5, 4, 6, 7 };
        uint32_t patch4[] = { 1, 8, 9, 2 };
        uint32_t patch5[] = { 2, 9, 10, 4 };
        uint32_t patch6[] = { 4, 10, 11, 6 };
        uint32_t patch7[] = { 8, 12, 13, 9 };
        uint32_t patch8[] = { 9, 13, 14, 10 };
        uint32_t patch9[] = { 10, 14, 15, 11 };

        shape.AddPolygon(patch1, patch1 + 4);
        shape.AddPolygon(patch2, patch2 + 4);
        shape.AddPolygon(patch3, patch3 + 4);
        shape.AddPolygon(patch4, patch4 + 4);
        shape.AddPolygon(patch5, patch5 + 4);
        shape.AddPolygon(patch6, patch6 + 4);
        shape.AddPolygon(patch7, patch7 + 4);
        shape.AddPolygon(patch8, patch8 + 4);
        shape.AddPolygon(patch9, patch9 + 4);

        auto classifiedPatches  = ClassifyPatches(shape);
        auto controlPoints      = CalculateControlPointWeights(classifiedPatches, shape);

        // TODO:
        // Face Points, Tangent vectors
        //  Remap ControlPointWeights::Indicies from global buffer idx to patch local coord, [0, vertexCount ] -> [ 0 -> 32]
    }


    /************************************************************************************************/


    void DrawPatch(FlexKit::FrameGraph& frameGraph)
    {
        struct DrawPatch
        {
            FrameResourceHandle renderTarget;
        };

        frameGraph.AddNode<DrawPatch>(
            DrawPatch{},
            [&](FrameGraphNodeBuilder& builder, DrawPatch& draw)
            {
                draw.renderTarget = builder.RenderTarget(renderWindow.GetBackBuffer());
            },
            [&](DrawPatch& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
            {
                /*
                VBPushBuffer        buffer  { vertexBuffer, 4096, *ctx.renderSystem };
                VertexBufferDataSet VB      { controlPoints, buffer};

                ctx.SetRootSignature(rootSig);
                ctx.SetPipelineState(resources.GetPipelineState(ACC));
                ctx.SetPrimitiveTopology(EInputTopology::EIT_PATCH_CP_20);
                ctx.SetVertexBuffers({ VB });
                ctx.SetScissorAndViewports({ resources.GetRenderTarget(data.renderTarget) });
                ctx.SetRenderTargets({ resources.GetRenderTarget(data.renderTarget) }, false);
                ctx.Draw(4);
                */
            });
    }


    /************************************************************************************************/


    FlexKit::UpdateTask* Draw(FlexKit::UpdateTask* updateTask, FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT, FlexKit::FrameGraph& frameGraph)
    {
        ClearBackBuffer(frameGraph, renderWindow.GetBackBuffer(), { 0.0f, 0.0f, 0.0f, 1.0f });

        DrawPatch(frameGraph);

        PresentBackBuffer(frameGraph, renderWindow.GetBackBuffer());

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
            if (evt.mType == Event::EventType::Input && evt.Action == Event::Release && evt.mData1.mKC[0] == KC_R)
                framework.GetRenderSystem().QueuePSOLoad(ACC);
        };

        return false;
    }


    /************************************************************************************************/


    ID3D12PipelineState* LoadPSO(FlexKit::RenderSystem* renderSystem)
    {
        auto ACC_VShader = renderSystem->LoadShader("VS_Main", "vs_6_0", "assets\\shaders\\ACC.hlsl");
        auto ACC_DShader = renderSystem->LoadShader("DS_Main", "ds_6_0", "assets\\shaders\\ACC.hlsl");
		auto ACC_HShader = renderSystem->LoadShader("HS_Main", "hs_6_0", "assets\\shaders\\ACC.hlsl");
        auto ACC_PShader = renderSystem->LoadShader("PS_Main", "ps_6_0", "assets\\shaders\\ACC.hlsl");


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
			{ "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        Rast_Desc.CullMode              = D3D12_CULL_MODE_NONE;
        Rast_Desc.FillMode              = D3D12_FILL_MODE_WIREFRAME;
        //Rast_Desc.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	= false;

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
			PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = renderSystem->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
    }


    /************************************************************************************************/


    FlexKit::ModifiableShape    shape;
    Vector<float>               controlWeights;

    FlexKit::VertexBufferHandle vertexBuffer;
    FlexKit::RootSignature      rootSig;
    FlexKit::Win32RenderWindow  renderWindow;
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
