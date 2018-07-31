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
#include "../graphicsutilities/TextRendering.h"

namespace FlexKit
{	/************************************************************************************************/


	bool PrintVar		(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void*);
	bool ListVars		(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void*);
	bool ListFunctions	(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void*);
	bool ToggleBool		(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void*);


	bool OperatorAssign	(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void*);


	/************************************************************************************************/



	void InitateConsole(Console* Out, SpriteFontAsset* Font, EngineMemory* Memory)
	{
		Out->Lines.clear();
		Out->Memory                       = Memory->BlockAllocator;
		Out->Font                         = Font;
		Out->InputBufferSize              = 0;
		Out->Variables.Allocator          = Out->Memory;
		Out->FunctionTable.Allocator      = Out->Memory;
		Out->BuiltInIdentifiers.Allocator = Out->Memory;
		Out->ConsoleUInts.Allocator		  = Out->Memory;

		AddConsoleFunction(Out, { "PrintVar",		PrintVar,		nullptr, 1, { ConsoleVariableType::CONSOLE_STRING } });
		AddConsoleFunction(Out, { "ListVars",		ListVars,		nullptr, 0, {} });
		AddConsoleFunction(Out, { "ListFunctions",	ListFunctions,	nullptr, 0, {} });
		AddConsoleFunction(Out, { "Toggle",			ToggleBool,		nullptr, 1, { ConsoleVariableType::CONSOLE_STRING } });

		AddConsoleOperator(Out, { "=",				OperatorAssign,	nullptr, 2, { ConsoleVariableType::CONSOLE_IDENTIFIER, ConsoleVariableType::STACK_INT } });


		AddStringVar(Out, "Version", "Pre-Alpha 0.0.0.1");
		AddStringVar(Out, "BuildDate", __DATE__);

		ConsolePrint(Out, "Type ListFunctions() for a list of Commands\n", Out->Memory);
		ConsolePrint(Out, "Up and Down Keys to go through history\n", Out->Memory);

		::memset(Out->InputBuffer, '\0', sizeof(Out->InputBuffer));
	}


	/************************************************************************************************/


	void InitateConsole(Console* Out, SpriteFontAsset* Font, EngineCore* Engine)
	{
		Out->ConstantBuffer				  = Engine->RenderSystem.CreateConstantBuffer(8096 * 2000, false);
		Out->VertexBuffer				  = Engine->RenderSystem.CreateVertexBuffer	 (8096 * 2000, false);
		Out->TextBuffer					  = Engine->RenderSystem.CreateVertexBuffer	 (8096 * 2000, false);

		Out->Lines.clear();
		Out->Memory                       = Engine->GetBlockMemory();
		Out->Font                         = Font;
		Out->InputBufferSize              = 0;
		Out->Variables.Allocator          = Out->Memory;
		Out->FunctionTable.Allocator      = Out->Memory;
		Out->BuiltInIdentifiers.Allocator = Out->Memory;
		Out->ConsoleUInts.Allocator		  = Out->Memory;

		AddConsoleFunction(Out, { "PrintVar",		PrintVar,		nullptr, 1, { ConsoleVariableType::CONSOLE_STRING } });
		AddConsoleFunction(Out, { "ListVars",		ListVars,		nullptr, 0, {} });
		AddConsoleFunction(Out, { "ListFunctions",	ListFunctions,	nullptr, 0, {} });
		AddConsoleFunction(Out, { "Toggle",			ToggleBool,		nullptr, 1, { ConsoleVariableType::CONSOLE_STRING } });

		AddConsoleOperator(Out, { "=",				OperatorAssign,	nullptr, 2, { ConsoleVariableType::CONSOLE_IDENTIFIER, ConsoleVariableType::STACK_INT } });

		AddStringVar(Out, "Version", "Pre-Alpha 0.0.0.1");
		AddStringVar(Out, "BuildDate", __DATE__);

		ConsolePrint(Out, "Type ListFunctions() for a list of Commands\n", Out->Memory);
		ConsolePrint(Out, "Up and Down Keys to go through history\n", Out->Memory);

		::memset(Out->InputBuffer, '\0', sizeof(Out->InputBuffer));
	}


	/************************************************************************************************/


	void ReleaseConsole(Console* C)
	{
		C->Lines.Release();
		C->CommandHistory.Release();
		C->ConsoleUInts.Release();
		C->Variables.Release();
		C->FunctionTable.Release();
		C->BuiltInIdentifiers.Release();
	}


	/************************************************************************************************/

	void DrawConsole(Console* C, FrameGraph& Graph, TextureHandle RenderTarget, iAllocator* TempMemory)
	{
		auto WindowWH = Graph.Resources.RenderSystem->GetRenderTargetWH(RenderTarget);

		ClearVertexBuffer(Graph, C->VertexBuffer);
		ClearVertexBuffer(Graph, C->TextBuffer);

		const float		HeightScale			= 0.5f;
		const auto		FontSize			= C->Font->FontSize;
		const float2	PixelSize			= { 0.5f / (float)WindowWH[0],  0.5f / (float)WindowWH[1]};
		const float		LineHeight			= FontSize[1] * PixelSize[1] * HeightScale;
		const float		AspectRatio			= float(WindowWH[0]) / float(WindowWH[1]);
		const float2	StartingPosition	= float2{ 1	, 0.5f + (FontSize[1] * PixelSize[1]) };


		DrawShapes(
				EPIPELINESTATES::DRAW_PSO, Graph, 
				C->VertexBuffer,
				C->ConstantBuffer,
				RenderTarget,
				TempMemory,
			RectangleShape(
				float2{0.0f	, 0.0f},
				StartingPosition, //
				{ 0.25f, 0.25f, 0.25f, 1.0f }));

		size_t	itr				= 1;
		float	y				= 0.5f - float(1 + (itr)) * LineHeight;

		PrintTextFormatting Format = PrintTextFormatting::DefaultParams();
		Format.StartingPOS = {0, 0.5f};
		Format.TextArea    = float2(1,1);
		Format.Color       = float4(1.0f, 1.0f, 1.0f, 1.0f);
		Format.Scale       = { 1.0f / AspectRatio, 1.0f };
		Format.PixelSize   = float2{ 1.0f, 1.0f } / WindowWH;
		Format.CenterX	   = false;
		Format.CenterY	   = false;

		DrawSprite_Text(
				C->InputBuffer, 
				Graph, 
				*C->Font, 
				C->TextBuffer, 
				RenderTarget, 
				TempMemory, 
				Format);

		for (auto& Line : C->Lines) {
			float y = 0.5f - LineHeight * itr * 2;

			if (y > 0) {
				float2 Position(0.0f, y);
				Format.StartingPOS	= Position;
				Format.TextArea		= float2(1.0f, 1.0f) - Position;
				Format.CurrentX		= 0;
				Format.CurrentY		= 0;
				DrawSprite_Text(
					Line.Str,
					Graph,
					*C->Font,
					C->TextBuffer,
					RenderTarget,
					TempMemory, 
					Format);

				itr++;
			}
			else
				break;
		}
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


	bool OperatorAssign(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void*)
	{
		if	((ArguementCount != 2) &&
			(	(Arguments[0].Type != ConsoleVariableType::CONSOLE_IDENTIFIER) &&
			(
				Arguments[1].Type != ConsoleVariableType::STACK_INT ||
				Arguments[1].Type != ConsoleVariableType::STACK_STRING)))
		{
			return false;
		}
		
		if (Arguments[1].Type == ConsoleVariableType::STACK_INT)
		{
			AddUIntVar(C, Arguments[0].VariableIdentifier.str, size_t(*(int*)Arguments[1].Data_ptr));
			return true;
		}

		if (Arguments[1].Type == ConsoleVariableType::STACK_STRING)
		{
			const char*		Str		= (const char*)Arguments[1].Data_ptr;
			const size_t	StrSize	= Arguments[1].Data_size;

			char*	Str2 = (char*)C->Memory->malloc(StrSize);
			memset(Str2, '\0', StrSize);
			memcpy(Str2, Str, StrSize);

			AddStringVar(C, Arguments[0].VariableIdentifier.str, Str2);
			return true;
		}

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
			case '\0':	// Str End Reached
			{
				size_t Length = (Str + I) - TokenStrBegin;
				if(Length)
					Out.push_back({ TokenStrBegin, Length, InputToken::CT_UNKNOWN });

				TokenStrBegin = Str + I + 1;
			}	break;
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
			}	break;

			// Operators
			case '=':
			{
				Out.push_back({ Str + I, 1, InputToken::CT_SYMBOL });
				TokenStrBegin = Str + I + 1;
			}	break;
			default:
				int x = 0;
				break;
			}
			++I;
		}

		return Out;
	}


	/************************************************************************************************/


	struct NumberInfo
	{
		bool IsNumber;
		bool IsFloat;
	};

	NumberInfo IsNumber(const char* str, size_t len)
	{
		bool DecimalReached = false;

		for (size_t I = 0; I < len; ++I)
		{
			switch (str[I])
			{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				break;
			case '.':
				DecimalReached = true;
			default:
				return {false, false};
			};
		}

		return {true, DecimalReached};
	}


	/************************************************************************************************/


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

			{
				auto [Number, Float] = IsNumber(in[i].Str, in[i].StrLen);
			if (Number && !Float)
				return ConsoleSyntax::CONSTNUMBER;
			else if(Number && Float)
				return ConsoleSyntax::CONSTFLOAT;
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
			case ')':
				return ConsoleSyntax::ARGUEMENTSEND;
			case '=':
				return ConsoleSyntax::OPERATOR;
			default:
				break;
			}
		}	break;
		default:
			break;
		}

		return ConsoleSyntax::SYNTAXERROR;
	}


	/************************************************************************************************/


	bool ExecuteGrammerTokens(Vector<GrammerToken>& Tokens, Console* C, Vector<ConsoleVariable>& TempVariables, iAllocator* Stack)
	{
		struct ScanContext
		{

		};


		struct TranslationRes
		{
			bool			Success;
			ConsoleVariable Var;
		};


		auto& TranslateTokenToVar = [&](Token& T) -> TranslationRes
		{
			return { false };
		};


		auto& GetVars = [&](auto& Begin, auto& End, auto& Out)
		{

		};
		
		struct TokenRange
		{
			Vector<Token>::Iterator Begin;
			Vector<Token>::Iterator End;

			operator bool()
			{
				return (Begin != End);
			}
		};


		auto& GetArguements = [&](auto Itr) -> TokenRange
		{
			auto Begin = (SyntaxType == ConsoleSyntax::ARGUEMENTSBEGIN) 
				? (Itr + 1) : Itr;

			do
			{
				if (itr->SyntaxType == ConsoleSyntax::ARGUEMENTSBEGIN)
				{
					return { Begin, itr - 1 };
				}
			} while (true);

			return {Itr, Itr};
		};


		auto begin	= Tokens.begin();
		auto end	= Tokens.end();

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
			case ConsoleSyntax::UNKNOWNIDENTIFIER:
			case ConsoleSyntax::IDENTIFIER:
				continue;
			case ConsoleSyntax::OPERATOR:
			{
				auto Token = &Tokens[I];
				auto Fn = FindConsoleFunction(C, Token->Token->Str, Token->Token->StrLen);
				if (Fn)
				{
					static_vector<ConsoleVariable> Arguments;
					if ((Tokens[I - 1].SyntaxType == ConsoleSyntax::UNKNOWNIDENTIFIER) ||
						(Tokens[I - 1].SyntaxType == ConsoleSyntax::IDENTIFIER))
					{
						size_t StrLen = Tokens[I - 1].Token->StrLen + 1;
						char* Str = (char*)C->Memory->malloc(StrLen);
						memset(Str, '\0', StrLen);
						memcpy(Str, Tokens[I - 1].Token->Str, StrLen - 1);
						
						ConsoleVariable IDVar;
						IDVar.Type						= ConsoleVariableType::CONSOLE_IDENTIFIER;
						IDVar.VariableIdentifier.str	= Str;
						IDVar.Data_ptr					= nullptr;
						IDVar.Data_size					= 0;

						TempVariables.push_back(IDVar);
						Arguments.push_back(TempVariables.back());
					}

					if (Tokens[I + 1].SyntaxType == ConsoleSyntax::CONSTNUMBER)
					{
						char temp[64];
						memset(temp, '\n', sizeof(temp));
						memcpy(temp, Tokens[I + 1].Token->Str, Tokens[I + 1].Token->StrLen);

						ConsoleVariable IDVar;
						IDVar.Type						= ConsoleVariableType::STACK_INT;
						IDVar.VariableIdentifier.str	= nullptr;
						IDVar.Data_ptr					= &Stack->allocate_aligned<size_t>(atoi(temp));
						IDVar.Data_size					= sizeof(size_t);

						TempVariables.push_back(IDVar);
						Arguments.push_back(TempVariables.back());
					}
					else if (Tokens[I + 1].SyntaxType == ConsoleSyntax::CONSTVARIABLE)
						Arguments.push_back(TempVariables[Tokens[I + 1].Usr]);

					if(Arguments.size() == 2)
						Fn->FN_Ptr(C, Arguments.begin(), Arguments.size(), Fn->USER);
				}
				int x = 0;
			}	return false;
			default:
				//ConsolePrint(C, "INVALID STATEMENT!");

				return false;
				break;
			}
		}
		return false;
	}


	/************************************************************************************************/


	bool ProcessTokens(iAllocator* Memory, iAllocator* TempMemory, Vector<InputToken>& in, Console* C, ErrorTable& ErrorHandler )
	{
		bool Success = true;

		Vector<GrammerToken>		Tokens(Memory);
		Vector<ConsoleVariable>		TempVariables(Memory);

		// Translate Tokens into Grammer Objects
		for (size_t I = 0; I < in.size(); ++I)
		{
			switch (in[I].Type)
			{
			case InputToken::CT_UNKNOWN:
			{
				auto TokenType = IdentifyToken(in, I, C->BuiltInIdentifiers, Tokens, ErrorHandler);
				Tokens.push_back({ &in[I], 0, TokenType });
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

		Success = ExecuteGrammerTokens(Tokens, C, TempVariables, TempMemory);

		return Success;
	}


	/************************************************************************************************/


	void EnterLineConsole(Console* C, iAllocator* TempMemory)
	{
		if (!C->InputBufferSize)
			return;

		C->InputBuffer[C->InputBufferSize++] = '\0';

		size_t BufferSize = C->InputBufferSize;
		char* str = (char*)C->Memory->malloc(BufferSize);
		strcpy_s(str, BufferSize, C->InputBuffer);

		PushCommandToHistory(C, str, C->InputBufferSize);
		ConsolePrint(C, str, C->Memory);

		ErrorTable ErrorHandler;

		std::cout << C->InputBuffer << "\n";

		auto Tokens = GetTokens(C->Memory, C->InputBuffer, ErrorHandler);
		if (!ProcessTokens(
							C->Memory, 
							TempMemory, 
							Tokens, 
							C, 
							ErrorHandler))
		{
			// Handle Errors
		}


		memset(C->InputBuffer, '\0', C->InputBufferSize);
		C->InputBufferSize = 0;

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
		C->CommandHistory.push_back(std::move(ConsoleLine(Str, C->Memory)));
	}


	/************************************************************************************************/


	void AddConsoleFunction(Console* C, ConsoleFunction NewFunc)
	{
		C->FunctionTable.push_back(NewFunc);
		C->BuiltInIdentifiers.push_back({ NewFunc.FunctionName, strlen(NewFunc.FunctionName),	IdentifierType::FUNCTION });
	}


	/************************************************************************************************/


	void AddConsoleOperator(Console* C, ConsoleFunction NewFunc)
	{
		C->FunctionTable.push_back(NewFunc);
		C->BuiltInIdentifiers.push_back({ NewFunc.FunctionName, strlen(NewFunc.FunctionName),	IdentifierType::OPERATOR });
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