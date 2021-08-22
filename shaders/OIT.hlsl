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
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 UV : TEXCOORD;
};

struct output_Vertex
{
    float4 pos : SV_Position;
    float3 pos_WT : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 UV : TEXCOORD;
};

output_Vertex VMain(input_Vertex input)
{
    output_Vertex outV;

    const float4 pos_WT   = mul(WT, float4(input.pos, 1));

    outV.pos_WT     = pos_WT;
    outV.pos		= mul(PV, mul(WT, float4(pos_WT.xyz, 1)));
    outV.normal     = float3(0, 0, 0);
    outV.tangent    = float3(0, 0, 0);
    outV.UV         = float2(0, 0);

    return outV;
}

struct output_Pixel
{
    float4 Color0       : SV_TARGET0;
    float4 Count        : SV_TARGET1;

    float Depth : SV_Depth;
};

output_Pixel PMain(output_Vertex vertex)
{
    output_Pixel Out;

    Out.Color0  = float4(0, 1, 0, 0.1f);
    Out.Count   = 1.0f;
    Out.Depth   = length(vertex.pos_WT - CameraPOS.xyz) / MaxZ;

    return Out;
}