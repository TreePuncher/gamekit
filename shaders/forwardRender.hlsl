#include "common.hlsl"
#include "pbr.hlsl"

struct PointLight
{
	float4 KI;	// Color + intensity in W
	float4 PR;	// XYZ + radius in W
};

cbuffer LocalConstants : register(b1)
{
	float4	 Albedo;
	float    Ks;
	float    IOR;
	float    Roughness;
	float    Anisotropic;
	float    Metallic;
	float4x4 WT;
	uint     textureChannels;
}

cbuffer ShadingConstants : register(b2)
{
	float lightCount;
	float globalTime;
	uint2 WH;
}

Texture2D<float4> textures[3]	: register(t0);

//TextureCube<float4>			HDRMap			: register(t3);
Texture2D<float4>				MRIATexture		: register(t4);

StructuredBuffer<uint>			lightLists		: register(t5);
ByteAddressBuffer				pointLights		: register(t6);
StructuredBuffer<float4x4>		Poses			: register(t7);


sampler BiLinear : register(s0); 
sampler NearestPoint : register(s1); // Nearest point

bool isLightContributing(uint idx, uint2 xy)
{
	const uint offset       = (xy.x + xy.y * WH[0]) * (1024 / 32);
	const uint wordOffset   = idx / 32;
	const uint bitMask      = 0x1 << (idx % 32);

	return lightLists[wordOffset + offset] & bitMask;
}

PointLight ReadPointLight(uint idx)
{
	PointLight pointLight;

	uint4 load1 = pointLights.Load4(idx * 32 + 00);
	uint4 load2 = pointLights.Load4(idx * 32 + 16);

	pointLight.KI   = asfloat(load1);
	pointLight.PR   = asfloat(load2);

	return pointLight;
}

struct Vertex
{
	float3 POS		: POSITION;
	float3 Normal	: NORMAL;
	float3 Tangent	: Tangent;
	float2 UV		: TEXCOORD;
};

struct Forward_VS_OUT
{
	float4 POS 		: SV_POSITION;
	float  depth    : DEPTH;
	float3 Normal	: NORMAL;
	float3 Tangent	: TANGENT;
	float2 UV		: TEXCOORD;
	float3 Bitangent : BITANGENT;
};

Forward_VS_OUT Forward_VS(Vertex In)
{
	const float3 POS_WS = mul(WT, float4(In.POS, 1));
	const float3 POS_VS = mul(View, float4(POS_WS, 1));

	Forward_VS_OUT Out;
	Out.depth       = -POS_VS.z / MaxZ;
	Out.POS		    = mul(PV, float4(POS_WS, 1));
	Out.Normal      = normalize(mul(WT, float4(In.Normal, 0.0f)));
	Out.Tangent     = normalize(mul(WT, float4(In.Tangent, 0.0f)));
	Out.Bitangent   = cross(Out.Tangent, Out.Normal);
	Out.UV		    = In.UV;

	return Out;
}


/************************************************************************************************/


struct VertexSkinned
{
	float3 POS				: POSITION;
	float3 Normal			: NORMAL;
	float3 Tangent			: Tangent;
	float2 UV				: TEXCOORD;
	float3 Weights			: BLENDWEIGHT;
	uint4  Indices			: BLENDINDICES;
	float3 POS_Blend		: BLENDPOS;
	float3 Normal_Blend		: BLENDNORM;
	float3 Tangent_Blend	: BLENDTAN;
};

Forward_VS_OUT ForwardSkinned_VS(VertexSkinned In)
{
	//float4 P = float4(In.POS_Blend, 0.0f);
	//float4 N = float4(In.Normal_Blend, 0.0f);
	//float4 T = float4(In.Tangent_Blend, 0.0f);

	float4 P = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 N = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 T = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 W = float4(In.Weights.xyz, 1 - In.Weights.x - In.Weights.y - In.Weights.z);

	float4x4 Identity =
		float4x4(
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1);

	[unroll(4)]
	for (uint I = 0; I < 4; ++I)
	{
		P += mul(Poses[In.Indices[I]], float4(In.POS, 1)) * W[I];
		N += mul(Poses[In.Indices[I]], float4(In.Normal, 0)) * W[I];
		T += mul(Poses[In.Indices[I]], float4(In.Tangent, 0)) * W[I];
	}

	const float3 POS_WS = mul(WT, float4(P.xyz, 1));
	const float3 POS_VS = mul(View, float4(POS_WS, 1));

	Forward_VS_OUT Out;
	Out.depth       = -POS_VS.z / MaxZ;
	Out.POS		    = mul(PV, float4(POS_WS, 1));
	Out.Normal      = normalize(mul(WT, float4(N.xyz, 0.0f)));
	Out.Tangent     = normalize(mul(WT, float4(T.xyz, 0.0f)));
	Out.Bitangent   = cross(Out.Tangent, Out.Normal);
	Out.UV		    = In.UV;

	return Out;
}

float4 DepthPass_VS(float3 POS : POSITION) : SV_POSITION
{
	return mul(PV, mul(WT, float4(POS, 1)));
}


/************************************************************************************************/


struct Forward_PS_IN
{
	centroid    float4  POS         : SV_POSITION;
				float   depth       : DEPTH;
				float3	Normal	    : NORMAL;
				float3	Tangent	    : TANGENT;
				float2	UV		    : TEXCOORD;
				float3	Bitangent	: BITANGENT;
};

struct Deferred_OUT
{
	float4 Albedo   : SV_TARGET0;
	float4 MRIA     : SV_TARGET1;
	float2 Normal   : SV_TARGET2;

	float Depth : SV_DepthLessEqual;
};


float4 SampleVirtualTexture(Texture2D source, in sampler textureSampler, in float2 UV)
{
	const float4 MIPColors[4] = 
	{
		float4(1, 1, 1, 1),
		float4(1, 0, 0, 1),
		float4(0, 1, 0, 1),
		float4(0, 0, 1, 1),
	};


	float MIPCount		= 1;
	float width			= 0;
	float height		= 0;
	source.GetDimensions(0u, width, height, MIPCount);

	float mip = source.CalculateLevelOfDetail(textureSampler, UV);
	uint state1;

	float4 texel = source.Sample(textureSampler, UV, 0, 0.0, state1);

	if (CheckAccessFullyMapped(state1))
		return texel;

	mip = floor(mip);

	while(mip < MIPCount)
	{
		float4 texel = source.SampleLevel(textureSampler, UV, trunc(mip), 0, state1);

		if(CheckAccessFullyMapped(state1))
			return texel;

		mip += 1;
	}
	
	return float4(1.0f, 0.0f, 1.0f, 0.0f); // NO PAGES LOADED!
}


float4 VirtualTextureDebug(Texture2D source, in sampler textureSampler, in float2 UV)
{
	float MIPCount		= 1;
	float width			= 0;
	float height		= 0;
	source.GetDimensions(0u, width, height, MIPCount);

	float4 colors[] = {
		float4(0.25f, 0.25f, 0.25f, 1),
		float4(0.5f, 0.5f, 0.5f, 1),
		float4(1.0f, 1.0f, 0.0f, 1),
		float4(0.0f, 1.0f, 1.0f, 1),
		float4(0.0f, 0.0f, 1.0f, 1),
		float4(1.0f, 0.0f, 1.0f, 1),
		float4(0.75f, 0.75f, 0.75f, 1),
		float4(1.0f, 1.0f, 1.0f, 1),
	};

	int mip = 0;
	while (mip < MIPCount)
	{
		uint state1;
		float4 texel = source.SampleLevel(textureSampler, UV, trunc(mip), 0, state1);

		if (CheckAccessFullyMapped(state1))
			return colors[mip % 8];

		mip += 1;
	}

	return float4(1, 0, 1, 0);
}

/************************************************************************************************/

#define ALBEDO 0x01
#define NORMAL 0x02
#define ROUGHNESS 0x04

float2 OctWrap(float2 v)
{
	return (1.0 - abs(v.yx)) * (v.xy >= 0.0 ? 1.0 : -1.0);
}

float2 Encode(float3 n)
{
	n /= (abs(n.x) + abs(n.y) + abs(n.z));
	n.xy = n.z >= 0.0 ? n.xy : OctWrap(n.xy);
	n.xy = n.xy * 0.5 + 0.5;
	return n.xy;
}

Deferred_OUT GBufferFill_PS(Forward_PS_IN IN)
{

	Deferred_OUT gbuffer;

	uint textureIdx = 0;

	float4 albedo;
	float4 roughMetal;

	if ((textureChannels & ALBEDO) != 0x00)
		albedo = SampleVirtualTexture(textures[NonUniformResourceIndex(textureIdx++)], BiLinear, IN.UV);
	else
		albedo = Albedo;

	if ((textureChannels & ROUGHNESS) != 0x00)
		roughMetal = SampleVirtualTexture(textures[NonUniformResourceIndex(textureIdx++)], BiLinear, IN.UV);
	else
		roughMetal = float4(IOR, Roughness, Metallic, Anisotropic);

	gbuffer.Albedo	= float4(albedo.xyz, Ks);
	gbuffer.MRIA	= float4(Metallic, Roughness, IOR, Anisotropic); //roughMetal.zyxw * 
	gbuffer.Depth	= IN.depth;

	if((textureChannels & NORMAL) != 0x00)
	{
		const float3 normalSample		= SampleVirtualTexture(textures[NonUniformResourceIndex(textureIdx++)], BiLinear, IN.UV).xyz;
		const float3 normalCorrected	= float3(normalSample.x, normalSample.y, normalSample.z);
		const float3 biTangent			= normalize(IN.Bitangent);
		float3x3 inverseTBN				= float3x3(normalize(IN.Tangent), normalize(biTangent), normalize(IN.Normal));
		float3x3 TBN					= transpose(inverseTBN);
		const float3 normal				= mul(TBN, normalCorrected * 2.0f - 1.0f);
		gbuffer.Normal					= Encode(normal);
	}
	else
		gbuffer.Normal = Encode(normalize(IN.Normal));


	return gbuffer;
}


/************************************************************************************************/


float4 Forward_PS(Forward_PS_IN IN) : SV_TARGET
{
	float3 Color = float3(0, 0, 0);
	return float4(Color, 1);
}

float4 FlatWhite(Forward_PS_IN IN) : SV_TARGET
{
	return float4(1, 1, 1, 1);
}


/************************************************************************************************/


float4 ColoredPolys(Forward_PS_IN IN, uint primitiveID : SV_PrimitiveID) : SV_TARGET
{
	static const float4 colors[] =
	{
		{ 185 / 255.0f, 131 / 255.0f, 255 / 255.0f, 1.0f },
		{ 148 / 255.0f, 179 / 255.0f, 253 / 255.0f, 1.0f },
		{ 148 / 255.0f, 218 / 255.0f, 255 / 255.0f, 1.0f },
		{ 153 / 255.0f, 254 / 255.0f, 255 / 255.0f, 1.0f },

		{ 137 / 255.0f, 181 / 255.0f, 175 / 255.0f, 1.0f },
		{ 150 / 255.0f, 199 / 255.0f, 193 / 255.0f, 1.0f },
		{ 222 / 255.0f, 217 / 255.0f, 196 / 255.0f, 1.0f },
		{ 208 / 255.0f, 202 / 255.0f, 178 / 255.0f, 1.0f },
	};

	return colors[primitiveID % 3] * saturate(dot(float3(0, 0, 1), mul(View, IN.Normal)));
}


float4 GreyPolys(Forward_PS_IN IN) : SV_TARGET
{
	return float4(0.5f, 0.5f, 0.5f, 1) * saturate(dot(float3(0, 0, 1), mul(View, IN.Normal)));
}


/**********************************************************************

Copyright (c) 2015 - 2022 Robert May

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
