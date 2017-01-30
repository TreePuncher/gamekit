/**********************************************************************

Copyright (c) 2015 Robert May

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

#include "common.hlsl"

StructuredBuffer<float4x4>	Transforms : register(t0);

static const float PI 			= 3.14159265359f;
static const float PIInverse 	= 1/PI;

struct VIN
{
	float3 V 			: POSITION;
	float3 N 			: NORMAL;
	float3 T 			: TANGENT;
	uint   instanceID 	: SV_InstanceID;
};

struct VOUT
{
	float3 WPOS 	: TEXCOORD0;
	float3 N 		: TEXCOORD1;
	float3 T 		: TEXCOORD2;
	float3 B 		: TEXCOORD3;
	float4 POS 	 	: SV_POSITION;
};

VOUT VMain( VIN IN )
{
	VOUT Out;

	float4x4 WT = Transforms[IN.instanceID];

	Out.WPOS 	= mul(WT, float4(IN.V, 1));
	Out.POS		= mul(Proj, mul(View, float4(Out.WPOS, 1.0)));
	Out.N 		= mul(WT, IN.N);
	Out.T 		= mul(WT, IN.T);
	Out.B 		= cross(IN.N, IN.T);

	return Out;
}