#include "AnimationUtilities.h"
#include "graphics.h"
#include "XMMathConversion.h"

namespace FlexKit
{   /************************************************************************************************/


	float4x4 GetPoseTransform(JointPose P)
	{
		const auto rotation		= Quaternion2Matrix(P.r);
		const auto scaling		= ScaleMatrix({ P.ts[3], P.ts[3], P.ts[3] });
		const auto translation	= TranslationMatrix(P.ts.xyz());

		return translation * (scaling * rotation);
	}


	JointPose GetPose(const float4x4& M)
	{
		auto q = FlexKit::MatrixToQuat(M);
		auto t = FlexKit::ExtractTranslationVector(M);

		return { q, float4(t, 1.0f) };
	}


	/************************************************************************************************/


	Joint&	Skeleton::operator [] (JointHandle hndl){ return Joints[hndl]; }


	/************************************************************************************************/


	JointHandle Skeleton::AddJoint(Joint J, const float4x4& I)
	{
		Joints		[JointCount] = J;
		IPose		[JointCount] = I;

		const auto parentI = GetInversePose(J.mParent);

		const auto localTransform   = FlexKit::Inverse(I) * parentI;
		const auto jointPose        = GetPose(localTransform);

		JointPoses[JointCount] = jointPose;

		return JointHandle{ JointCount++ };
	}
	

	/************************************************************************************************/


	void Skeleton::AddAnimationClip(AnimationClip clip, iAllocator* allocator)
	{
		auto& AL	= allocator->allocate_aligned<Skeleton::AnimationList>();
		AL.Next		= nullptr;
		AL.Clip		= clip;
		AL.Memory	= allocator;

		auto I		= &Animations;

		while (*I != nullptr)
			I = &(*I)->Next;

		(*I) = &AL;
	}


	/************************************************************************************************/


	float4x4 Skeleton::GetInversePose(const JointHandle H) const
	{
		return H != InvalidHandle ? IPose[H] : float4x4::Identity();
	}


	/************************************************************************************************/


	PoseState::Pose& PoseState::CreateSubPose(uint32_t ID, iAllocator& allocator)
	{
		Pose newSubPose;
		newSubPose.jointPose    = (JointPose*)allocator.malloc(sizeof(JointPose) * JointCount);
		newSubPose.sk           = Sk;
		newSubPose.poseID       = ID;

		for (size_t I = 0; I < JointCount; I++)
			newSubPose.jointPose[I] = JointPose{Quaternion{ 0, 0, 0, 1 }, float4(0, 0, 0, 1)};

		poses.emplace_back(newSubPose);

		return poses.back();
	}


	/************************************************************************************************/


	PoseState::Pose* PoseState::FindPose(uint32_t poseID)
	{
		auto res = std::find_if(
			poses.begin(), poses.end(),
			[&](auto e) { return e.poseID == poseID; });

		return res != poses.end() ? res : nullptr;
	}


	/************************************************************************************************/


	Skeleton::Skeleton(iAllocator* allocator, size_t jointCount)
	{
		InitiateSkeleton(allocator, jointCount);
	}


	/************************************************************************************************/


	void Skeleton::InitiateSkeleton(iAllocator* Allocator, size_t JC)
	{
		auto test = sizeof(Skeleton);
		Animations	= nullptr;
		JointCount	= 0;
		Joints		= (Joint*)		Allocator->_aligned_malloc(sizeof(Joint)	 * JC, 0x40);
		IPose		= (float4x4*)	Allocator->_aligned_malloc(sizeof(float4x4)  * JC, 0x40); // Inverse Global Space Pose
		JointPoses	= (JointPose*)	Allocator->_aligned_malloc(sizeof(JointPose) * JC, 0x40); // Local Space Pose
		Memory		= Allocator;
		FK_ASSERT(Joints);

		for (auto I = 0; I < JointCount; I++)
		{
			IPose[I]			= float4x4::Identity();
			Joints[I].mID		= nullptr;
			Joints[I].mParent	= JointHandle();
		}
	}


	/************************************************************************************************/


	PoseState CreatePoseState(Skeleton& skeleton, iAllocator* allocator)
	{
		using DirectX::XMMATRIX;
		using DirectX::XMMatrixIdentity;

		size_t JointCount = skeleton.JointCount;

		PoseState pose;
		pose.Joints		    = (JointPose*)	allocator->_aligned_malloc(sizeof(JointPose) * JointCount, 0x40);
		pose.CurrentPose	= (float4x4*)	allocator->_aligned_malloc(sizeof(float4x4) * JointCount, 0x40);
		pose.JointCount	    = JointCount;
		pose.Sk			    = &skeleton;
		pose.poses          = { allocator };

		for (size_t I = 0; I < JointCount; ++I)
			pose.Joints[I] = JointPose(Quaternion{ 0, 0, 0, 1 },float4{0, 0, 0, 1});

		for (size_t I = 0; I < pose.JointCount; ++I)
		{
			const auto skeletonT    = GetPoseTransform(skeleton.JointPoses[I]);
			const auto parent       = skeleton.Joints[I].mParent;
			const auto P            = (parent != InvalidHandle) ? pose.CurrentPose[parent] : float4x4::Identity();

			pose.CurrentPose[I]     = P * skeletonT;
		}

		return pose;
	}


	/************************************************************************************************/


	PoseState Skeleton::CreatePoseState(iAllocator& allocator)
	{
		return FlexKit::CreatePoseState(*this, &allocator);
	}


	/************************************************************************************************/


	ResourceHandle LoadMorphTarget(TriMesh* triMesh, const char* morphTargetName, RenderSystem& renderSystem, CopyContextHandle handle, iAllocator& allocator)
	{
		auto readCtx = OpenReadContext(triMesh->assetHandle);

		if (!readCtx)
			return InvalidHandle;

		for (auto& morphTarget : triMesh->morphTargetAssets)
		{
			if (!strcmp(morphTarget.name, morphTargetName))
			{
				auto* buffer = allocator._aligned_malloc(morphTarget.size);
				readCtx.Read(buffer, morphTarget.size, morphTarget.offset);

				return MoveBufferToDevice(renderSystem, (const char*)buffer, morphTarget.size, handle);
			}
		}

		return InvalidHandle;
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

		for (auto I = S->Animations; I != nullptr; I = I->Next) 
			I->Memory->_aligned_free(I);
	}


	/************************************************************************************************/


	JointHandle Skeleton::FindJoint(const char* ID)
	{
		for (size_t I = 0; I < JointCount; ++I)
		{
			if (!strcmp(Joints[I].mID, ID))
				return JointHandle{ I };
		}

		return InvalidHandle;
	}


}	/************************************************************************************************/


/**********************************************************************

Copyright (c) 2015 - 2021 Robert May

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

