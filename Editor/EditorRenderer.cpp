#include "PCH.h"
#include "EditorRenderer.h"
#include <Application.hpp>
#include <SharedEngineMemory.hpp>
#include <DefaultPipelineStates.hpp>


/************************************************************************************************/


EditorRenderer::EditorRenderer(FlexKit::GameFramework& IN_framework, FlexKit::FKApplication& IN_application, QApplication& IN_QtApplication) :
	FrameworkState	{ IN_framework		},
	QtApplication	{ IN_QtApplication	},
	application		{ IN_application	},
	vertexBuffer	{ GetRenderSystem().CreateVertexBuffer(MEGABYTE * 32, false) },
	constantBuffer	{ GetRenderSystem().CreateConstantBuffer(MEGABYTE * 128, false) },
	textureEngine	{ GetRenderSystem(), GetThreads(), GetAllocator() },
	worldRender		{ GetRenderSystem(), GetAllocator(), {}, 
	    PoolSizes { .UAVPoolByteSize = 512 * MEGABYTE, .RTPoolByteSize = 2 * GIGABYTE, .UAVTexturePoolByteSize = 2 * GIGABYTE }},

	materialComponent	{ IN_framework.GetRenderSystem(), IN_framework.core.GetBlockMemory(), &textureEngine },

	physX				{ IN_framework.core.Threads, IN_framework.core.GetBlockMemory() },
	staticBodies		{ physX },
	rigidBodies			{ physX },
	characterControllers{ physX, IN_framework.core.GetBlockMemory() },

	csg				{ IN_framework.core.GetBlockMemory() }
{
	auto& renderSystem = framework.GetRenderSystem();
	renderSystem.RegisterPSOLoader(DRAW_TEXTURED_PSO,	CreateTexturedTriStatePSO);
	renderSystem.RegisterPSOLoader(DRAW_3D_PSO,			CreateDrawTriStatePSO);
	renderSystem.RegisterPSOLoader(DRAW_TRI3D_PSO,		CreateDrawTri3DStatePSO);
	renderSystem.RegisterPSOLoader(DRAW_LINE_PSO,		CreateDrawLineStatePSO);

	renderSystem.QueuePSOLoad(DRAW_3D_PSO);
	renderSystem.QueuePSOLoad(DRAW_LINE_PSO);
	renderSystem.QueuePSOLoad(DRAW_TRI3D_PSO);

	allocator.Init((std::byte*)temporaryBuffer->buffer, sizeof(TempBuffer));

	static shared_memory_object shm_obj(
		open_or_create,
		"shared_memory",
		read_write);

	components = InitiateSharedMemory(shm_obj);
}


/************************************************************************************************/


size_t EditorRenderer::GetSharedAddress() const noexcept
{
	return (size_t)components;
}


/************************************************************************************************/


SharedEngineMemory* EditorRenderer::GetSharedMemory() const noexcept
{
	return components;
}


/************************************************************************************************/


EditorRenderer::~EditorRenderer()
{
	auto& renderSystem = framework.GetRenderSystem();

	renderSystem.ReleaseCB(constantBuffer);
	renderSystem.ReleaseVB(vertexBuffer);

	FK_LOG_0("Shutting Down Editor Renderer!");
}


/************************************************************************************************/


void EditorRenderer::DrawOneFrame(double dT)
{
	application.DrawOneFrame(dT);
}


/************************************************************************************************/


DXRenderWindow* EditorRenderer::CreateRenderWindow(QWidget* parent)
{
	auto viewPortWidget = new DXRenderWindow{ application.GetFramework().GetRenderSystem(), parent };
	renderWindows.push_back(viewPortWidget);

	return viewPortWidget;
}


/************************************************************************************************/


void EditorRenderer::DrawRenderWindow(DXRenderWindow* renderWindow)
{
	if (drawInProgress)
		return;

	return; // Causes crash for now
}


/************************************************************************************************/


TriMeshHandle EditorRenderer::LoadMesh(FlexKit::MeshResource& mesh)
{
	auto& renderSystem	= framework.GetRenderSystem();
	auto  copyContext	= renderSystem.GetImmediateCopyQueue();

	TriMesh newMesh;

	auto meshBlob = mesh.CreateBlob();
	TriMeshHandle handle = FlexKit::LoadTriMeshIntoTable(copyContext, meshBlob.buffer, meshBlob.bufferSize);

	return handle;
}


/************************************************************************************************/


UpdateTask& EditorRenderer::UpdatePhysx(UpdateDispatcher& dispatcher, double dT)
{
	return physX.Update(dispatcher, dT);
}


/************************************************************************************************/


UpdateTask* EditorRenderer::Update(EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT)
{
	renderWindows.erase(std::remove_if(std::begin(renderWindows), std::end(renderWindows),
		[](auto& I)
		{
			return !I->isValid();
		}),
		std::end(renderWindows));

	return nullptr;
}


/************************************************************************************************/


UpdateTask* EditorRenderer::Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{
	drawInProgress = true;

	ClearVertexBuffer(frameGraph, vertexBuffer);

	frameGraph.AddConstantBuffer(constantBuffer);
	frameGraph.AddVertexBuffer(vertexBuffer);

	for (auto renderWindow : renderWindows)
	{
		frameGraph.AddOutput(renderWindow->GetBackBuffer());
		renderWindow->Draw(core, dispatcher, dT, frameGraph, threadedAllocator);
	}

	return nullptr;
}


/************************************************************************************************/


void EditorRenderer::PostDrawUpdate(EngineCore& core, double dT)
{
	for (auto renderWindow : renderWindows)
		renderWindow->Present();

	GetRenderSystem().ResetConstantBuffer(constantBuffer);
	drawInProgress = false;
}


/**********************************************************************

Copyright (c) 2019-2025 Robert May

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
