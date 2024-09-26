#include "EditorComponentTable.hpp"
#include "EditorProject.h"
#include "EditorInspectorView.h"

#include <EditorReflection.hpp>
#include <Components.hpp>
#include <print>
#include <scn/scan.h>


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
					[this, componentView, variable](std::string& string)
					{
						auto* i = (int*)(componentView->blob.data() + variable.offset);
						string = fmt::format("{}", *i);
					},
					[this, componentView, &variable](const std::string& string)
					{
						auto res = scn::scan<int>(string, "{}");
						if (res)
						{
							auto* i = (int*)(componentView->blob.data() + variable.offset);
							*i = res->value();
						}
					});
				continue;
			}
			else if (variable.type == "uint" || variable.type == "uint32_t")
			{
				auto textEdit = layout.AddInputBox(
					variable.name,
					[this, componentView, variable](std::string& string)
					{
						auto* i = (uint32_t*)(componentView->blob.data() + variable.offset);
						string = fmt::format("{}", *i);
					},
					[this, componentView, &variable](const std::string& string)
					{
						auto res = scn::scan<uint32_t>(string, "{}");
						if (res)
						{
							auto* i = (uint32_t*)(componentView->blob.data() + variable.offset);
							*i = res->value();
						}
					});
				continue;
			}
			else if (variable.type == "float")
			{
				auto textEdit = layout.AddInputBox(
					variable.name,
					[this, componentView, variable](std::string& string)
					{
						auto* i = (float*)(componentView->blob.data() + variable.offset);
						string = fmt::format("{}", *i);
					},
					[this, componentView, &variable](const std::string& string)
					{
						auto res = scn::scan<float>(string, "{}");
						if (res)
						{
							auto* i = (float*)(componentView->blob.data() + variable.offset);
							*i = res->value();
						}
					});
				continue;
			}
			else if (variable.type == "double")
			{
				auto textEdit = layout.AddInputBox(
					variable.name,
					[this, componentView, variable](std::string& string)
					{
						auto* i = (double*)(componentView->blob.data() + variable.offset);
						string = fmt::format("{}", *i);
					},
					[this, componentView, &variable](const std::string& string)
					{
						auto res = scn::scan<double>(string, "{}");
						if (res)
						{
							auto* i = (double*)(componentView->blob.data() + variable.offset);
							*i = res->value();
						}
					});
				continue;
			}
			else
			{
				int x = 0;
				// Create,
				// Copy,
				// Create Blobs
			}
		}
	}


	struct RuntimeComponentView : FlexKit::ComponentViewBase
	{
		RuntimeComponentView(BasicComponentReflection* IN_definition) :
			ComponentViewBase	{ IN_definition->ID },
			definition			{ IN_definition }
		{
			blob.resize(FlexKit::Max(definition->size, blob.size()));

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
			view->blob.resize(FlexKit::Max(bufferSize, definition->size));
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
	newComponent->name	= component.componentName;
	newComponent->ID	= id.value;
	newComponent->size	= type.structInfo.size;

	for (auto& var : type.structInfo.fields)
	{
		newComponent->childVariables.emplace_back(
			ComponentVariable{
				.name		= var.name,
				.type		= var.type,
				.annotation = var.annotation,
				.offset		= var.offset,
				.size		= var.size,
			});
	}

	basicComponents.push_back(newComponent);
	auto editorReflection = new ReflectedComponent{ newComponent };
	editorComponents.push_back(editorReflection);

	for (auto& var : type.structInfo.fields)
	{
		if (var.type == "int"		|| var.type == "int32_t" ||
			var.type == "int"		|| var.type == "float" ||
			var.type == "double")
			continue;

		auto typeName = var.type;

#if 0
		std::string generatedSrc =
			R"(
#include <@Header>

template<FlexKit::Vector_t t>
constexpr bool IsVector() { return true; }

template<typename TY>
constexpr bool IsVector() { return false; }

extern "C"
{
	void __declspec(dllexport) Create_@TYPEID(void* _ptr)
	{
		new(_ptr) @Type{};
	}

	void __declspec(dllexport) Destroy_@TYPEID(void* _ptr)
	{
		auto* typed_ptr = reinterpret_cast<@Type*>(_ptr);
		typed_ptr->~@Type();
	}

	bool __declspec(dllexport) IsVector_@TYPEID()
	{
		return IsVector<@Type>();
	}

	int __declspec(dllexport) VectorSize_@TypeID(void* _ptr)
	{
		if constexpr (!IsVector<@Type>())
			return 0;
		else
		{
			auto* typed_ptr = reinterpret_cast<@Type*>(_ptr);
			return typed_ptr->size();
		}
	}
})";
		generatedSrc = SearchAndReplace(generatedSrc, "@Header",	sourceHeader);
		generatedSrc = SearchAndReplace(generatedSrc, "@Type",		typeName);
		generatedSrc = SearchAndReplace(generatedSrc, "@TypeID",	typeID);

		"cl /std:c++latest /EHsc /arch:AVX2 /LD test.cpp /I "F:\repos\TestProject\includes" /I "F:\repos\TestProject\src" /I F:\repos\flex\core\include /I F:\repos\flex\out\build\x64-debug\Editor\include       ";
		project_ptr->RunBuildCommand("cl.exe");
#endif

		auto moduleDir				= project_ptr->projectDirectory.string() + "\\modules\\test.dll";
		auto moduleHndl				= LoadLibraryA(moduleDir.c_str());
		void (*Create)(void*)		= (void (*)(void*)) GetProcAddress(moduleHndl, "Create_float4");
		void (*Destroy)(void*)		= (void (*)(void*))	GetProcAddress(moduleHndl, "Destroy_float4");
		bool (*IsVector)()			= (bool (*)())		GetProcAddress(moduleHndl, "IsVector_float4");
		int  (*VectorSize)(void*)	= nullptr;

		void (*VectorReadIndex)(int, void*)		= nullptr;
		void (*VectorWriteIndex)(int, void*)	= nullptr;

		if(IsVector())
			VectorSize = (int (*)(void*))GetProcAddress(moduleHndl, "VectorSize_float4");

		std::vector<char> buffer{ 16 };

		Create(buffer.data());

		int x = VectorSize(buffer.data());

		Destroy(buffer.data());

		int y = 0;
	}


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


