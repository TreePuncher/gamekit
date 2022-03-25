
#include "AnimationComponents.h"
#include "MathUtils.h"
#include "ScriptingRuntime.h"
#include <angelscript.h>

namespace FlexKit
{   /************************************************************************************************/


    float3 GetBonePosition(void*, int)
    {
        return {};
    }

    float3 GetBoneScale(void*, int)
    {
        return {};
    }

    Quaternion GetBoneOrientation(void*, int)
    {
        return {};
    }

    void SetBonePosition(void*, int, float3)
    {

    }

    void SetBoneScale(void*, int, float3)
    {

    }

    void SetBoneOrientation(void*, int, Quaternion)
    {

    }


    /************************************************************************************************/


    int GetBone(GameObject* obj, const std::string* ref)
    {
        return GetJoint(*obj, ref->c_str());
    }

    int GetPoseCount(GameObject* obj)
    {
        return 0;
    }

    void* GetPose(GameObject* obj)
    {
        return nullptr;
    }


    /************************************************************************************************/


    void RegisterGameObjectCore(asIScriptEngine* scriptEngine)
    {
        int res = 0;
        res = scriptEngine->RegisterObjectType("GameObject", 0, asOBJ_REF | asOBJ_NOCOUNT);                                                                         FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectMethod("GameObject", "int             GetBone(string)",               asFUNCTION(GetBone),        asCALL_CDECL_OBJFIRST); FK_ASSERT(res >= 0);

        /*
        res = scriptEngine->RegisterObjectMethod("GameObject", "bool            HasAnimationComponent(string)", asFUNCTION(GetBone),        asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("GameObject", "int             GetPoseCount()",                asFUNCTION(GetPoseCount),   asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("GameObject", "AnimationPose@  GetPose(int)",                  asFUNCTION(GetPose),        asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("GameObject", "bool            HasTransformComponent()",       asFUNCTION(GetBone),        asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("GameObject", "float3          GetPositionW()",                asFUNCTION(GetBone),        asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("GameObject", "float3          GetPositionL()",                asFUNCTION(GetBone),        asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("GameObject", "float3          GetOrientation()",              asFUNCTION(GetBone),        asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("GameObject", "float3          GetOrientationL()",             asFUNCTION(GetBone),        asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("GameObject", "int             GetParentNode()",               asFUNCTION(GetBone),        asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
        */

        int x = 0;
    }


    /************************************************************************************************/


    void ConstructFloat3(void* _ptr)
    {
    }

    void ConstructFloat3_2(float x, float y, float z, void* _ptr)
    {
        new(_ptr) float3(x, y, z);
    }

    void ConstructQuaternion(void* _ptr)
    {
    }

    void ConstructQuaternion_2(float x, float y, float z, float w, void* _ptr)
    {
        new(_ptr) Quaternion(x, y, z, w);
    }

    float3 QMULF3(Quaternion* lhs, float3& rhs)
    {
        return *lhs * rhs;
    }

    float4 float4Mul(float4* lhs, float4& rhs) noexcept
    {
        return *lhs * rhs;
    }

    void ConstructFloat4(void* _ptr) noexcept
    {
    }

    void ConstructFloat4_2(float x, float y, float z, float w, void* _ptr) noexcept
    {
        new(_ptr) float4(x, y, z, w);
    }

    float& ScalerIndexFloat4x4(const float4x4* m, int row, int col)
    {
        return (*m)[row][col];
    }

    Vect4& VectorIndexFloat4x4(const float4x4* m, int row)
    {
        return (*m)[row];
    }

    float4x4 MulFloat4x4_1(const float4x4* lhs, const float4x4& rhs)
    {
        return (*lhs) * rhs;
    }

    float4 MulFloat4x4_2(float4x4* lhs, float4& rhs)
    {
        return (*lhs) * rhs;
    }

    float4x4 MulFloat4x4_3(float4x4* lhs, float rhs)
    {
        return (*lhs) * rhs;
    }

    void ConstructFloat4x4(void* _ptr) noexcept
    {
    }

    float4x4 Float4x4Identity()
    {
        return float4x4::Identity();
    }

    float4x4 Float4x4Inverse(float4x4* m)
    {
        return Inverse(*m);
    }

    void RegisterMathTypes(asIScriptEngine* scriptEngine)
    {
        int res;
        
        res = scriptEngine->RegisterObjectType("float3", sizeof(float3), asOBJ_VALUE | asOBJ_APP_CLASS_ALLFLOATS | asOBJ_APP_CLASS_C | asOBJ_POD);                                                          FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectBehaviour("float3", asBEHAVE_CONSTRUCT, "void ConstructFloat3(float, float, float)",  asFUNCTION(ConstructFloat3_2),  asCALL_CDECL_OBJLAST);                      FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectBehaviour("float3", asBEHAVE_CONSTRUCT, "void ConstructFloat3()",                     asFUNCTION(ConstructFloat3),    asCALL_CDECL_OBJLAST);                      FK_ASSERT(res > 0);

        res = scriptEngine->RegisterObjectMethod("float3", "float3 opAdd(float3& in)", asMETHODPR(float3, operator +, (const float3&) const, float3), asCALL_THISCALL);                                     FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3 opSub(float3& in)", asMETHODPR(float3, operator -, (const float3&) const, float3), asCALL_THISCALL);                                     FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3 opMul(float3& in)", asMETHODPR(float3, operator *, (const float3&) const, float3), asCALL_THISCALL);                                     FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3 opDiv(float3& in)", asMETHODPR(float3, operator /, (const float3&) const, float3), asCALL_THISCALL);                                     FK_ASSERT(res > 0);

        res = scriptEngine->RegisterObjectMethod("float3", "float3& opAssign(float3& in)",    asMETHODPR(float3, operator =,  (const float3&), float3&), asCALL_THISCALL);                                  FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3& opAddAssign(float3& in)", asMETHODPR(float3, operator +=, (const float3&), float3&), asCALL_THISCALL);                                  FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3& opAddAssign(float)",      asMETHODPR(float3, operator +=, (const float),   float3&), asCALL_THISCALL);                                  FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3& opSubAssign(float3& in)", asMETHODPR(float3, operator -=, (const float3&), float3&), asCALL_THISCALL);                                  FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3& opSubAssign(float)",      asMETHODPR(float3, operator -=, (const float),   float3&), asCALL_THISCALL);                                  FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3& opMulAssign(float3& in)", asMETHODPR(float3, operator *=, (const float3&), float3&), asCALL_THISCALL);                                  FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3& opMulAssign(float)",      asMETHODPR(float3, operator *=, (const float),   float3&), asCALL_THISCALL);                                  FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3& opDivAssign(float3& in)", asMETHODPR(float3, operator /=, (const float3&), float3&), asCALL_THISCALL);                                  FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3& opDivAssign(float)",      asMETHODPR(float3, operator /=, (const float),   float3&), asCALL_THISCALL);                                  FK_ASSERT(res > 0);

        res = scriptEngine->RegisterObjectMethod("float3", "float3 Inverse()",               asMETHODPR(float3, inverse, () const noexcept, const float3),              asCALL_THISCALL);                   FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3 Cross(const float3& in)", asMETHODPR(float3, cross, (const float3&) const noexcept, const float3),   asCALL_THISCALL);                   FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3 Dot(const float3& in)",   asMETHODPR(float3, dot, (const float3&) const noexcept, float),            asCALL_THISCALL);                   FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3 Magnitude()",             asMETHODPR(float3, magnitude, () const noexcept, float),                   asCALL_THISCALL);                   FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3 MagnitudeSq()",           asMETHODPR(float3, magnitudeSq, () const noexcept, float),                 asCALL_THISCALL);                   FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3 Abs()",                   asMETHODPR(float3, abs, () const noexcept, float3),                        asCALL_THISCALL);                   FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3 isNan()",                 asMETHODPR(float3, isNaN, () const noexcept, bool),                        asCALL_THISCALL);                   FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "void   Normalize()",             asMETHODPR(float3, normalize, () noexcept, void),                          asCALL_THISCALL);                   FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float3", "float3 Normal()",                asMETHODPR(float3, normal, () const noexcept, float3),                     asCALL_THISCALL);                   FK_ASSERT(res > 0);

        res = scriptEngine->RegisterGlobalFunction("bool Compare(const float3& in, const float3& in, float e)", asFUNCTIONPR(float3::Compare, (const float3&, const float3&, float), bool), asCALL_CDECL);  FK_ASSERT(res > 0);
        res = scriptEngine->RegisterGlobalFunction("float3 Zero()", asFUNCTIONPR(float3::Zero, (), float3), asCALL_CDECL);                                                                                  FK_ASSERT(res > 0);

        res = scriptEngine->RegisterObjectProperty("float3", "float x", asOFFSET(float3, x)); FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectProperty("float3", "float y", asOFFSET(float3, y)); FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectProperty("float3", "float z", asOFFSET(float3, z)); FK_ASSERT(res >= 0);

        /************************************************************************************************/

        res = scriptEngine->RegisterObjectType("float4", sizeof(float4), asOBJ_VALUE | asOBJ_APP_CLASS_ALLFLOATS | asOBJ_APP_CLASS_C | asOBJ_POD);                                                          FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectBehaviour("float4", asBEHAVE_CONSTRUCT, "void ConstructFloat4(float, float, float, float)",   asFUNCTION(ConstructFloat4_2),  asCALL_CDECL_OBJLAST);                      FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectBehaviour("float4", asBEHAVE_CONSTRUCT, "void ConstructFloat4()",                             asFUNCTION(ConstructFloat4),    asCALL_CDECL_OBJLAST);                      FK_ASSERT(res > 0);

        res = scriptEngine->RegisterObjectMethod("float4", "float4 opAdd(float4& in)", asMETHODPR(float4, operator +, (const float4&) const, float4), asCALL_THISCALL);                                     FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float4", "float4 opSub(float4& in)", asMETHODPR(float4, operator -, (const float4&) const, float4), asCALL_THISCALL);                                     FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float4", "float4 opMul(float4& in)", asFUNCTION(float4Mul), asCALL_CDECL_OBJFIRST);                                                                       FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float4", "float4 opDiv(float4& in)", asMETHODPR(float4, operator /, (const float4&) const, float4), asCALL_THISCALL);                                     FK_ASSERT(res > 0);

        res = scriptEngine->RegisterObjectProperty("float4", "float x", asOFFSET(float4, x)); FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectProperty("float4", "float y", asOFFSET(float4, y)); FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectProperty("float4", "float z", asOFFSET(float4, z)); FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectProperty("float4", "float w", asOFFSET(float4, w)); FK_ASSERT(res >= 0);

        /************************************************************************************************/

        res = scriptEngine->RegisterObjectType("Quaternion", sizeof(Quaternion), asOBJ_VALUE | asOBJ_APP_CLASS_ALLFLOATS | asOBJ_APP_CLASS_C | asOBJ_POD);                                                  FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectBehaviour("Quaternion", asBEHAVE_CONSTRUCT, "void ConstructQuat(float, float, float, float)", asFUNCTION(ConstructQuaternion_2), asCALL_CDECL_OBJLAST);           FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectBehaviour("Quaternion", asBEHAVE_CONSTRUCT, "void ConstructQuat()",                           asFUNCTION(ConstructQuaternion), asCALL_CDECL_OBJLAST);             FK_ASSERT(res > 0);

        res = scriptEngine->RegisterObjectMethod("Quaternion", "Quaternion  opMul(Quaternion)",   asMETHODPR(Quaternion, operator *, (const Quaternion) const, Quaternion), asCALL_THISCALL);               FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("Quaternion", "float3      opMul(float3& in)",   asFUNCTION(QMULF3), asCALL_CDECL_OBJFIRST);                                                               FK_ASSERT(res > 0);

        res = scriptEngine->RegisterObjectMethod("Quaternion", "Quaternion& opAssign(Quaternion& in)",    asMETHODPR(Quaternion, operator =,  (const Quaternion&),  Quaternion&), asCALL_THISCALL);         FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("Quaternion", "Quaternion& opMulAssign(Quaternion& in)", asMETHODPR(Quaternion, operator *=, (const Quaternion&),  Quaternion&), asCALL_THISCALL);         FK_ASSERT(res > 0);

        res = scriptEngine->RegisterObjectMethod("Quaternion", "Quaternion  Conjugate()",             asMETHODPR(Quaternion, Conjugate, ()           const noexcept, Quaternion),    asCALL_THISCALL);      FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("Quaternion", "Quaternion  Dot(Quaternion& in)",     asMETHODPR(Quaternion, dot, (Quaternion rhs)   const noexcept, float),         asCALL_THISCALL);      FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("Quaternion", "Quaternion  Magnitude()",             asMETHODPR(Quaternion, Magnitude, ()           const noexcept, float),         asCALL_THISCALL);      FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("Quaternion", "Quaternion& Normalize()",             asMETHODPR(Quaternion, normalize, ()                 noexcept, Quaternion&),   asCALL_THISCALL);      FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("Quaternion", "Quaternion  Normal()",                asMETHODPR(Quaternion, normal, ()              const noexcept, Quaternion),    asCALL_THISCALL);      FK_ASSERT(res > 0);

        res = scriptEngine->RegisterObjectProperty("Quaternion", "float x", asOFFSET(Quaternion, x)); FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectProperty("Quaternion", "float y", asOFFSET(Quaternion, y)); FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectProperty("Quaternion", "float z", asOFFSET(Quaternion, z)); FK_ASSERT(res >= 0);
        res = scriptEngine->RegisterObjectProperty("Quaternion", "float w", asOFFSET(Quaternion, w)); FK_ASSERT(res >= 0);

        /************************************************************************************************/

        res = scriptEngine->RegisterObjectType("float4x4", sizeof(float4x4), asOBJ_VALUE | asOBJ_APP_CLASS_ALLFLOATS | asOBJ_APP_CLASS_C | asOBJ_POD);                                                    FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectBehaviour("float4x4", asBEHAVE_CONSTRUCT, "void ConstructF4x4()", asFUNCTION(ConstructFloat4x4), asCALL_CDECL_OBJLAST);                                           FK_ASSERT(res > 0);

        res = scriptEngine->RegisterObjectMethod("float4x4", "float4&   opIndex(int)",          asFUNCTION(VectorIndexFloat4x4),    asCALL_CDECL_OBJFIRST);                                                               FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float4x4", "float&    opIndex(int, int)",     asFUNCTION(ScalerIndexFloat4x4),    asCALL_CDECL_OBJFIRST);                                                               FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float4x4", "float4x4  opMul(float4x4& in)",   asFUNCTION(MulFloat4x4_1),          asCALL_CDECL_OBJFIRST);                                                               FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float4x4", "float4    opMul(float4& in)",     asFUNCTION(MulFloat4x4_2),          asCALL_CDECL_OBJFIRST);                                                               FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float4x4", "float4x4  opMul(float)",          asFUNCTION(MulFloat4x4_3),          asCALL_CDECL_OBJFIRST);                                                               FK_ASSERT(res > 0);
        res = scriptEngine->RegisterObjectMethod("float4x4", "float4x4  Inverse()",             asFUNCTION(Float4x4Inverse),        asCALL_CDECL_OBJFIRST);                                                               FK_ASSERT(res > 0);

        res = scriptEngine->RegisterGlobalFunction("float4x4 Identity()", asFUNCTION(Float4x4Identity), asCALL_CDECL);                                                               FK_ASSERT(res > 0);

        /************************************************************************************************/
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
