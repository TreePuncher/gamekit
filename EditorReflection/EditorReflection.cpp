#include "EditorReflection.hpp"


/************************************************************************************************/

namespace FlexKit
{
	Field HandleField(CXCursor cursor)
	{
		auto		type		= clang_getCursorType(cursor);
		ClangString name		= clang_getCursorSpelling(cursor);
		ClangString typeName	= clang_getTypeSpelling(type);

		Field field{
			.type = typeName,
			.name = name,
		};

		clang_visitChildren(cursor,
			[](CXCursor cursor, CXCursor parent, CXClientData client_data)
			{
				auto* field = (Field*)client_data;
				auto type = clang_getCursorKind(cursor);

				switch (type)
				{
				case CXCursor_AnnotateAttr:
				{
					auto type = clang_getCursorType(cursor);
					ClangString annoation = clang_getCursorSpelling(cursor);
					field->annotation = annoation.ToString();
				}	break;
				default:
					break;
				}

				return CXChildVisit_Continue;
			}, &field);

		return field;
	}


	/************************************************************************************************/


	AliasDecl HandleAliasDeclaration(CXCursor cursor)
	{
		AliasDecl aliasDecl;

		clang_visitChildren(cursor,
			[](CXCursor cursor, CXCursor parent, CXClientData client_data)
			{
				AliasDecl& data = *reinterpret_cast<AliasDecl*>(client_data);
				auto kind = clang_getCursorKind(cursor);

				switch (kind)
				{
				case CXCursor_TemplateRef:
				{
					data.isTemplate = true;

					auto spelling = ClangString(clang_getCursorSpelling(cursor)).ToString();
					if (spelling == "BasicComponent_t")
						data.isComponent = true;
				}	break;
				case CXCursor_TypeRef:
				{
					auto spelling = ClangString(clang_getCursorSpelling(cursor)).ToString();
					auto referenced = clang_getCursorReferenced(cursor);

					struct StructInformation info;
					info.component = true;
					TraverseStruct(referenced, info);
				}	break;
				case CXCursor_IntegerLiteral:
				{
					auto tu = clang_Cursor_getTranslationUnit(cursor);
					auto extent = clang_getCursorExtent(cursor);
					ClangString name = clang_getCursorSpelling(cursor);

					CXToken* tokens = nullptr;
					unsigned int tokenCount;
					clang_tokenize(tu, extent, &tokens, &tokenCount);

					std::string str = ClangString{ clang_getTokenSpelling(tu, tokens[0]) };

					IntegerLiteral integerLiteral;
					integerLiteral.value = std::stoi(str);
					integerLiteral.name = name.ToString();
					data.templateArguments.push_back(integerLiteral);
					clang_disposeTokens(tu, tokens, tokenCount);
				}	break;
				default:
					break;
				};

				return CXChildVisit_Continue;
			}, &aliasDecl);

		return aliasDecl;
	}


	/************************************************************************************************/


	void TraverseStruct(CXCursor cursor, StructInformation& structInfo)
	{
		CXType		structType = clang_getCursorType(cursor);
		ClangString	structName = clang_getTypeSpelling(structType);

		clang_visitChildren(cursor,
			[](CXCursor cursor, CXCursor parent, CXClientData client_data)
			{
				StructInformation& structInfo = *reinterpret_cast<StructInformation*>(client_data);

				auto cursorType = clang_getCursorKind(cursor);

				if (CXCursor_CXXBaseSpecifier != cursorType)
				{
					if (!structInfo.component)
						return CXChildVisit_Break;
				}

				switch (cursorType)
				{
				case CXCursor_CXXBaseSpecifier:
				{
					CXType		structType = clang_getCursorType(cursor);
					ClangString structName = clang_getTypeSpelling(structType);
					std::string	structNameStr = structName;

					if (structNameStr.find("ComponentBase") != std::string::npos)
						structInfo.component = true;

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
						ClangString functionName = clang_getCursorSpelling(cursor);
						int num_args = clang_Cursor_getNumArguments(cursor);

						CXType		functionType = clang_getCursorType(cursor);
						ClangString	functionTypeName = clang_getTypeSpelling(functionType);

						//structInfo.functions.emplace_back(functionName);

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
						ClangString functionName = clang_getCursorSpelling(cursor);
						int num_args = clang_Cursor_getNumArguments(cursor);

						CXType		functionType = clang_getCursorType(cursor);
						ClangString	functionTypeName = clang_getTypeSpelling(functionType);

						clang_visitChildren(cursor,
							[](CXCursor cursor, CXCursor parent, CXClientData client_data)
							{
								auto type = clang_getCursorKind(cursor);

								return CXChildVisit_Continue;
							}, nullptr);

						structInfo.functions.emplace_back(functionName.ToString());
					}
					break;
				case CXCursor_FieldDecl:
				{
					auto field = HandleField(cursor);

					structInfo.fields.emplace_back(std::move(field));
				}	break;
				case CXCursor_VarDecl:
				{
					auto type = clang_getCursorType(cursor);
					ClangString name = clang_getCursorSpelling(cursor);
					ClangString typeName = clang_getTypeSpelling(type);
				}	break;
				break;
				default:
					int x = 0;
					break;
				}
				return CXChildVisit_Continue;
			}, &structInfo);
	}


	/************************************************************************************************/


	ReflectionObjects TraverseTranslationUnit(CXCursor cursor)
	{
		ReflectionObjects objects;

		clang_visitChildren(
			cursor,
			[](CXCursor cursor, CXCursor parent, CXClientData client_data)
			{
				ReflectionObjects& objects = reinterpret_cast<ReflectionObjects&>(client_data);

				auto kind = clang_getCursorKind(cursor);
				switch (kind)
				{
				case CXCursor_TypeAliasDecl:
				{
					ClangString name = clang_getCursorSpelling(cursor);
					auto aliasDecl = HandleAliasDeclaration(cursor);

					if (aliasDecl.isComponent)
					{
						ComponentDefinition component;
						component.fields		= aliasDecl.structInfo.fields;
						component.componentName = name.ToString();
						objects.components.push_back(component);
					}
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
			&objects);

		return objects;
	}


	/************************************************************************************************/


	std::expected<ParsingResults, ParseError> ParseHeader(std::filesystem::path path)
	{
		if (!std::filesystem::exists(path) || std::filesystem::is_directory(path))
			return std::unexpected{ ParseError::InvalidFileInput };

		const char* args[] = {
		"-std = c++23"
		};

		auto strPath = path.string();

		CXIndex index = clang_createIndex(0, 0); //Create index
		CXTranslationUnit unit = clang_parseTranslationUnit(
			index,
			strPath.c_str(),
			args, 1,
			nullptr, 0,
			CXTranslationUnit_None); //Parse "file.cpp"

		if (unit == nullptr) {
			return std::unexpected{ ParseError::FailedToParseTranslationUnit };
		}

		auto cursor = clang_getTranslationUnitCursor(unit);
		TraverseTranslationUnit(cursor);

		clang_disposeTranslationUnit(unit);
		clang_disposeIndex(index);
	}


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

