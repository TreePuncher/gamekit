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
};

VOut VMain()
{
	VOut OUT;
	OUT.position = float4(0, 1, 2, 1);
	
	return OUT;
}

float4 PMain(VOut vin) : SV_Target
{
	//return float4(vin.position + pushConstants.asdfdsa * pushConstants.RGBA + xyzw + asdf + posw) + diffuse.Sample(defaultSampler, float2(0.5, 0.5));
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
