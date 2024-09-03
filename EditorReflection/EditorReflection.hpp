#include <clang-c/Index.h>
#include <string>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <variant>
#include <span>

namespace FlexKit
{	/************************************************************************************************/


	struct ClangString
	{
		ClangString(CXString IN_str) : str{ IN_str } {}

		~ClangString() { clang_disposeString(str); }
		operator const char* () const { return clang_getCString(str); }
		operator std::string()	const { return std::string{ clang_getCString(str) }; }
		std::string ToString()	const { return std::string{ clang_getCString(str) }; }

		CXString str;
	};


	/************************************************************************************************/


	struct Field
	{
		std::string type;
		std::string name;
		std::string annotation;

		uint32_t		size;
		uint32_t		offset;
	};


	/************************************************************************************************/


	struct StructInformation
	{
		bool isTemplate = false;
		bool component = false;

		std::vector<std::string>	bases;
		std::vector<std::string>	functions;
		std::vector<std::string>	staticfunctions;
		std::vector<Field>			fields;
		size_t						size;
	};


	/************************************************************************************************/


	struct ComponentDefinition
	{
		std::vector<Field>	fields;
		std::string			componentName;
	};


	/************************************************************************************************/


	struct TemplateType
	{
		std::string name;
		uint32_t	typeIdx;
	};


	struct IntegerLiteral
	{
		std::string name;
		int value;
	};


	struct FloatLiteral
	{
		std::string name;
		float value;
	};


	struct StringLiteral
	{
		std::string name;
		std::string value;
	};

	using TemplateArgument = std::variant<TemplateType, IntegerLiteral, FloatLiteral, StringLiteral>;

	enum class ObjectKind
	{
		Int,
		UInt,
		Record,
		Bool,
		Pointer,
		Double,
		Enum,
	};

	struct AliasDecl
	{
		bool							isComponent = false;
		bool							isTemplate = false;
		StructInformation				structInfo;
		std::vector<TemplateArgument>	templateArguments;
	};

	struct TypedefDecl
	{
		std::string							typeName;
		ObjectKind							kind;
		uint32_t							typeSize;
		std::unique_ptr<StructInformation>	structDefinition;
	};


	/************************************************************************************************/


	struct ReflectionObjects
	{
		std::vector<TypedefDecl>			types;
		std::vector<ComponentDefinition>	components;
	};


	/************************************************************************************************/


	enum class ParseError
	{
		UNKNOWN,
		InvalidArgument,
		FailedToParseTranslationUnit
	};


	/************************************************************************************************/


	AliasDecl										HandleAliasDeclaration(CXCursor cursor);
	Field											HandleField(CXCursor cursor);
	void											TraverseStruct(CXCursor cursor, StructInformation& structInfo);
	std::expected<ReflectionObjects, ParseError>	TraverseTranslationUnit(CXCursor cursor);
	std::expected<ReflectionObjects, ParseError>	ParseHeaders(std::span<const std::filesystem::path> paths);


}	/************************************************************************************************/


/**********************************************************************

Copyright (c) 2024 Robert May

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
