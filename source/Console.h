#ifndef CONSOLE_H
#define CONSOLE_H

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


#include "buildsettings.h"	
#include "EngineCore.h"
#include "Graphics.h"
#include "FrameGraph.h"
#include "GuiUtilities.h"

namespace FlexKit
{
	struct Console;

	struct InputToken
	{
		const char* Str;
		size_t StrLen;

		enum InputTokenType
		{
			CT_IDENTIFIER,
			CT_SYMBOL,
			CT_NUMBER,
			CT_STRINGLITERAL,

			CT_UNKNOWN
		}Type;
	};


	enum class ConsoleSyntax
	{
		VARIABLEDECLARATION,
		VARIABLEASSIGNMENT,
		FUNCTIONCALL,
		IDENTIFIER,
		UNKNOWNIDENTIFIER,
		CONSTVARIABLE,
		CONSTNUMBER,
		CONSTFLOAT,
		ARGUEMENTSBEGIN,
		ARGUEMENTSEND,
		OPERATOR,
		ENDSTATEMENT,
		UNUSEDSYMBOL,
		SYNTAXERROR,
	};

	struct GrammerToken
	{
		InputToken*		Token;
		size_t			Usr;
		ConsoleSyntax	SyntaxType;
	};

	typedef Vector<GrammerToken> TokenList;

	enum class ConsoleVariableType
	{
		CONSOLE_IDENTIFIER,
		CONSOLE_FLOAT,
		CONSOLE_STRING,
		CONSOLE_INT,
		CONSOLE_UINT,
		CONSOLE_ARRAY,
		CONSOLE_BOOL,

		STACK_FLOAT,
		STACK_STRING,
		STACK_INT,
		STACK_UINT,

		CONSOLE_COMPLEX,
	};


	enum class IdentifierType
	{
		FUNCTION,
		OPERATOR,
		VARIABLE,
		TYPE,
	};


	struct Identifier
	{
		const char*		Str;
		size_t			size;

		IdentifierType	Type;
	};


	struct ConsoleVariable
	{
		union Identifier {
			size_t		Index;
			const char*	str;
		}					VariableIdentifier;

		ConsoleVariableType	Type;
		void*				Data_ptr;
		size_t				Data_size;
	};


	typedef bool Console_FN(Console* C, ConsoleVariable*, size_t ArguementCount, void* USER);


	struct ConsoleFunction
	{
		const char*								FunctionName;
		Console_FN*								FN_Ptr;
		void*									USER;
		size_t									ExpectedArguementCount;
		static_vector<ConsoleVariableType, 6>	ExpectedArguementTypes;
	};

	struct ErrorEntry
	{
		const char* ErrorString;
		enum ErrorType
		{
			InvalidSyntax, 
			UnknownIdentified,
			InvalidOperation,
			InvalidArguments
		}ErrorCode;
	};


	struct ErrorTable
	{
		Vector<ErrorEntry> ErrorStack;
	};


	typedef Vector<Identifier>			ConsoleIdentifierTable;
	typedef Vector<ConsoleFunction>		ConsoleFunctionTable;

	struct ConsoleLine
	{
		ConsoleLine(const char* str = nullptr, iAllocator* memory = nullptr) : Str(str), Memory(memory)
		{}

		~ConsoleLine()
		{
			if (Memory)
				Memory->free((void*)Str);

			Memory	= nullptr;
			Str		= nullptr;
		}

		ConsoleLine(ConsoleLine&& rhs)
		{
			Memory	= rhs.Memory;
			Str		= rhs.Str;

			rhs.Str		= nullptr;
			rhs.Memory	= nullptr;
		}

		ConsoleLine& operator = (ConsoleLine&& rhs)
		{
			Memory		= rhs.Memory;
			Str			= rhs.Str;

			rhs.Str		= nullptr;
			rhs.Memory	= nullptr;

			return *this;
		}

		const char* Str;
		iAllocator*	Memory;

		operator const char* (){ return Str; }
	};


	struct Console
	{
		Console(SpriteFontAsset* font, RenderSystem& renderSystem, iAllocator* allocator);
		~Console();

		void Release();

		void Draw(FrameGraph& Graph, ResourceHandle RenderTarget, iAllocator* TempMemory);


		void Input( char InputCharacter );
		void EnterLine( iAllocator* Memory );
		void BackSpace();


		size_t AddStringVar	( const char* Identifier, const char* Str );
		size_t AddUIntVar	( const char* Identifier, size_t uint );

		size_t BindIntVar	( const char* Identifier, int* _ptr		);
		size_t BindUIntVar	( const char* Identifier, size_t* _ptr	);
		size_t BindBoolVar	( const char* Identifier, bool* _ptr	);


		void				PushCommandToHistory( const char* str, size_t StrLen );

		void				AddFunction	( ConsoleFunction NewFunc );
		void				AddOperator	( ConsoleFunction NewFunc );

		ConsoleFunction*	FindFunction			( const char* str, size_t StrLen );
		bool				ExecuteGrammerTokens	(Vector<GrammerToken>& Tokens, Vector<ConsoleVariable>& TempVariables, iAllocator* Stack);
		bool				ProcessTokens			(iAllocator* persistent, iAllocator* temporary, Vector<InputToken>& in, ErrorTable& errorHandler);


		void				PrintLine( const char* _ptr, iAllocator* Memory = nullptr );

		/************************************************************************************************/

		VertexBufferHandle		vertexBuffer    = InvalidHandle; 
		VertexBufferHandle		textBuffer      = InvalidHandle;
		ConstantBufferHandle	constantBuffer  = InvalidHandle;

		CircularBuffer<ConsoleLine, 32>	lines;
		CircularBuffer<ConsoleLine, 32>	commandHistory;
		SpriteFontAsset*				font;

		Vector<ConsoleVariable>		variables;
		ConsoleFunctionTable		functionTable;
		ConsoleIdentifierTable		builtInIdentifiers;

		Vector<size_t>				consoleUInts;

		char	inputBuffer[1024];
		size_t	inputBufferSize;

        RenderSystem& renderSystem;
		iAllocator*	allocator;

		const char* engineVersion = "Version: Pre-Alpha 0.0.0.3:" __DATE__;
	};


	/************************************************************************************************/

} // namespace FlexKit;

#endif
