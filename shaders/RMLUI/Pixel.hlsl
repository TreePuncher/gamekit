#include "Shared.hlsl"

float4 PMain(Vertex v) : SV_Target0
{
	return v.color;
}


Texture2D<float4>	Font			: register(t0);
SamplerState		defaultSampler	: register(s0);

float4 TexturedPMain(Vertex v) : SV_Target0
{
	float4 s = Font.Sample(defaultSampler, v.uv);
	
	return v.color * s;
}
