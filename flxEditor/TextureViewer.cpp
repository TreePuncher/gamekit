#include "TextureViewer.h"
#include "EditorRenderer.h"
#include <QtWidgets/qboxlayout.h>
#include <qevent.h>

/************************************************************************************************/



TextureViewer::TextureViewer(EditorRenderer& IN_renderer, QWidget *parent, FlexKit::ResourceHandle IN_resource) :
    QWidget         { parent },
    menuBar         { new QMenuBar{ this } },
    renderer        { IN_renderer },
    texture         { IN_resource }
{
	ui.setupUi(this);

    auto layout = findChild<QBoxLayout*>("verticalLayout");
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setMenuBar(menuBar);

    setMinimumSize(200, 200);
    setMaximumSize({ 1024 * 16, 1024 * 16 });

    auto file   = menuBar->addMenu("file");
    auto select = file->addAction("Select Texture");

    menuBar->show();
    
    renderWindow = renderer.CreateRenderWindow(this);
    renderWindow->SetOnDraw(
        [&](FlexKit::UpdateDispatcher& Dispatcher, double dT, TemporaryBuffers& temporary, FlexKit::FrameGraph& frameGraph, FlexKit::ResourceHandle renderTarget)
        {
            if (texture == FlexKit::InvalidHandle_t)
                return;

            frameGraph.AddRenderTarget(renderTarget);

            struct DrawTexture
            {
                FlexKit::FrameResourceHandle                renderTarget;
                FlexKit::ReserveConstantBufferFunction      ReserveConstantBuffer;
                FlexKit::ReserveVertexBufferFunction        ReserveVertexBuffer;
            };

            auto& draw = frameGraph.AddNode<DrawTexture>(
                DrawTexture{
                    FlexKit::InvalidHandle_t,
                    temporary.ReserveConstantBuffer,
                    temporary.ReserveVertexBuffer },
                [&](FlexKit::FrameGraphNodeBuilder& Builder, DrawTexture& data)
                {
                    data.renderTarget = Builder.WriteRenderTarget(renderTarget);
                },
                [=](DrawTexture& data, const FlexKit::ResourceHandler& frameResources, FlexKit::Context& context, FlexKit::iAllocator& allocator)
                {
                    FlexKit::float4 Color   { FlexKit::WHITE };
                    FlexKit::float2 POS     { 0, 0 };
                    FlexKit::float2 WH      { 1, 1 };

                    FlexKit::float2 RectUpperLeft   = POS;
                    FlexKit::float2 RectBottomRight = POS + WH;
                    FlexKit::float2 RectUpperRight  = { RectBottomRight.x,	RectUpperLeft.y };
                    FlexKit::float2 RectBottomLeft  = { RectUpperLeft.x,	RectBottomRight.y };

                    struct Vertex
                    {
                        FlexKit::float4 POS;
                    };

                    const FlexKit::ShapeVert verticeData[] = {
                        FlexKit::ShapeVert{ Position2SS(RectUpperLeft),	    { 0.0f, 1.0f }, Color },
                        FlexKit::ShapeVert{ Position2SS(RectBottomRight),   { 1.0f, 0.0f }, Color },
                        FlexKit::ShapeVert{ Position2SS(RectBottomLeft),	{ 0.0f, 0.0f }, Color },

                        FlexKit::ShapeVert{ Position2SS(RectUpperLeft),	    { 0.0f, 1.0f }, Color },
                        FlexKit::ShapeVert{ Position2SS(RectUpperRight),	{ 1.0f, 1.0f }, Color },
                        FlexKit::ShapeVert{ Position2SS(RectBottomRight),   { 1.0f, 0.0f }, Color } };

                    struct
                    {
                        FlexKit::float4 Color;
                        FlexKit::float4 Specular;
                    } constants;

                    FlexKit::VBPushBuffer           vertexBuffer = data.ReserveVertexBuffer(1024);
                    FlexKit::VertexBufferDataSet    vertexBufferSet{verticeData, sizeof(verticeData), vertexBuffer };

                    FlexKit::CBPushBuffer           constantBuffer = data.ReserveConstantBuffer(1024);
                    FlexKit::ConstantBufferDataSet  constantBufferSet{ constants, constantBuffer };

                    context.SetScissorAndViewports({ frameResources.GetRenderTarget(data.renderTarget) });
                    context.SetRenderTargets(
                        { frameResources.GetRenderTarget(data.renderTarget) },
                        false);

                    context.SetRootSignature(frameResources.renderSystem().Library.RS6CBVs4SRVs);
                    context.SetPipelineState(frameResources.GetPipelineState(FlexKit::DRAW_TEXTURED_PSO));
                    context.SetPrimitiveTopology(FlexKit::EInputTopology::EIT_TRIANGLE);

                    FlexKit::DescriptorHeap descHeap;
                    auto& desciptorTableLayout = frameResources.renderSystem().Library.RS6CBVs4SRVs.GetDescHeap(0);

                    descHeap.Init2(context, desciptorTableLayout, 1, &allocator);
                    descHeap.NullFill(context, 1);
                    descHeap.SetSRV(context, 0, texture);

                    context.SetGraphicsDescriptorTable(0, descHeap);
                    context.SetVertexBuffers({ vertexBufferSet });

                    context.SetGraphicsConstantBufferView(2, constantBufferSet);
                    context.Draw(6, 0);
                });

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

    auto layout = findChild<QBoxLayout*>("verticalLayout");
}


/************************************************************************************************/


void Render(FlexKit::FrameGraph& frameGraph)
{

}


/************************************************************************************************/


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
