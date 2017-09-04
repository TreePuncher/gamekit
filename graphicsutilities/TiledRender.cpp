/**********************************************************************

Copyright (c) 2015 - 2017 Robert May

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

#include "graphics.h"
#include "PipelineState.h"

namespace FlexKit
{
	/************************************************************************************************/


	ID3D12PipelineState* LoadShadeState(RenderSystem* RS)
	{
		auto ComputeShader = LoadShader("Tiled_Shading", "DeferredShader", "cs_5_0", "assets\\cshader.hlsl");

		D3D12_COMPUTE_PIPELINE_STATE_DESC CPSODesc{};
		ID3D12PipelineState* ShadingPSO = nullptr;

		CPSODesc.pRootSignature		= RS->Library.ShadingRTSig;
		CPSODesc.CS					= CD3DX12_SHADER_BYTECODE(ComputeShader.Blob);
		CPSODesc.Flags				= D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;

		HRESULT HR = RS->pDevice->CreateComputePipelineState(&CPSODesc, IID_PPV_ARGS(&ShadingPSO));
		FK_ASSERT(SUCCEEDED(HR));

		Release(&ComputeShader);

		return ShadingPSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* LoadLightPrePassState(RenderSystem* RS)
	{
		auto ComputeShader = LoadShader("Tiled_LightPrePass", "DeferredShader", "cs_5_0", "assets\\LightPrepass.hlsl");

		D3D12_COMPUTE_PIPELINE_STATE_DESC CPSODesc{};
		ID3D12PipelineState* ShadingPSO = nullptr;

		CPSODesc.pRootSignature = RS->Library.ShadingRTSig;
		CPSODesc.CS             = CD3DX12_SHADER_BYTECODE(ComputeShader.Blob);
		CPSODesc.Flags          = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;

		HRESULT HR = RS->pDevice->CreateComputePipelineState(&CPSODesc, IID_PPV_ARGS(&ShadingPSO));
		FK_ASSERT(SUCCEEDED(HR));

		Release(&ComputeShader);

		return ShadingPSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* LoadLightCopyOutState(RenderSystem* RS)
	{
		auto ComputeShader = LoadShader("ConvertOut", "DeferredShader", "cs_5_0", "assets\\CopyOut.hlsl");

		D3D12_COMPUTE_PIPELINE_STATE_DESC CPSODesc{};
		ID3D12PipelineState* ShadingPSO = nullptr;

		CPSODesc.pRootSignature = RS->Library.ShadingRTSig;
		CPSODesc.CS             = CD3DX12_SHADER_BYTECODE(ComputeShader.Blob);
		CPSODesc.Flags          = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;

		HRESULT HR = RS->pDevice->CreateComputePipelineState(&CPSODesc, IID_PPV_ARGS(&ShadingPSO));
		FK_ASSERT(SUCCEEDED(HR));

		Release(&ComputeShader);

		return ShadingPSO;
	}


	/************************************************************************************************/
	
	
	void ReleaseTiledRender(TiledDeferredRender* gb)
	{
		// GBuffer
		for(size_t I = 0; I < 3; ++I)
		{
			gb->GBuffers[I].ColorTex->Release();
			gb->GBuffers[I].SpecularTex->Release();
			gb->GBuffers[I].NormalTex->Release();
			//gb->GBuffers[I].PositionTex->Release();
			gb->GBuffers[I].LightTilesBuffer->Release();
			gb->GBuffers[I].OutputBuffer->Release();
			gb->GBuffers[I].EmissiveTex->Release();
			gb->GBuffers[I].RoughnessMetal->Release();
			gb->GBuffers[I].Temp->Release();

		}

		gb->Shading.ShaderConstants.Release();

		gb->Filling.PSO->Release();
		gb->Filling.PSOTextured->Release();
		gb->Filling.PSOAnimated->Release();
		gb->Filling.PSOAnimatedTextured->Release();
		gb->Filling.FillRTSig->Release();
		gb->Filling.NormalMesh.Blob->Release();
		gb->Filling.AnimatedMesh.Blob->Release();
		gb->Filling.NoTexture.Blob->Release();
		gb->Filling.Textured.Blob->Release();
	}
	


	
	/************************************************************************************************/


	void UpdateTiledBufferConstants(RenderSystem* RS, TiledDeferredRender* gb, size_t PLightCount, size_t SLightCount)
	{
		FlexKit::GBufferConstantsLayout constants;
		constants.DLightCount = 0;
		constants.PLightCount = PLightCount;
		constants.SLightCount = SLightCount;
		UpdateResourceByTemp(RS, &gb->Shading.ShaderConstants, &constants, sizeof(constants), 1, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	}


	/************************************************************************************************/


	void InitiateTiledDeferredRender(RenderSystem* RS, TiledRendering_Desc* GBdesc, TiledDeferredRender* out)
	{
		RegisterPSOLoader(RS, RS->States, TILEDSHADING_SHADE,		 LoadShadeState);
		RegisterPSOLoader(RS, RS->States, TILEDSHADING_LIGHTPREPASS, LoadLightPrePassState);
		RegisterPSOLoader(RS, RS->States, TILEDSHADING_COPYOUT,		 LoadLightCopyOutState);

		QueuePSOLoad(RS, TILEDSHADING_SHADE);
		QueuePSOLoad(RS, TILEDSHADING_COPYOUT);

		{
			// Create GBuffers
			for(size_t I = 0; I < 3; ++I)
			{
				Texture2D_Desc  desc;
				desc.Height		  = GBdesc->RenderWindow->WH[1];
				desc.Width		  = GBdesc->RenderWindow->WH[0];
				desc.Read         = false;
				desc.Write        = false;
				desc.CV           = true;
				desc.RenderTarget = true;
				desc.MipLevels    = 1;
				//desc.InitiateState = ;


				{
					{	// Alebdo Buffer
						desc.Format = FORMAT_2D::R8G8B8A8_UNORM;
						out->GBuffers[I].ColorTex = CreateTexture2D(RS, &desc);
						FK_ASSERT(out->GBuffers[I].ColorTex);
						SETDEBUGNAME(out->GBuffers[I].ColorTex.Texture, "Albedo Buffer");
					}
					{	// Alebdo Buffer
						desc.Format = FORMAT_2D::R8G8B8A8_UNORM;
						out->GBuffers[I].EmissiveTex = CreateTexture2D(RS, &desc);
						FK_ASSERT(out->GBuffers[I].EmissiveTex);
						SETDEBUGNAME(out->GBuffers[I].EmissiveTex.Texture, "Emissive Buffer");
					}
					{	// Roughness Metal Buffer
						desc.Format = FORMAT_2D::R8G8_UNORM;
						out->GBuffers[I].RoughnessMetal = CreateTexture2D(RS, &desc);
						FK_ASSERT(out->GBuffers[I].RoughnessMetal);
						SETDEBUGNAME(out->GBuffers[I].RoughnessMetal.Texture, "Roughness + Metal Buffer");
					}
					{
						// Create Specular Buffer
						desc.Format = FORMAT_2D::R8G8B8A8_UNORM;
						out->GBuffers[I].SpecularTex = CreateTexture2D(RS, &desc);
						FK_ASSERT(out->GBuffers[I].SpecularTex);
						SETDEBUGNAME(out->GBuffers[I].SpecularTex.Texture, "Specular Buffer");
					}
					{
						// Create Normal Buffer
						desc.Format = FORMAT_2D::R32G32B32A32_FLOAT;
						out->GBuffers[I].NormalTex = CreateTexture2D(RS, &desc);
						FK_ASSERT(out->GBuffers[I].NormalTex);
						SETDEBUGNAME(out->GBuffers[I].NormalTex.Texture, "Normal Buffer");
					}
#if 0
					{
						desc.Format = FORMAT_2D::R32G32B32A32_FLOAT;
						out->GBuffers[I].PositionTex = CreateTexture2D(RS, &desc);
						FK_ASSERT(out->GBuffers[I].PositionTex);
						SETDEBUGNAME(out->GBuffers[I].PositionTex.Texture, "WorldCord Texture");
					}
#endif
					{ // Create Output Buffer
						desc.Format			= FlexKit::FORMAT_2D::R32G32B32A32_FLOAT; // would a half float be enough?
						desc.UAV			= true;
						desc.CV				= true;
						desc.RenderTarget	= true;

						out->GBuffers[I].OutputBuffer = CreateTexture2D(RS, &desc);
						FK_ASSERT(out->GBuffers[I].OutputBuffer);
						SETDEBUGNAME(out->GBuffers[I].OutputBuffer.Texture, "Output Buffer");
					}
					{ // Create Tile Buffer
						desc.Width			= GBdesc->RenderWindow->WH[0]/8;
						desc.Height			= GBdesc->RenderWindow->WH[1]/8;
						desc.Format         = FlexKit::FORMAT_2D::R16G16_UINT;
						desc.UAV            = true;
						desc.CV				= false;
						desc.RenderTarget   = false;

						out->GBuffers[I].LightTilesBuffer = CreateTexture2D(RS, &desc);
						FK_ASSERT(out->GBuffers[I].LightTilesBuffer);
						SETDEBUGNAME(out->GBuffers[I].LightTilesBuffer.Texture, "Light Tiles Buffer");
					}
					{ // Create Temp Buffer
						desc.Width			= GBdesc->RenderWindow->WH[0];
						desc.Height			= GBdesc->RenderWindow->WH[1];
						desc.Format         = FlexKit::FORMAT_2D::R8G8B8A8_UNORM;
						desc.UAV            = true;
						desc.CV				= false;
						desc.RenderTarget   = false;

						out->GBuffers[I].Temp = CreateTexture2D(RS, &desc);
						FK_ASSERT(out->GBuffers[I].Temp);
						SETDEBUGNAME(out->GBuffers[I].Temp.Texture, "Temp Buffer");
					}
					out->GBuffers[I].DepthBuffer = GBdesc->DepthBuffer->Buffer[I];
				}
			}

			// Create Constant Buffers
			{
				GBufferConstantsLayout initial;
				initial.DLightCount = 0;
				initial.PLightCount = 0;
				initial.SLightCount = 0;
				initial.Width	    = GBdesc->RenderWindow->WH[0];
				initial.Height	    = GBdesc->RenderWindow->WH[1];
				initial.SLightCount = 0;
				initial.AmbientLight = float4(0, 0, 0, 0);
				initial.DebugRenderEnabled = 0;

				ConstantBuffer_desc CB_Desc;
				CB_Desc.InitialSize		= 1024;
				CB_Desc.pInital			= &initial;
				CB_Desc.Structured		= false;
				CB_Desc.StructureSize	= 0;

				auto NewConstantBuffer		 = CreateConstantBuffer(RS, &CB_Desc);
				out->Shading.ShaderConstants = NewConstantBuffer;
				NewConstantBuffer._SetDebugName("GBufferPass Constants");
			}

			D3D12_DESCRIPTOR_HEAP_DESC	Heap_Desc;
			Heap_Desc.Flags				= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			Heap_Desc.NodeMask			= 0;
			Heap_Desc.NumDescriptors	= 20;
			Heap_Desc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			
			D3D12_SHADER_RESOURCE_VIEW_DESC SRV_DESC;
			SRV_DESC.Format                        = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
			SRV_DESC.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			SRV_DESC.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
			SRV_DESC.Texture2D.MipLevels           = 1;
			SRV_DESC.Texture2D.MostDetailedMip     = 0;
			SRV_DESC.Texture2D.PlaneSlice          = 0;
			SRV_DESC.Texture2D.ResourceMinLODClamp = 0;

			D3D12_UNORDERED_ACCESS_VIEW_DESC	UAV_DESC = {};
			UAV_DESC.Format               = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
			UAV_DESC.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
			UAV_DESC.Texture2D.MipSlice   = 0;
			UAV_DESC.Texture2D.PlaneSlice = 0;
		}
		// GBuffer Fill Signature Setup
		{
			// LoadShaders
			out->Filling.NormalMesh		= LoadShader( "V2Main",				"V2Main",				"vs_5_0", "assets\\vshader.hlsl" );
			out->Filling.AnimatedMesh	= LoadShader( "VMainVertexPallet",	"VMainVertexPallet",	"vs_5_0", "assets\\vshader.hlsl");
			out->Filling.NoTexture		= LoadShader( "PMain",				"PShader",				"ps_5_0", "assets\\pshader.hlsl");
			out->Filling.Textured		= LoadShader( "PMain_TEXTURED",		"PMain_TEXTURED",		"ps_5_0", "assets\\pshader.hlsl");
	
			// Setup Pipeline State
			{
				CD3DX12_DESCRIPTOR_RANGE ranges[1];	{
					ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 0);
				}

				CD3DX12_ROOT_PARAMETER Parameters[DFRP_COUNT];{
					Parameters[DFRP_CameraConstants].InitAsConstantBufferView	(0, 0, D3D12_SHADER_VISIBILITY_ALL);
					Parameters[DFRP_ShadingConstants].InitAsConstantBufferView	(1, 0, D3D12_SHADER_VISIBILITY_ALL);
					Parameters[DFRP_TextureConstants].InitAsConstantBufferView	(2, 0, D3D12_SHADER_VISIBILITY_ALL);
					Parameters[DFRP_AnimationResources].InitAsShaderResourceView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
					Parameters[DFRP_Textures].InitAsDescriptorTable				(1, ranges, D3D12_SHADER_VISIBILITY_PIXEL);
				}

				ID3DBlob* SignatureDescBlob                    = nullptr;
				ID3DBlob* ErrorBlob		                       = nullptr;
				D3D12_ROOT_SIGNATURE_DESC	 Desc              = {};
				CD3DX12_ROOT_SIGNATURE_DESC  RootSignatureDesc = {};
				CD3DX12_STATIC_SAMPLER_DESC	 Default(0);

				RootSignatureDesc.Init(RootSignatureDesc, DFRP_COUNT, Parameters, 1, &Default, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
				HRESULT HR = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &SignatureDescBlob, &ErrorBlob);

				FK_ASSERT(SUCCEEDED(HR));

				ID3D12RootSignature* RootSig = nullptr;
				HR = RS->pDevice->CreateRootSignature(0, SignatureDescBlob->GetBufferPointer(),
													  SignatureDescBlob->GetBufferSize(), IID_PPV_ARGS(&RootSig));

				SETDEBUGNAME(RootSig, "FillRTSig");
				SignatureDescBlob->Release();

				FK_ASSERT(SUCCEEDED(HR));
				out->Filling.FillRTSig = RootSig;

				D3D12_INPUT_ELEMENT_DESC InputElements[5] = {	
					{"POSITION",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
					{"TEXCOORD",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	    1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
					{"NORMAL",			0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
					{"WEIGHTS",			0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,	3, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
					{"WEIGHTINDICES",	0, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT,  4, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				};

				D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
				D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
				Depth_Desc.DepthFunc					= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
				Depth_Desc.DepthEnable					= true;

				D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {};{
					PSO_Desc.pRootSignature			= RootSig;
					PSO_Desc.VS						= out->Filling.NormalMesh;
					PSO_Desc.PS						= out->Filling.NoTexture;
					PSO_Desc.RasterizerState		= Rast_Desc;
					PSO_Desc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
					PSO_Desc.SampleMask				= UINT_MAX;
					PSO_Desc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
					PSO_Desc.NumRenderTargets		= 6;
					PSO_Desc.RTVFormats[0]			= DXGI_FORMAT_R8G8B8A8_UNORM;		// Color
					PSO_Desc.RTVFormats[1]			= DXGI_FORMAT_R8G8B8A8_UNORM;		// Specular
					PSO_Desc.RTVFormats[2]			= DXGI_FORMAT_R8G8B8A8_UNORM;		// Emissive
					PSO_Desc.RTVFormats[3]			= DXGI_FORMAT_R8G8_UNORM;			// Roughness
					PSO_Desc.RTVFormats[4]			= DXGI_FORMAT_R32G32B32A32_FLOAT;	// Normal
					PSO_Desc.SampleDesc.Count		= 1;
					PSO_Desc.SampleDesc.Quality		= 0;
					PSO_Desc.DSVFormat				= DXGI_FORMAT_D32_FLOAT;
					PSO_Desc.InputLayout			={ InputElements, 3 };
					PSO_Desc.DepthStencilState		= Depth_Desc;
				}

				ID3D12PipelineState* PSO = nullptr;
				HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
				FK_ASSERT(SUCCEEDED(HR));
				out->Filling.PSO = PSO;

				PSO_Desc.PS = { (BYTE*)out->Filling.Textured.Blob->GetBufferPointer(), out->Filling.Textured.Blob->GetBufferSize() };
				HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
				FK_ASSERT(SUCCEEDED(HR));
				out->Filling.PSOTextured = PSO;

				PSO_Desc.VS = { (BYTE*)out->Filling.AnimatedMesh.Blob->GetBufferPointer(), out->Filling.AnimatedMesh.Blob->GetBufferSize() };
				PSO_Desc.PS = { (BYTE*)out->Filling.NoTexture.Blob->GetBufferPointer(), out->Filling.NoTexture.Blob->GetBufferSize() };
				PSO_Desc.InputLayout = { InputElements, 5 };
				HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
				FK_ASSERT(SUCCEEDED(HR));
				out->Filling.PSOAnimated = PSO;

				PSO_Desc.PS = { (BYTE*)out->Filling.Textured.Blob->GetBufferPointer(), out->Filling.Textured.Blob->GetBufferSize() };
				PSO_Desc.InputLayout = { InputElements, 5 };
				HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
				FK_ASSERT(SUCCEEDED(HR));
				out->Filling.PSOAnimatedTextured = PSO;
			}
		}
	}


	/************************************************************************************************/


	void IncrementPassIndex(TiledDeferredRender* Pass) {
		Pass->CurrentBuffer = ++Pass->CurrentBuffer % 3;
	}



	/************************************************************************************************/


	void TiledRender_LightPrePass(
		RenderSystem* RS, TiledDeferredRender* Pass, const Camera* C,
		const PointLightBuffer* PLB, const SpotLightBuffer* SPLB, uint2 WH)
	{
		auto CL = GetCurrentCommandList(RS);
		auto FrameResources		= GetCurrentFrameResources(RS);
		auto BufferIndex		= Pass->CurrentBuffer;
		auto& CurrentGBuffer	= Pass->GBuffers[BufferIndex];

		auto DescTable	= GetDescTableCurrentPosition_GPU(RS);
		auto TablePOS	= ReserveDescHeap(RS, 11);

		TablePOS = PushCBToDescHeap(RS, C->Buffer.Get(), TablePOS, CALCULATECONSTANTBUFFERSIZE(Camera::BufferLayout));
		TablePOS = PushCBToDescHeap(RS, Pass->Shading.ShaderConstants.Get(), TablePOS, CALCULATECONSTANTBUFFERSIZE(GBufferConstantsLayout));

		TablePOS = PushSRVToDescHeap(RS, PLB->Resource, TablePOS, max(PLB->size(), 1), PLB_Stride);
		TablePOS = PushSRVToDescHeap(RS, SPLB->Resource, TablePOS, max(SPLB->size(), 1), SPLB_Stride);

		TablePOS = PushTextureToDescHeap(RS, RS->NullSRV, TablePOS);
		TablePOS = PushTextureToDescHeap(RS, RS->NullSRV, TablePOS);
		TablePOS = PushTextureToDescHeap(RS, RS->NullSRV, TablePOS);
		TablePOS = PushTextureToDescHeap(RS, RS->NullSRV, TablePOS);
		TablePOS = PushTextureToDescHeap(RS, RS->NullSRV, TablePOS);
		TablePOS = PushTextureToDescHeap(RS, RS->NullSRV, TablePOS);

		TablePOS = PushUAV2DToDescHeap(RS, CurrentGBuffer.LightTilesBuffer, TablePOS, DXGI_FORMAT::DXGI_FORMAT_R16G16_UINT);

		CL->SetPipelineState(GetPSO(RS, TILEDSHADING_LIGHTPREPASS));
		CL->SetComputeRootSignature(RS->Library.ShadingRTSig);
		CL->SetComputeRootDescriptorTable(DSRP_DescriptorTable, DescTable);


		ID3D12DescriptorHeap* Heaps[] = { FrameResources->DescHeap.DescHeap };
		CL->SetDescriptorHeaps(1, Heaps);

		// Do Shading Here
		CL->SetPipelineState(GetPSO(RS, TILEDSHADING_LIGHTPREPASS));
		CL->SetComputeRootSignature(RS->Library.ShadingRTSig);

		{
			CD3DX12_RESOURCE_BARRIER Barrier1[] = {
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.LightTilesBuffer,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
			};
			CL->ResourceBarrier(sizeof(Barrier1) / sizeof(Barrier1[0]), Barrier1);
		}

		{
			CD3DX12_RESOURCE_BARRIER Barrier1[] = {
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.LightTilesBuffer,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
			};
			CL->ResourceBarrier(sizeof(Barrier1) / sizeof(Barrier1[0]), Barrier1);
		}
	}


	/************************************************************************************************/


	void UploadDeferredPassConstants(RenderSystem* RS, DeferredPass_Parameters* in, float4 A, TiledDeferredRender* Pass)
	{
		GBufferConstantsLayout Update = {};
		Update.PLightCount  = in->PointLightCount;
		Update.SLightCount  = in->SpotLightCount;
		Update.AmbientLight = A;
		Update.DebugRenderEnabled = in->Mode;
		Update.Height		= in->WH[1];
		Update.Width		= in->WH[0];

		UpdateResourceByTemp(RS, &Pass->Shading.ShaderConstants, &Update, sizeof(Update), 1, 
							D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	}


	/************************************************************************************************/


	void ClearTileRenderBuffers(RenderSystem* RS, TiledDeferredRender* Pass) {
		size_t BufferIndex = Pass->CurrentBuffer;
		float4	ClearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
		ClearGBuffer(RS, Pass, ClearValue, BufferIndex);
	}


	/************************************************************************************************/


	void TiledRender_Fill(
		RenderSystem*	RS,		PVS*			_PVS,	TiledDeferredRender*	Pass,	
		Texture2D Target,		const Camera*	C,		TextureManager* TM,
		GeometryTable*	GT,		TextureVTable*	Texture, OcclusionCuller* OC )
	{
		auto FrameResources	= GetCurrentFrameResources(RS);

		Context Ctx(GetCurrentCommandList(RS), RS);
		Ctx.Clear(); // Clear Any Residual State

		size_t BufferIndex = Pass->CurrentBuffer;

		Ctx.SetDepthStencil(&Pass->GBuffers[BufferIndex].DepthBuffer);
		Ctx.SetRenderTargets(
			{	&Pass->GBuffers[BufferIndex].ColorTex, 
				&Pass->GBuffers[BufferIndex].SpecularTex, 
				&Pass->GBuffers[BufferIndex].EmissiveTex, 
				&Pass->GBuffers[BufferIndex].RoughnessMetal, 
				&Pass->GBuffers[BufferIndex].NormalTex });


		// Setup State
		D3D12_RECT		RECT = D3D12_RECT();
		D3D12_VIEWPORT	VP	 = D3D12_VIEWPORT();

		RECT.right  = (UINT)Target.WH[0];
		RECT.bottom = (UINT)Target.WH[1];
		VP.Height   = (UINT)Target.WH[1];
		VP.Width    = (UINT)Target.WH[0];
		VP.MaxDepth = 1;
		VP.MinDepth = 0;
		VP.TopLeftX = 0;
		VP.TopLeftY = 0;

		D3D12_RECT		RECTs[] = { RECT, RECT, RECT, RECT, RECT, RECT };

		Ctx.SetRootSignature			(RS->Library.TestSignature);
		Ctx.SetPipelineState			(Pass->Filling.PSO);
		Ctx.SetViewports				({ VP, VP, VP, VP, VP, VP, });
		Ctx.SetPrimitiveTopology		(EInputTopology::EIT_TRIANGLELIST);

		Ctx.SetGraphicsConstantBufferView	(DFRP_ShadingConstants, RS->NullConstantBuffer);
		Ctx.SetGraphicsConstantBufferView	(DFRP_CameraConstants,	C->Buffer);
		Ctx.SetGraphicsConstantBufferView	(DFRP_TextureConstants, RS->NullConstantBuffer);

		
		TriMeshHandle	LastHandle	= INVALIDMESHHANDLE;
		TriMesh*		Mesh		= nullptr;
		size_t			IndexCount  = 0;

		auto itr = _PVS->begin();
		auto end = _PVS->end();

		for (; itr != end; ++itr)
		{

			Drawable* E = itr->D;
			TriMesh* CurrentMesh	= GetMesh(GT, E->MeshHandle);
			auto AnimationState		= E->PoseState;
			
			if (!E->Posed || E->Textured)
				break;
			
			if(!AnimationState || !AnimationState->Resource)
				continue;

			TriMeshHandle CurrentHandle = E->MeshHandle;
			if (CurrentHandle == INVALIDMESHHANDLE)
				continue;

			if (CurrentHandle != LastHandle)
			{
				Ctx.AddIndexBuffer(CurrentMesh);
				Ctx.AddVertexBuffers(
						CurrentMesh, 
						{	VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
							VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,
							VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL	});
				LastHandle = CurrentHandle;
			}


			DesciptorHeap Heap(RS, RS->Library.TestSignature.GetDescHeap(0), nullptr);

			Ctx.SetGraphicsDescriptorTable		(0, Heap);
			Ctx.SetGraphicsConstantBufferView	(DFRP_ShadingConstants,	C->Buffer);
			Ctx.DrawIndexed						(IndexCount);
		}

		/*
		if(itr != end)
			Ctx.SetPipelineState(Pass->Filling.PSOAnimated);

		for (; itr != end; ++itr)
		{

			Drawable* E = itr->D;
			TriMesh* CurrentMesh	= GetMesh(GT, E->MeshHandle);
			auto AnimationState		= E->PoseState;
			
			if (!E->Posed || E->Textured)
				break;
			
			if(!AnimationState || !AnimationState->Resource)
				continue;

			TriMeshHandle CurrentHandle = E->MeshHandle;
			if (CurrentHandle == INVALIDMESHHANDLE)
				continue;

			if (CurrentHandle != LastHandle)
			{
				Ctx.AddIndexBuffer(CurrentMesh);
				Ctx.AddVertexBuffers(
						CurrentMesh, 
						{	VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
							VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,
							VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
							VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1,
							VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2 });
				LastHandle = CurrentHandle;
			}


			DesciptorHeap Heap(FrameResources->DescHeap, RS->Library.TestSignature.GetDescHeap(0));

			Ctx.SetRootDescriptorTable			(0, Heap);
			Ctx.SetGraphicsConstantBufferView	(DFRP_ShadingConstants,		C->Buffer);
			Ctx.SetGraphicsShaderResourceView	(DFRP_AnimationResources,	AnimationState, AnimationState->JointCount, sizeof(float4x4) * 2);
			Ctx.DrawIndexed						(IndexCount);
		}
		*/
	}


	/************************************************************************************************/


	void SetGBufferState(RenderSystem* RS, ID3D12GraphicsCommandList* CL, TiledDeferredRender* Pass, size_t Index, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState)
	{
		static_vector<D3D12_RESOURCE_BARRIER>	Barriers;

		{
			/*
			typedef struct D3D12_RESOURCE_TRANSITION_BARRIER
			{
			ID3D12Resource *pResource;
			UINT Subresource;
			D3D12_RESOURCE_STATES StateBefore;
			D3D12_RESOURCE_STATES StateAfter;
			} 	D3D12_RESOURCE_TRANSITION_BARRIER;

			typedef struct D3D12_RESOURCE_BARRIER
			{
			D3D12_RESOURCE_BARRIER_TYPE Type;
			D3D12_RESOURCE_BARRIER_FLAGS Flags;
			union
			{
			D3D12_RESOURCE_TRANSITION_BARRIER Transition;
			D3D12_RESOURCE_ALIASING_BARRIER Aliasing;
			D3D12_RESOURCE_UAV_BARRIER UAV;
			} 	;
			} 	D3D12_RESOURCE_BARRIER;
			*/

			D3D12_RESOURCE_TRANSITION_BARRIER Transition = {
				nullptr, 0,
				BeforeState,
				AfterState,
			};


			D3D12_RESOURCE_BARRIER Barrier =
			{
				D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE,
				Transition,
			};

			auto PushBarrier = [&](ID3D12Resource* Res)
			{
				Transition.pResource = Res;
				Barrier.Transition   = Transition;
				Barriers.push_back(Barrier);
			};

			PushBarrier(Pass->GBuffers[Index].ColorTex);
			PushBarrier(Pass->GBuffers[Index].NormalTex);
			PushBarrier(Pass->GBuffers[Index].OutputBuffer);
			//PushBarrier(Pass->GBuffers[Index].PositionTex);
			PushBarrier(Pass->GBuffers[Index].SpecularTex);
		}

		CL->ResourceBarrier(Barriers.size(), Barriers.begin());
	}



	/************************************************************************************************/

	void TiledRender_Shade(
		RenderSystem* RS, PVS* _PVS, 
		TiledDeferredRender* Pass, Texture2D Target,
		const Camera* C,
		const PointLightBuffer* PLB, const SpotLightBuffer* SPLB)
	{
		auto CL				 = GetCurrentCommandList(RS);
		auto FrameResources  = GetCurrentFrameResources(RS);
		auto BufferIndex	 = Pass->CurrentBuffer;
		auto& CurrentGBuffer = Pass->GBuffers[BufferIndex];


		{
			CD3DX12_RESOURCE_BARRIER Barrier1[] = {
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.ColorTex,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),

				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.SpecularTex,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),


				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.EmissiveTex,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),

				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.RoughnessMetal,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),

				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.NormalTex,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),

#if 0
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.PositionTex,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
#endif

				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.OutputBuffer,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE) };

			CL->ResourceBarrier(sizeof(Barrier1) / sizeof(Barrier1[0]), Barrier1);
		}

		auto DescTable = GetDescTableCurrentPosition_GPU(RS);
		auto TablePOS  = ReserveDescHeap(RS, 11);
		auto DispatchDimensions = uint2{ Target.WH[0] / 32, Target.WH[1] / 12 };

		// The Max is to quiet a error if a no Lights are passed
		TablePOS = PushSRVToDescHeap	(RS, PLB->Resource, TablePOS,  max(PLB->size(), 1),  PLB_Stride);	
		TablePOS = PushSRVToDescHeap	(RS, SPLB->Resource, TablePOS, max(SPLB->size(), 1), SPLB_Stride);

		TablePOS = PushTextureToDescHeap	(RS, CurrentGBuffer.ColorTex,			TablePOS);
		TablePOS = PushTextureToDescHeap	(RS, CurrentGBuffer.SpecularTex,		TablePOS);
		TablePOS = PushTextureToDescHeap	(RS, CurrentGBuffer.EmissiveTex,		TablePOS);
		TablePOS = PushTextureToDescHeap	(RS, CurrentGBuffer.RoughnessMetal,		TablePOS);
		TablePOS = PushTextureToDescHeap	(RS, CurrentGBuffer.NormalTex,			TablePOS);
		TablePOS = Push2DSRVToDescHeap		(RS, CurrentGBuffer.LightTilesBuffer,	TablePOS);
		TablePOS = PushUAV2DToDescHeap		(RS, CurrentGBuffer.OutputBuffer,		TablePOS, DXGI_FORMAT_R32G32B32A32_FLOAT);

		auto ConstantBuffers = DescTable;

		TablePOS = PushCBToDescHeap			(RS, C->Buffer.Get(), TablePOS,						CALCULATECONSTANTBUFFERSIZE(Camera::BufferLayout));
		TablePOS = PushCBToDescHeap			(RS, Pass->Shading.ShaderConstants.Get(), TablePOS, CALCULATECONSTANTBUFFERSIZE(GBufferConstantsLayout));

		ID3D12DescriptorHeap* Heaps[] = { FrameResources->DescHeap.DescHeap };
		CL->SetDescriptorHeaps(1, Heaps);

		// Do Shading Here
		CL->SetPipelineState				(GetPSO(RS, TILEDSHADING_SHADE));
		CL->SetComputeRootSignature			(RS->Library.ShadingRTSig);
		CL->SetComputeRootDescriptorTable	(DSRP_DescriptorTable, DescTable);

		CL->Dispatch(DispatchDimensions[0], DispatchDimensions[1], 1);

		{
			CD3DX12_RESOURCE_BARRIER Barrier2[] = {
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.ColorTex,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),

				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.SpecularTex,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),

				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.EmissiveTex,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),

				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.RoughnessMetal,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),

				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.NormalTex,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET, -1, 
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),

#if 0
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.PositionTex,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
#endif

				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.OutputBuffer,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_COPY_SOURCE, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),

				CD3DX12_RESOURCE_BARRIER::Transition(Target,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_COPY_DEST,	-1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE) };

			CL->ResourceBarrier(sizeof(Barrier2)/ sizeof(Barrier2[0]), Barrier2);
		}

		{
			CD3DX12_RESOURCE_BARRIER Barrier3[] = {
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.OutputBuffer,
				D3D12_RESOURCE_STATE_COPY_SOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
				CD3DX12_RESOURCE_BARRIER::Transition(Target,
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_RENDER_TARGET, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE) };

			CL->ResourceBarrier(2, Barrier3);
		}

	}


	/************************************************************************************************/

	void ClearGBuffer(RenderSystem* RS, TiledDeferredRender* Pass, const float4& Clear, size_t Index)
	{
		auto CL = GetCurrentCommandList(RS);
		auto RTVPOSCPU = GetRTVTableCurrentPosition_CPU(RS); // _Ptr to Current POS On RTV heap on CPU
		auto RTVPOS = ReserveRTVHeap(RS, 6);
		auto RTVHeap = GetCurrentRTVTable(RS);

		size_t BufferIndex = Pass->CurrentBuffer;

		auto ClearTarget = [&](DescHeapPOS POS, Texture2D& Texture) {
			RTVPOS = PushRenderTarget(RS, &Texture, POS);
			CL->ClearRenderTargetView(POS, Clear, 0, nullptr);
		};


		ClearTarget(RTVPOS, Pass->GBuffers[BufferIndex].ColorTex);
		ClearTarget(RTVPOS, Pass->GBuffers[BufferIndex].SpecularTex);
		ClearTarget(RTVPOS, Pass->GBuffers[BufferIndex].NormalTex);
		ClearTarget(RTVPOS, Pass->GBuffers[BufferIndex].OutputBuffer);
	}


	/************************************************************************************************/


	void PresentBufferToTarget(RenderSystem* RS, ID3D12GraphicsCommandList* CL, TiledDeferredRender* Render, Texture2D* Target, Texture2D* ReflectionBuffer)
	{
		auto BufferIndex	 = Render->CurrentBuffer;
		auto& CurrentGBuffer = Render->GBuffers[BufferIndex];

		auto DescTable = GetDescTableCurrentPosition_GPU(RS);
		auto TablePOS  = ReserveDescHeap(RS, 11);

		
		// The Max is to quiet a error if a no Lights are passed

		TablePOS = PushTextureToDescHeap	(RS, CurrentGBuffer.OutputBuffer, TablePOS);		// t0
		TablePOS = PushCBToDescHeap			(RS, RS->NullConstantBuffer.Get(), TablePOS, 1024); // t1

		if(ReflectionBuffer)
			TablePOS = Push2DSRVToDescHeap		(RS, *ReflectionBuffer, TablePOS);	// t2
		else
			TablePOS = PushTextureToDescHeap(RS, RS->NullSRV, TablePOS); // t2

		TablePOS = PushTextureToDescHeap	(RS, RS->NullSRV, TablePOS); // t3
		TablePOS = PushTextureToDescHeap	(RS, RS->NullSRV, TablePOS); // t4
		TablePOS = PushTextureToDescHeap	(RS, RS->NullSRV, TablePOS); // t5
		TablePOS = PushTextureToDescHeap	(RS, RS->NullSRV, TablePOS); // t6

		TablePOS = Push2DSRVToDescHeap		(RS, CurrentGBuffer.LightTilesBuffer,	TablePOS); // t7
		TablePOS = PushUAV2DToDescHeap		(RS, CurrentGBuffer.Temp,				TablePOS, DXGI_FORMAT_R8G8B8A8_UNORM); //t8


		TablePOS = PushCBToDescHeap(RS, RS->NullConstantBuffer.Get(), TablePOS, 1024);
		TablePOS = PushCBToDescHeap(RS, RS->NullConstantBuffer.Get(), TablePOS, 1024);

		{
			CD3DX12_RESOURCE_BARRIER Barrier[] = {
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.OutputBuffer,
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, -1,
					D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.Temp,
					D3D12_RESOURCE_STATE_COPY_SOURCE,
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS, -1,
					D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
				CD3DX12_RESOURCE_BARRIER::Transition(*Target,
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_COPY_DEST, -1,
					D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE)
			};

			CL->ResourceBarrier(3, Barrier);
		}

		CL->SetComputeRootSignature			(RS->Library.ShadingRTSig);
		CL->SetComputeRootDescriptorTable	(0, DescTable);
		CL->SetPipelineState				(GetPSO(RS, EPIPELINESTATES::TILEDSHADING_COPYOUT));
		CL->Dispatch						(Target->WH[0] / 32, Target->WH[1] / 12, 1);

		{
			CD3DX12_RESOURCE_BARRIER Barrier[] = {
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.OutputBuffer,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_RENDER_TARGET, -1,
					D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.Temp,
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
					D3D12_RESOURCE_STATE_COPY_SOURCE, -1,
					D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
			};

			CL->ResourceBarrier(2, Barrier);
		}

		CL->CopyResource(*Target, CurrentGBuffer.Temp);

		{
			CD3DX12_RESOURCE_BARRIER Barrier[] = {
				CD3DX12_RESOURCE_BARRIER::Transition(*Target,
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_RENDER_TARGET, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE)
			};

			CL->ResourceBarrier(1, Barrier);
		}
	}


}	/************************************************************************************************/