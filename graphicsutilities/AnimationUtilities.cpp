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


	using DirectX::XMMatrixRotationQuaternion;
	using DirectX::XMMatrixScalingFromVector;
	using DirectX::XMMatrixTranslationFromVector;


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

		auto I = S->Animations;
		while (I != nullptr) {
			CleanUpSceneAnimation(&I->Clip, I->Memory);
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


	/************************************************************************************************/
	

	Pair<EPLAY_ANIMATION_RES, bool> CheckState_PlayAnimation(Drawable* E, GeometryTable* GT, iAllocator* MEM)
	{
		using FlexKit::DrawablePoseState;
		if (!E || !(E->MeshHandle != INVALIDMESHHANDLE))
			return{ EPLAY_ANIMATION_RES::EPLAY_INVALID_PARAM, false };

		auto MeshHandle = E->MeshHandle;

		if (!IsSkeletonLoaded(GT, MeshHandle))
			return{ EPLAY_ANIMATION_RES::EPLAY_NOT_ANIMATABLE, false };

		auto Mesh = GetMesh(GT, MeshHandle);

		if (!E->PoseState)
		{
			Skeleton*	S		= (Skeleton*)Mesh->Skeleton;
			auto NewPoseState	= CreatePoseState(E, GT, MEM);

			if (!NewPoseState)
				return{ EPLAY_NOT_ANIMATABLE, false };

			E->PoseState = NewPoseState;
		}// Create Animation State

		if (!E->AnimationState)
		{
			auto New_EAS = (DrawableAnimationState*)MEM->_aligned_malloc(sizeof(DrawableAnimationState));
			E->AnimationState = New_EAS;
			New_EAS->Clips = {};
		}

		return{ EPLAY_ANIMATION_RES::EPLAY_SUCCESS, true };
	}


	/************************************************************************************************/


	EPLAY_ANIMATION_RES PlayAnimation(Drawable* E, GeometryTable* GT, GUID_t Guid, iAllocator* Allocator, bool ForceLoop, float Weight)
	{
		using FlexKit::DrawablePoseState;
		auto Res = CheckState_PlayAnimation(E, GT, Allocator);

		if (!Res)
			return Res;

		auto MeshHandle = E->MeshHandle;
		auto Mesh		= GetMesh(GT, MeshHandle);

		auto EPS = E->PoseState;
		auto EAS = E->AnimationState;
		auto S	 = Mesh->Skeleton;

		Skeleton::AnimationList* I = S->Animations;

		while (I)
		{
			if (I->Clip.guid == Guid)
			{
				DrawableAnimationState::AnimationStateEntry ASE;
				ASE.Clip      = &I->Clip;
				ASE.order     = EAS->Clips.size();
				ASE.T         = 0;
				ASE.Playing   = true;
				ASE.Speed     = 1.0f;
				ASE.ForceLoop = ForceLoop;
				ASE.Weight    = max(min(Weight, 1.0f), 0.0f);
				ASE.ID		  = chrono::high_resolution_clock::now().time_since_epoch().count();
				EAS->Clips.push_back(ASE);
				E->Posed = true; // Enable Posing or Animation won't do anything
				return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
			}
			I = I->Next;
		}

		return EPLAY_ANIMATION_RES::EPLAY_ANIMATION_NOT_FOUND;
	}


	/************************************************************************************************/


	EPLAY_ANIMATION_RES PlayAnimation(Drawable* E, GeometryTable* GT, const char* Animation, iAllocator* Allocator, bool ForceLoop, float Weight)
	{
		using FlexKit::DrawablePoseState;
		auto Res = CheckState_PlayAnimation(E, GT, Allocator);

		if (!Res)
			return Res;

		auto MeshHandle = E->MeshHandle;
		auto Mesh       = GetMesh(GT, MeshHandle);

		auto EPS        = E->PoseState;
		auto EAS        = E->AnimationState;
		auto S          = Mesh->Skeleton;

		Skeleton::AnimationList* I = S->Animations;

		while (I)
		{
			if (!strcmp(I->Clip.mID, Animation))
			{
				DrawableAnimationState::AnimationStateEntry ASE;
				ASE.Clip	  = &I->Clip;
				ASE.order	  = EAS->Clips.size();
				ASE.T		  = 0;
				ASE.Playing	  = true;
				ASE.Speed	  = 1.0f;
				ASE.ForceLoop = ForceLoop;
				ASE.Weight	  = max(min(Weight, 1.0f), 0.0f);
				ASE.ID		  = chrono::high_resolution_clock::now().time_since_epoch().count();
				EAS->Clips.push_back(ASE);
				
				E->Posed = true; // Enable Posing or Animation won't do anything
				return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
			}
			I = I->Next;
		}

		return EPLAY_ANIMATION_RES::EPLAY_ANIMATION_NOT_FOUND;
	}


	/************************************************************************************************/


	EPLAY_ANIMATION_RES PlayAnimation(Drawable* E, GeometryTable* GT, GUID_t Guid, iAllocator* Allocator, bool ForceLoop, float Weight, uint32_t& out)
	{
		using FlexKit::DrawablePoseState;
		auto Res = CheckState_PlayAnimation(E, GT, Allocator);

		if (!Res)
			return Res;

		auto MeshHandle = E->MeshHandle;
		auto Mesh       = GetMesh(GT, MeshHandle);

		auto EPS        = E->PoseState;
		auto EAS        = E->AnimationState;
		auto S          = Mesh->Skeleton;

		Skeleton::AnimationList* I = S->Animations;

		while (I)
		{
			if (I->Clip.guid == Guid)
			{
				DrawableAnimationState::AnimationStateEntry ASE;
				ASE.Clip	  = &I->Clip;
				ASE.order	  = EAS->Clips.size();
				ASE.T		  = 0;
				ASE.Playing	  = true;
				ASE.Speed	  = 1.0f;
				ASE.ForceLoop = ForceLoop;
				ASE.Weight	  = max(min(Weight, 1.0f), 0.0f);
				ASE.ID		  = chrono::high_resolution_clock::now().time_since_epoch().count();
				EAS->Clips.push_back(ASE);
				
				out = ASE.ID;
				E->Posed = true; // Enable Posing or Animation won't do anything
				return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
			}
			I = I->Next;
		}

		return EPLAY_ANIMATION_RES::EPLAY_ANIMATION_NOT_FOUND;
	}


	/************************************************************************************************/


	EPLAY_ANIMATION_RES SetAnimationSpeed(DrawableAnimationState* AE, uint32_t ID, double Speed)
	{
		using FlexKit::DrawablePoseState;

		for (auto& C : AE->Clips)
		{
			if (C.ID == ID)
			{
				C.Speed = Speed;
				return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
			}
		}

		return EPLAY_ANIMATION_RES::EPLAY_FAILED;
	}


	/************************************************************************************************/


	EPLAY_ANIMATION_RES SetAnimationSpeed(DrawableAnimationState* AE, GUID_t AnimationID, double Speed)
	{
		using FlexKit::DrawablePoseState;

		for (auto& C : AE->Clips)
		{
			if (C.Clip->guid == AnimationID)
			{
				C.Speed = Speed;
				return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
			}
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

	Pair<EPLAY_ANIMATION_RES, bool> StopAnimation_CheckState(Drawable* E, GeometryTable* GT)
	{
		if (!E)
			return { EPLAY_ANIMATION_RES::EPLAY_INVALID_PARAM, false };

		if (E->MeshHandle == INVALIDMESHHANDLE || !IsSkeletonLoaded(GT, E->MeshHandle))
			return { EPLAY_ANIMATION_RES::EPLAY_NOT_ANIMATABLE, false };

		if (!E->Posed)
			return { EPLAY_ANIMATION_RES::EPLAY_NOT_ANIMATABLE, false };

		return { EPLAY_ANIMATION_RES::EPLAY_SUCCESS, true };
	}


	EPLAY_ANIMATION_RES StopAnimation(Drawable* E, GeometryTable* GT, GUID_t Guid)
	{
		auto Status = StopAnimation_CheckState(E, GT);
		if (!Status)
			return Status;

		auto Mesh	= GetMesh(GT, E->MeshHandle);
		auto EAS	= E->AnimationState;
		
		for (auto& C : EAS->Clips)
		{
			if (C.Clip->guid == Guid) {
				C.Playing = false;
				return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
			}
		}
		return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
	}


	/************************************************************************************************/


	EPLAY_ANIMATION_RES StopAnimation(Drawable* E, GeometryTable* GT, uint32_t ID)
	{
		auto Status = StopAnimation_CheckState(E, GT);
		if (!Status)
			return Status;

		auto Mesh	= GetMesh(GT, E->MeshHandle);
		auto EAS	= E->AnimationState;

		for (auto& C : EAS->Clips)
		{
			if (C.ID == ID) {
				C.Playing = false;
				return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
			}
		}

		return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
	}


	/************************************************************************************************/


	EPLAY_ANIMATION_RES StopAnimation(Drawable* E, GeometryTable* GT, const char* Animation)
	{
		auto Status = StopAnimation_CheckState(E, GT);
		if (!Status)
			return Status;

		auto Mesh	= GetMesh(GT, E->MeshHandle);
		auto EAS	= E->AnimationState;

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


	void UploadPose(RenderSystem* RS, Drawable* E, GeometryTable* GT, iAllocator* TEMP)
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
				UploadPose(RS, d, GT,TEMP);
		}
	}


	/************************************************************************************************/


	EPLAY_ANIMATION_RES ClearAnimationPose(DrawablePoseState* DPS, iAllocator* TEMP)
	{
		using DirectX::XMMatrixIdentity;
		auto S = DPS->Sk;

		XMMATRIX* M = (XMMATRIX*)TEMP->_aligned_malloc(S->JointCount * sizeof(float4x4));

		if (!M)
			return EPLAY_ANIMATION_RES::EPLAY_FAILED;

		for (size_t I = 0; I < S->JointCount; ++I) {
			auto BasePose	= S->JointPoses[I];
			auto JointPose	= DPS->Joints[I];

			M[I] = GetTransform(JointPose) * GetTransform(BasePose);
		}


		{
			for (size_t I = 0; I < S->JointCount; ++I) {
				auto P = (S->Joints[I].mParent != 0xFFFF) ? M[S->Joints[I].mParent] : XMMatrixIdentity();
				M[I] = M[I] * P;
			}

			for (size_t I = 0; I < DPS->JointCount; ++I)
				DPS->CurrentPose[I] = M[I];

			DPS->Dirty = true;
		}

		return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
	}


	/************************************************************************************************/


	size_t GetAnimationPlayingCount(Drawable* E)
	{
		size_t Count = 0;
		for (auto Clip : E->AnimationState->Clips)
			if (Clip.Playing)
				Count++;

		return Count;
	}


	/************************************************************************************************/


	void UpdateAnimation(RenderSystem* RS, Drawable* E, GeometryTable* GT, double dT, iAllocator* TEMP, bool AdvanceOnly)
	{
		using DirectX::XMMATRIX;
		using DirectX::XMMatrixIdentity;
		using DirectX::XMMatrixInverse;
		using DirectX::XMMatrixRotationQuaternion;
		using DirectX::XMMatrixScalingFromVector;
		using DirectX::XMMatrixTranslationFromVector;
		using DirectX::XMVectorSet;

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

			XMMATRIX* M = (XMMATRIX*)TEMP->_aligned_malloc(S->JointCount * sizeof(float4x4));

			for (size_t I = 0; I < S->JointCount; ++I) {
				auto BasePose	= S->JointPoses[I];
				auto JointPose	= PS->Joints[I];
				
				M[I] = GetTransform(JointPose) * GetTransform(BasePose);
			}

			bool AnimationPlayed = false;
			std::sort(AS->Clips.begin().I, AS->Clips.end().I, 
				[](const auto& LHS, const auto& RHS) -> bool { 
					return	(LHS.Playing ? 0x00 : 1 << 31) || LHS.order <= (RHS.Playing ? 0x00 : 1 << 31) || RHS.order; });

			for (auto& C : AS->Clips)
			{
				if (C.Playing && C.Clip->FrameCount)
				{
					const AnimationClip* Clip	= C.Clip;
					bool Loop					= Clip->isLooping;
					float  Weight				= C.Weight;
					double T					= C.T;
					double FrameDuration		= (1.0f / Clip->FPS) / abs(C.Speed);
					double AnimationDuration	= FrameDuration * Clip->FrameCount;

					if (!(Loop | C.ForceLoop) && C.T > AnimationDuration)
						C.Playing = false;
					else if (C.T > AnimationDuration)
						C.T = 0.0f;
					else if (C.T < 0.0f)
						C.T = AnimationDuration;
					else
						C.T += dT * C.Speed;

					size_t FrameIndex	= size_t(T / FrameDuration) % Clip->FrameCount;
					auto& CurrentFrame	= Clip->Frames[FrameIndex];

					if(!AdvanceOnly){
						for (size_t I = 0; I < CurrentFrame.JointCount; ++I){
							auto JHndl	 = CurrentFrame.Joints[I];
							auto Pose	 = CurrentFrame.Poses[JHndl];
							// TODO: DECOMPRESS SAMPLE FROM A CURVE

							Pose.r  = Slerp	(Quaternion(0, 0, 0, 1),	Pose.r.normalize(), Weight);
							Pose.ts = Lerp	(float4(0, 0, 0, 1),		Pose.ts,			Weight);

							auto Rotation		= XMMatrixRotationQuaternion	(Pose.r);
							auto Scaling		= XMMatrixScalingFromVector		(XMVectorSet(Pose.ts[3], Pose.ts[3], Pose.ts[3], 1.0f));
							auto Translation	= XMMatrixTranslationFromVector	(XMVectorSet(Pose.ts[0], Pose.ts[1], Pose.ts[2], 1.0f));

							auto PoseT	= GetTransform(Pose);
							M[JHndl]	= PoseT * M[JHndl];
						}

						AnimationPlayed = true;
					}
				}
			}

			for (int I = AS->Clips.size() - 1; I >= 0; --I)
				if (!AS->Clips[I].Playing) 
					AS->Clips.pop_back();

			// Palette Generation
			if(AnimationPlayed)
			{
				for (size_t I = 0; I < S->JointCount; ++I){
					auto P = (S->Joints[I].mParent != 0xFFFF) ? M[S->Joints[I].mParent] : XMMatrixIdentity();
					M[I] = M[I] * P;
				}

				for (size_t I = 0; I < PS->JointCount; ++I)
					PS->CurrentPose[I] = M[I];

				PS->Dirty = true;
			}
		}
	}


	/************************************************************************************************/


	DirectX::XMMATRIX GetTransform(JointPose P)
	{
		auto Rotation		= DirectX::XMMatrixRotationQuaternion(P.r);
		auto Scaling		= DirectX::XMMatrixScalingFromVector(DirectX::XMVectorSet(P.ts[3], P.ts[3], P.ts[3], 1.0f));
		auto Translation	= DirectX::XMMatrixTranslationFromVector(DirectX::XMVectorSet(P.ts[0], P.ts[1], P.ts[2], 1.0f));

		return (Rotation * Scaling) * Translation;
	}


	JointPose GetPose(DirectX::XMMATRIX M)
	{
		auto Q = DirectX::XMQuaternionRotationMatrix(M);
		auto P = M.r[3];

		return{ Q, float4(P, 1) };
	}


	/************************************************************************************************/


	float4x4 GetJointPosed_WT(JointHandle Joint, NodeHandle Node, SceneNodes* Nodes, DrawablePoseState* DPS)
	{
		float4x4 WT; GetWT(Nodes, Node, &WT);
		float4x4 JT = XMMatrixToFloat4x4(DPS->CurrentPose + Joint);
		return	JT * WT;
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


	/************************************************************************************************/


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

		for (size_t I = 1; I < S->JointCount; ++I)
		{
			float3 A, B;
			
			float4x4 PT;
			if (S->Joints[I].mParent != 0XFFFF)
				PT = M[S->Joints[I].mParent];
			else
				PT = float4x4::Identity();

			auto J  = S->JointPoses[I];
			auto JT = PT * XMMatrixToFloat4x4(&GetTransform(J));

			A = (WT * (JT * Zero)).xyz();
			B = (WT * (PT * Zero)).xyz();
			M[I] = JT;

			float3 X = (WT * (PT * float4{ 1, 0, 0, 1 })).xyz();
			float3 Y = (WT * (PT * float4{ 0, 1, 0, 1 })).xyz();
			float3 Z = (WT * (PT * float4{ 0, 0, 1, 1 })).xyz();

			AddLineSegment(Out, { A, WHITE, B, PURPLE });
			AddLineSegment(Out, { B, RED,	X, RED });
			AddLineSegment(Out, { B, GREEN, Y, GREEN });
			AddLineSegment(Out, { B, BLUE,	Z, BLUE });
		}
	}


	/************************************************************************************************/


	void DEBUG_DrawPoseState(DrawablePoseState* DPS, SceneNodes* Nodes, NodeHandle Node, Line3DPass* Out)
	{
		if (!DPS)
			return;

		Skeleton* S = DPS->Sk;
		float4 Zero(0.0f, 0.0f, 0.0f, 1.0f);
		float4x4 WT; GetWT(Nodes, Node, &WT);

		for (size_t I = 1; I < S->JointCount; ++I)
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

			float3 X = (WT * (PT * float4{ 1, 0, 0, 1 })).xyz();
			float3 Y = (WT * (PT * float4{ 0, 1, 0, 1 })).xyz();
			float3 Z = (WT * (PT * float4{ 0, 0, 1, 1 })).xyz();

			AddLineSegment(Out, { A, WHITE, B, PURPLE});
			AddLineSegment(Out, { B, RED,	X, RED	 });
			AddLineSegment(Out, { B, GREEN, Y, GREEN });
			AddLineSegment(Out, { B, BLUE,	Z, BLUE	 });
		}
	}


}	/************************************************************************************************/