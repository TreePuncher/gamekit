#include "Transparency.h"
#include "Materials.h"

namespace FlexKit
{   /************************************************************************************************/


    ID3D12PipelineState* Transparency::CreateOITDrawPSO(RenderSystem* RS)
    {
        auto VShader = RS->LoadShader("VMain",      "vs_6_0", "assets\\shaders\\OITPass.hlsl");
		auto PShader = RS->LoadShader("PassMain",   "ps_6_0", "assets\\shaders\\OITPass.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
            { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",	    0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    1, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    2, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,       3, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        Rast_Desc.CullMode                      = D3D12_CULL_MODE_BACK;

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	                = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	                = true;
        Depth_Desc.DepthWriteMask               = D3D12_DEPTH_WRITE_MASK_ZERO;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RSDefault;
			PSO_Desc.VS                    = VShader;
			PSO_Desc.PS                    = PShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets      = 2;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
			PSO_Desc.RTVFormats[1]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // count
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
			PSO_Desc.DepthStencilState     = Depth_Desc;

            PSO_Desc.BlendState.IndependentBlendEnable          = true;

            PSO_Desc.BlendState.RenderTarget[0].BlendEnable     = true;
            PSO_Desc.BlendState.RenderTarget[0].BlendOp         = D3D12_BLEND_OP_ADD;
            PSO_Desc.BlendState.RenderTarget[0].BlendOpAlpha    = D3D12_BLEND_OP_ADD;

            PSO_Desc.BlendState.RenderTarget[0].SrcBlend        = D3D12_BLEND_ONE;
            PSO_Desc.BlendState.RenderTarget[0].SrcBlendAlpha   = D3D12_BLEND_ONE;

            PSO_Desc.BlendState.RenderTarget[0].DestBlend       = D3D12_BLEND_ONE;
            PSO_Desc.BlendState.RenderTarget[0].DestBlendAlpha  = D3D12_BLEND_ONE;

            PSO_Desc.BlendState.RenderTarget[1].BlendEnable     = true;
            PSO_Desc.BlendState.RenderTarget[1].BlendOp         = D3D12_BLEND_OP_ADD;
            PSO_Desc.BlendState.RenderTarget[1].BlendOpAlpha    = D3D12_BLEND_OP_ADD;

            PSO_Desc.BlendState.RenderTarget[1].SrcBlend        = D3D12_BLEND_ZERO;
            PSO_Desc.BlendState.RenderTarget[1].SrcBlendAlpha   = D3D12_BLEND_ZERO;

            PSO_Desc.BlendState.RenderTarget[1].DestBlend       = D3D12_BLEND_INV_SRC_ALPHA;
            PSO_Desc.BlendState.RenderTarget[1].DestBlendAlpha  = D3D12_BLEND_INV_SRC_ALPHA;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* Transparency::CreateOITDrawAnimatedPSO(RenderSystem* RS)
    {
        return nullptr;
    }


    /************************************************************************************************/


    ID3D12PipelineState* Transparency::CreateOITBlendPSO(RenderSystem* RS)
    {
        auto VShader = RS->LoadShader("VMain", "vs_6_0", "assets\\shaders\\OITBlend.hlsl");
		auto PShader = RS->LoadShader("BlendMain", "ps_6_0", "assets\\shaders\\OITBlend.hlsl");

		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	                = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	                = false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RSDefault;
			PSO_Desc.VS                    = VShader;
			PSO_Desc.PS                    = PShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets      = 1;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout           = { nullptr, 0 };
			PSO_Desc.DepthStencilState     = Depth_Desc;

            PSO_Desc.BlendState.IndependentBlendEnable          = true;
            PSO_Desc.BlendState.RenderTarget[0].BlendEnable     = true;
            PSO_Desc.BlendState.RenderTarget[0].BlendOp         = D3D12_BLEND_OP_ADD;
            PSO_Desc.BlendState.RenderTarget[0].BlendOpAlpha    = D3D12_BLEND_OP_ADD;

            PSO_Desc.BlendState.RenderTarget[0].SrcBlend        = D3D12_BLEND_INV_SRC_ALPHA;
            PSO_Desc.BlendState.RenderTarget[0].SrcBlendAlpha   = D3D12_BLEND_ONE;

            PSO_Desc.BlendState.RenderTarget[0].DestBlend       = D3D12_BLEND_SRC_ALPHA;
            PSO_Desc.BlendState.RenderTarget[0].DestBlendAlpha  = D3D12_BLEND_ONE;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
    }


    /************************************************************************************************/


    Transparency::Transparency(RenderSystem& renderSystem)
    {
        renderSystem.RegisterPSOLoader(OITBLEND,           { &renderSystem.Library.RSDefault, CreateOITBlendPSO });
        renderSystem.RegisterPSOLoader(OITDRAW,            { &renderSystem.Library.RSDefault, CreateOITDrawPSO });
        renderSystem.RegisterPSOLoader(OITDRAWANIMATED,    { &renderSystem.Library.RSDefault, CreateOITDrawAnimatedPSO });
    }


    /************************************************************************************************/


    OITPass& Transparency::OIT_WB_Pass(
		UpdateDispatcher&               dispatcher,
		FrameGraph&                     frameGraph,
        GatherPassesTask&               passes,
        CameraHandle                    camera,
		ResourceHandle                  depthTarget,
		ReserveConstantBufferFunction   reserveCB,
		iAllocator*                     allocator)
    {
        return frameGraph.AddNode<OITPass>(
            OITPass{
                reserveCB,

                passes,
                camera
            },
			[&](FrameGraphNodeBuilder& builder, OITPass& data)
			{
                builder.AddDataDependency(passes);

                const auto WH = builder.GetRenderSystem().GetTextureWH(depthTarget);

                data.depthTarget        = builder.DepthRead(depthTarget);
                data.accumalatorObject  = builder.AcquireVirtualResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R16G16B16A16_FLOAT), DRS_RenderTarget, false);
                data.counterObject      = builder.AcquireVirtualResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R16G16B16A16_FLOAT), DRS_RenderTarget, false);

                builder.SetDebugName(data.accumalatorObject,    "Accumalator");
                builder.SetDebugName(data.counterObject,        "counterObject");
			},
            [=](OITPass& data, ResourceHandler& resources, Context& ctx, iAllocator& tempAllocator)
            {
                ProfileFunction();

                const RootSignature&    rootSig     = resources.renderSystem().Library.RSDefault;
                auto&                   materials   = MaterialComponent::GetComponent();


                TriMeshHandle   previous        = InvalidHandle_t;
                TriMesh*        triMesh         = nullptr;
                MaterialHandle  prevMaterial    = InvalidHandle_t;

                ctx.ClearRenderTarget(resources.GetRenderTarget(data.accumalatorObject));
                ctx.ClearRenderTarget(resources.GetRenderTarget(data.counterObject), float4{ 1, 1, 1, 1 });

                auto& passes    = data.PVS.GetData().passes;
                const PVS* pvs  = nullptr;
                if (auto res = std::find_if(passes.begin(), passes.end(),
                    [](auto& pass) -> bool
                    {
                        return pass.pass == PassHandle{GetCRCGUID(OIT_MCGUIRE)};
                    }); res != passes.end())
                {
                    pvs = &res->pvs;
                }
                else
                    return;

                if (!pvs->size())
                    return;

                ctx.BeginEvent_DEBUG("OIT - PASS");

                ctx.SetRootSignature(rootSig);
                ctx.SetPipelineState(resources.GetPipelineState(OITDRAW));

                CBPushBuffer constantBuffer{
                    data.reserveCB(
                        AlignedSize<Brush::VConstantsLayout>() * pvs->size() +
                        AlignedSize<Camera::ConstantBuffer>() )};

                const auto cameraConstantValues = GetCameraConstants(data.camera);
                const ConstantBufferDataSet cameraConstants{ cameraConstantValues, constantBuffer };


                ctx.SetGraphicsConstantBufferView(0, cameraConstants);
                ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);

                ctx.SetScissorAndViewports({
                        resources.GetRenderTarget(data.accumalatorObject),
                        resources.GetRenderTarget(data.counterObject),
                    });

                ctx.SetRenderTargets(
                    {
                        resources.GetRenderTarget(data.accumalatorObject),
                        resources.GetRenderTarget(data.counterObject),
                    },
                    true,
                    resources.GetRenderTarget(data.depthTarget)
                );

                for (auto& draw : *pvs)
                {
                    const auto mesh         = draw.brush->MeshHandle;
                    const auto material     = draw.brush->material;

                    if (mesh != previous)
                    {
                        previous    = mesh;
                        triMesh     = GetMeshResource(mesh);

                        auto& lod = triMesh->lods[draw.LODlevel];
                        
                        ctx.AddIndexBuffer(
                            triMesh,
                            draw.LODlevel);

                        ctx.AddVertexBuffers(
                            triMesh,
                            draw.LODlevel,
                            {
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,
                            }
                        );
                    }


                    if (material != prevMaterial)
                    {
                        const auto& textures = materials[material].Textures;

                        DescriptorHeap heap;

                        heap.Init2(ctx, rootSig.GetDescHeap(0), textures.size(), &tempAllocator);

                        for (size_t I = 0; I < textures.size(); ++I)
                            heap.SetSRV(ctx, I, textures[I]);

                        ctx.SetGraphicsDescriptorTable(4, heap);
                    }

                    const auto textureCount     = materials[material].Textures.size();
                    const auto constantValues   = draw.brush->GetConstants();

                    const ConstantBufferDataSet localConstants{ constantValues, constantBuffer };

                    ctx.SetGraphicsConstantBufferView(1, localConstants);

                    for (auto& subMesh : triMesh->GetHighestLoadedLod().subMeshes)
                        ctx.DrawIndexed(
                            subMesh.IndexCount,
                            subMesh.BaseIndex);

                }

                ctx.EndEvent_DEBUG();
            });
    }


    /************************************************************************************************/


    OITBlend& Transparency::OIT_WB_Blend(
        UpdateDispatcher&               dispatcher,
        FrameGraph&                     frameGraph,
        OITPass&                        OITPass,
        FrameResourceHandle             renderTarget,
        iAllocator*                     allocator)
    {
        return frameGraph.AddNode<OITBlend>(
            OITBlend{},
            [&](FrameGraphNodeBuilder& builder, OITBlend& data)
            {
                data.renderTargetObject = builder.WriteTransition(renderTarget, DRS_RenderTarget);
                data.accumalatorObject  = OITPass.accumalatorObject;
                data.counterObject      = OITPass.counterObject;

                builder.ReleaseVirtualResource(OITPass.accumalatorObject);
                builder.ReleaseVirtualResource(OITPass.counterObject);
            },
            [=](OITBlend& data, ResourceHandler& resources, Context& ctx, iAllocator& tempAllocator)
            {
                ProfileFunction();

                ctx.BeginEvent_DEBUG("OIT - Blend");

                ctx.SetPipelineState(resources.GetPipelineState(OITBLEND));
                ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);

                const RootSignature& rootSig = resources.renderSystem().Library.RSDefault;
                auto& descHeapLayout = rootSig.GetDescHeap(0);
                DescriptorHeap descHeap;

                descHeap.Init2(ctx, descHeapLayout, 2, &tempAllocator);
                descHeap.SetSRV(ctx, 0, resources.PixelShaderResource(data.accumalatorObject, ctx));
                descHeap.SetSRV(ctx, 1, resources.PixelShaderResource(data.counterObject, ctx));

                ctx.SetRootSignature(rootSig);
                ctx.SetGraphicsDescriptorTable(4, descHeap);

                ctx.SetScissorAndViewports({ resources.GetRenderTarget(data.renderTargetObject), });

                ctx.SetRenderTargets(
                    {
                        resources.GetRenderTarget(data.renderTargetObject),
                    },
                    false);

                ctx.Draw(6);

                ctx.EndEvent_DEBUG();
            });
    }


}   /************************************************************************************************/
