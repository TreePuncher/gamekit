Texture2DArray<float>		input	: register(s0);
RWTexture2DArray<float>		output : register(u0);

[numthreads(32, 32, 1)]
void CS_Main(
	const int3	groupID     : SV_GroupThreadID, 
	const int3	group       : SV_GroupID, 
	uint3		globalID    : SV_DispatchThreadID, 
	uint		threadID    : SV_GroupIndex)
{
	const uint arrayIdx			= globalID.z;
	float f = 0;


	static const int blurWidth = 10;
	for (int x = -blurWidth / 2; x < blurWidth / 2; x++)
		for (int y = -blurWidth / 2; y < blurWidth / 2; y++)
				f += input.Load(int4(globalID.xy + int2(x, y), arrayIdx, 0)) / (blurWidth * blurWidth);

	const int2 outputOffset	= int2(1 + (group.x > 7), 1 + (group.y > 7));

	output[int3(outputOffset + globalID.xy, arrayIdx)] = f;
}
