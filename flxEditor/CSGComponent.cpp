#include "PCH.h"
#include "CSGComponent.h"
#include "ViewportScene.h"
#include "EditorInspectorView.h"
#include "qlistwidget.h"
#include "EditorViewport.h"
#include <QtWidgets\qlistwidget.h>
#include <ImGuizmo.h>


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
    return (position[1] - position[0]).cross(position[2] - position[0]).normal();
}

const float3 Triangle::TriPoint() const noexcept
{
    return (position[0] + position[1] + position[2]) / 3;
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


CSGShape CreateCubeCSGShape() noexcept
{
    CSGShape shape;

    const float3 dimensions = float3{ 1.0f, 1.0f, 1.0f } / 2.0f;
    // Top
    shape.tris.push_back(
        {   float3{ dimensions.x, dimensions.y, dimensions.z },
            float3{-dimensions.x, dimensions.y, dimensions.z },
            float3{-dimensions.x, dimensions.y,-dimensions.z }
        });
    shape.tris.push_back(
        {   float3{ dimensions.x, dimensions.y, dimensions.z },
            float3{-dimensions.x, dimensions.y,-dimensions.z },
            float3{ dimensions.x, dimensions.y,-dimensions.z }
        });

    // Bottom
    shape.tris.push_back(
        {   float3{  dimensions.x,-dimensions.y, dimensions.z },
            float3{ -dimensions.x,-dimensions.y,-dimensions.z },
            float3{ -dimensions.x,-dimensions.y, dimensions.z }
        });
    shape.tris.push_back(
        {   float3{  dimensions.x,-dimensions.y, dimensions.z },
            float3{  dimensions.x,-dimensions.y,-dimensions.z },
            float3{ -dimensions.x,-dimensions.y,-dimensions.z }
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

bool Intersects(const Triangle& A, const Triangle& B, IntersectionPoints&& out = IntersectionPoints{})
{
    const auto A_n = A.Normal();
    const auto A_p = A.TriPoint();
    const auto B_n = B.Normal();
    const auto B_p = B.TriPoint();

    bool infront    = false;
    bool behind     = false;
    float3 distances_B;
    for (size_t idx = 0; idx < 3; ++idx)
    {
        const auto dp = distances_B[idx] = (B[idx] - A_p).dot(A_n);
        infront |= dp >= 0.0f;
        behind  |= dp <= 0.0f;
    }

    if(!(infront && behind))
        return false;

    infront = false;
    behind  = false;

    float3 distances_A;
    for (size_t idx = 0; idx < 3; ++idx)
    {
        const auto dp = distances_A[idx] = (A[idx] - B_p).dot(B_n);
        infront |= dp >= 0.0f;
        behind  |= dp <= 0.0f;
    }

    if (!(infront && behind))
        return false;

    auto M_a = distances_A.Max();
    auto M_b = distances_B.Max();

    if (M_a < 0.00001f || M_b < 0.00001f)
    {   // Coplanar

    }
    else
    {   // Non-Planar
        const auto L_n = A_n.cross(B_n);
        const FlexKit::Plane p{ B_n, B_p };

        const FlexKit::Ray r[] = {
            { (A[1] - A[0]).normal(), A[0] },
            { (A[2] - A[1]).normal(), A[1] },
            { (A[0] - A[2]).normal(), A[2] } };

        const float i[] =
        {
            FlexKit::Intesects(r[1], p),
            FlexKit::Intesects(r[2], p),
            FlexKit::Intesects(r[3], p)
        };

        size_t idx = 0;
        float d = INFINITY;

        for (size_t I = 0; I < 3; I++)
        {
            if (!std::isnan(i[I]) && !std::isinf(i[I]))
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

        const auto res = (a_min <= b_max && b_min <= a_max);
        return res;
    }

    return false;
}


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
        if (ImGui::Begin("Edit CSG Menu", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
        {
            ImGui::SetWindowPos({ 0, 0 });
            ImGui::SetWindowSize({ 400, 400 });

            CSG_OP op = CSG_OP::CSG_ADD;

            static const char* modeStr[] =
            {
                "Add",
                "Sub"
            };

            if (ImGui::BeginCombo("OP", modeStr[(int)op]))
            {
                if (ImGui::Selectable("Add"))
                    op = CSG_OP::CSG_ADD;
                if (ImGui::Selectable("Sub"))
                    op = CSG_OP::CSG_SUB;

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
        }
        ImGui::End();

        switch (mode)
        {
        case CSGEditMode::Mode::Selection:
        {

        }   break;
        case CSGEditMode::Mode::Manipulator:
        {
            const auto selectedIdx  = selection.GetData().selectedBrush;
            if (selectedIdx == -1)
                return;

            auto& brushes           = selection.GetData().brushes;
            auto& brush             = brushes[selectedIdx];


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
        }   break;
        default:
            break;
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
                size_t selectedIdx  = -1;
                float  minDistance  = 10000.0f;
                const auto r        = viewport.GetMouseRay();

                for (const auto& brush : selection.GetData().brushes)
                {
                    const auto AABB = brush.GetAABB();

                    if (auto res = FlexKit::Intersects(r, AABB); res && res.value() < minDistance && res.value() > 0.0f)
                    {
                    }
                }
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


    ImGuizmo::MODE      space       = ImGuizmo::MODE::WORLD;
    ImGuizmo::OPERATION operation   = ImGuizmo::OPERATION::TRANSLATE;

    EditorViewport&             viewport;
    CSGView&                    selection;
    FlexKit::ImGUIIntegrator&   hud;
    FlexKit::int2               previousMousePosition = FlexKit::int2{ -160000, -160000 };
};


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

    inline static bool _registered = Register();
};



/************************************************************************************************/


struct CSGComponentUpdate
{
    static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
    {
        auto& editorCSGComponent = static_cast<EditorComponentCSG&>(component);

    }

    IEntityComponentRuntimeUpdater::RegisterConstructorHelper<CSGComponentUpdate, CSGComponentID> _register;
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
