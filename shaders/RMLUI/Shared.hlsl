#define RS0 "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),"\
			"RootConstants(num32BitConstants = 20, b0),"\
			"DescriptorTable(SRV(t0, flags = DATA_STATIC)),"\
			"StaticSampler(s0, " \
				"addressU = TEXTURE_ADDRESS_CLAMP, "\
				"filter = FILTER_MIN_MAG_MIP_LINEAR )"

struct Vertex
{
	float4	pos		: SV_Position;
	float2	uv		: TEXCOORD;
	float4  color	: COLOR;
};

cbuffer constants : register(b0)
{
	float4x4 transform;
	uint     w;
	uint     h;
	float	 offsetX;
	float	 offsetY;
};
