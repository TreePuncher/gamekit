Texture2DArray<float>		input	: register(s0);
RWTexture2DArray<float>		output : register(u0);

[numthreads(32, 32, 1)]
void ColumnSumsMain(
	const int3	groupID     : SV_GroupThreadID, 
	const int3	group       : SV_GroupID, 
	uint3		globalID    : SV_DispatchThreadID, 
	uint		threadID    : SV_GroupIndex)
{
	const uint arrayIdx			= group.z;
	float f = 0;

	const uint quadrant = (group.x > 15) + ((group.y > 15) ? 2 : 0);

	static const int blurWidth = 0;
	for (int x = -blurWidth / 2; x < blurWidth / 2; x++)
		for (int y = -blurWidth / 2; y < blurWidth / 2; y ++)
				f += input.Load(int4(globalID.xy + int2(x, y), arrayIdx, 0)) / (blurWidth * blurWidth);

	if(blurWidth == 0)
		f = input.Load(int4(globalID.xy, arrayIdx, 0));

	const int2 outputOffset	= int2(1 + (group.x > 15), 1 + (group.y > 15));

	//-, +
	//+, -

	switch (quadrant)
	{
	case 0: // Upper Left
	{
		output[int3(outputOffset + globalID.xy, arrayIdx)] = f;
	}	break;
	case 1: // Upper Right
	{
		output[int3(outputOffset + globalID.xy, arrayIdx)] = f;
	}	break;
	case 2: // Lower Left
	{
		output[int3(outputOffset + globalID.xy, arrayIdx)] = f;
	}	break;
	case 3: // Lower Right
	{
		output[int3(outputOffset + globalID.xy, arrayIdx)] = f;
	}	break;
	}
}
