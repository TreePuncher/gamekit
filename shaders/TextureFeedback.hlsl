#include "common.hlsl"

cbuffer LocalConstants : register(b1)
{

}

// XY - SIZE
// Z  - ID
// W  - PADDING
cbuffer PassConstants : register(b2)
{
    uint4  textureConstants[16];
    uint4  zeroBlock[2];
}

globallycoherent    RWByteAddressBuffer    UAVCounters     : register(u0);
globallycoherent    RWByteAddressBuffer    UAVBuffer       : register(u1);

Texture2D<float4>   textures[16] : register(t0);

SamplerState   defaultSampler : register(s1);

void WriteOut(uint blockID, uint textureID, uint Offset)
{
    UAVBuffer.Store(Offset, blockID);
    UAVBuffer.Store(Offset + 4, textureID);
}

struct Forward_VS_OUT
{
    float4 POS 		: SV_POSITION;
    float3 WPOS 	: POSITION;
    float3 Normal	: NORMAL;
    float3 Tangent	: TANGENT;
    float2 UV		: TEXCOORD;
};


void TextureFeedback_PS(Forward_VS_OUT IN, float4 XY : SV_POSITION)
{
    uint textureCount = 1;
    for(uint I = 0; I < textureCount; I++)
    {
        float2 d_xy             = ddx(IN.UV);
        uint MIPLevelRequested  = floor(pow(d_xy, 2));
        uint2 WH                = uint2(0, 0);
        uint mip                = 0;
        const uint textureID    = textureConstants[I].z;
        const uint2 blockSize   = textureConstants[I].xy;

        textures[I].GetDimensions(MIPLevelRequested, WH.x, WH.y, mip);

        const uint2 blockXY     = min(uint2(WH / blockSize) * saturate(IN.UV), WH  / blockSize - 1);
        //const uint  blockID     = (0 << 16) | (0 << 4) | min(0, 16);
        const uint blockID    = (blockXY.x << 16) | (blockXY.y << 4) | min(0, 16);

        if(WH.x == 0 || WH.y == 0)
            return;

        uint offset = 0;
        UAVCounters.InterlockedAdd(0, 8, offset);

        uint bufferSize; UAVBuffer.GetDimensions(bufferSize);
        //if(bufferSize > offset)
            WriteOut(blockID, textureID, offset);
    }
}
