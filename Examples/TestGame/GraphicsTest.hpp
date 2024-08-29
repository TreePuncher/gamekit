#pragma once

#include "BaseState.h"

ID3D12PipelineState* CreateACCState(FlexKit::RenderSystem* renderSystem)
{
    auto VShader = renderSystem->LoadShader("VS_Main", "vs_5_0", "assets\\shaders\\ACC.hlsl");
    auto HShader = renderSystem->LoadShader("HS_Main", "hs_5_0", "assets\\shaders\\ACC.hlsl");
    auto DShader = renderSystem->LoadShader("DS_Main", "ds_5_0", "assets\\shaders\\ACC.hlsl");
    auto PShader = renderSystem->LoadShader("PS_Main", "ps_5_0", "assets\\shaders\\ACC.hlsl");

    D3D12_INPUT_ELEMENT_DESC InputElements[] = {
                { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 0, 24, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_RASTERIZER_DESC Rast_Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); 
    Rast_Desc.FillMode = D3D12_FILL_MODE_WIREFRAME;

    D3D12_DEPTH_STENCIL_DESC Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    Depth_Desc.DepthEnable = false;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC PSO_DESC;
    PSO_DESC.pRootSignature = renderSystem->Library.RSDefault;
    PSO_DESC.VS                     = VShader;
    PSO_DESC.PS                     = PShader;
    PSO_DESC.DS                     = DShader;
    PSO_DESC.HS                     = HShader;
    PSO_DESC.BlendState             = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    PSO_DESC.SampleMask             = UINT_MAX;
    PSO_DESC.RasterizerState        = Rast_Desc;
    PSO_DESC.DepthStencilState      = Depth_Desc;
    PSO_DESC.InputLayout            = { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
    PSO_DESC.PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    PSO_DESC.NumRenderTargets       = 1;
    PSO_DESC.RTVFormats[0]          = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
    PSO_DESC.SampleDesc             = { 1, 0 };

    ID3D12PipelineState* PSO = nullptr;

    const auto HR = renderSystem->pDevice->CreateGraphicsPipelineState(&PSO_DESC, IID_PPV_ARGS(&PSO));

    if(SUCCEEDED(HR)) FK_LOG_ERROR("Failed to create ACC PSO!");

    return PSO;
}

inline constexpr PSOHandle ACCState = PSOHandle(GetTypeGUID(CreateACCState));

class GraphicsTest : public FlexKit::FrameworkState
{
public:
    GraphicsTest(GameFramework& IN_framework, BaseState& IN_base) :
        FrameworkState  { IN_framework },
        base            { IN_base }
    {
        framework.GetRenderSystem().RegisterPSOLoader(ACCState, { &framework.GetRenderSystem().Library.RSDefault, CreateACCState });
        framework.GetRenderSystem().QueuePSOLoad(ACCState);
    }

    UpdateTask* Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT) override
    {
        return nullptr;
    }

    UpdateTask* Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph) override
    {
        ClearBackBuffer(frameGraph, base.renderWindow.GetBackBuffer(), 0.0f);
        ClearVertexBuffer(frameGraph, base.vertexBuffer);

        ReserveVertexBufferFunction     reserveVB = FlexKit::CreateVertexBufferReserveObject(base.vertexBuffer, core.RenderSystem, core.GetTempMemory());
        ReserveConstantBufferFunction   reserveCB = FlexKit::CreateConstantBufferReserveObject(base.constantBuffer, core.RenderSystem, core.GetTempMemory());

        struct ACCTest
        {
            ReserveVertexBufferFunction     reserveVB;
            ReserveConstantBufferFunction   reserveCB;
            FrameResourceHandle             renderTarget;
        };

        frameGraph.AddNode<ACCTest>(
            ACCTest
            {
                reserveVB,
                reserveCB,
            },
            [&](FrameGraphNodeBuilder& nodeBuilder, ACCTest& data)
            {
                data.renderTarget = nodeBuilder.RenderTarget(base.renderWindow.GetBackBuffer());
            },
            [=](ACCTest& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
            {
                struct ControlPoint
                {
                    float point[3];
                    float normals[3];
                    float textureCoord[2];
                }const vertexBuffer[] =
                {
                    { { -0.5,  -0.5, 1 }, { 0, 0, 0 }, { 0, 0 } },
                    { { -0.25,  0.5, 1 }, { 0, 0, 0 }, { 0, 0 } },
                    { {  0.5,  -0.5, 1 }, { 0, 0, 0 }, { 0, 0 } },
                    { {  0.25,  0.5, 1 }, { 0, 0, 0 }, { 0, 0 } },
                    { { -0.5,  0.75, 1 }, { 0, 0, 0 }, { 0, 0 } },
                    { {  0.5,  0.75, 1 }, { 0, 0, 0 }, { 0, 0 } },
                };

                const uint32_t indexBuffer[] = {
                    0, 1, 2, 3,
                    1, 4, 3, 5,
                };

                VBPushBuffer vertexPool = data.reserveVB(8192);
                VertexBufferDataSet vbDataSet{ vertexBuffer, 6, vertexPool };
                VertexBufferDataSet ibDataSet{ indexBuffer, 8, vertexPool };

                SetScissorAndViewports(ctx, std::tuple{ resources.GetRenderTarget(data.renderTarget) });

                ctx.SetRenderTargets(
                    { resources.GetResource(data.renderTarget) },
                    false);

                ctx.SetRootSignature(resources.renderSystem().Library.RSDefault);
                ctx.SetPipelineState(resources.GetPipelineState(ACCState));
                ctx.SetVertexBuffers({ vbDataSet });
                ctx.SetIndexBuffer(ibDataSet);
                ctx.SetPrimitiveTopology(EIT_PATCH_CP_4);
                ctx.DrawIndexed(8);
            });

        PresentBackBuffer(frameGraph, base.renderWindow);

        return nullptr;
    }

    void PostDrawUpdate(EngineCore& core, double dT) override
    {
        base.PostDrawUpdate(core, dT);
    }

    bool EventHandler(Event evt) override
    {
        switch (evt.InputSource)
        {
        case Event::E_SystemEvent:
        {
            switch (evt.Action)
            {
            case Event::InputAction::Resized:
            {
                const auto width = (uint32_t)evt.mData1.mINT[0];
                const auto height = (uint32_t)evt.mData2.mINT[0];
                base.Resize({ width, height });
            }   break;
            default:
                break;
            }
        }   break;
        };

        return base.EventHandler(evt);
    }


private:

    BaseState& base;
};
