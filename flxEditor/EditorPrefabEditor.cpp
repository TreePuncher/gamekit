#include "PCH.h"
#include "AnimationComponents.h"
#include "EditorSelectedPrefabObject.h"
#include "EditorAnimatorComponent.h"
#include "EditorCodeEditor.h"
#include "EditorPrefabEditor.h"
#include "EditorPrefabObject.h"
#include "EditorPrefabInputTab.h"
#include "EditorPrefabRenderer.h"
#include "EditorRenderer.h"
#include "EditorResourcePickerDialog.h"
#include "EditorScriptEngine.h"
#include "EditorPrefabEditor.h"
#include "SelectionContext.h"
#include "ResourceIDs.h"
#include "ViewportScene.h"

#include <angelscript.h>


/************************************************************************************************/


FlexKit::LayerHandle EditorPrefabEditor::GetPhysicsLayer() const
{
	return previewWindow->layer;
}


/************************************************************************************************/


FlexKit::Animation* EditorPrefabEditor::LoadAnimation(std::string& id, bool)
{
	auto resource   = project.FindProjectResource(id)->resource;
	auto blob       = resource->CreateBlob();

	FlexKit::AddAssetBuffer((FlexKit::Resource*)blob.buffer);
	return FlexKit::LoadAnimation(resource->GetResourceGUID(), FlexKit::SystemAllocator);
}


/************************************************************************************************/


void EditorPrefabEditor::ReleaseAnimation(FlexKit::Animation* anim)
{
	FlexKit::SystemAllocator.release(anim);
}


/************************************************************************************************/


EditorPrefabEditor::EditorPrefabEditor(SelectionContext& IN_selection, EditorScriptEngine& IN_engine, EditorRenderer& IN_renderer, EditorProject& IN_project, QWidget* parent)
	: QWidget			{ parent }
	, project			{ IN_project }
	, previewWindow		{ new EditorPrefabPreview{ IN_renderer, localSelection, IN_project } }
	, menubar			{ new QMenuBar{} }
	, localSelection	{ new EditorSelectedPrefabObject{} }
	, codeEditor		{ new EditorCodeEditor{ IN_project, IN_engine, this } }
	, globalSelection	{ IN_selection }
	, renderer			{ IN_renderer }
	, scriptEngine		{ IN_engine }
	, inputVariables	{ new EditorPrefabInputTab }
{
	static auto _REGISTERED = EditorSelectedPrefabObject::RegisterInterface(scriptEngine);

	auto engine		= FlexKit::GetScriptEngine();

	int res;
	res = engine->RegisterGlobalFunction("Animation@ LoadAnimation(string& in)", asMETHOD(EditorPrefabEditor, LoadAnimation), asCALL_THISCALL_ASGLOBAL, this);
	res = engine->RegisterGlobalFunction("void ReleaseAnimation(Animation@)", asMETHOD(EditorPrefabEditor, ReleaseAnimation), asCALL_THISCALL_ASGLOBAL, this);

	codeEditor->GetTabs()->addTab(inputVariables, "Input Variables");

	ui.setupUi(this);
	ui.horizontalLayout->setMenuBar(menubar);
	ui.horizontalLayout->setContentsMargins(1, 1, 1, 1);

	QVBoxLayout* l1 = new QVBoxLayout{};
	l1->addWidget(codeEditor);
	l1->setContentsMargins(1, 1, 1, 1);
	ui.prefabEditorArea->setLayout(l1);

	auto codeEditorMenu = codeEditor->GetMenuBar();
	auto animationMenu  = codeEditorMenu->addMenu("Animator");
	auto reloadObject   = animationMenu->addAction("Reload Object");

	connect(reloadObject, &QAction::triggered,
		[&]()
		{
			codeEditor->SaveDocument();

			if(localSelection)
				localSelection->Reload(scriptEngine);
		});

	auto layout = new QVBoxLayout{};
	ui.viewportArea->setLayout(layout);
	ui.viewportArea->setContentsMargins(1, 1, 1, 1);
	layout->addWidget(previewWindow);

	auto fileMenu				= menubar->addMenu("Prefab");
	auto createAnimatedObject	= fileMenu->addAction("Create Animated Prefab");

	connect(
		createAnimatedObject, &QAction::triggered,
		[&]
		{
			auto meshPicker = new EditorResourcePickerDialog(MeshResourceTypeID, IN_project, this);

			meshPicker->OnSelection(
				[&](ProjectResource_ptr resource)
				{
					auto skeletonPicker = new EditorResourcePickerDialog(SkeletonResourceTypeID, IN_project, this);

					skeletonPicker->OnSelection(
						[&, resource](ProjectResource_ptr skeleton)
						{
							localSelection->gameObject.Release();

							// Load Tri Mesh
							auto meshResource	= std::static_pointer_cast<FlexKit::MeshResource>(resource->resource);
							auto mesh			= renderer.LoadMesh(*meshResource);
							auto& gameObject	= localSelection->gameObject;

							// Load Skeleton
							auto blob			= skeleton->resource->CreateBlob();
							auto buffer			= blob.buffer;
							blob.buffer			= 0;
							blob.bufferSize		= 0;

							FlexKit::AddAssetBuffer((FlexKit::Resource*)buffer);

							// Create Script Object
							auto scriptResource = std::make_shared<ScriptResource>();
							auto context        = scriptEngine.BuildModule(scriptResource->source);

							project.AddResource(scriptResource);
							codeEditor->SetResource(scriptResource);

							// Build Editor object
							gameObject.AddView<FlexKit::SceneNodeView<>>();
							gameObject.AddView<FlexKit::BrushView>(mesh);
							gameObject.AddView<FlexKit::SkeletonView>(skeleton->resource->GetResourceGUID());
							gameObject.AddView<FlexKit::AnimatorView>();

							// serialize object
							PrefabGameObjectResource_ptr objectResource = std::make_shared<PrefabGameObjectResource>();
							project.AddResource(objectResource);

							auto brushComponent		= std::make_shared<FlexKit::EntityBrushComponent>();
							auto skeletonComponent	= std::make_shared<FlexKit::EntitySkeletonComponent>();
							auto animatorComponent	= std::make_shared<AnimatorComponent>();

							brushComponent->meshes.push_back(meshResource->GetResourceGUID());
							skeletonComponent->skeletonResourceID	= skeleton->resource->GetResourceGUID();
							animatorComponent->scriptResource		= scriptResource->GetResourceGUID();

							objectResource->entity.components.push_back(brushComponent);
							objectResource->entity.components.push_back(skeletonComponent);
							objectResource->entity.components.push_back(animatorComponent);

							localSelection->resource	= scriptResource;
							localSelection->resourceID	= objectResource->GetResourceGUID();
							localSelection->ID			= localSelection->resourceID;
							localSelection->animator	= animatorComponent.get();

							globalSelection.Clear();
							globalSelection.type		= AnimatorObject_ID;
							globalSelection.selection	= std::any{ localSelection };

							previewWindow->CenterCamera();
						});

					skeletonPicker->show();
				});

			meshPicker->show();
		});

	auto createPrefabObject = fileMenu->addAction("Create Prefab");
	connect(
		createPrefabObject, &QAction::triggered,
		[&]()
		{
			globalSelection.Clear();
			localSelection->Release();

			PrefabGameObjectResource_ptr objectResource = std::make_shared<PrefabGameObjectResource>();
			auto projectRes = project.AddResource(objectResource);

			localSelection->layer		= GetPhysicsLayer();
			localSelection->resourceID	= objectResource->GetResourceGUID();
			localSelection->prefab		= objectResource;

			localSelection->ID       = objectResource->GetResourceGUID();
			localSelection->animator = nullptr;

			globalSelection.Clear();
			globalSelection.type		= AnimatorObject_ID;
			globalSelection.selection	= std::any{ localSelection };
		});


	auto saveObject = fileMenu->addAction("Save");
	connect(
		saveObject, &QAction::triggered,
		[&]
		{
			if (localSelection && localSelection->prefab)
			{
				auto prefab = (PrefabGameObjectResource*)(project.FindProjectResource(localSelection->ID)->resource.get());

				ViewportSceneContext ctx{};

				std::vector<uint64_t> componentIds;

				for (auto& component : localSelection->gameObject)
				{
					auto component_res = prefab->FindComponent(component.GetID());
					componentIds.push_back(component.GetID());

					if (!component_res)
					{
						auto entityComponent = FlexKit::EntityComponent::CreateComponent(component.GetID());

						if (!entityComponent)
							continue;

						prefab->entity.components.emplace_back(FlexKit::EntityComponent_ptr(entityComponent));

						IEntityComponentRuntimeUpdater::Update(*entityComponent, component.Get_ref(), ctx);
					}
					else
						IEntityComponentRuntimeUpdater::Update(*component_res, component.Get_ref(), ctx);
				}

				auto range = std::ranges::partition(
					prefab->entity.components,
					[&](FlexKit::EntityComponent_ptr& element) -> bool
					{
						return std::ranges::find(componentIds, element->id) != componentIds.end();
					});

				prefab->entity.components.erase(range.begin(), range.end());
			}
		});

	auto loadObject = fileMenu->addAction("Load");
	connect(
		loadObject, &QAction::triggered,
		[&]
		{
			auto resourcePicker = new EditorResourcePickerDialog(FlexKit::EResource_Prefab, IN_project, this);
			resourcePicker->OnSelection(
				[&](ProjectResource_ptr projectObj)
				{
					localSelection->Release();

					auto prefabObjectRes = std::static_pointer_cast<PrefabGameObjectResource>(projectObj->resource);

					struct Context : public LoadEntityContextInterface
					{
						Context(FlexKit::GameObject& IN_obj, EditorRenderer& IN_renderer, EditorProject& IN_project, FlexKit::LayerHandle IN_layer, FlexKit::SceneEntity* IN_entity)
							: gameObject	{ IN_obj		}
							, renderer		{ IN_renderer	}
							, project		{ IN_project	}
							, layer			{ IN_layer		}
							, entity		{ IN_entity		} {}

						FlexKit::GameObject&	gameObject;
						EditorRenderer&			renderer;
						EditorProject&			project;
						FlexKit::LayerHandle	layer;
						FlexKit::SceneEntity*	entity;

						FlexKit::GameObject&	GameObject() override { return gameObject; }
						FlexKit::NodeHandle		GetNode(uint32_t idx) { return FlexKit::GetZeroedNode(); }

						ProjectResource_ptr		FindSceneResource(uint64_t assetID) { return project.FindProjectResource(assetID); }
						FlexKit::TriMeshHandle	LoadTriMeshResource (ProjectResource_ptr resource)
						{
							auto meshResource = std::static_pointer_cast<FlexKit::MeshResource>(resource->resource);
							return renderer.LoadMesh(*meshResource);
						}

						FlexKit::MaterialHandle	DefaultMaterial() const	{ return FlexKit::InvalidHandle; }
						FlexKit::Scene*			Scene()					{ return nullptr; }

						FlexKit::LayerHandle	LayerHandle()	final { return layer; }
						FlexKit::SceneEntity*	Resource()		final { return entity; }
					} context{ localSelection->gameObject, renderer, project, previewWindow->layer, &prefabObjectRes->entity };

					auto loadRes =
						[&](auto& resourceID)
						{
							auto sceneRes	= context.FindSceneResource(resourceID);

							auto blob		= sceneRes->resource->CreateBlob();
							auto buffer		= blob.buffer;
							blob.buffer		= nullptr;
							blob.bufferSize	= 0;
							FlexKit::AddAssetBuffer((Resource*)buffer);

							return sceneRes;
						};


					auto& components = prefabObjectRes->entity;

					if (auto res = components.FindComponent(FlexKit::AnimatorComponentID); res)
					{
						auto animator = std::static_pointer_cast<AnimatorComponent>(res);
						localSelection->animator = animator.get();

						auto resource	= loadRes(animator->scriptResource);
						auto scriptRes	= std::static_pointer_cast<ScriptResource>(resource->resource);
						localSelection->resource	= scriptRes;

						codeEditor->SetResource(scriptRes);
					}

					if (auto res = components.FindComponent(GetTypeGUID(EntitySkeletonComponent)); res)
					{
						auto sk = std::static_pointer_cast<FlexKit::EntitySkeletonComponent>(res);

						loadRes(sk->skeletonResourceID);
					}

					LoadEntity(prefabObjectRes->entity, context);

					localSelection->ID		= prefabObjectRes->GetResourceGUID();
					localSelection->prefab	= prefabObjectRes;

					globalSelection.Clear();
					globalSelection.type		= AnimatorObject_ID;
					globalSelection.selection	= std::any{ localSelection };

					previewWindow->CenterCamera();
				});

			resourcePicker->show();
		});

	auto createAnimator = fileMenu->addAction("Animator");
	createAnimator->setEnabled(false);

	connect(
		createAnimator, &QAction::triggered,
		[&]()
		{
			localSelection->gameObject.AddView<FlexKit::AnimatorView>();
		});

	auto viewMenu				= menubar->addMenu("View");
	auto centerView				= viewMenu->addAction("Center View On Object");
	auto toggleSkeletonOverlay	= viewMenu->addAction("Skeleton");
	auto toggleTurnTable		= viewMenu->addAction("Turntable");
	auto toggleAnimation		= viewMenu->addAction("Animation");
	auto toggleQDTree			= viewMenu->addAction("QT Tree vis");
	auto boundingVolumeVis		= viewMenu->addAction("Brush Bounding Volume");
	auto subMeshVolumesVis		= viewMenu->addAction("SubMesh Bounding Volumes");

	connect(
		centerView, &QAction::triggered,
		[&]()
		{
			previewWindow->CenterCamera();
		});

	toggleSkeletonOverlay->setCheckable(true);
	toggleSkeletonOverlay->setChecked(previewWindow->skeletonOverlay);

	connect(
		toggleSkeletonOverlay, &QAction::triggered,
		[=]
		{
			previewWindow->skeletonOverlay = toggleSkeletonOverlay->isChecked();
			toggleSkeletonOverlay->setChecked(previewWindow->skeletonOverlay);
		});

	toggleTurnTable->setCheckable(true);
	toggleTurnTable->setChecked(previewWindow->turnTable);

	connect(
		toggleTurnTable, &QAction::triggered,
		[=]
		{
			previewWindow->turnTable = toggleTurnTable->isChecked();
			toggleTurnTable->setChecked(previewWindow->turnTable);
		});

	toggleAnimation->setCheckable(true);
	toggleAnimation->setChecked(previewWindow->animate);

	connect(
		toggleAnimation, &QAction::triggered,
		[=]
		{
			previewWindow->animate = toggleAnimation->isChecked();
			toggleAnimation->setChecked(previewWindow->animate);
		});

	toggleQDTree->setCheckable(true);
	toggleQDTree->setChecked(previewWindow->QDTreeOverlay);

	connect(toggleQDTree, &QAction::triggered,
		[=] { previewWindow->QDTreeOverlay = toggleQDTree->isChecked(); });

	boundingVolumeVis->setCheckable(true);
	boundingVolumeVis->setChecked(previewWindow->boundingVolume);

	connect(boundingVolumeVis, &QAction::triggered,
		[=] { previewWindow->boundingVolume = boundingVolumeVis->isChecked(); });

	subMeshVolumesVis->setCheckable(true);
	subMeshVolumesVis->setChecked(previewWindow->SMboundingVolumes);

	connect(subMeshVolumesVis, &QAction::triggered,
		[=] { previewWindow->SMboundingVolumes = subMeshVolumesVis->isChecked(); });

	inputVariables->SetOnCreateEvent(
		[&](size_t typeID, const std::string& ID)
		{
			auto& gameObject = localSelection->gameObject;
			localSelection->AddInputValue(ID, typeID);
		});

	inputVariables->SetOnChangeEvent(
		[&](size_t idx, const std::string& ID, const std::string& value, const std::string& defaultValue)
		{
			auto& inputs        = localSelection->animator->inputs;
			auto& valueEntry    = inputs[idx];
			valueEntry.stringID = ID;

			localSelection->UpdateValue(idx, value);
			localSelection->UpdateDefaultValue(idx, defaultValue);
		});

	timer = new QTimer{ this };
	connect(timer, &QTimer::timeout,
		[&]()
		{
			if (isVisible())
			{
				if (localSelection && localSelection->ID != -1 && localSelection->animator)
				{
					if (auto inputs = localSelection->animator->inputs.size(); inputs)
					{
						inputVariables->Update(
							inputs,
							[&](size_t idx, std::string& ID, std::string& value, std::string& defaultValue)
							{
								auto& inputID	= localSelection->animator->inputs[idx];
								value			= localSelection->ValueString(idx, (uint32_t)inputID.type);
								defaultValue	= localSelection->DefaultValueString(idx);
								ID				= inputID.stringID;
							});
					}
				}
			}
			timer->start(500ms);
		});

	timer->setTimerType(Qt::PreciseTimer);
	timer->start(500ms);
}


/************************************************************************************************/


EditorPrefabEditor::~EditorPrefabEditor()
{
	delete localSelection;
}


/**********************************************************************

Copyright (c) 2021 - 2022 Robert May

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
