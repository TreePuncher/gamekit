cbuffer LocalConstants : register(b1)
{

}

// XY - SIZE
// Z  - ID
// W  - PADDING
cbuffer PassConstants : register(b2)
{
    uint2 bufferSize;
}


/************************************************************************************************/


RWStructuredBuffer<uint>    UAVCounters 	: register(u0);
RWStructuredBuffer<uint2>   textureSamples 	: register(u1);
RWStructuredBuffer<uint>    segmentSizes  	: register(u2);

//#if  _DEBUG
RWStructuredBuffer<uint>    debugArray  : register(u3);
//#endif


/************************************************************************************************/


#define BlockSize 1024
#define ThreadCount (BlockSize / 2)
#define BlockCount BlockSize / ThreadCount

groupshared uint2   localTextureSamples[BlockSize];
groupshared uint    offset[BlockSize];


/************************************************************************************************/


uint2 min2(const uint2 lhs, const uint2 rhs)
{
	if(lhs.x == rhs.x)
		return uint2(lhs.x, min(lhs.y, rhs.y));

    if(lhs.x < rhs.x)
        return lhs;
    else
        return rhs;
}


/************************************************************************************************/


uint2 max2(const uint2 lhs, const uint2 rhs)
{
	if(lhs.x == rhs.x)
		return uint2(lhs.x, max(lhs.y, rhs.y));

    if(lhs.x < rhs.x)
        return rhs;
    else
        return lhs;
}


/************************************************************************************************/


void __CmpSwap(uint lhs, uint rhs, uint op)
{
    const uint2 LValue = localTextureSamples[lhs];
    const uint2 RValue = localTextureSamples[rhs];
	
    const uint2 V1 = op == 0 ? min2(LValue, RValue) : max2(LValue, RValue);
    const uint2 V2 = op == 0 ? max2(LValue, RValue) : min2(LValue, RValue);
	
    localTextureSamples[lhs] = V1;
    localTextureSamples[rhs] = V2;
}


void BitonicPass(const uint localThreadID, const int I, const int J)
{
    const uint swapMask = (1 << (J + 1)) - 1;
    const uint offset   = 1 << J;

    if((localThreadID & swapMask) < offset)
    {
        const uint op  = (localThreadID >> (I + 1)) & 0x01;
        __CmpSwap(localThreadID, localThreadID + offset, op);
    }

    GroupMemoryBarrierWithGroupSync();
}


/************************************************************************************************/


// Pad extra elements with -1
void LocalBitonicSort(const uint localThreadID)
{
	for(int I = 0; I < log2(1024); I++)
        for(int J = I; J >= 0; J--)
        	BitonicPass(localThreadID, I, J);
}


/************************************************************************************************/


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

			offset[rhsIdx] = offset[lhsIdx] + offset[rhsIdx];
		}
		GroupMemoryBarrierWithGroupSync();
	}
	
	if(threadID == 0)
		offset[BlockSize - 1] = 0;

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
			
			const uint temp = offset[ai];

			offset[ai]  = offset[bi];
			offset[bi] += temp;
		}

		GroupMemoryBarrierWithGroupSync(); 
	}

	GroupMemoryBarrierWithGroupSync(); 
}


/************************************************************************************************/


void CompactSamples(const uint localThreadID)
{	
	GroupMemoryBarrierWithGroupSync();

    const uint waveCount = BlockSize / ThreadCount;      
    const uint waveSize  = BlockSize / waveCount;

	const uint idx  = localThreadID;
	const uint2 A 	= localTextureSamples[idx];
	const uint2 B 	= localTextureSamples[idx - 1];
	
	const uint m = idx != 0 ? !(A.y == B.y && A.x == B.x) : 1;
	offset[idx] = m != 0;
	
	GroupMemoryBarrierWithGroupSync();

    ParallelPreFixSum(localThreadID);		

    GroupMemoryBarrierWithGroupSync();

	if(m)
		localTextureSamples[offset[idx]] = A;

	GroupMemoryBarrierWithGroupSync();
}


/************************************************************************************************/


void LoadLocalValues(const uint threadID, const uint groupID, const uint sampleCount)
{
	uint2 tileID = uint2(-1, -1);

	if(threadID + groupID * BlockSize < sampleCount){
		tileID = textureSamples[threadID + groupID * BlockSize].xy;
    	tileID = (tileID != uint2(0, 0)) ? tileID : uint2(-1, -1);
	}

	localTextureSamples[threadID] = tileID;

	GroupMemoryBarrierWithGroupSync();
}


/************************************************************************************************/


void WriteBackValues(const uint threadID, const uint groupID)
{
    GroupMemoryBarrierWithGroupSync();

    const uint end = offset[BlockSize - 1];
	
	if(threadID < end)
		textureSamples[threadID + groupID * BlockSize] = localTextureSamples[threadID];
	else
		textureSamples[threadID + groupID * BlockSize] = uint2(-1, -1);

    if(threadID == 0)
        segmentSizes[groupID] = end - (localTextureSamples[BlockSize - 1] == uint2(-1, -1) ? 1 : 0);

    GroupMemoryBarrierWithGroupSync();
}


/************************************************************************************************/


[numthreads(1024, 1, 1)]
void CompressBlocks(const uint threadID : SV_GroupIndex, const uint3 groupID : SV_GROUPID)
{
	const uint sampleCount = UAVCounters[0];
	if(sampleCount < groupID.x * BlockSize)
	{
		segmentSizes[groupID.x] = 0;
		return;
	}

    LoadLocalValues(threadID, groupID.x, sampleCount);
    LocalBitonicSort(threadID);
    CompactSamples(threadID);
    WriteBackValues(threadID, groupID.x);
}


/************************************************************************************************/


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
