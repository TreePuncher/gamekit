#pragma once
// IMGUI based render system for quick n dirty in game ui's


#include "FrameGraph.hpp"
#include "Events.hpp"
#include "EngineCore.hpp"

struct ImDrawData;

namespace FlexKit
{
    struct Win32RenderWindow;

    class ImGUIIntegrator
    {
    public:
        ImGUIIntegrator(RenderSystem& renderSystem, iAllocator* memory);
        ~ImGUIIntegrator();

        void Update(uint2 MouseXY, uint2 WH, FlexKit::UpdateDispatcher& dispatcher, double dT);
        void Update(IRenderWindow& window, FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT);
        bool HandleInput(FlexKit::Event evt);

        void DrawImGui(const double dT, FlexKit::UpdateDispatcher&, FlexKit::FrameGraph&, FlexKit::ReserveVertexBufferFunction, FlexKit::ReserveConstantBufferFunction, FlexKit::ResourceHandle renderTarget);

    private:
        ResourceHandle imGuiFont;
    };

}


/**********************************************************************

Copyright (c) 2024 Robert May

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
