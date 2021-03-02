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
    uint4  zeroBlock[2];
}

AppendStructuredBuffer<uint2>   UAVBuffer : register(u0);
Texture2D<float4>               textures[16] : register(t0);
SamplerState                    defaultSampler : register(s1);

struct Forward_VS_OUT
{
    float4 POS 		: SV_POSITION;
    float3 WPOS 	: POSITION;
    float3 Normal	: NORMAL;
    float3 Tangent	: TANGENT;
    float2 UV		: TEXCOORD;
};

uint Packed(uint TextureIdx, const float2 UV, const uint lod)
{
    const uint2 blockSize   = Textures[TextureIdx].xy;
    
    uint2 MIPWH     = 0;
    uint MIPCount   = 0;
    
    textures[TextureIdx].GetDimensions(lod, MIPWH.x, MIPWH.y, MIPCount);
    const int packed = MIPWH.x < blockSize.x;

    return packed;
}

uint CreateTileID(const uint2 XY, const uint mipLevel, const bool packed, const uint mipCount)
{
    if(packed)
        return (packed << 31) | (mipLevel << 24);
    else
        return (packed << 31) | (mipLevel << 24) | (packed ? 0 : ((0xFF & XY.x) << 12) | (0xFF & XY.y));
}

void PushSample(const float2 UV, const uint desiredLod, const uint TextureIdx, uint2 XY)
{
    uint2 MIPWH     = 0;
    uint MIPCount   = 0;

    const uint textureID    = Textures[TextureIdx].z;
    const uint2 blockSize   = Textures[TextureIdx].xy;
    
    textures[TextureIdx].GetDimensions(desiredLod, MIPWH.x, MIPWH.y, MIPCount);

    if (MIPWH.x == 0 || MIPWH.y == 0)
        return;
        
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
    const uint  blockID     = CreateTileID(blockXY, lod, packed, MIPCount);

    UAVBuffer.Append(uint2(textureID, blockID));
}

bool CheckLoaded(Texture2D source, in sampler textureSampler, in float2 UV, uint mip)
{
    uint state;
    const float4 texel = source.SampleLevel(textureSampler, UV, mip, 0.0f, state);

    return CheckAccessFullyMapped(state);
}

[earlydepthstencil]  
void TextureFeedback_PS(Forward_VS_OUT IN)
{
    const float4 XY             = IN.POS;
    const float maxAniso        = 4;
    const float maxAnisoLog2    = log2(maxAniso);
    const float2 UV             = IN.UV % 1.0f;
    
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
        const float desiredLod  = max(min(floor(maxLod) + feedbackBias - 0.5f, MIPCount - 1), 0);
        
        for(int lod = MIPCount - 1; lod > 0 && lod > floor(desiredLod); --lod)
        {
            if(!CheckLoaded(textures[I], defaultSampler, UV, lod))
            {
                PushSample(UV, lod, I, XY.xy);
                break;
            }
        }
    }
}
