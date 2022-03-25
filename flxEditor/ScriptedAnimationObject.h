#pragma once
#include "EditorScriptEngine.h"
#include "EditorScriptObject.h"


/************************************************************************************************/


class AnimationObject;
class ScriptResource;

class ScriptedAnimationObject
{
public:
    ScriptResource_ptr  script;
    void*               obj;
    Module              scriptModule = nullptr;
    ScriptContext       ctx;

    void Update(AnimationObject* obj, double dT);
    void Reload(EditorScriptEngine& engine, AnimationObject* obj);

    static bool RegisterInterface(EditorScriptEngine& engine);
};
