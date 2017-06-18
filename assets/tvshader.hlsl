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

// Defines
#define TopLeft		0
#define TopRight	1
#define BottomRight	2
#define BottomLeft	3

// ---------------------------------------------------------------------------

#define	EPlane_FAR		0
#define	EPlane_NEAR		1
#define	EPlane_TOP		2
#define	EPlane_BOTTOM	3 
#define	EPlane_LEFT		4
#define	EPlane_RIGHT	5

#define MinScreenSpan 0.005f
#define MaxScreenSpan 0.05f

#define MinSpan 16
#define MaxSpan	2048
	  

#define TerrainHeight 1024

// Constant Buffers
// ---------------------------------------------------------------------------


struct Plane
{
	float4 Normal;
	float4 Orgin;
};


// ---------------------------------------------------------------------------


cbuffer CameraConstants : register(b0)
{
    float4x4 View;
    float4x4 ViewI;
    float4x4 Proj;
    float4x4 PV; // Projection x View
    float4x4 PVI; // Projection x View
    float4 CameraPOS;
    float MinZ;
    float MaxZ;
    float PointLightCount;
    float SpotLightCount;
    float WindowWidth;
    float WindowHeight;

    float4 WSTopLeft;
    float4 WSTopRight;
    float4 WSBottomLeft;
    float4 WSBottomRight;

    float4 WSTopLeft_Near;
    float4 WSTopRight_Near;
    float4 WSBottomLeft_Near;
    float4 WSBottomRight_Near;
};

// ---------------------------------------------------------------------------


cbuffer TerrainConstants : register( b1 )
{
	float4 Albedo;   // + roughness
	float4 Specular; // + metal factor
    float2 RegionDimensions;
	Plane Frustum[6];
    int   SplitCount;
};


// ---------------------------------------------------------------------------
// Texture Inputs
Texture2D<float> HeightMap : register(t0);

// ---------------------------------------------------------------------------
// Samplers

sampler DefaultSampler : register(s0);
sampler NearestPoint   : register(s1);

// ---------------------------------------------------------------------------
// Structs

struct VertexIn
{
	int4    POS 	        : REGION;
	int4    Parent	        : TEXCOORD0;
    float4  UVCords         : TEXCOORD1;
    float4  ParentUVCords   : TEXCOORD2;
};


struct Region_CP
{
	int4    POS             : REGION;
	int4    Parent	        : TEXCOORD0;
    float4  UVCords         : TEXCOORD1;
    float4 ParentUVCords    : TEXCOORD2;

};


struct GeometryOut
{
	float4 POS 	: POSITION0;
};


struct PS_IN
{
	float3 WPOS 	: TEXCOORD0;
	float4 N 		: TEXCOORD1;
	float4 POS 	 	: SV_POSITION;
};

struct PS_Colour_IN
{
	float3 WPOS 	: TEXCOORD0;
	float3 Colour	: TEXCOORD1;
	float3 N 		: TEXCOORD2;
	float4 POS 	 	: SV_POSITION;
};


struct PS_OUT
{
	float4 COLOUR : SV_TARGET;
};


struct SO_OUT
{
	int4 POS 	         : REGION;
	int4 Parent	         : TEXCOORD;
    float4 UVCords       : TEXCOORD1;
    float4 ParentUVCords : TEXCOORD2;

};


// ---------------------------------------------------------------------------


bool CompareAgainstFrustum(float3 V, float r)
{
	bool Near	= false;
	bool Far	= false;
	bool Left	= false;
	bool Right	= false;
    bool Top    = false;
    bool Bottom = true;


	{
		float3 P = V -	Frustum[EPlane_NEAR].Orgin.xyz;
		float3 N =		Frustum[EPlane_NEAR].Normal.xyz;

		float NdP	= dot(N, P);
		float D		= NdP - r;
		Near		= D <= 0;
	}
	{
		float3 P = V -	Frustum[EPlane_FAR].Orgin.xyz;
		float3 N =		Frustum[EPlane_FAR].Normal.xyz;

		float NdP	= dot(N,P);
		float D		= NdP - r;
		Far			= D <= 0;
	}
	{
		float3 P = V -	Frustum[EPlane_LEFT].Orgin.xyz;
		float3 N =		Frustum[EPlane_LEFT].Normal.xyz;

		float NdP	= dot(N,P);
		float D		= NdP - r;
		Left		= D <= 0;
	}
	{
		float3 P = V -	Frustum[EPlane_RIGHT].Orgin.xyz;
		float3 N =		Frustum[EPlane_RIGHT].Normal.xyz;

		float NdP	= dot(N,P);
		float D		= NdP - r;
		Right		= D <= 0;
	}
    {
        float3 P = V - Frustum[EPlane_TOP].Orgin.xyz;
        float3 N =     Frustum[EPlane_TOP].Normal.xyz;

        float NdP = -dot(N, P);
        float D = NdP - r;
        Top = D <= 0;
    }
    {
        float3 P = V - Frustum[EPlane_BOTTOM].Orgin.xyz;
        float3 N =     Frustum[EPlane_BOTTOM].Normal.xyz;

        float NdP = -dot(N, P);
        float D = NdP - r;
        Bottom  = D <= 0;
    }

	return !(Near & Far & Left & Right & Bottom);
}


// ---------------------------------------------------------------------------


float3 GetTopLeftPoint(Region_CP CP)
{
	float3 Out     = float3(0, 0, 0);
	float offset   = float(CP.POS.w);

    float2 UV_TL   = CP.UVCords.xy;
    float2 UV_BR   = CP.UVCords.zw;
    float2 UV_Span = UV_BR - UV_TL;

    float2 UV      = UV_TL;
    float2 Test    = float2(1 - UV.x, 1 - UV.y);
    float y        = HeightMap.Gather(NearestPoint, Test) * TerrainHeight;

	Out.x          = CP.POS.x - offset;
    Out.y          = y;
	Out.z          = CP.POS.z + offset;

	return Out;
}

float3 GetTopRightPoint(Region_CP CP)
{
	float3 Out     = float3(0, 0, 0);
	float offset   = float(CP.POS.w);
    float2 UV_TL   = CP.UVCords.xy;
    float2 UV_BR   = CP.UVCords.zw;
    float2 UV_Span = UV_BR - UV_TL;

    float2 UV      = UV_TL + float2(UV_Span.x, 0.0f);
    float2 Test    = float2(1 - UV.x, 1 - UV.y);
    float y        = HeightMap.Gather(NearestPoint, Test) * TerrainHeight;

	Out.x          = CP.POS.x + offset;
    Out.y          = y;
	Out.z          = CP.POS.z + offset;

	return Out;
}

float3 GetBottomLeftPoint(Region_CP CP)
{
	float3 Out     = float3(0, 0, 0);
	float offset   = float(CP.POS.w);
    float2 UV_TL   = CP.UVCords.xy;
    float2 UV_BR   = CP.UVCords.zw;
    float2 UV_Span = UV_BR - UV_TL;

    float2 UV      = UV_BR - float2(UV_Span.x, 0.0f);

    float2 Test    = float2(1 - UV.x, 1 - UV.y);
    float y        = HeightMap.Gather(NearestPoint, Test) * TerrainHeight;

	Out.x = CP.POS.x - offset;
    Out.y = y;
	Out.z = CP.POS.z - offset;

	return Out;
}

float3 GetBottomRightPoint(Region_CP CP)
{
	float3 Out     = float3(0, 0, 0);
	float offset   = float(CP.POS.w);
    float2 UV_TL   = CP.UVCords.xy;
    float2 UV_BR   = CP.UVCords.zw;
    float2 UV_Span = UV_BR - UV_TL;

    float2 UV      = UV_BR;
    float2 Test    = float2(1 - UV.x, 1 - UV.y);
    float y        = HeightMap.Gather(NearestPoint, Test) * TerrainHeight;

	Out.x          = CP.POS.x + offset;
    Out.y          = y;
	Out.z          = CP.POS.z - offset;

	return Out;
}


// ---------------------------------------------------------------------------


Region_CP VPassThrough( VertexIn IN )
{
	Region_CP OUT;
	OUT.POS 	      = IN.POS;
	OUT.Parent	      = IN.Parent;
    OUT.UVCords       = IN.UVCords;
    OUT.ParentUVCords = IN.ParentUVCords;

	return OUT;
}


// ---------------------------------------------------------------------------


float GetRegionSpan(Region_CP CP)
{
	float4 TL = mul(Proj, mul(View, float4(GetTopLeftPoint(CP)     * float3(1, 0, 1), 1)));
    float4 TR = mul(Proj, mul(View, float4(GetTopRightPoint(CP)    * float3(1, 0, 1), 1)));

    float4 BL = mul(Proj, mul(View, float4(GetBottomLeftPoint(CP)  * float3(1, 0, 1), 1)));
    float4 BR = mul(Proj, mul(View, float4(GetBottomRightPoint(CP) * float3(1, 0, 1), 1)));

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


// ---------------------------------------------------------------------------


PS_IN CPVisualiser( VertexIn IN )
{
	PS_IN OUT;
	OUT.POS = float4(float(IN.POS.x)/20, float(IN.POS.z)/20, 0, 0 );
	return OUT;
}


// ---------------------------------------------------------------------------


Region_CP MakeTopLeft(Region_CP IN)
{
	Region_CP R;
	int offset = uint(IN.POS.w)/2; // Position Height
    
    float2 UVTL     = IN.UVCords.xy;
    float2 UVBR     = IN.UVCords.zw;
    float2 UVWH     = (UVBR - UVTL) / 2; // Width Height of New Tile UV
    float2 UVCenter = (UVBR + UVTL) / 2; // Center of Parent

	R.Parent        = IN.POS;
    R.ParentUVCords = IN.UVCords;
    R.UVCords       = float4(UVCenter - UVWH, UVCenter); //  {{Top Left}, {Bottom Right}}

	R.POS 	= IN.POS;
	R.POS.x = R.POS.x + offset;
	R.POS.z = R.POS.z + offset;
	R.POS.w = offset;
    R.POS.y = TopLeft;
   
	return R;
}


// ---------------------------------------------------------------------------



Region_CP MakeTopRight(Region_CP IN)
{
	Region_CP R;
	int offset = uint(IN.POS.w)/2;

    float2 UVTL     = IN.UVCords.xy;
    float2 UVBR     = IN.UVCords.zw;
    float2 UVWH     = (UVBR - UVTL)/2;
    float2 UVCenter = (UVBR + UVTL)/2;
    
	R.Parent        = IN.POS;
    R.ParentUVCords = IN.UVCords;
    R.UVCords = float4(UVCenter + UVWH * float2(0, -1), UVCenter + UVWH * float2(1, 0)); //  {{Top Left}, {Bottom Right}}

	R.POS 	= IN.POS;
	R.POS.x = R.POS.x - offset;
	R.POS.z = R.POS.z + offset;
	R.POS.w = offset;
	R.POS.y = TopRight;

	return R;
}


// ---------------------------------------------------------------------------


Region_CP MakeBottomLeft(Region_CP IN)
{
    Region_CP R;
    int offset = uint(IN.POS.w) / 2;

    float2 UVTL     = IN.UVCords.xy;
    float2 UVBR     = IN.UVCords.zw;
    float2 UVWH     = (UVBR - UVTL)/2;
    float2 UVCenter = (UVBR + UVTL)/2;

    R.Parent        = IN.POS;
    R.ParentUVCords = IN.UVCords;
    R.UVCords       = float4(UVCenter - UVWH * float2(1, 0), UVCenter + UVWH * float2(0, 1));

    R.POS   = IN.POS;
    R.POS.x = R.POS.x + offset;
    R.POS.z = R.POS.z - offset;
    R.POS.w = offset;
    R.POS.y = BottomLeft;

    return R;
}


// ---------------------------------------------------------------------------



Region_CP MakeBottomRight(Region_CP IN)
{
	Region_CP R;
	int offset = uint(IN.POS.w)/2;

    float2 UVTL     = IN.UVCords.xy;
    float2 UVBR     = IN.UVCords.zw;
    float2 UVWH     = (UVBR - UVTL) / 2;
    float2 UVCenter = (UVBR + UVTL) / 2;

	R.Parent        = IN.POS;
    R.ParentUVCords = IN.UVCords;
    R.UVCords       = float4(UVCenter, UVCenter + UVWH); //  {{Top Left}, {Bottom Right}}

	R.POS 	= IN.POS;
	R.POS.x = R.POS.x - offset;
	R.POS.z = R.POS.z - offset;
	R.POS.w = offset;

	R.POS.y = BottomRight;
    //R.POS.y = R.Parent.y + 1;

	return R;
}


// ---------------------------------------------------------------------------


bool CullRegion(Region_CP R){ 
    float3 CenterPosition   = R.POS.xyz;
    bool TopPlane           = CompareAgainstFrustum(CenterPosition, R.POS.w * 2);

    float3 BL = GetBottomLeftPoint  (R);
    float3 BR = GetBottomRightPoint (R);
    float3 TL = GetTopLeftPoint     (R);
    float3 TR = GetTopRightPoint    (R);

    float  Radius = length(BL - TR)/2;
    float3 POS = (BL - TR) / 2;

    return TopPlane;
    //CompareAgainstFrustum(POS, Radius); //| BottomPlane;
}


// ---------------------------------------------------------------------------


[maxvertexcount(4)]
void GS_Split(
	point Region_CP IN[1], 
	inout PointStream<SO_OUT> IntermediateBuffer,
	inout PointStream<SO_OUT> FinalBuffer)
{	// TODO: Visibility Determination
	if(!CullRegion(IN[0]))
	{
		if (SplitRegion(IN[0]))
		{
            IntermediateBuffer.Append(MakeTopLeft(IN[0]));
            IntermediateBuffer.Append(MakeTopRight(IN[0]));
            IntermediateBuffer.Append(MakeBottomRight(IN[0]));
            IntermediateBuffer.Append(MakeBottomLeft(IN[0]));
            IntermediateBuffer.RestartStrip();
        } else {
			FinalBuffer.Append(IN[0]);
			FinalBuffer.RestartStrip();
		}
	}
}


// ---------------------------------------------------------------------------


struct HS_Corner
{
	float3 POS	    : POSITION;
	float3 Normal   : NORMAL;
	float2 UV	    : TEXCOORD;
	int    CornerID : TEXCOORD1;
};


// ---------------------------------------------------------------------------


struct RegionTessFactors
{
	float EdgeTess[4]   : SV_TessFactor;
	float InsideTess[2] : SV_InsideTessFactor;
};


// ---------------------------------------------------------------------------


struct Corner
{
	float3 POS : POSITION;
};


// ---------------------------------------------------------------------------


float GetPostProjectionSphereExtent(float3 Origin, float Diameter)
{
	float4 ClipPos = mul(View * Proj, float4(Origin, 1.0));
	return abs(Diameter * Proj[1][1] / ClipPos.w);
}


// ---------------------------------------------------------------------------


float CalculateTessellationFactor(float3 Control0, float3 Control1)
{
	float	e0 = distance(Control0, Control1);
	float3	m0 = (Control0 + Control1) / 2;
	return max(1, GetPostProjectionSphereExtent(m0, e0));
}


// ---------------------------------------------------------------------------


float GetEdgeSpan_SS(float3 P1, float3 P2)
{
	float4 ScreenSpaceCoord_1 = mul(Proj, mul(View, float4(P1, 1)));
	float4 ScreenSpaceCoord_2 = mul(Proj, mul(View, float4(P2, 1)));

    float3 SPC1Normalized = ScreenSpaceCoord_1.xyz / ScreenSpaceCoord_1.w;
    float3 SPC2Normalized = ScreenSpaceCoord_2.xyz / ScreenSpaceCoord_1.w;

	float Span = length(abs(SPC2Normalized - SPC1Normalized))/2;

	return Span;
}


int4 GetLeftNeighbor( int4 Region )
{
	int4 Neighbor = Region;
	Neighbor.x -= Region.w;

	return Neighbor;
}


int4 GetTopNeighbor(int4 Region)
{
	int4 Neighbor = Region;
	Neighbor.z += Region.w;

	return Neighbor;
}

int     GetRegionDepth  (int4 R)      { return 1 + log2(MaxSpan) - log2(R.w);}
int     GetRegionDepth  (int R)       { return 1 + log2(MaxSpan) - log2(R);}
float   ExpansionRate   (float x)     { return pow(x, 1); }


// Hull Shader - Per Control Point
RegionTessFactors ScreenSpaceFactors(InputPatch<Region_CP, 1> ip)
{
	RegionTessFactors factors;

	int4	Region      = ip[0].POS;
	int	    RegionWidth = Region.w * 2; 

	const int MinDepth = log2(MaxSpan);
	const int MaxDepth = log2(MinSpan);

	int		Depth = GetRegionDepth(RegionWidth);
	float	MaxFactor = pow(2, Depth - MaxDepth);
	float	MinFactor = pow(2, Depth - MaxDepth);

	// Get Regions Points
	float3 TopLeftPoint		= GetTopLeftPoint(ip[0])     * float3(1, 0, 1);
	float3 TopRightPoint	= GetTopRightPoint(ip[0])    * float3(1, 0, 1);
	float3 BottomLeftPoint	= GetBottomLeftPoint(ip[0])  * float3(1, 0, 1);
	float3 BottomRightPoint = GetBottomRightPoint(ip[0]) * float3(1, 0, 1);

    float BaseFactor = 8;
    float4 Factors   = float4(BaseFactor, BaseFactor, BaseFactor, BaseFactor); // {Right, Top, Left, Bottom}
    float4 EdgeSpans =
    {
        GetEdgeSpan_SS(TopLeftPoint, BottomLeftPoint),      // Left Edge
        GetEdgeSpan_SS(TopLeftPoint, TopRightPoint),        // Top Edge
        GetEdgeSpan_SS(TopRightPoint, BottomRightPoint),    // Right Edge
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

    float2 UVSpan       = ip[0].UVCords.zw - ip[0].UVCords.xy;
    float2 ParentUVSpan = ip[0].ParentUVCords.zw - ip[0].ParentUVCords.xy;

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
        {
            Factors[1] *= BoundaryFactor;
        }
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
        {
            Factors[3] *= BoundaryFactor;
        } 
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

        if (SplitRegion(Neighbor_Parent) && SplitRegion(Neighbor_Direct_1)){
            Factors[0] *= BoundaryFactor;
        }
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

        if (SplitRegion(Neighbor_Parent) && SplitRegion(Neighbor_Direct_1)){
            Factors[2] *= BoundaryFactor;
        }
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


// ---------------------------------------------------------------------------


// Hull Shader - Per Patch
[domain("quad")]
[partitioning("fractional_even")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ScreenSpaceFactors")]
[maxtessfactor(64.0f)]
HS_Corner RegionToQuadPatch(
	InputPatch<Region_CP, 1> ip,
	const uint i			: SV_OutputControlPointID,
	const uint PatchID	: SV_PrimitiveID)
{
	HS_Corner Output;
	const float3 Scale = float3(1, 1, 1);

    float2 UV_TL = ip[0].UVCords.xy;
    float2 UV_BR = ip[0].UVCords.zw;
    float2 UV_Span = UV_BR - UV_TL;

    if (i == TopLeft)
    {
        Output.POS = Scale * GetTopLeftPoint(ip[0]);
        Output.UV = UV_TL;
    }
    else if (i == TopRight) 
    {
        Output.POS = Scale * GetTopRightPoint(ip[0]);
        Output.UV = UV_TL + float2(UV_Span.x, 0.0f);
    }
	else if (i == BottomRight)
    {
        Output.POS = Scale * GetBottomRightPoint(ip[0]);
        Output.UV = UV_BR;
    }
	else if (i == BottomLeft)
    {
        Output.POS = Scale * GetBottomLeftPoint(ip[0]);
        Output.UV = UV_BR - float2(UV_Span.x, 0.0f);
    }

	Output.CornerID = ip[0].POS.y;

	return Output;
}


// ---------------------------------------------------------------------------


/*
struct PS_IN
{
float3 WPOS 	: TEXCOORD0;
float4 N 		: TEXCOORD1;
float4 POS 	 	: SV_POSITION;
};
*/


// ---------------------------------------------------------------------------

// Domain Shader
[domain("quad")]
PS_Colour_IN QuadPatchToTris(
	RegionTessFactors TessFactors,
	float2 uv : SV_DomainLocation, 
	const OutputPatch<HS_Corner, 4> bezPatch)
{ 
	PS_Colour_IN Point;

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
        //y = lerp(lerp(y1, y2, 1 - uv[0]), lerp(y3, y4, 1 - uv[0]), uv[1]);
    }

    float2 UV = float2(lerp(bezPatch[TopLeft].UV.x, bezPatch[TopRight].UV.x,   1 - uv[0]),
                        lerp(bezPatch[TopLeft].UV.y, bezPatch[BottomLeft].UV.y, uv[1]));

    float2 Test = float2(1 - UV.x, 1 - UV.y);
    y = HeightMap.Gather(NearestPoint, Test) * TerrainHeight;

    float4 WPOS = float4(x, y, z, 1.0f);

	Point.WPOS	= WPOS.xyz;
	Point.N		= float4(0.0f, 1.0f, 0.0f, 0.0f);
	Point.POS	= mul(Proj, mul(View, WPOS));

    float t = y / TerrainHeight;
    t = t * t;
    Point.Colour = float3(t, t, t);
	return Point;
}

// Domain Shader
[domain("quad")]
PS_Colour_IN QuadPatchToTris_DEBUG(
	RegionTessFactors TessFactors,
	float2 uv : SV_DomainLocation,
	const OutputPatch<HS_Corner, 4> bezPatch)
{
    PS_Colour_IN Point;

    float LeftMostX  = bezPatch[TopLeft].POS.x;
    float RightMostX = bezPatch[TopRight].POS.x;
    float Topz       = bezPatch[TopLeft].POS.z;
    float Bottomz    = bezPatch[BottomLeft].POS.z;

    float x = lerp(LeftMostX, RightMostX, uv[0]);
    float z = lerp(Topz, Bottomz, uv[1]);
    float y = 1.0f;
    //bezPatch[TopLeft].POS.y;

	{
		float y1 = bezPatch[TopLeft].POS.y;
		float y2 = bezPatch[TopRight].POS.y;
		float y3 = bezPatch[BottomLeft].POS.y;
		float y4 = bezPatch[BottomRight].POS.y;
        y = lerp( lerp(y1, y2, 1 - uv[0]), lerp(y3, y4, 1 - uv[0]), uv[1]) + 1.0f;
    }

    float2 UV = float2(lerp(bezPatch[TopLeft].UV.x, bezPatch[TopRight].UV.x, 1 - uv[0]),
                       lerp(bezPatch[TopLeft].UV.y, bezPatch[BottomLeft].UV.y, uv[1]));

    float2 Test = float2(1 - UV.x, 1 - UV.y);
    y = HeightMap.Gather(NearestPoint, Test) * TerrainHeight;

    float4 WPOS = float4(x, y, z, 1.0f);

    Point.WPOS  = WPOS.xyz;
    Point.N     = float4(0.0f, 1.0f, 0.0f, 0.0f);
    Point.POS   = mul(Proj, mul(View, WPOS));

    float t = 1 - y * 1 / -TerrainHeight;

#if 0
    t = t * t;
    //Point.Colour = float3(t, t, t);
    Point.Colour = float3(UV.x, UV.x, UV.x);
#else

    int TileID = bezPatch[TopLeft].CornerID;
    if (TileID == TopLeft)
        Point.Colour = float3(.5, .5, .5);

    if (TileID == TopRight)
        Point.Colour = float3(1, 0, 0);

    if (TileID == BottomLeft)
        Point.Colour = float3(0, 1, 0);

    if (TileID == BottomRight)
        Point.Colour = float3(0, 0, 1);

#endif
    //Point.Colour *= t * t;

    return Point;
}


// ---------------------------------------------------------------------------

// Geometry Shader
[maxvertexcount(6)]
void RegionToTris(
	point Region_CP IN[1], 
	inout TriangleStream<PS_IN> OUT )
{
	PS_IN v;
	float3 N = float3(0, 1, 0);
	float offset = IN[0].POS.w;

	v.N		= float4(N, 0);//mul(Proj, mul(View, N));
	//v.N   = mul(Proj, mul(View, N));

	v.POS	= float4(0, 0, 0, 1);
	v.POS.x = float(IN[0].POS.x + offset);
	v.POS.z = float(IN[0].POS.z + offset);
	v.POS.y = 0.0f;
	v.WPOS	= v.POS.xyz;
	v.POS	= mul(Proj, mul(View, v.POS));
	OUT.Append(v);

	v.POS	= float4(0, 0, 0, 1);
	v.POS.x = float(IN[0].POS.x - offset);
	v.POS.z = float(IN[0].POS.z + offset);
	v.POS.y = 0.0f;
	v.WPOS	= v.POS.xyz;
	v.POS	= mul(Proj, mul(View, v.POS));
	OUT.Append(v);

	v.POS	= float4(0, 0, 0, 1);
	v.POS.x = float(IN[0].POS.x - offset);
	v.POS.z = float(IN[0].POS.z - offset);
	v.POS.y = 0.0f;
	v.WPOS	= v.POS.xyz;
	v.POS	= mul(Proj, mul(View, v.POS));

	OUT.Append(v);
	OUT.RestartStrip();

	v.POS	= float4(0, 0, 0, 1);
	v.POS.x = float(IN[0].POS.x + offset);
	v.POS.z = float(IN[0].POS.z + offset);
	v.POS.y = 0.0f;
	v.WPOS	= v.POS.xyz;
	v.POS	= mul(Proj, mul(View, v.POS));

	OUT.Append(v);
	v.POS	= float4(0, 0, 0, 1);
	v.POS.x = float(IN[0].POS.x - offset);
	v.POS.z = float(IN[0].POS.z - offset);
	v.POS.y = 0.0f;
	v.WPOS	= v.POS.xyz;
	v.POS	= mul(Proj, mul(View, v.POS));
	OUT.Append(v);
	
	v.POS	= float4(0, 0, 0, 1);
	v.POS.x = float(IN[0].POS.x + offset);
	v.POS.z = float(IN[0].POS.z - offset);
	v.POS.y = 0.0f;
	v.WPOS	= v.POS.xyz;
	v.POS	= mul(Proj, mul(View, v.POS));
	OUT.Append(v);

	OUT.RestartStrip();
}


// ---------------------------------------------------------------------------