#define TopLeft float3(-1,  1,  0);
#define TopRight float3( 1,  1,  0);
#define BottomLeft float3(-1, -1,  0);
#define BottomRight float3( 1, -1,  0);

cbuffer CameraConstants : register( b0 )
{
	float4x4 View;
	float4x4 ViewI;
	float4x4 Proj;
	float4x4 PV;				// Projection x View
	float4x4 PVI;				
	float4   CameraPOS;
	float  	 MinZ;
	float  	 MaxZ;
	float    AspectRatio;
	float    FOV;
	
	float3   TLCorner_VS;
	float3   TRCorner_VS;

	float3   BLCorner_VS;
	float3   BRCorner_VS;
};

struct PS_IN
{
	float3 WPOS  : TEXCOORD0;
	float3 N     : TEXCOORD1;
	float2 UV    : TEXCOORD2;
	float4 POS   : SV_POSITION;
	float Depth  : TEXCOORD3;
};

struct PS_IN2
{
	float3 WPOS 	: TEXCOORD0;
	float4 N 		: TEXCOORD1;
	float3 T 		: TEXCOORD2;
	float3 B 		: TEXCOORD3;
	float4 POS 	 	: SV_POSITION;
	float Depth     : TEXCOORD4;
};

struct RectPoint_PS
{
	float4 Color	: COLOR;
	float2 UV		: TEXCOORD;
	float4 POS 		: SV_POSITION;
};

struct RectPointDepth_PS
{
	float4	Color	: COLOR;
	float2	UV		: TEXCOORD;
	float4	POS 	: SV_POSITION;
	float	depth	: DEPTH;
};

struct DrawFlatTri3D_IN
{
	float4 Color	: COLOR;
	float2 UV		: TEXCOORD;
	float4 POS 		: SV_POSITION;
	float  Depth    : DEPTH;
};

struct PS_Colour_IN
{
	float3 WPOS 	: TEXCOORD0;
	float3 Colour	: TEXCOORD1;
	float3 N 		: TEXCOORD2;
};

struct NormalMapped_IN
{
	float3 WPOS 	: TEXCOORD0;
	float4 N 		: TEXCOORD1;
	float3 T 		: TEXCOORD2;
	float3 B 		: TEXCOORD3;
};

struct Plane
{
	float4 Normal;
	float4 Orgin;
};

float Hash(in float x)
{
	uint2 n = uint(x) * uint2(1597334673U, 3812015801U);
	uint q = (n.x ^ n.y) * 1597334673U;
	return float(q) * (1.0 / float(0xFFFFFFFFU));
}

float2 Hash2(in uint q)
{
	uint2 n = q * uint2(1597334673U, 3812015801U);
	n = (n.x ^ n.y) * uint2(1597334673U, 3812015801U);
	return float2(n) * (1.0 / float(0xFFFFFFFFU));
}

float Hash(float2 IN)// Thanks nVidia
{
	return frac(1.0e4 * sin(17.0f * IN.x + 0.1 * IN.y) *
				(0.1 + abs(sin(13.0 * IN.y + IN.x))));
}

float Hash3D(float3 IN)// Thanks nVidia
{
	return Hash(float2(Hash(IN.xy), IN.z));
}

float4 SampleVTexture(Texture2D OffsetTable, Texture2D TextureAtlas, float2 Coordinate)
{
	return float4(0, 0, 0, 0);
}

float3 GetVectorToBack(float3 pos)
{
	return CameraPOS.xyz - pos;
}

float PL(float3 LightPOS, float3 POS, float r, float lm) // Lighting Function Point Light
{
	float L = length(POS-LightPOS);
	float E = (lm/(L*L)) * saturate((1-(L/r)));
	return E;
}

float cot(float theta)
{
	return cos(theta)/sin(theta);
}

float2 PixelCordToCameraCord(int2 UV, float2 WH)
{
	float2 Out = float2(UV) / WH;
	return Out * 2 - 1.0;
}

float2 GetTextureSpaceCord(float2 UV, float2 WH)
{
	return UV / WH;
}

float2 ViewToTextureSpace(float2 CoordDS, float2 WH)
{
   return float2((CoordDS.x + 1) / 2, (1 - CoordDS.y) / 2) * WH;
}

float NormalizeAndRescaleZ(float Z_in, float Scale)
{
	return saturate((Z_in - MinZ) / (Scale - MinZ));
}

float3 GetViewVector_VS(const float2 UV) // View Space Vector
{
	const float3 LeftPoint  = lerp(TLCorner_VS, BLCorner_VS, UV.y); // Left Edge
	const float3 RightPoint = lerp(TRCorner_VS, BRCorner_VS, UV.y); // Right Edge
	const float3 FarPos     = lerp(LeftPoint, RightPoint, UV.x);

	return normalize(FarPos);
}

float3 GetViewVector(const float2 UV)
{
	const float3 View_VS = GetViewVector_VS(UV);
	return mul(ViewI, View_VS);
}

float3 GetViewSpacePosition(float2 UV, float D)
{
	const float3 LeftPoint  = lerp(TLCorner_VS, BLCorner_VS, UV.y); // Left Edge
	const float3 RightPoint = lerp(TRCorner_VS, BRCorner_VS, UV.y); // Right Edge
	const float3 V          = lerp(LeftPoint, RightPoint, UV.x) * D;

	return V;
}

void GetViewSpacePOSanDIR(float2 UV, float D, out float3 POS_VS, out float3 V)
{
	const float3 LeftPoint  = lerp(TLCorner_VS, BLCorner_VS, UV.y); // Left Edge
	const float3 RightPoint = lerp(TRCorner_VS, BRCorner_VS, UV.y); // Right Edge
	const float3 farPoint   = lerp(LeftPoint, RightPoint, UV.x);

	POS_VS  = farPoint * D;
	V = normalize(farPoint);
}

float3 GetWorldSpacePosition(float2 UV, float D) 
{
	const float3 V = GetViewSpacePosition(UV, D);

	return mul(ViewI, float4(V, 1));
}

void GetWorldSpacePositionAndViewDir(float2 UV, float D, out float3 POS_WS, out float3 VWS)
{
	GetViewSpacePOSanDIR(UV, D, POS_WS, VWS);

	POS_WS  = mul(ViewI, float4(POS_WS, 1));
	VWS     = mul(ViewI, float4(VWS, 1));
}


/**********************************************************************

Copyright (c) 2015 - 2021 Robert May

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
