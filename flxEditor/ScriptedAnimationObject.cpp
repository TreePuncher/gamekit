#include "PCH.h"
#include "angelscript.h"
#include "ScriptedAnimationObject.h"
#include "AnimationObject.h"
#include "AnimationUtilities.h"
#include "AnimationComponents.h"
#include <fmt/format.h>
#include <scn/scn.h>

/************************************************************************************************/


std::string AnimationInput::ValueToString() const noexcept
{
    switch (type)
    {
        case InputType::Float:
            return fmt::format("{}", x);
        case InputType::Float2:
            return fmt::format("{}, {}", xy[0], xy[1]);
        case InputType::Float3:
            return fmt::format("{}, {}, {}", xyz[0], xyz[1], xyz[2]);
        case InputType::Float4:
            return fmt::format("{}, {}, {}", xyzw[0], xyzw[1], xyzw[2], xyzw[3]);
        case InputType::Uint:
            return fmt::format("{}", a);
        case InputType::Uint2:
            return fmt::format("{}, {}", ab[0], ab[1]);
        case InputType::Uint3:
            return fmt::format("{}, {}, {}", abc[0], abc[1], abc[2]);
        case InputType::Uint4:
            return fmt::format("{}, {}, {}, {}", abcd[0], abcd[1], abcd[2], abcd[3]);
        default:
            return "";
    }
}


/************************************************************************************************/


void AnimationInput::StringToValue(const std::string& in) noexcept
{
    switch (type)
    {
    case InputType::Float:
    {
        scn::scan(in, "{}", x);
    }   break;
    case InputType::Float2:
    {
        scn::scan(in, "{}, {}", xy.x, xy.y);
    }   break;
    case InputType::Float3:
    {
        scn::scan(in, "{}, {}, {}", xyz.x, xyz.y, xyz.z);
    }   break;
    case InputType::Float4:
    {
        scn::scan(in, "{}, {}, {}, {}", xyzw.x, xyzw.y, xyzw.z, xyzw.w);
    }   break;
    case InputType::Uint:
    {
        scn::scan(in, "{}", a);
    }   break;
    case InputType::Uint2:
    {
        scn::scan(in, "{}, {}", ab[0], ab[1]);
    }   break;
    case InputType::Uint3:
    {
        scn::scan(in, "{}, {}, {}", abc[0], abc[1], abc[2]);
    }   break;
    case InputType::Uint4:
    {
        scn::scan(in, "{}, {}, {}, {}", abcd[0], abcd[1], abcd[2], abcd[3]);
    }   break;
    default:
        return;
    }
}


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
    {
        obj = nullptr;
        return;
    }

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
