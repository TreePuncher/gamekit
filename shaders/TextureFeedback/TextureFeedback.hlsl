#include "common.hlsl"

cbuffer EntityConstants : register(b1)
{
	float4		Albedo;
	float		Ks;
	float		IOR;
	float		Roughness;
	float		Anisotropic;
	float		Metallic;
	float4x4	WT;
	uint		textureFlags;
	uint4		texturesInfo[16]; // XY - SIZE, ID, Padding
}

cbuffer PassConstants : register(b2)
{
	float	feedbackBias;
	uint	offset0;
	uint	offset1;
	uint	offset2;
	uint	offset3;
	uint	offset4;
	uint	offset5;
	uint	offset6;
	uint	offset7;
	uint	offset8;
	uint	offset9;
	uint	offset10;
	uint	offset11;
	uint	offset12;
	uint	offset13;
	uint	offset14;
	uint	offset15;
}

globallycoherent	RWStructuredBuffer<uint>	texturesFeedback	: register(u0);
					Texture2D<float4>			textures[]			: register(t0);
SamplerState									defaultSampler		: register(s1);

struct Forward_VS_OUT
{
	float4 POS		: SV_POSITION;
	float  depth	: DEPTH;
	float3 Normal	: NORMAL;
	float3 Tangent	: TANGENT;
	float2 UV		: TEXCOORD;
};

uint GetOffset(uint idx)
{
	switch (idx)
	{
	case 0:
		return offset0;
	case 1:
		return offset1;
	case 2:
		return offset2;
	case 3:
		return offset3;
	case 4:
		return offset4;
	case 5:
		return offset5;
	case 6:
		return offset6;
	case 7:
		return offset7;
	case 8:
		return offset8;
	case 9:
		return offset9;
	case 10:
		return offset10;
	case 11:
		return offset11;
	case 12:
		return offset12;
	case 13:
		return offset13;
	case 14:
		return offset14;
	case 15:
		return offset15;
	default:
		return -1;
	}
}

[earlydepthstencil]  
void TextureFeedback_PS(Forward_VS_OUT IN)
{
	const float2 UV	= IN.UV % 1.0f;
	int textureIdx = 0;

	for(uint I = 0; I < 16; I++)
	{
		if ((textureFlags & (0x01U << I)) == 0)
			continue;

		uint2 WH = uint2(0, 0);
		uint MIPCount = 0;
		textures[NonUniformResourceIndex(textureIdx)].GetDimensions(0.0f, WH.x, WH.y, MIPCount);

		const float mip_temp		= textures[NonUniformResourceIndex(textureIdx)].CalculateLevelOfDetail(defaultSampler, UV);
		const float desiredLod		= clamp(mip_temp + feedbackBias, 0.0f, MIPCount - 1.0f);

		const uint2 tileSize		= texturesInfo[textureIdx].xy;
		const uint2 tileArea		= WH / tileSize;
		const uint textureOffset	= GetOffset(textureIdx);

		for (int lod = desiredLod; lod < MIPCount; ++lod)
		{
			const uint2 tile = uint2(tileArea * UV) >> lod;
			InterlockedOr(texturesFeedback[(tile.x + tile.y * tileArea.x) + textureOffset], 0x01 << lod);
		}

		textureIdx++;
	}
}


/**********************************************************************

Copyright (c) 2023 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/
