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

#include "..\PCH.h"
#include "..\buildsettings.h"
#include "AnimationUtilities.h"

namespace FlexKit
{
	/************************************************************************************************/

	Joint&	Skeleton::operator [] (JointHandle hndl){ return Joints[hndl]; }

	/************************************************************************************************/


	JointHandle Skeleton::AddJoint(Joint J, XMMATRIX& I)
	{
		Joints		[JointCount] = J;
		IPose		[JointCount] = I;

		auto Temp = GetInversePose(J.mParent) * DirectX::XMMatrixInverse(nullptr, I);
		auto Pose = GetPose(Temp);
		JointPoses[JointCount] = Pose;

		return JointCount++;
	}


	/************************************************************************************************/


	XMMATRIX Skeleton::GetInversePose(JointHandle H)
	{
		return (H != 0XFFFF)? IPose[H] : DirectX::XMMatrixIdentity();
	}

	DrawablePoseState* CreatePoseState(Drawable* E, GeometryTable* GT, iAllocator* MEM)
	{
		using DirectX::XMMATRIX;
		using DirectX::XMMatrixIdentity;
		auto Mesh = GetMesh(GT, E->MeshHandle);

		if (!Mesh && !Mesh->Skeleton)
			return nullptr;

		size_t JointCount = Mesh->Skeleton->JointCount;

		auto New_EAS = (DrawablePoseState*)MEM->_aligned_malloc(sizeof(DrawablePoseState));
		New_EAS->Resource		= FrameBufferedResource();
		New_EAS->Joints			= (JointPose*)MEM->_aligned_malloc(sizeof(JointPose) * JointCount, 0x40);
		New_EAS->CurrentPose	= (XMMATRIX*)MEM->_aligned_malloc(sizeof(XMMATRIX) * JointCount, 0x40);
		New_EAS->JointCount		= JointCount;
		New_EAS->Sk				= Mesh->Skeleton;
		New_EAS->Dirty			= true; // Forces First Upload

		auto S = New_EAS->Sk;
		
		for (size_t I = 0; I < JointCount; ++I)
			New_EAS->Joints[I] = JointPose(Quaternion{ 0, 0, 0, 1 },float4{0, 0, 0, 1});
		
		for (size_t I = 0; I < New_EAS->JointCount; ++I) 
		{
			auto P = (S->Joints[I].mParent != 0xFFFF) ? New_EAS->CurrentPose[S->Joints[I].mParent] : XMMatrixIdentity();
			New_EAS->CurrentPose[I] = New_EAS->CurrentPose[I] * P;
		}

		return New_EAS;
	}


	/************************************************************************************************/


	bool InitiatePoseState(RenderSystem* RS, DrawablePoseState* EAS, PoseState_DESC& Desc, VShaderJoint* InitialState)
	{
		bool ReturnState = false;
		size_t ResourceSize = Desc.JointCount * sizeof(VShaderJoint) * 2;
		ShaderResourceBuffer NewResource = CreateShaderResource(RS, ResourceSize);
		NewResource._SetDebugName("POSESTATE");
		
		if (NewResource)
		{
			ReturnState = true;

			UpdateResourceByTemp(RS, &NewResource, InitialState, ResourceSize, 1, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			EAS->Resource = NewResource;
		}
		else
			NewResource.Release();

		return (ReturnState);
	}


	/************************************************************************************************/


	void Destroy(DrawablePoseState* EPS)
	{
		EPS->Resource.Release();
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


	/************************************************************************************************/


	EPLAY_ANIMATION_RES PlayAnimation(FlexKit::Drawable* E, GeometryTable* GT, const char* Animation, iAllocator* MEM, bool ForceLoop, float Weight)
	{
		using FlexKit::DrawablePoseState;
		if (!E || !(E->MeshHandle != INVALIDMESHHANDLE))
			return EPLAY_ANIMATION_RES::EPLAY_INVALID_PARAM;

		auto MeshHandle = E->MeshHandle;

		if (!IsSkeletonLoaded(GT, MeshHandle))
			return EPLAY_ANIMATION_RES::EPLAY_NOT_ANIMATABLE;

		auto Mesh = GetMesh(GT, MeshHandle);

		if (!E->PoseState)
		{
			Skeleton*	S		= (Skeleton*)Mesh->Skeleton;
			auto NewPoseState	= CreatePoseState(E, GT, MEM);

			if (!NewPoseState)
				return EPLAY_NOT_ANIMATABLE;

			E->PoseState = NewPoseState;
		}// Create Animation State

		if (!E->AnimationState)
		{
			auto New_EAS = (DrawableAnimationState*)MEM->_aligned_malloc(sizeof(DrawableAnimationState));
			E->AnimationState	= New_EAS;
			New_EAS->Clips		={};
		}

		auto EPS	= E->PoseState;
		auto EAS	= E->AnimationState;
		auto S		= Mesh->Skeleton;

		Skeleton::AnimationList* I = S->Animations;

		while (I)
		{
			if (!strcmp(I->Clip.mID, Animation))
			{
				DrawableAnimationState::AnimationStateEntry ASE;
				ASE.Clip	  = &I->Clip;
				ASE.T		  = 0;
				ASE.Playing	  = true;
				ASE.Speed	  = 1.0f;
				ASE.ForceLoop = ForceLoop;
				ASE.Weight	  = Weight;
				EAS->Clips.push_back(ASE);
				
				E->Posed = true; // Enable Posing or Animation won't do anything
				return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
			}
			I = I->Next;
		}

		return EPLAY_ANIMATION_RES::EPLAY_ANIMATION_NOT_FOUND;
	}


	/************************************************************************************************/


	EPLAY_ANIMATION_RES SetAnimationSpeed(DrawableAnimationState* AE, const char* AnimationID, double Speed)
	{
		using FlexKit::DrawablePoseState;

		for(auto& C : AE->Clips)
		{
			if (!strcmp(C.Clip->mID, AnimationID))
			{
				C.Speed = Speed;
				return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
			}
		}
		return EPLAY_ANIMATION_RES::EPLAY_ANIMATION_NOT_FOUND;
	}


	/************************************************************************************************/


	EPLAY_ANIMATION_RES StopAnimation(FlexKit::Drawable* E, GeometryTable* GT, const char* Animation)
	{
		if (!E)
			return EPLAY_ANIMATION_RES::EPLAY_INVALID_PARAM;


		if (E->MeshHandle == INVALIDMESHHANDLE || !IsSkeletonLoaded(GT, E->MeshHandle))
			return EPLAY_ANIMATION_RES::EPLAY_NOT_ANIMATABLE;

		if (!E->Posed)
		{
			return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
		}

		auto Mesh	= GetMesh(GT, E->MeshHandle);
		auto EAS	= E->AnimationState;
		auto S		= Mesh->Skeleton;
		
		for (auto& C : EAS->Clips)
		{
			if (!strcmp(C.Clip->mID, Animation)) {
				C.Playing = false;
				return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
			}
		}
		return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
	}


	/************************************************************************************************/


	void UploadPose(RenderSystem* RS, GeometryTable* GT, FlexKit::Drawable* E, iAllocator* TEMP)
	{
		using DirectX::XMMATRIX;
		using DirectX::XMMatrixIdentity;
		using DirectX::XMMatrixInverse;

		auto Mesh	= GetMesh(GT, E->MeshHandle);
		auto PS		= E->PoseState;
		auto S		= Mesh->Skeleton;

		VShaderJoint* Out = (VShaderJoint*)TEMP->_aligned_malloc(sizeof(VShaderJoint) * S->JointCount);

		PS->Dirty = false;

		for (size_t I = 0; I < S->JointCount; ++I)
			Out[I] = { S->IPose[I], PS->CurrentPose[I] };

		if (!PS->Resource)
		{	// Create Animation State with Data as Initial
			FlexKit::PoseState_DESC	Desc = { S->JointCount };
			auto RES = InitiatePoseState(RS, PS, Desc, Out);
		}
		else
		{
			UpdateResourceByTemp(RS, &PS->Resource, Out, sizeof(VShaderJoint) * S->JointCount, 1, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}
	}


	/************************************************************************************************/


	void UploadPoses(RenderSystem* RS, PVS* Drawables, GeometryTable* GT, iAllocator* TEMP)
	{
		for (Drawable* d : *Drawables)
		{
			if(d->PoseState && d->PoseState->Dirty)
				UploadPose(RS, GT, d, TEMP);
		}
	}


	/************************************************************************************************/

	#if 0
	Quaternion Slerp(Quaternion P, Quaternion Q, float W)
	{
		Quaternion Qout;
		
		float dot = P.V().dot(Q.V());
		if (dot < 0)
		{
			dot = -dot;
		}else Qout = Q;

		if (dot < 0.95f)
		{
			float Angle = acosf(dot);
			return (P * 
				sinf(Angle * (1 - W)) + 
				Qout * sinf(Angle * W) / sinf(Angle));
		}

		return Qout;
	}
	#endif

	Quaternion Qlerp(Quaternion P, Quaternion Q, float W)
	{
		float W_Inverse = 1 - W;
		Quaternion Qout = P * W_Inverse + Q * W;
		Qout.normalize();

		return Qout;
	}


	void UpdateAnimation(RenderSystem* RS, FlexKit::Drawable* E, GeometryTable* GT, double dT, iAllocator* TEMP)
	{
		using DirectX::XMMATRIX;
		using DirectX::XMMatrixIdentity;
		using DirectX::XMMatrixInverse;

		if (E && E->Posed)
		{
			if (!E->PoseState || !E->AnimationState) {
				E->Posed = false;
				return;
			}

			auto PS		= E->PoseState;
			auto AS		= E->AnimationState;
			auto Mesh	= GetMesh(GT, E->MeshHandle);
			auto S		= Mesh->Skeleton;

			float4x4* M = (float4x4*)TEMP->_aligned_malloc(S->JointCount * sizeof(float4x4));

			for (size_t I = 0; I < S->JointCount; ++I) {
				auto Pose		= S->JointPoses[I];
				auto JointPose	= PS->Joints[I];
				
				auto Scale  = Pose.ts.w   * JointPose.ts.w;
				Pose.r		= JointPose.r * Pose.r;
				Pose.ts	    = Pose.ts + JointPose.ts;
				Pose.ts.w	= Scale;

				M[I] = XMMatrixToFloat4x4(&GetTransform(&Pose));
			}


			bool AnimationPlayed = false;
			for (auto& C : AS->Clips)
			{
				if (C.Playing && C.Clip->FrameCount)
				{
					const AnimationClip* Clip	= C.Clip;
					bool Loop					= Clip->isLooping;
					float  Weight				= C.Weight;
					double T					= C.T;
					double FrameDuration		= (1.0f / Clip->FPS) / C.Speed;
					double AnimationDuration	= FrameDuration * Clip->FrameCount;
					size_t FrameIndex			= size_t(T / FrameDuration) % Clip->FrameCount;
					auto& CurrentFrame			= Clip->Frames[FrameIndex];

					if (!(Loop | C.ForceLoop) && C.T > AnimationDuration)
						C.Playing = false;
					else if (C.T > AnimationDuration)
						C.T = 0.0f;
					else
						C.T += dT;

					for (size_t I = 0; I < CurrentFrame.JointCount; ++I){
						auto Pose	= CurrentFrame.Poses[I];
						//M[I] = XMMatrixToFloat4x4(E->PoseState->Joints + I) * M[I];
					}
					
					AnimationPlayed = true;
				}
			}


			if(AnimationPlayed)
			{
				for (size_t I = 0; I < S->JointCount; ++I)
				{
					auto P = (S->Joints[I].mParent != 0xFFFF) ? M[S->Joints[I].mParent] : float4x4::Identity();
					M[I] = P * M[I];
				}

				for (size_t I = 0; I < PS->JointCount; ++I)
					PS->CurrentPose[I] = Float4x4ToXMMATIRX(M + I);

				PS->Dirty = true;
			}
		}
	}


	/************************************************************************************************/


	DirectX::XMMATRIX GetTransform(JointPose* P)
	{
		return	DirectX::XMMatrixRotationQuaternion(P->r) *
			DirectX::XMMatrixScalingFromVector(DirectX::XMVectorSet(P->ts[3], P->ts[3], P->ts[3], 0.0f)) *
			DirectX::XMMatrixTranslationFromVector(DirectX::XMVectorSet(P->ts[0], P->ts[1], P->ts[2], 0.0f));
	}


	JointPose GetPose(DirectX::XMMATRIX& M)
	{
		auto Q = DirectX::XMQuaternionRotationMatrix(M);
		auto P = M.r[3];

		return{ Q, float4(P, 1) };
	}


	/************************************************************************************************/


	void PrintChildren(FlexKit::Skeleton* S, JointHandle J, size_t Depth = 0)
	{
		for (size_t II = 0; II < S->JointCount; II++)
		if (S->Joints[II].mParent == J)
		{
			for (size_t I = 0; I < Depth; I++)
				std::cout << "    ";

			std::cout << S->Joints[II].mID << " -- Children: \n";
			PrintChildren(S, II, Depth + 1);
		}
	}


	void DEBUG_PrintSkeletonHierarchy(FlexKit::Skeleton* S)
	{
		for (size_t I = 0; I < S->JointCount; I++)
		{
			if (S->Joints[I].mParent == 0XFFFF)
			{
				std::cout << S->Joints[I].mID << " -- Children: \n";
				PrintChildren(S, I, 1);
			}
		}
	}


	/************************************************************************************************/


	void DEBUG_DrawSkeleton(Skeleton* S, SceneNodes* Nodes, NodeHandle Node, iAllocator* TEMP, Line3DPass* Out)
	{
		float4 Zero(0.0f, 0.0f, 0.0f, 1.0f);
		float4x4 WT; GetWT(Nodes, Node, &WT);

		float4x4* M = (float4x4*)TEMP->_aligned_malloc(S->JointCount * sizeof(float4x4));

		for (size_t I = 0; I < S->JointCount; ++I)
		{
			float3 A, B;
			
			float4x4 PT;
			if (S->Joints[I].mParent != 0XFFFF)
				PT = M[S->Joints[I].mParent];
			else
				PT = float4x4::Identity();

			auto J  = S->JointPoses[I];
			auto JT = PT * XMMatrixToFloat4x4(&GetTransform(&J));

			A = (WT * (JT * Zero)).xyz();
			B = (WT * (PT * Zero)).xyz();
			M[I] = JT;

			AddLineSegment(Out, {A,B});
		}
	}


	/************************************************************************************************/


	void DEBUG_DrawPoseState(Skeleton* S, DrawablePoseState* DPS, SceneNodes* Nodes, NodeHandle Node, Line3DPass* Out)
	{
		if (!S || !DPS)
			return;

		float4 Zero(0.0f, 0.0f, 0.0f, 1.0f);
		float4x4 WT; GetWT(Nodes, Node, &WT);

		for (size_t I = 0; I < S->JointCount; ++I)
		{
			float3 A, B;

			float4x4 PT;
			if (S->Joints[I].mParent != 0XFFFF)
				PT = XMMatrixToFloat4x4(DPS->CurrentPose + S->Joints[I].mParent);
			else
				PT = float4x4::Identity();

			auto JT = XMMatrixToFloat4x4(DPS->CurrentPose + I);

			A = (WT * (JT * Zero)).xyz();
			B = (WT * (PT * Zero)).xyz();

			AddLineSegment(Out, { A,B });
		}
	}


}	/************************************************************************************************/