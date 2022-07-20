#pragma once
#include "Components.h"
#include "EditorPrefabObject.h"
#include "EditorScriptEngine.h"

class AnimatorComponent;
class EditorScriptEngine;

struct AnimationEditorObject
{
    FlexKit::GameObject         gameObject;
    uint64_t                    ID          = (uint64_t)-1;
    uint64_t                    resourceID;

    AnimatorComponent*              animator;
    ScriptResource_ptr              resource;
    PrefabGameObjectResource_ptr    prefab;
    FlexKit::LayerHandle            layer;

    void        Reload(EditorScriptEngine& engine);

    uint32_t    AddInputValue(const std::string& name, uint32_t valueType);

    void        UpdateDefaultValue(uint32_t idx, const std::string& str);
    std::string DefaultValueString(uint32_t idx);

    std::string ValueString(uint32_t idx, uint32_t valueType);
    void        UpdateValue(uint32_t idx, const std::string& value);

    void        Release();

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
