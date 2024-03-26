#include "PCH.h"
#include "angelscript.h"
#include "AnimationComponents.h"
#include "EditorSelectedPrefabObject.h"
#include "AnimationUtilities.h"
#include "EditorAnimatorComponent.h"
#include "EditorPrefabObject.h"
#include "EditorScriptEngine.h"
#include "ResourceIDs.h"

#include <fmt/format.h>
#include <scn/scan.h>


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
		auto res = scn::scan<float>(in, "{}");
		if(res)
			input.x = res->value();
	}   break;
	case FlexKit::AnimatorInputType::Float2:
	{
		auto res = scn::scan<float, float>(in, "{}, {}");
		if (res)
		{
			auto& [x, y] = res->values();
			input.xy = { x,  y };
		}
	}   break;
	case FlexKit::AnimatorInputType::Float3:
	{
		auto res = scn::scan<float, float, float>(in, "{}, {}, {}");
		if (res)
		{
			auto& [x, y, z] = res->values();
			input.xyz = { x, y, z };
		}
	}   break;
	case FlexKit::AnimatorInputType::Float4:
	{
		auto res = scn::scan<float, float, float, float>(in, "{}, {}, {}, {}");
		if (res)
		{
			auto& [x, y, z, w] = res->values();
			input.xyzw = { x, y, z, w };
		}
	}   break;
	case FlexKit::AnimatorInputType::Uint:
	{
		auto res = scn::scan<uint32_t>(in, "{}");
		if (res)
			input.a = res->value();
	}   break;
	case FlexKit::AnimatorInputType::Uint2:
	{
		auto res = scn::scan<uint32_t, uint32_t>(in, "{}, {}");
		if (res)
		{
			auto& [a, b] = res->values();
			input.ab = { a, b };
		}
	}   break;
	case FlexKit::AnimatorInputType::Uint3:
	{
		auto res = scn::scan<uint32_t, uint32_t, uint32_t>(in, "{}, {}, {}");
		if (res)
		{
			auto& [a, b, c] = res->values();
			input.ab = { a, b, c };
		}
	}   break;
	case FlexKit::AnimatorInputType::Uint4:
	{
		auto res = scn::scan<uint32_t, uint32_t, uint32_t, uint32_t>(in, "{}, {}, {}, {}");
		if (res)
		{
			auto& [a, b, c, d] = res->values();
			input.ab = { a, b, c, d };
		}
	}   break;
	default:
		return;
	}
}


/************************************************************************************************/


void EditorSelectedPrefabObject::Reset()
{
	gameObject.Release();

	resource    = nullptr;
	prefab      = nullptr;
	animator    = nullptr;
	resourceID  = 0xffffffffffffffff;
	ID          = 0xffffffffffffffff;
}


/************************************************************************************************/


void RunAngelScriptExceptionHandles(asIScriptContext* ctx, asIScriptFunction* func, FlexKit::GameObject& gameObject, FlexKit::AnimatorView* animatorView, FlexKit::IAnimatorController* controller)
{
	__try
	{
		ctx->Prepare(func);
		SetArgAddress(ctx, 0, &gameObject);
		
		auto res = ctx->Execute();

		if (asEXECUTION_FINISHED == res)
		{
			auto& controller = FlexKit::SystemAllocator.allocate<FlexKit::AngelScriptController>(*static_cast<asIScriptObject*>(GetReturnObject(ctx)), gameObject, FlexKit::SystemAllocator);
			animatorView->SetController(controller);
		}
		else
			FK_LOG_ERROR("Editor: Failed to run angel script animator object constructor!");
	}
	__except (STATUS_ACCESS_VIOLATION == GetExceptionCode())
	{
		int line = ctx->GetLineNumber();
		ctx->SetException("Critical Failure: Segmentation Fault");
		FK_LOG_ERROR("Editor: Critical Exception Detected!");
	}
}


/************************************************************************************************/


void EditorSelectedPrefabObject::ReloadScript(EditorScriptEngine& engine)
{
	auto* animatorView	= static_cast<FlexKit::AnimatorView*>(gameObject.GetView(FlexKit::AnimatorComponentID));
	auto [obj, ctr]		= animatorView->GetScriptState();
	
	auto scriptModule = engine.BuildModule(resource->source);
	if (!scriptModule)
	{
		animatorView->ClearController();
		return;
	}
	
	auto ctx = FlexKit::GetContext();
	EXITSCOPE(
		ctx->Unprepare();
		FlexKit::ReleaseContext(ctx););
	
	auto func = scriptModule->GetFunctionByName("InitiateAnimator");
	RunAngelScriptExceptionHandles(ctx, func, gameObject, animatorView, ctr);
}


/************************************************************************************************/


uint32_t EditorSelectedPrefabObject::AddInputValue(const std::string& name, uint32_t valueType)
{
	return FlexKit::Apply(
		gameObject,
		[&](FlexKit::AnimatorView& animatorView) -> uint32_t
		{
			FlexKit::AnimatorComponent::InputID ID;
			FlexKit::AnimatorComponent::InputValue value;

			strncpy_s(ID.stringID, name.c_str(), FlexKit::Min(sizeof(ID.stringID), name.size()));
			ID.type = (FlexKit::AnimatorInputType)valueType;


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
				FlexKit::float4 xyzw{ 0, 0, 0, 0 };
				memcpy(animationValue.defaultValue, &xyzw, sizeof(FlexKit::float4));
				memcpy(&value, &xyzw, sizeof(FlexKit::float4));
			}   break;
			case (uint32_t)AnimationInput::InputType::Uint:
			case (uint32_t)AnimationInput::InputType::Uint2:
			case (uint32_t)AnimationInput::InputType::Uint3:
			case (uint32_t)AnimationInput::InputType::Uint4:
			{
				FlexKit::uint4 abcd{ 0, 0, 0, 0 };
				memcpy(animationValue.defaultValue, &abcd, sizeof(FlexKit::uint4));
				memcpy(&value, &abcd, sizeof(FlexKit::float4));
			}   break;
			default:
				break;
			}

			animatorView.GetState().inputIDs.push_back(ID);
			animatorView.GetState().inputValues.push_back(value);
			animator->inputs.push_back(animationValue);

			return animator->inputs.size() - 1;
		},
		[]() -> uint32_t { return -1; });
}


/************************************************************************************************/


std::string EditorSelectedPrefabObject::ValueString(uint32_t idx, uint32_t valueType)
{
	return FlexKit::Apply(
		gameObject,
		[&](FlexKit::AnimatorView& animator) -> std::string
		{
			auto value      = animator.GetInputValue(idx).value_or(nullptr);
			auto valueType  = animator.GetInputType(idx).value_or(FlexKit::AnimatorInputType::Unknown);

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


std::string EditorSelectedPrefabObject::DefaultValueString(uint32_t idx)
{
	auto& value = animator->inputs[idx];
	

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


void EditorSelectedPrefabObject::UpdateDefaultValue(uint32_t idx, const std::string& str)
{
	auto& value = animator->inputs[idx];

	switch (value.type)
	{
	case (AnimationInput::InputType)FlexKit::AnimatorInputType::Float:
	{
		auto res = scn::scan<float>(str, "{}");
		if (res)
			memcpy(value.defaultValue, &res->value(), sizeof(float));
	}   return;
	case (AnimationInput::InputType)FlexKit::AnimatorInputType::Float2:
	{
		auto res = scn::scan<float, float>(str, "{}, {}");
		if (res)
			memcpy(value.defaultValue, &res->values(), sizeof(FlexKit::float2));
	}   return;
	case (AnimationInput::InputType)FlexKit::AnimatorInputType::Float3:
	{
		auto res = scn::scan <float, float, float> (str, "{}, {}, {}");
		if (res)
			memcpy(value.defaultValue, &res->values(), sizeof(FlexKit::float3));
	}   return;
	case (AnimationInput::InputType)FlexKit::AnimatorInputType::Float4:
	{
		auto res = scn::scan<float, float, float, float>(str, "{}, {}, {}, {}");
		if (res)
			memcpy(value.defaultValue, &res->values(), sizeof(FlexKit::float4));
	}   return;
	case (AnimationInput::InputType)FlexKit::AnimatorInputType::Uint:
	{
		auto res = scn::scan<uint32_t>(str, "{}");
		if (res)
			memcpy(value.defaultValue, &res->value(), sizeof(uint32_t));
	}   return;
	case (AnimationInput::InputType)FlexKit::AnimatorInputType::Uint2:
	{
		auto res = scn::scan<uint32_t, uint32_t>(str, "{}, {}");
		if (res)
			memcpy(value.defaultValue, &res->values(), sizeof(FlexKit::uint2));
	}   return;
	case (AnimationInput::InputType)FlexKit::AnimatorInputType::Uint3:
	{
		auto res = scn::scan<uint32_t, uint32_t, uint32_t>(str, "{}, {}, {}");
		if (res)
			memcpy(value.defaultValue, &res->values(), sizeof(FlexKit::uint3));
	}   return;
	case (AnimationInput::InputType)FlexKit::AnimatorInputType::Uint4:
	{
		auto res = scn::scan<uint32_t, uint32_t, uint32_t, uint32_t>(str, "{}, {}, {}, {}");
		if (res)
			memcpy(value.defaultValue, &res->values(), sizeof(FlexKit::uint4));
	}   return;
	default:
		return;
	}
}


/************************************************************************************************/


void EditorSelectedPrefabObject::UpdateValue(uint32_t idx, const std::string& valueString)
{
	FlexKit::Apply(
		gameObject, [&](FlexKit::AnimatorView& animator)
		{
			auto value		= animator.GetInputValue(idx).value_or(nullptr);
			auto valueType	= animator.GetInputType(idx).value_or(FlexKit::AnimatorInputType::Unknown);

			StringToValue(
				valueString,
				*value,
				(FlexKit::AnimatorInputType)valueType);
		});
}


/************************************************************************************************/


void EditorSelectedPrefabObject::Release()
{
	gameObject.Release();
}


/************************************************************************************************/



/**********************************************************************

Copyright (c) 2021 - 2022 Robert May

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
