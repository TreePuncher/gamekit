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
#include <stdio.h>

#pragma warning( disable : 4267 )

namespace FlexKit
{
	/************************************************************************************************/


	void DrawLandscape(Context& ctx, Landscape* ls, Camera* c, size_t splitcount, FINALPASSDRAWFN FinalDraw, void* _ptr)
	{
		UINT Offsets[] =
		{ 0, 0 };

		UINT Strides[] =
		{ 32, 32 };
		
		SetTopology(ctx, EInputTopology::EIT_POINT);
		SetShader(ctx, FlexKit::SHADER_TYPE::SHADER_TYPE_Pixel);
		
		ctx->IASetInputLayout(ls->IL_ptr);
		ctx->IASetVertexBuffers(0, 1, &ls->InputBuffer, Strides, Offsets);
		VSPushConstantBuffer(ctx, c->Buffer);
		GSPushConstantBuffer(ctx, c->Buffer);
		GSPushConstantBuffer(ctx, ls->ConstantBuffer);

		SetShader(ctx, ls->GSubdivShader);
		SetShader(ctx, ls->PassThroughShader);

		AddStreamOut(ctx, ls->RegionBuffers[0], Offsets);
		Draw(ctx, 1);
		
		size_t Out = 0;
		for (int I = 0; I < splitcount; I++)
		{
			size_t In = I % 2;
			Out = (I + 1) % 2;
			PopStreamOut(ctx);
			ctx->IASetVertexBuffers(0, 1, &ls->RegionBuffers[In], Strides, Offsets);
			AddStreamOut(ctx, ls->RegionBuffers[Out], Offsets);
			DrawAuto(ctx);
		}

		PopStreamOut(ctx);
		PopStreamOut(ctx);

		SetShader(ctx, ls->RegionToTri);
		PSPushConstantBuffer(ctx, c->Buffer);
		PSPushConstantBuffer(ctx, ls->ConstantBuffer);

		ctx->IASetVertexBuffers(0, 1, &ls->RegionBuffers[Out], Strides, Offsets);
		FinalDraw(ctx, _ptr);

		// Reset pipeline
		GSPopConstantBuffer(ctx);
		GSPopConstantBuffer(ctx);
		PSPopConstantBuffer(ctx);
		PSPopConstantBuffer(ctx);
		VSPopConstantBuffer(ctx);

		SetShader(ctx, SHADER_TYPE::SHADER_TYPE_Pixel);
		SetShader(ctx, SHADER_TYPE::SHADER_TYPE_Vertex);
	}


	/************************************************************************************************/


	void CreateTerrain( RenderSystem* RS, FlexKit::Landscape* out, NodeHandle node )
	{
		out->Albedo			= {1, 1, 1, .5};
		out->Specular		= {1, 1, 1, 1};
		out->InputBuffer	= nullptr;
		out->SplitCount		= 4;
		out->OutputBuffer	= 0;
		out->Strides.push_back(sizeof(Landscape::ViewableRegion));
		out->Offsets.push_back(0);

		{
			Landscape::ConstantBufferLayout initial;
			initial.Albedo	 = out->Albedo;
			initial.Specular = out->Specular;

			initial.WT = DirectX::XMMatrixIdentity();

			FlexKit::ConstantBuffer_desc bd;
			bd.Dest             = FlexKit::PIPELINE_DEST_GS;
			bd.Freq             = bd.PERFRAME;
			bd.InitialSize      = 1024;
			bd.pInital          = &initial;
			bd.Structured       = false;
			bd.StructureSize    = 0;

			out->ConstantBuffer = FlexKit::CreateConstantBuffer(RS, &bd);

			char* DebugName = "TerrainConstants";
			FK_ASSERT(0);
			//SetDebugName(out->ConstantBuffer, DebugName, strlen(DebugName));
		}

		{
			D3D11_BUFFER_DESC BDesc;
			BDesc.BindFlags           = D3D11_BIND_VERTEX_BUFFER;
			BDesc.ByteWidth	          = sizeof(Landscape::ViewableRegion);
			BDesc.CPUAccessFlags      = 0;
			BDesc.StructureByteStride = 0;
			BDesc.Usage               = D3D11_USAGE_DEFAULT;
			BDesc.MiscFlags           = 0;

			Landscape::ViewableRegion Parent ={ {0, 0, 0, (4096)}, {0, 0, 0}, {0} };

			D3D11_SUBRESOURCE_DATA SR;
			SR.pSysMem = &Parent;

			ID3D11Buffer* InputBuffer = nullptr;

			FK_ASSERT(0);
			/*
			HRESULT HR;
			HR = RS->pDevice->CreateBuffer(&BDesc, &SR, &InputBuffer);
			if (FAILED(HR))
				FK_ASSERT(0, "FAILED TO BUILD BUFFER: CREATETERRAIN()");
			*/

			out->InputBuffer = InputBuffer;
		}
		Shader GSubdivShader;
		ID3D11InputLayout*	Layout = nullptr;
		{
			FlexKit::ShaderDesc SDesc;
			strcpy(SDesc.entry, "VPassThrough");
			strcpy(SDesc.ID,	"VPassThrough");
			strcpy(SDesc.shaderVersion, "vs_5_0");

			bool res = false;
			{
				Shader PassThroughShader;
				do
				{
					printf("LoadingShader - VShader VPassThrough -\n");
					res = FlexKit::LoadVertexShaderFromFile(RS, "assets\\tvshader.hlsl", &SDesc, &PassThroughShader);
					if (!res)
					{
						std::cout << "Failed to Load\n Press Enter to try again\n";
						char str[100];
						std::cin >> str;
					}
				} while (!res);
				out->PassThroughShader = PassThroughShader;

				strcpy(SDesc.entry, "CPVisualiser");
				strcpy(SDesc.ID,	"CPVisualiser");
				strcpy(SDesc.shaderVersion, "vs_5_0");

				Shader VisualiserShader;
				do
				{
					printf("LoadingShader - VisualiserShader VPassThrough -\n");
					res = FlexKit::LoadVertexShaderFromFile(RS, "assets\\tvshader.hlsl", &SDesc, &VisualiserShader);
					if (!res) std::cout << "Failed to Load\n";
				} while (!res);
				out->Visualiser = VisualiserShader;

				static_vector<D3D11_INPUT_ELEMENT_DESC> InputDesc = 
				{
					{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
					{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }
				};

				FK_ASSERT(0);
				//auto HR = RS->pDevice->CreateInputLayout(InputDesc.begin(), InputDesc.size(), PassThroughShader.Blob->GetBufferPointer(), PassThroughShader.Blob->GetBufferSize(), &Layout);
				//if ( FAILED( HR ) )
				//	FK_ASSERT( false, "FAILED TO CREATE INPUT LAYOUT" );

				out->IL_ptr = Layout;
			}

			// ------------ Create StreamOut Buffers ------------
			{
				D3D11_BUFFER_DESC bDesc =
				{
					MEGABYTE * 16,
					D3D11_USAGE_DEFAULT,
					D3D11_BIND_STREAM_OUTPUT | D3D11_BIND_VERTEX_BUFFER,
					0, 0, 0
				};

				ID3D11Buffer* Buffer = nullptr;

				char* D1Text = "RegionBuffer1";
				char* D2Text = "RegionBuffer2";
				
				FK_ASSERT(0);
				/*
				HRESULT HR;
				HR = RS->pDevice->CreateBuffer(&bDesc, nullptr, &out->RegionBuffers[0]); FK_ASSERT( FAILED(HR), "FAILED TO CREATE DEVICE BUFFER" );
				HR = RS->pDevice->CreateBuffer(&bDesc, nullptr, &out->RegionBuffers[1]); FK_ASSERT( FAILED(HR), "FAILED TO CREATE DEVICE BUFFER" );
				
				SetDebugName(out->RegionBuffers[0], D1Text, strnlen_s(D1Text, 32));
				SetDebugName(out->RegionBuffers[1], D1Text, strnlen_s(D2Text, 32));
				*/
			}
			//

			D3D11_SO_DECLARATION_ENTRY SOentries[] = 
			{ 
				{ 0, "POSITION", 0, 0, 4, 0 },
				{ 0, "TEXCOORD", 0, 0, 4, 0 },
			};

			FlexKit::SODesc sd	= { 0 };
			sd.Descs			= SOentries;
			sd.Element_Count	= 2;
			sd.SO_Count			= 1;
			sd.Strides[0]		= 32;
			sd.Strides[1]		= 32;
			sd.Flags			= D3D11_SO_NO_RASTERIZED_STREAM;

			strcpy(SDesc.entry,			"GS_Split");
			strcpy(SDesc.ID,			"GS_Split");
			strcpy(SDesc.shaderVersion, "gs_5_0");

			do
			{
				printf("LoadingShader - GShader SUBDIV -\n");
				res = FlexKit::LoadGeometryWithSOShaderFromFile(RS, "assets\\tvshader.hlsl", &SDesc, &sd, &GSubdivShader );
				FK_ASSERT( false, "FAILED TO LOAD GSHADER SUBDIV" );
			} while (!res);
			out->GSubdivShader = GSubdivShader;

			strcpy(SDesc.entry,			"RegionToTris");
			strcpy(SDesc.ID,			"RegionToTris");
			strcpy(SDesc.shaderVersion, "gs_5_0");

			Shader RegionToTri;
			do
			{
				printf("LoadingShader - RegionToTris -\n");
				res = FlexKit::LoadGeometryShaderFromFile(RS, "assets\\tvshader.hlsl",&SDesc, &RegionToTri );


				FK_ASSERT( false, "FAILED TO LOAD RegionToTris SUBDIV" );
			} while (!res);
			out->RegionToTri = RegionToTri;

			strcpy(SDesc.entry,			"PSMain");
			strcpy(SDesc.ID,			"PSHADER");
			strcpy(SDesc.shaderVersion, "ps_5_0");
		}
	}


	/************************************************************************************************/


	void CleanUpTerrain(SceneNodes* Nodes, Landscape* ls)
	{
		FlexKit::Destroy(&ls->GSubdivShader);
		FlexKit::Destroy(&ls->PassThroughShader);
		FlexKit::Destroy(&ls->Visualiser);
		FlexKit::Destroy(&ls->RegionToTri);

		if(ls->Query)		ls->Query->Release();
		if(ls->InputBuffer)	ls->InputBuffer->Release();

		if (ls->IL_ptr)
			ls->IL_ptr->Release();

		for (auto B : ls->RegionBuffers)
			B->Release();
	}


	/************************************************************************************************/


	void UpdateLandscape(RenderSystem* RS, Landscape* ls, SceneNodes* Nodes, Camera* Camera)
	{
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
			MapWriteDiscard(RS, (char*)&Buffer, sizeof(BufferLayout), ls->ConstantBuffer);

			ls->Dirty = false;
		}
	}


	/************************************************************************************************/
}