//*************************************************************************************


Buffer<float> weights : register(s0);


//*************************************************************************************


#define PI 3.14159265359

struct VS_Input
{
    float3 p : POSITION;
    float3 n : NORMAL;
    float2 t : TEXCOORD;
};

struct HS_Input
{
    float4 p    : SV_POSITION;
    float3 n    : NORMAL;
    float2 t    : TEXCOORD;
    uint   vid  : VERTEXID;
};

struct PS_Input
{
    float4 p : SV_POSITION;
    float3 n : NORMAL;
    float2 t : TEXCOORD;
};


//*************************************************************************************


HS_Input VS_Main(VS_Input input, const uint vid : SV_VertexID)
{
    PS_Input output;
    output.position = float4(input.position * 0.5f, 1);
    output.normal   = input.normal;
    output.texcoord = input.texcoord;
    output.vid      = 0;

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
    InputPatch<PS_Input, 20>    patch,
    uint                        PatchID : SV_PRIMITIVEID
)
{
    PatchConstants_Output output;
    output.Edges[0] = 4.0f;
    output.Edges[1] = 1.0f;
    output.Edges[2] = 1.0f;
    output.Edges[3] = 1.0f;

    output.Inside[0] = 2.0f;
    output.Inside[1] = 2.0f;

    return output;
}


//*************************************************************************************



float3 GetPatchPoint(const uint pointIdx)
{
    return patchPoints[pointIdx];
}


//*************************************************************************************


[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(20)]
[patchconstantfunc("PatchConstants")]
Gregory_CP HS_Main(
    InputPatch<PS_Input, 4> inputPatch,
    uint                    I : SV_OutputControlPointID)
{
    Gregory_CP controlPoint;
    controlPoint.p = float3(0, 0, 0);

    return controlPoint;
}


//*************************************************************************************


[domain("quad")]
PS_Input DS_Main(
    PatchConstants_Output               constants,
    float2                              UV : SV_DOMAINLOCATION,
    const OutputPatch<Gregory_CP, 20>   gregoryPatch)
{
    PS_Input output;
    output.p = float4(1, 1, 1, 1);
    output.n = float3(1, 1, 1);
    output.t = float2(0, 0);

    return output;
}


//*************************************************************************************


float4 PS_Main(PS_Input input) : SV_TARGET
{
    return float4(input.texcoord.xy, 1, 0);
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
