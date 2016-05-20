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
#define MinSpan = 10;
#define LeftSide	0
#define TopSide		1
#define RightSide	2
#define BottomSide	3

// ---------------------------------------------------------------------------

#define	EPlane_FAR 0
#define	EPlane_NEAR 1
#define	EPlane_TOP 2
#define	EPlane_BOTTOM 3 
#define	EPlane_LEFT 4
#define	EPlane_RIGHT 5

// Constant Buffers
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------

cbuffer TerrainConstants : register( b1 )
{
	float4	 Albedo;   // + roughness
	float4	 Specular; // + metal factor
	Plane Frustum[6];
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
// Structs
struct VertexIn
{
	int4 POS 		  : POSITION0;
	int4 ParentRegion : TEXCOORD0;

};

struct Region_CP
{
	int4 POS     		: POSITION0;
	int4 ParentRegion	: TEXCOORD0;
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


struct PS_OUT
{
	float4 COLOUR : SV_TARGET;	
};
// ---------------------------------------------------------------------------


Region_CP VPassThrough( VertexIn IN )
{
	Region_CP OUT;
	OUT.POS 			= IN.POS;
	OUT.ParentRegion	= IN.ParentRegion;
	return OUT;
}


// ---------------------------------------------------------------------------


PS_IN CPVisualiser( VertexIn IN )
{
	PS_IN OUT;
	OUT.POS 	= float4(float(IN.POS.x)/20, float(IN.POS.z)/20, 0, 0 );
	return OUT;
}


// ---------------------------------------------------------------------------


Region_CP MakeTopLeft(Region_CP IN)
{
	Region_CP R;
	int offset = uint(IN.POS.w)/2;

	R.ParentRegion 		= IN.POS;
	R.ParentRegion.w 	= TopSide;

	R.POS 	= IN.POS;
	R.POS.x = R.POS.x - offset;
	R.POS.y = 0;
	R.POS.z = R.POS.z + offset;
	R.POS.w = offset;
	return R;
}


// ---------------------------------------------------------------------------


Region_CP MakeTopRight(Region_CP IN)
{
	Region_CP R;
	int offset = uint(IN.POS.w)/2;

	R.ParentRegion 		= IN.POS;
	R.ParentRegion.w 	= TopSide;

	R.POS 	= IN.POS;
	R.POS.x = R.POS.x + offset;
	R.POS.y = 0;
	R.POS.z = R.POS.z + offset;
	R.POS.w = offset;
	return R;
}


// ---------------------------------------------------------------------------


Region_CP MakeBottomRight(Region_CP IN)
{
	Region_CP R;
	int offset = uint(IN.POS.w)/2;

	R.ParentRegion 		= IN.POS;
	R.ParentRegion.w 	= TopSide;

	R.POS 	= IN.POS;
	R.POS.x = R.POS.x + offset;
	R.POS.y = 0;
	R.POS.z = R.POS.z - offset;
	R.POS.w = offset;
	return R;
}


// ---------------------------------------------------------------------------


Region_CP MakeBottomLeft(Region_CP IN)
{
	Region_CP R;
	int offset = uint(IN.POS.w)/2;

	R.ParentRegion 		= IN.POS;
	R.ParentRegion.w 	= TopSide;

	R.POS 	= IN.POS;
	R.POS.x = R.POS.x - offset;
	R.POS.y = 0;
	R.POS.z = R.POS.z - offset;
	R.POS.w = offset;
	return R;
}


// ---------------------------------------------------------------------------


bool SplitRegion(Region_CP R)
{
	float3 POS	= CameraPOS.xyz  * float3(1, 0, 1);
	float3 A	= float3(R.POS.xyz) - POS;
	float L		= length(A);

	return (L <= R.POS.w * 30); // TEMP;
}


bool CullRegion(Region_CP R){ 
	return !CompareAgainstFrustum(R.POS.xyz, R.POS.w * 2);
}


struct SO_OUT
{
	int4 POS 	: POSITION0;
	int4 offset	: TEXCOORD0;
};


// ---------------------------------------------------------------------------



[maxvertexcount(4)]
void GS_Split(
	point Region_CP IN[1], 
	inout PointStream<SO_OUT> RegionBuffer,
	inout PointStream<SO_OUT> FinalBuffer)
{	// TODO: Visibility Determination
	if(CullRegion(IN[0]))
	if (SplitRegion(IN[0]))
	{
		RegionBuffer.Append(MakeTopLeft(IN[0]));
		RegionBuffer.Append(MakeTopRight(IN[0]));
		RegionBuffer.Append(MakeBottomRight(IN[0]));
		RegionBuffer.Append(MakeBottomLeft(IN[0]));
	} else {
		FinalBuffer.Append(IN[0]);
	}
	RegionBuffer.RestartStrip();
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

PS_OUT PSMain( PS_IN IN )
{
	PS_OUT OUT;
	OUT.COLOUR = float4(1, 1, 1, .5);
	return OUT;
}

// ---------------------------------------------------------------------------