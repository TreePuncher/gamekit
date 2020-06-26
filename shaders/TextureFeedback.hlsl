#include "common.hlsl"

#define NODESIZE 12

cbuffer EntityConstants : register(b1)
{
    float4      Albedo;
    float       Ks;
    float       IOR;
    float       Roughness;
    float       Anisotropic;
    float       Metallic;
    float4x4    WT;

    uint4 Textures[16];
}


// XY - SIZE
// Z  - ID
// W  - PADDING
cbuffer PassConstants : register(b2)
{
    float  feedbackBias;
    float  padding;
    uint4  textureConstants[16];
    uint4  zeroBlock[2];
}

globallycoherent    RWByteAddressBuffer UAVCounters     : register(u0);
globallycoherent    RWByteAddressBuffer UAVBuffer       : register(u1);
globallycoherent    RWByteAddressBuffer UAVOffsets      : register(u2);
globallycoherent    RWTexture2D<uint>   PPLinkedList    : register(u3);

Texture2D<float4>   textures[16] : register(t0);

SamplerState   defaultSampler : register(s1);

void WriteOut(uint blockID, uint textureID, uint offset, uint prev)
{
    UAVBuffer.Store3(offset, uint3(blockID, textureID, prev));
}

struct Forward_VS_OUT
{
    float4 POS 		: SV_POSITION;
    float3 WPOS 	: POSITION;
    float3 Normal	: NORMAL;
    float3 Tangent	: TANGENT;
    float2 UV		: TEXCOORD;
};

void ClearPS(float4 XY : SV_POSITION)
{
    UAVOffsets.Store(XY.x * 4 + XY.y * 4 * 240, 0);

    if(XY.x == 0.5f && XY.y == 0.5f)
        UAVCounters.Store(0, 0);
}

void TextureFeedback_PS(Forward_VS_OUT IN, float4 XY : SV_POSITION)
{
    const float maxAniso = 4;
    const float maxAnisoLog2 = log2(maxAniso);
    const uint textureCount = 2;
    const float2 UV = IN.UV;
    
    uint offset     = 0;
    UAVCounters.InterlockedAdd(0, NODESIZE * textureCount, offset);
    UAVOffsets.Store(XY.x * 4 + XY.y * 4 * 240, offset);
    uint prev = PPLinkedList[XY.xy];

    for(uint I = 0; I < textureCount; I++)
    {
        uint2 WH = uint2(0, 0);
        uint MIPCount = 0;
        textures[I].GetDimensions(0.0f, WH.x, WH.y, MIPCount);
        
        const float2 dx = ddx(UV * WH);
        const float2 dy = ddy(UV * WH);

        const float px = dot(dx, dx);
        const float py = dot(dy, dy);

        const float maxLod = 0.5 * log2(max(px, py));
        const float minLod = 0.5 * log2(min(px, py));

        const float anisoLOD    = maxLod - min(maxLod - minLod, maxAnisoLog2);
        const float desiredLod = max(min(floor(maxLod) + feedbackBias + 0.5f, MIPCount - 1), 0);
        

        const uint textureID    = Textures[I].z;
        const uint2 blockSize   = Textures[I].xy;

        int lod = desiredLod;
        while (lod < MIPCount - 1)
        {
            // TODO: mark all needed MIP levels, rather then one level at a time
            // Make sure that the lower level mip is loaded first
            uint state;
            textures[I].SampleLevel(defaultSampler, UV, min(MIPCount, lod + 1), 0.0f, state);

            if(CheckAccessFullyMapped(state))
                break;

            lod += 1;
            textures[I].GetDimensions(max(lod, 0), WH.x, WH.y, MIPCount);
        }

        uint2 MIPWH = 0;
        textures[I].GetDimensions(lod, MIPWH.x, MIPWH.y, MIPCount);
        
        const int packed = MIPWH.x < blockSize.x;
        
        if (packed)
        {
            uint2 WH;
            int MIPCount;
            int mipLevel = 0;
            
            for (int mipLevel = 0; mipLevel < 16; mipLevel++)
            {
                textures[I].GetDimensions(mipLevel, WH.x, WH.y, MIPCount);
                if(WH.x < blockSize.x)
                    break;
            }

            lod = mipLevel;
            textures[I].GetDimensions(lod, MIPWH.x, MIPWH.y, MIPCount);
        }
        
        const uint2 blockArea   = max(uint2(MIPWH / blockSize), uint2(1, 1));
        const uint2 blockXY     = min(blockArea * saturate(IN.UV), blockArea - uint2(1, 1));
        const uint  blockID     = (packed << 31) | ((MIPCount - lod) << 24 ) | (packed ? 0 : (blockXY.x << 12) | (blockXY.y));

        if (MIPWH.x == 0 || MIPWH.y == 0)
            return;

        const uint nodeOffset = offset + NODESIZE * I;
        WriteOut(blockID, textureID, nodeOffset, prev);
        prev = nodeOffset;
    }

    PPLinkedList[XY.xy] = prev;
}
