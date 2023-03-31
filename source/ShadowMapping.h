#pragma once

#include "graphics.h"
#include "FrameGraph.h"
#include "Scene.h"

namespace FlexKit
{   /************************************************************************************************/


	using AdditionalShadowMapPass =
		TypeErasedCallable
		<void (
			ReserveConstantBufferFunction&,
			ReserveVertexBufferFunction&,
			ConstantBufferDataSet*,
			ResourceHandle*,
			const size_t,
			const ResourceHandler&,
			Context&,
			iAllocator&), 256>;

	struct ShadowMapPassData
	{
		FrameGraphNodeHandle multiPass;
	};

	struct LocalShadowMapPassData
	{
		const Vector<LightHandle>&		pointLightShadows;
		ShadowMapPassData&					sharedData;
		ReserveConstantBufferFunction		reserveCB;
		ReserveVertexBufferFunction			reserveVB;

		static_vector<AdditionalShadowMapPass>	additionalShadowPass;
	};

	struct AcquireShadowMapResources
	{
	};

	using AcquireShadowMapTask = UpdateTaskTyped<AcquireShadowMapResources>;


	struct ShadowPassMatrices
	{
		float4x4 View[6];
		float4x4 ViewI[6];
		float4x4 PV[6];
	};


	struct AnimationPoseUpload;


	/************************************************************************************************/

	ShadowPassMatrices CalculateShadowMapMatrices(const float3 pos, const float r, const float T);

	/************************************************************************************************/


	constexpr PSOHandle SHADOWMAPPASS				= PSOHandle(GetTypeGUID(SHADOWMAPPASS));
	constexpr PSOHandle SHADOWMAPANIMATEDPASS		= PSOHandle(GetTypeGUID(SHADOWMAPANIMATEDPASS));

	constexpr PSOHandle BUILD2DSAT					= PSOHandle(GetTypeGUID(BUILD2DSAT));

	constexpr PassHandle ShadowMapPassID			= PassHandle{ GetTypeGUID(SHADOWMAPPASS) };
	constexpr PassHandle ShadowMapAnimatedPassID	= PassHandle{ GetTypeGUID(SHADOWMAPANIMATEDPASS) };


	/************************************************************************************************/


	inline static constexpr size_t ShadowMapSize = 1024;

	class ShadowMapper
	{
		public:
		ShadowMapper(RenderSystem& renderSystem, iAllocator& allocator);

		AcquireShadowMapTask& AcquireShadowMaps(
								UpdateDispatcher&		dispatcher,
								RenderSystem&			renderSystem,
								MemoryPoolAllocator&	UAVPool,
								PointLightUpdate&		pointLightUpdate);


		ShadowMapPassData&  ShadowMapPass(
								FrameGraph&								frameGraph,
								const LightShadowGatherTask&			shadowMaps,
								UpdateTask&								cameraUpdate,
								GatherPassesTask&						passes,
								AcquireShadowMapTask&					shadowMapAcquire,
								ReserveConstantBufferFunction&			reserveCB,
								ReserveVertexBufferFunction&			reserveVB,
								static_vector<AdditionalShadowMapPass>&	additional,
								const double							t,
								iAllocator&								tempAllocator,
								AnimationPoseUpload*					poses = nullptr);

	private:

		struct ResourceEntry
		{
			ResourceHandle	resource;
			size_t			availibility;
		};

		ResourceHandle	GetResource(const size_t frameID);
		void			AddResource(ResourceHandle, const size_t frameID);

		Vector<ResourceEntry>  resourcePool;

		ID3D12PipelineState* CreateShadowMapPass(RenderSystem* RS);
		ID3D12PipelineState* CreateShadowMapAnimatedPass(RenderSystem* RS);

		ID3D12PipelineState* Fill2DPlanes(RenderSystem* RS);

		FlexKit::RootSignature rootSignature;
	};


}   /************************************************************************************************/
