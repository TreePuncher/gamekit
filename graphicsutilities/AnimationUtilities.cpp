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


#include "../graphicsutilities/AnimationUtilities.h"

namespace FlexKit
{


	/************************************************************************************************/


	DirectX::XMMATRIX GetPoseTransform(JointPose P)
	{
		auto Rotation = DirectX::XMMatrixRotationQuaternion(P.r);
		auto Scaling = DirectX::XMMatrixScalingFromVector(DirectX::XMVectorSet(P.ts[3], P.ts[3], P.ts[3], 1.0f));
		auto Translation = DirectX::XMMatrixTranslationFromVector(DirectX::XMVectorSet(P.ts[0], P.ts[1], P.ts[2], 1.0f));

		return (Rotation * Scaling) * Translation;
	}


	JointPose GetPose(DirectX::XMMATRIX M)
	{
		auto Q = DirectX::XMQuaternionRotationMatrix(M);
		auto P = M.r[3];

		return{ Q, float4(P, 1) };
	}


	/************************************************************************************************/


	Joint&	Skeleton::operator [] (JointHandle hndl){ return Joints[hndl]; }


	/************************************************************************************************/


	JointHandle Skeleton::AddJoint(Joint J, XMMATRIX& I)
	{
		Joints		[JointCount] = J;
		IPose		[JointCount] = I;

		auto Temp = DirectX::XMMatrixInverse(nullptr, I) * GetInversePose(J.mParent);
		auto Pose = GetPose(Temp);
		JointPoses[JointCount] = Pose;

		return JointCount++;
	}


	/************************************************************************************************/


	XMMATRIX Skeleton::GetInversePose(JointHandle H)
	{
		return (H != 0XFFFF)? IPose[H] : DirectX::XMMatrixIdentity();
	}


	/************************************************************************************************/


	void Skeleton::InitiateSkeleton(iAllocator* Allocator, size_t JC)
	{
		auto test = sizeof(Skeleton);
		Animations	= nullptr;
		JointCount	= 0;
		Joints		= (Joint*)		Allocator->_aligned_malloc(sizeof(Joint)	 * JC, 0x40);
		IPose		= (XMMATRIX*)	Allocator->_aligned_malloc(sizeof(XMMATRIX)  * JC, 0x40); // Inverse Global Space Pose
		JointPoses	= (JointPose*)	Allocator->_aligned_malloc(sizeof(JointPose) * JC, 0x40); // Local Space Pose
		Memory		= Allocator;
		FK_ASSERT(Joints);

		for (auto I = 0; I < JointCount; I++)
		{
			IPose[I]			= DirectX::XMMatrixIdentity();
			Joints[I].mID		= nullptr;
			Joints[I].mParent	= JointHandle();
		}
	}


	/************************************************************************************************/


	void CleanUpSkeleton(Skeleton* S)
	{
		for (size_t I = 0; I < S->JointCount; ++I)
			if (S->Joints[I].mID) S->Memory->free((void*)S->Joints[I].mID);

		S->Memory->free((void*)S->Joints);
		S->Memory->free((void*)S->JointPoses);
		S->Memory->free((void*)S->IPose);
		S->Memory->free(S);

		auto I = S->Animations;
		while (I != nullptr) {
			//ReleaseSceneAnimation(&I->Clip, I->Memory);
			I->Memory->_aligned_free(I);
			I = I->Next;
		}
	}


	/************************************************************************************************/


	JointHandle Skeleton::FindJoint(const char* ID)
	{
		for (size_t I = 0; I < JointCount; ++I)
		{
			if (!strcmp(Joints[I].mID, ID))
				return I;
		}

		return -1;
	}

	/************************************************************************************************/


	void Skeleton_PushAnimation(Skeleton* S, iAllocator* Allocator, AnimationClip AC)
	{
		auto& AL	= Allocator->allocate_aligned<Skeleton::AnimationList>();
		AL.Next		= nullptr;
		AL.Clip		= AC;
		AL.Memory	= Allocator;

		auto I		= &S->Animations;

		while (*I != nullptr)
			I = &(*I)->Next;

		(*I) = &AL;
	}


}	/************************************************************************************************/