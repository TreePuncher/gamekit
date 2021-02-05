#pragma once
#include <GameFramework.h>
#include <QtWidgets/qapplication>
#include "DXRenderWindow.h"
#include "TextureStreamingUtilities.h"
#include "WorldRender.h"

class QWidget;


class EditorRenderer : public FlexKit::FrameworkState
{
public:
    EditorRenderer(FlexKit::GameFramework& IN_framework, FlexKit::FKApplication& IN_application, QApplication& IN_QtApplication) :
        FrameworkState  { IN_framework      },
        QtApplication   { IN_QtApplication  },
        application     { IN_application    },
        vertexBuffer    { IN_framework.core.RenderSystem.CreateVertexBuffer(MEGABYTE * 1, false)        },
        constantBuffer  { IN_framework.core.RenderSystem.CreateConstantBuffer(MEGABYTE * 128, false)    },
        textureEngine   { IN_framework.core.RenderSystem, IN_framework.core.GetBlockMemory() },
        worldRender     { IN_framework.core.RenderSystem, textureEngine, IN_framework.core.GetBlockMemory() }
    {
        auto& renderSystem = framework.GetRenderSystem();
        renderSystem.RegisterPSOLoader(FlexKit::DRAW_TEXTURED_PSO, { &renderSystem.Library.RS6CBVs4SRVs, FlexKit::CreateTexturedTriStatePSO });
    }

    ~EditorRenderer()
    {
        auto& renderSystem = framework.GetRenderSystem();

        renderSystem.ReleaseCB(constantBuffer);
        renderSystem.ReleaseVB(vertexBuffer);
    }

    void DrawOneFrame()
    {
        application.DrawOneFrame(0.0);
    }

    DXRenderWindow* CreateRenderWindow(QWidget* parent = nullptr)
    {
        auto viewPortWidget = new DXRenderWindow{ application.GetFramework().GetRenderSystem(), parent };
        renderWindows.push_back(viewPortWidget);

        return viewPortWidget;
    }

    void DrawRenderWindow(DXRenderWindow* renderWindow)
    {
        if (drawInProgress)
            return;

        return; // Causes crash for now
    }

protected:
    void Update(FlexKit::EngineCore& Engine, FlexKit::UpdateDispatcher& Dispatcher, double dT) override
    {
        renderWindows.erase(std::remove_if(std::begin(renderWindows), std::end(renderWindows),
            [](auto& I)
            {
                return !I->isValid();
            }),
            std::end(renderWindows));
    }


    void Draw(FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT, FlexKit::FrameGraph& frameGraph) override
    {
        drawInProgress = true;

        FlexKit::ClearVertexBuffer(frameGraph, vertexBuffer);

        TemporaryBuffers temporaries{
            FlexKit::CreateVertexBufferReserveObject(vertexBuffer, core.RenderSystem, core.GetTempMemory()),
            FlexKit::CreateConstantBufferReserveObject(constantBuffer, core.RenderSystem, core.GetTempMemory())
        };

        for (auto renderWindow : renderWindows)
            renderWindow->Draw(core, temporaries, dispatcher, dT, frameGraph);
    }


    void PostDrawUpdate(FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT) override
    {
        for (auto renderWindow : renderWindows)
            renderWindow->Present();

        drawInProgress = false;
    }

    std::atomic_bool                drawInProgress = false;


public:
    FlexKit::TextureStreamingEngine textureEngine;
    FlexKit::WorldRender            worldRender;

private:
    // Temp Buffers
    FlexKit::VertexBufferHandle		vertexBuffer;
    FlexKit::ConstantBufferHandle	constantBuffer;

    QApplication&                   QtApplication;
    FlexKit::FKApplication&         application;
    std::vector<DXRenderWindow*>    renderWindows;
};


/**********************************************************************

Copyright (c) 2019-2021 Robert May

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
