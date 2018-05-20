/**********************************************************************

Copyright (c) 2017 Robert May

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

#include "stdafx.h"
#include "Console.h"

namespace FlexKit
{	/************************************************************************************************/

	bool PrintVar		(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void*);
	bool ListVars		(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void*);
	bool ListFunctions	(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void*);
	bool ToggleBool		(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void*);


	/************************************************************************************************/


	void InitateConsole(Console* Out, FontAsset* Font, EngineCore* Engine)
	{
		Out->Lines.clear();
		Out->Memory                       = Engine->GetBlockMemory();
		Out->Font                         = Font;
		Out->InputBufferSize              = 0;
		Out->Variables.Allocator          = Out->Memory;
		Out->FunctionTable.Allocator      = Out->Memory;
		Out->BuiltInIdentifiers.Allocator = Out->Memory;
		Out->ConsoleUInts.Allocator		  = Out->Memory;

		AddConsoleFunction(Out, { "PrintVar",		PrintVar, nullptr, 1,{ ConsoleVariableType::CONSOLE_STRING } });
		AddConsoleFunction(Out, { "ListVars",		ListVars, nullptr, 0,{} });
		AddConsoleFunction(Out, { "ListFunctions",	ListFunctions, nullptr, 0,{} });
		AddConsoleFunction(Out, { "Toggle",			ToggleBool, nullptr, 1,{ ConsoleVariableType::CONSOLE_STRING } });

		AddStringVar(Out, "Version", "Pre-Alpha 0.0.0.1");
		AddStringVar(Out, "BuildDate", __DATE__);

		memset(Out->InputBuffer, '\0', sizeof(Out->InputBuffer));
	}


	/************************************************************************************************/


	void ReleaseConsole(Console* C)
	{
		//ConsolePrintf(out, 1, 2, 3, 4);

		C->ConsoleUInts.Release();
		C->Variables.Release();
		C->FunctionTable.Release();
		C->BuiltInIdentifiers.Release();
	}


	/************************************************************************************************/


	bool ListFunctions(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void*)
	{
		ConsolePrint(C, "Function List:", nullptr);

		for (auto& V : C->FunctionTable)
			ConsolePrint(C, V.FunctionName, nullptr);

		return true;
	}


	/************************************************************************************************/


	/*
	void DrawConsole(Console* C, ImmediateRender* IR, uint2 Window_WH)
	{
		const float LineHeight = (float(C->Font->FontSize[1]) / Window_WH[1]) / 4;
		const float AspectRatio = float(Window_WH[0]) / float(Window_WH[1]);
		size_t itr = 0;

		float y = 0.5f - float(1 + (itr)) * LineHeight;
		PrintTextFormatting Format;
		Format.StartingPOS = { 0, y };
		Format.TextArea    = float2(1.0f, 1.0f) - float2(0.0f, y);
		Format.Color       = float4(WHITE, 1.0f);
		Format.Scale       = { 0.5f / AspectRatio, 0.5f };
		Format.PixelSize   = float2(1.0f,1.0f ) / Window_WH;
		Format.CenterX	   = false;
		Format.CenterY	   = false;
		PrintText(IR, C->InputBuffer, C->Font, Format, C->Memory);

		for (auto& Line : C->Lines) {
			float y = 0.5f - float(2 + (itr)) * LineHeight ;

			if (y > 0) {
				float2 Position(0.0f, y);
				Format.StartingPOS = Position;
				Format.TextArea = float2(1.0f, 1.0f) - Position;
				PrintText(IR, Line, C->Font, Format, C->Memory);
				itr++;
			}
			else
				break;
		}
	}
	*/

	/************************************************************************************************/


	void InputConsole(Console* C, char InputCharacter)
	{
		C->InputBuffer[C->InputBufferSize++] = InputCharacter;
	}


	/************************************************************************************************/


	bool ToggleBool(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void*)
	{
		if (ArguementCount != 1 &&
			(Arguments->Type != ConsoleVariableType::CONSOLE_STRING) ||
			(Arguments->Type != ConsoleVariableType::STACK_STRING)) {
			ConsolePrint(C, "INVALID NUMBER OF ARGUMENTS OR INVALID ARGUMENT!");
			return false;
		}

		const char* VariableIdentifier = (const char*)Arguments->Data_ptr;
		for (auto Var : C->Variables)
		{
			if (!strncmp(Var.VariableIdentifier.str, VariableIdentifier, min(strlen(Var.VariableIdentifier.str), Arguments->Data_size)))
			{
				if (Var.Type == ConsoleVariableType::CONSOLE_BOOL) {
					*(bool*)Var.Data_ptr = !(*(bool*)Var.Data_ptr);
					return true;
				}
				else
				{
					ConsolePrint(C, "Invalid Target Variable!");
					return false;
				}
			}
		}
		return false;
	}


	bool ListVars(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void*)
	{
		ConsolePrint(C, "Listing Variables:", nullptr);

		for (auto& V : C->Variables)
		{
			ConsolePrint(C, V.VariableIdentifier.str, nullptr);
		}
		return true;
	}


	bool PrintVar(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void*)
	{
		if (ArguementCount != 1 && 
			(Arguments->Type != ConsoleVariableType::CONSOLE_STRING) || 
			(Arguments->Type != ConsoleVariableType::STACK_STRING)) {
			ConsolePrint(C, "INVALID NUMBER OF ARGUMENTS OR INVALID ARGUMENT!");
			return false;
		}

		const char* VariableIdentifier = (const char*)Arguments->Data_ptr;
		for (auto Var : C->Variables)
		{
			if (!strncmp(Var.VariableIdentifier.str, VariableIdentifier, min(strlen(Var.VariableIdentifier.str), Arguments->Data_size)))
			{
				// Print Variable
				switch (Var.Type)
				{
				case ConsoleVariableType::CONSOLE_STRING:
					ConsolePrint(C, (const char*)Var.Data_ptr);
					break;
				case ConsoleVariableType::CONSOLE_UINT:
				{
					char* Str = (char*)C->Memory->malloc(64);
					memset(Str, '\0', 64);

					_itoa_s(*(size_t*)Var.Data_ptr, Str, 64, 10);
					ConsolePrint(C, Str, C->Memory);
				}	break;
				case ConsoleVariableType::CONSOLE_BOOL:
				{
					if(*(bool*)Var.Data_ptr)
						ConsolePrint(C, "True", C->Memory);
					else
						ConsolePrint(C, "False", C->Memory);
				}	break;
				default:
					break;
				}
				return true;
			}
		}

		ConsolePrint(C, "Variable Not Found!");
		return false;
	}


	/************************************************************************************************/


	Vector<InputToken> GetTokens(iAllocator* Memory, const char* Str, ErrorTable)
	{
		Vector<InputToken> Out(Memory);

		size_t StrLen = strlen(Str);
		size_t I = 0;

		const char* TokenStrBegin = Str;

		auto SkipSpace = [&]() {
			while (Str[I] == ' ' || Str[I] == '\t' || Str[I] == '\n')
				I++;
		};

		while(I <= StrLen) {
			switch (Str[I])
			{
			case ' ':	// Space Reached
			case '\n':	// EndLine Reached
			case '\t':	// Tab Reached
			{
				size_t Length = (Str + I) - TokenStrBegin;
				Out.push_back({ TokenStrBegin, Length, InputToken::CT_UNKNOWN });
				TokenStrBegin = Str + I;
			}	break;
			case '\0':	// Str End Reached
				break;
			case '(':	// 
			{
				size_t Length = (Str + I) - TokenStrBegin;
				Out.push_back({ TokenStrBegin, Length, InputToken::CT_UNKNOWN });
				TokenStrBegin = Str + I;
			
				Out.push_back({ TokenStrBegin, 1, InputToken::CT_SYMBOL });
				TokenStrBegin = Str + I;
			}	break;
			case ')':	// 
			{
				Out.push_back({ Str + I, 1, InputToken::CT_SYMBOL });
				TokenStrBegin = Str + I + 1;
			}	break;
			case '\'':	// 
			case '\"':	// 
			{
				size_t II = I + 1;
				while (II <= StrLen && (Str[II] != '\'' && Str[II] != '\"'))
					++II;

				size_t Length = II - I + 1;
				Out.push_back({ Str + I + 1, Length - 1, InputToken::CT_STRINGLITERAL });
				TokenStrBegin = Str + I;
				I = II;
			}
			default:
				break;
			}
			++I;
		}

		return Out;
	}

	ConsoleSyntax IdentifyToken(Vector<InputToken>& in, size_t i, ConsoleIdentifierTable& Identifiers, TokenList& Tokens, ErrorTable& ErrorHandler)
	{
		switch (in[i].Type)
		{
		case InputToken::CT_UNKNOWN: // Try to Identify Token
			for (auto& I : Identifiers)
			{
				if (!strncmp(in[i].Str, I.Str, min(in[i].StrLen, I.size)))
				{
					switch (I.Type)
					{
					case IdentifierType::TYPE:
					{	// Verify Grammer
						return ConsoleSyntax::IDENTIFIER;
					}
					case IdentifierType::VARIABLE:
					{	// No Grammer Checking Needed
						return ConsoleSyntax::IDENTIFIER;
					}	break;
					case IdentifierType::FUNCTION:
					{	// Quick Grammer Check on function Argument syntax
						if ((in[i + 1].Type == InputToken::CT_SYMBOL) &&
							(in[i + 1].Str[0] == '('))
						{
							size_t ii = 0;
							while ((i + ii) < in.size()) 
							{ 
								if (in[i + ii].Type == InputToken::CT_SYMBOL && in[i + ii].Str[0] != ')')
									return ConsoleSyntax::FUNCTIONCALL;
								++ii;
							}

							return ConsoleSyntax::SYNTAXERROR;

						}
						else
							return ConsoleSyntax::SYNTAXERROR;

					}	break;
					default:
						break;
					}
				}
			}
			return ConsoleSyntax::UNKNOWNIDENTIFIER;
		case InputToken::CT_NUMBER:
		case InputToken::CT_STRINGLITERAL:
			return ConsoleSyntax::CONSTVARIABLE;
		case InputToken::CT_SYMBOL:
		{
			switch (in[i].Str[0])
			{
			case '(':
				return ConsoleSyntax::ARGUEMENTSBEGIN;
				break;
			case ')':
				return ConsoleSyntax::ARGUEMENTSEND;
				break;

			default:
				break;
			}
		}	break;
		default:
			break;
		}

		return ConsoleSyntax::SYNTAXERROR;
	}


	bool ExecuteGrammerTokens(Vector<GrammerToken>& Tokens, Console* C, Vector<ConsoleVariable>& TempVariables)
	{
		for (size_t I = 0; I < Tokens.size(); ++I)
		{
			switch (Tokens[I].SyntaxType)
			{
			case ConsoleSyntax::FUNCTIONCALL:
			{
				auto Token = &Tokens[I];
				auto Fn = FindConsoleFunction(C, Token->Token->Str, Token->Token->StrLen);
				if (Fn)
				{
					static_vector<ConsoleVariable> Arguments;
					if (Tokens[I + 1].SyntaxType == ConsoleSyntax::ARGUEMENTSBEGIN)
					{
						size_t II = + 2;
						while (Tokens[I + II].SyntaxType != ConsoleSyntax::ARGUEMENTSEND)
						{
							if (Tokens[I + II].SyntaxType == ConsoleSyntax::CONSTVARIABLE)
								Arguments.push_back(TempVariables[Tokens[I + II].Usr]);
							else
							{	// Search For Var
								int x = 0;
								FK_ASSERT(0, "Un-Implemented!");
							}
							II++;
						}
					}

					Fn->FN_Ptr(C, Arguments.begin(), Arguments.size(), Fn->USER);
				}
				else
				{
					ConsolePrint(C, "Function Not Found");
				}
			}	break;
			case ConsoleSyntax::ENDSTATEMENT:
				return true;
			default:
				//ConsolePrint(C, "INVALID STATEMENT!");

				return false;
				break;
			}
		}
		return false;
	}

	bool ProcessTokens(iAllocator* Memory, Vector<InputToken>& in, Console* C, ErrorTable& ErrorHandler )
	{
		bool Success = true;

		Vector<GrammerToken>		Tokens(Memory);
		Vector<ConsoleVariable>	TempVariables(Memory);

		// Translate Tokens into Grammer Objects
		for (size_t I = 0; I < in.size(); ++I)
		{
			switch (in[I].Type)
			{
			case InputToken::CT_UNKNOWN:
			{
				auto TokenType = IdentifyToken(in, I, C->BuiltInIdentifiers, Tokens, ErrorHandler);
				Tokens.push_back({ &in[I], 0, TokenType });
				int x = 0;
			}	break;

			case InputToken::CT_SYMBOL:
			{
				auto TokenType = IdentifyToken(in, I, C->BuiltInIdentifiers, Tokens, ErrorHandler);
				if(TokenType != ConsoleSyntax::UNUSEDSYMBOL)
					Tokens.push_back({ &in[I], 0, TokenType });
			}	break;
			case InputToken::CT_NUMBER:
				{
				Tokens.push_back({ &in[I], TempVariables.size(), ConsoleSyntax::CONSTVARIABLE });
				ConsoleVariable Var;
				Var.Data_ptr	= &in[I].Str;
				Var.Data_size	=  in[I].StrLen;

				Var.Type					 = ConsoleVariableType::STACK_INT;
				Var.VariableIdentifier.Index = TempVariables.size();

				TempVariables.push_back(Var);
			}	break;
			case InputToken::CT_STRINGLITERAL:
			{
				Tokens.push_back({ &in[I], TempVariables.size(), ConsoleSyntax::CONSTVARIABLE });
				ConsoleVariable Var;
				Var.Data_ptr	= (void*)in[I].Str;
				Var.Data_size	= in[I].StrLen;

				Var.Type					 = ConsoleVariableType::STACK_STRING;
				Var.VariableIdentifier.Index = TempVariables.size();

				TempVariables.push_back(Var);
			}	break;
			case InputToken::CT_IDENTIFIER:
				Tokens.push_back({ &in[I], 0, ConsoleSyntax::IDENTIFIER });
			default:
				break;
			}
		}

		if( Tokens.size() && Tokens.back().SyntaxType != ConsoleSyntax::ENDSTATEMENT)
			Tokens.push_back({ nullptr, 0, ConsoleSyntax::ENDSTATEMENT });

		Success = ExecuteGrammerTokens(Tokens, C, TempVariables);

		return Success;
	}

	void EnterLineConsole(Console* C)
	{
		C->InputBuffer[C->InputBufferSize++] = '\0';
		size_t BufferSize = C->InputBufferSize + 256;
		char* str = (char*)C->Memory->malloc(BufferSize);
		strcpy_s(str, BufferSize, C->InputBuffer);

		PushCommandToHistory(C, str, C->InputBufferSize + 256);
		ConsolePrint(C, str, C->Memory);

		C->InputBufferSize = 0;

		ErrorTable ErrorHandler;


		auto Tokens = GetTokens(C->Memory, C->InputBuffer, ErrorHandler);
		if (!ProcessTokens(C->Memory, Tokens, C, ErrorHandler))
		{
			// Handle Errors
		}

		memset(C->InputBuffer, '\0', sizeof(C->InputBuffer));
	}


	/************************************************************************************************/


	void BackSpaceConsole(Console* C)
	{
		if(C->InputBufferSize)
			C->InputBuffer[--C->InputBufferSize] = '\0';
	}


	/************************************************************************************************/

	size_t AddStringVar(Console* C, const char* Identifier, const char* Str)
	{
		size_t Out = C->Variables.size();

		ConsoleVariable NewVar;
		NewVar.Data_ptr					= (void*)Str;
		NewVar.Data_size				= strlen(Str);
		NewVar.VariableIdentifier.str	= Identifier;
		NewVar.Type						= ConsoleVariableType::CONSOLE_STRING;

		C->Variables.push_back(NewVar);
		return Out;
	}


	/************************************************************************************************/


	size_t AddUIntVar(Console* C, const char* Identifier, size_t uint )
	{
		size_t Out = C->Variables.size();

		C->ConsoleUInts.push_back(uint);
		size_t& Value = C->ConsoleUInts.back();

		ConsoleVariable NewVar;
		NewVar.Data_ptr					= &Value;
		NewVar.Data_size				= sizeof(size_t);
		NewVar.VariableIdentifier.str	= Identifier;
		NewVar.Type						= ConsoleVariableType::CONSOLE_UINT;

		C->Variables.push_back(NewVar);
		return Out;
	}


	/************************************************************************************************/


	size_t BindIntVar(Console* C, const char* Identifier, int* _ptr)
	{
		size_t Out = C->Variables.size();
		ConsoleVariable NewVar;
		NewVar.Data_ptr					= (void*)_ptr;
		NewVar.Data_size				= sizeof(int);
		NewVar.VariableIdentifier.str	= Identifier;
		NewVar.Type						= ConsoleVariableType::CONSOLE_INT;

		C->Variables.push_back(NewVar);
		return Out;
	}


	/************************************************************************************************/


	size_t BindUIntVar(Console* C, const char* Identifier, size_t* _ptr)
	{
		size_t Out = C->Variables.size();
		ConsoleVariable NewVar;
		NewVar.Data_ptr					= (void*)_ptr;
		NewVar.Data_size				= sizeof(size_t);
		NewVar.VariableIdentifier.str	= Identifier;
		NewVar.Type						= ConsoleVariableType::CONSOLE_UINT;

		C->Variables.push_back(NewVar);
		return Out;
	}


	/************************************************************************************************/


	size_t BindBoolVar(Console* C, const char* Identifier, bool* _ptr)
	{
		size_t Out = C->Variables.size();
		ConsoleVariable NewVar;
		NewVar.Data_ptr					= (void*)_ptr;
		NewVar.Data_size				= sizeof(size_t);
		NewVar.VariableIdentifier.str	= Identifier;
		NewVar.Type						= ConsoleVariableType::CONSOLE_BOOL;

		C->Variables.push_back(NewVar);
		return Out;
	}

	/************************************************************************************************/


	void PushCommandToHistory(Console* C, const char* Str, size_t StrLen)
	{
		size_t BufferSize = StrLen + 1;
		char* NewStr = (char*)C->Memory->malloc(BufferSize);

		strcpy_s(NewStr, BufferSize, Str);

		C->CommandHistory.push_back(ConsoleLine(NewStr, C->Memory));
	}


	/************************************************************************************************/


	void AddConsoleFunction(Console* C, ConsoleFunction NewFunc)
	{
		C->FunctionTable.push_back(NewFunc);
		C->BuiltInIdentifiers.push_back({ NewFunc.FunctionName, strlen(NewFunc.FunctionName),	IdentifierType::FUNCTION });
	}


	/************************************************************************************************/


	ConsoleFunction*	FindConsoleFunction(Console* C, const char* str, size_t StrLen)
	{
		for (auto& fn : C->FunctionTable)
			if (!strncmp(fn.FunctionName, str, min(StrLen, strlen(fn.FunctionName))))
				return &fn;

		return nullptr;
	}


	/************************************************************************************************/


	void ConsolePrint(Console* out, const char* _ptr, iAllocator* Allocator)
	{
		out->Lines.push_back({ _ptr, Allocator });
	}


	/************************************************************************************************/
}