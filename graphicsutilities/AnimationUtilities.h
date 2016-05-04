/**********************************************************************

Copyright (c) 2015 - 2016 Robert May

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


#ifndef ANIMATION_UTILITIES_H
#define ANIMATION_UTILITIES_H

#include "..\PCH.h"
#include "..\\coreutilities\memoryutilities.h"
#include "..\\coreutilities\MathUtils.h"

#include "graphics.h"

#include <DirectXMath.h>
#include <functional>


namespace FlexKit
{
	using DirectX::XMMATRIX;

	/************************************************************************************************/

	typedef uint16_t	JointHandle;
	struct Skeleton;

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
		Quaternion	r;
		float4		ts;
	};


	struct JointAndPose
	{
		Joint		Linkage;
		//JointPose	Pose;
	};

	FLEXKITAPI XMMATRIX		GetTransform(JointPose* P);
	FLEXKITAPI JointPose	GetPose(DirectX::XMMATRIX& M);

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
		JointHandle		FindJoint( const char*);

		DirectX::XMMATRIX*	IPose; // Global Inverse Space Pose
		Joint*				Joints;
		JointPose*			JointPoses;
		size_t				JointCount;

		//JointHandle			Root;

		struct AnimationList
		{
			AnimationClip	Clip;
			AnimationList*	Next;
		};

		AnimationList* Animations;
	};

	FLEXKITAPI void Skeleton_PushAnimation(Skeleton* S, iAllocator* Allocator, AnimationClip AC);


	/************************************************************************************************/


	struct FLEXKITAPI SkeletonPose
	{
		Skeleton*	S			= nullptr;
		JointPose*	JointPoses	= nullptr;
		char*		mID			= nullptr;

		void ClearPose()
		{
			for (auto I = 0; I < S->JointCount; I++)
			{
				JointPoses[I].ts ={ 0, 0, 0, 0 };
				JointPoses[I].r = DirectX::XMQuaternionIdentity();
			}
		}

		void InitiatePose(iAllocator* Allocator, Skeleton* s)
		{
			JointPoses = (JointPose*)Allocator->_aligned_malloc(sizeof(JointPose) * s->JointCount, 16);
			S = s;

			ClearPose();
		}
	};


	/************************************************************************************************/


	struct DrawableAnimationState
	{
		struct AnimationStateEntry
		{
			AnimationClip*	Clip;
			double			T;
			double			Speed;
			bool			Playing;
			bool			ForceLoop;
			char			_p[6];
		};

		static_vector<AnimationStateEntry> Clips; // Animations in
	};

	struct PoseState_DESC
	{
		size_t JointCount;
	};

	struct DrawablePoseState
	{
		ID3D12Resource*		Resource	= nullptr;
		DirectX::XMMATRIX*	Joints		= nullptr;
		DirectX::XMMATRIX*	CurrentPose	= nullptr;
		Skeleton*			Sk			= nullptr;
		size_t				JointCount	= 0;
		size_t				Dirty		= 0;
		size_t				padding[2];
	};

	inline void RotateJoint(DrawablePoseState* E, JointHandle J, Quaternion Q)
	{
		if (!E)
			return;
		if (J != -1) {
			E->Joints[J] = DirectX::XMMatrixRotationQuaternion(Q) * E->Joints[J];
			E->Dirty = true;
		}
	}

	inline void TranslateJoint(DrawablePoseState* E, JointHandle J, float3 xyz)
	{
		if (!E)
			return;
		if (J != -1) {
			E->Joints[J] = E->Joints[J] * DirectX::XMMatrixTranslation(xyz.x, xyz.y, xyz.z);
			E->Dirty = true;
		}
	}

	inline void SetJointTranslation(DrawablePoseState* E, JointHandle J, float3 xyz)
	{
		if (!E)
			return;
		if (J != -1) {
			E->Joints[J].r[3] = float4( xyz.x, xyz.y, xyz.z, 1);
			E->Dirty = true;
		}
	}

	inline void ScaleJoint(DrawablePoseState* E, JointHandle J, float S)
	{
		if (!E)
			return;
		if (J!= -1)
			E->Joints[J] = E->Joints[J] * DirectX::XMMatrixScaling(S, S, S);
	}

	/************************************************************************************************/

	FLEXKITAPI DrawablePoseState*	CreatePoseState(Drawable* E, iAllocator* MEM);
	FLEXKITAPI bool					InitiatePoseState(RenderSystem* RS, DrawablePoseState* EAS, PoseState_DESC& Desc, VShaderJoint* Initial);

	FLEXKITAPI void					Destroy(DrawablePoseState* EAS);

	/************************************************************************************************/

	enum EPLAY_ANIMATION_RES
	{
		EPLAY_NOT_ANIMATABLE,
		EPLAY_ANIMATION_NOT_FOUND,
		EPLAY_INVALID_PARAM,
		EPLAY_SUCCESS
	};

	FLEXKITAPI EPLAY_ANIMATION_RES PlayAnimation	(FlexKit::Drawable* E, const char* AnimationID, iAllocator* MEM, bool ForceLoop = false);
	FLEXKITAPI EPLAY_ANIMATION_RES SetAnimationSpeed(FlexKit::Drawable* E, const char* AnimationID, double Speed = false);
	FLEXKITAPI EPLAY_ANIMATION_RES StopAnimation	(FlexKit::Drawable* E, const char* AnimationID);

	FLEXKITAPI void UpdateAnimation(RenderSystem* RS, FlexKit::Drawable* E, double dT, iAllocator* TEMP);

	FLEXKITAPI void PrintSkeletonHierarchy(FlexKit::Skeleton* S);
}
#endif
