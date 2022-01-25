#include "EditorViewport.h"
#include "DXRenderWindow.h"
#include "EditorRenderer.h"
#include "qevent.h"
#include "DebugUI.cpp"

#include <QtWidgets/qmenubar.h>
#include <QShortcut>


using FlexKit::float2;
using FlexKit::float3;
using FlexKit::float4;
using FlexKit::float4x4;


/************************************************************************************************/


EditorViewport::EditorViewport(EditorRenderer& IN_renderer, SelectionContext& IN_context, QWidget *parent)
    :   QWidget{ parent }
    ,   hud                 { IN_renderer.GetRenderSystem(), FlexKit::SystemAllocator }
    ,   menuBar             { new QMenuBar{ this } }
    ,   renderer            { IN_renderer }
    ,   gbuffer             { { 100, 200 }, IN_renderer.framework.GetRenderSystem() }
    ,   depthBuffer         { IN_renderer.framework.GetRenderSystem(), { 100, 200 } }
    ,   viewportCamera      { FlexKit::CameraComponent::GetComponent().CreateCamera() }
    ,   selectionContext    { IN_context }
{
	ui.setupUi(this);

    auto layout = findChild<QBoxLayout*>("verticalLayout");
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setMenuBar(menuBar);

    setMinimumSize(100, 100);
    setMaximumSize({ 1024 * 16, 1024 * 16 });

    auto file           = menuBar->addMenu("Add");
    auto addGameObject  = file->addAction("Empty GameObject");

    addGameObject->connect(addGameObject, &QAction::triggered,
        [&]
        {
            auto& scene = GetScene();

            if (scene == nullptr)
                return;

            ViewportGameObject_ptr object   = std::make_shared<ViewportGameObject>();
            object->objectID                = rand();

            scene->sceneObjects.push_back(object);
        });

    menuBar->show();

    renderWindow = renderer.CreateRenderWindow();
    renderWindow->SetOnDraw(
        [&](FlexKit::UpdateDispatcher& dispatcher, double dT, TemporaryBuffers& temporaries, FlexKit::FrameGraph& frameGraph, FlexKit::ResourceHandle renderTarget, FlexKit::ThreadSafeAllocator& allocator)
        {
            Render(dispatcher, dT, temporaries, frameGraph, renderTarget, allocator);
        });

    layout->addWidget(renderWindow);
    adjustSize();

    FlexKit::SetCameraNode(viewportCamera, FlexKit::GetZeroedNode());

    show();

    auto& RS = IN_renderer.GetRenderSystem();
    RS.RegisterPSOLoader(FlexKit::DRAW_LINE3D_PSO, { &RS.Library.RS6CBVs4SRVs, FlexKit::CreateDraw2StatePSO });
}


/************************************************************************************************/


EditorViewport ::~EditorViewport()
{
}


/************************************************************************************************/


void EditorViewport::resizeEvent(QResizeEvent* evt)
{
    QWidget::resizeEvent(evt);

    evt->size().width();
    evt->size().height();

    renderWindow->resize({ evt->size().width(), evt->size().height() });
    depthBuffer.Resize({ (uint32_t)evt->size().width(), (uint32_t)evt->size().height() });
    gbuffer.Resize({ (uint32_t)evt->size().width(), (uint32_t)evt->size().height() });

    FlexKit::SetCameraAspectRatio(viewportCamera, float(evt->size().width())/ float(evt->size().height()));
    MarkCameraDirty(viewportCamera);
}


/************************************************************************************************/


void EditorViewport::SetScene(EditorScene_ptr scene)
{
    auto viewportScene = std::make_shared<ViewportScene>(scene);
    auto& renderSystem = renderer.framework.GetRenderSystem();

    std::vector<FlexKit::NodeHandle> nodes;

    for (auto node : scene->sceneResource->nodes)
    {
        auto newNode = FlexKit::GetZeroedNode();
        nodes.push_back(newNode);

        const auto position = node.position;
        const auto q        = node.Q;

        FlexKit::SetScale(newNode, float3{ node.scale.x, node.scale.x, node.scale.x });
        FlexKit::SetOrientationL(newNode, q);
        FlexKit::SetPositionL(newNode, position);

        if(node.parent != -1)
            FlexKit::SetParentNode(nodes[node.parent], newNode);

        SetFlag(newNode, FlexKit::SceneNodes::StateFlags::SCALE);
    }

    for (auto& entity : scene->sceneResource->entities)
    {
        auto viewObject = std::make_shared<ViewportGameObject>();
        viewObject->objectID = entity.objectID;
        viewportScene->sceneObjects.emplace_back(viewObject);

        if (entity.Node != -1)
            viewObject->gameObject.AddView<FlexKit::SceneNodeView<>>(nodes[entity.Node]);

        if (entity.id.size())
            viewObject->gameObject.AddView<FlexKit::StringIDView>(entity.id.c_str(), entity.id.size());

        for (auto& componentEntry : entity.components)
        {
            bool addedToScene = false;

            switch (componentEntry->id)
            {
            case FlexKit::TransformComponentID:
            {
                //if (entity.Node != -1)
                //    viewObject->gameObject.AddView<FlexKit::SceneNodeView<>>(nodes[entity.Node]);
            }   break;
            case FlexKit::BrushComponentID:
            {
                auto brushComponent = std::static_pointer_cast<FlexKit::EntityBrushComponent>(componentEntry);

                if (brushComponent)
                {
                    auto res = scene->FindSceneResource(brushComponent->MeshGuid);

                    if (!res) // TODO: Mesh not found, use placeholder model?
                        continue;


                    if (auto prop = res->properties.find(GetCRCGUID(TriMeshHandle)); prop != res->properties.end())
                    {
                        FlexKit::TriMeshHandle handle = std::any_cast<FlexKit::TriMeshHandle>(prop->second);

                        viewObject->gameObject.AddView<FlexKit::BrushView>(handle, nodes[entity.Node]);
                    }
                    else
                    {
                        auto meshBlob                   = res->resource->CreateBlob();
                        FlexKit::TriMeshHandle handle   = FlexKit::LoadTriMeshIntoTable(renderSystem, renderSystem.GetImmediateUploadQueue(), meshBlob.buffer, meshBlob.bufferSize);

                        res->properties[GetCRCGUID(TriMeshHandle)] = std::any{ handle };

                        viewObject->gameObject.AddView<FlexKit::BrushView>(handle, nodes[entity.Node]);
                    }

                    if(!addedToScene)
                        viewportScene->scene.AddGameObject(viewObject->gameObject, nodes[entity.Node]);

                    addedToScene = true;

                    FlexKit::SetBoundingSphereFromMesh(viewObject->gameObject);
                }
            }   break;
            case FlexKit::PointLightComponentID:
            {
                if (!addedToScene)
                    viewportScene->scene.AddGameObject(viewObject->gameObject, nodes[entity.Node]);

                auto  blob      = componentEntry->GetBlob();
                auto& component = FlexKit::ComponentBase::GetComponent(componentEntry->id);

                component.AddComponentView(viewObject->gameObject, blob, blob.size(), FlexKit::SystemAllocator);

                addedToScene = true;
            }   break;
            default:
            {
                auto  blob      = componentEntry->GetBlob();
                auto& component = FlexKit::ComponentBase::GetComponent(componentEntry->id);

                component.AddComponentView(viewObject->gameObject, blob, blob.size(), FlexKit::SystemAllocator);
            }   break;
            }
        }
    }


    this->scene = viewportScene;
}


/************************************************************************************************/


void EditorViewport::keyPressEvent(QKeyEvent* event)
{
    auto keyCode = event->key();

    if (keyCode == Qt::Key_Alt)
        state = InputState::PanOrbit;

    if(keyCode == Qt::Key_F)
    {
        auto& scene = GetScene();
        if (scene == nullptr)
            return;

        if (selectionContext.GetSelectionType() == ViewportObjectList_ID)
        {
            auto selection = selectionContext.GetSelection<ViewportObjectList>();

            FlexKit::AABB aabb;
            for (auto& object : selection)
            {
                auto boundingSphere = FlexKit::GetBoundingSphere(object->gameObject);
                aabb = aabb + boundingSphere;
            }

            const FlexKit::Camera c = FlexKit::CameraComponent::GetComponent().GetCamera(viewportCamera);

            const auto target           = aabb.MidPoint();
            const auto desiredDistance  = (2.0f/ std::sqrt(2.0f)) *  aabb.Span().magnitude() / std::tan(c.FOV);

            auto position_VS        = c.View.Transpose()    * float4{ target, 1 };
            auto updatedPosition_WS = c.IV.Transpose()      * float4{ position_VS.x, position_VS.y, position_VS.z + desiredDistance, 1 };

            const auto node         = FlexKit::GetCameraNode(viewportCamera);
            Quaternion Q = GetOrientation(node);
            auto forward = -(Q * float3(0, 0, 1)).normal();

            FlexKit::SetPositionW(node, updatedPosition_WS.xyz());
            FlexKit::MarkCameraDirty(viewportCamera);
        }
    }
}


/************************************************************************************************/


void EditorViewport::keyReleaseEvent(QKeyEvent* event)
{
    auto keyCode = event->key();

    if (keyCode == Qt::Key_Alt)
    {
        state = InputState::ClickSelect;
        previousMousePosition = FlexKit::int2{ -160000, -160000 };
    }
}


/************************************************************************************************/


void EditorViewport::mousePressEvent(QMouseEvent* event)
{

    if (event->button() == Qt::MouseButton::LeftButton)
    {
        previousMousePosition = FlexKit::int2{ -160000, -160000 };

        FlexKit::Event mouseEvent;
        mouseEvent.InputSource  = FlexKit::Event::Mouse;
        mouseEvent.Action       = FlexKit::Event::Pressed;
        mouseEvent.mType        = FlexKit::Event::Input;

        mouseEvent.mData1.mKC[0] = FlexKit::KC_MOUSELEFT;
        hud.HandleInput(mouseEvent);
    }
    else if (event->button() == Qt::MouseButton::RightButton)
    {
        previousMousePosition = FlexKit::int2{ -160000, -160000 };

        FlexKit::Event mouseEvent;
        mouseEvent.InputSource  = FlexKit::Event::Mouse;
        mouseEvent.Action       = FlexKit::Event::Pressed;
        mouseEvent.mType        = FlexKit::Event::Input;

        mouseEvent.mData1.mKC[0] = FlexKit::KC_MOUSERIGHT;
        hud.HandleInput(mouseEvent);
    }

    if (!scene)
        return;


    switch (state)
    {
    case InputState::ClickSelect:
    {
        if (event->button() == Qt::MouseButton::LeftButton)
        {
            auto qPos = renderWindow->mapFromGlobal(event->globalPos());

            const FlexKit::uint2 XY{ (uint32_t)qPos.x(), (uint32_t)qPos.y() };
            const FlexKit::uint2 screenWH = depthBuffer.WH;

            const FlexKit::float2 UV{ XY[0] / float(screenWH[0]), XY[1] / float(screenWH[1]) };
            const FlexKit::float2 ScreenCoord{ FlexKit::float2{ 2, -2 } * UV + FlexKit::float2{ -1.0f, 1.0f } };

            const auto cameraConstants      = FlexKit::GetCameraConstants(viewportCamera);
            const auto cameraOrientation    = FlexKit::GetOrientation(FlexKit::GetCameraNode(viewportCamera));

            const FlexKit::float3 v_dir = cameraOrientation * (Inverse(cameraConstants.Proj) * FlexKit::float4{ ScreenCoord.x, ScreenCoord.y,  1.0f, 1.0f }).xyz().normal();
            const FlexKit::float3 v_o   = cameraConstants.WPOS.xyz();

            auto results = scene->RayCast(
                FlexKit::Ray{
                            .D = v_dir.normal(),
                            .O = v_o,});

            if (results.empty())
            {
                if (event->button() == Qt::MouseButton::LeftButton)
                {
                    previousMousePosition = FlexKit::int2{ -160000, -160000 };

                    FlexKit::Event mouseEvent;
                    mouseEvent.InputSource  = FlexKit::Event::Mouse;
                    mouseEvent.Action       = FlexKit::Event::Pressed;
                    mouseEvent.mType        = FlexKit::Event::Input;

                    mouseEvent.mData1.mKC[0] = FlexKit::KC_MOUSELEFT;
                    hud.HandleInput(mouseEvent);
                }
                else if (event->button() == Qt::MouseButton::RightButton)
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

            ViewportObjectList selection;
            selection.push_back(results.front());

            selectionContext.selection  = std::move(selection);
            selectionContext.type       = ViewportObjectList_ID;
        }
    }   break;
    default:
    {
    }   break;
    }
}


/************************************************************************************************/


void EditorViewport::mouseMoveEvent(QMouseEvent* event)
{
    switch (state)
    {
    case InputState::PanOrbit:
    {
        if (event->buttons().testFlag(Qt::MiddleButton))
        {
            if (previousMousePosition == FlexKit::int2{ -160000, -160000 })
                previousMousePosition = { event->pos().x(), event->pos().y() };
            else
            {
                FlexKit::int2 newPosition{ event->pos().x(), event->pos().y() };
                FlexKit::int2 deltaPosition = previousMousePosition - newPosition;

                const auto node   = FlexKit::GetCameraNode(viewportCamera);
                const auto q      = FlexKit::GetOrientation(node);

                FlexKit::TranslateWorld(node, q * float3(deltaPosition[0] * 1.0f / 60.0f * panSpeed, -deltaPosition[1] * 1.0f / 60.0f * panSpeed, 0));

                MarkCameraDirty(viewportCamera);
                previousMousePosition = newPosition;
            }
        }
        else if(event->buttons().testFlag(Qt::LeftButton))
        {
            if (previousMousePosition == FlexKit::int2{ -160000, -160000 })
                previousMousePosition = { event->pos().x(), event->pos().y() };
            else
            {
                const FlexKit::int2 newPosition{ event->pos().x(), event->pos().y() };
                const FlexKit::int2 deltaPosition = previousMousePosition - newPosition;

                const auto  node   = FlexKit::GetCameraNode(viewportCamera);
                const auto  q      = FlexKit::GetOrientation(node);
                const float x      = float(deltaPosition[0]);
                const float y      = float(deltaPosition[1]);

                FlexKit::Yaw(node, x / 1000.0f);
                FlexKit::Pitch(node, y / 1000.0f);

                MarkCameraDirty(viewportCamera);

                previousMousePosition = newPosition;
            }
        }
        else
            previousMousePosition = FlexKit::int2{ -160000, -160000 };
    }   break;
    default:
    {
    }   break;
    }
}


/************************************************************************************************/


void EditorViewport::wheelEvent(QWheelEvent* event)
{
    const auto node   = FlexKit::GetCameraNode(viewportCamera);
    const auto q      = FlexKit::GetOrientation(node);

    FlexKit::TranslateWorld(node, q * float3{ 0, 0, event->angleDelta().x() / -10.0f });
    MarkCameraDirty(viewportCamera);
}


/************************************************************************************************/


void EditorViewport::mouseReleaseEvent(QMouseEvent* event)
{
    previousMousePosition = FlexKit::int2{ -160000, -160000 };

    if (event->button() == Qt::MouseButton::LeftButton)
    {
        previousMousePosition = FlexKit::int2{ -160000, -160000 };

        FlexKit::Event mouseEvent;
        mouseEvent.InputSource  = FlexKit::Event::Mouse;
        mouseEvent.Action       = FlexKit::Event::Release;
        mouseEvent.mType        = FlexKit::Event::Input;

        mouseEvent.mData1.mKC[0] = FlexKit::KC_MOUSELEFT;
        hud.HandleInput(mouseEvent);
    }
    else if (event->button() == Qt::MouseButton::RightButton)
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


/************************************************************************************************/


void EditorViewport::Render(FlexKit::UpdateDispatcher& dispatcher, double dT, TemporaryBuffers& temporaries, FlexKit::FrameGraph& frameGraph, FlexKit::ResourceHandle renderTarget, FlexKit::ThreadSafeAllocator& allocator)
{
    const auto HW           = frameGraph.GetRenderSystem().GetTextureWH(renderTarget);
    QPoint globalCursorPos  = QCursor::pos();
    auto localPosition      = renderWindow->mapFromGlobal(globalCursorPos);


    hud.Update({ (float)localPosition.x() * 1.5f, (float)localPosition.y() * 1.5f }, HW, dispatcher, dT);

    ImGui::NewFrame();
    if (ImGui::Begin("Test"))
    {
        ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
    }
    ImGui::End();

    ImGui::EndFrame();
    ImGui::Render();

    if (scene)
    {
        allocator.clear();

        auto& transforms        = QueueTransformUpdateTask(dispatcher);
        auto& cameras           = FlexKit::CameraComponent::GetComponent().QueueCameraUpdate(dispatcher);
        auto& cameraConstants   = FlexKit::MakeHeapCopy(FlexKit::Camera::ConstantBuffer{}, allocator);

        depthBuffer.Increment();

        FlexKit::ClearDepthBuffer(frameGraph, depthBuffer.Get(), 1.0f);
        FlexKit::ClearGBuffer(gbuffer, frameGraph);
        FlexKit::ClearBackBuffer(frameGraph, renderTarget);

        FlexKit::WorldRender_Targets targets {
            .RenderTarget   = renderTarget,
            .DepthTarget    = depthBuffer,
        };

        FlexKit::DrawSceneDescription sceneDesc =
        {
            .camera = viewportCamera,
            .scene  = scene->scene,
            .dt     = dT,
            .t      = T,

            .gbuffer    = gbuffer,
            .reserveVB  = temporaries.ReserveVertexBuffer,
            .reserveCB  = temporaries.ReserveConstantBuffer,

            .debugDisplay   = FlexKit::DebugVisMode::Disabled,
            .BVHVisMode     = FlexKit::BVHVisMode::BVH,
            .debugDrawMode  = FlexKit::ClusterDebugDrawMode::BVH,

            .transformDependency    = transforms,
            .cameraDependency       = cameras,

            .additionalGbufferPasses    = {},
            .additionalShadowPasses     = {}
        };

        auto drawSceneRes = renderer.worldRender.DrawScene(dispatcher, frameGraph, sceneDesc, targets, FlexKit::SystemAllocator, allocator);

        EditorViewport::DrawSceneOverlay_Desc desc{
            .brushes        = drawSceneRes.passes.GetData().solid,
            .lights         = drawSceneRes.pointLights,

            .buffers        = temporaries,
            .renderTarget   = renderTarget,
            .allocator      = allocator
        };

        DrawSceneOverlay(dispatcher, frameGraph, desc);

        renderer.textureEngine.TextureFeedbackPass(
            dispatcher,
            frameGraph,
            viewportCamera,
            depthBuffer.WH,
            drawSceneRes.passes,
            drawSceneRes.skinnedDraws,
            temporaries.ReserveConstantBuffer,
            temporaries.ReserveVertexBuffer);

    }
    else
        FlexKit::ClearBackBuffer(frameGraph, renderTarget, { 0.25f, 0.25f, 0.25f, 0 });

    hud.DrawImGui(dT, dispatcher, frameGraph, temporaries.ReserveVertexBuffer, temporaries.ReserveConstantBuffer, renderTarget);
    FlexKit::PresentBackBuffer(frameGraph, renderTarget);

    T += dT;
}


/************************************************************************************************/


void EditorViewport::DrawSceneOverlay(FlexKit::UpdateDispatcher& Dispatcher, FlexKit::FrameGraph& frameGraph, EditorViewport::DrawSceneOverlay_Desc& desc)
{
    if (!isVisible())
        return;

    struct DrawOverlay
    {
        const FlexKit::PVS&                     brushes;
        const FlexKit::PointLightHandleList&    lights;

        FlexKit::ReserveVertexBufferFunction    ReserveVertexBuffer;
        FlexKit::ReserveConstantBufferFunction  ReserveConstantBuffer;

        FlexKit::FrameResourceHandle    renderTarget;
    };

    frameGraph.AddNode<DrawOverlay>(
        DrawOverlay{
            desc.brushes,
            desc.lights.GetData().pointLightShadows,

            desc.buffers.ReserveVertexBuffer,
            desc.buffers.ReserveConstantBuffer,
        },
        [&](FlexKit::FrameGraphNodeBuilder& builder, DrawOverlay& data)
        {
            data.renderTarget = builder.RenderTarget(desc.renderTarget);
            builder.AddDataDependency(desc.lights);
        },
        [&, viewportCamera = viewportCamera](DrawOverlay& data, FlexKit::ResourceHandler& resources, FlexKit::Context& ctx, auto& allocator)
        {
            struct Vertex
            {
                float3 Position;
                float3 Color;
            };

            auto& PVS           = data.brushes;
            auto& pointLights   = data.lights;

            auto& visibilityComponent   = FlexKit::SceneVisibilityComponent::GetComponent();
            auto& pointLightComponnet   = FlexKit::PointLightComponent::GetComponent();

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

            struct PassConstants {
                FlexKit::float4x4 x;
            } passConstantData;

            auto constantBuffer = data.ReserveConstantBuffer(
                FlexKit::AlignedSize<FlexKit::Camera::ConstantBuffer>() + 
                FlexKit::AlignedSize<PassConstants>());

            
            FlexKit::ConstantBufferDataSet cameraConstants  { FlexKit::GetCameraConstants(viewportCamera), constantBuffer };
            FlexKit::ConstantBufferDataSet passConstants    { passConstantData, constantBuffer };

            auto viewportObjects = selectionContext.GetSelectionType() == ViewportObjectList_ID ? selectionContext.GetSelection<ViewportObjectList>() : ViewportObjectList{};

            // Draw Point Lights
            for (auto& lightHandle : pointLights)
            {
                bool selectedLight = false;
                for (auto& viewportObject : viewportObjects)
                {
                    FlexKit::PointLightHandle pointlight = FlexKit::GetPointLight(viewportObject->gameObject);
                    if (pointlight == lightHandle)
                        selectedLight = true;
                }

                struct Vertex
                {
                    FlexKit::float4 position;
                    FlexKit::float4 color;
                    FlexKit::float2 UV;
                };

                const auto temp = pointLightComponnet[lightHandle].Position;
                const float3 position   = FlexKit::GetPositionW(pointLightComponnet[lightHandle].Position);
                const float radius      = pointLightComponnet[lightHandle].R;

                const size_t divisions  = 64;
                FlexKit::VBPushBuffer VBBuffer   = data.ReserveVertexBuffer(sizeof(Vertex) * 6 * divisions);

			    const float Step = 2.0f * (float)FlexKit::pi / divisions;
                const auto range = FlexKit::MakeRange(0, divisions);
                const float4 color = selectedLight ? float4{ 1, 1, 1, 1 } : float4{ 0, 0, 0, 1 };

			    const FlexKit::VertexBufferDataSet vertices{
                    FlexKit::SET_TRANSFORM_OP,
                    range,
				    [&](const size_t I, auto& pushBuffer) -> Vertex
				    {
					    const float3 V1 = { radius * cos(Step * (I + 1)),	0.0f, (radius * sin(Step * (I + 1))) };
					    const float3 V2 = { radius * cos(Step * I),		    0.0f, (radius * sin(Step * I)) };

                        pushBuffer.Push(Vertex{ float4{ V1, 1 }, color, float2{ 0, 0 } });
                        pushBuffer.Push(Vertex{ float4{ V2, 1 }, color, float2{ 1, 1 } });

                        pushBuffer.Push(Vertex{ float4{ V1.x, V1.z, 0, 1 }, color, float2{ 0, 0 } });
                        pushBuffer.Push(Vertex{ float4{ V2.x, V2.z, 0, 1 }, color, float2{ 1, 1 } });

                        
                        pushBuffer.Push(Vertex{ float4{ 0, V1.x, V1.z, 1 }, color, float2{ 0, 0 } });
                        pushBuffer.Push(Vertex{ float4{ 0, V2.x, V2.z, 1 }, color, float2{ 1, 1 } });

                        return {};
				    },                                                  
				    VBBuffer };

                ctx.SetVertexBuffers({ vertices });

                struct {
                    float4     unused1;
                    float4     unused2;
                    float4x4   transform;
                } CB_Data {
				    .unused1    = float4{ 1, 1, 1, 1 },
				    .unused2    = float4{ 1, 1, 1, 1 },
				    .transform  = FlexKit::TranslationMatrix(position)
			    };

                auto constantBuffer = data.ReserveConstantBuffer(256);
                FlexKit::ConstantBufferDataSet constants{ CB_Data, constantBuffer };

                ctx.SetGraphicsConstantBufferView(1, cameraConstants);
                ctx.SetGraphicsConstantBufferView(2, constants);

                ctx.Draw(divisions * 6);
            }


            // Draw Selection
            if(selectionContext.GetSelectionType() == ViewportObjectList_ID)
            {
                struct Vertex
                {
                    FlexKit::float4 position;
                    FlexKit::float4 color;
                    FlexKit::float2 UV;
                };

                FlexKit::VBPushBuffer VBBuffer = data.ReserveVertexBuffer(sizeof(Vertex) * 24 * viewportObjects.size());

                for (auto& object : viewportObjects)
                {
                    if (!object->gameObject.hasView(FlexKit::BrushComponentID))
                        continue;


                    const auto      node        = FlexKit::GetSceneNode(object->gameObject);
                    const auto      BS          = FlexKit::GetBoundingSphere(object->gameObject);
                    const auto      radius      = BS.w;
                    const float3    position    = FlexKit::GetPositionW(node);
                    const size_t    divisions   = 64;


			        const float Step = 2.0f * (float)FlexKit::pi / divisions;
                    const auto range = FlexKit::MakeRange(0, divisions);

                    Vertex vertices[] = {
                        // Top
                        { float4{ -radius,  radius,  radius, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
                        { float4{  radius,  radius,  radius, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

                        { float4{ -radius,  radius, -radius, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
                        { float4{  radius,  radius, -radius, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

                        { float4{ -radius,  radius,  radius, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
                        { float4{ -radius,  radius, -radius, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },


                        { float4{  radius,  radius,  radius, 1 }, float4{  1, 1, 1, 1 }, float2{ 0, 0 } },
                        { float4{  radius,  radius, -radius, 1 }, float4{  1, 1, 1, 1 }, float2{ 0, 0 } },

                        // Bottom
                        { float4{ -radius, -radius,  radius, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
                        { float4{  radius, -radius,  radius, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

                        { float4{ -radius, -radius, -radius, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
                        { float4{  radius, -radius, -radius, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

                        { float4{ -radius, -radius,  radius, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
                        { float4{ -radius, -radius, -radius, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

                        { float4{  radius, -radius,  radius, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
                        { float4{  radius, -radius, -radius, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

                        // Sides
                        { float4{  radius,  radius,  radius, 1 }, float4{ 1, 1,  1, 1 }, float2{ 0, 0 } },
                        { float4{  radius, -radius,  radius, 1 }, float4{ 1, 1,  1, 1 }, float2{ 0, 0 } },

                        { float4{  radius,  radius, -radius, 1 }, float4{ 1, 1,  1, 1 }, float2{ 0, 0 } },
                        { float4{  radius, -radius, -radius, 1 }, float4{ 1, 1,  1, 1 }, float2{ 0, 0 } },


                        { float4{ -radius,  radius,  radius, 1 }, float4{ 1, 1,  1, 1 }, float2{ 0, 0 } },
                        { float4{ -radius, -radius,  radius, 1 }, float4{ 1, 1,  1, 1 }, float2{ 0, 0 } },


                        { float4{ -radius,  radius, -radius, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
                        { float4{ -radius, -radius, -radius, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
                    };

                    const FlexKit::VertexBufferDataSet vbDataSet{
                            vertices,
                            sizeof(vertices),
                            VBBuffer };

                    ctx.SetVertexBuffers({ vbDataSet });

                    struct {
                        float4     unused1;
                        float4     unused2;
                        float4x4   transform;
                    } CB_Data {
				        .unused1    = float4{ 1, 1, 1, 1 },
				        .unused2    = float4{ 1, 1, 1, 1 },
				        .transform  = FlexKit::TranslationMatrix(position)
			        };

                    auto constantBuffer = data.ReserveConstantBuffer(256);
                    const FlexKit::ConstantBufferDataSet constants{ CB_Data, constantBuffer };

                    ctx.SetGraphicsConstantBufferView(1, cameraConstants);
                    ctx.SetGraphicsConstantBufferView(2, constants);

                    ctx.Draw(24);
                }
            }
        });
}


/************************************************************************************************/


std::shared_ptr<FlexKit::TriMesh> EditorViewport::BuildTriMesh(FlexKit::MeshUtilityFunctions::OptimizedMesh& mesh) const
{
    return {};
}


/**********************************************************************

Copyright (c) 2021 Robert May

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
