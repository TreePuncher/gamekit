RWStructuredBuffer<uint2> textureSamples_Out 	: register(u0);
RWStructuredBuffer<uint> feedbackCounters		: register(u1); // readback to CPU

StructuredBuffer<uint2>	textureSamples_In	: register(t0);
StructuredBuffer<uint>  requestBlockSizes	: register(t1);
StructuredBuffer<uint>  requestBlockOffsets : register(t2);

[numthreads(1024, 1, 1)]
void MergeBlocks(const uint threadID : SV_GroupIndex, const uint3 blockIdx : SV_GROUPID)
{
	const uint sampleCount		= requestBlockOffsets[255];       
    const uint blockOffset      = requestBlockOffsets[blockIdx.x];
	const uint localBlockSize 	= requestBlockSizes[blockIdx.x];
	const uint localBlockOffset = threadID;
	const uint inputBlockOffset = blockIdx.x * 1024 + localBlockOffset;
	const uint outputOffset		= localBlockOffset + blockOffset;
	const uint2 request 		= textureSamples_In[inputBlockOffset];

	GroupMemoryBarrierWithGroupSync();

	if(localBlockOffset < localBlockSize)
		textureSamples_Out[outputOffset] = request;

	GroupMemoryBarrierWithGroupSync();
}

[numthreads(256, 1, 1)]
void SetBlockCounters(const uint threadID : SV_GroupIndex)
{
	const int sampleCount 		= requestBlockOffsets[255];
	const int localBlockCount	= sampleCount * 1024 > threadID * 1024 ? 1024 : ((sampleCount / 1024 == threadID) ? sampleCount % 1024 : 0);

	if(threadID == 0)
		feedbackCounters[0] = sampleCount;
	else
		feedbackCounters[threadID] = min(localBlockCount, 1024);
}

/**********************************************************************

Copyright (c) 2021 Robert May

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