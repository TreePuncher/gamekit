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
        auto res = scn::scan(in, "{}", input.x);
    }   break;
    case FlexKit::AnimatorInputType::Float2:
    {
        auto res = scn::scan(in, "{}, {}", input.xy.x, input.xy.y);
    }   break;
    case FlexKit::AnimatorInputType::Float3:
    {
        auto res = scn::scan(in, "{}, {}, {}", input.xyz.x, input.xyz.y, input.xyz.z);
    }   break;
    case FlexKit::AnimatorInputType::Float4:
    {
        auto res = scn::scan(in, "{}, {}, {}, {}", input.xyzw.x, input.xyzw.y, input.xyzw.z, input.xyzw.w);
    }   break;
    case FlexKit::AnimatorInputType::Uint:
    {
        auto res = scn::scan(in, "{}", input.a);
    }   break;
    case FlexKit::AnimatorInputType::Uint2:
    {
        auto res = scn::scan(in, "{}, {}", input.ab[0], input.ab[1]);
    }   break;
    case FlexKit::AnimatorInputType::Uint3:
    {
        auto res = scn::scan(in, "{}, {}, {}", input.abc[0], input.abc[1], input.abc[2]);
    }   break;
    case FlexKit::AnimatorInputType::Uint4:
    {
        auto res = scn::scan(in, "{}, {}, {}, {}", input.abcd[0], input.abcd[1], input.abcd[2], input.abcd[3]);
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

            switch (valueType)
            {
            case (uint32_t)AnimationInput::InputType::Float:
            case (uint32_t)AnimationInput::InputType::Float2:
            case (uint32_t)AnimationInput::InputType::Float3:
            case (uint32_t)AnimationInput::InputType::Float4:
            {
                FlexKit::float4 value{ 0, 0, 0, 0 };
                memcpy(animationValue.defaultValue,
                    &value, sizeof(FlexKit::float4));
            }   break;
            case (uint32_t)AnimationInput::InputType::Uint:
            case (uint32_t)AnimationInput::InputType::Uint2:
            case (uint32_t)AnimationInput::InputType::Uint3:
            case (uint32_t)AnimationInput::InputType::Uint4:
            {
                FlexKit::uint4 value{ 0, 0, 0, 0 };
                memcpy(animationValue.defaultValue,
                    &value, sizeof(FlexKit::uint4));
            }   break;
            default:
                break;
            }

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


/************************************************************************************************/


std::string ScriptedAnimationObject::DefaultValueString(uint32_t idx)
{
    auto& value = resource->inputs[idx];
    

    switch (value.type)
    {
    case (AnimationInput::InputType)FlexKit::AnimatorInputType::Float:
        return fmt::format("{}", *reinterpret_cast<float*>(value.defaultValue));
    case (AnimationInput::InputType)FlexKit::AnimatorInputType::Float2:
    {
        FlexKit::float2 xy = *reinterpret_cast<FlexKit::float2*>(value.defaultValue);
        return fmt::format("{}, {}", xy[0], xy[1]);
    }
    case (AnimationInput::InputType)FlexKit::AnimatorInputType::Float3:
    {
        FlexKit::float3 xyz = *reinterpret_cast<FlexKit::float3*>(value.defaultValue);
        return fmt::format("{}, {}, {}", xyz[0], xyz[1], xyz[2]);
    }
    case (AnimationInput::InputType)FlexKit::AnimatorInputType::Float4:
    {
        FlexKit::float4 xyzw = *reinterpret_cast<FlexKit::float4*>(value.defaultValue);
        return fmt::format("{}, {}, {}", xyzw[0], xyzw[1], xyzw[2], xyzw[3]);
    }
    case (AnimationInput::InputType)FlexKit::AnimatorInputType::Uint:
    {
        auto a = *reinterpret_cast<uint*>(value.defaultValue);
        return fmt::format("{}", a);
    }
    case (AnimationInput::InputType)FlexKit::AnimatorInputType::Uint2:
    {
        auto ab = *reinterpret_cast<FlexKit::uint2*>(value.defaultValue);
        return fmt::format("{}, {}", ab[0], ab[1]);
    }
    case (AnimationInput::InputType)FlexKit::AnimatorInputType::Uint3:
    {
        auto abc = *reinterpret_cast<FlexKit::uint3*>(value.defaultValue);
        return fmt::format("{}, {}, {}", abc[0], abc[1], abc[2]);
    }
    case (AnimationInput::InputType)FlexKit::AnimatorInputType::Uint4:
    {
        auto abcd = *reinterpret_cast<FlexKit::uint4*>(value.defaultValue);
        return fmt::format("{}, {}, {}, {}", abcd[0], abcd[1], abcd[2], abcd[3]);
    }
    default:
        return "";
    }
}


/************************************************************************************************/


void ScriptedAnimationObject::UpdateDefaultValue(uint32_t idx, const std::string& str)
{
    auto& value = resource->inputs[idx];

    switch (value.type)
    {
    case (AnimationInput::InputType)FlexKit::AnimatorInputType::Float:
    {
        float x;
        auto res = scn::scan(str, "{}", x);
        if (res)
            memcpy(value.defaultValue, &x, sizeof(x));
    }   return;
    case (AnimationInput::InputType)FlexKit::AnimatorInputType::Float2:
    {
        FlexKit::float2 xy;
        auto res = scn::scan(str, "{}, {}", xy.x, xy.y);
        if(res)
            memcpy(value.defaultValue, &xy, sizeof(xy));

    }   return;
    case (AnimationInput::InputType)FlexKit::AnimatorInputType::Float3:
    {
        FlexKit::float3 xyz;
        auto res = scn::scan(str, "{}, {}, {}", xyz.x, xyz.y, xyz.z);
        if (res)
            memcpy(value.defaultValue, &xyz, sizeof(xyz));
    }   return;
    case (AnimationInput::InputType)FlexKit::AnimatorInputType::Float4:
    {
        FlexKit::float4 xyzw;
        auto res = scn::scan(str, "{}, {}, {}, {}", xyzw.x, xyzw.y, xyzw.z, xyzw.w);
        if (res)
            memcpy(value.defaultValue, &xyzw, sizeof(xyzw));
    }   return;
    case (AnimationInput::InputType)FlexKit::AnimatorInputType::Uint:
    {
        uint32_t x;
        auto res = scn::scan(str, "{}", x);
        if (res)
            memcpy(value.defaultValue, &x, sizeof(x));
    }   return;
    case (AnimationInput::InputType)FlexKit::AnimatorInputType::Uint2:
    {
        FlexKit::uint2 xy;
        auto res = scn::scan(str, "{}, {}", xy[0], xy[1]);
        if (res)
            memcpy(value.defaultValue, &xy, sizeof(xy));
    }   return;
    case (AnimationInput::InputType)FlexKit::AnimatorInputType::Uint3:
    {
        FlexKit::uint3 xyz;
        auto res = scn::scan(str, "{}, {}, {}", xyz[0], xyz[1], xyz[2]);
        if (res)
            memcpy(value.defaultValue, &xyz, sizeof(xyz));
    }   return;
    case (AnimationInput::InputType)FlexKit::AnimatorInputType::Uint4:
    {
        FlexKit::uint4 xyzw;
        auto res = scn::scan(str, "{}, {}, {}, {}", xyzw[0], xyzw[1], xyzw[2]);
        if (res)
            memcpy(value.defaultValue, &xyzw, sizeof(xyzw));
    }   return;
    default:
        return;
    }
}


/************************************************************************************************/


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

    res = api->RegisterInterface("AnimatorInterface");                                                                                           FK_ASSERT(res > 0);
    res = api->RegisterInterfaceMethod("AnimatorInterface", "void Update(GameObject@ object, double dt)");                                       FK_ASSERT(res > 0);

    return true;
}


/************************************************************************************************/
