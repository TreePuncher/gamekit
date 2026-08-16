[[fk::BeginRootSignatureDef(id=rootsig1)]]
[[fk::RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)]]

[[fk::PushConstants(num=16)]] 
{
	float4x4 WT;
};

[[fk::CBV(id=rootsig1)]]
{
	float4x4 View;
	float4x4 ViewI;
	float4x4 Proj;
	float4x4 PV;
};

[[fk::RootSignature(id=rootsig1)]]
float4 DepthPass_VS(float3 POS : POSITION) : SV_POSITION
{
	return mul(PV, mul(WT, float4(POS, 1)));
}



/**********************************************************************

Copyright (c) 2015 - 2026 Robert May

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
