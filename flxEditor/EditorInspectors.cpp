#include "PCH.h"
#include "EditorInspectors.h"
#include "EditorResourcePickerDialog.h"
#include "ResourceIDs.h"
#include "EditorViewport.h"
#include <TriggerSlotStrings.hpp>


/************************************************************************************************/


void StringIDEditorComponent::Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject&, FlexKit::ComponentViewBase& component)
{
	auto& stringIDView = static_cast<FlexKit::StringIDView&>(component);

	panelCtx.PushHorizontalLayout("", true);

	panelCtx.AddInputBox(
		"ID",
		[&](std::string& txt)
		{
			txt = fmt::format("{}", stringIDView.GetString());
		},
		[&](const std::string& txt)
		{
			if (txt != stringIDView.GetString())
				stringIDView.SetString(txt.c_str());
		});

	panelCtx.Pop();
}


/************************************************************************************************/


void TransformEditorComponent::Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject&, FlexKit::ComponentViewBase& component)
{
	auto& sceneNodeView = static_cast<FlexKit::SceneNodeView&>(component);

	const auto initialPos		= sceneNodeView.GetPosition();
	const auto initialPosLcl	= sceneNodeView.GetPositionL();
	const auto scale			= sceneNodeView.GetScale();
	const auto orientation		= sceneNodeView.GetOrientation();

	panelCtx.PushHorizontalLayout("Linkage", true);

	panelCtx.AddText(fmt::format("Node: {}", sceneNodeView.node.to_uint()));
	panelCtx.AddText(fmt::format("Parent: {}", sceneNodeView.GetParentNode().to_uint()));
	panelCtx.AddButton(
		"DisconnectNode",
		[&]()
		{
			sceneNodeView.SetParentNode(FlexKit::NodeHandle{ 0 });
		});

	panelCtx.AddButton(
		"Clear",
		[&]()
		{
			sceneNodeView.SetParentNode(FlexKit::NodeHandle{ 0 });
			sceneNodeView.SetOrientation({ 0, 0, 0, 1 });
			sceneNodeView.SetPosition({ 0, 0, 0 });
			sceneNodeView.SetScale({ 1, 1, 1 });
			sceneNodeView.SetWT(FlexKit::float4x4::Identity());
		});
	panelCtx.Pop();

	auto positionTxt = panelCtx.AddText(fmt::format("Global: [{}, {}, {}]", initialPos.x, initialPos.y, initialPos.z));
	panelCtx.PushHorizontalLayout("", true);

	panelCtx.AddInputBox(
		"X",
		[&](std::string& txt)
		{
			auto pos = sceneNodeView.GetPosition();
			txt = fmt::format("{}", pos.x);
		},
		[&, positionTxt = positionTxt](const std::string& txt)
		{
			char* p;
			const float x = strtof(txt.c_str(), &p);
			if (!*p)
			{
				auto position = sceneNodeView.GetPosition();

				if (position.x != x)
				{
					position.x = x;
					sceneNodeView.SetPosition(position);
					positionTxt->setText(fmt::format("Position: [{}, {}, {}]", position.x, position.y, position.z).c_str());
				}
			}
		});

	panelCtx.AddInputBox(
		"Y",
		[&](std::string& txt)
		{
			auto pos = sceneNodeView.GetPosition();
			txt = fmt::format("{}", pos.y);
		},
		[&, positionTxt = positionTxt](const std::string& txt)
		{
			char* p;
			const float y = strtof(txt.c_str(), &p);
			if (!*p)
			{
				auto position = sceneNodeView.GetPosition();

				if (position.y != y)
				{
					position.y = y;
					sceneNodeView.SetPosition(position);
					positionTxt->setText(fmt::format("Position: [{}, {}, {}]", position.x, position.y, position.z).c_str());
				}
			}
		});

	panelCtx.AddInputBox(
		"Z",
		[&](std::string& txt)
		{
			auto pos = sceneNodeView.GetPosition();
			txt = fmt::format("{}", pos.z);
		},
		[&, positionTxt = positionTxt](const std::string& txt)
		{
			char* p;
			const float z = strtof(txt.c_str(), &p);
			if (!*p)
			{
				auto position = sceneNodeView.GetPosition();

				if (position.z != z)
				{
					position.z = z;
					sceneNodeView.SetPosition(position);
					positionTxt->setText(fmt::format("Position: [{}, {}, {}]", position.x, position.y, position.z).c_str());
				}
			}
		});

	panelCtx.Pop();

	auto lclPosTxt = panelCtx.AddText(fmt::format("Local: [{}, {}, {}]", initialPosLcl.x, initialPosLcl.y, initialPosLcl.z));

	panelCtx.PushHorizontalLayout("", true);

	panelCtx.AddInputBox(
		"X",
		[&](std::string& txt)
		{
			auto pos = sceneNodeView.GetPositionL();
			txt = fmt::format("{}", pos.x);
		},
		[&, positionTxt = positionTxt](const std::string& txt)
		{
			char* p;
			const float x = strtof(txt.c_str(), &p);
			if (!*p)
			{
				auto position = sceneNodeView.GetPositionL();

				if (position.x != x)
				{
					position.x = x;
					sceneNodeView.SetPositionL(position);
					positionTxt->setText(fmt::format("Position: [{}, {}, {}]", position.x, position.y, position.z).c_str());
				}
			}
		});

	panelCtx.AddInputBox(
		"Y",
		[&](std::string& txt)
		{
			auto pos = sceneNodeView.GetPositionL();
			txt = fmt::format("{}", pos.y);
		},
		[&, positionTxt = positionTxt](const std::string& txt)
		{
			char* p;
			const float y = strtof(txt.c_str(), &p);
			if (!*p)
			{
				auto position = sceneNodeView.GetPositionL();

				if (position.y != y)
				{
					position.y = y;
					sceneNodeView.SetPositionL(position);
					positionTxt->setText(fmt::format("Position: [{}, {}, {}]", position.x, position.y, position.z).c_str());
				}
			}
		});

	panelCtx.AddInputBox(
		"Z",
		[&](std::string& txt)
		{
			auto pos = sceneNodeView.GetPositionL();
			txt = fmt::format("{}", pos.z);
		},
		[&, positionTxt = positionTxt](const std::string& txt)
		{
			char* p;
			const float z = strtof(txt.c_str(), &p);
			if (!*p)
			{
				auto position = sceneNodeView.GetPositionL();

				if (position.z != z)
				{
					position.z = z;
					sceneNodeView.SetPositionL(position);
					positionTxt->setText(fmt::format("Position: [{}, {}, {}]", position.x, position.y, position.z).c_str());
				}
			}
		});

	panelCtx.Pop();

	panelCtx.PushHorizontalLayout("Scale", true);

	panelCtx.AddInputBox(
		"X",
		[&](std::string& txt)
		{
			auto scale = sceneNodeView.GetScale();
			txt = fmt::format("{}", scale.x);
		},
		[&, positionTxt = positionTxt](const std::string& txt)
		{
			char* p;
			const float x = strtof(txt.c_str(), &p);
			if (!*p)
			{
				auto scale = sceneNodeView.GetScale();

				if (scale.x != x)
				{
					scale.x = x;
					sceneNodeView.SetScale(scale);
					positionTxt->setText(fmt::format("Scale: [{}, {}, {}]", scale.x, scale.y, scale.z).c_str());
				}
			}
		});

	panelCtx.AddInputBox(
		"Y",
		[&](std::string& txt)
		{
			auto scale = sceneNodeView.GetScale();
			txt = fmt::format("{}", scale.y);
		},
		[&, positionTxt = positionTxt](const std::string& txt)
		{
			char* p;
			const float y = strtof(txt.c_str(), &p);
			if (!*p)
			{
				auto scale = sceneNodeView.GetPositionL();

				if (scale.y != y)
				{
					scale.y = y;
					sceneNodeView.SetScale(scale);
					positionTxt->setText(fmt::format("Scale: [{}, {}, {}]", scale.x, scale.y, scale.z).c_str());
				}
			}
		});

	panelCtx.AddInputBox(
		"Z",
		[&](std::string& txt)
		{
			auto scale = sceneNodeView.GetScale();
			txt = fmt::format("{}", scale.z);
		},
		[&, positionTxt = positionTxt](const std::string& txt)
		{
			char* p;
			const float z = strtof(txt.c_str(), &p);
			if (!*p)
			{
				auto scale = sceneNodeView.GetPositionL();

				if (scale.z != z)
				{
					scale.z = z;
					sceneNodeView.SetScale(scale);
					positionTxt->setText(fmt::format("Scale: [{}, {}, {}]", scale.x, scale.y, scale.z).c_str());
				}
			}
		});

	panelCtx.Pop();
	panelCtx.PushHorizontalLayout("Orientation:World", true);

	panelCtx.AddInputBox(
		"X",
		[&](std::string& txt)
		{
			auto q = sceneNodeView.GetOrientation();
			txt = fmt::format("{}", q.x);
		},
		[&, positionTxt = positionTxt](const std::string& txt)
		{
			char* p;
			const float x = strtof(txt.c_str(), &p);
			if (!*p)
			{
				auto q = sceneNodeView.GetOrientation();

				if (q.x != x)
				{
					q.x = x;
					sceneNodeView.SetOrientation(q);
					positionTxt->setText(fmt::format("Scale: [{}, {}, {}, {}]", q.x, q.y, q.z, q.w).c_str());
				}
			}
		});

	panelCtx.AddInputBox(
		"Y",
		[&](std::string& txt)
		{
			auto q = sceneNodeView.GetOrientation();
			txt = fmt::format("{}", q.y);
		},
		[&, positionTxt = positionTxt](const std::string& txt)
		{
			char* p;
			const float y = strtof(txt.c_str(), &p);
			if (!*p)
			{
				auto q = sceneNodeView.GetOrientation();
				if (q.y != y)
				{
					q.y = y;
					sceneNodeView.SetOrientation(q);
					positionTxt->setText(fmt::format("Scale: [{}, {}, {}, {}]", q.x, q.y, q.z, q.w).c_str());
				}
			}
		});

	panelCtx.AddInputBox(
		"Z",
		[&](std::string& txt)
		{
			auto q = sceneNodeView.GetOrientation();
			txt = fmt::format("{}", q.z);
		},
		[&, positionTxt = positionTxt](const std::string& txt)
		{
			char* p;
			const float z = strtof(txt.c_str(), &p);
			if (!*p)
			{
				auto q = sceneNodeView.GetOrientation();

				if (q.z != z)
				{
					q.z = z;
					sceneNodeView.SetOrientation(q);
					positionTxt->setText(fmt::format("Scale: [{}, {}, {}, {}]", q.x, q.y, q.z, q.w).c_str());
				}
			}
		});

	panelCtx.AddInputBox(
		"W",
		[&](std::string& txt)
		{
			auto q = sceneNodeView.GetOrientation();
			txt = fmt::format("{}", q.w);
		},
		[&, positionTxt = positionTxt](const std::string& txt)
		{
			char* p;
			const float w = strtof(txt.c_str(), &p);
			if (!*p)
			{
				auto q = sceneNodeView.GetOrientation();

				if (q.w != w)
				{
					q.w = w;
					sceneNodeView.SetOrientation(q);
					positionTxt->setText(fmt::format("Scale: [{}, {}, {}, {}]", q.x, q.y, q.z, q.w).c_str());
				}
			}
		});

	panelCtx.Pop();

	panelCtx.PushHorizontalLayout("Orientation:Local", true);

	panelCtx.AddInputBox(
		"X",
		[&](std::string& txt)
		{
			auto q = sceneNodeView.GetOrientationL();
			txt = fmt::format("{}", q.x);
		},
		[&, positionTxt = positionTxt](const std::string& txt)
		{
			char* p;
			const float x = strtof(txt.c_str(), &p);
			if (!*p)
			{
				auto q = sceneNodeView.GetOrientationL();

				if (q.x != x)
				{
					q.x = x;
					sceneNodeView.SetOrientationL(q);
				}
			}
		});

	panelCtx.AddInputBox(
		"Y",
		[&](std::string& txt)
		{
			auto q = sceneNodeView.GetOrientationL();
			txt = fmt::format("{}", q.y);
		},
		[&](const std::string& txt)
		{
			char* p;
			const float y = strtof(txt.c_str(), &p);
			if (!*p)
			{
				auto q = sceneNodeView.GetOrientationL();
				if (q.y != y)
				{
					q.y = y;
					sceneNodeView.SetOrientationL(q);
				}
			}
		});

	panelCtx.AddInputBox(
		"Z",
		[&](std::string& txt)
		{
			auto q = sceneNodeView.GetOrientationL();
			txt = fmt::format("{}", q.z);
		},
		[&](const std::string& txt)
		{
			char* p;
			const float z = strtof(txt.c_str(), &p);
			if (!*p)
			{
				auto q = sceneNodeView.GetOrientationL();

				if (q.z != z)
				{
					q.z = z;
					sceneNodeView.SetOrientationL(q);
				}
			}
		});

	panelCtx.AddInputBox(
		"W",
		[&](std::string& txt)
		{
			auto q = sceneNodeView.GetOrientationL();
			txt = fmt::format("{}", q.w);
		},
		[&](const std::string& txt)
		{
			char* p;
			const float w = strtof(txt.c_str(), &p);
			if (!*p)
			{
				auto q = sceneNodeView.GetOrientationL();

				if (q.w != w)
				{
					q.w = w;
					sceneNodeView.SetOrientationL(q);
				}
			}
		});

	panelCtx.Pop();

}


/************************************************************************************************/


void VisibilityEditorComponent::Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject&, FlexKit::ComponentViewBase& component)
{
	auto& visibility = static_cast<FlexKit::SceneVisibilityView&>(component);

	panelCtx.AddHeader("Visibility");
	panelCtx.AddText("Bounding Volume type: Bounding Sphere");

	panelCtx.AddInputBox(
		"Bounding Sphere Radius",
		[&](std::string& str)
		{
			str = fmt::format("{}", visibility.GetBoundingSphere().w);
		},
		[&](const std::string& txt)
		{
			char* p;
			float r = strtof(txt.c_str(), &p);
			if (!*p)
			{
				auto BS = visibility.GetBoundingSphere();
				BS.w = r;
				visibility.SetBoundingSphere(BS);
			}
		});
}


/************************************************************************************************/


void PointLightEditorComponent::Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject&, FlexKit::ComponentViewBase& component)
{
	auto& pointLight = static_cast<FlexKit::LightView&>(component);

	panelCtx.AddHeader("Point Light");

	panelCtx.AddText(fmt::format("Node: {}", pointLight.GetNode().to_uint()));

	panelCtx.AddInputBox(
		"Radius",
		[&](std::string& txt)
		{
			txt = fmt::format("{}", pointLight.GetRadius());
		},
		[&](const std::string& txt)
		{
			char* p;
			float r = strtof(txt.c_str(), &p);
			if (!*p)
			{
				auto currentRadius = pointLight.GetRadius();

				if (currentRadius != r)
					pointLight.SetRadius(r);
			}
		});

	panelCtx.AddInputBox(
		"Intensity",
		[&](std::string& string) {
			string = fmt::format("{}", pointLight.GetIntensity());
		},
		[&](const std::string& txt)
		{
			char* p;
			float i = strtof(txt.c_str(), &p);
			if (!*p)
			{
				auto currentIntensity = pointLight.GetIntensity();

				if (currentIntensity != i)
					pointLight.SetIntensity(i);
			}
		});

	{
		panelCtx.PushHorizontalLayout();

		auto inputBox = panelCtx.AddInputBox(
			"Outer Range",
			[&](std::string& string) {
				string = fmt::format("{}", pointLight.GetOuterAngle());
			},
			[&](const std::string& txt)
			{
				char* p;
				float i = strtof(txt.c_str(), &p);
				if (!*p)
				{
					auto currentIntensity = pointLight.GetOuterAngle();

					if (currentIntensity != i)
						pointLight.SetOuterAngle(i);
				}
			});

		panelCtx.AddSliderHorizontal(
			"", 0.0f, FlexKit::pi / 2,
			[&pointLight]() { return  pointLight.GetOuterAngle(); },
			[inputBox, &pointLight](float outerRange)
			{
				pointLight.SetOuterAngle(outerRange);
				inputBox->update();
			});


		panelCtx.Pop();
	}

	{
		panelCtx.PushHorizontalLayout();

		auto inputBox = panelCtx.AddInputBox(
			"Inner Range",
			[&](std::string& string) {
				string = fmt::format("{}", pointLight.GetInnerAngle());
			},
			[&](const std::string& txt)
			{
				char* p;
				float i = strtof(txt.c_str(), &p);
				if (!*p)
				{
					auto currentIntensity = pointLight.GetInnerAngle();

					if (currentIntensity != i)
						pointLight.SetInnerAngle(i);
				}
			});

		panelCtx.AddSliderHorizontal(
			"", 0.0f, FlexKit::pi / 2,
			[&pointLight]() { return  pointLight.GetInnerAngle(); },
			[inputBox, &pointLight](float innerangle)
			{
				pointLight.SetInnerAngle(innerangle);
				inputBox->update();
			});


		panelCtx.Pop();
	}

	{
		panelCtx.PushHorizontalLayout();

		auto inputBox = panelCtx.AddInputBox(
			"Light Size",
			[&](std::string& string) {
				string = fmt::format("{}", pointLight.GetSize());
			},
			[&](const std::string& txt)
			{
				char* p;
				float i = strtof(txt.c_str(), &p);
				if (!*p)
				{
					auto currentSizd = pointLight.GetSize();

					if (currentSizd != i)
						pointLight.SetSize(i);
				}
			});

		panelCtx.AddSliderHorizontal(
			"", 0.0f, 1.0f,
			[&pointLight]() { return  pointLight.GetSize(); },
			[inputBox, &pointLight](float innerangle)
			{
				pointLight.SetSize(innerangle);
				inputBox->update();
			});


		panelCtx.Pop();
	}

	{
		static const char* typeNames[] = {
			"Point Light",
			"Spot Light",
			"Directional Light",
			"Spot Light No Shadows",
			"Spot light No Shadows",
		};

		panelCtx.AddComboBox(
			typeNames,
			[&pointLight]() -> uint32_t
			{
				return (uint32_t)pointLight.GetType();
			},
			[&pointLight](uint32_t item)
			{
				pointLight.SetType(FlexKit::LightType(item));
			});
	}
}


/************************************************************************************************/


SceneBrushEditorComponent::SceneBrushEditorComponent(EditorProject& IN_project, EditorViewport& IN_viewport)
	: project	{ IN_project }
	, viewport	{ IN_viewport } {}


void SceneBrushEditorComponent::Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject& gameObject, FlexKit::ComponentViewBase& component)
{
	auto& brush = static_cast<FlexKit::BrushView&>(component);

	panelCtx.PushVerticalLayout("Brush", true);

	auto meshes = brush.GetMeshes();

	panelCtx.AddText(fmt::format("Mesh List"));
	panelCtx.PushVerticalLayout();

	auto list = panelCtx.AddList(
		[&brush]() { return brush.GetMeshes().size(); },
		[&brush](size_t idx, QListWidgetItem* item)
		{
			auto meshes		= brush.GetMeshes();
			auto& mesh		= meshes[idx];
			auto mesh_ptr	= FlexKit::GetMeshResource(mesh);

			item->setText(mesh_ptr->ID);
		},
		[&](QListWidget* item)
		{
		});

	panelCtx.PushVerticalLayout();

	panelCtx.AddButton("Add",
		[&, inspector = panelCtx.inspector]()
		{
			auto resourcePicker = new EditorResourcePickerDialog(MeshResourceTypeID, project);

			resourcePicker->OnSelection(
				[&, inspector = inspector](ProjectResource_ptr resource_ptr)
				{
					if (resource_ptr->resource->GetResourceTypeID() == MeshResourceTypeID)
					{
						auto trimesh = viewport.LoadTriMeshResource(resource_ptr);
						brush.PushMesh(trimesh);

						if (brush.GetMaterial() == FlexKit::InvalidHandle)
						{
							auto& materials = FlexKit::MaterialComponent::GetComponent();
							auto newMaterial = materials.CreateMaterial(viewport.gbufferPass);

							brush.SetMaterial(newMaterial);
						}

						if (viewport.isVisible())
							viewport.GetScene()->scene.AddGameObject(gameObject, FlexKit::GetSceneNode(gameObject));
					}
				});

			resourcePicker->show();
		});

	panelCtx.AddButton("Remove",
		[&brush, inspector = panelCtx.inspector, list]()
		{
			auto item	= list->currentRow();
			auto meshes = brush.GetMeshes();

			if(meshes.size() > item)
				brush.RemoveMesh(meshes[item]);
		});

	panelCtx.Pop();
	panelCtx.Pop();
	panelCtx.Pop();
}


/************************************************************************************************/


void TriggerEditorComponent::Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject&, FlexKit::ComponentViewBase& component)
{
	auto& triggers = static_cast<FlexKit::TriggerView&>(component);

	panelCtx.PushVerticalLayout("Available Triggers", true);

	auto list = panelCtx.AddList(
		[&triggers]
		{
			return triggers->triggers.size();
		},
		[&triggers](size_t idx, QListWidgetItem* item)
		{
			const uint32_t id = triggers->triggerIDs[idx];
			auto res = TriggerIDStringMappings.find(id);
			if (res != TriggerIDStringMappings.end())
				item->setText(res->second.data());
			else
				item->setText(fmt::format("Unknown Trigger  ID: {}", id).c_str());
		},
		[&](QListWidget* item)
		{
		});

	panelCtx.AddButton("Connect",
		[&triggers, list]
		{
			auto data		= triggers->userData;
			auto editorData	= std::any_cast<TriggerEditorComponent::EditorTriggerData>(&triggers->userData);
		});

	panelCtx.Pop();

	panelCtx.PushVerticalLayout("Slots", true);

	auto textBox = panelCtx.AddList(
		[&triggers]
		{
			return triggers->actionSlots.size();
		},
		[list, &triggers](size_t idx, QListWidgetItem* item)
		{
			const uint32_t id = triggers->actionSlotIDs[idx];
			auto res = SlotIDStringMappings.find(id);
			if (res != SlotIDStringMappings.end())
				item->setText(res->second.data());
			else
				item->setText(fmt::format("Unknown Slot ID: {}", id).c_str());
		},
		[&](QListWidget* item)
		{
		});

	panelCtx.AddButton("Inspect",
		[&triggers, list]
		{
		});


	/*
	textBox->connect(
		list,
		&QListWidget::itemChanged,
		textBox,
		[=] { textBox->update(); });
	*/

	panelCtx.Pop();
}


/************************************************************************************************/


/**********************************************************************

Copyright (c) 2021 - 2023 Robert May

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
