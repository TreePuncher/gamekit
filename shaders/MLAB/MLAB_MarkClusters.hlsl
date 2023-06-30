#include "common.hlsl"

#define rootSig	"RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | " \
						 "DENY_VERTEX_SHADER_ROOT_ACCESS), " \
				"CBV(b0, space = 1, flags = DATA_STATIC)" 

globallycoherent RWTexture2D<uint2> ClusterLayout : register(u0);


uint GetSliceIdx(float z)
{
#if 1
	const float numSlices	= 24;
	const float MinOverMax	= MaxZ / MinZ;
	const float LogMoM		= log(MinOverMax);

	return (log(z) * numSlices / LogMoM) - (numSlices * log(MinZ) / LogMoM);
#else
	return z / MaxZ * 24;
#endif
}

[RootSignature(rootSig)]
void MarkCluster(uint2 clusterXY : SV_Position, float depth : DEPTH)
{
	InterlockedOr(ClusterLayout[clusterXY].x, 0x01 << GetSliceIdx(depth));
}
