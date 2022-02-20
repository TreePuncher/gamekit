#include "PCH.h"

#include "EditorScriptEngine.h"
#include "EditorGadgetInterface.h"

#include <angelscript/scriptbuilder/scriptbuilder.h>
#include <angelscript/scriptany/scriptany.h>
#include <angelscript/scriptarray/scriptarray.h>
#include <angelscript/scriptstdstring/scriptstdstring.h>
#include <angelscript/scriptmath/scriptmath.h>
#include <angelscript/scriptmath/scriptmathcomplex.h>

#include <assert.h>

#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>


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


EditorScriptEngine::EditorScriptEngine() :
    scriptEngine{ asCreateScriptEngine() }
{
    scriptContext   = scriptEngine->CreateContext();
    auto r          = scriptEngine->SetMessageCallback(asFUNCTION(EditorScriptEngine::MessageCallback), this, asCALL_CDECL); assert(r >= 0);

    RegisterAPI();
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


void EditorScriptEngine::RegisterAPI()
{
    RegisterScriptArray(scriptEngine, true);
    RegisterScriptAny(scriptEngine);
    RegisterStdString(scriptEngine);
    RegisterStdStringUtils(scriptEngine);
    RegisterScriptMath(scriptEngine);
    RegisterScriptMathComplex(scriptEngine);

    auto n = scriptEngine->RegisterInterface("iGadget");                        assert(n >= 0);
    n = scriptEngine->RegisterInterfaceMethod("iGadget", "void Execute()");     assert(n >= 0);
    n = scriptEngine->RegisterInterfaceMethod("iGadget", "string GadgetID()");  assert(n >= 0);

    n = scriptEngine->RegisterGlobalFunction("void RegisterGadget(iGadget@ gadget)",    asMETHOD(EditorScriptEngine, RegisterGadget), asCALL_THISCALL_ASGLOBAL, this);      assert(n >= 0);
    n = scriptEngine->RegisterGlobalFunction("void Print(string text)",                 asMETHOD(EditorScriptEngine, PrintToOutputWindow), asCALL_THISCALL_ASGLOBAL, this); assert(n >= 0);

    int x = 0;
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


void EditorScriptEngine::RunStdString(const std::string& string)
{
    try
    {
        CScriptBuilder builder{};
        builder.StartNewModule(scriptEngine, "temp");

        if (auto r = builder.AddSectionFromMemory("", string.c_str(), string.size()); r < 0)
        {
            // TODO: show error in output window
        }

        if (auto r = builder.BuildModule(); r < 0)
        {
            // TODO: show error in output window
        }

        auto asModule   = builder.GetModule();
        auto func       = asModule->GetFunctionByDecl("void main()");

        if (func)
        {
            scriptContext->Prepare(func);
            scriptContext->Execute();
        }

        asModule->Discard();
    }
    catch (...)
    {
        OutputDebugString(L"Unknown error. Continuing as usual.\n");
    }
}


/************************************************************************************************/



/**********************************************************************

Copyright (c) 2021 Robert May

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
