#include "CBT.hlsl"

#define SumReductionRS	"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),"\
						"UAV(u0),"\
						"RootConstants(num32BitConstants=6, b0)"

cbuffer constants : register(b0)
{
	uint32_t maxDepth;
	uint32_t i;
	uint32_t stepSize;
	uint32_t end;
	uint32_t start_IN;
	uint32_t start_OUT;
}

RWStructuredBuffer<uint32_t> CBTBuffer : register(u0);

[RootSignature(SumReductionRS)]
[NumThreads(1024, 1, 1)]
void SumReduction(const uint j : SV_DispatchThreadID)
{
	if (j >= end)
		return;
	
	const uint32_t a = ReadValue(CBTBuffer, start_IN + (2 * j + 0) * stepSize, i + 1);
	const uint32_t b = ReadValue(CBTBuffer, start_IN + (2 * j + 1) * stepSize, i + 1);
	const uint32_t c = a + b;

	WriteValue(CBTBuffer, start_OUT + j * (stepSize + 1), i + 2, c);
}


/**********************************************************************

Copyright (c) 2024 Robert May

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
