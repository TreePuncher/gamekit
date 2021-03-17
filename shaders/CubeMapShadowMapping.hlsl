struct ShadowMapData
{
    float4x4 ViewI;
	float4x4 PV;				
};

cbuffer PassConstants : register( b0 )
{
    ShadowMapData matrices[6];
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
}

float4 VS_Main(float3 POS : POSITION) : SV_POSITION
{
    return mul(WT, float4(POS, 1));
}

struct Skinned_Vertex
{
    float3 POS		: POSITION;
    float3 Weights  : BLENDWEIGHT;
    uint4  Indices  : BLENDINDICES;
};

cbuffer PoseConstants : register(b3)
{
    float4x4 transforms[256];
}

float4 VS_Skinned_Main(Skinned_Vertex IN) : SV_POSITION
{
	float3 V = float3(0, 0, 0);
    const float4 W = float4(IN.Weights.xyz, 1 - IN.Weights.x - IN.Weights.y - IN.Weights.z);
    
	float4x4 MTs[4] =
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

    return mul(WT, float4(V.xyz, 1));
}

struct Vertex
{
    float4 pos              : SV_POSITION;
    uint   arrayTargetIndex : SV_RenderTargetArrayIndex;
};

[maxvertexcount(18)]
void GS_Main(
    triangle float4 vertices[3] : SV_POSITION,
    inout TriangleStream<Vertex> renderTarget)
{
    for (uint face_ID = 0; face_ID < 6; face_ID++) // Loop over faces
    {
        for(uint triangle_ID = 0; triangle_ID < 3; triangle_ID++)
        {
            Vertex Out;
            Out.pos                 = mul(matrices[face_ID].PV, vertices[triangle_ID]);
            Out.arrayTargetIndex    = face_ID;

            renderTarget.Append(Out);
        }

        renderTarget.RestartStrip();
    }
}
