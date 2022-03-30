#pragma once
#include "EditorScriptEngine.h"
#include "EditorScriptObject.h"


/************************************************************************************************/


class AnimationObject;
class ScriptResource;


struct AnimationInput
{
    AnimationInput() {}

    AnimationInput(const AnimationInput& rhs)
    {
        memcpy(this, &rhs, sizeof(AnimationInput));
    }

    AnimationInput& operator = (const AnimationInput& rhs)
    {
        memcpy(this, &rhs, sizeof(AnimationInput));
    }

    enum class InputType : uint32_t
    {
        Float,
        Float2,
        Float3,
        Float4,
        Uint,
        Uint2,
        Uint3,
        Uint4
    } type;

    union
    {
        float           x;
        FlexKit::float2 xy;
        FlexKit::float3 xyz;
        FlexKit::float4 xyzw;

        uint32_t        a;
        FlexKit::uint2  ab;
        FlexKit::uint3  abc;
        FlexKit::uint4  abcd;
    };

    std::string ValueToString() const noexcept;
    void StringToValue(const std::string& in) noexcept;

    uint32_t    IDHash;
    char        stringID[32];
};


/************************************************************************************************/


class ScriptedAnimationObject
{
public:
    ScriptResource_ptr  script;
    void*               obj;
    Module              scriptModule = nullptr;
    ScriptContext       ctx;

    std::vector<AnimationInput> inputs;

    void Update(AnimationObject* obj, double dT);
    void Reload(EditorScriptEngine& engine, AnimationObject* obj);

    static bool RegisterInterface(EditorScriptEngine& engine);
};


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
