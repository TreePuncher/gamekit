#include "common.hlsl"

struct Vertex
{
    float3 POS		    : POSITION;
    float3 Normal	    : NORMAL;
    float3 Tangent	    : Tangent;
    float4 instancePOS  : INSTANCEPOS;
    float4 params       : INSTANCEARGS;
};

struct Forward_VS_OUT
{
    float4 POS 		: SV_POSITION;
    float3 WPOS 	: POSITION;
    float3 Normal	: NORMAL;
    float3 Tangent	: TANGENT;
    float3 Bitangent : BITANGENT;
};

Forward_VS_OUT ParticleMeshInstanceVS(Vertex In)
{
    Forward_VS_OUT Out;

    Out.WPOS	    = float4(0.2f, 0.2f, 0.2f, 1.0f) * In.POS + In.instancePOS;
    Out.POS		    = mul(PV, float4(Out.WPOS, 1));
    Out.Normal      = mul(View, float4(In.Normal, 0.0f));
    Out.Tangent     = mul(View, float4(In.Tangent, 0.0f));
    Out.Bitangent   = cross(Out.Tangent, Out.Normal);

    return Out;
}

struct Deferred_OUT
{
    float4 Albedo       : SV_TARGET0;
    float4 MRIA         : SV_TARGET1;
    float4 Normal       : SV_TARGET2;

    float Depth         : SV_DepthLessEqual;
};

struct Forward_PS_IN
{
    centroid    float4	POS         : SV_POSITION;
                float3	WPOS 	    : POSITION;
                float3	Normal	    : NORMAL;
                float3	Tangent	    : TANGENT;
                float3	Bitangent	: BITANGENT;
};

Deferred_OUT ParticleMeshInstancePS(Forward_PS_IN IN)
{
    Deferred_OUT Out;
    Out.Albedo  = float4(0.5f, 0.5f, 0.5f, 0);
    Out.MRIA    = float4(0, 0.5f, 0, 0);
    Out.Normal  = float4(normalize(IN.Normal), 0);
    Out.Depth   = length(IN.WPOS - CameraPOS.xyz) / MaxZ;

    return Out;
}

struct VertexDepth_IN
{
    float3 POS		        : POSITION;
    float4 instancePOS      : INSTANCEPOS;
    float4 params           : INSTANCEARGS;
};



float4 ParticleMeshInstanceDepthVS(VertexDepth_IN In) : SV_POSITION
{
    return float4(0.4f, 0.4f, 0.4f, 1.0f) * float4(In.POS, 1) + In.instancePOS;
}

struct ShadowMapData
{
    float4x4 ViewI;
	float4x4 PV;				
};

cbuffer PassConstants : register( b1 )
{
    ShadowMapData matrices[64];
};

struct DepthVertex
{
    float4 pos              : SV_POSITION;
    uint   arrayTargetIndex : SV_RenderTargetArrayIndex;
};

[maxvertexcount(18)]
void ParticleMeshInstanceDepthGS(
    triangle float4 vertices[3] : SV_POSITION,
    inout TriangleStream<DepthVertex> renderTarget)
{
    for (uint face_ID = 0; face_ID < 6; face_ID++) // Loop over faces
    {
        for(uint triangle_ID = 0; triangle_ID < 3; triangle_ID++)
        {
            DepthVertex Out;
            Out.pos                 = mul(matrices[face_ID].PV, vertices[triangle_ID]);
            Out.arrayTargetIndex    = face_ID;

            renderTarget.Append(Out);
        }

        renderTarget.RestartStrip();
    }
}