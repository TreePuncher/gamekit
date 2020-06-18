


struct VS_Input
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
};

struct PS_Input
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
};

PS_Input VS_Main(VS_Input input)
{
    PS_Input output;
    output.position = float4(input.position, 1);
    output.normal = input.normal;
    output.texcoord = input.texcoord;

    return output;
}

struct PatchConstants_Output
{
    float Edges[4]        : SV_TessFactor;
    float Inside[2]       : SV_InsideTessFactor;
};

PatchConstants_Output PatchConstants(
    InputPatch<PS_Input, 4> patch,
    uint PatchID : SV_PRIMITIVEID
)
{
    PatchConstants_Output output;
    output.Edges[0] = 2.0f;
    output.Edges[1] = 2.0f;
    output.Edges[2] = 2.0f;
    output.Edges[3] = 2.0f;

    output.Inside[0] = 2.0f;
    output.Inside[1] = 2.0f;

    return output;
}

struct Gregory_CP
{
    float3 position : POINT;
    float3 normal : NORMAL;
};

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("PatchConstants")]
Gregory_CP HS_Main(
    InputPatch<PS_Input, 4> ip,
    uint i : SV_OutputControlPointID)
{
    Gregory_CP controlPoint;
    controlPoint.position = ip[i].position;
    controlPoint.normal = ip[i].normal;

    return controlPoint;
}

[domain("quad")]
PS_Input DS_Main(
    PatchConstants_Output constants,
    float2 UV : SV_DOMAINLOCATION,
    const OutputPatch<Gregory_CP, 4> patch)
{
    const float3 p1 = patch[0].position;
    const float3 p2 = patch[1].position;
    const float3 p3 = patch[2].position;
    const float3 p4 = patch[3].position;

    const float3 n1 = patch[0].normal;
    const float3 n2 = patch[1].normal;
    const float3 n3 = patch[2].normal;
    const float3 n4 = patch[3].normal;

    PS_Input output;
    output.position = float4(lerp(lerp(p1, p2, UV.x), lerp(p3, p4, UV.x), UV.y), 1);
    output.normal = lerp(lerp(n1, n2, UV.x), lerp(n3, n4, UV.x), UV.y);
    output.texcoord = UV;

    return output;
}

float4 PS_Main(PS_Input input) : SV_TARGET
{
    return float4(input.texcoord.xy, 1, 0);
}