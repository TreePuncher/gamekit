#pragma once
#include "GameFramework.h"
#include "Serialization.hpp"
#include "TextureStreamingUtilities.h"
#include "WorldRender.h"

namespace FlexKit
{
	struct Win32RenderWindow;
}

struct SharedEngineMemory;


/************************************************************************************************/

class EditorPlayerState;

struct MessageInterface : public FlexKit::SerializableInterface<GetTypeGUID(MessageInterface)>
{
	virtual void Do(EditorPlayerState&) = 0;
};


struct EditorMessageInterface : public FlexKit::SerializableInterface<GetTypeGUID(EditorMessageInterface)>
{
	virtual void Do() = 0;
};


/************************************************************************************************/


template<typename TY>
concept EditorIPCMessage = std::is_base_of_v<EditorMessageInterface, typename TY::element_type>;


class EditorPlayerState : public FlexKit::FrameworkState
{
public:
	EditorPlayerState(FlexKit::GameFramework&, SharedEngineMemory* IN_shared);

	FlexKit::UpdateTask* Update	(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT) override;
	FlexKit::UpdateTask* Draw	(FlexKit::UpdateTask* update, FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT, FlexKit::FrameGraph& frameGraph) override;


	void PostDrawUpdate(FlexKit::EngineCore&, double dT) override;
	bool EventHandler(FlexKit::Event evt) override;

	void Reset();

	void SendEditorMessage(EditorIPCMessage auto& message)
	{
		FlexKit::SaveArchiveContext archive;

		archive& message;
		auto blob = archive.GetBlob();

		SendBlob(blob);
	}

	void SendBlob(FlexKit::Blob& blob);

	void SendErrorMessage(const std::string& message);

	void AddResource(FlexKit::Blob&& blob);
	void AddGameObject(FlexKit::GameObject& gameObject);

	void Shutdown();

	FlexKit::Scene						scene;
	SharedEngineMemory*					shared		= nullptr;
	FlexKit::GameObject*				gameObject	= nullptr;
	FlexKit::BrushComponent				brushes;

	FlexKit::ConstantBufferHandle		constantBuffer;
	FlexKit::VertexBufferHandle			vertexBuffer;
	FlexKit::TextureStreamingEngine		textureStreaming;
	FlexKit::WorldRender				renderer;

	FlexKit::Win32RenderWindow&		renderWindow;
	std::vector<FlexKit::Blob>		resourceBlobs;
};

int PlayerMain(int argc, char* args[]);


/************************************************************************************************/


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
