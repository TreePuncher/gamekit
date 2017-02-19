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
#include "..\\coreutilities\MathUtils.h"
#include "..\\coreutilities\memoryutilities.h"
#include "..\\coreutilities\Resources.h"

#include "graphics.h"

#include <DirectXMath.h>
#include <functional>

// TODOs:
//	Additive Animation
//	Curve Driven Animation


namespace FlexKit
{
	using DirectX::XMMATRIX;

	/************************************************************************************************/

	typedef uint16_t	JointHandle;

	struct Resources;
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
		JointPose() {}
		JointPose(Quaternion Q, float4 TS) : r(Q), ts(TS) {}

		Quaternion	r;
		float4		ts;
	};


	FLEXKITAPI XMMATRIX		GetTransform(JointPose P);
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


	struct PoseState_DESC
	{
		size_t JointCount;
	};

	struct DrawablePoseState
	{
		FrameBufferedResource	Resource;
		JointPose*				Joints		= nullptr;
		DirectX::XMMATRIX*		CurrentPose	= nullptr;
		Skeleton*				Sk			= nullptr;
		size_t					JointCount	= 0;
		size_t					Dirty		= 0;
		size_t					padding[2];
	};

	struct AnimationStateEntry
	{
		AnimationClip*	Clip;
		double			T;
		double			Speed;
		float			Weight;
		int32_t			order;
		int64_t			ID;
		uint8_t			Playing;
		bool			ForceLoop;

		enum
		{
			Skeletal,
			Simulation,
			Vertex,
		}TargetType;

		byte			Pad[38];

	};
	struct DrawableAnimationState
	{
		static_vector<AnimationStateEntry>	Clips; // Animations in
		DrawablePoseState*					Target;
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
	FLEXKITAPI float EaseOut_Square	(float Weight);

	struct Pose {
		size_t		JointCount;
		float4x4*	JointPoses;

		iAllocator* Memory;
	};


	typedef uint16_t DAStateHandle;
	typedef uint16_t DAConditionHandle;

	struct AnimationStateMachine
	{
		uint16_t StateCount;
		uint16_t ConditionCount;

		uint16_t MaxConditionCount;
		uint16_t MaxStateCount;

		size_t TargetDrawable;

		struct AnimationState
		{
			bool			Enabled;
			bool			Active;
			bool			Loop;		// Loops Animation While Active
			bool			ForceComplete;

			EASE_FN			EaseOut;

			WeightFunction		In;
			WeightFunction		Out;
			//byte				Pad;
			float				Speed;
			float				EaseOutDuration;	// ratio from the end
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
		DrawablePoseState*		Target;
	};


	/************************************************************************************************/


	struct AnimationStateEntry_Desc
	{
		EASE_FN			EaseOut				= EaseOut_Square;

		WeightFunction		In					= EWF_Ramp;
		WeightFunction		Out					= EWF_Ramp;
		float				Speed				= 1.0f;
		float				EaseOutDuration		= 0.0f;
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


	struct GraphicScene;
	FLEXKITAPI void UpdateASM(double dt, AnimationStateMachine* ASM, iAllocator* TempMemory, iAllocator* Memory, GraphicScene* Scene);

	/************************************************************************************************/


	inline void RotateJoint			( DrawablePoseState* E, JointHandle J, Quaternion Q )
	{
		if (!E)
			return;

		if (J != -1 && J < E->JointCount) {
			E->Joints[J].r = DirectX::XMQuaternionMultiply(E->Joints[J].r, Q);
			E->Dirty = true;
		}
	}

	inline void TranslateJoint		( DrawablePoseState* PS, JointHandle J, float3 xyz )
	{
		if (!PS)
			return;

		if (J != -1) {
			float4 xyz(xyz, 0);
			PS->Joints[J].ts = PS->Joints[J].ts + xyz;
			PS->Dirty = true;
		}
	}

	inline void SetJointTranslation ( DrawablePoseState* E, JointHandle J, float3 xyz )
	{
		if (!E)
			return;

		if (J != -1) {
			float s = E->Joints[J].ts.w;
			E->Joints[J].ts = float4( xyz.x, xyz.y, xyz.z, s);
			E->Dirty = true;
		}
	}

	inline void ScaleJoint			( DrawablePoseState* E, JointHandle J, float S )
	{
		if (!E)
			return;

		if (J!= -1)
			E->Joints[J].ts.w *= S;
	}


	/************************************************************************************************/


	FLEXKITAPI DrawablePoseState*	CreatePoseState		( Drawable* E, GeometryTable* GT, iAllocator* MEM );
	FLEXKITAPI bool					InitiatePoseState	( RenderSystem* RS, DrawablePoseState* EAS, PoseState_DESC& Desc, VShaderJoint* Initial );
	FLEXKITAPI void					InitiateASM			( AnimationStateMachine* AS, iAllocator*, size_t Target );
	FLEXKITAPI void					Release				( DrawablePoseState* EAS );


	/************************************************************************************************/


	enum EPLAY_ANIMATION_RES
	{
		EPLAY_SUCCESS				=  0,
		EPLAY_FAILED				= -1,
		EPLAY_NOT_ANIMATABLE		= -2,
		EPLAY_ANIMATION_NOT_FOUND	= -3,
		EPLAY_INVALID_PARAM			= -4,
	};


	FLEXKITAPI EPLAY_ANIMATION_RES PlayAnimation			( Drawable* E, GeometryTable* GT,	GUID_t		Guid,			iAllocator* MEM, bool ForceLoop = false, float Weight = 1.0f, int64_t* IDOut = nullptr );
	FLEXKITAPI EPLAY_ANIMATION_RES PlayAnimation			( Drawable* E, GeometryTable* GT,	const char* AnimationID,	iAllocator* MEM, bool ForceLoop = false, float Weight = 1.0f, int64_t* IDOut = nullptr );
	FLEXKITAPI EPLAY_ANIMATION_RES PlayAnimation			( Drawable* E, GeometryTable* GT,	GUID_t		Guid,			iAllocator* MEM, bool ForceLoop, float Weight,				  int64_t& out );

	FLEXKITAPI EPLAY_ANIMATION_RES SetAnimationSpeed		( DrawableAnimationState* AE,		int64_t		ID,				double Speed = 1.0f );
	FLEXKITAPI EPLAY_ANIMATION_RES SetAnimationSpeed		( DrawableAnimationState* AE,		GUID_t		AnimationID,	double Speed = 1.0f );
	FLEXKITAPI EPLAY_ANIMATION_RES SetAnimationSpeed		( DrawableAnimationState* AE,		const char*	AnimationID,	double Speed = 1.0f );

	FLEXKITAPI EPLAY_ANIMATION_RES SetAnimationWeight		( DrawableAnimationState* AE,		int64_t		ID,				float Weight = 1.0f );
	FLEXKITAPI EPLAY_ANIMATION_RES SetAnimationWeight		( DrawableAnimationState* AE,		GUID_t		AnimationID,	float Weight = 1.0f );
	FLEXKITAPI EPLAY_ANIMATION_RES SetAnimationWeight		( DrawableAnimationState* AE,		const char*	AnimationID,	float Weight = 1.0f );

	FLEXKITAPI EPLAY_ANIMATION_RES StopAnimation			( Drawable* E, GeometryTable* GT,	GUID_t		Guid);
	FLEXKITAPI EPLAY_ANIMATION_RES StopAnimation			( Drawable* E, GeometryTable* GT,	const char*	Animation);
	FLEXKITAPI EPLAY_ANIMATION_RES StopAnimation			( Drawable* E, GeometryTable* GT,	uint64_t	ID);
	FLEXKITAPI EPLAY_ANIMATION_RES ClearAnimationPose		( DrawablePoseState* DPS, iAllocator* TEMP );
	
	FLEXKITAPI size_t				GetAnimationPlayingCount	( Drawable* E );
	FLEXKITAPI AnimationStateEntry*	GetAnimation				( DrawableAnimationState* AS, int64_t ID );
	FLEXKITAPI double				GetAnimation_Completed		( AnimationStateEntry* AS );

	FLEXKITAPI bool isStillPlaying( DrawableAnimationState* AS, int64_t );


	// Advance skip updating Joint, updates timers
	FLEXKITAPI void UpdateAnimation	( RenderSystem* RS, Drawable* E,		GeometryTable* GT, double dT, iAllocator* TEMP, bool AdvanceOnly = false );
	FLEXKITAPI void UploadPose		( RenderSystem* RS, Drawable* E,		GeometryTable* GT, iAllocator* TEMP);
	FLEXKITAPI void UploadPoses		( RenderSystem* RS, PVS* Drawables,	GeometryTable* GT, iAllocator* TEMP);

	//	Call After Updating PoseState
	FLEXKITAPI float4x4 GetJointPosed_WT( JointHandle Joint, NodeHandle Node, SceneNodes* Nodes, DrawablePoseState* DPS );

	FLEXKITAPI void DEBUG_DrawSkeleton				( Skeleton* S,				SceneNodes* Nodes, NodeHandle Node, iAllocator* TEMP, LineSet* Out );
	FLEXKITAPI void DEBUG_DrawPoseState				( DrawablePoseState* DPS,	SceneNodes* Nodes, NodeHandle Node, LineSet* Out ) ;
	FLEXKITAPI void DEBUG_PrintSkeletonHierarchy	( Skeleton* S);
}
#endif
