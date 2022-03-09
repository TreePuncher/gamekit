#include "common.hlsl"

struct Cluster
{
    float4 MinPoint;
    float4 MaxPoint;
};

cbuffer Constants : register(b1)
{
    float4x4 IProj;
};

RWTexture2D<uint>               indexBuffer     : register(u1); // in-out
RWStructuredBuffer<uint>        counters        : register(u2); // in-out
Texture2D<float>                depthBuffer     : register(t0); // in

sampler BiLinear : register(s0); 
sampler NearestPoint : register(s1); // Nearest point

#define SharedMemorySize 32 * 32
#define NUMSLICES 24

uint2 GetTextureWH(Texture2D<float> texture)
{
    uint width;
    uint height;
    uint numLevels;
    texture.GetDimensions(0, width, height, numLevels);

    return uint2(width, height);
}


#define NEARZ 0.1f
#define MAXZ 10000.0f


[numthreads(128, 1, 1)]
void ClearCounters(uint threadID : SV_GroupIndex)
{
    counters[threadID] = 0;
}

uint GetSliceIdx(float z)
{            
    const float numSlices   = NUMSLICES;                                      
    const float MinOverMax  = MaxZ / MinZ;
    const float LogMoM      = log(MinOverMax);

    return (log(z) * numSlices / LogMoM) - (numSlices * log(MinZ) / LogMoM);
}


float GetSliceDepth(float slice)
{
    const float numSlices = NUMSLICES;                                      
    return MinZ * pow(MaxZ / MinZ, floor(slice) / numSlices);
}

Cluster CreateCluster(const uint clusterID)
{
    const uint2 XY          = uint2((clusterID >> 16) & 0xff, (clusterID >> 8) & 0xff);
    const uint SliceIdx     = clusterID & 0xff;

    const float minZ        = GetSliceDepth(SliceIdx + 0) / MaxZ;
    const float maxZ        = GetSliceDepth(SliceIdx + 1) / MaxZ;

    const float2 WH         = GetTextureWH(depthBuffer);
    const float2 TileSize   = float2(32, 32);
    const float2 TileWHDim  = ceil(WH / TileSize);

    const float2 leftPoint  = (float2(XY + 0) / TileWHDim);
    const float2 rightPoint = (float2(XY + 1) / TileWHDim);

    const float3 MinPointNear   = GetViewSpacePosition(leftPoint, minZ);
    const float3 MinPointFar    = GetViewSpacePosition(leftPoint, maxZ);

    const float3 MaxPointNear   = GetViewSpacePosition(rightPoint, minZ);
    const float3 MaxPointFar    = GetViewSpacePosition(rightPoint, maxZ);

    const float3 ClusterMin =
        min(
            min(MinPointNear, MinPointFar),
            min(MaxPointNear, MaxPointFar));

    const float3 ClusterMax =
        max(
            max(MinPointNear, MinPointFar),
            max(MaxPointNear, MaxPointFar));

    Cluster cluster;
    cluster.MinPoint = float4(ClusterMin, asfloat(0));
    cluster.MaxPoint = float4(ClusterMax, asfloat(clusterID));

    return cluster;
}


groupshared uint sliceBitfield;
groupshared uint clusterIndexes[24];

RWStructuredBuffer<Cluster> clusters : register(u0); // in-out

[numthreads(32, 32, 1)]
void CreateClusters(
    const uint3     globalThreadID  : SV_DISPATCHTHREADID, 
    const uint      localThreadID   : SV_GroupIndex, 
    const uint3     groupID         : SV_GroupID)
{       
    if(localThreadID == 0)
        sliceBitfield = 0;

    if(localThreadID < 24)
        clusterIndexes[localThreadID] = -1;

    GroupMemoryBarrierWithGroupSync();

    const float2    WH      = GetTextureWH(depthBuffer);
    const float2    UV      = float2(globalThreadID.xy) / WH;
    const float     depth   = depthBuffer.Load(uint3(globalThreadID.xy, 0)); // Normalized View Space Depth

    const uint slice            = GetSliceIdx(depth * MaxZ);
    const uint X                = groupID.x;
    const uint Y                = groupID.y;
    const uint sliceBitID       = 0x01 << slice;

    GroupMemoryBarrierWithGroupSync();

    InterlockedOr(sliceBitfield, sliceBitID);

    GroupMemoryBarrierWithGroupSync();

    if(localThreadID < 24 && (sliceBitfield & (0x01 << localThreadID)))
    {
        const uint idx          = clusters.IncrementCounter();
        const uint clusterID    = ((X << 16) | (Y << 8) | localThreadID);

        clusterIndexes[localThreadID]   = idx;
        clusters[idx]                   = CreateCluster(clusterID);
    }

    GroupMemoryBarrierWithGroupSync();

    if( globalThreadID.x < WH.x && globalThreadID.y < WH.y)
            indexBuffer[globalThreadID.xy] = clusterIndexes[slice];
}

/**********************************************************************

Copyright (c) 2015 - 2020 Robert May

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
