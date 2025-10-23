#define RS0 "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)"

//						Texture2D<float4>	diffuse			: register(t2);
//[[vk::binding(0, 2)]]	sampler				defaultSampler	: register(s0);

struct PushConstants
{
	float4 asdfdsa;
	float4 RGBA;
};

//[[vk::push_constant]] PushConstants pushConstants;

cbuffer cb0 : register(b0)
{
	float4 xyzw;
};

cbuffer cb1 : register(b1)
{
	float4 asdf;
};

cbuffer cb0_1 : register(b0, space1)
{
	float4 posw;
};

struct VIN
{
	[[vk::location(0)]] float4 xyz : POSITION;
};

struct VOut
{
	[[vk::location(0)]] float4 position : SV_Position;
	[[vk::location(1)]] float3 uvw : UVW;
};

[RootSignature(RS0)]
VOut VMain(uint vertexID : SV_VertexID)
{
	const static float3 tri[3] =
	{
		float3(-1.0f, -1.0f, 0.0f),
		float3(0.0f, 1.0f, 0.0f),
		float3(1.0f, -1.0f, 0.0f),
	};
	
	VOut OUT;
	OUT.position = float4(tri[vertexID], 1);
	OUT.uvw = tri[vertexID] / 2.0f + 0.5f;

	return OUT;
}


float4 PMain(VOut vin) : SV_Target
{
	//return float4(vin.position + pushConstants.asdfdsa * pushConstants.RGBA + xyzw + asdf + posw) + diffuse.Sample(defaultSampler, float2(0.5, 0.5));
	return float4(
		0,
		vin.uvw.y,
		vin.position.y / 553.0f,
		1.0f);
}
