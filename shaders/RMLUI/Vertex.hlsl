#include "Shared.hlsl"

[RootSignature(RS0)]
Vertex VMain(half2 POS : POSITION, half2 UV : TEXCOORD, float4 color : COLOR)
{
	Vertex Out;

	float2 XY		= mul(transform, float4(POS.x, POS.y, 0, 1)).xy;
	float2 XY_SS	= float2(-1 + 2 * ((XY.x + offsetX) / w), 1 - 2 * ((XY.y + offsetY) / h));
	
	Out.pos		= float4(XY_SS, 1.0f, 1.0f);
	Out.uv		= UV;
	Out.color	= color;
	
	return Out;
}
