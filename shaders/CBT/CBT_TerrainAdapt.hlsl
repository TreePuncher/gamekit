#include "CBT.hlsl"

#define RS_DEBUGVIS "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),"\
			"UAV(u0),"\
			"RootConstants(num32BitConstants=18, b1),"\
			"DescriptorTable(SRV(t0)),"\
			"RootConstants(num32BitConstants=1, b0),"\
			"CBV(b2),"\
			"StaticSampler(s0, filter=FILTER_MIN_MAG_MIP_LINEAR, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP)"

struct Plane
{
	float4 n;
	float4 o;
};

struct Frustum
{
	Plane planes[6];
};

struct AABB
{
	float3 min;
	float3 max;
};

cbuffer indirectArguments : register(b0)
{
	uint32_t	nodeCount;
}

cbuffer constants : register(b1)
{
	float4x4	PV;
	uint32_t	maxDepth;
	float		scale;
}

cbuffer cameraConstants : register(b2)
{
	float4x4	view;
	Frustum		f;
}

globallycoherent RWStructuredBuffer<uint32_t>	CBTBuffer	: register(u0);
Texture2D<float>								heightMap	: register(t0);
sampler											bilinear	: register(s0);


bool Intersects(const in Frustum frustum, const in AABB aabb)
{
	int Result = 1;

	for (int I = 0; I < 6; ++I)
	{
		float px = (frustum.planes[I].n.x >= 0.0f) ? aabb.min.x : aabb.max.x;
		float py = (frustum.planes[I].n.y >= 0.0f) ? aabb.min.y : aabb.max.y;
		float pz = (frustum.planes[I].n.z >= 0.0f) ? aabb.min.z : aabb.max.z;

		float3 pV = float3(px, py, pz) - frustum.planes[I].o;
		float dP = dot(frustum.planes[I].n, pV);

		if (dP >= 0)
			return false;
	}

	return true;
}


bool Intersects(const in Frustum frustum, const in float3x3 tri)
{
	AABB aabb;
	aabb.min = float3(
		min(min(tri[0].x, tri[1].x), tri[2].x),
		min(min(tri[0].y, tri[1].y), tri[2].y),
		min(min(tri[0].z, tri[1].z), tri[2].z));

	aabb.max = float3(
		max(max(tri[0].x, tri[1].x), tri[2].x),
		max(max(tri[0].y, tri[1].y), tri[2].y),
		max(max(tri[0].z, tri[1].z), tri[2].z));
	
	return Intersects(frustum, aabb);
}


float TriangleLevelOfDetail_Perspective(in const float3x3 verts)
{
	if (!Intersects(f, verts))
		return 0.0f;
	
	const float3	v0 = verts[0];
	const float3	v2 = verts[2];
	
	const float3	edgeCenter = (v0 + v2) / 2; 
	const float3	edgeVector = (v2 - v0);
	const float		distanceToEdgeSqr	= dot(edgeCenter, edgeCenter);
	const float		edgeLengthSqr		= dot(edgeVector, edgeVector);

	return 16.04509354 + log2(edgeLengthSqr / distanceToEdgeSqr);
}


float3x3 DecodeTrianglePointsVS(uint32_t heapID, in float4x3 IN_points, in float4x3 IN_texcoord)
{
	float3x3 points;
	float3x3 UVs;
	
	if (GetBitValue(heapID, FindMSB(heapID) - 1) != 0)
	{
		points =
			float3x3(
				IN_points[0],
				IN_points[1],
				IN_points[2]);
		
		UVs =
			float3x3(
				IN_texcoord[0],
				IN_texcoord[1],
				IN_texcoord[2]);
	}
	else
	{
		points =
			float3x3(
				IN_points[2],
				IN_points[3],
				IN_points[0]);
		
		UVs =
			float3x3(
				IN_texcoord[2],
				IN_texcoord[3],
				IN_texcoord[0]);
	}
	
	const float3x3 m	= GetLEBMatrix(heapID);
	const float3x3 UV	= mul(m, UVs);
	const float3x3 tri	= mul(m, points);
	
	tri[0].y = -10.0f + 10.0f * sqrt(heightMap.SampleLevel(bilinear, UV[0], 0));
	tri[1].y = -10.0f + 10.0f * sqrt(heightMap.SampleLevel(bilinear, UV[1], 0));
	tri[2].y = -10.0f + 10.0f * sqrt(heightMap.SampleLevel(bilinear, UV[2], 0));
	
	return float3x3(
			mul(view, float4(tri[0].xyz, 1.0f)).xyz,
			mul(view, float4(tri[1].xyz, 1.0f)).xyz,
			mul(view, float4(tri[2].xyz, 1.0f)).xyz);
	}


[RootSignature(RS_DEBUGVIS)]
[NumThreads(1024, 1, 1)]
void UpdateAdaptiveTerrain(const uint tid : SV_DispatchThreadID)
{
	const float4x3 IN_points =
	{
		float3(-100.0f, 0.0f,  100.0f),
		float3(-100.0f, 0.0f, -100.0f),
		float3( 100.0f, 0.0f, -100.0f),
		float3( 100.0f, 0.0f,  100.0f),
	};
	
	const float4x3 IN_texcoord =
	{
		float3(0.0f, 1.0f, 0.0f),
		float3(0.0f, 0.0f, 0.0f),
		float3(1.0f, 0.0f, 0.0f),
		float3(1.0f, 1.0f, 0.0f),
	};
	
	const uint	heapID	= DecodeNode(CBTBuffer, maxDepth, tid);
	
	if (FindMSB(heapID) < maxDepth && TriangleLevelOfDetail_Perspective(DecodeTrianglePointsVS(heapID, IN_points, IN_texcoord)) >= 1.0f)
	{
		Split(CBTBuffer, heapID, maxDepth);
		return;
	}

	const uint parent		= heapID / 2;
	const uint edgeNeighbor = LEBQuadNeighbors(parent).edge;
	
	bool mergeParent	= (TriangleLevelOfDetail_Perspective(DecodeTrianglePointsVS(parent, IN_points, IN_texcoord)) <= 1.0f);
	bool mergeNeighbor  = (TriangleLevelOfDetail_Perspective(DecodeTrianglePointsVS(edgeNeighbor, IN_points, IN_texcoord)) <= 1.0f);
	
	if(mergeParent && mergeNeighbor)
		Merge(CBTBuffer, heapID, maxDepth);
}