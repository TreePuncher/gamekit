cbuffer constants : register(b0)
{
	float4x4    View;
	uint        rootNode;
}

cbuffer counters : register(b1)
{
	uint clusterCount;
}

struct Cluster
{
	float4 MinPoint;
	float4 MaxPoint;
};

struct BVH_Node
{
	float4 MinPoint;
	float4 MaxPoint;

	uint Offset;
	uint Count; // max child count is 16

	uint Leaf;
	uint pad;
};

struct AABB
{
	float3 MinPoint;
	float3 MaxPoint;
};

struct PointLight
{
	float4	KI;	// Color + intensity in W
	float4	PR;	// XYZ + radius in W
	float4	DA; // Direction attentuation
	uint4	TypeExtra;
};

#define NODEMAXSIZE 32

					RWStructuredBuffer<uint2>       lightList           : register(u0); // in-out
globallycoherent    RWStructuredBuffer<uint>        lightListBuffer     : register(u1); // in-out
globallycoherent    RWStructuredBuffer<uint>        lightListCounter    : register(u2); // in-out

StructuredBuffer<BVH_Node>    BVHNodes          : register(t0); 
StructuredBuffer<uint>        LightLookup       : register(t1); 
StructuredBuffer<PointLight>  PointLights       : register(t2); 
StructuredBuffer<Cluster>     ClusterBuffer     : register(t3); 


AABB GetClusterAABB(in Cluster C)
{
	AABB aabb;
	aabb.MinPoint = C.MinPoint;
	aabb.MaxPoint = C.MaxPoint;

	return aabb;
}


AABB GetBVHAABB(in BVH_Node n)
{
	AABB aabb;
	aabb.MinPoint = n.MinPoint;
	aabb.MaxPoint = n.MaxPoint;

	return aabb;
}


AABB GetPointLightAABB(in uint lightID)
{
	PointLight pointLight = PointLights[lightID];
	const float3 pos_VS = mul(View, float4(pointLight.PR.xyz, 1));

	AABB aabb;
	aabb.MinPoint = pos_VS - pointLight.PR.w;
	aabb.MaxPoint = pos_VS + pointLight.PR.w;

	return aabb;
}


float4 GetPointLightBS(in uint lightID)
{
	PointLight pointLight = PointLights[lightID];
	const float4 pos_WS = float4(pointLight.PR.xyz, 1);
	const float3 pos_VS = mul(View, pos_WS);

	return float4(pos_VS, pointLight.PR.w);
}


bool CompareBSToAABB(in const float4 BS, in const AABB aabb)
{
	const float radiusSquared = BS.w * BS.w;
	const float3 closestPoint = clamp(BS.xyz, aabb.MinPoint, aabb.MaxPoint);

	const float3 ab  = closestPoint - BS.xyz;
	const float3 ab2 = (ab * ab);
	const float d2  = ab2.x + ab2.y + ab2.z; 

	return  d2 < radiusSquared;
}

// https://bartwronski.com/2017/04/13/cull-that-cone/
bool TestConeVsSphere(in float3 origin, in float3 forward, in float size, in float angle, in float4 testSphere)
{
	const float3	V		= testSphere.xyz - origin;
	const float		VlenSq	= dot(V, V);
	const float		V1len	= dot(V, forward);
	const float		distanceClosestPoint = cos(angle) * sqrt(VlenSq - V1len * V1len) - V1len * sin(angle);

	const bool	angleCull	= distanceClosestPoint > testSphere.w;
	const bool	frontCull	= V1len > testSphere.w + size;
	const bool	backCull	= V1len < -testSphere.w;
	return !(angleCull || frontCull || backCull);
}

bool CompareLightToAABB(in const uint lightID, in const AABB aabb)
{
	uint type = PointLights[lightID].TypeExtra[0];
	switch (type)
	{
	case 0:// PointLight
	{
		const float4 BS = GetPointLightBS(lightID);
		return CompareBSToAABB(BS, aabb);
	}
	case 1:// SpotLight
	{
		const float3 center = (aabb.MinPoint + aabb.MaxPoint) / 2;
		const float  r		= length(aabb.MaxPoint - aabb.MinPoint) / 2;

		const float4 posr	= PointLights[lightID].PR;
		const float4 da		= PointLights[lightID].DA;

		const float3 pos_VS = mul(View, float4(posr.xyz, 1));
		const float3 d_VS	= mul(View, float4(da.xyz, 0));

		return TestConeVsSphere(pos_VS, -d_VS, posr.w, da.w, float4(center, r));
	}
	}

	return true;
}


bool CompareAABBToAABB(in AABB A, in AABB B)
{
	return  (A.MinPoint.x <= B.MaxPoint.x && B.MinPoint.x <= A.MaxPoint.x) &&
			(A.MinPoint.y <= B.MaxPoint.y && B.MinPoint.y <= A.MaxPoint.y) &&
			(A.MinPoint.z <= B.MaxPoint.z && B.MinPoint.z <= A.MaxPoint.z);
}


bool IsLeafNode(in const BVH_Node node)
{
	return node.Leaf == 1;
}


uint GetFirstChild(in const BVH_Node node)
{
	return node.Offset;
}


uint GetChildCount(in const BVH_Node node)
{
	return node.Count;
}

groupshared uint stack[512];
groupshared  int stackSize;
groupshared uint parentIdx;

#define lightBufferSize 256

groupshared uint lights[lightBufferSize];
groupshared uint lightCount;
groupshared uint offset;

void InitStack()
{
	stackSize   = 0;
	lightCount  = 0;
	parentIdx   = rootNode;
}

void PushLight(in const uint lightID)
{
	uint i;
	InterlockedAdd(lightCount, 1, i);

	lights[i] = lightID;
}

void PushNode(in const uint nodeID)
{
	uint i;
	InterlockedAdd(stackSize, 1, i);
	stack[i] = nodeID;
}

uint PopNode()
{
	InterlockedAdd(stackSize, -1);

	return stack[stackSize];
}

uint GetClusterIdx(in const uint clusterID)
{
	const uint X            = (clusterID >> 16) & 0xf;
	const uint Y            = (clusterID >> 8)  & 0xf;
	const uint SliceIdx     = (clusterID >> 00) & 0xf;

	const uint rowPitch     = 60;
	const uint slicePitch   = rowPitch * 34;

	return X + Y * rowPitch + SliceIdx * slicePitch;
}

[numthreads(32, 1, 1)]
void CreateClustersLightLists(const uint threadID : SV_GroupIndex, const uint3 groupID : SV_GroupID)
{
	const uint groupIdx = groupID.x + groupID.y * 1024;

	if(groupIdx < clusterCount)
	{
		Cluster localCluster        = ClusterBuffer[groupIdx];
		const AABB aabb             = GetClusterAABB(localCluster);

		if(threadID == 0)
		{
			InitStack();
			PushNode(rootNode);
		}

		GroupMemoryBarrierWithGroupSync();

		do
		{
			const BVH_Node parent = BVHNodes[parentIdx];
			const uint childCount = GetChildCount(parent);

			if(threadID < childCount)
			{
				if(IsLeafNode(parent))
				{
					const uint idx      = GetFirstChild(parent) + threadID;
					const uint lightID  = LightLookup[idx];

					if (CompareLightToAABB(lightID, aabb))
						PushLight(lightID);
				}
				else
				{ 
					const uint idx          = GetFirstChild(parent) + threadID; 
					const AABB b            = GetBVHAABB(BVHNodes[idx]);

					if(CompareAABBToAABB(aabb, b))
						PushNode(idx);
				}

				GroupMemoryBarrierWithGroupSync();
			}

			GroupMemoryBarrierWithGroupSync();

			if(threadID == 0)
				parentIdx = PopNode();

			GroupMemoryBarrierWithGroupSync();
		}while(parentIdx != rootNode);

		if(lightCount > 0)
		{
			if(threadID == 0)
			{
				InterlockedAdd(lightListCounter[0], lightCount, offset);
				lightList[groupIdx]     = uint2(lightCount, offset);
			}

			GroupMemoryBarrierWithGroupSync();

			// Move Light List to global memory
			const uint end = ceil(float(lightCount) / NODEMAXSIZE);
			for(uint I = 0; I < end; I++)
			{
				const uint idx = threadID + (I * NODEMAXSIZE);
				if(idx < lightCount)
					lightListBuffer[idx + offset] = lights[idx];
			}
			GroupMemoryBarrierWithGroupSync();
		}
		else
		{
			if(threadID == 0)
				lightList[groupIdx] = uint2(0, -1);

			GroupMemoryBarrierWithGroupSync();
		}
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
