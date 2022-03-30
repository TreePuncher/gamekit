#include "PCH.h"
#include "angelscript.h"
#include "ScriptedAnimationObject.h"
#include "AnimationObject.h"
#include "AnimationUtilities.h"
#include "AnimationComponents.h"
#include <fmt/format.h>
#include <scn/scn.h>


/************************************************************************************************/


std::string ValueToString(FlexKit::AnimatorComponent::InputValue& input, const FlexKit::AnimatorInputType type) noexcept
{
    switch (type)
    {
    case FlexKit::AnimatorInputType::Float:
        return fmt::format("{}", input.x);
    case FlexKit::AnimatorInputType::Float2:
        return fmt::format("{}, {}", input.xy[0], input.xy[1]);
    case FlexKit::AnimatorInputType::Float3:
        return fmt::format("{}, {}, {}", input.xyz[0], input.xyz[1], input.xyz[2]);
    case FlexKit::AnimatorInputType::Float4:
        return fmt::format("{}, {}, {}", input.xyzw[0], input.xyzw[1], input.xyzw[2], input.xyzw[3]);
    case FlexKit::AnimatorInputType::Uint:
        return fmt::format("{}", input.a);
    case FlexKit::AnimatorInputType::Uint2:
        return fmt::format("{}, {}", input.ab[0], input.ab[1]);
    case FlexKit::AnimatorInputType::Uint3:
        return fmt::format("{}, {}, {}", input.abc[0], input.abc[1], input.abc[2]);
    case FlexKit::AnimatorInputType::Uint4:
        return fmt::format("{}, {}, {}, {}", input.abcd[0], input.abcd[1], input.abcd[2], input.abcd[3]);
        default:
            return "";
    }
}


/************************************************************************************************/


void StringToValue(const std::string& in, FlexKit::AnimatorComponent::InputValue& input, const FlexKit::AnimatorInputType type) noexcept
{
    switch (type)
    {
    case FlexKit::AnimatorInputType::Float:
    {
        scn::scan(in, "{}", input.x);
    }   break;
    case FlexKit::AnimatorInputType::Float2:
    {
        scn::scan(in, "{}, {}", input.xy.x, input.xy.y);
    }   break;
    case FlexKit::AnimatorInputType::Float3:
    {
        scn::scan(in, "{}, {}, {}", input.xyz.x, input.xyz.y, input.xyz.z);
    }   break;
    case FlexKit::AnimatorInputType::Float4:
    {
        scn::scan(in, "{}, {}, {}, {}", input.xyzw.x, input.xyzw.y, input.xyzw.z, input.xyzw.w);
    }   break;
    case FlexKit::AnimatorInputType::Uint:
    {
        scn::scan(in, "{}", input.a);
    }   break;
    case FlexKit::AnimatorInputType::Uint2:
    {
        scn::scan(in, "{}, {}", input.ab[0], input.ab[1]);
    }   break;
    case FlexKit::AnimatorInputType::Uint3:
    {
        scn::scan(in, "{}, {}, {}", input.abc[0], input.abc[1], input.abc[2]);
    }   break;
    case FlexKit::AnimatorInputType::Uint4:
    {
        scn::scan(in, "{}, {}, {}, {}", input.abcd[0], input.abcd[1], input.abcd[2], input.abcd[3]);
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

    scriptModule = engine.BuildModule(resource->source);
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


uint32_t ScriptedAnimationObject::AddInputValue(FlexKit::GameObject& obj, const std::string& name, uint32_t valueType)
{
    return FlexKit::Apply(
        obj,
        [&](FlexKit::AnimatorView& animator) -> uint32_t
        {
            FlexKit::AnimatorComponent::InputID ID;
            FlexKit::AnimatorComponent::InputValue value;

            strncpy_s(ID.stringID, name.c_str(), FlexKit::Min(sizeof(ID.stringID), name.size()));
            ID.type = (FlexKit::AnimatorInputType)valueType;

            animator.GetState().inputIDs.push_back(ID);
            animator.GetState().inputValues.push_back(value);

            AnimationInput animationValue;
            animationValue.type      = (AnimationInput::InputType)valueType;
            animationValue.stringID  = name;

            resource->inputs.push_back(animationValue);

            return resource->inputs.size() - 1;
        },
        []() -> uint32_t {return -1; });

}


/************************************************************************************************/


std::string ScriptedAnimationObject::ValueString(FlexKit::GameObject& obj, uint32_t idx, uint32_t valueType)
{
    return FlexKit::Apply(
        obj,
        [&](FlexKit::AnimatorView& animator) -> std::string
        {
            auto value      = animator.GetInputValue(idx).value();
            auto valueType  = animator.GetInputType(idx).value();

            return ValueToString(
                *value,
                (FlexKit::AnimatorInputType)valueType);
        },
        []() -> std::string
        {
            return std::string("");
        });
}


void ScriptedAnimationObject::UpdateValue(FlexKit::GameObject& obj, uint32_t idx, const std::string& valueString)
{
    FlexKit::Apply(
        obj,
        [&](FlexKit::AnimatorView& animator)
        {
            auto value      = animator.GetInputValue(idx).value();
            auto valueType  = animator.GetInputType(idx).value();

            StringToValue(
                valueString,
                *value,
                (FlexKit::AnimatorInputType)valueType);
        });
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
