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

#include "..\graphicsutilities\TerrainRendering.h"
#include "..\coreutilities\intersection.h"

#include <d3dx12.h>
#include <stdio.h>

#pragma warning( disable : 4267 )

namespace FlexKit
{
	/************************************************************************************************/


	const static size_t SO_BUFFERSIZES = MEGABYTE * 4;

	//void DrawLandscape(RenderSystem* RS, Landscape* LS, TiledDeferredRender* Pass, size_t splitcount, Camera* C, bool DrawWireframe)
	//{	
	//	auto CL = RS->_GetCurrentCommandList();
	//	if (LS->Regions.size())
	//	{
	//		/*
	//		typdef struct D3D12_VERTEX_BUFFER_VIEW Views
	//		{
	//			D3D12_GPU_VIRTUAL_ADDRESS	BufferLocation;
	//			UINT						SizeInBytes;
	//			UINT						StrideInBytes;
	//		} 	D3D12_VERTEX_BUFFER_VIEW;
	//		*/

	//		auto CL             = RS->_GetCurrentCommandList();
	//		auto FrameResources = RS->_GetCurrentFrameResources();
	//		auto DescPOSGPU     = RS->_GetDescTableCurrentPosition_GPU(); // _Ptr to Beginning of Heap On GPU
	//		auto DescPOS        = RS->_ReserveDescHeap(11);
	//		auto DescriptorHeap = RS->_GetCurrentDescriptorTable();

	//		auto RTVPOSCPU      = RS->_GetRTVTableCurrentPosition_CPU(); // _Ptr to Current POS On RTV heap on CPU
	//		auto RTVPOS         = RS->_ReserveRTVHeap(5);
	//		auto RTVHeap        = RS->_GetCurrentRTVTable();

	//		CL->SetDescriptorHeaps(1, &DescriptorHeap);

	//		size_t BufferIndex = Pass->CurrentBuffer;

	//		RTVPOS = PushRenderTarget(RS, &Pass->GBuffers[BufferIndex].ColorTex,		RTVPOS);
	//		RTVPOS = PushRenderTarget(RS, &Pass->GBuffers[BufferIndex].SpecularTex,		RTVPOS);
	//		RTVPOS = PushRenderTarget(RS, &Pass->GBuffers[BufferIndex].EmissiveTex,		RTVPOS);
	//		RTVPOS = PushRenderTarget(RS, &Pass->GBuffers[BufferIndex].RoughnessMetal,	RTVPOS);
	//		RTVPOS = PushRenderTarget(RS, &Pass->GBuffers[BufferIndex].NormalTex,		RTVPOS);

	//		DescPOS = PushTextureToDescHeap(RS, LS->HeightMap, DescPOS);
	//		DescPOS = PushCBToDescHeap(RS, RS->NullConstantBuffer.Get(), DescPOS, 1024); // t1

	//		UINT Stride = sizeof(Landscape::ViewableRegion);

	//		D3D12_VERTEX_BUFFER_VIEW SO_1[] = { {
	//				LS->RegionBuffers[0]->GetGPUVirtualAddress(),
	//				(UINT)SO_BUFFERSIZES,
	//				Stride,//sizeof(Landscape::ViewableRegion)
	//			}, };

	//		D3D12_VERTEX_BUFFER_VIEW SO_2[] = { {
	//				LS->RegionBuffers[1]->GetGPUVirtualAddress(),
	//				(UINT)SO_BUFFERSIZES,
	//				Stride,//sizeof(Landscape::ViewableRegion)
	//			}, };

	//		D3D12_VERTEX_BUFFER_VIEW SO_Initial[] = { {
	//				LS->InputBuffer->GetGPUVirtualAddress(),
	//				(UINT)LS->Regions.size() * sizeof(Landscape::ViewableRegion),
	//				Stride,//sizeof(Landscape::ViewableRegion)
	//			}, };

	//		D3D12_VERTEX_BUFFER_VIEW FinalBufferInput[] = { {
	//				LS->FinalBuffer->GetGPUVirtualAddress(),
	//				(UINT)SO_BUFFERSIZES,
	//				Stride,//(UINT)sizeof(Landscape::ViewableRegion)
	//			}, };

	//		/*
	//		typedef struct D3D12_STREAM_OUTPUT_BUFFER_VIEW
	//		{
	//			D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
	//			UINT64 SizeInBytes;
	//			D3D12_GPU_VIRTUAL_ADDRESS BufferFilledSizeLocation;
	//		} 	D3D12_STREAM_OUTPUT_BUFFER_VIEW;
	//		*/

	//		ID3D12Resource*	IndirectBuffers[] = {
	//			LS->IndirectOptions1.Get(),
	//			LS->IndirectOptions2.Get(),
	//		};

	//		D3D12_STREAM_OUTPUT_BUFFER_VIEW SOViews1[] = {
	//			{ LS->RegionBuffers[0]->GetGPUVirtualAddress(),	SO_BUFFERSIZES, LS->SOCounter_1->GetGPUVirtualAddress() },
	//			{ LS->FinalBuffer->GetGPUVirtualAddress(),		SO_BUFFERSIZES, LS->FB_Counter->GetGPUVirtualAddress()  },
	//		};

	//		D3D12_STREAM_OUTPUT_BUFFER_VIEW SOViews2[] = {
	//			{ LS->RegionBuffers[1]->GetGPUVirtualAddress(),	SO_BUFFERSIZES, LS->SOCounter_2->GetGPUVirtualAddress() },
	//			{ LS->FinalBuffer->GetGPUVirtualAddress(),		SO_BUFFERSIZES, LS->FB_Counter->GetGPUVirtualAddress()  },
	//		};

	//		ID3D12Resource*				Buffers[] = { LS->RegionBuffers[0].Get(), LS->RegionBuffers[1].Get() };
	//		D3D12_VERTEX_BUFFER_VIEW*	Input[] = { SO_1 , SO_2 };

	//		D3D12_STREAM_OUTPUT_BUFFER_VIEW*	Output[] = { SOViews1, SOViews2 };
	//		ID3D12Resource*						OutputCounters[] = { LS->SOCounter_1.Get(), LS->SOCounter_2.Get() };


	//		auto DSVPOSCPU	= RS->_GetDSVTableCurrentPosition_CPU(); // _Ptr to Current POS On DSV heap on CPU
	//		auto POS		= RS->_ReserveDSVHeap(1);
	//		PushDepthStencil(RS, &Pass->GBuffers[Pass->CurrentBuffer].DepthBuffer, POS);

	//		CL->OMSetRenderTargets(5, &RTVPOSCPU, true, &DSVPOSCPU);

	//		{
	//			D3D12_VERTEX_BUFFER_VIEW SO_Initial[] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, };
	//			CL->IASetVertexBuffers(0, sizeof(SO_Initial) / sizeof(SO_Initial[0]), SO_Initial);
	//		}

	//		// Reset Stream Counters
	//		CL->CopyResource(LS->FB_Counter.Get(),  LS->ZeroValues);
	//		CL->CopyResource(LS->SOCounter_1.Get(), LS->ZeroValues);
	//		CL->CopyResource(LS->SOCounter_2.Get(), LS->ZeroValues);

	//		{
	//			CD3DX12_RESOURCE_BARRIER Barrier[] = {
	//				CD3DX12_RESOURCE_BARRIER::Transition(LS->FB_Counter.Get(),  D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_STREAM_OUT),
	//				CD3DX12_RESOURCE_BARRIER::Transition(LS->SOCounter_1.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_STREAM_OUT),
	//				CD3DX12_RESOURCE_BARRIER::Transition(LS->SOCounter_2.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_STREAM_OUT),
	//				CD3DX12_RESOURCE_BARRIER::Transition(LS->FinalBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_STREAM_OUT),
	//			};

	//			CL->ResourceBarrier(4, Barrier);
	//		}

	//		{
	//			auto PSO = RS->GetPSO(TERRAIN_CULL_PSO);
	//			CL->SetGraphicsRootSignature(RS->Library.RS4CBVs_SO);
	//			CL->SetPipelineState(PSO);
	//			CL->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	//		}
	//		//CL->SetGraphicsRootConstantBufferView(0, C->Buffer->GetGPUVirtualAddress());
	//		CL->SetGraphicsRootConstantBufferView(1, LS->ConstantBuffer->GetGPUVirtualAddress());
	//		//CL->SetGraphicsRootConstantBufferView (2, RS->NullConstantBuffer);
	//		CL->SetGraphicsRootDescriptorTable(3, DescPOSGPU);

	//		// Prime System
	//		CL->IASetVertexBuffers(0, 1, SO_Initial);
	//		CL->BeginQuery(LS->SOQuery.Get(), D3D12_QUERY_TYPE_SO_STATISTICS_STREAM1, 0);
	//		CL->SOSetTargets(0, 2, Output[0]);

	//		size_t I = 0;
	//		for (; I < splitcount; ++I)
	//		{
	//			bool Index = (I % 2) != 0;
	//			bool NextIndex = Index ? 0 : 1;

	//			CL->BeginQuery(LS->SOQuery.Get(), D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0, 1);

	//			if (I)
	//				CL->ExecuteIndirect(LS->CommandSignature, 1, IndirectBuffers[Index], 0, nullptr, 0);
	//			else
	//				CL->DrawInstanced(1, 1, 0, 0);

	//			{
	//				CD3DX12_RESOURCE_BARRIER Barrier[] = {
	//					CD3DX12_RESOURCE_BARRIER::Transition(Buffers[Index],			D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
	//					CD3DX12_RESOURCE_BARRIER::Transition(Buffers[!Index],			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_STREAM_OUT),
	//					CD3DX12_RESOURCE_BARRIER::Transition(IndirectBuffers[!Index],	D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_DEST),
	//					CD3DX12_RESOURCE_BARRIER::Transition(LS->SOCounter_1.Get(),		D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_DEST),
	//					CD3DX12_RESOURCE_BARRIER::Transition(LS->SOCounter_2.Get(),		D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_DEST),

	//				};

	//				CL->ResourceBarrier(sizeof(Barrier) / sizeof(Barrier[0]), Barrier);
	//			}

	//			CL->EndQuery(LS->SOQuery.Get(), D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0, 1);
	//			CL->ResolveQueryData(LS->SOQuery.Get(), D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0, 1, 1, IndirectBuffers[NextIndex], 0);
	//			CL->CopyBufferRegion(IndirectBuffers[NextIndex], 4, LS->ZeroValues, 4, 12);

	//			CL->CopyBufferRegion(LS->SOCounter_1.Get(), 0, LS->ZeroValues, 16, 16);
	//			CL->CopyBufferRegion(LS->SOCounter_2.Get(), 0, LS->ZeroValues, 16, 16);

	//			{
	//				CD3DX12_RESOURCE_BARRIER Barrier[] = {
	//					CD3DX12_RESOURCE_BARRIER::Transition(IndirectBuffers[!Index],	D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
	//					CD3DX12_RESOURCE_BARRIER::Transition(LS->SOCounter_1.Get(),		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_STREAM_OUT),
	//					CD3DX12_RESOURCE_BARRIER::Transition(LS->SOCounter_2.Get(),		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_STREAM_OUT),
	//				};

	//				CL->ResourceBarrier(sizeof(Barrier) / sizeof(Barrier[0]), Barrier);
	//			}

	//			CL->IASetVertexBuffers(0, 1, Input[Index]);
	//			CL->SOSetTargets(0, 2, Output[!Index]);
	//		}


	//		{
	//			D3D12_STREAM_OUTPUT_BUFFER_VIEW NullSO[] = {
	//				{ 0, 0, 0 },
	//				{ 0, 0, 0 },
	//			};

	//			CL->SOSetTargets(0, 2, NullSO);
	//		}

	//		// Do Draw Here

	//		uint16_t Index = I % 2;
	//		uint16_t NextIndex = Index ? 0 : 1;

	//		{
	//			CD3DX12_RESOURCE_BARRIER Barrier[] = {
	//				CD3DX12_RESOURCE_BARRIER::Transition(Buffers[Index],			D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
	//				CD3DX12_RESOURCE_BARRIER::Transition(Buffers[!Index],			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_STREAM_OUT),
	//				CD3DX12_RESOURCE_BARRIER::Transition(LS->FinalBuffer.Get(),		D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
	//				CD3DX12_RESOURCE_BARRIER::Transition(IndirectBuffers[0],		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,	D3D12_RESOURCE_STATE_COPY_DEST),
	//				CD3DX12_RESOURCE_BARRIER::Transition(IndirectBuffers[1],		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,	D3D12_RESOURCE_STATE_COPY_DEST),
	//			};

	//			CL->ResourceBarrier(sizeof(Barrier) / sizeof(Barrier[0]), Barrier);
	//		}

	//		CL->EndQuery(LS->SOQuery.Get(), D3D12_QUERY_TYPE_SO_STATISTICS_STREAM1, 0);
	//		CL->ResolveQueryData(LS->SOQuery.Get(), D3D12_QUERY_TYPE_SO_STATISTICS_STREAM1, 0, 1, IndirectBuffers[NextIndex], 0);
	//		CL->CopyBufferRegion(IndirectBuffers[NextIndex], 4, LS->ZeroValues, 4, 12);

	//		{
	//			CD3DX12_RESOURCE_BARRIER Barrier[] = {
	//				CD3DX12_RESOURCE_BARRIER::Transition(IndirectBuffers[0],		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
	//				CD3DX12_RESOURCE_BARRIER::Transition(IndirectBuffers[1],		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
	//			};

	//			CL->ResourceBarrier(sizeof(Barrier) / sizeof(Barrier[0]), Barrier);
	//		}

	//		CL->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
	//		//CL->ExecuteIndirect(LS->CommandSignature, 1, IndirectBuffers[Index], 0, nullptr, 0);



	//		if (DrawWireframe){
	//			//CL->IASetVertexBuffers	(0, 1, Input[Index]);

	//			// Draw Final Buffer
	//			CL->IASetVertexBuffers	(0, 1, FinalBufferInput);
	//			CL->SetPipelineState	(RS->GetPSO(TERRAIN_DRAW_PSO_DEBUG));
	//			CL->ExecuteIndirect		(LS->CommandSignature, 1, IndirectBuffers[NextIndex], 0, nullptr, 0);

	//			CL->SetPipelineState	(RS->GetPSO(TERRAIN_DRAW_WIRE_PSO));
	//			CL->ExecuteIndirect		(LS->CommandSignature, 1, IndirectBuffers[NextIndex], 0, nullptr, 0);

	//			// Draw Remainder
	//			CL->IASetVertexBuffers	(0, 1, Input[NextIndex]);
	//			CL->SetPipelineState	(RS->GetPSO(TERRAIN_DRAW_PSO_DEBUG));
	//			CL->ExecuteIndirect		(LS->CommandSignature, 1, IndirectBuffers[Index], 0, nullptr, 0);

	//			CL->SetPipelineState	(RS->GetPSO(TERRAIN_DRAW_WIRE_PSO));
	//			CL->ExecuteIndirect		(LS->CommandSignature, 1, IndirectBuffers[Index], 0, nullptr, 0);
	//		}
	//		else
	//		{
	//			CL->SetPipelineState	(RS->GetPSO(TERRAIN_DRAW_PSO));
	//			// Draw Final Buffer
	//			CL->IASetVertexBuffers(0, 1, FinalBufferInput);
	//			CL->ExecuteIndirect		(LS->CommandSignature, 1, IndirectBuffers[NextIndex], 0, nullptr, 0);

	//			CL->IASetVertexBuffers(0, 1, Input[NextIndex]);
	//			CL->ExecuteIndirect(LS->CommandSignature, 1, IndirectBuffers[Index], 0, nullptr, 0);
	//		}

	//		LS->RegionBuffers[0].IncrementCounter();
	//		LS->RegionBuffers[1].IncrementCounter();
	//		LS->FinalBuffer.IncrementCounter();
	//		LS->TriBuffer.IncrementCounter();
	//		LS->SOCounter_1.IncrementCounter();
	//		LS->SOCounter_2.IncrementCounter();
	//		LS->FB_Counter.IncrementCounter();
	//		LS->IndirectOptions1.IncrementCounter();
	//		LS->IndirectOptions2.IncrementCounter();
	//		LS->SOQuery.IncrementCounter();
	//	}
	//}


	/************************************************************************************************/


	void PushRegion(Landscape* ls, Landscape::ViewableRegion R)
	{
		ls->Regions.push_back(R);
	}


	/************************************************************************************************/

	ID3D12PipelineState* LoadTerrainPSO_Generate(RenderSystem* RS)
	{
		auto ShaderVS             = LoadShader("VPassThrough",			"VPassThrough",			 "vs_5_0", "assets\\tvshader.hlsl");
		auto ShaderGS             = LoadShader("GS_Split",				"GS_Split",			 	 "gs_5_0", "assets\\tvshader.hlsl");
		auto ShaderRegion2Tri     = LoadShader("RegionToTris",			"RegionToTris",	 		 "gs_5_0", "assets\\tvshader.hlsl");
		auto ShaderRegion2Quad    = LoadShader("RegionToQuadPatch",		"RegionToQuadPatch",	 "hs_5_0", "assets\\tvshader.hlsl");
		auto ShaderQuad2Tri       = LoadShader("QuadPatchToTris",		"QuadPatchToTris",		 "ds_5_0", "assets\\tvshader.hlsl");
		auto ShaderPaint          = LoadShader("DebugTerrainPaint",		"TerrainPaint",			 "ps_5_0", "assets\\pshader.hlsl");

		FINALLY
			Release(&ShaderVS);
			Release(&ShaderGS);
			Release(&ShaderRegion2Tri);
			Release(&ShaderRegion2Quad);
			Release(&ShaderQuad2Tri);
			Release(&ShaderPaint);
		FINALLYOVER;

		{
			D3D12_INPUT_ELEMENT_DESC InputElements[] =
			{
				{ "REGION",	  0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 1, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	0,	32, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 2, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	0,	32, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			auto RootSig			= RS->Library.RS4CBVs_SO;
			auto Rast_State			= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			Rast_State.FillMode		= D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
			auto BlendState			= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			auto DepthState			= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
			DepthState.DepthEnable	= true;
			DepthState.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;


			D3D12_SHADER_BYTECODE GCode = { (BYTE*)ShaderRegion2Tri.Blob->GetBufferPointer(),	ShaderRegion2Tri.Blob->GetBufferSize()	};
			D3D12_SHADER_BYTECODE PCode = { (BYTE*)ShaderPaint.Blob->GetBufferPointer(),		ShaderPaint.Blob->GetBufferSize()		};
			D3D12_SHADER_BYTECODE HCode = { (BYTE*)ShaderRegion2Quad.Blob->GetBufferPointer(),	ShaderRegion2Quad.Blob->GetBufferSize()	};
			D3D12_SHADER_BYTECODE DCode = { (BYTE*)ShaderQuad2Tri.Blob->GetBufferPointer(),		ShaderQuad2Tri.Blob->GetBufferSize()	};
			D3D12_SHADER_BYTECODE VCode = { (BYTE*)ShaderVS.Blob->GetBufferPointer(),			ShaderVS.Blob->GetBufferSize()			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
				PSO_Desc.pRootSignature                = RootSig;
				PSO_Desc.VS                            = VCode;
				PSO_Desc.HS							   = HCode;
				PSO_Desc.DS                            = DCode;
				PSO_Desc.PS                            = PCode;
				PSO_Desc.SampleMask                    = UINT_MAX;
				PSO_Desc.PrimitiveTopologyType         = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
				PSO_Desc.NumRenderTargets			   = 5;
				PSO_Desc.RTVFormats[0]				   = DXGI_FORMAT_R8G8B8A8_UNORM;		// Color
				PSO_Desc.RTVFormats[1]				   = DXGI_FORMAT_R8G8B8A8_UNORM;		// Specular
				PSO_Desc.RTVFormats[2]				   = DXGI_FORMAT_R8G8B8A8_UNORM;		// Emissive
				PSO_Desc.RTVFormats[3]				   = DXGI_FORMAT_R8G8_UNORM;			// Roughness and Metal
				PSO_Desc.RTVFormats[4]				   = DXGI_FORMAT_R32G32B32A32_FLOAT;	// Normal
				PSO_Desc.SampleDesc.Count              = 1;
				PSO_Desc.SampleDesc.Quality            = 0;
				PSO_Desc.RasterizerState               = Rast_State;
				PSO_Desc.DepthStencilState			   = DepthState;
				PSO_Desc.BlendState                    = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
				PSO_Desc.DSVFormat                     = DXGI_FORMAT_D32_FLOAT;
				PSO_Desc.InputLayout                   = { InputElements, sizeof(InputElements)/sizeof(InputElements[0]) };
				PSO_Desc.DepthStencilState.DepthEnable = true;
				PSO_Desc.DSVFormat					   = DXGI_FORMAT_D32_FLOAT;
			}

			ID3D12PipelineState* PSO = nullptr;
			auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));

			return PSO;
		}

		return nullptr;
	}

	/************************************************************************************************/


	ID3D12PipelineState* LoadTerrainPSO_GenerateDebug(RenderSystem* RS)
	{
		auto ShaderVS             = LoadShader("VPassThrough",			"VPassThrough",			 "vs_5_0", "assets\\tvshader.hlsl");
		auto ShaderRegion2Tri     = LoadShader("RegionToTris",			"RegionToTris",	 		 "gs_5_0", "assets\\tvshader.hlsl");
		auto ShaderRegion2Quad    = LoadShader("RegionToQuadPatch",		"RegionToQuadPatch",	 "hs_5_0", "assets\\tvshader.hlsl");
		auto ShaderQuad2Tri_Debug = LoadShader("QuadPatchToTris_DEBUG",	"QuadPatchToTris_DEBUG", "ds_5_0", "assets\\tvshader.hlsl");
		auto ShaderPaint          = LoadShader("DebugTerrainPaint",		"TerrainPaint",			 "ps_5_0", "assets\\pshader.hlsl");
		auto ShaderPaint_Wire     = LoadShader("DebugTerrainPaint_2",	"DebugTerrainPaint_2",	 "ps_5_0", "assets\\pshader.hlsl");

		FINALLY
			Release(&ShaderVS);
			Release(&ShaderRegion2Tri);
			Release(&ShaderRegion2Quad);
			Release(&ShaderQuad2Tri_Debug);
			Release(&ShaderPaint);
			Release(&ShaderPaint_Wire);
		FINALLYOVER;

		{
			D3D12_INPUT_ELEMENT_DESC InputElements[] =
			{
				{ "REGION",	  0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 1, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	0,	32, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 2, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	0,	32, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			auto RootSig			= RS->Library.RS4CBVs_SO;
			auto Rast_State			= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			Rast_State.FillMode		= D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
			auto BlendState			= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			auto DepthState			= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
			DepthState.DepthEnable	= true;
			DepthState.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;


			D3D12_SHADER_BYTECODE GCode = { (BYTE*)ShaderRegion2Tri.Blob->GetBufferPointer(),	ShaderRegion2Tri.Blob->GetBufferSize()	};
			D3D12_SHADER_BYTECODE PCode = { (BYTE*)ShaderPaint.Blob->GetBufferPointer(),		ShaderPaint.Blob->GetBufferSize()		};
			D3D12_SHADER_BYTECODE PCode2= { (BYTE*)ShaderPaint_Wire.Blob->GetBufferPointer(),	ShaderPaint_Wire.Blob->GetBufferSize()	};
			D3D12_SHADER_BYTECODE HCode = { (BYTE*)ShaderRegion2Quad.Blob->GetBufferPointer(),	ShaderRegion2Quad.Blob->GetBufferSize()	};
			D3D12_SHADER_BYTECODE VCode = { (BYTE*)ShaderVS.Blob->GetBufferPointer(),			ShaderVS.Blob->GetBufferSize()			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
				PSO_Desc.pRootSignature                = RootSig;
				PSO_Desc.VS                            = VCode;
				PSO_Desc.HS							   = HCode;
				PSO_Desc.DS                            = ShaderQuad2Tri_Debug;
				PSO_Desc.PS                            = PCode;
				PSO_Desc.SampleMask                    = UINT_MAX;
				PSO_Desc.PrimitiveTopologyType         = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
				PSO_Desc.NumRenderTargets			   = 6;
				PSO_Desc.RTVFormats[0]				   = DXGI_FORMAT_R8G8B8A8_UNORM;
				PSO_Desc.RTVFormats[1]				   = DXGI_FORMAT_R8G8B8A8_UNORM;
				PSO_Desc.RTVFormats[2]				   = DXGI_FORMAT_R8G8B8A8_UNORM;
				PSO_Desc.RTVFormats[3]				   = DXGI_FORMAT_R8G8_UNORM;
				PSO_Desc.RTVFormats[4]				   = DXGI_FORMAT_R32G32B32A32_FLOAT;
				PSO_Desc.RTVFormats[5]				   = DXGI_FORMAT_R32G32B32A32_FLOAT;
				PSO_Desc.SampleDesc.Count              = 1;
				PSO_Desc.SampleDesc.Quality            = 0;
				PSO_Desc.RasterizerState               = Rast_State;
				PSO_Desc.DepthStencilState			   = DepthState;
				PSO_Desc.BlendState                    = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
				PSO_Desc.DSVFormat                     = DXGI_FORMAT_D32_FLOAT;
				PSO_Desc.InputLayout                   = { InputElements, sizeof(InputElements)/sizeof(InputElements[0]) };
				PSO_Desc.DepthStencilState.DepthEnable = true;
				PSO_Desc.DSVFormat					   = DXGI_FORMAT_D32_FLOAT;
			}

			ID3D12PipelineState* PSO = nullptr;
			auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));

			return PSO;
		}

		return nullptr;
	}


	/************************************************************************************************/


	ID3D12PipelineState* LoadTerrainPSO_WireDebug(RenderSystem* RS)
	{
		auto ShaderVS             = LoadShader("VPassThrough",			"VPassThrough",			 "vs_5_0", "assets\\tvshader.hlsl");
		auto ShaderRegion2Tri     = LoadShader("RegionToTris",			"RegionToTris",	 		 "gs_5_0", "assets\\tvshader.hlsl");
		auto ShaderRegion2Quad    = LoadShader("RegionToQuadPatch",		"RegionToQuadPatch",	 "hs_5_0", "assets\\tvshader.hlsl");
		auto ShaderQuad2Tri		  = LoadShader("QuadPatchToTris",		"QuadPatchToTris", "ds_5_0", "assets\\tvshader.hlsl");
		auto ShaderQuad2Tri_Debug = LoadShader("QuadPatchToTris_DEBUG",	"QuadPatchToTris_DEBUG", "ds_5_0", "assets\\tvshader.hlsl");
		auto ShaderPaint_Wire     = LoadShader("DebugTerrainPaint_2",	"DebugTerrainPaint_2",	 "ps_5_0", "assets\\pshader.hlsl");

		
		FINALLY
			Release(&ShaderVS);
			Release(&ShaderRegion2Tri);
			Release(&ShaderRegion2Quad);
			Release(&ShaderQuad2Tri_Debug);
			Release(&ShaderPaint_Wire);
		FINALLYOVER;

		{
			D3D12_INPUT_ELEMENT_DESC InputElements[] =
			{
				{ "REGION",	  0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 1, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	0,	32, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 2, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	0,	32, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			auto RootSig			= RS->Library.RS4CBVs_SO;
			auto Rast_State			= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			Rast_State.FillMode		= D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
			auto BlendState			= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			
			auto NoDepthState					= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
			NoDepthState.DepthFunc				= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
			NoDepthState.DepthEnable			= true;

			D3D12_SHADER_BYTECODE DCode = { (BYTE*)ShaderQuad2Tri_Debug.Blob->GetBufferPointer(),	ShaderQuad2Tri_Debug.Blob->GetBufferSize() };
			D3D12_SHADER_BYTECODE GCode = { (BYTE*)ShaderRegion2Tri.Blob->GetBufferPointer(),		ShaderRegion2Tri.Blob->GetBufferSize()	};
			D3D12_SHADER_BYTECODE PCode2= { (BYTE*)ShaderPaint_Wire.Blob->GetBufferPointer(),		ShaderPaint_Wire.Blob->GetBufferSize()	};
			D3D12_SHADER_BYTECODE HCode = { (BYTE*)ShaderRegion2Quad.Blob->GetBufferPointer(),		ShaderRegion2Quad.Blob->GetBufferSize()	};
			D3D12_SHADER_BYTECODE VCode = { (BYTE*)ShaderVS.Blob->GetBufferPointer(),				ShaderVS.Blob->GetBufferSize()			};


			D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
				PSO_Desc.pRootSignature                = RootSig;
				PSO_Desc.VS                            = VCode;
				PSO_Desc.HS							   = HCode;
				PSO_Desc.DS                            = DCode;
				PSO_Desc.PS                            = PCode2;
				PSO_Desc.SampleMask                    = UINT_MAX;
				PSO_Desc.PrimitiveTopologyType         = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
				PSO_Desc.NumRenderTargets			   = 6;
				PSO_Desc.RTVFormats[0]				   = DXGI_FORMAT_R8G8B8A8_UNORM;
				PSO_Desc.RTVFormats[1]				   = DXGI_FORMAT_R8G8B8A8_UNORM;
				PSO_Desc.RTVFormats[2]				   = DXGI_FORMAT_R8G8B8A8_UNORM;
				PSO_Desc.RTVFormats[3]				   = DXGI_FORMAT_R8G8_UNORM;
				PSO_Desc.RTVFormats[4]				   = DXGI_FORMAT_R32G32B32A32_FLOAT;
				PSO_Desc.RTVFormats[5]				   = DXGI_FORMAT_R32G32B32A32_FLOAT;
				PSO_Desc.SampleDesc.Count              = 1;
				PSO_Desc.SampleDesc.Quality            = 0;
				PSO_Desc.RasterizerState               = Rast_State;
				PSO_Desc.DepthStencilState			   = NoDepthState;
				PSO_Desc.BlendState                    = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
				PSO_Desc.DSVFormat                     = DXGI_FORMAT_D32_FLOAT;
				PSO_Desc.InputLayout                   = { InputElements, sizeof(InputElements)/sizeof(InputElements[0]) };
				PSO_Desc.DSVFormat					   = DXGI_FORMAT_D32_FLOAT;
				PSO_Desc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_WIREFRAME;
			}


			ID3D12PipelineState* PSO = nullptr;
			auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));

			return PSO;
		}

		return nullptr;
	}


	/************************************************************************************************/


	ID3D12PipelineState* LoadTerrainPSO_CULL(RenderSystem* RS)
	{
		auto ShaderVS = LoadShader("VPassThrough", "VPassThrough", "vs_5_0", "assets\\tvshader.hlsl");
		auto ShaderGS = LoadShader("GS_Split", "GS_Split", "gs_5_0", "assets\\tvshader.hlsl");
		auto RootSig  = RS->Library.RS4CBVs_SO;


		FINALLY
			Release(&ShaderVS);
			Release(&ShaderGS);
		FINALLYOVER;

		D3D12_INPUT_ELEMENT_DESC InputElements[] =
		{
			{ "REGION",	  0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	0,	32, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 2, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	0,	32, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		/*
		typedef struct D3D12_SO_DECLARATION_ENTRY
		{
		UINT Stream;
		LPCSTR SemanticName;
		UINT SemanticIndex;
		BYTE StartComponent;
		BYTE ComponentCount;
		BYTE OutputSlot;
		} 	D3D12_SO_DECLARATION_ENTRY;
		*/

		D3D12_SO_DECLARATION_ENTRY SO_Entries[] = {
			{ 0, "REGION",		0, 0, 4, 0 },
			{ 0, "TEXCOORD",	0, 0, 4, 0 },
			{ 0, "TEXCOORD",	1, 0, 4, 0 },
			{ 0, "TEXCOORD",	2, 0, 4, 0 },
			{ 1, "REGION",		0, 0, 4, 1 },
			{ 1, "TEXCOORD",	0, 0, 4, 1 },
			{ 1, "TEXCOORD",	1, 0, 4, 1 },
			{ 1, "TEXCOORD",	2, 0, 4, 1 },
		};

		UINT Strides = sizeof(Landscape::ViewableRegion);
		UINT SO_Strides[] = {
			Strides,
			Strides,
			Strides,
			Strides,
		};

		D3D12_STREAM_OUTPUT_DESC SO_Desc = {};{
			SO_Desc.NumEntries		= 8;
			SO_Desc.NumStrides		= 4;
			SO_Desc.pBufferStrides	= SO_Strides;
			SO_Desc.pSODeclaration	= SO_Entries;
		}

		D3D12_SHADER_BYTECODE GCode = { ShaderGS.Blob->GetBufferPointer(),  ShaderGS.Blob->GetBufferSize() };
		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature                = RootSig;
			PSO_Desc.VS                            = { (BYTE*)ShaderVS.Blob->GetBufferPointer(), ShaderVS.Blob->GetBufferSize() };
			PSO_Desc.GS                            = GCode;
			PSO_Desc.SampleMask                    = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType         = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			PSO_Desc.NumRenderTargets              = 0;
			PSO_Desc.SampleDesc.Count              = 1;
			PSO_Desc.SampleDesc.Quality            = 0;
			PSO_Desc.RasterizerState               = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			PSO_Desc.BlendState                    = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.DSVFormat                     = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout                   = { InputElements, sizeof(InputElements) / sizeof(InputElements[0]) };
			PSO_Desc.DepthStencilState.DepthEnable = false;
			PSO_Desc.StreamOutput				   = SO_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));

		return  PSO;
	}


	/************************************************************************************************/


	void InitiateLandscape( RenderSystem* RS, NodeHandle node, Landscape_Desc* desc, iAllocator* alloc, Landscape* out )
	{
		/*
		out->SplitState         = nullptr;
		out->GenerateState      = nullptr;
		out->GenerateStateDebug = nullptr;
		out->WireFrameState     = nullptr;
		*/

		out->Albedo				= {1, 1, 1, .5};
		out->Specular			= {1, 1, 1, 1};
		out->InputBuffer		= nullptr;
		out->SplitCount			= 4;
		out->OutputBuffer		= 0;
		out->Regions.Allocator	= alloc;
		out->HeightMap			= desc->HeightMap;


		RS->RegisterPSOLoader(EPIPELINESTATES::TERRAIN_DRAW_PSO,		LoadTerrainPSO_Generate			);
		RS->RegisterPSOLoader(EPIPELINESTATES::TERRAIN_DRAW_WIRE_PSO,	LoadTerrainPSO_WireDebug		);
		RS->RegisterPSOLoader(EPIPELINESTATES::TERRAIN_DRAW_PSO_DEBUG,	LoadTerrainPSO_GenerateDebug	);
		RS->RegisterPSOLoader(EPIPELINESTATES::TERRAIN_CULL_PSO,		LoadTerrainPSO_CULL				);

		RS->QueuePSOLoad( TERRAIN_DRAW_PSO );
		RS->QueuePSOLoad( TERRAIN_DRAW_WIRE_PSO );
		RS->QueuePSOLoad( TERRAIN_DRAW_PSO_DEBUG );
		RS->QueuePSOLoad( TERRAIN_CULL_PSO );

		// Create Resources
		{
			Landscape::ConstantBufferLayout initial;
			initial.Albedo	  = out->Albedo;
			initial.Specular  = out->Specular;
			initial.PassCount = 1;
			initial.RegionDimensions = { 0.0f, 0.0f };

			FlexKit::ConstantBuffer_desc bd;
			bd.InitialSize      = 2048;
			bd.pInital          = &initial;

			auto FinalBuffer	= CreateStreamOut(RS, SO_BUFFERSIZES);	FinalBuffer._SetDebugName("FinalBuffer");
			auto TriBuffer		= CreateStreamOut(RS, SO_BUFFERSIZES);	TriBuffer._SetDebugName("TriBuffer");

			auto CounterBuffer1 = CreateStreamOut(RS, 512);	CounterBuffer1._SetDebugName("SO Counter 1");
			auto CounterBuffer2 = CreateStreamOut(RS, 512);	CounterBuffer2._SetDebugName("SO Counter 2");
			auto CounterBuffer3	= CreateStreamOut(RS, 512);	CounterBuffer2._SetDebugName("FB SO Counter 3");
			
			//out->ConstantBuffer	  = CreateConstantBuffer(RS, &bd);		out->ConstantBuffer._SetDebugName("TerrainConstants");
			out->RegionBuffers[0] = CreateStreamOut(RS, SO_BUFFERSIZES); out->RegionBuffers[0]._SetDebugName("IntermediateBuffer_1");
			out->RegionBuffers[1] = CreateStreamOut(RS, SO_BUFFERSIZES); out->RegionBuffers[1]._SetDebugName("IntermediateBuffer_2");
			
			out->IndirectOptions1 = CreateStreamOut(RS, SO_BUFFERSIZES); out->IndirectOptions1._SetDebugName("Indirect Arguments 1");
			out->IndirectOptions2 = CreateStreamOut(RS, SO_BUFFERSIZES); out->IndirectOptions2._SetDebugName("Indirect Arguments 2");

			//out->InputBuffer        = CreateVertexBufferResource(RS, KILOBYTE * 2);
			out->FinalBuffer        = FinalBuffer;
			out->TriBuffer          = TriBuffer;
			out->SOCounter_1        = CounterBuffer1;
			out->SOCounter_2		= CounterBuffer2;
			out->FB_Counter			= CounterBuffer3;
			out->SOQuery            = CreateSOQuery(RS);

			SETDEBUGNAME(out->InputBuffer, "LANDSCAPE INPUT BUFFER");

			{
				D3D12_INDIRECT_ARGUMENT_DESC Args[1];
				Args[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

				D3D12_COMMAND_SIGNATURE_DESC Command_Sig = {};
				Command_Sig.ByteStride			= 32;
				Command_Sig.NodeMask			= 0;
				Command_Sig.NumArgumentDescs	= sizeof(Args)/sizeof(Args[0]);
				Command_Sig.pArgumentDescs		= Args;
				ID3D12CommandSignature* CommandSignature; //RS->Library.RS4CBVs_SO
				HRESULT HR = RS->pDevice->CreateCommandSignature(&Command_Sig, nullptr, IID_PPV_ARGS(&CommandSignature));
				CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE CONSTANT BUFFER"));

				out->CommandSignature = CommandSignature;
			}
			{
				D3D12_RESOURCE_DESC Resource_DESC	= CD3DX12_RESOURCE_DESC::Buffer(1024); {
					Resource_DESC.Alignment             = 0;
					Resource_DESC.DepthOrArraySize      = 1;
					Resource_DESC.Dimension             = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
					Resource_DESC.Layout                = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
					Resource_DESC.Width                 = 512;
					Resource_DESC.Height                = 1;
					Resource_DESC.Format                = DXGI_FORMAT_UNKNOWN;
					Resource_DESC.SampleDesc.Count      = 1;
					Resource_DESC.SampleDesc.Quality    = 0;
					Resource_DESC.Flags                 = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
				}

				D3D12_HEAP_PROPERTIES HEAP_Props = {}; {
					HEAP_Props.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
					HEAP_Props.Type                  = D3D12_HEAP_TYPE_DEFAULT;
					HEAP_Props.MemoryPoolPreference  = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
					HEAP_Props.CreationNodeMask      = 0;
					HEAP_Props.VisibleNodeMask		 = 0;
				}

				ID3D12Resource* Resource = nullptr;
				HRESULT HR = RS->pDevice->CreateCommittedResource(
					&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
					&Resource_DESC, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr,
					IID_PPV_ARGS(&Resource));

				SETDEBUGNAME(Resource, "Landscape Rendering Constant Buffer");

				CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE CONSTANT BUFFER"));
				out->ZeroValues = Resource;

				// Zero Out Counters and Indirect Arguments
				{
					static_vector<uint4_32> Zero = {{0u, 1u, 0u, 0u},{ 0u, 0u, 0u, 0u }};
					UpdateResourceByTemp(RS, Resource, Zero.begin(), sizeof(Zero), 1);

					UpdateResourceByTemp(RS, out->IndirectOptions1[0], &Zero, sizeof(Zero), 1);
					UpdateResourceByTemp(RS, out->IndirectOptions1[1], &Zero, sizeof(Zero), 1);
					UpdateResourceByTemp(RS, out->IndirectOptions1[2], &Zero, sizeof(Zero), 1);
					UpdateResourceByTemp(RS, out->IndirectOptions2[0], &Zero, sizeof(Zero), 1);
					UpdateResourceByTemp(RS, out->IndirectOptions2[1], &Zero, sizeof(Zero), 1);
					UpdateResourceByTemp(RS, out->IndirectOptions2[2], &Zero, sizeof(Zero), 1);
				}
				{
					CD3DX12_RESOURCE_BARRIER Barrier[] = {
						CD3DX12_RESOURCE_BARRIER::Transition(out->IndirectOptions1[0], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
						CD3DX12_RESOURCE_BARRIER::Transition(out->IndirectOptions1[1], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
						CD3DX12_RESOURCE_BARRIER::Transition(out->IndirectOptions1[2], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
						CD3DX12_RESOURCE_BARRIER::Transition(out->IndirectOptions2[0], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
						CD3DX12_RESOURCE_BARRIER::Transition(out->IndirectOptions2[1], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
						CD3DX12_RESOURCE_BARRIER::Transition(out->IndirectOptions2[2], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
					};

					auto CL = RS->_GetCurrentCommandList();
					CL->ResourceBarrier(sizeof(Barrier) / sizeof(Barrier[0]), Barrier);
				}

			}
		}
	}


	/************************************************************************************************/


	void ReleaseTerrain(Landscape* ls)
	{
		ls->RegionBuffers[0].Release();
		ls->RegionBuffers[1].Release();
		ls->Regions.Release();

		if ( ls->CommandSignature )	 ls->CommandSignature->Release();	ls->CommandSignature = nullptr;
		if ( ls->ZeroValues )		 ls->ZeroValues->Release();			ls->ZeroValues		 = nullptr;
		if ( ls->ConstantBuffer )	 ls->ConstantBuffer.Release();
		if ( ls->FinalBuffer )		 ls->FinalBuffer.Release(); 
		if ( ls->TriBuffer )		 ls->TriBuffer.Release();
		if ( ls->SOCounter_1 )		 ls->SOCounter_1.Release();
		if ( ls->SOCounter_2 )		 ls->SOCounter_2.Release();
		if ( ls->FB_Counter )		 ls->FB_Counter.Release();
		if ( ls->SOQuery )			 ls->SOQuery.Release();

		if( ls->IndirectOptions1 ) ls->IndirectOptions1.Release();
		if( ls->IndirectOptions2 ) ls->IndirectOptions2.Release();

		if( ls->InputBuffer )	   ls->InputBuffer->Release();
	}


	/************************************************************************************************/

	/*
	cbuffer TerrainConstants : register( b1 )
	{
	float4	 Albedo;   // + roughness
	float4	 Specular; // + metal factor
	Plane Frustum[6];
	};
	*/

	void UploadLandscape(RenderSystem* RS, Landscape* ls, Camera* Camera, bool UploadRegions, bool UploadConstants, int PassCount)
	{
		if (!ls->Regions.size())
			return;

		if(UploadConstants)
		{
			Landscape::ConstantBufferLayout Buffer;

			float3 POS      = GetPositionW(Camera->Node);
			Quaternion Q    = GetOrientation(Camera->Node);
			Buffer.Albedo	= {1, 1, 1, 0.9f};
			Buffer.Specular = {1, 1, 1, 1};

			Buffer.RegionDimensions = {};
			Buffer.Frustum			= GetFrustum(Camera, POS, Q);
			Buffer.PassCount		= PassCount;
			UpdateResourceByTemp(RS, &ls->ConstantBuffer, &Buffer, sizeof(Buffer), 1, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		}

		if (UploadRegions)
		{
			UpdateResourceByTemp(
				RS, ls->InputBuffer, ls->Regions.begin(), 
				sizeof(Landscape::ViewableRegion) * ls->Regions.size(), 
				1, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		}
	}


	/************************************************************************************************/

	/*
		UploadLandscape2
			Allows use of a different Frustum for debugging purposes.
	*/
	void UploadLandscape2(RenderSystem* RS, Landscape* ls, Camera* Camera, Frustum F, bool UploadRegions, bool UploadConstants, int PassCount)
	{
		if (!ls->Regions.size())
			return;

		if (UploadConstants)
		{
			Landscape::ConstantBufferLayout Buffer;

			float3 POS = GetPositionW(Camera->Node);
			Quaternion Q = GetOrientation(Camera->Node);
			Buffer.Albedo = { 1, 1, 1, 0.9f };
			Buffer.Specular = { 1, 1, 1, 1 };

			Buffer.RegionDimensions = {};
			Buffer.Frustum = F;
			Buffer.PassCount = PassCount;
			UpdateResourceByTemp(RS, &ls->ConstantBuffer, &Buffer, sizeof(Buffer), 1, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		}

		if (UploadRegions)
		{
			UpdateResourceByTemp(
				RS, ls->InputBuffer, ls->Regions.begin(),
				sizeof(Landscape::ViewableRegion) * ls->Regions.size(),
				1, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		}
	}


	/************************************************************************************************/
}