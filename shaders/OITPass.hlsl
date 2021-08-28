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

    outV.pos_WT     = mul(WT, float4(input.pos, 1));
    outV.pos		= mul(PV, float4(outV.pos_WT.xyz, 1));
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

output_Pixel PassMain(output_Vertex vertex)
{
    output_Pixel Out;

    Out.Color0  = float4(Albedo.xyz, 0.1f);
    Out.Count   = 1.0f;
    Out.Depth   = length(vertex.pos_WT - CameraPOS.xyz) / MaxZ;

    return Out;
}


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
