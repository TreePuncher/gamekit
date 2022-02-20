#include "PCH.h"
#include "DXRenderWindow.h"

#include <fmt/format.h>
#include <qevent.h>


/************************************************************************************************/


DXRenderWindow::DXRenderWindow(FlexKit::RenderSystem& renderSystem, QWidget *parent) :
    QWidget         { parent },
    renderWindow    { FlexKit::CreateWin32RenderWindowFromHWND(renderSystem, (HWND)winId()).first }
{
    hide();

    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setAutoFillBackground(true);
    setPalette(pal);

    setMinimumSize(100, 100);

    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setContentsMargins(0, 0, 0, 0);

    adjustSize();

    show();
}


/************************************************************************************************/


DXRenderWindow::~DXRenderWindow()
{
    Release();
}


/************************************************************************************************/


void DXRenderWindow::Release()
{
    renderWindow.Release();
}


/************************************************************************************************/


void DXRenderWindow::Draw(FlexKit::EngineCore& Engine, TemporaryBuffers& temporaries, FlexKit::UpdateDispatcher& Dispatcher, double dT, FlexKit::FrameGraph& frameGraph, FlexKit::ThreadSafeAllocator& threadSafeAllocator)
{
    t += dT;

    if(t >= 1.0f / 30.0f)
    {
        dirty = true;

        if (onDraw)
        {
            onDraw(Dispatcher, t, temporaries, frameGraph, renderWindow.GetBackBuffer(), threadSafeAllocator);
        }
        else
        {
            frameGraph.AddRenderTarget(renderWindow.backBuffer);

            FlexKit::ClearBackBuffer(frameGraph, renderWindow.GetBackBuffer(), FlexKit::float4{ 0.0f, 0.0f, 0.0f, 1 });
            FlexKit::PresentBackBuffer(frameGraph, renderWindow.GetBackBuffer());
        }

        t = 0.0f;
    }
}


/************************************************************************************************/


void DXRenderWindow::Present()
{
    if (dirty)
    {
        dirty = false;
        renderWindow.Present(1);
    }
}


/************************************************************************************************/


void DXRenderWindow::paintEvent(QPaintEvent* event)
{
}


/************************************************************************************************/


QPaintEngine* DXRenderWindow::paintEngine() const
{
    return nullptr;
}


/************************************************************************************************/


void DXRenderWindow::showEvent(QShowEvent* event)
{
    auto string = fmt::format("showing window, winID : {}\n", winId());
    OutputDebugStringA(string.c_str());
}


/************************************************************************************************/


void DXRenderWindow::resizeSwapChain(int width, int height)
{
    renderWindow.Resize(FlexKit::uint2{ (size_t)width, (size_t)height });
}


/************************************************************************************************/


void DXRenderWindow::resizeEvent(QResizeEvent* evt)
{
    QWidget::resizeEvent(evt);

    const auto widgetSize = size();
    const auto width      = widgetSize.width() / 2;
    const auto height     = widgetSize.height() / 2;

    resizeSwapChain(evt->size().width() / 2, evt->size().height() / 2);

    if (ResizeEventHandler)
        ResizeEventHandler(this);

    if (onResize)
        onResize({ (uint32_t)width, (uint32_t)height });
}


/************************************************************************************************/


void DXRenderWindow::OnFrame()
{
    auto string = fmt::format("winID : {}\n", winId());
    OutputDebugStringA(string.c_str());

    //Present();
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
