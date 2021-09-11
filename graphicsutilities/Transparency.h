#pragma once
#include "FrameGraph.h"
#include "graphics.h"
#include "Scene.h"

namespace FlexKit
{   /************************************************************************************************/


    constexpr PSOHandle OITBLEND        = PSOHandle(GetTypeGUID(OITBLEND));
    constexpr PSOHandle OITDRAW         = PSOHandle(GetTypeGUID(OITDRAW));
    constexpr PSOHandle OITDRAWANIMATED = PSOHandle(GetTypeGUID(OITDRAWANIMATED));


    /************************************************************************************************/


    struct OITPass
	{
        ReserveConstantBufferFunction       reserveCB;

        UpdateTaskTyped<GetPVSTaskData>&    PVS;

        CameraHandle            camera;
        FrameResourceHandle     accumalatorObject;
        FrameResourceHandle     counterObject;
        FrameResourceHandle     depthTarget;
	};


    /************************************************************************************************/


    struct OITBlend
	{
        FrameResourceHandle     renderTargetObject;
        FrameResourceHandle     accumalatorObject;
        FrameResourceHandle     counterObject;
        FrameResourceHandle     depthTarget;
	};


    /************************************************************************************************/


    class Transparency
    {
    public:
        Transparency(RenderSystem&);

        OITPass& OIT_WB_Pass(
		        UpdateDispatcher&               dispatcher,
		        FrameGraph&                     frameGraph,
                GatherPassesTask&               passes,
                CameraHandle                    camera,
		        ResourceHandle                  depthTarget,
		        ReserveConstantBufferFunction   reserveCB,
		        iAllocator*                     allocator);

        OITBlend& OIT_WB_Blend(
		        UpdateDispatcher&               dispatcher,
		        FrameGraph&                     frameGraph,
                OITPass&                        OITPass,
                FrameResourceHandle             renderTarget,
		        iAllocator*                     allocator);
        
        static ID3D12PipelineState* CreateOITBlendPSO                  (RenderSystem* RS);
        static ID3D12PipelineState* CreateOITDrawPSO                   (RenderSystem* RS);
        static ID3D12PipelineState* CreateOITDrawAnimatedPSO           (RenderSystem* RS);
    };


}   /************************************************************************************************/


/**********************************************************************

Copyright (c) 2016-2021 Robert May

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
