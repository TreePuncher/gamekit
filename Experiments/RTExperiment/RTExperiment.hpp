#include <Application.hpp>
#include <Scene.hpp>
#include <RenderSystemInterface.hpp>
#include <FrameGraph.hpp>
#include <filesystem>
#include <PersistentGPUAllocator.hpp>
#include <Transforms.hpp>
#include <Type.hpp>

#include <Win32Graphics.hpp>
#include <dxContext.hpp>
#include <dxRenderSystem.hpp>

#include "CameraComponent.hpp"
#include "DepthBuffer.hpp"
#include "ShaderBindingTable.hpp"

using namespace FlexKit;

constexpr GUID_t VertexShaderAssetID = GetCRCGUID(VertexShader);
constexpr GUID_t PixelShaderAssetID = GetCRCGUID(PixelShader);
constexpr PassHandle RTPass = GetCRC32("RTPass");

struct ForwardPassData
{
	FrameResourceHandle renderTarget;
	FrameResourceHandle depthTarget;
};

struct UpdateSBTData
{
	FrameResourceHandle sbtBuffer;
};

struct TracePassData
{
	FrameResourceHandle sbtBuffer;
	FrameResourceHandle traceBuffer;
	FrameResourceHandle renderTarget;
};

struct RTExperimentState final : FrameworkState
{
	RTExperimentState(GameFramework& IN_framework);

	~RTExperimentState() override;

	UpdateTask* Update(EngineCore&, UpdateDispatcher&, double dT) override;

	UpdateTask* Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph) override;

	ForwardPassData&	ForwardPass		(FrameGraph& framegraph, ResourceHandle renderTarget, GatherPassesTask& passes, CameraUpdateTask& cameraUpdate);
	UpdateSBTData&		UpdateSBT		(FrameGraph& frameGraph, GatherPassesTask& passes);
	TracePassData&		PathTracePass	(FrameGraph& frameGraph, CameraUpdateTask& cameraUpdate, UpdateSBTData& sbtUpdate, ResourceHandle renderTarget);

    void PostDrawUpdate(EngineCore&, double dT) override;

	SceneNodeComponent			transforms;
	BrushComponent				brushes;
	CameraComponent				cameras;
	LightComponent				lights;
	MaterialComponent			materials;
	SceneVisibilityComponent	visibility;
	TriggerComponent			triggers;
	ObjectPool<GameObject>		gameObjects;

	Scene					scene;
	CameraHandle			activeCamera;
	GameObject*				cameraObj;

	double					t = 0.0;
	IRenderWindow*			renderWindow = nullptr;
	VertexBufferHandle		vBuffer = InvalidHandle;
	ConstantBufferHandle	cBuffer = InvalidHandle;

	TriMeshHandle			suzanneMesh		= InvalidHandle;
	TriMeshHandle			roomMesh		= InvalidHandle;
	TriMeshHandle			lightMesh		= InvalidHandle;

	dx_Internal::MemoryPoolAllocator gpuAllocator;

	DepthBuffer				depthBuffer;
	PersistentAllocator		persistent;
	IPipelineInterface*		globalInterface = nullptr;
	IPipelineStateLibrary*	library			= nullptr;
	ShaderBindingTable		sbt;

	bool	trace = false;

	ShaderID raygenID;
	ShaderID missID;
	ShaderID defaultGroup1;

	using DeviceAddressRange = FlexKit::DeviceAddressRange;
	GPURange SBTMemory;
	GPURange hitTable;
	GPURange missTable;
	GPURange rayGenerator;
};


/**********************************************************************

Copyright (c) 2015 - 2025 Robert May

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
