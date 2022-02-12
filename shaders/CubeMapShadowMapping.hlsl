struct ShadowMapData
{
    float4x4 ViewI;
	float4x4 PV;				
};

cbuffer PassConstants : register(b0)
{
    ShadowMapData matrices[6];
};

cbuffer LocalConstants : register(b1)
{
    float4x4 WT;
}

/*
cbuffer LocalConstants : register(b1)
{
    float4	 Albedo;
	float    Ks;
	float    IOR;
	float    Roughness;
	float    Anisotropic;
	float    Metallic;
    float4x4 WT;
}
*/
cbuffer PoseConstants : register(b2)
{
    float4x4 transforms[256];
}

cbuffer DrawConstants : register(b3)
{
    float4x4    PV;
    float4x4    View;
    uint        arrayTarget;
    float       maxZ;
};

struct Vertex
{
    float4 pos              : SV_POSITION;
    float3 pos_VS           : POSITION;
    uint   arrayTargetIndex : SV_RenderTargetArrayIndex;
};

Vertex VS_Main(float3 POS : POSITION)
{
    const float3 POS_WT = mul(WT, float4(POS, 1)).xyz;
    const float4 POS_DC = mul(PV, float4(POS_WT, 1));
    const float4 POS_VS = mul(View, float4(POS_WT, 1));

    Vertex Out;
    Out.pos                 = POS_DC;//float4(POS_DC.xy / POS_DC.w, -POS_VS.z / maxZ, 1);
    Out.pos_VS              = POS_VS;
    //Out.pos                 = mul(PV, mul(WT, float4(POS, 1)));
    Out.arrayTargetIndex    = arrayTarget;

    return Out;
}

struct Skinned_Vertex
{
    float3 POS		: POSITION;
    float3 Weights  : BLENDWEIGHT;
    uint4  Indices  : BLENDINDICES;
};

Vertex VS_Skinned_Main(Skinned_Vertex IN)
{
	float3 V = float3(0, 0, 0);
    const float4 W = float4(IN.Weights.xyz, 1 - IN.Weights.x - IN.Weights.y - IN.Weights.z);
    
	const float4x4 MTs[4] =
	{
		transforms[IN.Indices[0]],
		transforms[IN.Indices[1]],
		transforms[IN.Indices[2]],
		transforms[IN.Indices[3]],
    };

    V += (mul(MTs[0], float4(IN.POS, 1)) * W[0]).xyz;
    V += (mul(MTs[1], float4(IN.POS, 1)) * W[1]).xyz;
    V += (mul(MTs[2], float4(IN.POS, 1)) * W[2]).xyz;
    V += (mul(MTs[3], float4(IN.POS, 1)) * W[3]).xyz;

    const float3 POS_WT = mul(WT, float4(V.xyz, 1)).xyz;
    const float4 POS_DC = mul(PV, float4(POS_WT, 1));
    const float4 POS_VS = mul(View, float4(POS_WT, 1));

    Vertex Out;
    Out.pos                 = float4(POS_DC.xy, POS_VS.z / maxZ, 1);
    Out.arrayTargetIndex    = arrayTarget;

    return Out;
}


float PS_Main(Vertex IN, const bool frontFacing : SV_IsFrontFace) : SV_DEPTH
{
    if (frontFacing)
        return (-IN.pos_VS.z / maxZ) + 0.015f;
    else
        return (-IN.pos_VS.z / maxZ) - 0.00015f;
}
