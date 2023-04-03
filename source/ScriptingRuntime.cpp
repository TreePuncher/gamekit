
#include "AnimationComponents.h"
#include "MathUtils.h"
#include "ScriptingRuntime.h"
#include "RuntimeComponentIDs.h"

#include <angelscript.h>
#include <angelscript/scriptbuilder/scriptbuilder.h>
#include <angelscript/scriptany/scriptany.h>
#include <angelscript/scriptarray/scriptarray.h>
#include <angelscript/scriptstdstring/scriptstdstring.h>
#include <angelscript/scriptmath/scriptmath.h>
#include <angelscript/scriptmath/scriptmathcomplex.h>

namespace FlexKit
{   /************************************************************************************************/


	iAllocator* allocator;


	/************************************************************************************************/


	bool QueryForComponent(GameObject* obj, ComponentID id)
	{
		return obj->hasView(id);
	}


	/************************************************************************************************/


	FlexKit::PoseState* GetPoseState_AS(GameObject* obj)
	{
		return Apply(*obj,
			[](SkeletonView& view) -> FlexKit::PoseState*
			{
				return &view.GetPoseState();
			},
			[]() -> FlexKit::PoseState*
			{
				return nullptr;
			}
		);
	}

	AnimatorView* GetAnimator_AS(GameObject * obj)
	{
		return Apply(*obj,
			[](AnimatorView& view) -> AnimatorView*
			{
				return &view;
			},
			[]() -> AnimatorView*
			{
				return nullptr;
			}
			);
	}


	/************************************************************************************************/

	int AnimatorViewGetInputType_AS(AnimatorView* view, uint32_t idx)
	{
		return (int)view->GetInputType(idx).value();
	}

	void AnimatorSetFloat1_AS(AnimatorView* view, uint32_t idx, float value)
	{
		if (auto res = view->GetInputValue(idx); res.has_value())
		{
			auto animatorValue = res.value();
			animatorValue->x = value;
		}
	}

	void AnimatorSetFloat2_AS(AnimatorView* view, uint32_t idx, float2& value)
	{
		if (auto res = view->GetInputValue(idx); res.has_value())
		{
			auto animatorValue = res.value();
			animatorValue->xy = value;
		}
	}

	void AnimatorSetFloat3_AS(AnimatorView* view, uint32_t idx, float3& value)
	{
		if (auto res = view->GetInputValue(idx); res.has_value())
		{
			auto animatorValue = res.value();
			animatorValue->xyz = value;
		}
	}

	void AnimatorSetFloat4_AS(AnimatorView* view, uint32_t idx, float4& value)
	{
		if (auto res = view->GetInputValue(idx); res.has_value())
		{
			auto animatorValue  = res.value();
			animatorValue->xyzw = value;
		}
	}

	float AnimatorGetFloat1_AS(AnimatorView* view, uint32_t idx)
	{
		if (auto res = view->GetInputValue(idx); res.has_value())
		{
			auto animatorValue = res.value();
			return animatorValue->x;
		}
		else return NAN;
	}

	/*
	float2* AnimatorGetFloat2_AS(AnimatorView* view, uint32_t idx)
	{
		if (auto res = view->GetInputValue(idx); res.has_value())
		{
			auto outValue       = ConstructFloat2();
			auto animatorValue  = res.value();

			*outValue = animatorValue->xy;
			return outValue;
		}
		else return nullptr;
	}
	*/

	float3 AnimatorGetFloat3_AS(AnimatorView* view, uint32_t idx)
	{
		if (auto res = view->GetInputValue(idx); res.has_value())
			return res.value()->xyz;
		else
			return {};
	}


	float4 AnimatorGetFloat4_AS(AnimatorView* view, uint32_t idx)
	{
		if (auto res = view->GetInputValue(idx); res.has_value())
			return res.value()->xyzw;
		else
			return {};
	}

	void* AnimatorGetCallback_AS(AnimatorView* view, uint32_t idx)
	{
		auto& state = view->GetState();

		return &state.callbacks[idx];
	}

	void AnimatorCallback_Call(AnimatorCallback* view, GameObject* gameObject)
	{
		(*view)(*gameObject);
	}

	uint32_t AnimatorPlayAnimation_AS(AnimatorView* view, Animation* anim, bool loop)
	{
		return view->Play(*anim, loop);
	}


	void AnimatorStopAnimation_AS(AnimatorView* view, uint32_t playID)
	{
		view->Stop(playID);
	}


	void AnimatorPauseAnimation_AS(AnimatorView* view, uint32_t playID)
	{
		view->Pause(playID);
	}


	/************************************************************************************************/


	void AnimatorSetProgressAnimation_AS(AnimatorView* view, uint32_t playID, float p)
	{
		view->SetProgress(playID, p);
	}


	/************************************************************************************************/


	int GetBone(FlexKit::PoseState* obj, const std::string* ref)
	{
		return obj->Sk->FindJoint(ref->c_str());
	}

	int PoseStatePoseCount(FlexKit::PoseState* poseState)
	{
		return (int)poseState->poses.size();
	}

	PoseState::Pose* PoseStateGetPose(FlexKit::PoseState* poseState, int idx)
	{
		return &poseState->poses[idx];
	}


	/************************************************************************************************/


	uint32_t PoseStateCreatePose(FlexKit::PoseState* poseState, uint32_t poseID, iAllocator* allocator)
	{
		poseState->CreateSubPose(poseID, *allocator);

		return (uint32_t)(poseState->poses.size() - 1);
	}


	/************************************************************************************************/


	JointPose* PoseGetJointPose(FlexKit::PoseState::Pose* pose, uint32_t idx)
	{
		return pose->jointPose + idx;
	}


	/************************************************************************************************/


	int PoseGetJointCount(FlexKit::PoseState::Pose* pose)
	{
		return (int)pose->sk->JointCount;
	}


	/************************************************************************************************/


	float3 JointPoseGetPosition(JointPose* jp)
	{
		return jp->ts.xyz();
	}


	void JointPoseSetPosition(JointPose* jp, float3* pos)
	{
		float4 temp;
		temp = *pos;
		temp.w = jp->ts.w;
		jp->ts = temp;
	}


	/************************************************************************************************/


	float JointPoseGetScale(JointPose* jp)
	{
		return jp->ts.w;
	}


	void JointPoseSetScale(JointPose* jp, float scale)
	{
		jp->ts.w = scale;
	}


	/************************************************************************************************/


	void JointPoseSetOrientation(JointPose* jp, const Quaternion& Q)
	{
		jp->r = Q;
	}


	Quaternion JointPoseGetOrientation(JointPose* jp)
	{
		return jp->r;
	}


	/************************************************************************************************/


	Animation* LoadAnimation1_AS(GUID_t handle, iAllocator* allocator)
	{
		return LoadAnimation(handle, *allocator);
	}

	Animation* LoadAnimation2_AS(std::string& name, iAllocator* IN_allocator)
	{
		return LoadAnimation(name.c_str(), *allocator);
	}

	void ReleaseAnimation_AS(FlexKit::Animation& animation, iAllocator* allocator)
	{
		allocator->release(animation);
	};


	/************************************************************************************************/


	void LoadMorphTarget_AS(TriMesh* triMesh, std::string& morphTargetName)
	{
		//LoadMorphTarget(triMesh, morphTargetName.c_str(), , , );
	}


	/************************************************************************************************/


	PlayID_t AnimatorPlay_AS(AnimatorView* animator, Animation* animation, bool loop)
	{
		return animator->Play(*animation, loop);
	}


	/************************************************************************************************/


	auto GetWorldPosition_AS   (GameObject* obj) { return FlexKit::GetWorldPosition(*obj);	};
	auto GetLocalPosition_AS   (GameObject* obj) { return FlexKit::GetLocalPosition(*obj);	};
	auto GetOrientation_AS     (GameObject* obj) { return FlexKit::GetOrientation(*obj);	};
	auto GetScale_AS           (GameObject* obj) { return FlexKit::GetScale(*obj);			};
	
	auto SetWorldPosition_AS   (GameObject* obj, float3& v)        { FlexKit::SetWorldPosition(*obj, v); };
	auto SetLocalPosition_AS   (GameObject* obj, float3& v)        { FlexKit::SetLocalPosition(*obj, v); };
	auto SetOrientation_AS     (GameObject* obj, Quaternion& q)    { FlexKit::SetOrientation(*obj, q); };
	auto SetScale_AS           (GameObject* obj, float3& s)        { FlexKit::SetScale(*obj, s); };


	/************************************************************************************************/

	void		Log_AS(std::string& in)	{ FK_LOG_INFO(in.c_str()); }
	iAllocator* GetSystemAllocator()	{ return SystemAllocator; }

	/************************************************************************************************/

	void RegisterRuntimeAPI(asIScriptEngine* scriptEngine, RegisterFlags flags)
	{
		RegisterScriptArray(scriptEngine, true);
		RegisterScriptAny(scriptEngine);
		RegisterStdString(scriptEngine);
		RegisterStdStringUtils(scriptEngine);
		RegisterScriptMath(scriptEngine);
		RegisterScriptMathComplex(scriptEngine);


		/************************************************************************************************/

		int res = 0;

		res = scriptEngine->RegisterGlobalFunction("void Log(string& in)", asFUNCTION(Log_AS), asCALL_CDECL, allocator);  FK_ASSERT(res > 0);


		/************************************************************************************************/


		res = scriptEngine->RegisterTypedef("ComponentID_t", "uint32");
		res = scriptEngine->RegisterTypedef("JointHandle", "uint32");
		res = scriptEngine->RegisterTypedef("AssetHandle", "uint64");
		res = scriptEngine->RegisterTypedef("PlayID", "uint32");


		/************************************************************************************************/


		res = scriptEngine->RegisterEnum("ComponentID");

		res = scriptEngine->RegisterEnumValue("ComponentID", "AnimatorComponentID",           AnimatorComponentID);           FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterEnumValue("ComponentID", "BindPointComponentID",          BindPointComponentID);          FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterEnumValue("ComponentID", "BrushComponentID",              BrushComponentID);              FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterEnumValue("ComponentID", "CameraComponentID",             CameraComponentID);             FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterEnumValue("ComponentID", "FABRIKComponentID",             FABRIKComponentID);             FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterEnumValue("ComponentID", "FABRIKTargetComponentID",       FABRIKTargetComponentID);       FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterEnumValue("ComponentID", "MaterialComponentID",           MaterialComponentID);           FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterEnumValue("ComponentID", "PointLightShadowMapID",         PointLightShadowMapID);         FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterEnumValue("ComponentID", "LightComponentID",         LightComponentID);         FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterEnumValue("ComponentID", "SceneVisibilityComponentID",    SceneVisibilityComponentID);    FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterEnumValue("ComponentID", "SkeletonComponentID",           SkeletonComponentID);           FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterEnumValue("ComponentID", "StringComponentID",             StringComponentID);             FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterEnumValue("ComponentID", "TransformComponentID",          TransformComponentID);          FK_ASSERT(res >= 0);


		/************************************************************************************************/

		res = scriptEngine->RegisterEnum("AnimatorValueType");
		res = scriptEngine->RegisterEnumValue("AnimatorValueType", "Float1", (int32_t)AnimatorInputType::Float);        FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterEnumValue("AnimatorValueType", "Float2", (int32_t)AnimatorInputType::Float2);       FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterEnumValue("AnimatorValueType", "Float3", (int32_t)AnimatorInputType::Float3);       FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterEnumValue("AnimatorValueType", "Float4", (int32_t)AnimatorInputType::Float4);       FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterEnumValue("AnimatorValueType", "Uint1",  (int32_t)AnimatorInputType::Uint);         FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterEnumValue("AnimatorValueType", "Uint2",  (int32_t)AnimatorInputType::Uint2);        FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterEnumValue("AnimatorValueType", "Uint3",  (int32_t)AnimatorInputType::Uint3);        FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterEnumValue("AnimatorValueType", "Uint4",  (int32_t)AnimatorInputType::Uint4);        FK_ASSERT(res >= 0);


		res = scriptEngine->RegisterObjectType("GameObject", 0, asOBJ_REF | asOBJ_NOCOUNT);                                                                    FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterObjectType("AnimatorCallback", 0, asOBJ_REF | asOBJ_NOCOUNT);                                                              FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterObjectMethod("AnimatorCallback", "void opCall(GameObject@)", asFUNCTION(AnimatorCallback_Call), asCALL_CDECL_OBJFIRST);    FK_ASSERT(res > 0);


		/************************************************************************************************/


		res = scriptEngine->RegisterObjectType("AllocatorHandle", 0, asOBJ_REF | asOBJ_NOCOUNT);																		FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterGlobalFunction("AllocatorHandle@ GetSystemAllocator()", asFUNCTION(GetSystemAllocator), asCALL_CDECL);									FK_ASSERT(res >= 0);

		/************************************************************************************************/


		res = scriptEngine->RegisterObjectType("JointPose", 0, asOBJ_REF | asOBJ_NOCOUNT);                                                                              FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterObjectMethod("JointPose", "void SetOrientation(const Quaternion& in)",	asFUNCTION(JointPoseSetOrientation),    asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("JointPose", "void SetPosition(float3& in)",				asFUNCTION(JointPoseSetPosition),       asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("JointPose", "void SetScale(float)",						asFUNCTION(JointPoseSetScale),          asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);

		res = scriptEngine->RegisterObjectMethod("JointPose", "Quaternion	GetOrientation()",          asFUNCTION(JointPoseGetOrientation),    asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("JointPose", "float3		GetPosition()",             asFUNCTION(JointPoseGetPosition),       asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("JointPose", "float		GetScale()",                asFUNCTION(JointPoseGetScale),          asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);


		/************************************************************************************************/


		res = scriptEngine->RegisterObjectType("Pose", 0, asOBJ_REF | asOBJ_NOCOUNT);                                                                                   FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterObjectMethod("Pose", "JointPose@ GetJointPose(JointHandle)",                asFUNCTION(PoseGetJointPose),  asCALL_CDECL_OBJFIRST);  FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("Pose", "int GetJointCount()",                                 asFUNCTION(PoseGetJointCount), asCALL_CDECL_OBJFIRST);  FK_ASSERT(res > 0);


		/************************************************************************************************/


		res = scriptEngine->RegisterObjectType("PoseState", 0, asOBJ_REF | asOBJ_NOCOUNT);                                                                              FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterObjectMethod("PoseState", "int GetPoseCount()",									asFUNCTION(PoseStatePoseCount), asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("PoseState", "Pose@ GetPose(int)",									asFUNCTION(PoseStateGetPose),   asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("PoseState", "uint	 CreatePose(uint poseID, AllocatorHandle@)",	asFUNCTION(PoseStateCreatePose), asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("PoseState", "JointHandle FindJoint(string)",						asFUNCTION(GetBone),            asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);


		/************************************************************************************************/


		res = scriptEngine->RegisterObjectType("Animation", 0, asOBJ_REF | asOBJ_NOCOUNT);                                                                                  FK_ASSERT(res >= 0);

		if(!(flags & EXCLUDE_LOADANIMATION))
		{
			res = scriptEngine->RegisterGlobalFunction("Animation@	LoadAnimation(AssetHandle, AllocatorHandle@)",   asFUNCTION(LoadAnimation1_AS), asCALL_CDECL);               FK_ASSERT(res > 0);
			res = scriptEngine->RegisterGlobalFunction("Animation@	LoadAnimation(string& in)",                      asFUNCTION(LoadAnimation2_AS), asCALL_CDECL, allocator);    FK_ASSERT(res > 0);
			res = scriptEngine->RegisterGlobalFunction("void		ReleaseAnimation(Animation& in)",                asFUNCTION(ReleaseAnimation_AS), asCALL_CDECL, allocator);  FK_ASSERT(res > 0);
		}


		/************************************************************************************************/


		res = scriptEngine->RegisterObjectType("TriMesh", 0, asOBJ_REF | asOBJ_NOCOUNT);                                                                                FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterGlobalFunction("void LoadMorphTarget(TriMesh@, string& in)", asFUNCTION(LoadMorphTarget_AS), asCALL_CDECL);                         FK_ASSERT(res > 0);


		/************************************************************************************************/


		res = scriptEngine->RegisterObjectType("Animator", 0, asOBJ_REF | asOBJ_NOCOUNT);																					FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterObjectMethod("Animator", "uint ValueType(uint)",				asFUNCTION(AnimatorViewGetInputType_AS), asCALL_CDECL_OBJFIRST);		FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("Animator", "void SetFloat(uint, float)",			asFUNCTION(AnimatorSetFloat1_AS), asCALL_CDECL_OBJFIRST);				FK_ASSERT(res > 0);
		//res = scriptEngine->RegisterObjectMethod("Animator", "void SetFloat2(idx, float2)",		asFUNCTION(AnimatorSetFloat2_AS), asCALL_CDECL_OBJFIRST);				FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("Animator", "void SetFloat3(uint, float3 &in)",	asFUNCTION(AnimatorSetFloat3_AS), asCALL_CDECL_OBJFIRST);				FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("Animator", "void SetFloat4(uint, float4 &in)",	asFUNCTION(AnimatorSetFloat4_AS), asCALL_CDECL_OBJFIRST);				FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("Animator", "float  GetFloat(uint)",				asFUNCTION(AnimatorGetFloat1_AS), asCALL_CDECL_OBJFIRST);				FK_ASSERT(res > 0);
		//res = scriptEngine->RegisterObjectMethod("Animator", "float2@ GetFloat2(idx)",			asFUNCTION(AnimatorGetFloat2_AS), asCALL_CDECL_OBJFIRST);				FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("Animator", "float3 GetFloat3(uint)",				asFUNCTION(AnimatorGetFloat3_AS), asCALL_CDECL_OBJFIRST);				FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("Animator", "float4 GetFloat4(uint)",				asFUNCTION(AnimatorGetFloat4_AS), asCALL_CDECL_OBJFIRST);				FK_ASSERT(res > 0);

		res = scriptEngine->RegisterObjectMethod("Animator", "AnimatorCallback@ GetCallback(uint)",	asFUNCTION(AnimatorGetCallback_AS), asCALL_CDECL_OBJFIRST);				FK_ASSERT(res > 0);

		res = scriptEngine->RegisterObjectMethod("Animator", "PlayID Play(Animation@, bool)",		asFUNCTION(AnimatorPlayAnimation_AS), asCALL_CDECL_OBJFIRST);			FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("Animator", "void Stop(PlayID)",					asFUNCTION(AnimatorStopAnimation_AS), asCALL_CDECL_OBJFIRST);			FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("Animator", "void Pause(PlayID)",					asFUNCTION(AnimatorPauseAnimation_AS), asCALL_CDECL_OBJFIRST);			FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("Animator", "void SetProgress(PlayID, float)",		asFUNCTION(AnimatorSetProgressAnimation_AS), asCALL_CDECL_OBJFIRST);	FK_ASSERT(res > 0);


		/************************************************************************************************/


		res = scriptEngine->RegisterObjectMethod("GameObject", "bool			Query(ComponentID_t)",	asFUNCTION(QueryForComponent),		asCALL_CDECL_OBJFIRST);	FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("GameObject", "PoseState@		GetPoseState()",		asFUNCTION(GetPoseState_AS),		asCALL_CDECL_OBJFIRST);	FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("GameObject", "Animator@		GetAnimator()",			asFUNCTION(GetAnimator_AS),			asCALL_CDECL_OBJFIRST);	FK_ASSERT(res > 0);

		res = scriptEngine->RegisterObjectMethod("GameObject", "float3			GetWorldPosition()",	asFUNCTION(GetWorldPosition_AS),	asCALL_CDECL_OBJFIRST);	FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("GameObject", "float3			GetLocalPosition()",	asFUNCTION(GetLocalPosition_AS),	asCALL_CDECL_OBJFIRST);	FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("GameObject", "Quaternion		GetOrientation()",		asFUNCTION(GetOrientation_AS),		asCALL_CDECL_OBJFIRST);	FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("GameObject", "float3			GetScale()",			asFUNCTION(GetScale_AS),			asCALL_CDECL_OBJFIRST);	FK_ASSERT(res > 0);

		res = scriptEngine->RegisterObjectMethod("GameObject", "void SetWorldPosition(float3& in)",		asFUNCTION(SetWorldPosition_AS), asCALL_CDECL_OBJFIRST);	FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("GameObject", "void SetLocalPosition(float3& in)",		asFUNCTION(SetLocalPosition_AS), asCALL_CDECL_OBJFIRST);	FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("GameObject", "void SetOrientation(Quaternion& in)",	asFUNCTION(SetOrientation_AS), asCALL_CDECL_OBJFIRST);		FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("GameObject", "void SetScale(float3& in)",				asFUNCTION(SetScale_AS), asCALL_CDECL_OBJFIRST);			FK_ASSERT(res > 0);


		/************************************************************************************************/


		res = scriptEngine->RegisterInterface("AnimatorInterface");																									FK_ASSERT(res > 0);
		res = scriptEngine->RegisterInterfaceMethod("AnimatorInterface", "void PreUpdate(GameObject@ object, double dt)");											FK_ASSERT(res > 0);
		res = scriptEngine->RegisterInterfaceMethod("AnimatorInterface", "void PostUpdate(GameObject@ object, double dt)");											FK_ASSERT(res > 0);


	}   /************************************************************************************************/


	/************************************************************************************************/


	void ScopedRelease(void* s)
	{
		allocator->_aligned_free(s);
	}


	/************************************************************************************************/


	float& ScalerIndexFloat4x4(const float4x4* m, int row, int col)
	{
		return (*m)[row][col];
	}


	Vect4& VectorIndexFloat4x4(const float4x4* m, int row)
	{
		return (*m)[row];
	}


	float4x4* MulFloat4x4_1(const float4x4* lhs, const float4x4& rhs)
	{
		auto temp = new(allocator->_aligned_malloc(sizeof(float4x4))) float4x4();
		*temp = *lhs * rhs;

		return temp;
	}


	float4* MulFloat4x4_2(float4x4* lhs, float4* rhs)
	{
		auto temp = new(allocator->_aligned_malloc(sizeof(float4))) float4();

		*temp = *lhs * *rhs;

		return temp;
	}


	float4x4* MulFloat4x4_3(float4x4* lhs, float rhs)
	{
		auto temp = new(allocator->_aligned_malloc(sizeof(float4x4))) float4x4();

		*temp = *lhs * rhs;
		return temp;
	}


	float4x4* ConstructFloat4x4() noexcept
	{
		return new(allocator->_aligned_malloc(sizeof(float4x4))) float4x4();
	}


	float4x4* Float4x4Identity()
	{
		auto m44 = new(allocator->_aligned_malloc(sizeof(float4x4))) float4x4();
		*m44 = float4x4::Identity();
		return m44;
	}


	float4x4* Float4x4Inverse(float4x4* m)
	{
		auto m44 = new(allocator->_aligned_malloc(sizeof(float4x4))) float4x4();
		*m44 = Inverse(*m);
		return m44;
	}


	/************************************************************************************************/


	uint3 uint3Add(const uint3* self, const uint3& rhs)
	{
		return (*self) + rhs;
	}

	uint3 uint3Sub(const uint3* self, const uint3& rhs)
	{
		return (*self) - rhs;
	}

	uint3 uint3Mul(const uint3* self, const uint3& rhs)
	{
		return (*self) * rhs;
	}

	uint3 uint3Div(const uint3* self, const uint3& rhs)
	{
		return (*self) / rhs;
	}

	uint3 uint3AddScaler(const uint3* self, const uint32_t rhs)
	{
		return (*self) + rhs;
	}

	uint3 uint3SubScaler(const uint3* self, const uint32_t rhs)
	{
		return (*self) - rhs;
	}

	uint3 uint3MulScaler(const uint3* self, const uint32_t rhs)
	{
		return (*self) * rhs;
	}

	uint3 uint3DivScaler(const uint3* self, const uint32_t rhs)
	{
		return (*self) / rhs;
	}

	/************************************************************************************************/


	void ConstructQuaternion_2(void* _ptr, float x, float y, float z)
	{
		new(_ptr) Quaternion{ x, y, z };
	}


	void ConstructQuaternion_3(void* _ptr, float x, float y, float z, float w)
	{
		new(_ptr) Quaternion{ x, y, z, w };
	}


	/************************************************************************************************/


	void RegisterMathTypes(asIScriptEngine* scriptEngine, iAllocator* IN_allocator)
	{
		allocator = IN_allocator;
		int res;


		/************************************************************************************************/


		res = scriptEngine->RegisterObjectType("uint3", sizeof(uint3), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS);																							FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterObjectProperty("uint3", "uint32 x", 0);																																	FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterObjectProperty("uint3", "uint32 y", 4);																																	FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterObjectProperty("uint3", "uint32 z", 8);																																	FK_ASSERT(res >= 0);

		res = scriptEngine->RegisterObjectMethod("uint3", "uint3 opAdd(const uint3& in)", asFUNCTION(uint3Add), asCALL_CDECL_OBJFIRST);																		FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterObjectMethod("uint3", "uint3 opSub(const uint3& in)", asFUNCTION(uint3Sub), asCALL_CDECL_OBJFIRST);																		FK_ASSERT(res >= 0);

		res = scriptEngine->RegisterObjectMethod("uint3", "uint3 opAdd(const uint32)", asFUNCTION(uint3AddScaler), asCALL_CDECL_OBJFIRST);																	FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterObjectMethod("uint3", "uint3 opSub(const uint32)", asFUNCTION(uint3SubScaler), asCALL_CDECL_OBJFIRST);																	FK_ASSERT(res >= 0);

		res = scriptEngine->RegisterObjectMethod("uint3", "uint3 opMul(const uint3& in)", asFUNCTION(uint3Mul), asCALL_CDECL_OBJFIRST);																		FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterObjectMethod("uint3", "uint3 opDiv(const uint3& in)", asFUNCTION(uint3Div), asCALL_CDECL_OBJFIRST);																		FK_ASSERT(res >= 0);

		res = scriptEngine->RegisterObjectMethod("uint3", "uint3 opMul(const uint32)", asFUNCTION(uint3DivScaler), asCALL_CDECL_OBJFIRST);																	FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterObjectMethod("uint3", "uint3 opDiv(const uint32)", asFUNCTION(uint3MulScaler), asCALL_CDECL_OBJFIRST);


		/************************************************************************************************/


		res = scriptEngine->RegisterObjectType("float3", sizeof(float3), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS);																				FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterObjectMethod("float3", "float3& opAssign(float3)",				asMETHODPR(float3, operator =, (const float3&) noexcept, float3&), asCALL_THISCALL);			FK_ASSERT(res >= 0);

		res = scriptEngine->RegisterObjectMethod("float3", "float3 opAdd(const float3)",			asMETHODPR(float3, operator+, (const float3) const noexcept, float3),	asCALL_THISCALL);		FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float3", "float3 opSub(const float3)",			asMETHODPR(float3, operator -, (const float3) const noexcept, float3),	asCALL_THISCALL);		FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float3", "float3 opMul(const float3)",			asMETHODPR(float3, operator *, (const float3) const noexcept, float3),	asCALL_THISCALL);		FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float3", "float3 opDiv(const float3)",			asMETHODPR(float3, operator *, (const float3) const noexcept, float3),	asCALL_THISCALL);		FK_ASSERT(res > 0);

		res = scriptEngine->RegisterObjectMethod("float3", "float3& opAddAssign(const float3)",		asMETHODPR(float3, operator +=, (const float3) noexcept,	float3&),	asCALL_THISCALL);		FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float3", "float3& opAddAssign(float)",			asMETHODPR(float3, operator +=, (const float) noexcept,		float3&),	asCALL_THISCALL);		FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float3", "float3& opSubAssign(const float3)",		asMETHODPR(float3, operator -=, (const float3) noexcept,	float3&),	asCALL_THISCALL);		FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float3", "float3& opSubAssign(float)",			asMETHODPR(float3, operator -=, (const float) noexcept,		float3&),	asCALL_THISCALL);		FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float3", "float3& opMulAssign(const float3)",		asMETHODPR(float3, operator *=, (const float) noexcept,		float3&),	asCALL_THISCALL);		FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float3", "float3& opMulAssign(float)",			asMETHODPR(float3, operator *=, (const float) noexcept,		float3&),	asCALL_THISCALL);		FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float3", "float3& opDivAssign(const float3)",		asMETHODPR(float3, operator /=, (const float3) noexcept,	float3&),	asCALL_THISCALL);		FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float3", "float3& opDivAssign(float)",			asMETHODPR(float3, operator /=, (const float) noexcept,		float3&),	asCALL_THISCALL);		FK_ASSERT(res > 0);

		res = scriptEngine->RegisterObjectMethod("float3", "float3	Inverse()",				asMETHOD(float3, inverse),		asCALL_THISCALL);														FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float3", "float3	Cross(float3)",			asMETHOD(float3, cross),		asCALL_THISCALL);														FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float3", "float	Dot(float3)",			asMETHOD(float3, dot),			asCALL_THISCALL);														FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float3", "float	Magnitude()",			asMETHOD(float3, magnitude),	asCALL_THISCALL);														FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float3", "float	MagnitudeSq()",			asMETHOD(float3, magnitudeSq),	asCALL_THISCALL);														FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float3", "float3&	Abs()",					asMETHOD(float3, abs),			asCALL_THISCALL);														FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float3", "bool	isNan()",				asMETHOD(float3, isNaN),		asCALL_THISCALL);														FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float3", "void	Normalize()",			asMETHOD(float3, normalize),	asCALL_THISCALL);														FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float3", "float3&	Normal()",				asMETHOD(float3, normal),		asCALL_THISCALL);														FK_ASSERT(res > 0);

		res = scriptEngine->RegisterGlobalFunction("bool Compare(float3, float3, float)",	asFUNCTION(float3::Compare),	asCALL_CDECL);															FK_ASSERT(res > 0);
		res = scriptEngine->RegisterGlobalFunction("float3 Zero()",							asFUNCTION(float3::Zero),		asCALL_CDECL);															FK_ASSERT(res > 0);

		res = scriptEngine->RegisterObjectProperty("float3", "float x", asOFFSET(float3, x));																										FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterObjectProperty("float3", "float y", asOFFSET(float3, y));																										FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterObjectProperty("float3", "float z", asOFFSET(float3, z));																										FK_ASSERT(res >= 0);


		/************************************************************************************************/


		res = scriptEngine->RegisterObjectType("float4", sizeof(float4), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS);																				FK_ASSERT(res > 0);
		
		res = scriptEngine->RegisterObjectMethod("float4", "float4 opAdd(float4)", asMETHODPR(float4, operator +, (const float4) const noexcept, float4), asCALL_THISCALL);							FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float4", "float4 opSub(float4)", asMETHODPR(float4, operator -, (const float4) const noexcept, float4), asCALL_THISCALL);							FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float4", "float4 opMul(float4)", asMETHODPR(float4, operator *, (const float4) const noexcept, float4), asCALL_THISCALL);							FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float4", "float4 opDiv(float4)", asMETHODPR(float4, operator /, (const float4) const noexcept, float4), asCALL_THISCALL);							FK_ASSERT(res > 0);
			
		res = scriptEngine->RegisterObjectMethod("float4", "float4& opAddAssign(float4)", asMETHODPR(float4, operator *=, (const float4) noexcept, float4&), asCALL_THISCALL);				FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float4", "float4& opSubAssign(float4)", asMETHODPR(float4, operator *=, (const float4) noexcept, float4&), asCALL_THISCALL);				FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float4", "float4& opMulAssign(float4)", asMETHODPR(float4, operator *=, (const float4) noexcept, float4&), asCALL_THISCALL);				FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float4", "float4& opDivAssign(float4)", asMETHODPR(float4, operator *=, (const float4) noexcept, float4&), asCALL_THISCALL);				FK_ASSERT(res > 0);

		res = scriptEngine->RegisterObjectProperty("float4", "float x", asOFFSET(float4, x));																										FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterObjectProperty("float4", "float y", asOFFSET(float4, y));																										FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterObjectProperty("float4", "float z", asOFFSET(float4, z));																										FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterObjectProperty("float4", "float w", asOFFSET(float4, w));																										FK_ASSERT(res >= 0);


		/************************************************************************************************/


		res = scriptEngine->RegisterObjectType("Quaternion", sizeof(Quaternion), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS);																			FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectBehaviour("Quaternion", asBEHAVE_CONSTRUCT, "void ConstructQuat(float, float, float, float)",	asFUNCTION(ConstructQuaternion_3),	asCALL_CDECL_OBJFIRST);		FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectBehaviour("Quaternion", asBEHAVE_CONSTRUCT, "void ConstructQuat(float, float, float)",		asFUNCTION(ConstructQuaternion_2),	asCALL_CDECL_OBJFIRST);		FK_ASSERT(res > 0);

		res = scriptEngine->RegisterObjectMethod("Quaternion", "Quaternion	opMul(Quaternion)",		asMETHODPR(Quaternion, operator *, (const Quaternion) const	noexcept,	Quaternion),	asCALL_THISCALL);		FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("Quaternion", "float3		opMul(float3)",			asFUNCTIONPR(operator *, (const Quaternion, const float3)	noexcept,	float3),		asCALL_CDECL_OBJFIRST);	FK_ASSERT(res > 0);

		res = scriptEngine->RegisterObjectMethod("Quaternion", "Quaternion& opAssign(Quaternion)",			asMETHODPR(Quaternion, operator =,	(const Quaternion) noexcept, Quaternion&),	asCALL_THISCALL);		FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("Quaternion", "Quaternion& opMulAssign(Quaternion)",		asMETHODPR(Quaternion, operator *=,	(const Quaternion) noexcept, Quaternion&),	asCALL_THISCALL);		FK_ASSERT(res > 0);

		res = scriptEngine->RegisterObjectMethod("Quaternion", "Quaternion  Conjugate()",		asMETHOD(Quaternion, Conjugate),	asCALL_THISCALL);													FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("Quaternion", "float Dot(Quaternion& in)",		asMETHOD(Quaternion, dot),			asCALL_THISCALL);													FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("Quaternion", "float Magnitude()",				asMETHOD(Quaternion, Magnitude),	asCALL_THISCALL);													FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("Quaternion", "Quaternion&		Normalize()",	asMETHOD(Quaternion, normalize),	asCALL_THISCALL);													FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("Quaternion", "Quaternion		Normal()",		asMETHOD(Quaternion, normal),		asCALL_THISCALL);													FK_ASSERT(res > 0);

		res = scriptEngine->RegisterObjectProperty("Quaternion", "float x", asOFFSET(Quaternion, x));																									FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterObjectProperty("Quaternion", "float y", asOFFSET(Quaternion, y));																									FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterObjectProperty("Quaternion", "float z", asOFFSET(Quaternion, z));																									FK_ASSERT(res >= 0);
		res = scriptEngine->RegisterObjectProperty("Quaternion", "float w", asOFFSET(Quaternion, w));																									FK_ASSERT(res >= 0);


		/************************************************************************************************/


		res = scriptEngine->RegisterObjectType("float4x4", sizeof(float4x4), asOBJ_REF | asOBJ_SCOPED);																									FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectBehaviour("float4x4", asBEHAVE_FACTORY, "float4x4@ ConstructF4x4()",	asFUNCTION(ConstructFloat4x4),	asCALL_CDECL);											FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectBehaviour("float4x4", asBEHAVE_RELEASE, "void f()",					asFUNCTION(ScopedRelease),		asCALL_CDECL_OBJFIRST);									FK_ASSERT(res > 0);

		res = scriptEngine->RegisterObjectMethod("float4x4", "float4&   opIndex(int)",			asFUNCTION(VectorIndexFloat4x4),	asCALL_CDECL_OBJFIRST);												FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float4x4", "float&    opIndex(int, int)",		asFUNCTION(ScalerIndexFloat4x4),	asCALL_CDECL_OBJFIRST);												FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float4x4", "float4x4@ opMul(float4x4& in)",	asFUNCTION(MulFloat4x4_1),			asCALL_CDECL_OBJFIRST);												FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float4x4", "float4&   opMul(float4& in)",		asFUNCTION(MulFloat4x4_2),			asCALL_CDECL_OBJFIRST);												FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float4x4", "float4x4@ opMul(float)",			asFUNCTION(MulFloat4x4_3),			asCALL_CDECL_OBJFIRST);												FK_ASSERT(res > 0);
		res = scriptEngine->RegisterObjectMethod("float4x4", "float4x4@ Inverse()",				asFUNCTION(Float4x4Inverse),		asCALL_CDECL_OBJFIRST);												FK_ASSERT(res > 0);

		res = scriptEngine->RegisterGlobalFunction("float4x4@ Identity()", asFUNCTION(Float4x4Identity), asCALL_CDECL);																					FK_ASSERT(res > 0);


		/************************************************************************************************/


		res = scriptEngine->RegisterGlobalFunction("Quaternion Vector2Quat(const float3& in, const float3& in, const float3& in)", asFUNCTION(Vector2Quaternion), asCALL_CDECL);						FK_ASSERT(res > 0);


	}	/************************************************************************************************/


	/************************************************************************************************/


	ScriptResourceBlob::ScriptResourceBlob(size_t byteCodeSize) :
		blobSize{ byteCodeSize }
	{
		ResourceSize	= sizeof(ScriptResourceBlob) + byteCodeSize;
		Type			= EResourceType::EResource_ByteCode;
		State			= Resource::ResourceState::EResourceState_UNLOADED;		// Runtime Member
	}


	/************************************************************************************************/


	asIScriptEngine* scriptEngine = nullptr;

	std::mutex                                       m;
	FlexKit::CircularBuffer<asIScriptContext*, 128>  contexts;

	void DefaultMessageCallback(const asSMessageInfo* msg, void* param)
	{
		const char* type = "ERR ";
		if (msg->type == asMSGTYPE_WARNING)
			type = "WARN";
		else if (msg->type == asMSGTYPE_INFORMATION)
			type = "INFO";

		FK_LOG_INFO("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
	}

	void InitiateScriptRuntime()
	{
		scriptEngine = asCreateScriptEngine();

		for (size_t I = 0; I < 128; I++)
			contexts.push_back(scriptEngine->CreateContext());

		scriptEngine->SetMessageCallback(asFUNCTION(DefaultMessageCallback), nullptr, asCALL_CDECL);
	}


	/************************************************************************************************/


	void ReleaseScriptRuntime()
	{
		if (scriptEngine)
			scriptEngine->ShutDownAndRelease();

		scriptEngine = nullptr;
	}

	/************************************************************************************************/


	void AddGlobal(const char* str, void*)
	{

	}

	/************************************************************************************************/


	void ReleaseGlobal(const char* str)
	{

	}


	/************************************************************************************************/


	class BytecodeStream : public asIBinaryStream
	{
	public:
		BytecodeStream(const char* IN_byteCode, size_t IN_size)
			: byteCode      { IN_byteCode   }
			, streamSize    { IN_size       }{}

		int Write(const void* ptr, asUINT size) { return size; }

		int Read(void* ptr, asUINT size)
		{
			if (size == 0 || offset + size > streamSize)
				return 0;

			memcpy(ptr, byteCode + offset, size);
			offset += size;

			return size;
		}

		const char* byteCode;
		size_t      streamSize;
		size_t      offset = 0;
	};


	/************************************************************************************************/


	asIScriptModule* LoadScriptFile(const char* moduleName, std::filesystem::path path, iAllocator& tempAllocator)
	{
		auto test = path.is_relative();

		if (!std::filesystem::exists(path))
			return nullptr;

		const auto fileSize = std::filesystem::file_size(path);
		auto fileBuffer = (byte*)tempAllocator.malloc(fileSize);

		auto cPath = path.string();
		if (!FlexKit::LoadFileIntoBuffer(cPath.c_str(), fileBuffer, fileSize))
			return nullptr;

		CScriptBuilder builder{};
		builder.StartNewModule(scriptEngine, moduleName);

		if (auto r = builder.AddSectionFromMemory(moduleName, (const char*)fileBuffer, fileSize); r < 0)
			return nullptr;

		if (auto r = builder.BuildModule(); r < 0)
			return nullptr;

		auto asModule = builder.GetModule();

		return asModule;
	}


	/************************************************************************************************/


	asIScriptModule* LoadByteCode(const char* moduleName, const char* byteCode, size_t streamSize)
	{
		std::scoped_lock l{ m };

		auto scriptModule = scriptEngine->GetModule(moduleName, asGM_CREATE_IF_NOT_EXISTS);

		BytecodeStream byteStream{ byteCode, streamSize };

		auto res = scriptModule->LoadByteCode(&byteStream);

		if (res >= 0)
			return scriptModule;
		else
			return nullptr;
	}


	/************************************************************************************************/


	asIScriptModule* LoadByteCodeAsset(uint64_t assetID)
	{
		char moduleName[32];
		sprintf(moduleName, "%I64u", assetID);

		auto module_ptr = scriptEngine->GetModule(moduleName, asGM_ONLY_IF_EXISTS);

		if (module_ptr)
			return module_ptr;

		auto assetHandle = FlexKit::LoadGameAsset(assetID);
		if (assetHandle == -1)
			return nullptr;

		auto asset_ptr  = static_cast<ScriptResourceBlob*>(GetAsset(assetHandle));
		EXITSCOPE(FlexKit::FreeAsset(assetHandle));

		if (asset_ptr->Type != FlexKit::EResource_ByteCode)
			return nullptr;

		char* buffer    = ((char*)asset_ptr) + sizeof(ScriptResourceBlob);
		size_t blobSize = asset_ptr->blobSize;
		auto module     = LoadByteCode(moduleName, buffer, blobSize);

		return module;
	}


	/************************************************************************************************/


	asIScriptEngine* GetScriptEngine()
	{
		return scriptEngine;
	}


	/************************************************************************************************/


	asIScriptContext* GetContext()
	{
		std::scoped_lock l{ m };
		return contexts.pop_front();
	}


	/************************************************************************************************/


	void ReleaseContext(asIScriptContext* ctx)
	{
		std::scoped_lock l{ m };
		contexts.push_back(ctx);
	}


}   /************************************************************************************************/


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
