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

#ifndef ANIMATIONUTILITIES_H_INCLUDED
#define ANIMATIONUTILITIES_H_INCLUDED

#include "..\buildsettings.h"
#include <DirectXMath.h>


namespace FlexKit
{

	using DirectX::XMMATRIX;

	/************************************************************************************************/

	typedef uint16_t	JointHandle;

	// PreDeclarations
	struct	Drawable;
	struct	Skeleton;

	struct Joint
	{
		const char*			mID				= nullptr;
		JointHandle			mParent			= JointHandle(0);
		char				mPad[6]			={};
	};


	/************************************************************************************************/

	
	struct VShaderJoint
	{
		XMMATRIX I;
		XMMATRIX T;
	};

	struct JointPose
	{
		JointPose() {}
		JointPose(Quaternion Q, float4 TS) : r(Q), ts(TS) {}

		Quaternion	r;
		float4		ts;
	};


	FLEXKITAPI XMMATRIX		GetPoseTransform(JointPose P);
	FLEXKITAPI JointPose	GetPose(DirectX::XMMATRIX M);


	/************************************************************************************************/


	struct AnimationClip
	{
		struct KeyFrame
		{
			JointHandle*	Joints		= nullptr;
			JointPose*		Poses		= nullptr;
			size_t			JointCount	= 0;
		};

		uint32_t		FPS				= 0;
		Skeleton*		Skeleton		= nullptr;
		size_t			FrameCount		= 0;
		KeyFrame*		Frames			= 0;
		size_t			guid			= 0;
		char*			mID				= nullptr;
		bool			isLooping		= false;
	};


	/************************************************************************************************/


	struct FLEXKITAPI Skeleton
	{
		Joint&			operator [] (JointHandle hndl);

		JointHandle		AddJoint(Joint J, XMMATRIX& I);
		void			InitiateSkeleton(iAllocator* Allocator, size_t jointcount = 64);
		XMMATRIX		GetInversePose(JointHandle H);
		JointHandle		FindJoint(const char*);

		DirectX::XMMATRIX*	IPose; // Global Inverse Space Pose
		Joint*				Joints;
		JointPose*			JointPoses;
		size_t				JointCount;
		GUID_t				guid;
		iAllocator*			Memory;

		struct AnimationList
		{
			AnimationClip	Clip;
			AnimationList*	Next;
			iAllocator*		Memory;
		};

		AnimationList* Animations;
	};


	FLEXKITAPI void Skeleton_PushAnimation(Skeleton* S, iAllocator* Allocator, AnimationClip AC);

}

#endif
