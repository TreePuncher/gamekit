// Resource Layout
// 4 UAV's in registers 0 - 3
// 4 SRV's in registers 0 - 3
// 8 CBV's in registers 0 - 7

/************************************************************************************************/


float3 TopLeft		= float3(-1, 1,  0);
float3 TopRight		= float3( 1, 1,  0);
float3 BottomLeft	= float3(-1, -1, 0);
float3 BottomRight	= float3( 1, -1, 0);


/************************************************************************************************/


cbuffer CameraConstants : register(b0)
{
	float4x4 View;
	float4x4 ViewI;
	float4x4 Proj;
	float4x4 PV;				// Projection x View
	float4x4 PVI;				// Projection x View
	float4   CameraPOS;
	float  	 MinZ;
	float  	 MaxZ;

	float4  WSTopLeft;
	float4  WSTopRight;
	float4  WSBottomLeft;
	float4  WSBottomRight;

	float4  WSTopLeft_Near;
	float4  WSTopRight_Near;
	float4  WSBottomLeft_Near;
	float4  WSBottomRight_Near;
};


/************************************************************************************************/


// Defines
#define TopLeft		0
#define TopRight	1
#define BottomRight	2
#define BottomLeft	3


/************************************************************************************************/


#define	EPlane_FAR		0
#define	EPlane_NEAR		1
#define	EPlane_TOP		2
#define	EPlane_BOTTOM	3 
#define	EPlane_LEFT		4
#define	EPlane_RIGHT	5

#define MinScreenSpan 0.05fs
#define MaxScreenSpan 0.2f

#define MinSpan 16
#define MaxSpan	1024


#define TerrainHeight 1024


/************************************************************************************************/


struct Plane
{
	float4 Normal;
	float4 Orgin;
};


cbuffer TerrainConstants : register(b1)
{
	float4  Padding;
	float4	Albedo;   // + roughness
	float4	Specular; // + metal factor
	float2	RegionDimensions;
	Plane	Frustum[6];
	int		SplitCount;
}


struct Region_CP
{
	int4    POS             : REGION;
	int4    Parent	        : TEXCOORD0;
	float4  UVCords         : TEXCOORD1;
	int4    TerrainInfo     : TEXCOORD2;
};


/************************************************************************************************/


Texture2D<float> HeightMap		: register(t0);
sampler DefaultSampler			: register(s0);

struct Corner {
	float3 pos;
	float2 uv;
};

Corner GetTopLeftPoint(Region_CP CP)
{
	Corner Out;
	float offset   = float(CP.POS.w);
    float2 UV_TL   = CP.UVCords.xy;
    float2 UV_BR   = CP.UVCords.zw;

    float2 UV      = UV_TL;
	float y		   = HeightMap.Gather(DefaultSampler, UV) * TerrainHeight;

	Out.pos	= float3(CP.POS.x - offset, y, CP.POS.z + offset);
	Out.uv	= UV;

	return Out;
}


/************************************************************************************************/


Corner GetTopRightPoint(Region_CP CP)
{
	Corner Out;

	float offset   = float(CP.POS.w);
    float2 UV_TL   = CP.UVCords.xy;
    float2 UV_BR   = CP.UVCords.zw;

	float2 UV	   = float2(UV_BR.x, UV_TL.y);
    float y        = HeightMap.Gather(DefaultSampler, UV) * TerrainHeight;

	Out.pos = float3(CP.POS.x + offset, y, CP.POS.z + offset);
	Out.uv	= UV;

	return Out;
}


/************************************************************************************************/


Corner GetBottomLeftPoint(Region_CP CP)
{
	Corner	Out;

	float offset   = float(CP.POS.w);
    float2 UV_TL   = CP.UVCords.xy;
    float2 UV_BR   = CP.UVCords.zw;

    float2 UV      = float2(UV_TL.x, UV_BR.y);
    float y        = HeightMap.Gather(DefaultSampler, UV) * TerrainHeight;

	Out.pos = float3( CP.POS.x - offset, y, CP.POS.z - offset);
	Out.uv	= UV;

	return Out;
}


/************************************************************************************************/


Corner GetBottomRightPoint(Region_CP CP)
{
	Corner Out;

	float offset   = float(CP.POS.w);
    float2 UV_TL   = CP.UVCords.xy;
    float2 UV_BR   = CP.UVCords.zw;

    float2 UV      = UV_BR;
    float y        = HeightMap.Gather(DefaultSampler, UV) * TerrainHeight;

	Out.pos		= float3(CP.POS.x + offset, y, CP.POS.z - offset);
	Out.uv		= UV;

	return Out;
}


float3 CalculateNormal(float3 V0, float3 V1, float3 V2)
{
	float3 normal = cross(V2 - V0, V1 - V0);
	normal = normalize(normal);
	return normal;
}

float3 calculateCornerNormal(float UVSpan, float regionSpan, Corner Point)
{
	// Center point
	float Sample1 = HeightMap.Gather(DefaultSampler, Point.uv) * TerrainHeight;

	// West of Center point
	float Sample2 = HeightMap.Gather(DefaultSampler, Point.uv - UVSpan) * TerrainHeight;

	// East of Center Point
	float Sample3 = HeightMap.Gather(DefaultSampler, Point.uv + UVSpan) * TerrainHeight;

	// South of Center Point
	float Sample4 = HeightMap.Gather(DefaultSampler, Point.uv - UVSpan) * TerrainHeight;

	// north of Center Point
	float Sample5 = HeightMap.Gather(DefaultSampler, Point.uv + UVSpan) * TerrainHeight;

	const float offset	= regionSpan;
	float3 normal		= float3(0, 0, 0);
	float3 v0			= float3(0, 0, 0);
	float3 v1			= float3(0, 0, 0);
	float3 v2			= float3(0, 0, 0);
	
	// Top Left tri
	v0 = float3(Point.pos.x - offset,	Sample2, Point.pos.z);
	v1 = float3(Point.pos.x,			Sample5, Point.pos.z + offset);
	v2 = float3(Point.pos.x,			Sample1, Point.pos.z);
	normal += CalculateNormal(v1, v0, v2);

	// Top Right tri
	v0 = float3(Point.pos.x,			Sample1, Point.pos.z);
	v1 = float3(Point.pos.x,			Sample5, Point.pos.z + offset);
	v2 = float3(Point.pos.x + offset,	Sample2, Point.pos.z);
	normal += CalculateNormal(v1, v0, v2);

	// Bottom Left tri
	v0 = float3(Point.pos.x - offset,	Sample2, Point.pos.z);
	v1 = float3(Point.pos.x,			Sample1, Point.pos.z);
	v2 = float3(Point.pos.x,			Sample4, Point.pos.z - offset);
	normal += CalculateNormal(v1, v0, v2);

	// Bottom Right tri
	v0 = float3(Point.pos.x,			Sample1, Point.pos.z);
	v1 = float3(Point.pos.x + offset,	Sample3, Point.pos.z);
	v2 = float3(Point.pos.x,			Sample4, Point.pos.z - offset);
	normal += CalculateNormal(v1, v0, v2);

	return normalize(normal/4);
}


float GetRegionSpan(Region_CP CP)
{
	float4 TL = mul(Proj, mul(View, float4(GetTopLeftPoint(CP).pos	* float3(1, 1, 1), 1)));
	float4 TR = mul(Proj, mul(View, float4(GetTopRightPoint(CP).pos * float3(1, 1, 1), 1)));

	float4 BL = mul(Proj, mul(View, float4(GetBottomLeftPoint(CP).pos  * float3(1, 1, 1), 1)));
	float4 BR = mul(Proj, mul(View, float4(GetBottomRightPoint(CP).pos * float3(1, 1, 1), 1)));

	float3 TLSS = (TL.xyz / TL.w);
	float3 BRSS = (BR.xyz / BR.w);
	float3 TRSS = (TR.xyz / TR.w);
	float3 BLSS = (BL.xyz / BL.w);

	float Span1 = length(TLSS - TRSS);
	float Span2 = length(BLSS - BRSS);
	float Span3 = length(TLSS - BLSS);
	float Span4 = length(TRSS - BRSS);

	return max(max(Span1, Span2), max(Span3, Span4));
}


bool SplitRegion(Region_CP CP)
{
	return (CP.POS.w > MaxSpan) || (MinSpan < CP.POS.w && GetRegionSpan(CP) > MaxScreenSpan);
}


/**********************************************************************

Copyright (c) 2015 - 2019 Robert May

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