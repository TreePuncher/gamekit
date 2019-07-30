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


#define	EPlane_FAR		0
#define	EPlane_NEAR		1
#define	EPlane_TOP		2
#define	EPlane_BOTTOM	3 
#define	EPlane_LEFT		4
#define	EPlane_RIGHT	5

struct lightSample
{
    uint pixelID;
    uint lightID;
};


struct PointLight
{
    float4 KI;
    float4 PositionR;
};

struct Plane
{
    float4 Normal;
    float4 Origin;
};

struct Fustrum
{
    Plane planes[6];
};


RWTexture2D<uint2>  lightMap    : register(u0); // out
RWBuffer<uint>      lightBuffer : register(u1); // out
ByteAddressBuffer   pointLights : register(t0); // in

cbuffer             constants   : register(b0)
{
    Plane       fustrum[6];
    float4x4    iproj;
    float4x4    view;
    uint2       LightMapWidthHeight;
    uint        lightCount;
};

bool CompareAgainstFrustum(float3 V, float r, Fustrum fustrum)
{
	bool Near = false;
	bool Far = false;
	bool Left = false;
	bool Right = false;
	bool Top = false;
	bool Bottom = false;

    {
        float3 P = V -  fustrum.planes[EPlane_FAR].Origin.xyz;
        float3 N =      fustrum.planes[EPlane_FAR].Normal.xyz;

        float NdP = dot(N, P);
        float D = NdP - r;
        Far = D <= 0;
    }
	{
        float3 P = V -  fustrum.planes[EPlane_NEAR].Origin.xyz;
        float3 N =      fustrum.planes[EPlane_NEAR].Normal.xyz;

		float NdP = dot(N, P);
		float D = NdP - r;
		Near = D <= 0;
	}
	{
        float3 P = V -  fustrum.planes[EPlane_TOP].Origin.xyz;
        float3 N =      fustrum.planes[EPlane_TOP].Normal.xyz;

		float NdP = dot(N, P);
		float D = NdP - r;
		Top = D <= 0;
	}
	{
        float3 P = V -  fustrum.planes[EPlane_BOTTOM].Origin.xyz;
        float3 N =      fustrum.planes[EPlane_BOTTOM].Normal.xyz;

		float NdP = dot(N, P);
		float D = NdP - r;
		Bottom = D <= 0;
	}
	{
        float3 P = V -  fustrum.planes[EPlane_LEFT].Origin.xyz;
        float3 N =      fustrum.planes[EPlane_LEFT].Normal.xyz;

		float NdP = dot(N, P);
		float D = NdP + r;
		Left = D >= 0;
	}
	{
        float3 P = V -  fustrum.planes[EPlane_RIGHT].Origin.xyz;
        float3 N =      fustrum.planes[EPlane_RIGHT].Normal.xyz;

		float NdP = dot(N, P);
		float D = NdP + r;
		Right = D >= 0;
	}

	return (Right && Left && Top && Bottom);
}

Plane GetPlane(float3 point1, float3 point2, float3 point3)
{
    Plane plane;

    float3 normal1 = normalize(point2 - point1);
    float3 normal2 = normalize(point1 - point3);

    plane.Normal = float4(normalize(cross(normal1, normal2)), 0);
    plane.Origin = float4(point3, 1);

    return plane;
}

Fustrum CreateSubFustrum(uint2 bucketID)
{
    const float2 TileSpan   = 2.0f * float2(1, 1) / LightMapWidthHeight;
    const float2 begin      = float2(-1, 1) + TileSpan * bucketID * float2(1, -1);

    float4 topLeftPoint     = mul(iproj, float4(begin.x,                begin.y, 1.0f, 1.0f));
    float4 topRightPoint    = mul(iproj, float4(begin.x + TileSpan.x,   begin.y, 1.0f, 1.0f));
    float4 bottomLeftPoint  = mul(iproj, float4(begin.x,                begin.y - TileSpan.y, 1.0f, 1.0f));
    float4 bottomRightPoint = mul(iproj, float4(begin.x + TileSpan.x,   begin.y - TileSpan.y, 1.0f, 1.0f));
    float4 Origin           = mul(iproj, float4(begin.x,                begin.y, 0.001f, 1.0f));
    
    topLeftPoint     /= topLeftPoint.w;
    topRightPoint    /= topRightPoint.w;
    bottomLeftPoint  /= bottomLeftPoint.w;
    bottomRightPoint /= bottomRightPoint.w;
    Origin           /= Origin.w;

    //
    Plane topPlane      = GetPlane(topLeftPoint,    topRightPoint,      Origin);
    Plane bottomPlane   = GetPlane(bottomLeftPoint, bottomRightPoint,   Origin);    
    Plane farPlane      = GetPlane(topRightPoint,   topLeftPoint,       bottomLeftPoint);
    Plane leftPlane     = GetPlane(Origin,          topLeftPoint,       bottomLeftPoint);
    Plane rightPlane    = GetPlane(Origin,          bottomRightPoint,   topRightPoint);
    Plane nearPlane;

    nearPlane.Normal    = -farPlane.Normal;
    nearPlane.Origin    = Origin;


    Fustrum f_out;
    f_out.planes[EPlane_TOP]    = topPlane;
    f_out.planes[EPlane_BOTTOM] = bottomPlane;
    f_out.planes[EPlane_FAR]    = farPlane;
    f_out.planes[EPlane_NEAR]   = nearPlane;
    f_out.planes[EPlane_LEFT]   = leftPlane;
    f_out.planes[EPlane_RIGHT]  = rightPlane;

    return f_out;
}


PointLight ReadPointLight(uint idx)
{
    PointLight pointLight;
    pointLight.KI           = asfloat(pointLights.Load4(idx * 32));
    pointLight.PositionR    = asfloat(pointLights.Load4(idx * 32 + 16));

    return pointLight;
}

#define GroupWidth 8
#define GroupHeight 8
#define GroupSize 64

groupshared Fustrum f;
groupshared uint    culledLightCount  = 0;
groupshared uint    lightOffset       = 0;
groupshared uint    lights[512];

[numthreads(GroupWidth, GroupHeight, 1)]
void tiledLightCulling(uint3 ID : SV_GroupID, uint3 TID : SV_GroupThreadID)
{
    if (TID.x == 0 && TID.y == 0)
        f = CreateSubFustrum(ID.xy);

    const uint2 bucketID    = ID; 
    const uint localIdx     = TID.x + (TID.y * GroupWidth);
    const uint waveCount    = lightCount / GroupSize + 1;
    const uint offset       = (ID.x + ID.y * GroupSize) * 512;

    GroupMemoryBarrierWithGroupSync();

    for (int i = 0; i < waveCount; ++i )
    {
        uint idx = localIdx + GroupSize * i;
        if (idx < lightCount)
        {
            // TODO: culling
            PointLight light    = ReadPointLight(idx);
            float3 viewPosition = mul(view, float4(light.PositionR.xyz, 1));


            if (CompareAgainstFrustum(viewPosition, 100, f))
            {
                uint OutIdx;
                InterlockedAdd(culledLightCount, 1, OutIdx);
                lights[OutIdx] = idx;
            }
        }
    }
    
    GroupMemoryBarrierWithGroupSync();


    if (TID.x == 0 && TID.y == 0)
        lightMap[ID.xy] = uint2(culledLightCount, offset);

    // TODO: buffer compaction
    for (int i = 0; i < waveCount; ++i)
    {
        uint idx = localIdx + GroupSize * i;
        if (idx < lightCount)
            lightBuffer[offset + idx] = lights[idx];
    }
}
