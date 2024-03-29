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


#ifndef ANIMATION_UTILITIES_H
#define ANIMATION_UTILITIES_H

#include "..\PCH.h"
#include "MathUtils.h"
#include "memoryutilities.h"
#include "Assets.h"
#include "AnimationUtilities.h"
#include "CoreSceneObjects.h"
#include "graphics.h"

#include <DirectXMath/DirectXMath.h>
#include <functional>

// TODOs:
//	Additive Animation
//	Curve Driven Animation


namespace FlexKit
{
	using DirectX::XMMATRIX;


	class Scene;


	FLEXKITAPI void Skeleton_PushAnimation	( Skeleton* S, iAllocator* Allocator, AnimationClip AC );
	FLEXKITAPI void CleanUpSkeleton			( Skeleton* S );


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
				JointPoses[I].ts = { 0, 0, 0, 0 };
				JointPoses[I].r  = DirectX::XMQuaternionIdentity();
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


	struct AnimationStateEntry
	{
		AnimationClip*	Clip;
		double			T;
		double			Speed;
		float			Weight;
		int32_t			order;
		int64_t			ID;
		uint8_t			Playing;
		bool			FirstPlay;
		bool			ForceLoop;

		enum
		{
			Skeletal,
			Simulation,
			Vertex,
		}TargetType;

		byte			Pad[38];

	};

	struct BrushAnimationState
	{
		static_vector<AnimationStateEntry>	Clips; // Animations in
        PoseState*					        Target;
	};


	/************************************************************************************************/


	enum WeightFunction : byte
	{
		EWF_Power,
		EWF_Sin,
		EWF_Cos,
		EWF_Log,
		EWF_Ramp,
		EWF_Square,
		EWF_INVALID,
	};


	struct BlendLeaf
	{
		float		Weights		[2];
		uint32_t	IDs			[2];
		GUID_t		Animation	[2];
	};


	struct BlendNode
	{
		bool Leaf[2];

		union {
			BlendLeaf	Leaf;
			BlendNode*	Branch;
		}Child1, Child2;
	};


	struct BlendTree
	{
		BlendNode*	Nodes;
		size_t		NodeCount;
	};


	enum ASCondition_OP
	{
		EASO_TRUE,
		EASO_FALSE,
		EASO_GREATERTHEN,
		EASO_LESSTHEN,
		EASO_WITHIN,
		EASO_FARTHERTHEN,
	};


	enum ASCondition_InputType
	{
		ASC_BOOL,
		ASC_INT,
		ASC_FLOAT,
	};

	typedef float(*EASE_FN)(float Weight);

	FLEXKITAPI float EaseOut_RAMP	(float Weight);
	FLEXKITAPI float EaseOut_Squared(float Weight);

	FLEXKITAPI float EaseIn_RAMP	(float Weight);
	FLEXKITAPI float EaseIn_Squared	(float Weight);

	typedef uint16_t DAStateHandle;
	typedef uint16_t DAConditionHandle;


	struct AnimationStateMachine
	{
		uint16_t StateCount;
		uint16_t ConditionCount;

		uint16_t MaxConditionCount;
		uint16_t MaxStateCount;

		struct AnimationState
		{
			bool			Enabled;
			bool			Active;
			bool			Loop;		// Loops Animation While Active
			bool			ForceComplete;

			EASE_FN			EaseIn;
			EASE_FN			EaseOut;

			WeightFunction		In;
			WeightFunction		Out;
			//byte				Pad;
			float				Speed;
			float				EaseOutDuration;	// ratio from the end
			float				EaseInDuration;
			float				EaseOutProgress;
			DAConditionHandle	TriggerOnExit;
			int64_t				ID;
			GUID_t				Animation;
		}States[16];

		struct StateCondition
		{
			ASCondition_OP			Operation;
			DAStateHandle			TargetState;
			ASCondition_InputType	Type;
			bool					Enabled;

			union Inputs
			{
				float F;
				int   I;
				bool  B;
			}Inputs[3];
		}Conditions[16];
	};


	struct AnimationController
	{
		AnimationStateMachine*	ASM;
		BlendTree*				BlendTree;
        PoseState*		        Target;
	};


	/************************************************************************************************/


	struct AnimationStateEntry_Desc
	{
		EASE_FN			EaseOut				= EaseOut_Squared;
		EASE_FN			EaseIn				= EaseIn_Squared;

		WeightFunction		In					= EWF_Ramp;
		WeightFunction		Out					= EWF_Ramp;
		float				Speed				= 1.0f;
		float				EaseOutDuration		= 0.0f;
		float				EaseInDuration		= 0.0f;
		bool				Loop				= false;
		bool				ForceComplete		= false;
		DAConditionHandle	OnExitTrigger		= (DAConditionHandle)INVALIDHANDLE;
		GUID_t				Animation			= (DAConditionHandle)INVALIDHANDLE;
	};

	struct AnimationCondition_Desc
	{
		ASCondition_InputType	InputType;
		ASCondition_OP			Operation;
		DAStateHandle			DrivenState;
	};


	FLEXKITAPI DAStateHandle		DASAddState		( AnimationStateEntry_Desc&, AnimationStateMachine* Out );
	FLEXKITAPI DAConditionHandle	DASAddCondition	( AnimationCondition_Desc&,  AnimationStateMachine* Out );

	FLEXKITAPI void	ASSetBool	( DAConditionHandle, bool	B, AnimationStateMachine* ASM, size_t index = 0 );
	FLEXKITAPI void	ASSetFloat	( DAConditionHandle, float	F, AnimationStateMachine* ASM, size_t index = 0 );

	FLEXKITAPI void ASEnableCondition	( DAConditionHandle H, AnimationStateMachine* ASM );
	FLEXKITAPI void ASDisableCondition	( DAConditionHandle H, AnimationStateMachine* ASM );


	FLEXKITAPI void UpdateASM(double dt, AnimationStateMachine* ASM, iAllocator* TempMemory, iAllocator* Memory, Scene* Scene);


	/************************************************************************************************/


	inline void RotateJoint			(PoseState* E, JointHandle J, Quaternion Q )
	{
		if (!E)
			return;

		if (J != InvalidHandle && J < E->JointCount) {
			E->Joints[J].r = DirectX::XMQuaternionMultiply(E->Joints[J].r, Q);
			E->Dirty = true;
		}
	}

	inline void TranslateJoint		(PoseState* PS, JointHandle J, float3 xyz )
	{
		if (!PS)
			return;

		if (J != InvalidHandle)
        {
            const float4 xyzw{ xyz, 0 };
			PS->Joints[J].ts = PS->Joints[J].ts + xyzw;
			PS->Dirty = true;
		}
	}

	inline void SetJointTranslation (PoseState* E, JointHandle J, float3 xyz )
	{
		if (!E)
			return;

		if (J != InvalidHandle) {
			float s = E->Joints[J].ts.w;
			E->Joints[J].ts = float4( xyz.x, xyz.y, xyz.z, s);
			E->Dirty = true;
		}
	}

	inline void ScaleJoint			(PoseState* E, JointHandle J, float S )
	{
		if (!E)
			return;

		if (J!= InvalidHandle)
			E->Joints[J].ts.w *= S;
	}


	/************************************************************************************************/


	FLEXKITAPI PoseState	CreatePoseState		( Skeleton&, iAllocator*);
	FLEXKITAPI void			InitiateASM			( AnimationStateMachine* AS, iAllocator*, EntityHandle Target );


	/************************************************************************************************/


	enum EPLAY_ANIMATION_RES
	{
		EPLAY_SUCCESS				=  0,
		EPLAY_FAILED				= -1,
		EPLAY_NOT_ANIMATABLE		= -2,
		EPLAY_ANIMATION_NOT_FOUND	= -3,
		EPLAY_INVALID_PARAM			= -4,
	};


	//	Call After Updating PoseState
	FLEXKITAPI float4x4     GetJointPosed_WT( JointHandle Joint, NodeHandle Node, PoseState* DPS );
	FLEXKITAPI LineSegments BuildSkeletonLineSet            ( Skeleton* S, NodeHandle Node, iAllocator* TEMP);

    FLEXKITAPI LineSegments DEBUG_DrawPoseState             ( PoseState&, NodeHandle, iAllocator*);
	FLEXKITAPI void         DEBUG_PrintSkeletonHierarchy	( Skeleton* S);


    /************************************************************************************************/


    FLEXKITAPI Skeleton*        Resource2Skeleton(AssetHandle RHandle, iAllocator* Memory);


	/************************************************************************************************/
}
#endif
