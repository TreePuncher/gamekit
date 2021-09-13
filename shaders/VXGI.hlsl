#include "common.hlsl"
#include "VXGI_common.hlsl"

/************************************************************************************************/


sampler BiLinear        : register(s0);
sampler NearestPoint    : register(s1);

Texture2D<float> depth  : register(t0);

struct VoxelSample
{
    uint4   volumeID;
    float4  color;
};

RWStructuredBuffer<VoxelSample> voxelSampleBuffer   : register(u0);
RWStructuredBuffer<OctTreeNode> octree              : register(u1);
RWStructuredBuffer<uint>        freeList            : register(u2);


/************************************************************************************************/


void InitNode(uint idx, uint4 volumeCord)
{
    OctTreeNode n;
    n.data          = -1;
    n.flags         = CLEAR;
    n.volumeCord    = volumeCord;
    n.parent        = -1;
    n.pad           = 0;

    for (uint I = 0; I < 8; I++)
        n.children[I] = -1;

    octree[idx] = n;
}


/************************************************************************************************/


[numthreads(32, 32, 1)]
void Injection(uint3 threadID : SV_DispatchThreadID)
{
    uint Width, Height, _;
    depth.GetDimensions(0, Width, Height, _);

    if (threadID.x >= Width || threadID.y >= Height)
        return;

    const float2 UV             = 4.0f * float2(threadID.xy) / (float2(Width, Height)); // [0;1]
    const float depthSample     = depth.Load(uint3(4 * threadID.xy, 0));

    const float3 volumePosition = float3(0, 0, 0);
    const float3 volumeSize     = VOLUME_SIZE;
    const float3 pos_WS         = GetWorldSpacePosition(UV, depthSample);

    if (InsideVolume(pos_WS, volumePosition, volumeSize))
    {
        const uint3 volumeCord = WS2VolumeCord(pos_WS, volumePosition, volumeSize);

        VoxelSample v;
        v.volumeID  = uint4(volumeCord.x, volumeCord.y, volumeCord.z, MAX_DEPTH);
        v.color     = float4(5, 0, 5, 0);

        uint numStructs, _;
        voxelSampleBuffer.GetDimensions(numStructs, _);
        const uint  idx         = voxelSampleBuffer.IncrementCounter();

        if(idx < numStructs)
            voxelSampleBuffer[idx] = v;
    }
}


/************************************************************************************************/


void FlagForSubdivion(uint nodeIdx)
{
    InterlockedOr(octree[nodeIdx].flags, NODE_FLAGS::SUBDIVISION_REQUEST);
}


/************************************************************************************************/


[numthreads(1024, 1, 1)]
void GatherSubdivionRequests(uint3 threadID : SV_DispatchThreadID)
{
    uint requestCount, _;
    voxelSampleBuffer.GetDimensions(requestCount, _);

    if (threadID.x > requestCount)
        return;

    VoxelSample v = voxelSampleBuffer[threadID.x];
    const TraverseResult result = TraverseOctree(v.volumeID, octree);

    if (result.flags == TRAVERSE_RESULT_CODES::NODE_NOT_ALLOCATED)
        FlagForSubdivion(result.node);
}


/************************************************************************************************/


[numthreads(1024, 1, 1)]
void ProcessSubdivionRquests(uint3 threadID : SV_DispatchThreadID)
{
    const uint4 childVIDOffsets[] = {
        uint4(0, 0, 0, 0),
        uint4(1, 0, 0, 0),
        uint4(0, 0, 1, 0),
        uint4(1, 0, 1, 0),

        uint4(0, 1, 0, 0),
        uint4(1, 1, 0, 0),
        uint4(0, 1, 1, 0),
        uint4(1, 1, 1, 0)
    };

    const uint nodeIdx  = threadID.x;
    OctTreeNode node    = octree[nodeIdx];

    if ((node.flags & NODE_FLAGS::SUBDIVISION_REQUEST) && (node.flags & NODE_FLAGS::LEAF))
    {
        const uint4 VIDBase = uint4(node.volumeCord.xyz * 2, node.volumeCord.w + 1);

        for (uint childIdx = 0; childIdx < 8; childIdx++)
        {
            OctTreeNode childNode;

            for (uint I = 0; I < 8; I++)
                childNode.children[I] = -1;

            const uint4 VIDoffset = childVIDOffsets[childIdx];

            childNode.volumeCord    = VIDBase + VIDoffset;
            childNode.flags         = NODE_FLAGS::LEAF;
            childNode.data          = -1;
            childNode.parent        = nodeIdx;
            childNode.pad           = 0;

            // Publish node
            uint numStructs, _;
            octree.GetDimensions(numStructs, _);

            const uint idx = octree.IncrementCounter();

            if(idx < numStructs)
            {
                octree[idx]             = childNode;
                node.children[childIdx] = idx;
            }
        }

        node.flags          = NODE_FLAGS::BRANCH;
        octree[threadID.x]  = node;
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
