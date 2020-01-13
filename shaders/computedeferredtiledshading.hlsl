#include "Common.hlsl"

cbuffer ShadingConstants : register(b1)
{
    int     LightCount;
    uint2   WH;
}

Texture2D<float4> Albedo	: register(t0);
Texture2D<float4> MRIA		: register(t1);
Texture2D<float4> Normal    : register(t2);
Texture2D<float2> Tangent   : register(t3);
Texture2D<float4> Depth	    : register(t4);

StructuredBuffer<uint>	lightBitBuckets	: register(t5);
ByteAddressBuffer       pointLights     : register(t6);

struct PointLight
{
    float4 KI;
    float4 PositionR;
};

PointLight ReadPointLight(uint idx)
{
    PointLight pointLight;
    pointLight.KI           = asfloat(pointLights.Load4(idx * 32));
    pointLight.PositionR    = asfloat(pointLights.Load4(idx * 32 + 16));

    return pointLight;
}

RWTexture2D<float4> output  : register(u0);

groupshared float3 color[100];
[numthreads(10, 10, 8)]
void csmain(uint3 threadID : SV_GROUPTHREADID, uint3 groupID : SV_GROUPID)
{
    const uint   localthreadID  = threadID.x + threadID.y * 10 + threadID.z * 100;
    const uint2  localBucketID  = groupID;
    const uint   localPixelID   = threadID.x + threadID.y * 10;// + threadID.z * 100;
    const uint2  globalPixelID  = threadID.xy + groupID * 10;

    AllMemoryBarrier();

    const uint   offset         = (localBucketID.x + localBucketID.y * WH.x/10) * 32;
    const float2 NDS_Cord       = float2(-1, 1) + float2(2, -2) * float2(globalPixelID) / float2(WH);
    const float2 UV             = float2(globalPixelID) / float2(WH);
    const float4 albedo         = Albedo.Load(uint3(globalPixelID, 0));
    const float4 normal         = Normal.Load(uint3(globalPixelID, 0));
    const float  depth          = Depth.Load(uint3(globalPixelID, 0));

    const float3 N              = normalize(normal.xyz);
    const float3 V              = GetViewVector(UV);
    const float3 worldPosition  = V * depth * MaxZ + CameraPOS;
    
    float3 localColor = float3(0, 0, 0);
    for(uint I = 0; I < LightCount / 32; I += 1) // 32 bits
    {
        const uint lightBitBucket = I;

        for(uint II = 0; II < 4; II++) // increments 1 byte at a time
        {
            const uint threadZ          = threadID.z;
            const uint bitMask          = 1 << (threadZ + II * 8);
            const uint lightID          = (I * 32 + II * 8) + threadZ;
            if(lightBitBuckets[offset + lightBitBucket] & bitMask)
            {
                PointLight light = ReadPointLight(lightID);
                const float3 L   = normalize(light.PositionR.xyz - worldPosition);
                const float  Ld  = length(light.PositionR.xyz - worldPosition);
                const float  Lr  = light.PositionR.w;
                const float3 Lk  = light.KI.xyz;
                const float  Li  = light.KI.w;
                const float  La  = Li / pow(Ld, 2);
                localColor += Lk * dot(N, L) * La * saturate(1 - (pow(Ld, 10) / pow(Lr, 10)));
            }
        }
    }

    if(threadID.z == 0)
        color[localPixelID] = localColor;

    GroupMemoryBarrier();

    if(threadID.z == 8)
    {   
        float3 temp = color[localPixelID];
        GroupMemoryBarrier();
        color[localPixelID] = temp + localColor;
        GroupMemoryBarrier();
    }

    GroupMemoryBarrier();
    if(threadID.z == 0)
        output[globalPixelID] = float4(color[localPixelID], 1);
}
