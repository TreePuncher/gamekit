/**********************************************************************

Copyright (c) 2015-2016 Robert May

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


struct Plane
{
	float4 Normal;
	float4 Orgin;
};


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


/************************************************************************************************/


struct VIN
{
	float4 POS : POSITION;
};

struct VOUT
{
	float4 POS 	: SV_POSITION;
	float4 WPOS : POSITION;
};


/************************************************************************************************/


VOUT VMain( VIN In )
{
	VOUT Out;
	Out.POS 	= mul(PV, mul(WT, In.POS)); 
	Out.WPOS 	= mul(WT, In.POS);

	return Out;
}

struct LinePoint
{
	float4 POS		: POSITION;
	float4 Colour	: COLOUR;
};

struct LinePointPS
{
	float4 Colour	: COLOUR;
	float4 POS		: SV_POSITION;
};

LinePointPS VSegmentPassthrough(LinePoint In)
{
	LinePointPS Out;
	Out.POS		= mul(PV, In.POS);
	Out.Colour	= In.Colour;
	return Out;
}

/************************************************************************************************/


struct RectPoint_VS
{
	float2 POS 		: POSITION;
	float2 UV		: TEXCOORD;
	float4 Color 	: COLOR;
};


struct RectPoint_PS
{
	float4 Color	: Color;
	float2 UV		: TEXCOORD;
	float4 POS 		: SV_POSITION;
};


RectPoint_PS DrawRect_VS( RectPoint_VS In )
{
	RectPoint_PS Out;
	Out.POS		= float4(In.POS, 1, 1);
	Out.UV		= In.UV;
	Out.Color	= In.Color;

	return Out;
}


/************************************************************************************************/

struct VIN2
{
	float4 POS 		: POSITION;
	float2 UV		: TEXCOORD;
	float4 N	 	: NORMAL;
};

struct PS_IN
{
	float3 WPOS 	: TEXCOORD0;
	float3 N 		: TEXCOORD1;
	float2 UV 		: TEXCOORD2;
	float4 POS 	 	: SV_POSITION;
};

PS_IN V2Main( VIN2 In )
{
	float3x3 rot = WT;
	PS_IN Out;
	Out.WPOS 	= mul(WT, In.POS);
	Out.POS 	= mul(PV, mul(WT, In.POS));
	Out.N   	= normalize(mul(rot, In.N.xyz));
	Out.UV		= In.UV;
	return Out;
}


/************************************************************************************************/


struct VIN3
{
	float4 POS 		: POSITION;
	float4 N	 	: NORMAL;
	float3 T 		: TANGENT;
};

struct PS_IN2
{
	float3 WPOS 	: TEXCOORD0;
	float4 N 		: TEXCOORD1;
	float3 T 		: TEXCOORD2;
	float3 B 		: TEXCOORD3;
	float4 POS 	 	: SV_POSITION;
};


/************************************************************************************************/


PS_IN2 V3Main( VIN3 In )
{
	float3x3 rot = WT;
	PS_IN2 Out;
	Out.WPOS 	= mul(WT, float4(In.POS.xyz, 1.0f));
	Out.POS 	= In.POS;
	Out.POS 	= mul(PV, mul(WT, float4(In.POS.xyz, 1.0f)));
	Out.N   	= float4(mul(rot, In.N), 0);
	Out.T   	= mul(rot, In.T);
	Out.B   	= mul(rot, cross(In.N, In.T));
	
	return Out;
}


/************************************************************************************************/


struct Joint
{
	float4x4 I;
	float4x4 T;
};

struct VIN4
{
	float3 POS 		: POSITION;
	float3 N	 	: NORMAL;
	float3 W		: WEIGHTS;
	float3 UV		: TEXCOORD;
	uint4  I		: WEIGHTINDICES;
};


/************************************************************************************************/


StructuredBuffer<Joint> Bones : register(t0);


/************************************************************************************************/


PS_IN VMainVertexPallet(VIN4 In)
{
	PS_IN Out;
	float3 N = float3(0.0f, 0.0f, 0.0f);
	float3 V = float3(0.0f, 0.0f, 0.0f);
	float4 W = float4(In.W.xyz, 1 - In.W.x - In.W.y - In.W.z);

	[unroll(4)]
	for (uint I = 0; I < 4; ++I)
	{
		float4x4 MT = Bones[In.I[I]].T;
		float4x4 MI = Bones[In.I[I]].I;

		float4 TP = mul(MI, float4(In.POS, 1));
		float3 TN = mul(MI, In.N);

		V += mul(MT, TP) * W[I];
		N += mul(MT, TN) * W[I];
	}

	float4 V2 = float4(V, 1.0f);
	Out.WPOS = mul(WT, V2);
	Out.POS  = mul(PV, mul(WT, V2));
	Out.N    = mul(WT, N);
	Out.UV	 = In.UV;

	return Out;
}


/************************************************************************************************/