StructuredBuffer<uint> sourceBuffer : register(t0);

RWStructuredBuffer<uint2> outputBuffer	: register(u0);

cbuffer constants : register(b0)
{
	uint p;
	uint blockSize;
}

uint2 DiagonalIntersection(const uint i)
{
	const uint index	= i * (blockSize + blockSize) / p;
	uint a_top			= index > blockSize ? blockSize : index;
	uint b_top			= index > blockSize ? index - blockSize : 0;
	uint a_bottom		= b_top;

	for(uint j = 0; j < 4; j++)
	{
		const uint offset	= (a_top - a_bottom) / 2;
		const uint a_itr	= a_top - offset;
		const uint b_itr	= b_top + offset;

		if (a[a_itr] > b[b_itr - 1])
		{
			if ((a[a_itr - 1]) <= b[b_itr])
			{
				return uint2(a_itr, b_itr);
			}
			else
			{
				a_top = a_itr - 1;
				b_top = b_itr + 1;
			}
		}
		else
			a_bottom = a_itr + 1;
	}

	return uint2(-1, -1);
}

[numthreads(32, 1, 1)]
void GlobalMergePathSort(const uint3 dispatchID : SV_DispatchThreadID, const uint3 groupID : SV_GroupThreadID)
{
	if (dispatchID.x < p && dispatchID.x >= 1)
		outputBuffer[dispatchID.x] = DiagonalIntersection(dispatchID.x);
	else if (dispatchID.x == 0)
	{
		outputBuffer[0] = uint2(0, 0);
		outputBuffer[p]	= uint2(blockSize, blockSize);
	}
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

