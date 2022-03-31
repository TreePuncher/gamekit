#include "PCH.h"
#include "EditorViewport.h"
#include "DXRenderWindow.h"
#include "EditorRenderer.h"
#include "qevent.h"
#include "DebugUI.cpp"
#include "physicsutilities.h"
#include "SceneLoadingContext.h"
#include "EditorInspectorView.h"

#include <QtWidgets/qmenubar.h>
#include <QShortcut>
#include <fmt/format.h>


using FlexKit::float2;
using FlexKit::float3;
using FlexKit::float4;
using FlexKit::float4x4;


/************************************************************************************************/


class EditorVewportSelectionMode : public IEditorViewportMode
{
public:
    EditorVewportSelectionMode(SelectionContext&, std::shared_ptr<ViewportScene>&, DXRenderWindow* window, FlexKit::CameraHandle camera);

    ViewportModeID GetModeID() const override { return VewportSelectionModeID; };

    void mousePressEvent    (QMouseEvent* event) override;
    void DrawImguI();

    DXRenderWindow*         renderWindow;
    FlexKit::CameraHandle   viewportCamera;

    SelectionContext&               selectionContext;
    std::shared_ptr<ViewportScene>  scene;
};


class EditorVewportPanMode : public IEditorViewportMode
{
public:
    EditorVewportPanMode(SelectionContext&, std::shared_ptr<ViewportScene>&, DXRenderWindow* window, FlexKit::CameraHandle camera, ViewportMode_ptr IN_previous = nullptr);

    ViewportModeID GetModeID() const override { return VewportPanModeID; };

    void keyPressEvent      (QKeyEvent* event) override;

    void mouseMoveEvent     (QMouseEvent* event) override;
    void mouseReleaseEvent  (QMouseEvent* event) override;
    void wheelEvent         (QWheelEvent* event) override;

    void DrawImguI() override;
    void Draw(FlexKit::UpdateDispatcher& Dispatcher, FlexKit::FrameGraph& frameGraph, TemporaryBuffers& temps, FlexKit::ResourceHandle renderTarget, FlexKit::ResourceHandle depthBuffer) override;

    ViewportMode_ptr        previous;
    DXRenderWindow*         renderWindow;
    FlexKit::CameraHandle   viewportCamera;

    SelectionContext&               selectionContext;
    std::shared_ptr<ViewportScene>  scene;

    FlexKit::int2 previousMousePosition = FlexKit::int2{ -160000, -160000 };
    float panSpeed = 10.0f;
};


class EditorVewportTranslationMode : public IEditorViewportMode
{
public:
    EditorVewportTranslationMode(SelectionContext&, DXRenderWindow*, FlexKit::CameraHandle, FlexKit::ImGUIIntegrator& hud);

    ViewportModeID GetModeID() const override { return TranslationModeID; };

    void keyPressEvent      (QKeyEvent* event) override;

    void mousePressEvent    (QMouseEvent* event) override;
    void mouseReleaseEvent  (QMouseEvent* event) override;

    void DrawImguI  () override;

    DXRenderWindow*             renderWindow;
    SelectionContext&           selectionContext;
    FlexKit::CameraHandle       viewportCamera;
    FlexKit::ImGUIIntegrator&   hud;

    FlexKit::int2 previousMousePosition     = FlexKit::int2{ -160000, -160000 };
    ImGuizmo::OPERATION manipulatorState    = ImGuizmo::OPERATION::TRANSLATE;
    ImGuizmo::MODE mode                     = ImGuizmo::MODE::LOCAL;
};


/************************************************************************************************/


EditorVewportTranslationMode::EditorVewportTranslationMode(
    SelectionContext&           IN_selectionContext,
    DXRenderWindow*             IN_renderWindow,
    FlexKit::CameraHandle       IN_camera,
    FlexKit::ImGUIIntegrator&   IN_hud) :
        selectionContext    { IN_selectionContext },
        renderWindow        { IN_renderWindow },
        viewportCamera      { IN_camera },
        hud                 { IN_hud } {}


void EditorVewportTranslationMode::keyPressEvent(QKeyEvent* event)
{
    auto keyCode = event->key();

    if (keyCode == Qt::Key_T)
        mode = mode == ImGuizmo::MODE::LOCAL ? ImGuizmo::MODE::WORLD : ImGuizmo::MODE::LOCAL;

    if (keyCode == Qt::Key_W)
        manipulatorState = ImGuizmo::OPERATION::TRANSLATE;

    if (keyCode == Qt::Key_E)
        manipulatorState = ImGuizmo::OPERATION::SCALE;

    if (keyCode == Qt::Key_R)
        manipulatorState = ImGuizmo::OPERATION::ROTATE;
}

void EditorVewportTranslationMode::mousePressEvent(QMouseEvent* event)
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

void EditorVewportTranslationMode::mouseReleaseEvent(QMouseEvent* event)
{
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

void EditorVewportTranslationMode::DrawImguI()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    FlexKit::CameraComponent::GetComponent().GetCamera(viewportCamera).UpdateMatrices();
    auto& camera                = FlexKit::CameraComponent::GetComponent().GetCamera(viewportCamera);
    const float4x4 view         = camera.View.Transpose();
    const float4x4 projection   = camera.Proj;
    const float4x4 grid         = float4x4{   1,   0,   0,   0,
                                              0,   1,   0,   0,
                                              0,   0,   1,   0,
                                              0,   0,   0,   1  };

    QPoint globalCursorPos      = QCursor::pos();
    const auto localPosition    = renderWindow->mapFromGlobal(globalCursorPos);

    if (localPosition.x() >= 0 && localPosition.y() >= 0 &&
        localPosition.x() * 1.5f < io.DisplaySize.x  &&
        localPosition.y() * 1.5f < io.DisplaySize.y &&

        selectionContext.GetSelectionType() == ViewportObjectList_ID)
    {
        auto selection = selectionContext.GetSelectionType() == ViewportObjectList_ID ? selectionContext.GetSelection<ViewportSelection>() : ViewportSelection{};

        if (selection.viewportObjects.size())
        {
            auto& gameObject = selection.viewportObjects.front()->gameObject;

            float4x4 wt     = FlexKit::GetWT(gameObject).Transpose();
            float4x4 delta  = float4x4::Identity();

            if (ImGuizmo::Manipulate(view, projection, manipulatorState, mode, wt, delta))
                FlexKit::SetWT(gameObject, wt);

            const std::string text = fmt::format("\nWT\n{}, {}, {}, {}, \n{}, {}, {}, {}, \n{}, {}, {}, {},\n{}, {}, {}, {} ]\n",
                                        wt[0][0], wt[0][1], wt[0][2], wt[0][3],
                                        wt[1][0], wt[1][1], wt[1][2], wt[1][3],
                                        wt[2][0], wt[2][1], wt[2][2], wt[2][3],
                                        wt[3][0], wt[3][1], wt[3][2], wt[3][3]);

            if (0)
            {
                if (ImGui::Begin("Transform", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
                    ImGui::SetWindowPos({ 0, 0 });
                    ImGui::Text(text.c_str());
                }
                ImGui::End();
            }
        }
    }
}


/************************************************************************************************/


EditorVewportSelectionMode::EditorVewportSelectionMode(
    SelectionContext&               IN_selection,
    std::shared_ptr<ViewportScene>& IN_scene,
    DXRenderWindow*                 IN_window,
    FlexKit::CameraHandle           IN_camera) :
        renderWindow        { IN_window     },
        viewportCamera      { IN_camera     },
        selectionContext    { IN_selection  },
        scene               { IN_scene      } {}


void EditorVewportSelectionMode::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        auto qPos = renderWindow->mapFromGlobal(event->globalPos());

        const FlexKit::uint2 XY{ (uint32_t)qPos.x(), (uint32_t)qPos.y() };
        const FlexKit::uint2 screenWH = renderWindow->WH();

        const FlexKit::float2 UV{ XY[0] / float(screenWH[0]), XY[1] / float(screenWH[1]) };
        const FlexKit::float2 ScreenCoord{ FlexKit::float2{ 2, -2 } * UV + FlexKit::float2{ -1.0f, 1.0f } };

        const auto cameraConstants      = FlexKit::GetCameraConstants(viewportCamera);
        const auto cameraOrientation    = FlexKit::GetOrientation(FlexKit::GetCameraNode(viewportCamera));

        const FlexKit::float3 v_dir = cameraOrientation * (Inverse(cameraConstants.Proj) * FlexKit::float4{ ScreenCoord.x, ScreenCoord.y,  1.0f, 1.0f }).xyz().normal();
        const FlexKit::float3 v_o   = cameraConstants.WPOS.xyz();

        auto results = scene->RayCast(
            FlexKit::Ray{
                        .D = v_dir,
                        .O = v_o,});

        if(results.size())
        {
            ViewportSelection selection;
            selection.viewportObjects.push_back(results.front());
            selection.scene = scene.get();

            selectionContext.Clear();
            selectionContext.selection  = std::move(selection); 
            selectionContext.type       = ViewportObjectList_ID;
        }
    }
}


/************************************************************************************************/


void EditorVewportSelectionMode::DrawImguI()
{
    if (ImGui::Begin("SelectionMode", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        ImGui::SetWindowPos({ 0, 0 });
        ImGui::SetWindowSize({ 400, 400 });
    }
    ImGui::End();
}


/************************************************************************************************/


EditorVewportPanMode::EditorVewportPanMode(
    SelectionContext&               IN_selection,
    std::shared_ptr<ViewportScene>& IN_scene,
    DXRenderWindow*                 IN_window,
    FlexKit::CameraHandle           IN_camera,
    ViewportMode_ptr                IN_previous) :
        selectionContext    { IN_selection  },
        scene               { IN_scene      },
        viewportCamera      { IN_camera     },
        renderWindow        { IN_window     },
        previous{ IN_previous }  {}


void EditorVewportPanMode::keyPressEvent(QKeyEvent* event)
{
    auto keyCode = event->key();

    if (keyCode == Qt::Key_F)
    {
        if (scene == nullptr)
            return;

        if (selectionContext.GetSelectionType() == ViewportObjectList_ID)
        {
            auto selection = selectionContext.GetSelection<ViewportSelection>();

            FlexKit::AABB aabb;
            for (auto& object : selection.viewportObjects)
            {
                auto boundingSphere = FlexKit::GetBoundingSphere(object->gameObject);
                aabb = aabb + boundingSphere;
            }

            const FlexKit::Camera c = FlexKit::CameraComponent::GetComponent().GetCamera(viewportCamera);

            const auto target           = aabb.MidPoint();
            const auto desiredDistance  = (2.0f / std::sqrt(2.0f)) * aabb.Span().magnitude() / std::tan(c.FOV);

            auto position_VS        = c.View.Transpose() * float4 { target, 1 };
            auto updatedPosition_WS = c.IV.Transpose() * float4 { position_VS.x, position_VS.y, position_VS.z + desiredDistance, 1 };
            const auto node         = FlexKit::GetCameraNode(viewportCamera);

            FlexKit::SetPositionW(node, updatedPosition_WS.xyz());
            FlexKit::MarkCameraDirty(viewportCamera);
        }
    }

}


void EditorVewportPanMode::mouseMoveEvent(QMouseEvent* event)
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
}


void EditorVewportPanMode::mouseReleaseEvent(QMouseEvent* event)
{
    previousMousePosition = FlexKit::int2{ -160000, -160000 };
}


void EditorVewportPanMode::wheelEvent(QWheelEvent* event)
{
    const auto node = FlexKit::GetCameraNode(viewportCamera);
    const auto q    = FlexKit::GetOrientation(node);

    FlexKit::TranslateWorld(node, q * float3{ 0, 0, event->angleDelta().x() / -100.0f });
    MarkCameraDirty(viewportCamera);
}


void EditorVewportPanMode::DrawImguI()
{
    if (previous)
        previous->DrawImguI();
}


void EditorVewportPanMode::Draw(FlexKit::UpdateDispatcher& dispatcher, FlexKit::FrameGraph& frameGraph, TemporaryBuffers& temps, FlexKit::ResourceHandle renderTarget, FlexKit::ResourceHandle depthBuffer)
{
    if (previous)
        previous->Draw(dispatcher, frameGraph, temps, renderTarget, depthBuffer);
}


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

    auto scene      = menuBar->addMenu("Scene");
    auto saveScene  = scene->addAction("Save");
    saveScene->connect(saveScene, &QAction::triggered, this, &EditorViewport::SaveScene);

    auto file           = menuBar->addMenu("Add");
    auto addGameObject  = file->addAction("Empty GameObject");

    addGameObject->connect(addGameObject, &QAction::triggered,
        [&]
        {
            auto& scene = GetScene();

            if (scene == nullptr)
                return;

            scene->CreateObject();
        });

    auto addLight = file->addAction("Point Light");
    addLight->connect(addLight, &QAction::triggered,
        [&]
        {
            auto& scene = GetScene();

            if (scene == nullptr)
                return;

            ViewportGameObject_ptr viewportObject = scene->CreateObject();

            viewportObject->gameObject.AddView<FlexKit::SceneNodeView<>>();
            viewportObject->gameObject.AddView<FlexKit::PointLightView>(float3{ 1, 1, 1 }, 10, 10);

            scene->scene.AddGameObject(*viewportObject, FlexKit::GetSceneNode(*viewportObject));

            FlexKit::SetBoundingSphereFromLight(*viewportObject);
        });

    auto debugMenu = menuBar->addMenu("Debug");
    auto reloadShaders = debugMenu->addAction("ReloadShaders");
    reloadShaders->connect(reloadShaders, &QAction::triggered,
        [&]()
        {
            IN_renderer.framework.GetRenderSystem().QueuePSOLoad(FlexKit::SHADINGPASS);
            IN_renderer.framework.GetRenderSystem().QueuePSOLoad(FlexKit::CREATECLUSTERLIGHTLISTS);
            IN_renderer.framework.GetRenderSystem().QueuePSOLoad(FlexKit::CREATECLUSTERBUFFER);
            IN_renderer.framework.GetRenderSystem().QueuePSOLoad(FlexKit::CREATECLUSTERS);
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

    gbufferPass =
        []
        {
            auto& materials = FlexKit::MaterialComponent::GetComponent();
            auto material = materials.CreateMaterial();

            materials.Add2Pass(material, FlexKit::PassHandle{ GetCRCGUID(PBR_CLUSTERED_DEFERRED) });
            materials.AddRef(material);

            return material;
        }();
}


/************************************************************************************************/


EditorViewport ::~EditorViewport()
{
}


/************************************************************************************************/


void EditorViewport::resizeEvent(QResizeEvent* evt)
{
    QWidget::resizeEvent(evt);

    FlexKit::uint2 newWH = { evt->size().width() * 1.5, evt->size().height() * 1.5 };

    renderWindow->resize(evt->size());
    depthBuffer.Resize(newWH);
    gbuffer.Resize(newWH);

    FlexKit::SetCameraAspectRatio(viewportCamera, float(evt->size().width())/ float(evt->size().height()));
    MarkCameraDirty(viewportCamera);
}


/************************************************************************************************/


FlexKit::TriMeshHandle EditorViewport::LoadTriMeshResource(ProjectResource_ptr res)
{
    if (auto prop = res->properties.find(GetCRCGUID(TriMeshHandle)); prop != res->properties.end())
    {
        FlexKit::TriMeshHandle handle = std::any_cast<FlexKit::TriMeshHandle>(prop->second);
        AddRef(handle);

        return handle;
    }
    else
    {
        auto& renderSystem = renderer.framework.GetRenderSystem();

        auto meshBlob = res->resource->CreateBlob();
        FlexKit::TriMeshHandle handle = FlexKit::LoadTriMeshIntoTable(renderSystem.GetImmediateUploadQueue(), meshBlob.buffer, meshBlob.bufferSize);

        res->properties[GetCRCGUID(TriMeshHandle)] = std::any{ handle };
        return handle;
    }
}



void EditorViewport::SetScene(EditorScene_ptr newScene)
{
    if (scene)
    {
        scene->scene.bvh.Clear();
        scene->scene.ClearScene();

        for (auto& object : scene->sceneObjects)
            object->gameObject.Release();

        FlexKit::SceneVisibilityComponent::GetComponent();

        selectionContext.Clear();
        selectionContext.selection  = std::any{};
        selectionContext.type       = -1;
    }

    auto viewportScene = std::make_shared<ViewportScene>(newScene);
    auto& renderSystem = renderer.framework.GetRenderSystem();
    
    
    FlexKit::SceneLoadingContext ctx{
        .scene = viewportScene->scene,
        .layer = viewportScene->GetLayer(),
        .nodes = Vector<FlexKit::NodeHandle>(FlexKit::SystemAllocator)
    };

    for (auto& dependantResource : newScene->sceneResources)
    {
        auto blob   = dependantResource->resource->CreateBlob();
        auto buffer = blob.buffer;

        blob.buffer     = nullptr;
        blob.bufferSize = 0;

        FlexKit::AddAssetBuffer((Resource*)buffer);
    }

    std::vector<FlexKit::NodeHandle> nodes;
    for (auto node : newScene->sceneResource->nodes)
    {
        auto newNode = FlexKit::GetZeroedNode();
        nodes.push_back(newNode);

        const auto position     = node.position;
        const auto orientation  = node.orientation;

        FlexKit::SetScale(newNode, float3{ node.scale.x, node.scale.x, node.scale.x });
        FlexKit::SetOrientationL(newNode, orientation);
        FlexKit::SetPositionL(newNode, position);

        if(node.parent != -1)
            FlexKit::SetParentNode(nodes[node.parent], newNode);

        SetFlag(newNode, FlexKit::SceneNodes::StateFlags::SCALE);
    }

    for (auto& entity : newScene->sceneResource->entities)
    {
        auto viewObject = std::make_shared<ViewportGameObject>();
        viewObject->objectID = entity.objectID;
        viewportScene->sceneObjects.emplace_back(viewObject);

        //if (entity.Node != -1)
        //    viewObject->gameObject.AddView<FlexKit::SceneNodeView<>>(nodes[entity.Node]);

        if (entity.id.size())
            viewObject->gameObject.AddView<FlexKit::StringIDView>(entity.id.c_str(), entity.id.size());

        bool addedToScene = false;

        for (auto& componentEntry : entity.components)
        {

            switch (componentEntry->id)
            {
            case FlexKit::TransformComponentID:
            {
                auto nodeComponent = std::static_pointer_cast<FlexKit::EntitySceneNodeComponent>(componentEntry);
                viewObject->gameObject.AddView<FlexKit::SceneNodeView<>>(nodes[nodeComponent->nodeIdx]);
            }   break;
            case FlexKit::BrushComponentID:
            {
                auto brushComponent = std::static_pointer_cast<FlexKit::EntityBrushComponent>(componentEntry);

                if (brushComponent)
                {
                    auto res = newScene->FindSceneResource(brushComponent->MeshGuid);

                    if (!res) // TODO: Mesh not found, use placeholder model?
                        continue;


                    auto& materials = FlexKit::MaterialComponent::GetComponent();
                    auto material   = materials.CreateMaterial(gbufferPass);


                    //TODO: Seems the issue if further up the asset pipeline. Improve material generation?
                    if (brushComponent->material.subMaterials.size() > 1)
                    {
                        for (auto& subMaterialData : brushComponent->material.subMaterials)
                        {
                            auto subMaterial = materials.CreateMaterial();
                            materials.AddSubMaterial(material, subMaterial);

                            FlexKit::ReadContext rdCtx{};
                            for (auto texture : subMaterialData.textures)
                                materials.AddTexture(texture, subMaterial, rdCtx);
                        }
                    }
                    else if(brushComponent->material.subMaterials.size() == 1)
                    {
                        auto& subMaterialData = brushComponent->material.subMaterials[0];

                        FlexKit::ReadContext rdCtx{};

                        for (auto texture : subMaterialData.textures)
                            materials.AddTexture(texture, material, rdCtx);
                    }

                    FlexKit::TriMeshHandle handle = LoadTriMeshResource(res);

                    auto& view = (FlexKit::BrushView&)EditorInspectorView::ConstructComponent(FlexKit::BrushComponentID, *viewObject, *viewportScene);

                    auto& brush         = view.GetBrush();
                    brush.material      = material;
                    brush.MeshHandle    = handle;

                    viewObject->gameObject.AddView<FlexKit::MaterialComponentView>(material);

                    if(!addedToScene)
                        viewportScene->scene.AddGameObject(viewObject->gameObject, FlexKit::GetSceneNode(viewObject->gameObject));

                    addedToScene = true;

                    FlexKit::SetBoundingSphereFromMesh(viewObject->gameObject);
                }
            }   break;
            case FlexKit::PointLightComponentID:
            {
                if (!addedToScene)
                    viewportScene->scene.AddGameObject(viewObject->gameObject, FlexKit::GetSceneNode(viewObject->gameObject));

                auto  blob          = componentEntry->GetBlob();
                auto& component     = FlexKit::ComponentBase::GetComponent(componentEntry->id);
                auto& componentView = EditorInspectorView::ConstructComponent(componentEntry->id, *viewObject, *viewportScene);

                component.AddComponentView(viewObject->gameObject, &ctx, blob, blob.size(), FlexKit::SystemAllocator);
                FlexKit::SetBoundingSphereFromLight(viewObject->gameObject);

                addedToScene = true;
            }   break;
            default:
            {
                auto  blob              = componentEntry->GetBlob();
                //auto& componentView     = EditorInspectorView::ConstructComponent(componentEntry->id, *viewObject, *viewportScene);
                auto& component         = FlexKit::ComponentBase::GetComponent(componentEntry->id);

                component.AddComponentView(viewObject->gameObject, &ctx, blob, blob.size(), FlexKit::SystemAllocator);
            }   break;
            }
        }
    }


    scene = viewportScene;
}


/************************************************************************************************/


FlexKit::Ray EditorViewport::GetMouseRay() const
{
    const auto qPos = renderWindow->mapFromGlobal(QCursor::pos());

    const FlexKit::uint2 XY{ (uint32_t)qPos.x(), (uint32_t)qPos.y() };
    const FlexKit::uint2 screenWH = renderWindow->WH();

    const FlexKit::float2 UV{ XY[0] / float(screenWH[0]), XY[1] / float(screenWH[1]) };
    const FlexKit::float2 ScreenCoord{ FlexKit::float2{ 2, -2 } * UV + FlexKit::float2{ -1.0f, 1.0f } };

    const auto cameraConstants      = FlexKit::GetCameraConstants(viewportCamera);
    const auto cameraOrientation    = FlexKit::GetOrientation(FlexKit::GetCameraNode(viewportCamera));

    const FlexKit::float3 v_dir = cameraOrientation * (Inverse(cameraConstants.Proj) * FlexKit::float4{ ScreenCoord.x, ScreenCoord.y,  1.0f, 1.0f }).xyz().normal();
    const FlexKit::float3 v_o   = cameraConstants.WPOS.xyz();

    return { .D = v_dir, .O = v_o };
}


/************************************************************************************************/


void EditorViewport::keyPressEvent(QKeyEvent* evt)
{
    if (!scene)
        return;

    switch (evt->key())
    {
    case Qt::Key_Escape:
    {
        if(mode.size())
            mode.pop_back();
    }   break;
    case Qt::Key_Q:
    {
        if ((mode.size() && mode.back()->GetModeID() == VewportSelectionModeID))
            mode.pop_back();
        else if(mode.size() == 0 || !(mode.size() && mode.back()->GetModeID() != VewportSelectionModeID))
            mode.emplace_back(std::static_pointer_cast<IEditorViewportMode>(
                std::make_shared<EditorVewportSelectionMode>(selectionContext, scene, renderWindow, viewportCamera)));
    }   break;
    case Qt::Key_W:
    case Qt::Key_E:
    case Qt::Key_R:
    {
        if (mode.size() != 0 && !(mode.back()->GetModeID() == VewportSelectionModeID || mode.back()->GetModeID() == VewportPanModeID))
            goto A;
        if ((mode.size() && mode.back()->GetModeID() == TranslationModeID))
            mode.back()->keyPressEvent(evt);
        else if (mode.size() == 0 || !(mode.size() && mode.back()->GetModeID() != TranslationModeID))
            mode.emplace_back(std::static_pointer_cast<IEditorViewportMode>(
                std::make_shared<EditorVewportTranslationMode>(selectionContext, renderWindow, viewportCamera, hud)));

    }   break;
    default:
    {
        A:
        if (mode.size())
            mode.back()->keyPressEvent(evt);
    }   break;
    }
}


/************************************************************************************************/


void EditorViewport::keyReleaseEvent(QKeyEvent* evt)
{
    if (!scene)
        return;

    switch (evt->key())
    {
    default:
        if (mode.size())
            mode.back()->keyReleaseEvent(evt);
    }
}


/************************************************************************************************/


void EditorViewport::mousePressEvent(QMouseEvent* evt)
{
    if (!scene)
        return;

    if (mode.size())
        mode.back()->mousePressEvent(evt);
}


/************************************************************************************************/


void EditorViewport::mouseMoveEvent(QMouseEvent* evt)
{
    if (!scene)
        return;

    if (mode.size())
        mode.back()->mouseMoveEvent(evt);
}


/************************************************************************************************/


void EditorViewport::wheelEvent(QWheelEvent* evt)
{
    if (!scene)
        return;

    if (mode.size())
        mode.back()->wheelEvent(evt);
}


/************************************************************************************************/


void EditorViewport::mouseReleaseEvent(QMouseEvent* evt)
{
    if (!scene)
        return;

    if (mode.size())
        mode.back()->mouseReleaseEvent(evt);
}


/************************************************************************************************/


void EditorViewport::SaveScene()
{
    GetScene()->Update();
}


/************************************************************************************************/


void EditorViewport::Render(FlexKit::UpdateDispatcher& dispatcher, double dT, TemporaryBuffers& temporaries, FlexKit::FrameGraph& frameGraph, FlexKit::ResourceHandle renderTarget, FlexKit::ThreadSafeAllocator& allocator)
{
    const auto modifers = QApplication::queryKeyboardModifiers();
    if(mode.size() && mode.back()->GetModeID() == VewportPanModeID && !modifers.testFlag(Qt::KeyboardModifier::AltModifier))
        mode.pop_back();
    else if (modifers.testFlag(Qt::KeyboardModifier::AltModifier) &&
            !(mode.size() && mode.back()->GetModeID() == VewportPanModeID))
    {
        mode.emplace_back(std::static_pointer_cast<IEditorViewportMode>(
            std::make_shared<EditorVewportPanMode>(selectionContext, scene, renderWindow, viewportCamera, mode.size() ? mode.back() : nullptr)));
    }
    const auto HW           = frameGraph.GetRenderSystem().GetTextureWH(renderTarget);
    QPoint globalCursorPos  = QCursor::pos();
    auto localPosition      = renderWindow->mapFromGlobal(globalCursorPos);

    hud.Update({ (float)localPosition.x() * 1.5f, (float)localPosition.y() * 1.5f }, HW, dispatcher, dT);

    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    if (mode.size())
        mode.back()->DrawImguI();

    ImGui::EndFrame();
    ImGui::Render();

    if (scene)
    {
        allocator.clear();

        auto& physXUpdate       = renderer.UpdatePhysx(dispatcher, dT);
        auto& transforms        = QueueTransformUpdateTask(dispatcher);
        auto& cameras           = FlexKit::CameraComponent::GetComponent().QueueCameraUpdate(dispatcher);
        auto& cameraConstants   = FlexKit::MakeHeapCopy(FlexKit::Camera::ConstantBuffer{}, allocator);

        transforms.AddInput(physXUpdate);
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
            .BVHVisMode     = FlexKit::BVHVisMode::BoundingVolumes,
            .debugDrawMode  = FlexKit::ClusterDebugDrawMode::Clusters,

            .transformDependency    = transforms,
            .cameraDependency       = cameras,

            .additionalGbufferPasses    = {},
            .additionalShadowPasses     = {}
        };

        renderer.csgRender.Render(
            dispatcher, frameGraph,
            temporaries.ReserveConstantBuffer,
            temporaries.ReserveVertexBuffer,
            frameGraph.Resources.AddResource(targets.RenderTarget, true),
            dT);

        auto drawSceneRes = renderer.worldRender.DrawScene(dispatcher, frameGraph, sceneDesc, targets, FlexKit::SystemAllocator, allocator);

        EditorViewport::DrawSceneOverlay_Desc desc{
            .brushes        = drawSceneRes.passes.GetData().solid,
            .lights         = drawSceneRes.pointLights,

            .buffers        = temporaries,
            .renderTarget   = renderTarget,
            .allocator      = allocator
        };

        DrawSceneOverlay(dispatcher, frameGraph, desc);

        if (mode.size())
            mode.back()->Draw(dispatcher, frameGraph, temporaries, renderTarget, depthBuffer.Get());

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
            ctx.BeginEvent_DEBUG("Editor HUD");

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

            auto selection = selectionContext.GetSelectionType() == ViewportObjectList_ID ? selectionContext.GetSelection<ViewportSelection>() : ViewportSelection{};

            ctx.SetGraphicsConstantBufferView(1, cameraConstants);

            // Draw Point Lights
            for (auto& lightHandle : pointLights)
            {
                bool selectedLight = false;
                for (auto& viewportObject : selection.viewportObjects)
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

                const float3 position   = FlexKit::GetPositionW(pointLightComponnet[lightHandle].Position);
                const float radius      = pointLightComponnet[lightHandle].R;

                const size_t divisions  = 64;
                FlexKit::VBPushBuffer VBBuffer   = data.ReserveVertexBuffer(sizeof(Vertex) * 6 * divisions);

			    const float Step = 2.0f * (float)FlexKit::pi / divisions;
                const auto range = FlexKit::MakeRange(0, divisions);
                const float4 color = selectedLight ? float4{ 1, 1, 0, 1 } : float4{ 1, 1, 1, 1 };

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

                ctx.SetGraphicsConstantBufferView(2, constants);

                ctx.Draw(divisions * 6);
            }


            auto& physX = FlexKit::PhysXComponent::GetComponent();
            auto& layer = physX.GetLayer_ref(scene->GetLayer());

            if(layer.debugGeometry.size())
            {
                ctx.BeginEvent_DEBUG("PhysX Debug");
                FlexKit::VBPushBuffer VBBuffer = data.ReserveVertexBuffer(sizeof(FlexKit::PhysicsLayer::DebugVertex) * layer.debugGeometry.size());
                const FlexKit::VertexBufferDataSet vertexBuffer{ layer.debugGeometry, VBBuffer };

                ctx.SetVertexBuffers({ vertexBuffer });
                ctx.Draw(layer.debugGeometry.size());
                ctx.EndEvent_DEBUG();
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

                FlexKit::VBPushBuffer VBBuffer = data.ReserveVertexBuffer(sizeof(Vertex) * 24 * selection.viewportObjects.size());

                for (auto& object : selection.viewportObjects)
                {
                    if (!object->gameObject.hasView(FlexKit::BrushComponentID))
                        continue;


                    const auto      node        = FlexKit::GetSceneNode(object->gameObject);
                    const auto      BS          = FlexKit::GetBoundingSphereFromMesh(object->gameObject);
                    const auto      Q           = FlexKit::GetOrientation(node);
                    const float4    position    = float4(BS.xyz(), 0);
                    const auto      radius      = BS.w;

                    Vertex vertices[] = {
                        // Top
                        { float4{ -radius,  radius,  radius, 1 } + position, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
                        { float4{  radius,  radius,  radius, 1 } + position, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

                        { float4{ -radius,  radius, -radius, 1 } + position, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
                        { float4{  radius,  radius, -radius, 1 } + position, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

                        { float4{ -radius,  radius,  radius, 1 } + position, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
                        { float4{ -radius,  radius, -radius, 1 } + position, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },


                        { float4{  radius,  radius,  radius, 1 } + position, float4{  1, 1, 1, 1 }, float2{ 0, 0 } },
                        { float4{  radius,  radius, -radius, 1 } + position, float4{  1, 1, 1, 1 }, float2{ 0, 0 } },

                        // Bottom
                        { float4{ -radius, -radius,  radius, 1 } + position, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
                        { float4{  radius, -radius,  radius, 1 } + position, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

                        { float4{ -radius, -radius, -radius, 1 } + position, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
                        { float4{  radius, -radius, -radius, 1 } + position, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

                        { float4{ -radius, -radius,  radius, 1 } + position, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
                        { float4{ -radius, -radius, -radius, 1 } + position, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

                        { float4{  radius, -radius,  radius, 1 } + position, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
                        { float4{  radius, -radius, -radius, 1 } + position, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

                        // Sides
                        { float4{  radius,  radius,  radius, 1 } + position, float4{ 1, 1,  1, 1 }, float2{ 0, 0 } },
                        { float4{  radius, -radius,  radius, 1 } + position, float4{ 1, 1,  1, 1 }, float2{ 0, 0 } },

                        { float4{  radius,  radius, -radius, 1 } + position, float4{ 1, 1,  1, 1 }, float2{ 0, 0 } },
                        { float4{  radius, -radius, -radius, 1 } + position, float4{ 1, 1,  1, 1 }, float2{ 0, 0 } },


                        { float4{ -radius,  radius,  radius, 1 } + position, float4{ 1, 1,  1, 1 }, float2{ 0, 0 } },
                        { float4{ -radius, -radius,  radius, 1 } + position, float4{ 1, 1,  1, 1 }, float2{ 0, 0 } },


                        { float4{ -radius,  radius, -radius, 1 } + position, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
                        { float4{ -radius, -radius, -radius, 1 } + position, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
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
                        .transform  = GetWT(node).Transpose()
			        };

                    auto constantBuffer = data.ReserveConstantBuffer(256);
                    const FlexKit::ConstantBufferDataSet constants{ CB_Data, constantBuffer };

                    ctx.SetGraphicsConstantBufferView(2, constants);

                    ctx.Draw(24);
                }
            }

            ctx.EndEvent_DEBUG();
        });
}


/************************************************************************************************/


std::shared_ptr<FlexKit::TriMesh> EditorViewport::BuildTriMesh(FlexKit::MeshUtilityFunctions::OptimizedMesh& mesh) const
{
    return {};
}


/**********************************************************************

Copyright (c) 2021 - 2022 Robert May

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
