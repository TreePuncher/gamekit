#include "PCH.h"

#include "EditorScriptEngine.h"
#include "EditorGadgetInterface.h"
#include "ScriptingRuntime.h"
#include "memoryutilities.h"

#include <angelscript.h>

#include <angelscript/scriptbuilder/scriptbuilder.h>
#include <angelscript/scriptany/scriptany.h>
#include <angelscript/scriptarray/scriptarray.h>
#include <angelscript/scriptstdstring/scriptstdstring.h>
#include <angelscript/scriptmath/scriptmath.h>
#include <angelscript/scriptmath/scriptmathcomplex.h>

#include <angelscript/debugger/debugger.cpp>

#include <assert.h>

#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>


/************************************************************************************************/


AngelScriptGadget::AngelScriptGadget(
    asIScriptObject*    IN_object,
    asIScriptContext*   IN_context) :
        object  { IN_object },
        context { IN_context }
{
    object->AddRef();

    asfnExecute = object->GetObjectType()->GetMethodByDecl("void Execute()");
    asfnGetID = object->GetObjectType()->GetMethodByDecl("string GadgetID()");

}


/************************************************************************************************/


AngelScriptGadget::~AngelScriptGadget()
{
    object->Release();
}


/************************************************************************************************/


void AngelScriptGadget::Execute()
{
    context->Prepare(asfnExecute);
    context->SetObject(object);
    context->Execute();
}


/************************************************************************************************/


std::string AngelScriptGadget::GadgetID()
{
    context->Prepare(asfnGetID);
    context->SetObject(object);
    context->Execute();

    return *(std::string*)context->GetReturnObject();
}

/************************************************************************************************/


void EditorScriptEngine::MessageCallback(const asSMessageInfo* msg, EditorScriptEngine* param)
{
    param->PrintToErrorWindow(msg->message);

#ifdef DEBUG

    const auto strLen = strlen(msg->message);
    WCHAR*  strBuffer = new WCHAR[strLen + 1];

    auto mbsSize = mbstowcs(strBuffer, msg->message, strLen + 1);


    if (mbsSize != -1)
    {

        strBuffer[mbsSize] = '\0';
        OutputDebugString(strBuffer);
        OutputDebugString(L"\n");
    }

    delete[] strBuffer;

#endif
}


/************************************************************************************************/


void EditorScriptEngine::PrintToOutputWindow(std::string* str)
{
    outputTextBuffer += *str + "\n";
}


/************************************************************************************************/


void EditorScriptEngine::PrintToErrorWindow(const char* str)
{
    errorTextBuffer += str;
}


/************************************************************************************************/


EditorScriptEngine::EditorScriptEngine(FlexKit::iAllocator* allocator) :
    scriptEngine    { asCreateScriptEngine() }
    //debugger        { new CDebugger{} }
{
    scriptContext   = scriptEngine->CreateContext();
    auto r          = scriptEngine->SetMessageCallback(asFUNCTION(EditorScriptEngine::MessageCallback), this, asCALL_CDECL); assert(r >= 0);

    //scriptContext->SetLineCallback(asMETHOD(CDebugger, LineCallback), debugger, asCALL_THISCALL);

    RegisterAPI(allocator);
}


EditorScriptEngine::~EditorScriptEngine()
{
}

/************************************************************************************************/


void EditorScriptEngine::RegisterGadget(asIScriptObject* gObj)
{
    auto func   = gObj->GetObjectType()->GetMethodByDecl("void Execute()");
    auto engine = gObj->GetEngine();
    auto gadget = new AngelScriptGadget{  gObj, scriptContext };

    gadgets.push_back(gadget);
}


/************************************************************************************************/


void EditorScriptEngine::RegisterCoreTypesAPI(FlexKit::iAllocator* allocator)
{
    FlexKit::RegisterMathTypes(scriptEngine, allocator);
    FlexKit::RegisterGameObjectCore(scriptEngine);
}


void EditorScriptEngine::RegisterGadgetAPI()
{
    auto n = scriptEngine->RegisterInterface("iGadget");                        assert(n >= 0);
    n = scriptEngine->RegisterInterfaceMethod("iGadget", "void Execute()");     assert(n >= 0);
    n = scriptEngine->RegisterInterfaceMethod("iGadget", "string GadgetID()");  assert(n >= 0);

    n = scriptEngine->RegisterGlobalFunction("void RegisterGadget(iGadget@ gadget)",    asMETHOD(EditorScriptEngine, RegisterGadget), asCALL_THISCALL_ASGLOBAL, this);      assert(n >= 0);
    n = scriptEngine->RegisterGlobalFunction("void Print(string text)",                 asMETHOD(EditorScriptEngine, PrintToOutputWindow), asCALL_THISCALL_ASGLOBAL, this); assert(n >= 0);
}


/************************************************************************************************/


void EditorScriptEngine::RegisterAPI(FlexKit::iAllocator* allocator)
{
    RegisterScriptArray(scriptEngine, true);
    RegisterScriptAny(scriptEngine);
    RegisterStdString(scriptEngine);
    RegisterStdStringUtils(scriptEngine);
    RegisterScriptMath(scriptEngine);
    RegisterScriptMathComplex(scriptEngine);

    RegisterCoreTypesAPI(allocator);
    RegisterGadgetAPI();

    scriptEngine->RegisterGlobalProperty("AllocatorHandle@ SystemAllocator", (FlexKit::iAllocator*)FlexKit::SystemAllocator);
}


/************************************************************************************************/


void EditorScriptEngine::LoadModules()
{
    std::filesystem::path scriptsPath{ "assets/editorscripts" };
    if (std::filesystem::exists(scriptsPath) && std::filesystem::is_directory(scriptsPath))
    {
        CScriptBuilder builder{};
        auto ctx = scriptEngine->CreateContext();
        auto tmp = std::filesystem::directory_iterator{ scriptsPath };

        for (auto file : tmp)
        {
            auto temp = file.path().extension();

            if (file.path().extension() == ".as")
            {
                auto moduleName = file.path().filename().string();
                builder.StartNewModule(scriptEngine, moduleName.c_str());
                if (auto r = builder.AddSectionFromFile(file.path().string().c_str()); r < 0)
                {
                    // Error!
                    // TODO: log to script error window
                    continue;
                }
                else
                {
                }

                if(auto r = builder.BuildModule(); r < 0)
                {
                    // Error!
                    // TODO: log to script error window
                    continue;
                }

                auto asModule = builder.GetModule();
                auto func   = asModule->GetFunctionByDecl("void Register()");

                ctx->Prepare(func);
                ctx->Execute();
            }
        }
    }
}


/************************************************************************************************/


void RunSTDStringMessageHandler(const asSMessageInfo* msg, ErrorCallbackFN* callback)
{
    if(callback)
        (*callback)(msg->col, msg->row, msg->message, msg->section, msg->type);
}


/************************************************************************************************/


Module EditorScriptEngine::BuildModule(const std::string& string, ErrorCallbackFN errorCallback)
{
    try
    {
        CScriptBuilder builder{};
        builder.StartNewModule(scriptEngine, "temp");

        scriptEngine->SetMessageCallback(asFUNCTION(RunSTDStringMessageHandler), &errorCallback, asCALL_CDECL);

        if (auto r = builder.AddSectionFromMemory("Memory", string.c_str(), string.size()); r < 0)
            return nullptr;

        if (auto r = builder.BuildModule(); r < 0)
            return nullptr;

        auto r          = scriptEngine->SetMessageCallback(asFUNCTION(EditorScriptEngine::MessageCallback), this, asCALL_CDECL); assert(r >= 0);
        auto asModule   = builder.GetModule();

        return asModule;
    }
    catch (...)
    {
        OutputDebugString(L"Unknown error. Continuing as usual.\n");
    }

    return nullptr;
}


/************************************************************************************************/


void EditorScriptEngine::ReleaseModule(Module module)
{
    if(module)
        module->Discard();
}


/************************************************************************************************/


ScriptContext EditorScriptEngine::CreateContext()
{
    return scriptEngine->CreateContext();
}


/************************************************************************************************/


void EditorScriptEngine::ReleaseContext(ScriptContext context)
{
    context->Release();
}


/************************************************************************************************/


void EditorScriptEngine::CompileString(const std::string& string, asIScriptContext* ctx, ErrorCallbackFN errorCallback)
{
    try
    {
        CScriptBuilder builder{};
        auto temp = fmt::format("{}", rand());
        builder.StartNewModule(scriptEngine, temp.c_str());

        scriptEngine->SetMessageCallback(asFUNCTION(RunSTDStringMessageHandler), &errorCallback, asCALL_CDECL);

        if (auto r = builder.AddSectionFromMemory(temp.c_str(), string.c_str(), string.size()); r < 0)
        {
            // TODO: show error in output window
        }

        if (auto r = builder.BuildModule(); r < 0)
        {
            // TODO: show error in output window
        }

        auto r          = scriptEngine->SetMessageCallback(asFUNCTION(EditorScriptEngine::MessageCallback), this, asCALL_CDECL); assert(r >= 0);
        auto asModule   = builder.GetModule();
        asModule->Discard();
    }
    catch (...)
    {
        OutputDebugString(L"Unknown error. Continuing as usual.\n");
    }
}


/************************************************************************************************/


void EditorScriptEngine::RunStdString(const std::string& string, asIScriptContext* ctx, ErrorCallbackFN errorCallback)
{
    try
    {
        CScriptBuilder builder{};
        builder.StartNewModule(scriptEngine, "temp");

        scriptEngine->SetMessageCallback(asFUNCTION(RunSTDStringMessageHandler), &errorCallback, asCALL_CDECL);

        if (auto r = builder.AddSectionFromMemory("Memory", string.c_str(), string.size()); r < 0)
        {
            // TODO: show error in output window
        }

        if (auto r = builder.BuildModule(); r < 0)
        {
            // TODO: show error in output window
        }

        auto r          = scriptEngine->SetMessageCallback(asFUNCTION(EditorScriptEngine::MessageCallback), this, asCALL_CDECL); assert(r >= 0);
        auto asModule   = builder.GetModule();
        auto func       = asModule->GetFunctionByDecl("void main()");

        if (func)
        {
            ctx->Prepare(func);
            ctx->Execute();
        }

        asModule->Discard();
    }
    catch (...)
    {
        OutputDebugString(L"Unknown error. Continuing as usual.\n");
    }
}



bool RunScriptFunction(ScriptContext ctx, Module module, const std::string_view view)
{
    if (!module)
        return false;

    auto r = ctx->GetEngine()->SetMessageCallback(asFUNCTION(RunSTDStringMessageHandler), nullptr, asCALL_CDECL); assert(r >= 0);

    auto func = module->GetFunctionByName(view.data());
    if (!func)
        return false;

    ctx->Prepare(func);
    auto res = ctx->Execute();

    return res >= 0;
}


void* GetReturnObject(ScriptContext ctx)
{
    return ctx->GetReturnObject();
}


void SetArg(ScriptContext ctx, uint32_t idx, void* obj)
{
    ctx->SetArgObject(idx, obj);
}

void SetArgAddress(ScriptContext ctx, uint32_t idx, void* obj)
{
    ctx->SetArgAddress(idx, obj);
}

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
