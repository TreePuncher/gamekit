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
    const uint textureCount = 1;
    const float2 UV = IN.UV;

    for(uint I = 0; I < textureCount; I++)
    {
        int   MIP               = textures[I].CalculateLevelOfDetail(defaultSampler, UV);
        uint2 WH                = uint2(0, 0);
        uint MIPCount           = 0;
        const uint textureID    = textureConstants[I].z;
        const uint2 blockSize   = textureConstants[I].xy;
        textures[I].GetDimensions(MIP, WH.x, WH.y, MIPCount);

        /*
        while(MIP < MIPCount)
        {
            uint state;
            textures[I].SampleLevel(defaultSampler, UV, MIP, 0.0f, state);

            if(CheckAccessFullyMapped(state))
            {
                break;
            } 
            else
            {
                MIP = floor(MIP) + 1.0f;
                textures[I].GetDimensions(max(MIP - 1, 0), WH.x, WH.y, MIPCount);
            }
        }
        */

        //MIP = min(MIP, MIPCount - 1);
        const uint2 blockArea   = max(uint2(WH / blockSize), uint2(1, 1));
        const uint2 blockXY     = min(blockArea * saturate(IN.UV), blockArea - uint2(1, 1));
        const uint  blockID     = ((MIPCount - MIP) << 24 ) | (blockXY.x << 12) | (blockXY.y);

        if(WH.x == 0 || WH.y == 0)
            return;

        uint offset = 0;
        UAVCounters.InterlockedAdd(0, 8, offset);

        uint bufferSize; UAVBuffer.GetDimensions(bufferSize);
        WriteOut(blockID, textureID, offset);
    }
}
