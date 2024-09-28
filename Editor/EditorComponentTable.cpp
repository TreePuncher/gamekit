#include "EditorComponentTable.hpp"
#include "EditorProject.h"
#include "EditorInspectorView.h"

#include <EditorReflection.hpp>
#include <Components.hpp>
#include <print>
#include <scn/scan.h>


std::string GenerateTypeID()
{
	std::string ID;
	ID.reserve(15);

	for (int i = 0; i < 15; i++)
	{
		int n = rand() % 26;
		char c = 'a' + n;

		ID.push_back(c);
	}

	return ID;
}

/************************************************************************************************/


ComplexVariableMethods::~ComplexVariableMethods()
{
	FreeLibrary(moduleHNDL);
}


/************************************************************************************************/


constexpr uint32_t ReflectedComponentID = GetTypeGUID(ReflectedComponentID);

class EditorReflectedComponent :
	public FlexKit::Serializable<EditorReflectedComponent, FlexKit::EntityComponent, ReflectedComponentID>
{
public:
	EditorReflectedComponent(uint32_t IN_componentID = 0) :
		Serializable{ IN_componentID } {}

	void Serialize(auto& ar)
	{
		EntityComponent::Serialize(ar);
		uint32_t version = 1;

		ar& componentData;
	}

	FlexKit::Blob GetBlob() override
	{
		return componentData;
	}

	FlexKit::Blob	componentData;
};


/************************************************************************************************/


class ReflectedComponent : public IEditorComponent
{
public:
	ReflectedComponent(BasicComponentReflection_ptr	IN_definition) :
		definition			{ IN_definition },
		name				{ IN_definition->name },
		componentID			{ IN_definition->ID },
		runtimeComponent	{ IN_definition.get() }
	{
		EditorInspectorView::AddComponent(*this);
		IEntityComponentRuntimeUpdater::updaters[componentID] =
			[componentID = this->componentID](FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& runtime, ViewportSceneContext& scene)
			{
				std::print("Reflected component updating serialized data: {}\n", componentID);
				ReflectedComponent::Update(component, runtime, scene);
			};

		FlexKit::EntityComponent::RegisterFactory(
			componentID, 
			[this]()
			{
				return new EditorReflectedComponent{ componentID };
			});
	}

	FlexKit::ComponentID ComponentID()	const noexcept { return componentID; }
	const std::string& ComponentName()	const noexcept { return name; }

	FlexKit::ComponentViewBase* Construct(FlexKit::GameObject& gameObject, ComponentConstructionContext& scene, bool constructRemot)
	{
		return runtimeComponent.AddComponentView(gameObject);
	}

	bool Constructable() const noexcept
	{
		return true;
	}


	static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
	{
		auto& editorComponent	= static_cast<EditorReflectedComponent&>(component); // This gets serialized
		auto& runtimeComponent	= static_cast<RuntimeComponentView&>(base);

		editorComponent.componentData = runtimeComponent.blob;
	}

	void Inspect(ComponentViewPanelContext& layout, FlexKit::GameObject& gameObject, FlexKit::ComponentViewBase& component, bool remoteObject)
	{
		RuntimeComponentView* componentView = static_cast<RuntimeComponentView*>(gameObject.GetView(componentID));

		for (auto& variable : definition->childVariables)
		{
			if (variable.type == "int" || variable.type == "int32_t")
			{
				auto textEdit = layout.AddInputBox(
					variable.name,
					[this, componentView, &variable](std::string& string)
					{
						auto* i = (int*)(componentView->blob.data() + variable.byteOffset);
						string = fmt::format("{}", *i);
					},
					[this, componentView, &variable](const std::string& string)
					{
						auto res = scn::scan<int>(string, "{}");
						if (res)
						{
							auto* i = (int*)(componentView->blob.data() + variable.byteOffset);
							*i = res->value();
						}
					});
				continue;
			}
			else if (variable.type == "uint" || variable.type == "uint32_t")
			{
				auto textEdit = layout.AddInputBox(
					variable.name,
					[this, componentView, &variable](std::string& string)
					{
						auto* i = (uint32_t*)(componentView->blob.data() + variable.byteOffset);
						string = fmt::format("{}", *i);
					},
					[this, componentView, &variable](const std::string& string)
					{
						auto res = scn::scan<uint32_t>(string, "{}");
						if (res)
						{
							auto* i = (uint32_t*)(componentView->blob.data() + variable.byteOffset);
							*i = res->value();
						}
					});
				continue;
			}
			else if (variable.type == "float")
			{
				auto textEdit = layout.AddInputBox(
					variable.name,
					[this, componentView, &variable](std::string& string)
					{
						auto* i = (float*)(componentView->blob.data() + variable.byteOffset);
						string = fmt::format("{}", *i);
					},
					[this, componentView, &variable](const std::string& string)
					{
						auto res = scn::scan<float>(string, "{}");
						if (res)
						{
							auto* i = (float*)(componentView->blob.data() + variable.byteOffset);
							*i = res->value();
						}
					});
				continue;
			}
			else if (variable.type == "double")
			{
				auto textEdit = layout.AddInputBox(
					variable.name,
					[this, componentView, &variable](std::string& string)
					{
						auto* i = (double*)(componentView->blob.data() + variable.byteOffset);
						string = fmt::format("{}", *i);
					},
					[this, componentView, &variable](const std::string& string)
					{
						auto res = scn::scan<double>(string, "{}");
						if (res)
						{
							auto* i = (double*)(componentView->blob.data() + variable.byteOffset);
							*i = res->value();
						}
					});
				continue;
			}
			else if (variable.methods && variable.methods->IsVector())
			{
				layout.AddText(variable.name);
				layout.PushHorizontalLayout();

				auto* methods		= (ComplexVectorMethods*)variable.methods.get();
				auto* vector_ptr	= (componentView->blob.data() + variable.byteOffset);

				const int vectorSize = methods->Size(vector_ptr);

				const char* xyzw[] = { "X", "Y", "Z", "W" };
				for(int i = 0; i < vectorSize; i++)
				{
					auto textEdit = layout.AddInputBox(
						((vectorSize > 4) ? std::format("{}", i) : std::string{ xyzw[i] }),
						[this, componentView, &variable, i, vector_ptr, methods](std::string& string)
						{
							char buffer[512];
							memset(buffer, 0, 512);
							methods->formatElement(i, vector_ptr, buffer);
							string = buffer;
						},
						[this, componentView, &variable, i, vector_ptr, methods](const std::string& string)
						{
							//methods->scanElement(i, vector_ptr, string);
						});
				}
				layout.Pop();
			}
		}
	}


	struct RuntimeComponentView : FlexKit::ComponentViewBase
	{
		RuntimeComponentView(BasicComponentReflection* IN_definition) :
			ComponentViewBase	{ IN_definition->ID },
			definition			{ IN_definition }
		{
			blob.resize(FlexKit::Max(definition->byteSize, blob.size()));

			memset(blob.data(), 0, blob.size());
		}

		FlexKit::Blob blob;

		BasicComponentReflection* definition = nullptr;
		FlexKit::ComponentID	GetComponentID()	{ return ID;}
		FlexKit::ComponentBase&	GetComponent()		{ return FlexKit::ComponentBase::GetComponent(ID); }
	};


	struct RuntimeComponent : FlexKit::ComponentBase
	{
		RuntimeComponent(BasicComponentReflection* IN_definition) :
			componentID	{ IN_definition->ID },
			definition	{ IN_definition }
		{
			AddComponent(*this);
		}

		RuntimeComponentView* AddComponentView(FlexKit::GameObject& GO)
		{
			auto view = new RuntimeComponentView(definition);
			GO.AddView(view);

			elements.push_back(view);

			return view;
		}

		void AddComponentView(FlexKit::GameObject& GO, FlexKit::ValueMap user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
		{
			auto view = new RuntimeComponentView(definition);
			view->blob.resize(FlexKit::Max(bufferSize, definition->byteSize));
			memcpy(view->blob.data(), buffer, bufferSize);

			GO.AddView(view);

			elements.push_back(view);
		}

		void FreeComponentView(void* _ptr)
		{
			auto res = std::find(elements.begin(), elements.end(), (RuntimeComponentView*)_ptr);
			if (res != elements.end())
				elements.erase(res);

			delete static_cast<RuntimeComponentView*>(_ptr);
		}

		std::vector<RuntimeComponentView*>	elements;
		BasicComponentReflection*			definition;

		FlexKit::ComponentID GetID() override { return componentID; }
		FlexKit::ComponentID componentID;
	} runtimeComponent;

	BasicComponentReflection_ptr	definition;
	FlexKit::ComponentID			componentID;
	std::string						name;
	size_t							size = 0;
};


/************************************************************************************************/


EditorComponentTable::EditorComponentTable(class EditorProject& IN_project) :
	project_ptr{ &IN_project } 
{
	onHeaderAdded.Bind(
		[this](auto&& header)
		{
			AddHeader(header);
		});
}


/************************************************************************************************/


void EditorComponentTable::GenerateHeader(const std::string& header)
{
	project_ptr->GetGeneratedPath();
}


/************************************************************************************************/


ComplexVariableMethods_ptr	CreateMethods(const FlexKit::Field& f,  const std::string& sourceHeader, EditorProject* project_ptr)
{
	const auto& typeName	= f.type;
	const auto	typeID		= GenerateTypeID();

	std::string generatedSrc = R"(
#include <@Header>
#include <MathUtilities.hpp>
#include <memory>
#include <string>
#include <scn/scan.h>
#include <iostream>

template<FlexKit::Vector_t t>
constexpr bool IsVector() { return true; }

template<typename TY>
constexpr bool IsVector() { return false; }

extern "C"
{
	__declspec(dllexport) void Create_@TypeID(void* _ptr)
	{
		new(_ptr) @Type{};
	}

	__declspec(dllexport) void Destroy_@TypeID(void* _ptr)
	{
		auto* typed_ptr = reinterpret_cast<@Type*>(_ptr);
		std::destroy_at(typed_ptr);
	}

	__declspec(dllexport) bool IsVector_@TypeID()
	{
		return IsVector<@Type>();
	}

	__declspec(dllexport) int VectorSize_@TypeID(void* _ptr)
	{
		if constexpr (!IsVector<@Type>())
			return;
		else
		{
			auto* typed_ptr = reinterpret_cast<@Type*>(_ptr);
			return typed_ptr->size();
		}
	}

	__declspec(dllexport) void VectorRead_@TypeID(int idx, void* out_ptr, void* c)
	{
		if constexpr (!IsVector<@Type>())
			return;
		else
		{
			auto vector_ptr		= reinterpret_cast<@Type*>(c);
			using ScalerType	= std::remove_reference_t<decltype((*vector_ptr)[0])>;

			*reinterpret_cast<ScalerType*>(out_ptr) = (*vector_ptr)[idx];
		}
	}

	__declspec(dllexport) void  VectorWrite_@TypeID(int idx, void* out_ptr, void* c)
	{
		if constexpr (!IsVector<@Type>())
			return;
		else
		{
			auto vector_ptr		= reinterpret_cast<@Type*>(c);
			using ScalerType	= std::remove_reference_t<decltype((*vector_ptr)[0])>;

			(*vector_ptr)[idx] = *reinterpret_cast<ScalerType*>(out_ptr);
		}
	}

	__declspec(dllexport)  void* VectorGet_@TypeID(int idx, void* c)
	{
		if constexpr (!IsVector<@Type>())
			return nullptr;
		else
		{
			auto vector_ptr		= reinterpret_cast<@Type*>(c);

			if(vector_ptr->size() > idx)
			{
				return &(*vector_ptr)[idx];
			}
			else return nullptr;
		}
	}

	__declspec(dllexport) void  VectorFormat_@TypeID(int idx, void* c, char* c_str)
	{
		if constexpr (!IsVector<@Type>())
			return;
		else
		{
			auto vector_ptr		= reinterpret_cast<@Type*>(c);
		
			auto formatted = std::format("{}", (*vector_ptr)[idx]);
			strcpy_s(c_str, 512, formatted.c_str());
		}
	}

	__declspec(dllexport)  void VectorScan_@TypeID(int idx, void* c, char* c_str)
	{
		if constexpr (!IsVector<@Type>())
			return;
		else
		{
			auto vector_ptr		= reinterpret_cast<@Type*>(c);
			using ScalerType	= std::remove_reference_t<decltype((*vector_ptr)[0])>;
		}
	}
})";

	generatedSrc = SearchAndReplace(generatedSrc, "@Header",	sourceHeader);
	generatedSrc = SearchAndReplace(generatedSrc, "@TypeID",	typeID);
	generatedSrc = SearchAndReplace(generatedSrc, "@Type",		typeName);


	std::string sourceFile = std::format(R"({}\modules\{}.cpp)", project_ptr->projectDirectory.string(), typeID);
	std::string moduleFile = std::format(R"({}\modules\{}.dll)", project_ptr->projectDirectory.string(), typeID);

	sourceFile = SearchAndReplace(sourceFile, "/", "\\");
	moduleFile = SearchAndReplace(moduleFile, "/", "\\");

	auto file = fopen(sourceFile.c_str(), "w");
	size_t written = 0;
	for(;written < generatedSrc.size();)
		written += fwrite(generatedSrc.c_str() + written, 1, generatedSrc.size() - written, file);
	fclose(file);

	std::string buildCommand = std::format(R"(cd modules && cl /DEBUG /std:c++latest /EHsc /arch:AVX2 /LD {} /I "F:\repos\TestProject\includes" /I "F:\repos\TestProject\src" /I F:\repos\flex\core\include /I F:\repos\flex\out\build\x64-debug\Editor\include)", sourceFile);

	if (auto success = project_ptr->RunBuildCommand(buildCommand); success != 0)
	{
		FK_LOG_ERROR("Failed to build module! Type: %s", typeName.c_str());
		std::print("{}\n", generatedSrc);
		std::filesystem::remove(sourceFile);
		return {};
	}

	//std::filesystem::remove(sourceFile);

	auto moduleDir				= project_ptr->projectDirectory.string() + std::format(R"(\modules\{}.dll)", typeID);
	auto moduleHNDL				= LoadLibraryA(moduleDir.c_str());

	if(moduleHNDL)
	{
		std::string CreateFNID		= std::format("Create_{}", typeID);
		std::string DestroyFNID		= std::format("Destroy_{}", typeID);
		std::string IsVectorFNID	= std::format("IsVector_{}", typeID);

		auto create		= (void (*)(void*)) GetProcAddress(moduleHNDL, CreateFNID.c_str());
		auto destroy	= (void (*)(void*))	GetProcAddress(moduleHNDL, DestroyFNID.c_str());
		auto isVector	= (bool (*)())		GetProcAddress(moduleHNDL, IsVectorFNID.c_str());

		if (isVector())
		{
			std::string SizeFNID	= std::format("VectorSize_{}", typeID);
			std::string ReadFNID	= std::format("VectorRead_{}", typeID);
			std::string WriteFNID	= std::format("VectorWrite_{}", typeID);
			std::string GetFNID		= std::format("VectorGet_{}", typeID);
			std::string FormatFNID	= std::format("VectorFormat_{}", typeID);
			std::string ScanFNID	= std::format("VectorScan_{}", typeID);

			auto vectorSize		= (Vector_Size)GetProcAddress(moduleHNDL, SizeFNID.c_str());
			auto vectorRead		= (Vector_ReadIndex)GetProcAddress(moduleHNDL, ReadFNID.c_str());
			auto vectorWrite	= (Vector_WriteIndex)GetProcAddress(moduleHNDL, WriteFNID.c_str());
			auto vectorGet		= (Vector_Get)GetProcAddress(moduleHNDL, GetFNID.c_str());
			auto format			= (Vector_Format)GetProcAddress(moduleHNDL, FormatFNID.c_str());
			auto scan			= (Vector_Scan)GetProcAddress(moduleHNDL, ScanFNID.c_str());

			auto methods = std::make_unique<ComplexVectorMethods>();
			methods->moduleHNDL		= moduleHNDL;
			methods->Create			= create;
			methods->Destroy		= destroy;
			methods->IsVector		= isVector;
			methods->Size			= vectorSize;
			methods->Read			= vectorRead;
			methods->Write			= vectorWrite;
			methods->Get			= vectorGet;
			methods->formatElement	= format;
			methods->scanElement	= scan;

			return methods;
		}
		else
		{
			auto methods = std::make_unique<ComplexVariableMethods>();
			methods->moduleHNDL		= moduleHNDL;
			methods->Create			= create;
			methods->Destroy		= destroy;
			methods->IsVector		= isVector;

			return methods;
		}
	}

	return {};
}


/************************************************************************************************/


void EditorComponentTable::CreateBasicComponent(const FlexKit::ComponentDefinition& component, std::span<const FlexKit::TypedefDecl> types, const std::string& sourceHeader)
{
	using std::ranges::find_if;

	auto res = find_if(basicComponents,
		[&](const auto& c) -> bool
		{
			return c->name == component.componentName;
		});

	if (res != basicComponents.end())
	{
		UpdateBasicComponent(component, types, res->get());
		return;
	}

	const auto& type	= std::get<FlexKit::TypeArgument>(component.subTypes[0]);
	const auto& id		= std::get<FlexKit::IntegerLiteral>(component.subTypes[2]);

	auto newComponent = std::make_shared<BasicComponentReflection>();
	newComponent->name		= component.componentName;
	newComponent->ID		= id.value;
	newComponent->byteSize	= type.structInfo.size;

	for (auto& var : type.structInfo.fields)
	{
		newComponent->childVariables.emplace_back(
				ComponentVariable{
					.name		= var.name,
					.type		= var.type,
					.annotation = var.annotation,
					.byteOffset	= var.offset,
					.byteSize	= var.size,
				});

		if (var.type == "int" || var.type == "int32_t" ||
			var.type == "uint32_t" || var.type == "float" ||
			var.type == "double")
			continue;

		auto methods_ptr = CreateMethods(var, sourceHeader, project_ptr);
		newComponent->childVariables.back().methods = std::move(methods_ptr);
	}

	basicComponents.push_back(newComponent);
	auto editorReflection = new ReflectedComponent{ newComponent };
	editorComponents.push_back(editorReflection);
}


/************************************************************************************************/


void EditorComponentTable::UpdateBasicComponent(const FlexKit::ComponentDefinition&, std::span<const FlexKit::TypedefDecl>, BasicComponentReflection* component)
{

}


/************************************************************************************************/


void EditorComponentTable::AddHeader(const std::string& header)
{
	std::filesystem::path paths[] = { { header } };
	auto results = FlexKit::ParseHeaders(std::span<const std::filesystem::path>{ paths, 1u });

	if (!results.has_value())
		return;

	auto&& [types, components] = results.value();

	for (const auto& component : components)
		std::print("Component Found: {}\n", component.componentName);

	for (const FlexKit::ComponentDefinition& component : components)
	{
		switch (component.type)
		{
		case FlexKit::ComponentType::Basic:
		{
			CreateBasicComponent(component, types, std::filesystem::path{ header }.filename().string());
		}	break;
		case FlexKit::ComponentType::MultiField:
		{

		}	break;
		}
	}
}


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


