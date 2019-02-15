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

#ifndef TERRAINRENDERING
#define TERRAINRENDERING

#include "..\buildsettings.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\MathUtils.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\graphicsutilities\graphics.h"
#include "..\graphicsutilities\DDSUtilities.h"

#include <d3dx12.h>
#include <DirectXMath.h>


/************************************************************************************************/


namespace FlexKit
{	/************************************************************************************************/
	using TerrainTileHandle = FlexKit::Handle_t<16, GetTypeGUID(TerrainTileHandle)>;
	const FlexKit::PSOHandle TERRAIN_COMPUTE_CULL_PSO				= PSOHandle(GetTypeGUID(TERRAIN_COMPUTE_CULL_PSO));
	const FlexKit::PSOHandle TERRAIN_RENDER_FOWARD_PSO				= PSOHandle(GetTypeGUID(TERRAIN_RENDER_FOWARD_PSO));
	const FlexKit::PSOHandle TERRAIN_RENDER_FOWARD_WIREFRAME_PSO	= PSOHandle(GetTypeGUID(TERRAIN_RENDER_FOWARD_WIREFRAME_PSO));

	struct ViewableRegion
	{
		int32_t xyz[4];
		int32_t unused[3];
		int32_t BitID;
		float2  UV_TL;			// Top	 Left
		float2  UV_BR;			// Bottom Right
		int32_t TerrainInfo[4]; // Texture Index
	};


	struct TerrainConstantBufferLayout
	{
		float4		Padding;
		float4		Albedo;   // + roughness
		float4		Specular; // + metal factor
		float2		RegionDimensions;
		Frustum		Frustum;
		uint32_t	PassCount;
		uint32_t	debugMode;
	};


	struct Region_CP
	{
		int4	regionID;
		int4	parentID;
		float4	UVs;
		int4	d; // I forget what this is for
	};


	struct TerrainCullerData
	{
		uint16_t				regionCount;
		uint16_t				splitCount;
		size_t					outputBuffer;

		IndirectLayout			indirectLayout;

		FrameResourceHandle		finalBuffer;
		FrameResourceHandle		intermediateBuffer1;
		FrameResourceHandle		intermediateBuffer2;
		FrameResourceHandle		querySpace;

		DescriptorHeap			heap;
		FrameResourceHandle		indirectArgs; // UAV Buffer

		QueryHandle				queryFinal;
		QueryHandle				queryIntermediate;

		VertexBufferHandle		vertexBuffer;
		ConstantBufferHandle	constantBuffer;
		size_t					cameraConstants;
		size_t					terrainConstants;
		size_t					inputVertices;
		size_t					indirectArgsInitial;

		Vector<size_t>			tileHeightMaps;
	};


	ID3D12PipelineState* CreateCullTerrainComputePSO(RenderSystem* renderSystem)
	{
		auto cullTerrain_shader_VS = LoadShader("CP_PassThroughVS", "CP_PassThroughVS", "vs_5_0", "assets\\cullterrain.hlsl");
		auto cullTerrain_shader_GS = LoadShader("CullTerrain",		"CullTerrain",		"gs_5_0", "assets\\cullterrain.hlsl");


		D3D12_INPUT_ELEMENT_DESC InputElements[] =
		{
			{ "REGION",	  0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	0,	32, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 2, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	48, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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

		UINT Strides = sizeof(ViewableRegion);
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

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature                = renderSystem->Library.RS4CBVs_SO;
			PSO_Desc.VS                            = cullTerrain_shader_VS;
			PSO_Desc.GS                            = cullTerrain_shader_GS;
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
		auto HR = renderSystem->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));

		return PSO;
	}


	ID3D12PipelineState* CreateForwardRenderTerrainPSO(RenderSystem* renderSystem)
	{
		auto forwardRenderTerrain_shader_VS = LoadShader("CP_PassThroughVS",	"CP_PassThroughVS",		"vs_5_0", "assets\\forwardRenderTerrain.hlsl");
		auto ShaderQuad2Tri					= LoadShader("QuadPatchToTris",		"QuadPatchToTris",		"ds_5_0", "assets\\forwardRenderTerrain.hlsl");
		auto ShaderRegion2Quad				= LoadShader("RegionToQuadPatch",	"RegionToQuadPatch",	"hs_5_0", "assets\\forwardRenderTerrain.hlsl");
		auto forwardRenderTerrain_shader_GS = LoadShader("GS_RenderTerrain",	"GS_RenderTerrain",		"gs_5_0", "assets\\forwardRenderTerrain.hlsl");
		auto forwardRenderTerrain_shader_PS = LoadShader("PS_RenderTerrain",	"PS_RenderTerrain",		"ps_5_0", "assets\\forwardRenderTerrain.hlsl");

		EXITSCOPE(
			Release(forwardRenderTerrain_shader_VS);
			Release(ShaderQuad2Tri);
			Release(ShaderRegion2Quad);
			Release(forwardRenderTerrain_shader_GS);
			Release(forwardRenderTerrain_shader_PS);
			);

		D3D12_INPUT_ELEMENT_DESC InputElements[] =
		{
			{ "REGION",	  0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	0,	32, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 2, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	48, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); {
		}

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); {
			Depth_Desc.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature                = renderSystem->Library.RS4CBVs_SO;
			PSO_Desc.VS                            = forwardRenderTerrain_shader_VS;
			PSO_Desc.DS                            = ShaderQuad2Tri;
			PSO_Desc.HS                            = ShaderRegion2Quad;
			//PSO_Desc.GS                            = forwardRenderTerrain_shader_GS;
			PSO_Desc.PS							   = forwardRenderTerrain_shader_PS;
			PSO_Desc.RasterizerState			   = Rast_Desc;
			PSO_Desc.SampleMask                    = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType         = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
			PSO_Desc.NumRenderTargets              = 1;
			PSO_Desc.RTVFormats[0]				   = DXGI_FORMAT_R16G16B16A16_FLOAT;
			PSO_Desc.SampleDesc.Count              = 1;
			PSO_Desc.SampleDesc.Quality            = 0;
			PSO_Desc.RasterizerState               = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			PSO_Desc.BlendState                    = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.DSVFormat                     = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout                   = { InputElements, sizeof(InputElements) / sizeof(InputElements[0]) };
			PSO_Desc.DepthStencilState			   = Depth_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = renderSystem->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));

		return PSO;
	}


	ID3D12PipelineState* CreateForwardRenderTerrainWireFramePSO(RenderSystem* renderSystem)
	{
		auto forwardRenderTerrain_shader_VS = LoadShader("CP_PassThroughVS",	"CP_PassThroughVS",			"vs_5_0", "assets\\forwardRenderTerrain.hlsl");
		auto forwardRenderTerrain_shader_GS = LoadShader("GS_RenderTerrain",	"GS_RenderTerrain",			"gs_5_0", "assets\\forwardRenderTerrain.hlsl");


		auto ShaderRegion2Quad				= LoadShader("RegionToQuadPatchDEBUG",	"RegionToQuadPatchDEBUG",	"hs_5_0", "assets\\forwardRenderTerrainDEBUG.hlsl");
		auto ShaderQuad2Tri_Debug			= LoadShader("QuadPatchToTrisDEBUG",	"QuadPatchToTrisDEBUG",		"ds_5_0", "assets\\forwardRenderTerrainDEBUG.hlsl");
		auto ShaderPaint_Wire				= LoadShader("PS_RenderTerrainDEBUG",	"PS_RenderTerrainDEBUG",	"ps_5_0", "assets\\forwardRenderTerrainDEBUG.hlsl");


		EXITSCOPE(
			Release(forwardRenderTerrain_shader_VS);
			Release(ShaderQuad2Tri_Debug);
			Release(ShaderRegion2Quad);
			Release(forwardRenderTerrain_shader_GS);
			Release(ShaderPaint_Wire);
			);

		D3D12_INPUT_ELEMENT_DESC InputElements[] =
		{
			{ "REGION",	  0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	0,	32, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 2, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	48, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); {
			Rast_Desc.FillMode	= D3D12_FILL_MODE_WIREFRAME;
			Rast_Desc.DepthBias = -10;
		}

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); {
			Depth_Desc.DepthEnable	= true;
			Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS_EQUAL;
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature                = renderSystem->Library.RS4CBVs_SO;
			PSO_Desc.VS                            = forwardRenderTerrain_shader_VS;
			PSO_Desc.DS                            = ShaderQuad2Tri_Debug;
			PSO_Desc.HS                            = ShaderRegion2Quad;
			//PSO_Desc.GS                            = forwardRenderTerrain_shader_GS;
			PSO_Desc.PS							   = ShaderPaint_Wire;
			PSO_Desc.RasterizerState			   = Rast_Desc;
			PSO_Desc.SampleMask                    = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType         = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
			PSO_Desc.NumRenderTargets              = 1;
			PSO_Desc.RTVFormats[0]				   = DXGI_FORMAT_R16G16B16A16_FLOAT;
			PSO_Desc.SampleDesc.Count              = 1;
			PSO_Desc.SampleDesc.Quality            = 0;
			PSO_Desc.RasterizerState               = Rast_Desc;
			PSO_Desc.BlendState                    = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.DSVFormat                     = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout                   = { InputElements, sizeof(InputElements) / sizeof(InputElements[0]) };
			PSO_Desc.DepthStencilState			   = Depth_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = renderSystem->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));

		return PSO;
	}


	struct TileMaps
	{
		FlexKit::TextureHandle heightMap;
		const char*	File;
	};


	struct Tile
	{
		int2	tileID;
		size_t	textureMaps;
	};


	class TerrainEngine
	{
	public:
		TerrainEngine(RenderSystem* RS, iAllocator* IN_allocator, size_t IN_tileEdgeSize = 128, size_t IN_tileHeight = 128, uint2 IN_tileOffset = {0, 0}) :
			renderSystem				{ RS																	},
			tiles						{ IN_allocator															},
			tileTextures				{ IN_allocator															},
			tileHeight					{ IN_tileHeight															}, 
			tileOffset					{ IN_tileOffset															},
			allocator					{ IN_allocator															},
			finalBuffer					{ RS->CreateStreamOutResource(MEGABYTE * 64)							},
			intermdediateBuffer1		{ RS->CreateStreamOutResource(MEGABYTE * 64)							},
			intermdediateBuffer2		{ RS->CreateStreamOutResource(MEGABYTE * 64)							},
			queryBufferFinalBuffer		{ RS->CreateSOQuery(1,1)												},
			queryBufferIntermediate		{ RS->CreateSOQuery(0,1)												},
			indirectArgs				{ RS->CreateUAVBufferResource(512)										},
			querySpace					{ RS->CreateUAVBufferResource(512)										},
			indirectLayout				{ RS->CreateIndirectLayout({ILE_DrawCall}, IN_allocator)				}
		{
			renderSystem->RegisterPSOLoader(
				TERRAIN_COMPUTE_CULL_PSO, 
				{	&renderSystem->Library.RS4CBVs_SO,
					CreateCullTerrainComputePSO });

			renderSystem->RegisterPSOLoader(
				TERRAIN_RENDER_FOWARD_PSO,
				{	&renderSystem->Library.RS4CBVs_SO,
					CreateForwardRenderTerrainPSO });

			renderSystem->RegisterPSOLoader(
				TERRAIN_RENDER_FOWARD_WIREFRAME_PSO,
				{ &renderSystem->Library.RS4CBVs_SO,
					CreateForwardRenderTerrainWireFramePSO });

			renderSystem->QueuePSOLoad(TERRAIN_COMPUTE_CULL_PSO);
			renderSystem->QueuePSOLoad(TERRAIN_RENDER_FOWARD_PSO);
			renderSystem->QueuePSOLoad(TERRAIN_RENDER_FOWARD_WIREFRAME_PSO);
		}


		void SetTileSize(size_t tileEdgeSize)
		{

		}


		TerrainTileHandle AddTile(int2 tileID, const char* heightMap, RenderSystem* RS, iAllocator* tempMemory)
		{
			if(false)
			{
				static_vector<TextureBuffer> Textures;
				Textures.push_back(TextureBuffer{});
				LoadBMP(heightMap, tempMemory, &Textures.back());


				//Build Mip Maps
				auto BuildMipMap = [](TextureBuffer& sourceMap, iAllocator* memory) -> TextureBuffer
				{
					using RBGA = Vect<4, uint8_t>;

					TextureBuffer		MIPMap	= TextureBuffer( sourceMap.WH / 2, sizeof(RGBA), memory );
					TextureBufferView	View	= TextureBufferView<RBGA>(&sourceMap);
					TextureBufferView	MipView = TextureBufferView<RBGA>(&MIPMap);

					const auto WH = MIPMap.WH;
					for (size_t Y = 0; Y < WH[0]; Y++)
					{
						for (size_t X = 0; X < WH[1]; X++)
						{
							uint2 in_Cord	= uint2{ min(X, WH[0] - 1) * 2, min(Y, WH[1] - 1) * 2 };
							uint2 out_Cord	= uint2{ min(X, WH[0] - 1), min(Y, WH[1] - 1) };

							auto Sample = 
								View[in_Cord + uint2{0, 0}]/ 4 +
								View[in_Cord + uint2{0, 1}]/ 4 +
								View[in_Cord + uint2{1, 0}]/ 4 +
								View[in_Cord + uint2{1, 1}]/ 4;

							MipView[out_Cord] = Sample;
						}
					}

					return MIPMap;
				};

				Textures.push_back(BuildMipMap(Textures.back(), tempMemory));
				Textures.push_back(BuildMipMap(Textures.back(), tempMemory));
				Textures.push_back(BuildMipMap(Textures.back(), tempMemory));
				Textures.push_back(BuildMipMap(Textures.back(), tempMemory));
				auto textureHandle	= MoveTextureBuffersToVRAM(RS, Textures.begin(), Textures.size(), allocator);
				//auto textureHandle	= MoveTextureBufferToVRAM(RS, &Textures[0], allocator);
			}


			auto [textureHandle, res] = LoadDDSTexture2DFromFile_2(heightMap, tempMemory, RS);
			auto tileMaps = tileTextures.push_back(TileMaps{ textureHandle, heightMap });
			tiles.push_back(Tile{ tileID, tileMaps });

			return InvalidHandle_t;
		}

		void SetTileOffset(int2 IN_tileOffset)
		{
			tileOffset = IN_tileOffset;
		}

		// No update dependencies
		FlexKit::UpdateTask* Update(FlexKit::UpdateDispatcher& Dispatcher)
		{
			// TODO: Streaming textures
			// Make sure textures are loaded
			return nullptr;
		}


		SOResourceHandle	finalBuffer;
		SOResourceHandle	intermdediateBuffer1;
		SOResourceHandle	intermdediateBuffer2;

		UAVResourceHandle	querySpace;
		UAVResourceHandle	indirectArgs;

		IndirectLayout		indirectLayout;

		QueryHandle			queryBufferFinalBuffer;
		QueryHandle			queryBufferIntermediate;

		RenderSystem*		renderSystem;

		Vector<Tile>		tiles;
		Vector<TileMaps>	tileTextures;

		enum TERRAINDEBUGRENDERMODE
		{
			DISABLED,
			WIREFRAME,
		}				DebugMode = DISABLED;

		int2			tileOffset;
		size_t			tileEdgeSize;
		size_t			tileHeight;
		iAllocator*		allocator;
	};


	/************************************************************************************************/


	TerrainCullerData* const CullTerrain(
		TerrainEngine&			terrainEngine,
		RenderSystem*			renderSystem,
		FrameGraph&				frameGraph,
		CameraHandle			camera,
		VertexBufferHandle		vertexBuffer,
		ConstantBufferHandle	constantBuffer,
		iAllocator*				tempMemory)
	{
		//renderSystem->ResetQuery(terrainEngine.queryBufferFinalBuffer);
		//renderSystem->ResetQuery(terrainEngine.queryBufferIntermediate);

		frameGraph.Resources.AddSOResource(terrainEngine.finalBuffer,			0);
		frameGraph.Resources.AddSOResource(terrainEngine.intermdediateBuffer1,	0);
		frameGraph.Resources.AddSOResource(terrainEngine.intermdediateBuffer2,	0);

		frameGraph.Resources.AddUAVResource(terrainEngine.indirectArgs, 0, renderSystem->GetObjectState(terrainEngine.indirectArgs));
		frameGraph.Resources.AddUAVResource(terrainEngine.querySpace,	0, renderSystem->GetObjectState(terrainEngine.querySpace));

		frameGraph.Resources.AddQuery(terrainEngine.queryBufferFinalBuffer);
		frameGraph.Resources.AddQuery(terrainEngine.queryBufferIntermediate);

		auto& cullerStage = frameGraph.AddNode<TerrainCullerData>(
			GetCRCGUID(TERRAINCOMPUTEVISABLETILES),
			[&](FrameGraphNodeBuilder& builder, TerrainCullerData& data)
			{	
				data.finalBuffer			= builder.WriteSOBuffer	(terrainEngine.finalBuffer);
				data.intermediateBuffer1	= builder.WriteSOBuffer	(terrainEngine.intermdediateBuffer1);
				data.intermediateBuffer2	= builder.WriteSOBuffer	(terrainEngine.intermdediateBuffer2);

				data.indirectLayout			= terrainEngine.indirectLayout;
				data.indirectArgs			= builder.ReadWriteUAVBuffer(terrainEngine.indirectArgs);

				data.queryFinal				= terrainEngine.queryBufferFinalBuffer;
				data.queryIntermediate		= terrainEngine.queryBufferIntermediate;
				data.querySpace				= builder.ReadWriteUAVBuffer(terrainEngine.querySpace);

				auto tableLayout			= renderSystem->Library.RS4CBVs_SO.GetDescHeap(0);
				data.heap.Init(renderSystem, tableLayout, tempMemory);
				data.heap.NullFill(renderSystem);

				for(auto& tileMap : terrainEngine.tileTextures)
					data.heap.SetSRV(renderSystem, 0, tileMap.heightMap);


				data.splitCount				= 12;

				TerrainConstantBufferLayout constants;
				uint32_t					indirectArgsInitial[] = { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

				auto cameraNode		= GetCameraNode		(camera);
				float3 POS			= GetPositionW		(cameraNode);
				Quaternion Q		= GetOrientation	(cameraNode);
				constants.Albedo	= {0.0f, 1.0f, 1.0f, 0.9f};
				constants.Specular	= {1, 1, 1, 1};


				constants.RegionDimensions	= { 4096, 4096 };
				constants.Frustum			= GetFrustum(camera);
				constants.PassCount			= data.splitCount;
				constants.debugMode			= terrainEngine.DebugMode;


				data.constantBuffer			= constantBuffer;
				data.terrainConstants		= LoadConstants			(constants,				constantBuffer, frameGraph.Resources);
				data.cameraConstants		= LoadCameraConstants	(camera,				constantBuffer, frameGraph.Resources);
				data.indirectArgsInitial	= LoadConstants			(indirectArgsInitial,	constantBuffer, frameGraph.Resources);

				data.vertexBuffer		= vertexBuffer;
				data.inputVertices		= frameGraph.Resources.GetVertexBufferOffset(vertexBuffer);

				data.regionCount		= terrainEngine.tiles.size();
			
				data.tileHeightMaps = Vector<size_t>{tempMemory};

				for (auto& tile: terrainEngine.tiles) 
				{
					Region_CP tile_CP;
					tile_CP.parentID = { 0, 0, 0, 0 };
					tile_CP.UVs		 = { 0, 0, 1, 1 };
					tile_CP.d		 = { 1, 0, 0, static_cast<int32_t>(tile.textureMaps) };
					tile_CP.regionID = {tile.tileID[0], 0, tile.tileID[1], 1024 * 16};
					PushVertex(tile_CP, vertexBuffer, frameGraph.Resources);

					data.tileHeightMaps.push_back(tile.textureMaps);
				}
			},
			[=](const TerrainCullerData& data, const FrameResources& resources, Context* ctx)
			{
				auto PSO = resources.GetPipelineState(TERRAIN_COMPUTE_CULL_PSO);

				if (!PSO)
					return;

				auto& rootSig = resources.renderSystem->Library.RS4CBVs_SO;
			
				ctx->SetRootSignature(rootSig);
				ctx->SetPipelineState(PSO);

				FrameResourceHandle	intermediateSOs[]	= { data.intermediateBuffer1, data.intermediateBuffer2 };
				int					offset				= 0;

				ctx->SetPrimitiveTopology			(EInputTopology::EIT_POINT);
				ctx->SetGraphicsConstantBufferView	(0, data.constantBuffer, data.cameraConstants);
				ctx->SetGraphicsConstantBufferView	(1, data.constantBuffer, data.terrainConstants);
				ctx->SetGraphicsConstantBufferView	(2, data.constantBuffer, data.terrainConstants);
				ctx->SetGraphicsDescriptorTable		(3, data.heap);


				{	// Clear counters
					auto constants		= resources.GetObjectResource(data.constantBuffer);
					auto indirectArgs	= resources.GetUAVDeviceResource(data.indirectArgs);
					auto querySpace		= resources.GetUAVDeviceResource(data.querySpace);
					auto UAV			= resources.WriteUAV(data.indirectArgs, ctx);

					auto iaState = resources.GetObjectState(data.indirectArgs);
					auto qsState = resources.GetObjectState(data.querySpace);


					ctx->CopyBufferRegion(
						{ constants, constants										}, // source 
						{ data.indirectArgsInitial, data.indirectArgsInitial + 8,	}, // source offset
						{ indirectArgs, querySpace									}, // destinations
						{ 0, 16														}, // dest offset
						{ sizeof(uint4), 16											}, // copy sizes
						{ iaState, qsState											},
						{ iaState, qsState											});
				}
				// prime pipeline
				ctx->SetVertexBuffers({
					{ data.vertexBuffer, sizeof(ViewableRegion), (UINT)data.inputVertices}});

				ctx->SetSOTargets({
						resources.WriteStreamOut(intermediateSOs[0],	ctx, sizeof(Region_CP)),
						resources.WriteStreamOut(data.finalBuffer,		ctx, sizeof(Region_CP)) });

				ctx->BeginQuery(data.queryFinal, 0);
				ctx->BeginQuery(data.queryIntermediate, 0);
				ctx->Draw(data.regionCount);

				for (size_t I = 1; I < data.splitCount; ++I) 
				{
					size_t SO1 =   offset % 2;
					size_t SO2 = ++offset % 2;


					ctx->EndQuery(data.queryIntermediate, 0);

					ctx->ResolveQuery(
						data.queryIntermediate, 0, 1,
						resources.WriteUAV(data.querySpace, ctx), 0);

					auto resource		= resources.GetObjectResource	(resources.WriteUAV(data.indirectArgs, ctx));
					auto querySpace		= resources.GetObjectResource	(resources.ReadUAVBuffer(data.querySpace, DRS_Read, ctx));
					auto constants		= resources.GetObjectResource	(data.constantBuffer);
					auto iaState		= resources.GetObjectState		(data.indirectArgs);

					ctx->CopyBufferRegion(
						{ querySpace				},	// sources
						{ 0							},  // source offsets
						{ resource					},  // destinations
						{ 0							},  // destination offsets
						{ 4							},  // copy sizes
						{ iaState					},  // source initial state
						{ iaState					}); // source final	state


					ctx->BeginQuery(data.queryIntermediate, 0);

					ctx->ClearSOCounters({ resources.ClearStreamOut(intermediateSOs[SO2], ctx) });

					ctx->SetVertexBuffers2({
						resources.ReadStreamOut(
							intermediateSOs[SO1], ctx, sizeof(Region_CP)) });

					ctx->SetSOTargets({
						resources.WriteStreamOut(intermediateSOs[SO2],	ctx, sizeof(Region_CP)),
						resources.WriteStreamOut(data.finalBuffer,		ctx, sizeof(Region_CP)) });

					ctx->ExecuteIndirect(
						resources.ReadIndirectArgs(data.indirectArgs, ctx),
						data.indirectLayout);
				}

				ctx->EndQuery(data.queryFinal, 0);
				ctx->EndQuery(data.queryIntermediate, 0);

				ctx->ResolveQuery(
					data.queryIntermediate, 0, 1,
					resources.WriteUAV(data.querySpace, ctx), 0);

				ctx->ResolveQuery(
					data.queryFinal, 0, 1,
					resources.WriteUAV(data.querySpace, ctx), 16);

				ctx->SetSOTargets({});
			});

			return &cullerStage;
	}

	auto DrawTerrain_Forward(
		TerrainCullerData* const	culledTerrain,
		TerrainEngine&				terrainEngine,
		RenderSystem*				renderSystem,
		FrameGraph&					frameGraph,
		CameraHandle				camera,
		VertexBufferHandle			vertexBuffer,
		ConstantBufferHandle		constantBuffer,
		TextureHandle				renderTarget,
		TextureHandle				depthTarget,
		iAllocator*					tempMemory)
	{
		struct RenderTerrainForward
		{
			uint16_t				regionCount;
			uint16_t				splitCount;
			size_t					outputBuffer;

			IndirectLayout			indirectLayout;

			FrameResourceHandle		renderTarget;
			FrameResourceHandle		depthTarget;

			FrameResourceHandle		finalBuffer;
			FrameResourceHandle		intermediateBuffer1;
			FrameResourceHandle		intermediateBuffer2;
			FrameResourceHandle		querySpace;

			DescriptorHeap*			heap;
			FrameResourceHandle		indirectArgs; // UAV Buffer

			QueryHandle				queryFinal;
			QueryHandle				queryIntermediate;
	
			VertexBufferHandle		vertexBuffer;
			ConstantBufferHandle	constantBuffer;
			size_t					cameraConstants;
			size_t					terrainConstants;
			size_t					inputVertices;
			size_t					indirectArgsInitial;

			TerrainEngine::TERRAINDEBUGRENDERMODE debugMode;
		};

		auto& renderStage = frameGraph.AddNode<RenderTerrainForward>(
			GetCRCGUID(TERRAINCOMPUTEVISABLETILES),
			[&](FrameGraphNodeBuilder& builder, RenderTerrainForward& data)
			{
				data.debugMode				= terrainEngine.DebugMode;
				data.renderTarget			= builder.WriteRenderTarget(renderTarget);
				data.depthTarget			= builder.WriteDepthBuffer(depthTarget);

				data.indirectArgs			= builder.ReadWriteUAVBuffer(terrainEngine.indirectArgs);
				data.querySpace				= builder.ReadWriteUAVBuffer(terrainEngine.querySpace);
				data.finalBuffer			= builder.ReadSOBuffer(terrainEngine.finalBuffer);
				data.intermediateBuffer1	= builder.ReadSOBuffer(terrainEngine.intermdediateBuffer1);
				data.intermediateBuffer2	= builder.ReadSOBuffer(terrainEngine.intermdediateBuffer2);

				data.constantBuffer			= culledTerrain->constantBuffer;
				data.cameraConstants		= culledTerrain->cameraConstants;
				data.terrainConstants		= culledTerrain->terrainConstants;
			
				data.indirectArgsInitial	= culledTerrain->indirectArgsInitial;
				data.indirectLayout			= culledTerrain->indirectLayout;

				data.heap					= &culledTerrain->heap;

			},
			[=](const RenderTerrainForward& data, const FrameResources& resources, Context* ctx)
			{
				auto PSO = resources.GetPipelineState(TERRAIN_RENDER_FOWARD_PSO);

				if (!PSO)
					return;

				auto& rootSig = resources.renderSystem->Library.RS4CBVs_SO;

				ctx->SetRootSignature(rootSig);
				ctx->SetPipelineState(PSO);

				ctx->SetPrimitiveTopology(EInputTopology::EIT_PATCH_CP_1);
				ctx->SetGraphicsConstantBufferView(0, data.constantBuffer, data.cameraConstants);
				ctx->SetGraphicsConstantBufferView(1, data.constantBuffer, data.terrainConstants);
				ctx->SetGraphicsConstantBufferView(2, data.constantBuffer, data.terrainConstants);
				ctx->SetGraphicsDescriptorTable(3, *data.heap);

				ctx->SetScissorAndViewports({ resources.GetRenderTarget(data.renderTarget) });
				ctx->SetRenderTargets(
					{ resources.GetRenderTargetObject(data.renderTarget) },
					true,
					resources.GetRenderTargetObject(data.depthTarget));

				const size_t inputArray					= (data.splitCount + 1) % 2;
				FrameResourceHandle	intermediateSOs[]	= { data.intermediateBuffer1, data.intermediateBuffer2 };

				//ctx->SetVertexBuffers2({
				//		resources.ReadStreamOut(
				//			intermediateSOs[inputArray], ctx, sizeof(Region_CP)) });
				ctx->SetVertexBuffers2({
						resources.ReadStreamOut(
							data.finalBuffer, ctx, sizeof(Region_CP)) });


				auto destResource	= resources.GetObjectResource(resources.WriteUAV(data.indirectArgs, ctx));
				auto querySpace		= resources.GetObjectResource(resources.ReadUAVBuffer(data.querySpace, DRS_Read, ctx));
				auto constants		= resources.GetObjectResource(data.constantBuffer);
				auto iaState		= resources.GetObjectState(data.indirectArgs);

				ctx->CopyBufferRegion(
					{ querySpace, querySpace		},  // sources
					{ 16,			0				},	// source offset
					{ destResource, destResource	},  // destinations
					{ 0,			32				},  // destination offsets
					{ 4, 			4				},  // copy sizes
					{ iaState,		iaState			},  // source initial state
					{ iaState,		iaState			}); // destination final state

				ctx->ExecuteIndirect(
					resources.ReadIndirectArgs(data.indirectArgs, ctx),
					data.indirectLayout);


				switch (data.debugMode)
				{
				case TerrainEngine::TERRAINDEBUGRENDERMODE::WIREFRAME:
				{
					auto PSO = resources.GetPipelineState(TERRAIN_RENDER_FOWARD_WIREFRAME_PSO);

					ctx->SetPipelineState(PSO);
					ctx->ExecuteIndirect(
						resources.ReadIndirectArgs(data.indirectArgs, ctx),
						data.indirectLayout);
				}	break;
				}

				ctx->ClearSOCounters(
					{	resources.ClearStreamOut(intermediateSOs[0],	ctx),
						resources.ClearStreamOut(intermediateSOs[1],	ctx),
						resources.ClearStreamOut(data.finalBuffer,		ctx) });
			});

		return &renderStage;
	}

	/************************************************************************************************/

}

#endif