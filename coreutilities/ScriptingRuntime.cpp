
#include "AnimationComponents.h"
#include "MathUtils.h"
#include "ScriptingRuntime.h"
#include "RuntimeComponentIDs.h"

#include <angelscript.h>

namespace FlexKit
{   /************************************************************************************************/


    iAllocator* allocator;


    //float2* ConstructFloat2();
    float3* ConstructFloat3() noexcept;
    float4* ConstructFloat4() noexcept;

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

    float3* AnimatorGetFloat3_AS(AnimatorView* view, uint32_t idx)
    {
        if (auto res = view->GetInputValue(idx); res.has_value())
        {
            auto outValue = ConstructFloat3();
            auto animatorValue = res.value();

            *outValue = animatorValue->xyz;
            return outValue;
        }
        else return nullptr;
    }

    float4* AnimatorGetFloat4_AS(AnimatorView* view, uint32_t idx)
    {
        if (auto res = view->GetInputValue(idx); res.has_value())
        {
            auto outValue = ConstructFloat4();
            auto animatorValue = res.value();

            *outValue = animatorValue->xyzw;
            return outValue;
        }
        else return nullptr;
    }


    /************************************************************************************************/


    int GetBone(FlexKit::PoseState* obj, const std::string* ref)
    {
        return obj->Sk->FindJoint(ref->c_str());
    }

    int PoseStatePoseCount(FlexKit::PoseState* poseState)
    {
        return 0;
    }

    PoseState::Pose* PoseStateGetPose(FlexKit::PoseState* poseState, int idx)
    {
        return &poseState->poses[idx];
    }


    /************************************************************************************************/


    JointPose* PoseGetJointPose(FlexKit::PoseState::Pose* pose, int idx)
    {
        return pose->jointPose + idx;
    }


    /************************************************************************************************/


    int PoseGetJointCount(FlexKit::PoseState::Pose* pose)
    {
        return pose->sk->JointCount;
    }


    /************************************************************************************************/


    float3* JointPoseGetPosition(JointPose* jp)
    {
        auto& vec = allocator->allocate_aligned<float3>();
        vec = jp->ts.xyz();

        return &vec;
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


    void JointPoseSetOrientation(JointPose* jp, const Quaternion* Q)
    {
        jp->r = *Q;
    }


    Quaternion* JointPoseGetOrientation(JointPose* jp)
    {
        auto& p = allocator->allocate_aligned<Quaternion>();
        p = jp->r;

        return &p;
    }


    /************************************************************************************************/


    Animation* LoadAnimation_AS(GUID_t handle, iAllocator* allocator)
    {
        return LoadAnimation(handle, *allocator);
    }


    /************************************************************************************************/


    PlayID_t AnimatorPlay_AS(AnimatorView* animator, Animation* animation, bool loop)
    {
        return animator->Play(*animation, loop);
    }


    /************************************************************************************************/


    void RegisterGameObjectCore(asIScriptEngine* scriptEngine)
    {
        int res = 0;
        res = scriptEngine->RegisterTypedef("ComponentID_t", "uint32");
        res = scriptEngine->RegisterTypedef("BoneHandle", "uint32");
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
        res = scriptEngine->RegisterEnumValue("ComponentID", "PointLightComponentID",         PointLightComponentID);         FK_ASSERT(res >= 0);
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
        res = scriptEngine->RegisterEnumValue("AnimatorValueType", "Uint1", (int32_t)AnimatorInputType::Uint);          FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterEnumValue("AnimatorValueType", "Uint2", (int32_t)AnimatorInputType::Uint2);         FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterEnumValue("AnimatorValueType", "Uint3", (int32_t)AnimatorInputType::Uint3);         FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterEnumValue("AnimatorValueType", "Uint4", (int32_t)AnimatorInputType::Uint4);         FK_ASSERT(res >= 0);


        /************************************************************************************************/


        res = scriptEngine->RegisterObjectType("AllocatorHandle", 0, asOBJ_REF | asOBJ_NOCOUNT);                                                                        FK_ASSERT(res >= 0);

        res = scriptEngine->RegisterObjectType("JointPose", 0, asOBJ_REF | asOBJ_NOCOUNT);                                                                              FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectMethod("JointPose", "void SetOrientation(Quaternion& in)",    asFUNCTION(JointPoseSetOrientation),    asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("JointPose", "void SetPosition(float3& in)",           asFUNCTION(JointPoseSetPosition),       asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("JointPose", "void SetScale(float)",                   asFUNCTION(JointPoseSetScale),          asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);

        res = scriptEngine->RegisterObjectMethod("JointPose", "Quaternion@  GetOrientation()",          asFUNCTION(JointPoseGetOrientation),    asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("JointPose", "float3@      GetPosition()",             asFUNCTION(JointPoseGetPosition),       asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("JointPose", "float        GetScale()",                asFUNCTION(JointPoseGetScale),          asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);


        /************************************************************************************************/


        res = scriptEngine->RegisterObjectType("Pose", 0, asOBJ_REF | asOBJ_NOCOUNT);                                                                                   FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectMethod("Pose", "JointPose@ GetJointPose(int)",                        asFUNCTION(PoseGetJointPose),  asCALL_CDECL_OBJFIRST);  FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("Pose", "int GetJointCount()",                                 asFUNCTION(PoseGetJointCount), asCALL_CDECL_OBJFIRST);  FK_ASSERT(res > 0);


        /************************************************************************************************/


        res = scriptEngine->RegisterObjectType("PoseState", 0, asOBJ_REF | asOBJ_NOCOUNT);                                                                              FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectMethod("PoseState", "int GetPoseCount()",                             asFUNCTION(PoseStatePoseCount), asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("PoseState", "Pose@ GetPose(int)",                             asFUNCTION(PoseStateGetPose),   asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("PoseState", "BoneHandle FindBone(string)",                    asFUNCTION(GetBone),            asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);


        /************************************************************************************************/


        res = scriptEngine->RegisterObjectType("Animation", 0, asOBJ_REF | asOBJ_NOCOUNT);                                                                              FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterGlobalFunction("Animation@ LoadAnimation(AssetHandle, AllocatorHandle@)",   asFUNCTION(LoadAnimation_AS), asCALL_CDECL);            FK_ASSERT(res > 0);

        /************************************************************************************************/


        res = scriptEngine->RegisterObjectType("Animator", 0, asOBJ_REF | asOBJ_NOCOUNT);                                                                               FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectMethod("Animator", "uint ValueType(uint)",            asFUNCTION(AnimatorViewGetInputType_AS), asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("Animator", "void SetFloat(uint, float)",      asFUNCTION(AnimatorSetFloat1_AS), asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        //res = scriptEngine->RegisterObjectMethod("Animator", "void SetFloat2(idx, float2)",    asFUNCTION(AnimatorSetFloat2_AS), asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("Animator", "void SetFloat3(uint, float3 &in)",asFUNCTION(AnimatorSetFloat3_AS), asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("Animator", "void SetFloat4(uint, float4 &in)",asFUNCTION(AnimatorSetFloat4_AS), asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("Animator", "float  GetFloat(uint)",           asFUNCTION(AnimatorGetFloat1_AS), asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        //res = scriptEngine->RegisterObjectMethod("Animator", "float2@ GetFloat2(idx)",           asFUNCTION(AnimatorGetFloat2_AS), asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("Animator", "float3@ GetFloat3(uint)",          asFUNCTION(AnimatorGetFloat3_AS), asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("Animator", "float4@ GetFloat4(uint)",          asFUNCTION(AnimatorGetFloat4_AS), asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        
        //res = scriptEngine->RegisterObjectMethod("Animator", "PlayID Play(Animation@)",                         asFUNCTION(PoseStatePoseCount), asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);


        /************************************************************************************************/


        res = scriptEngine->RegisterObjectType("GameObject", 0, asOBJ_REF | asOBJ_NOCOUNT);                                                                             FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectMethod("GameObject", "bool            Query(ComponentID_t)",          asFUNCTION(QueryForComponent), asCALL_CDECL_OBJFIRST);  FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("GameObject", "PoseState@      GetPoseState()",                asFUNCTION(GetPoseState_AS),   asCALL_CDECL_OBJFIRST);  FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("GameObject", "Animator@       GetAnimator()",                 asFUNCTION(GetAnimator_AS),   asCALL_CDECL_OBJFIRST);   FK_ASSERT(res > 0);

        int x = 0;
    }


    /************************************************************************************************/


    void ScopedRelease(void* s)
    {
        allocator->_aligned_free(s);
    }

    float3* ConstructFloat3() noexcept
    {
        return new(allocator->_aligned_malloc(sizeof(float3))) float3{};
    }

    float3* ConstructFloat3_2(float x, float y, float z)
    {
        return new(allocator->_aligned_malloc(sizeof(float3))) float3(x, y, z);
    }

    Quaternion* ConstructQuaternion()
    {
        return new(allocator->_aligned_malloc(sizeof(Quaternion))) Quaternion{};
    }

    Quaternion* ConstructQuaternion_2(float x, float y, float z, float w, iAllocator* allocator)
    {
        return new(allocator->_aligned_malloc(sizeof(Quaternion))) Quaternion{ x, y, z, w };
    }

    Quaternion* ConstructQuaternion_3(float x, float y, float z)
    {
        return new(allocator->_aligned_malloc(sizeof(Quaternion))) Quaternion{ x, y, z };
    }

    Quaternion* QuaternionMul(Quaternion* lhs, Quaternion* rhs)
    {
        auto Q = ConstructQuaternion();

        *Q = *lhs * *rhs;

        return Q;
    }

    float3* QMULF3(Quaternion* lhs, float3* rhs)
    {
        auto out = ConstructFloat3();
        *out = *lhs * *rhs;

        return out;
    }

    Quaternion& QuatAssign_AS(Quaternion* lhs, float3* rhs)
    {
        *lhs = *rhs;
        return *lhs;
    }

    Quaternion& QuatMulAssign_AS(Quaternion* lhs, Quaternion* rhs)
    {
        *lhs *= *rhs;
        return *lhs;
    }

    Quaternion* QuatConjugate_AS(Quaternion* lhs)
    {
        auto Q = ConstructQuaternion();

        *Q = lhs->Conjugate();
        return Q;
    }

    float QuatDot_AS(Quaternion* lhs, Quaternion* rhs)
    {
        return lhs->dot(*rhs);
    }

    float QuatMag_AS(Quaternion* lhs, Quaternion* rhs)
    {
        return lhs->Magnitude();
    }

    Quaternion* QuatNormalize_AS(Quaternion* lhs)
    {
        lhs->normalize();

        return lhs;
    }

    Quaternion* QuatNormal_AS(Quaternion* lhs)
    {
        auto Q = ConstructQuaternion();

        *Q = lhs->normal();

        return Q;
    }


    /************************************************************************************************/


    float3* float3_OpAssign(float3* lhs, float3* rhs)
    {
        *lhs = *rhs;
        return lhs;
    }


    /************************************************************************************************/


    float3* float3_OpAdd(float3* lhs, float3* rhs)
    {
        auto out = new(allocator->_aligned_malloc(sizeof(float3))) float3{};

        *out = *lhs + *rhs;

        return out;
    }


    /************************************************************************************************/


    float3* float3_OpSub(float3* lhs, float3* rhs)
    {
        auto out = new(allocator->_aligned_malloc(sizeof(float3))) float3{};

        *out = *lhs - *rhs;

        return out;
    }


    /************************************************************************************************/


    float3* float3_OpMul(float3* lhs, float3* rhs)
    {
        auto out = new(allocator->_aligned_malloc(sizeof(float3))) float3{};

        *out = *lhs * *rhs;

        return out;
    }

    /************************************************************************************************/


    float3* float3_OpDiv(float3* lhs, float3* rhs)
    {
        auto out = new(allocator->_aligned_malloc(sizeof(float3))) float3{};

        *out = *lhs / *rhs;

        return out;
    }


    /************************************************************************************************/


    float3& float3_OpAddAssign1(float3* lhs, float3* rhs)
    {
        *lhs = *lhs + *rhs;

        return *lhs;
    }


    /************************************************************************************************/


    float3& float3_OpAddAssign2(float3* lhs, float rhs)
    {
        *lhs = *lhs + rhs;

        return *lhs;
    }


    /************************************************************************************************/


    float3& float3_OpSubAssign1(float3* lhs, float3* rhs)
    {
        *lhs = *lhs - *rhs;

        return *lhs;
    }


    /************************************************************************************************/


    float3& float3_OpSubAssign2(float3* lhs, float rhs)
    {
        *lhs = *lhs - rhs;

        return *lhs;
    }

    /************************************************************************************************/


    float3& float3_OpMulAssign1(float3* lhs, float3* rhs)
    {
        *lhs = *lhs * *rhs;

        return *lhs;
    }


    /************************************************************************************************/


    float3& float3_OpMulAssign2(float3* lhs, float rhs)
    {
        *lhs *= rhs;

        return *lhs;
    }

    /************************************************************************************************/


    float3& float3_OpDivAssign1(float3* lhs, float3* rhs)
    {
        *lhs = *lhs / *rhs;

        return *lhs;
    }


    /************************************************************************************************/


    float3& float3_OpDivAssign2(float3* lhs, float rhs)
    {
        *lhs /= rhs;

        return *lhs;
    }


    /************************************************************************************************/


    float3* float3_Inverse(float3* lhs)
    {
        auto out = new(allocator->_aligned_malloc(sizeof(float3))) float3{};

        *out = lhs->inverse();

        return out;
    }


    /************************************************************************************************/


    float3* float3_Cross(float3* lhs, float3* rhs)
    {
        auto out = new(allocator->_aligned_malloc(sizeof(float3))) float3{};

        *out = lhs->cross(*rhs);

        return out;
    }


    /************************************************************************************************/


    float float3_Dot(float3* lhs, float3* rhs)
    {
        return lhs->dot(*rhs);
    }

    /************************************************************************************************/


    float float3_Magnitude(float3* lhs)
    {
        return lhs->magnitude();
    }


    /************************************************************************************************/


    float float3_MagnitudeSq(float3* lhs)
    {
        return lhs->magnitudeSq();
    }


    /************************************************************************************************/


    float3* float3_Abs(float3* lhs)
    {
        auto out = new(allocator->_aligned_malloc(sizeof(float3))) float3{};
        *out = lhs->abs();
        return out;
    }


    /************************************************************************************************/


    bool float3_IsNan(float3* lhs)
    {
        return lhs->isNaN();
    }


    /************************************************************************************************/


    float3* float3_Normalize(float3* lhs)
    {
        lhs->normalize();
        return lhs;
    }

    /************************************************************************************************/


    float3* float3_Normal(float3* lhs)
    {
        auto out = new(allocator->_aligned_malloc(sizeof(float3))) float3{};
        *out = lhs->normal();

        return out;
    }


    bool float3_Compare(float3* lhs, float3* rhs, float e)
    {
        return float3::Compare(*lhs, *rhs, e);
    }


    float3* float3_Zero()
    {
        auto out = new(allocator->_aligned_malloc(sizeof(float3))) float3{};
        *out = float3::Zero();

        return out;
    }


    /************************************************************************************************/


    float4* ConstructFloat4() noexcept
    {
        return new(allocator->_aligned_malloc(16)) float4();

    }

    float4* ConstructFloat4_2(float x, float y, float z, float w) noexcept
    {
        return new(allocator->_aligned_malloc(16)) float4(x, y, z, w);
    }


    float4* float4Add(float4* lhs, float4& rhs) noexcept
    {
        auto f4 = ConstructFloat4();
        *f4 = *lhs + rhs;

        return f4;
    }

    float4* float4Sub(float4* lhs, float4& rhs) noexcept
    {
        auto f4 = ConstructFloat4();
        *f4 = *lhs - rhs;

        return f4;
    }

    float4* float4Mul(float4* lhs, float4& rhs) noexcept
    {
        auto f4 = ConstructFloat4();
        *f4 = *lhs * rhs;

        return f4;
    }

    float4* float4Div(float4* lhs, float4& rhs) noexcept
    {
        auto f4 = ConstructFloat4();
        *f4 = *lhs / rhs;

        return f4;
    }

    /************************************************************************************************/

    float4& float4AddAssign(float4* lhs, float4& rhs) noexcept
    {
        *lhs = *lhs + rhs;

        return *lhs;
    }

    float4& float4SubAssign(float4* lhs, float4& rhs) noexcept
    {
        *lhs = *lhs - rhs;

        return *lhs;
    }

    float4& float4MulAssign(float4* lhs, float4& rhs) noexcept
    {
        *lhs = *lhs * rhs;

        return *lhs;
    }

    float4& float4DivAssign(float4* lhs, float4& rhs) noexcept
    {
        *lhs = *lhs / rhs;

        return *lhs;
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


    void RegisterMathTypes(asIScriptEngine* scriptEngine, iAllocator* IN_allocator)
    {
        allocator = IN_allocator;
        int res;
        
        res = scriptEngine->RegisterObjectType("float3", 0, asOBJ_REF | asOBJ_SCOPED );                                                                                                                     FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectBehaviour("float3", asBEHAVE_FACTORY, "float3@ ConstructFloat3()",                     asFUNCTION(ConstructFloat3),    asCALL_CDECL);                             FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectBehaviour("float3", asBEHAVE_FACTORY, "float3@ ConstructFloat3(float, float, float)",  asFUNCTION(ConstructFloat3_2),  asCALL_CDECL);                             FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectBehaviour("float3", asBEHAVE_RELEASE, "void ReleaseFloat3()",                          asFUNCTION(ScopedRelease),      asCALL_CDECL_OBJFIRST);                    FK_ASSERT(res >= 0);

        res = scriptEngine->RegisterObjectMethod("float3", "float3@ opAssign(const float3 &in)",      asFUNCTION(float3_OpAssign), asCALL_CDECL_OBJFIRST);                                                  FK_ASSERT(res >= 0);

        res = scriptEngine->RegisterObjectMethod("float3", "float3@ opAdd(const float3 &in)",         asFUNCTION(float3_OpAdd), asCALL_CDECL_OBJFIRST);                                                     FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3@ opSub(const float3 &in)",         asFUNCTION(float3_OpSub), asCALL_CDECL_OBJFIRST);                                                     FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3@ opMul(const float3 &in)",         asFUNCTION(float3_OpMul), asCALL_CDECL_OBJFIRST);                                                     FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3@ opDiv(const float3 &in)",         asFUNCTION(float3_OpDiv), asCALL_CDECL_OBJFIRST);                                                     FK_ASSERT(res > 0);

        res = scriptEngine->RegisterObjectMethod("float3", "float3& opAddAssign(const float3 &in)",     asFUNCTION(float3_OpAddAssign1), asCALL_CDECL_OBJFIRST);                                            FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3& opAddAssign(float)",                asFUNCTION(float3_OpAddAssign2), asCALL_CDECL_OBJFIRST);                                            FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3& opSubAssign(const float3 &in)",     asFUNCTION(float3_OpSubAssign1), asCALL_CDECL_OBJFIRST);                                            FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3& opSubAssign(float)",                asFUNCTION(float3_OpSubAssign2), asCALL_CDECL_OBJFIRST);                                            FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3& opMulAssign(const float3 &in)",     asFUNCTION(float3_OpMulAssign1), asCALL_CDECL_OBJFIRST);                                            FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3& opMulAssign(float)",                asFUNCTION(float3_OpMulAssign2), asCALL_CDECL_OBJFIRST);                                            FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3& opDivAssign(const float3 &in)",     asFUNCTION(float3_OpDivAssign1), asCALL_CDECL_OBJFIRST);                                            FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3& opDivAssign(float)",                asFUNCTION(float3_OpDivAssign2), asCALL_CDECL_OBJFIRST);                                            FK_ASSERT(res > 0);

        res = scriptEngine->RegisterObjectMethod("float3", "float3@ Inverse()",             asFUNCTION(float3_Inverse),       asCALL_CDECL_OBJFIRST);                                                       FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3@ Cross(float3& in)",     asFUNCTION(float3_Cross),         asCALL_CDECL_OBJFIRST);                                                       FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float   Dot(float3& in)",       asFUNCTION(float3_Dot),           asCALL_CDECL_OBJFIRST);                                                       FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float   Magnitude()",           asFUNCTION(float3_Magnitude),     asCALL_CDECL_OBJFIRST);                                                       FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float   MagnitudeSq()",         asFUNCTION(float3_MagnitudeSq),   asCALL_CDECL_OBJFIRST);                                                       FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3@ Abs()",                 asFUNCTION(float3_Abs),           asCALL_CDECL_OBJFIRST);                                                       FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3@ isNan()",               asFUNCTION(float3_IsNan),         asCALL_CDECL_OBJFIRST);                                                       FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "void    Normalize()",           asFUNCTION(float3_Normalize),     asCALL_CDECL_OBJFIRST);                                                       FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3@ Normal()",              asFUNCTION(float3_Normal),        asCALL_CDECL_OBJFIRST);                                                       FK_ASSERT(res > 0);

        res = scriptEngine->RegisterGlobalFunction("bool Compare(const float3 &in, const float3 &in, float e)", asFUNCTION(float3_Compare), asCALL_CDECL);                                                  FK_ASSERT(res > 0);
        res = scriptEngine->RegisterGlobalFunction("float3@ Zero()",                                            asFUNCTION(float3_Zero), asCALL_CDECL, allocator);                                          FK_ASSERT(res > 0);

        res = scriptEngine->RegisterObjectProperty("float3", "float x", asOFFSET(float3, x));                                                                                                               FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectProperty("float3", "float y", asOFFSET(float3, y));                                                                                                               FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectProperty("float3", "float z", asOFFSET(float3, z));                                                                                                               FK_ASSERT(res >= 0);

        /************************************************************************************************/

        res = scriptEngine->RegisterObjectType("float4", 0, asOBJ_REF | asOBJ_SCOPED);                                                                                                                      FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectBehaviour("float4", asBEHAVE_FACTORY, "float4@ ConstructFloat4()",                             asFUNCTION(ConstructFloat4),    asCALL_CDECL);                     FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectBehaviour("float4", asBEHAVE_FACTORY, "float4@ ConstructFloat4(float, float, float, float)",   asFUNCTION(ConstructFloat4_2),  asCALL_CDECL);                     FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectBehaviour("float4", asBEHAVE_RELEASE, "void f()",                                              asFUNCTION(ScopedRelease),     asCALL_CDECL_OBJFIRST);             FK_ASSERT(res > 0);

        res = scriptEngine->RegisterObjectMethod("float4", "float4@ opAdd(float4& in)", asFUNCTION(float4Add), asCALL_CDECL_OBJFIRST);                                                                      FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float4", "float4@ opSub(float4& in)", asFUNCTION(float4Sub), asCALL_CDECL_OBJFIRST);                                                                      FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float4", "float4@ opMul(float4& in)", asFUNCTION(float4Mul), asCALL_CDECL_OBJFIRST);                                                                      FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float4", "float4@ opDiv(float4& in)", asFUNCTION(float4Div), asCALL_CDECL_OBJFIRST);                                                                      FK_ASSERT(res > 0);

        res = scriptEngine->RegisterObjectMethod("float4", "float4& opAddAssign(float4& in)", asFUNCTION(float4AddAssign), asCALL_CDECL_OBJFIRST);                                                          FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float4", "float4& opSubAssign(float4& in)", asFUNCTION(float4SubAssign), asCALL_CDECL_OBJFIRST);                                                          FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float4", "float4& opMulAssign(float4& in)", asFUNCTION(float4MulAssign), asCALL_CDECL_OBJFIRST);                                                          FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float4", "float4& opDivAssign(float4& in)", asFUNCTION(float4DivAssign), asCALL_CDECL_OBJFIRST);                                                          FK_ASSERT(res > 0);

        res = scriptEngine->RegisterObjectProperty("float4", "float x", asOFFSET(float4, x));                                                                                                               FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectProperty("float4", "float y", asOFFSET(float4, y));                                                                                                               FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectProperty("float4", "float z", asOFFSET(float4, z));                                                                                                               FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectProperty("float4", "float w", asOFFSET(float4, w));                                                                                                               FK_ASSERT(res >= 0);

        /************************************************************************************************/

        res = scriptEngine->RegisterObjectType("Quaternion", sizeof(Quaternion), asOBJ_REF | asOBJ_SCOPED);                                                                                                 FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectBehaviour("Quaternion", asBEHAVE_FACTORY, "Quaternion@ ConstructQuat(float, float, float, float)", asFUNCTION(ConstructQuaternion_2), asCALL_CDECL);              FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectBehaviour("Quaternion", asBEHAVE_FACTORY, "Quaternion@ ConstructQuat(float, float, float)",        asFUNCTION(ConstructQuaternion_3), asCALL_CDECL);              FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectBehaviour("Quaternion", asBEHAVE_FACTORY, "Quaternion@ ConstructQuat()",                           asFUNCTION(ConstructQuaternion), asCALL_CDECL);                FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectBehaviour("Quaternion", asBEHAVE_RELEASE, "void f()",                                              asFUNCTION(ScopedRelease), asCALL_CDECL_OBJFIRST);             FK_ASSERT(res > 0);

        res = scriptEngine->RegisterObjectMethod("Quaternion", "Quaternion@  opMul(Quaternion& in)",   asFUNCTION(QuaternionMul),   asCALL_CDECL_OBJFIRST);                                                 FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("Quaternion", "float3@      opMul(float3& in)",       asFUNCTION(QMULF3),          asCALL_CDECL_OBJFIRST);                                                 FK_ASSERT(res > 0);

        res = scriptEngine->RegisterObjectMethod("Quaternion", "Quaternion& opAssign(Quaternion& in)",    asFUNCTION(QuatAssign_AS), asCALL_CDECL_OBJFIRST);                                                FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("Quaternion", "Quaternion& opMulAssign(Quaternion& in)", asFUNCTION(QuatMulAssign_AS), asCALL_CDECL_OBJFIRST);                                             FK_ASSERT(res > 0);

        res = scriptEngine->RegisterObjectMethod("Quaternion", "Quaternion@  Conjugate()",              asFUNCTION(QuatConjugate_AS), asCALL_CDECL_OBJFIRST);                                               FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("Quaternion", "float Dot(Quaternion& in)",             asFUNCTION(QuatDot_AS), asCALL_CDECL_OBJFIRST);                                                     FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("Quaternion", "float Magnitude()",                     asFUNCTION(QuatMag_AS), asCALL_CDECL_OBJFIRST);                                                     FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("Quaternion", "Quaternion@  Normalize()",              asFUNCTION(QuatNormalize_AS), asCALL_CDECL_OBJFIRST);                                               FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("Quaternion", "Quaternion@  Normal()",                 asFUNCTION(QuatNormal_AS), asCALL_CDECL_OBJFIRST);                                                  FK_ASSERT(res > 0);

        res = scriptEngine->RegisterObjectProperty("Quaternion", "float x", asOFFSET(Quaternion, x));                                                                                                       FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectProperty("Quaternion", "float y", asOFFSET(Quaternion, y));                                                                                                       FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectProperty("Quaternion", "float z", asOFFSET(Quaternion, z));                                                                                                       FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectProperty("Quaternion", "float w", asOFFSET(Quaternion, w));                                                                                                       FK_ASSERT(res >= 0);

        /************************************************************************************************/

        res = scriptEngine->RegisterObjectType("float4x4", sizeof(float4x4), asOBJ_REF | asOBJ_SCOPED);                                                                                                     FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectBehaviour("float4x4", asBEHAVE_FACTORY, "float4x4@ ConstructF4x4()",  asFUNCTION(ConstructFloat4x4),  asCALL_CDECL);                                              FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectBehaviour("float4x4", asBEHAVE_RELEASE, "void f()",                   asFUNCTION(ScopedRelease),      asCALL_CDECL_OBJFIRST);                                     FK_ASSERT(res > 0);

        res = scriptEngine->RegisterObjectMethod("float4x4", "float4&   opIndex(int)",          asFUNCTION(VectorIndexFloat4x4),    asCALL_CDECL_OBJFIRST);                                                 FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float4x4", "float&    opIndex(int, int)",     asFUNCTION(ScalerIndexFloat4x4),    asCALL_CDECL_OBJFIRST);                                                 FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float4x4", "float4x4@ opMul(float4x4& in)",   asFUNCTION(MulFloat4x4_1),          asCALL_CDECL_OBJFIRST);                                                 FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float4x4", "float4@   opMul(float4& in)",     asFUNCTION(MulFloat4x4_2),          asCALL_CDECL_OBJFIRST);                                                 FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float4x4", "float4x4@ opMul(float)",          asFUNCTION(MulFloat4x4_3),          asCALL_CDECL_OBJFIRST);                                                 FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float4x4", "float4x4@ Inverse()",             asFUNCTION(Float4x4Inverse),        asCALL_CDECL_OBJFIRST);                                                 FK_ASSERT(res > 0);

        res = scriptEngine->RegisterGlobalFunction("float4x4@ Identity()", asFUNCTION(Float4x4Identity), asCALL_CDECL);                                                                                     FK_ASSERT(res > 0);


    }   /************************************************************************************************/


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
