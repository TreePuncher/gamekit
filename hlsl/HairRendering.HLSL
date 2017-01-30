/**********************************************************************

Copyright (c) 2015 - 2016 Robert May

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

cbuffer ScalpConstants : register(b0)
{
};


struct PS_Colour_IN
{
	float3 WPOS 	: TEXCOORD0;
	float3 Colour	: TEXCOORD1;
	float3 N 		: TEXCOORD2;
	float4 POS 	 	: SV_POSITION;
};


struct HairPointIn
{
	float4 POS : POSITION;
};


struct HairPointOut
{
	float4 POS 		: POSITION;
	float4 Normal	: NORMAL;
};


HairPointIn VPassThrough(HairPointIn IN)
{
	return IN;
}


[maxvertexcount(4)]
void GS_HairPlaneExpansion(
	line HairPointIn IN[2],
	inout PointStream<HairPointOut> Planes)
{
}


struct Factor
{
	float EdgeFactor[2]	: SV_TessFactor;
};


Factor ConstantFactors(InputPatch<HairPointIn, 2> ip)
{
	Factor Out;
	Out.EdgeFactor[0] = 1;
	Out.EdgeFactor[1] = 1;

	return Out;
}


struct IsoLine
{
	float3	Position[2] : POSITION;
};


[domain					("isoline")]
[outputtopology			("line")]
[outputcontrolpoints	(1)]
[partitioning			("fractional_even")]
[patchconstantfunc		("ConstantFactors")]
[maxtessfactor			(64.0f)]
IsoLine HShader(
	InputPatch<HairPointIn, 2> ip,
	uint i			: SV_OutputControlPointID,
	uint PatchID	: SV_PrimitiveID)
{
	IsoLine Out;
	Out.Position[0] = ip[0].POS.xyz;
	Out.Position[1] = ip[1].POS.xyz;
	return Out;
}


[domain("isoline")]
PS_Colour_IN DShader(
	Factor TessFactor,
	float2 uv : SV_DomainLocation,
	const OutputPatch<IsoLine, 1> isoLine)
{
	PS_Colour_IN Out;
	Out.WPOS	= lerp(isoLine[0].Position[0], isoLine[0].Position[1], uv.x);
	Out.Colour	= float3(1, 1, 1);
	Out.N		= float3(0, 1, 0);
	Out.POS 	= float4(Out.WPOS, 1);

	return Out;
}