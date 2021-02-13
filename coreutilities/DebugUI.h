#pragma once
// IMGUI based render system for quick n dirty in game ui's


#include "FrameGraph.h"
#include "Events.h"
#include "EngineCore.h"

struct ImDrawData;

namespace FlexKit
{
    struct Win32RenderWindow;

    class ImGUIIntegrator
    {
    public:
        ImGUIIntegrator(RenderSystem& renderSystem, iAllocator* memory);
        ~ImGUIIntegrator();

        void Update(Win32RenderWindow& window, FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT);
        bool HandleInput(FlexKit::Event evt);

        void DrawImGui(const double dT, FlexKit::UpdateDispatcher&, FlexKit::FrameGraph&, FlexKit::ReserveVertexBufferFunction, FlexKit::ReserveConstantBufferFunction, FlexKit::ResourceHandle renderTarget);

    private:
        ResourceHandle imGuiFont;
    };

}
