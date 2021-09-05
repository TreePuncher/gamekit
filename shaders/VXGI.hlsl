#include "common.hlsl"

sampler BiLinear        : register(s0);
sampler NearestPoint    : register(s1);

Texture2D<float> depth  : register(t0);

struct VoxelSample
{
    uint4   volumeID;
    float4  color;
};

struct OctTreeNode
{
    uint    nodes[8];
    uint4   volumeCord;
    uint    data;
    uint    flags;
    uint    pad[2];
};

enum NODE_FLAGS
{
    CLEAR               = 0,
    SUBDIVISION_REQUEST = 1 << 0,
    UNITIALISED         = 1 << 1,
    LEAF                = 1 << 2,
};

RWStructuredBuffer<VoxelSample> voxelSampleBuffer   : register(u0);
RWStructuredBuffer<OctTreeNode> octree              : register(u1);

void InitNode(uint idx, uint4 volumeCord)
{
    OctTreeNode n;
    n.data          = -1;
    n.flags         = CLEAR;
    n.volumeCord    = volumeCord;
    n.pad[0]        = 0;
    n.pad[1]        = 0;

    for (uint I = 0; I < 8; I++)
        n.nodes[I] = -1;

    octree[idx] = n;
}

#define VOLUMESIDE_LENGTH   64
#define VOLUME_SIZE         uint3(VOLUMESIDE_LENGTH, VOLUMESIDE_LENGTH, VOLUMESIDE_LENGTH)
#define MAX_DEPTH           9
#define VOLUME_RESOLUTION   float3(1 << MAX_DEPTH, 1 << MAX_DEPTH, 1 << MAX_DEPTH)

struct OctreeNodeVolume
{
    float3 min;
    float3 max;
};

float3 GetVoxelPoint(uint4 volumeID)
{
    const uint      depth           = volumeID.w;
    const float     voxelSideLength = float(VOLUMESIDE_LENGTH) / float(1 << depth);
    const float3    xyz             = voxelSideLength * volumeID.xyz;

    return xyz;
}

OctreeNodeVolume GetVolume(uint3 xyz, uint depth)
{
    const float     voxelSideLength = float(VOLUMESIDE_LENGTH) / float(1 << depth);
    const float3    volumeDIM       = float3(voxelSideLength, voxelSideLength, voxelSideLength);
    const float3    voxelSize       = float3(volumeDIM);

    const float3 min    = voxelSize * xyz;
    const float3 max    = min + voxelSize;

    OctreeNodeVolume outVolume;
    outVolume.min   = min;
    outVolume.max   = max;

    return outVolume;
}

bool IsInVolume(float3 voxelPoint, OctreeNodeVolume volume)
{
    return
        (   volume.min.x <= voxelPoint.x &&
            volume.min.y <= voxelPoint.y &&
            volume.min.z <= voxelPoint.z &&

            volume.max.x >= voxelPoint.x &&
            volume.max.y >= voxelPoint.y &&
            volume.max.z >= voxelPoint.z);
}

float3 VolumeCord2WS(const uint3 cord, const float3 volumePOS_ws, const float3 volumeSize)
{
    return float3(cord) / VOLUME_RESOLUTION * volumeSize + volumePOS_ws;
}

uint3 WS2VolumeCord(const float3 pos_WS, const float3 volumePOS_ws, const float3 volumeSize)
{
    const float3 UVW = ((pos_WS - volumePOS_ws) / volumeSize) * VOLUME_RESOLUTION;

    return uint3(UVW);
}


bool OnScreen(float2 DC)
{
    return ((DC.x >= -1.0f && DC.x <= 1.0f) &&
            (DC.y >= -1.0f && DC.y <= 1.0f));
}


bool InsideVolume(float3 pos_WT, const float3 volumePOS_ws, const float3 volumeSize)
{
    return (
        pos_WT.x >= volumePOS_ws.x &&
        pos_WT.y >= volumePOS_ws.y &&
        pos_WT.z >= volumePOS_ws.z &&

        pos_WT.x <= volumePOS_ws.x + volumeSize.x &&
        pos_WT.y <= volumePOS_ws.y + volumeSize.y &&
        pos_WT.z <= volumePOS_ws.z + volumeSize.z );
}

/*
[numthreads(8, 8, 8)]
void UpdateVoxelVolumes(uint3 threadID : SV_DispatchThreadID)
{
    const float3 volumePosition     = float3(0, 0, 0);
    const float3 voxelPOS_ws        = VolumeCord2WS(threadID, volumePosition, VOLUME_SIZE);
    const float4 temp               = mul(PV, float4(voxelPOS_ws, 1));
    const float3 deviceCord         = temp.xyz / temp.w;


    if (OnScreen(deviceCord.xy))
    {
        const float3 cord = float3(deviceCord.x / 2.0f + 0.5f, 1 - (0.5f + deviceCord.y / 2.0f), 0);

        if (cord.y < 0.0f || cord.y > 1.0f ||
            cord.x < 0.0f || cord.x > 1.0f)
            return;

        const float d = depth.Load(cord * uint3(1920, 1080, 1)) * MaxZ;
        const float l = length(voxelPOS_ws - CameraPOS.xyz);

        const bool cull = abs(l - d) > 0.8f;

        const float4 element = primary[threadID];
    }
}
*/

[numthreads(32, 32, 1)]
void Injection(uint3 threadID : SV_DispatchThreadID)
{
    uint Width, Height, _;
    depth.GetDimensions(0, Width, Height, _);

    if (threadID.x >= Width || threadID.y >= Height)
        return;

    const float2 UV = 4.0f * float2(threadID.xy) / (float2(Width, Height)); // [0;1]
    const float depthSample = depth.Load(uint3(4 * threadID.xy, 0));

    const float3 volumePosition = float3(0, 0, 0);
    const float3 volumeSize     = VOLUME_SIZE;
    const float3 pos_WS         = GetWorldSpacePosition(UV, depthSample);

    if (InsideVolume(pos_WS, volumePosition, volumeSize))
    {
        const uint3 volumeCord = WS2VolumeCord(pos_WS, volumePosition, volumeSize);

        VoxelSample v;
        v.volumeID  = uint4(volumeCord.x, volumeCord.y, volumeCord.z, MAX_DEPTH);
        v.color     = float4(5, 0, 5, 0);

        const uint  idx         = voxelSampleBuffer.IncrementCounter();
        voxelSampleBuffer[idx] = v;
    }
}


struct TraverseResult
{
    uint node;
    uint flags;
};

enum TRAVERSE_RESULT_CODES
{
    ERROR = -1,
    NODE_NOT_ALLOCATED = -2,
    NODE_FOUND
};


TraverseResult TraverseOctree(const uint4 volumeID)
{
    const float3    voxelPoint = GetVoxelPoint(volumeID);
    uint            nodeID     = 0;

    for (uint depth = 0; depth <= MAX_DEPTH; depth++)
    {
        OctTreeNode             n           = octree[nodeID];
        const OctreeNodeVolume  volume      = GetVolume(n.volumeCord.xyz, n.volumeCord.w);
        const bool              volumeCheck = IsInVolume(voxelPoint, volume);

        if(volumeCheck)
        {
            if (n.volumeCord.w == volumeID.w)
            {   // Node Found
                TraverseResult res;
                res.node    = nodeID;
                res.flags   = TRAVERSE_RESULT_CODES::NODE_FOUND;

                return res;
            }
            else
            {
                if ((n.flags & NODE_FLAGS::LEAF) && volumeCheck)
                {   // Closest node is leaf, node not allocated
                    TraverseResult res;
                    res.node    = nodeID;
                    res.flags   = TRAVERSE_RESULT_CODES::NODE_NOT_ALLOCATED;

                    return res;
                }
                else
                {   // traverse to child
                    const uint4 childCords = uint4(n.volumeCord.xyz * 2, depth + 1);

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

                    for (uint childID = 0; childID < 8; childID++)
                    {
                        const uint4 childVID            = childCords + childVIDOffsets[childID];
                        const OctreeNodeVolume volume   = GetVolume(childVID.xyz, childVID.w);

                        if (IsInVolume(voxelPoint, volume))
                        {
                            nodeID = n.nodes[childID];
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            TraverseResult res;
            res.node    = -1;
            res.flags   = TRAVERSE_RESULT_CODES::ERROR;

            return res;
        }
    }

    TraverseResult res;
    res.node = -1;
    res.flags = TRAVERSE_RESULT_CODES::ERROR;

    return res;
}


void FlagForSubdivion(uint nodeIdx)
{
    InterlockedOr(octree[nodeIdx].flags, NODE_FLAGS::SUBDIVISION_REQUEST);
}

[numthreads(1024, 1, 1)]
void GatherSubdivionRequests(uint3 threadID : SV_DispatchThreadID)
{
    uint requestCount, _;
    voxelSampleBuffer.GetDimensions(requestCount, _);

    if (threadID.x > requestCount)
        return;

    VoxelSample v = voxelSampleBuffer[threadID.x];
    const TraverseResult result = TraverseOctree(v.volumeID);

    if (result.flags == TRAVERSE_RESULT_CODES::NODE_NOT_ALLOCATED)
        FlagForSubdivion(result.node);
}

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

    OctTreeNode node = octree[threadID.x];

    if ((node.flags & NODE_FLAGS::SUBDIVISION_REQUEST) && (node.flags & NODE_FLAGS::LEAF))
    {
        const uint4 VIDBase = uint4(node.volumeCord.xyz * 2, node.volumeCord.w + 1);

        for (uint childIdx = 0; childIdx < 8; childIdx++)
        {
            OctTreeNode childNode;

            for (uint I = 0; I < 8; I++)
                childNode.nodes[I] = -1;

            const uint4 VIDoffset = childVIDOffsets[childIdx];

            childNode.volumeCord    = VIDBase + VIDoffset;
            childNode.flags         = NODE_FLAGS::LEAF;
            childNode.data          = -1;
            childNode.pad[0]        = 0;
            childNode.pad[1]        = 0;

            // Publish node
            const uint idx          = octree.IncrementCounter();
            octree[idx]             = childNode;
            node.nodes[childIdx]    = idx;
        }

        node.flags          = CLEAR;
        octree[threadID.x]  = node;
    }
}

/*
[numthreads(1024, 1, 1)]
void ProcessSubdivionRquests(uint3 threadID : SV_DispatchThreadID)
{
    OctTreeNode n = octree[threadID.x];

    if (n.flags & NODE_FLAGS::SUBDIVISION_REQUEST)
    {
    }
}
*/

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
