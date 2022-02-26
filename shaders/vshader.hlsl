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


#include "common.hlsl"

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

LinePointPS VSegmentPassthrough_NOPV(LinePoint In)
{
    LinePointPS Out;
    Out.POS     = In.POS;
    Out.Colour  = In.Colour;
    return Out;
}


/************************************************************************************************/


struct RectPoint_VS
{
	float2 POS 		: POSITION;
	float2 UV		: TEXCOORD;
	float4 Color 	: COLOR;
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


PS_IN V2Main( VIN2 In )
{
	float3x3 rot = WT;
	PS_IN Out;
	Out.WPOS 	= mul(WT, In.POS);
	Out.POS 	= mul(mul(PV, WT), In.POS);

	Out.N   	= normalize(mul(rot, In.N.xyz));
	Out.UV		= In.UV;
    Out.Depth   = Out.POS.z / Out.POS.w;

	return Out;
}


/************************************************************************************************/


struct VIN10
{
	float4 POS 		: POSITION;
	float4 Color	: COLOR;
	float2 UV		: TEXCOORD;
};


RectPoint_PS V10Main( VIN10 In )
{
	RectPoint_PS Out;
	Out.POS 	= mul(mul(PV, WT), In.POS);

	Out.UV		= In.UV;
	Out.Color	= In.Color;

	return Out;
}


/************************************************************************************************/


DrawFlatTri3D_IN V11Main(VIN10 In)
{
    const float3 POS_WT = mul(WT, In.POS);
    const float4 POS_VS = mul(View, float4(POS_WT, 1));

    DrawFlatTri3D_IN Out;
    Out.POS     = mul(mul(PV, WT), In.POS);
    Out.Depth   = -POS_VS.z / MaxZ;
    Out.UV      = In.UV;
    Out.Color   = In.Color;

    return Out;
}


/************************************************************************************************/


float4 FullScreeQuad(int ID : SV_VertexID) : SV_Position
{
    float4 Out;

    if(ID == 0) // Top Left
    {
        float4 Out = float4(-1, 1, 0, 1);
    }
    if (ID == 1) // Top Right
    {
        float4 Out = float4(-1, 1, 0, 1);

    }
    if (ID == 2) // Bottom Left
    {
        float4 Out = float4(-1, 1, 0, 1);

    }
    if (ID == 3) // BottomLeft
    {
        float4 Out = float4(-1, 1, 0, 1);
        
    }
    if (ID == 4) // Top Right
    {
        float4 Out = float4(-1, 1, 0, 1);

    }
    if (ID == 5) // Bottom Right
    {
        float4 Out = float4(-1, 1, 0, 1);
        
    }
    Out = mul(PV, Out);

    return Out;
}


struct VIN3
{
	float4 POS 		: POSITION;
	float4 N	 	: NORMAL;
	float3 T 		: TANGENT;
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
    Out.Depth   = Out.POS.z / Out.POS.w;
	
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



PS_IN VMainVertexPallet(VIN4 In)
{
	PS_IN Out;
	float4 N = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 V = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 W = float4(In.W.xyz, 1 - In.W.x - In.W.y - In.W.z);

	float4x4 MTs[4] =
	{
		Bones[In.I[0]].T,
		Bones[In.I[1]].T,
		Bones[In.I[2]].T,
		Bones[In.I[3]].T,
	};

	float4x4 MIs[4] = 
	{
		Bones[In.I[0]].I,
		Bones[In.I[1]].I,
		Bones[In.I[2]].I,
		Bones[In.I[3]].I,
	};

	[unroll(4)]
	for (uint I = 0; I < 4; ++I)
	{
		float4 TP = mul(MIs[I], float4(In.POS, 1)); // Temp Position
		float4 TN = mul(MIs[I], float4(In.N, 0));   // Temp Normal

		V += mul(MTs[I], TP) * W[I];
		N += mul(MTs[I], TN) * W[I];
	}

	float4 V2   = V;
	Out.WPOS    = mul(WT, V2);
	Out.POS     = mul(PV, mul(WT, V2));
	Out.N       = mul(WT, N).xyz;
	Out.UV      = In.UV;
    Out.Depth   = Out.POS.z / Out.POS.w;

	return Out;
}



/************************************************************************************************/


struct DepthPass_IN {
	float4 POS_Normalized	: POSITION;
	float4 POS				: SV_POSITION;
};


DepthPass_IN VMain_ShadowMapping(VIN In)
{
	DepthPass_IN Out;
	Out.POS			   = mul(PV, mul(WT, In.POS));
	Out.POS_Normalized = Out.POS;

	return Out;
}


/************************************************************************************************/


struct VIN5
{
	float3 POS 		: POSITION;
	float3 W		: WEIGHTS;
	uint4  I		: WEIGHTINDICES;
};

DepthPass_IN VMainVertexPallet_ShadowMapping(VIN5 In)
{
	DepthPass_IN Out;
	float3 V = float3(0.0f, 0.0f, 0.0f);
	float4 W = float4(In.W.xyz, 1 - In.W.x - In.W.y - In.W.z);

	[unroll(4)]
	for (uint I = 0; I < 4; ++I)
	{
		float4x4 MT = Bones[In.I[I]].T;
		float4x4 MI = Bones[In.I[I]].I;

		float4 TP = mul(MI, float4(In.POS, 1));

		V += mul(MT, TP) * W[I];
	}

	float4 V2 = float4(V, 1.0f);
	Out.POS				= mul(PV, mul(WT, V2));
	Out.POS_Normalized  = Out.POS;

	return Out;
}


/************************************************************************************************/
