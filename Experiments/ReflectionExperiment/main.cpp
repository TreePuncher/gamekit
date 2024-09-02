#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/IRBuilder.h>
// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"
#include <clang-c/Index.h>
#include <iostream>
#include <string>

using llvm::LLVMContext;
using llvm::IRBuilder;
using llvm::Module;
using llvm::FunctionType;
using llvm::Function;
using llvm::BasicBlock;
using llvm::Type;
using llvm::ArrayRef;


using namespace clang::tooling;
using namespace llvm;
using namespace llvm::cl;

inline static const char* TestInput = R"(

struct T {
	int x;
};

)";

inline static cl::OptionCategory MyToolCategory("my-tool options");


struct ClangString
{
	ClangString(CXString IN_str) : str { IN_str}{}

	~ClangString()
	{
		clang_disposeString(str);
	}

	operator const char* () const {	return clang_getCString(str); }
	operator std::string () const { return std::string{ clang_getCString(str) }; }

	std::string ToString() const { return std::string{ clang_getCString(str) }; }


	CXString str;
};

struct FieldInformation
{
	std::string type;
	std::string name;
	std::string annotation;
};

struct StructInformation
{
	bool isTemplate = false;
	bool component	= false;

	std::vector<std::string> bases;
	std::vector<std::string> functions;
	std::vector<std::string> staticfunctions;
	std::vector<FieldInformation> fields;
};

void TraverseStruct(CXCursor cursor, StructInformation& structInfo)
{
	CXType		structType = clang_getCursorType(cursor);
	ClangString	structName = clang_getTypeSpelling(structType);

	std::cout << "struct : " << structName << "\n{\n";
	clang_visitChildren(cursor,
	[](CXCursor cursor, CXCursor parent, CXClientData client_data)
	{
		StructInformation& structInfo = *reinterpret_cast<StructInformation*>(client_data);

		auto cursorType = clang_getCursorKind(cursor);

		//if (CXCursor_CXXBaseSpecifier != cursorType)
		//{
		//	if (!structInfo.component)
		//	{
		//		std::cout << "Skipping, not a component!\n";
		//		return CXChildVisit_Break;
		//	}
		//}

		switch (cursorType)
		{
		case CXCursor_CXXBaseSpecifier:
		{
			CXType		structType		= clang_getCursorType(cursor);
			ClangString structName		= clang_getTypeSpelling(structType);
			std::string	structNameStr	= structName;
			std::cout << "base: " << structName << "\n";

			if (structNameStr.find("ComponentBase") != std::string::npos)
			{
				std::cout << "Component Found!\n";
				structInfo.component = true;

			}

			clang_visitChildren(cursor,
				[](CXCursor cursor, CXCursor parent, CXClientData client_data)
				{
					CXType		structType = clang_getCursorType(cursor);
					ClangString structName = clang_getTypeSpelling(structType);

					return CXChildVisit_Continue;
				},
				nullptr);

		}	break;
		case CXCursor_CXXMethod:
				if (clang_CXXMethod_isStatic(cursor) != 0)
				{
					ClangString functionName		= clang_getCursorSpelling(cursor);
					int num_args					= clang_Cursor_getNumArguments(cursor);

					CXType		functionType		= clang_getCursorType(cursor);
					ClangString	functionTypeName	= clang_getTypeSpelling(functionType);

					//structInfo.functions.emplace_back(functionName);

					std::cout << "functionName: "		<< functionName << "\n";
					std::cout << "functionTypeName: "	<< functionTypeName << "\n";

					/*(
					for (int i = 0; i < num_args; ++i)
					{
						auto arg_cursor = clang_Cursor_getArgument(cursor, i);
						NamedObject arg;
						arg.Name = parser::Convert(
							clang_getCursorSpelling(arg_cursor));
						if (arg.Name.empty())
						{
							arg.Name = "nameless";
						}
						auto arg_type = clang_getArgType(type, i);
						arg.Type = parser::GetName(arg_type);
						f.Arguments.push_back(arg);
					}
					f.ReturnType = parser::GetName(clang_getResultType(type));
					*/
				}
				else
				{
					ClangString functionName		= clang_getCursorSpelling(cursor);
					int num_args					= clang_Cursor_getNumArguments(cursor);

					CXType		functionType		= clang_getCursorType(cursor);
					ClangString	functionTypeName	= clang_getTypeSpelling(functionType);

					//structInfo.functions.emplace_back(functionName);

					std::cout << "functionName: "		<< functionName << "\n";
					std::cout << "functionTypeName: "	<< functionTypeName << "\n";

					clang_visitChildren(cursor,
						[](CXCursor cursor, CXCursor parent, CXClientData client_data)
						{
							auto type = clang_getCursorKind(cursor);

							return CXChildVisit_Continue;
						}, nullptr);
				}
				break;
			case CXCursor_FieldDecl:
			{
				auto type				= clang_getCursorType(cursor);
				ClangString name		= clang_getCursorSpelling(cursor);
				ClangString typeName	= clang_getTypeSpelling(type);

				FieldInformation field{
					.type = typeName,
					.name = name,
				};

				clang_visitChildren(cursor,
					[](CXCursor cursor, CXCursor parent, CXClientData client_data)
					{
						auto* field = (FieldInformation*)client_data;
						auto type	= clang_getCursorKind(cursor);

						switch (type)
						{
						case CXCursor_AnnotateAttr:
						{
							auto type = clang_getCursorType(cursor);
							ClangString annoation = clang_getCursorSpelling(cursor);
							field->annotation = annoation.ToString();
						}	break;
						}

						return CXChildVisit_Continue;
					}, &field);

				if(field.annotation.size())
					std::cout << "[[" << field.annotation << "]]\t";
				std::cout << "field " << name << " : " << typeName << "\n";

				structInfo.fields.emplace_back(std::move(field));
			}	break;
			case CXCursor_VarDecl:
			{
				auto type				= clang_getCursorType(cursor);
				ClangString name		= clang_getCursorSpelling(cursor);
				ClangString typeName	= clang_getTypeSpelling(type);
				std::cout << "field " << name << " : " << typeName << "\n";
			}	break;
			break;
			default:
				int x = 0;
				break;
		}
		return CXChildVisit_Continue;
	}, &structInfo);

	std::cout << "}; //" << structName << "\n";
}

CXTranslationUnit* translationUnit;

int main(int argc, const char* argv[])
{
	const char* args[] = {
		"-std = c++23"
	};

	CXIndex index = clang_createIndex(0, 0); //Create index
	CXTranslationUnit unit = clang_parseTranslationUnit(
		index,
		"TestHeaders/Clang.hpp",
		args, 1,
		nullptr, 0,
		CXTranslationUnit_None); //Parse "file.cpp"

	translationUnit = &unit;

	if (unit == nullptr) {
		std::cerr << "Unable to parse translation unit. Quitting.\n";
		return -1;
	}

	auto cursor = clang_getTranslationUnitCursor(unit);

	clang_visitChildren(
		cursor,
		[](CXCursor cursor, CXCursor parent, CXClientData client_data)
		{
			auto kind = clang_getCursorKind(cursor);
			switch (kind)
			{
			case CXCursor_TypedefDecl:
			{
				CXType type = clang_getCursorType(cursor);
				std::cout << "typedef: \n";
			}
			case CXCursor_TypeAliasDecl:
			{
				CXType type					= clang_getCursorType(cursor);

				struct AliasDecl
				{

				} data;
				
				clang_visitChildren(cursor,
					[](CXCursor cursor, CXCursor parent, CXClientData client_data)
					{
						AliasDecl& data = *reinterpret_cast<AliasDecl*>(client_data);
						auto kind	= clang_getCursorKind(cursor);

						switch (kind)
						{
						case CXCursor_TemplateRef:
						{
							auto spelling = ClangString(clang_getCursorSpelling(cursor)).ToString();
							if (spelling == "BasicComponent_t")
								std::cout << "Component Found!\n";

							int x = 0;
						}	break;
						case CXCursor_TypeRef:
						{
							auto spelling = ClangString(clang_getCursorSpelling(cursor)).ToString();

							auto referenced = clang_getCursorReferenced(cursor);

							std::cout << "Component data: \n";
							struct StructInformation info;
							info.component = true;
							TraverseStruct(referenced, info);
						}	break;
						case CXCursor_IntegerLiteral:
						{
							auto tu = clang_Cursor_getTranslationUnit(cursor);
							auto extent = clang_getCursorExtent(cursor);
							CXToken* tokens = nullptr;
							unsigned int tokenCount;
							clang_tokenize(tu, extent, &tokens, &tokenCount);

							for (int i = 0; i < tokenCount; i++)
							{
								std::string str = ClangString{ clang_getTokenSpelling(*translationUnit, tokens[i]) };
								std::cout << str << '\n';
							}

							clang_disposeTokens(tu, tokens, tokenCount);

						}	break;
						};

						return CXChildVisit_Continue;
					}, &data);
			}	break;
			case CXCursor_ClassDecl:
			case CXCursor_StructDecl:
			{
				StructInformation	structInfo;
				TraverseStruct(cursor, structInfo);
			}	break;
			default:
				int x = 0;
				break;
			}
			return CXChildVisit_Continue;
		},
		nullptr);

	return 0;
}
