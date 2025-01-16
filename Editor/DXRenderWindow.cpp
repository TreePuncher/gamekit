#include "PCH.h"
#include "DXRenderWindow.h"

#include <fmt/format.h>
#include <qevent.h>


/************************************************************************************************/


DXRenderWindow::DXRenderWindow(FlexKit::RenderSystem& renderSystem, QWidget *parent) :
	QWidget         { parent },
	renderWindow    { FlexKit::CreateWin32RenderWindowFromHWND(renderSystem, (HWND)winId()) }
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
	setContentsMargins(1, 1, 1, 1);

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
	renderWindow->Release();
}


/************************************************************************************************/


void DXRenderWindow::Draw(FlexKit::EngineCore& Engine, TemporaryBuffers& temporaries, FlexKit::UpdateDispatcher& Dispatcher, double dT, FlexKit::FrameGraph& frameGraph, FlexKit::ThreadSafeAllocator& threadSafeAllocator)
{
	if (resizeFinished)
	{
		if (ResizeEventHandler)
			ResizeEventHandler(this);

		if (onResize)
			onResize(newWidthHeight);

		resizeFinished		= false;
	}

	t += dT;

	if (!resizeInProgress && isVisible())
	{
		dirty = true;

		if (onDraw)
		{
			onDraw(Dispatcher, dT, temporaries, frameGraph, renderWindow->GetBackBuffer(), threadSafeAllocator);
		}
		else
		{
			frameGraph.AddResource(renderWindow->backBuffer);

			FlexKit::ClearBackBuffer(frameGraph, renderWindow->GetBackBuffer(), FlexKit::float4{ 0.0f, 0.0f, 0.0f, 1 });
			FlexKit::PresentBackBuffer(frameGraph, renderWindow->GetBackBuffer());
		}
	}
}


/************************************************************************************************/


void DXRenderWindow::Present()
{
	if (resizeInProgress)
		return;

	if (dirty)
	{
		dirty = false;
		renderWindow->Present(1);
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
	renderWindow->Resize(FlexKit::uint2{ (size_t)width, (size_t)height });
}


/************************************************************************************************/


void DXRenderWindow::SetOnDraw(FNRender_t draw)
{
	onDraw = draw;
}


/************************************************************************************************/


void DXRenderWindow::SetOnResize(FNResize_t resize)
{
	onResize = resize;
}


/************************************************************************************************/


FlexKit::uint2 DXRenderWindow::WH() const noexcept { return renderWindow->GetWH(); }


/************************************************************************************************/


void DXRenderWindow::resizeEvent(QResizeEvent* evt)
{
	QWidget::resize(evt->size());

	const auto widgetSize = size();
	const auto width      = widgetSize.width();
	const auto height     = widgetSize.height();

	const float dpiScaling	= GetDPIScaling();
	const auto newWidth     = evt->size().width() * dpiScaling;
	const auto newHeight    = evt->size().height() * dpiScaling;

	newWidthHeight		= { newWidth, newHeight };

	if(!resizeInProgress)
	{
		resizeInProgress	= true;

		resizeTask = FlexKit::CreateWorkItem(
			[this](auto&& _)
			{
				while (renderWindow->WH != newWidthHeight.load())
					renderWindow->Resize(newWidthHeight);

				resizeFinished		= true;
				resizeInProgress	= false;
			});

		FlexKit::PushToLocalQueue(*resizeTask);
	}
}


/************************************************************************************************/


void DXRenderWindow::enterEvent(QMouseEvent* event)
{
	renderWindow->PIX_SetActiveWindow();
}


/************************************************************************************************/


void DXRenderWindow::DEBUG_SetActiveWindow()
{
	renderWindow->PIX_SetActiveWindow();
}


/************************************************************************************************/


FlexKit::ResourceHandle DXRenderWindow::GetBackBuffer() const
{
	return renderWindow->GetBackBuffer();
}


/************************************************************************************************/


float DXRenderWindow::GetDPIScaling() const noexcept
{
	return renderWindow->GetDPIScaling();
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
