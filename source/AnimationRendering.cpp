#include "AnimationRendering.h"
#include "graphics.h"
#include <ranges>

namespace FlexKit
{	/************************************************************************************************/


	AnimationPoseUpload::PoseView AnimationPoseUpload::GetIterator()
	{
		auto& buffer = GetBuffer(reserve, passes);
		return CreateCBIterator<BrushPoseBlock>( buffer );
	}

	CBPushBuffer& AnimationPoseUpload::GetData()
	{
		return GetBuffer(reserve, passes);
	}


	AnimationPoseUpload& UploadPoses(FrameGraph& frameGraph, GatherPassesTask& passes, ReserveConstantBufferFunction& reserveCB, iAllocator& allocator)
	{
		return frameGraph.AddNode<AnimationPoseUpload>(
			AnimationPoseUpload{
				reserveCB,
				passes,
				allocator
			},
			[&](FrameGraphNodeBuilder& builder, AnimationPoseUpload& data)
			{
			},
			[&](AnimationPoseUpload& data, ResourceHandler& resources, Context& ctx, iAllocator& localAllocator)
			{
				auto& pass  = passes.GetData().passes;
				auto res    = FindPass(pass.begin(), pass.end(), GBufferAnimatedPassID);

				if (!res || !res->pvs.size())
					return;

				auto& buffer = data.GetData();

				struct EntityPoses
				{
					float4x4 transforms[768];

					auto& operator [](size_t idx)
					{
						return transforms[idx];
					}
				}poses;

				auto& poseBuffer = data.GetData();

				for (auto& skinnedBrush : res->pvs)
				{
					auto pose       = GetPoseState(*skinnedBrush.gameObject);
					auto skeleton   = pose->Sk;

					for (size_t I = 0; I < pose->JointCount; I++)
						poses[I] = (skeleton->IPose[I] * pose->CurrentPose[I]);

					ConstantBufferDataSet((char*)&poses, sizeof(pose->JointCount) * sizeof(float4x4), poseBuffer);
				}
			});
	}


	/************************************************************************************************/


	const ResourceAllocation& AcquirePoseResources(
		FrameGraph&						frameGraph,
		UpdateDispatcher&				dispatcher,
		GatherPassesTask&				passes,
		ReserveConstantBufferFunction&	reserveConstants,
		PoolAllocatorInterface&			pool,
		iAllocator&						allocator)
	{
		PassDrivenResourceAllocation allocation {
			.getPass				= [&passes]() -> std::span<const PVEntry> { return passes.GetData().GetPass(GBufferAnimatedPassID); },
			.initializeResources	=
				[](std::span<const PVEntry> brushes, std::span<const FrameResourceHandle> handles, auto& transferContext, iAllocator& allocator)
				{
					auto itr = handles.begin();

					for (auto&& [sortID, brush, gameObject, occlusionID, submissionID, LODlevel] : brushes)
					{

						auto poseState	= GetPoseState(*gameObject);
						auto skeleton	= poseState->Sk;

						const size_t poseSize = sizeof(float4x4) * poseState->JointCount;
						float4x4* pose = (float4x4*)allocator.malloc(poseSize);

						for (size_t I = 0; I < poseState->JointCount; I++)
							pose[I] = (skeleton->IPose[I] * poseState->CurrentPose[I]);

						transferContext.CreateResource(*(itr++), poseSize, pose);
					}
				},

			.layout		= FlexKit::DeviceLayout::DeviceLayout_Common,
			.access		= FlexKit::DeviceAccessState::DASNonPixelShaderResource,
			.max		= 32,
			.pool		= &pool,
			.dependency = &passes
		};

		return frameGraph.AllocateResourceSet(allocation);
	}


	/************************************************************************************************/


	UpdateTask& LoadNeededMorphTargets(UpdateDispatcher& dispatcher, GatherPassesTask& passes)
	{
		using std::views::zip;

		struct _ {};
		return dispatcher.Add<_>(
			[&](auto& builder, _&)
			{
				builder.AddInput(passes);
			},
			[&passes](_& data, iAllocator& threadAllocator)
			{
				ProfileFunction();

				FK_LOG_9("Start Loading Needed Morph Targets Updates.");

				const auto& pass = passes.GetData().GetPass(MorphTargetID);

				for (const auto& [sortID, brush, gameObject, occlusioID, submissionID, lods] : pass)
				{
					for (auto [lod, mesh] : zip(lods, brush->meshes))
					{
						if (lod == 0 && gameObject->hasView(AnimatorComponentID))
						{
							AnimatorView& animator = *gameObject->GetView<AnimatorView>();
							auto resource = FlexKit::GetMeshResource(mesh);
							for (auto& morphTarget : resource->morphTargetAssets)
							{
								fmt::print("{} : offset {} : size {}\n", morphTarget.name, morphTarget.offset, morphTarget.size);
							}
						}
					}
				}

				FK_LOG_9("Done Loading Needed Morph Targets Updates");
			});
	}


	/************************************************************************************************/


	const MorphTargets&	BuildMorphTargets(
			FrameGraph&						frameGraph,
			UpdateDispatcher&				dispatcher,
			GatherPassesTask&				passes,
			UpdateTask&						loadedMorphs,
			ReserveConstantBufferFunction&	reserveConstants,
			PoolAllocatorInterface&			pool,
			iAllocator&						allocator)
	{
		PassDrivenResourceAllocation desc {
			.getPass			= [&passes] { return passes.GetData().GetPass(MorphTargetID); },
			.initializeResources =
				[](std::span<const PVEntry> brushes, std::span<const FrameResourceHandle> handles, auto& transferContext, iAllocator& allocator)
				{
					auto itr = handles.begin();

					for (auto&& [sortID, brush, gameObject, occlusionID, submissionID, LODlevel] : brushes)
					{
						auto poseState	= GetPoseState(*gameObject);
						auto skeleton	= poseState->Sk;

						transferContext.CreateZeroedResource(*(itr++));
					}
				},

			.layout	= FlexKit::DeviceLayout::DeviceLayout_Common,
			.access	= FlexKit::DeviceAccessState::DASNonPixelShaderResource,
			.max	= 32,
			.pool	= &pool
		};

		auto& allocation = frameGraph.AllocateResourceSet(desc);

		return frameGraph.AddNode<MorphTargets>(
			{
				Vector<MorphTarget>{ allocator },
				allocation
			},
			[&](FrameGraphNodeBuilder& builder, MorphTargets& data)
			{
				builder.AddNodeDependency(allocation.node);
				builder.AddDataDependency(loadedMorphs);
			},
			[&](MorphTargets& data, ResourceHandler& resources, Context& ctx, iAllocator& localAllocator)
			{

			});
	}

} // namespace FlexKit;

/**********************************************************************

Copyright (c) 2015 - 2022 Robert May

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
