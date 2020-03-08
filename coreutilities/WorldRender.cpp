#include "WorldRender.h"
#include "TextureStreamingUtilities.h"

#include <d3d12.h>
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <d3d11sdklayers.h>
#include <d3d11shader.h>

namespace FlexKit
{	/************************************************************************************************/


    ID3D12PipelineState* CreateLightPassPSO(RenderSystem* RS)
    {
        auto lightPassShader = LoadShader("tiledLightCulling", "tiledLightCulling", "cs_5_0", "assets\\shaders\\lightPass.hlsl");

        D3D12_COMPUTE_PIPELINE_STATE_DESC PSO_desc = {};
        PSO_desc.CS				= lightPassShader;
        PSO_desc.pRootSignature = RS->Library.ComputeSignature;

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&PSO_desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    ID3D12PipelineState* CreateForwardDrawPSO(RenderSystem* RS)
    {
        auto DrawRectVShader = LoadShader("Forward_VS", "Forward_VS", "vs_5_0",	"assets\\shaders\\forwardRender.hlsl");
        auto DrawRectPShader = LoadShader("Forward_PS", "Forward_PS", "ps_5_0",	"assets\\shaders\\forwardRender.hlsl");

        FINALLY
            
        Release(&DrawRectVShader);
        Release(&DrawRectPShader);

        FINALLYOVER

        /*
        typedef struct D3D12_INPUT_ELEMENT_DESC
        {
        LPCSTR SemanticName;
        UINT SemanticIndex;
        DXGI_FORMAT Format;
        UINT InputSlot;
        UINT AlignedByteOffset;
        D3D12_INPUT_CLASSIFICATION InputSlotClass;
        UINT InstanceDataStepRate;
        } 	D3D12_INPUT_ELEMENT_DESC;
        */

        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
                { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 3, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };


        D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;
        Depth_Desc.DepthEnable	= true;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
            PSO_Desc.pRootSignature        = RS->Library.RS6CBVs4SRVs;
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

            PSO_Desc.BlendState.RenderTarget[0].BlendEnable = true;
            PSO_Desc.BlendState.RenderTarget[0].BlendOp		= D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;

            PSO_Desc.BlendState.RenderTarget[0].DestBlend   = D3D12_BLEND::D3D12_BLEND_ONE;
            PSO_Desc.BlendState.RenderTarget[0].SrcBlend    = D3D12_BLEND::D3D12_BLEND_ONE;

            PSO_Desc.BlendState.RenderTarget[0].SrcBlendAlpha   = D3D12_BLEND::D3D12_BLEND_ONE;
            PSO_Desc.BlendState.RenderTarget[0].DestBlendAlpha  = D3D12_BLEND::D3D12_BLEND_ONE;
        }


        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateDepthPrePassPSO(RenderSystem* RS)
    {
        auto DrawRectVShader = LoadShader("DepthPass_VS", "DepthPass_VS", "vs_5_0",	"assets\\shaders\\forwardRender.hlsl");

        EXITSCOPE(Release(&DrawRectVShader));
            

        /*
        typedef struct D3D12_INPUT_ELEMENT_DESC
        {
        LPCSTR SemanticName;
        UINT SemanticIndex;
        DXGI_FORMAT Format;
        UINT InputSlot;
        UINT AlignedByteOffset;
        D3D12_INPUT_CLASSIFICATION InputSlotClass;
        UINT InstanceDataStepRate;
        } 	D3D12_INPUT_ELEMENT_DESC;
        */

        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
                { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };


        D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
        Depth_Desc.DepthEnable	= true;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
            PSO_Desc.pRootSignature        = RS->Library.RS6CBVs4SRVs;
            PSO_Desc.VS                    = DrawRectVShader;
            PSO_Desc.RasterizerState       = Rast_Desc;
            PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            PSO_Desc.SampleMask            = UINT_MAX;
            PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            PSO_Desc.NumRenderTargets      = 0;
            PSO_Desc.SampleDesc.Count      = 1;
            PSO_Desc.SampleDesc.Quality    = 0;
            PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
            PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
            PSO_Desc.DepthStencilState     = Depth_Desc;
            PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
        }

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateGBufferPassPSO(RenderSystem* RS)
    {
        auto DrawRectVShader = LoadShader("Forward_VS", "Forward_VS",           "vs_5_0",	"assets\\shaders\\forwardRender.hlsl");
        auto DrawRectPShader = LoadShader("GBufferFill_PS", "GBufferFill_PS",   "ps_5_0",	"assets\\shaders\\forwardRender.hlsl");

        FINALLY
         Release(&DrawRectVShader);
         Release(&DrawRectPShader);
        FINALLYOVER

        /*
        typedef struct D3D12_INPUT_ELEMENT_DESC
        {
        LPCSTR SemanticName;
        UINT SemanticIndex;
        DXGI_FORMAT Format;
        UINT InputSlot;
        UINT AlignedByteOffset;
        D3D12_INPUT_CLASSIFICATION InputSlotClass;
        UINT InstanceDataStepRate;
        } 	D3D12_INPUT_ELEMENT_DESC;
        */

        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
            { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 3, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };


        D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
        Depth_Desc.DepthEnable	= true;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
            PSO_Desc.pRootSignature        = RS->Library.RS6CBVs4SRVs;
            PSO_Desc.VS                    = DrawRectVShader;
            PSO_Desc.PS                    = DrawRectPShader;
            PSO_Desc.RasterizerState       = Rast_Desc;
            PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            PSO_Desc.SampleMask            = UINT_MAX;
            PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            PSO_Desc.NumRenderTargets      = 4;
            PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM; // Albedo
            PSO_Desc.RTVFormats[1]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // Specular
            PSO_Desc.RTVFormats[2]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // Normal
            PSO_Desc.RTVFormats[3]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // Tangent
            PSO_Desc.SampleDesc.Count      = 1;
            PSO_Desc.SampleDesc.Quality    = 0;
            PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
            PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
            PSO_Desc.DepthStencilState     = Depth_Desc;
            PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
        }

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateGBufferSkinnedPassPSO(RenderSystem* RS)
    {
        auto DrawRectVShader = LoadShader("ForwardSkinned_VS",  "ForwardSkinned_VS",    "vs_5_0",	"assets\\shaders\\forwardRender.hlsl");
        auto DrawRectPShader = LoadShader("GBufferFill_PS",     "GBufferFill_PS",       "ps_5_0",	"assets\\shaders\\forwardRender.hlsl");

        FINALLY
         Release(&DrawRectVShader);
         Release(&DrawRectPShader);
        FINALLYOVER

        /*
        typedef struct D3D12_INPUT_ELEMENT_DESC
        {
        LPCSTR SemanticName;
        UINT SemanticIndex;
        DXGI_FORMAT Format;
        UINT InputSlot;
        UINT AlignedByteOffset;
        D3D12_INPUT_CLASSIFICATION InputSlotClass;
        UINT InstanceDataStepRate;
        } 	D3D12_INPUT_ELEMENT_DESC;
        */

        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
            { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 3, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

            { "BLENDWEIGHT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    4, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "BLENDINDICES",	0, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT,  5, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };


        D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
        Depth_Desc.DepthEnable	= true;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
            PSO_Desc.pRootSignature        = RS->Library.RS6CBVs4SRVs;
            PSO_Desc.VS                    = DrawRectVShader;
            PSO_Desc.PS                    = DrawRectPShader;
            PSO_Desc.RasterizerState       = Rast_Desc;
            PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            PSO_Desc.SampleMask            = UINT_MAX;
            PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            PSO_Desc.NumRenderTargets      = 4;
            PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM; // Albedo
            PSO_Desc.RTVFormats[1]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // Specular
            PSO_Desc.RTVFormats[2]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // Normal
            PSO_Desc.RTVFormats[3]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // Tangent
            PSO_Desc.SampleDesc.Count      = 1;
            PSO_Desc.SampleDesc.Quality    = 0;
            PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
            PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
            PSO_Desc.DepthStencilState     = Depth_Desc;
            PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
        }

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateDeferredShadingPassPSO(RenderSystem* RS)
    {
        auto VShader = LoadShader("ShadingPass_VS",     "DeferredShade_VS",   "vs_5_0",	"assets\\shaders\\deferredRender.hlsl");
        auto PShader = LoadShader("DeferredShade_PS",   "DeferredShade_VS",   "ps_5_0",	"assets\\shaders\\deferredRender.hlsl");

        FINALLY
            Release(&VShader);
            Release(&PShader);
        FINALLYOVER

        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
            { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

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
            PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
            PSO_Desc.DepthStencilState     = Depth_Desc;

            PSO_Desc.BlendState.RenderTarget[0].BlendEnable = true;
            PSO_Desc.BlendState.RenderTarget[0].BlendOp		= D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;

            PSO_Desc.BlendState.RenderTarget[0].DestBlend   = D3D12_BLEND::D3D12_BLEND_ONE;
            PSO_Desc.BlendState.RenderTarget[0].SrcBlend    = D3D12_BLEND::D3D12_BLEND_ONE;

            PSO_Desc.BlendState.RenderTarget[0].SrcBlendAlpha   = D3D12_BLEND::D3D12_BLEND_ONE;
            PSO_Desc.BlendState.RenderTarget[0].DestBlendAlpha  = D3D12_BLEND::D3D12_BLEND_ONE;
        }

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateBilaterialBlurHorizontalPSO(RenderSystem* RS)
    {
        auto VShader = LoadShader("ShadingPass_VS",             "DeferredShade_VS",             "vs_5_0", "assets\\shaders\\deferredRender.hlsl");
        auto PShader = LoadShader("BilateralBlurHorizontal_PS", "BilateralBlurHorizontal_PS",   "ps_5_0", "assets\\shaders\\BilateralBlur.hlsl");

        FINALLY
            Release(&VShader);
            Release(&PShader);
        FINALLYOVER

        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
            { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

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
            PSO_Desc.NumRenderTargets      = 2;
            PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
            PSO_Desc.RTVFormats[1]         = DXGI_FORMAT_R16G16_FLOAT; // backBuffer
            PSO_Desc.SampleDesc.Count      = 1;
            PSO_Desc.SampleDesc.Quality    = 0;
            PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
            PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
            PSO_Desc.DepthStencilState     = Depth_Desc;
        }

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    ID3D12PipelineState* CreateBilaterialBlurVerticalPSO(RenderSystem* RS)
    {
        auto VShader = LoadShader("ShadingPass_VS",             "DeferredShade_VS",         "vs_5_0", "assets\\shaders\\deferredRender.hlsl");
        auto PShader = LoadShader("BilateralBlurVertical_PS",   "BilateralBlurVertical_PS", "ps_5_0", "assets\\shaders\\BilateralBlur.hlsl");

        FINALLY
            Release(&VShader);
            Release(&PShader);
        FINALLYOVER

        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
            { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

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
            PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
            PSO_Desc.DepthStencilState     = Depth_Desc;
        }

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateComputeTiledDeferredPSO(RenderSystem* RS)
    {
        Shader computeShader = LoadShader("csmain", "main", "cs_5_0", "assets\\shaders\\computedeferredtiledshading.hlsl");

        EXITSCOPE(Release(&computeShader));

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
            RS->Library.RSDefault,
            computeShader
        };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

        FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateEnvironmentPassPSO(RenderSystem* RS)
    {
        auto VShader = LoadShader("passthrough_VS", "FORWARDShade_VS", "vs_5_0", "assets\\shaders\\DeferredRender.hlsl");
        auto PShader = LoadShader("environment_PS", "FORWARDShade_VS", "ps_5_0", "assets\\shaders\\DeferredRender.hlsl");

        FINALLY
            Release(&VShader);
            Release(&PShader);
        FINALLYOVER


        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
            { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };


        D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;
        Depth_Desc.DepthEnable	= false;

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
            PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
            PSO_Desc.DepthStencilState     = Depth_Desc;

            PSO_Desc.BlendState.RenderTarget[0].BlendEnable = true;
            PSO_Desc.BlendState.RenderTarget[0].BlendOp     = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;

            PSO_Desc.BlendState.RenderTarget[0].DestBlend   = D3D12_BLEND::D3D12_BLEND_ONE;
            PSO_Desc.BlendState.RenderTarget[0].SrcBlend    = D3D12_BLEND::D3D12_BLEND_ONE;

            PSO_Desc.BlendState.RenderTarget[0].SrcBlendAlpha   = D3D12_BLEND::D3D12_BLEND_ONE;
            PSO_Desc.BlendState.RenderTarget[0].DestBlendAlpha  = D3D12_BLEND::D3D12_BLEND_ONE;
        }

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateForwardDrawInstancedPSO(RenderSystem* RS)
    {
        auto VShader = LoadShader("VMain",		"InstanceForward_VS",	"vs_5_0", "assets\\shaders\\DrawInstancedVShader.hlsl");
        auto PShader = LoadShader("FlatWhite",	"FlatWhite",			"ps_5_0", "assets\\shaders\\forwardRender.hlsl");

        EXITSCOPE(
            Release(&VShader);
            Release(&PShader);
        );

        /*
        typedef struct D3D12_INPUT_ELEMENT_DESC
        {
        LPCSTR SemanticName;
        UINT SemanticIndex;
        DXGI_FORMAT Format;
        UINT InputSlot;
        UINT AlignedByteOffset;
        D3D12_INPUT_CLASSIFICATION InputSlotClass;
        UINT InstanceDataStepRate;
        } 	D3D12_INPUT_ELEMENT_DESC;
        */

        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
                { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TANGENT",    0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 3, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

                { "INSTANCEWT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	4, 0,   D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
                { "INSTANCEWT",	1, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	4, 16,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
                { "INSTANCEWT",	2, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	4, 32,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
                { "INSTANCEWT",	3, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	4, 48,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        };


        D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;
        Depth_Desc.DepthEnable	= true;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
            PSO_Desc.pRootSignature        = RS->Library.RS6CBVs4SRVs;
            PSO_Desc.VS                    = VShader;
            PSO_Desc.PS                    = PShader;
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
            PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
            PSO_Desc.BlendState.RenderTarget[0].DestBlend	= D3D12_BLEND::D3D12_BLEND_DEST_COLOR;
            PSO_Desc.BlendState.RenderTarget[0].SrcBlend	= D3D12_BLEND::D3D12_BLEND_SRC_COLOR;
            PSO_Desc.BlendState.RenderTarget[0].BlendOp		= D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
        }

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateOcclusionDrawPSO(RenderSystem* RS)
    {
        return nullptr;
        auto DrawRectVShader = LoadShader("Forward_VS", "Forward_VS", "vs_5_0",	"assets\\shaders\\forwardRender.hlsl");

        FINALLY
            
        Release(&DrawRectVShader);

        FINALLYOVER

        /*
        typedef struct D3D12_INPUT_ELEMENT_DESC
        {
        LPCSTR SemanticName;
        UINT SemanticIndex;
        DXGI_FORMAT Format;
        UINT InputSlot;
        UINT AlignedByteOffset;
        D3D12_INPUT_CLASSIFICATION InputSlotClass;
        UINT InstanceDataStepRate;
        } 	D3D12_INPUT_ELEMENT_DESC;
        */

        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
            { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };


        D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
        Depth_Desc.DepthEnable	= true;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
            PSO_Desc.pRootSignature        = RS->Library.RS6CBVs4SRVs;
            PSO_Desc.VS                    = DrawRectVShader;
            PSO_Desc.RasterizerState       = Rast_Desc;
            PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            PSO_Desc.SampleMask            = UINT_MAX;
            PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            PSO_Desc.NumRenderTargets      = 0;
            PSO_Desc.SampleDesc.Count      = 1;
            PSO_Desc.SampleDesc.Quality    = 0;
            PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
            PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
            PSO_Desc.DepthStencilState     = Depth_Desc;
        }

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateTexture2CubeMapIrradiancePSO(RenderSystem* RS)
    {
        auto VShader = LoadShader("texture2CubeMap_VS"       , "texture2CubeMap_VS",        "vs_5_0", "assets\\shaders\\texture2Cubemap.hlsl");
        auto GShader = LoadShader("texture2CubeMap_GS"       , "texture2CubeMap_GS",        "gs_5_0", "assets\\shaders\\texture2Cubemap.hlsl");
        auto PShader = LoadShader("texture2CubeMapDiffuse_PS", "texture2CubeMapDiffuse_PS", "ps_5_0", "assets\\shaders\\texture2Cubemap.hlsl");

        FINALLY
            Release(&VShader);
            Release(&PShader);
        FINALLYOVER


        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
            { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };


        D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        Rast_Desc.CullMode                      = D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
        D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;
        Depth_Desc.DepthEnable	= false;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
            PSO_Desc.pRootSignature        = RS->Library.RSDefault;
            PSO_Desc.VS                    = VShader;
            PSO_Desc.GS                    = GShader;
            PSO_Desc.PS                    = PShader;
            PSO_Desc.RasterizerState       = Rast_Desc;
            PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            PSO_Desc.SampleMask            = UINT_MAX;
            PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
            PSO_Desc.NumRenderTargets      = 1;
            PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
            PSO_Desc.SampleDesc.Count      = 1;
            PSO_Desc.SampleDesc.Quality    = 0;
            //PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
            PSO_Desc.DepthStencilState     = Depth_Desc;
        }

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    /************************************************************************************************/

    
    ID3D12PipelineState* CreateTexture2CubeMapGGXPSO(RenderSystem* RS)
    {
        auto VShader = LoadShader("texture2CubeMap_VS",     "texture2CubeMap_VS",       "vs_5_0", "assets\\shaders\\texture2Cubemap.hlsl");
        auto GShader = LoadShader("texture2CubeMap_GS",     "texture2CubeMap_GS",       "gs_5_0", "assets\\shaders\\texture2Cubemap.hlsl");
        auto PShader = LoadShader("texture2CubeMapGGX_PS",  "texture2CubeMapGGX_PS",    "ps_5_0", "assets\\shaders\\texture2Cubemap.hlsl");

        FINALLY
            Release(&VShader);
            Release(&PShader);
        FINALLYOVER


        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
            { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };


        D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        Rast_Desc.CullMode                      = D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
        D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;
        Depth_Desc.DepthEnable	= false;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
            PSO_Desc.pRootSignature        = RS->Library.RSDefault;
            PSO_Desc.VS                    = VShader;
            PSO_Desc.GS                    = GShader;
            PSO_Desc.PS                    = PShader;
            PSO_Desc.RasterizerState       = Rast_Desc;
            PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            PSO_Desc.SampleMask            = UINT_MAX;
            PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
            PSO_Desc.NumRenderTargets      = 1;
            PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
            PSO_Desc.SampleDesc.Count      = 1;
            PSO_Desc.SampleDesc.Quality    = 0;
            //PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
            PSO_Desc.DepthStencilState     = Depth_Desc;
        }

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    /************************************************************************************************/


    DepthPass& WorldRender::DepthPrePass(
        UpdateDispatcher&               dispatcher,
        FrameGraph&                     frameGraph,
        const CameraHandle              camera,
        GatherTask&                     pvs,
        const ResourceHandle            depthBufferTarget,
        ReserveConstantBufferFunction   reserveConsantBufferSpace,
        iAllocator*                     allocator)
    {
        const size_t MaxEntityDrawCount = 1000;

        auto& pass = frameGraph.AddNode<DepthPass>(
                pvs.GetData().solid,
                [&, camera](FrameGraphNodeBuilder& builder, DepthPass& data)
                {
                    const size_t localBufferSize = std::max(sizeof(Camera::ConstantBuffer), sizeof(ForwardDrawConstants));
                    
                    data.entityConstantsBuffer  = std::move(reserveConsantBufferSpace(sizeof(ForwardDrawConstants) * MaxEntityDrawCount));
                    data.passConstantsBuffer    = std::move(reserveConsantBufferSpace(2048));
                    data.depthBufferObject      = builder.WriteDepthBuffer(depthBufferTarget);
                    data.depthPassTarget        = depthBufferTarget;

                    builder.AddDataDependency(pvs);
                },
                [=](DepthPass& data, const FrameResources& resources, Context& ctx, iAllocator& allocator)
                {
                    const auto cameraConstants = ConstantBufferDataSet{ GetCameraConstants(camera), data.passConstantsBuffer };

                    DescriptorHeap heap{
                        ctx,
                        resources.renderSystem.Library.RS6CBVs4SRVs.GetDescHeap(0),
                        &allocator };

                    heap.NullFill(ctx);

                    ctx.SetRootSignature(resources.renderSystem.Library.RS6CBVs4SRVs);
                    ctx.SetPipelineState(resources.GetPipelineState(DEPTHPREPASS));

                    ctx.SetScissorAndViewports({ data.depthPassTarget });
                    ctx.SetRenderTargets(
                        {},
                        true,
                        resources.GetTexture(data.depthBufferObject));

                    ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);
                    ctx.SetGraphicsDescriptorTable(0, heap);
                    ctx.SetGraphicsConstantBufferView(1, cameraConstants);
                    ctx.SetGraphicsConstantBufferView(3, cameraConstants);
                    ctx.NullGraphicsConstantBufferView(6);

                    TriMesh* prevMesh = nullptr;
                    for (const auto& drawable : data.drawables)
                    {
                        auto* const triMesh = GetMeshResource(drawable.D->MeshHandle);
                        if (triMesh != prevMesh)
                        {
                            prevMesh = triMesh;

                            ctx.AddIndexBuffer(triMesh);
                            ctx.AddVertexBuffers(triMesh,
                                {   VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
                                });
                        }

                        auto test = drawable.D->GetConstants();
                        auto constants = ConstantBufferDataSet{ test, data.entityConstantsBuffer };
                        ctx.SetGraphicsConstantBufferView(2, constants);
                        ctx.DrawIndexedInstanced(triMesh->IndexCount);
                    }
                });

        return pass;
    }


    /************************************************************************************************/


    BackgroundEnvironmentPass& WorldRender::BackgroundPass(
        UpdateDispatcher&               dispatcher,
        FrameGraph&                     frameGraph,
        const CameraHandle              camera,
        const ResourceHandle            renderTarget,
        const ResourceHandle            hdrMap,
        ReserveConstantBufferFunction   reserveCB,
        ReserveVertexBufferFunction     reserveVB,
        iAllocator*                     allocator)
    {
        auto& pass = frameGraph.AddNode<BackgroundEnvironmentPass>(
            BackgroundEnvironmentPass{},
            [&](FrameGraphNodeBuilder& builder, BackgroundEnvironmentPass& data)
            {
                const size_t localBufferSize        = std::max(sizeof(Camera::ConstantBuffer), sizeof(ForwardDrawConstants));
                auto& renderSystem                  = frameGraph.GetRenderSystem();

                data.renderTargetObject             = builder.WriteRenderTarget(renderTarget);
                data.passConstants                  = reserveCB(6 * KILOBYTE);
                data.passVertices                   = reserveVB(sizeof(float4) * 6);
                data.diffuseMap                     = hdrMap;
            },
            [=](BackgroundEnvironmentPass& data, const FrameResources& frameResources, Context& ctx, iAllocator& tempAllocator)
            {
                DescriptorHeap descHeap;
                descHeap.Init2(ctx, renderSystem.Library.RSDefault.GetDescHeap(0), 20, &tempAllocator);
                descHeap.SetSRV(ctx, 6, data.diffuseMap);
                descHeap.NullFill(ctx, 20);

                auto& renderSystem          = frameResources.renderSystem;
                const auto WH               = frameResources.renderSystem.GetTextureWH(renderTarget);
                const auto cameraConstants  = GetCameraConstants(camera);

                struct
                {
                    float4 Position;
                }   vertices[] =
                {
                    float4(-1,   1,  1, 1),
                    float4(1,    1,  1, 1),
                    float4(-1,  -1,  1, 1),

                    float4(-1,  -1,  1, 1),
                    float4(1,    1,  1, 1),
                    float4(1,   -1,  1, 1),
                };

                struct
                {
                    float2 WH;
                }passConstants = { float2(WH[0], WH[1]) };

                ctx.SetRootSignature(frameResources.renderSystem.Library.RSDefault);
                ctx.SetPipelineState(frameResources.GetPipelineState(ENVIRONMENTPASS));
                ctx.SetGraphicsDescriptorTable(4, descHeap);

                ctx.SetScissorAndViewports({ renderTarget });
                ctx.SetRenderTargets({ frameResources.GetTexture({ data.renderTargetObject }) }, false, {});
                ctx.SetVertexBuffers({ VertexBufferDataSet{ vertices, data.passVertices } });
                ctx.SetGraphicsConstantBufferView(0, ConstantBufferDataSet{ cameraConstants, data.passConstants });
                ctx.SetGraphicsConstantBufferView(1, ConstantBufferDataSet{ passConstants, data.passConstants });

                ctx.Draw(6);
            });

        return pass;
    }


    /************************************************************************************************/


    BackgroundEnvironmentPass& WorldRender::RenderPBR_IBL_Deferred(
        UpdateDispatcher&               dispatcher,
        FrameGraph&                     frameGraph,
        const SceneDescription&         sceneDescription,
        const CameraHandle              camera,
        const ResourceHandle            renderTarget,
        const ResourceHandle            depthTarget,
        const ResourceHandle            diffuseMap,
        const ResourceHandle            GGXMap,
        GBuffer&                        gbuffer,
        ReserveConstantBufferFunction   reserveCB,
        ReserveVertexBufferFunction     reserveVB,
        float                           t,
        iAllocator*                     allocator)
    {
        auto& pass = frameGraph.AddNode<BackgroundEnvironmentPass>(
            BackgroundEnvironmentPass{},
            [&](FrameGraphNodeBuilder& builder, BackgroundEnvironmentPass& data)
            {
                builder.AddDataDependency(sceneDescription.cameras);

                const size_t localBufferSize    = std::max(sizeof(Camera::ConstantBuffer), sizeof(ForwardDrawConstants));
                auto& renderSystem              = frameGraph.GetRenderSystem();
                data.renderTargetObject         = builder.WriteRenderTarget(renderTarget);

                data.AlbedoTargetObject         = builder.ReadShaderResource(gbuffer.Albedo);
                data.NormalTargetObject         = builder.ReadShaderResource(gbuffer.Normal);
                data.TangentTargetObject        = builder.ReadShaderResource(gbuffer.Tangent);
                data.MRIATargetObject           = builder.ReadShaderResource(gbuffer.MRIA);
                data.depthBufferTargetObject    = builder.ReadShaderResource(depthTarget);

                data.passConstants  = reserveCB(6 * KILOBYTE);
                data.passVertices   = reserveVB(sizeof(float4) * 6);

                data.diffuseMap = diffuseMap;
                data.GGX        = GGXMap;
            },
            [=](BackgroundEnvironmentPass& data, const FrameResources& frameResources, Context& ctx, iAllocator& allocator)
            {
                auto& renderSystem          = frameResources.renderSystem;
                const auto WH               = frameResources.renderSystem.GetTextureWH(renderTarget);
                const auto cameraConstants  = GetCameraConstants(camera);

                struct
                {
                    float4 Position;
                }   vertices[] =
                {
                    float4(-1,   1,  1, 1),
                    float4(1,    1,  1, 1),
                    float4(-1,  -1,  1, 1),

                    float4(-1,  -1,  1, 1),
                    float4(1,    1,  1, 1),
                    float4(1,   -1,  1, 1),
                };


                struct
                {
                    float2 WH;
                    float  t;
                }passConstants = { float2(WH[0], WH[1]), t };

                DescriptorHeap descHeap;
                descHeap.Init2(ctx, renderSystem.Library.RSDefault.GetDescHeap(0), 20, &allocator);

                descHeap.SetSRV(ctx, 0, frameResources.GetTexture(data.AlbedoTargetObject));
                descHeap.SetSRV(ctx, 1, frameResources.GetTexture(data.MRIATargetObject));
                descHeap.SetSRV(ctx, 2, frameResources.GetTexture(data.NormalTargetObject));
                descHeap.SetSRV(ctx, 3, frameResources.GetTexture(data.TangentTargetObject));
                descHeap.SetSRV(ctx, 4, frameResources.GetTexture(data.depthBufferTargetObject), DeviceFormat::R32_FLOAT);
                descHeap.SetSRVCubemap(ctx, 5, data.GGX);
                descHeap.SetSRVCubemap(ctx, 6, data.diffuseMap);
                //descHeap.NullFill(ctx, 20);

                ctx.SetRootSignature(frameResources.renderSystem.Library.RSDefault);
                ctx.SetPipelineState(frameResources.GetPipelineState(ENVIRONMENTPASS));
                ctx.SetGraphicsDescriptorTable(3, descHeap);

                ctx.SetScissorAndViewports({ renderTarget });
                ctx.SetRenderTargets({ frameResources.GetTexture(data.renderTargetObject) }, false);
                ctx.SetVertexBuffers({ VertexBufferDataSet{ vertices, data.passVertices } });
                ctx.SetGraphicsConstantBufferView(0, ConstantBufferDataSet{ cameraConstants, data.passConstants });
                ctx.SetGraphicsConstantBufferView(1, ConstantBufferDataSet{ passConstants, data.passConstants });
                ctx.SetGraphicsConstantBufferView(2, ConstantBufferDataSet{ passConstants, data.passConstants });

                ctx.Draw(6);
            });

        return pass;
    }


    /************************************************************************************************/


    BilateralBlurPass& WorldRender::BilateralBlur(
        FrameGraph&                     frameGraph,
        const ResourceHandle            source,
        const ResourceHandle            temp1,
        const ResourceHandle            temp2,
        const ResourceHandle            temp3,
        const ResourceHandle            destination,
        const ResourceHandle            testImage,
        GBuffer&                        gbuffer,
        const ResourceHandle            depthBuffer,
        ReserveConstantBufferFunction   reserveCB,
        ReserveVertexBufferFunction     reserveVB,
        iAllocator*                     tempMemory)
    {
        auto& pass = frameGraph.AddNode<BilateralBlurPass>(
            BilateralBlurPass
            {
                reserveCB,
                reserveVB
            },
            [&](FrameGraphNodeBuilder& builder, BilateralBlurPass& data)
            {
                data.DestinationObject  = builder.WriteRenderTarget(destination);
                data.TempObject1        = builder.WriteRenderTarget(temp1);
                data.TempObject2        = builder.WriteRenderTarget(temp2); // 2 channel
                data.TempObject3        = builder.WriteRenderTarget(temp3); // 2 channel

                data.DepthSource        = builder.ReadShaderResource(depthBuffer);
                data.NormalSource       = builder.ReadShaderResource(gbuffer.Normal);
                data.Source             = builder.ReadShaderResource(source);
            },
            [=](BilateralBlurPass& data, const FrameResources& frameResources, Context& ctx, iAllocator& allocator)
            {
                auto& renderSystem          = frameResources.renderSystem;
                const auto WH               = frameResources.renderSystem.GetTextureWH(destination);

                auto constantBuffer = data.reserveCB(2048);
                auto vertexBuffer   = data.reserveVB(2048);

                struct
                {
                    float4 Position;
                }   vertices[] =
                {
                    float4(-1,   1,  1, 1),
                    float4(1,    1,  1, 1),
                    float4(-1,  -1,  1, 1),

                    float4(-1,  -1,  1, 1),
                    float4(1,    1,  1, 1),
                    float4(1,   -1,  1, 1),
                };


                struct
                {
                    float2 WH;
                }passConstants = { float2(WH[0], WH[1]) };

                DescriptorHeap descHeap;
                descHeap.Init2(ctx, renderSystem.Library.RSDefault.GetDescHeap(0), 5, &allocator);

                descHeap.SetSRV(ctx, 0, frameResources.GetTexture(data.Source));
                descHeap.SetSRV(ctx, 1, frameResources.GetTexture(data.NormalSource));
                descHeap.SetSRV(ctx, 2, frameResources.GetTexture(data.DepthSource), DeviceFormat::R32_FLOAT);
                descHeap.NullFill(ctx, 3);

                ctx.SetRootSignature(frameResources.renderSystem.Library.RSDefault);
                ctx.SetPipelineState(frameResources.GetPipelineState(BILATERALBLURPASSHORIZONTAL));
                ctx.SetGraphicsDescriptorTable(3, descHeap);

                ctx.SetScissorAndViewports({ destination });
                ctx.SetRenderTargets({ frameResources.GetTexture(data.TempObject1), frameResources.GetTexture(data.TempObject2) }, false);
                ctx.SetVertexBuffers({ VertexBufferDataSet{ vertices, vertexBuffer } });
                ctx.SetGraphicsConstantBufferView(1, ConstantBufferDataSet{ passConstants, constantBuffer });

                ctx.Draw(6);

                DescriptorHeap descHeap2;
                descHeap2.Init2(ctx, renderSystem.Library.RSDefault.GetDescHeap(0), 5, &allocator);

                
                descHeap2.SetSRV(ctx, 0, frameResources.ReadRenderTarget(data.TempObject1, &ctx));
                descHeap2.SetSRV(ctx, 1, frameResources.GetTexture(data.NormalSource));
                descHeap2.SetSRV(ctx, 2, frameResources.GetTexture(data.DepthSource), DeviceFormat::R32_FLOAT);
                descHeap2.SetSRV(ctx, 3, frameResources.ReadRenderTarget(data.TempObject2, &ctx));
                descHeap2.SetSRV(ctx, 4, testImage);

                ctx.SetPipelineState(frameResources.GetPipelineState(BILATERALBLURPASSVERTICAL));
                ctx.SetGraphicsDescriptorTable(3, descHeap2);
                ctx.SetRenderTargets({ frameResources.GetTexture(data.DestinationObject) }, false);
                ctx.Draw(6);
            });

        return pass;
    }


    /************************************************************************************************/


    ForwardPlusPass& WorldRender::RenderPBR_ForwardPlus(
        UpdateDispatcher&			    dispatcher,
        FrameGraph&					    frameGraph, 
        const DepthPass&			    depthPass,
        const CameraHandle			    camera,
        const WorldRender_Targets&	    Targets,
        const SceneDescription&	        desc,
        const float                     t,
        ReserveConstantBufferFunction   reserveCB,
        ResourceHandle                  environmentMap,
        iAllocator*					    allocator)
    {
        const size_t MaxEntityDrawCount = 10000;

        typedef Vector<ForwardPlusPass> ForwardDrawableList;
        

        auto& pass = frameGraph.AddNode<ForwardPlusPass>(
            ForwardPlusPass{
                desc.lights.GetData().pointLights,
                depthPass.drawables,
                depthPass.entityConstantsBuffer },
            [&](FrameGraphNodeBuilder& builder, ForwardPlusPass& data)
            {
                builder.AddDataDependency(desc.PVS);
                builder.AddDataDependency(desc.cameras);

                data.BackBuffer			    = builder.WriteRenderTarget	(Targets.RenderTarget);
                data.DepthBuffer            = builder.WriteDepthBuffer (Targets.DepthTarget);
                size_t localBufferSize      = std::max(sizeof(Camera::ConstantBuffer), sizeof(ForwardDrawConstants));

                data.passConstantsBuffer    = std::move(reserveCB(localBufferSize * 2));

                data.lightLists			    = builder.ReadWriteUAV(lightLists,			DRS_ShaderResource);
                data.pointLightBuffer	    = builder.ReadWriteUAV(pointLightBuffer,	DRS_ShaderResource);

                data.WH                     = lightMapWH;
            },
            [=](ForwardPlusPass& data, const FrameResources& resources, Context& ctx, iAllocator& allocator)
            {
                const auto cameraConstants  = ConstantBufferDataSet{ GetCameraConstants(camera), data.passConstantsBuffer };
                const auto passConstants    = ConstantBufferDataSet{ ForwardDrawConstants{ (float)data.pointLights.size(), t, data.WH }, data.passConstantsBuffer };

                DescriptorHeap descHeap;
                descHeap.Init(
                    ctx,
                    resources.renderSystem.Library.RS6CBVs4SRVs.GetDescHeap(0),
                    &allocator);

                descHeap.SetSRV(ctx, 1, lightLists);
                descHeap.SetSRV(ctx, 2, pointLightBuffer);
                descHeap.NullFill(ctx);

                ctx.SetRootSignature(resources.renderSystem.Library.RS6CBVs4SRVs);
                ctx.SetPipelineState(resources.GetPipelineState(FORWARDDRAW));

                // Setup Initial Shading State
                ctx.SetScissorAndViewports({Targets.RenderTarget});
                ctx.SetRenderTargets(
                    { resources.GetTexture(data.BackBuffer) },
                    true,
                    resources.GetTexture(data.DepthBuffer));

                ctx.SetPrimitiveTopology			(EInputTopology::EIT_TRIANGLE);
                ctx.SetGraphicsDescriptorTable		(0, descHeap);
                ctx.SetGraphicsConstantBufferView	(1, cameraConstants);
                ctx.SetGraphicsConstantBufferView	(3, passConstants);
                ctx.NullGraphicsConstantBufferView	(6);

                TriMesh* prevMesh = nullptr;

                for (size_t itr = 0; itr < data.drawables.size(); ++itr)
                {
                    auto& drawable = data.drawables[itr];
                    TriMesh* triMesh = GetMeshResource(drawable.D->MeshHandle);

                    if (triMesh != prevMesh)
                    {
                        prevMesh = triMesh;

                        ctx.AddIndexBuffer(triMesh);
                        ctx.AddVertexBuffers(triMesh,
                            {	VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,
                            });
                    }

                    auto proxy = CreateCBIterator<decltype(drawable.D->GetConstants())>(data.entityConstants)[itr];

                    ctx.SetGraphicsConstantBufferView(2, proxy);
                    ctx.DrawIndexedInstanced(triMesh->IndexCount, 0, 0, 1, 0);
                }
            });

        return pass;
    }


    /************************************************************************************************/


    LightBufferUpdate& WorldRender::UpdateLightBuffers(
        UpdateDispatcher&		        dispatcher,
        FrameGraph&				        graph,
        const CameraHandle	            camera,
        const GraphicScene&	            scene,
        const SceneDescription&         sceneDescription,
        ReserveConstantBufferFunction   reserveCB,
        iAllocator*				        tempMemory, 
        LighBufferDebugDraw*	        drawDebug)
    {
        graph.Resources.AddUAVResource(lightLists,			0, graph.GetRenderSystem().GetObjectState(lightLists));
        graph.Resources.AddUAVResource(pointLightBuffer,	0, graph.GetRenderSystem().GetObjectState(pointLightBuffer));

        auto& lightBufferData = graph.AddNode<LightBufferUpdate>(
            LightBufferUpdate{
                    Vector<GPUPointLight>(tempMemory, 1024),
                    &sceneDescription.lights.GetData().pointLights,
                    ReserveUploadBuffer(graph.GetRenderSystem(), 1024 * sizeof(GPUPointLight)),// max point light count of 1024
                    camera,
                    reserveCB(2 * KILOBYTE),
            },
            [&, this](FrameGraphNodeBuilder& builder, LightBufferUpdate& data)
            {
                auto& renderSystem      = graph.GetRenderSystem();
                data.lightListObject	= builder.ReadWriteUAV(lightLists,		 DRS_UAV);
                data.lightBufferObject	= builder.ReadWriteUAV(pointLightBuffer, DRS_Write);
                data.camera				= camera;


                builder.AddDataDependency(sceneDescription.lights);
                builder.AddDataDependency(sceneDescription.cameras);
            },
            [XY = lightMapWH](LightBufferUpdate& data, FrameResources& resources, Context& ctx, iAllocator& allocator)
            {
                const auto cameraConstants = GetCameraConstants(data.camera);

                struct ConstantsLayout
                {
                    float4x4 iproj;
                    float4x4 view;
                    uint2    LightMapWidthHeight;
                    uint32_t lightCount;
                }constantsValues = {
                    XMMatrixToFloat4x4(DirectX::XMMatrixInverse(nullptr, Float4x4ToXMMATIRX(cameraConstants.Proj))),
                    cameraConstants.View,
                    XY,
                    (uint32_t)data.pointLightHandles->size()
                };

                ConstantBufferDataSet constants{ constantsValues, data.constants };
                PointLightComponent& pointLights = PointLightComponent::GetComponent();

                for (const auto light : *data.pointLightHandles)
                {
                    PointLight pointLight	= pointLights[light];
                    float3 position			= GetPositionW(pointLight.Position);

                    data.pointLights.push_back(
                        {	{ pointLight.K, pointLight.I	},
                            { position, pointLight.R        } });
                }

                const size_t uploadSize = data.pointLights.size() * sizeof(GPUPointLight);
                MoveBuffer2UploadBuffer(data.lightBuffer, (byte*)data.pointLights.begin(), uploadSize);
                ctx.CopyBuffer(data.lightBuffer, uploadSize, resources.WriteUAV(data.lightBufferObject, &ctx));

                DescriptorHeap descHeap;
                descHeap.Init(ctx, resources.renderSystem.Library.ComputeSignature.GetDescHeap(0), &allocator);
                descHeap.SetUAV(ctx, 1, resources.GetUAVBufferResource	(data.lightListObject));
                descHeap.SetSRV(ctx, 2, resources.ReadUAVBuffer			(data.lightBufferObject, DRS_ShaderResource, &ctx));
                descHeap.SetCBV(ctx, 6, constants.Handle(), constants.Offset(), sizeof(ConstantsLayout));
                descHeap.NullFill(ctx);

                ctx.SetPipelineState(resources.GetPipelineState(LIGHTPREPASS));
                ctx.SetComputeRootSignature(resources.renderSystem.Library.ComputeSignature);
                ctx.SetComputeDescriptorTable(0, descHeap);

                ctx.Dispatch({ XY[0], XY[1], 1 });
            });

        return lightBufferData;
    }


    /************************************************************************************************/


    GBufferPass& WorldRender::RenderPBR_GBufferPass(
        UpdateDispatcher&               dispatcher,
        FrameGraph&                     frameGraph,
        const SceneDescription&         sceneDescription,
        const CameraHandle              camera,
        GBuffer&                        gbuffer,
        ResourceHandle                  depthTarget,
        ReserveConstantBufferFunction   reserveCB,
        iAllocator*                     allocator)
    {
        constexpr size_t MaxEntityDrawCount = 10000;
        constexpr size_t localBufferSize    = std::max(sizeof(Camera::ConstantBuffer), sizeof(ForwardDrawConstants));


        auto& pass = frameGraph.AddNode<GBufferPass>(
            GBufferPass{
                gbuffer,
                sceneDescription.PVS.GetData().solid,
                sceneDescription.skinned.GetData().skinned,
                reserveCB
            },
            [&](FrameGraphNodeBuilder& builder, GBufferPass& data)
            {
                builder.AddDataDependency(sceneDescription.cameras);
                builder.AddDataDependency(sceneDescription.PVS);
                builder.AddDataDependency(sceneDescription.skinned);

                data.AlbedoTargetObject      = builder.WriteRenderTarget(gbuffer.Albedo);
                data.MRIATargetObject        = builder.WriteRenderTarget(gbuffer.MRIA);
                data.NormalTargetObject      = builder.WriteRenderTarget(gbuffer.Normal);
                data.TangentTargetObject     = builder.WriteRenderTarget(gbuffer.Tangent);
                data.depthBufferTargetObject = builder.WriteDepthBuffer(depthTarget);
            },
            [camera](GBufferPass& data, FrameResources& resources, Context& ctx, iAllocator& allocator)
            {
                auto passConstantBuffer   = data.reserveCB(1024);
                auto entityConstantBuffer = data.reserveCB((data.pvs.size() + data.skinned.size()) * 1024);
                auto poseBuffer           = data.reserveCB(data.skinned.size() * sizeof(float4x4[128]));

                const auto cameraConstants  = ConstantBufferDataSet{ GetCameraConstants(camera), passConstantBuffer };
                const auto passConstants    = ConstantBufferDataSet{ ForwardDrawConstants{ 1, 1 }, passConstantBuffer };

                DescriptorHeap descHeap;
                descHeap.Init(
                    ctx,
                    resources.renderSystem.Library.RS6CBVs4SRVs.GetDescHeap(0),
                    &allocator);
                descHeap.NullFill(ctx);

                ctx.SetRootSignature(resources.renderSystem.Library.RS6CBVs4SRVs);
                ctx.SetPipelineState(resources.GetPipelineState(GBUFFERPASS));
                ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLELIST);

                // Setup pipeline resources
                ctx.SetScissorAndViewports(
                    std::tuple{
                        data.gbuffer.Albedo,
                        data.gbuffer.MRIA,
                        data.gbuffer.Normal,
                        data.gbuffer.Tangent
                    });

                RenderTargetList renderTargets = {
                    resources.GetTexture(data.AlbedoTargetObject),
                    resources.GetTexture(data.MRIATargetObject),
                    resources.GetTexture(data.NormalTargetObject),
                    resources.GetTexture(data.TangentTargetObject),
                };

                ctx.SetGraphicsDescriptorTable(0, descHeap);

                ctx.SetRenderTargets(
                    renderTargets,
                    true,
                    resources.GetTexture(data.depthBufferTargetObject));

                // Setup Constants
                ctx.SetGraphicsConstantBufferView(1, cameraConstants);
                ctx.SetGraphicsConstantBufferView(3, passConstants);

                // submit draw calls
                TriMesh* prevMesh = nullptr;


                // unskinned models
                for (auto& drawable : data.pvs)
                {
                    const auto constants    = drawable.D->GetConstants();
                    auto* triMesh           = GetMeshResource(drawable.D->MeshHandle);

                    if (triMesh != prevMesh)
                    {
                        prevMesh = triMesh;

                        ctx.AddIndexBuffer(triMesh);
                        ctx.AddVertexBuffers(
                            triMesh,
                            {
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,
                            }
                        );
                    }

                    ctx.SetGraphicsConstantBufferView(2, ConstantBufferDataSet(constants, entityConstantBuffer));
                    ctx.DrawIndexed(triMesh->IndexCount);
                }

                // skinned models
                ctx.SetPipelineState(resources.GetPipelineState(GBUFFERPASS_SKINNED));

                struct EntityPoses
                {
                    float4x4 transforms[128];

                    auto& operator [](size_t idx)
                    {
                        return transforms[idx];
                    }
                };

                auto& poses = allocator.allocate<EntityPoses>();

                for (const auto& skinnedDraw : data.skinned)
                {
                    const auto constants    = skinnedDraw.drawable->GetConstants();
                    auto* triMesh           = GetMeshResource(skinnedDraw.drawable->MeshHandle);

                    if (triMesh != prevMesh)
                    {
                        prevMesh = triMesh;

                        ctx.AddIndexBuffer(triMesh);
                        ctx.AddVertexBuffers(
                            triMesh,
                            {
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2,
                            }
                        );
                    }

                    auto pose       = skinnedDraw.pose;
                    auto skeleton   = pose->Sk;

                    for(size_t I = 0; I < pose->JointCount; I++)
                        poses[I] = XMMatrixToFloat4x4(skeleton->IPose[I] * pose->CurrentPose[I]);

                    ctx.SetGraphicsConstantBufferView(2, ConstantBufferDataSet(constants, entityConstantBuffer));
                    ctx.SetGraphicsConstantBufferView(4, ConstantBufferDataSet(poses, poseBuffer));
                    ctx.DrawIndexed(triMesh->IndexCount);
                }
            }
            );

        return pass;
    }



    /************************************************************************************************/


    TiledDeferredShade& WorldRender::RenderPBR_DeferredShade(
        UpdateDispatcher&               dispatcher,
        FrameGraph&                     frameGraph,
        const SceneDescription&         sceneDescription,
        PointLightGatherTask&           gather,
        GBuffer&                        gbuffer,
        ResourceHandle                  depthTarget,
        ResourceHandle                  renderTarget,
        ResourceHandle                  GGXSpecularMap,
        ResourceHandle                  diffuseMap,
        ReserveConstantBufferFunction   reserveCB,
        ReserveVertexBufferFunction     reserveVB,
        float                           t,
        iAllocator*                     allocator)
    {
        struct PointLight
        {
            float4 KI;	// Color + intensity in W
            float4 PR;	// XYZ + radius in W
        };

        frameGraph.Resources.AddUAVResource(pointLightBuffer,	0, frameGraph.GetRenderSystem().GetObjectState(pointLightBuffer));

        auto& pass = frameGraph.AddNode<TiledDeferredShade>(
            TiledDeferredShade{
                gbuffer,
                gather,
                ReserveUploadBuffer(frameGraph.GetRenderSystem(), 1024 * sizeof(GPUPointLight)),// max point light count of 1024
            },
            [&](FrameGraphNodeBuilder& builder, TiledDeferredShade& data)
            {
                builder.AddDataDependency(sceneDescription.cameras);

                auto& renderSystem = frameGraph.GetRenderSystem();

                data.pointLights                = allocator;
                data.pointLights.reserve(1024);

                data.pointLightHandles          = &sceneDescription.lights.GetData().pointLights;

                data.AlbedoTargetObject         = builder.ReadShaderResource(gbuffer.Albedo);
                data.NormalTargetObject         = builder.ReadShaderResource(gbuffer.Normal);
                data.TangentTargetObject        = builder.ReadShaderResource(gbuffer.Tangent);
                data.MRIATargetObject           = builder.ReadShaderResource(gbuffer.MRIA);
                data.depthBufferTargetObject    = builder.ReadShaderResource(depthTarget);
                data.renderTargetObject         = builder.WriteRenderTarget(renderTarget);

                data.pointLightBufferObject     = builder.ReadWriteUAV(pointLightBuffer, DRS_Write);

                data.passConstants = reserveCB(6 * KILOBYTE);
                data.passVertices  = reserveVB(sizeof(float4) * 6);
            },
            [camera = sceneDescription.camera, renderTarget, diffuseMap, GGXSpecularMap, t]
            (TiledDeferredShade& data, FrameResources& resources, Context& ctx, iAllocator& allocator)
            {
                PointLightComponent& pointLights = PointLightComponent::GetComponent();

                for (const auto light : *data.pointLightHandles)
                {
                    auto    pointLight	= pointLights[light];
                    float3  position	= GetPositionW(pointLight.Position);

                    data.pointLights.push_back(
                        {	{ pointLight.K, 100	},
                            { position, 2000    } });
                }

                const size_t uploadSize = data.pointLights.size() * sizeof(GPUPointLight);
                MoveBuffer2UploadBuffer(data.lightBuffer, (byte*)data.pointLights.begin(), uploadSize);
                ctx.CopyBuffer(data.lightBuffer, uploadSize, resources.WriteUAV(data.pointLightBufferObject, &ctx));

                auto& renderSystem          = resources.renderSystem;
                const auto WH               = resources.renderSystem.GetTextureWH(renderTarget);
                const auto cameraConstants  = GetCameraConstants(camera);


                struct
                {
                    float2  WH;
                    float   time;
                    float   lightCount;
                }passConstants = { {(float)WH[0], (float)WH[1]}, t, (float)data.pointLights.size() };


                struct
                {
                    float4 Position;
                }   vertices[] =
                {
                    float4(-1,   1, 0, 1),
                    float4( 1,   1, 0, 1),
                    float4(-1,  -1, 0, 1),

                    float4(-1,  -1, 0, 1),
                    float4( 1,   1, 0, 1),
                    float4( 1,  -1, 0, 1),
                };


                DescriptorHeap descHeap;
                descHeap.Init2(ctx, resources.renderSystem.Library.RSDefault.GetDescHeap(0), 20, &allocator);
                descHeap.SetSRV(ctx, 0, resources.GetTexture(data.AlbedoTargetObject));
                descHeap.SetSRV(ctx, 1, resources.GetTexture(data.MRIATargetObject));
                descHeap.SetSRV(ctx, 2, resources.GetTexture(data.NormalTargetObject));
                descHeap.SetSRV(ctx, 3, resources.GetTexture(data.TangentTargetObject));
                descHeap.SetSRV(ctx, 4, resources.GetTexture(data.depthBufferTargetObject), DeviceFormat::R32_FLOAT);
                descHeap.SetSRV(ctx, 5, GGXSpecularMap);
                descHeap.SetSRV(ctx, 6, diffuseMap);
                descHeap.SetSRV(ctx, 7, resources.ReadUAVBuffer(data.pointLightBufferObject, DRS_ShaderResource, &ctx));
                descHeap.NullFill(ctx, 20);

                ctx.SetRootSignature(resources.renderSystem.Library.RSDefault);
                ctx.SetPipelineState(resources.GetPipelineState(SHADINGPASS));

                ctx.SetScissorAndViewports({ renderTarget });
                ctx.SetRenderTargets({ resources.GetTexture({ data.renderTargetObject })}, false, {});
                ctx.SetVertexBuffers({ VertexBufferDataSet{ vertices, data.passVertices } });
                ctx.SetGraphicsConstantBufferView(0, ConstantBufferDataSet{ cameraConstants, data.passConstants });
                ctx.SetGraphicsConstantBufferView(1, ConstantBufferDataSet{ passConstants, data.passConstants });
                ctx.SetGraphicsDescriptorTable(4, descHeap);

                ctx.Draw(6);
            });

        return pass;
    }


    /************************************************************************************************/


    ComputeTiledPass& WorldRender::RenderPBR_ComputeDeferredTiledShade(
            UpdateDispatcher&                       dispatcher,
            FrameGraph&                             frameGraph,
            ReserveConstantBufferFunction&          constantBufferAllocator,
            const ComputeTiledDeferredShadeDesc&    scene)
    {


        frameGraph.Resources.AddUAVResource(tempBuffer, 0, frameGraph.GetRenderSystem().GetObjectState(tempBuffer));


        auto& pass = frameGraph.AddNode<ComputeTiledPass>(
            ComputeTiledPass{
                constantBufferAllocator,
                scene.pointLightGather
            },
            [&](FrameGraphNodeBuilder& builder, ComputeTiledPass& data)
            {
                data.dispatchDims   = { lightMapWH[0], lightMapWH[1], 1 };
                data.activeCamera   = scene.activeCamera;
                data.WH             = lightMapWH * 10;
                // Inputs
                data.albedoObject         = builder.ReadShaderResource(scene.gbuffer.Albedo);
                data.MRIAObject           = builder.ReadShaderResource(scene.gbuffer.MRIA);
                data.normalObject         = builder.ReadShaderResource(scene.gbuffer.Normal);
                data.tangentObject        = builder.ReadShaderResource(scene.gbuffer.Tangent);
                data.depthBufferObject    = builder.ReadShaderResource(scene.depthTarget);

                data.lightBitBucketObject   = builder.ReadWriteUAV(lightLists,          DRS_ShaderResource);
                data.lightBuffer            = builder.ReadWriteUAV(pointLightBuffer,    DRS_ShaderResource);

                // Ouputs
                data.tempBufferObject   = builder.ReadWriteUAV(tempBuffer);
                data.renderTargetObject = builder.WriteRenderTarget(scene.renderTarget);
            },
            [=]
            (ComputeTiledPass& data, FrameResources& resources, Context& ctx, iAllocator& allocator)
            {
                CBPushBuffer pushBuffer{ data.constantBufferAllocator(2048) };

                ConstantBufferDataSet cameraConstants{
                    CameraComponent::GetComponent().GetCamera(data.activeCamera).GetConstants(),
                    pushBuffer
                };

                struct LocalPassConstants
                {
                    uint32_t    lightCount;
                    uint2       WH;
                };

                ConstantBufferDataSet localConstants{
                    LocalPassConstants{
                        (uint32_t)data.pointLights.GetData().pointLights.size(),
                        data.WH
                    },
                    pushBuffer
                };

                PointLightComponent& pointLights = PointLightComponent::GetComponent();
                DescriptorHeap srvHeap;
                srvHeap.Init2(ctx, resources.renderSystem.Library.RSDefault.GetDescHeap(0), 7, &allocator);
                srvHeap.SetSRV(ctx, 0, resources.GetTexture(data.albedoObject));
                srvHeap.SetSRV(ctx, 1, resources.GetTexture(data.MRIAObject));
                srvHeap.SetSRV(ctx, 2, resources.GetTexture(data.normalObject));
                srvHeap.SetSRV(ctx, 3, resources.GetTexture(data.tangentObject));
                srvHeap.SetSRV(ctx, 4, resources.GetTexture(data.depthBufferObject), DeviceFormat::R32_FLOAT);
                srvHeap.SetSRV(ctx, 5, lightLists);
                srvHeap.SetSRV(ctx, 6, pointLightBuffer);

                DescriptorHeap uavHeap;
                uavHeap.Init2(ctx, resources.renderSystem.Library.RSDefault.GetDescHeap(1), 10, &allocator);
                uavHeap.SetUAV(ctx, 0, resources.GetUAVTextureResource(data.tempBufferObject));

                ctx.SetComputeRootSignature(resources.renderSystem.Library.RSDefault);
                ctx.SetPipelineState(resources.GetPipelineState(COMPUTETILEDSHADINGPASS));

                ctx.SetComputeConstantBufferView(0, cameraConstants);
                ctx.SetComputeConstantBufferView(1, localConstants);
                ctx.SetComputeDescriptorTable(3, srvHeap);
                ctx.SetComputeDescriptorTable(4, uavHeap);
                ctx.Dispatch(data.dispatchDims);

                ctx.CopyTexture2D(
                    resources.CopyToTexture(data.renderTargetObject, ctx),
                    resources.CopyUAVTexture(data.tempBufferObject, ctx));
            });

        return pass;
    }


    /************************************************************************************************/



    ID3D12PipelineState* CreateTextureFeedbackPSO(RenderSystem* RS)
    {
        auto VShader = LoadShader("Forward_VS",         "Forward_VS",           "vs_5_0", "assets\\shaders\\forwardRender.hlsl");
        auto PShader = LoadShader("TextureFeedback_PS", "TextureFeedback_PS",   "ps_5_0", "assets\\shaders\\TextureFeedback.hlsl");

        FINALLY
            
        Release(&VShader);
        Release(&PShader);

        FINALLYOVER

        /*
        typedef struct D3D12_INPUT_ELEMENT_DESC
        {
        LPCSTR SemanticName;
        UINT SemanticIndex;
        DXGI_FORMAT Format;
        UINT InputSlot;
        UINT AlignedByteOffset;
        D3D12_INPUT_CLASSIFICATION InputSlotClass;
        UINT InstanceDataStepRate;
        } 	D3D12_INPUT_ELEMENT_DESC;
        */

        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
                { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 3, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };


        D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);    // enable conservative rast
        Rast_Desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;

        D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // Disable depth 
        //Depth_Desc.DepthEnable      = true;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
            PSO_Desc.pRootSignature        = RS->Library.RSDefault;
            PSO_Desc.VS                    = VShader;
            PSO_Desc.PS                    = PShader;
            PSO_Desc.RasterizerState       = Rast_Desc;
            PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            PSO_Desc.SampleMask            = UINT_MAX;
            PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            PSO_Desc.NumRenderTargets      = 0;
            PSO_Desc.SampleDesc.Count      = 1;
            PSO_Desc.SampleDesc.Quality    = 0;
            PSO_Desc.DSVFormat             = DXGI_FORMAT_UNKNOWN;
            //PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
            PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
            PSO_Desc.DepthStencilState     = Depth_Desc;
        }


        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    template<typename TY>
    size_t GetConstantsAlignedSize()
    {
        const auto unalignedSize    = sizeof(TY);
        const auto offset           = unalignedSize & (256 + 1);
        const auto adjustedOffset   = offset != 0 ? 256 - offset : 0;

        return unalignedSize + adjustedOffset;
    }

    void TextureStreamingEngine::TextureFeedbackPass(
            UpdateDispatcher&               dispatcher,
            FrameGraph&                     frameGraph,
            CameraHandle                    camera,
            const SceneDescription&         sceneDescription,
            ResourceHandle                  testTexture,
            ReserveConstantBufferFunction&  constantBufferAllocator)
    {
        if (updateInProgress)
            return;

        updateInProgress = true;

        frameGraph.Resources.AddUAVResource(feedbackBuffer, 0, frameGraph.Resources.renderSystem.GetObjectState(feedbackBuffer));
        //frameGraph.Resources.AddUAVResource(feedbackLists, 0, frameGraph.Resources.renderSystem.GetObjectState(feedbackLists));
        frameGraph.Resources.AddUAVResource(feedbackCounters, 0, frameGraph.Resources.renderSystem.GetObjectState(feedbackCounters));
        //frameGraph.Resources.AddRenderTarget(feedbackTarget, 0, frameGraph.Resources.renderSystem.GetObjectState(feedbackTarget));
        frameGraph.Resources.AddDepthBuffer(feedbackDepth, 0, frameGraph.Resources.renderSystem.GetObjectState(feedbackDepth));

        frameGraph.AddNode<TextureFeedbackPass_Data>(
            TextureFeedbackPass_Data{
                camera,
                sceneDescription.PVS,
                constantBufferAllocator
            },
            [&](FrameGraphNodeBuilder& nodeBuilder, TextureFeedbackPass_Data& data)
            {
                data.feedbackBuffer     = nodeBuilder.ReadWriteUAV(feedbackBuffer);
                data.feedbackCounters   = nodeBuilder.ReadWriteUAV(feedbackCounters);
                //data.feedbackLists      = nodeBuilder.ReadWriteUAV(feedbackLists);
                data.feedbackDepth      = nodeBuilder.WriteDepthBuffer(feedbackDepth);
                //data.feedbackTarget     = nodeBuilder.WriteRenderTarget(feedbackTarget);
            },
            [=](TextureFeedbackPass_Data& data, FrameResources& resources, Context& ctx, iAllocator& allocator)
            {
                const size_t entityBufferSize = GetConstantsAlignedSize<Drawable::VConstantsLayout>();

                CBPushBuffer entityConstantBuffer   { data.constantBufferAllocator(entityBufferSize * data.pvs.GetData().solid.size()) };
                CBPushBuffer passContantBuffer      { data.constantBufferAllocator(4096) };

                ctx.SetRootSignature(resources.renderSystem.Library.RSDefault);
                ctx.SetPipelineState(resources.GetPipelineState(TEXTUREFEEDBACK));

                //ctx.ClearUAVTexture(resources.ReadWriteUAVTexture(data.feedbackLists, &ctx), uint4{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff });

                ctx.SetScissorAndViewports({ resources.GetRenderTarget(data.feedbackTarget) });
                /*
                ctx.SetRenderTargets(
                    { resources.GetRenderTarget(data.feedbackTarget) },
                    false, resources.GetRenderTarget(data.feedbackDepth));
                */

                struct constantBufferLayout
                {
                    uint32_t    textureHandles[16];
                    uint2       blockSizes[16];

                    uint32_t    zeroBlock[8];
                } constants =
                {
                    { testTexture },
                    { { 256, 256 }}
                };

                auto passConstants = ConstantBufferDataSet{ constants, passContantBuffer };

                ctx.CopyBufferRegion(
                    { resources.GetObjectResource(passConstants.Handle()) },
                    { passConstants.Offset() + ((size_t)&constants.zeroBlock - (size_t)&constants) },
                    { resources.GetObjectResource(resources.CopyToUAV(data.feedbackCounters, ctx)) },
                    { 0 },
                    { 8 },
                    { resources.GetObjectState(data.feedbackCounters) },
                    { DRS_Write }
                );

                DescriptorHeap uavHeap;
                uavHeap.Init2(ctx, resources.renderSystem.Library.RSDefault.GetDescHeap(1), 4, &allocator);
                //uavHeap.SetUAV(ctx, 1, resources.GetUAVTextureResource(data.feedbackLists));
                uavHeap.SetUAV(ctx, 0, resources.WriteUAV(data.feedbackCounters, &ctx));
                uavHeap.SetUAV(ctx, 1, resources.GetUAVBufferResource(data.feedbackBuffer));

                DescriptorHeap srvHeap;
                srvHeap.Init2(ctx, resources.renderSystem.Library.RSDefault.GetDescHeap(0), 4, &allocator);
                srvHeap.SetSRV(ctx, 0, testTexture);

                ctx.SetGraphicsDescriptorTable(3, srvHeap);
                ctx.SetGraphicsDescriptorTable(4, uavHeap);

                ctx.SetGraphicsConstantBufferView(0, ConstantBufferDataSet{ GetCameraConstants(camera), passContantBuffer });
                ctx.SetGraphicsConstantBufferView(2, passConstants);

                TriMesh* prevMesh = nullptr;

                //for (auto& visable : data.pvs.GetData().solid)
                for (size_t I = 0; I < data.pvs.GetData().solid.size() && I < 5; ++I)
                {
                    auto& visable = data.pvs.GetData().solid[I];
                    auto* triMesh = GetMeshResource(visable.D->MeshHandle);

                    visable.D->GetConstants();

                    if (triMesh != prevMesh)
                    {
                        prevMesh = triMesh;

                        ctx.AddIndexBuffer(triMesh);
                        ctx.AddVertexBuffers(
                            triMesh,
                            {
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2,
                            }
                        );
                    }

                    ctx.SetGraphicsConstantBufferView(1, ConstantBufferDataSet{ visable.D->GetConstants(), entityConstantBuffer });
                    ctx.DrawIndexed(triMesh->IndexCount);
                }

                ctx.CopyBufferRegion(
                    {   resources.GetObjectResource(resources.ReadUAVBuffer(data.feedbackCounters, DRS_Read, &ctx)) ,
                        resources.GetObjectResource(resources.ReadUAVBuffer(data.feedbackBuffer, DRS_Read, &ctx)) },
                    { 0, 0 },
                    {   resources.GetObjectResource(feedbackReturnBuffer[readBackBlock]),
                        resources.GetObjectResource(feedbackReturnBuffer[readBackBlock]) },
                    { 0, 64 },
                    { 8, MEGABYTE / 2 },
                    { DRS_Write, DRS_Write },
                    { DRS_Write, DRS_Write }
                );

                resources.renderSystem.LockReadBack(
                    feedbackReturnBuffer[readBackBlock],
                    resources.renderSystem.GetCurrentFrame() + 2);
            });

        readBackBlock = readBackBlock < 2 ? readBackBlock + 1 : 0;
    }


    /************************************************************************************************/


    TextureStreamingEngine::TextureBlockUpdate::TextureBlockUpdate(TextureStreamingEngine& IN_textureStreamEngine, char* buffer, iAllocator* IN_allocator) :
            iWork               { IN_allocator              },
            allocator           { IN_allocator              },
            textureStreamEngine { IN_textureStreamEngine    }
        {
            memcpy(&requestCount, buffer, sizeof(uint64_t));

            if (requestCount)
            {
                requests = reinterpret_cast<TileRequest*>(IN_allocator->malloc(requestCount));

                memcpy(requests, buffer + 64, requestCount * 8);
            }
        }


    /************************************************************************************************/


    void TextureStreamingEngine::TextureBlockUpdate::Run()
    {
        if (!requests || !requestCount)
            return;

        auto lg_comparitor = [](TileRequest& lhs, TileRequest& rhs) -> bool
        {
            return lhs.GetSortingID() < rhs.GetSortingID();
        };

        auto eq_comparitor = [](TileRequest& lhs, TileRequest& rhs) -> bool
        {
            return lhs.GetSortingID() == rhs.GetSortingID();
        };

        const auto localRequestCount = requestCount;

        std::sort(
            requests,
            requests + localRequestCount,
            lg_comparitor);

        const auto end            = std::unique(requests, requests + localRequestCount, eq_comparitor);
        const auto uniqueCount    = (size_t)(end - requests);

        BlockList requestList{ allocator };
        requestList.reserve(uniqueCount);

        for (auto itr = requests; itr < end; ++itr)
        {
            auto texture = ResourceHandle(itr->TextureID);
            auto x = itr->tileID.GetTileX();
            auto y = itr->tileID.GetTileY();

            if (auto res = textureStreamEngine.AllocateTile(itr->tileID, texture); res)
                requestList.push_back(res.value());
        }

        allocator->free(requests);
        textureStreamEngine.PostUpdatedTiles(requestList);
    }


    /************************************************************************************************/


    void TextureStreamingEngine::BindAsset(const AssetHandle textureAsset, const ResourceHandle  resource)
    {
        auto comparator =
            [](const MappedAsset& lhs, const MappedAsset& rhs)
            {
                return lhs.GetResourceID() < rhs.GetResourceID();
            };

        auto res = std::lower_bound(
            std::begin(mappedAssets),
            std::end(mappedAssets),
            MappedAsset{ resource },
            comparator);

        if (res == std::end(mappedAssets) || res->resource != resource)
            mappedAssets.insert(res, MappedAsset{ resource, textureAsset });
        else
            res->textureAsset = textureAsset;
    }


    /************************************************************************************************/


    std::optional<AssetHandle>   TextureStreamingEngine::GetResourceAsset(const ResourceHandle  resource) const
    {
        auto comparator =
            [](const MappedAsset& lhs, const MappedAsset& rhs)
            {
                return lhs.GetResourceID() < rhs.GetResourceID();
            };

        auto res = std::lower_bound(
            std::begin(mappedAssets),
            std::end(mappedAssets),
            MappedAsset{ resource },
            comparator);

        return (res != std::end(mappedAssets) && res->GetResourceID() == resource) ?
            std::optional<AssetHandle>{ res->textureAsset } :
            std::optional<AssetHandle>{};
    }


    /************************************************************************************************/


    void TextureStreamingEngine::PostUpdatedTiles(const BlockList& blocks)
    {
        auto ctxHandle      = renderSystem.OpenUploadQueue();
        auto& ctx           = renderSystem._GetCopyContext(ctxHandle);
        auto uploadQueue    = renderSystem._GetCopyQueue();
        auto deviceHeap     = renderSystem.GetDeviceResource(heap);

        ResourceHandle          prevResource    = InvalidHandle_t;
        TextureStreamContext    streamContext   = { allocator };
        uint2                   blockSize       = { 256, 256 };

        for (const RequestedBlock& block : blocks)
        {
            if (prevResource != block.resource)
            {
                if (prevResource != InvalidHandle_t)
                {
                    auto currentState = renderSystem.GetObjectState(block.resource);
                    if(currentState != DeviceResourceState::DRS_GENERIC)
                        ctx.Barrier(
                            renderSystem.GetDeviceResource(block.resource),
                            currentState,
                            DeviceResourceState::DRS_GENERIC);
                }


                if (auto res = GetResourceAsset(block.resource); res)
                {
                    if (!streamContext.Open(block.tileID.GetMip(), res.value()))
                        continue;

                    prevResource    = block.resource;
                    blockSize       = streamContext.GetBlockSize();
                }
                else 
                    continue;
            }

            auto deviceResource = renderSystem.GetDeviceResource(block.resource);
            auto resourceState  = renderSystem.GetObjectState(block.resource);

            if (resourceState != DeviceResourceState::DRS_Write)
                ctx.Barrier(
                    deviceResource,
                    resourceState,
                    DeviceResourceState::DRS_Write);

            renderSystem.SetObjectState(block.resource, DRS_Write);

            const auto tile = streamContext.ReadTile(block.tileID, blockSize, ctx);

            renderSystem._UpdateTileMapping(
                block.tileID,
                blockSize,
                deviceResource,
                deviceHeap,
                block.tileIdx);

            ctx.CopyTile(
                deviceResource,
                block.tileID,
                block.tileIdx,
                tile);
        }

        renderSystem.SubmitUploadQueues(&ctxHandle);
    }


    /************************************************************************************************/


    std::optional<RequestedBlock> TextureStreamingEngine::AllocateTile(TileID_t tile, ResourceHandle resource)
    {
        auto res = std::lower_bound(
            std::begin(mappedAssets),
            std::end(mappedAssets),
            MappedAsset{ resource },
            [](const MappedAsset& lhs, const MappedAsset& rhs)
            {
                return lhs.GetResourceID() < rhs.GetResourceID();
            });

        if (res == std::end(mappedAssets)) 
            return {}; // not a mapped asset

        return textureBlockAllocator.Allocate(tile, resource, renderSystem.GetCurrentFrame());
    }


}	/************************************************************************************************/

/**********************************************************************

Copyright (c) 2019-2020 Robert May

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
