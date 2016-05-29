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

#define MinScreenSpan 0.05f
#define MaxScreenSpan 0.2f

#define MinSpan 16
#define MaxSpan	512	  
	  
// Constant Buffers
// ---------------------------------------------------------------------------


struct Plane
{
	float4 Normal;
	float4 Orgin;
};


// ---------------------------------------------------------------------------


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


// ---------------------------------------------------------------------------


cbuffer TerrainConstants : register( b1 )
{
	float4	 Albedo;   // + roughness
	float4	 Specular; // + metal factor
	Plane Frustum[6];
};


// ---------------------------------------------------------------------------
// Structs

struct VertexIn
{
	int4 POS 	: REGION;
	int4 Parent	: TEXCOORD0;

};


struct Region_CP
{
	int4 POS    : REGION;
	int4 Parent	: TEXCOORD0;
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
	int4 POS 	: REGION;
	int4 Parent	: TEXCOORD;
};


// ---------------------------------------------------------------------------


bool CompareAgainstFrustum(float3 V, float r)
{
	bool Near	= false;
	bool Far	= false;
	bool Left	= false;
	bool Right	= false;

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

	return !(Near & Far & Left & Right);
}


// ---------------------------------------------------------------------------


float3 GetTopLeftPoint(int4 IN)
{
	float3 Out = float3(0, 0, 0);
	float offset = float(IN.w);

	Out.x = IN.x - offset;
	Out.y = 10 + sin(length(float2(IN.x + offset, IN.y + offset))/3.14);
	Out.z = IN.z + offset;

	return Out;
}

float3 GetTopRightPoint(int4 IN)
{
	float3 Out = float3(0, 0, 0);
	float offset = float(IN.w);

	Out.x = IN.x + offset;
	Out.y = 10 + sin(length(float2(IN.x + offset, IN.y + offset))/3.14);
	Out.z = IN.z + offset;

	return Out;
}

float3 GetBottomLeftPoint(int4 IN)
{
	float3 Out = float3(0, 0, 0);
	float offset = float(IN.w);

	Out.x = IN.x - offset;
	Out.y = 10 + sin(length(float2(IN.x + offset, IN.y + offset))/3.14);
	Out.z = IN.z - offset;

	return Out;
}

float3 GetBottomRightPoint(int4 IN)
{
	float3 Out = float3(0, 0, 0);
	float offset = float(IN.w);

	Out.x = IN.x + offset;
	Out.y = 10 + sin(length(float2(IN.x + offset, IN.y + offset))/3.14);
	Out.z = IN.z - offset;

	return Out;
}


// ---------------------------------------------------------------------------


Region_CP VPassThrough( VertexIn IN )
{
	Region_CP OUT;
	OUT.POS 	= IN.POS;
	OUT.Parent	= IN.Parent;
	return OUT;
}


// ---------------------------------------------------------------------------


float GetRegionSpan(int4 R)
{
	float4 TL = mul(Proj, mul(View, float4(GetTopLeftPoint(R), 1)));
	float4 BR = mul(Proj, mul(View, float4(GetBottomRightPoint(R), 1)));

	float4 TR = mul(Proj, mul(View, float4(GetTopRightPoint(R), 1)));
	float4 BL = mul(Proj, mul(View, float4(GetBottomLeftPoint(R), 1)));

	float3 TLNormalized = TL.xyz / TL.w;
	float3 BRNormalized = BR.xyz / BR.w;
	float3 TRNormalized = TR.xyz / TR.w;
	float3 BLNormalized = BL.xyz / BL.w;

	float Span1 = length(TLNormalized - BRNormalized);
	float Span2 = length(TRNormalized - BLNormalized);

	return max(Span1, Span2);
}


bool SplitRegion(int4 R){
	return (MinSpan <= R.w && GetRegionSpan(R) > .1) || (R.w >= MaxSpan);
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
	int offset = uint(IN.POS.w)/2;

	R.Parent = IN.POS;

	R.POS 	= IN.POS;
	R.POS.x = R.POS.x - offset;
	R.POS.y = TopLeft;
	R.POS.z = R.POS.z + offset;
	R.POS.w = offset;

	return R;
}


Region_CP MakeTopRight(Region_CP IN)
{
	Region_CP R;
	int offset = uint(IN.POS.w)/2;

	R.Parent = IN.POS;

	R.POS 	= IN.POS;
	R.POS.x = R.POS.x + offset;
	R.POS.y = TopRight;
	R.POS.z = R.POS.z + offset;
	R.POS.w = offset;
	return R;
}


Region_CP MakeBottomRight(Region_CP IN)
{
	Region_CP R;
	int offset = uint(IN.POS.w)/2;

	R.Parent = IN.POS;

	R.POS 	= IN.POS;
	R.POS.x = R.POS.x + offset;
	R.POS.y = BottomRight;
	R.POS.z = R.POS.z - offset;
	R.POS.w = offset;
	return R;
}


Region_CP MakeBottomLeft(Region_CP IN)
{
	Region_CP R;
	int offset = uint(IN.POS.w)/2;

	R.Parent = IN.POS;

	R.POS 	= IN.POS;
	R.POS.x = R.POS.x - offset;
	R.POS.y = BottomLeft;
	R.POS.z = R.POS.z - offset;
	R.POS.w = offset;

	return R;
}


// ---------------------------------------------------------------------------


bool CullRegion(Region_CP R){ 
	return CompareAgainstFrustum(R.POS.xyz, R.POS.w * 2);
}


// ---------------------------------------------------------------------------



[maxvertexcount(4)]
void GS_Split(
	point Region_CP IN[1], 
	inout PointStream<SO_OUT> RegionBuffer,
	inout PointStream<SO_OUT> FinalBuffer)
{	// TODO: Visibility Determination
	if(!CullRegion(IN[0]))
	{
		if (SplitRegion(IN[0].POS))
		{
			RegionBuffer.Append(MakeTopLeft(IN[0]));
			RegionBuffer.Append(MakeTopRight(IN[0]));
			RegionBuffer.Append(MakeBottomRight(IN[0]));
			RegionBuffer.Append(MakeBottomLeft(IN[0]));
			RegionBuffer.RestartStrip();
		} else {
			FinalBuffer.Append(IN[0]);
			FinalBuffer.RestartStrip();
		}
	}
}


// ---------------------------------------------------------------------------


struct HS_Corner
{
	float3 POS	  : POSITION;
	float3 Normal : NORMAL;
	float2 UV	  : TEXCOORD;
	float  R	  : TEXCOORD1;
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


float GetEdgeSpan(float3 P1, float3 P2)
{
	float4 ScreenSpaceCoord_1 = mul(Proj, mul(View, float4(P1, 1)));
	float4 ScreenSpaceCoord_2 = mul(Proj, mul(View, float4(P2, 1)));

	float3 SPC1Normalized = ScreenSpaceCoord_1.xyz / ScreenSpaceCoord_1.w;
	float3 SPC2Normalized = ScreenSpaceCoord_2.xyz / ScreenSpaceCoord_2.w;

	//SPC2Normalized *= float3(1, 1, 0);
	//SPC2Normalized *= float3(1, 1, 0);

	float Span = length(SPC2Normalized - SPC1Normalized);

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
	Neighbor.y += Region.w;

	return Neighbor;
}

int GetRegionDepth(int4 R){
	return 1 + log2(MaxSpan) - log2(R.w);
}

int GetRegionDepth(int R) {
	return log2(MaxSpan) - log2(R);
}

float ExpansionRate(float x)
{
	return pow(x, 1);
}

RegionTessFactors ConstantFactors(InputPatch<Region_CP, 1> ip)
{
	RegionTessFactors factors;

	int4	Region  = ip[0].POS;
	float	RegionWidth = Region.w; 

	const int MinDepth = log2(MaxSpan);
	const int MaxDepth = log2(MinSpan);

	int		Depth = GetRegionDepth(RegionWidth);
	float	MaxFactor = pow(2, Depth - MaxDepth);
	float	MinFactor = pow(2, Depth - MaxDepth);

	// Get Regions Neighboring and Calculate Edge Factors
	float3 TopLeftPoint		= GetTopLeftPoint(Region);
	float3 TopRightPoint	= GetTopRightPoint(Region);
	float3 BottomLeftPoint	= GetBottomLeftPoint(Region);
	float3 BottomRightPoint = GetBottomRightPoint(Region);

	float Factor1 = 1;
	float Factor2 = 1;
	float Factor3 = 1;
	float Factor4 = 1;

	float EdgeSpan1 = GetEdgeSpan(TopLeftPoint,		BottomLeftPoint);	// Left Edge
	float EdgeSpan2 = GetEdgeSpan(TopLeftPoint,		TopRightPoint);		// Bottom Edge
	float EdgeSpan3 = GetEdgeSpan(TopRightPoint,	BottomRightPoint);	// Right Edge
	float EdgeSpan4 = GetEdgeSpan(BottomLeftPoint,	BottomRightPoint);	// Top Edge

	/*
	if(Depth == MaxDepth)
	{
		EdgeSpan1 = max(0.1, EdgeSpan1);
		EdgeSpan3 = max(0.1, EdgeSpan3);
		EdgeSpan2 = max(0.1, EdgeSpan2);
		EdgeSpan4 = max(0.1, EdgeSpan4);

		Factor1 *= 0.2 + (EdgeSpan1);
		Factor2 *= 0.2 + (EdgeSpan2);
		Factor3 *= 0.2 + (EdgeSpan3);
		Factor4 *= 0.2 + (EdgeSpan4);

		Factor1 = ExpansionRate(Factor1);
		Factor2 = ExpansionRate(Factor2);
		Factor3 = ExpansionRate(Factor3);
		Factor4 = ExpansionRate(Factor4);

		EdgeSpan1 = min(0.1, 16);
		EdgeSpan2 = min(0.1, 16);
		EdgeSpan3 = min(0.1, 16);
		EdgeSpan4 = min(0.1, 16);

	}
	*/

	factors.EdgeTess[0] = Factor1; //pow(2, 12 - SubDivLevel); // Left
	factors.EdgeTess[1] = Factor2; //pow(2, 12 - SubDivLevel); // Bottom
	factors.EdgeTess[2] = Factor3; //pow(2, 12 - SubDivLevel); // Right
	factors.EdgeTess[3] = Factor4; //pow(2, 12 - SubDivLevel); // Top


	float Inner = max(	max(Factor1, Factor3), 
						max(Factor2, Factor4));

	factors.InsideTess[0] = min(Factor2, Factor4); // North South
	factors.InsideTess[1] = min(Factor1, Factor3); // East West

	return factors;
}


// ---------------------------------------------------------------------------


[domain("quad")]
[partitioning("fractional_even")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ConstantFactors")]
[maxtessfactor(64.0f)]
HS_Corner RegionToQuadPatch(
	InputPatch<Region_CP, 1> ip,
	uint i			: SV_OutputControlPointID,
	uint PatchID	: SV_PrimitiveID)
{
	HS_Corner Output;
	const float3 Scale = float3(1, 1, 1);

	if(i == TopLeft)
		Output.POS = Scale * GetTopLeftPoint(ip[0].POS);
	else if (i == TopRight)
		Output.POS = Scale * GetTopRightPoint(ip[0].POS);
	else if (i == BottomRight)
		Output.POS = Scale * GetBottomRightPoint(ip[0].POS);
	else if (i == BottomLeft)
		Output.POS = Scale * GetBottomLeftPoint(ip[0].POS);

	Output.R = ip[0].POS.w;

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

		//lerp(y1, y2, uv[0])(1 -) + lerp(y3, y4, uv[1]);
	}

	float3 POS  = float3(x, y, z);
	float4 WPOS = float4(POS, 1.0f);

	Point.WPOS	= WPOS.xyz;
	Point.N		= float4(0,1,0, 0.0f);
	Point.POS	= mul(Proj, mul(View, WPOS));

	float T = GetRegionDepth(bezPatch[0].R) / (1 + log2(MaxSpan));
	Point.Colour = float3(T, T, T);

	return Point;
}


// ---------------------------------------------------------------------------


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