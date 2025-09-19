#pragma once

#include "BuildSettings.hpp"
#include "ClusteredRendering.hpp"
#include "Components.hpp"
#include "FrameGraph.hpp"
#include "RenderSystemInterface.hpp"
#include "Transparency.hpp"


namespace FlexKit
{   /************************************************************************************************/


	constexpr PSOHandle VXGI_INITOCTREE                 = PSOHandle(GetTypeGUID(VXGI_INITOCTREE));
	constexpr PSOHandle VXGI_CLEAR                      = PSOHandle(GetTypeGUID(VXGI_CLEAR));
	constexpr PSOHandle VXGI_ALLOCATENODES              = PSOHandle(GetTypeGUID(VXGI_ALLOCATENODES));
	constexpr PSOHandle VXGI_DECREMENTCOUNTER           = PSOHandle(GetTypeGUID(VXGI_DECREMENTCOUNTER));
	constexpr PSOHandle VXGI_SAMPLEINJECTION            = PSOHandle(GetTypeGUID(VXGI_SAMPLEINJECTION));
	constexpr PSOHandle VXGI_DRAWVOLUMEVISUALIZATION    = PSOHandle(GetTypeGUID(VXGI_DRAWVOLUMEVISUALIZATION));
	constexpr PSOHandle VXGI_GATHERDISPATCHARGS         = PSOHandle(GetTypeGUID(VXGI_GATHERSISPATCHARGS));
	constexpr PSOHandle VXGI_GATHERREMOVEARGS           = PSOHandle(GetTypeGUID(VXGI_GATHERREMOVEARGS));
	constexpr PSOHandle VXGI_GATHERDRAWARGS             = PSOHandle(GetTypeGUID(VXGI_GATHERDRAWARGS));
	constexpr PSOHandle VXGI_GATHERSUBDIVISIONREQUESTS  = PSOHandle(GetTypeGUID(VXGI_GATHERSUBDIVISIONREQUESTS));
	constexpr PSOHandle VXGI_PROCESSSUBDREQUESTS        = PSOHandle(GetTypeGUID(VXGI_PROCESSSUBDREQUESTS));

	constexpr PSOHandle SVO_Voxelize                    = PSOHandle(GetTypeGUID(SVO_Voxelize));
	constexpr PSOHandle SVO_GatherArguments             = PSOHandle(GetTypeGUID(SVO_GATHERARGS));
	constexpr PSOHandle SVO_GATHERSUBDIVISIONREQUESTS   = PSOHandle(GetTypeGUID(SVO_GATHERSUBDIVISIONREQUESTS));
	constexpr PSOHandle SVO_EXPANDNODES                 = PSOHandle(GetTypeGUID(SVO_EXPANDNODES));
	constexpr PSOHandle SVO_FILLNODES                   = PSOHandle(GetTypeGUID(SVO_FILLNODES));
	constexpr PSOHandle SVO_BUILDMIPLEVEL               = PSOHandle(GetTypeGUID(SVO_BUILDMIPLEVEL));


	/************************************************************************************************/


	struct UpdateVoxelVolume
	{
		CameraHandle		camera;
		FrameResourceHandle octree;
		FrameResourceHandle depthTarget;
		FrameResourceHandle counters;
		FrameResourceHandle indirectArgs;
		FrameResourceHandle sampleBuffer;
	};


	/************************************************************************************************/


	struct SVO_RayTrace
	{
		CameraHandle		camera;

		FrameResourceHandle depthTarget;
		FrameResourceHandle renderTarget;
		FrameResourceHandle normals;
		FrameResourceHandle albedo;

		FrameResourceHandle indirectArgs;
		FrameResourceHandle octree;
		FrameResourceHandle brickBuffer;
	};


	/************************************************************************************************/


	class StaticVoxelizer
	{
	public:
		StaticVoxelizer(IRenderSystem& renderSystem, iAllocator& allocator);

		
		// No Copy
							StaticVoxelizer (const StaticVoxelizer&) = delete;
		StaticVoxelizer&    operator =      (const StaticVoxelizer&) = delete;

		// No Move
							StaticVoxelizer (StaticVoxelizer&&) = delete;
		StaticVoxelizer&   operator =       (StaticVoxelizer&&) = delete;

		struct VoxelizePass
		{
			GatherPassesTask&   passes;
			FrameResourceHandle sampleBuffer;
			FrameResourceHandle argBuffer;
			FrameResourceHandle tempBuffer;
			FrameResourceHandle octree;
			FrameResourceHandle parentBuffer;
			FrameResourceHandle counters;
		};


		VoxelizePass& VoxelizeScene(
			FrameGraph&                     frameGraph,
			Scene&                          scene,
			ResourceHandle                  octreeBuffer,
			uint3                           XYZ,
			GatherPassesTask&               passes);


	private:

		struct CommonResources
		{
			const ConstantBufferDataSet cameraConstants;
			const FrameResourceHandle   octree;
			const FrameResourceHandle   parentBuffer;
			const FrameResourceHandle   sampleBuffer;
			const FrameResourceHandle   argBuffer;
			const FrameResourceHandle   tempBuffer;
		};


		void GatherArgs(FrameResourceHandle argBuffer, FrameResourceHandle sampleBuffer, ResourceHandler& resources, IDirectContext& ctx, iAllocator& TL_allocator, const size_t offset = 0);

		void GatherSamples  (CommonResources&, Scene&, ResourceHandler&, IDirectContext&, iAllocator& TL_allocator);
		void BuildTree      (CommonResources&, ResourceHandler& resources, IDirectContext& ctx, iAllocator& TL_allocator);
		void BuildMIPLevels (CommonResources&, ResourceHandler& resources, IDirectContext& ctx, iAllocator& TL_allocator);


		LoadPipelineStateRes CreateVoxelizerPSO         (IRenderSystem& RS, iAllocator& temp);
		LoadPipelineStateRes CreateGatherArgsPSO        (IRenderSystem& RS, iAllocator& temp);
		LoadPipelineStateRes CreateMarkNodesPSO         (IRenderSystem& RS, iAllocator& temp);
		LoadPipelineStateRes CreateExpandNodesPSO       (IRenderSystem& RS, iAllocator& temp);
		LoadPipelineStateRes CreateFillAttributesPSO    (IRenderSystem& RS, iAllocator& temp);
		LoadPipelineStateRes CreateBuildMIPLevelPSO     (IRenderSystem& RS, iAllocator& temp);


		const IRootSignature* voxelizeSignature;
		const IRootSignature* markSignature;

		IndirectLayout dispatch;
	};


	/************************************************************************************************/


	class GILightingEngine
	{
	public:
		GILightingEngine(IRenderSystem& renderSystem, iAllocator& allocator);
		~GILightingEngine();

		// No Copy
							GILightingEngine    (const GILightingEngine&) = delete;
		GILightingEngine&   operator =          (const GILightingEngine&) = delete;

		// No Move
							GILightingEngine    (GILightingEngine&&) = delete;
		GILightingEngine&   operator =          (GILightingEngine&&) = delete;


		void InitializeOctree(FrameGraph& frameGraph);


		StaticVoxelizer::VoxelizePass& VoxelizeScene(
				FrameGraph&                     frameGraph,
				Scene&                          scene,
				GatherPassesTask&               passes);

		UpdateVoxelVolume&        UpdateVoxelVolumes(
				UpdateDispatcher&               dispatcher,
				FrameGraph&                     frameGraph,
				const CameraHandle              camera,
				ResourceHandle                  depthTarget,
				iAllocator*                     allocator);

		SVO_RayTrace&           RayTrace(
				UpdateDispatcher&               dispatcher,
				FrameGraph&                     frameGraph,
				const CameraHandle              camera,
				ResourceHandle                  depthTarget,
				FrameResourceHandle             renderTarget,
				GBuffer&                        gbuffer,
				iAllocator*                     allocator,
				uint32_t                        mipOffset = 0);

	private:

		struct alignas(64) OctTreeNode
		{
			uint parent;
			uint children;
			uint flags;
			//uint RGBA[8];
			uint padding;
		};


		void CleanUpPhase(UpdateVoxelVolume& data, ResourceHandler& resources, IDirectContext&, iAllocator& temp);
		void CreateNodePhase(UpdateVoxelVolume& data, ResourceHandler& resources, IDirectContext&, iAllocator& temp);

		void _GatherArgs(FrameResourceHandle source, FrameResourceHandle argsBuffer, ResourceHandler& resources, IDirectContext& ctx, iAllocator& temp);

		IRenderSystem& renderSystem;

		ResourceHandle  octreeBuffer;

		IndirectLayout dispatch;
		IndirectLayout draw;

		IndirectLayout remove;

		const IRootSignature* gatherSignature;
		const IRootSignature* dispatchSignature;
		const IRootSignature* removeSignature;

		StaticVoxelizer staticVoxelizer;

		//ID3D12PipelineState* gatherDispatchArgs;
		//ID3D12PipelineState* gatherDispatchArgs2;

		LoadPipelineStateRes CreateAllocatePSO						(IRenderSystem& RS, iAllocator& temp);
		LoadPipelineStateRes CreateTransferPSO						(IRenderSystem& RS, iAllocator& temp);
		LoadPipelineStateRes CreateVXGIGatherDispatchArgsPSO		(IRenderSystem& RS, iAllocator& temp);
		LoadPipelineStateRes CreateVXGIEraseDispatchArgsPSO			(IRenderSystem& RS, iAllocator& temp);
		LoadPipelineStateRes CreateVXGIDecrementDispatchArgsPSO		(IRenderSystem& RS, iAllocator& temp);

		static LoadPipelineStateRes CreateVXGI_InitOctree				(IRenderSystem& RS, iAllocator& temp);
		static LoadPipelineStateRes CreateInjectVoxelSamplesPSO			(IRenderSystem& RS, iAllocator& temp);
		static LoadPipelineStateRes CreateMarkErasePSO					(IRenderSystem& RS, iAllocator& temp);
		static LoadPipelineStateRes CreateUpdateVolumeVisualizationPSO	(IRenderSystem& RS, iAllocator& temp);
		static LoadPipelineStateRes CreateVXGIGatherDrawArgsPSO			(IRenderSystem& RS, iAllocator& temp);
		static LoadPipelineStateRes CreateVXGIGatherSubDRequestsPSO		(IRenderSystem& RS, iAllocator& temp);
		static LoadPipelineStateRes CreateVXGIProcessSubDRequestsPSO	(IRenderSystem& RS, iAllocator& temp);
	};
}


/**********************************************************************

Copyright (c) 2014-2025 Robert May

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
