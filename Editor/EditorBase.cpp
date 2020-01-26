#include "EditorBase.h"
#include <imgui.cpp>
#include <imgui_impl_win32.cpp>
#include <imgui_widgets.cpp>
#include <imgui_draw.cpp>


/************************************************************************************************/


using namespace FlexKit;

ID3D12PipelineState* Create_DrawImGUI(FlexKit::RenderSystem* renderSystem)
{
    auto DrawRectVShader = FlexKit::LoadShader("ImGui_VS", "ImGui_VS", "vs_5_0", "assets\\shaders\\imguiShaders.hlsl");
	auto DrawRectPShader = FlexKit::LoadShader("ImGui_PS", "ImGui_PS", "ps_5_0", "assets\\shaders\\imguiShaders.hlsl");

    EXITSCOPE(
        Release(&DrawRectVShader);
        Release(&DrawRectPShader);
        );

	D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 0, 8,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR",		0, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM,  0, 16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};


	D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC	(D3D12_DEFAULT);
	D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	Depth_Desc.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	Depth_Desc.DepthEnable = false;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
		PSO_Desc.pRootSignature        = renderSystem->Library.RSDefault;
		PSO_Desc.VS                    = DrawRectVShader;
		PSO_Desc.PS                    = DrawRectPShader;
		PSO_Desc.RasterizerState       = Rast_Desc;
		PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		PSO_Desc.SampleMask            = UINT_MAX;
		PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		PSO_Desc.NumRenderTargets      = 1;
		PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT;
		PSO_Desc.SampleDesc.Count      = 1;
		PSO_Desc.SampleDesc.Quality    = 0;
		PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
		PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
		PSO_Desc.DepthStencilState     = Depth_Desc;

        PSO_Desc.BlendState.RenderTarget[0].BlendEnable     = true;
        PSO_Desc.BlendState.RenderTarget[0].BlendOp         = D3D12_BLEND_OP_ADD;

        PSO_Desc.BlendState.RenderTarget[0].SrcBlend        = D3D12_BLEND_SRC_ALPHA;
        PSO_Desc.BlendState.RenderTarget[0].SrcBlendAlpha   = D3D12_BLEND_SRC_ALPHA;

        PSO_Desc.BlendState.RenderTarget[0].DestBlend       = D3D12_BLEND_INV_SRC_ALPHA;
        PSO_Desc.BlendState.RenderTarget[0].DestBlendAlpha  = D3D12_BLEND_INV_SRC_ALPHA;
	}

	ID3D12PipelineState* PSO = nullptr;
	auto HR = renderSystem->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
	FK_ASSERT(SUCCEEDED(HR));

	return PSO;
}

const FlexKit::PSOHandle DRAW_imgui = FlexKit::PSOHandle(GetTypeGUID(DRAW_imgui));


/************************************************************************************************/


EditorBase::EditorBase(FlexKit::GameFramework& IN_framework) :
    FlexKit::FrameworkState{ IN_framework },

    streamingEngine{ framework.core.RenderSystem, framework.core.GetBlockMemory() },

    render{
        framework.core.GetTempMemory(),
        framework.core.RenderSystem,
        streamingEngine,
        framework.ActiveWindow->WH  },

    depthBuffer         { framework.core.RenderSystem.CreateDepthBuffer(framework.ActiveWindow->WH,	true)   },
    vertexBuffer        { framework.core.RenderSystem.CreateVertexBuffer(8096 * 64, false)                  },
    constantBuffer      { framework.core.RenderSystem.CreateConstantBuffer(8096 * 2000, false)              },

   	cameras		        { framework.core.GetBlockMemory()                               },
	ids			        { framework.core.GetBlockMemory()                               },
	drawables	        { framework.core.GetBlockMemory(), framework.GetRenderSystem()  },
	visables	        { framework.core.GetBlockMemory()                               },
	pointLights	        { framework.core.GetBlockMemory()                               },
    skeletonComponent   { framework.core.GetBlockMemory()                               },
    gbuffer             { framework.ActiveWindow->WH, framework.core.RenderSystem       },
    shadowCasters       { framework.core.GetBlockMemory()                               }
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();


    auto&                       renderSystem    = framework.GetRenderSystem();
    FlexKit::UploadQueueHandle  uploadQueue     = renderSystem.ImmediateUpload;
    ImGuiIO&                    io              = ImGui::GetIO();
    io.FontGlobalScale = 2.0f;

    renderSystem.RegisterPSOLoader(DRAW_imgui, { &renderSystem.Library.RSDefault, Create_DrawImGUI });
    renderSystem.QueuePSOLoad(DRAW_imgui);

    io.Fonts->GetTexDataAsRGBA32(&atlas.tex_pixels, &atlas.tex_w, &atlas.tex_h);
    FlexKit::TextureBuffer buffer{ {(uint32_t)atlas.tex_w, (uint32_t)atlas.tex_h}, (FlexKit::byte*)atlas.tex_pixels, 4 };


    imGuiFont = MoveTextureBuffersToVRAM(
        renderSystem,
        uploadQueue,
        &buffer,
        1,
        framework.core.GetBlockMemory(),
        FlexKit::FORMAT_2D::R8G8B8A8_UNORM);

    io.Fonts->TexID = TextreHandleToIM(imGuiFont);
}


/************************************************************************************************/


EditorBase::~EditorBase()
{
    ImGui::DestroyContext();
}


/************************************************************************************************/


void EditorBase::Draw(
    FlexKit::EngineCore&        core,
    FlexKit::UpdateDispatcher&  dispatcher,
    double                      dT,
    FlexKit::FrameGraph&        frameGraph)
{
    // Draw imGui
    ImGuiIO& io = ImGui::GetIO();
    
    io.DisplaySize = ImVec2(core.Window.WH[0], core.Window.WH[1]);
    io.DeltaTime = dT;

    auto reserveVertexBufferSpace   = CreateVertexBufferReserveObject(vertexBuffer, core.RenderSystem, core.GetTempMemory());
    auto reserveConstantBufferSpace = CreateConstantBufferReserveObject(constantBuffer, core.RenderSystem, core.GetTempMemory());

    ImGui::NewFrame();
    ImGui::SetNextWindowSize({ float(core.Window.WH[0] / 8), (float)core.Window.WH[1] });
    ImGui::SetNextWindowPos({ (float)(core.Window.WH[0] - core.Window.WH[0] / 8), 0 });

    if (ImGui::Begin("properties"))
    {
        float f = 0;
        char input[256];
        memset(input, '\0', 256);

        ImGui::Text("!Hello, World!");
        ImGui::InputText("input text", input, 256);
        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
    }

    ImGui::End();
    ImGui::Render();

    ClearVertexBuffer(frameGraph, vertexBuffer);
    ClearBackBuffer(frameGraph, core.Window.backBuffer, 0.0f);

    DrawImGui(dispatcher, frameGraph, reserveVertexBufferSpace, reserveConstantBufferSpace, core.Window.backBuffer, ImGui::GetDrawData());
}


/************************************************************************************************/


void EditorBase::DrawImGui(
    FlexKit::UpdateDispatcher&      dispatcher,
    FlexKit::FrameGraph&            frameGraph,
    ReserveVertexBufferFunction     ReserveVBSpace,
    ReserveConstantBufferFunction   ReserveCBSpace,
    FlexKit::ResourceHandle         renderTarget,
    ImDrawData*                     drawData)
{
    struct DrawImGui_data
    {
        FlexKit::VertexBufferHandle     vertexBuffer;
        ReserveVertexBufferFunction     ReserveVBSpace;
        ReserveConstantBufferFunction   ReserveCBSpace;
        FlexKit::uint2                  WH;
    };

    
    auto& UI_Pass = frameGraph.AddNode<DrawImGui_data>(
        DrawImGui_data{ vertexBuffer, ReserveVBSpace, ReserveCBSpace, frameGraph.GetRenderSystem().GetTextureWH(renderTarget) },
        [](auto& builder, DrawImGui_data&)
        {
        },
        [drawData, renderTarget](DrawImGui_data& pass, FlexKit::FrameResources& frameResources, FlexKit::Context& ctx, auto& allocator)
        {
            const auto cmdCount     = drawData->CmdListsCount;
            auto& cmdLists          = drawData->CmdLists;
            const ImVec2 clip_off   = drawData->DisplayPos;

            struct Constants
            {
                FlexKit::uint2 WidthHeight;
            };


            auto  constantBuffer = pass.ReserveCBSpace(1024);
            auto  constants      = FlexKit::ConstantBufferDataSet(Constants{ pass.WH }, constantBuffer);
            auto& rootSig        = frameResources.renderSystem.Library.RSDefault;

            // Setup draw State
            //

            auto SetupState = [&] {
                ctx.SetRootSignature(rootSig);
                ctx.SetPipelineState(frameResources.GetPipelineState(DRAW_imgui));
                ctx.SetScissorAndViewports({ renderTarget });
                ctx.SetRenderTargets({ renderTarget }, false);
                ctx.SetPrimitiveTopology(FlexKit::EInputTopology::EIT_TRIANGLELIST);
                ctx.SetGraphicsConstantBufferView(0, constants);
            };

            SetupState();

            for (int32_t i = 0; i < cmdCount; ++i)
            {
                auto&                           cmdList     = cmdLists[i];
                auto&                           cmdBuffer   = cmdList->CmdBuffer;
                
                FlexKit::VBPushBuffer           VBSpace     = pass.ReserveVBSpace(cmdList->VtxBuffer.Size * sizeof(ImDrawVert) + cmdList->IdxBuffer.Size * sizeof(uint32_t));
                FlexKit::VertexBufferDataSet    VBSet{ (ImDrawVert*)cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert), VBSpace };
                FlexKit::VertexBufferDataSet    IBSet{ (ImDrawIdx*)cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx), VBSpace };

                ctx.SetVertexBuffers({ VBSet });
                ctx.SetIndexBuffer(
                    IBSet,
                    sizeof(ImDrawIdx) == 2 ? FlexKit::FORMAT_2D::R16_UINT : FlexKit::FORMAT_2D::R32_UINT);


                for (auto& cmd : cmdBuffer)
                {
                    if (cmd.UserCallback != NULL)
                    {
                        /*
                        if (cmd.UserCallback == ImDrawCallback_ResetRenderState)
                            SetupState();
                        else
                            pcmd->UserCallback(cmd_list, pcmd);
                        */
                    }

                    const D3D12_RECT r = {
                        (LONG)(cmd.ClipRect.x - clip_off.x),
                        (LONG)(cmd.ClipRect.y - clip_off.y),
                        (LONG)(cmd.ClipRect.z - clip_off.x),
                        (LONG)(cmd.ClipRect.w - clip_off.y) };

                    auto texture = FlexKit::ResourceHandle{ (size_t)cmd.TextureId };

                    FlexKit::DescriptorHeap heap;
                    heap.Init2( ctx, rootSig.GetDescHeap(0), 1, &allocator );
                    heap.SetSRV(ctx, 0, texture);

                    ctx.SetGraphicsDescriptorTable(3, heap);
                    ctx.SetScissorRects({ r });
                    ctx.DrawIndexed(cmd.ElemCount, cmd.IdxOffset, cmd.VtxOffset);
                }
            }
        });
}


/************************************************************************************************/


void EditorBase::Update(FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT)
{
    ImGuiIO& io = ImGui::GetIO();
    auto hwnd   = core.Window.hWindow;

    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        ::SetCursor(NULL);
    }
    else
    {
        // Show OS mouse cursor
        LPTSTR win32_cursor = IDC_ARROW;
        switch (imgui_cursor)
        {
        case ImGuiMouseCursor_Arrow:        win32_cursor = IDC_ARROW; break;
        case ImGuiMouseCursor_TextInput:    win32_cursor = IDC_IBEAM; break;
        case ImGuiMouseCursor_ResizeAll:    win32_cursor = IDC_SIZEALL; break;
        case ImGuiMouseCursor_ResizeEW:     win32_cursor = IDC_SIZEWE; break;
        case ImGuiMouseCursor_ResizeNS:     win32_cursor = IDC_SIZENS; break;
        case ImGuiMouseCursor_ResizeNESW:   win32_cursor = IDC_SIZENESW; break;
        case ImGuiMouseCursor_ResizeNWSE:   win32_cursor = IDC_SIZENWSE; break;
        case ImGuiMouseCursor_Hand:         win32_cursor = IDC_HAND; break;
        case ImGuiMouseCursor_NotAllowed:   win32_cursor = IDC_NO; break;
        }
        ::SetCursor(::LoadCursor(NULL, win32_cursor));
    }

    // Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
    if (io.WantSetMousePos)
    {
        POINT pos = { (int)io.MousePos.x, (int)io.MousePos.y };
        ::ClientToScreen(core.Window.hWindow, &pos);
        ::SetCursorPos(pos.x, pos.y);
    }

    // Set mouse position
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    POINT pos;
    if (HWND active_window = ::GetForegroundWindow())
        if (active_window == hwnd || ::IsChild(active_window, hwnd))
            if (::GetCursorPos(&pos) && ::ScreenToClient(hwnd, &pos))
                io.MousePos = ImVec2((float)pos.x, (float)pos.y);
}


/************************************************************************************************/


void EditorBase::PostDrawUpdate(FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT, FlexKit::FrameGraph& frameGraph)
{
    FlexKit::PresentBackBuffer(frameGraph, &core.Window);
}


bool EditorBase::EventHandler(FlexKit::Event evt)
{
    if (ImGui::GetCurrentContext() == NULL)
        return false;

    ImGuiIO& io = ImGui::GetIO();

	if (evt.InputSource == Event::Keyboard)
	{
		switch (evt.Action)
		{
		case Event::Pressed:
		{
			switch (evt.mData1.mKC[0])
			{
			default:
			{
				if ((evt.mData1.mKC[0] >= KC_A && evt.mData1.mKC[0] <= KC_Z ) || 
					(evt.mData1.mKC[0] >= KC_0 && evt.mData1.mKC[0] <= KC_9)  ||
					(evt.mData1.mKC[0] >= KC_SYMBOLS_BEGIN && evt.mData1.mKC[0] <= KC_SYMBOLS_END ) || 
					(evt.mData1.mKC[0] == KC_PLUS) || (evt.mData1.mKC[0] == KC_MINUS) ||
					(evt.mData1.mKC[0] == KC_UNDERSCORE) || (evt.mData1.mKC[0] == KC_EQUAL) ||
					(evt.mData1.mKC[0] == KC_SYMBOL ))
                {
                    io.AddInputCharacter((char)evt.mData2.mINT[0]);
                }
			}	break;
			}
		}	break;
		}
	}

    if (evt.InputSource == Event::Mouse)
    {
        int KC = -1;

        switch (evt.mData1.mKC[0])
        {
            case FlexKit::KC_MOUSELEFT:
                KC = 0; break;
            case FlexKit::KC_MOUSEMIDDLE:
                KC = 2; break;
            case FlexKit::KC_MOUSERIGHT:
                KC = 1; break;
        }

        
        if (KC != -1 && evt.Action == Event::Pressed || evt.Action == Event::Release)
        {
            const bool pressed = evt.Action == Event::Pressed ? true : false;
            io.MouseDown[KC] = pressed;
        }
    }


    return true;
}


/**********************************************************************

Copyright (c) 2019 Robert May

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
