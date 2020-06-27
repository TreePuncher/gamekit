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
    uint        textureCount;
    uint4       Textures[16];
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


uint Packed(uint TextureIdx, const float2 UV, const uint lod)
{
    const uint2 blockSize   = Textures[TextureIdx].xy;
    
    uint2 MIPWH     = 0;
    uint MIPCount   = 0;
    
    textures[TextureIdx].GetDimensions(lod, MIPWH.x, MIPWH.y, MIPCount);
    const int packed = MIPWH.x < blockSize.x;

    return packed;
}

uint PushSample(const uint Prev, const float2 UV, const uint desiredLod, const uint TextureIdx, uint2 XY)
{
    uint2 MIPWH     = 0;
    uint MIPCount   = 0;

    const uint textureID    = Textures[TextureIdx].z;
    const uint2 blockSize   = Textures[TextureIdx].xy;
    
    textures[TextureIdx].GetDimensions(desiredLod, MIPWH.x, MIPWH.y, MIPCount);

    int lod             = desiredLod;
    const int packed    = MIPWH.x < blockSize.x;
    
    if (packed)
    {
        uint2 WH;
        int MIPCount;
        int mipLevel = 0;
            
        for (int mipLevel = 0; mipLevel < 16; mipLevel++)
        {
            textures[TextureIdx].GetDimensions(mipLevel, WH.x, WH.y, MIPCount);
            if (WH.x < blockSize.x)
                break;
        }

        lod = mipLevel;
        textures[TextureIdx].GetDimensions(lod, MIPWH.x, MIPWH.y, MIPCount);
    }
    
    const uint2 blockArea   = max(uint2(MIPWH / blockSize), uint2(1, 1));
    const uint2 blockXY     = min(blockArea * saturate(UV), blockArea - uint2(1, 1));
    const uint  blockID     = (packed << 31) | ((MIPCount - lod) << 24) | (packed ? 0 : (blockXY.x << 12) | (blockXY.y));

    MIPWH = 0;
    textures[TextureIdx].GetDimensions(lod, MIPWH.x, MIPWH.y, MIPCount);
        
    if (MIPWH.x == 0 || MIPWH.y == 0)
        return Prev;

    uint offset = -1;
    UAVCounters.InterlockedAdd(0, NODESIZE, offset);
    WriteOut(blockID, textureID, offset, Prev);
    //UAVOffsets.Store(XY.x * 4 + XY.y * 4 * 240, Prev);
    
    return offset;
}


void TextureFeedback_PS(Forward_VS_OUT IN, float4 XY : SV_POSITION)
{
    const float maxAniso = 4;
    const float maxAnisoLog2 = log2(maxAniso);
    const float2 UV = IN.UV;
    
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
        const float desiredLod  = max(min(floor(maxLod) + feedbackBias + 0.5f, MIPCount - 1), 0);

        int lod = desiredLod;
        while (lod < MIPCount - 1)
        {
            prev = PushSample(prev, UV, lod, I, XY.xy);
            if (Packed(I, UV, lod))
                break;
            
            lod += 1;
        }
    }

    PPLinkedList[XY.xy] = prev;
}
