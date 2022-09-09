#include "AnimationRendering.h"
#include "graphics.h"

namespace FlexKit
{
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
