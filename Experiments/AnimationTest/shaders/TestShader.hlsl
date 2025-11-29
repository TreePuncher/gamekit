[[fk::RootFlag(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)]]
[[fk::DescriptorSet(set=0,
	CBV(num=1)
)]]

[[fk::CBV(binding=0, set=0)]]
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

VOut VMain(VIN vin, uint vertexID : SV_VertexID)
{
	VOut OUT;
	OUT.position	= float4(vin.xyz, 1);
	OUT.uvw			= vin.xyz / 2.0f + 0.5f;

	return OUT;
}

float4 PMain(VOut vin) : SV_Target
{
	return
        float4(
            sin(time) / 2.0f + 0.5f,
            cos(time) / 2.0f + 0.5f,
            vin.position.y / 553.0f,
            1.0f);
}
