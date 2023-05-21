#include <Application.h>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <SharedEngineMemory.hpp>

#include <scn/scn.h>
#include <string_view>

#include "Win32Graphics.h"
#include "Serialization.hpp"
#include "EditorPlayer.h"
#include "EditorProject.h"
#include "DepthBuffer.h"
#include "WorldRender.h"

using namespace boost::interprocess;
using namespace FlexKit;

/************************************************************************************************/

void SendResource(FlexKit::iResource& resource, SharedEngineMemory& shared)
{
	SendResource(resource, shared, shared.idGenerator());
}

void SendResource(FlexKit::iResource& resource, SharedEngineMemory& shared, uint64_t uuid)
{
	struct ResourceMessage : public FlexKit::Serializable<ResourceMessage, MessageInterface, GetTypeGUID(ResourceMessage)>
	{
		void Do(EditorPlayerState& player) override
		{
			player.AddResource(std::move(blob));
		}

		void Serialize(auto& archive)
		{
			archive& blob;
		}

		FlexKit::Blob blob;
	};

	auto resourceBlob	= resource.CreateBlob();
	auto resourceSend	= std::make_shared<ResourceMessage>();
	resourceSend->blob	= Blob{ resourceBlob.buffer, resourceBlob.bufferSize };

	shared.PushMessageToPlayer(resourceSend, uuid);
}


/************************************************************************************************/


EditorPlayerState::EditorPlayerState(GameFramework & in_framework, SharedEngineMemory* IN_shared) :
	FrameworkState		{ in_framework	},
	shared				{ IN_shared		},
	scene				{ IN_shared->blockAllocator },
	renderWindow		{ IN_shared->blockAllocator.allocate<Win32RenderWindow>(std::move(FlexKit::CreateWin32RenderWindowFromHWND(framework.GetRenderSystem(), shared->targetWindow).first)) },
	constantBuffer		{ in_framework.GetRenderSystem().CreateConstantBuffer(16 * MEGABYTE, false) },
	vertexBuffer		{ in_framework.GetRenderSystem().CreateVertexBuffer(16 * MEGABYTE, false) },
	textureStreaming	{ in_framework.GetRenderSystem(), in_framework.core.GetBlockMemory() },
	renderer			{ in_framework.GetRenderSystem(), textureStreaming, in_framework.core.GetBlockMemory() },
	depthBuffer			{ in_framework.GetRenderSystem(), { 400, 400 } },
	gbuffer				{ { 400, 400 }, in_framework.GetRenderSystem() },
	activeCamera		{ CameraComponent::GetComponent().CreateCamera() },
	materials			{ in_framework.GetRenderSystem(), textureStreaming, in_framework.core.GetBlockMemory() }
{
	FlexKit::EventNotifier<>::Subscriber sub;
	sub.Notify	= &FlexKit::EventsWrapper;
	sub._ptr	= &framework;

	renderWindow.Handler->Subscribe(sub);

	SetCameraNode(activeCamera, GetZeroedNode());

	SetLoadFailureHandler({ *this, &EditorPlayerState::LoadAsset });
}


/************************************************************************************************/


UpdateTask* EditorPlayerState::Update(EngineCore&, UpdateDispatcher&, double dT)
{
	while (shared->playerQueue.size())
	{
		auto message = shared->playerQueue.pop_back();

		auto& temp = message.value();
		if (!temp.buffer.size())
			continue;

		FlexKit::Blob blob{ (const char*)temp.buffer.data(), temp.buffer.size() };
		FlexKit::LoadBlobArchiveContext loader{ blob };

		std::shared_ptr<MessageInterface> freshMessage;
		loader& freshMessage;

		freshMessage->Do(*this);
	}

	return nullptr;
}


/************************************************************************************************/


UpdateTask* EditorPlayerState::Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{
	//if (!drawRequested)
	//	return nullptr;

	FlexKit::Pitch(FlexKit::GetCameraNode(activeCamera), dT * 3.14159f);

	ClearBackBuffer(frameGraph, renderWindow.GetBackBuffer(), float4{ 0.0f, 0.0f, 1.0f, 1.0f });
	ClearDepthBuffer(frameGraph, depthBuffer.Get(), 1.0f);

	frameGraph.AddOutput(renderWindow.GetBackBuffer());

	if (gameObject && !scene.sceneEntities.size())
	{
		scene.AddGameObject(*gameObject);
		FlexKit::SetBoundingSphereFromMesh(*gameObject);
	}
	else if(!gameObject)
		return nullptr;

	CenterObject();

	auto& transformUpdate	= FlexKit::QueueTransformUpdateTask(dispatcher);
	auto& cameraUpdate		= FlexKit::CameraComponent::GetComponent().QueueCameraUpdate(dispatcher);

	cameraUpdate.AddInput(transformUpdate);

	DrawSceneDescription drawSceneDesc =
	{
		.camera					= activeCamera,
		.scene					= scene,
		.dt						= dT,
		.t						= 0.0,
		.gbuffer				= gbuffer,
		.reserveVB				= FlexKit::CreateVertexBufferReserveObject(vertexBuffer, core.RenderSystem, core.GetTempMemory()),
		.reserveCB				= FlexKit::CreateConstantBufferReserveObject(constantBuffer, core.RenderSystem, core.GetTempMemory()),
		.transformDependency	= transformUpdate,
		.cameraDependency		= cameraUpdate
	};

	renderer.DrawScene(dispatcher, frameGraph, drawSceneDesc, { renderWindow.GetBackBuffer(), depthBuffer }, core.GetBlockMemory(), core.GetTempMemoryMT());

	frameGraph.SubmitDirect(dispatcher, core.RenderSystem, core.GetBlockMemory());

	drawRequested = false;

	return nullptr;
}


/************************************************************************************************/


void EditorPlayerState::PostDrawUpdate(EngineCore& core, double dT)
{
	depthBuffer.Increment();
	renderWindow.Present(1, 0);
}


/************************************************************************************************/


bool EditorPlayerState::EventHandler(Event evt)
{
	return false;
}


/************************************************************************************************/


void EditorPlayerState::CenterObject()
{
	if (!gameObject)
		return;

	auto meshes			= FlexKit::GetTriMesh(*gameObject);

	if (meshes.empty())
		return;

	auto aabb				= FlexKit::GetAABBFromMesh(*gameObject);
	const FlexKit::Camera c = FlexKit::CameraComponent::GetComponent().GetCamera(activeCamera);

	const auto target			= aabb.MidPoint();
	const auto desiredDistance	= 2.5f * aabb.Span().magnitude() / std::tan(c.FOV);

	auto position_VS		= c.View.Transpose() * float4 { target, 1 };
	auto updatedPosition_WS	= c.IV.Transpose() * float4 { position_VS.x, position_VS.y, position_VS.z + desiredDistance, 1 };

	const auto node		= FlexKit::GetCameraNode(activeCamera);
	const Quaternion Q	= GetOrientation(node);
	auto forward		= (Q * float3{ 0.0f, 0.0f, -1.0f}).normal();

	FlexKit::SetPositionW(node, updatedPosition_WS.xyz());
	FlexKit::MarkCameraDirty(activeCamera);
}


/************************************************************************************************/


void EditorPlayerState::Reset()
{
	if (gameObject)
		gameObject->Release();
	else
		gameObject = &shared->blockAllocator.allocate<GameObject>();

	scene.ClearScene();

	shared->currentGameObject = gameObject;
	gameObject->AddView<FlexKit::SceneNodeView>();
	scene.AddGameObject(*gameObject);
}


/************************************************************************************************/


void EditorPlayerState::SendBlob(FlexKit::Blob& blob)
{
	FlexKit::Vector<std::byte> outputBlob{ shared->blockAllocator };

	outputBlob.resize(blob.buffer.size());
	memcpy(outputBlob.data(), blob.data(), outputBlob.size());

	shared->editorQueue.push_front({ .buffer = std::move(outputBlob) });
}


/************************************************************************************************/


void EditorPlayerState::SendErrorMessage(const std::string& message)
{
	struct ErrorMessage : public FlexKit::Serializable<ErrorMessage, EditorMessageInterface, GetTypeGUID(ResizeMessage)>
	{
		void Do(EditorContext&) override
		{
			FK_LOG_ERROR(message.c_str());
		}

		void Serialize(auto& archive)
		{
			archive& message;
		}

		std::string message;
	};

	auto errorMessage = std::make_shared<ErrorMessage>();
	errorMessage->message = message;
}


/************************************************************************************************/


void EditorPlayerState::AddResource(FlexKit::Blob&& blob)
{
	auto resource = reinterpret_cast<FlexKit::Resource*>(blob.data());
	FlexKit::AddAssetBuffer(resource);

	resourceBlobs.emplace_back(std::move(blob));
}


/************************************************************************************************/


void EditorPlayerState::AddGameObject(FlexKit::GameObject& gameObject)
{

}


/************************************************************************************************/


void EditorPlayerState::Shutdown()
{
	framework.quit = true;
}


/************************************************************************************************/


uint64_t EditorPlayerState::RequestAsset(FlexKit::GUID_t guid)
{
	struct RequestMessage : public FlexKit::Serializable<RequestMessage, EditorMessageInterface, GetCRC32("RequestAsset::GUID")>
	{
		RequestMessage(FlexKit::GUID_t IN_guid = -1, uint64_t IN_uuid = -1) : guid{ IN_guid }, uuid{ IN_uuid } {}

		FlexKit::GUID_t		guid;
		uint64_t			uuid;

		void Do(EditorContext& editor) override
		{
			auto asset	= editor.project.FindProjectResource(guid);
			SendResource(*asset->resource, editor.shared, uuid);
		}

		void Serialize(auto& archive)
		{
			archive& guid;
			archive& uuid;
		}
	};

	auto uuid = shared->idGenerator();
	return shared->PushMessageToEditor(std::make_shared<RequestMessage>(guid, uuid), uuid);
}


/************************************************************************************************/


uint64_t EditorPlayerState::RequestAsset(std::string_view ID)
{
	struct RequestMessage : public FlexKit::Serializable<RequestMessage, EditorMessageInterface, GetCRC32("RequestAsset::STRING")>
	{
		RequestMessage(std::string IN_ID = "", uint64_t IN_uuid = -1) : resourceID{ IN_ID }, uuid{ IN_uuid } {}

		std::string	resourceID;
		uint64_t	uuid;

		void Do(EditorContext& editor) override
		{
			auto asset = editor.project.FindProjectResource(resourceID);

			SendResource(*asset->resource.get(), editor.shared, uuid);
		}

		void Serialize(auto& archive)
		{
			archive& resourceID;
			archive& uuid;
		}
	};

	auto uuid = shared->idGenerator();
	return shared->PushMessageToEditor(std::make_shared<RequestMessage>(std::string{ ID }, uuid));
}


/************************************************************************************************/


bool EditorPlayerState::WaitForMessage(uint64_t UUID, uint32_t ms)
{
	for(auto& item : shared->playerQueue)
	{
		if (item.UUID == UUID)
			return true;
	}

	return false;
}


/************************************************************************************************/


FlexKit::AssetHandle EditorPlayerState::LoadAsset(FlexKit::AssetIdentifier identifier)
{
	const auto request = std::visit(
		[&](auto identifier)
		{
			return RequestAsset(identifier);
		}, identifier);

	while (!WaitForMessage(request));

	auto message = shared->GetMessageFromEditor(request);
	if (message)
	{	// Handle message
		UnwrapMessage(message.value())->Do(*this);
	}

	return std::visit([](auto a)
		{
			return FlexKit::LoadGameAsset(a);
		}, identifier);
}


/************************************************************************************************/


FlexKit::TriMeshHandle EditorPlayerState::LoadMesh(FlexKit::GUID_t guid)
{
	auto res = FlexKit::FindMesh(guid);
	if (!res)
		return res.value();
	else
		return InvalidHandle;
}


/************************************************************************************************/


int PlayerMain(int argc, char* argv[])
{
	if (argc < 3)
		return -1;

	try
	{
		shared_memory_object shm_obj(
			open_or_create,
			"shared_memory",
			read_write);

		size_t offset = 0;
		for (int I = 0; I + 1 < argc; I++)
		{
			std::string_view arg{ argv[I] };
			if (arg == "--player")
			{
				auto res = scn::scan(std::string_view{ argv[I + 1] }, "{}", offset);
				if (res)
					break;
			}
		}

		if (offset == 0)
		{
			FK_LOG_ERROR("Invalid Arguments!");
			return -1;
		}

		SharedEngineMemory* shared = GetSharedMemory(shm_obj, offset);

		auto* allocator = FlexKit::CreateEngineMemory();
		EXITSCOPE(ReleaseEngineMemory(allocator));

		const CoreOptions options{
			.GPUdebugMode = false
		};

		auto app = std::make_unique<FlexKit::FKApplication>(allocator, options);
		app->PushState<EditorPlayerState>(shared);


		struct ReadyMessage : public FlexKit::Serializable<ReadyMessage, EditorMessageInterface, GetCRC32("Ready")>
		{
			void Do(EditorContext& editor) override { }

			void Serialize(auto& archive) {}
		};

		shared->PushMessageToEditor(std::make_shared<ReadyMessage>());

		app->GetCore().FPSLimit		= 90;
		app->GetCore().FrameLock	= false;
		app->GetCore().vSync		= true;
		app->Run();
	}
	catch (...)
	{
		return -1;
	}
	return 0;
}


/**********************************************************************

Copyright (c) 2015 - 2023 Robert May

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
