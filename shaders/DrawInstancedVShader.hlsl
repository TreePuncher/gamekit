#include "common.hlsl"


struct InstancedIN
{
	float3 POS		: POSITION;
	float2 UV		: TEXCOORD;
	float3 N		: NORMAL;
	float3 Tangent	: TANGENT;
	float4x4 WT 	: INSTANCEWT;
};

struct VOUT
{
	float4 POS 		: SV_POSITION;
    float  depth    : DEPTH;
	float3 Normal	: NORMAL;
	float3 Tangent	: TANGENT;
	float2 UV		: TEXCOORD;
    float3 Bitangent : BITANGENT;
};

VOUT VMain(InstancedIN In )
{
	VOUT Out;
	//Out.WPOS 		= mul(In.WT, float4(In.POS, 1));
	Out.POS 		= mul(PV, mul(In.WT, float4(In.POS, 1))); 
	Out.Normal  	= mul(In.WT, float4(In.N, 0));
	Out.Tangent 	= mul(In.WT, float4(In.Tangent, 0));
	Out.UV			= In.UV;
    Out.Bitangent 	= cross(Out.Tangent, Out.Normal);
    Out.depth       = abs(-mul(View, mul(In.WT, float4(In.POS, 1))).z / MaxZ);

	return Out;
}
