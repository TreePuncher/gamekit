/**********************************************************************

Copyright (c) 2015 - 2016 Robert May

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


/************************************************************************************************/


#include "common.hlsl"

static const float PI = 3.14159265359f;
static const float PIInverse = 1 / PI;

// Outputs
RWTexture2D<float4> BB : register(u0); // out


/************************************************************************************************/
// OUTPUT STRUCTS

void WriteOut(float4 C, uint2 PixelCord, uint2 offset)
{
    BB[PixelCord + offset] = C;
}

float4 ReadIn(uint2 PixelCord)
{
    return BB[PixelCord];
}


/************************************************************************************************/
// INPUT STRUCTS

cbuffer GBufferConstantsLayout : register(b1)
{
    uint DLightCount;
    uint DeferredPointLightCount;
    uint DeferredSpotLightCount;
    uint Height;
    uint Width;
    uint DebugRenderMode;
    uint Padding[2];
    float4 AmbientLight;
}

struct PointLight
{
    float4 P; // Position + R
    float4 K; // Color + I
};

struct SpotLight
{
    float4 P; // Position + R
    float4 K; // Color + I
    float4 D; // Direction;
};

struct DirectionalLight
{
    float4 D; // Direction
    float4 K; // Color + I
};

struct LightTile
{
    uint Offset;
    uint Count;
};

// Inputs
sampler Sampler : register(s0);

/************************************************************************************************/


StructuredBuffer<PointLight> PointLights : register(t0);
StructuredBuffer<SpotLight> SpotLights   : register(t1);
StructuredBuffer<LightTile> LightTiles   : register(t7);

Texture2D<float4> Color      : register(t2);
Texture2D<float4> Specular   : register(t3);
Texture2D<float4> Radience   : register(t4);
Texture2D<float2> RoughMetal : register(t5);
Texture2D<float4> Normal     : register(t6);
Texture2D<float> DepthView   : register(t8);


/************************************************************************************************/


struct SurfaceProperties
{
    float A; // Roughness
    float Ks; //
    float kd; //
    float Kr; //
};



/************************************************************************************************/

void Write2x2Dot(uint2 ID, float4 Color)
{
    BB[ID.xy + uint2(0, 0)] = Color;
    BB[ID.xy + uint2(1, 0)] = Color;
    BB[ID.xy + uint2(1, 1)] = Color;
    BB[ID.xy + uint2(0, 1)] = Color;
}

void Write1x1Dot(uint2 ID, float4 Color)
{
    BB[ID.xy + uint2(0, 0)] = Color;
}

groupshared PointLight Lights[256];

float2 ViewToTextureSpace(float2 CoordDS)
{
    return float2((CoordDS.x + 1) / 2, (1 - CoordDS.y) / 2) * float2(WindowWidth, WindowHeight);
}


bool IntersectsDepthBuffer(float Z, float MinZ, float MaxZ)
{
    return (MaxZ >= Z) && (MinZ <= Z);
}

bool Raycast(const float3 OriginCS, const float3 RayCS, out float2 HitPixel, out float3 HitPosition)
{
    HitPixel    = 0;
    HitPosition = 0;

    float RayLength = ((OriginCS.z + RayCS.z * MaxZ) < MinZ) ? 
        (MinZ - OriginCS.z) / RayCS.z : MaxZ;

    float3 EndPointCS = OriginCS + RayCS * RayLength;

    float4 OriginDS = mul(PV, float4(OriginCS, 1.0f) );
    OriginDS /= OriginDS.w;

    float4 EndPDS = mul(PV, float4(EndPointCS, 1.0f));
    EndPDS /= EndPDS.w;

    float4 H0 = float4(ViewToTextureSpace(OriginDS.xy), 0, 0);
    float4 H1 = float4(ViewToTextureSpace(EndPDS.xy), 0, 0);

    float k0 = 1.0f / H0.w;
    float k1 = 1.0f / H1.w;

    float3 Q0 = OriginCS * k0;
    float3 Q1 = EndPointCS * k1;

    float2 P0 = H0.xy * k0;
    float2 P1 = H1.xy * k1;

    HitPixel = H1;

    Write2x2Dot(H0, 1);
    Write2x2Dot(H1, 1);

    bool hitFound           = false;
    float Steps             = 512.0f;
    float PrevZMaxEstimate  = OriginCS.z;
    float RayZMin           = 0;
    float RayZMax           = 0;
    float Z                 = 100.0f;

    for (   float I = 0; 
            I < Steps && !IntersectsDepthBuffer(Z, RayZMin, RayZMax); 
            I+= 1.0f)
    {
        float fi = 1/Steps * I;
        float fi2 = fi * fi;
        float2 Temp2 = lerp(H0, H1, fi);
        Write1x1Dot(Temp2, float4(lerp(float2(1, 0), float2(0, 1), fi2), 0, 0));
        hitFound = true;
    }

    return hitFound;
}

float3 GetWorldSpacePositionAndViewDir(float3 UVW, out float3 VRayWS)
{
    float3 Temp1    = lerp(WSTopLeft, WSBottomLeft, UVW.y);
    float3 Temp2    = lerp(WSTopRight, WSBottomRight, UVW.y);
    float3 FarPos   = lerp(Temp1, Temp2, UVW.x);

    //float3 Temp3    = lerp(WSTopLeft_Near, WSBottomLeft_Near, UVW.y);
    //float3 Temp4    = lerp(WSTopRight_Near, WSBottomRight_Near, UVW.y);
    //float3 NearPOS  = lerp(Temp3, Temp4, UVW.x);

    VRayWS = normalize(FarPos - CameraPOS);
    return CameraPOS.xyz + VRayWS * UVW.z;
}

[numthreads(32, 12, 1)]
void Trace(uint3 ID : SV_DispatchThreadID, uint3 TID : SV_GroupThreadID)
{
    uint ThreadID = TID.x % 16 + TID.y * 8;
    int MatID = Color.Load(int3(ID.xy, 0)).w;
    
    int Itr = 0;
    while (Itr < PointLightCount)
    {
        Lights[Itr + ThreadID] = PointLights[ThreadID];
        Itr += 16 * 8;
    }

    float4 StartingColor = BB[ID.xy];

    GroupMemoryBarrierWithGroupSync();

    if (MatID == 0)
    {
        BB[ID.xy] = 0;
        return;
    }

    // Reconstruct World Position

    float3 ViewRayWS = 0;

	float3 N = Normal.Load(int3(ID.xy, 0)).xyz;
	float  l = Normal.Load(int3(ID.xy, 0)).w;

    float3 UVW = float3(ID.xy / float2(WindowWidth, WindowHeight), l);

    float Distance          = l;
    float DistanceTravelled = 0;
    float LinearDistanceNormalized = (Distance - MinZ) / (100 - MinZ);

    const float3 OriginWS   = GetWorldSpacePositionAndViewDir(UVW, ViewRayWS);
	const float  r           = RoughMetal.Load(int3(ID.xy, 0)).x;

    float4 Out = float4(OriginWS, 0);
    float MaxDistance = 1000.0f;
    
    if (ID.x % 256 == 0 && ID.y % 256 == 0 && TID.z == 0)// Making debugging easier with this, should be able to follow a limited number of traces
    //if (TID.x == 0 && TID.y == 0 && TID.z == 0)// Making debugging easier with this, should be able to follow a limited number of traces
    {
        float3 RayWS    = reflect(ViewRayWS, N);
        float4 OriginCS = mul(View, float4(OriginWS, 1));
        float4 RayCS    = mul(View, float4(RayWS, 0));

        float2 HitPixel;
        float3 HitPosition;

        #if 1
        if (Raycast(OriginWS.xyz, RayWS.xyz, HitPixel, HitPosition))
        {
            //Write2x2Dot(HitPixel, float4(LinearDistanceNormalized, LinearDistanceNormalized, LinearDistanceNormalized, 0));
            //Write2x2Dot(HitPixel, float4(0, 1, 0, 0));
            Write2x2Dot(ID, float4(0, 0, 1, 0));
        }
        #else
        Write2x2Dot(ID, float4(OriginWS, 0));
        #endif
    }
}
