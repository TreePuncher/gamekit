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


#include "SSReflections.h"
#include <d3dx12.h>

namespace FlexKit
{
	ID3D12PipelineState* LoadSSReflectionState(RenderSystem* RS)
	{
		ID3D12PipelineState* PSO = nullptr;
		D3D12_COMPUTE_PIPELINE_STATE_DESC Desc = {};
		Shader CS = LoadShader("Trace", "SSR", "cs_5_0", "assets\\SSRCompute.hlsl");

		Desc.pRootSignature = RS->Library.ShadingRTSig;
		Desc.CS = CS;
		Desc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;

		HRESULT HR = RS->pDevice->CreateComputePipelineState(&Desc, IID_PPV_ARGS(&PSO));
		
		return PSO;
	}

	void InitiateSSReflectionTracer(RenderSystem* RS, uint2 WH, SSReflectionBuffers* Out)
	{
		Texture2D_Desc  desc;
		desc.Height		  = WH[1];
		desc.Width		  = WH[0];
		desc.Read         = false;
		desc.Write        = false;
		desc.CV           = true;
		desc.RenderTarget = true;
		desc.MipLevels    = 1;
		desc.UAV		  = true;
		//desc.InitiateState = ;

		Out->CurrentBuffer = 0;

		for(size_t I = 0; I < 3; ++I)
		{	// Alebdo Buffer
			desc.Format = FORMAT_2D::R32G32B32A32_FLOAT;
			Out->ReflectionBuffer[I] = CreateTexture2D(RS, &desc);
			FK_ASSERT(Out->ReflectionBuffer[I]);
			SETDEBUGNAME(Out->ReflectionBuffer[I], "Reflection Buffer");
		}

		RS->RegisterPSOLoader	(EPIPELINESTATES::SSREFLECTIONS, LoadSSReflectionState);
		RS->QueuePSOLoad		(EPIPELINESTATES::SSREFLECTIONS);
	}

	void ReleaseSSR(SSReflectionBuffers* SSR)
	{
		for( auto& I : SSR->ReflectionBuffer )
			I->Release();
	}


	Texture2D*	GetCurrentBuffer(SSReflectionBuffers* Buffers)
	{
		return Buffers->ReflectionBuffer + Buffers->CurrentBuffer;
	}


	void IncrementIndex(SSReflectionBuffers* Buffers)
	{
		Buffers->CurrentBuffer = (Buffers->CurrentBuffer + 1)%3;
	}


	void ClearBuffer(RenderSystem* RS, ID3D12GraphicsCommandList* CL, SSReflectionBuffers* Buffers)
	{
		auto GPUPOS		= RS->_GetGPUDescTableCurrentPosition_GPU();
		auto POS		= RS->_ReserveGPUDescHeap();

		PushUAV2DToDescHeap(RS, *GetCurrentBuffer(Buffers), POS, DXGI_FORMAT_R32G32B32A32_FLOAT);

		float4 ClearValues(0, 0, 0, 0);
		CL->ClearUnorderedAccessViewFloat(GPUPOS, POS, *GetCurrentBuffer(Buffers), ClearValues, 0, nullptr);
		int x = 0;
	}


	void TraceReflections(RenderSystem* RS, ID3D12GraphicsCommandList* CL, 
		TiledDeferredRender* In, const Camera* C,
		const PointLightBuffer* PLB, const SpotLightBuffer* SPLB, uint2 WH, SSReflectionBuffers* SSR)
	{
		auto FrameResources  = RS->_GetCurrentFrameResources();
		auto BufferIndex	 = In->CurrentBuffer;
		auto& CurrentGBuffer = In->GBuffers[BufferIndex];

		auto DescTable					= RS->_GetDescTableCurrentPosition_GPU();
		auto TablePOS					= RS->_ReserveDescHeap(11);
		auto DispatchDimensions			= uint2{WH[0]/32, WH[1]/12 };
		auto CurrentReflectionBuffer	= SSR->ReflectionBuffer[SSR->CurrentBuffer];

		// The Max is to quiet a error if a no Lights are passed
		TablePOS = PushSRVToDescHeap	(RS, PLB->Resource, TablePOS,  max(PLB->size(), 1),  PLB_Stride);	
		TablePOS = PushSRVToDescHeap	(RS, SPLB->Resource, TablePOS, max(SPLB->size(), 1), SPLB_Stride);

		TablePOS = PushTextureToDescHeap	(RS, CurrentGBuffer.ColorTex,			TablePOS);
		TablePOS = PushTextureToDescHeap	(RS, CurrentGBuffer.SpecularTex,		TablePOS);
		TablePOS = PushTextureToDescHeap	(RS, CurrentGBuffer.OutputBuffer,		TablePOS);
		TablePOS = PushTextureToDescHeap	(RS, CurrentGBuffer.RoughnessMetal,		TablePOS);
		TablePOS = PushTextureToDescHeap	(RS, CurrentGBuffer.NormalTex,			TablePOS);
		TablePOS = Push2DSRVToDescHeap		(RS, CurrentGBuffer.LightTilesBuffer,	TablePOS);
		TablePOS = PushUAV2DToDescHeap		(RS, CurrentReflectionBuffer,			TablePOS, DXGI_FORMAT_R32G32B32A32_FLOAT);
		//TablePOS = PushUAV2DToDescHeap		(RS, CurrentGBuffer.OutputBuffer,		TablePOS, DXGI_FORMAT_R32G32B32A32_FLOAT);

		auto ConstantBuffers = DescTable;

		TablePOS = PushCBToDescHeap			(RS, C->Buffer.Get(), TablePOS,						CALCULATECONSTANTBUFFERSIZE(Camera::BufferLayout));
		TablePOS = PushCBToDescHeap			(RS, In->Shading.ShaderConstants.Get(), TablePOS,	CALCULATECONSTANTBUFFERSIZE(GBufferConstantsLayout));
		
		ID3D12DescriptorHeap* Heaps[] = { FrameResources->DescHeap.DescHeap };

		{
			CD3DX12_RESOURCE_BARRIER Barrier[] = {
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.OutputBuffer,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
			CD3DX12_RESOURCE_BARRIER::Transition(CurrentReflectionBuffer,
					D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS, -1,
					D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE) };
			
			CL->ResourceBarrier(1, Barrier);
		}


		ClearBuffer(RS, CL, SSR);
		CL->SetComputeRootSignature(RS->Library.ShadingRTSig);
		CL->SetDescriptorHeaps(1, Heaps);
		CL->SetComputeRootDescriptorTable(DSRP_DescriptorTable, DescTable);
		CL->SetPipelineState(RS->GetPSO(EPIPELINESTATES::SSREFLECTIONS));
		CL->Dispatch(DispatchDimensions[0], DispatchDimensions[1], 1);

		{
			CD3DX12_RESOURCE_BARRIER Barrier[] = {
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.OutputBuffer,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),

			CD3DX12_RESOURCE_BARRIER::Transition(CurrentReflectionBuffer,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE) };
			
			CL->ResourceBarrier(1, Barrier);
		};
	}
}