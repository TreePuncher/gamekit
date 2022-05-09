//*************************************************************************************


#define PI 3.14159265359

enum GregoryQuadPatchPoint
{
    p0      = 0,
    p1      = 1,
    p2      = 2,
    p3      = 3,

    e0Minus = 4,
    e0Plus  = 5,
    e1Minus = 6,
    e1Plus  = 7,
    e2Minus = 8,
    e2Plus  = 9,
    e3Minus = 10,
    e3Plus  = 11,

    r0Minus = 12,
    r0Plus  = 13,

    r1Minus = 14,
    r1Plus  = 15,

    r2Minus = 16,
    r2Plus  = 17,

    r3Minus = 18,
    r3Plus  = 19
};



struct VS_Input
{
    float3 p : POSITION;
    //float3 n : NORMAL;
    //float2 t : TEXCOORD;
};

struct HS_Input
{
    float4 p    : SV_POSITION;
    //float3 n    : NORMAL;
    //float2 t    : TEXCOORD;
};

struct PS_Input
{
    float4 p : SV_POSITION;
    //float3 n : NORMAL;
    float2 t : TEXCOORD;
};

struct ControlPoint
{
    float16_t  w[32];
};

struct GregoryPatch
{
    ControlPoint controlPoints[20];
    /*

    struct ControlPoint_F_n
    {
        uint C_0;
        uint C_n[2];
    } F_n[4];
    */
};


StructuredBuffer<GregoryPatch> patch : register(t0);


//*************************************************************************************


HS_Input VS_Main(VS_Input input, const uint vid : SV_VertexID)
{
    HS_Input output;
    output.p = float4(input.p + float3(-1, -1, 0), 1);
    //output.normal   = input.normal;
    //output.texcoord = input.texcoord;

    return output;
}


//*************************************************************************************


struct Gregory_CP
{
    float3 p : POINT;
};


//*************************************************************************************


struct PatchConstants_Output
{
    float Edges[4]        : SV_TessFactor;
    float Inside[2]       : SV_InsideTessFactor;

    //float3 Bezier_CP[16] : BZ_CONTROLPOINTS;
};


//*************************************************************************************


PatchConstants_Output PatchConstants(
    InputPatch<HS_Input, 32>            patch,
    const OutputPatch<Gregory_CP, 20>   controlPoints,
    uint                                patchIdx : SV_PRIMITIVEID
)
{
    PatchConstants_Output output;
    output.Edges[0] = 3.0f;
    output.Edges[1] = 3.0f;
    output.Edges[2] = 3.0f;
    output.Edges[3] = 3.0f;

    output.Inside[0] = 3.0f;
    output.Inside[1] = 3.0f;

    return output;
}


//*************************************************************************************


[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(20)]
[patchconstantfunc("PatchConstants")]
Gregory_CP HS_Main(
    InputPatch<HS_Input, 32>            inputPatch,
    const uint                  I           : SV_OutputControlPointID,
    const uint                  patchIdx    : SV_PrimitiveID)
{
    float3 pos = 0.0f;

    for (int J = 0; J < 32; J++)
        pos += inputPatch[J].p * patch[patchIdx].controlPoints[I].w[J];

    Gregory_CP controlPoint;
    controlPoint.p = pos;

    return controlPoint;
}


//*************************************************************************************


[domain("quad")]
PS_Input DS_Main(
    PatchConstants_Output               constants,
    float2                              UV : SV_DOMAINLOCATION,
    const OutputPatch<Gregory_CP, 20>   controlPoints)
{
    PS_Input output;

    output.p =
        float4(lerp(lerp(controlPoints[p0].p, controlPoints[p1].p, UV.x),
                    lerp(controlPoints[p3].p, controlPoints[p2].p, UV.x), UV.y),
               1.0f);

    output.t = UV;

    return output;
}


//*************************************************************************************


float4 PS_Main(PS_Input input) : SV_TARGET
{
    return float4(input.t, 1, 0);
}


//*************************************************************************************


/**********************************************************************

Copyright (c) 2015 - 2022 Robert May

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
