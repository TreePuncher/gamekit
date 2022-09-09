#pragma once
#include "AnimationComponents.h"
#include "ClusteredRendering.h"
#include "FrameGraph.h"
#include "Scene.h"
#include "ThreadUtilities.h"

namespace FlexKit
{
	struct BrushPoseBlock
	{
		float4x4 transforms[8];

		auto& operator [](size_t idx)
		{
			return transforms[idx];
		}
	};

	inline auto CreateGetFN(iAllocator& allocator)
	{
		return MakeLazyObject<CBPushBuffer>(
			&allocator,
			[&](ReserveConstantBufferFunction& reserve, GatherPassesTask& passesGather) mutable -> CBPushBuffer
			{
				auto& passes    = passesGather.GetData().passes;
				auto res        = FindPass(passes.begin(), passes.end(), GBufferAnimatedPassID);

				size_t requiredBlocks = 0;
				for (auto& skinnedBrush : res->pvs)
				{
					auto pose = GetPoseState(*skinnedBrush.gameObject);
					requiredBlocks += (size_t)ceil((float)pose->JointCount / 8.0f);
				}

				if (res)
					return reserve(requiredBlocks * sizeof(BrushPoseBlock));
				else
					return {};
			});
	}

	struct AnimationPoseUpload
	{
		AnimationPoseUpload(
			ReserveConstantBufferFunction&  IN_reserve,
			GatherPassesTask&               IN_passes,
			iAllocator&                     IN_allocator) :
				GetBuffer   { CreateGetFN(IN_allocator) },
				passes      { IN_passes },
				reserve     { IN_reserve } {}

		using CreateFN = decltype(CreateGetFN(std::declval<iAllocator&>()));
		using PoseView = decltype(CreateCBIterator<BrushPoseBlock>(std::declval<CBPushBuffer&>()));

		CreateFN                        GetBuffer;
		GatherPassesTask&               passes;
		ReserveConstantBufferFunction&  reserve;

		PoseView        GetIterator();
		CBPushBuffer&   GetData();
	};

	AnimationPoseUpload& UploadPoses(FrameGraph&, GatherPassesTask& passes, ReserveConstantBufferFunction& reserveCB, iAllocator& allocator);
}


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
