#include "EditorPanels.h"
#include "worldRender.h"
#include "GraphicScene.h"
#include "graphics.h"
#include "GraphicsComponents.h"
#include "EditorBase.h"

#include <algorithm>
#include <imgui.h>

using namespace FlexKit;


/************************************************************************************************/


Viewer3D_Panel::Viewer3D_Panel(FlexKit::GameFramework& framework, FlexKit::TextureStreamingEngine& streamingEngine) :
    renderSystem    { framework.core.RenderSystem                                                           },
    scene           { framework.core.GetBlockMemory()                                                       },
    camera          { CameraComponent::GetComponent().CreateCamera()                                        },
    gbuffer         { framework.ActiveWindow->WH, framework.core.RenderSystem                               },
    depthBuffer     { framework.core.RenderSystem.CreateDepthBuffer(framework.ActiveWindow->WH / 3,	true)   },
    render{ framework.core.GetTempMemory(),
            framework.core.RenderSystem,
            streamingEngine,
            framework.ActiveWindow->WH }
{
    framework.core.RenderSystem.RegisterPSOLoader(DRAW_PSO, { &framework.core.RenderSystem.Library.RS6CBVs4SRVs, CreateDrawTriStatePSO });
}


/************************************************************************************************/


Viewer3D_Panel::~Viewer3D_Panel()
{

}


/************************************************************************************************/


void Viewer3D_Panel::Draw(
    EngineCore&             core,
    UpdateDispatcher&       dispatcher,
    double                  dT,
    FrameGraph&             frameGraph,
    const LayoutContext&    layout,
    DrawUIResouces&         resources)
{
    auto& pointLightGather  = scene.GetPointLights(dispatcher, core.GetTempMemory());
    auto& transforms        = QueueTransformUpdateTask(dispatcher);
    auto& cameras           = CameraComponent::GetComponent().QueueCameraUpdate(dispatcher);
    auto& cameraConstants   = MakeHeapCopy(Camera::ConstantBuffer{}, core.GetTempMemory());
    auto& PVS               = GatherScene(dispatcher, scene, camera, core.GetTempMemory());

    PVS.AddInput(cameras);
    PVS.AddInput(transforms);

    const SceneDescription sceneDesc = {
        camera,
        pointLightGather,
        transforms,
        cameras,
        PVS,
    };

    
    DrawShapes(
        DRAW_PSO,
        frameGraph,
        resources.reserveVBSpace,
        resources.reserveCBSpace,
        resources.renderTarget,
        resources.allocator,
        RectangleShape{ layout.XY, layout.Size, float4(0, 1, 0, 1) });

    /*
    ClearDepthBuffer(frameGraph, depthBuffer, 1.0f);

    AddGBufferResource(gbuffer, frameGraph);
    ClearGBuffer(gbuffer, frameGraph);

    render.RenderPBR_GBufferPass(
        dispatcher, frameGraph,
        sceneDesc, camera, PVS,
        gbuffer, depthBuffer,
        core.GetTempMemory());

    render.RenderPBR_IBL_Deferred(
        dispatcher, frameGraph,
        sceneDesc, camera,
        resources.renderTarget,
        depthBuffer,
        irradianceMap,
        GGXMap,
        gbuffer,
        resources.reserveVBSpace,
        T, core.GetTempMemory());
    */
    T += dT;
}


/************************************************************************************************/


void Viewer3D_Panel::Resize(uint2 WH)
{
    gbuffer.Resize(WH);
    renderSystem.ReleaseTexture(depthBuffer);
    depthBuffer = renderSystem.CreateDepthBuffer(WH, true);
}


/************************************************************************************************/


Property_Panel::Property_Panel(FlexKit::GameFramework&)
{
}


/************************************************************************************************/


Property_Panel::~Property_Panel()
{
}


/************************************************************************************************/


void Property_Panel::Draw(
    EngineCore&             core,
    UpdateDispatcher&       dispatcher,
    double                  dT,
    FrameGraph&             frameGraph,
    const LayoutContext&    layout,
    DrawUIResouces&         resources)
{
    const auto XY = layout.XY   * layout.windowWH;
    const auto WH = layout.Size * layout.windowWH;
    
    ImGui::SetNextWindowSize({ float(WH[0]), (float)WH[1] });
    ImGui::SetNextWindowPos({ (float)XY[0], (float)XY[1] });

    if (ImGui::Begin("properties", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
    {
        ImGui::InputText("input text", Text, 256);
        ImGui::SliderFloat("float", &x, 0.0f, 1.0f);
    }

    ImGui::End();
}


/************************************************************************************************/


Menu_Panel::Menu_Panel(FlexKit::GameFramework&, EditorBase& IN_editor) :
    editor{ IN_editor }
{

}


Menu_Panel::~Menu_Panel()
{

}


/************************************************************************************************/


void Menu_Panel::Draw(
    EngineCore&             core,
    UpdateDispatcher&       dispatcher,
    double                  dT,
    FrameGraph&             frameGraph,
    const LayoutContext&    layout,
    DrawUIResouces&         resources)
{
    const auto XY = layout.XY * layout.windowWH;
    const auto WH = layout.Size * layout.windowWH;

    ImGui::SetNextWindowSize({ float(WH[0]), (float)WH[1] });
    ImGui::SetNextWindowPos({ (float)XY[0], (float)XY[1] });

    if(ImGui::BeginMainMenuBar())
    {
        // Adjust rest of window to fix the menu bar
        auto FrameHeight        = ImGui::GetFrameHeight() / float(layout.windowWH[1]);
        auto prevHeight         = editor.UI.rowHeights[0];
        editor.UI.rowHeights[0] = FrameHeight;
        editor.UI.rowHeights[1] += prevHeight - FrameHeight;

        if(ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Load File"))
                FK_LOG_INFO("UnImplemented Menu item called!");

            if (ImGui::MenuItem("Exit"))
                core.End = true;

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}


/************************************************************************************************/


UIGrid::UIGrid(const FlexKit::uint2 rowColumns, FlexKit::iAllocator* allocator) :
    columnWidths    { allocator, rowColumns[1] },
    rowHeights      { allocator, rowColumns[0] },
    panels          { allocator }
{
    for (size_t i = 0; i < rowColumns[0]; i++)
        rowHeights.push_back(1.0f / rowColumns[0]);

    for (size_t i = 0; i < rowColumns[1]; i++)
        columnWidths.push_back(1.0f / rowColumns[1]);
}


/************************************************************************************************/


bool UIGrid::AddPanel(FlexKit::uint2 XY, FlexKit::uint2 WH, iEditorPanel& panel)
{
    if(_CheckGridOccupancy(XY, WH))
        return false;

    panels.push_back({ XY, WH, &panel });

    return true;
}


/************************************************************************************************/


void UIGrid::Draw(
    FlexKit::EngineCore&        core,
    FlexKit::UpdateDispatcher&  dispatcher,
    double                      dT,
    FlexKit::FrameGraph&        frameGraph,
    DrawUIResouces&             resources)
{
    LayoutContext   layout;
    const auto      windowSize  = core.RenderSystem.GetTextureWH(core.Window.backBuffer);
    const float2    pixelSize   = float2{ 1.0f / windowSize[0], 1.0f / windowSize[1] };

    layout.windowWH  = windowSize;
    layout.pixelSize = pixelSize;

    for (auto& panel : panels)
    {
        const auto width    = GetPanelWidth (panel.cellID, panel.wh);
        const auto height   = GetPanelHeight(panel.cellID, panel.wh);

        const auto offsetX  = GetPanelWidth({ 0, 0 },  panel.cellID);
        const auto offsetY  = GetPanelHeight({ 0, 0 }, panel.cellID);

        layout.Size = { width, height };
        layout.XY   = { offsetX, offsetY };

        panel.panel->Draw(core, dispatcher, dT, frameGraph, layout, resources);
    }
}


/************************************************************************************************/


float UIGrid::GetPanelWidth(const uint2 XY, const uint2 WH) const
{
    return std::accumulate(columnWidths.begin() + XY[0], columnWidths.begin() + WH[0] + XY[0], 0.0f);
}


/************************************************************************************************/


float UIGrid::GetPanelHeight(const uint2 XY, const uint2 WH) const
{
    return std::accumulate(rowHeights.begin() + XY[1], rowHeights.begin() + WH[1] + XY[1], 0.0f);
}


/************************************************************************************************/


float2 UIGrid::GetSize(uint2 XY, uint2 WH) const
{

    return {
        GetPanelWidth(XY, WH),
        GetPanelHeight(XY, WH)
    };
}


/************************************************************************************************/


bool UIGrid::_CheckGridOccupancy(FlexKit::uint2 XY, FlexKit::uint2 WH)
{
    return false;
}


/************************************************************************************************/


std::optional<iEditorPanel*> UIGrid::FindPanel(FlexKit::uint2 XY)
{
    for (const auto& panel : panels)
    {
        if (panel.cellID[0] <= XY[0] && panel.cellID[0] + panel.wh[0] - 1 >= XY[0])
            if (panel.cellID[1] <= XY[1] && panel.cellID[1] + panel.wh[1] - 1>= XY[1])
                return panel.panel;
    }

    return {};
}


/************************************************************************************************/
