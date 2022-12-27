#pragma once

#include <Application.h>
#include <Win32Graphics.h>
#include <filesystem>
#include <expected>
#include <DebugUI.h>


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
	HairRenderingTest(FlexKit::GameFramework& IN_framework);
	~HairRenderingTest() final;

	ID3D12PipelineState* CreateApplyForcesPSO();
	ID3D12PipelineState* CreateApplyShapeConstraintsPSO();
	ID3D12PipelineState* CreateApplyEdgeLengthConstraintPSO();
	ID3D12PipelineState* CreateStrandRenderPSO();
	ID3D12PipelineState* CreateDebugRenderPSO();

	void ClearStyleBuffers(HairStyle& style);

	FlexKit::UpdateTask* Update(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT) final;
	FlexKit::UpdateTask* Draw(FlexKit::UpdateTask* update, FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT, FlexKit::FrameGraph& frameGraph) final;

	void Simulate(		FlexKit::UpdateTask*					update,
						FlexKit::EngineCore&					core,
						FlexKit::UpdateDispatcher&				dispatcher,
						double									dT,
						FlexKit::FrameGraph&					frameGraph,
						FlexKit::ReserveVertexBufferFunction&	reserveVB,
						FlexKit::ReserveConstantBufferFunction&	reserveCB);

	void DrawStrands(	FlexKit::UpdateTask*					update,
						FlexKit::EngineCore&					core,
						FlexKit::UpdateDispatcher&				dispatcher,
						double									dT,
						FlexKit::FrameGraph&					frameGraph,
						FlexKit::ReserveVertexBufferFunction&	reserveVB,
						FlexKit::ReserveConstantBufferFunction&	reserveCB);

	void DrawDebug	(	FlexKit::UpdateTask*					update,
						FlexKit::EngineCore&					core,
						FlexKit::UpdateDispatcher&				dispatcher,
						double									dT,
						FlexKit::FrameGraph&					frameGraph,
						FlexKit::ReserveVertexBufferFunction&	reserveVB,
						FlexKit::ReserveConstantBufferFunction&	reserveCB);

	void PostDrawUpdate(FlexKit::EngineCore&, double dT) final;
	bool EventHandler(FlexKit::Event evt) final;

	FlexKit::CameraComponent		cameras;
	FlexKit::SceneNodeComponent		sceneNodes;

	bool							pause		= false;
	bool							debugVis	= false;
	size_t							fps			= 0;
	size_t							counter		= 0;
	double							T			= 0.0;

	HairStyle						style;

	FlexKit::ImGUIIntegrator		debugUI;

	FlexKit::NodeHandle				cameraRig;
	FlexKit::CameraHandle			camera;

	FlexKit::RootSignature			strandRenderRootSignature;
	FlexKit::Win32RenderWindow		renderWindow;
	FlexKit::ResourceHandle			depthBuffer;

	FlexKit::VertexBufferHandle		vertexBuffer;
	FlexKit::ConstantBufferHandle	constantBuffer;

	FlexKit::RunOnceQueue<void (FlexKit::UpdateDispatcher&, FlexKit::FrameGraph&)>	runOnceQueue;
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
