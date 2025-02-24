#pragma once

#include <Application.hpp>
#include <Win32Graphics.hpp>
#include <filesystem>
#include <expected>
#include <DebugUI.hpp>
#include <RMLRenderer.hpp>
#include <GPUAllocators.hpp>


/************************************************************************************************/

// Forward Declarations
namespace FlexKit
{
	class UpdateTask;
}


/************************************************************************************************/

using FlexKit::float4;

struct ControlPoint
{
	float		position[3];
	uint16_t	w;
	uint16_t	l;
	float		velocity[3] = { 0, 0, 0 };
	uint16_t	angularVelocity;
	uint16_t	pad;
	float4		q = { 0, 0, 0, 1 };
};


struct HairStyle
{
	uint32_t	strandLength	= 0;
	uint32_t	strandCount		= 0;
	uint8_t		currentBuffer	= 0;

	FlexKit::ResourceHandle	styleBuffer;
	FlexKit::ResourceHandle	hairBuffers[2];
	FlexKit::ResourceHandle	strandbuffer;

	FlexKit::ResourceHandle	GetCurrentHairBuffer()		const { return hairBuffers[(currentBuffer + 1) % 2]; }
	FlexKit::ResourceHandle	GetCurrentSourceBuffer()	const { return hairBuffers[(currentBuffer + 0) % 2]; }
};


struct ImportedStyleBuffer
{
	uint32_t	strandLength = 0;
	uint32_t	strandCount = 0;

	FlexKit::Vector<ControlPoint> controlPoints;
};


HairStyle	CreateStyle(FlexKit::RenderSystem& renderSystem, const uint32_t strandLength, const uint32_t strandCount);
void		ReleaseStyle(HairStyle& style, FlexKit::RenderSystem& renderSystem);
void		UploadHairStyle(HairStyle& style, const ImportedStyleBuffer& stylePoints, FlexKit::RenderSystem& renderSystem);

std::expected<ImportedStyleBuffer, int> ImportCSV(const std::filesystem::path& path, FlexKit::iAllocator& allocator);


/************************************************************************************************/


class HairRenderingTest : public FlexKit::FrameworkState
{
public:
	HairRenderingTest(FlexKit::GameFramework& IN_framework, bool enableWorkGraph = false);
	~HairRenderingTest() final;

	void CreateWorkGraphObjects();

	FlexKit::LoadPipelineStateRes CreateApplyForcesPSO					(FlexKit::iAllocator& tempMemory);
	FlexKit::LoadPipelineStateRes CreateApplyShapeConstraintsPSO		(FlexKit::iAllocator& tempMemory);
	FlexKit::LoadPipelineStateRes CreateApplyEdgeLengthConstraintPSO	(FlexKit::iAllocator& tempMemory);

	FlexKit::LoadPipelineStateRes CreateStrandRenderOpaquePSO			(FlexKit::iAllocator& tempMemory);

	FlexKit::LoadPipelineStateRes CreateStrandRender1PSO				(FlexKit::iAllocator& tempMemory);
	FlexKit::LoadPipelineStateRes CreateStrandRender2PSO				(FlexKit::iAllocator& tempMemory);
	FlexKit::LoadPipelineStateRes CreateBlendState						(FlexKit::iAllocator& tempMemory);

	void ClearStyleBuffers(HairStyle& style);


	FlexKit::UpdateTask* Update(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT) final;
	FlexKit::UpdateTask* Draw(FlexKit::UpdateTask* update, FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT, FlexKit::FrameGraph& frameGraph) final;

	void Simulate(		FlexKit::UpdateTask*					update,
						FlexKit::EngineCore&					core,
						FlexKit::UpdateDispatcher&				dispatcher,
						const double							dT,
						FlexKit::FrameGraph&					frameGraph);

	void DrawStrands(	FlexKit::UpdateTask*					update,
						FlexKit::EngineCore&					core,
						FlexKit::UpdateDispatcher&				dispatcher,
						const double							dT,
						FlexKit::FrameGraph&					frameGraph);

	void DrawStrandsOIT(FlexKit::UpdateTask*					update,
						FlexKit::EngineCore&					core,
						FlexKit::UpdateDispatcher&				dispatcher,
						const double							dT,
						FlexKit::FrameGraph&					frameGraph);

	void WorkGraph(		FlexKit::UpdateTask*					update,
						FlexKit::EngineCore&					core,
						FlexKit::UpdateDispatcher&				dispatcher,
						const double							dT,
						FlexKit::FrameGraph&					frameGraph);

	void PostDrawUpdate	(FlexKit::EngineCore&, double dT) final;
	bool EventHandler	(FlexKit::Event evt) final;


	FlexKit::RmlIntegrator			ui;

	FlexKit::CameraComponent		cameras;
	FlexKit::SceneNodeComponent		sceneNodes;

	bool							pause		= false;
	bool							debugVis	= false;
	size_t							fps			= 0;
	size_t							counter		= 0;
	uint32_t						debugOffset = 0;
	double							T			= 0.0;

	HairStyle						style;

	FlexKit::NodeHandle				cameraRig;
	FlexKit::CameraHandle			camera;

	FlexKit::Win32RenderWindow*		renderWindow;
	FlexKit::ResourceHandle			depthBuffer;

	FlexKit::VertexBufferHandle		vertexBuffer;
	FlexKit::ConstantBufferHandle	constantBuffer;
	FlexKit::MemoryPoolAllocator	UAVPool;
	FlexKit::MemoryPoolAllocator	RTPool;

	FlexKit::RunOnceQueue<void (FlexKit::UpdateDispatcher&, FlexKit::FrameGraph&)>	runOnceQueue;

	FlexKit::GPUBlockAllocator		persistentConstants;
	FlexKit::ResourceHandle			constantBufferPool;

	FlexKit::DevicePointer			constants = 0;

	enum class Mode
	{
		Default,
		WorkGraph
	}	mode = Mode::Default;

	struct WorkGraphObjects
	{
		~WorkGraphObjects()
		{
			if (stateObject)
			{
				stateObject->Release();
				properties->Release();

				stateObject = nullptr;
				properties	= nullptr;
			}
		}

		FlexKit::RootSignature*			globalRootSignature	= nullptr;

		ID3D12StateObject*				stateObject			= nullptr;
		ID3D12StateObjectProperties1*	properties			= nullptr;
		ID3D12WorkGraphProperties*		workGraphProperties	= nullptr;

		D3D12_PROGRAM_IDENTIFIER		main;
		uint32_t						mainID;
	}	workGraphObjects;
};


/**********************************************************************

Copyright (c) 2014-2022 Robert May

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
