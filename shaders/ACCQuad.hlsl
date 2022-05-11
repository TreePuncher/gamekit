//*************************************************************************************


#define PI 3.14159265359

enum GregoryQuadControlPoints
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


//*************************************************************************************


// Gregory Quad Patch
//(P3)------(e3-)------(e2+)-------(P2)
// |          |          |           |
// |          |          |           |
// |        (f3-)      (f2+)         |
// |        ____         ____        |
// |                                 |
//(e3+)---(f3+)            (f2-)---(e2-)
// |                                 |
// |                                 |
// |                                 |
// |                                 |
// |                                 |
//(e0-)---(f0-)            (f1+)---(e1+)
// |       _____         ______      |
// |                                 |
// |        (f0+)      (f1-)         |
// |          |          |           |
// |          |          |           |
//(P0)------(e0+)------(e1-)-------(P1)


//*************************************************************************************


struct VS_Input
{
    float3 p : POSITION;
};

struct HS_Input
{
    float4 p    : SV_POSITION;
};

struct PS_Input
{
    float4 p : SV_POSITION;
    float2 t : TEXCOORD;
};

struct ControlPoint
{
    float16_t  w[32];
};

struct GregoryPatch
{
    ControlPoint controlPoints[20];

    uint C_0;
    uint F_n[4];
};


StructuredBuffer<GregoryPatch>  patch : register(t0);
RWStructuredBuffer<float3>      debug : register(u0);


//*************************************************************************************


cbuffer CameraConstants : register(b0)
{
    float4x4 View;
    float4x4 ViewI;
    float4x4 Proj;
    float4x4 PV;				// Projection x View
    float4x4 PVI;
    float4   CameraPOS;
    float  	 MinZ;
    float  	 MaxZ;
    float    AspectRatio;
    float    FOV;

    float3   TLCorner_VS;
    float3   TRCorner_VS;

    float3   BLCorner_VS;
    float3   BRCorner_VS;
};


cbuffer LocalConstants : register(b1)
{
    float4x4 WT;
};

cbuffer LocalConstants : register(b2)
{
    float expansionRate;
};

//*************************************************************************************


HS_Input VS_Main(VS_Input input, const uint vid : SV_VertexID)
{
    HS_Input output;
    output.p = float4(input.p, 1);
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
    float Edges[4]  : SV_TessFactor;
    float Inside[2] : SV_InsideTessFactor;

    float3 gregoryPatch[20] : BZ_CONTROLPOINTS;
};


//*************************************************************************************


uint4 GetFaceIndices(const uint c)
{
    const uint p1 = (c & (0xff << 0))  >> 0;
    const uint p2 = (c & (0xff << 8))  >> 8;
    const uint p3 = (c & (0xff << 16)) >> 16;
    const uint p4 = (c & (0xff << 24)) >> 24;

    return uint4(p1, p2, p3, p4);
}


float3 GetFaceMidPoint(uint4 indices, in InputPatch<HS_Input, 32> patch)
{
    const float3 pos =
        ((indices[0] != 0xff || true) ? patch[indices[0]].p : float3(0, 0, 0)) +
        ((indices[1] != 0xff || true) ? patch[indices[1]].p : float3(0, 0, 0)) +
        ((indices[2] != 0xff || true) ? patch[indices[2]].p : float3(0, 0, 0)) +
        ((indices[3] != 0xff || true) ? patch[indices[3]].p : float3(0, 0, 0));

    const float a =
        (indices[0] != 0xff ? 1.0f : 0.0f) +
        (indices[1] != 0xff ? 1.0f : 0.0f) +
        (indices[2] != 0xff ? 1.0f : 0.0f) +
        (indices[3] != 0xff ? 1.0f : 0.0f);

    return pos / a;
}


//*************************************************************************************


float3 CalculateFacePoint(
    in const float3 C0,
    in const float3 C1,
    in const float3 P,
    in const float3 eP,
    in const float3 eM,
    in const float3 r)
{
    const float c0 = cos(2 * PI / 4);
    const float c1 = cos(2 * PI / 4);
    static const float D = 4.0f;
    return (c1 * P + (D - (2.0f * c0) - c1) * eP + (2 * c0 * eM) + r) / D;
}


//*************************************************************************************


PatchConstants_Output PatchConstants(
    const InputPatch<HS_Input, 32>      inputPoints,
    const OutputPatch<Gregory_CP, 20>   controlPoints,
    uint                                patchIdx : SV_PRIMITIVEID
)
{
    const uint4 F0  = GetFaceIndices(patch[patchIdx].C_0);
    const float3 C0 = GetFaceMidPoint(F0, inputPoints);

    const uint4 N1  = GetFaceIndices(patch[patchIdx].F_n[0]);
    const float3 C1 = GetFaceMidPoint(N1, inputPoints);

    const uint4 N2  = GetFaceIndices(patch[patchIdx].F_n[1]);
    const float3 C2 = GetFaceMidPoint(N2, inputPoints);

    const uint4 N3  = GetFaceIndices(patch[patchIdx].F_n[2]);
    const float3 C3 = GetFaceMidPoint(N3, inputPoints);

    const uint4 N4  = GetFaceIndices(patch[patchIdx].F_n[3]);
    const float3 C4 = GetFaceMidPoint(N4, inputPoints);

    PatchConstants_Output output;
    output.Edges[0] = 64;
    output.Edges[1] = 64;
    output.Edges[2] = 64;
    output.Edges[3] = 64;

    output.Inside[0] = 64;
    output.Inside[1] = 64;

    output.gregoryPatch[p0] = controlPoints[p0].p;
    output.gregoryPatch[p1] = controlPoints[p1].p;
    output.gregoryPatch[p2] = controlPoints[p2].p;
    output.gregoryPatch[p3] = controlPoints[p3].p;

    output.gregoryPatch[e0Minus]    = controlPoints[p0].p + 2.0f / 3.0f * controlPoints[e0Minus].p;
    output.gregoryPatch[e0Plus]     = controlPoints[p0].p + 2.0f / 3.0f * controlPoints[e0Plus].p;

    output.gregoryPatch[e1Minus]    = controlPoints[p1].p + 2.0f / 3.0f * controlPoints[e1Minus].p;
    output.gregoryPatch[e1Plus]     = controlPoints[p1].p + 2.0f / 3.0f * controlPoints[e1Plus].p;

    output.gregoryPatch[e2Plus]     = controlPoints[p2].p + 2.0f / 3.0f * controlPoints[e2Plus].p;
    output.gregoryPatch[e2Minus]    = controlPoints[p2].p + 2.0f / 3.0f * controlPoints[e2Minus].p;

    output.gregoryPatch[e3Plus]     = controlPoints[p3].p + 2.0f / 3.0f * controlPoints[e3Plus].p;
    output.gregoryPatch[e3Minus]    = controlPoints[p3].p + 2.0f / 3.0f * controlPoints[e3Minus].p;

    float y = 0.940972328;
    bool enable = false;

    output.gregoryPatch[r0Plus] =
        (patchIdx == 0  && enable) ?
            float3(1.45833, y, 1.25) :
            CalculateFacePoint(
                C0, C1,
                output.gregoryPatch[p0],
                output.gregoryPatch[e0Plus],
                output.gregoryPatch[e1Minus],
                controlPoints[r0Plus].p);

    output.gregoryPatch[r0Minus] =
        (patchIdx == 0 && enable) ?
            float3(1.25, y, 1.45833) :
            CalculateFacePoint(
                C0, C4,
                output.gregoryPatch[p0],
                output.gregoryPatch[e0Minus],
                output.gregoryPatch[e3Plus],
                controlPoints[r0Minus].p);

    output.gregoryPatch[r1Plus] =
        (patchIdx == 0 && enable) ?
            float3(1.75, y, 1.45833) :
            CalculateFacePoint(
                C0, C2,
                output.gregoryPatch[p1],
                output.gregoryPatch[e1Plus],
                output.gregoryPatch[e2Minus],
                controlPoints[r1Plus].p);

    output.gregoryPatch[r1Minus] =
        (patchIdx == 0 && enable) ?
            float3(1.54167, y, 1.25) :
            CalculateFacePoint(
                C0, C1,
                output.gregoryPatch[p1],
                output.gregoryPatch[e1Minus],
                output.gregoryPatch[e0Plus],
                controlPoints[r1Minus].p);

    output.gregoryPatch[r2Plus] =
        (patchIdx == 0 && enable) ?
            float3(1.54167, y, 1.75) :
            CalculateFacePoint(
                C0, C3,
                output.gregoryPatch[p2],
                output.gregoryPatch[e2Plus],
                output.gregoryPatch[e3Minus],
                controlPoints[r2Plus].p);

    output.gregoryPatch[r2Minus] =
        (patchIdx == 0 && enable) ?
            float3(1.75, y, 1.54167) :
            CalculateFacePoint(
                C0, C2,
                output.gregoryPatch[p2],
                output.gregoryPatch[e2Minus],
                output.gregoryPatch[e1Plus],
                controlPoints[r2Minus].p);

    output.gregoryPatch[r3Plus] =
        (patchIdx == 0 && enable) ?
            float3(1.25, y, 1.54167) :
            CalculateFacePoint(
                C0, C4,
                output.gregoryPatch[p3],
                output.gregoryPatch[e3Plus],
                output.gregoryPatch[e0Minus],
                controlPoints[r3Plus].p);

    output.gregoryPatch[r3Minus] =
        (patchIdx == 0 && enable) ?
            float3(1.45833, y, 1.75) :
            CalculateFacePoint(
                C0, C3,
                output.gregoryPatch[p3],
                output.gregoryPatch[e3Minus],
                output.gregoryPatch[e2Plus],
                controlPoints[r3Minus].p);

    for (int J = 0; J < 20; J++)
        debug[25 * patchIdx + J] = output.gregoryPatch[J];

    debug[25 * patchIdx + 20] = C0;
    debug[25 * patchIdx + 21] = C1;
    debug[25 * patchIdx + 22] = C2;
    debug[25 * patchIdx + 23] = C3;
    debug[25 * patchIdx + 24] = C4;

    return output;
}


//*************************************************************************************


[domain("quad")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(20)]
[patchconstantfunc("PatchConstants")]
Gregory_CP HS_Main(
    InputPatch<HS_Input, 32>    inputPatch,
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

    const float u = UV.x;
    const float v = UV.y;
    const float U = 1 - UV.x;
    const float V = 1 - UV.y;

    const float d11 = (u + v == 0.0f) ? 1.0f : u + v;
    const float d12 = (U + v == 0.0f) ? 1.0f : U + v;
    const float d21 = (u + V == 0.0f) ? 1.0f : u + V;
    const float d22 = (U + V == 0.0f) ? 1.0f : U + V;

    float3 bezierPatch[4][4];
    bezierPatch[0][0] = constants.gregoryPatch[p0];
    bezierPatch[0][1] = constants.gregoryPatch[e0Plus];
    bezierPatch[0][2] = constants.gregoryPatch[e1Minus];
    bezierPatch[0][3] = constants.gregoryPatch[p1];

    bezierPatch[1][0] = constants.gregoryPatch[e0Minus];
    bezierPatch[1][1] = (u * constants.gregoryPatch[r0Plus] + v * constants.gregoryPatch[r0Minus]) / d11;
    bezierPatch[1][2] = (U * constants.gregoryPatch[r1Minus] + v * constants.gregoryPatch[r1Plus]) / d12;
    bezierPatch[1][3] = constants.gregoryPatch[e1Plus];

    bezierPatch[2][0] = constants.gregoryPatch[e3Plus];
    bezierPatch[2][1] = (u * constants.gregoryPatch[r3Minus] + V * constants.gregoryPatch[r3Plus]) / d21;
    bezierPatch[2][2] = (U * constants.gregoryPatch[r2Plus] + V * constants.gregoryPatch[r2Minus]) / d22;
    bezierPatch[2][3] = constants.gregoryPatch[e2Minus];

    bezierPatch[3][0] = constants.gregoryPatch[p3];
    bezierPatch[3][1] = constants.gregoryPatch[e3Minus]; 
    bezierPatch[3][2] = constants.gregoryPatch[e2Plus];
    bezierPatch[3][3] = constants.gregoryPatch[p2];

    const float3 i0 =
        lerp(
            lerp(
                lerp(bezierPatch[0][0], bezierPatch[0][1], u),
                lerp(bezierPatch[0][1], bezierPatch[0][2], u),
                u),
            lerp(
                lerp(bezierPatch[0][1], bezierPatch[0][2], u),
                lerp(bezierPatch[0][2], bezierPatch[0][3], u),
                u),
            u);

    const float3 i1 =
        lerp(
            lerp(
                lerp(bezierPatch[1][0], bezierPatch[1][1], u),
                lerp(bezierPatch[1][1], bezierPatch[1][2], u),
                u),
            lerp(
                lerp(bezierPatch[1][1], bezierPatch[1][2], u),
                lerp(bezierPatch[1][2], bezierPatch[1][3], u),
                u),
            u);

    const float3 i2 =
        lerp(
            lerp(
                lerp(bezierPatch[2][0], bezierPatch[2][1], u),
                lerp(bezierPatch[2][1], bezierPatch[2][2], u),
                u),
            lerp(
                lerp(bezierPatch[2][1], bezierPatch[2][2], u),
                lerp(bezierPatch[2][2], bezierPatch[2][3], u),
                u),
            u);

    const float3 i3 =
        lerp(
            lerp(
                lerp(bezierPatch[3][0], bezierPatch[3][1], u),
                lerp(bezierPatch[3][1], bezierPatch[3][2], u),
                UV.x),
            lerp(
                lerp(bezierPatch[3][1], bezierPatch[3][2], u),
                lerp(bezierPatch[3][2], bezierPatch[3][3], u),
                u),
            u);

    const float3 p =
        lerp(
            lerp(
                lerp(i0, i1, v),
                lerp(i1, i2, v),
                v),
            lerp(
                lerp(i1, i2, v),
                lerp(i2, i3, v),
                v),
            v) - float3(0, 2.5f, 0);

    const float4 POS_MS = float4(p.xyz, 1) * float4(3, 3, 3, 1) + float4(-5, 6, -2, 0);
    const float4 pos_WS = mul(WT, POS_MS);
    const float4 pos_DC = mul(PV, pos_WS);

    output.p = pos_DC;
    output.t = UV;

    return output;
}


//*************************************************************************************


float4 PS_Main(PS_Input input) : SV_TARGET
{
    return float4(input.t, 1, 1);
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
