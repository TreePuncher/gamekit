#include "PCH.h"
#include "EditorRenderer.h"


/************************************************************************************************/


EditorRenderer::EditorRenderer(FlexKit::GameFramework& IN_framework, FlexKit::FKApplication& IN_application, QApplication& IN_QtApplication) :
	FrameworkState	{ IN_framework		},
	QtApplication	{ IN_QtApplication	},
	application		{ IN_application	},
	vertexBuffer	{ IN_framework.core.RenderSystem.CreateVertexBuffer(MEGABYTE * 32, false) },
	constantBuffer	{ IN_framework.core.RenderSystem.CreateConstantBuffer(MEGABYTE * 128, false) },
	textureEngine	{ IN_framework.core.RenderSystem, IN_framework.core.GetBlockMemory() },
	worldRender		{ IN_framework.core.RenderSystem, textureEngine, IN_framework.core.GetBlockMemory(), { .UAVPoolByteSize = 512 * MEGABYTE, .RTPoolByteSize = 2 * GIGABYTE, .UAVTexturePoolByteSize = 2 * GIGABYTE }},

	csg						{ IN_framework.core.GetBlockMemory() },
	brushComponent			{ IN_framework.core.GetBlockMemory(), IN_framework.core.RenderSystem },
	stringIDComponent		{ IN_framework.core.GetBlockMemory() },
	materialComponent		{ IN_framework.core.RenderSystem, textureEngine, IN_framework.core.GetBlockMemory() },
	cameraComponent			{ IN_framework.core.GetBlockMemory() },
	visibilityComponent		{ IN_framework.core.GetBlockMemory() },
	skeletonComponent		{ IN_framework.core.GetBlockMemory() },
	animatorComponent		{ IN_framework.core.GetBlockMemory() },

	lightComponent			{ IN_framework.core.GetBlockMemory() },
	shadowMaps				{ IN_framework.core.GetBlockMemory() },

	triggers				{ IN_framework.core.GetBlockMemory(), IN_framework.core.GetBlockMemory() },

	physX					{ IN_framework.core.Threads, IN_framework.core.GetBlockMemory() },
	staticBodies			{ physX },
	rigidBodies				{ physX },
	characterControllers	{ physX, IN_framework.core.GetBlockMemory() },

	ikTargetComponent		{ IN_framework.core.GetBlockMemory() },
	ikComponent				{ IN_framework.core.GetBlockMemory() }
{
	auto& renderSystem = framework.GetRenderSystem();
	renderSystem.RegisterPSOLoader(FlexKit::DRAW_TEXTURED_PSO,	{ &renderSystem.Library.RS6CBVs4SRVs, FlexKit::CreateTexturedTriStatePSO });
	renderSystem.RegisterPSOLoader(FlexKit::DRAW_3D_PSO,		{ &renderSystem.Library.RS6CBVs4SRVs, FlexKit::CreateDrawTriStatePSO });
	renderSystem.RegisterPSOLoader(FlexKit::DRAW_TRI3D_PSO,		{ &renderSystem.Library.RS6CBVs4SRVs, FlexKit::CreateDrawTri3DStatePSO });
	renderSystem.RegisterPSOLoader(FlexKit::DRAW_LINE_PSO,		{ &renderSystem.Library.RS6CBVs4SRVs, FlexKit::CreateDrawLineStatePSO });

	renderSystem.QueuePSOLoad(FlexKit::DRAW_3D_PSO);
	renderSystem.QueuePSOLoad(FlexKit::DRAW_LINE_PSO);
	renderSystem.QueuePSOLoad(FlexKit::DRAW_TRI3D_PSO);

	allocator.Init((byte*)temporaryBuffer->buffer, sizeof(TempBuffer));
}


/************************************************************************************************/


EditorRenderer::~EditorRenderer()
{
	auto& renderSystem = framework.GetRenderSystem();

	renderSystem.ReleaseCB(constantBuffer);
	renderSystem.ReleaseVB(vertexBuffer);
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


FlexKit::TriMeshHandle EditorRenderer::LoadMesh(FlexKit::MeshResource& mesh)
{
	auto& renderSystem	= framework.GetRenderSystem();
	auto  copyContext	= renderSystem.GetImmediateCopyQueue();

	TriMesh newMesh;

	auto meshBlob = mesh.CreateBlob();
	FlexKit::TriMeshHandle handle = FlexKit::LoadTriMeshIntoTable(copyContext, meshBlob.buffer, meshBlob.bufferSize);

	return handle;
}


/************************************************************************************************/


FlexKit::UpdateTask& EditorRenderer::UpdatePhysx(FlexKit::UpdateDispatcher& dispatcher, double dT)
{
	return physX.Update(dispatcher, dT);
}


/************************************************************************************************/


FlexKit::UpdateTask* EditorRenderer::Update(FlexKit::EngineCore& Engine, FlexKit::UpdateDispatcher& Dispatcher, double dT)
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


FlexKit::UpdateTask* EditorRenderer::Draw(FlexKit::UpdateTask* update, FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT, FlexKit::FrameGraph& frameGraph)
{
	drawInProgress = true;

	core.RenderSystem.ResetConstantBuffer(constantBuffer);
	FlexKit::ClearVertexBuffer(frameGraph, vertexBuffer);

	TemporaryBuffers temporaries{
		FlexKit::CreateVertexBufferReserveObject(vertexBuffer, core.RenderSystem, core.GetTempMemory()),
		FlexKit::CreateConstantBufferReserveObject(constantBuffer, core.RenderSystem, core.GetTempMemory()),
	};

	for (auto renderWindow : renderWindows)
	{
		frameGraph.AddOutput(renderWindow->GetBackBuffer());
		renderWindow->Draw(core, temporaries, dispatcher, dT, frameGraph, threadedAllocator);
	}

	return nullptr;
}


/************************************************************************************************/


void EditorRenderer::PostDrawUpdate(FlexKit::EngineCore& core, double dT)
{
	for (auto renderWindow : renderWindows)
		renderWindow->Present();

	drawInProgress = false;
}


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
