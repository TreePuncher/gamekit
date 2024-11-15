#include "CBT.hlsl"

#define RS_DEBUGVIS "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),"\
			"RootConstants(num32BitConstants=18, b0),"\
			"SRV(t0),"\
			"DescriptorTable(SRV(t1)),"\
			"StaticSampler(s0, filter=FILTER_MIN_MAG_MIP_LINEAR, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP)"

cbuffer constants : register(b)
{
	float4x4 PV;
	uint32_t maxDepth;
	float	 scale;
}

StructuredBuffer<uint32_t>		CBTBuffer : register(t0);
Texture2D<float>				heightMap : register(t1);
sampler							bilinear  : register(s0);

struct Vertex
{
	float2	texcoord	: texcoord;
	float4	position	: SV_Position;
	uint	heapID		: ID;
};

[RootSignature(RS_DEBUGVIS)]
Vertex DrawCBTTerrain_VS(const uint vertexID : SV_VertexID)
{	
	const uint tid	= (vertexID / 3);
	
	const float3 IN_points[] =
	{
		float3(-100.0f, 0.0f,  100.0f),
		float3(-100.0f, 0.0f, -100.0f),
		float3( 100.0f, 0.0f, -100.0f),
		float3( 100.0f, 0.0f,  100.0f),
	};
	
	const float3 IN_texcoord[] =
	{
		float3(0.0f, 1.0f, 0.0f),
		float3(0.0f, 0.0f, 0.0f),
		float3(1.0f, 0.0f, 0.0f),
		float3(1.0f, 1.0f, 0.0f),
	};
	
	const uint heapID	= DecodeNode(CBTBuffer, maxDepth, tid);
	
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
	const float3x3 tri	= mul(m, points);
	const float3x3 UV	= mul(m, UVs);
	const float2   uv	= UV[vertexID % 3].xy;
	const float3 pos	= tri[vertexID % 3];
	const float    h	= -10.0f + 10.0f * sqrt(heightMap.SampleLevel(bilinear, uv, 0));
	
	Vertex OUT;
	OUT.position	= mul(PV, float4(pos.x, h, pos.z, 1));
	OUT.texcoord	= uv;
	OUT.heapID		= heapID;
	
	return OUT;
}


float4 DrawCBTTerrain_PS1(Vertex v) : SV_Target
{
	uint width;
	uint height;
	heightMap.GetDimensions(width, height);
	
	const float dU = 10.0f / width;
	const float dV = 10.0f / height;
	const float s0 = heightMap.Sample(bilinear, v.texcoord + float2(-dU,  0));
	const float s1 = heightMap.Sample(bilinear, v.texcoord + float2( dU, 0));
	const float s2 = heightMap.Sample(bilinear, v.texcoord + float2( 0,  -dV));
	const float s3 = heightMap.Sample(bilinear, v.texcoord + float2( 0,  -dV));
	
	const float sX = s1 - s0;
	const float sY = s3 - s2;
	
	float3 n = normalize(
	float3(
		float2(
			3.0f * 0.03f / dU * 0.5f * -sX, 
			3.0f * 0.03f / dV * 0.5f * -sY), 
		1)).xzy;

	return pow(float4(dot(n, float3(0, 1, 0)) * float3(0.7, 0.7, 0.7), 1), 2);
}


float4 DrawCBTTerrain_PS2(Vertex v) : SV_Target
{
	static const float3 colors[] =
	{
		float3(1, 0, 0),
		float3(0, 1, 0),
		float3(1, 0, 1),
		float3(0, 0, 1),
		float3(1, 1, 1),
	};
	
	return float4(0, 0, 0, 1);
}