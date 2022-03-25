#include "PCH.h"
#include "angelscript.h"
#include "ScriptedAnimationObject.h"
#include "AnimationObject.h"
#include "AnimationUtilities.h"
#include "AnimationComponents.h"


/************************************************************************************************/


void ScriptedAnimationObject::Update(AnimationObject* animatedObj, double dT)
{
    auto api_obj = static_cast<asIScriptObject*>(obj);

    if (!api_obj)
        return;

    auto method = api_obj->GetObjectType()->GetMethodByName("Update");

    ctx->Prepare(method);
    ctx->SetObject(obj);
    ctx->SetArgAddress(0, animatedObj);
    ctx->SetArgDouble(1, dT);
    ctx->Execute();
}


/************************************************************************************************/


void ScriptedAnimationObject::Reload(EditorScriptEngine& engine, AnimationObject* animatedObject)
{
    if (!ctx)
        ctx = engine.CreateContext();

    if (scriptModule)
    {   // Release Old Module
        SetArg(ctx, 0, obj);
        RunScriptFunction(ctx, scriptModule, "ReleaseModule");
        engine.ReleaseModule(scriptModule);
    }

    scriptModule = engine.BuildModule(script->source);
    if (!scriptModule)
        obj = nullptr;

    auto func = scriptModule->GetFunctionByName("InitiateModule");

    ctx->Prepare(func);
    SetArgAddress(ctx, 0, animatedObject);

    auto res = ctx->Execute();

   obj = GetReturnObject(ctx);
   auto api_obj = static_cast<asIScriptObject*>(obj);
}


/************************************************************************************************/


bool ScriptedAnimationObject::RegisterInterface(EditorScriptEngine& engine)
{
    auto api = engine.GetScriptEngine();
    int res;

    /*
    res = api->RegisterObjectType("AnimationPose", 0, asOBJ_REF | asOBJ_NOCOUNT);                                                                           FK_ASSERT(res > 0);
    res = api->RegisterObjectMethod("AnimationPose", "int GetBone(string)", asFUNCTION(GetBone), asCALL_CDECL_OBJFIRST);                                    FK_ASSERT(res > 0);

    res = api->RegisterObjectMethod("AnimationPose", "float3 GetBonePosition(int)",         asFUNCTION(GetBonePosition),    asCALL_CDECL_OBJFIRST);         FK_ASSERT(res > 0);
    res = api->RegisterObjectMethod("AnimationPose", "float3 GetBoneScale(int)",            asFUNCTION(GetBoneScale),       asCALL_CDECL_OBJFIRST);         FK_ASSERT(res > 0);
    res = api->RegisterObjectMethod("AnimationPose", "Quaternion GetBoneOrientation(int)",  asFUNCTION(GetBoneOrientation), asCALL_CDECL_OBJFIRST);         FK_ASSERT(res > 0);

    res = api->RegisterObjectMethod("AnimationPose", "void SetBonePosition(int, float3)",           asFUNCTION(SetBonePosition),    asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
    res = api->RegisterObjectMethod("AnimationPose", "void SetBoneScale(int, float3)",              asFUNCTION(SetBoneScale),       asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
    res = api->RegisterObjectMethod("AnimationPose", "void SetBoneOrientation(int, Quaternion)",    asFUNCTION(SetBoneOrientation), asCALL_CDECL_OBJFIRST); FK_ASSERT(res > 0);
    */

    res = api->RegisterInterface("AnimatorInterface");                                                                                           FK_ASSERT(res > 0);
    res = api->RegisterInterfaceMethod("AnimatorInterface", "void Update(GameObject@ object, double dt)");                                       FK_ASSERT(res > 0);

    return true;
}


/************************************************************************************************/
