/**********************************************************************

Copyright (c) 2019 Robert May

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
#include "TextRendering.h"
#include "defaultpipelinestates.h"


namespace FlexKit
{	/************************************************************************************************/


	bool PrintVar		(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void*);
	bool ListVars		(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void*);
	bool ListFunctions	(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void*);
	bool ToggleBool		(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void*);


	bool OperatorAssign	(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void*);


	/************************************************************************************************/



	Console::Console(SpriteFontAsset* IN_font, RenderSystem& IN_renderSystem, iAllocator* IN_allocator) :
        vertexBuffer    { IN_renderSystem.CreateVertexBuffer(8096 * 64, false)     },
        textBuffer      { IN_renderSystem.CreateVertexBuffer(8096 * 64, false)     },
        constantBuffer  { IN_renderSystem.CreateConstantBuffer(1024 * 32, false)   },
        renderSystem    { IN_renderSystem }
	{
		lines.clear();
		allocator					 = IN_allocator;
		font                         = IN_font;
		inputBufferSize              = 0;
		variables.Allocator          = allocator;
		functionTable.Allocator      = allocator;
		builtInIdentifiers.Allocator = allocator;
		consoleUInts.Allocator		 = allocator;

		AddFunction({ "PrintVar",		PrintVar,		nullptr, 1, { ConsoleVariableType::CONSOLE_STRING } });
		AddFunction({ "ListVars",		ListVars,		nullptr, 0, {} });
		AddFunction({ "ListFunctions",	ListFunctions,	nullptr, 0, {} });
		AddFunction({ "Toggle",			ToggleBool,		nullptr, 1, { ConsoleVariableType::CONSOLE_STRING } });

		AddOperator({ "=",				OperatorAssign,	nullptr, 2, { ConsoleVariableType::CONSOLE_IDENTIFIER, ConsoleVariableType::STACK_INT } });

		AddStringVar("Version", "Pre-Alpha 0.0.0.3");
		AddStringVar("BuildDate", __DATE__);

		PrintLine("Type ListFunctions() for a list of Commands\n",	allocator);
		PrintLine("Up and Down Keys to go through history\n",		allocator);

		::memset(inputBuffer, '\0', sizeof(inputBuffer));
	}


	/************************************************************************************************/


	Console::~Console()
	{
		Release();
	}


	/************************************************************************************************/


	void Console::Release()
	{
		lines.Release();
		commandHistory.Release();
		consoleUInts.Release();
		variables.Release();
		functionTable.Release();
		builtInIdentifiers.Release();
	}


	/************************************************************************************************/

	void Console::Draw(FrameGraph& graph, ResourceHandle renderTarget, iAllocator* allocator)
	{
		if (!font) {
			FK_LOG_ERROR("Console has Null font, aborting draw!");
			return;
		}
		const auto WindowWH = graph.Resources.renderSystem.GetTextureWH(renderTarget);

		const float		HeightScale			= 0.5f;
		const auto		FontSize			= font->FontSize;
		const float2	PixelSize			= { 0.5f / (float)WindowWH[0],  0.5f / (float)WindowWH[1]};
		const float		LineHeight			= FontSize[1] * PixelSize[1] * HeightScale;
		const float		AspectRatio			= float(WindowWH[0]) / float(WindowWH[1]);
		const float2	StartingPosition	= float2{ 1	, 0.5f + (FontSize[1] * PixelSize[1]) };

        FK_ASSERT(0);
        /*
		DrawShapes(
				DRAW_PSO, graph, 
				vertexBuffer,
				constantBuffer,
				renderTarget,
				allocator,
            RectangleShape{
                float2{0.0f	, 0.0f},
                StartingPosition, //
                { 0.25f, 0.25f, 0.25f, 1.0f } });
        */

		size_t	itr				= 1;
		float	y				= 0.5f - float(1 + (itr)) * LineHeight;

		PrintTextFormatting format = PrintTextFormatting::DefaultParams();
		format.StartingPOS = {0, 0.5f};
		format.TextArea    = float2(1,1);
		format.Color       = float4(1.0f, 1.0f, 1.0f, 1.0f);
		format.Scale       = { 1.0f / AspectRatio, 1.0f };
		format.PixelSize   = float2{ 1.0f, 1.0f } / WindowWH;
		format.CenterX	   = false;
		format.CenterY	   = false;

		DrawSprite_Text(
				inputBuffer, 
				graph, 
				*font, 
				textBuffer, 
				renderTarget, 
				allocator, 
				format);

		for (auto& Line : lines) {
			float y = 0.5f - LineHeight * itr * 2;

			if (y > 0) {
				float2 Position(0.0f, y);
				format.StartingPOS	= Position;
				format.TextArea		= float2(1.0f, 1.0f) - Position;
				format.CurrentX		= 0;
				format.CurrentY		= 0;

				DrawSprite_Text(
					Line.Str,
					graph,
					*font,
					textBuffer,
					renderTarget,
					allocator, 
					format);

				itr++;
			}
			else
				break;
		}
	}


	/************************************************************************************************/


	bool ListFunctions(Console* C, ConsoleVariable* arguments, size_t arguementCount, void*)
	{
		C->PrintLine("Function List:", nullptr);

		for (auto& function : C->functionTable)
			C->PrintLine(function.FunctionName, nullptr);

		return true;
	}


	/************************************************************************************************/


	void Console::Input(char inputCharacter)
	{
		inputBuffer[inputBufferSize++] = inputCharacter;
	}


	/************************************************************************************************/


	bool ToggleBool(Console* console, ConsoleVariable* Arguments, size_t ArguementCount, void*)
	{
		if (ArguementCount != 1 &&
			(Arguments->Type != ConsoleVariableType::CONSOLE_STRING) ||
			(Arguments->Type != ConsoleVariableType::STACK_STRING)) {
			console->PrintLine("INVALID NUMBER OF ARGUMENTS OR INVALID ARGUMENT!");
			return false;
		}

		const char* variableIdentifier = (const char*)Arguments->Data_ptr;
		for (auto& variable : console->variables)
		{
			if (!strncmp(
				variable.VariableIdentifier.str, 
				variableIdentifier, 
                Min(strlen(
					variable.VariableIdentifier.str), Arguments->Data_size)))
			{
				if (variable.Type == ConsoleVariableType::CONSOLE_BOOL) {
					*(bool*)variable.Data_ptr = !(*(bool*)variable.Data_ptr);
					return true;
				}
				else
				{
					console->PrintLine("Invalid Target Variable!");
					return false;
				}
			}
		}
		return false;
	}


	bool ListVars(Console* console, ConsoleVariable* Arguments, size_t ArguementCount, void*)
	{
		console->PrintLine("Listing Variables:", nullptr);

		for (auto& variable : console->variables)
		{
			console->PrintLine(variable.VariableIdentifier.str, nullptr);
		}
		return true;
	}


	bool PrintVar(Console* console, ConsoleVariable* Arguments, size_t ArguementCount, void*)
	{
		if (ArguementCount != 1 && 
			(Arguments->Type != ConsoleVariableType::CONSOLE_STRING) || 
			(Arguments->Type != ConsoleVariableType::STACK_STRING)) {
			console->PrintLine("INVALID NUMBER OF ARGUMENTS OR INVALID ARGUMENT!");
			return false;
		}

		const char* VariableIdentifier = (const char*)Arguments->Data_ptr;
		for (auto Var : console->variables)
		{
			if (!strncmp(Var.VariableIdentifier.str, VariableIdentifier, Min(strlen(Var.VariableIdentifier.str), Arguments->Data_size)))
			{
				// Print Variable
				switch (Var.Type)
				{
				case ConsoleVariableType::CONSOLE_STRING:
					console->PrintLine((const char*)Var.Data_ptr);
					break;
				case ConsoleVariableType::CONSOLE_UINT:
				{
					char* Str = (char*)console->allocator->malloc(64);
					memset(Str, '\0', 64);

					_itoa_s((uint32_t)*(size_t*)Var.Data_ptr, Str, 64, 10);
					console->PrintLine(Str, console->allocator);
				}	break;
				case ConsoleVariableType::CONSOLE_BOOL:
				{
					if(*(bool*)Var.Data_ptr)
						console->PrintLine("True", console->allocator);
					else
						console->PrintLine("False", console->allocator);
				}	break;
				default:
					break;
				}
				return true;
			}
		}

		console->PrintLine("Variable Not Found!");
		return false;
	}


	/************************************************************************************************/


	bool OperatorAssign(Console* console, ConsoleVariable* Arguments, size_t ArguementCount, void*)
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
			console->AddUIntVar(Arguments[0].VariableIdentifier.str, size_t(*(int*)Arguments[1].Data_ptr));
			return true;
		}

		if (Arguments[1].Type == ConsoleVariableType::STACK_STRING)
		{
			const char*		Str		= (const char*)Arguments[1].Data_ptr;
			const size_t	StrSize	= Arguments[1].Data_size;

			char*	Str2 = (char*)console->allocator->malloc(StrSize);
			memset(Str2, '\0', StrSize);
			memcpy(Str2, Str, StrSize);

			console->AddStringVar(Arguments[0].VariableIdentifier.str, Str2);
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
				if (!strncmp(in[i].Str, I.Str, Min(in[i].StrLen, I.size)))
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


	bool Console::ExecuteGrammerTokens(Vector<GrammerToken>& Tokens, Vector<ConsoleVariable>& TempVariables, iAllocator* Stack)
	{
		struct TranslationRes
		{
			bool			Success;
			ConsoleVariable Var;
		};


		const auto TranslateTokenToVar = [&](GrammerToken& T) -> TranslationRes
		{
			return { false };
		};


		const auto GetVars = [&](auto& Begin, auto& End, auto& Out)
		{

		};
		
		struct TokenRange
		{
			Vector<GrammerToken>::Iterator Begin;
			Vector<GrammerToken>::Iterator End;

			operator bool()
			{
				return (Begin != End);
			}
		};


		const auto GetArguments = [&](auto Itr) -> TokenRange
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
				auto Fn = FindFunction(Token->Token->Str, Token->Token->StrLen);
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

					Fn->FN_Ptr(this, Arguments.begin(), Arguments.size(), Fn->USER);
				}
				else
				{
					PrintLine("Function Not Found");
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
				auto Fn = FindFunction(Token->Token->Str, Token->Token->StrLen);
				if (Fn)
				{
					static_vector<ConsoleVariable> Arguments;
					if ((Tokens[I - 1].SyntaxType == ConsoleSyntax::UNKNOWNIDENTIFIER) ||
						(Tokens[I - 1].SyntaxType == ConsoleSyntax::IDENTIFIER))
					{
						size_t StrLen = Tokens[I - 1].Token->StrLen + 1;
						char* Str = (char*)allocator->malloc(StrLen);
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
						Fn->FN_Ptr(this, Arguments.begin(), Arguments.size(), Fn->USER);
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


	bool Console::ProcessTokens(iAllocator* persistent, iAllocator* temporary, Vector<InputToken>& in, ErrorTable& errorHandler )
	{
		bool Success = true;

		Vector<GrammerToken>		tokens			(persistent);
		Vector<ConsoleVariable>		tempVariables	(persistent);

		// Translate Tokens into Grammer Objects
		for (size_t I = 0; I < in.size(); ++I)
		{
			switch (in[I].Type)
			{
			case InputToken::CT_UNKNOWN:
			{
				auto tokenType = IdentifyToken(in, I, builtInIdentifiers, tokens, errorHandler);
				tokens.push_back({ &in[I], 0, tokenType });
			}	break;
			case InputToken::CT_SYMBOL:
			{
				auto TokenType = IdentifyToken(in, I, builtInIdentifiers, tokens, errorHandler);
				if(TokenType != ConsoleSyntax::UNUSEDSYMBOL)
					tokens.push_back({ &in[I], 0, TokenType });
			}	break;
			case InputToken::CT_NUMBER:
			{
				tokens.push_back({ &in[I], tempVariables.size(), ConsoleSyntax::CONSTVARIABLE });
				ConsoleVariable Var;
				Var.Data_ptr	= &in[I].Str;
				Var.Data_size	=  in[I].StrLen;

				Var.Type					 = ConsoleVariableType::STACK_INT;
				Var.VariableIdentifier.Index = tempVariables.size();

				tempVariables.push_back(Var);
			}	break;
			case InputToken::CT_STRINGLITERAL:
			{
				tokens.push_back({ &in[I], tempVariables.size(), ConsoleSyntax::CONSTVARIABLE });
				ConsoleVariable Var;
				Var.Data_ptr	= (void*)in[I].Str;
				Var.Data_size	= in[I].StrLen;

				Var.Type					 = ConsoleVariableType::STACK_STRING;
				Var.VariableIdentifier.Index = tempVariables.size();

				tempVariables.push_back(Var);
			}	break;
			case InputToken::CT_IDENTIFIER:
				tokens.push_back({ &in[I], 0, ConsoleSyntax::IDENTIFIER });
			default:
				break;
			}
		}

		if( tokens.size() && tokens.back().SyntaxType != ConsoleSyntax::ENDSTATEMENT)
			tokens.push_back({ nullptr, 0, ConsoleSyntax::ENDSTATEMENT });

		Success = ExecuteGrammerTokens(tokens, tempVariables, temporary);

		return Success;
	}


	/************************************************************************************************/


	void Console::EnterLine(iAllocator* temporary)
	{
		if (!inputBufferSize)
			return;

		inputBuffer[inputBufferSize++] = '\0';

		size_t bufferSize	= inputBufferSize;
		char* str			= (char*)allocator->malloc(bufferSize);
		strcpy_s(str, bufferSize, inputBuffer);

		PushCommandToHistory	(str, inputBufferSize);
		PrintLine(str, allocator);

		ErrorTable ErrorHandler;

		std::cout << inputBuffer << "\n";

		auto Tokens = GetTokens(allocator, inputBuffer, ErrorHandler);
		if (!ProcessTokens(
							allocator, 
							temporary, 
							Tokens, 
							ErrorHandler))
		{
			// Handle Errors
		}


		memset(inputBuffer, '\0', inputBufferSize);
		inputBufferSize = 0;
	}


	/************************************************************************************************/


	void Console::BackSpace()
	{
		if(inputBufferSize)
			inputBuffer[--inputBufferSize] = '\0';
	}


	/************************************************************************************************/

	size_t Console::AddStringVar(const char* Identifier, const char* Str)
	{
		ConsoleVariable NewVar;
		NewVar.Data_ptr					= (void*)Str;
		NewVar.Data_size				= strlen(Str);
		NewVar.VariableIdentifier.str	= Identifier;
		NewVar.Type						= ConsoleVariableType::CONSOLE_STRING;

		return variables.push_back(NewVar);
	}


	/************************************************************************************************/


	size_t Console::AddUIntVar(const char* Identifier, size_t uint )
	{
		consoleUInts.push_back(uint);
		size_t& Value = consoleUInts.back();

		ConsoleVariable NewVar;
		NewVar.Data_ptr					= &Value;
		NewVar.Data_size				= sizeof(size_t);
		NewVar.VariableIdentifier.str	= Identifier;
		NewVar.Type						= ConsoleVariableType::CONSOLE_UINT;

		return variables.push_back(NewVar);;
	}


	/************************************************************************************************/


	size_t Console::BindIntVar(const char* Identifier, int* _ptr)
	{
		ConsoleVariable NewVar;
		NewVar.Data_ptr					= (void*)_ptr;
		NewVar.Data_size				= sizeof(int);
		NewVar.VariableIdentifier.str	= Identifier;
		NewVar.Type						= ConsoleVariableType::CONSOLE_INT;

		return variables.push_back(NewVar);
	}


	/************************************************************************************************/


	size_t Console::BindUIntVar(const char* Identifier, size_t* _ptr)
	{
		size_t OUT_idx = variables.size();
		ConsoleVariable NewVar;
		NewVar.Data_ptr					= (void*)_ptr;
		NewVar.Data_size				= sizeof(size_t);
		NewVar.VariableIdentifier.str	= Identifier;
		NewVar.Type						= ConsoleVariableType::CONSOLE_UINT;

		variables.push_back(NewVar);

		return OUT_idx;
	}


	/************************************************************************************************/


	size_t Console::BindBoolVar(const char* Identifier, bool* _ptr)
	{
		size_t OUT_idx = variables.size();
		ConsoleVariable NewVar;
		NewVar.Data_ptr					= (void*)_ptr;
		NewVar.Data_size				= sizeof(size_t);
		NewVar.VariableIdentifier.str	= Identifier;
		NewVar.Type						= ConsoleVariableType::CONSOLE_BOOL;

		variables.push_back(NewVar);

		return OUT_idx;
	}


	/************************************************************************************************/


	void Console::PushCommandToHistory(const char* Str, size_t StrLen)
	{
		commandHistory.push_back(std::move(ConsoleLine(Str, allocator)));
	}


	/************************************************************************************************/


	void Console::AddFunction(ConsoleFunction NewFunc)
	{
		functionTable.push_back(NewFunc);
		builtInIdentifiers.push_back({ NewFunc.FunctionName, strlen(NewFunc.FunctionName),	IdentifierType::FUNCTION });
	}


	/************************************************************************************************/


	void Console::AddOperator(ConsoleFunction NewFunc)
	{
		functionTable.push_back(NewFunc);
		builtInIdentifiers.push_back({ NewFunc.FunctionName, strlen(NewFunc.FunctionName),	IdentifierType::OPERATOR });
	}

	/************************************************************************************************/


	ConsoleFunction* Console::FindFunction(const char* str, size_t StrLen)
	{
		for (auto& fn : functionTable)
			if (!strncmp(fn.FunctionName, str, Min(StrLen, strlen(fn.FunctionName))))
				return &fn;

		return nullptr;
	}


	/************************************************************************************************/


	void Console::PrintLine(const char* _ptr, iAllocator* Allocator)
	{
		lines.push_back({ _ptr, Allocator });
	}


	/************************************************************************************************/
}
