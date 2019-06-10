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
#include "..\coreutilities\intersection.h"
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


	ID3D12PipelineState* CreateCullTerrainComputePSO			(RenderSystem* renderSystem);
	ID3D12PipelineState* CreateForwardRenderTerrainPSO			(RenderSystem* renderSystem);
	ID3D12PipelineState* CreateForwardRenderTerrainWireFramePSO	(RenderSystem* renderSystem);


	struct TileMaps
	{
		FlexKit::TextureHandle	heightMap;
		const char*				File;
	};


	using TileMapHandle = Handle_t<16, GetTypeGUID(TileMaps)>;

	class TileID
	{
	public:
		static const size_t sBitFields = 56 / 2;

		enum TerrainTileBitSection : uint8_t
		{
			NorthEast,
			NorthWest,
			SouthWest,
			SouthEast,
			MASK
		};


		template<size_t size = 0>
		TileID(TerrainTileBitSection ID[size]) : 
			tileDepth		{ size													},
			tileIDBitField	{ _GenerateBitField(static_cast<uint8_t*>(ID, size))	}
		{
			static_assert(size < sBitFields, "Too many bit fields!");

			tileIDBitField	= tileIDBitField | id[0];
			size_t i		= 0;

			do
			{
				tileIDBitField = (tileIDBitField << 2) | ID[i];
			} while (i < size);
		}


		TileID(uint64_t bitField = 0, size_t depth = 0) :
			tileDepth		{ depth		},
			tileIDBitField	{ bitField	} {}


		TileID CreateChildID(TerrainTileBitSection localID) const
		{
			uint64_t temp = tileIDBitField;
			temp <<= 2;
			temp |= localID;

			return { temp, tileDepth + 1u };
		}

		TileID GetParentID() const
		{
			uint64_t temp = tileIDBitField;
			return { _GenerateBitField(
						reinterpret_cast<uint8_t*>(&temp), tileDepth - 1), 
						isRoot() ? 0u : tileDepth - 1u };
		}

		bool isRoot() const
		{
			return { tileDepth == 0 };
		}

		TileID(const TileID& rhs)				= default;
		TileID& operator = (const TileID& rhs)	= default;


		bool operator == (const TileID& rhs) const
		{
			return rhs.tileDepth == tileDepth && tileIDBitField == rhs.tileIDBitField;
		}


		bool operator != (const TileID& rhs) const
		{
			return !(*this == rhs);
		}
	

		TerrainTileBitSection operator [](size_t idx)
		{
			return static_cast<TerrainTileBitSection>(tileIDBitField >> (tileDepth - idx));
		}


		uint8_t GetDepth() { return static_cast<uint8_t>(tileDepth); }

	private:

		static uint64_t _GenerateBitField(uint8_t* bitFields, const uint8_t fieldCount)
		{
			uint64_t bitField	= 0;
			bitField			= bitField | bitFields[0];
			size_t i			= 0;

			do
			{
				bitField = (bitField << 2) | bitFields[i];
			} while (i < fieldCount);

			return bitField;
		}

		const size_t	tileDepth		: 8;
		const size_t	tileIDBitField	: 56;
	};


	struct TerrainPatch
	{
		AABB GetAABB() const noexcept
		{
			return { 
				{ static_cast<float>(SWPoint[0]), static_cast<float>(lowerHeight), static_cast<float>(SWPoint[1]) },
				{ static_cast<float>(NEPoint[0]), static_cast<float>(upperHeight), static_cast<float>(NEPoint[1]) } };
		}

		static_vector<float3, 4> GetPointList(const float terrainHeight = 512) const noexcept
		{
			// TODO: Sample Height Map
			float HeightSamples[] = { 0, 0, 0, 0 };

			return {
				{ float(NEPoint[0]),	float(HeightSamples[0]) * terrainHeight, float(NEPoint[1]) },
				{ float(SWPoint[0]),	float(HeightSamples[1]) * terrainHeight, float(NEPoint[1]) },
				{ float(SWPoint[0]),	float(HeightSamples[2]) * terrainHeight, float(SWPoint[1]) },
				{ float(NEPoint[0]),	float(HeightSamples[3]) * terrainHeight, float(SWPoint[1]) } };
		}

		static_vector<float3, 4> GetUVList(const float terrainHeight = 512) const noexcept
		{
			return {
				{ float(NEUV[0]), float(NEUV[1]) },
				{ float(SWUV[0]), float(NEUV[1]) },
				{ float(SWUV[0]), float(SWUV[1]) },
				{ float(NEUV[0]), float(SWUV[1]) } };
		}

		uint16_t		upperHeight	= 0u;
		uint16_t		lowerHeight	= 0u;
		int2			SWPoint		= { 0, 0 };
		int2			NEPoint		= { 0, 0 };
		float2			SWUV		= { 0.0f, 0.0f };
		float2			NEUV		= { 1.0f, 1.0f };

		TextureHandle	HeightMap			= InvalidHandle_t;
		int				HeightMapSampleBias = 0;
	};


	class TerrainPayload // Interface
	{
	public:
		virtual void Release() {};
		virtual void Load()	{};
	};


	struct TerrainNode
	{
		AABB GetAABB() const noexcept
		{
			return patch.GetAABB();
		}

		struct Edge
		{
			float3 A = { 0, 0, 0 };
			float3 B = { 0, 0, 0 };
		};

		
		TerrainPatch	patch;
		TileID			id;

		typedef static_vector<TerrainNode*, 4> TNodeList;

		TNodeList		children;
		TileMapHandle	heightMap;

		TerrainPayload* payload = nullptr;

		void Split(iAllocator* allocator)
		{

		}

		TileID GetNearestID(const TileID ID)
		{
			return {};
		}

		TerrainNode& GetNearestNode(const TileID)
		{
			return *this;
		}
	};


	static_vector<TerrainPatch, 4> CreateSubPatches(const TerrainPatch& patch)
	{
		auto NWPatch = patch;
		auto NEPatch = patch;
		auto SWPatch = patch;
		auto SEPatch = patch;

		auto parentPatchWidth	= (patch.NEPoint[0]	- patch.SWPoint[0]) / 2;
		auto parentUVWidth		= (patch.NEUV[0]	- patch.SWUV[0]) / 2;

		if (parentPatchWidth == 16)
			return {};

		NWPatch.SWPoint[1]	+= parentPatchWidth;
		NWPatch.NEPoint[0]	-= parentPatchWidth;
		NWPatch.SWUV[1]		+= parentUVWidth;
		NWPatch.NEUV[0]		-= parentUVWidth;

		NEPatch.SWPoint[0]	+= parentPatchWidth;
		NEPatch.SWPoint[1]	+= parentPatchWidth;
		NEPatch.SWUV[0]		+= parentUVWidth;
		NEPatch.SWUV[1]		+= parentUVWidth;

		SWPatch.NEPoint[0]	-= parentPatchWidth;
		SWPatch.NEPoint[1]	-= parentPatchWidth;
		SWPatch.NEUV[0]		-= parentUVWidth;
		SWPatch.NEUV[1]		-= parentUVWidth;

		SEPatch.SWPoint[0]	+= parentPatchWidth;
		SEPatch.NEPoint[1]	-= parentPatchWidth;
		SEPatch.SWUV[0]		+= parentUVWidth;
		SEPatch.NEUV[1]		-= parentUVWidth;

		return { NWPatch, NEPatch, SWPatch, SEPatch };
	}

	enum ECORNERID
	{
		EC_NW, 
		EC_NE, 
		EC_SW, 
		EC_SE
	};

	using TNodeList = TerrainNode::TNodeList;
	using PatchList = Vector<TerrainPatch>;


	class TerrainEngine
	{
	public:
		TerrainEngine(RenderSystem* RS, iAllocator* IN_allocator, size_t IN_tileEdgeSize = 128, size_t IN_tileHeight = 128, uint2 IN_tileOffset = {0, 0}) :
			renderSystem				{ RS														},
			tileTextures				{ IN_allocator												},
			tileHeight					{ IN_tileHeight												}, 
			tileOffset					{ IN_tileOffset												},
			allocator					{ IN_allocator												}
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
				{	&renderSystem->Library.RS4CBVs_SO,
					CreateForwardRenderTerrainWireFramePSO });

			renderSystem->QueuePSOLoad(TERRAIN_COMPUTE_CULL_PSO);
			renderSystem->QueuePSOLoad(TERRAIN_RENDER_FOWARD_PSO);
			renderSystem->QueuePSOLoad(TERRAIN_RENDER_FOWARD_WIREFRAME_PSO);

			root.Split(IN_allocator);
		}


		void SetMainHeightMap(const char* heightMap, RenderSystem* RS, iAllocator* tempMemory)
		{
			if(true)
			{
				// Load bitmap
				static_vector<TextureBuffer> Textures;
				Textures.push_back(TextureBuffer{});
				LoadBMP(heightMap, tempMemory, &Textures.back());
				
				Textures.push_back(BuildMipMap(Textures.back(), tempMemory));
				Textures.push_back(BuildMipMap(Textures.back(), tempMemory));
				Textures.push_back(BuildMipMap(Textures.back(), tempMemory));
				Textures.push_back(BuildMipMap(Textures.back(), tempMemory));

				//auto textureHandle	= MoveTextureBuffersToVRAM(RS, Textures.begin(), Textures.size(), allocator);
				auto textureHandle		= MoveTextureBufferToVRAM(RS, &Textures[0], allocator);
				
				unsigned int tileMap	= tileTextures.push_back(TileMaps{ textureHandle, heightMap });
				auto minMax				= GetMinMax(Textures.front());

				root.heightMap			= TileMapHandle{ tileMap };
				root.patch.lowerHeight	= minMax.Get<0>();
				root.patch.upperHeight	= minMax.Get<1>();
			}
			else
			{
				// Load DDS
				auto [textureHandle, res] = LoadDDSTexture2DFromFile_2(heightMap, tempMemory, RS);

				if (!res)
					throw std::runtime_error("Failed to created texture!");

				unsigned int tileMap = tileTextures.push_back(TileMaps{ textureHandle, heightMap });

				root.heightMap = TileMapHandle{ tileMap };
			}
		}

		void SetRegionDimensions(size_t width)
		{
			// TODO: propogate changes to chilren
			root.patch.NEPoint = {  int(width) / 2,	 int(width) / 2 };
			root.patch.SWPoint = { -int(width) / 2, -int(width) / 2 };
		}

		struct TerrainUpdate
		{
			CameraHandle			camera;
			TerrainEngine*			engine;
			Vector<TerrainPatch>	patches; //Patch list ready for rendering
			StackAllocator			localMemory;
		};


		UpdateTask& CreatePatchList(CameraHandle camera, float maxPatchEdgeSize, UpdateDispatcher& dispatcher, UpdateTask& cameraUpdate, iAllocator* tempMemory)
		{
			// constants
			static const size_t TempMemorySize = 512 * KILOBYTE;

			return dispatcher.Add<TerrainUpdate>(
				[&, this](auto& builder, TerrainUpdate& data)
				{
					builder.AddInput(cameraUpdate);

					data.localMemory.Init((byte*)tempMemory->malloc(TempMemorySize), TempMemorySize);

					data.camera				= camera;
					data.patches			= Vector<TerrainPatch>(data.localMemory);
					data.engine				= this;
				},
				[camera, maxPatchEdgeSize, this](TerrainUpdate& data)
				{
					FK_LOG_9("Begin Terrain Patch Generation");

					auto frustum	= GetFrustum(camera);
					auto transform	= GetCameraPV(camera);

					_GatherPatches(root, frustum, transform, maxPatchEdgeSize, data.patches);

					FK_LOG_9("End Terrain Patch Generation");
				});
		}


		static bool _CreateSubPatches(TerrainPatch patch, const Frustum frustum, const float4x4 transform, float maxPatchEdgeSize, Vector<TerrainPatch>& out_patche)
		{
			return false;
		}

		
		static bool _GatherPatches(TerrainNode& node, const Frustum frustum, const float4x4 transform, float maxPatchEdgeSize, Vector<TerrainPatch>& out_patches)
		{
			if (auto aabb = node.GetAABB(); Intersects(frustum, aabb))
			{
				const float longestEdgeSize = _GetLargestEdgeSize(node.patch, transform);
				const float minSize = 0.25f;

				if (longestEdgeSize > minSize)
				{
					bool res = false;

					if (node.children.size())
					{	// use child node payloads instead
						for (auto child : node.children)
							res |= _GatherPatches(*child, frustum, transform, maxPatchEdgeSize, out_patches);
					}
					else// Create virtual nodes
					{
						auto addSubPatches = [&](TerrainPatch& patch, auto& addSubPatches) ->bool
						{
							bool addedPatches			= false;
							auto patches				= CreateSubPatches(patch);
							const float largestEdgeSize = _GetLargestEdgeSize(patch, transform);

							if (largestEdgeSize < minSize)
								return false;

							for (auto childPatch : patches)
							{
								if (Intersects(frustum, childPatch.GetAABB())) {
									const bool res = addSubPatches(childPatch, addSubPatches);
									addedPatches |= res;

									if(!res)
										out_patches.push_back(childPatch);
								}
							}

							return addedPatches;
						};

						res |= addSubPatches(node.patch, addSubPatches);
					}

					return res;
				}
				else 
				{
					out_patches.push_back(node.patch);
					return true;
				}
			}

			return false;
		}

		// Edge Size in Screen Space
		static float _GetLargestEdgeSize(TerrainPatch& patch, const float4x4 transform)
		{
			float longestEdge	= 0.0f;
			const auto WSPoints = patch.GetPointList();

			auto point1 = Vect4ToFloat4(transform * float4(WSPoints[0], 1));
			auto point2 = Vect4ToFloat4(transform * float4(WSPoints[1], 1));
			auto point3 = Vect4ToFloat4(transform * float4(WSPoints[2], 1));
			auto point4 = Vect4ToFloat4(transform * float4(WSPoints[3], 1));

			point1 /= max(0.000001f, point1.w);
			point2 /= max(0.000001f, point2.w);
			point3 /= max(0.000001f, point3.w);
			point4 /= max(0.000001f, point4.w);

			longestEdge = max(longestEdge, (point1 - point2).xyz().magnitude()) / 2;
			longestEdge = max(longestEdge, (point2 - point3).xyz().magnitude()) / 2;
			longestEdge = max(longestEdge, (point3 - point4).xyz().magnitude()) / 2;
			longestEdge = max(longestEdge, (point4 - point1).xyz().magnitude()) / 2;

			return longestEdge;
		}

		RenderSystem*		renderSystem;

		TerrainNode			root;
		Vector<TileMaps>	tileTextures;

		enum TERRAINDEBUGRENDERMODE
		{
			DISABLED,
			WIREFRAME,
		} DebugMode = DISABLED;

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


	template<typename _GETCAMERACONSTANTS>
	auto& DEBUG_DrawTerrainPatches(
		UpdateTask&				gatherPatches,
		FrameGraph&				frameGraph,
		VertexBufferHandle		vertexBuffer,
		ConstantBufferHandle	constantBuffer,
		TextureHandle			renderTarget,
		iAllocator*				tempMemory,
		_GETCAMERACONSTANTS		GetCameraConstants)
	{
		static_assert(std::is_same<decltype(GetCameraConstants(CBPushBuffer{})), ConstantBufferDataSet > (), "GetCameraConstants is required to return a ConstantBufferDataSet has one argument of type CBPushBuffer!");

		struct debugDraw
		{
			debugDraw(
				RenderSystem&			renderSystem,
				VertexBufferHandle		vertexBuffer, 
				ConstantBufferHandle	constantBuffer,
				_GETCAMERACONSTANTS		IN_GetCameraConstants
			) : 
				GetConstants	{ IN_GetCameraConstants					},
				patches			{ vertexBuffer,		4096 * KILOBYTE, renderSystem	},
				constants		{ constantBuffer,	1024, renderSystem	} {}

			FlexKit::DescriptorHeap	heap; // Null Filled

			FrameResourceHandle		renderTarget;
			VBPushBuffer			patches;
			CBPushBuffer			constants;
			_GETCAMERACONSTANTS		GetConstants;
		};

		auto& update = *reinterpret_cast<TerrainEngine::TerrainUpdate*>(gatherPatches.Data);

		return frameGraph.AddNode<debugDraw>(
			debugDraw{
				frameGraph.GetRenderSystem(),
				vertexBuffer, 
				constantBuffer,
				GetCameraConstants
			},
			[&](FrameGraphNodeBuilder& builder, debugDraw& data)
			{
				builder.AddDataDependency(gatherPatches);

				data.renderTarget = builder.WriteRenderTarget(renderTarget);

				data.heap.Init(
					frameGraph.Resources.renderSystem,
					frameGraph.Resources.renderSystem.Library.RS6CBVs4SRVs.GetDescHeap(0),
					tempMemory).NullFill(frameGraph.Resources.renderSystem);
			}, 
			[&patches = update.patches](debugDraw& data, const FrameResources& resources, Context* ctx)
			{
				struct Vertex
				{
					float4 POS;
					float4 Color;
					float2 UV;
				};

				struct LocalConstants
				{
					float4		unused1;
					float4		unused2;
					float4x4	worldTransform = float4x4::Identity();
				};


				VertexBufferDataSet	vertices = {
						SET_TRANSFORM_OP,
						patches,
						[&](const TerrainPatch& patch, auto& out)
						{
							Vertex v;
							v.Color = float4(WHITE, 1);

							const auto points	= patch.GetPointList();
							const auto UVs		= patch.GetUVList();

							for (size_t i = 0; i < points.size(); ++i)
							{
								v.Color = float4{ UVs[i].y, UVs[i].x, 1, 0 };
								v.UV	= UVs[i];
								v.POS	= { points[i], 1 };
								out.append(v);

								size_t nextIdx	= (i + points.size() + 1) % points.size();
								v.Color			= float4{ UVs[nextIdx].y, UVs[nextIdx].x, 1, 0 };
								v.UV			= UVs[nextIdx];
								v.POS			= { points[nextIdx], 1 };
								out.append(v);
							}
						},
						data.patches,
						Vertex{}
					};

				ConstantBufferDataSet localConstants	= ConstantBufferDataSet{ LocalConstants{}, data.constants };
				ConstantBufferDataSet cameraConstants	= data.GetConstants(data.constants);

				ctx->SetRootSignature(resources.renderSystem.Library.RS6CBVs4SRVs);

				ctx->SetPipelineState(resources.GetPipelineState(DRAW_LINE3D_PSO));
				ctx->SetRenderTargets({ resources.GetRenderTargetDescHeapEntry(data.renderTarget) }, false);

				ctx->SetPrimitiveTopology(EInputTopology::EIT_LINE);
				ctx->SetVertexBuffers({ vertices });

				ctx->SetGraphicsDescriptorTable(0, data.heap);
				ctx->SetGraphicsConstantBufferView(1, cameraConstants);
				ctx->SetGraphicsConstantBufferView(2, localConstants);

				ctx->Draw(patches.size() * 2 * 4, 0);
			});
	}


	auto CreateDefaultTerrainConstants(CameraHandle camera, const TerrainEngine& terrainEngine)
	{
		return [debugMode = terrainEngine.DebugMode, camera](CBPushBuffer& pushBuffer) -> ConstantBufferDataSet {
			uint32_t indirectArgsInitial[] = { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

			TerrainConstantBufferLayout constants;
			constants.Albedo           = { 0.0f, 1.0f, 1.0f, 0.9f };
			constants.Specular         = { 1.0f, 1.0f, 1.0f, 1.0f };
			constants.RegionDimensions = { 4096.0f, 4096.0f };
			constants.Frustum          = GetFrustum(camera);
			constants.PassCount        = 16;
			constants.debugMode        = debugMode;

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
				iAllocator*					IN_tempMemory) :
			GetCameraConstants	{ IN_GetCameraConstantSet	},
			GetTerrainConstants	{ IN_GetTerrainConstantSet	},
			tempMemory			{ IN_tempMemory				},
			terrainEngine		{ IN_terrainEngine			},
			dataDependencies	{ IN_tempMemory				} {}


		void DependsOn(UpdateTask& task)
		{
			dataDependencies.push_back(&task);
		}

		auto& Draw(
			FrameGraph&				frameGraph,
			UpdateTask&				gatherPatches,
			TextureHandle			renderTarget, 
			TextureHandle			depthTarget,
			TerrainRenderResources&	resources)
		{
			struct _RenderTerrainForward
			{
				_RenderTerrainForward(
					RenderSystem&				renderSystem,
					PatchList&					IN_patches,
					TY_GetCameraConstantSetFN	IN_GetCameraConstants,
					TY_GetTerrainConstantSetFN	IN_GetTerrainConstants,
					ConstantBufferHandle		IN_constantBuffer) : 
					GetCameraConstants	{ IN_GetCameraConstants							},
					GetTerrainConstants	{ IN_GetTerrainConstants						},
					pushBuffer			{ IN_constantBuffer, 2 * KILOBYTE, renderSystem	},
					patches				{ IN_patches									} {}

				PatchList&					patches;
				FrameResourceHandle			renderTarget;
				FrameResourceHandle			depthTarget;
				CBPushBuffer				pushBuffer;
				TY_GetCameraConstantSetFN	GetCameraConstants;
				TY_GetTerrainConstantSetFN	GetTerrainConstants;
			};
			
			auto& gatherResults		= *reinterpret_cast<TerrainEngine::TerrainUpdate*>(gatherPatches.Data);
			PatchList patches		= gatherResults.patches;

			auto& renderStage = frameGraph.AddNode<_RenderTerrainForward>(
				_RenderTerrainForward{
					frameGraph.GetRenderSystem(),
					patches,
					GetCameraConstants,
					GetTerrainConstants,
					resources.constantBuffer
				},
				[&](FrameGraphNodeBuilder& builder, _RenderTerrainForward& data)
				{
					data.renderTarget	= builder.WriteRenderTarget(renderTarget);
					data.depthTarget	= builder.WriteDepthBuffer(depthTarget);
				},
				[=](_RenderTerrainForward& data, const FrameResources& resources, Context* ctx)
				{
					// TODO: convert patches

					//ctx->SetRootSignature();
					//ctx->SetPipelineState();
					ctx->SetPrimitiveTopology(EInputTopology::EIT_PATCH_CP_1);
					ctx->SetRenderTargets(
						{ resources.GetRenderTargetDescHeapEntry(data.renderTarget) }, 
						true, 
						resources.GetRenderTargetDescHeapEntry(data.depthTarget));
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