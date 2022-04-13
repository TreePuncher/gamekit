#include "PCH.h"

#include "EditorScriptEngine.h"
#include "EditorGadgetInterface.h"
#include "ScriptingRuntime.h"
#include "memoryutilities.h"

#include <angelscript.h>
#include <angelscript/debugger/debugger.cpp>
#include <angelscript/scriptbuilder/scriptbuilder.h>

#include <assert.h>
#include <fmt/format.h>
#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>


/************************************************************************************************/


AngelScriptGadget::AngelScriptGadget(asIScriptObject* IN_object) : object  { IN_object }
{
    object->AddRef();

    asfnExecute = object->GetObjectType()->GetMethodByDecl("void Execute()");
    asfnGetID   = object->GetObjectType()->GetMethodByDecl("string GadgetID()");
}


/************************************************************************************************/


AngelScriptGadget::~AngelScriptGadget()
{
    if(object)
        object->Release();
}


/************************************************************************************************/


AngelScriptGadget::AngelScriptGadget(AngelScriptGadget&& rhs)
{
    object      = rhs.object;
    asfnExecute = rhs.asfnExecute;
    asfnGetID   = rhs.asfnGetID;

    rhs.object = nullptr;
}


/************************************************************************************************/


AngelScriptGadget& AngelScriptGadget::operator = (AngelScriptGadget& rhs)
{
    object      = rhs.object;
    asfnExecute = rhs.asfnExecute;
    asfnGetID   = rhs.asfnGetID;

    rhs.object  = nullptr;

    return *this;
}


/************************************************************************************************/


void AngelScriptGadget::Execute()
{
    auto ctx = FlexKit::GetContext();

    ctx->Prepare(asfnExecute);
    ctx->SetObject(object);
    ctx->Execute();

    FlexKit::ReleaseContext(ctx);
}


/************************************************************************************************/


std::string AngelScriptGadget::GadgetID()
{
    auto ctx = FlexKit::GetContext();

    ctx->Prepare(asfnGetID);
    ctx->SetObject(object);
    ctx->Execute();

    auto ret_value = std::move(*(std::string*)ctx->GetReturnObject());
    FlexKit::ReleaseContext(ctx);

    return ret_value;
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


EditorScriptEngine::EditorScriptEngine(FlexKit::iAllocator* allocator)
{
    FlexKit::InitiateScriptRuntime();

    auto r          = FlexKit::GetScriptEngine()->SetMessageCallback(asFUNCTION(EditorScriptEngine::MessageCallback), this, asCALL_CDECL); assert(r >= 0);

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

    gadgets.emplace_back(AngelScriptGadget{ gObj });
}


/************************************************************************************************/


void EditorScriptEngine::RegisterCoreTypesAPI(FlexKit::iAllocator* allocator)
{
    FlexKit::RegisterMathTypes(FlexKit::GetScriptEngine(), allocator);
    FlexKit::RegisterRuntimeAPI(FlexKit::GetScriptEngine());
}


void EditorScriptEngine::RegisterGadgetAPI()
{
    auto engine = FlexKit::GetScriptEngine();

    auto n = engine->RegisterInterface("iGadget");                        assert(n >= 0);
    n = engine->RegisterInterfaceMethod("iGadget", "void Execute()");     assert(n >= 0);
    n = engine->RegisterInterfaceMethod("iGadget", "string GadgetID()");  assert(n >= 0);

    n = engine->RegisterGlobalFunction("void RegisterGadget(iGadget@ gadget)",    asMETHOD(EditorScriptEngine, RegisterGadget), asCALL_THISCALL_ASGLOBAL, this);      assert(n >= 0);
    n = engine->RegisterGlobalFunction("void Print(string text)",                 asMETHOD(EditorScriptEngine, PrintToOutputWindow), asCALL_THISCALL_ASGLOBAL, this); assert(n >= 0);
}


/************************************************************************************************/


void EditorScriptEngine::RegisterAPI(FlexKit::iAllocator* allocator)
{
    auto engine = FlexKit::GetScriptEngine();

    RegisterCoreTypesAPI(allocator);
    RegisterGadgetAPI();

    engine->RegisterGlobalProperty("AllocatorHandle@ SystemAllocator", (FlexKit::iAllocator*)FlexKit::SystemAllocator);
}


/************************************************************************************************/


void EditorScriptEngine::LoadModules()
{
    std::filesystem::path scriptsPath{ "assets/editorscripts" };
    if (std::filesystem::exists(scriptsPath) && std::filesystem::is_directory(scriptsPath))
    {
        CScriptBuilder builder{};
        auto scriptEngine   = FlexKit::GetScriptEngine();
        auto ctx            = FlexKit::GetContext();
        auto tmp            = std::filesystem::directory_iterator{ scriptsPath };

        for (auto file : tmp)
        {
            auto temp = file.path().extension();

            if (file.path().extension() == ".as")
            {
                auto moduleName = file.path().filename().string();
                builder.StartNewModule(scriptEngine, moduleName.c_str());

                if(auto r = builder.AddSectionFromFile(file.path().string().c_str()); r < 0)
                    continue;

                if (auto r = builder.BuildModule(); r < 0)
                    continue;

                auto asModule = builder.GetModule();
                auto func   = asModule->GetFunctionByDecl("void Register()");

                ctx->Prepare(func);
                ctx->Execute();
            }
        }

        FlexKit::ReleaseContext(ctx);
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
        auto scriptEngine = FlexKit::GetScriptEngine();

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


class BytecodeBlobStream : public asIBinaryStream
{
public:
    int Write(const void* ptr, asUINT size)
    {
        FlexKit::Blob temp{ (const char*)ptr, size };
        blob += temp;

        return size;
    }

    int Read(void* ptr, asUINT size) { return 0; }

    FlexKit::Blob GetBlob()
    {
        return std::move(blob);
    }

    FlexKit::Blob blob;
};

std::optional<FlexKit::Blob> EditorScriptEngine::GetByteCode(const std::string& moduleName)
{
    auto engine = FlexKit::GetScriptEngine();

    auto scriptModule = engine->GetModule(moduleName.c_str());

    if (!scriptModule)
        return {};

    BytecodeBlobStream stream;
    scriptModule->SaveByteCode(&stream, false);

    return stream.GetBlob();
}


/************************************************************************************************/


std::optional<FlexKit::Blob> EditorScriptEngine::CompileToBlob(const std::string& string, ErrorCallbackFN errorCallback)
{
    try
    {
        auto engine = FlexKit::GetScriptEngine();

        CScriptBuilder builder{};
        auto randomModuleName = fmt::format("{}", rand());
        builder.StartNewModule(engine, randomModuleName.c_str());

        engine->SetMessageCallback(asFUNCTION(RunSTDStringMessageHandler), &errorCallback, asCALL_CDECL);

        if (auto r = builder.AddSectionFromMemory(randomModuleName.c_str(), string.c_str(), string.size()); r < 0)
            return {};

        if (auto r = builder.BuildModule(); r < 0)
            return {};

        auto r          = engine->SetMessageCallback(asFUNCTION(EditorScriptEngine::MessageCallback), nullptr, asCALL_CDECL); assert(r >= 0);
        auto asModule   = builder.GetModule();

        auto blob = GetByteCode(randomModuleName);

        asModule->Discard();

        return blob;
    }
    catch (...)
    {
        OutputDebugString(L"Unknown angelscript error. Continuing as usual.\n");
    }

    return {};
}


/************************************************************************************************/


ScriptContext EditorScriptEngine::CreateContext()
{
    return FlexKit::GetContext();
}


/************************************************************************************************/


void EditorScriptEngine::ReleaseContext(ScriptContext context)
{
    FlexKit::ReleaseContext(context);
}


/************************************************************************************************/


void EditorScriptEngine::CompileString(const std::string& string, asIScriptContext* ctx, ErrorCallbackFN errorCallback)
{
    try
    {
        auto engine = FlexKit::GetScriptEngine();

        CScriptBuilder builder{};
        auto temp = fmt::format("{}", rand());
        builder.StartNewModule(engine, temp.c_str());

        engine->SetMessageCallback(asFUNCTION(RunSTDStringMessageHandler), &errorCallback, asCALL_CDECL);

        if (auto r = builder.AddSectionFromMemory(temp.c_str(), string.c_str(), string.size()); r < 0)
        {
            // TODO: show error in output window
        }

        if (auto r = builder.BuildModule(); r < 0)
        {
            // TODO: show error in output window
        }

        auto r          = engine->SetMessageCallback(asFUNCTION(EditorScriptEngine::MessageCallback), nullptr, asCALL_CDECL); assert(r >= 0);
        auto asModule   = builder.GetModule();
        asModule->Discard();
    }
    catch (...)
    {
        OutputDebugString(L"Unknown angelscript error. Continuing as usual.\n");
    }
}


/************************************************************************************************/


void EditorScriptEngine::RunStdString(const std::string& string, asIScriptContext* ctx, ErrorCallbackFN errorCallback)
{
    try
    {
        auto engine = FlexKit::GetScriptEngine();

        CScriptBuilder builder{};
        builder.StartNewModule(engine, "temp");

        engine->SetMessageCallback(asFUNCTION(RunSTDStringMessageHandler), &errorCallback, asCALL_CDECL);

        if (auto r = builder.AddSectionFromMemory("Memory", string.c_str(), string.size()); r < 0)
            return;

        if (auto r = builder.BuildModule(); r < 0)
            return;

        auto r          = engine->SetMessageCallback(asFUNCTION(EditorScriptEngine::MessageCallback), nullptr, asCALL_CDECL); assert(r >= 0);
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


/************************************************************************************************/


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


/************************************************************************************************/


void* GetReturnObject(ScriptContext ctx)
{
    return ctx->GetReturnObject();
}


/************************************************************************************************/


void SetArg(ScriptContext ctx, uint32_t idx, void* obj)
{
    ctx->SetArgObject(idx, obj);
}


/************************************************************************************************/


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
