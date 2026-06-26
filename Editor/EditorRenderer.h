#pragma once
#include <QtWidgets/qapplication>

#include "CSGComponent.h"
#include "CSGRendering.h"
#include "DXRenderWindow.h"
#include "EditorMeshResource.hpp"

#include <AnimationComponents.hpp>
#include <DebugUI.hpp>
#include <GameFramework.hpp>
#include <Materials.hpp>
#include <PhysicsUtilities.hpp>
#include <Scene.hpp>
#include <TriggerComponent.hpp>
#include <WorldRender.hpp>
#include <TextureStreaming/TextureStreamingUtilities.hpp>

class QWidget;
class SharedEngineMemory;

class EditorRenderer : public FlexKit::FrameworkState
{
public:
	EditorRenderer(FlexKit::GameFramework& IN_framework, FlexKit::FKApplication& IN_application, QApplication& IN_QtApplication);
	~EditorRenderer();

	void DrawOneFrame(double dT);

	DXRenderWindow* CreateRenderWindow(QWidget* parent = nullptr);
	void DrawRenderWindow(DXRenderWindow* renderWindow);

	FlexKit::TriMeshHandle LoadMesh(FlexKit::MeshResource& mesh);
	FlexKit::IRenderSystem& GetRenderSystem() { return *framework.core.RenderSystem; }

	FlexKit::UpdateTask& UpdatePhysx(FlexKit::UpdateDispatcher& dispatcher, double dT);

	size_t				GetSharedAddress() const noexcept;
	SharedEngineMemory*	GetSharedMemory() const noexcept;

protected:
	FlexKit::UpdateTask* Update (FlexKit::EngineCore& Engine, FlexKit::UpdateDispatcher& Dispatcher, double dT) override;
	FlexKit::UpdateTask* Draw   (FlexKit::UpdateTask* update, FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT, FlexKit::FrameGraph& frameGraph) override;

	void PostDrawUpdate(FlexKit::EngineCore& core, double dT) override;

	std::atomic_bool				drawInProgress = false;

public:
	FlexKit::TextureStreamingEngine	textureEngine;
	FlexKit::WorldRender			worldRender;
	CSGRender						csgRender;

private:
	struct TempBuffer
	{
		char buffer[MEGABYTE * 32];
	};

	BlockAllocator		sharedAllocator;
	SharedEngineMemory*	components	= nullptr;

	FlexKit::MaterialComponent	materialComponent;

	// physX components
	FlexKit::PhysXComponent					physX;
	FlexKit::StaticBodyComponent			staticBodies;
	FlexKit::RigidBodyComponent				rigidBodies;
	FlexKit::CharacterControllerComponent	characterControllers;

	// Editor Only Components
	CSGComponent csg;

	std::unique_ptr<TempBuffer>		temporaryBuffer = std::make_unique<TempBuffer>();

	FlexKit::StackAllocator			allocator;
	FlexKit::ThreadSafeAllocator	threadedAllocator{ FlexKit::SystemAllocator };

	// Temp Buffers
	FlexKit::VertexBufferHandle		vertexBuffer;
	FlexKit::ConstantBufferHandle	constantBuffer;


	QApplication&					QtApplication;
	FlexKit::FKApplication&			application;
	std::vector<DXRenderWindow*>	renderWindows;
};


/**********************************************************************

Copyright (c) 2019-2024 Robert May

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
