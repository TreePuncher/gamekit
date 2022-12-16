StructuredBuffer<uint> 		segmentSizes 	: register(t0);
RWStructuredBuffer<uint>  	segmentOffsets	: register(u0);

#define BlockSize 512
groupshared uint offsets[BlockSize];

void ParallelPreFixSum(const uint threadID)
{
	// https://developer.nvidia.com/gpugems/gpugems3/part-vi-gpu-computing/chapter-39-parallel-prefix-sum-scan-cuda
    GroupMemoryBarrierWithGroupSync();
	
	// Up-Sweep
	for(uint D = 0; D < log2(BlockSize - 1); D++)
	{
		const uint step = pow(2, D + 1);
		const uint K 	= threadID * step;
		
		if(threadID < BlockSize / step)
		{
			const uint lhsIdx = K + pow(2, D) - 1;
			const uint rhsIdx = K + step - 1;

			offsets[rhsIdx] = offsets[lhsIdx] + offsets[rhsIdx];
		}
		GroupMemoryBarrierWithGroupSync();
	}
	
	if(threadID == 0)
		offsets[BlockSize - 1] = 0;

	GroupMemoryBarrierWithGroupSync(); 
	
	// Down-Sweep
	uint step = BlockSize;
	for(int I = 0; I < log2(BlockSize - 1); I++)
	{
		step = step >> 1;

		if(threadID <= 1 << I)
		{
			const uint ai = step * (threadID * 2 + 1) - 1;
			const uint bi = step * (threadID * 2 + 2) - 1;
			
			const uint temp = offsets[ai];

			offsets[ai]  = offsets[bi];
			offsets[bi] += temp;
		}

		GroupMemoryBarrierWithGroupSync(); 
	}

	GroupMemoryBarrierWithGroupSync(); 
}

[numthreads(BlockSize, 1, 1)]
void PreFixSumBlockSizes(const uint threadID : SV_GroupIndex)
{
	offsets[threadID] = segmentSizes[threadID];

	GroupMemoryBarrierWithGroupSync();

	ParallelPreFixSum(threadID);

	GroupMemoryBarrierWithGroupSync();

	segmentOffsets[threadID] = offsets[threadID];
}

/**********************************************************************

Copyright (c) 2015 - 2020 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/