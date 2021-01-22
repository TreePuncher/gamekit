#include "terrainRenderingCommon.hlsl"
#include "pbr.hlsl"


/************************************************************************************************/

// Defines
#define Left	0
#define Top		1
#define Right	2
#define Bottom	3


int     GetRegionDepth(int4 R) { return 1 + log2(MaxSpan) - log2(R.w); }
int     GetRegionDepth(int R) { return 1 + log2(MaxSpan) - log2(R); }
float   ExpansionRate(float x) { return pow(x, 1); }


/************************************************************************************************/


float GetPostProjectionSphereExtent(float3 Origin, float Diameter)
{
	float4 ClipPos = mul(View * Proj, float4(Origin, 1.0));
	return abs(Diameter * Proj[1][1] / ClipPos.w);
}


/************************************************************************************************/


float CalculateTessellationFactor(float3 Control0, float3 Control1)
{
	float	e0 = distance(Control0, Control1);
	float3	m0 = (Control0 + Control1) / 2;
	return Max(1, GetPostProjectionSphereExtent(m0, e0));
}


/************************************************************************************************/


float GetEdgeSpan_SS(float3 P1, float3 P2)
{
	float4 ScreenSpaceCoord_1 = mul(Proj, mul(View, float4(P1, 1)));
	float4 ScreenSpaceCoord_2 = mul(Proj, mul(View, float4(P2, 1)));

    float2 SPC1Normalized = float3(aspectRatio, 1, 1) * ScreenSpaceCoord_1.xyz / ScreenSpaceCoord_1.w;
    float2 SPC2Normalized = float3(aspectRatio, 1, 1) * ScreenSpaceCoord_2.xyz / ScreenSpaceCoord_1.w;

	float Span = length(abs(SPC2Normalized - SPC1Normalized));

	return Span;
}


/************************************************************************************************/


Region_CP CP_PassThroughVS(Region_CP region)
{
	return region;
}


/************************************************************************************************/


struct Vertex
{
	float4 position		: SV_POSITION0;
	float4 color		: COLOR0;
	float3 positionW	: POSITION0;
	float3 normal		: NORMAL0;
	float2 uv			: TEXCOORD0;
};


/************************************************************************************************/


Vertex CreateVertexPoint(Region_CP region, float2 UV)
{
	Vertex v;

	float3 pos	= GetRegionPoint2(region, UV);
	v.position	= mul(PV, float4(pos, 1)); // North East
	v.positionW = pos;
	v.uv		= GetRegionUV(region, UV);
	v.normal	= float3(0, 1, 0);
	v.color		= float4(v.uv, 0, 0);

	if (region.POS.y == NorthEast)
		v.color = float4(1, 0, 0, 0);
	if (region.POS.y == NorthWest)
		v.color = float4(0, 1, 0, 0);
	if (region.POS.y == SouthWest)
		v.color = float4(0, 0, 1, 0);
	if (region.POS.y == SouthEast)
		v.color = float4(1, 1, 1, 0);

	return v;
}


// Takes a screen span (0 - 1) -> (0 - 0.5)
float CalculateSplitFactor(float Span)
{
	return saturate((Span - MaxScreenSpan / 2) / MaxScreenSpan);
}

[maxvertexcount(24)]
void GS_RenderTerrain(point Region_CP region[1], inout TriangleStream<Vertex> vertices)
{
	Vertex v;
	float  length		= region[0].POS.w;
	float3 position;

	Corner NorthEastCorner = GetNEPoint(region[0]);
	Corner NorthWestCorner = GetNWPoint(region[0]);
	Corner SouthWestCorner = GetSWPoint(region[0]);
	Corner SouthEastCorner = GetSEPoint(region[0]);

	float northSpan		= GetEdgeSpan(NorthWestCorner.pos, NorthEastCorner.pos);
	float southSpan		= GetEdgeSpan(SouthWestCorner.pos, SouthEastCorner.pos);

	float westSpan		= GetEdgeSpan(NorthWestCorner.pos, SouthWestCorner.pos);
	float eastSpan		= GetEdgeSpan(NorthEastCorner.pos, SouthEastCorner.pos);

	float northMorphFactor		= CalculateSplitFactor(northSpan);
	float southMorphFactor		= CalculateSplitFactor(southSpan);
	float2 CenterPointUV		= (northMorphFactor + southMorphFactor) / 2;

	float UVSpan		= GetRegionPoint(region[0], float2(0.5f, 0.5f));
	float regionSpan	= float(region[0].POS.w);

	float3 NENormal		= calculateCornerNormal(UVSpan, regionSpan, NorthEastCorner, 0);
	float3 NWNormal		= calculateCornerNormal(UVSpan, regionSpan, NorthWestCorner, 0);
	float3 SWNormal		= calculateCornerNormal(UVSpan, regionSpan, SouthWestCorner, 0);
	float3 SENormal		= calculateCornerNormal(UVSpan, regionSpan, SouthEastCorner, 0);

	// NorthEast
	/************************************************************************************************/

	vertices.Append(CreateVertexPoint(region[0], float2(0.0f, 1.0f)));
	vertices.Append(CreateVertexPoint(region[0], float2(northMorphFactor, 1.0f)));
	vertices.Append(CreateVertexPoint(region[0], CenterPointUV));
	vertices.RestartStrip();

	vertices.Append(CreateVertexPoint(region[0], CenterPointUV));
	vertices.Append(CreateVertexPoint(region[0], float2(0.0f, CenterPointUV.y)));
	vertices.Append(CreateVertexPoint(region[0], float2(0.0f, 1.0f)));
	vertices.RestartStrip();

	// NorthEast
	/************************************************************************************************/

	vertices.Append(CreateVertexPoint(region[0], float2(northMorphFactor, 1.0f)));
	vertices.Append(CreateVertexPoint(region[0], float2(1.0f, 1.0f)));
	vertices.Append(CreateVertexPoint(region[0], CenterPointUV));
	vertices.RestartStrip();

	vertices.Append(CreateVertexPoint(region[0], CenterPointUV));
	vertices.Append(CreateVertexPoint(region[0], float2(1.0f, 1.0f)));
	vertices.Append(CreateVertexPoint(region[0], float2(1.0f, CenterPointUV.y)));
	vertices.RestartStrip();

	// SouthWest
	/************************************************************************************************/

	vertices.Append(CreateVertexPoint(region[0], float2(0.0f, CenterPointUV.y)));
	vertices.Append(CreateVertexPoint(region[0], CenterPointUV));
	vertices.Append(CreateVertexPoint(region[0], float2(0, 0)));
	vertices.RestartStrip();

	vertices.Append(CreateVertexPoint(region[0], float2(0.0f, 0.0f)));
	vertices.Append(CreateVertexPoint(region[0], CenterPointUV));
	vertices.Append(CreateVertexPoint(region[0], float2(southMorphFactor, 0)));
	vertices.RestartStrip();

	// SouthEast
	/************************************************************************************************/

	vertices.Append(CreateVertexPoint(region[0], CenterPointUV));
	vertices.Append(CreateVertexPoint(region[0], float2(1, CenterPointUV.y)));
	vertices.Append(CreateVertexPoint(region[0], float2(1, 0)));
	vertices.RestartStrip();

	vertices.Append(CreateVertexPoint(region[0], CenterPointUV));
	vertices.Append(CreateVertexPoint(region[0], float2(1, 0)));
	vertices.Append(CreateVertexPoint(region[0], float2(southMorphFactor, 0)));
	vertices.RestartStrip();
}


/************************************************************************************************/
// Shading
float4 PS_RenderTerrain(Vertex input) : SV_TARGET
{
	//return (input.uv, 1, 1);
	float3 positionW	= input.positionW;
	float3 vdir			= normalize(CameraPOS.xyz - positionW);

	// Test Spot Light
	// Lighting parameters
	const float3 Lc			= float3(0.9f, 0.9f, 0.9f);
	const float3 Lp			= float3(0, 15, 30);
	const float3 Lv			= float3(0, 1, 0);//normalize(Lp - positionW);
	const float  ld			= length(Lp - positionW);
	const float  Li			= 1.0f;
	const float  La			= Li;//Li / Max((ld * ld), 0.00001f);

	// surface parameters
	const float h			= positionW.y / TerrainHeight;
	const float2 uv			= input.uv;
	const float3 Kd			= float3(0.3f, 0.3f, 0.3f);//pow(float3(196.0f/256.0f, 135.0f / 256.0f, 13.0f / 256.0f), 2.1f);//float3( sin(h * 10 * pi) * sin(h * 0.2 * pi), sin(h * 10 * pi) * sin(h * 0.2 * pi), sin(h * 10 * pi) * sin(h * 0.2 * pi)); // albedo
	//const float3 Kd			= pow(float3(196.0f/256.0f, 135.0f / 256.0f, 13.0f / 256.0f), 2.1f);//float3( sin(h * 10 * pi) * sin(h * 0.2 * pi), sin(h * 10 * pi) * sin(h * 0.2 * pi), sin(h * 10 * pi) * sin(h * 0.2 * pi)); // albedo
	const float3 Ks			= float3(1.0f, 1.0f, 1.0f); // Specular
	const float3 Pss 		= float3(0.5f, 0.5f, 0.5f);
	const float3 n			= normalize(input.normal);
	const float  roughness	= 0.9f;
	const float  metallic	= 0.0f;
	//return float4(saturate(float3(0, 1, 0) * dot(n, float3(0, 1, 0)) * dot(vdir, n)), 0);

	float3 Fr	= 
		BRDF(
			Lv, 
			Lc,
			vdir,
			positionW,
			Kd,
			n, 
			Ks, 
			metallic,
			roughness);

	//return float4(pow(input.color.xyz * dot(n, Lv) * dot(n, vdir), 1.0f), 1);
	float3 Color = saturate(La * Fr * dot(n, vdir) *  dot(n, Lv));
	return float4(input.color.xyz * pow(Color, 1/2.1), 1);
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