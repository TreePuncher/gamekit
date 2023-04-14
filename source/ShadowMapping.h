#pragma once

#include "graphics.h"
#include "FrameGraph.h"
#include "Scene.h"

namespace FlexKit
{   /************************************************************************************************/


	using AdditionalShadowMapPass =
		TypeErasedCallable
		<void (
			ReserveConstantBufferFunction&	cbReserve,
			ReserveVertexBufferFunction&	vbReserve,
			ResourceHandle					renderTarget,
			LightType						type,
			FrameResources&					resources,
			Context&						ctx,
			iAllocator&						allocator), 64>;

	struct ShadowMapPassData
	{
		const ResourceAllocation&	acquireMaps;
		FrameGraphNodeHandle		multiPass;

		operator FrameGraphNodeHandle () const noexcept { return multiPass; }

		FrameResourceHandle	operator [](size_t idx) const noexcept
		{
			return acquireMaps.handles[idx];
		}
	};

	struct LocalShadowMapPassData
	{
		const Vector<LightHandle>&		pointLightShadows;
		ShadowMapPassData&					sharedData;
		ReserveConstantBufferFunction		reserveCB;
		ReserveVertexBufferFunction			reserveVB;

		static_vector<AdditionalShadowMapPass>	additionalShadowPass;
	};


	struct ShadowCubeMatrices
	{
		float4x4 View[6];
		float4x4 ViewI[6];
		float4x4 PV[6];
	};

	struct ShadowMapMatrices
	{
		float4x4 PV;
		float4x4 view;
	};

	struct AnimationPoseUpload;


	/************************************************************************************************/

	ShadowCubeMatrices	CalculateShadowMapMatrices(const float3 pos, const float r);
	ShadowMapMatrices	CalculateSpotLightMatrices(const float3 pos, const Quaternion q, const float r, const float a);

	/************************************************************************************************/


	constexpr PSOHandle SHADOWMAPPASS				= PSOHandle(GetTypeGUID(SHADOWMAPPASS));
	constexpr PSOHandle SHADOWMAPANIMATEDPASS		= PSOHandle(GetTypeGUID(SHADOWMAPANIMATEDPASS));

	constexpr PSOHandle BUILDROWSUMS				= PSOHandle(GetTypeGUID(BUILDROWSUMS));
	constexpr PSOHandle BUILDCOLUMNSUMS				= PSOHandle(GetTypeGUID(BUILDCOLUMNSUMS));

	constexpr PassHandle ShadowMapPassID			= PassHandle{ GetTypeGUID(SHADOWMAPPASS) };
	constexpr PassHandle ShadowMapAnimatedPassID	= PassHandle{ GetTypeGUID(SHADOWMAPANIMATEDPASS) };


	/************************************************************************************************/


	inline static constexpr size_t ShadowMapSize = 1024;

	class ShadowMapper
	{
		public:
		ShadowMapper(RenderSystem& renderSystem, iAllocator& allocator);

		ShadowMapPassData&  ShadowMapPass(
								FrameGraph&								frameGraph,
								const GatherVisibleLightsTask&			visibleLights,
								LightUpdate&							lightUpdate,
								UpdateTask&								cameraUpdate,
								GatherPassesTask&						passes,
								ReserveConstantBufferFunction&			reserveCB,
								ReserveVertexBufferFunction&			reserveVB,
								static_vector<AdditionalShadowMapPass>&	additional,
								const double							t,
								MemoryPoolAllocator&					shadowMapPool,
								iAllocator&								tempAllocator,
								AnimationPoseUpload*					poses = nullptr);

	private:

		struct PassData
		{
			uint32_t		passIdx;
			LightHandle		pointLightHandle;
			CubeMapState*	shadowMapData;
		};

		struct ResourceEntry
		{
			ResourceHandle	resource;
			size_t			availibility;
		};

		struct Common_Data
		{
			ReserveConstantBufferFunction			reserveCB;
			ReserveVertexBufferFunction				reserveVB;
			FrameResourceHandle						depthBuffer;
			static_vector<AdditionalShadowMapPass>	additional;
		};

		ResourceHandle	GetResource(const size_t frameID);
		void			AddResource(ResourceHandle, const size_t frameID);

		using Iterator_TY = std::span<VisibilityHandle>::iterator;

		void RenderPointLightShadowMap	(Iterator_TY begin, Iterator_TY end, PassData& pass, ShadowMapper::Common_Data& common, ResourceHandle renderTarget, FrameResources& resources, Context& ctx, iAllocator& allocator);
		void RenderSpotLightShadowMap	(Iterator_TY begin, Iterator_TY end, PassData& pass, ShadowMapper::Common_Data& common, ResourceHandle renderTarget, FrameResources& resources, Context& ctx, iAllocator& allocator);
		void BuildSummedAreaTable		(ResourceHandle target, FrameResources& resources, Context& ctx, iAllocator& allocator);


		Vector<ResourceEntry>  resourcePool;

		ID3D12PipelineState* CreateShadowMapPass(RenderSystem* RS);
		ID3D12PipelineState* CreateShadowMapAnimatedPass(RenderSystem* RS);

		ID3D12PipelineState* CreateRowSums(RenderSystem* RS);
		ID3D12PipelineState* CreateColumnSums(RenderSystem* RS);

		FlexKit::RootSignature rootSignature;
	};


}   /************************************************************************************************/



/**********************************************************************

Copyright (c) 2015 - 2023 Robert May

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
