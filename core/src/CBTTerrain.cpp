#include "CBTTerrain.hpp"
#include <TextureUtilities.hpp>
#include <stb_image.h>


namespace FlexKit
{	/************************************************************************************************/


	CBTTerain::CBTTerain(iAllocator& persistent, IRenderSystem& renderSystem) :
		cbt{ renderSystem, persistent }
	{
		static bool registerStates = [&]
			{
				RegisterAdaptiveUpdateCBT(renderSystem);

				renderSystem.GetPSO(AdaptiveTerrainUpdate, persistent);
				auto temp = std::get<1>(renderSystem.GetPSOAndRootSignature(AdaptiveTerrainUpdate, persistent));
				indirectDispatchLayout = renderSystem.CreateIndirectLayout(
					{
						IndirectDrawDescription{
							IndirectDrawDescription::Constant{.rootParameterIdx = 3, .destinationOffset = 0, .numValues = 1 } },
						IndirectDrawDescription{ IndirectLayoutEntryType::ILE_DispatchCall },
					},
					persistent, temp);


				indirectDrawLayout = renderSystem.CreateIndirectLayout(
					{
						IndirectDrawDescription{ IndirectLayoutEntryType::ILE_DrawCall },
					},
					persistent);


				return true;
			}();


		const uint32_t depth = 27;
		cbt.Initialize({ .maxDepth = depth });

		cbt.SetBit(0 * (1 << (depth - 1)), true); // 2
		cbt.SetBit(1 * (1 << (depth - 1)), true); // 3

		auto descAllocation = renderSystem.CreateDescriptorRange(1);
		textureDesc	= descAllocation.value();
		renderSystem.CreateTextureView(renderSystem.DefaultTexture(), textureDesc);
	}


	/************************************************************************************************/


	void CBTTerain::RegisterAdaptiveUpdateCBT(IRenderSystem& renderSystem)
	{
		renderSystem.RegisterPSOLoader(
			AdaptiveTerrainUpdateArgs,
			[](IRenderSystem& irs, iAllocator& tempAllocator)
			{
				return PipelineBuilder{ irs, tempAllocator }.
					AddComputeShader("AdaptiveUpdateGetArgs", R"(assets\shaders\CBT\CBT_GetArguments.hlsl)", { .hlsl2021 = true }).
					Build(irs);
			});

		renderSystem.RegisterPSOLoader(
			AdaptiveTerrainUpdate,
			[](IRenderSystem& irs, iAllocator& tempAllocator)
			{
				return PipelineBuilder{ irs, tempAllocator }.
					AddComputeShader("UpdateAdaptiveTerrain", R"(assets\shaders\CBT\CBT_TerrainAdapt.hlsl)", { .hlsl2021 = true }).
					Build(irs);
			});

		renderSystem.RegisterPSOLoader(
			AdaptiveTerrainDrawArgs,
			[](IRenderSystem& irs, iAllocator& tempAllocator)
			{
				return PipelineBuilder{ irs, tempAllocator }.
					AddComputeShader("AdaptiveDrawGetArgs", R"(assets\shaders\CBT\CBT_GetArguments.hlsl)", { .hlsl2021 = true }).
					Build(irs);
			});

		renderSystem.RegisterPSOLoader(
			RenderTerrain, [](IRenderSystem& irs, iAllocator& allocator) -> LoadPipelineStateRes
			{
				return PipelineBuilder{ irs, allocator }.
						AddInputTopology(ETopology::EIT_TRIANGLE).
						AddVertexShader("DrawCBTTerrain_VS", "assets\\shaders\\cbt\\CBT_DebugVis.hlsl", { .hlsl2021 = true }).
						AddPixelShader("DrawCBTTerrain_PS1", "assets\\shaders\\cbt\\CBT_DebugVis.hlsl", { .hlsl2021 = true }).
						AddRasterizerState({ .fill = EFillMode::SOLID, .CullMode = ECullMode::NONE, .depthClipEnable = true }).
						AddRenderTargetState(
							{	
								.targetCount	= 1, 
								.targetFormats	= { DeviceFormat::R16G16B16A16_FLOAT },
							}).
						AddDepthStencilState({
								.depthEnable	= true,
								.stencilEnable	= false
							}).
						AddDepthStencilFormat(DeviceFormat::D32_FLOAT).
						Build(irs);
			});

		renderSystem.RegisterPSOLoader(
			RenderTerrainWireframe, [](IRenderSystem& irs, iAllocator& allocator) -> LoadPipelineStateRes
			{
				return PipelineBuilder{ irs, allocator }.
						AddInputTopology(ETopology::EIT_TRIANGLE).
						AddVertexShader("DrawCBTTerrain_VS", "assets\\shaders\\cbt\\CBT_TerrainForward.hlsl", { .hlsl2021 = true }).
						AddPixelShader("DrawCBTTerrain_PS2", "assets\\shaders\\cbt\\CBT_TerrainForward.hlsl", { .hlsl2021 = true }).
						AddRasterizerState({ .fill = EFillMode::WIREFRAME, .CullMode = ECullMode::NONE, .depthClipEnable = false }).
						AddRenderTargetState(
							{	
								.targetCount	= 1, 
								.targetFormats	= { DeviceFormat::R16G16B16A16_FLOAT },
							}).
						AddDepthStencilState({
								.depthEnable	= false,
								.depthFunc		= FlexKit::EComparison::EQUAL,
								.stencilEnable	= false,
							}).
						AddDepthStencilFormat(DeviceFormat::D32_FLOAT).
						Build(irs);
			});

		renderSystem.QueuePSOLoad(AdaptiveTerrainUpdateArgs);
		renderSystem.QueuePSOLoad(AdaptiveTerrainDrawArgs);
		renderSystem.QueuePSOLoad(AdaptiveTerrainUpdate);
	}


	/************************************************************************************************/


	void CBTTerain::LoadHeightMapFromPath(IRenderSystem& renderSystem, std::filesystem::path heightMapPath, iAllocator& allocator)
	{
		int width, height, channels;
		auto heightValues = stbi_loadf(heightMapPath.string().c_str(), &width, &height, &channels, 1);

		if (!heightValues)
			throw std::runtime_error("Failed to load Height Map!");

		const uint2 wh{ width, height };
		size_t rowPitch		= FlexKit::AlignedSize(width * 4);
		size_t bufferSize	= rowPitch * height;

		TextureBuffer converted				{ wh, 4, bufferSize, FlexKit::SystemAllocator };
		TextureBuffer buffer				{ wh, (std::byte*)heightValues, 4 };
		TextureBufferView<float> inputView	{ buffer };
		TextureBufferView<float> outputView	{ converted, rowPitch };

		for (size_t y_itr = 0; y_itr < wh[1]; y_itr++)
		{
			for (size_t x_itr = 0; x_itr < wh[0]; x_itr++)
			{
				FlexKit::uint2 px	= { x_itr, y_itr };
				outputView[px]		= inputView[px];
			}
		}

		free(heightValues);

		heightMap = renderSystem.LoadTexture(&converted, renderSystem.GetImmediateCopyQueue(), DeviceFormat::R32_FLOAT, allocator);
		//heightMap = LoadTexture(&converted, renderSystem.GetImmediateCopyQueue(), renderSystem, allocator, DeviceFormat::R32_FLOAT); // WIP: FOR LATER REMOVAL

		auto descAllocation = renderSystem.CreateDescriptorRange(1);
		textureDesc = descAllocation.value();
		renderSystem.CreateTextureView(heightMap, textureDesc);
		//PushTextureToDescHeap(renderSystem, DXGI_FORMAT_R32_FLOAT, (uint32_t)0u, heightMap, textureDesc); // WIP: FOR LATER REMOVAL
	}


	/************************************************************************************************/


	void CBTTerain::SetHeightMap(ResourceHandle handle)
	{
		heightMap = handle;
	}


	/************************************************************************************************/


	void CBTTerain::AdaptiveLODUpdate(CameraHandle camera, ReserveConstantBufferFunction& cbAllocator, FrameGraph& frameGraph, double dT)
	{
		struct CBT_UpdateAdaptiveTerrain
		{
			FrameResourceHandle				indirectArgumentsBuffer;
			FrameResourceHandle				cbtBuffer;
			ReserveConstantBufferFunction	cbAllocator;
		};

		frameGraph.AddNode<CBT_UpdateAdaptiveTerrain>(
			{
				.cbAllocator = cbAllocator
			},
			[&](FlexKit::FrameGraphNodeBuilder& builder, CBT_UpdateAdaptiveTerrain& args)
			{
				builder.Requires(AdaptiveTerrainUpdate);
				builder.Requires(AdaptiveTerrainUpdateArgs);
				
				args.indirectArgumentsBuffer	= builder.AcquireVirtualResource(FlexKit::GPUResourceDesc::UAVResource(1024), FlexKit::DeviceAccessState::DASUAV);
				args.cbtBuffer					= builder.UnorderedAccess(cbt.GetBuffer());
			},
			[&, camera, dT](CBT_UpdateAdaptiveTerrain& args, ResourceHandler& resources, IDirectContext& ctx, iAllocator& threadLocalAllocator)
			{
				ctx.BeginEvent_DEBUG("Update CBT Adaptive Terrain");
				const uint32_t maxDepth = cbt.GetMaxDepth();

				ctx.DiscardResource(resources.GetResource(args.indirectArgumentsBuffer));
				ctx.SetComputePipelineState(AdaptiveTerrainUpdateArgs, threadLocalAllocator);
				ctx.SetComputeUnorderedAccessView(0, resources.GetResource(args.cbtBuffer));
				ctx.SetComputeUnorderedAccessView(1, resources.GetResource(args.indirectArgumentsBuffer));
				ctx.SetComputeConstantValue(2, 1, &maxDepth);
				ctx.Dispatch({ 1, 1, 1 });

				static double t = (FlexKit::pi * 3.0f / 2.0f);

				auto cameraConstants = GetCameraConstants(camera);
				struct {
					FlexKit::float4x4_GPU	PV;
					uint32_t				maxDepth;
					float					scale;
				} constants{ 
					.PV			= cameraConstants.PV,
					.maxDepth	= cbt.GetMaxDepth(),
					.scale		= 50 * (sinf(t) / 2.0f + 0.5f)
				};

				struct {
					float4x4_GPU	view;
					Frustum			frustum;
				}	intersectionConstants{
					.view		= cameraConstants.View,
					.frustum	= GetFrustumVS(camera)
				};

				CBPushBuffer			cbLocalAllocator{ args.cbAllocator(AlignedSize(sizeof(intersectionConstants))) };
				ConstantBufferDataSet	frustumConstants{ intersectionConstants, cbLocalAllocator };

				ctx.SetComputePipelineState(AdaptiveTerrainUpdate, threadLocalAllocator);
				ctx.SetComputeUnorderedAccessView(0, resources.UAV(args.cbtBuffer, ctx));
				ctx.SetComputeConstantValue(1, 18, &constants);
				ctx.SetComputeDescriptorTable(2, textureDesc);
				ctx.SetComputeConstantBufferView(4, frustumConstants);
				ctx.ExecuteIndirect(resources.IndirectArgs(args.indirectArgumentsBuffer, ctx), indirectDispatchLayout, 0);

				ctx.AddUAVBarrier(resources.GetResource(args.cbtBuffer));
				ctx.EndEvent_DEBUG();
			});

		cbt.SumReduction_GPU(frameGraph);
	}


	/************************************************************************************************/


	void CBTTerain::Render(CameraHandle camera, UpdateTask* update, ResourceHandle renderTarget, ResourceHandle depthTarget, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
	{
		struct CBTDebugVis
		{
			FrameResourceHandle renderTarget;
			FrameResourceHandle depthTarget;
			FrameResourceHandle cbtBuffer;
			FrameResourceHandle indirectArgumentsBuffer;
		};

		frameGraph.AddNode<CBTDebugVis>(
			{},
			[&](FrameGraphNodeBuilder& builder, CBTDebugVis& debugVis)
			{
				if (update)
					builder.AddDataDependency(*update);

				builder.Requires(RenderTerrain);
				builder.Requires(RenderTerrainWireframe);
				builder.Requires(AdaptiveTerrainDrawArgs);

				debugVis.renderTarget				= builder.RenderTarget(renderTarget);
				debugVis.depthTarget				= builder.DepthTarget(depthTarget);
				debugVis.cbtBuffer					= builder.NonPixelShaderResource(cbt.GetBuffer());
				debugVis.indirectArgumentsBuffer	= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(1024), DeviceAccessState::DASUAV);
			},
			[&, camera, dT](CBTDebugVis& debugVis, ResourceHandler& resources, IDirectContext& ctx, iAllocator& threadLocalAllocator)
			{
				ctx.BeginEvent_DEBUG("DebugVisCBTTree");

				struct {
					float4x4_GPU	PV;
					uint32_t		maxDepth;
					float			scale;
				} constants{
					.PV			= GetCameraConstants(camera).PV,
					.maxDepth	= cbt.GetMaxDepth(),
				};

				ctx.DiscardResource(resources.GetResource(debugVis.indirectArgumentsBuffer));
				ctx.SetComputePipelineState(AdaptiveTerrainDrawArgs, threadLocalAllocator);
				ctx.SetComputeUnorderedAccessView(0, resources.UAV(debugVis.cbtBuffer, ctx));
				ctx.SetComputeUnorderedAccessView(1, resources.UAV(debugVis.indirectArgumentsBuffer, ctx));
				ctx.SetComputeConstantValue(2, 1, &constants.maxDepth);
				ctx.Dispatch({ 1, 1, 1 });

				ctx.SetGraphicsPipelineState(RenderTerrain, threadLocalAllocator);

				ctx.SetGraphicsConstantValue(0, 18, &constants);
				ctx.SetGraphicsShaderResourceView(1, resources.NonPixelShaderResource(debugVis.cbtBuffer, ctx));
				ctx.SetGraphicsDescriptorTable(2, textureDesc);
				
				RenderTargetList renderTargets = { resources.RenderTarget(debugVis.renderTarget, ctx) };
				ctx.SetScissorAndViewports(renderTargets);
				ctx.SetInputPrimitive(EInputPrimitive::INPUTPRIMITIVETRIANGLELIST);
				ctx.SetRenderTargets(renderTargets, true, resources.DepthTarget(debugVis.depthTarget, ctx));

				ctx.ExecuteIndirect(
					resources.IndirectArgs(debugVis.indirectArgumentsBuffer, ctx), 
					indirectDrawLayout);

				if (wireframe)
				{
					ctx.SetGraphicsPipelineState(RenderTerrainWireframe, threadLocalAllocator);
					ctx.ExecuteIndirect(
						resources.IndirectArgs(debugVis.indirectArgumentsBuffer, ctx),
						indirectDrawLayout);
				}

				ctx.EndEvent_DEBUG();
			});
	}


	/************************************************************************************************/


	void CBTTerain::Upload(FrameGraph& frameGraph)
	{
		cbt.Upload(frameGraph);
		cbt.SumReduction_GPU(frameGraph);
	}

}


/**********************************************************************

Copyright (c) 2024 Robert May

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
