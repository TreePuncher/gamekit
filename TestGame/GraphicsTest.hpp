#pragma once

#include "BaseState.h"

ID3D12PipelineState* CreateACCState(FlexKit::RenderSystem* renderSystem)
{
    auto VShader = LoadShader("VS_Main", "VS_Main", "vs_5_0", "assets\\shaders\\ACC.hlsl");
    auto HShader = LoadShader("HS_Main", "HS_Main", "hs_5_0", "assets\\shaders\\ACC.hlsl");
    auto DShader = LoadShader("DS_Main", "DS_Main", "ds_5_0", "assets\\shaders\\ACC.hlsl");
    auto PShader = LoadShader("PS_Main", "PS_Main", "ps_5_0", "assets\\shaders\\ACC.hlsl");

    D3D12_INPUT_ELEMENT_DESC InputElements[] = {
                { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 0, 24, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_RASTERIZER_DESC Rast_Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); 
    Rast_Desc.FillMode = D3D12_FILL_MODE_WIREFRAME;

    D3D12_DEPTH_STENCIL_DESC Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    Depth_Desc.DepthEnable = false;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC PSO_DESC = {
        .pRootSignature         = renderSystem->Library.RSDefault,
        .VS                     = VShader,
        .PS                     = PShader,
        .DS                     = DShader,
        .HS                     = HShader,
        .BlendState             = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask             = UINT_MAX,
        .RasterizerState        = Rast_Desc,
        .DepthStencilState      = Depth_Desc,
        .InputLayout            = { InputElements, sizeof(InputElements) / sizeof(*InputElements) },
        .PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH,
        .NumRenderTargets       = 1,
        .RTVFormats             = { DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT },
        .SampleDesc             = { 1, 0 },
    };

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

    void Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT) override
    {
    }

    void DebugDraw(EngineCore& core, UpdateDispatcher& dispatcher, double dT) override
    {
    }

    void PreDrawUpdate(EngineCore& core, UpdateDispatcher& dispatcher, double dT) override
    {
    }

    void Draw(EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph) override
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
                data.renderTarget = nodeBuilder.WriteRenderTarget(base.renderWindow.GetBackBuffer());
            },
            [=](ACCTest& data, FrameResources& resources, Context& ctx, iAllocator& allocator)
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
                VertexBufferDataSet vbDataSet{ vertexBuffer, vertexPool };
                VertexBufferDataSet ibDataSet{ indexBuffer, vertexPool };

                ctx.SetScissorAndViewports(std::tuple{ resources.GetRenderTarget(data.renderTarget) });

                ctx.SetRenderTargets(
                    { resources.GetTexture(data.renderTarget) },
                    false);

                ctx.SetRootSignature(resources.renderSystem.Library.RSDefault);
                ctx.SetPipelineState(resources.GetPipelineState(ACCState));
                ctx.SetVertexBuffers({ vbDataSet });
                ctx.SetIndexBuffer(ibDataSet);
                ctx.SetPrimitiveTopology(EIT_PATCH_CP_4);
                ctx.DrawIndexed(8);
            });
    }

    void PostDrawUpdate(EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph) override
    {
        PresentBackBuffer(frameGraph, base.renderWindow);
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
