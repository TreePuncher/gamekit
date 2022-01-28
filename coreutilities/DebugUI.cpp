
#include "DebugUI.h"
#include "Win32Graphics.h"
#include <imgui.h>
#include <implot.h>

namespace FlexKit
{   /************************************************************************************************/


    inline ImTextureID TextreHandleToIM(FlexKit::ResourceHandle texture)
    {
        return reinterpret_cast<ImTextureID>((size_t)texture.INDEX);
    }


    /************************************************************************************************/


    ID3D12PipelineState* Create_DrawImGUI(FlexKit::RenderSystem* renderSystem)
    {
	    auto DrawRectVShader = renderSystem->LoadShader("ImGui_VS", "vs_6_0", "assets\\shaders\\imguiShaders.hlsl");
	    auto DrawRectPShader = renderSystem->LoadShader("ImGui_PS", "ps_6_0", "assets\\shaders\\imguiShaders.hlsl");

	    D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			    { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			    { "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 0, 8,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			    { "COLOR",		0, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM,  0, 16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	    };


	    D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC	(D3D12_DEFAULT);
        Rast_Desc.CullMode      = D3D12_CULL_MODE_NONE;

	    D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	    Depth_Desc.DepthFunc    = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	    Depth_Desc.DepthEnable  = false;

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


    ImGUIIntegrator::ImGUIIntegrator(RenderSystem& renderSystem, iAllocator* memory)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();

        ImPlot::GetStyle().AntiAliasedLines = true;

        FlexKit::CopyContextHandle  uploadQueue = renderSystem.ImmediateUpload;
        ImGuiIO& io                     = ImGui::GetIO();
        io.FontGlobalScale              = 1.5f;
        io.KeyMap[ImGuiKey_Backspace]   = VK_BACK;

        renderSystem.RegisterPSOLoader(DRAW_imgui, { &renderSystem.Library.RSDefault, Create_DrawImGUI });
        renderSystem.QueuePSOLoad(DRAW_imgui);

        unsigned char* tex_pixels = nullptr;
        int             tex_w;
        int             tex_h;

        io.Fonts->GetTexDataAsRGBA32(&tex_pixels, &tex_w, &tex_h);
        FlexKit::TextureBuffer buffer{ {(uint32_t)tex_w, (uint32_t)tex_h}, (FlexKit::byte*)tex_pixels, 4 };


        imGuiFont = MoveTextureBuffersToVRAM(
            renderSystem,
            uploadQueue,
            &buffer,
            1,
            FlexKit::DeviceFormat::R8G8B8A8_UNORM);

        io.Fonts->TexID = TextreHandleToIM(imGuiFont);
    }


    ImGUIIntegrator::~ImGUIIntegrator()
    {
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
    }


    /************************************************************************************************/

    void ImGUIIntegrator::Update(uint2 MouseXY, uint2 WH, FlexKit::UpdateDispatcher& dispatcher, double dT)
    {
        ImGuiIO& io     = ImGui::GetIO();
        io.DisplaySize  = ImVec2(WH[0], WH[1]);
        io.DeltaTime    = dT;
        io.MousePos     = { (float)MouseXY[0], (float)MouseXY[1] };

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
    }



    void ImGUIIntegrator::Update(Win32RenderWindow& window, FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT)
    {
        const auto WH   = window.GetWH();
        auto hwnd       = window.hWindow;

        ImGuiIO& io     = ImGui::GetIO();
        io.DisplaySize  = ImVec2(WH[0], WH[1]);
        io.DeltaTime    = dT;


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
		    ::ClientToScreen(window.hWindow, &pos);
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


    bool ImGUIIntegrator::HandleInput(FlexKit::Event evt)
    {
	    if (ImGui::GetCurrentContext() == NULL)
		    return false;

	    ImGuiIO& io = ImGui::GetIO();

	    switch (evt.InputSource)
	    {
		    case Event::Keyboard:
            {
                if (evt.mData1.mKC[0] == KC_BACKSPACE && ((evt.Action == Event::Pressed) | (evt.Action == Event::Release)))
                {
                    io.KeysDown[io.KeyMap[ImGuiKey_Backspace]] =
                        evt.Action == Event::Pressed ? true : false;
                }

                switch (evt.Action)
                {
                case Event::Pressed:
                {
                    if ((evt.mData1.mKC[0] >= KC_A && evt.mData1.mKC[0] <= KC_Z) ||
                        (evt.mData1.mKC[0] >= KC_0 && evt.mData1.mKC[0] <= KC_9) ||
                        (evt.mData1.mKC[0] >= KC_SYMBOLS_BEGIN && evt.mData1.mKC[0] <= KC_SYMBOLS_END) ||
                        (evt.mData1.mKC[0] == KC_PLUS) ||
                        (evt.mData1.mKC[0] == KC_MINUS) ||
                        (evt.mData1.mKC[0] == KC_UNDERSCORE) ||
                        (evt.mData1.mKC[0] == KC_EQUAL) ||
                        (evt.mData1.mKC[0] == KC_SYMBOL) ||
                        (evt.mData1.mKC[0] == KC_BACKSPACE) ||
                        (evt.mData1.mKC[0] == KC_SPACE))
                    {
                        io.AddInputCharacter((char)evt.mData2.mINT[0]);
                    }
                }	break;
                }
            }   break;
		    case Event::Mouse:
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
		    default:
			    break;
	    }


	    return true;
    }


    /************************************************************************************************/


    void ImGUIIntegrator::DrawImGui(const double dT, FlexKit::UpdateDispatcher&, FlexKit::FrameGraph& frameGraph, FlexKit::ReserveVertexBufferFunction reserveVB, FlexKit::ReserveConstantBufferFunction reserveCB, FlexKit::ResourceHandle renderTarget)
    {
        ImGuiIO& io         = ImGui::GetIO();
        auto*   drawData    = ImGui::GetDrawData();
        
        const auto WH   = frameGraph.GetRenderSystem().GetTextureWH(renderTarget);

        io.DisplaySize  = ImVec2(WH[0], WH[1]);
        io.DeltaTime    = dT;

        struct DrawImGui_data
        {
            ReserveVertexBufferFunction     ReserveVBSpace;
            ReserveConstantBufferFunction   ReserveCBSpace;
            FlexKit::uint2                  WH;
        };

        auto& UI_Pass = frameGraph.AddNode<DrawImGui_data>(
		    DrawImGui_data{
                reserveVB,
                reserveCB,
                WH },
		    [](auto& builder, DrawImGui_data& data) {},
		    [drawData, renderTarget](DrawImGui_data& pass, FlexKit::ResourceHandler& frameResources, FlexKit::Context& ctx, auto& allocator)
		    {
                ctx.BeginEvent_DEBUG("ImGui");

			    const auto cmdCount     = drawData->CmdListsCount;
			    auto& cmdLists          = drawData->CmdLists;
			    const ImVec2 clip_off   = drawData->DisplayPos;

			    struct alignas(256) Constants
			    {
				    FlexKit::uint2 WidthHeight;
			    };


			    auto  constantBuffer = pass.ReserveCBSpace(1024);
			    auto  constants      = FlexKit::ConstantBufferDataSet(Constants{ pass.WH }, constantBuffer);
			    auto& rootSig        = frameResources.renderSystem().Library.RSDefault;

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

				    auto _debug = (ImDrawVert*)cmdList->VtxBuffer.Data;

				    FlexKit::VBPushBuffer           VBSpace = pass.ReserveVBSpace(cmdList->VtxBuffer.Size * sizeof(ImDrawVert) + cmdList->IdxBuffer.Size * sizeof(uint32_t) + 1024);
				    FlexKit::VertexBufferDataSet    VBSet{ (ImDrawVert*)cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert), VBSpace };
				    FlexKit::VertexBufferDataSet    IBSet{ (ImDrawIdx*)cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx), VBSpace };

				    ctx.SetVertexBuffers({ VBSet });
				    ctx.SetIndexBuffer(
					    IBSet,
					    sizeof(ImDrawIdx) == 2 ? FlexKit::DeviceFormat::R16_UINT : FlexKit::DeviceFormat::R32_UINT);


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

					    ctx.SetGraphicsDescriptorTable(4, heap);
					    ctx.SetScissorRects({ r });
					    ctx.DrawIndexed(cmd.ElemCount, cmd.IdxOffset, cmd.VtxOffset);
				    }
			    }

                ctx.EndEvent_DEBUG();
		    });
    }


}   /************************************************************************************************/
