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



#define	EPlane_TOP		0
#define	EPlane_BOTTOM	1 
#define	EPlane_LEFT		2
#define	EPlane_RIGHT	3
#define	EPlane_FAR		4
#define	EPlane_NEAR		5

#define GroupWidth 8
#define GroupHeight 8
#define GroupSize 64

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
    float3 Normal;
    float  d;
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
    float4x4    iproj;
    float4x4    view;
    uint2       LightMapWidthHeight;
    uint        lightCount;
};


bool CompareSphereAgainstPlane(float3 origin, float r, Plane P)
{
    return dot(P.Normal, origin) - P.d <= -r;
}


bool CompareAgainstFrustum(float3 O, float r, Fustrum fustrum)
{
    bool result = true;
    [unroll]
    for (uint i = 0; i < 4; ++i)
        result = result & !CompareSphereAgainstPlane(O, r, fustrum.planes[i]);
            
    return result;
}

Plane GetPlane(float3 p0, float3 p1, float3 p2)
{
    Plane plane;

    float3 V1 = p1 - p0;
    float3 V2 = p2 - p0;

    plane.Normal    = normalize(cross(V1, V2));
    plane.d         = dot(plane.Normal, p0);

    return plane;
}

float2 GetScreenCord(uint2 pixel)
{
    const float2 TileSpan = 2.0f * float2(1, 1) / float2(LightMapWidthHeight);
    return float2(-1, 1) + TileSpan * float2(pixel) * float2(1.0f, -1.0f);
}

Fustrum CreateSubFustrum(uint2 bucketID)
{
    float4 topLeftPoint     = mul(iproj, float4(GetScreenCord(bucketID + uint2(0,  0)), -0.1f, 1.0f));
    float4 topRightPoint    = mul(iproj, float4(GetScreenCord(bucketID + uint2(1,  0)), -0.1f, 1.0f));
    float4 bottomLeftPoint  = mul(iproj, float4(GetScreenCord(bucketID + uint2(0,  1)), -0.1f, 1.0f));
    float4 bottomRightPoint = mul(iproj, float4(GetScreenCord(bucketID + uint2(1,  1)), -0.1f, 1.0f));
    float3 Origin           = float3(0, 0, 0); // we are in screen space, duh, center is at 0, 0, 0

    topLeftPoint     /= topLeftPoint.w;
    topRightPoint    /= topRightPoint.w;
    bottomLeftPoint  /= bottomLeftPoint.w;
    bottomRightPoint /= bottomRightPoint.w;

    //
    Plane topPlane      = GetPlane(Origin,          topLeftPoint,       topRightPoint);
    Plane bottomPlane   = GetPlane(Origin,          bottomRightPoint,   bottomLeftPoint); // Off
    Plane leftPlane     = GetPlane(Origin,          topLeftPoint,       bottomLeftPoint); // Off
    Plane rightPlane    = GetPlane(Origin,          bottomRightPoint,   topRightPoint);  
    Plane farPlane      = GetPlane(topRightPoint,   topLeftPoint,       bottomLeftPoint);
    Plane nearPlane;

    nearPlane.d         = dot(nearPlane.Normal.xyz, Origin.xyz);

    Fustrum f_out;
    f_out.planes[EPlane_TOP]    = topPlane;
    f_out.planes[EPlane_BOTTOM] = bottomPlane;// bugged
    f_out.planes[EPlane_LEFT]   = leftPlane;
    f_out.planes[EPlane_RIGHT]  = rightPlane;
    f_out.planes[EPlane_NEAR]   = nearPlane;
    f_out.planes[EPlane_FAR]    = farPlane;

    return f_out;
}


PointLight ReadPointLight(uint idx)
{
    PointLight pointLight;
    pointLight.KI           = asfloat(pointLights.Load4(idx * 32));
    pointLight.PositionR    = asfloat(pointLights.Load4(idx * 32 + 16));

    return pointLight;
}

groupshared Fustrum f;
groupshared uint    culledLightCount  = 0;
groupshared uint    lightOffset       = 0;
groupshared uint    lights[768];

[numthreads(GroupWidth, GroupHeight, 1)]
void tiledLightCulling(uint3 ID : SV_GroupID, uint3 TID : SV_GroupThreadID)
{
    if (TID.x == 0 && TID.y == 0)
        f = CreateSubFustrum(ID.xy);

    const uint2 bucketID     = ID; 
    const uint  localIdx     = TID.x + (TID.y * GroupWidth);
    const uint  waveCount    = lightCount / GroupSize + 1;
    const uint  offset       = (ID.x + ID.y * 192) * 768;

    GroupMemoryBarrierWithGroupSync();

    for (int i = 0; i < waveCount; ++i )
    {
        uint idx = localIdx + GroupSize * i;
        if (idx < lightCount)
        {
            // TODO: culling
            PointLight light    = ReadPointLight(idx);
            float3 viewPosition = mul(view, float4(light.PositionR.xyz, 1));

            if (CompareAgainstFrustum(viewPosition, light.PositionR.w, f))
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
