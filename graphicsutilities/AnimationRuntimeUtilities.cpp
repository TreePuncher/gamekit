#include "..\PCH.h"
#include "buildsettings.h"
#include "AnimationUtilities.h"
#include "AnimationRuntimeUtilities.h"

namespace FlexKit
{   /************************************************************************************************/


    Skeleton* Resource2Skeleton(AssetHandle RHandle, iAllocator* Memory)
	{
		SkeletonResourceBlob* Blob                  = (SkeletonResourceBlob*)GetAsset(RHandle);
        SkeletonResourceBlob::JointEntry* joints    = reinterpret_cast<SkeletonResourceBlob::JointEntry*>(((std::byte*)Blob) + sizeof(SkeletonResourceBlob::Header));

		Skeleton*	S = &Memory->allocate_aligned<Skeleton, 0x40>();
		S->InitiateSkeleton(Memory, Blob->header.JointCount);

		char* StringPool = (char*)Memory->malloc(64 * Blob->header.JointCount);

		for (size_t I = 0; I < Blob->header.JointCount; ++I)
		{
			Joint		J;
			JointPose	JP;
			float4x4	IP;
			memcpy(&IP, &joints[I].IPose,   sizeof(float4x4));
			memcpy(&JP,	&joints[I].Pose,    sizeof(JointPose));

			J.mID		= StringPool + (64 * I);
			J.mParent	= joints[I].Parent;
			strcpy_s((char*)J.mID, 64, joints[I].ID);

			S->AddJoint(J, *(XMMATRIX*)&IP);
		}

		FreeAsset(RHandle);
		return S;
	}


	/************************************************************************************************/


	AnimationClip Resource2AnimationClip(Resource* R, iAllocator* Memory)
	{
		AnimationResourceBlob* Anim = (AnimationResourceBlob*)R;
		AnimationClip	AC;// = &Memory->allocate_aligned<AnimationClip, 0x10>();
		AC.FPS             = (uint32_t)Anim->header.FPS;
		AC.FrameCount      = Anim->header.FrameCount;
		AC.isLooping       = Anim->header.IsLooping;
		AC.guid			   = Anim->header.GUID;
		size_t StrSize     = 1 + strlen(Anim->header.ID);
		AC.mID	           = (char*)Memory->malloc(strlen(Anim->header.ID));
		strcpy_s(AC.mID, StrSize, Anim->header.ID);
		AC.Frames		   = (AnimationClip::KeyFrame*)Memory->_aligned_malloc(sizeof(AnimationClip::KeyFrame) * AC.FrameCount);
		
		AnimationResourceBlob::FrameEntry* Frames = (AnimationResourceBlob::FrameEntry*)(Anim->Buffer);
		for (size_t I = 0; I < AC.FrameCount; ++I)
		{
			size_t jointcount       = Frames[I].JointCount;
			AC.Frames[I].JointCount = jointcount;
			AC.Frames[I].Joints     = (JointHandle*)	Memory->_aligned_malloc(sizeof(JointHandle) * jointcount, 0x10);
			AC.Frames[I].Poses      = (JointPose*)		Memory->_aligned_malloc(sizeof(JointPose)   * jointcount, 0x10);
			memcpy(AC.Frames[I].Joints, Anim->Buffer + Frames[I].JointStarts,  sizeof(JointHandle)  * jointcount);
			memcpy(AC.Frames[I].Poses,  Anim->Buffer + Frames[I].PoseStarts,   sizeof(JointPose)    * jointcount);
		}

		return AC;
	}


	/************************************************************************************************/


	using DirectX::XMMatrixRotationQuaternion;
	using DirectX::XMMatrixScalingFromVector;
	using DirectX::XMMatrixTranslationFromVector;


	/************************************************************************************************/


	PoseState CreatePoseState(Skeleton& skeleton, iAllocator* allocator)
	{
		using DirectX::XMMATRIX;
		using DirectX::XMMatrixIdentity;

		size_t JointCount = skeleton.JointCount;

        PoseState poseState;
        poseState.Joints		= (JointPose*)	allocator->_aligned_malloc(sizeof(JointPose) * JointCount, 0x40);
        poseState.CurrentPose	= (XMMATRIX*)	allocator->_aligned_malloc(sizeof(XMMATRIX) * JointCount, 0x40);
        poseState.JointCount	= JointCount;
        poseState.Sk			= &skeleton;

		for (size_t I = 0; I < JointCount; ++I)
            poseState.Joints[I] = JointPose(Quaternion{ 0, 0, 0, 1 },float4{0, 0, 0, 1});
		
		for (size_t I = 0; I < poseState.JointCount; ++I)
		{
			auto P = (skeleton.Joints[I].mParent != 0xFFFF) ? poseState.CurrentPose[skeleton.Joints[I].mParent] : XMMatrixIdentity();
            poseState.CurrentPose[I] = P * Float4x4ToXMMATIRX(GetPoseTransform(skeleton.JointPoses[I]));
		}

		return poseState;
	}


	/************************************************************************************************/


	bool InitiatePoseState(RenderSystem* RS, PoseState* EAS, PoseState_DESC& Desc, VShaderJoint* InitialState)
	{
		size_t ResourceSize = Desc.JointCount * sizeof(VShaderJoint) * 2;
		return true;
	}


	/************************************************************************************************/


	AnimationStateMachine::AnimationState SetDefaultState()
	{
		AnimationStateMachine::AnimationState State;
		State.Active			= false;
		State.Animation			= INVALIDHANDLE;
		State.EaseOutDuration	= 0;
		State.Enabled			= false;
		State.ID				= -1;
		State.In				= WeightFunction::EWF_INVALID;
		State.Out				= WeightFunction::EWF_INVALID;
		State.Speed				= 1.0f;

		return State;
	}


	/************************************************************************************************/


	void InitiateASM( AnimationStateMachine* ASM, iAllocator* Memory, EntityHandle Target)
	{
		ASM->StateCount			= 0;
		ASM->MaxConditionCount	= 16;
		ASM->MaxStateCount		= 16;

		for (auto& S : ASM->States) {
			S = SetDefaultState();
		}

		for (auto& C : ASM->Conditions) {
			C.Enabled = false;
		}
	}


	/************************************************************************************************/


	void DASEnableCondition(DAConditionHandle H, AnimationStateMachine* ASM){
		ASM->Conditions[H].Enabled = true;
	}

	void DASDisableCondition(DAConditionHandle H, AnimationStateMachine* ASM){
		ASM->Conditions[H].Enabled = false;
	}


	/************************************************************************************************/

	
	DAStateHandle DASAddState( AnimationStateEntry_Desc& Desc, AnimationStateMachine* Out )
	{
        auto Index = (DAStateHandle)INVALIDHANDLE;
		if (Out->StateCount < Out->MaxStateCount){
			Index       = Out->StateCount++;
			auto& State = Out->States[Index];

			State.Active			= false;
			State.Enabled			= true;
			State.ID				= -1;
			State.EaseOutDuration	= Desc.EaseOutDuration;
			State.EaseInDuration	= Desc.EaseInDuration;
			State.EaseIn			= Desc.EaseIn;
			State.In				= Desc.In;
			State.Out				= Desc.Out;
			State.Speed				= Desc.Speed;
			State.Animation			= Desc.Animation;
			State.Loop				= Desc.Loop;
			State.ForceComplete		= Desc.ForceComplete;
			State.EaseOut			= Desc.EaseOut;
			State.TriggerOnExit		= Desc.OnExitTrigger;
		}

		return Index;
	}


	/************************************************************************************************/

	DAConditionHandle	DASAddCondition(AnimationCondition_Desc& Desc, AnimationStateMachine* Out)
	{
        auto Index = (DAConditionHandle)INVALIDHANDLE;
		if (Out->ConditionCount < Out->MaxConditionCount) {
			Index = Out->ConditionCount++;
			auto& Condition			= Out->Conditions[Index];

			Condition.Enabled		= true;
			Condition.Operation		= Desc.Operation;
			Condition.TargetState	= Desc.DrivenState;
			Condition.Type			= Desc.InputType;
			Condition.Inputs[0].B	= false;
			Condition.Inputs[1].B	= false;
			Condition.Inputs[2].B	= false;
		}

		return Index;
	}


	/************************************************************************************************/

	
	void ASSetBool(DAConditionHandle Condition, bool B, AnimationStateMachine* ASM, size_t index)
	{
		FK_ASSERT(index < size(ASM->Conditions[Condition].Inputs));

		if(ASM->Conditions[Condition].Operation == EASO_TRUE || EASO_FALSE)
			ASM->Conditions[Condition].Inputs[index].B = B;
	}


	/************************************************************************************************/


	void UpdateASM( double dt, AnimationStateMachine* ASM, iAllocator* TempMemory, iAllocator* Memory, GraphicScene* Scene)
	{
		/*
		// Set all States to False
		for (auto& S : ASM->States) 
			S.Active = true;

		for (auto C : ASM->Conditions) {
			if (C.Enabled) {
				switch (C.Operation)
				{
				case EASO_TRUE:
					ASM->States[C.TargetState].Active &= (C.Inputs[0].B || C.Inputs[1].B || C.Inputs[2].B);	break;
				case EASO_FALSE:
					ASM->States[C.TargetState].Active &= !(C.Inputs[0].B || C.Inputs[1].B || C.Inputs[2].B); break;
				case EASO_GREATERTHEN:
					ASM->States[C.TargetState].Active &= (C.Inputs[0].I < C.Inputs[1].I); break;
				case EASO_LESSTHEN:
					ASM->States[C.TargetState].Active &= (C.Inputs[0].I > C.Inputs[1].I); break;
				case EASO_WITHIN:
					ASM->States[C.TargetState].Active &= (C.Inputs[0].I - C.Inputs[1].I) < C.Inputs[2].I; break;
				case EASO_FARTHERTHEN:
					ASM->States[C.TargetState].Active &= (C.Inputs[0].I - C.Inputs[1].I) > C.Inputs[2].I; break;
				default:
					break;
				}
			}
		}

		for (auto& S : ASM->States) {
			if (S.Enabled) {
				if (S.Active && S.ID == INVALIDHANDLE) {
					float W = 1;

					if (S.EaseIn && S.EaseInDuration > 0)
						W = S.EaseIn(0);

					auto RES = Scene->EntityPlayAnimation(ASM->TargetDrawable, S.Animation, W, S.Loop);

					if (!CompareFloats(1.0f, S.Speed, 0.001f))
						SetAnimationSpeed(Scene->GetEntityAnimationState(ASM->TargetDrawable), (int64_t)RES, S.Speed);

					if (RES != false)
						S.ID = RES;

					S.EaseOutProgress = 0.0f;
				}
				else if(S.ID != INVALIDHANDLE) // Animation In flight
				{
					auto AnimationState = Scene->GetEntityAnimationState(ASM->TargetDrawable);
					if (AnimationState && isStillPlaying(AnimationState, S.ID))
					{
						auto C = GetAnimation(AnimationState, S.ID);
						C->Speed = S.Speed;

						if (S.Loop)
						{
							if (!S.Active) {
								if (S.ForceComplete) {
									C->ForceLoop = false;
								} else {
									C->Speed = 0.0f;
								}
							}
							else
								C->ForceLoop = true;
						}

						auto Animation	= GetAnimation(AnimationState, S.ID);
						if (Animation) {
							float EaseOutBegin		= 1 - S.EaseOutDuration;
							float EaseInDuration	= S.EaseInDuration;
							float AnimationProgress = GetAnimation_Completed(Animation);
							
							if (S.Active)
							{
								if (Animation->FirstPlay && S.EaseIn && EaseInDuration > 0.0 && AnimationProgress <= EaseInDuration)
									Animation->Weight = S.EaseIn(AnimationProgress / EaseInDuration);
								else
									Animation->Weight = 1;

								S.EaseOutProgress = 0.0f;
							}
							else
							{
								// Calculate ease out Animation Blend Weighting
								if (S.ForceComplete)
								{	// Blend Animation with next Animation
									if (AnimationProgress > EaseOutBegin) {
										if( CompareFloats( S.EaseOutProgress, 0.0f, 1.0f/60.0f) ) 
											ASSetBool(S.TriggerOnExit, true, ASM); // Trigger next Condition

										if (S.EaseOutDuration > 0.0f)
											S.EaseOutProgress += dt / S.EaseOutDuration;
										else
											S.EaseOutProgress = 1.0f;

										float W = AnimationProgress - EaseOutBegin;
										Animation->Weight = S.EaseOut(W);
									}
								}
								else if (!S.ForceComplete)
								{	// Pause Animation + Transition to next Animation/Pose
									if (S.EaseOutDuration > 0.0f)
										S.EaseOutProgress += dt / S.EaseOutDuration;
									else
										S.EaseOutProgress = 1.0f;

									float W = S.EaseOutProgress;
									Animation->Weight = S.EaseOut(1 - W);

									if (S.EaseOutProgress >= 1.0f) {
										AnimationState->Clips.erase(Animation);
										S.ID = INVALIDHANDLE;

										if (S.TriggerOnExit != INVALIDHANDLE)
											ASSetBool(S.TriggerOnExit, true, ASM);
									}
								}
							}


							//std::cout << "Animation Weight: " << Animation->Weight << "\n";
						}
					}
					else
					{
						// Animation Ended
						S.ID	 = INVALIDHANDLE;
						S.Active = false;
					}
				}
			}
		}
		*/
	}


	/************************************************************************************************/


	float EaseOut_RAMP(float Weight)
	{
		return Weight;
	}


	float EaseOut_Squared(float Weight)
	{
		return Weight * Weight;
	}

	float EaseIn_RAMP(float Weight)
	{
		return Weight;
	}

	float EaseIn_Squared(float Weight)
	{
		Weight = Weight;
		return Weight * Weight;
	}

	bool isStillPlaying(DrawableAnimationState* AS, int64_t ID)
	{
		for (auto C : AS->Clips)
			if (C.ID == ID && C.Playing)
				return true;
		return false;
	}
	

	Pair<EPLAY_ANIMATION_RES, bool> CheckState_PlayAnimation(Drawable* E, iAllocator* MEM)
	{
		using FlexKit::PoseState;
		if (!E || !(E->MeshHandle != InvalidHandle_t))
			return{ EPLAY_ANIMATION_RES::EPLAY_INVALID_PARAM, false };

		auto MeshHandle = E->MeshHandle;

		if (!IsSkeletonLoaded(MeshHandle))
			return{ EPLAY_ANIMATION_RES::EPLAY_NOT_ANIMATABLE, false };

		auto Mesh = GetMeshResource(MeshHandle);

		if (false)//!E->PoseState)
		{
            /*
			Skeleton*	S		= (Skeleton*)Mesh->Skeleton;
			auto NewPoseState	= CreatePoseState(E, MEM);

			if (!NewPoseState)
				return{ EPLAY_NOT_ANIMATABLE, false };
            */
			//E->PoseState = NewPoseState;
		}// Create Animation State

		//if (!E->AnimationState)
		//{
		//	auto New_EAS = (DrawableAnimationState*)MEM->_aligned_malloc(sizeof(DrawableAnimationState));
		//	E->AnimationState = New_EAS;
		//	New_EAS->Clips = {};
		//}

		return{ EPLAY_ANIMATION_RES::EPLAY_SUCCESS, true };
	}


	/************************************************************************************************/


	EPLAY_ANIMATION_RES PlayAnimation(Drawable* E, GUID_t Guid, iAllocator* Allocator, bool ForceLoop, float Weight, int64_t* OutID)
	{
		using FlexKit::PoseState;
		auto Res = CheckState_PlayAnimation(E, Allocator);

		if (!Res)
			return Res;

		auto MeshHandle = E->MeshHandle;
		auto Mesh		= GetMeshResource(MeshHandle);

		//auto EPS = E->PoseState;
		//auto EAS = E->AnimationState;
		//auto S	 = Mesh->Skeleton;

		/*
		Skeleton::AnimationList* I = S->Animations;

		while (I)
		{
			if (I->Clip.guid == Guid)
			{
				AnimationStateEntry ASE;
				ASE.Clip      = &I->Clip;
				ASE.order     = EAS->Clips.size();
				ASE.T         = 0;
				ASE.Playing   = true;
				ASE.Speed     = 1.0f;
				ASE.ForceLoop = ForceLoop;
				ASE.Weight    = max(min(Weight, 1.0f), 0.0f);
				ASE.ID		  = chrono::high_resolution_clock::now().time_since_epoch().count();
				EAS->Clips.push_back(ASE);
				E->Posed	  = true; // Enable Posing or Animation won't do anything
				if (OutID) *OutID = ASE.ID;
				return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
			}
			I = I->Next;
		}
		*/

		//return EPLAY_ANIMATION_RES::EPLAY_ANIMATION_NOT_FOUND;
		return EPLAY_ANIMATION_RES::EPLAY_FAILED;
	}


	/************************************************************************************************/


	EPLAY_ANIMATION_RES PlayAnimation(Drawable* E, const char* Animation, iAllocator* Allocator, bool ForceLoop, float Weight, int64_t* OutID)
	{
		using FlexKit::PoseState;
		auto Res = CheckState_PlayAnimation(E, Allocator);

		if (!Res)
			return Res;

		auto MeshHandle = E->MeshHandle;
		auto Mesh       = GetMeshResource(MeshHandle);

		/*
		auto EPS        = E->PoseState;
		auto EAS        = E->AnimationState;
		auto S          = Mesh->Skeleton;

		Skeleton::AnimationList* I = S->Animations;

		while (I)
		{
			if (!strcmp(I->Clip.mID, Animation))
			{
				AnimationStateEntry ASE;
				ASE.Clip	  = &I->Clip;
				ASE.order	  = EAS->Clips.size();
				ASE.T		  = 0;
				ASE.Playing	  = true;
				ASE.Speed	  = 1.0f;
				ASE.ForceLoop = ForceLoop;
				ASE.Weight	  = max(min(Weight, 1.0f), 0.0f);
				ASE.ID		  = chrono::high_resolution_clock::now().time_since_epoch().count();
				EAS->Clips.push_back(ASE);
				
				if (OutID) *OutID = ASE.ID;
				E->Posed	  = true; // Enable Posing or Animation won't do anything
				return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
			}
			I = I->Next;
		}
		*/
		//return EPLAY_ANIMATION_RES::EPLAY_ANIMATION_NOT_FOUND;
		return EPLAY_ANIMATION_RES::EPLAY_FAILED;
	}


	/************************************************************************************************/


	EPLAY_ANIMATION_RES PlayAnimation(Drawable* E, GUID_t Guid, iAllocator* Allocator, bool ForceLoop, float Weight, int64_t& out)
	{
		using FlexKit::PoseState;
		auto Res = CheckState_PlayAnimation(E, Allocator);

		if (!Res)
			return Res;

		auto MeshHandle = E->MeshHandle;
		auto Mesh       = GetMeshResource(MeshHandle);

		/*
		auto EPS        = E->PoseState;
		auto EAS        = E->AnimationState;
		auto S          = Mesh->Skeleton;

		Skeleton::AnimationList* I = S->Animations;

		while (I)
		{
			if (I->Clip.guid == Guid)
			{
				AnimationStateEntry ASE;
				ASE.Clip	  = &I->Clip;
				ASE.order	  = EAS->Clips.size();
				ASE.T		  = 0;
				ASE.Playing	  = true;
				ASE.Speed	  = 1.0f;
				ASE.ForceLoop = ForceLoop;
				ASE.Weight	  = max(min(Weight, 1.0f), 0.0f);
				ASE.ID		  = chrono::high_resolution_clock::now().time_since_epoch().count();
				ASE.FirstPlay = true;
				EAS->Clips.push_back(ASE);
				
				out = ASE.ID;
				E->Posed = true; // Enable Posing or Animation won't do anything
				return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
			}
			I = I->Next;
		}

		return EPLAY_ANIMATION_RES::EPLAY_ANIMATION_NOT_FOUND;
		*/
		return EPLAY_ANIMATION_RES::EPLAY_FAILED;
	}


	/************************************************************************************************/


	EPLAY_ANIMATION_RES SetAnimationSpeed(DrawableAnimationState* AE, int64_t ID, double Speed)
	{
		using FlexKit::PoseState;

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
		using FlexKit::PoseState;

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


	EPLAY_ANIMATION_RES SetAnimationWeight(DrawableAnimationState* AE, int64_t	ID, float Weight)
	{
		for(auto& C : AE->Clips)
		{
			if (C.ID == ID)
			{
				C.Weight = Weight;
				return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
			}
		}
		return EPLAY_ANIMATION_RES::EPLAY_FAILED;
	}


	/************************************************************************************************/


	EPLAY_ANIMATION_RES SetAnimationWeight(DrawableAnimationState* AE, const char*	AnimationID, float Weight)
	{
		for (auto& C : AE->Clips)
		{
			if (!strcmp(C.Clip->mID, AnimationID))
			{
				C.Weight = Weight;
				return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
			}
		}
		return EPLAY_ANIMATION_RES::EPLAY_FAILED;
	}


	/************************************************************************************************/


	EPLAY_ANIMATION_RES SetAnimationWeight(DrawableAnimationState* AE, GUID_t	Guid, float Weight)
	{
		for (auto& C : AE->Clips)
		{
			if (C.Clip->guid == Guid)
			{
				C.Weight = Weight;
				return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
			}
		}
		return EPLAY_ANIMATION_RES::EPLAY_FAILED;
	}


	/************************************************************************************************/


	Pair<EPLAY_ANIMATION_RES, bool> StopAnimation_CheckState(Drawable* E)
	{
		return { EPLAY_ANIMATION_RES::EPLAY_FAILED, false };

		if (!E)
			return { EPLAY_ANIMATION_RES::EPLAY_INVALID_PARAM, false };

		if (E->MeshHandle == InvalidHandle_t || !IsSkeletonLoaded(E->MeshHandle))
			return { EPLAY_ANIMATION_RES::EPLAY_NOT_ANIMATABLE, false };

		//if (!E->Posed)
		//	return { EPLAY_ANIMATION_RES::EPLAY_NOT_ANIMATABLE, false };

		return { EPLAY_ANIMATION_RES::EPLAY_SUCCESS, true };
	}


	EPLAY_ANIMATION_RES StopAnimation(Drawable* E, GUID_t Guid)
	{
		return EPLAY_ANIMATION_RES::EPLAY_FAILED;

		//auto Status = StopAnimation_CheckState(E);
		//if (!Status)
		//	return Status;

		//auto EAS	= E->AnimationState;
		
		//for (auto& C : EAS->Clips)
		//{
		//	if (C.Clip->guid == Guid) {
		//		C.Playing = false;
		//		return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
		//	}
		//}
		//return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
	}


	/************************************************************************************************/


	EPLAY_ANIMATION_RES StopAnimation(Drawable* E, uint32_t ID)
	{
		return EPLAY_ANIMATION_RES::EPLAY_FAILED;

		//auto Status = StopAnimation_CheckState(E);
		//if (!Status)
		//	return Status;

		//auto Mesh	= GetMeshResource(E->MeshHandle);
		//auto EAS	= E->AnimationState;

		//for (auto& C : EAS->Clips)
		//{
		//	if (C.ID == ID) {
		//		C.Playing = false;
		//		return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
		//	}
		//}

		//return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
	}


	/************************************************************************************************/


	EPLAY_ANIMATION_RES StopAnimation(Drawable* E, const char* Animation)
	{
		return EPLAY_ANIMATION_RES::EPLAY_FAILED;

		//auto Status = StopAnimation_CheckState(E);
		//if (!Status)
		//	return Status;

		//auto EAS	= E->AnimationState;

		//for (auto& C : EAS->Clips)
		//{
		//	if (!strcmp(C.Clip->mID, Animation)) {
		//		C.Playing = false;
		//		return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
		//	}
		//}

		//return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
	}


	/************************************************************************************************/


	void UploadPose(RenderSystem* RS, Drawable* E, iAllocator* TEMP)
	{
		using DirectX::XMMATRIX;
		using DirectX::XMMatrixIdentity;
		using DirectX::XMMatrixInverse;

		//auto Mesh	= GetMeshResource(E->MeshHandle);
		//auto PS		= E->PoseState;
		//auto S		= Mesh->Skeleton;

		//VShaderJoint* Out = (VShaderJoint*)TEMP->_aligned_malloc(sizeof(VShaderJoint) * S->JointCount);

		//PS->Dirty = false;

		//for (size_t I = 0; I < S->JointCount; ++I)
		//	Out[I] = { S->IPose[I], PS->CurrentPose[I] };

		FK_ASSERT(0);
		//if (!PS->Resource)
		{	// Create Animation State with Data as Initial
			//FlexKit::PoseState_DESC	Desc = { S->JointCount };
			//auto RES = InitiatePoseState(RS, PS, Desc, Out);
		}
		//else
		{
			//UpdateResourceByTemp(RS, &PS->Resource, Out, sizeof(VShaderJoint) * S->JointCount, 1, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}
	}


	/************************************************************************************************/


	void UploadPoses(RenderSystem* RS, PVS* Drawables, iAllocator* TEMP)
	{
		for (PVEntry& d : *Drawables)
		{
			auto D = d.D;

			//if(D->PoseState && D->PoseState->Dirty)
			//	UploadPose(RS, D, TEMP);
		}
	}


	/************************************************************************************************/


	EPLAY_ANIMATION_RES ClearAnimationPose(PoseState* DPS, iAllocator* TEMP)
	{
		using DirectX::XMMatrixIdentity;
		auto S = DPS->Sk;

		XMMATRIX* M = (XMMATRIX*)TEMP->_aligned_malloc(S->JointCount * sizeof(float4x4));

		if (!M)
			return EPLAY_ANIMATION_RES::EPLAY_FAILED;

		for (size_t I = 0; I < S->JointCount; ++I) {
			auto BasePose	= S->JointPoses[I];
			auto JointPose	= DPS->Joints[I];
			
			M[I] = Float4x4ToXMMATIRX(GetPoseTransform(JointPose) * GetPoseTransform(BasePose));
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


	size_t GetAnimationCount(Drawable* E)
	{
		size_t Count = 0;
		//if(E && E->AnimationState)
		//	for (auto Clip : E->AnimationState->Clips)
		//		if (Clip.Playing)
		//			Count++;

		return Count;
	}

	AnimationStateEntry* GetAnimation(DrawableAnimationState* AS, int64_t ID)
	{
		for (auto& C : AS->Clips)
			if (C.ID == ID)
				return &C;

		return nullptr;
	}


	double GetAnimation_Completed(AnimationStateEntry* A)
	{
		double FrameDuration	= 1.0 / A->Clip->FPS;
		double AnimationLength	= A->Clip->FrameCount * FrameDuration;
		return A->T / AnimationLength;
	}


	size_t GetAnimationClipCount(Skeleton* E)
	{
		size_t Out	= 0;
		auto I		= E->Animations;
		while (I)
			if (I)
			{
				I = I->Next;
				Out++;
			}
		return Out;
	}

	/************************************************************************************************/


	void UpdateAnimation(RenderSystem* RS, Drawable* E, double dT, iAllocator* TEMP, bool AdvanceOnly)
	{
		using DirectX::XMMATRIX;
		using DirectX::XMMatrixIdentity;
		using DirectX::XMMatrixInverse;
		using DirectX::XMMatrixRotationQuaternion;
		using DirectX::XMMatrixScalingFromVector;
		using DirectX::XMMatrixTranslationFromVector;
		using DirectX::XMVectorSet;


		FK_ASSERT(false);
		/*
		if (E && E->Posed)
		{
			if (!E->PoseState || !E->AnimationState) {
				E->Posed = false;
				return;
			}

			auto PS		= E->PoseState;
			auto AS		= E->AnimationState;
			auto Mesh	= GetMeshResource(E->MeshHandle);
			auto S		= Mesh->Skeleton;

			XMMATRIX* M = (XMMATRIX*)TEMP->_aligned_malloc(S->JointCount * sizeof(float4x4));

			for (size_t I = 0; I < S->JointCount; ++I) {
				auto BasePose	= S->JointPoses[I];
				auto JointPose	= PS->Joints[I];
				
				M[I] = GetPoseTransform(JointPose) * GetPoseTransform(BasePose);
			}

			bool AnimationPlayed = false;
			size_t size = AS->Clips.size();
			int c = 0;
			std::sort(AS->Clips.begin().I, AS->Clips.end().I, 
				[](const AnimationStateEntry& LHS, const AnimationStateEntry& RHS) -> bool 
			{
					size_t W1 = (LHS.Playing ? 0x00 : 1 << 31) | LHS.order;
					size_t W2 = (RHS.Playing ? 0x00 : 1 << 31) | RHS.order;
					return W1 < W2; });

			for (auto& C : AS->Clips)
			{
				if (C.Playing && C.Clip->FrameCount)
				{
					const AnimationClip* Clip = C.Clip;
					bool Loop                = Clip->isLooping;
					float  Weight            = Saturate(C.Weight);
					double T                 = C.T;
					double FrameDuration     = (1.0f / Clip->FPS);
					double AnimationDuration = FrameDuration * Clip->FrameCount;

					if (!(Loop | C.ForceLoop) && C.T > AnimationDuration)
						C.Playing	= false;
					else if (C.T > AnimationDuration) {
						C.FirstPlay = false;
						C.T = 0.0f;
					}
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

							auto PoseT	= GetPoseTransform(Pose);
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
		*/
	}


	/************************************************************************************************/


	float4x4 GetJointPosed_WT(JointHandle Joint, NodeHandle Node, PoseState* DPS)
	{
		float4x4 WT; GetTransform(Node, &WT);
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
			PrintChildren(S, (JointHandle)II, Depth + 1);
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
				PrintChildren(S, (JointHandle)I, 1);
			}
		}
	}


	/************************************************************************************************/


    LineSegments BuildSkeletonLineSet(Skeleton* S, NodeHandle Node, iAllocator* allocator)
	{
        LineSegments lines{ allocator };
		float4 Zero(0.0f, 0.0f, 0.0f, 1.0f);

		float4x4 WT         = GetWT(Node);
        Vector<float4x4> M( allocator, S->JointCount, float4x4::Identity() );

        const float4 debugLines[] =
        {
            { 1, 0, 0, 1 },
            { 0, 1, 0, 1 },
            { 0, 0, 1, 1 }
        };

        const float4 Colors[] =
        {
            RED,
            GREEN,
            BLUE
        };


		for (size_t I = 1; I < S->JointCount; ++I)
		{
			float4x4 PT = (S->Joints[I].mParent != InvalidHandle_t) ? M[S->Joints[I].mParent] : float4x4::Identity();

			auto J  = S->JointPoses[I];
			auto JT = S->GetInversePose(JointHandle(I)) * GetPoseTransform(J);

            const auto temp = (JT * Zero);
            const float4 VA = (WT * (JT * Zero));
            const float4 VB = (WT * (PT * Zero));

            const float3 A = VA.xyz();
            const float3 B = VB.xyz();

			M[I] = JT;

            if (VA.w <= 0 || VB.w <= 0)
                continue;

            lines.push_back({ A, BLACK,	B, PURPLE	});

            for (size_t itr = 0; itr < sizeof(debugLines) / sizeof(debugLines[0]); ++itr)
            {
                const float4 V      = WT * (PT * debugLines[itr]);
                const auto& color   = Colors[itr];
                lines.push_back({ B, color, V.xyz(), color });
            }
		}

        return lines;
	}


	/************************************************************************************************/


    LineSegments DEBUG_DrawPoseState(PoseState& poseState, NodeHandle Node, iAllocator* allocator)
	{
        LineSegments lines{ allocator };

		Skeleton* S = poseState.Sk;
		float4 Zero(0.0f, 0.0f, 0.0f, 1.0f);
		float4x4 WT; GetTransform(Node, &WT);

		for (size_t I = 1; I < S->JointCount; ++I)
		{
			float3 A, B;

			float4x4 PT;
			if (S->Joints[I].mParent != 0XFFFF)
				PT = XMMatrixToFloat4x4(poseState.CurrentPose + S->Joints[I].mParent);
			else
				PT = float4x4::Identity();

			auto JT = XMMatrixToFloat4x4(poseState.CurrentPose + I);

			A = (WT * (JT * Zero)).xyz();
			B = (WT * (PT * Zero)).xyz();

			float3 X = (WT * (PT * float4{ 1, 0, 0, 1 })).xyz();
			float3 Y = (WT * (PT * float4{ 0, 1, 0, 1 })).xyz();
			float3 Z = (WT * (PT * float4{ 0, 0, 1, 1 })).xyz();

			lines.push_back({ A, WHITE,	B, PURPLE	});
			lines.push_back({ B, RED,	X, RED		});
			lines.push_back({ B, GREEN,	Y, GREEN	});
			lines.push_back({ B, BLUE,	Z, BLUE		});
		}

        return lines;
	}


}	/************************************************************************************************/


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
