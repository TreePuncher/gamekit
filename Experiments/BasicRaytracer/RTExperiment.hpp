#pragma once
#include <cstdint>
#include <BuildSettings.hpp>
#include <Components.hpp>
#include <Containers.hpp>
#include <EngineCore.hpp>
#include <GameFramework.hpp>
#include <Handle.hpp>
#include <ResourceHandles.hpp>
#include <Scene.hpp>
#include <RenderSystemInterface.hpp>
#include <PersistentGPUAllocator.hpp>
#include <Transforms.hpp>
#include <Type.hpp>

#include <dxRenderSystem.hpp>

#include "CameraComponent.hpp"
#include "DepthBuffer.hpp"
#include "ShaderBindingTable.hpp"


constexpr FlexKit::GUID_t		VertexShaderAssetID	= GetCRCGUID(VertexShader);
constexpr FlexKit::GUID_t		PixelShaderAssetID	= GetCRCGUID(PixelShader);
constexpr FlexKit::PassHandle	RTPass				= GetCRC32("RTPass");

constexpr uint32_t DiffuseColor		= GetCRC32("Diffuse");
constexpr uint32_t LightIrradiance	= GetCRC32("Irradiance");

struct ForwardPassData
{
	FlexKit::FrameResourceHandle renderTarget;
	FlexKit::FrameResourceHandle depthTarget;
};

struct UpdateSBTData
{
	FlexKit::FrameGraphNodeHandle	node;
	FlexKit::FrameResourceHandle	sbtBuffer;
};

struct TracePassData
{
	FlexKit::FrameResourceHandle sbtBuffer;
	FlexKit::FrameResourceHandle tlas;
	FlexKit::FrameResourceHandle traceBuffer;
	FlexKit::FrameResourceHandle renderTarget;
};

struct RTExperimentState final : FlexKit::FrameworkState
{
	RTExperimentState(FlexKit::GameFramework& IN_framework);

	~RTExperimentState() override;

	FlexKit::UpdateTask* Update(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT) override;

	FlexKit::UpdateTask* Draw(FlexKit::UpdateTask* update, FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT, FlexKit::FrameGraph& frameGraph) override;

	ForwardPassData&	ForwardPass		(FlexKit::FrameGraph& framegraph, FlexKit::ResourceHandle renderTarget, FlexKit::GatherPassesTask& passes, FlexKit::CameraUpdateTask& cameraUpdate);
	UpdateSBTData&		UpdateSBT		(FlexKit::FrameGraph& frameGraph, FlexKit::GatherPassesTask& passes);
	TracePassData&		PathTracePass	(FlexKit::FrameGraph& frameGraph, FlexKit::GatherPassesTask& passes, FlexKit::CameraUpdateTask& cameraUpdate, UpdateSBTData& sbtUpdate, FlexKit::ResourceHandle renderTarget);

    void PostDrawUpdate(FlexKit::EngineCore&, double dT) override;

	FlexKit::SceneNodeComponent			transforms;
	FlexKit::BrushComponent				brushes;
	FlexKit::CameraComponent				cameras;
	FlexKit::LightComponent				lights;
	FlexKit::MaterialComponent			materials;
	FlexKit::SceneVisibilityComponent	visibility;
	FlexKit::TriggerComponent			triggers;
	FlexKit::ObjectPool<FlexKit::GameObject>		gameObjects;

	FlexKit::Scene					scene;
	FlexKit::CameraHandle			activeCamera;
	FlexKit::GameObject*				cameraObj;

	double					t = 0.0;
	FlexKit::IRenderWindow*			renderWindow = nullptr;
	FlexKit::VertexBufferHandle		vBuffer			= FlexKit::InvalidHandle;
	FlexKit::ConstantBufferHandle	cBuffer			= FlexKit::InvalidHandle;

	FlexKit::TriMeshHandle			suzanneMesh		= FlexKit::InvalidHandle;
	FlexKit::TriMeshHandle			roomMesh		= FlexKit::InvalidHandle;
	FlexKit::TriMeshHandle			lightMesh		= FlexKit::InvalidHandle;

	dx_Internal::MemoryPoolAllocator gpuAllocator;

	FlexKit::DepthBuffer			depthBuffer;
	FlexKit::PersistentAllocator	persistent;
	FlexKit::IPipelineInterface*	globalInterface = nullptr;
	FlexKit::IPipelineStateLibrary*	library			= nullptr;
	FlexKit::ShaderBindingTable		sbt;

	bool	trace = true;

	FlexKit::ShaderID raygenID;
	FlexKit::ShaderID missID;
	FlexKit::ShaderID defaultMaterial;
	FlexKit::ShaderID lightMaterial;

	using DeviceAddressRange = FlexKit::DeviceAddressRange;
	FlexKit::GPURange	SBTMemory;
	FlexKit::GPURange	hitTable;
	FlexKit::GPURange	missTable;
	FlexKit::GPURange	rayGenerator;
	FlexKit::GPURange	sceneInstances;
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
