#include "PlanetComponent.h"
#include "WorldRender.h"
#include <bit>


namespace FlexKit
{	/************************************************************************************************/

	PlanetComponentData PlanetComponentEventHandler::OnCreate(GameObject& gameObject)
	{
		if (!gameObject.hasView(TransformComponentID))
			gameObject.AddView<SceneNodeView>();

		return {
			.radius = 10,
			.node	= GetSceneNode(gameObject)
		};
	}


	/************************************************************************************************/



	LoadPipelineStateRes CreateTestStatePSO(RenderSystem* renderSystem, iAllocator& tempAllocator)
	{
		PipelineBuilder builder{ tempAllocator };
		builder.AddVertexShader	("VTestMain", R"(assets\shaders\CBT\ConcurrentBinaryTree.hlsl)");

		builder.AddRasterizerState({
				.fill		= EFillMode::WIREFRAME,
				.CullMode	= ECullMode::NONE,
			});

		builder.AddPixelShader("PTestMain", R"(assets\shaders\CBT\ConcurrentBinaryTree.hlsl)");

		builder.AddRenderTargetState({
				.targetCount	= 1,
				.targetFormats	= {
					DeviceFormat::R16G16B16A16_FLOAT
				}
			});


		return builder.Build(*renderSystem);
	}


	/************************************************************************************************/


	LoadPipelineStateRes CreateSumReductionCBTPSO(RenderSystem* renderSystem, iAllocator& tempAllocator)
	{
		PipelineBuilder builder{ tempAllocator };
		builder.AddComputeShader("ComputeSumReductionCBT", R"(assets\shaders\CBT\ConcurrentBinaryTree.hlsl)");

		return builder.Build(*renderSystem);
	}


	/************************************************************************************************/


	void RegisterPlanetRenderingPipelineStates(RenderSystem& renderSystem)
	{
		renderSystem.RegisterPSOLoader(PlanetTestPSO,		{ CreateTestStatePSO });
		renderSystem.RegisterPSOLoader(SumReductionCBTPSO,	{ CreateSumReductionCBTPSO });
	}


	/************************************************************************************************/


	struct PlanetPass
	{
		FrameResourceHandle albedo;
		FrameResourceHandle mria;
		FrameResourceHandle normal;
		FrameResourceHandle depthTarget;
	};


	void PlanetGBufferPass(ExtraGBufferPassInputs& inputs)
	{
		return;
		auto& planets = PlanetComponent::GetComponent();

		if (planets.size())
		{
			inputs.frameGraph.AddNode<PlanetPass>(
				{},
				[&](FrameGraphNodeBuilder& builder, PlanetPass& data)
				{
					data.albedo			= builder.RenderTarget(inputs.gbuffer.albedo);
					data.mria			= builder.RenderTarget(inputs.gbuffer.MRIA);
					data.normal			= builder.RenderTarget(inputs.gbuffer.normal);
					data.depthTarget	= builder.DepthTarget(inputs.depthTarget);
				},
				[=](PlanetPass& data, const ResourceHandler& frameResources, Context& ctx, iAllocator& allocator)
				{
					ctx.BeginEvent_DEBUG("Planet Rendering Pass");
					ctx.EndEvent_DEBUG();
				});
		}
	}


	/************************************************************************************************/


	void PlanetTestPass(ExtraForwardPassInputs& inputs)
	{
		auto& planets = PlanetComponent::GetComponent();

		if (planets.size())
		{
			struct TestPass
			{
				FrameResourceHandle renderTarget;
				FrameResourceHandle depthTarget;
				FrameResourceHandle CBT;
			};

			inputs.frameGraph.AddNode<TestPass>(
				{},
				[&](FrameGraphNodeBuilder& builder, TestPass& data)
				{
					data.renderTarget	= builder.RenderTarget(inputs.renderTarget);
					data.depthTarget	= builder.DepthTarget(inputs.depthTarget);
					data.CBT			= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(64*KILOBYTE), DeviceAccessState::DASCopyDest);

					builder.Requires(PlanetTestPSO);
					builder.Requires(SumReductionCBTPSO);
				},
				[=](TestPass& data, const ResourceHandler& frameResources, Context& ctx, iAllocator& allocator)
				{
					constexpr auto t = 2 << (4 - 1);

					ctx.BeginEvent_DEBUG("Planet Rendering Pass");

					constexpr uint D = 4;

					static_vector<uint32_t, 64> CBT;
					CBT.SetFull();

					uint TriangleCount = 0;
					CBT[0] = D;
#if 1
					CBT[ipow(2u, D) +  0] = 1;
					CBT[ipow(2u, D) +  1] = 0;
					CBT[ipow(2u, D) +  2] = 0;
					CBT[ipow(2u, D) +  3] = 0;
					CBT[ipow(2u, D) +  4] = 0;
					CBT[ipow(2u, D) +  5] = 0;
					CBT[ipow(2u, D) +  6] = 0;
					CBT[ipow(2u, D) +  7] = 0;
					CBT[ipow(2u, D) +  8] = 0;
					CBT[ipow(2u, D) +  9] = 0;
					CBT[ipow(2u, D) + 10] = 0;
					CBT[ipow(2u, D) + 11] = 0;
					CBT[ipow(2u, D) + 12] = 0;
					CBT[ipow(2u, D) + 13] = 0;
					CBT[ipow(2u, D) + 14] = 0;
					CBT[ipow(2u, D) + 15] = 0;

					for (auto& b : std::span(CBT.begin() + ipow(2u, D), CBT.end()))
						TriangleCount += 1;
#else
					CBT[ipow(2u, D) +  1] = 0;
					CBT[ipow(2u, D) +  2] = 0;
					CBT[ipow(2u, D) +  3] = 0;
					CBT[ipow(2u, D) +  4] = 0;
					CBT[ipow(2u, D) +  5] = 0;
					CBT[ipow(2u, D) +  6] = 0;
					CBT[ipow(2u, D) +  7] = 0;
					CBT[ipow(2u, D) +  8] = 0;
					CBT[ipow(2u, D) +  9] = 0;
					CBT[ipow(2u, D) + 10] = 0;
					CBT[ipow(2u, D) + 11] = 0;
					CBT[ipow(2u, D) + 12] = 0;
					CBT[ipow(2u, D) + 13] = 0;
					CBT[ipow(2u, D) + 14] = 0;
					CBT[ipow(2u, D) + 15] = 0;
#endif
					auto reserve = ctx.ReserveDirectUploadSpace(CBT.ByteSize());
					memcpy(reserve.buffer, CBT.data(), CBT.ByteSize());

					ctx.CopyBuffer(reserve, frameResources.CopyDest(data.CBT, ctx, Sync_All_Shading, Sync_Copy));

					ctx.SetComputePipelineState(SumReductionCBTPSO, allocator);
					ctx.SetComputeUnorderedAccessView(1, frameResources.UAV(data.CBT, ctx, Sync_Copy, Sync_Compute));

					for (int i = D - 1; i >= 0; --i)
					{
						ctx.SetComputeConstantValue(0, 1, &i);
						ctx.SetComputeConstantValue(0, 15, CBT.data(), 1);
						ctx.Dispatch({ 1, 1, 1 });
						ctx.AddUAVBarrier(frameResources.GetResource(data.CBT));
					}

					ctx.SetGraphicsPipelineState(PlanetTestPSO, allocator);
					ctx.SetGraphicsUnorderedAccessView(1, frameResources.GetResource(data.CBT));
					ctx.SetRenderTargets({ frameResources.RenderTarget(data.renderTarget, ctx, Sync_Compute, Sync_Draw) });
					ctx.SetScissorAndViewports({ frameResources.GetResource(data.renderTarget) });
					ctx.Draw(3 * TriangleCount);

					ctx.EndEvent_DEBUG();
				});
		}
	}


}	/************************************************************************************************/


/**********************************************************************

Copyright (c) 2016-2024 Robert May

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
