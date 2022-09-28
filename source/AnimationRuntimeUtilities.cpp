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

			S->AddJoint(J, IP);
		}

		FreeAsset(RHandle);
		return S;
	}


	/************************************************************************************************/


	using DirectX::XMMatrixRotationQuaternion;
	using DirectX::XMMatrixScalingFromVector;
	using DirectX::XMMatrixTranslationFromVector;


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
		FK_ASSERT(index < size_t(ASM->Conditions[Condition].Inputs));

		if(ASM->Conditions[Condition].Operation == EASO_TRUE || EASO_FALSE)
			ASM->Conditions[Condition].Inputs[index].B = B;
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

	bool isStillPlaying(BrushAnimationState* AS, int64_t ID)
	{
		for (auto C : AS->Clips)
			if (C.ID == ID && C.Playing)
				return true;
		return false;
	}
	

	/************************************************************************************************/


	float4x4 GetJointPosed_WT(JointHandle Joint, NodeHandle Node, PoseState* DPS)
	{
		const float4x4 WT = GetWT(Node);
		const float4x4 JT = DPS->CurrentPose[Joint];

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
			{ 0.1f, 0.0f, 0.0f, 1.0f },
			{ 0.0f, 0.1f, 0.0f, 1.0f },
			{ 0.0f, 0.0f, 0.1f, 1.0f }
		};

		const float4 Colors[] =
		{
			{ RED },
			{ GREEN },
			{ BLUE }
		};


		for (size_t I = 1; I < S->JointCount; ++I)
		{
			auto parent = S->Joints[I].mParent;
			float4x4 PT = (parent != 0xffff) ? M[parent] : float4x4::Identity();
			

			const float4x4 IT   = S->GetInversePose(JointHandle(I));
			const float4x4 P_T  = XMMatrixToFloat4x4(DirectX::XMMatrixInverse(nullptr, Float4x4ToXMMATIRX(IT)));
			const float4x4 JT   = P_T;

			const float4 VA = (WT * (JT * Zero));
			const float4 VB = (WT * (PT * Zero));

			const float3 A = VA.xyz();
			const float3 B = VB.xyz();

			M[I] = P_T;

			if (VA.w <= 0 || VB.w <= 0)
				continue;

			lines.push_back({ A, BLACK,	B, PURPLE	});

			for (size_t itr = 0; itr < sizeof(debugLines) / sizeof(debugLines[0]); ++itr)
			{
				const float4 V      = WT * (PT * debugLines[itr]);
				const float4 color  = Colors[itr];
				lines.emplace_back(B, color.xyz(), V.xyz(), color.xyz());
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
			const float4x4 PT   = (S->Joints[I].mParent != 0XFFFF) ?
									poseState.CurrentPose[S->Joints[I].mParent]:
									float4x4::Identity();

			const auto JT   = poseState.CurrentPose[I];

			const float3 A = (WT * (JT * Zero)).xyz();
			const float3 B = (WT * (PT * Zero)).xyz();

			const float3 X = (WT * (PT * float4{ 0.3f, 0.0f, 0.0f, 1.0f })).xyz();
			const float3 Y = (WT * (PT * float4{ 0.0f, 0.3f, 0.0f, 1.0f })).xyz();
			const float3 Z = (WT * (PT * float4{ 0.0f, 0.0f, 0.3f, 1.0f })).xyz();

			lines.push_back({ A, WHITE,	B, PURPLE	});
			lines.push_back({ B, RED,	X, RED		});
			lines.push_back({ B, GREEN,	Y, GREEN	});
			lines.push_back({ B, BLUE,	Z, BLUE		});
		}

		return lines;
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
