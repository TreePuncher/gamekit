#include "MBOIT.hlsl"

#define RS1 "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),"	\
			"RootConstants(num32BitConstants = 17, b0),"		\
			"DescriptorTable("									\
				"SRV(t0, numDescriptors = 3))"


/************************************************************************************************/


#define INFINITY ((float)(1e+300 * 1e+300))
#define M 4


/************************************************************************************************/


cbuffer constants : register(b0)
{
	float4x4	PV;
	uint		offset;
};


struct MLAB_Samples
{
	half4 samples[M];
};

struct MLAB_Depth
{
	float samples[M];
};

Texture2D<float> b0Buffer		: register(t0);
Texture2D<float4> moments		: register(t1);
Texture2D<float4> accumulation	: register(t2);


/************************************************************************************************/


static const float4 fullScreenTriangle[] =
{
	float4(-1, -1, 1, 1),
	float4(-1,  3, 1, 1),
	float4( 3, -1, 1, 1),
};


[RootSignature(RS1)]
float4 VS_FullScreen(const uint ID : SV_VertexID) : SV_POSITION
{
	return fullScreenTriangle[ID];
}


/************************************************************************************************/


float4 PS_Blend(const float4 xy : SV_POSITION) : SV_TARGET
{
	float4 rgba = 0;

	return moments[uint2(xy.xy)];
}


/**********************************************************************

Copyright (c) 2014-2023 Robert May

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
