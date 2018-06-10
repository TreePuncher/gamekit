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

#include "ForwardRendering.h"
#include <d3dx12.h>

namespace FlexKit
{

	void InitiateForwardPass( RenderSystem* RS, ForwardPass_DESC* FBdesc, ForwardRender* out )
	{
		// Load Shaders
		out->VShader = LoadShader("VMain", "Forward", "vs_5_0", "assets\\ForwardPassVShader.hlsl");
		out->PShader = LoadShader("PMain", "Forward", "ps_5_0", "assets\\ForwardPassPShader.hlsl");

		ID3DBlob* SignatureDescBlob	= nullptr;
		ID3DBlob* ErrorBlob			= nullptr;

		CD3DX12_DESCRIPTOR_RANGE ConstantsBuffer[2];
		CD3DX12_DESCRIPTOR_RANGE ShaderResources[1];
		CD3DX12_ROOT_PARAMETER	 Parameters[5];

		ConstantsBuffer[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
		ConstantsBuffer[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
		ShaderResources[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		Parameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		Parameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		Parameters[2].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
		Parameters[3].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
		Parameters[4].InitAsShaderResourceView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc = CD3DX12_ROOT_SIGNATURE_DESC(CD3DX12_DEFAULT());
		RootSignatureDesc.Init(RootSignatureDesc, 5, Parameters, 0, nullptr, rootSignatureFlags);
		HRESULT HR = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &SignatureDescBlob, &ErrorBlob );

		if (FAILED(HR)){
			printf((char*)ErrorBlob->GetBufferPointer());
			assert(0);
		}

		if (ErrorBlob)
			ErrorBlob->Release();

		ID3DBlob* SignatureBlob = nullptr;
		HR = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &SignatureBlob, &ErrorBlob);

		ID3D12RootSignature* RootSig = nullptr;
		HR = RS->pDevice->CreateRootSignature(0, SignatureDescBlob->GetBufferPointer(), 
												 SignatureDescBlob->GetBufferSize(), IID_PPV_ARGS(&RootSig));

		SETDEBUGNAME(RootSig, "PassRTSig");

		SignatureDescBlob->Release();
		SignatureBlob->Release();

		ID3D12DescriptorHeap*		CBDescHeap = nullptr;
		D3D12_DESCRIPTOR_HEAP_DESC	CBDescHeapDesc = {};

		CBDescHeapDesc.NumDescriptors = 1;
		CBDescHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		CBDescHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		CBDescHeapDesc.NodeMask       = 0;

		RS->pDevice->CreateDescriptorHeap(&CBDescHeapDesc, IID_PPV_ARGS(&CBDescHeap));
		out->RenderTarget	= FBdesc->OutputTarget;
		out->DepthTarget	= FBdesc->DepthBuffer;
		out->CBDescHeap		= CBDescHeap;
		out->PassRTSig		= RootSig;

		SETDEBUGNAME(CBDescHeap, "ForwardRenderCBHeap");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = 
		{
			{"POSITION", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	  1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"NORMAL",	 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};

		D3D12_RASTERIZER_DESC		Rast_Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); {
		}

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);{
			Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
			//Depth_Desc.DepthEnable	= false;
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {};
		PSO_Desc.pRootSignature                      = RootSig;
		PSO_Desc.VS                                  = out->VShader;
		PSO_Desc.PS                                  = out->PShader;
		PSO_Desc.RasterizerState                     = Rast_Desc;
		PSO_Desc.BlendState		                     = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		PSO_Desc.SampleMask		                     = UINT_MAX;
		PSO_Desc.PrimitiveTopologyType               = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		PSO_Desc.NumRenderTargets	                 = 1;
		PSO_Desc.RTVFormats[0]		                 = out->RenderTarget->Format;
		PSO_Desc.SampleDesc.Count	                 = 1;
		PSO_Desc.SampleDesc.Quality	                 = 0;
		PSO_Desc.DSVFormat			                 = out->DepthTarget->Buffer[out->DepthTarget->CurrentBuffer].Format;
		PSO_Desc.InputLayout		                 = {InputElements, 3};
		PSO_Desc.DepthStencilState					 = Depth_Desc;

		ID3D12PipelineState* PSO = nullptr;
		HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		if (FAILED(HR))
		{	// Failed
			FK_ASSERT(0);
		}

		HR = RS->pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&out->CommandAllocator));							FK_ASSERT(SUCCEEDED(HR));
		HR = RS->pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, out->CommandAllocator, PSO, IID_PPV_ARGS(&out->CommandList));	FK_ASSERT(SUCCEEDED(HR));

		out->CommandList->Close();

		out->PSO = PSO;
	}


	/************************************************************************************************/


	void ReleaseForwardPass(ForwardRender* FP)
	{
		SAFERELEASE(FP->CommandList);
		SAFERELEASE(FP->CommandAllocator);
		SAFERELEASE(FP->PSO);
		SAFERELEASE(FP->PassRTSig);
		SAFERELEASE(FP->CBDescHeap);

		SAFERELEASE(FP->VShader.Blob);
		SAFERELEASE(FP->PShader.Blob);
	}


	/************************************************************************************************/


	void ForwardPass(PVS* _PVS, ForwardRender* Pass, RenderSystem* RS, Camera* C, float4& ClearColor, PointLightList* PLB, GeometryTable* GT)
	{
		return;

		auto CL = RS->_GetCurrentCommandList();
		if(!_PVS->size())
			return;

		{	// Setup State
			assert(0);
			CL->SetPipelineState(Pass->PSO);
			CL->SetGraphicsRootSignature(Pass->PassRTSig);
			//CL->OMSetRenderTargets(1, &GetBackBufferView(Pass->RenderTarget), true, &RS->DefaultDescriptorHeaps.DSVDescHeap->GetCPUDescriptorHandleForHeapStart());
			SetViewport(CL, GetBackBufferTexture(Pass->RenderTarget));
		}

		CL->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//CL->SetGraphicsRootShaderResourceView(4, PLB->Resource->GetGPUVirtualAddress());

		for(auto& PV : *_PVS)
		{
			auto E                  = (Drawable*)PV.D;
			TriMesh* CurrentMesh	= GetMesh(GT, E->MeshHandle);
			size_t IBIndex          = CurrentMesh->VertexBuffer.MD.IndexBuffer_Index;
			size_t IndexCount       = CurrentMesh->IndexCount;

			D3D12_INDEX_BUFFER_VIEW		IndexView;
			IndexView.BufferLocation	= CurrentMesh->VertexBuffer.VertexBuffers[IBIndex].Buffer->GetGPUVirtualAddress();
			IndexView.Format			= DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
			IndexView.SizeInBytes		= IndexCount * 32;

			//CL->SetGraphicsRootConstantBufferView(0, C->Buffer->GetGPUVirtualAddress());
			//CL->SetGraphicsRootConstantBufferView(1, E->VConstants->GetGPUVirtualAddress()); 
			CL->IASetIndexBuffer(&IndexView);

			static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;
			D3D12_VERTEX_BUFFER_VIEW VBView;
			for (size_t I = 0; I < CurrentMesh->VertexBuffer.VertexBuffers.size(); ++I)
			{
				if (!CurrentMesh->VertexBuffer.VertexBuffers[I].Buffer)
					continue;

				auto& VB = CurrentMesh->VertexBuffer.VertexBuffers[I];
				VBView.BufferLocation	= VB.Buffer->GetGPUVirtualAddress();
				VBView.SizeInBytes		= VB.BufferSizeInBytes;
				VBView.StrideInBytes	= VB.BufferStride;
				VBViews.push_back(VBView);
			}

			CL->IASetVertexBuffers(0, VBViews.size(), VBViews.begin() );
			CL->DrawIndexedInstanced(IndexCount, 1, 0, 0, 0);
		}

		RS->_GetCurrentCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetBackBufferResource(Pass->RenderTarget),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	}
}