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
RWStructuredBuffer<uint>        octreeCounters      : register(u2);


/************************************************************************************************/


void InitNode(uint idx, uint4 volumeCord)
{
    OctTreeNode n;
    n.flags         = CLEAR;
    n.parent        = -1;
    n.children      = -1;
    n.padding       = -1;

    octree[idx] = n;
}


/************************************************************************************************/

/************************************************************************************************/


void FlagForSubdivion(uint nodeIdx, uint childIdx)
{
    InterlockedOr(
        octree[nodeIdx].flags,
        NODE_FLAGS::SUBDIVISION_REQUEST |
        SetChildFlags(childIdx, CHILD_FLAGS::FILLED));
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

        const TraverseResult result = TraverseOctree(v.volumeID, octree);

        if (result.flags != TRAVERSE_RESULT_CODES::NODE_FOUND)
        {
            uint numStructs, _;
            voxelSampleBuffer.GetDimensions(numStructs, _);

            const uint idx = voxelSampleBuffer.IncrementCounter();

            if (idx < numStructs)
                voxelSampleBuffer[idx] = v;
        }
    }
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
    const uint childIdx = GetChildIdxFromChildOffset(uint3(v.volumeID.x % 2, v.volumeID.y % 2, v.volumeID.z % 2));

    if (result.flags == TRAVERSE_RESULT_CODES::NODE_NOT_ALLOCATED)
        FlagForSubdivion(result.node, childIdx);
    else if(result.flags == TRAVERSE_RESULT_CODES::NODE_EMPTY)
        InterlockedOr(
            octree[result.node].flags,
            SetChildFlags(result.childIdx, CHILD_FLAGS::FILLED));
}


/************************************************************************************************/


[numthreads(1024, 1, 1)]
void ProcessSubdivionRquests(uint3 threadID : SV_DispatchThreadID)
{
    const uint nodeIdx  = threadID.x;
    OctTreeNode node    = octree[nodeIdx];

    if ((node.flags & NODE_FLAGS::SUBDIVISION_REQUEST) && (node.flags & NODE_FLAGS::LEAF))
    {
        uint childBlock = -1;
        InterlockedAdd(octreeCounters[0], 8, childBlock);

        for (uint childIdx = 0; childIdx < 8; childIdx++)
        {
            OctTreeNode childNode;

            childNode.flags     = NODE_FLAGS::LEAF;
            childNode.parent    = nodeIdx;
            childNode.padding   = -1;
            childNode.children  = -1;

            // Publish node
            uint numStructs, _;
            octree.GetDimensions(numStructs, _);

            if(childBlock + childIdx < numStructs)
                octree[childBlock + childIdx] = childNode;
        }

        node.children   = childBlock;
        node.flags      = (node.flags | NODE_FLAGS::BRANCH) & ~(NODE_FLAGS::LEAF | NODE_FLAGS::SUBDIVISION_REQUEST);
        octree[nodeIdx] = node;
    }
}


/************************************************************************************************/


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
