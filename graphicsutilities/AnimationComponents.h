/**********************************************************************

Copyright (c) 2015 - 2019 Robert May

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

#ifndef ANIMATIONCOMPONENTS_H_INCLUDED
#define ANIMATIONCOMPONENTS_H_INCLUDED

#include "AnimationUtilities.h"
#include "AnimationRuntimeUtilities.h"

#include "..\coreutilities\Transforms.h"
#include "..\coreutilities\Components.h"

namespace FlexKit
{	/************************************************************************************************/


	constexpr ComponentID AnimatorComponentID = GetTypeGUID(Animator);
	using AnimatorHandle = Handle_t<32, AnimatorComponentID>;

	class AnimatorComponent : FlexKit::Component<AnimatorComponent, AnimatorHandle, AnimatorComponentID>
	{
	public:
		AnimatorComponent(iAllocator* allocator) : 
			animators{ allocator } {}

		struct AnimatorState
		{
			AnimationStateMachine ASM;
		};

		Vector<AnimatorState>							animators;
		HandleUtilities::HandleTable<SkeletonHandle>	handles;
	};


	/************************************************************************************************/


	constexpr ComponentID SkeletonComponentID = GetTypeGUID(Skeleton);
	using SkeletonHandle = Handle_t<32, SkeletonComponentID>;

	class SkeletonComponent : FlexKit::Component<SkeletonComponent, SkeletonHandle, SkeletonComponentID>
	{
	public:
		SkeletonComponent(iAllocator* allocator) : 
			skeletons	{ allocator },
			handles		{ allocator	} {}

		struct SkeletonState
		{
			Skeleton*	skeleton;
			PoseState	poseState;
		};

		Vector<SkeletonState>							skeletons;
		HandleUtilities::HandleTable<SkeletonHandle>	handles;
	};


	/************************************************************************************************/


	constexpr ComponentID BindPointComponentID = GetTypeGUID(BindPoint);
	using BindPointHandle = Handle_t<32, BindPoint>;

	class BindPointComponent : FlexKit::Component<BindPointComponent, BindPointHandle, AnimatorComponentID>
	{
	public:
		BindPointComponent(iAllocator* allocator) : bindPoints{ allocator } {}

		struct BindPoint
		{
			SkeletonHandle	skeleton;
			NodeHandle		node;
		};

		Vector<BindPoint>								bindPoints;
		HandleUtilities::HandleTable<SkeletonHandle>	handles;
	};


}	/************************************************************************************************/
#endif

