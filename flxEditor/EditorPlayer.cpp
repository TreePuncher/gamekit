#include <Application.h>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <SharedEngineMemory.hpp>

#include <scn/scn.h>
#include <string_view>

#include "Win32Graphics.h"
#include "Serialization.hpp"
#include "EditorPlayer.h"

using namespace boost::interprocess;
using namespace FlexKit;


/************************************************************************************************/


EditorPlayerState::EditorPlayerState(GameFramework & in_framework, SharedEngineMemory* IN_shared) :
	FrameworkState		{ in_framework	},
	shared				{ IN_shared		},
	scene				{ IN_shared->blockAllocator },
	renderWindow		{ IN_shared->blockAllocator.allocate<Win32RenderWindow>(std::move(FlexKit::CreateWin32RenderWindowFromHWND(framework.GetRenderSystem(), shared->targetWindow).first)) },
	constantBuffer		{ in_framework.GetRenderSystem().CreateConstantBuffer(16 * MEGABYTE, false) },
	vertexBuffer		{ in_framework.GetRenderSystem().CreateVertexBuffer(16 * MEGABYTE, false) },
	textureStreaming	{ in_framework.GetRenderSystem(), in_framework.core.GetBlockMemory() },
	renderer			{ in_framework.GetRenderSystem(), textureStreaming, in_framework.core.GetBlockMemory() }
{
	FlexKit::EventNotifier<>::Subscriber sub;
	sub.Notify	= &FlexKit::EventsWrapper;
	sub._ptr	= &framework;

	renderWindow.Handler->Subscribe(sub);
}


/************************************************************************************************/


UpdateTask* EditorPlayerState::Update(EngineCore&, UpdateDispatcher&, double dT)
{
	while (shared->inputQueue.size())
	{
		auto message = shared->inputQueue.pop_back();
		auto& temp = message.value();
		FlexKit::Blob blob{ (const char*)temp.data(), temp.size() };
		FlexKit::LoadBlobArchiveContext loader{ blob };

		std::shared_ptr<MessageInterface> freshMessage;
		loader& freshMessage;

		freshMessage->Do(*this);
	}
	return nullptr;
}


/************************************************************************************************/


UpdateTask* EditorPlayerState::Draw(UpdateTask* update, EngineCore&, UpdateDispatcher&, double dT, FrameGraph& frameGraph)
{
	frameGraph.AddOutput(renderWindow.GetBackBuffer());

	ClearBackBuffer(frameGraph, renderWindow.GetBackBuffer(), float4{ 1.0f, 0.0f, 1.0f, 1.0f });

	return nullptr;
}


/************************************************************************************************/


void EditorPlayerState::PostDrawUpdate(EngineCore&, double dT)
{
	renderWindow.Present(1, 0);
}


/************************************************************************************************/


bool EditorPlayerState::EventHandler(Event evt)
{
	return false;
}


/************************************************************************************************/


void EditorPlayerState::Reset()
{
	if (gameObject)
		gameObject->Release();
	else
		gameObject = &shared->blockAllocator.allocate<GameObject>();

	scene.ClearScene();

	gameObject->AddView<FlexKit::SceneNodeView>();
	scene.AddGameObject(*gameObject);
}


/************************************************************************************************/


void EditorPlayerState::SendBlob(FlexKit::Blob& blob)
{
	FlexKit::Vector<std::byte> outputBlob{ shared->blockAllocator };

	outputBlob.resize(blob.buffer.size());
	memcpy(outputBlob.data(), blob.data(), outputBlob.size());

	shared->outputQueue.push_front(outputBlob);
}


/************************************************************************************************/


void EditorPlayerState::SendErrorMessage(const std::string& message)
{
	struct ErrorMessage : public FlexKit::Serializable<ErrorMessage, EditorMessageInterface, GetTypeGUID(ResizeMessage)>
	{
		void Do() override
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

		FlexKit::Vector<std::byte> empty{};
		shared->outputQueue.push_front(empty);

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
