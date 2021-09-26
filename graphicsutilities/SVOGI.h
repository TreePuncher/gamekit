#pragma once

#include "buildsettings.h"
#include "Components.h"
#include "FrameGraph.h"
#include "graphics.h"
#include "Transparency.h"

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
    constexpr PSOHandle SVO_SortSingleBlock             = PSOHandle(GetTypeGUID(SVO_SortSingleBlock));
    constexpr PSOHandle SVO_SortMultiBlock              = PSOHandle(GetTypeGUID(SVO_SortMultiBlock));
    constexpr PSOHandle SVO_GatherArguments             = PSOHandle(GetTypeGUID(SVO_GATHERARGS));


    /************************************************************************************************/


    struct UpdateVoxelVolume
    {
        ReserveConstantBufferFunction   reserveCB;
        CameraHandle                    camera;

        FrameResourceHandle octree;

        FrameResourceHandle depthTarget;
        FrameResourceHandle counters;
        FrameResourceHandle indirectArgs;
        FrameResourceHandle sampleBuffer;
    };


    /************************************************************************************************/


    struct DEBUGVIS_VoxelVolume
    {
        ReserveConstantBufferFunction   reserveCB;
        CameraHandle                    camera;

        FrameResourceHandle depthTarget;
        FrameResourceHandle renderTarget;

        FrameResourceHandle indirectArgs;
        FrameResourceHandle octree;
        FrameResourceHandle brickBuffer;
    };


    /************************************************************************************************/


    class StaticVoxelizer
    {
    public:
        StaticVoxelizer(RenderSystem& renderSystem, iAllocator& allocator);

        
        // No Copy
                            StaticVoxelizer (const StaticVoxelizer&) = delete;
        StaticVoxelizer&    operator =      (const StaticVoxelizer&) = delete;

        // No Move
                            StaticVoxelizer (StaticVoxelizer&&) = delete;
        StaticVoxelizer&   operator =       (StaticVoxelizer&&) = delete;

        struct VoxelizePass
        {
            GatherPassesTask&               passes;
            ReserveConstantBufferFunction   reserveCB;

            FrameResourceHandle             sampleBuffer;
            FrameResourceHandle             sortedBuffer;
            FrameResourceHandle             argBuffer;
        };


        VoxelizePass& VoxelizeScene(
            FrameGraph&                     frameGraph,
            Scene&                          scene,
            uint3                           XYZ,
            GatherPassesTask&               passes,
            ReserveConstantBufferFunction   reserveCB);

        void GatherArgs(FrameResourceHandle argBuffer, FrameResourceHandle sampleBuffer, ResourceHandler& resources, Context& ctx);

    private:

        ID3D12PipelineState* CreateVoxelizerPSO         (RenderSystem* RS);
        ID3D12PipelineState* CreateSortPSO              (RenderSystem* RS);
        ID3D12PipelineState* CreateMultiBlockSortPSO    (RenderSystem* RS);
        ID3D12PipelineState* CreateGatherArgsPSO        (RenderSystem* RS);


        RootSignature voxelizeSignature;
        RootSignature sortingSignature;

        IndirectLayout dispatch;
    };


    /************************************************************************************************/


    class GILightingEngine
    {
    public:
        GILightingEngine(RenderSystem& renderSystem, iAllocator& allocator);
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
                ReserveConstantBufferFunction   reserveCB,
                GatherPassesTask&               passes);

        UpdateVoxelVolume&        UpdateVoxelVolumes(
			    UpdateDispatcher&               dispatcher,
			    FrameGraph&                     frameGraph,
			    const CameraHandle              camera,
			    ResourceHandle                  depthTarget,
			    ReserveConstantBufferFunction   reserveCB,
			    iAllocator*                     allocator);

        DEBUGVIS_VoxelVolume&     DrawVoxelVolume(
			    UpdateDispatcher&               dispatcher,
			    FrameGraph&                     frameGraph,
			    const CameraHandle              camera,
                ResourceHandle                  depthTarget,
                FrameResourceHandle             renderTarget, 
			    ReserveConstantBufferFunction   reserveCB,
			    iAllocator*                     allocator);

    private:

        struct alignas(64) OctTreeNode
        {
            uint parent;
            uint children;
            uint flags;
            //uint RGBA[8];
            uint padding;
        };


        void CleanUpPhase(UpdateVoxelVolume& data, ResourceHandler& resources, Context&, iAllocator& temp);
        void CreateNodePhase(UpdateVoxelVolume& data, ResourceHandler& resources, Context&, iAllocator& temp);

        void _GatherArgs(FrameResourceHandle source, FrameResourceHandle argsBuffer, ResourceHandler& resources, Context& ctx);
        void _GatherArgs2(FrameResourceHandle freeList, FrameResourceHandle octree, FrameResourceHandle argsBuffer, ResourceHandler& resources, Context& ctx);

        RenderSystem& renderSystem;

        ResourceHandle  octreeBuffer;

        IndirectLayout dispatch;
        IndirectLayout draw;

        IndirectLayout remove;

        RootSignature gatherSignature;
        RootSignature dispatchSignature;

        RootSignature removeSignature;

        StaticVoxelizer staticVoxelizer;

        ID3D12PipelineState* gatherDispatchArgs;
        ID3D12PipelineState* gatherDispatchArgs2;

        ID3D12PipelineState* CreateAllocatePSO                  (RenderSystem* RS);
        ID3D12PipelineState* CreateTransferPSO                  (RenderSystem* RS);
        ID3D12PipelineState* CreateVXGIGatherDispatchArgsPSO    (RenderSystem* RS);
        ID3D12PipelineState* CreateVXGIEraseDispatchArgsPSO     (RenderSystem* RS);
        ID3D12PipelineState* CreateVXGIDecrementDispatchArgsPSO (RenderSystem* RS);

        static ID3D12PipelineState* CreateVXGI_InitOctree              (RenderSystem* RS);
        static ID3D12PipelineState* CreateInjectVoxelSamplesPSO        (RenderSystem* RS);
        static ID3D12PipelineState* CreateMarkErasePSO                  (RenderSystem* RS);
        static ID3D12PipelineState* CreateUpdateVolumeVisualizationPSO (RenderSystem* RS);
        static ID3D12PipelineState* CreateVXGIGatherDrawArgsPSO        (RenderSystem* RS);
        static ID3D12PipelineState* CreateVXGIGatherSubDRequestsPSO    (RenderSystem* RS);
        static ID3D12PipelineState* CreateVXGIProcessSubDRequestsPSO   (RenderSystem* RS);
    };
}


/**********************************************************************

Copyright (c) 2014-2021 Robert May

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
