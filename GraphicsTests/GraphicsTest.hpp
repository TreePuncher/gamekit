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


#ifndef GRAPHICSTEST_INCLUDED
#define GRAPHICSTEST_INCLUDED

#include "..\coreutilities\GameFramework.h"

const size_t lookupTableSize	= 128;

enum TestEvents
{
	ReloadShaders_1,
	Forward,
	Backward,
	Left,
	Right
};

using namespace FlexKit;


void NelderMead()
{

}

struct BRDF_ggx
{
	static float3 F_Schlick(float3 f0, float3 f90, float u)
	{
		return f0 + (f90 - f0) * pow(1.0f - u, 5.0f);
	}

	static float Fr_Disney(float ndotv, float ndotl, float ldoth, float linearRoughness) // Fresnel Factor
	{
		float energyBias	= lerp(0.0f, 0.5f,			linearRoughness);
		float energyFactor	= lerp(1.0f, 1.0f / 1.51f,	linearRoughness);
		float fd90 			= energyBias + 2.0 * ldoth * ldoth * linearRoughness;
		float3 f0 			= float3(1.0f, 1.0f, 1.0f);

		float lightScatter	= F_Schlick(f0, fd90, ndotl).x;
		float viewScatter	= F_Schlick(f0, fd90, ndotv).x;

		return lightScatter * viewScatter * energyFactor;
	}

	static float V_SmithGGXCorrelated(float ndotl, float ndotv, float alphaG)
	{
		float alphaG2       = alphaG * alphaG;
		float Lambda_GGXV   = ndotl * sqrt((-ndotv * alphaG2 + ndotv) * ndotv + alphaG2);
		float Lambda_GGXL   = ndotv * sqrt((-ndotl * alphaG2 + ndotl) * ndotl + alphaG2);
		return 0.5f / (Lambda_GGXV + Lambda_GGXL);
	}

	static float D_GGX(float ndoth , float m)
	{
		//Divide by PI is apply later
		float m2    = m * m;
		float f     = (ndoth * m2 - ndoth) * ndoth + 1;
		return m2 / (f * f);
	} 

	static float DistanceSquared2(float2 a, float2 b)
	{
 		a -= b; 
 		return DotProduct2(a, a);
	}

	static float DistanceSquared3(float2 a, float2 b)
	{
 		a -= b;
 		return DotProduct2(a, a);
	}

	static float3 Frd(float3 l, float3 v, float3 n, float3 Ks, float m, float r)
	{
		float3 h  = (v + l).normal();
		float  A  = saturate(pow(r, 2));

		float ndotv = saturate(dot(n, v) + 1e-5);
		float ndotl = saturate(dot(n, l));
		float ndoth = saturate(dot(n, h));
		float ldoth = saturate(dot(l, h));
	
		//	Specular BRDF
		float3 F    = F_Schlick(Ks, 0.0f, ldoth);
		float Vis   = V_SmithGGXCorrelated(ndotv, ndotl, A);
		float D     = D_GGX(ndoth, A);
		float3 Fr   = D * F * Vis;

		// 	Diffuse BRDF
		float Fd = Fr_Disney(ndotv, ndotl, ldoth, A) / (4.0f * pi * pi);

		return float3(Fr + (Fd * (1 - m))) * ndotl;
	}

	static float	Eval	(const float3& V, const float3& L,	float3 N, const float alpha, float& pdf)
	{
		pdf = V.z / pi;
		return Frd(L, V, N, float3(1), 0, alpha).magnitude();
	}
	
	static float3	Sample	(const float3& V, const float alpha,	const float U1,		const	float U2)
	{
		const float r	= sqrtf(U1);
		const float phi = 2.0f*3.14159f * U2;
		const float3 L	= float3(r*cosf(phi), r*sinf(phi), sqrtf(1.0f - r * r));
		return L;
	};
};


/************************************************************************************************/


class BrdfDisneyDiffuse
{
public:

	static float Eval(const float3 V, const float3 L, const float alpha, float& pdf)
	{
		if(V.z <= 0 || L.z <= 0)
		{
			pdf = 0;
			return 0;
		}

		pdf = L.z / 3.14159f;

		float NdotV = V.z;
		float NdotL = L.z;
		float LdotH = dot(L, (V+L).normal());
		float perceptualRoughness = sqrtf(alpha);
		float fd90 = 0.5 + 2 * LdotH * LdotH * perceptualRoughness;
		float lightScatter    = (1 + (fd90 - 1) * powf(1 - NdotL, 5.0f));
		float viewScatter    = (1 + (fd90 - 1) * powf(1 - NdotV, 5.0f));
		return lightScatter * viewScatter * L.z / 3.14159f;
	}

	static float3 Sample(const float3 V, const float alpha, const float U1, const float U2)
	{
		const float r	= sqrtf(U1);
		const float phi = 2.0f*3.14159f * U2;
		const float3 L	= float3(r*cosf(phi), r*sinf(phi), sqrtf(1.0f - r*r));
		return L;
	}	
};


/************************************************************************************************/


float ComputeBRDFNorm(const float3& v, const float alpha, const int sampleCount = 64)
{
	float albedo = 0.0f;
	for (size_t i = 0; i < sampleCount; ++i)
	{
		for (size_t j = 0; j < sampleCount; ++j)
		{
			const float U1 = (i + 0.5f) / (float)sampleCount;
			const float U2 = (j + 0.5f) / (float)sampleCount;

			const float3 l	= BrdfDisneyDiffuse::Sample(v, alpha, U1, U2);
			float pdf		= 0;

			float eval1 = BRDF_ggx::Eval(v, l, {0, 0, 1}, alpha, pdf);
			float eval2	= BrdfDisneyDiffuse::Eval(v, l, alpha, pdf);

			albedo += (pdf > 0.0f ) ? eval2 / pdf : 0.0f;
		}
	}

	return albedo / (sampleCount * sampleCount);
}


/************************************************************************************************/


void DEBUG_DrawQuadTree(
	GraphicScene&			scene, 
	FlexKit::FrameGraph&	graph, 
	CameraHandle			camera,
	VertexBufferHandle		vertexBuffer, 
	ConstantBufferHandle	constantBuffer,
	TextureHandle			renderTarget,
	iAllocator*				tempMemory)
{
	Vector<FlexKit::Rectangle> rects{ tempMemory };

	const float4 colors[] =
	{
		{0, 0, 1, 1},
		{0, 1, 1, 1},
		{1, 1, 0, 1},
		{1, 0, 0, 1}
	};

	const auto area		= scene.SceneManagement.GetArea();
	const auto areaLL	= (area.y <  area.z) ? float2{ area.x, area.y } : float2{ area.z, area.w };
	const auto areaUR	= (area.y >= area.z) ? float2{ area.x, area.y } : float2{ area.z, area.w };
	const auto areaSpan = (areaUR - areaLL);
	int itr = 0;

	auto drawNode = [&](auto& self, const QuadTreeNode& Node, const int depth) -> void
	{
		auto nodeArea = Node.GetArea();

		//const auto ll = (float2{ nodeArea.x, nodeArea.y } - areaLL) / areaSpan;
		//const auto ur = (float2{ nodeArea.z, nodeArea.w } - areaLL) / areaSpan;

		const auto ll = float2{nodeArea.x, nodeArea.y};// lower left
		const auto ur = float2{nodeArea.z, nodeArea.w};// upper right

		FlexKit::Rectangle rect;
		rect.Color		= colors[depth % 4];
		rect.Position	= ll;
		rect.WH			= ur - ll;
		rects.push_back(rect);

		for (auto& child : Node.ChildNodes)
			self(self, *child, depth + 1 + itr++);
	};

	drawNode(drawNode, scene.SceneManagement.root, 0);

	DrawWireframeRectangle_Desc drawDesc =
	{
		renderTarget,
		vertexBuffer,
		constantBuffer,
		camera,
		DRAW_LINE3D_PSO
	};

	WireframeRectangleList(
		graph,
		drawDesc,
		rects,
		tempMemory);
}


/************************************************************************************************/


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


using TerrainTileHandle = FlexKit::Handle_t<16, GetTypeGUID(TerrainTileHandle)>;
const FlexKit::PSOHandle TERRAIN_COMPUTE_CULL_PSO	= PSOHandle(GetTypeGUID(TERRAIN_COMPUTE_CULL_PSO));
const FlexKit::PSOHandle TERRAIN_RENDER_FOWARD_PSO	= PSOHandle(GetTypeGUID(TERRAIN_RENDER_FOWARD_PSO));


ID3D12PipelineState* CreateCullTerrainComputePSO(RenderSystem* renderSystem)
{
	auto cullTerrain_shader_VS = LoadShader("CP_PassThroughVS", "CP_PassThroughVS", "vs_5_0", "assets\\cullterrain.hlsl");
	auto cullTerrain_shader_GS = LoadShader("CullTerrain",		"CullTerrain", "gs_5_0", "assets\\cullterrain.hlsl");


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
	auto forwardRenderTerrain_shader_VS = LoadShader("CP_PassThroughVS", "CP_PassThroughVS", "vs_5_0", "assets\\forwardRenderTerrain.hlsl");
	auto forwardRenderTerrain_shader_GS = LoadShader("GS_RenderTerrain", "GS_RenderTerrain", "gs_5_0", "assets\\forwardRenderTerrain.hlsl");
	auto forwardRenderTerrain_shader_PS = LoadShader("PS_RenderTerrain", "PS_RenderTerrain", "ps_5_0", "assets\\forwardRenderTerrain.hlsl");

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
		Depth_Desc.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
		PSO_Desc.pRootSignature                = renderSystem->Library.RS4CBVs_SO;
		PSO_Desc.VS                            = forwardRenderTerrain_shader_VS;
		PSO_Desc.GS                            = forwardRenderTerrain_shader_GS;
		PSO_Desc.PS							   = forwardRenderTerrain_shader_PS;
		PSO_Desc.RasterizerState			   = Rast_Desc;
		PSO_Desc.SampleMask                    = UINT_MAX;
		PSO_Desc.PrimitiveTopologyType         = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
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
		finalBuffer					{ RS->CreateStreamOutResource(MEGABYTE)									},
		intermdediateBuffer1		{ RS->CreateStreamOutResource(MEGABYTE)									},
		intermdediateBuffer2		{ RS->CreateStreamOutResource(MEGABYTE)									},
		queryBufferFinalBuffer		{ RS->CreateSOQuery(0,1)												},
		queryBufferIntermediate		{ RS->CreateSOQuery(1,1)												},
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

		renderSystem->QueuePSOLoad(TERRAIN_COMPUTE_CULL_PSO);
		renderSystem->QueuePSOLoad(TERRAIN_RENDER_FOWARD_PSO);
	}


	void SetTileSize(size_t tileEdgeSize)
	{

	}


	TerrainTileHandle AddTile(int2 tileID, const char* HeightMap, RenderSystem* RS, iAllocator* tempMemory)
	{
		static_vector<TextureBuffer> Textures;
		Textures.push_back(TextureBuffer{});
		LoadBMP(HeightMap, tempMemory, &Textures.back());

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
					uint2 Cord = uint2{ min(X, WH[0] - 1), min(Y, WH[1] - 1) };

					auto Sample = 
						View[Cord + uint2{0, 0}] +
						View[Cord + uint2{0, 1}] +
						View[Cord + uint2{1, 0}] +
						View[Cord + uint2{1, 1}];

					Sample = Sample / 4;

					MipView[Cord] = Sample;
				}
			}

			return MIPMap;
		};


		Textures.push_back(BuildMipMap(Textures.back(), tempMemory));
		Textures.push_back(BuildMipMap(Textures.back(), tempMemory));
		Textures.push_back(BuildMipMap(Textures.back(), tempMemory));
		Textures.push_back(BuildMipMap(Textures.back(), tempMemory));

		auto textureHandle	= MoveTextureBufferToVRAM(&Textures.front(), RS, allocator);
		auto tileMaps		= tileTextures.push_back(TileMaps{textureHandle, HeightMap});
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

	RenderSystem*	renderSystem;

	Vector<Tile>		tiles;
	Vector<TileMaps>	tileTextures;

	int2		tileOffset;
	size_t		tileEdgeSize;
	size_t		tileHeight;
	iAllocator* allocator;
};


/************************************************************************************************/


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
	float4	 Albedo;   // + roughness
	float4	 Specular; // + metal factor
	float2	 RegionDimensions;
	Frustum	 Frustum;
	int      PassCount;
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
};


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
			data.splitCount = 1;

			TerrainConstantBufferLayout constants;
			uint4_32					indirectArgsInitial{ 1, 0, 0, 0 };

			data.constantBuffer			= constantBuffer;
			data.cameraConstants		= LoadCameraConstants	(camera,				constantBuffer, frameGraph.Resources);
			data.terrainConstants		= LoadConstants			(constants,				constantBuffer, frameGraph.Resources);
			data.indirectArgsInitial	= LoadConstants			(indirectArgsInitial,	constantBuffer, frameGraph.Resources);

			data.vertexBuffer		= vertexBuffer;
			data.inputVertices		= frameGraph.Resources.GetVertexBufferOffset(vertexBuffer);

			data.regionCount = terrainEngine.tiles.size();
			
			for (auto& tile: terrainEngine.tiles) 
			{
				Region_CP tile_CP;
				tile_CP.parentID = { 0, 0, 0, 0 };
				tile_CP.UVs		 = { 0, 1, 1, 0 };
				tile_CP.d		 = { 0, 0, 0, 1 };
				tile_CP.regionID = {tile.tileID[0], 0, tile.tileID[1], 512 };
				PushVertex(tile_CP, vertexBuffer, frameGraph.Resources);
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
			ctx->SetGraphicsConstantBufferView	(1, data.constantBuffer, data.cameraConstants);
			ctx->SetGraphicsConstantBufferView	(2, data.constantBuffer, data.cameraConstants);
			ctx->SetGraphicsDescriptorTable		(3, data.heap);

			// prime pipeline
			ctx->SetVertexBuffers({
				{ data.vertexBuffer, sizeof(ViewableRegion), (UINT)data.inputVertices}});

			ctx->SetSOTargets({
					resources.WriteStreamOut(intermediateSOs[0],	ctx, sizeof(Region_CP)),
					resources.WriteStreamOut(data.finalBuffer,		ctx, sizeof(Region_CP)) });

			auto iaState = resources.GetObjectState(data.indirectArgs);
			// clear IndirectArgs


			ctx->CopyBufferRegion(
				{ resources.GetObjectResource(data.constantBuffer)	},
				{ data.indirectArgsInitial							},
				{ resources.GetUAVDeviceResource(data.indirectArgs)	},
				{ 0													},
				{ sizeof(uint4)										},
				{ iaState											},
				{ iaState											});

			ctx->ClearSOCounters(
				{	resources.GetSOResource(intermediateSOs[0]),	
					resources.GetSOResource(intermediateSOs[1]), 
					resources.GetSOResource(data.finalBuffer)
				},
				{	resources.GetObjectState(intermediateSOs[0]), 
					resources.GetObjectState(intermediateSOs[1]),
					resources.GetObjectState(data.finalBuffer),
				});

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


				ctx->SetVertexBuffers2({
					resources.ReadStreamOut(
						intermediateSOs[SO1], ctx, sizeof(Region_CP)) });


				ctx->CopyBufferRegion(
					{ querySpace, constants			},	// sources
					{ 16, data.indirectArgsInitial	},  // source offsets
					{ resource, resource			},  // destinations
					{ 0, 4							},  // destination offsets
					{ 4, 12							},  // copy sizes
					{ iaState, iaState				},  // source initial state
					{ iaState, iaState				}); // source final	state


				ctx->BeginQuery(data.queryIntermediate, 0);


				ctx->ExecuteIndirect(
					resources.ReadIndirectArgs(data.indirectArgs, ctx),
					data.indirectLayout);


				ctx->SetSOTargets({
					resources.WriteStreamOut(intermediateSOs[SO2],	ctx, sizeof(Region_CP)),
					resources.WriteStreamOut(data.finalBuffer,		ctx, sizeof(Region_CP)) });


				ctx->ClearSOCounters(
					{ resources.GetSOResource	(intermediateSOs[SO2]) },
					{ resources.GetObjectState	(intermediateSOs[SO2]) });
			}

			ctx->EndQuery(data.queryFinal, 0);
			ctx->EndQuery(data.queryIntermediate, 0);

			ctx->ResolveQuery(
				data.queryIntermediate, 0, 1,
				resources.WriteUAV(data.querySpace, ctx), 0);

			ctx->ResolveQuery(
				data.queryFinal, 0, 1,
				resources.WriteUAV(data.querySpace, ctx), 8);
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
	};

	auto& renderStage = frameGraph.AddNode<RenderTerrainForward>(
		GetCRCGUID(TERRAINCOMPUTEVISABLETILES),
		[&](FrameGraphNodeBuilder& builder, RenderTerrainForward& data)
		{
			data.renderTarget			= builder.WriteRenderTarget(renderTarget);
			data.depthTarget			= builder.WriteDepthBuffer(depthTarget);

			data.indirectArgs			= builder.ReadWriteUAVBuffer(terrainEngine.indirectArgs);
			data.querySpace				= builder.ReadWriteUAVBuffer(terrainEngine.querySpace);
			data.finalBuffer			= builder.ReadSOBuffer(terrainEngine.finalBuffer);
			data.intermediateBuffer1	= builder.ReadSOBuffer(terrainEngine.intermdediateBuffer1);
			data.intermediateBuffer2	= builder.ReadSOBuffer(terrainEngine.intermdediateBuffer2);

			data.cameraConstants		= culledTerrain->cameraConstants;
			data.constantBuffer			= culledTerrain->constantBuffer;
			
			data.indirectArgsInitial	= culledTerrain->indirectArgsInitial;
			data.indirectLayout			= culledTerrain->indirectLayout;

			auto tableLayout = renderSystem->Library.RS4CBVs_SO.GetDescHeap(0);
			data.heap.Init(renderSystem, tableLayout, tempMemory);
			data.heap.NullFill(renderSystem);
			data.splitCount = culledTerrain->splitCount;

		},
		[=](const RenderTerrainForward& data, const FrameResources& resources, Context* ctx)
		{
			auto PSO = resources.GetPipelineState(TERRAIN_RENDER_FOWARD_PSO);

			if (!PSO)
				return;

			auto& rootSig = resources.renderSystem->Library.RS4CBVs_SO;

			ctx->SetRootSignature(rootSig);
			ctx->SetPipelineState(PSO);

			ctx->SetPrimitiveTopology(EInputTopology::EIT_POINT);
			ctx->SetGraphicsConstantBufferView(0, data.constantBuffer, data.cameraConstants);
			ctx->SetGraphicsConstantBufferView(1, data.constantBuffer, data.cameraConstants);
			ctx->SetGraphicsConstantBufferView(2, data.constantBuffer, data.cameraConstants);
			ctx->SetGraphicsDescriptorTable(3, data.heap);

			ctx->SetScissorAndViewports({ resources.GetRenderTarget(data.renderTarget) });
			ctx->SetRenderTargets(
				{ resources.GetRenderTargetObject(data.renderTarget) },
				true,
				resources.GetRenderTargetObject(data.depthTarget));

			const size_t inputArray					= (data.splitCount + 1) % 2;
			FrameResourceHandle	intermediateSOs[]	= { data.intermediateBuffer1, data.intermediateBuffer2 };

			ctx->SetVertexBuffers2({
					resources.ReadStreamOut(
						intermediateSOs[inputArray], ctx, sizeof(Region_CP)) });

			auto destResource	= resources.GetObjectResource(resources.WriteUAV(data.indirectArgs, ctx));
			auto querySpace		= resources.GetObjectResource(resources.ReadUAVBuffer(data.querySpace, DRS_Read, ctx));
			auto constants		= resources.GetObjectResource(data.constantBuffer);
			auto iaState		= resources.GetObjectState(data.indirectArgs);

			ctx->CopyBufferRegion(
				{ querySpace, constants															},  // sources
				{ 16, data.indirectArgsInitial													},	// source offset
				{ destResource, destResource },  // destinations
				{ 0, 4																			},  // destination offsets
				{ 4, 12																			},  // copy sizes
				{ iaState, iaState																},  // source initial	state
				{ iaState, iaState																}); // source final		state

			ctx->ExecuteIndirect(
				resources.ReadIndirectArgs(data.indirectArgs, ctx),
				data.indirectLayout);

			int x = 0;
		});

	return &renderStage;
}


/************************************************************************************************/


class GraphicsTest : public FrameworkState
{
public:

	GraphicsTest(GameFramework* IN_framework) :
		FrameworkState(IN_framework),
		render			{IN_framework->core->GetBlockMemory(), IN_framework->core->RenderSystem},
		depthBuffer		{IN_framework->core->RenderSystem.CreateDepthBuffer		({ 1920, 1080 },	true)},
		vertexBuffer	{IN_framework->core->RenderSystem.CreateVertexBuffer	(8096 * 64,			false)},
		textBuffer		{IN_framework->core->RenderSystem.CreateVertexBuffer	(8096 * 64,			false)},
		constantBuffer	{IN_framework->core->RenderSystem.CreateConstantBuffer	(8096 * 2000,		false)},
		eventMap		{IN_framework->core->GetBlockMemory()},
		terrain			{ IN_framework->GetRenderSystem(), IN_framework->core->GetBlockMemory()},
		scene			{	
			IN_framework->core->RenderSystem, 
			IN_framework->core->GetBlockMemory(),
			IN_framework->core->GetTempMemory()}
		//ltcLookup_1{ IN_framework->Core->RenderSystem.CreateTexture2D(FlexKit::uint2{lookupTableSize, lookupTableSize}, FlexKit::FORMAT_2D::R32G32B32A32_FLOAT)},
		//ltcLookup_2{ IN_framework->Core->RenderSystem.CreateTexture2D(FlexKit::uint2{lookupT's ableSize, lookupTableSize}, FlexKit::FORMAT_2D::R32G32B32A32_FLOAT)}
	{
		auto& RS = IN_framework->core->RenderSystem;
		RS.RegisterPSOLoader(DRAW_PSO,					{&RS.Library.RS4CBVs4SRVs, CreateDrawTriStatePSO});
		RS.RegisterPSOLoader(DRAW_TEXTURED_PSO,			{&RS.Library.RS4CBVs4SRVs, CreateTexturedTriStatePSO});
		RS.RegisterPSOLoader(DRAW_TEXTURED_DEBUG_PSO,	{&RS.Library.RS4CBVs4SRVs, CreateTexturedTriStateDEBUGPSO});
		RS.RegisterPSOLoader(DRAW_LINE_PSO,				{&RS.Library.RS4CBVs4SRVs, CreateDrawLineStatePSO});
		RS.RegisterPSOLoader(DRAW_LINE3D_PSO,			{&RS.Library.RS4CBVs4SRVs, CreateDraw2StatePSO});
		RS.RegisterPSOLoader(DRAW_SPRITE_TEXT_PSO,		{&RS.Library.RS4CBVs4SRVs, LoadSpriteTextPSO });

		RS.QueuePSOLoad(DRAW_PSO);
		RS.QueuePSOLoad(DRAW_LINE3D_PSO);
		RS.QueuePSOLoad(DRAW_TEXTURED_DEBUG_PSO);

		AddResourceFile("testScene.gameres");

		if (!FlexKit::LoadScene(IN_framework->core, &scene, 6001))
		{
			std::cerr << "Failed to Load Scene!\n";
			IN_framework->quit = true;
		}

		std::cout << "Objects in Scene: "	<< scene.Drawables.size() << "\n";
		std::cout << "Lights in Scene: "	<< scene.PLights.size() << "\n";

		scene.ListEntities();

		orbitCamera.TranslateWorld({0, 2.5, 0});

		eventMap.MapKeyToEvent(KEYCODES::KC_R, ReloadShaders_1);
		eventMap.MapKeyToEvent(KEYCODES::KC_W, OCE_MoveForward);
		eventMap.MapKeyToEvent(KEYCODES::KC_S, OCE_MoveBackward);
		eventMap.MapKeyToEvent(KEYCODES::KC_A, OCE_MoveLeft);
		eventMap.MapKeyToEvent(KEYCODES::KC_D, OCE_MoveRight);

		const int TileSize = 1024;
		terrain.SetTileOffset	(int2{-TileSize/2, -TileSize/2});
		terrain.SetTileSize		(TileSize); // will assume square tiles
		terrain.AddTile			({ 0, 0 }, "assets/textures/tiles/tile_0_0.bmp", framework->GetRenderSystem(), framework->core->GetTempMemory());

		/*
		float3x3*	ltc_Table		= (float3x3*)	IN_framework->Core->GetTempMemory()._aligned_malloc(sizeof(FlexKit::float3x3[lookupTableSize*lookupTableSize]));
		float*		ltc_Amplitude	= (float*)		IN_framework->Core->GetTempMemory()._aligned_malloc(sizeof(float [lookupTableSize*lookupTableSize]));

		const float MinAlpha = 0.0001f;

		for (size_t a = lookupTableSize; a >= 0; --a)// steps roughness
		{
			for (size_t t = 0; t < lookupTableSize; ++t)// steps angle
			{
				size_t idx = a * lookupTableSize + t;

				float roughness		= a / (lookupTableSize - 1);
				float alpha			= FlexKit::max(roughness * roughness, MinAlpha);
				float theta			= t *  1.0f / float(lookupTableSize - 1) * pi / 2;

				float3 v{ std::sin(theta), 0, cos(theta) };
				
				float amplitude = ComputeBRDFNorm(v, alpha);

				ltc_Table[idx][0][1] = 0;
				ltc_Table[idx][1][0] = 0;
				ltc_Table[idx][2][1] = 0;
				ltc_Table[idx][1][2] = 0;

				ltc_Table[idx] = ltc_Table[idx] * (1 / ltc_Table[idx][2][2]);
			}
		}
		*/
		//framework->Core->RenderSystem.
	}


	/************************************************************************************************/


	~GraphicsTest()
	{
		auto rendersystem = framework->GetRenderSystem();
		rendersystem->ReleaseVB(vertexBuffer);
		rendersystem->ReleaseVB(textBuffer);
		rendersystem->ReleaseCB(constantBuffer);
		rendersystem->ReleaseDB(depthBuffer);

		FlexKit::ReleaseCameraTable();
	}


	/************************************************************************************************/


	bool Update(
		EngineCore*			Engine, 
		UpdateDispatcher&	Dispatcher, 
		double				dT) override
	{
		auto transformTask = QueueTransformUpdateTask	(Dispatcher);
		auto cameraUpdate  = QueueCameraUpdate			(Dispatcher, transformTask);
		auto orbitUpdate   = QueueOrbitCameraUpdateTask	(Dispatcher, transformTask, cameraUpdate, orbitCamera, framework->MouseState, dT);
		auto sceneUpdate   = scene.Update				(Dispatcher, transformTask);
		auto terrainUpdate = terrain.Update				(Dispatcher);

		return true;
	}


	/************************************************************************************************/


	bool DebugDraw(
		EngineCore*			core, 
		UpdateDispatcher&	dispatcher, 
		double dT) override
	{
		return true;
	}


	/************************************************************************************************/


	bool PreDrawUpdate(
		EngineCore*			engine, 
		UpdateDispatcher&	dispatcher, 
		double				dT) override
	{ 
		return true; 
	}


	/************************************************************************************************/


	bool Draw(
		EngineCore*			core, 
		UpdateDispatcher&	dispatcher, 
		double				dT, 
		FrameGraph&			frameGraph) override
	{
		frameGraph.Resources.AddDepthBuffer(depthBuffer);


		FlexKit::WorldRender_Targets targets = {
			GetCurrentBackBuffer(&core->Window),
			depthBuffer
		};

		PVS	solidDrawables			(core->GetTempMemory());
		PVS	transparentDrawables	(core->GetTempMemory());

		ClearVertexBuffer	(frameGraph, vertexBuffer);
		ClearVertexBuffer	(frameGraph, textBuffer);

		ClearBackBuffer		(frameGraph, 0.0f);
		ClearDepthBuffer	(frameGraph, depthBuffer, 1.0f);

		DrawUI_Desc DrawDesk
		{
			&frameGraph, 
			targets.RenderTarget,
			vertexBuffer, 
			textBuffer, 
			constantBuffer
		};


		CameraHandle activeCamera = (CameraHandle)orbitCamera;

		GetGraphicScenePVS(scene, activeCamera, &solidDrawables, &transparentDrawables);

		if (true) {
			auto culledTerrain = CullTerrain(
				terrain,
				core->RenderSystem,
				frameGraph,
				activeCamera,
				vertexBuffer,
				constantBuffer,
				core->GetTempMemory());

			DrawTerrain_Forward(
				culledTerrain,
				terrain,
				core->RenderSystem,
				frameGraph,
				activeCamera,
				vertexBuffer,
				constantBuffer,
				core->Window.GetBackBuffer(),
				depthBuffer,
				core->GetTempMemory());
		}

		if(true)
			render.DefaultRender(
				solidDrawables,
				activeCamera,
				targets,
				frameGraph,
				core->GetTempMemory());

		if (true)
			DEBUG_DrawQuadTree(
				scene,
				frameGraph,
				activeCamera,
				vertexBuffer,
				constantBuffer,
				core->Window.GetBackBuffer(),
				core->GetTempMemory());

		return true; 
	}


	/************************************************************************************************/


	bool PostDrawUpdate(
		EngineCore*			core, 
		UpdateDispatcher&	dispatcher, 
		double				dT, 
		FrameGraph&			frameGraph)
	{ 

		if (framework->drawDebug)
			framework->DrawDebugHUD(dT, textBuffer, frameGraph);

		PresentBackBuffer(frameGraph, &core->Window);

		return true; 
	}


	/************************************************************************************************/


	bool EventHandler	(Event evt)
	{ 
		eventMap.Handle(
			evt, 
			[&](auto evt)
			{
				orbitCamera.HandleEvent(evt);
			});

		return true; 
	}


	/************************************************************************************************/


private:
	TerrainEngine						terrain;

	FlexKit::InputMap					eventMap;

	FlexKit::TextureHandle				ltcLookup_1;
	FlexKit::TextureHandle				ltcLookup_2;

	FlexKit::OrbitCameraBehavior		orbitCamera;
	FlexKit::GraphicScene				scene;
	FlexKit::WorldRender				render;
	FlexKit::TextureHandle				depthBuffer;
	FlexKit::VertexBufferHandle			vertexBuffer;
	FlexKit::VertexBufferHandle			textBuffer;
	FlexKit::ConstantBufferHandle		constantBuffer;
};


/************************************************************************************************/
#endif