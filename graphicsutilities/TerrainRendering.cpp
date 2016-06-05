/**********************************************************************

Copyright (c) 2015 - 2016 Robert May

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

	const static size_t SO_BUFFERSIZES = KILOBYTE * 512;
	void DrawLandscape(RenderSystem* RS, Landscape* LS, size_t splitcount, Camera* C)
	{	
		auto CL = GetCurrentCommandList(RS);
		if(LS->Regions.size())
		{
			/*
			typdef struct D3D12_VERTEX_BUFFER_VIEW Views
			{
				D3D12_GPU_VIRTUAL_ADDRESS	BufferLocation;
				UINT						SizeInBytes;
				UINT						StrideInBytes;
			} 	D3D12_VERTEX_BUFFER_VIEW;
			*/

			D3D12_VERTEX_BUFFER_VIEW SO_1[] = { {
					LS->RegionBuffers[0]->GetGPUVirtualAddress(),
					(UINT)SO_BUFFERSIZES,
					(UINT)32,//sizeof(Landscape::ViewableRegion)
				}, };

			D3D12_VERTEX_BUFFER_VIEW SO_2[] = { {
					LS->RegionBuffers[1]->GetGPUVirtualAddress(),
					(UINT)SO_BUFFERSIZES,
					(UINT)32,//sizeof(Landscape::ViewableRegion)
				}, };

			D3D12_VERTEX_BUFFER_VIEW SO_Initial[] = { {
					LS->InputBuffer->GetGPUVirtualAddress(),
					(UINT)LS->Regions.size() * sizeof(Landscape::ViewableRegion),
					(UINT)32,//sizeof(Landscape::ViewableRegion)
				}, };

			D3D12_VERTEX_BUFFER_VIEW FinalBufferInput[] = { {
					LS->FinalBuffer->GetGPUVirtualAddress(),
					(UINT)SO_BUFFERSIZES,
					(UINT)32,//(UINT)sizeof(Landscape::ViewableRegion)
				}, };

			/*
			typedef struct D3D12_STREAM_OUTPUT_BUFFER_VIEW
			{
				D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
				UINT64 SizeInBytes;
				D3D12_GPU_VIRTUAL_ADDRESS BufferFilledSizeLocation;
			} 	D3D12_STREAM_OUTPUT_BUFFER_VIEW;
			*/

			ID3D12Resource*	IndirectBuffers[] = {
				LS->IndirectOptions1.Get(),
				LS->IndirectOptions2.Get(),
			};

			D3D12_STREAM_OUTPUT_BUFFER_VIEW SOViews1[] = {
				{ LS->RegionBuffers[0]->GetGPUVirtualAddress(),	SO_BUFFERSIZES, LS->SOCounter_1->GetGPUVirtualAddress() },
				{ LS->FinalBuffer->GetGPUVirtualAddress(),		SO_BUFFERSIZES, LS->FB_Counter->GetGPUVirtualAddress()  },
			};

			D3D12_STREAM_OUTPUT_BUFFER_VIEW SOViews2[] = {
				{ LS->RegionBuffers[1]->GetGPUVirtualAddress(),	SO_BUFFERSIZES, LS->SOCounter_2->GetGPUVirtualAddress() },
				{ LS->FinalBuffer->GetGPUVirtualAddress(),		SO_BUFFERSIZES, LS->FB_Counter->GetGPUVirtualAddress()  },
			};

			ID3D12Resource*				Buffers[]	= { LS->RegionBuffers[0].Get(), LS->RegionBuffers[1].Get() };
			D3D12_VERTEX_BUFFER_VIEW*	Input[]		= { SO_1 , SO_2 };
			
			D3D12_STREAM_OUTPUT_BUFFER_VIEW*	Output[]			= { SOViews1, SOViews2 };
			ID3D12Resource*						OutputCounters[]	= { LS->SOCounter_1.Get(), LS->SOCounter_2.Get() };


			{
				D3D12_VERTEX_BUFFER_VIEW SO_Initial[] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, };
				CL->IASetVertexBuffers(0, sizeof(SO_Initial)/sizeof(SO_Initial[0]), SO_Initial);
			}

			// Reset Stream Counters
			CL->CopyResource(LS->FB_Counter.Get(), LS->ZeroValues);
			CL->CopyResource(LS->SOCounter_1.Get(), LS->ZeroValues);
			CL->CopyResource(LS->SOCounter_2.Get(), LS->ZeroValues);

			{
				CD3DX12_RESOURCE_BARRIER Barrier[] = {
					CD3DX12_RESOURCE_BARRIER::Transition(LS->FB_Counter.Get(),  D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_STREAM_OUT),
					CD3DX12_RESOURCE_BARRIER::Transition(LS->SOCounter_1.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_STREAM_OUT),
					CD3DX12_RESOURCE_BARRIER::Transition(LS->SOCounter_2.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_STREAM_OUT),
					CD3DX12_RESOURCE_BARRIER::Transition(LS->FinalBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_STREAM_OUT),
				};

				CL->ResourceBarrier(4, Barrier);
			}

			CL->SetGraphicsRootSignature(RS->Library.RS4CBVs_SO);
			CL->SetPipelineState(LS->SplitState);
			CL->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

			CL->SetGraphicsRootConstantBufferView(1, C->Buffer->GetGPUVirtualAddress());
			CL->SetGraphicsRootConstantBufferView(2, LS->ConstantBuffer->GetGPUVirtualAddress());

			// Prime System
			CL->IASetVertexBuffers(0, 1, SO_Initial);
			CL->BeginQuery(LS->SOQuery.Get(), D3D12_QUERY_TYPE_SO_STATISTICS_STREAM1, 0);
			CL->SOSetTargets(0, 2, Output[0]);

			size_t I = 0;
			for(; I < splitcount; ++I)
			{	
				bool Index = (I % 2) != 0;
				bool NextIndex = Index ? 0 : 1;

				CL->BeginQuery(LS->SOQuery.Get(), D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0, 1);
				
				if(I)
					CL->ExecuteIndirect(LS->CommandSignature, 1, IndirectBuffers[Index], 0, nullptr, 0);
				else
					CL->DrawInstanced(1, 1, 0, 0);

				{
					CD3DX12_RESOURCE_BARRIER Barrier[] = {
						CD3DX12_RESOURCE_BARRIER::Transition(Buffers[Index],			D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
						CD3DX12_RESOURCE_BARRIER::Transition(Buffers[!Index],			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_STREAM_OUT),
						CD3DX12_RESOURCE_BARRIER::Transition(IndirectBuffers[!Index],	D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_DEST),
						CD3DX12_RESOURCE_BARRIER::Transition(LS->SOCounter_1.Get(),		D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_DEST),
						CD3DX12_RESOURCE_BARRIER::Transition(LS->SOCounter_2.Get(),		D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_DEST),
						
					};

					CL->ResourceBarrier(sizeof(Barrier) / sizeof(Barrier[0]), Barrier);
				}

				CL->EndQuery		(LS->SOQuery.Get(), D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0, 1);
				CL->ResolveQueryData(LS->SOQuery.Get(), D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0, 1, 1, IndirectBuffers[NextIndex], 0);
				CL->CopyBufferRegion(IndirectBuffers[NextIndex], 4, LS->ZeroValues, 4, 12);

				CL->CopyBufferRegion(LS->SOCounter_1.Get(), 0, LS->ZeroValues, 16, 16);
				CL->CopyBufferRegion(LS->SOCounter_2.Get(), 0, LS->ZeroValues, 16, 16);

				{
					CD3DX12_RESOURCE_BARRIER Barrier[] = {
						CD3DX12_RESOURCE_BARRIER::Transition(IndirectBuffers[!Index],	D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
						CD3DX12_RESOURCE_BARRIER::Transition(LS->SOCounter_1.Get(),		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_STREAM_OUT),
						CD3DX12_RESOURCE_BARRIER::Transition(LS->SOCounter_2.Get(),		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_STREAM_OUT),
					};

					CL->ResourceBarrier(sizeof(Barrier) / sizeof(Barrier[0]), Barrier);
				}

				CL->IASetVertexBuffers(0, 1, Input[Index]);
				CL->SOSetTargets(0, 2, Output[!Index]);
			}


			{
				D3D12_STREAM_OUTPUT_BUFFER_VIEW NullSO[] = {
					{ 0, 0, 0 },
					{ 0, 0, 0 },
				};

				CL->SOSetTargets(0, 2, NullSO);
			}

			// Do Draw Here
			uint16_t Index		= I % 2;
			uint16_t NextIndex	= Index ? 0 : 1;

			{
				CD3DX12_RESOURCE_BARRIER Barrier[] = {
					CD3DX12_RESOURCE_BARRIER::Transition(Buffers[Index],			D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
					CD3DX12_RESOURCE_BARRIER::Transition(Buffers[!Index],			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_STREAM_OUT),
					CD3DX12_RESOURCE_BARRIER::Transition(LS->FinalBuffer.Get(),		D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
					CD3DX12_RESOURCE_BARRIER::Transition(IndirectBuffers[0],		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,	D3D12_RESOURCE_STATE_COPY_DEST),
					CD3DX12_RESOURCE_BARRIER::Transition(IndirectBuffers[1],		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,	D3D12_RESOURCE_STATE_COPY_DEST),
				};

				CL->ResourceBarrier(sizeof(Barrier) / sizeof(Barrier[0]), Barrier);
			}

			CL->EndQuery(LS->SOQuery.Get(), D3D12_QUERY_TYPE_SO_STATISTICS_STREAM1, 0);
			CL->ResolveQueryData(LS->SOQuery.Get(), D3D12_QUERY_TYPE_SO_STATISTICS_STREAM1, 0, 1, IndirectBuffers[NextIndex], 0);
			CL->CopyBufferRegion(IndirectBuffers[NextIndex], 4, LS->ZeroValues, 4, 12);

			{
				CD3DX12_RESOURCE_BARRIER Barrier[] = {
					CD3DX12_RESOURCE_BARRIER::Transition(IndirectBuffers[0],		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
					CD3DX12_RESOURCE_BARRIER::Transition(IndirectBuffers[1],		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
				};

				CL->ResourceBarrier(sizeof(Barrier) / sizeof(Barrier[0]), Barrier);
			}

			CL->SetPipelineState(LS->GenerateState);
			CL->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
			CL->IASetVertexBuffers(0, 1, Input[Index]);
			//CL->ExecuteIndirect(LS->CommandSignature, 1, IndirectBuffers[Index], 0, nullptr, 0);


			CL->IASetVertexBuffers(0, 1, FinalBufferInput);
			CL->ExecuteIndirect(LS->CommandSignature, 1, IndirectBuffers[NextIndex], 0, nullptr, 0);

			LS->RegionBuffers[0].IncrementCounter();
			LS->RegionBuffers[1].IncrementCounter();
			LS->FinalBuffer.IncrementCounter();
			LS->TriBuffer.IncrementCounter();
			LS->SOCounter_1.IncrementCounter();
			LS->SOCounter_2.IncrementCounter();
			LS->FB_Counter.IncrementCounter();
			LS->IndirectOptions1.IncrementCounter();
			LS->IndirectOptions2.IncrementCounter();
			LS->SOQuery.IncrementCounter();
		}
	}


	/************************************************************************************************/


	void PushRegion(Landscape* ls, Landscape::ViewableRegion R)
	{
		ls->Regions.push_back(R);
	}


	/************************************************************************************************/


	void InitiateLandscape( RenderSystem* RS, NodeHandle node, Landscape_Desc* desc, iAllocator* alloc, Landscape* out )
	{
		out->Albedo				= {1, 1, 1, .5};
		out->Specular			= {1, 1, 1, 1};
		out->InputBuffer		= nullptr;
		out->SplitCount			= 4;
		out->OutputBuffer		= 0;
		out->Regions.Allocator	= alloc;

		// Load Shaders
		{
			{
				// Passthrough VShader
				bool res = false;
				Shader VShader;
				do
				{
					ShaderDesc	VShaderDesc;
					strcpy( VShaderDesc.ID, "PASSTHROUGH");
					strcpy( VShaderDesc.entry, "VPassThrough");
					strcpy( VShaderDesc.shaderVersion, "vs_5_0");
					res = FlexKit::LoadVertexShaderFromFile(RS, "assets\\tvshader.hlsl", &VShaderDesc, &VShader);

					if (!res) {
						char str[256];
						std::cin >> str;
					}
				}while(!res);
				out->VShader = VShader;
			}
			{
				bool res = false;
				Shader GShader;
				do
				{
					ShaderDesc	GShaderDesc;
					strcpy(GShaderDesc.ID, "GS_Split");
					strcpy(GShaderDesc.entry, "GS_Split");
					strcpy(GShaderDesc.shaderVersion, "gs_5_0");
					res = FlexKit::LoadGeometryShaderFromFile(RS, "assets\\tvshader.hlsl", &GShaderDesc, &GShader);

					if (!res) {
						char str[256];
						std::cin >> str;
					}
				} while (!res);
				out->GSubdivShader = GShader;
			}
			{
				bool res = false;
				Shader GShader;
				do
				{
					ShaderDesc	GShaderDesc;
					strcpy(GShaderDesc.ID,		"RegionToTris");
					strcpy(GShaderDesc.entry,	"RegionToTris");
					strcpy(GShaderDesc.shaderVersion, "gs_5_0");
					res = FlexKit::LoadGeometryShaderFromFile(RS, "assets\\tvshader.hlsl", &GShaderDesc, &GShader);

					if (!res) {
						char str[256];
						std::cin >> str;
					}
				} while (!res);
				out->RegionToTri = GShader;
			}
			{
				bool res = false;
				Shader HShader;
				do
				{
					ShaderDesc	GShaderDesc;
					strcpy(GShaderDesc.ID, "RegionToQuadPatch");
					strcpy(GShaderDesc.entry, "RegionToQuadPatch");
					strcpy(GShaderDesc.shaderVersion, "hs_5_0");
					res = FlexKit::LoadGeometryShaderFromFile(RS, "assets\\tvshader.hlsl", &GShaderDesc, &HShader);

					if (!res) {
						char str[256];
						std::cin >> str;
					}
				} while (!res);
				out->HullShader = HShader;
			}
			{
				bool res = false;
				Shader DShader;
				do
				{
					ShaderDesc	GShaderDesc;
					strcpy(GShaderDesc.ID, "QuadPatchToTris");
					strcpy(GShaderDesc.entry, "QuadPatchToTris");
					strcpy(GShaderDesc.shaderVersion, "ds_5_0");
					res = FlexKit::LoadGeometryShaderFromFile(RS, "assets\\tvshader.hlsl", &GShaderDesc, &DShader);

					if (!res) {
						char str[256];
						std::cin >> str;
					}
				} while (!res);
				out->DomainShader = DShader;
			}
			{
				bool res = false;
				Shader DShader;
				do
				{
					ShaderDesc	GShaderDesc;
					strcpy(GShaderDesc.ID, "DebugTerrainPaint");
					strcpy(GShaderDesc.entry, "DebugTerrainPaint");
					strcpy(GShaderDesc.shaderVersion, "ps_5_0");
					res = FlexKit::LoadGeometryShaderFromFile(RS, "assets\\pshader.hlsl", &GShaderDesc, &DShader);

					if (!res) {
						char str[256];
						std::cin >> str;
					}
				} while (!res);
				out->PShader = DShader;
			}
		}
		// PSO Creation
		{
			auto RootSig = RS->Library.RS4CBVs_SO;

			/*
			typedef struct D3D12_INPUT_ELEMENT_DESC
			{
				LPCSTR						SemanticName;
				UINT						SemanticIndex;
				DXGI_FORMAT					Format;
				UINT						InputSlot;
				UINT						AlignedByteOffset;
				D3D12_INPUT_CLASSIFICATION	InputSlotClass;
				UINT						InstanceDataStepRate;
			} 	D3D12_INPUT_ELEMENT_DESC;
			*/

			D3D12_INPUT_ELEMENT_DESC InputElements[] =
			{
				{ "REGION", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,		0,	0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			// Generate Geometry State
			{
				auto Rast_State			= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
				Rast_State.FillMode		= D3D12_FILL_MODE_WIREFRAME;
				auto BlendState			= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
				auto DepthState			= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
				DepthState.DepthEnable	= true;
				DepthState.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;


				D3D12_SHADER_BYTECODE GCode = { (BYTE*)out->RegionToTri.Blob->GetBufferPointer(),  out->RegionToTri.Blob->GetBufferSize() };
				D3D12_SHADER_BYTECODE PCode = { (BYTE*)out->PShader.Blob->GetBufferPointer(), out->PShader.Blob->GetBufferSize() };
				D3D12_SHADER_BYTECODE HCode = { (BYTE*)out->HullShader.Blob->GetBufferPointer(), out->HullShader.Blob->GetBufferSize() };
				D3D12_SHADER_BYTECODE DCode = { (BYTE*)out->DomainShader.Blob->GetBufferPointer(), out->DomainShader.Blob->GetBufferSize() };
				D3D12_SHADER_BYTECODE VCode = { (BYTE*)out->VShader.Blob->GetBufferPointer(), out->VShader.Blob->GetBufferSize() };

				D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
					PSO_Desc.pRootSignature                = RootSig;
					PSO_Desc.VS                            = VCode;
					PSO_Desc.HS							   = HCode;
					PSO_Desc.DS                            = DCode;
					PSO_Desc.PS                            = PCode;
					PSO_Desc.SampleMask                    = UINT_MAX;
					PSO_Desc.PrimitiveTopologyType         = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
					PSO_Desc.NumRenderTargets			   = 4;
					PSO_Desc.RTVFormats[0]				   = DXGI_FORMAT_R8G8B8A8_UNORM;
					PSO_Desc.RTVFormats[1]				   = DXGI_FORMAT_R8G8B8A8_UNORM;
					PSO_Desc.RTVFormats[2]				   = DXGI_FORMAT_R32G32B32A32_FLOAT;
					PSO_Desc.RTVFormats[3]				   = DXGI_FORMAT_R32G32B32A32_FLOAT;
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
				CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE TERRAIN STATE!"));

				out->GenerateState = PSO;
			}
			// GPU-Quad Tree expansion state
			{
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
					{ 1, "REGION",		0, 0, 4, 1 },
					{ 1, "TEXCOORD",	0, 0, 4, 1 },
				};

				UINT SO_Strides[] = {
					32,
					32,
				};

				D3D12_STREAM_OUTPUT_DESC SO_Desc = {};{
					SO_Desc.NumEntries		= 4;
					SO_Desc.NumStrides		= 2;
					SO_Desc.pBufferStrides	= SO_Strides;
					SO_Desc.pSODeclaration	= SO_Entries;
				}

				D3D12_SHADER_BYTECODE GCode = { out->GSubdivShader.Blob->GetBufferPointer(),  out->GSubdivShader.Blob->GetBufferSize() };
				D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
					PSO_Desc.pRootSignature                = RootSig;
					PSO_Desc.VS                            = { (BYTE*)out->VShader.Blob->GetBufferPointer(), out->VShader.Blob->GetBufferSize() };
					PSO_Desc.GS                            = GCode;
					PSO_Desc.SampleMask                    = UINT_MAX;
					PSO_Desc.PrimitiveTopologyType         = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
					PSO_Desc.NumRenderTargets              = 0;
					PSO_Desc.SampleDesc.Count              = 1;
					PSO_Desc.SampleDesc.Quality            = 0;
					PSO_Desc.RasterizerState               = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
					PSO_Desc.BlendState                    = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
					PSO_Desc.DSVFormat                     = DXGI_FORMAT_D32_FLOAT;
					PSO_Desc.InputLayout                   = { InputElements, 2 };
					PSO_Desc.DepthStencilState.DepthEnable = false;
					PSO_Desc.StreamOutput				   = SO_Desc;
				}

				ID3D12PipelineState* PSO = nullptr;
				auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
				CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE TERRAIN STATE!"));

				out->SplitState = PSO;
			}
		}
		// Create Resources
		{
			Landscape::ConstantBufferLayout initial;
			initial.Albedo	 = out->Albedo;
			initial.Specular = out->Specular;
			initial.WT = DirectX::XMMatrixIdentity();

			FlexKit::ConstantBuffer_desc bd;
			bd.InitialSize      = 1024;
			bd.pInital          = &initial;

			auto FinalBuffer	= CreateStreamOut(RS, SO_BUFFERSIZES);	FinalBuffer._SetDebugName("FinalBuffer");
			auto TriBuffer		= CreateStreamOut(RS, SO_BUFFERSIZES);	TriBuffer._SetDebugName("TriBuffer");

			auto CounterBuffer1 = CreateStreamOut(RS, 512);	CounterBuffer1._SetDebugName("SO Counter 1");
			auto CounterBuffer2 = CreateStreamOut(RS, 512);	CounterBuffer2._SetDebugName("SO Counter 2");
			auto CounterBuffer3	= CreateStreamOut(RS, 512);	CounterBuffer2._SetDebugName("FB SO Counter 3");
			
			out->ConstantBuffer	  = CreateConstantBuffer(RS, &bd);		out->ConstantBuffer._SetDebugName("TerrainConstants");
			out->RegionBuffers[0] = CreateStreamOut(RS, SO_BUFFERSIZES); out->RegionBuffers[0]._SetDebugName("IntermediateBuffer_1");
			out->RegionBuffers[1] = CreateStreamOut(RS, SO_BUFFERSIZES); out->RegionBuffers[1]._SetDebugName("IntermediateBuffer_2");
			
			out->IndirectOptions1 = CreateStreamOut(RS, SO_BUFFERSIZES); out->IndirectOptions1._SetDebugName("Indirect Arguments 1");
			out->IndirectOptions2 = CreateStreamOut(RS, SO_BUFFERSIZES); out->IndirectOptions2._SetDebugName("Indirect Arguments 2");

			out->InputBuffer        = CreateVertexBufferResource(RS, KILOBYTE * 2);
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
					Resource_DESC.Width                 = 1024;
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

					auto CL = GetCurrentCommandList(RS);
					CL->ResourceBarrier(sizeof(Barrier) / sizeof(Barrier[0]), Barrier);
				}

			}
		}
	}


	/************************************************************************************************/


	void CleanUpTerrain(SceneNodes* Nodes, Landscape* ls)
	{
		Destroy(&ls->GSubdivShader);
		Destroy(&ls->PassThroughShader);
		Destroy(&ls->Visualiser);
		Destroy(&ls->RegionToTri);
		Destroy(&ls->HullShader);
		Destroy(&ls->DomainShader);

		ls->RegionBuffers[0].Release();
		ls->RegionBuffers[1].Release();
		ls->Regions.Release();

		if (ls->CommandSignature)	ls->CommandSignature->Release();	ls->CommandSignature = nullptr;
		if (ls->ZeroValues)			ls->ZeroValues->Release();			ls->ZeroValues = nullptr;
		if (ls->ConstantBuffer)		ls->ConstantBuffer.Release();
		if (ls->FinalBuffer)		ls->FinalBuffer.Release(); 
		if (ls->TriBuffer)			ls->TriBuffer.Release();
		if (ls->SOCounter_1)		ls->SOCounter_1.Release();
		if (ls->SOCounter_2)		ls->SOCounter_2.Release();
		if (ls->FB_Counter)			ls->FB_Counter.Release();

		if (ls->GenerateState)	ls->GenerateState->Release();
		if (ls->SplitState)		ls->SplitState->Release();

		if (ls->SOQuery) ls->SOQuery.Release();

		if(ls->IndirectOptions1) ls->IndirectOptions1.Release();
		if(ls->IndirectOptions2) ls->IndirectOptions2.Release();

		if (ls->InputBuffer) ls->InputBuffer->Release();
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

	void UploadLandscape(RenderSystem* RS, Landscape* ls, SceneNodes* Nodes, Camera* Camera, bool UploadRegions, bool UploadConstants)
	{
		if (!ls->Regions.size())
			return;

		if(UploadConstants)
		{
			struct BufferLayout
			{
				float4	 Albedo;   // + roughness
				float4	 Specular; // + metal factor
				Frustum	 Frustum;
			}Buffer;

			float3 POS = FlexKit::GetPositionW(Nodes, Camera->Node);
			Quaternion Q;
			GetOrientation(Nodes, Camera->Node, &Q);
			Buffer.Albedo	= {1, 1, 1, 0.9f};
			Buffer.Specular = {1, 1, 1, 1};
			Buffer.Frustum  = GetFrustum(Camera, POS, Q);

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