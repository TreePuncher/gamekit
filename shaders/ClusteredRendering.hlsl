#include "common.hlsl"

struct Cluster
{
    float4 MinPoint;
    float4 MaxPoint;
};

RWStructuredBuffer<Cluster>     clusterBuffer   : register(u0); // in-out
RWTexture2D<uint4>              indexBuffer     : register(u1); // in-out
RWTexture1D<uint>               counters        : register(u2); // in-out
RWTexture2D<uint4>              debugBuffer     : register(u3); // in-out
Texture2D<float>                depthBuffer     : register(t0); // in

sampler BiLinear : register(s0); 
sampler NearestPoint : register(s1); // Nearest point

#define SharedMemorySize 32 * 32
#define NUMSLICES 24

groupshared uint localClusterIDs[SharedMemorySize];
groupshared uint localClusterIndexes[NUMSLICES];

groupshared uint uniqueClusters[NUMSLICES];
groupshared uint uniqueClusterCounter;

void CmpSwap(const uint const lhs, const uint rhs, const uint op)
{ 
    const uint LValue = localClusterIDs[lhs]; 
    const uint RValue = localClusterIDs[rhs];

    const uint V1 = op == 0 ? min(LValue, RValue) : max(LValue, RValue);
    const uint V2 = op == 0 ? max(LValue, RValue) : min(LValue, RValue);

    localClusterIDs[lhs] = V1;
    localClusterIDs[rhs] = V2;
}

void BitonicPass(const uint localThreadID, const int I, const int J)
{
    const uint swapMask = (1 << (J + 1)) - 1;
    const uint offset   = 1 << J;

    if((localThreadID & swapMask) < offset)
    {
        const uint op  = (localThreadID >> (I + 1)) & 0x01;
        CmpSwap(localThreadID, localThreadID + offset, op);
    }

    GroupMemoryBarrierWithGroupSync();
} 


// Pad extra elements with -1
void LocalBitonicSort(const uint localThreadID)
{
	for(int I = 0; I < log2(SharedMemorySize); I++)
        for(int J = I; J >= 0; J--)
        	BitonicPass(localThreadID, I, J);
} 


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

[numthreads(8, 1, 1)]
void ClearCounters(uint3 gid : SV_DISPATCHTHREADID)
{
    counters[gid.x] = 0;
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
    const uint X            = (clusterID >> 20) & 0xff;
    const uint Y            = (clusterID >> 10) & 0xff;
    const uint SliceIdx     = (clusterID >> 00) & 0xff;

    const float minZ = (GetSliceDepth(SliceIdx + 0) - MinZ);
    const float maxZ = (GetSliceDepth(SliceIdx + 1) - MinZ);

    const uint2 WH          = GetTextureWH(depthBuffer);
    const uint2 TileSize    = uint2(32, 32);
    const float2 TileWHDim  = (WH / TileSize);

    const uint2 Temp = uint2(X, Y);
    const float2 XY1 = float2(Temp.x, Temp.y) / TileWHDim; // {{ -1 -> 1 }, {1 -> -1}}
    const float2 XY2 = float2(Temp.x + 1, Temp.y + 1) / TileWHDim; // {{ -1 -> 1 },  {1 -> -1}}

    const float2 DC_Min = float2(-1.0f + 2 * XY1.x, 1.0f - 2 * XY1.y);//GetViewSpacePosition(XY1, minZ);
    const float2 DC_Max = float2(-1.0f + 2 * XY2.x, 1.0f - 2 * XY2.y);//GetViewSpacePosition(XY2, minZ);

    const float3 point1 = GetViewVector_VS2(XY1) * minZ;
    const float3 point2 = GetViewVector_VS2(XY2) * maxZ;

    const float3 VS_Min = point1;
    const float3 VS_Max = point2;

    float3 AABBCorners[8];
    AABBCorners[0] = mul(ViewI, float4(VS_Max.x, VS_Max.y, VS_Max.z, 1));
    AABBCorners[1] = mul(ViewI, float4(VS_Min, 1));  

    AABBCorners[2] = mul(ViewI, float4(VS_Min.x, VS_Max.yz, 1));
    AABBCorners[3] = mul(ViewI, float4(VS_Min.x, VS_Max.y, VS_Min.z, 1));
    AABBCorners[4] = mul(ViewI, float4(VS_Max.x, VS_Max.y, VS_Min.z, 1));

    AABBCorners[5] = mul(ViewI, float4(VS_Min.xy, VS_Max.z, 1)); 
    AABBCorners[6] = mul(ViewI, float4(VS_Max.x,  VS_Min.y, VS_Max.z, 1));
    AABBCorners[7] = mul(ViewI, float4(VS_Max.x, VS_Min.yz, 1)); 

    //float3 WS_Min = float3( 10000,  10000,  10000);
    //float3 WS_Max = float3( -10000,  -10000, -10000);

    float3 WS_Min = float3( -10000,  -10000, -10000);
    float3 WS_Max = float3( 10000,  10000,  10000);

    for(uint I = 0; I < 0; I++)
    {
        WS_Min = min(WS_Min, AABBCorners[I]);
        WS_Max = max(WS_Max, AABBCorners[I]);
    }

    Cluster cluster;
    cluster.MinPoint = float4(WS_Min, 0);
    cluster.MaxPoint = float4(WS_Max, 0);

    return cluster;
}


void GetUniqueClusters(const uint localThreadID)
{	
	if(localThreadID == 0)
	{
         const int x_0 = localClusterIDs[localThreadID];

         if(x_0 != -1)
         {                 
            uint index = -1;
            InterlockedAdd(uniqueClusterCounter, 1, index);
            
            uniqueClusters[index] = localClusterIDs[localThreadID];
         }
	}
	else
	{
    	const int x_0 = localClusterIDs[localThreadID];
	    const int x_1 = localClusterIDs[localThreadID - 1];         
        
         if(x_0 != x_1 && x_0 != -1)
         {                 
            uint index = -1;
            InterlockedAdd(uniqueClusterCounter, 1, index);
            
            uniqueClusters[index] = localClusterIDs[localThreadID];
         }    
	}    
    
    #if 0
    const uint count = uniqueClusterCounter;
    if(localThreadID >= count)
        uniqueClusters[localThreadID] = -4;
	
    #endif

	GroupMemoryBarrierWithGroupSync();
}


void CreateClusterEntries(const uint localThreadID)
{
    const uint count = uniqueClusterCounter;
    if(localThreadID < uniqueClusterCounter)
    {
		const uint clusterIdx               = clusterBuffer.IncrementCounter();
        localClusterIndexes[localThreadID]  = clusterIdx;
        clusterBuffer[clusterIdx]           = CreateCluster(uniqueClusters[localThreadID]);
    }

	GroupMemoryBarrierWithGroupSync();
}


void FindClusterIndex(const uint3 globalThreadID, const uint localClusterID)
{
    GroupMemoryBarrierWithGroupSync();

    for(uint I = 0; I < uniqueClusterCounter; I++)
    {                        
        const uint clusterId = uniqueClusters[I];

        if(localClusterID == clusterId)
        {   	
            GroupMemoryBarrierWithGroupSync();

            const uint clusterIndex         = localClusterIndexes[I];
            indexBuffer[globalThreadID.xy]  = uint4(clusterIndex, localClusterID, uniqueClusterCounter, 0);
			return;
        }
    }

}


[numthreads(32, 32, 1)]
void CreateClusters(uint3 globalThreadID : SV_DISPATCHTHREADID, uint localThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{       
    if(localThreadID == 0)
        uniqueClusterCounter = 0;
    
    const uint2 WH              = GetTextureWH(depthBuffer);
    const float2 UV             = float2(globalThreadID.xy) / WH;
    const float  depth          = depthBuffer.Load(uint3(globalThreadID.xy, 0)); // Normalized View Space Depth
    const float3 POS_VS         = GetViewSpacePosition(UV, depth);

    const uint slice            = GetSliceIdx(-POS_VS.z);
    const uint X                = groupID.x;
    const uint Y                = groupID.y;

    const uint localClusterID   = (depth != 1.0f && globalThreadID.x < WH.x && globalThreadID.y < WH.y) ? (X << 20 | Y << 10 | slice) : -1;

    localClusterIDs[localThreadID] = localClusterID;  

    GroupMemoryBarrierWithGroupSync();

    LocalBitonicSort(localThreadID);

    GroupMemoryBarrierWithGroupSync();

    GetUniqueClusters(localThreadID); 
    CreateClusterEntries(localThreadID);

    if(globalThreadID.x < WH.x && globalThreadID.y < WH.y)
	    FindClusterIndex(globalThreadID, localClusterID);

    debugBuffer[globalThreadID.xy] = uint4(localClusterIDs[localThreadID], slice, uniqueClusterCounter, localClusterID);
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