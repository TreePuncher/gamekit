#pragma once
#include <Signals.hpp>
#include <vector>

using Type_Create		= void (*)(void*);
using Type_Destroy		= void (*)(void*);
using Type_IsVector		= bool (*)();

using Vector_Size		= int	(*)(void*);
using Vector_ReadIndex	= void	(*)(int, void* out_ptr, void* c);
using Vector_WriteIndex = void	(*)(int, void* out_ptr, void* c);
using Vector_Get		= void*	(*)(int, void* c);

using Vector_Format = void (*)(int idx, void* c, char*);
using Vector_Scan	= void (*)(int idx, void* c, char*);

struct ComplexVariableMethods
{
	ComplexVariableMethods() = default;
	~ComplexVariableMethods();

	HMODULE			moduleHNDL;
	Type_Create		Create;
	Type_Destroy	Destroy;
	Type_IsVector	IsVector;
};

struct ComplexVectorMethods : ComplexVariableMethods
{
	Vector_Size			Size;
	Vector_ReadIndex	Read;
	Vector_WriteIndex	Write;
	Vector_Get			Get;

	Vector_Format	formatElement;
	Vector_Scan		scanElement;
};

using ComplexVariableMethods_ptr = std::unique_ptr<ComplexVariableMethods>;

struct ComponentVariable
{
	std::string	name;
	std::string	type;
	std::string	annotation;

	uint32_t	byteOffset;
	uint32_t	byteSize;

	ComplexVariableMethods_ptr methods;
};

struct ComponentField
{
	std::string						name;
	std::string						type;
	std::string						annotation;
	std::vector<ComponentVariable>	childVariables;
};

struct BasicComponentReflection
{
	size_t							byteSize;
	FlexKit::ComponentID			ID;
	std::string						name;
	std::vector<ComponentVariable>	childVariables;
};

using BasicComponentReflection_ptr = std::shared_ptr<BasicComponentReflection>;

struct MultiFieldComponentReflection
{
	std::string					name;
	std::vector<ComponentField> fields;
};

namespace FlexKit
{
	class ComponentDefinition;
	class TypedefDecl;
}

class EditorComponentTable
{
public:
	EditorComponentTable(class EditorProject& IN_project);

	void GenerateHeader	(const std::string& header);
	void AddHeader		(const std::string& header);

	void CreateBasicComponent(const FlexKit::ComponentDefinition&, std::span<const FlexKit::TypedefDecl>, const std::string& sourceheader);
	void UpdateBasicComponent(const FlexKit::ComponentDefinition&, std::span<const FlexKit::TypedefDecl>, BasicComponentReflection* component);
	
	FlexKit::Signal<void(const std::string&)>::Slot onHeaderAdded;

	std::vector<BasicComponentReflection_ptr>	basicComponents;
	std::vector<MultiFieldComponentReflection>	complexComponents;
	std::vector<class IEditorComponent*>		editorComponents;

	class EditorProject* project_ptr;
};


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
