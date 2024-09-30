#include "EditorReflection.hpp"
#include <map>
#include <memory>
#include <print>

/************************************************************************************************/

namespace FlexKit
{
	StructInformation	HandleAliasDeclaration(CXCursor cursor);
	Field				HandleField(CXCursor cursor);
	TypedefDecl			HandleTypedefDeclaration(CXCursor cursor);
	void				TraverseStruct(CXCursor cursor, StructInformation& structInfo, bool recurse = false);


	/************************************************************************************************/


	using StructInformation_ptr = std::shared_ptr<StructInformation>;
	std::map<std::string, StructInformation_ptr>	structs;


	StructInformation_ptr GetStruct(const std::string& name, CXCursor cursor, ComponentType type = ComponentType::NotAComponent)
	{
		if (auto res = structs.find(name); res != structs.end())
			return res->second;
		else
		{
			StructInformation structDefinition;
			StructInformation_ptr struct_ptr = std::make_shared<StructInformation>(std::move(structDefinition));
			structs[name] = struct_ptr;

			TraverseStruct(cursor, *struct_ptr, true);

			return struct_ptr;
		}
	}


	Field HandleField(CXCursor cursor)
	{
		auto		type		= clang_getCursorType(cursor);
		ClangString name		= clang_getCursorSpelling(cursor);
		ClangString typeName	= clang_getTypeSpelling(type);
		uint32_t	offset		= clang_Cursor_getOffsetOfField(cursor) / 8;
		uint32_t	size		= clang_Type_getSizeOf(type);

		Field field{
			.type	= typeName,
			.name	= name,
			.size	= size,
			.offset	= offset
		};

		clang_visitChildren(cursor,
			[](CXCursor cursor, CXCursor parent, CXClientData client_data)
			{
				auto* field = (Field*)client_data;
				auto type = clang_getCursorKind(cursor);

				switch (type)
				{
				case CXCursor_TypeRef:
				{
					auto name = ClangString{ clang_getCursorSpelling(cursor) }.ToString();

					auto referencedType = clang_getCursorReferenced(cursor);
					auto referencedName = ClangString{ clang_getCursorSpelling(referencedType) }.ToString();

					auto type = clang_getCursorType(cursor);

					int fieldCount = 0;
					clang_Type_visitFields(type,
						[](CXCursor C, CXClientData client_data) -> CXVisitorResult
						{
							int& fieldCount = *reinterpret_cast<int*>(client_data);
							auto type = clang_getCursorKind(C);

							if (type == CXCursor_FieldDecl)
								fieldCount++;

							return CXVisitorResult::CXVisit_Continue;
						}, &fieldCount);

					if (fieldCount)
						field->structInfo = GetStruct(referencedName, referencedType);
				}	break;
				case CXCursor_TemplateRef:
				{
					auto spelling = ClangString(clang_getCursorSpelling(cursor)).ToString();
					if (spelling == "Vector")
						field->fieldType = FieldType::Vector;
					else if (spelling == "vector")
						field->fieldType = FieldType::std_vector;

					int x = 0;
				}	break;
				case CXCursor_AnnotateAttr:
				{
					auto type				= clang_getCursorType(cursor);
					ClangString annoation	= clang_getCursorSpelling(cursor);
					field->annotation		= annoation.ToString();
				}	break;
				default:
					break;
				}

				return CXChildVisit_Continue;
			}, &field);

		return field;
	}


	/************************************************************************************************/


	StructInformation HandleAliasDeclaration(CXCursor cursor)
	{
		StructInformation aliasDecl;

		clang_visitChildren(cursor,
			[](CXCursor cursor, CXCursor parent, CXClientData client_data)
			{
				StructInformation& data = *reinterpret_cast<StructInformation*>(client_data);
				auto kind = clang_getCursorKind(cursor);
				ClangString annoation = clang_getCursorSpelling(cursor);

				switch (kind)
				{
				case CXCursor_TemplateTypeParameter:
				{
					ClangString spelling = clang_getCursorSpelling(cursor);
					std::string spellingString = spelling;

					int x = 0;
				}	break;
				case CXCursor_ClassDecl:
				{
					ClangString spelling = clang_getCursorSpelling(cursor);
					std::string spellingString = spelling;

					StructInformation structDefinition;
					TraverseStruct(cursor, structDefinition);

					int x = 0;
				}	break;
				case CXCursor_TemplateRef:
				{
					data.isTemplate = true;

					auto spelling = ClangString(clang_getCursorSpelling(cursor)).ToString();
					if (spelling == "BasicComponent_t")
						data.componentType = ComponentType::Basic;
					else if (spelling == "MultiFieldComponent_t")
						data.componentType = ComponentType::MultiField;
				}	break;
				case CXCursor_TypeRef:
				{
					auto spelling	= ClangString(clang_getCursorSpelling(cursor)).ToString();
					auto referenced = clang_getCursorReferenced(cursor);
					auto struct_ptr = GetStruct(spelling, referenced, ComponentType::IsAComponent);

					data.templateArguments.emplace_back(TypeArgument{ spelling, struct_ptr });
				}	break;
				case CXCursor_IntegerLiteral:
				{
					auto tu				= clang_Cursor_getTranslationUnit(cursor);
					auto extent			= clang_getCursorExtent(cursor);
					ClangString name	= clang_getCursorSpelling(cursor);

					CXToken* tokens = nullptr;
					unsigned int tokenCount;
					clang_tokenize(tu, extent, &tokens, &tokenCount);

					std::string str = ClangString{ clang_getTokenSpelling(tu, tokens[0]) };

					IntegerLiteral integerLiteral;
					integerLiteral.value	= std::stoi(str);
					integerLiteral.name		= name.ToString();
					data.templateArguments.push_back(integerLiteral);
					clang_disposeTokens(tu, tokens, tokenCount);
				}	break;
				case CXCursor_FloatingLiteral:
				{
					auto tu				= clang_Cursor_getTranslationUnit(cursor);
					auto extent			= clang_getCursorExtent(cursor);
					ClangString name	= clang_getCursorSpelling(cursor);

					CXToken* tokens = nullptr;
					unsigned int tokenCount;
					clang_tokenize(tu, extent, &tokens, &tokenCount);

					std::string str = ClangString{ clang_getTokenSpelling(tu, tokens[0]) };

					FloatLiteral literal;
					literal.value	= std::stof(str);
					literal.name	= name.ToString();
					data.templateArguments.push_back(literal);
					clang_disposeTokens(tu, tokens, tokenCount);
				}	break;
				case CXCursor_StringLiteral:
				{
					auto tu				= clang_Cursor_getTranslationUnit(cursor);
					auto extent			= clang_getCursorExtent(cursor);
					ClangString name	= clang_getCursorSpelling(cursor);

					CXToken* tokens = nullptr;
					unsigned int tokenCount;
					clang_tokenize(tu, extent, &tokens, &tokenCount);

					std::string str = ClangString{ clang_getTokenSpelling(tu, tokens[0]) };

					StringLiteral literal;
					literal.value	= str;
					literal.name	= name.ToString();
					data.templateArguments.push_back(literal);
					clang_disposeTokens(tu, tokens, tokenCount);
				}	break;
				case CXCursor_FirstExpr:
				case CXCursor_CallExpr:
				{
					auto name		= ClangString{ clang_getCursorSpelling(cursor) };
					auto expression = clang_Cursor_Evaluate(cursor);
					auto exprKind	= clang_EvalResult_getKind(expression);

					switch (exprKind)
					{
					case CXEval_Int:
					{
						IntegerLiteral value;
						value.name = name.ToString();
						value.value = clang_EvalResult_getAsInt(expression);
						data.templateArguments.push_back(value);
					}	break;
					case CXEval_Float:
					{
						FloatLiteral value;
						value.name = name.ToString();
						value.value = clang_EvalResult_getAsDouble(expression);
						data.templateArguments.push_back(value);
					}	break;
					case CXEval_StrLiteral:
					{
						StringLiteral value;
						value.name	= name.ToString();
						value.value = clang_EvalResult_getAsStr(expression);
						data.templateArguments.push_back(value);
					}	break;
					default:
						std::print("Warning, clang_EvalResult_getKind returned kind that is unhandled!\n");
						break;
					}
					clang_EvalResult_dispose(expression);
				}
				case CXCursor_VarDecl:
				{
					auto type = clang_getCursorType(cursor);
					ClangString spelled		= clang_getCursorSpelling(cursor);
					ClangString typeName	= clang_getTypeSpelling(type);

					std::string spelledStr	= spelled;
					std::string typeNameStr	= typeName;

					int x = 0;
				}	break;
				case CXCursor_TypeAliasDecl:
				{
					ClangString spelled			= clang_getCursorSpelling(cursor);
					std::string spelledStr		= spelled;

					TypedefDecl res = HandleTypedefDeclaration(cursor);

				}	break;
				default:
					break;
				};

				return CXChildVisit_Continue;
			}, &aliasDecl);

		return aliasDecl;
	}


	/************************************************************************************************/


	TypedefDecl HandleTypedefDeclaration(CXCursor cursor)
	{
		TypedefDecl out;
		auto referenced = clang_getCursorReferenced(cursor);

		auto cursorKind		= clang_getCursorKind(cursor);
		auto cursorSpelling = ClangString(clang_getCursorSpelling(cursor)).ToString();
		auto underlyingType	= clang_getTypedefDeclUnderlyingType(cursor);

		out.typeSize = clang_Type_getSizeOf(underlyingType);
		out.typeName = cursorSpelling;

		switch (underlyingType.kind)
		{
		case CXType_Int:
		{
			out.kind = ObjectKind::Int;
		}	break;
		case CXType_UInt:
		{
			out.kind = ObjectKind::UInt;
		}	break;
		case CXType_Record:
		{
			out.kind = ObjectKind::Record;
		}	break;
		case CXType_Bool:
		{
			out.kind = ObjectKind::Bool;
		}	break;
		case CXType_Pointer:
		{
			out.kind = ObjectKind::Pointer;
		}	break;
		case CXType_Double:
		{
			out.kind = ObjectKind::Double;
		}	break;
		case CXType_Enum:
		{
			out.kind = ObjectKind::Enum;
		}	break;
		default:
			break;
		}

		return out;
	}


	/************************************************************************************************/


	void TraverseStruct(CXCursor cursor, StructInformation& structInfo, bool recurse)
	{
		CXType		structType = clang_getCursorType(cursor);
		ClangString	structName = clang_getTypeSpelling(structType);
		size_t		structSize = clang_Type_getSizeOf(structType);

		if(recurse)
			structInfo.componentType	= ComponentType::IsAComponent;

		structInfo.typeName			= structName.ToString();
		structInfo.size				= structSize;

		clang_visitChildren(cursor,
			[](CXCursor cursor, CXCursor parent, CXClientData client_data)
			{
				StructInformation& structInfo = *reinterpret_cast<StructInformation*>(client_data);

				auto cursorType = clang_getCursorKind(cursor);

				switch (cursorType)
				{
				case CXCursor_CXXBaseSpecifier:
				{
					CXType		structType		= clang_getCursorType(cursor);
					ClangString structName		= clang_getTypeSpelling(structType);
					std::string	structNameStr	= structName;

					if (structNameStr.find("ComponentBase") != std::string::npos)
						structInfo.componentType = ComponentType::Custom;
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


	std::expected<ReflectionObjects, ParseError> TraverseTranslationUnit(CXCursor cursor)
	{
		ReflectionObjects objects;

		clang_visitChildren(
			cursor,
			[](CXCursor cursor, CXCursor parent, CXClientData client_data)
			{
				ReflectionObjects& objects = *reinterpret_cast<ReflectionObjects*>(client_data);

				auto kind = clang_getCursorKind(cursor);
				switch (kind)
				{
				case CXCursor_TypedefDecl:
				{
					ClangString name		= clang_getCursorSpelling(cursor);
					auto		typedefDecl	= HandleTypedefDeclaration(cursor);

					objects.types.emplace_back(std::move(typedefDecl));
				}	break;
				case CXCursor_TypeAliasDecl:
				{
					ClangString name		= clang_getCursorSpelling(cursor);
					auto structDefinition	= HandleAliasDeclaration(cursor);

					if (structDefinition.componentType != ComponentType::NotAComponent)
					{
						ComponentDefinition component;
						component.type			= structDefinition.componentType;
						component.subTypes		= std::move(structDefinition.templateArguments);
						component.componentName = name.ToString();
						objects.components.push_back(component);
					}
				}	break;
				case CXCursor_ClassDecl:
				case CXCursor_StructDecl:
				{
					ClangString name	= clang_getCursorSpelling(cursor);
					auto struct_ptr		= GetStruct(name, cursor);

					objects.types.emplace_back(
						FlexKit::TypedefDecl{
							.typeName			= name,
							.structDefinition	= struct_ptr });
				}	break;

				case CXCursor_FunctionTemplate:
				{
					ClangString name = clang_getCursorSpelling(cursor);
					std::string nameStr = name;
					int x = 0;
				}	break;
				case CXCursor_ClassTemplate:
				{
					ClangString name = clang_getCursorSpelling(cursor);
					std::string nameStr = name;
					int x = 0;
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


	std::expected<ReflectionObjects, ParseError> ParseHeaders(std::span<const std::filesystem::path> paths, std::span<const std::filesystem::path> includePaths)
	{
		if(!paths.size())
			return std::unexpected{ ParseError::InvalidArgument };

		std::vector<const char*> args = {
			"-std=c++23",
			"-DREFLECTOR=1",
			"-D__cplusplus",
			"-D_HAS_CXX23",
			"-D__x86_64__",
			"--include-directory=stl/",
			"--include-directory=core/",
			"--include-directory=include/"
		};

		std::vector<std::string> strings = {};

		for(auto&& include : includePaths)
			strings.emplace_back(std::format("--include-directory={}", include.string()));

		for (const std::string& string : strings)
			args.push_back(string.c_str());

		CXIndex index = clang_createIndex(0, 0); //Create index

		std::vector<CXTranslationUnit> translationUnits;
		ReflectionObjects out;

		for (auto path : paths)
		{
			if (!std::filesystem::exists(path) || std::filesystem::is_directory(path))
				return std::unexpected{ ParseError::InvalidArgument };

			auto strPath = path.string();

			CXTranslationUnit unit = nullptr;
			auto res = clang_parseTranslationUnit2(
				index,
				strPath.c_str(),
				args.data(), args.size(),
				nullptr, 0,
				CXTranslationUnit_None,
				&unit); //Parse "file.cpp"

			if (unit == nullptr) {
				return std::unexpected{ ParseError::FailedToParseTranslationUnit };
			}
			else
			{
				translationUnits.push_back(unit);

				auto numDiagnostics = clang_getNumDiagnostics(unit);
				for (unsigned int i = 0; i < numDiagnostics; i++)
				{
					auto diagnostic = clang_getDiagnostic(unit, i);
					auto message = clang_formatDiagnostic(diagnostic, CXDiagnostic_DisplaySourceLocation | CXDiagnostic_DisplayColumn | CXDiagnostic_DisplayCategoryName);

					std::print("{}\n", clang_getCString(message));
				}


				auto cursor = clang_getTranslationUnitCursor(unit);
				if (auto results = TraverseTranslationUnit(cursor); results.has_value())
				{
					auto& [types, reflectionObjects] = results.value();

					for (auto& type : types)
						out.types.push_back(std::move(type));

					for (auto& object : reflectionObjects)
						out.components.push_back(std::move(object));
				}
			}
		}

		for(auto& unit : translationUnits)
			clang_disposeTranslationUnit(unit);

		clang_disposeIndex(index);

		return out;
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

