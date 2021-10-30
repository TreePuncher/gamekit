#include "VXGI_common.hlsl"


cbuffer constant1 : register(b0)
{
    float4 XYZ_DIM;
    float4 XYZ_RES;
    float4 Offset;
};

cbuffer constant2 : register(b1)
{
    float4x4 WT;

};


struct VS_in
{
    float3 pos_MS       : POSITION;
    float3 normal_MS    : NORMAL;
    float2 UV           : TEXCOORD;
};


struct VS_out
{
    float4 pos_WS       : POSITION;
    float3 normal_WS    : NORMAL;
    float2 UV           : TEXCOORD;
};


struct PS_in
{
    float4 pos      : SV_POSITION;
    float3 pos_WS   : POSITION_WT;
    float2 UV       : TEXCOORD;
};


VS_out voxelize_VS(VS_in vertex)
{
    VS_out output;
    output.pos_WS       = mul(WT, float4(vertex.pos_MS, 1));
    output.normal_WS    = mul(WT, float4(vertex.normal_MS, 0));
    output.UV           = vertex.UV;


    return output;
}


float3 CalculateNormal(float3 V0, float3 V1, float3 V2)
{
    float3 normal = cross(V2 - V0, V1 - V0);
    normal = normalize(normal);
    return normal;
}


enum EAxis
{
    X = 0,
    Y = 1,
    Z = 2,
};


/************************************************************************************************/


[maxvertexcount(3)]
void voxelize_GS(triangle VS_out input[3], inout TriangleStream<PS_in> outputStream)
{
    const float3 N = CalculateNormal(
        input[0].pos_WS,
        input[1].pos_WS,
        input[2].pos_WS);

    uint dominantAxis = -1;
    float l = 0.0f;

    [unroll(3)]
    for (uint I = 0; I < 3; I++)
    {
        if (l < N[I])
        {
            l = N[I];
            dominantAxis = I;
        }
    }

    static const float3x3 M[3] = {
        float3x3(
            0, 0, 1,
            0, 1, 0,
            1, 0, 0),
        float3x3(
            1, 0, 0,
            0, 0, 1,
            0, 1, 0),
        float3x3(
            1, 0, 0,
            0, 1, 0,
            0, 0, 1) };

    for (uint II = 0; II < 3; II++)
    {
        PS_in output;
        output.pos      = float4(mul(dominantAxis, (input[II].pos_WS - Offset.xyz) / XYZ_DIM.xyz).xyz * float3(2, -2, 1) + float3(-1, 1, 0), 1);
        output.pos_WS   = input[II].pos_WS;
        output.UV       = input[II].UV;

        outputStream.Append(output);
    }
}


struct VoxelSample
{
    uint64_t    mortonID;
    float4      POS;    // Position + ???
    float4      ColorR; // Color + roughness
};


AppendStructuredBuffer<VoxelSample> voxelSampleBuffer : register(u0);


void voxelize_PS(PS_in input)
{
    VoxelSample vx_sample;
    vx_sample.mortonID  = CreateMortonCode64(input.pos_WS - Offset.xyz);
    vx_sample.ColorR    = float4(input.UV, 0.5f, 0.5f);
    vx_sample.POS       = float4(input.pos_WS.xyz, 0.5f);

    voxelSampleBuffer.Append(vx_sample);
}


/************************************************************************************************/


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
