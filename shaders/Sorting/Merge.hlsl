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


void Merge(int2 ASpan, int2 BSpan, int2 OutSpan)
{
	int a_itr = ASpan.x;
	int b_itr = BSpan.x;

	if (a_itr < 0 || b_itr < 0)
		return;

	uint a = sourceBuffer[a_itr];
	uint b = sourceBuffer[b_itr];

	while (a_itr < ASpan.y || b_itr < BSpan.y)
	{
		if (a_itr < ASpan.y && b_itr < BSpan.y)
		{
			if (a <= b)
			{
				outputBuffer[OutSpan.x] = a;
				a_itr++;
				a = sourceBuffer[a_itr];
			}
			else
			{
				outputBuffer[OutSpan.x] = b;
				b_itr++;
				b = sourceBuffer[b_itr];
			}

			OutSpan.x++;
		}
		else
		{
			while (a_itr < ASpan.y && OutSpan.x < OutSpan.y)
			{
				outputBuffer[OutSpan.x] = a;
				OutSpan.x++;
				a_itr++;
				a = sourceBuffer[a_itr];
			}

			while (b_itr < BSpan.y && OutSpan.x < OutSpan.y)
			{
				outputBuffer[OutSpan.x] = b;
				OutSpan.x++;
				b_itr++;
				b = sourceBuffer[b_itr];
			}

			return;
		}
	}
}


/************************************************************************************************/




[numthreads(16, 1, 1)]
void GlobalMerge(const uint3 dispatchID : SV_DispatchThreadID, const uint3 groupID : SV_GroupID, const uint groupIdx : SV_GroupIndex)
{
	if (dispatchID.x >= blockCount / 2 * p)
		return;

	const int blockIdx	= dispatchID.x / p;
	const int i			= dispatchID.x % p;

	const int2 begin	=  (i == 0) ? uint2(blockSize * (2 * blockIdx + 0), blockSize * (2 * blockIdx + 1)) : mergePathTable[dispatchID.x - 1];
	const int2 end		= mergePathTable[dispatchID.x];

	Merge(
		int2(begin.x, end.x),
		int2(begin.y, end.y),
		int2(
			2 * blockIdx * blockSize + i * 2 * blockSize / p,
			2 * blockIdx * blockSize + i * 2 * blockSize / p + 32));
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

