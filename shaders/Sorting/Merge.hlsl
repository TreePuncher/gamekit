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


void Merge(uint2 ASpan, uint2 BSpan, uint outItr, uint outEnd)
{
	uint a_itr = ASpan.x;
	uint b_itr = BSpan.x;

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
				outputBuffer[outItr] = a;
				a_itr++;
				a = sourceBuffer[a_itr];
			}
			else
			{
				outputBuffer[outItr] = b;
				b_itr++;
				b = sourceBuffer[b_itr];
			}

			outItr++;
		}
		else
		{
			while (a_itr < ASpan.y && outItr < outEnd)
			{
				outputBuffer[outItr] = a;
				outItr++;
				a_itr++;
				a = sourceBuffer[a_itr];
			}

			while (b_itr < BSpan.y && outItr < outEnd)
			{
				outputBuffer[outItr] = b;
				outItr++;
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

	const uint blockIdx	= dispatchID.x / p;
	const uint i		= dispatchID.x % p;

	const uint a = blockSize * (2 * blockIdx + 0);
	const uint b = blockSize * (2 * blockIdx + 1);

	const uint2 begin	=  (i == 0) ? uint2(a, b) : mergePathTable[dispatchID.x - 1];
	const uint2 end		= mergePathTable[dispatchID.x];

	const uint outBegin = (dispatchID.x + 0) * 256;
	const uint outEnd	= (dispatchID.x + 1) * 256;

	Merge(
		uint2(begin.x, end.x),
		uint2(begin.y, end.y),
		outBegin, outEnd);
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

