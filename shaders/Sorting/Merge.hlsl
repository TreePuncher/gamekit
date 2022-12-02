StructuredBuffer<uint>		sourceBuffer	: register(t0);
StructuredBuffer<uint2>		mergePathTable	: register(t1);
RWStructuredBuffer<uint>	outputBuffer	: register(u0);

cbuffer constants : register(b0)
{
	uint p;
	uint blockSize;
	uint blockCount;
}


/************************************************************************************************/

groupshared uint local[1024];

void __CmpSwap(uint lhs, uint rhs, uint op)
{ 
	const uint LValue = local[lhs];
	const uint RValue = local[rhs];

	const uint V1 = op == 0 ? min(LValue, RValue) : max(LValue, RValue);
	const uint V2 = op == 0 ? max(LValue, RValue) : min(LValue, RValue);

	local[lhs] = V1;
	local[rhs] = V2;
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


void BitonicSort(const uint localThreadID)
{ 
	for(int I = 0; I < log2(1024); I++) 
		for(int J = I; J >= 0; J--) 
			BitonicPass(localThreadID, I, J);
} 


/************************************************************************************************/


[numthreads(1024, 1, 1)]
void GlobalMerge(const uint3 dispatchID : SV_DispatchThreadID, const uint3 groupID : SV_GroupID, const uint groupIdx : SV_GroupIndex)
{
	const uint2 begin	= mergePathTable[groupID.x];
	const uint2 end		= mergePathTable[groupID.x + 1];

	if (begin.x <= dispatchID.x && dispatchID.x < end.x)
		local[groupIdx] = sourceBuffer[begin.x + groupIdx + (groupID.x / 2 + 0) * blockSize];
	else
		local[groupIdx] = sourceBuffer[begin.y + groupIdx + (groupID.x / 2 + 1) * blockSize];

	GroupMemoryBarrierWithGroupSync();

	BitonicSort(groupIdx);

	GroupMemoryBarrierWithGroupSync();

	outputBuffer[groupIdx + groupID.x * blockSize] = local[groupIdx];
}


/**********************************************************************

Copyright (c) 2022 Robert May

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

