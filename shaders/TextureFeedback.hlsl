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
    uint2 WH                = uint2(0, 0);
    uint mip                = 0;

    texture1.GetDimensions(MIPLevelRequested, WH.x, WH.y, mip);
    const uint2 blockSize = uint2(256, 256);
    //const uint2 blockSize = textureBlockSize[0];
    const uint2 blockXY   = min(uint2(WH / blockSize) * saturate(IN.UV), WH  / blockSize - 1);
    uint blockID    = (blockXY.x << 16) | (blockXY.y << 4) | min(MIPLevelRequested, 16);

    if(WH.x == 0 || WH.y == 0)
        return;

    uint status = 0;
    texture1.Load(uint3(IN.UV, 0), status);

    if(!CheckAccessFullyMapped(status)) // Texture not mapped!
    {
        uint offset = 0;
        UAVCounters.InterlockedAdd(0, 1, offset);

        uint bufferSize; UAVBuffer.GetDimensions(bufferSize);
        if(bufferSize > offset)
            WriteOut(blockID, textureID[0], offset * 8);
    }
}