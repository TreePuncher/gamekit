#ifndef ALGORITHMS_H
#define ALGORITHMS_H


//void __CmpSwap(uint lhs, uint rhs, uint op, inout uint array[1024])
#define __CmpSwap(lhs, rhs, op, array) \
{ \
    const uint LValue = array[lhs]; \
    const uint RValue = array[rhs]; \
	\
    const uint V1 = op == 0 ? min(LValue, RValue) : max(LValue, RValue); \
    const uint V2 = op == 0 ? max(LValue, RValue) : min(LValue, RValue); \
	\
    array[lhs] = V1; \
    array[rhs] = V2; \
}   \

//void BitonicPass(const uint localThreadID, const int I, const int J, inout uint array[1024])
#define BitonicPass(localThreadID, I, J, array) { \
    const uint swapMask = (1 << (J + 1)) - 1; \
    const uint offset   = 1 << J; \
\
    if((localThreadID & swapMask) < offset) \
    { \
        const uint op  = (localThreadID >> (I + 1)) & 0x01; \
        __CmpSwap(localThreadID, localThreadID + offset, op, array); \
    } \
\
    GroupMemoryBarrierWithGroupSync(); \
} \


// Pad extra elements with -1
//void LocalBitonicSort(const uint localThreadID, inout uint array[1024])
#define LocalBitonicSort(localThreadID, array) { \
	for(int I = 0; I < log2(1024); I++) \
        for(int J = I; J >= 0; J--) \
        	BitonicPass(localThreadID, I, J, array); \
} \


void ParallelPreFixSum(const uint threadID, inout uint array[1024], const uint blockSize)
{
	// https://developer.nvidia.com/gpugems/gpugems3/part-vi-gpu-computing/chapter-39-parallel-prefix-sum-scan-cuda
    GroupMemoryBarrierWithGroupSync();
	
	// Up-Sweep
    for(uint D = 0; D < log2(blockSize - 1); D++)
    {
		const uint step = pow(2, D + 1);
		const uint K 	= threadID * step;
		
		if(threadID < blockSize / step)
		{
			const uint lhsIdx = K + pow(2, D) - 1;
			const uint rhsIdx = K + step - 1;

			array[rhsIdx] = array[lhsIdx] + array[rhsIdx];
		}
        GroupMemoryBarrierWithGroupSync();
    }
	
	if(threadID == 0)
		array[blockSize - 1] = 0;

	GroupMemoryBarrierWithGroupSync(); 
	
	// Down-Sweep
	uint step = blockSize;
	for(int I = 0; I < log2(blockSize - 1); I++)
	{
		step = step >> 1;

		//GroupMemoryBarrierWithGroupSync(); 

		if(threadID <= 1 << I)
		{
			const uint ai = step * (threadID * 2 + 1) - 1;
			const uint bi = step * (threadID * 2 + 2) - 1;
			
			const uint temp = array[ai];

			array[ai]  = array[bi];
			array[bi] += temp;
		}

		GroupMemoryBarrierWithGroupSync(); 
	}
}

void LocalParallelRadixSort(const uint threadID, const uint threadCount, const uint blockSize, const uint blockCount, inout bool f[1024], inout uint offset[1024], inout uint totalFalses, RWStructuredBuffer<uint2> array)
{
    GroupMemoryBarrierWithGroupSync();
	
	for(uint bitIdx = 0; bitIdx < 32; bitIdx++)
	{
		for(uint I = 0; I < blockCount; I++)
		{
			const uint Idx 		= threadCount * I + threadID;
			const uint mask 	= 0x01 << bitIdx;
			const uint value 	= array[Idx];
			
			const uint bit 		= (mask & value) != 0;
			f[Idx] 				= 1 - bit;
			offset[Idx] 		= 1 - bit;		
		}

		ParallelPreFixSum(threadID, offset, blockSize);
		
		if(threadID == 0)
			totalFalses = f[blockSize - 1] + offset[blockSize - 1];
		
		// move values to new locations using the offsetBuffer
		// Load Values
        // max for thread Blocks
		uint temp[4];	
		for(uint I = 0; I < blockCount; I++)
		{
			const uint Idx 	= threadCount * I + threadID;
			temp[I] 		= array[Idx];
		}

		for(uint I = 0; I < blockCount; I++)
		{
			const uint Idx 	= threadCount * I + threadID;
			
			if(f[Idx])
				array[offset[Idx]] = temp[I];
			else
				array[offset[Idx] + totalFalses] = temp[I];
		}
		
		GroupMemoryBarrierWithGroupSync(); 
	}

    /*
	// Write Debug output
	// Dump contents of texture samples

	#if 1
	for(uint I = 0; I < BlockSize / ThreadCount; I++)
	{
		const uint destIdx = ThreadCount * I + threadID;
		debugArray[destIdx] = localTextureSamples[destIdx];
	}
	#endif


	#if 0
	// Dump offsets to debug array
	for(uint I = 0; I < BlockSize / ThreadCount; I++)
    {
        const uint Idx = ThreadCount * I + threadID;
		debugArray[Idx] = f[Idx];
	}
	#endif 

    GroupMemoryBarrierWithGroupSync();
    */
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

#endif