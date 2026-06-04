[[fk::BeginRootSignatureDef(id=rootsig1)]]
[[fk::RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)]]
[[fk::PushConstants(num=4)]] 
{
    float time;
};

struct VIN
{
	[[vk::location(0)]] float3 xyz : POSITION;
};

struct VOut
{
	[[vk::location(0)]] float4 position : SV_Position;
	[[vk::location(1)]] float3 uvw : UVW;
};

VOut VMain(VIN vin)
{
	VOut OUT;
	OUT.position	= float4(vin.xyz, 1);
	OUT.uvw			= vin.xyz / 2.0f + 0.5f;

	return OUT;
}

[[fk::RootSignature(id=rootsig1)]]
float4 PMain(VOut vin) : SV_Target
{
	return
        float4(
            sin(time) / 2.0f + 0.5f,
            cos(time) / 2.0f + 0.5f,
            vin.position.y / 553.0f,
            1.0f);
}
