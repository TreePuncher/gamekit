#include "common.hlsl"
#include "pbr.hlsl"

struct PointLight
{
    float4 KI;	// Color + intensity in W
    float4 PR;	// XYZ + radius in W
};

struct Cluster
{
    float4 MinPoint;
    float4 MaxPoint;
};

struct LightList
{
    uint length;
    uint offset;
};

cbuffer LocalConstants : register(b1)
{
    float4	 Albedo;
	float    Ks;
	float    IOR;
	float    Roughness;
	float    Anisotropic;
	float    Metallic;
    float4x4 WT;
    uint     textureCount;
}

struct input_Vertex
{
    float3 point;
    float3 normal;
    float3 tangent;
    float2 UV;
};

struct output_Vertex
{
    float3 point;
    float3 normal;
    float3 tangent;
    float2 UV;
};

output_Vertex VMain(input_Vertex input)
{
    output_Vertex outV;

    return outV;
}

float4 PMain(output_Vertex vertex) : SV_TARGET
{
    return float4(0, 1, 0, 1);
}