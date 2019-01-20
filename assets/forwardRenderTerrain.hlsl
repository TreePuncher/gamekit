#include "terrainRenderingCommon.hlsl"

/************************************************************************************************/


Region_CP CP_PassThroughVS(Region_CP region)
{
	return region;
}



/************************************************************************************************/


struct Vertex
{
	float4 position : SV_POSITION0;
	float4 color	: COLOR0;
	float3 normal	: NORMAL0;
	float2 uv		: TEXCOORD0;
};



[maxvertexcount(6)]
void GS_RenderTerrain(point Region_CP region[1], inout TriangleStream<Vertex> vertices)
{
	Vertex v;
	float  length			= region[0].POS.w;
	float3 position;

	Corner topLeftPoint		= GetTopLeftPoint(region[0]);
	Corner topRightPoint	= GetTopRightPoint(region[0]);
	Corner bottomLeftPoint	= GetBottomLeftPoint(region[0]);
	Corner bottomRightPoint = GetBottomRightPoint(region[0]);

	float UVSpan		= (region[0].UVCords.zw - region[0].UVCords.xy).x;
	float regionSpan	= float(region[0].POS.w);
	float3 TLNormal		= calculateCornerNormal(UVSpan, regionSpan, topLeftPoint);
	float3 TRNormal		= calculateCornerNormal(UVSpan, regionSpan, topRightPoint);
	float3 BLNormal		= calculateCornerNormal(UVSpan, regionSpan, bottomLeftPoint);
	float3 BRNormal		= calculateCornerNormal(UVSpan, regionSpan, bottomRightPoint);


	if (region[0].POS.y == 0)
		v.color = float4(1, 0, 0, 0);
	if (region[0].POS.y == 1)
		v.color = float4(0, 1, 0, 0);
	if (region[0].POS.y == 2)
		v.color = float4(0, 0, 2, 0);
	if (region[0].POS.y == 3)
		v.color = float4(1, 1, 1, 0);

	v.position	= mul(PV, float4(topRightPoint.pos, 1));
	v.uv		= topRightPoint.uv;
	v.normal	= TRNormal;
	vertices.Append(v);

	v.position	= mul(PV, float4(topLeftPoint.pos, 1));
	v.uv		= topLeftPoint.uv;
	v.normal	= TLNormal;
	vertices.Append(v);

	v.position	= mul(PV, float4(bottomLeftPoint.pos, 1));
	v.uv		= bottomLeftPoint.uv;
	v.normal	= BLNormal;
	vertices.Append(v);
	vertices.RestartStrip();


	v.position	= mul(PV, float4(topRightPoint.pos, 1));
	v.uv		= topRightPoint.uv;
	v.normal	= TRNormal;
	vertices.Append(v);

	v.position	= mul(PV, float4(bottomLeftPoint.pos, 1));
	v.uv		= bottomLeftPoint.uv;
	v.normal	= BLNormal;
	vertices.Append(v);

	v.position	= mul(PV, float4(bottomRightPoint.pos, 1));
	v.uv		= bottomRightPoint.uv;
	v.normal	= BRNormal;
	vertices.Append(v);
	vertices.RestartStrip();
}


/************************************************************************************************/
// tessellation + gap filling


struct RegionTessFactors
{
	float EdgeTess[4]   : SV_TessFactor;
	float InsideTess[2] : SV_InsideTessFactor;
};


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
	return max(1, GetPostProjectionSphereExtent(m0, e0));
}


/************************************************************************************************/


float GetEdgeSpan_SS(float3 P1, float3 P2)
{
	float4 ScreenSpaceCoord_1 = mul(Proj, mul(View, float4(P1, 1)));
	float4 ScreenSpaceCoord_2 = mul(Proj, mul(View, float4(P2, 1)));

    float3 SPC1Normalized = ScreenSpaceCoord_1.xyz / ScreenSpaceCoord_1.w;
    float3 SPC2Normalized = ScreenSpaceCoord_2.xyz / ScreenSpaceCoord_1.w;

	float Span = length(abs(SPC2Normalized - SPC1Normalized))/2;

	return Span;
}


/************************************************************************************************/


int4 GetLeftNeighbor( int4 Region )
{
	int4 Neighbor = Region;
	Neighbor.x -= Region.w;

	return Neighbor;
}


/************************************************************************************************/


int4 GetTopNeighbor(int4 Region)
{
	int4 Neighbor = Region;
	Neighbor.z += Region.w;

	return Neighbor;
}


int     GetRegionDepth  (int4 R)      { return 1 + log2(MaxSpan) - log2(R.w); }
int     GetRegionDepth  (int R)       { return 1 + log2(MaxSpan) - log2(R); }
float   ExpansionRate   (float x)     { return pow(x, 1); }


/************************************************************************************************/


struct HS_Corner
{
	float3 POS			: POSITION;
	float3 Normal		: NORMAL;
	float2 UV			: TEXCOORD;
	float  UVSpan		: TEXCOORD1;
	float  regionSpan	: TEXCOORD4;
	int    CornerID		: TEXCOORD2;
};


// Domain Shader
[domain("quad")]
Vertex QuadPatchToTris(
	RegionTessFactors TessFactors,
	float2 uv : SV_DomainLocation, 
	const OutputPatch<HS_Corner, 4> bezPatch)
{ 

	float LeftMostX		= bezPatch[TopLeft].POS.x;
	float RightMostX	= bezPatch[TopRight].POS.x;
	float Topz			= bezPatch[TopLeft].POS.z;
	float Bottomz		= bezPatch[BottomLeft].POS.z;

	float x = lerp(LeftMostX, RightMostX, uv[0]);
	float z = lerp(Topz, Bottomz, uv[1]);


    float y = 0;
    {
        float y1 = bezPatch[TopLeft].POS.y;
        float y2 = bezPatch[TopRight].POS.y;
        float y3 = bezPatch[BottomLeft].POS.y;
        float y4 = bezPatch[BottomRight].POS.y;
        y = lerp(lerp(y1, y2, 1 - uv[0]), lerp(y3, y4, 1 - uv[0]), uv[1]);
    }

    float2 UV = float2(lerp(bezPatch[TopLeft].UV.x, bezPatch[TopRight].UV.x,   uv[0]),
                        lerp(bezPatch[TopLeft].UV.y, bezPatch[BottomLeft].UV.y, uv[1]));

    float2 Test = float2(UV.x, UV.y);
    y = (HeightMap.Gather(DefaultSampler, Test) * TerrainHeight);

    float4 WPOS = float4(x, y, z, 1.0f);

	Vertex Point;
	Point.position	= mul(Proj, mul(View, WPOS));



    float t = y / TerrainHeight;
    t = t * t;
    Point.color		= float4(t, t, t, 1);
	Point.uv		= UV;

	Corner C;
	C.pos	= Point.position;
	C.uv	= Point.uv;


	Point.normal = calculateCornerNormal(
		bezPatch[0].UVSpan,
		bezPatch[0].regionSpan * 2,
		C);


	if (bezPatch[0].CornerID == 0)
		Point.color *= float4(1, 0, 0, 0);
	if (bezPatch[0].CornerID == 1)
		Point.color *= float4(0, 1, 0, 0);
	if (bezPatch[0].CornerID == 2)
		Point.color *= float4(0, 0, 2, 0);
	if (bezPatch[0].CornerID == 3)
		Point.color *= float4(1, 1, 1, 0);

	return Point;
}


/************************************************************************************************/


RegionTessFactors ScreenSpaceFactors(InputPatch<Region_CP, 1> ip)
{
	RegionTessFactors factors;

	int4	Region      = ip[0].POS;
	int	    RegionWidth = Region.w * 4; 

	const int MinDepth  = log2(MaxSpan);
	const int MaxDepth  = log2(MinSpan);

	int		Depth       = GetRegionDepth(RegionWidth);
	float	MaxFactor   = pow(2, Depth - MaxDepth);
	float	MinFactor   = pow(2, Depth - MaxDepth);

	// Get Regions Points
	float3 TopLeftPoint		    = GetTopLeftPoint(ip[0]).pos     * float3(1, 0, 1);
	float3 TopRightPoint	    = GetTopRightPoint(ip[0]).pos    * float3(1, 0, 1);
	float3 BottomLeftPoint	    = GetBottomLeftPoint(ip[0]).pos  * float3(1, 0, 1);
	float3 BottomRightPoint     = GetBottomRightPoint(ip[0]).pos * float3(1, 0, 1);

    float BaseFactor = 8;
    float4 Factors   = float4(BaseFactor, BaseFactor, BaseFactor, BaseFactor); // {Right, Top, Left, Bottom}
    float4 EdgeSpans =
    {
        GetEdgeSpan_SS(TopLeftPoint,    BottomLeftPoint),   // Left Edge
        GetEdgeSpan_SS(TopLeftPoint,    TopRightPoint),     // Top Edge
        GetEdgeSpan_SS(TopRightPoint,   BottomRightPoint),  // Right Edge
        GetEdgeSpan_SS(BottomLeftPoint, BottomRightPoint)   // Bottom Edge
    }; 

    //Factors = EdgeSpans / 0.01;
    
    /*
    [unroll]
    for (int I = 0; I < 4; ++I)
    {
        int4 Comparisons =
        {
            (EdgeSpans[0] > MinScreenSpan),
            (EdgeSpans[1] > MinScreenSpan),
            (EdgeSpans[2] > MinScreenSpan),
            (EdgeSpans[3] > MinScreenSpan)
        };


        half4 Factor2   = { 2.0f, 2.0f, 2.0f, 2.0f };
        half4 DivFactor = { 0.5f, 0.5f, 0.5f, 0.5 };
        int4 One        = { 1, 1, 1, 1 };

        Factors   = Factors + (2 * Factors * Comparisons);
        EdgeSpans = half4(int4(EdgeSpans) - half4(int4(EdgeSpans) * (DivFactor * Comparisons)));
    }
    */

    float2 UVSpan = ip[0].UVCords.zw - ip[0].UVCords.xy;

    //if (RegionWidth > MaxSpan)
    //    Factors = float4(8, 8, 8, 8);

    const float BoundaryFactor = 2;


    if (ip[0].POS.y == TopRight || ip[0].POS.y == TopLeft) // Grey, Red
    {
        Region_CP Neighbor_Parent   = ip[0];
        Region_CP Neighbor_Direct_1 = ip[0];
        Region_CP Neighbor_Direct_2 = ip[0];
        Neighbor_Parent.POS.z   += ip[0].Parent.w * 2;
        Neighbor_Direct_1.POS.z += RegionWidth; // Northern Neighbor
        Neighbor_Direct_2.POS.z -= RegionWidth; // Southern Neighbor

        //Neighbor_Parent.UVCords  = ip[0].ParentUVCords;
        //Neighbor_Parent.UVCords += float4(ParentUVSpan, ParentUVSpan) * float4(0, -1, 0, -1);

        //Neighbor_Direct_1.UVCords += float4(UVSpan, UVSpan) * float4(0,  1, 0,  1);
        //Neighbor_Direct_2.UVCords += float4(UVSpan, UVSpan) * float4(0,  1, 0,  1);


        if (SplitRegion(Neighbor_Parent) && SplitRegion(Neighbor_Direct_1))
            Factors[1] *= BoundaryFactor;

        if (SplitRegion(Neighbor_Direct_2))
            Factors[3] *= BoundaryFactor;
    }

    if (ip[0].POS.y == BottomLeft || ip[0].POS.y == BottomRight) // Blue, Green
    {
        Region_CP Neighbor_Parent   = ip[0];
        Region_CP Neighbor_Direct_1 = ip[0];
        Region_CP Neighbor_Direct_2 = ip[0];
        Neighbor_Parent.POS.z   -= ip[0].Parent.w * 2;
        Neighbor_Direct_1.POS.z -= RegionWidth; // Southern Neighbor
        Neighbor_Direct_2.POS.z += RegionWidth; // Southern Neighbor

        //Neighbor_Parent.UVCords  = ip[0].ParentUVCords;
        //Neighbor_Parent.UVCords += float4(ParentUVSpan, ParentUVSpan) * float4(0, 1, 0, 1);

        //Neighbor_Direct_1.UVCords += float4(UVSpan, UVSpan) * float4(0, -1, 0, -1);
        //Neighbor_Direct_2.UVCords += float4(UVSpan, UVSpan) * float4(0, -1, 0, -1);

        if (SplitRegion(Neighbor_Parent) && SplitRegion(Neighbor_Direct_1))
            Factors[3] *= BoundaryFactor;

        if(SplitRegion(Neighbor_Direct_2))
            Factors[1] *= BoundaryFactor;
    }

    if (ip[0].POS.y == TopRight || ip[0].POS.y == BottomRight) // Right Side
    {
        Region_CP Neighbor_Parent = ip[0];
        Region_CP Neighbor_Direct_1 = ip[0];
        Region_CP Neighbor_Direct_2 = ip[0];
        Neighbor_Parent.POS.x -= ip[0].Parent.w * 2;
        Neighbor_Direct_1.POS.x -= RegionWidth;
        Neighbor_Direct_2.POS.x += RegionWidth;

        //Neighbor_Parent.UVCords  = ip[0].ParentUVCords;
        //Neighbor_Parent.UVCords += float4(ParentUVSpan, ParentUVSpan) * float4(-1, 0, -1, 0);

        //Neighbor_Direct_1.UVCords += float4(UVSpan, UVSpan) * float4(1, 0, 1, 0);
        //Neighbor_Direct_2.UVCords += float4(UVSpan, UVSpan) * float4(1, 0, 1, 0);

        if (SplitRegion(Neighbor_Parent) && SplitRegion(Neighbor_Direct_1))
            Factors[0] *= BoundaryFactor;

        if (SplitRegion(Neighbor_Direct_2))
            Factors[2] *= BoundaryFactor;
    }

    if (ip[0].POS.y == TopLeft || ip[0].POS.y == BottomLeft) // Left
    {
        Region_CP Neighbor_Parent   = ip[0];
        Region_CP Neighbor_Direct_1 = ip[0];
        Region_CP Neighbor_Direct_2 = ip[0];
        Neighbor_Parent.POS.x   += ip[0].Parent.w * 2;
        Neighbor_Direct_1.POS.x += RegionWidth;
        Neighbor_Direct_2.POS.x -= RegionWidth;

        //Neighbor_Parent.UVCords  = ip[0].ParentUVCords;
        //Neighbor_Parent.UVCords += float4(ParentUVSpan, ParentUVSpan) * float4( 1, 0, 1, 0);

        //Neighbor_Direct_1.UVCords += float4(UVSpan, UVSpan) * float4(-1, 0, -1, 0);
        //Neighbor_Direct_2.UVCords += float4(UVSpan, UVSpan) * float4(-1, 0, -1, 0);

        if (SplitRegion(Neighbor_Parent) && SplitRegion(Neighbor_Direct_1))
            Factors[2] *= BoundaryFactor;

        if (SplitRegion(Neighbor_Direct_2))
            Factors[0] *= BoundaryFactor;
    }
    
    float MaxSubFactor    = 32;
    float4 MaxSubFactors  = float4(MaxSubFactor, MaxSubFactor, MaxSubFactor, MaxSubFactor);
    float4 ClampedFactors = max(float4(1.0f, 1.0f, 1.0f, 1.0f), min(Factors, MaxSubFactors));

    [unroll]
    for (int II = 0; II < 4; ++II)
        factors.EdgeTess[II] = ClampedFactors[II];

    factors.InsideTess[0] = min(float((Factors[1] + Factors[3])) / 2.0f, MaxSubFactor); // North South
    factors.InsideTess[1] = min(float((Factors[0] + Factors[2])) / 2.0f, MaxSubFactor); // East West

	return factors;
}



// Hull Shader - Per Patch
[domain("quad")]
[partitioning("fractional_even")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ScreenSpaceFactors")]
[maxtessfactor(64.0f)]
HS_Corner RegionToQuadPatch(
	InputPatch<Region_CP, 1> ip,
	const uint i		: SV_OutputControlPointID,
	const uint PatchID	: SV_PrimitiveID)
{
	HS_Corner Output;
	const float3 Scale = float3(1, 1, 1);

    float2	UV_TL		= ip[0].UVCords.xy;
    float2	UV_BR		= ip[0].UVCords.zw;
	float	UVSpan		= (UV_BR - UV_TL).x;
	float   regionSpan	= ip[0].POS.w;

	Output.UVSpan		= UVSpan;
	Output.regionSpan	= regionSpan;

    if (i == TopLeft)
    {
		Corner Point	= GetTopLeftPoint(ip[0]);
		Output.POS		= Point.pos;
        Output.UV		= Point.uv;
    }
    else if (i == TopRight) 
    {
		Corner Point	= GetTopRightPoint(ip[0]);
		Output.POS		= Point.pos;
		Output.UV		= Point.uv;
	}
	else if (i == BottomRight)
    {
		Corner Point	= GetBottomRightPoint(ip[0]);
		Output.POS		= Point.pos;
		Output.UV		= Point.uv;
	}
	else if (i == BottomLeft)
    {
		Corner Point	= GetBottomLeftPoint(ip[0]);
		Output.POS		= Point.pos;
		Output.UV		= Point.uv;
	}

	Output.CornerID = ip[0].POS.y;

	return Output;
}



/************************************************************************************************/
// Shading


float4 PS_RenderTerrain(Vertex input) : SV_TARGET
{
	return  float4(input.normal, 1);

	//float A = dot(float3(0, 1, 0), input.normal);
	//return  pow(A * input.color, 2.1);
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