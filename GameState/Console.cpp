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

#include "Console.h"

bool GetVariable(Console* C, ConsoleVariable* Arguments, size_t ArguementCount);

void InitateConsole(Console* Out, FontAsset* Font, EngineMemory* Engine)
{
	Out->Lines.clear();
	Out->Memory                       = Engine->BlockAllocator;
	Out->Font                         = Font;
	Out->InputBufferSize              = 0;
	Out->Variables.Allocator          = Out->Memory;
	Out->FunctionTable.Allocator      = Out->Memory;
	Out->BuiltInIdentifiers.Allocator = Out->Memory;

	Out->FunctionTable.push_back({ "GetVariable", GetVariable, 1,{ ConsoleVariableType::CONSOLE_STRING } });
	Out->BuiltInIdentifiers.push_back({ "GetVariable",	strlen("GetVariable"),	IdentifierType::FUNCTION });

	AddStringVar(Out, "Version", "Pre-Alpha 0.0.0.1");
	AddStringVar(Out, "BuildDate", __DATE__);


	memset(Out->InputBuffer, '\0', sizeof(Out->InputBuffer));
}


/************************************************************************************************/


void ReleaseConsole(Console* out)
{
	//ConsolePrintf(out, 1, 2, 3, 4);
}


/************************************************************************************************/


void DrawConsole(Console* C, ImmediateRender* IR, uint2 Window_WH)
{
	const float LineHeight = (float(C->Font->FontSize[1]) / Window_WH[1]) / 4;
	const float AspectRatio = float(Window_WH[0]) / float(Window_WH[1]);
	size_t itr = 0;

	float y = 1.0f - float(1 + (itr)) * LineHeight;
	PrintText(IR, C->InputBuffer, C->Font, { 0, y }, float2(1.0f, 1.0f) - float2(0.0f, y), float4(WHITE, 1.0f), { 0.5f / AspectRatio, 0.5f });

	for (auto Line : C->Lines) {
		float y = 1.0f - float(2 + (itr)) * LineHeight ;

		if (y > 0) {
			float2 Position(0.0f, y);

			PrintText(IR, Line, C->Font, Position, float2(1.0f, 1.0f) - Position, float4(WHITE, 1.0f), { 0.5f / AspectRatio, 0.5f });
			itr++;
		}
		else
			break;
	}
}


/************************************************************************************************/


void InputConsole(Console* C, char InputCharacter)
{
	C->InputBuffer[C->InputBufferSize++] = InputCharacter;
}


/************************************************************************************************/


bool GetVariable(Console* C, ConsoleVariable* Arguments, size_t ArguementCount)
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


DynArray<InputToken> GetTokens(iAllocator* Memory, const char* Str, ErrorTable)
{
	DynArray<InputToken> Out(Memory);

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

ConsoleSyntax IdentifyToken(DynArray<InputToken>& in, size_t i, ConsoleIdentifierTable& Identifiers, TokenList& Tokens, ErrorTable& ErrorHandler)
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
						// Count the Number of Arguements
						size_t ii = 0;
						while ((i + ii) < in.size() && in[i + ii + 2].Type != InputToken::CT_SYMBOL) 
						{ 
							++ii; 
						}

						// Check for Arguement End
						if (in[i+ii].Type != InputToken::CT_SYMBOL && in[i + ii].Str[0] != ')')
							return ConsoleSyntax::SYNTAXERROR;

						return ConsoleSyntax::FUNCTIONCALL;
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


bool ExecuteGrammerTokens(DynArray<GrammerToken>& Tokens, Console* C, DynArray<ConsoleVariable>& TempVariables)
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
						}
						II++;
					}
				}

				Fn->FN_Ptr(C, Arguments.begin(), Arguments.size());
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

bool ProcessTokens(iAllocator* Memory, DynArray<InputToken>& in, Console* C, ErrorTable& ErrorHandler )
{
	bool Success = true;

	DynArray<GrammerToken>		Tokens(Memory);
	DynArray<ConsoleVariable>	TempVariables(Memory);

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
	//C->InputBuffer[C->InputBufferSize++] = '\0';
	char* str = (char*)C->Memory->malloc(C->InputBufferSize + 256);
	strcpy(str, C->InputBuffer);

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


ConsoleFunction*	FindConsoleFunction(Console* C, const char* str, size_t StrLen)
{
	for (auto& fn : C->FunctionTable)
		if (!strncmp(fn.FunctionName, str, min(StrLen, strlen(fn.FunctionName))))
			return &fn;

	return nullptr;
}


void ConsolePrint(Console* out, const char* _ptr, iAllocator* Allocator)
{
	out->Lines.push_back({ _ptr, Allocator });
}


/************************************************************************************************/
