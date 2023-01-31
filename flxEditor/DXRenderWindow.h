#pragma once

#include <QtWidgets/qwidget>
#include "Application.h"
#include "Win32Graphics.h"
#include <functional>

class FlexKit::UpdateDispatcher;
class FlexKit::FrameGraph;

struct TemporaryBuffers
{
	FlexKit::ReserveVertexBufferFunction    ReserveVertexBuffer;
	FlexKit::ReserveConstantBufferFunction  ReserveConstantBuffer;
};

using FNRender_t = std::function<void (FlexKit::UpdateDispatcher& Dispatcher, double dT, TemporaryBuffers&, FlexKit::FrameGraph& graph, FlexKit::ResourceHandle renderTarget, FlexKit::ThreadSafeAllocator& threadSafeAllocator)>;
using FNResize_t = std::function<void (FlexKit::uint2 newSize)>;

class DXRenderWindow : public QWidget
{
	Q_OBJECT

public:
	DXRenderWindow(FlexKit::RenderSystem& renderSystem, QWidget *parent = Q_NULLPTR);
	~DXRenderWindow();

	void Release();

	void Draw(FlexKit::EngineCore& Engine, TemporaryBuffers& temporaries, FlexKit::UpdateDispatcher& Dispatcher, double dT, FlexKit::FrameGraph& graph, FlexKit::ThreadSafeAllocator& threadSafeAllocator);
	void Present();

	bool isValid() { return renderWindow.isValid(); }
	std::function<void (DXRenderWindow* renderWindow)> ResizeEventHandler;

	void SetOnDraw(FNRender_t draw)
	{
		onDraw = draw;
	}

	void SetOnResize(FNResize_t resize)
	{
		onResize = resize;
	}

	FlexKit::uint2 WH() const { return renderWindow.GetWH(); }

	void            resizeEvent(QResizeEvent* evt);

	FlexKit::ResourceHandle GetBackBuffer() const;

public slots:
	void OnFrame();


private:
	void            paintEvent(QPaintEvent* event) override;
	QPaintEngine*   paintEngine() const override;
	void            showEvent(QShowEvent* event) override;

	void            resizeSwapChain(int width, int height);

	bool            dirty = false;

	double          t = 0.0f;

	FNRender_t                  onDraw;
	FNResize_t                  onResize;

	FlexKit::Win32RenderWindow  renderWindow;
	FlexKit::CameraHandle       camera  = FlexKit::InvalidHandle;
};


/**********************************************************************

Copyright (c) 2021-2022 Robert May

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
