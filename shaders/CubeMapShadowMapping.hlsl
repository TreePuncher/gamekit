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
