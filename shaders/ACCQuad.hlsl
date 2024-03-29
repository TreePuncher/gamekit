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

struct ControlPoint
{
    float16_t  w[32];
};

struct GregoryPatch
{
    ControlPoint controlPoints[20];

    uint vertexValences;
};


StructuredBuffer<GregoryPatch>  patch           : register(t0);
RWStructuredBuffer<uint>        counter         : register(u0);
RWStructuredBuffer<float3>      normal_Debug    : register(u1); // normal
RWStructuredBuffer<float3>      CP_debug        : register(u2); // control point


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
    output.p = float4(input.p.zyx * float3(1, 1, 1), 1);

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


uint4 Unpack(uint packed)
{
    const uint p0 = (packed & (0xff << 0))  >> 0;
    const uint p1 = (packed & (0xff << 8))  >> 8;
    const uint p2 = (packed & (0xff << 16)) >> 16;
    const uint p3 = (packed & (0xff << 24)) >> 24;

    return uint4(p0, p1, p2, p3);
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
    in const float C0,
    in const float C1,
    in const float3 P,
    in const float3 eP,
    in const float3 eM,
    in const float3 r)
{
    static const float D = 4.0f;

    return (C1 * P + (D - (2.0f * C0) - C1) * eP + (2 * C0 * eM) + r) / D;
}


//*************************************************************************************


PatchConstants_Output PatchConstants(
    const InputPatch<HS_Input, 32>      inputPoints,
    const OutputPatch<Gregory_CP, 20>   controlPoints,
    uint                                patchIdx : SV_PRIMITIVEID
)
{
    const uint4 valences    = Unpack(patch[patchIdx].vertexValences);
    const float4 C          = cos(2 * PI / float4(valences));


    PatchConstants_Output output;

    output.gregoryPatch[p0] = controlPoints[p0].p;
    output.gregoryPatch[p1] = controlPoints[p1].p;
    output.gregoryPatch[p2] = controlPoints[p2].p;
    output.gregoryPatch[p3] = controlPoints[p3].p;

    output.gregoryPatch[e0Minus]    = controlPoints[e0Minus].p;
    output.gregoryPatch[e0Plus]     = controlPoints[e0Plus].p;

    output.gregoryPatch[e1Minus]    = controlPoints[e1Minus].p;
    output.gregoryPatch[e1Plus]     = controlPoints[e1Plus].p;

    output.gregoryPatch[e2Plus]     = controlPoints[e2Plus].p;
    output.gregoryPatch[e2Minus]    = controlPoints[e2Minus].p;

    output.gregoryPatch[e3Plus]     = controlPoints[e3Plus].p;
    output.gregoryPatch[e3Minus]    = controlPoints[e3Minus].p;


    output.gregoryPatch[r0Plus] =
            CalculateFacePoint(
                C[0], C[3],
                output.gregoryPatch[p0],
                output.gregoryPatch[e0Plus],
                output.gregoryPatch[e1Minus],
                controlPoints[r0Plus].p);

    output.gregoryPatch[r0Minus] =
            CalculateFacePoint(
                C[0], C[1],
                output.gregoryPatch[p0],
                output.gregoryPatch[e0Minus],
                output.gregoryPatch[e3Plus],
                controlPoints[r0Minus].p);

    output.gregoryPatch[r1Plus] =
            CalculateFacePoint(
                C[1], C[0],
                output.gregoryPatch[p1],
                output.gregoryPatch[e1Plus],
                output.gregoryPatch[e2Minus],
                controlPoints[r1Plus].p);

    output.gregoryPatch[r1Minus] =
            CalculateFacePoint(
                C[1], C[2],
                output.gregoryPatch[p1],
                output.gregoryPatch[e1Minus],
                output.gregoryPatch[e0Plus],
                controlPoints[r1Minus].p);

    output.gregoryPatch[r2Plus] =
            CalculateFacePoint(
                C[2], C[1],
                output.gregoryPatch[p2],
                output.gregoryPatch[e2Plus],
                output.gregoryPatch[e3Minus],
                controlPoints[r2Plus].p);

    output.gregoryPatch[r2Minus] =
            CalculateFacePoint(
                C[2], C[3],
                output.gregoryPatch[p2],
                output.gregoryPatch[e2Minus],
                output.gregoryPatch[e1Plus],
                controlPoints[r2Minus].p);

    output.gregoryPatch[r3Plus] =
            CalculateFacePoint(
                C[3], C[2],
                output.gregoryPatch[p3],
                output.gregoryPatch[e3Plus],
                output.gregoryPatch[e0Minus],
                controlPoints[r3Plus].p);

    output.gregoryPatch[r3Minus] =
            CalculateFacePoint(
                C[3], C[0],
                output.gregoryPatch[p3],
                output.gregoryPatch[e3Minus],
                output.gregoryPatch[e2Plus],
                controlPoints[r3Minus].p);

    CP_debug[2 * p0 + 0 + patchIdx * 40] = output.gregoryPatch[0] + float3(0, 0.0f, 0);
    CP_debug[2 * p1 + 0 + patchIdx * 40] = output.gregoryPatch[1] + float3(0, 0.05f, 0);
    CP_debug[2 * p2 + 0 + patchIdx * 40] = output.gregoryPatch[2] + float3(0, 0.1f, 0);
    CP_debug[2 * p3 + 0 + patchIdx * 40] = output.gregoryPatch[3] + float3(0, 0.15f, 0);
    CP_debug[2 * p0 + 1 + patchIdx * 40] = float4(5, 0, 0, 0);
    CP_debug[2 * p1 + 1 + patchIdx * 40] = float4(5, 1, 0, 0);
    CP_debug[2 * p2 + 1 + patchIdx * 40] = float4(5, 2, 0, 0);
    CP_debug[2 * p3 + 1 + patchIdx * 40] = float4(5, 3, 0, 0);

    CP_debug[2 * e0Minus + 0 + patchIdx * 40] = output.gregoryPatch[e0Minus];
    CP_debug[2 * e0Minus + 1 + patchIdx * 40] = float4(0, 0, 0, 0); // Red
    CP_debug[2 * e0Plus  + 0 + patchIdx * 40] = output.gregoryPatch[e0Plus];
    CP_debug[2 * e0Plus  + 1 + patchIdx * 40] = float4(0, 1, 0, 0); // Green

    CP_debug[2 * e1Minus + 0 + patchIdx * 40] = output.gregoryPatch[e1Minus];
    CP_debug[2 * e1Minus + 1 + patchIdx * 40] = float4(1, 0, 0, 0); // Red
    CP_debug[2 * e1Plus  + 0 + patchIdx * 40] = output.gregoryPatch[e1Plus];
    CP_debug[2 * e1Plus  + 1 + patchIdx * 40] = float4(1, 1, 0, 0); // Green

    CP_debug[2 * e2Minus + 0 + patchIdx * 40] = output.gregoryPatch[e2Minus];
    CP_debug[2 * e2Minus + 1 + patchIdx * 40] = float4(2, 0, 0, 0); // Red
    CP_debug[2 * e2Plus  + 0 + patchIdx * 40] = output.gregoryPatch[e2Plus];
    CP_debug[2 * e2Plus  + 1 + patchIdx * 40] = float4(2, 1, 0, 0); // Green

    CP_debug[2 * e3Minus + 0 + patchIdx * 40] = output.gregoryPatch[e3Minus];
    CP_debug[2 * e3Minus + 1 + patchIdx * 40] = float4(3, 0, 0, 0); // Red
    CP_debug[2 * e3Plus  + 0 + patchIdx * 40] = output.gregoryPatch[e3Plus];
    CP_debug[2 * e3Plus  + 1 + patchIdx * 40] = float4(3, 1, 0, 0); // Green

    CP_debug[2 * r0Minus +  0 + patchIdx * 40] = output.gregoryPatch[r0Minus];
    CP_debug[2 * r0Minus +  1 + patchIdx * 40] = float4(0, 0, 0, 0); // Red
    CP_debug[2 * r0Minus +  2 + patchIdx * 40] = output.gregoryPatch[r0Plus]; // 
    CP_debug[2 * r0Minus +  3 + patchIdx * 40] = float4(0, 1, 0, 0); // Green
    CP_debug[2 * r0Minus +  4 + patchIdx * 40] = output.gregoryPatch[r1Minus];
    CP_debug[2 * r0Minus +  5 + patchIdx * 40] = float4(1, 0, 0, 0); // Red
    CP_debug[2 * r0Minus +  6 + patchIdx * 40] = output.gregoryPatch[r1Plus];
    CP_debug[2 * r0Minus +  7 + patchIdx * 40] = float4(1, 1, 0, 0); // Green
    CP_debug[2 * r0Minus +  8 + patchIdx * 40] = output.gregoryPatch[r2Minus];
    CP_debug[2 * r0Minus +  9 + patchIdx * 40] = float4(2, 0, 0, 0); // Red
    CP_debug[2 * r0Minus + 10 + patchIdx * 40] = output.gregoryPatch[r2Plus];
    CP_debug[2 * r0Minus + 11 + patchIdx * 40] = float4(2, 1, 0, 0); // Green
    CP_debug[2 * r0Minus + 12 + patchIdx * 40] = output.gregoryPatch[r3Minus];
    CP_debug[2 * r0Minus + 13 + patchIdx * 40] = float4(3, 0, 0, 0); // Red
    CP_debug[2 * r0Minus + 14 + patchIdx * 40] = output.gregoryPatch[r3Plus];
    CP_debug[2 * r0Minus + 15 + patchIdx * 40] = float4(3, 1, 0, 0); // Green

    const float4 pos0_DC = mul(PV, mul(WT, float4(controlPoints[p0].p, 1)));
    const float4 pos1_DC = mul(PV, mul(WT, float4(controlPoints[p1].p, 1)));
    const float4 pos2_DC = mul(PV, mul(WT, float4(controlPoints[p2].p, 1)));
    const float4 pos3_DC = mul(PV, mul(WT, float4(controlPoints[p3].p, 1)));

    const float MaxZ = max(max(pos0_DC.z, pos1_DC.z), max(pos2_DC.z, pos3_DC.z));

    if (MaxZ < 0.0f)
    {
        output.Edges[0] = 0;
        output.Edges[1] = 0;
        output.Edges[2] = 0;
        output.Edges[3] = 0;

        output.Inside[0] = 0;
        output.Inside[1] = 0;
    }
    else
    {
        const float3 pos0_NDC = pos0_DC.xyz / pos0_DC.w;
        const float3 pos1_NDC = pos1_DC.xyz / pos1_DC.w;
        const float3 pos2_NDC = pos2_DC.xyz / pos2_DC.w;
        const float3 pos3_NDC = pos3_DC.xyz / pos3_DC.w;

        const float e0_factor = min(expansionRate, (2160.0f / 16) * length((pos0_NDC - pos1_NDC).xy * float2(AspectRatio, 1)) / 2.0f);
        const float e1_factor = min(expansionRate, (2160.0f / 16) * length((pos1_NDC - pos2_NDC).xy * float2(AspectRatio, 1)) / 2.0f);
        const float e2_factor = min(expansionRate, (2160.0f / 16) * length((pos2_NDC - pos3_NDC).xy * float2(AspectRatio, 1)) / 2.0f);
        const float e3_factor = min(expansionRate, (2160.0f / 16) * length((pos3_NDC - pos0_NDC).xy * float2(AspectRatio, 1)) / 2.0f);

        output.Edges[0] = e3_factor;
        output.Edges[1] = e2_factor;
        output.Edges[2] = e1_factor;
        output.Edges[3] = e0_factor;

        output.Inside[0] = (e0_factor + e2_factor + e1_factor + e3_factor) / 4.0f;
        output.Inside[1] = (e0_factor + e2_factor + e1_factor + e3_factor) / 4.0f;
    }

    return output;
}


//*************************************************************************************


[domain("quad")]
[partitioning("fractional_even")]
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


struct PS_Input
{
    float4 p        : SV_POSITION;
    float3 pos_WS   : POSITION;
    float2 tx       : TEXCOORD;
    float3 bt       : BITANGENT;
    float3 t        : TANGENT;
};

void Univar4(in float u, out float B[4], out float D[4])
{
    float t = u;
    float s = 1.0 - u;

    float A0 =     s * s;
    float A1 = 2 * s * t;
    float A2 = t * t;

    B[0] =          s * A0;
    B[1] = t * A0 + s * A1;
    B[2] = t * A1 + s * A2;
    B[3] = t * A2;

    D[0] =    - A0;
    D[1] = A0 - A1;
    D[2] = A1 - A2;
    D[3] = A2;
}

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

    const float3 u0 = 3 * (constants.gregoryPatch[e0Plus]   - constants.gregoryPatch[p0]);
    const float3 u1 = 3 * (constants.gregoryPatch[e1Minus]  - constants.gregoryPatch[e0Plus]);
    const float3 u2 = 3 * (constants.gregoryPatch[p1]       - constants.gregoryPatch[e1Minus]);

    const float3 v0 = 3 * (constants.gregoryPatch[e0Plus]   - constants.gregoryPatch[p0]);
    const float3 v1 = 4 * (constants.gregoryPatch[r0Plus]   - constants.gregoryPatch[e0Plus]);
    const float3 v2 = 4 * (constants.gregoryPatch[r1Minus]  - constants.gregoryPatch[e1Minus]);
    const float3 v3 = 3 * (constants.gregoryPatch[e1Plus]   - constants.gregoryPatch[p1]);

    float B[4], D[4];

    Univar4(v, B, D);
    float3 BUCP[4], DUCP[4];


    for (int i = 0; i < 4; ++i)
    {
        BUCP[i] = float3(0, 0, 0);
        DUCP[i] = float3(0, 0, 0);

        for (uint j = 0; j < 4; ++j)
        {
            float3 A = bezierPatch[j][i];

            BUCP[i] += A * B[j];
            DUCP[i] += A * D[j];
        }
    }

    Univar4(u, B, D);

    float3 Tangent      = float3(0, 0, 0);
    float3 BiTangent    = float3(0, 0, 0);

    for (uint j = 0; j < 4; ++j)
    {
        Tangent     += B[j] * DUCP[j];
        BiTangent   += D[j] * BUCP[j];
    }

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
            v);

    const float4 POS_MS = float4(p.xyz, 1);
    const float4 POS_WS = mul(WT, POS_MS);
    const float4 POS_DC = mul(PV, POS_WS);

    output.t    = mul(WT, Tangent);
    output.bt   = mul(WT, BiTangent);

    output.pos_WS   = POS_WS;
    output.p        = POS_DC;
    output.tx       = UV;

    uint idx = 0xffffffff;
    InterlockedAdd(counter[0], 2, idx);
    normal_Debug[idx + 0] = POS_WS;
    normal_Debug[idx + 1] = POS_WS + normalize(cross(output.bt, output.t)) * 0.05f;

    return output;
}


//*************************************************************************************


float4 PS_Main(PS_Input input) : SV_TARGET
{
    const float3 light_pos      = float3(2, 2, 2);
    const float3 l_dir          = normalize(light_pos - input.pos_WS);
    const float3 v_dir          = normalize(input.pos_WS - CameraPOS);
    const float3 n              = normalize(cross(input.bt, input.t));
    const float3 rDir           = reflect(l_dir, n);
    const float d               = length(input.pos_WS - light_pos);
    const float i               = 0.4f;

    const float spec            = i * pow(saturate(dot(v_dir, rDir)), 30) / (d * d);
    const float diff            = i * dot(n, l_dir) / (d * d);
    const float3 K              = spec + diff * pow(float3(1.0f, 1.0f, 1.0f), 2.2f);

    //return float4(1, 1, 1, 1);
    return float4(K, 1);
    //return float4(float3(1, 1, 1) * (n.xzy / 2.0f + 0.5f), 1);
}


float4 PS_Main2(PS_Input input) : SV_TARGET
{
    return float4(0, 0, 0, 1);
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
