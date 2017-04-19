#ifndef CONSOLE_H
#define CONSOLE_H

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


#include "..\buildsettings.h"	
#include "..\Application\GameMemory.h"
#include "..\graphicsutilities\Graphics.h"
#include "..\graphicsutilities\GuiUtilities.h"

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
	FUNCTIONCALL,
	IDENTIFIER,
	UNKNOWNIDENTIFIER,
	CONSTVARIABLE,
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

typedef DynArray<GrammerToken> TokenList;

enum class ConsoleVariableType
{
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


typedef bool Console_FN(Console* C, ConsoleVariable*, size_t ArguementCount);

struct ConsoleFunction
{
	const char*								FunctionName;
	Console_FN*								FN_Ptr;
	size_t									ExpectedArguementCount;
	static_vector<ConsoleVariableType, 6>	ExpectedArguementTypes;
};

struct ErrorTable
{
};

typedef FlexKit::DynArray<Identifier>		ConsoleIdentifierTable;
typedef FlexKit::DynArray<ConsoleFunction>	ConsoleFunctionTable;

struct ConsoleLine
{
	ConsoleLine(const char* str = nullptr, iAllocator* memory = nullptr) : Str(str), Memory(memory)
	{}

	~ConsoleLine()
	{
		if (Memory)
			Memory->free((void*)Str);
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

	operator const char* (){return Str; }
};

struct Console
{
	FlexKit::CircularBuffer<ConsoleLine, 4>		Lines;
	FlexKit::FontAsset*							Font;

	DynArray<ConsoleVariable>	Variables;
	ConsoleFunctionTable		FunctionTable;
	ConsoleIdentifierTable		BuiltInIdentifiers;

	char	InputBuffer[1024];
	size_t	InputBufferSize;

	iAllocator*	Memory;

	const char* EngineVersion = "Version: Pre-Alpha 0.0.0.1";
};


/************************************************************************************************/


void InitateConsole ( Console* out, FontAsset* Font, EngineMemory* Engine );	
void ReleaseConsole	( Console* out );

void DrawConsole	( Console* C, ImmediateRender* IR, uint2 Window_WH );

void InputConsole		( Console* C, char InputCharacter );
void EnterLineConsole	( Console* C );
void BackSpaceConsole	( Console* C );


size_t AddStringVar( Console* C, const char* Identifier, const char* Str );

size_t BindIntVar	( Console* C, const char* Identifier, int* _ptr );
size_t BindUIntVar	( Console* C, const char* Identifier, size_t* _ptr );

size_t BindBoolVar ( Console* C, const char* Identifier, bool* _ptr );


void				AddConsoleFunction( Console* C, ConsoleFunction NewFunc);
ConsoleFunction*	FindConsoleFunction(Console* C, const char* str, size_t StrLen);

void ConsolePrint	( Console* out, const char* _ptr, iAllocator* Memory = nullptr );
void ConsolePrintf	( Console* out );


/************************************************************************************************/


template<typename Ty, typename ... Ty_Args >
void ConsolePrintf(Console* out, const char* _ptr, Ty, Ty_Args ... Args )
{
	ConsolePrintf(out, Args...);
}

#endif