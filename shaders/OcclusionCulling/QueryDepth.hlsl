#define RS1		"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),"		\
				"RootConstants(num32BitConstants = 16, b0)"

cbuffer vertex : register(b0)
{
	float4x4 transform;
};

float3 GetCubeVert(in const uint vertexID)
{
	const int tri	= vertexID / 3;
	const int idx	= vertexID % 3;
	const int face = tri / 2;
	const int top	= tri % 2;

	const int dir = face % 3;
	const int pos = face / 3;
	
	const int nz = dir >> 1;
	const int ny = dir & 1;
	const int nx = 1 ^ (ny | nz);

    const float3	d		= float3(nx, ny, nz);
    const float		flip	= 1.0f - 2.0f * pos;

    const float3	n = flip * d;
    const float3	u = -d.yzx;
    const float3	v = flip * d.zxy;

    const float mirror	= -1.0f + 2.0f * top;
    const float3 xyz	= n + mirror * (1.0f - 2.0f * (idx & 1)) * u + mirror * float(1.0f - 2.0f * (idx >> 1)) * v;

	return xyz;
}

[RootSignature(RS1)]
float4 VMain(uint vertexID : SV_VertexID) : SV_POSITION
{
	return mul(transform, float4(GetCubeVert(vertexID), 1.0f));
}

struct InstancedTarget
{
	uint   target	: TARGET;
	float4 position : SV_POSITION;
};

[RootSignature(RS1)]
InstancedTarget VMainInstanced(uint vertexID : SV_VertexID, float3 pos : POSITION, uint target : TARGET)
{
	InstancedTarget OUT;
	OUT.target		= target;
	OUT.position	= mul(transform, float4(GetCubeVert(vertexID) + pos, 1.0f));

	return OUT;
}

[earlydepthstencil]
void PMain()
{
}
