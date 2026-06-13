[[fk::BeginRootSignatureDef(id=rootsig1)]]
[[fk::RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)]]
[[fk::DescriptorSet(CBV(num=unbounded))]]

[[fk::CBV(id=rootsig1)]]
{
    float4x4 View;
    float4x4 ViewI;
    float4x4 Proj;
    float4x4 PV;
};

[[fk::PushConstants(num=4)]] 
{
    float time;
};

struct VIN
{
	[[vk::location(0)]] float3 pos : POSITION;
	[[vk::location(1)]] float3 normal : NORMAL;
};

struct VOut
{
	[[vk::location(0)]] float4 position : SV_Position;
	[[vk::location(1)]] float3 uvw : UVW;
	[[vk::location(2)]] float3 n : NORMAL;
};

VOut VMain(VIN vin)
{
	VOut OUT;
	OUT.position = mul(PV, float4(vin.pos, 1));
	OUT.n = mul(PV, float4(vin.normal, 0.0f));
	OUT.uvw = vin.pos / 2.0f + 0.5f;

	return OUT;
}

[[fk::RootSignature(id=rootsig1)]]
float4 PMain(VOut vin) : SV_Target
{
	return float4(vin.n / 2.0f + 0.5f, 1.0f);
}
