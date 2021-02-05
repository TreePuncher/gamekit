struct BVH_Node
{
    float4 MinPoint;
    float4 MaxPoint;

    uint Offset;
    uint Count; // max child count is 16

    uint Leaf;
};

struct PointLight
{
    float4 KI;	// Color + intensity in W
    float4 PR;	// XYZ + radius in W
};

cbuffer constants : register(b0)
{
    float4x4    iproj;
    float4x4    view;
    uint2       LightMapWidthHeight;
    uint        lightCount;

    uint        nodeCount;
    uint        inputOffset;
};

RWStructuredBuffer<BVH_Node> BVHNodes    : register(u0); // in-out
RWStructuredBuffer<uint>     lightLookup : register(u1); // in-out
StructuredBuffer<PointLight> pointLights : register(t0); // in

#define GROUPSIZE   1024
#define NODEMAXSIZE 32

groupshared uint2 scratchSpace[GROUPSIZE];

groupshared int minX;
groupshared int minY;
groupshared int minZ;

groupshared int maxX;
groupshared int maxY;
groupshared int maxZ;

#define ComponentSize 9
#define ComponentMask (1 << ComponentSize) - 1


/************************************************************************************************/


uint2 lt_MortonCode(const uint2 lhs, const uint2 rhs)
{
    return lhs[0] <= rhs[0] ? lhs : rhs;
}


/************************************************************************************************/


uint2 gt_MortonCode(const uint2 lhs, const uint2 rhs)
{
    return lhs[0] > rhs[0] ? lhs : rhs;
}


/************************************************************************************************/


void __CmpSwap(uint lhs, uint rhs, uint op)
{
    const uint2 LValue = scratchSpace[lhs];
    const uint2 RValue = scratchSpace[rhs];
	
    const uint2 V1 = op == 0 ? lt_MortonCode(LValue, RValue) : gt_MortonCode(LValue, RValue);
    const uint2 V2 = op == 0 ? gt_MortonCode(LValue, RValue) : lt_MortonCode(LValue, RValue);
	
    scratchSpace[lhs] = V1;
    scratchSpace[rhs] = V2;
}


/************************************************************************************************/


void BitonicPass(const uint localThreadID, const int I, const int J)
{
    const uint swapMask = (1 << (J + 1)) - 1;
    const uint offset   = 1 << J;

    if((localThreadID & swapMask) < offset)
    {
        const uint op  = (localThreadID >> (I + 1)) & 0x01;
        __CmpSwap(localThreadID, localThreadID + offset, op);
    }

    GroupMemoryBarrierWithGroupSync();
}


/************************************************************************************************/


// Pad extra elements with -1
void LocalBitonicSort(const uint localThreadID)
{
	for(int I = 0; I < 10; I++)
        for(int J = I; J >= 0; J--)
        	BitonicPass(localThreadID, I, J);
}


/************************************************************************************************/


uint CreateMortonCode(const uint3 XYZ)
{
    const uint X = uint(XYZ.x) & ComponentMask;
    const uint Y = uint(XYZ.y) & ComponentMask;
    const uint Z = uint(XYZ.z) & ComponentMask;

    uint mortonCode = 0;

    for(uint I = 0; I < ComponentSize; I++)
    {
        const uint x_bit = X & (1 << I);   
        const uint y_bit = Y & (1 << I);   
        const uint z_bit = Z & (1 << I);   

        const uint XYZ = x_bit << 2 | y_bit << 0 | z_bit << 1;
        //const uint XYZ = x_bit << 0 | y_bit << 1 | z_bit << 2;

        mortonCode |= XYZ << I * 3;
    }

    return mortonCode;
}

/************************************************************************************************/

[numthreads(GROUPSIZE, 1, 1)]
void CreateLightBVH_PHASE1(const uint threadID : SV_GroupIndex)
{
    if(threadID == 0)
    {
        minX = 1000000000;
        minY = 1000000000;
        minZ = 1000000000;

        maxX = -1000000000;
        maxY = -1000000000;
        maxZ = -1000000000;
    }

    GroupMemoryBarrierWithGroupSync();

    //  Create Sortables
    //      calculated Min Max for lights
    //      create based on normalized position

    //float3 range;
    //float3 offset;

        if(threadID == 0)
    {
        minX = 1000000000;
        minY = 1000000000;
        minZ = 1000000000;

        maxX = -1000000000;
        maxY = -1000000000;
        maxZ = -1000000000;
    }

    //  Create Sortables
    //      calculated Min Max for lights
    //      create based on normalized position

    //float3 range;
    //float3 offset;

	PointLight localLight;

	if(threadID < lightCount)
		localLight = pointLights[threadID];

    const float3 WS_pos = localLight.PR.xyz;
    const float3 VS_P = mul(view, float4(WS_pos, 1));

	GroupMemoryBarrierWithGroupSync();

	if(threadID < lightCount)
	{
		InterlockedMin(minX, floor(VS_P.x - localLight.PR.w));
		InterlockedMin(minY, floor(VS_P.y - localLight.PR.w));
		InterlockedMin(minZ, floor(VS_P.z - localLight.PR.w));

		InterlockedMax(maxX, ceil(VS_P.x + localLight.PR.w));
		InterlockedMax(maxY, ceil(VS_P.y + localLight.PR.w));
		InterlockedMax(maxZ, ceil(VS_P.z + localLight.PR.w));
	}

	GroupMemoryBarrierWithGroupSync();

	const int3 range   = float3(maxX - minX, maxY - minY, maxZ - minZ);
	const int3 offset  = -float3(minX, minY, minZ);

	const float3    normalizedPosition  = (VS_P + offset) / range;
	const uint      mortonCode          = CreateMortonCode(normalizedPosition * ComponentMask);

	scratchSpace[threadID]  = (threadID < lightCount) ? uint2(mortonCode, threadID) : uint2(-1, -1);

    // Sort Scratch Space
    LocalBitonicSort(threadID);

    GroupMemoryBarrierWithGroupSync();

    // Build BVH Tree
    // ...
    if(threadID * NODEMAXSIZE < lightCount)
    {
        // Determine MinMax for node
        // Slow, optimize this
        const uint nodeSize = (threadID + 1) * NODEMAXSIZE < lightCount ? NODEMAXSIZE : min(max(int(lightCount) - NODEMAXSIZE * threadID, 0), NODEMAXSIZE);
        float3 minXYZ = float3( 10000,  10000,  10000);
        float3 maxXYZ = float3(-10000, -10000, -10000);
        
        for(uint itr = 0; itr < nodeSize; itr++)
        {
            const uint lightID  = scratchSpace[(threadID * NODEMAXSIZE) + itr].y;
            const float4 PR     = pointLights[lightID].PR;
            const float R       = PR.w;
            
            const float3 VS_P = mul(view, float4(PR.xyz, 1));

            minXYZ = min(VS_P - R, minXYZ);
            maxXYZ = max(VS_P + R, maxXYZ);

            //minXYZ = min(PR.xyz - R, minXYZ);
            //maxXYZ = max(PR.xyz + R, maxXYZ);
        }

        BVH_Node node;
        node.MinPoint   = float4(minXYZ, 0);
        node.MaxPoint   = float4(maxXYZ, 0);

        node.Offset = threadID * NODEMAXSIZE;
        node.Count  = nodeSize;

        node.Leaf   = 1;

        BVHNodes[threadID] = node;
    }

    if(threadID < lightCount)
        lightLookup[threadID] = scratchSpace[threadID].y;
}

[numthreads(GROUPSIZE, 1, 1)]
void CreateLightBVH_PHASE2(const uint threadID : SV_GroupIndex)
{
    const uint OutID = inputOffset + nodeCount + threadID;

    if(threadID * NODEMAXSIZE < nodeCount)
    {
        const uint count    = (threadID + 1) * NODEMAXSIZE < nodeCount ? NODEMAXSIZE : nodeCount % NODEMAXSIZE;
        const uint offset   = inputOffset + threadID * NODEMAXSIZE;

        float3 minXYZ = float3( 10000,  10000,  10000);
        float3 maxXYZ = float3(-10000, -10000, -10000);

        for(uint itr = 0; itr < count; itr++)
        {
            const BVH_Node node = BVHNodes[offset + itr];

            minXYZ = min(node.MinPoint, minXYZ);
            maxXYZ = max(node.MinPoint, maxXYZ);
            minXYZ = min(node.MaxPoint, maxXYZ);
            maxXYZ = max(node.MaxPoint, maxXYZ);
        }

        BVH_Node node;
        node.MinPoint   = float4(minXYZ, 0);
        node.MaxPoint   = float4(maxXYZ, 0);

        node.Offset = (count != 1) ? offset : BVHNodes[offset].Offset;
        node.Count  = (count != 1) ? count : BVHNodes[offset].Count;
        node.Leaf   = (count != 1) ? 0 : BVHNodes[offset].Leaf;

        BVHNodes[OutID] = node;
    }
}


/**********************************************************************

Copyright (c) 2021 Robert May

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