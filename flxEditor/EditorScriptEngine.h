#pragma once

#include "EditorGadgetInterface.h"
#include <vector>


/************************************************************************************************/


class asIScriptContext;
class asIScriptEngine;
class asIScriptFunction;
class asIScriptObject;
class asIScriptModule;
class AngelScriptGadget;
struct asSMessageInfo;


/************************************************************************************************/


class AngelScriptGadget : public iEditorGadget
{
public:
    AngelScriptGadget(asIScriptObject*, asIScriptContext* IN_context = nullptr);
    ~AngelScriptGadget();

    void        Execute() override;
    std::string GadgetID() override;

    asIScriptObject*    object       = nullptr;
    asIScriptFunction*  asfnExecute  = nullptr;
    asIScriptFunction*  asfnGetID    = nullptr;
    asIScriptContext*   context      = nullptr;
};


/************************************************************************************************/


struct BreakPoint
{
    int line;


    bool operator < (const BreakPoint& rhs) const noexcept
    {
        return line < rhs.line;
    }
};

using ErrorCallbackFN = std::function<void(int, int, const char*, const char*, int)>;

using Module = asIScriptModule*;
using ScriptContext = asIScriptContext*;

class EditorScriptEngine
{
public:

    EditorScriptEngine(FlexKit::iAllocator* allocator = FlexKit::SystemAllocator);
    ~EditorScriptEngine();

    std::vector<AngelScriptGadget*> GetGadgets() { return gadgets; };
    
    void LoadModules();

    Module  BuildModule(const std::string& string, ErrorCallbackFN errorCallback = [](int, int, const char*, const char*, int) {});
    void    ReleaseModule(Module);

    void    CompileString(const std::string& string, asIScriptContext* ctx, ErrorCallbackFN errorCallback = [](int, int, const char*, const char*, int) {});
    void    RunStdString(const std::string& string, asIScriptContext* ctx, ErrorCallbackFN errorCallback = [](int, int, const char*, const char*, int) {});

    ScriptContext   CreateContext();
    void            ReleaseContext(ScriptContext);

    asIScriptEngine*    GetScriptEngine()   { return scriptEngine; }
    const std::string&  GetTextBuffer()     { return outputTextBuffer; }
    const std::string&  GetErrorBuffer()    { return errorTextBuffer; }

private:

    void RegisterAPI(FlexKit::iAllocator*);

    void RegisterGadget(asIScriptObject* gObj);
    void PrintToOutputWindow(std::string* str);
    void PrintToErrorWindow(const char* str);

protected:


    void        RegisterCoreTypesAPI(FlexKit::iAllocator* allocator);
    void        RegisterGadgetAPI();
    static void MessageCallback(const asSMessageInfo* msg, EditorScriptEngine* param);

    asIScriptEngine*    scriptEngine    = nullptr;
    asIScriptContext*   scriptContext   = nullptr;

    std::string         outputTextBuffer;
    std::string         errorTextBuffer;

    std::vector<AngelScriptGadget*> gadgets;
};


bool    RunScriptFunction   (ScriptContext, Module, const std::string_view view);
void*   GetReturnObject     (ScriptContext);
void    SetArg              (ScriptContext ctx, uint32_t idx, void* obj);
void    SetArgAddress       (ScriptContext ctx, uint32_t idx, void* obj);


/**********************************************************************

Copyright (c) 2019-2022 Robert May

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
