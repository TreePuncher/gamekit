#pragma once

#include "EditorInspectorView.h"

#include <Transforms.h>
#include <TriggerComponent.h>
#include <fmt/format.h>
#include <QtWidgets\qlabel.h>
#include <format>
#include <CSGComponent.h>
#include <ViewportScene.h>


/************************************************************************************************/


struct StringIDEditorComponent final : public IEditorComponent
{
	FlexKit::ComponentID	ComponentID()	const noexcept	{ return FlexKit::StringComponentID; }
	const std::string&		ComponentName()	const noexcept	{ return name; }


	void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject&, FlexKit::ComponentViewBase& component) override;


	FlexKit::ComponentViewBase& Construct(FlexKit::GameObject& gameObject, ComponentConstructionContext& ctx)
	{
		return gameObject.AddView<FlexKit::StringIDView>(nullptr, 0);
	}


	inline static const std::string name = "StringID";

	static bool Register()
	{
		static StringIDEditorComponent component;
		EditorInspectorView::AddComponent(component);

		return true;
	}

	inline static bool _registered = Register();
};


/************************************************************************************************/


struct TransformEditorComponent final : public IEditorComponent
{
	FlexKit::ComponentID ComponentID() const noexcept { return FlexKit::TransformComponentID; }
	const std::string& ComponentName() const noexcept { return name; }

	void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject&, FlexKit::ComponentViewBase& component) override;


	static FlexKit::ComponentViewBase& ConstructNode(FlexKit::GameObject& gameObject, ComponentConstructionContext& ctx)
	{
		return (FlexKit::ComponentViewBase&)gameObject.AddView<FlexKit::SceneNodeView>(FlexKit::GetZeroedNode());
	}

	FlexKit::ComponentViewBase& Construct(FlexKit::GameObject& gameObject, ComponentConstructionContext& ctx)
	{
		return ConstructNode(gameObject, ctx);
	}

	inline static const std::string name = "Transform";

	static bool Register()
	{
		static TransformEditorComponent component;
		EditorInspectorView::AddComponent(component);
		return true;
	}

	inline static bool _registered = Register();
};



/************************************************************************************************/


class VisibilityEditorComponent final : public IEditorComponent
{
	inline static const std::string name = "Visibility:RuntimeComponent";

	FlexKit::ComponentID ComponentID()  const noexcept
	{
		return FlexKit::SceneVisibilityComponentID;
	}

	const std::string& ComponentName() const noexcept
	{
		return name;
	}

	static bool Register()
	{
		static VisibilityEditorComponent component{};
		EditorInspectorView::AddComponent(component);
		return true;
	}

	void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject&, FlexKit::ComponentViewBase& component) override;

	FlexKit::ComponentViewBase& Construct(FlexKit::GameObject&, ComponentConstructionContext& scene) { std::unreachable(); }
	bool Constructable() const noexcept { return false; }

	inline static bool registered = Register();
};


/************************************************************************************************/


class PointLightEditorComponent final : public IEditorComponent
{
public:
	FlexKit::ComponentID	ComponentID()	const noexcept { return FlexKit::LightComponentID; }
	const std::string&		ComponentName() const noexcept { return name; }


	void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject&, FlexKit::ComponentViewBase& component) override;


	static FlexKit::ComponentViewBase& ConstructPointLight(FlexKit::GameObject& gameObject, ComponentConstructionContext& ctx) noexcept
	{
		if (!gameObject.hasView(FlexKit::TransformComponentID))
			gameObject.AddView<FlexKit::SceneNodeView>();

		gameObject.AddView<FlexKit::LightView>();

		if (!gameObject.hasView(FlexKit::SceneVisibilityComponentID))
			ctx.AddToScene(gameObject);

		return *gameObject.GetView(FlexKit::LightComponentID);
	}

	FlexKit::ComponentViewBase& Construct(FlexKit::GameObject& gameObject, ComponentConstructionContext& ctx)
	{
		return ConstructPointLight(gameObject, ctx);
	}

	inline static const std::string name = "PointLight";

	static bool Register()
	{
		static PointLightEditorComponent component{};
		EditorInspectorView::AddComponent(component);
		return true;
	}

	inline static bool _registered = Register();
};


/************************************************************************************************/


class PointLightShadowMapEditorComponent final : public IEditorComponent
{
public:

	FlexKit::ComponentID	ComponentID()	const noexcept { return FlexKit::PointLightShadowMapID; }
	const std::string&		ComponentName()	const noexcept { return "Point Light Shadow"; }

	void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject&, FlexKit::ComponentViewBase& component) override
	{
		auto& cubeShadowMapView = static_cast<FlexKit::ShadowMapView&>(component);

		panelCtx.AddHeader("Point Light Shadow Map");
		panelCtx.AddText("No Fields!");
	}

	bool Constructable() const noexcept { return false; }

	inline static const std::string name = "Point Light Shadow";
};


/*
struct CubicShadowMapFactory : public IComponentFactory
{
	FlexKit::ComponentViewBase& Construct(FlexKit::GameObject& gameObject, ComponentConstructionContext& ctx)
	{
		if(gameObject.GetView(FlexKit::LightComponent::GetComponentID()))
			gameObject.AddView<FlexKit::PointLightShadowMapView>(
				FlexKit::_PointLightShadowCaster{ FlexKit::GetPointLight(gameObject),
				FlexKit::GetSceneNode(gameObject) } );

		return *gameObject.GetView(FlexKit::LightComponent::GetComponentID());
	}

	inline static const std::string name = "Cubic Shadow Map";

	std::string				ComponentName() const noexcept { return name; }
	FlexKit::ComponentID	ComponentID() const noexcept { return FlexKit::PointLightShadowMapID; }

	static bool Register()
	{
		EditorInspectorView::AddComponentFactory(std::make_unique<CubicShadowMapFactory>());
		return true;
	}

	inline static bool _registered = Register();
};
*/

/************************************************************************************************/


class SceneBrushEditorComponent final : public IEditorComponent
{
public:
	SceneBrushEditorComponent(EditorProject&, EditorViewport&);

	inline static const std::string name = "Brush";
	FlexKit::ComponentID	ComponentID()	const noexcept { return FlexKit::BrushComponentID; }
	const std::string&		ComponentName()	const noexcept { return name; }

	void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject&, FlexKit::ComponentViewBase& component) override;

	FlexKit::ComponentViewBase& Construct(FlexKit::GameObject& gameObject, ComponentConstructionContext& ctx)
	{
		if (!gameObject.hasView(FlexKit::TransformComponentID))
			gameObject.AddView<FlexKit::SceneNodeView>();

		return gameObject.AddView<FlexKit::BrushView>();
	}


	static void Register(EditorProject& project, EditorViewport& viewport)
	{
		static SceneBrushEditorComponent component{ project, viewport };
		EditorInspectorView::AddComponent(component);
	}

	EditorViewport& viewport;
	EditorProject&  project;
};


/************************************************************************************************/


struct TriggerEditorComponent final : public IEditorComponent
{
	struct EditorTriggerData
	{
		std::vector<std::string> ids;
	};


	FlexKit::ComponentViewBase& Construct(FlexKit::GameObject& gameObject, ComponentConstructionContext& ctx)
	{
		auto& triggerView		= gameObject.AddView<FlexKit::TriggerView>();
		triggerView->userData	= EditorTriggerData{};

		return triggerView;
	}

	static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
	{
		auto&	triggerView	= static_cast<FlexKit::TriggerView&>(base);
		auto&	triggerData	= triggerView.GetData();

		if(!triggerData.userData.has_value())
			triggerData.userData = EditorTriggerData{};

		auto&	editorData	= std::any_cast<EditorTriggerData&>(triggerData.userData);
	}

	inline static const std::string name = "Trigger";

	FlexKit::ComponentID	ComponentID()		const noexcept { return FlexKit::TriggerComponentID; }
	const std::string&		ComponentName()		const noexcept { return name; }

	void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject&, FlexKit::ComponentViewBase& component) override;

	static bool Register()
	{
		static TriggerEditorComponent component;
		EditorInspectorView::AddComponent(component);
		return true;
	}

	inline static bool _registered = Register();
	IEntityComponentRuntimeUpdater::RegisterConstructorHelper<TriggerEditorComponent, FlexKit::TriggerComponentID> _register;
};


/************************************************************************************************/


/**********************************************************************

Copyright (c) 2019-2023 Robert May

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

