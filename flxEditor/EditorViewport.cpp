#include "EditorViewport.h"
#include "DXRenderWindow.h"
#include "EditorRenderer.h"
#include "qevent.h"

#include <QtWidgets/qmenubar.h>


/************************************************************************************************/


EditorViewport::EditorViewport(EditorRenderer& IN_renderer, SelectionContext& IN_context, QWidget *parent)
    :   QWidget{ parent }
    ,   menuBar             { new QMenuBar{ this } }
    ,   renderer            { IN_renderer }
    ,   gbuffer             { { 100, 200 }, IN_renderer.framework.GetRenderSystem() }
    ,   depthBuffer         { IN_renderer.framework.GetRenderSystem(), { 100, 200 } }
    ,   viewportCamera      { FlexKit::CameraComponent::GetComponent().CreateCamera() }
    ,   selectionContext    { IN_context }
{
	ui.setupUi(this);

    auto layout = findChild<QBoxLayout*>("verticalLayout");
    layout->setContentsMargins(0, 4, 0, 2);
    layout->setMenuBar(menuBar);

    setMinimumSize(100, 100);
    setMaximumSize({ 1024 * 16, 1024 * 16 });

    auto file   = menuBar->addMenu("View");
    auto select = file->addAction("Select Texture");

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

    this->scene = viewportScene;

    for (auto node : scene->sceneResource->nodes)
    {
        auto newNode = FlexKit::GetZeroedNode();
        nodes.push_back(newNode);

        const auto position = node.position;
        const auto q = node.Q;

        FlexKit::SetOrientationL(newNode, q);
        FlexKit::SetPositionL(newNode, position);

        SetFlag(newNode, FlexKit::SceneNodes::StateFlags::SCALE);
    }

    for (auto& entity : scene->sceneResource->entities)
    {
        auto viewObject = std::make_shared<ViewportGameObject>();
        viewObject->objectID = entity.objectID;

        viewportScene->sceneObjects.emplace_back(viewObject);

        for (auto& componentEntry : entity.components)
        {
            switch (componentEntry->id)
            {
            case FlexKit::TransformComponentID:
            {
                if (entity.Node != -1)
                    viewObject->gameObject.AddView<FlexKit::SceneNodeView<>>(nodes[entity.Node]);
            }   break;
            case FlexKit::DrawableComponentID:
            {
                auto drawableComponent = std::static_pointer_cast<FlexKit::ResourceBuilder::DrawableComponent>(componentEntry);

                if (drawableComponent)
                {
                    auto res = scene->FindSceneResource(drawableComponent->MeshGuid);

                    if (!res) // TODO: Mesh not found, use placeholder model?
                        continue;


                    if (auto prop = res->properties.find(GetCRCGUID(TriMeshHandle)); prop != res->properties.end())
                    {
                        FlexKit::TriMeshHandle handle = std::any_cast<FlexKit::TriMeshHandle>(prop->second);

                        viewObject->gameObject.AddView<FlexKit::DrawableView>(handle, nodes[entity.Node]);
                    }
                    else
                    {
                        auto meshBlob                   = res->resource->CreateBlob();
                        FlexKit::TriMeshHandle handle   = FlexKit::LoadTriMeshIntoTable(renderSystem, renderSystem.GetImmediateUploadQueue(), meshBlob.buffer, meshBlob.bufferSize);

                        res->properties[GetCRCGUID(TriMeshHandle)] = std::any{ handle };

                        viewObject->gameObject.AddView<FlexKit::DrawableView>(handle, nodes[entity.Node]);
                    }

                    viewportScene->scene.AddGameObject(viewObject->gameObject, nodes[entity.Node]);
                    FlexKit::SetBoundingSphereFromMesh(viewObject->gameObject);
                }
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
}


/************************************************************************************************/


void EditorViewport::keyPressEvent(QKeyEvent* event)
{
    auto keyCode = event->key();

    if (keyCode == Qt::Key_Alt)
        state = InputState::PanOrbit;
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
    if (!scene)
        return;

    switch (state)
    {
    case InputState::ClickSelect:
    {
        if (event->button() == Qt::MouseButton::LeftButton)
        {
            auto qPos = event->localPos();

            const FlexKit::uint2 XY = FlexKit::uint2{ (uint32_t)qPos.x(), (uint32_t)qPos.y() };
            const FlexKit::uint2 screenWH = depthBuffer.WH;

            const FlexKit::float2 UV{ XY[0] / float(screenWH[0]), XY[1] / float(screenWH[1]) };
            const FlexKit::float2 ScreenCoord{ FlexKit::float2{ 2, -2 } * UV + FlexKit::float2{ -1.0f, 1.0f } };

            const auto cameraConstants      = FlexKit::GetCameraConstants(viewportCamera);
            const auto cameraOrientation    = FlexKit::GetOrientation(FlexKit::GetCameraNode(viewportCamera));

            const FlexKit::float3 v_dir = cameraOrientation * ((Inverse(Inverse(cameraConstants.Proj)) * FlexKit::float4(ScreenCoord.x, ScreenCoord.y,  1.0f, 1.0f)).xyz()).normal();
            const FlexKit::float3 v_o   = cameraConstants.WPOS.xyz();

            auto results = scene->RayCast(
                FlexKit::Ray{
                            .D = v_dir.normal(),
                            .O = v_o,});

            for(auto& sceneObject : results)
                FlexKit::SetVisable(sceneObject->gameObject, false);

            selectionContext.selection  = std::move(results);
            selectionContext.type       = ViewportObjectList_ID;
        }
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

                auto node   = FlexKit::GetCameraNode(viewportCamera);
                auto q      = FlexKit::GetOrientation(node);

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
                FlexKit::int2 newPosition{ event->pos().x(), event->pos().y() };
                FlexKit::int2 deltaPosition = previousMousePosition - newPosition;

                auto node   = FlexKit::GetCameraNode(viewportCamera);
                auto q      = FlexKit::GetOrientation(node);

                auto x  = float(deltaPosition[0]);
                FlexKit::Yaw(node, float(deltaPosition[0]) / 1000.0f);
                FlexKit::Pitch(node, float(deltaPosition[1]) / 1000.0f);

                MarkCameraDirty(viewportCamera);

                previousMousePosition = newPosition;
            }
        }
        else
            previousMousePosition = FlexKit::int2{ -160000, -160000 };
    }   break;
    default:
        break;
    }
}


/************************************************************************************************/


void EditorViewport::wheelEvent(QWheelEvent* event)
{
    auto node = FlexKit::GetCameraNode(viewportCamera);
    auto q = FlexKit::GetOrientation(node);

    FlexKit::TranslateWorld(node, q * float3(0, 0, event->angleDelta().x() / -10.0f));
    MarkCameraDirty(viewportCamera);
}

/************************************************************************************************/


void EditorViewport::mouseReleaseEvent(QMouseEvent* event)
{
    previousMousePosition = FlexKit::int2{ -160000, -160000 };
}


/************************************************************************************************/


void EditorViewport::Render(FlexKit::UpdateDispatcher& dispatcher, double dT, TemporaryBuffers& temporaries, FlexKit::FrameGraph& frameGraph, FlexKit::ResourceHandle renderTarget, FlexKit::ThreadSafeAllocator& allocator)
{
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

            .additionalGbufferPass  = {},
            .additionalShadowPass   = {}
        };

        auto drawnScene = renderer.worldRender.DrawScene(dispatcher, frameGraph, sceneDesc, targets, FlexKit::SystemAllocator, allocator);

        renderer.textureEngine.TextureFeedbackPass(
            dispatcher,
            frameGraph,
            viewportCamera,
            depthBuffer.WH,
            drawnScene.PVS,
            drawnScene.skinnedDraws,
            temporaries.ReserveConstantBuffer,
            temporaries.ReserveVertexBuffer);
    }
    else
        FlexKit::ClearBackBuffer(frameGraph, renderTarget, { 1, 0, 1, 0 });


    FlexKit::PresentBackBuffer(frameGraph, renderTarget);

    T += dT;
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
