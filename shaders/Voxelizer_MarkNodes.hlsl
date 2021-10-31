#include "VXGI_common.hlsl"

cbuffer range : register(b0)
{
    uint sampleCount;
};

struct VoxelSample
{
    uint64_t    mortonID;
    float4      POS;    // Position + ???
    float4      ColorR; // Color + roughness
};


StructuredBuffer<VoxelSample>   voxelSampleBuffer   : register(t0);

AppendStructuredBuffer<uint>        dirtynodes      : register(u0);
RWStructuredBuffer<OctTreeNode>     octree          : register(u1);

[numthreads(1024, 1, 1)]
void MarkNodes(const uint3 threadID : SV_DispatchThreadID)
{
    const uint threadIdx = threadID.x;

    if (threadIdx >= sampleCount)
        return;

    const VoxelSample voxelSample   = voxelSampleBuffer[threadIdx];
    //const uint4 voxelcord           = MortonID2VoxelID(voxelSample.mortonID);
    if (voxelSample.POS.x >= VOLUMESIDE_LENGTH ||
        voxelSample.POS.y >= VOLUMESIDE_LENGTH ||
        voxelSample.POS.z >= VOLUMESIDE_LENGTH ||
        voxelSample.POS.x < 0 ||
        voxelSample.POS.y < 0 ||
        voxelSample.POS.z < 0) return;

    const uint4 voxelcord           = uint4(WS2VolumeCord(voxelSample.POS, float3(0, 0, 0), VOLUME_SIZE), MAX_DEPTH);

    const TraverseResult quearyResult = TraverseOctree(voxelcord, octree);

    if(quearyResult.flags == NODE_NOT_ALLOCATED ||
       quearyResult.flags == TRAVERSE_RESULT_CODES::NODE_EMPTY)
    {
        if (quearyResult.childIdx == -1)
            return;

        const uint childFlags = SetChildFlags(quearyResult.childIdx, CHILD_FLAGS::FILLED, 0);

        uint result = 0;
        InterlockedOr(
            octree[quearyResult.node].flags,
            SUBDIVISION_REQUEST | childFlags,
            result);

        if(!(result & SUBDIVISION_REQUEST))
            dirtynodes.Append(quearyResult.node);
    }
}


/**********************************************************************

Copyright (c) 2015 - 2021 Robert May

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
