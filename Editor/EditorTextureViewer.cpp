#include "PCH.h"
#include "EditorTextureViewer.h"
#include "EditorRenderer.h"
#include <RenderSystemInterface.hpp>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qtextedit.h>
#include <qevent.h>


using namespace FlexKit;


/************************************************************************************************/


TextureViewer::TextureViewer(EditorRenderer& IN_renderer, QWidget *parent, FlexKit::ResourceHandle IN_resource) :
	QWidget         { parent },
	menuBar         { new QMenuBar{ this } },
	renderer        { IN_renderer },
	texture         { IN_resource }
{
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);

	auto layout = findChild<QBoxLayout*>("verticalLayout");
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setMenuBar(menuBar);

	setMinimumSize(100, 100);
	setMaximumSize({ 1024 * 16, 1024 * 16 });

	auto file   = menuBar->addMenu("file");
	auto select = file->addAction("Select Texture");

	menuBar->show();

	renderWindow = renderer.CreateRenderWindow();
	renderWindow->SetOnDraw(
		[&](UpdateDispatcher& Dispatcher, double dT, FrameGraph& frameGraph, ResourceHandle renderTarget, ThreadSafeAllocator& allocator)
		{
			ClearBackBuffer(frameGraph, renderTarget, { 1, 0, 1, 1 });

			if (texture == FlexKit::InvalidHandle)
				return;


			struct DrawTexture
			{
				FrameResourceHandle                renderTarget;
			};


			auto& draw = frameGraph.AddNode<DrawTexture>(
				DrawTexture{ FlexKit::InvalidHandle },
				[&](FrameGraphNodeBuilder& Builder, DrawTexture& data)
				{
					data.renderTarget = Builder.RenderTarget(renderTarget);
				},
				[=](DrawTexture& data, const ResourceHandler& frameResources, IDirectContext& context, iAllocator& allocator)
				{
					auto XY = frameResources.GetTextureWH(data.renderTarget);
					float aspectRatio = (XY[0]) / float(XY[1]);

					float4 Color   { FlexKit::WHITE };
					float2 WH      = (aspectRatio < 1.0f ) ? float2{ 1.0,	aspectRatio } : FlexKit::float2{ 1.0f / aspectRatio, 1.0f };
					float2 POS     = (aspectRatio < 1.0f) ? float2{ 0,	(1 - WH[1]) / 2.0f} : float2{ (1 - WH[0])/ 2.0f, 0.0f };

					fmt::print("aspectration: {}\n", aspectRatio);

					float2 RectUpperLeft   = POS;
					float2 RectBottomRight = POS + WH;
					float2 RectUpperRight  = { RectBottomRight.x,	RectUpperLeft.y };
					float2 RectBottomLeft  = { RectUpperLeft.x,	RectBottomRight.y };

					struct Vertex
					{
						float4 POS;
					};

					const ShapeVert verticeData[] = {
						ShapeVert{ Position2SS(RectUpperLeft),	    { 0.0f, 0.0f }, Color },
						ShapeVert{ Position2SS(RectBottomRight),	{ 1.0f, 1.0f }, Color },
						ShapeVert{ Position2SS(RectBottomLeft),		{ 0.0f, 1.0f }, Color },

						ShapeVert{ Position2SS(RectUpperLeft),	    { 0.0f, 0.0f }, Color },
						ShapeVert{ Position2SS(RectUpperRight),		{ 1.0f, 0.0f }, Color },
						ShapeVert{ Position2SS(RectBottomRight),	{ 1.0f, 1.0f }, Color } };

					struct
					{
						float4 Color;
						float4 Specular;
					} constants;

					VBPushBuffer           vertexBuffer = frameResources.ReserveVB(1024);
					VertexBufferDataSet    vertexBufferSet{verticeData, sizeof(verticeData), vertexBuffer };

					CBPushBuffer           constantBuffer = frameResources.ReserveCB(1024);
					ConstantBufferDataSet  constantBufferSet{ constants, constantBuffer };

					context.ClearRenderTarget(frameResources.GetResource(data.renderTarget));

					context.SetScissorAndViewports({ frameResources.GetResource(data.renderTarget) });
					context.SetRenderTargets(
						{ frameResources.GetResource(data.renderTarget) },
						false);

					static const auto drawTexturedState = frameResources.GetPipelineState(FlexKit::DRAW_TEXTURED_PSO, allocator);
					context.SetPipelineState(drawTexturedState);
					context.SetInputPrimitive(FlexKit::INPUTPRIMITIVETRIANGLELIST);

					DescriptorSet descHeap;
					auto& desciptorTableLayout = drawTexturedState->GetDescriptorSetLayout(0);

					descHeap.Init2(context, desciptorTableLayout, 1, allocator);
					descHeap.NullFill(context, 1);
					descHeap.SetSRV(context, 0, texture);

					context.SetGraphicsDescriptorSet(0, descHeap);
					context.SetVertexBuffers({ vertexBufferSet });

					context.SetGraphicsConstantBufferView(2, constantBufferSet);
					context.Draw(6, 0);
				});

			FlexKit::PresentBackBuffer(frameGraph, renderTarget);
		});


	layout->addWidget(renderWindow);

	show();
}


/************************************************************************************************/


TextureViewer::~TextureViewer()
{
	renderWindow->Release();
}


/************************************************************************************************/


void TextureViewer::closeEvent(QCloseEvent* event)
{
	renderWindow->Release();
}


/************************************************************************************************/


void TextureViewer::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);

	renderWindow->resize(event->size());
}


/************************************************************************************************/


void TextureViewer::Render(FrameGraph& frameGraph)
{

}


/************************************************************************************************/


/**********************************************************************

Copyright (c) 2019-2026 Robert May

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
