/**********************************************************************

Copyright (c) 2018 Robert May

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

#include "TextRendering.h"
#include "Graphics.h"

#include <memory>
#include <Windows.h>
#include <windowsx.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3d11sdklayers.h>
#include <d3d11shader.h>


namespace FlexKit
{

	// Create/Load Text Rendering State
	ID3D12PipelineState* LoadSpriteTextPSO(RenderSystem* RS)
	{
		Shader DrawTextVShader = RS->LoadShader("VTextMain", "vs_6_0", "assets\\Shaders\\TextRendering.hlsl");
		Shader DrawTextGShader = RS->LoadShader("GTextMain", "gs_6_0", "assets\\Shaders\\TextRendering.hlsl");
		Shader DrawTextPShader = RS->LoadShader("PTextMain", "ps_6_0", "assets\\Shaders\\TextRendering.hlsl");

		HRESULT HR;
		ID3D12PipelineState* PSO = nullptr;

		/*
		struct TextEntry
		{
			float2 POS;
			float2 Size;
			float2 TopLeftUV;
			float2 BottomRightUV;
			float4 Color;
		};
		*/

		D3D12_INPUT_ELEMENT_DESC InputElements[5] =	{
			{ "POS",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,			0, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{ "WH",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,			0, 8,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{ "UVTL",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,			0, 16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{ "UVBL",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,			0, 24, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{ "COLOR",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,		0, 32, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};

		D3D12_RASTERIZER_DESC Rast_Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);{
		}

		D3D12_DEPTH_STENCIL_DESC Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);{
			Depth_Desc.DepthEnable	 = false;
			Depth_Desc.StencilEnable = false;
		}

		D3D12_RENDER_TARGET_BLEND_DESC TransparencyBlend_Desc;{
			TransparencyBlend_Desc.BlendEnable           = true;
				
			TransparencyBlend_Desc.SrcBlend              = D3D12_BLEND_SRC_ALPHA;
			TransparencyBlend_Desc.DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
			TransparencyBlend_Desc.BlendOp               = D3D12_BLEND_OP_ADD;
			//TransparencyBlend_Desc.BlendOp               = D3D12_BLEND_OP_;
			TransparencyBlend_Desc.SrcBlendAlpha         = D3D12_BLEND_ZERO;
			TransparencyBlend_Desc.DestBlendAlpha        = D3D12_BLEND_ZERO;
			TransparencyBlend_Desc.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
			TransparencyBlend_Desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			TransparencyBlend_Desc.LogicOpEnable         = false;
			TransparencyBlend_Desc.LogicOp               = D3D12_LOGIC_OP_NOOP;
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC PSO_Desc = {};{
			PSO_Desc.pRootSignature             = RS->Library.RS6CBVs4SRVs;
			PSO_Desc.VS                         = DrawTextVShader;
			PSO_Desc.GS                         = DrawTextGShader;
			PSO_Desc.PS                         = DrawTextPShader;
			PSO_Desc.RasterizerState            = Rast_Desc;
			PSO_Desc.BlendState                 = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask                 = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType      = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			PSO_Desc.NumRenderTargets           = 1;
			PSO_Desc.RTVFormats[0]              = DXGI_FORMAT_R16G16B16A16_FLOAT;
			PSO_Desc.BlendState.RenderTarget[0] = TransparencyBlend_Desc;
			PSO_Desc.SampleDesc.Count           = 1;
			PSO_Desc.SampleDesc.Quality         = 0;
			PSO_Desc.DSVFormat                  = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout                = { InputElements, 5 };
			PSO_Desc.DepthStencilState          = Depth_Desc;
		}

		HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE PIPELINE STATE OBJECT"));

		return PSO;
	}

	void DrawSprite_Text(
		const char*				Str,
		FrameGraph&				frameGraph,
		SpriteFontAsset&		Font,
		VertexBufferHandle		Buffer, 
		ResourceHandle			renderTarget,
		iAllocator*				TempMemory,
		PrintTextFormatting&	Formatting)
	{
		struct DrawSpriteText
		{
			FrameResourceHandle		renderTarget;
			VertexBufferHandle		VertexBuffer;
			ConstantBufferHandle	ConstantBuffer;
			size_t					VertexCount;
			size_t					VertexOffset;

			uint2                   WH;
		};

		frameGraph.AddNode<DrawSpriteText>(
			DrawSpriteText{},
			[&](FrameGraphNodeBuilder& Builder, DrawSpriteText& Data)
			{
				if (Formatting.PixelSize == float2{ 1.0f, 1.0f })
					Formatting.PixelSize = float2{ 1.0f, 1.0f } / frameGraph.Resources.renderSystem.GetTextureWH(renderTarget);

				Data.renderTarget	= Builder.RenderTarget(renderTarget);

				size_t StrLen		= strlen(Str);
				size_t BufferSize	= StrLen * sizeof(TextEntry);
				Vector<TextEntry>	Text{ TempMemory };

				Data.VertexBuffer	= Buffer;
				Data.VertexCount	= StrLen;
				Data.VertexOffset	= GetCurrentVBufferOffset<TextEntry>(Buffer, frameGraph.Resources);
				Data.WH             = frameGraph.GetRenderSystem().GetTextureWH(renderTarget);

				const float XBegin	= Formatting.StartingPOS.x;
				const float YBegin	= Formatting.StartingPOS.y;

				float CurrentX	= Formatting.CurrentX;
				float CurrentY	= Formatting.CurrentY;
				float YAdvance	= Formatting.YAdvance;

				size_t itr_2		= 0;
				size_t OutputBegin	= itr_2;
				size_t LineBegin	= OutputBegin;

				Text.reserve(StrLen);
				
				auto CenterLine = [&]()
				{
					if (Formatting.CenterX)
					{
						const float Offset_X = (Formatting.TextArea.x - CurrentX) / 2.0f;
						const float Offset_Y = (Formatting.TextArea.y - CurrentY - YAdvance / 2) / 2.0f;

						if (abs(Offset_X) > 0.0001f)
						{
							for (size_t ii = LineBegin; ii < itr_2; ++ii)
							{
								Text[ii].POS.x += Offset_X;
								Text[ii].POS.y += Offset_Y;
							}
						}
					}
				};

				for (size_t StrIdx = 0; StrIdx < StrLen; ++StrIdx)
				{
					const auto C           = Str[StrIdx];
					const float2 GlyphArea = Font.GlyphTable[C].WH * Formatting.PixelSize;
					const float  XAdvance  = Font.GlyphTable[C].Xadvance * Formatting.PixelSize.x;
					const auto G           = Font.GlyphTable[C];

					const float2 Scale	= float2(1.0f, 1.0f) / Font.TextSheetDimensions;
					const float2 WH		= G.WH * Scale;
					const float2 XY		= G.XY * Scale;
					const float2 UVTL	= XY;
					const float2 UVBR	= XY + WH;

					if (C == '\n' || CurrentX + XAdvance > Formatting.TextArea.x)
					{
						CenterLine();

						LineBegin = itr_2;
						CurrentX = 0;
						CurrentY += YAdvance / 2 * Formatting.Scale.y;
					}

					if (CurrentY > Formatting.StartingPOS.y + Formatting.TextArea.y)
						continue;

					TextEntry Character		= {};
					Character.POS           = float2(CurrentX + XBegin, CurrentY + YBegin);
					Character.TopLeftUV     = UVTL;
					Character.BottomRightUV = UVBR;
					Character.Color         = Formatting.Color;
					Character.Size          = WH * Formatting.Scale;
						
					Text.push_back(Character);
					YAdvance  = Max(YAdvance, GlyphArea.y);
					CurrentX += XAdvance * Formatting.Scale.x;
					itr_2++;
				}

				CenterLine();


				Formatting.CurrentX = CurrentX;
				Formatting.CurrentY = CurrentY;
				Formatting.CurrentY = YAdvance;

				for (auto Character : Text) 
				{
					auto Character2 = Character;
					Character2.POS = Position2SS(Character2.POS);
					FlexKit::PushVertex(Character2, Buffer, frameGraph.Resources);
				}
			},
			[spriteSheet = Font.Texture](DrawSpriteText& data, const ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
				DescriptorHeap descHeap;
				descHeap.Init(
					ctx,
					resources.renderSystem().Library.RS6CBVs4SRVs.GetDescHeap(0),
					&allocator);

				descHeap.SetSRV(ctx, 0, spriteSheet);
				descHeap.NullFill(ctx);

				ctx.SetRootSignature				(resources.renderSystem().Library.RS6CBVs4SRVs);
				ctx.SetPipelineState				(resources.GetPipelineState(DRAW_SPRITE_TEXT_PSO));

				ctx.SetScissorAndViewports			({ resources.GetResource(data.renderTarget) });
				ctx.SetRenderTargets				({ resources.GetResource(data.renderTarget) }, false);

				ctx.SetGraphicsDescriptorTable		(0, descHeap);
				ctx.SetPrimitiveTopology			(EInputTopology::EIT_POINT);
				ctx.SetVertexBuffers				(VertexBufferList{ { data.VertexBuffer, sizeof(TextEntry) } });

				ctx.NullGraphicsConstantBufferView	(1);
				ctx.NullGraphicsConstantBufferView	(2);
				ctx.NullGraphicsConstantBufferView	(3);
				ctx.NullGraphicsConstantBufferView	(4);
				ctx.NullGraphicsConstantBufferView	(5);
				ctx.NullGraphicsConstantBufferView	(6);

				ctx.Draw							(data.VertexCount, data.VertexOffset);
			});
	}
}	// Namespace FlexKit
