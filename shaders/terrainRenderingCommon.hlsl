// Resource Layout
// 4 UAV's in registers 0 - 3
// 4 SRV's in registers 0 - 3
// 8 CBV's in registers 0 - 7

/************************************************************************************************/


float3 TopLeft		= float3(-1, 1, 0);
float3 TopRight		= float3(1, 1, 0);
float3 BottomLeft	= float3(-1, -1, 0);
float3 BottomRight	= float3(1, -1, 0);


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
#define NorthEast	0
#define NorthWest	1
#define SouthWest	2
#define SouthEast	3


/************************************************************************************************/


#define	EPlane_FAR		0
#define	EPlane_NEAR		1
#define	EPlane_TOP		2
#define	EPlane_BOTTOM	3 
#define	EPlane_LEFT		4
#define	EPlane_RIGHT	5

#define MinScreenSpan 0.05f
#define MaxScreenSpan 0.1f

#define MinSpan 16
#define MaxSpan	4096

#define aspectRatio		(1920.0f / 1080.0f)
#define TerrainWidth	1024 * 32
#define TerrainHeight	TerrainWidth
//#define TerrainHeight	0

#define UVSampleWidth	1.0f / float(TerrainWidth)


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
	int		DebugRenderMode;
}


struct Region_CP
{
	int4    POS             : REGION;
	int4    Parent	        : TEXCOORD0;
	float4  UVDomain        : TEXCOORD1;
	int4    TerrainInfo     : TEXCOORD2;
};


/************************************************************************************************/


Texture2D<float4> HeightMap		: register(t0);

SamplerState TextureSampler : register(s0);

struct Corner {
	float3 pos;
	float2 uv;
};


float SampleHeight(float2 UV, float sampleLevel)
{
	//float heightSample = HeightMap.GatherRed(TextureSampler, UV, int3(0, 0, 5));
	//return TerrainHeight * heightSample;
	return 0;
}


/************************************************************************************************/


float2 GetRegionUV(Region_CP region, float2 regionLocalUV)
{
	return lerp(
		region.UVDomain.xy,
		region.UVDomain.zw,
		float2(regionLocalUV.x, regionLocalUV.y));
}


float2 GetRegionPoint(Region_CP region, float2 regionLocalUV)
{
	const int2 centerPoint	= region.POS.xz;
	const int  span			= region.POS.w;
	const float2 NEPoint	= float2(centerPoint + int2( span, -span));
	const float2 SWPoint	= float2(centerPoint + int2(-span,  span));

	return lerp(SWPoint, NEPoint, regionLocalUV);
}

float3 GetRegionPoint2(Region_CP region, float2 regionLocalUV)
{
	const int2 centerPoint		= region.POS.xz;
	const int  span				= region.POS.w;
	const float2 NEPoint		= float2(centerPoint + int2( span, -span));
	const float2 SWPoint		= float2(centerPoint + int2(-span,  span));
	const float	y				= SampleHeight(GetRegionUV(region, regionLocalUV), 0);
	const float2 regionPoint	= lerp(SWPoint, NEPoint, regionLocalUV);

	return float3(regionPoint.x, y, regionPoint.y);
}

/************************************************************************************************/


Corner GetNWPoint(Region_CP CP)
{
	Corner Out;
	const float2 localRegionCord	= float2(0, 1);
	float2 UV						= GetRegionUV(CP, localRegionCord);
	float y							= SampleHeight(UV, 0);
	float2 POS						= GetRegionPoint(CP, localRegionCord);
	Out.pos							= float3(POS.x, y, POS.y);
	Out.uv							= UV;

	return Out;
}


/************************************************************************************************/


Corner GetNEPoint(Region_CP CP)
{
	Corner Out;
	const float2 localRegionCord	= float2(1, 1);
	float2 UV						= GetRegionUV(CP, localRegionCord);
	float y							= SampleHeight(UV, 0);
	float2 POS						= GetRegionPoint(CP, localRegionCord);
	Out.pos							= float3(POS.x, y, POS.y);
	Out.uv							= UV;

	return Out;
}


/************************************************************************************************/


Corner GetSWPoint(Region_CP CP)
{
	Corner Out;
	const float2 localRegionCord	= float2(0, 0);
	float2 UV						= GetRegionUV(CP, localRegionCord);
	float y							= SampleHeight(UV, 0);
	float2 POS						= GetRegionPoint(CP, localRegionCord);
	Out.pos							= float3(POS.x, y, POS.y);
	Out.uv							= UV;

	return Out;
}


/************************************************************************************************/


Corner GetSEPoint(Region_CP CP)
{
	Corner Out;
	const float2 localRegionCord	= float2(1, 0);
	float2 UV						= GetRegionUV(CP, localRegionCord);
	float y							= SampleHeight(UV, 0);
	float2 POS						= GetRegionPoint(CP, localRegionCord);
	Out.pos							= float3(POS.x, y, POS.y);
	Out.uv							= UV;

	return Out;
}


/************************************************************************************************/


float3 CalculateNormal(float3 V0, float3 V1, float3 V2)
{
	float3 normal = cross(V2 - V0, V1 - V0);
	normal = normalize(normal);
	return normal;
}


/************************************************************************************************/


float3 calculateCornerNormal(float UVSpan, float regionSpan, Corner Point, float sampleLevel)
{
	UVSpan *= 2;
	// Center point
	float Sample1 = SampleHeight(Point.uv, sampleLevel);

	// West of Center point
	float Sample2 = SampleHeight(Point.uv - float2(UVSpan, 0), sampleLevel);

	// East of Center Point
	float Sample3 = SampleHeight(Point.uv + float2(UVSpan, 0), sampleLevel);

	// South of Center Point
	float Sample4 = SampleHeight(Point.uv + float2(0, UVSpan), sampleLevel);

	// North of Center Point
	float Sample5 = SampleHeight(Point.uv - float2(0, UVSpan), sampleLevel);

	const float offset = regionSpan;
	float3 normal = float3(0, 0, 0);
	float3 v0 = float3(0, 0, 0);
	float3 v1 = float3(0, 0, 0);
	float3 v2 = float3(0, 0, 0);

	// Top Left tri
	v0 = float3(Point.pos.x - offset, Sample2, Point.pos.z);
	v1 = float3(Point.pos.x, Sample5, Point.pos.z + offset);
	v2 = float3(Point.pos.x, Sample1, Point.pos.z);
	normal += CalculateNormal(v1, v0, v2);

	// Top Right tri
	v0 = float3(Point.pos.x, Sample1, Point.pos.z);
	v1 = float3(Point.pos.x, Sample5, Point.pos.z + offset);
	v2 = float3(Point.pos.x + offset, Sample2, Point.pos.z);
	normal += CalculateNormal(v1, v0, v2);

	// Bottom Left tri
	v0 = float3(Point.pos.x - offset, Sample2, Point.pos.z);
	v1 = float3(Point.pos.x, Sample1, Point.pos.z);
	v2 = float3(Point.pos.x, Sample4, Point.pos.z - offset);
	normal += CalculateNormal(v1, v0, v2);

	// Bottom Right tri
	v0 = float3(Point.pos.x, Sample1, Point.pos.z);
	v1 = float3(Point.pos.x + offset, Sample3, Point.pos.z);
	v2 = float3(Point.pos.x, Sample4, Point.pos.z - offset);
	normal += CalculateNormal(v1, v0, v2);

	return normalize(normal / 4);
}


float GetEdgeSpan(float3 Point1, float3 Point2)
{
	const float4 P1 = mul(Proj, mul(View, float4(Point1, 1)));
	const float4 P2 = mul(Proj, mul(View, float4(Point2, 1)));

	const float3 P1SS = P1 / Max(0.00001, P1.w) * float3(aspectRatio, 1, 0);
	const float3 P2SS = P2 / Max(0.00001, P2.w) * float3(aspectRatio, 1, 0);

	const float span = length(P2SS - P1SS) / 2;
	return span;
}


float GetRegionSpan(Region_CP CP)
{
	const float4 NW = mul(Proj, mul(View, float4(GetNWPoint(CP).pos, 1)));
	const float4 NE = mul(Proj, mul(View, float4(GetNEPoint(CP).pos, 1)));
	const float4 SW = mul(Proj, mul(View, float4(GetSWPoint(CP).pos, 1)));
	const float4 SE = mul(Proj, mul(View, float4(GetSEPoint(CP).pos, 1)));

	const float3 NWSS = (NW.xyz / Max(0.00001, NW.w)) * float3( 1, 0, 1 );
	const float3 NESS = (NE.xyz / Max(0.00001, NE.w)) * float3( 1, 0, 1 );
	const float3 SESS = (SE.xyz / Max(0.00001, SE.w)) * float3( 1, 0, 1 );
	const float3 SWSS = (SW.xyz / Max(0.00001, SW.w)) * float3( 1, 0, 1 );

	const float Span1 = length(NWSS - NESS) / 2;
	const float Span2 = length(SWSS - SESS) / 2;
	const float Span3 = length(NESS - SESS) / 2;
	const float Span4 = length(NWSS - SWSS) / 2;

	float temp1 = Max(Span1, Span2);
	float temp2 = Max(Span3, Span4);
	return Max(temp1, temp2);
}


bool SplitRegion(Region_CP CP)
{
	return 
		//CP.POS.w > MaxSpan || 
		MinSpan < CP.POS.w && 
		GetRegionSpan(CP) > MaxScreenSpan;
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