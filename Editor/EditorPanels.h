#ifndef EDITORPANELS_H // include guards
#define EDITORPANELS_H

#include "MathUtils.h"
#include "memoryutilities.h"

#include <optional>

class FlexKit::EngineCore;
class FlexKit::UpdateDispatcher;
class FlexKit::FrameGraph;
class FlexKit::GraphicScene;
class FlexKit::GraphicScene;

class EditorBase;

/************************************************************************************************/


struct DrawUIResouces
{
    FlexKit::ResourceHandle                 renderTarget;
    FlexKit::ReserveVertexBufferFunction    reserveVBSpace;
    FlexKit::ReserveConstantBufferFunction  reserveCBSpace;
    FlexKit::iAllocator*                    allocator; // temp memory
};


struct LayoutContext
{
    FlexKit::float2     XY;
    FlexKit::float2     Size;
    FlexKit::float2     pixelSize;
    FlexKit::uint2      windowWH;
};


/************************************************************************************************/


class iEditorPanel
{
public:
    virtual ~iEditorPanel() {}

    virtual void Draw(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT, FlexKit::FrameGraph&, const LayoutContext&, DrawUIResouces&) = 0;
};


/************************************************************************************************/


class Viewer3D_Panel : public iEditorPanel
{
public:
    Viewer3D_Panel(FlexKit::GameFramework&, FlexKit::TextureStreamingEngine&);
    ~Viewer3D_Panel();

    void Draw(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT, FlexKit::FrameGraph&, const LayoutContext&, DrawUIResouces&) final override;
    void Resize(FlexKit::uint2 WH);

    float T = 0;

    FlexKit::ResourceHandle irradianceMap   = FlexKit::InvalidHandle_t;
    FlexKit::ResourceHandle GGXMap          = FlexKit::InvalidHandle_t;

    FlexKit::GraphicScene   scene;
    FlexKit::CameraHandle   camera;

    FlexKit::WorldRender	render;
    FlexKit::GBuffer        gbuffer;
    FlexKit::ResourceHandle	depthBuffer;

    FlexKit::RenderSystem&  renderSystem;
};


/************************************************************************************************/


class Property_Panel : public iEditorPanel
{
public:
    Property_Panel(FlexKit::GameFramework&);
    ~Property_Panel();

    void Draw(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT, FlexKit::FrameGraph&, const LayoutContext&, DrawUIResouces&) final override;

    float x         = 0;
    char  Text[256] = "Hello";
};


class Menu_Panel : public iEditorPanel
{
public:
    Menu_Panel(FlexKit::GameFramework&, EditorBase&);
    ~Menu_Panel();

    void Draw(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT, FlexKit::FrameGraph&, const LayoutContext&, DrawUIResouces&) final override;


    EditorBase& editor;
};

/************************************************************************************************/


class UIGrid
{
public:
    UIGrid(const FlexKit::uint2 rowColumns, FlexKit::iAllocator* allocator);

    bool AddPanel   (FlexKit::uint2 XY, FlexKit::uint2 WH, iEditorPanel& panel);
    void Draw       (FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT, FlexKit::FrameGraph&, DrawUIResouces& resources);

    FlexKit::float2 GetSize(FlexKit::uint2 XY, FlexKit::uint2 WH) const;

    float GetPanelWidth    (const FlexKit::uint2 XY, const FlexKit::uint2 WH) const;
    float GetPanelHeight   (const FlexKit::uint2 XY, const FlexKit::uint2 WH) const;

    struct panel
    {
        FlexKit::uint2  cellID;
        FlexKit::uint2  wh;
        iEditorPanel*   panel;
    };

    bool                            _CheckGridOccupancy(FlexKit::uint2 XY, FlexKit::uint2 WH);
    std::optional<iEditorPanel*>    FindPanel(FlexKit::uint2 XY);


    FlexKit::Vector<float> columnWidths;   // must sum to 1!
    FlexKit::Vector<float> rowHeights;     // must sum to 1!
    FlexKit::Vector<panel> panels;
};


/************************************************************************************************/


#endif EDITORPANELS_H
