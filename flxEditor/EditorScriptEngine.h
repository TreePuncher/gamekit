#pragma once

#include <angelscript.h>
#include "EditorGadgetInterface.h"
#include <vector>

class EditorModule
{
public:
};


/************************************************************************************************/


class AngelScriptGadget : public iEditorGadget
{
public:
    AngelScriptGadget(
        asIScriptObject*    IN_object   = nullptr,
        asIScriptContext*   IN_context  = nullptr) :
            object      { IN_object },
            context     { IN_context }
    {
        object->AddRef();

        asfnExecute = object->GetObjectType()->GetMethodByDecl("void Execute()");
        asfnGetID   = object->GetObjectType()->GetMethodByDecl("string GadgetID()");

    }

    ~AngelScriptGadget()
    {
        object->Release();
    }

    asIScriptObject*    object       = nullptr;
    asIScriptFunction*  asfnExecute  = nullptr;
    asIScriptFunction*  asfnGetID    = nullptr;
    asIScriptContext*   context      = nullptr;


    void Execute() override
    {
        context->Prepare(asfnExecute);
        context->SetObject(object);
        context->Execute();
    }


    std::string GadgetID() override
    {
        context->Prepare(asfnGetID);
        context->SetObject(object);
        context->Execute();

        return *(std::string*)context->GetReturnObject();
    }
};


/************************************************************************************************/


class EditorScriptEngine
{
public:

    EditorScriptEngine();

    std::vector<AngelScriptGadget*> GetGadgets() { return gadgets; };
    
    void LoadModules();

    void RunStdString(const std::string& string);


    asIScriptEngine*    GetScriptEngine()   { return scriptEngine; }
    const std::string&  GetTextBuffer()     { return outputTextBuffer; }

private:

    void RegisterAPI();

    void RegisterGadget(asIScriptObject* gObj);
    void PrintToOutputWindow(std::string* str);

    asIScriptEngine*    scriptEngine    = nullptr;
    asIScriptContext*   scriptContext   = nullptr;

    std::string         outputTextBuffer;

    std::vector<AngelScriptGadget*> gadgets;
};

/**********************************************************************

Copyright (c) 2019-2021 Robert May

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
