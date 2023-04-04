RWTexture2DArray<float>		target : register(u0);

#define BLOCKSIZE 1024

groupshared float array[BLOCKSIZE];

void ParallelPreFixSum(const uint threadID)
{
	// https://developer.nvidia.com/gpugems/gpugems3/part-vi-gpu-computing/chapter-39-parallel-prefix-sum-scan-cuda
	GroupMemoryBarrierWithGroupSync();

	// Up-Sweep
	for (uint D = 0; D < log2(BLOCKSIZE - 1); D++)
	{
		const uint step = pow(2, D + 1);
		const uint K = threadID * step;

		if (threadID < BLOCKSIZE / step)
		{
			const uint lhsIdx = K + pow(2, D) - 1;
			const uint rhsIdx = K + step - 1;

			array[rhsIdx] = array[lhsIdx] + array[rhsIdx];
		}

		GroupMemoryBarrierWithGroupSync();
	}

	if (threadID == 0)
		array[BLOCKSIZE - 1] = 0;

	GroupMemoryBarrierWithGroupSync();

	// Down-Sweep
	uint step = BLOCKSIZE;
	for (int I = 0; I < log2(BLOCKSIZE - 1); I++)
	{
		step = step >> 1;

		if (threadID <= 1 << I)
		{
			const uint ai = step * (threadID * 2 + 1) - 1;
			const uint bi = step * (threadID * 2 + 2) - 1;

			const float temp = array[ai];

			array[ai] = array[bi];
			array[bi] += temp;
		}

		GroupMemoryBarrierWithGroupSync();
	}
}

[numthreads(1024, 1, 1)]
void RowSumsMain(
	const int3	groupID		: SV_GroupThreadID, 
	const int3	group		: SV_GroupID, 
	uint3		globalID	: SV_DispatchThreadID, 
	uint		threadID	: SV_GroupIndex)
{
	const uint row		= group.y;

	float temp		= target.Load(int4(threadID, row, 0, 0));
	array[threadID]	= temp;

	GroupMemoryBarrierWithGroupSync();

	ParallelPreFixSum(threadID);

	GroupMemoryBarrierWithGroupSync();

	temp = array[threadID];
	target[int3(threadID, row, 0)] = temp;
}


[numthreads(1024, 1, 1)]
void ColumnSumsMain(
	const int3	groupID		: SV_GroupThreadID,
	const int3	group		: SV_GroupID,
	uint3		globalID	: SV_DispatchThreadID,
	uint		threadID	: SV_GroupIndex)
{
	const uint column	= group.x;

	float temp		= target.Load(int4(column, threadID, 0, 0));
	array[threadID] = temp;

	GroupMemoryBarrierWithGroupSync();

	ParallelPreFixSum(threadID);

	GroupMemoryBarrierWithGroupSync();

	target[int3(column, threadID, 0)] = array[threadID];
}
