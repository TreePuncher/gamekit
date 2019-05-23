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


	ID3D12PipelineState* CreateCullTerrainComputePSO			(RenderSystem* renderSystem);
	ID3D12PipelineState* CreateForwardRenderTerrainPSO			(RenderSystem* renderSystem);
	ID3D12PipelineState* CreateForwardRenderTerrainWireFramePSO	(RenderSystem* renderSystem);


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
			renderSystem				{ RS														},
			tiles						{ IN_allocator												},
			tileTextures				{ IN_allocator												},
			tileHeight					{ IN_tileHeight												}, 
			tileOffset					{ IN_tileOffset												},
			allocator					{ IN_allocator												},
			finalBuffer					{ RS->CreateStreamOutResource(MEGABYTE * 64)				},
			intermdediateBuffer1		{ RS->CreateStreamOutResource(MEGABYTE * 64)				},
			intermdediateBuffer2		{ RS->CreateStreamOutResource(MEGABYTE * 64)				},
			queryBufferFinalBuffer		{ RS->CreateSOQuery(1,1)									},
			queryBufferIntermediate		{ RS->CreateSOQuery(0,1)									},
			indirectArgs				{ RS->CreateUAVBufferResource(512)							},
			querySpace					{ RS->CreateUAVBufferResource(512)							},
			indirectLayout				{ RS->CreateIndirectLayout({ILE_DrawCall}, IN_allocator)	}
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
		/*
		FlexKit::UpdateTask& Update(FlexKit::UpdateDispatcher& Dispatcher)
		{
			FK_LOG_9("Terrain Update");

			// TODO: Streaming textures
			// Make sure textures are loaded
			return nullptr;
		}
		*/

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


	struct TerrainRenderResources
	{
		VertexBufferHandle		vertexBuffer;
		ConstantBufferHandle	constantBuffer;
		TextureHandle			renderTarget;
		TextureHandle			depthTarget;
		RenderSystem&			renderSystem;
	};


	auto CreateDefaultTerrainConstants(CameraHandle camera, const TerrainEngine& terrainEngine)
	{
		return [&, camera](CBPushBuffer& pushBuffer) -> ConstantBufferDataSet {
			uint32_t indirectArgsInitial[] = { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

			TerrainConstantBufferLayout constants;
			constants.Albedo           = { 0.0f, 1.0f, 1.0f, 0.9f };
			constants.Specular         = { 1, 1, 1, 1 };
			constants.RegionDimensions = { 4096, 4096 };
			constants.Frustum          = GetFrustum(camera);
			constants.PassCount        = 16;
			constants.debugMode        = terrainEngine.DebugMode;

			return { constants, pushBuffer };
		};
	}


	template<
		typename TY_GetCameraConstantSetFN,
		typename TY_GetTerrainConstantSetFN>	
	class ForwardTerrainRenderer
	{
		using TY_Renderer = ForwardTerrainRenderer<TY_GetCameraConstantSetFN, TY_GetTerrainConstantSetFN>;

	public:
		ForwardTerrainRenderer(
				TY_GetCameraConstantSetFN&	IN_GetCameraConstantSet, 
				TY_GetTerrainConstantSetFN& IN_GetTerrainConstantSet, 
				TerrainEngine&				IN_terrainEngine, 
				iAllocator* IN_tempMemory) :
			GetCameraConstants	{ IN_GetCameraConstantSet	},
			GetTerrainConstants	{ IN_GetTerrainConstantSet	},
			tempMemory			{ IN_tempMemory				},
			terrainEngine		{ IN_terrainEngine			},
			dataDependencies	{ IN_tempMemory				} {}


		void DependsOn(UpdateTask& task)
		{
			dataDependencies.push_back(&task);
		}


		struct TerrainCullerResults
		{
			TerrainCullerResults(
				const uint16_t				IN_regionCount,
				TerrainRenderResources&		IN_resources, 
				TerrainEngine&				IN_terrainEngine,
				TY_GetCameraConstantSetFN	IN_GetCameraConstants,
				TY_GetTerrainConstantSetFN	IN_GetTerrainConstants,
				iAllocator*					IN_allocator
			) noexcept :
					GetCameraConstants	{ IN_GetCameraConstants													},
					GetTerrainConstants	{ IN_GetTerrainConstants												},
					regionCount			{ IN_regionCount														},
					terrainEngine		{ IN_terrainEngine														},
					vertexBuffer		{ IN_resources.vertexBuffer,	512 * 12,	IN_resources.renderSystem	},
					constantBuffer		{ IN_resources.constantBuffer,	2048,		IN_resources.renderSystem	},
					tileHeightMaps		{ IN_allocator,								IN_regionCount				} {}

			TY_GetCameraConstantSetFN	GetCameraConstants;
			TY_GetTerrainConstantSetFN	GetTerrainConstants;

			size_t						GetOutputBuffer() const
			{
				return (splitCount + 1) % 2;
			}

			TerrainEngine&			terrainEngine;

			const uint16_t			regionCount;
			const uint16_t			splitCount		= 12;

			IndirectLayout			indirectLayout;

			FrameResourceHandle		finalBuffer;
			FrameResourceHandle		intermediateBuffer1;
			FrameResourceHandle		intermediateBuffer2;
			FrameResourceHandle		querySpace;

			DescriptorHeap			heap;
			FrameResourceHandle		indirectArgs; // UAV Buffer

			QueryHandle				queryFinal;
			QueryHandle				queryIntermediate;

			VBPushBuffer			vertexBuffer;
			CBPushBuffer			constantBuffer;

			Vector<size_t>			tileHeightMaps;
		};


		auto& Cull(
			FrameGraph&				frameGraph, 
			CameraHandle&			camera,
			TerrainRenderResources&	resources)
		{
			frameGraph.Resources.AddSOResource(terrainEngine.finalBuffer,			0);
			frameGraph.Resources.AddSOResource(terrainEngine.intermdediateBuffer1,	0);
			frameGraph.Resources.AddSOResource(terrainEngine.intermdediateBuffer2,	0);

			frameGraph.Resources.AddUAVResource(terrainEngine.indirectArgs, 0, resources.renderSystem.GetObjectState(terrainEngine.indirectArgs));
			frameGraph.Resources.AddUAVResource(terrainEngine.querySpace,	0, resources.renderSystem.GetObjectState(terrainEngine.querySpace));
				
			frameGraph.Resources.AddQuery(terrainEngine.queryBufferFinalBuffer);
			frameGraph.Resources.AddQuery(terrainEngine.queryBufferIntermediate);

			auto& terrainEngine_ref = terrainEngine;
			
			auto& cullerStage = frameGraph.AddNode<TerrainCullerResults>(
				TerrainCullerResults{
					1,
					resources,
					terrainEngine,
					GetCameraConstants,
					GetTerrainConstants,
					tempMemory
				},
				[&, terrainEngine_ref, resources](FrameGraphNodeBuilder& builder, TerrainCullerResults& data)
				{
					for(auto dependency : dataDependencies)
						builder.AddDataDependency(*dependency);

					data.finalBuffer			= builder.WriteSOBuffer	(terrainEngine_ref.finalBuffer);
					data.intermediateBuffer1	= builder.WriteSOBuffer	(terrainEngine_ref.intermdediateBuffer1);
					data.intermediateBuffer2	= builder.WriteSOBuffer	(terrainEngine_ref.intermdediateBuffer2);

					data.indirectLayout			= terrainEngine_ref.indirectLayout;
					data.indirectArgs			= builder.ReadWriteUAVBuffer(terrainEngine_ref.indirectArgs);

					data.queryFinal				= terrainEngine_ref.queryBufferFinalBuffer;
					data.queryIntermediate		= terrainEngine_ref.queryBufferIntermediate;
					data.querySpace				= builder.ReadWriteUAVBuffer(terrainEngine_ref.querySpace);

					auto tableLayout			= resources.renderSystem.Library.RS4CBVs_SO.GetDescHeap(0);
					data.heap.Init(resources.renderSystem, tableLayout, tempMemory);
					data.heap.NullFill(resources.renderSystem);

					for(auto& tileMap : terrainEngine.tileTextures)
						data.heap.SetSRV(resources.renderSystem, 0, tileMap.heightMap);
				},
				[=](TerrainCullerResults& data, const FrameResources& resources, Context* ctx)
				{
					uint32_t				indirectArgsValues[]	= { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
					ConstantBufferDataSet	terrainConstants		= data.GetTerrainConstants(data.constantBuffer);
					ConstantBufferDataSet	cameraConstants			= data.GetCameraConstants(data.constantBuffer);
					ConstantBufferDataSet	indirectArgs			= { indirectArgsValues, data.constantBuffer };

					VertexBufferDataSet		initialVertices			= {	
							data.terrainEngine.tiles, 
							[&](auto& tile) -> Region_CP
							{ 
								Region_CP tile_CP;
								tile_CP.parentID	= { 0, 0, 0, 0 };
								tile_CP.UVs			= { 0, 0, 1, 1 };
								tile_CP.d			= { 1, 0, 0, static_cast<int32_t>(tile.textureMaps) };
								tile_CP.regionID	= { tile.tileID[0], 0, tile.tileID[1], 1024 * 16 };

								data.tileHeightMaps.push_back(tile.textureMaps);

								return tile_CP;
							}, 
							data.vertexBuffer 
						};

					auto PSO = resources.GetPipelineState(TERRAIN_COMPUTE_CULL_PSO);

					if (!PSO)
						return;

					auto& rootSig = resources.renderSystem->Library.RS4CBVs_SO;
			
					ctx->SetRootSignature(rootSig);
					ctx->SetPipelineState(PSO);

					FrameResourceHandle	intermediateSOs[]	= { data.intermediateBuffer1, data.intermediateBuffer2 };
					int					offset				= 0;

					ctx->SetPrimitiveTopology			(EInputTopology::EIT_POINT);
					ctx->SetGraphicsConstantBufferView	(0, cameraConstants);
					ctx->SetGraphicsConstantBufferView	(1, terrainConstants);
					ctx->SetGraphicsConstantBufferView	(2, terrainConstants);
					ctx->SetGraphicsDescriptorTable		(3, data.heap);

					{	// Clear counters
						auto constantBufferResource	= resources.GetObjectResource((ConstantBufferHandle)data.constantBuffer);
						auto indirectArgsResource	= resources.GetUAVDeviceResource(data.indirectArgs);
						auto querySpaceResource		= resources.GetUAVDeviceResource(data.querySpace);
						auto UAV					= resources.WriteUAV(data.indirectArgs, ctx);

						auto iaState = resources.GetObjectState(data.indirectArgs);
						auto qsState = resources.GetObjectState(data.querySpace);

						ctx->CopyBufferRegion(
							{ constantBufferResource,	constantBufferResource			}, // source 
							{ indirectArgs,				indirectArgs + 8,				}, // source offset
							{ indirectArgsResource,		querySpaceResource				}, // destinations
							{ 0, 16														}, // dest offset
							{ sizeof(uint4), sizeof(uint4)								}, // copy sizes
							{ iaState, qsState											},
							{ iaState, qsState											});
					}

					// prime pipeline
					ctx->SetVertexBuffers({ initialVertices });
					
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
						auto constants		= resources.GetObjectResource	((ConstantBufferHandle)data.constantBuffer);
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

					ctx->EndQuery(data.queryIntermediate, 0);
					ctx->EndQuery(data.queryFinal, 0);

					ctx->ResolveQuery(
						data.queryIntermediate, 0, 1,
						resources.WriteUAV(data.querySpace, ctx), 0);
					ctx->ResolveQuery(
						data.queryFinal, 0, 1,
						resources.WriteUAV(data.querySpace, ctx), 16);

					{
						auto indirectArgs	= resources.GetObjectResource(resources.WriteUAV(data.indirectArgs, ctx));
						auto queryResults	= resources.GetObjectResource(resources.ReadUAVBuffer(data.querySpace, DRS_Read, ctx));
						auto iaState		= resources.GetObjectState(data.indirectArgs);

						ctx->CopyBufferRegion(
							{ queryResults,	},  // sources
							{ 16,			},	// source offset
							{ indirectArgs, },  // destinations
							{ 0,			},  // destination offsets
							{ 4, 			},  // copy sizes
							{ iaState,		},  // source initial state
							{ iaState,		}); // destination final state
					}
					ctx->SetSOTargets({});

					ctx->ClearSOCounters(
						{	resources.ClearStreamOut(intermediateSOs[0],	ctx),
							resources.ClearStreamOut(intermediateSOs[1],	ctx),
							resources.ClearStreamOut(data.finalBuffer,		ctx) });
				});

			return cullerStage;
		}


		// Will take ownership of cullerResults
		template<typename TY_CULLERRESULTS>
		auto& Draw(
			FrameGraph&				frameGraph,
			TY_CULLERRESULTS&		culledInput,
			TextureHandle			renderTarget, 
			TextureHandle			depthTarget,
			TerrainRenderResources&	resources)
		{
			struct _RenderTerrainForward
			{
				_RenderTerrainForward(
					TerrainRenderResources		IN_resources, 
					TY_CULLERRESULTS&			IN_culledInput,
					IndirectLayout&				IN_indirectLayout, 
					DescriptorHeap&				IN_descriptorHeap,
					TY_GetCameraConstantSetFN&	IN_GetCameraConstants,
					TY_GetTerrainConstantSetFN&	IN_GetTerrainConstants
				) noexcept :
					resoures			{ IN_resources															},
					culledTerrain		{ IN_culledInput														},
					indirectLayout		{ IN_indirectLayout														},
					regionCount			{ IN_culledInput.regionCount											},
					splitCount			{ IN_culledInput.splitCount												},
					heap				{ IN_descriptorHeap														},
					vertexBuffer		{ IN_resources.vertexBuffer,	512 * 12,	IN_resources.renderSystem	},
					constantBuffer		{ IN_resources.constantBuffer,	2048,		IN_resources.renderSystem	},
					GetCameraConstants	{ IN_GetCameraConstants													},
					GetTerrainConstants	{ IN_GetTerrainConstants												} {}

				IndirectLayout&				indirectLayout;

				TY_GetCameraConstantSetFN	GetCameraConstants;
				TY_GetTerrainConstantSetFN	GetTerrainConstants;

				TY_CULLERRESULTS&			culledTerrain;
				TerrainRenderResources		resoures;


				const uint16_t				regionCount;
				const uint16_t				splitCount;


				FrameResourceHandle			renderTarget;
				FrameResourceHandle			depthTarget;

				FrameResourceHandle			finalBuffer;
				FrameResourceHandle			intermediateBuffer1;
				FrameResourceHandle			intermediateBuffer2;
				FrameResourceHandle			querySpace;
				DescriptorHeap&				heap;
				FrameResourceHandle			indirectArgs; // UAV Buffer

				QueryHandle					queryFinal;
				QueryHandle					queryIntermediate;
	
				VBPushBuffer				vertexBuffer;
				CBPushBuffer				constantBuffer;

				TerrainEngine::TERRAINDEBUGRENDERMODE debugMode;
			};
			
			auto& renderStage = frameGraph.AddNode<_RenderTerrainForward>(
				_RenderTerrainForward{
					resources, 
					culledInput,
					terrainEngine.indirectLayout,
					culledInput.heap,
					GetCameraConstants,
					GetTerrainConstants
				},
				[&](FrameGraphNodeBuilder& builder, _RenderTerrainForward& data)
				{
					data.debugMode				= terrainEngine.DebugMode;
					data.renderTarget			= builder.WriteRenderTarget(renderTarget);
					data.depthTarget			= builder.WriteDepthBuffer(depthTarget);

					data.indirectArgs			= builder.ReadWriteUAVBuffer(terrainEngine.indirectArgs);
					data.querySpace				= builder.ReadWriteUAVBuffer(terrainEngine.querySpace);
					data.finalBuffer			= builder.ReadSOBuffer(terrainEngine.finalBuffer);
					data.intermediateBuffer1	= builder.ReadSOBuffer(terrainEngine.intermdediateBuffer1);
					data.intermediateBuffer2	= builder.ReadSOBuffer(terrainEngine.intermdediateBuffer2);
				},
				[=](_RenderTerrainForward& data, const FrameResources& resources, Context* ctx)
				{
					ConstantBufferDataSet	terrainConstants			= data.GetTerrainConstants(data.constantBuffer);
					ConstantBufferDataSet	cameraConstants				= data.GetCameraConstants(data.constantBuffer);

					auto PSO = resources.GetPipelineState(TERRAIN_RENDER_FOWARD_PSO);

					if (!PSO)
						return;

					auto& rootSig = resources.renderSystem->Library.RS4CBVs_SO;

					ctx->SetRootSignature(rootSig);
					ctx->SetPipelineState(PSO);

					ctx->SetPrimitiveTopology(EInputTopology::EIT_PATCH_CP_1);
					ctx->SetGraphicsConstantBufferView(0, cameraConstants);
					ctx->SetGraphicsConstantBufferView(1, terrainConstants);
					ctx->SetGraphicsConstantBufferView(2, terrainConstants);
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
								data.finalBuffer, ctx, sizeof(Region_CP)) });

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

					});
			return renderStage;
		}

	private:
		TerrainEngine&				terrainEngine;
		Vector<UpdateTask*>			dataDependencies;
		iAllocator*					tempMemory;

		TY_GetCameraConstantSetFN	GetCameraConstants;
		TY_GetTerrainConstantSetFN	GetTerrainConstants;
	};


	template<typename TY_GetCameraConstantsFN>
	auto& CreateForwardTerrainRender(TY_GetCameraConstantsFN& getCameraConstantsFN, TerrainEngine& IN_terrainEngine, iAllocator* tempAllocator)
	{
		return tempAllocator->allocate<ForwardTerrainRenderer<TY_GetCameraConstantsFN>>(getCameraConstantsFN, IN_terrainEngine, tempAllocator);
	}

	/************************************************************************************************/

}

#endif