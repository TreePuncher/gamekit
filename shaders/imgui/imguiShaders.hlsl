[[fk::BeginRootSignatureDef(id=pipelineInterface)]]
[[fk::RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)]]
[[fk::DescriptorSet(SRVTexture(num=1))]]

[[fk::PushConstants(num=4)]] 
{
	uint2 WH;
};

[[fk::Texture2D(id=font, binding=0, set=0, type=float4)]]

sampler BiLinear : register(s0); // Nearest point
sampler NearestPoint : register(s1); // Nearest point


struct ImDrawVert
{
	float2 pos : POSITION;
	float2 uv : TEXCOORD;
	float4 col : COLOR;
};

struct PS_Point
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
	float4 col : COLOR;
};

[[fk::RootSignature(id=pipelineInterface)]]
PS_Point ImGui_VS(ImDrawVert vert)
{
	PS_Point output;
	output.pos	= float4(float2(-1, 1) + float2(vert.pos) / float2(WH) * float2(2, -2), 0, 1);
	output.uv	= vert.uv;
	output.col	= vert.col;
	return output;
}

float4 ImGui_PS(PS_Point fragment) : SV_TARGET
{
	float4 sample = font.Sample(BiLinear, fragment.uv);
	return fragment.col * sample;
}
