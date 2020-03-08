#include "common.hlsl"

cbuffer LocalConstants : register(b1)
{

}

cbuffer PassConstants : register(b2)
{
    uint  textureID[16];
    uint2 textureBlockSize[16];
}

globallycoherent    RWByteAddressBuffer    UAVCounters     : register(u0);
globallycoherent    RWByteAddressBuffer    UAVBuffer       : register(u1);

Texture2D<float4>   texture1 : register(t0);

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
    // Get Texture Block ID
    float2 d_xy             = ddx(IN.UV);
    uint MIPLevelRequested  = floor(pow(d_xy, 2));
    uint width              = 0;
    uint height             = 0;
    uint mip                = 0;

    texture1.GetDimensions(MIPLevelRequested, width, height, mip);
    uint blockX  = min(uint(width  / 256) * saturate(IN.UV.x), uint(width  / 256) - 1);
    uint blockY  = min(uint(height / 256) * saturate(IN.UV.y), uint(height  / 256) - 1);
    uint blockID = (blockX << 16) | (blockY << 4) | min(MIPLevelRequested, 16);
    //uint blockID = (0<< 16) | (0 << 4) | min(MIPLevelRequested, 16);

    if(width == 0 || height == 0)
        return;

    uint status = 0;
    texture1.Sample(defaultSampler, uint3(IN.UV, MIPLevelRequested), status);
    

    if(!CheckAccessFullyMapped(status)) // Texture not mapped!
    {
        uint offset = 0;
        UAVCounters.InterlockedAdd(0, 1, offset);

        uint bufferSize; UAVBuffer.GetDimensions(bufferSize);
        if(bufferSize > offset)
            WriteOut(blockID, textureID[0], offset * 8);
    }
}