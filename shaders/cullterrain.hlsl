#include "terrainRenderingCommon.hlsl"


/************************************************************************************************/


bool CompareAgainstFrustum(float3 V, float r)
{
	bool Near = false;
	bool Far = false;
	bool Left = false;
	bool Right = false;
	bool Top = false;
	bool Bottom = false;


	{
		float3 P = V - Frustum[EPlane_NEAR].Orgin.xyz;
		float3 N = Frustum[EPlane_NEAR].Normal.xyz;

		float NdP = dot(N, P);
		float D = NdP - r;
		Near = D <= 0;
	}
	{
		float3 P = V - Frustum[EPlane_FAR].Orgin.xyz;
		float3 N = Frustum[EPlane_FAR].Normal.xyz;

		float NdP = dot(N, P);
		float D = NdP - r;
		Far = D <= 0;
	}
	{
		float3 P = V - Frustum[EPlane_LEFT].Orgin.xyz;
		float3 N = Frustum[EPlane_LEFT].Normal.xyz;

		float NdP = dot(N, P);
		float D = NdP - r;
		Left = D <= 0;
	}
	{
		float3 P = V - Frustum[EPlane_RIGHT].Orgin.xyz;
		float3 N = Frustum[EPlane_RIGHT].Normal.xyz;

		float NdP = dot(N, P);
		float D = NdP - r;
		Right = D <= 0;
	}
	{
		float3 P = V - Frustum[EPlane_TOP].Orgin.xyz;
		float3 N = Frustum[EPlane_TOP].Normal.xyz;

		float NdP = dot(N, P);
		float D = NdP - r;
		Top = D <= 0;
	}
	{
		float3 P = V - Frustum[EPlane_BOTTOM].Orgin.xyz;
		float3 N = Frustum[EPlane_BOTTOM].Normal.xyz;

		float NdP = dot(N, P);
		float D = NdP - r;
		Bottom = D <= 0;
	}

	return !(Near & Far & Left & Right & Bottom);
}


/************************************************************************************************/


Region_CP CP_PassThroughVS(Region_CP IN)
{
	return IN;
}


/************************************************************************************************/


Region_CP MakeNorthEast(Region_CP CP)
{
	Region_CP R;
	int offset = uint(CP.POS.w) / 2;

	R.Parent		= CP.POS;
	R.TerrainInfo	= CP.TerrainInfo;
	R.UVDomain		= float4(
						GetRegionUV(CP, float2(0.5f, 0.5f)),  // South West UV
						GetRegionUV(CP, float2(1.0f, 1.0f))
						); // North East UV 

	R.POS	= CP.POS;
	R.POS.x = R.POS.x + offset;
	R.POS.z = R.POS.z - offset;
	R.POS.w = offset;
	R.POS.y = NorthEast;

	return R;
}


/************************************************************************************************/


Region_CP MakeNorthWest(Region_CP CP)
{
	Region_CP R;
	int offset = uint(CP.POS.w) / 2; // Position Height

	R.Parent		= CP.POS;
	R.TerrainInfo	= CP.TerrainInfo;
	R.UVDomain		= float4(
						GetRegionUV(CP, float2(0.0f, 0.5f)),  // South West UV
						GetRegionUV(CP, float2(0.5f, 1.0f))
					); // North East UV 

	R.POS	= CP.POS;
	R.POS.x = R.POS.x - offset;
	R.POS.z = R.POS.z - offset;
	R.POS.w = offset;
	R.POS.y = NorthWest;

	return R;
}


/************************************************************************************************/


Region_CP MakeSouthWest(Region_CP CP)
{
	Region_CP R;
	int offset = uint(CP.POS.w) / 2;

	R.Parent		= CP.POS;
	R.TerrainInfo	= CP.TerrainInfo;
	R.UVDomain		= float4(
						GetRegionUV(CP, float2(0.0f, 0.0f)), // South West UV
						GetRegionUV(CP, float2(0.5f, 0.5))
					); // North East UV 

	R.POS	= CP.POS;
	R.POS.x = R.POS.x - offset;
	R.POS.z = R.POS.z + offset;
	R.POS.w = offset;
	R.POS.y = SouthWest;

	return R;
}


/************************************************************************************************/



Region_CP MakeSouthEast(Region_CP CP)
{
	Region_CP R;
	int offset = uint(CP.POS.w) / 2;

	R.Parent		= CP.POS;
	R.TerrainInfo	= CP.TerrainInfo;
	R.UVDomain		= float4(
						GetRegionUV(CP, float2(0.5f, 0.0f)),  // South West UV
						GetRegionUV(CP, float2(1.0f, 0.5f))
					); // North East UV 

	R.POS	= CP.POS;
	R.POS.x = R.POS.x + offset;
	R.POS.z = R.POS.z + offset;
	R.POS.w = offset;
	R.POS.y = SouthEast;

	return R;
}


/************************************************************************************************/


struct AABB
{
	float2  Position; // Center Position
	float3  MinMax[2];
	float   TopHeight;
	float   R;
};


bool IntersectsFrustum(AABB aabb)
{
	int Result = 1;

	for (int I = 0; I < 6; ++I)
	{
		float px = ((Frustum[I].Normal.x > 0.0f) ? -aabb.R : aabb.R);
		float py = (Frustum[I].Normal.y > 0.0f) ? 0 : TerrainHeight;
		float pz = ((Frustum[I].Normal.z > 0.0f) ? -aabb.R : aabb.R);

		float3 Temp = float3(
			aabb.Position.x + px,
			py,
			aabb.Position.y + pz);

		float3 pV = Temp - Frustum[I].Orgin.xyz;
		float dP = dot(Frustum[I].Normal.xyz, pV);

		if (dP >= 0)
			return false;
	}

	return true;
}


bool CullRegion(Region_CP R) {
	float3 NWCorner = GetNWPoint(R).pos;
	float3 NECorner = GetNEPoint(R).pos;
	float3 SWCorner = GetSWPoint(R).pos;
	float3 SECorner = GetSEPoint(R).pos;

	AABB aabb;
	aabb.Position	= R.POS.xz;
	aabb.TopHeight	= TerrainHeight;
	aabb.R			= R.POS.w;

	aabb.TopHeight = max(max(NWCorner.y, NECorner.y), max(SWCorner.y, SECorner.y));
	aabb.MinMax[0] = SWCorner;
	aabb.MinMax[1] = SECorner;

	return !IntersectsFrustum(aabb);
}


[maxvertexcount(4)]
void CullTerrain(
	point Region_CP IN[1],
	inout PointStream<Region_CP> IntermediateBuffer,
	inout PointStream<Region_CP> FinalBuffer)
{
	if (!CullRegion(IN[0]))
	{
		if (SplitRegion(IN[0]))
		{
			IntermediateBuffer.Append(MakeNorthEast(IN[0]));
			IntermediateBuffer.Append(MakeNorthWest(IN[0]));
			IntermediateBuffer.Append(MakeSouthWest(IN[0]));
			IntermediateBuffer.Append(MakeSouthEast(IN[0]));
			IntermediateBuffer.RestartStrip();
		}
		else 
		{
			FinalBuffer.Append(IN[0]);
			FinalBuffer.RestartStrip();
		}
	}
}


/**********************************************************************

Copyright (c) 2015 - 2018 Robert May

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