#include "EditorComponentTable.hpp"
#include "EditorProject.h"
#include "EditorInspectorView.h"
#include <EditorReflection.hpp>
#include <Components.hpp>

class ReflectedComponent : public IEditorComponent
{
public:
	ReflectedComponent(BasicComponentReflection_ptr	IN_definition) :
		definition			{ IN_definition },
		name				{ IN_definition->name },
		componentID			{ (FlexKit::ComponentID)rand() },
		runtimeComponent	{ componentID }
	{
		EditorInspectorView::AddComponent(*this);
	}

	FlexKit::ComponentID ComponentID()	const noexcept { return componentID; }
	const std::string& ComponentName()	const noexcept { return name; }

	FlexKit::ComponentViewBase* Construct(FlexKit::GameObject& gameObject, ComponentConstructionContext& scene, bool constructRemot)
	{
		return nullptr;
	}

	bool Constructable() const noexcept
	{
		return true;
	}

	void Inspect(ComponentViewPanelContext& layout, FlexKit::GameObject&, FlexKit::ComponentViewBase& component, bool remoteObject)
	{
		for (auto& variable : definition->childVariables)
			CreateVariableUIField(variable);
	}

	void CreateVariableUIField(ComponentVariable& variable)
	{

	}

	struct RuntimeComponentView : FlexKit::ComponentViewBase
	{
		RuntimeComponentView(FlexKit::ComponentID IN_componentID) : ComponentViewBase(IN_componentID) {}

		FlexKit::ComponentID	GetComponentID()	{ return ID;}
		FlexKit::ComponentBase&	GetComponent()		{ return FlexKit::ComponentBase::GetComponent(ID); }
	};

	BasicComponentReflection_ptr	definition;
	FlexKit::ComponentID			componentID;
	std::string						name;

	struct RuntimeComponent : FlexKit::ComponentBase
	{
		RuntimeComponent(FlexKit::ComponentID IN_componentID) :
			componentID{ IN_componentID }
		{
			AddComponent(*this);
		}

		void AddComponentView(FlexKit::GameObject& GO, FlexKit::ValueMap user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
		{

		}

		void FreeComponentView(void* _ptr)
		{

		}

		FlexKit::ComponentID GetID() override { return componentID; }


		FlexKit::ComponentID componentID;
	} runtimeComponent;
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


void EditorComponentTable::AddHeader(const std::string& header)
{
	using std::ranges::find_if;

	std::filesystem::path paths[] = { { header } };
	auto results = FlexKit::ParseHeaders(std::span<const std::filesystem::path>{ paths, 1u });

	if (!results.has_value())
		return;

	auto&& [types, components] = results.value();

	for (const FlexKit::ComponentDefinition& component : components)
	{
		switch (component.type)
		{
		case FlexKit::ComponentType::Basic:
		{
			auto res = find_if(basicComponents,
				[&](const auto& c) -> bool
				{
					return c->name == component.componentName;
				});

			if (res == basicComponents.end())
			{
				auto newComponent = std::make_shared<BasicComponentReflection>();
				newComponent->name = component.componentName;
				auto type = component.subTypes.back();

				for (auto subType : component.subTypes)
				{
					std::visit(
						FlexKit::overloaded{
							[&](const FlexKit::TypeArgument& parameter)
							{
								for (auto& var : parameter.structInfo.fields)
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
							},
							[&](const FlexKit::TemplateType& parameter)
							{
								auto& type = types[parameter.typeIdx];

								int x = 0;
							},
							[&](const FlexKit::IntegerLiteral& parameter)
							{
								int x = 0;
							},
							[&](const auto& _)
							{
								int x = 0;
							}
						},
						subType);
				}

				basicComponents.push_back(newComponent);
				auto editorReflection = new ReflectedComponent{ newComponent };
				editorComponents.push_back(editorReflection);
			}
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


