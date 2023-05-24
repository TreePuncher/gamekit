#include "PCH.h"
#include "EditorInspectors.h"
#include "EditorResourcePickerDialog.h"
#include "EditorPlayer.h"
#include "EditorViewport.h"
#include "ResourceIDs.h"
#include <TriggerSlotStrings.hpp>


/************************************************************************************************/


void StringIDEditorComponent::Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject&, FlexKit::ComponentViewBase& component, bool remoteObject)
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


void TransformEditorComponent::Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject&, FlexKit::ComponentViewBase& component, bool remoteObject)
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
		[&](const std::string& txt)
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
		[&](const std::string& txt)
		{
			char* p;
			const float y = strtof(txt.c_str(), &p);
			if (!*p)
			{
				auto scale = sceneNodeView.GetScale();

				if (scale.y != y)
				{
					scale.y = y;
					sceneNodeView.SetScale(scale);
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
		[&](const std::string& txt)
		{
			char* p;
			const float z = strtof(txt.c_str(), &p);
			if (!*p)
			{
				auto scale = sceneNodeView.GetScale();

				if (scale.z != z)
				{
					scale.z = z;
					sceneNodeView.SetScale(scale);
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
		[&](const std::string& txt)
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


void VisibilityEditorComponent::Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject&, FlexKit::ComponentViewBase& component, bool remoteObject)
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


void PointLightEditorComponent::Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject&, FlexKit::ComponentViewBase& component, bool remoteObject)
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
			"Point Light No Shadows",
			"Spot light No Shadows",
			"Spot light Basic Shadows",
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


struct RemoteComponentUIContext
{
	FlexKit::GameObject&		gameObject;
	SharedEngineMemory*			shared;

	std::shared_ptr<MessageInterface>	lastMessage;
	uint64_t							lastMessageUUID;

	template<typename ... TY_args>
	struct ReturnContext
	{
		RemoteComponentUIContext& context;

		template<typename FN_TY>
		RemoteComponentUIContext& Respond(FN_TY responseFn)
		{
			struct Response : public ResponseInterface
			{
				Response(
					FlexKit::GameObject&	IN_gameObject,
					FN_TY					IN_response,
					uint64_t				IN_messageUUID) :
						gameObject	{ IN_gameObject		},
						response	{ IN_response		},
						messageUUID	{ IN_messageUUID	} {}

				FlexKit::GameObject&	gameObject;
				FN_TY					response;
				uint64_t				messageUUID = -1;

				void Respond(EditorMessageInterface& fetchMessage) override
				{
					auto fetchTyped = static_cast<FlexKit::GetLastArg<TY_args...>&>(fetchMessage);

					response(gameObject, fetchTyped.data);
				}
			};

			auto response =
				std::make_unique<Response>(
						context.gameObject,
						responseFn,
						context.lastMessageUUID);

			context.shared->responders.emplace_back(std::move(response));

			return context;
		}
	};

	template<size_t MessageID>
	RemoteComponentUIContext& Apply(auto fn)
	{
		struct Wrapper : public FlexKit::Serializable<Wrapper, MessageInterface, MessageID>
		{
			Wrapper(FlexKit::GameObject* gameObject = nullptr) :
				gameObjectAddr{ (uint64_t)gameObject } {}

			uint64_t gameObjectAddr;
			uint64_t messageUUID = std::chrono::high_resolution_clock::now().time_since_epoch().count();

			void Do(EditorPlayerState& playerState) override
			{
				decltype(fn) fn2;
				fn2(*reinterpret_cast<FlexKit::GameObject*>(gameObjectAddr));
			}

			void Serialize(auto& archive)
			{
				archive& gameObjectAddr;
			}
		};

		auto temp = std::make_shared<Wrapper>(&gameObject);
		shared->PushMessageToPlayer(temp);

		lastMessage		= temp;
		lastMessageUUID	= temp->messageUUID;

		return *this;
	}

	template<size_t MessageID>
	auto Retrieve(auto fn)
	{
		using FetchResultTY = decltype(fn(std::declval<FlexKit::GameObject&>()));

		struct FetchMessage : public FlexKit::Serializable<FetchMessage, EditorMessageInterface, MessageID>
		{
			FetchResultTY data;
			uint64_t messageUUID;

			void Do(EditorContext& editor) override
			{
				auto res = std::ranges::find_if(
					editor.shared.responders,
					[&](auto& rhs)
					{
						return messageUUID == messageUUID;
					});

				if (res)
				{
					(*res)->Respond(*this);
					editor.shared.responders.remove_unstable(res);
				}
			}

			void Serialize(auto& archive)
			{
				archive& data;
			}
		};

		struct Wrapper : public FlexKit::Serializable<Wrapper, MessageInterface, MessageID + 1>
		{
			Wrapper(FlexKit::GameObject* gameObject = nullptr) :
				gameObjectAddr{ (uint64_t)gameObject } {}

			uint64_t gameObjectAddr;
			uint64_t messageUUID		= std::chrono::high_resolution_clock::now().time_since_epoch().count();

			void Do(EditorPlayerState& playerState) override
			{
				decltype(fn) fn2;
				auto res	= fn2(*reinterpret_cast<FlexKit::GameObject*>(gameObjectAddr));

				auto temp			= std::make_shared<FetchMessage>();
				temp->data			= res;
				temp->messageUUID	= messageUUID;

				playerState.shared->PushMessageToEditor(temp);
			}

			void Serialize(auto& archive)
			{
				archive& gameObjectAddr;
				archive& messageUUID;
			}
		};

		auto temp = std::make_shared<Wrapper>(&gameObject);
		shared->PushMessageToPlayer(temp);

		lastMessage		= temp;
		lastMessageUUID = temp->messageUUID;

		return ReturnContext<FetchResultTY, Wrapper, FetchMessage>{ *this };
	}

	uint64_t Send(auto&& msg) { return shared->PushMessageToPlayer(msg); }
	//uint64_t SendResourceBlob(FlexKit::ResourceBlob& blob)
	//{
	//	struct ResourceBlobMessage : public FlexKit::Serializable<ResourceBlobMessage, MessageInterface, GetCRC32("ResourceBlobMessage")>
	//	{
	//		void Do(EditorPlayerState&) {};
	//	};
	//
	//	return Send(std::make_shared<ResourceBlobMessage>{});
	//}
};


void SceneBrushEditorComponent::Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject& gameObject, FlexKit::ComponentViewBase& component, bool remoteObject)
{
	//std::shared_ptr<RemoteComponentUIContext> remoteContext;
	//
	//if (remoteObject)
	//{
	//	remoteContext = std::make_shared<RemoteComponentUIContext>(gameObject, viewport.GetRenderer().GetSharedMemory());
	//
		/*
		panelCtx.AddButton("Add",
			[inspector = panelCtx.inspector, remoteContext, &project = this->project]()
			{
				auto resourcePicker = new EditorResourcePickerDialog(MeshResourceTypeID, project);

				resourcePicker->OnSelection(
					[inspector, remoteContext](ProjectResource_ptr resource_ptr)
					{
						if (resource_ptr->resource->GetResourceTypeID() == MeshResourceTypeID)
						{
							struct ResourceBlobMessage : public FlexKit::Serializable<ResourceBlobMessage, MessageInterface, GetCRC32("ResourceBlobMessage")>
							{
								ResourceBlobMessage(FlexKit::GUID_t IN_guid = INVALIDHANDLE) : guid{ IN_guid } {}

								FlexKit::GUID_t guid;

								void Do(EditorPlayerState& state) override
								{
									//if (auto mesh = state.GetTriMesh(guid); mesh)
									//	state.gameObject->AddView<FlexKit::BrushView>(mesh.value());
									//else
									//	FK_LOG_ERROR("EditorPlayer: Failed to find asset!");
								}

								void Serialize(auto& archive)
								{
									archive& guid;
								}
							};

							remoteContext->Send(std::make_shared<ResourceBlobMessage>(resource_ptr->resource->GetResourceGUID()));

							//remoteContext->SendResourceBlob(blob);

							//auto trimesh = viewport.LoadTriMeshResource(resource_ptr);

							//if (brush.GetMaterial() == FlexKit::InvalidHandle)
							//{
							//	auto& materials = FlexKit::MaterialComponent::GetComponent();
							//	auto newMaterial = materials.CreateMaterial(viewport.gbufferPass);
							//
							//	brush.SetMaterial(newMaterial);
							//}
							//
							//if (viewport.isVisible())
							//	viewport.GetScene()->scene.AddGameObject(gameObject, FlexKit::GetSceneNode(gameObject));
						}
					});

				resourcePicker->show();
			});

		panelCtx.AddButton("Test",
			[&, inspector = panelCtx.inspector, remoteContext]()
			{
				auto& brush = static_cast<FlexKit::BrushView&>(component);

				remoteContext->Retrieve<GetCRC32("Test1")>(
					[](FlexKit::GameObject& gameObject)
					{
						auto meshes = FlexKit::GetBrush(gameObject)->meshes;

						struct Data
						{
							uint64_t x = 0;
						} datass{ meshes.size() + 1234u };

						return datass;
					}).
					Respond(
					[&](FlexKit::GameObject&, auto datass)
					{
						std::cout << "Test Value " << datass.x << "\n";
						FK_LOG_INFO("test Value = %u", datass.x);
					});
			});

		panelCtx.AddText(fmt::format("Remote Inspection Not Available!"));
		*/
	//}

	auto& brush = static_cast<FlexKit::BrushView&>(component);

	panelCtx.PushVerticalLayout("Brush", true);

	auto meshes = brush.GetMeshes();

	panelCtx.AddText(fmt::format("Mesh List"));
	panelCtx.PushVerticalLayout();

	auto list = panelCtx.AddList(
		[&brush]() { return brush.GetMeshes().size(); },
		[&brush, remoteObject](size_t idx, QListWidgetItem* item)
		{
			if(!remoteObject)
			{
				auto meshes		= brush.GetMeshes();
				auto& mesh		= meshes[idx];
				auto mesh_ptr	= FlexKit::GetMeshResource(mesh);

				item->setText(mesh_ptr->ID);
			}
		},
		[&](QListWidget* item)
		{
		});

	panelCtx.PushVerticalLayout();

	panelCtx.AddButton("Add",
		[&gameObject, &brush, inspector = panelCtx.inspector, remoteObject, &project = this->project, &viewport = this->viewport]()
		{
			auto resourcePicker = new EditorResourcePickerDialog(MeshResourceTypeID, project);

			resourcePicker->OnSelection(
				[&, inspector = inspector](ProjectResource_ptr resource_ptr)
				{
					if (resource_ptr->resource->GetResourceTypeID() == MeshResourceTypeID)
					{
						/*
						if (remoteObject)
						{
							struct ResourceBlobMessage : public FlexKit::Serializable<ResourceBlobMessage, MessageInterface, GetCRC32("ResourceBlobMessage")>
							{
								ResourceBlobMessage(FlexKit::GUID_t IN_guid = INVALIDHANDLE) : guid{ IN_guid } {}

								FlexKit::GUID_t guid;

								void Do(EditorPlayerState& state) override
								{
									if (auto mesh = FlexKit::GetMesh(guid); mesh != INVALIDHANDLE)
									{
										static auto defaultMaterial = 
											[]
											{
												auto& materials = FlexKit::MaterialComponent::GetComponent();
												auto material = materials.CreateMaterial();

												materials.Add2Pass(material, FlexKit::PassHandle{ GetCRCGUID(PBR_CLUSTERED_DEFERRED) });
												materials.Add2Pass(material, FlexKit::PassHandle{ GetCRCGUID(SHADOWMAPPASS) });
												materials.AddRef(material);

												return material;
											}();

										if (!state.gameObject->hasView(FlexKit::MaterialComponentID))
											state.gameObject->AddView<FlexKit::MaterialView>(defaultMaterial);

										if (!state.gameObject->hasView(FlexKit::BrushComponentID))
											state.gameObject->AddView<FlexKit::BrushView>(mesh).SetMaterial(FlexKit::GetMaterialHandle(*state.gameObject));

										auto& brushView = *state.gameObject->GetView<FlexKit::BrushView>();
										auto& meshes	= brushView.GetBrush().meshes;
										meshes.push_back(mesh);
									}
									else
										FK_LOG_ERROR("EditorPlayer: Failed to find asset!");
								}

								void Serialize(auto& archive)
								{
									archive& guid;
								}
							};

							remoteContext->Send(std::make_shared<ResourceBlobMessage>(resource_ptr->resource->GetResourceGUID()));
						}
						else
						*/
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


FlexKit::ComponentViewBase* SceneBrushEditorComponent::Construct(FlexKit::GameObject& gameObject, ComponentConstructionContext& ctx, bool remote)
{
	if (remote)
	{
		auto shared = viewport.GetRenderer().GetSharedMemory();

		struct AddBrushMessage : public FlexKit::Serializable<AddBrushMessage, MessageInterface, GetTypeGUID(AddBrushMessage)>
		{
			void Do(EditorPlayerState& player) override
			{
				if (!player.gameObject->hasView(FlexKit::TransformComponentID))
					player.gameObject->AddView<FlexKit::SceneNodeView>();

				player.gameObject->AddView<FlexKit::BrushView>();
			}

			void Serialize(auto& archive) {}
		};

		auto addBrush = std::make_shared<AddBrushMessage>();

		viewport.GetRenderer().GetSharedMemory()->PushMessageToPlayer(addBrush);

		return nullptr;
	}

	if (!gameObject.hasView(FlexKit::TransformComponentID))
		gameObject.AddView<FlexKit::SceneNodeView>();

	return &gameObject.AddView<FlexKit::BrushView>();
}


/************************************************************************************************/


void TriggerEditorComponent::Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject&, FlexKit::ComponentViewBase& component, bool remoteObject)
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
