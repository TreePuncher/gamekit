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
    constexpr PSOHandle VXGI_FREENODES                  = PSOHandle(GetTypeGUID(VXGI_FREENODES));
    constexpr PSOHandle VXGI_CLEANUPVOXELVOLUMES        = PSOHandle(GetTypeGUID(VXGI_CLEANUPVOXELVOLUMES));
    constexpr PSOHandle VXGI_SAMPLEINJECTION            = PSOHandle(GetTypeGUID(VXGI_SAMPLEINJECTION));
    constexpr PSOHandle VXGI_DRAWVOLUMEVISUALIZATION    = PSOHandle(GetTypeGUID(VXGI_DRAWVOLUMEVISUALIZATION));
    constexpr PSOHandle VXGI_GATHERARGS1                = PSOHandle(GetTypeGUID(VXGI_GATHERARGS1));
    constexpr PSOHandle VXGI_GATHERARGS2                = PSOHandle(GetTypeGUID(VXGI_GATHERARGS2));
    constexpr PSOHandle VXGI_GATHERSUBDIVISIONREQUESTS  = PSOHandle(GetTypeGUID(VXGI_GATHERSUBDIVISIONREQUESTS));
    constexpr PSOHandle VXGI_PROCESSSUBDREQUESTS        = PSOHandle(GetTypeGUID(VXGI_PROCESSSUBDREQUESTS));


    /************************************************************************************************/


    struct UpdateVoxelVolume
    {
        ReserveConstantBufferFunction   reserveCB;
        CameraHandle                    camera;

        FrameResourceHandle     depthTarget;
        FrameResourceHandle     voxelBuffer;
        FrameResourceHandle     counters;
        FrameResourceHandle     octree;
        FrameResourceHandle     indirectArgs;
        FrameResourceHandle     freeList;
    };


    /************************************************************************************************/


    struct DEBUGVIS_VoxelVolume
    {
        ReserveConstantBufferFunction   reserveCB;
        CameraHandle                    camera;

        FrameResourceHandle     depthTarget;
        FrameResourceHandle     indirectArgs;
        FrameResourceHandle     octree;

        FrameResourceHandle     counter;
        FrameResourceHandle     accumlator;
    };

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
                OITPass&                        target,
			    ReserveConstantBufferFunction   reserveCB,
			    iAllocator*                     allocator);

    private:

        RenderSystem& renderSystem;

        ResourceHandle voxelBuffer;
        ResourceHandle octreeBuffer;
        ResourceHandle freeList;

        IndirectLayout dispatch;
        IndirectLayout gather;
        IndirectLayout draw;

        RootSignature testSignature;
        RootSignature gatherSignature;
        RootSignature dispatchSignature;

        static ID3D12PipelineState* CreateVXGI_InitOctree              (RenderSystem* RS);
        static ID3D12PipelineState* CreateInjectVoxelSamplesPSO        (RenderSystem* RS);
        static ID3D12PipelineState* CreateUpdateVoxelVolumesPSO        (RenderSystem* RS);
        static ID3D12PipelineState* CreateReleaseVoxelNodesPSO         (RenderSystem* RS);
        static ID3D12PipelineState* CreateUpdateVolumeVisualizationPSO (RenderSystem* RS);
        static ID3D12PipelineState* CreateVXGIGatherArgs1PSO           (RenderSystem* RS);
        static ID3D12PipelineState* CreateVXGIGatherArgs2PSO           (RenderSystem* RS);
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
