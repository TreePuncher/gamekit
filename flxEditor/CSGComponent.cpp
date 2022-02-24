#include "PCH.h"
#include "CSGComponent.h"
#include "ViewportScene.h"
#include "EditorInspectorView.h"
#include "qlistwidget.h"
#include "EditorViewport.h"
#include <QtWidgets\qlistwidget.h>


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


/************************************************************************************************/


FlexKit::AABB CSGShape::GetAABB() const noexcept
{
    const FlexKit::AABB aabb{
        .Min = float3{ -1, -1, -1 },
        .Max = float3{  1,  1,  1 }
    };

    return aabb;
}

FlexKit::AABB CSGNode::GetAABB() const noexcept
{
    return shape.GetAABB();
}

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
        Translate,
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
                CSGNode newNode;
                newNode.op = op;

                selection.GetData().nodes.push_back(newNode);
            }

            ImGui::SameLine();

            switch (mode)
            {
            case CSGEditMode::Mode::Selection:
            {

            }   break;
            case CSGEditMode::Mode::Translate:
                break;
            default:
                break;
            }

        }
        ImGui::End();
    }


    void Draw(FlexKit::UpdateDispatcher& dispatcher, FlexKit::FrameGraph& frameGraph, TemporaryBuffers& temps, FlexKit::ResourceHandle renderTarget) final
    {

        struct DrawHUD
        {
            FlexKit::ReserveVertexBufferFunction    ReserveVertexBuffer;
            FlexKit::ReserveConstantBufferFunction  ReserveConstantBuffer;
            FlexKit::FrameResourceHandle            renderTarget;
        };


        if(selection.GetData().nodes.size())
        {
            frameGraph.AddNode<DrawHUD>(
                DrawHUD{
                    temps.ReserveVertexBuffer,
                    temps.ReserveConstantBuffer
                },
                [&](FlexKit::FrameGraphNodeBuilder& builder, DrawHUD& data)
                {
                    data.renderTarget = builder.RenderTarget(renderTarget);
                },
                [&](DrawHUD& data, FlexKit::ResourceHandler& resources, FlexKit::Context& ctx, auto& allocator)
                {
                    ctx.BeginEvent_DEBUG("Draw CSG HUD");

                    const auto& nodes = selection.GetData().nodes;

                    // Setup state
                    FlexKit::DescriptorHeap descHeap;
			        descHeap.Init(
				        ctx,
				        resources.renderSystem().Library.RS6CBVs4SRVs.GetDescHeap(0),
				        &allocator);
			        descHeap.NullFill(ctx);

			        ctx.SetRootSignature(resources.renderSystem().Library.RS6CBVs4SRVs);
			        ctx.SetPipelineState(resources.GetPipelineState(FlexKit::DRAW_LINE3D_PSO));

			        ctx.SetScissorAndViewports({ resources.GetRenderTarget(data.renderTarget) });
			        ctx.SetRenderTargets({ resources.GetRenderTarget(data.renderTarget) }, false);

			        ctx.SetPrimitiveTopology(FlexKit::EInputTopology::EIT_LINE);

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
                        FlexKit::AlignedSize<DrawConstants>() * nodes.size());

                    FlexKit::ConstantBufferDataSet cameraConstants{ FlexKit::GetCameraConstants(viewport.GetViewportCamera()), constantBuffer };
                    ctx.SetGraphicsConstantBufferView(1, cameraConstants);

                    const size_t selectedNodeIdx    = selection.GetData().selectedNode;
                    FlexKit::VBPushBuffer VBBuffer  = data.ReserveVertexBuffer(sizeof(Vertex) * 24 * nodes.size());

                    size_t idx = 0;
                    for (const auto& node : nodes)
                    {
                        FlexKit::float4 Color = (idx == selectedNodeIdx) ? FlexKit::float4{ 1, 1, 1, 0 } : FlexKit::float4{ 0.5f, 0.5f, 0.5f, 0.5f };

                        const auto aabb = node.GetAABB();
                        const float r = aabb.Dim()[aabb.LongestAxis()];
                        const auto offset = aabb.MidPoint();

                        auto cube = CreateWireframeCube(r / 2);

                        for (auto& vertex : cube)
                        {
                            vertex.Color = Color;
                        }

                        const FlexKit::VertexBufferDataSet vbDataSet{
                            cube.data(),
                            cube.ByteSize(),
                            VBBuffer };

                        ctx.SetVertexBuffers({ vbDataSet });

                        const DrawConstants CB_Data = {
                            .unused1    = FlexKit::float4{ 1, 1, 1, 1 },
                            .unused2    = FlexKit::float4{ 1, 1, 1, 1 },
                            .transform  = FlexKit::TranslationMatrix({ 0, 0, 0 })
                        };

                        const FlexKit::ConstantBufferDataSet constants{ CB_Data, constantBuffer };

                        ctx.SetGraphicsConstantBufferView(2, constants);

                        ctx.Draw(24);

                        ++idx;

                    }

                    ctx.EndEvent_DEBUG();
                }
            );
        }
    }


    void mousePressEvent(QMouseEvent* evt) final 
    {
        if(ImGui::IsAnyItemHovered())
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
            case CSGEditMode::Mode::Translate:
                break;
            default:
                break;
            }
        }
    }

    void mouseReleaseEvent(QMouseEvent* evt) final 
    {
        if (ImGui::IsAnyItemHovered())
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

                for (const auto& node : selection.GetData().nodes)
                {
                    const auto AABB = node.GetAABB();

                    if (auto res = FlexKit::Intersects(r, AABB); res && res.value() < minDistance && res.value() > 0.0f)
                    {
                    }
                }
            }   break;
            case CSGEditMode::Mode::Translate:
                break;
            default:
                break;
            }
        }
    }

    void keyPressEvent(QKeyEvent* event) final
    {
    }

    void keyReleaseEvent(QKeyEvent* event) final
    {
    }


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
                return csgView.GetData().nodes.size();
            }, 
            [&](auto idx, QListWidgetItem* item)
            {   // Update Contents
                auto op = csgView.GetData().nodes[idx].op;

                std::string label = op == CSG_OP::CSG_ADD ? "Add" :
                                    op == CSG_OP::CSG_SUB ? "Sub" : "";

                item->setData(Qt::DisplayRole, label.c_str());
            },             
            [&](QListWidget* listWidget)
            {   // On Event
                listWidget->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
                std::cout << "Changed\n";
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


std::vector<Triangle> CSGShape::GetTris() const noexcept
{
    return {};
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
