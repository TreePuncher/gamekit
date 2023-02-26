#include "PCH.h"
#include "MaterialResource.h"
#include "Materials.h"

/************************************************************************************************/


FlexKit::ResourceBlob MaterialResource::CreateBlob() const
{
	return {};
}


/************************************************************************************************/


const std::string& MaterialResource::GetResourceID() const noexcept
{
	return id;
}


/************************************************************************************************/


uint64_t MaterialResource::GetResourceGUID() const noexcept
{
	return guid;
}


/************************************************************************************************/


ResourceID_t MaterialResource::GetResourceTypeID() const noexcept
{
	return GetTypeGUID(MaterialResource);
}


/************************************************************************************************/


void MaterialResource::SetResourceID(const std::string& IN_id)	noexcept
{
	id = IN_id;
}


/************************************************************************************************/


void MaterialResource::SetResourceGUID(uint64_t IN_guid) noexcept
{
	guid = IN_guid;
}


/************************************************************************************************/


struct MaterialEditorComponent final : public IEditorComponent
{
	void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject&, FlexKit::ComponentViewBase& component)
	{
		auto& material = static_cast<FlexKit::MaterialView&>(component);

		panelCtx.AddHeader("Material");

		auto passes = material.GetPasses();
		auto subMaterials = material.HasSubMaterials();
		panelCtx.AddText("Passes");

		if (passes.size())
		{
			panelCtx.PushVerticalLayout();
			for (auto& pass : passes)
				panelCtx.AddText(std::string{} + std::format("{}", pass.to_uint()));

			panelCtx.Pop();
		}

		if (passes.size())
		{
			panelCtx.PushVerticalLayout();
			for (auto& pass : passes)
				panelCtx.AddText(std::string{} + std::format("{}", pass.to_uint()));

			panelCtx.Pop();
		}
		panelCtx.AddText("Pass Count" + std::format("{}", passes.size()));
		panelCtx.AddButton("Add Pass", [&]() {});
	}


	FlexKit::ComponentViewBase& Construct(FlexKit::GameObject& gameObject, ComponentConstructionContext& ctx)
	{
		auto& materialComponentView = gameObject.AddView<FlexKit::MaterialView>(FlexKit::MaterialComponent::GetComponent().CreateMaterial());
		FlexKit::SetMaterialHandle(gameObject, FlexKit::GetMaterialHandle(gameObject));

		return materialComponentView;
	}


	inline static const std::string name = "Material";
	const std::string&		ComponentName()	const noexcept { return name; }
	FlexKit::ComponentID	ComponentID()	const noexcept { return FlexKit::MaterialComponentID; }

	static bool Register()
	{
		static MaterialEditorComponent materialComponent;
		EditorInspectorView::AddComponent(materialComponent);

		return true;
	}

	inline static bool _registered = Register();
};



/**********************************************************************

Copyright (c) 2015 - 2022 Robert May

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
