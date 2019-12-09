#include "common.hlsl"


struct InstancedIN
{
	float3 POS		: POSITION;
	float2 T		: TEXCOORD0;
	float3 N		: NORMAL;
	float3 Tangent	: TANGENT;
	float2 UV		: TEXCOORD;
	float4x4 WT 	: INSTANCEWT;
};

struct VOUT
{
	float4 POS 		: SV_POSITION;
	float4 WPOS		: POSITION0;
	float3 NORMAL	: NORMAL;
	float3 Tangent	: TANGENT;
	float2 UV		: TEXCOORD;
};

VOUT VMain(InstancedIN In )
{
	VOUT Out;
	Out.WPOS 	= mul(In.WT, float4(In.POS, 1));
	Out.POS 	= mul(PV, mul(In.WT, float4(In.POS, 1))); 
	Out.NORMAL  = mul(In.WT, float4(In.N, 0));
	Out.Tangent = mul(In.WT, float4(In.Tangent, 0));
	Out.UV		= In.UV;
	return Out;
}