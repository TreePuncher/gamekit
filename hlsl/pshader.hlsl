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

/************************************************************************************************/

cbuffer CameraConstants : register( b0 )
{
	float4x4 View;
	float4x4 Proj;
	float4x4 CameraWT;			// World Transform
	float4x4 PV;				// Projection x View
	float4x4 CameraInverse;
	float4   CameraPOS;
	uint  	 MinZ;
	uint  	 MaxZ;
	int 	 PointLightCount;
	int 	 SpotLightCount;
};


cbuffer LocalConstants : register( b1 )
{
	float4	 Albedo; // + roughness
	float4	 Specular;
	float4x4 WT;
}

struct GBuffer
{
	float4 COLOR	: SV_TARGET0;
	float4 Specular	: SV_TARGET1;
	float4 NORMAL 	: SV_TARGET2;
	float4 WPOS 	: SV_TARGET3;
};

struct PS_IN
{
	float3 WPOS 	: TEXCOORD0;
	float4 N 		: TEXCOORD1;
};

struct NormalMapped_IN
{
	float3 WPOS 	: TEXCOORD0;
	float4 N 		: TEXCOORD1;
	float3 T 		: TEXCOORD2;
	float3 B 		: TEXCOORD3;
};

/************************************************************************************************/

GBuffer PMain(PS_IN IN)
{
	GBuffer Out;
	Out.COLOR 		= Albedo;				// Last Float is Roughness
	Out.NORMAL 		= IN.N;					// last Float is Unused
	Out.WPOS 		= float4(IN.WPOS,  	0);	// last Float is Unused
	Out.Specular 	= Specular;				// Last Float is Metal Factor

	return Out;
}

GBuffer PMainNormalMapped(PS_IN IN)
{
	GBuffer Out;
	Out.COLOR 		= Albedo;				// Last Float is Roughness
	Out.NORMAL 		= IN.N;					//  
	Out.WPOS 		= float4(IN.WPOS,  0);	// last Float is Unused
	Out.Specular 	= Specular;

	return Out;
}

GBuffer DebugPaint(PS_IN IN)
{
	GBuffer Out;
	/*
	float3 Color 	= float3(abs(IN.WPOS.x), abs(IN.WPOS.y), abs(IN.WPOS.z));
	Out.COLOR 		= float4(Color, 	0.5f);
	Out.Specular 	= float4(Color, 	0.0f);
	Out.WPOS 		= float4(IN.WPOS,	0.0f);
	*/
	float3 Color 	= float3(1.0f, 1.0f, 1.0f);
	Out.COLOR 		= float4(Color, 	0.5f);
	Out.Specular 	= float4(Color, 	0.0f);
	Out.WPOS 		= float4(IN.WPOS,	0.0f);
	Out.NORMAL 		= IN.N;						
	return Out;
}

/************************************************************************************************/