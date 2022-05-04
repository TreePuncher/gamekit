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
    uint     textureCount;
}

cbuffer ShadingConstants : register(b2)
{
    float lightCount;
    float globalTime;
    uint2 WH;
}

cbuffer Poses : register(b3)
{
    float4x4 Poses[768];
}

Texture2D<float4> albedoTexture  : register(t0);
Texture2D<float4> normalTexture  : register(t1);
Texture2D<float4> roughnessMetalTexture : register(t2);

//TextureCube<float4>             HDRMap          : register(t3);
Texture2D<float4>		        MRIATexture     : register(t4);

StructuredBuffer<uint>		    lightLists	    : register(t5);
ByteAddressBuffer               pointLights     : register(t6);

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


struct VertexSkinned
{
    float3 POS		: POSITION;
    float3 Normal	: NORMAL;
    float3 Tangent	: Tangent;
    float2 UV		: TEXCOORD;
    float3 Weights  : BLENDWEIGHT;
    uint4  Indices  : BLENDINDICES; 
};

Forward_VS_OUT ForwardSkinned_VS(VertexSkinned In)
{
	float4 N = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 T = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 V = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 W = float4(In.Weights.xyz, 1 - In.Weights.x - In.Weights.y - In.Weights.z);

	float4x4 MTs[4] =
	{
		Poses[In.Indices[0]],
		Poses[In.Indices[1]],
		Poses[In.Indices[2]],
		Poses[In.Indices[3]],
    };

	[unroll(4)]
	for (uint I = 0; I < 4; ++I)
	{
        V += mul(MTs[I], float4(In.POS, 1)) * W[I];
        N += mul(MTs[I], float4(In.Normal, 0)) * W[I];
        T += mul(MTs[I], float4(In.Tangent, 0)) * W[I];
    }

    const float3 POS_WS = mul(WT, float4(V.xyz, 1));
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


    float MIPCount      = 1;
    float width         = 0;
    float height        = 0;
    source.GetDimensions(0u, width, height, MIPCount);

    float mip = source.CalculateLevelOfDetail(textureSampler, UV);

    while(mip < MIPCount)
    {
        uint state;
        const float4 texel = source.SampleLevel(textureSampler, UV, mip, 0.0f, state);

        if(CheckAccessFullyMapped(state))
            return texel;

        mip = floor(mip + 1);
    }
    
    return float4(1.0f, 0.0f, 1.0f, 0.0f); // NO PAGES LOADED!
}

Deferred_OUT GBufferFill_PS(Forward_PS_IN IN)
{
    Deferred_OUT gbuffer;

    const float4 albedo             = textureCount >= 1 ? SampleVirtualTexture(albedoTexture, BiLinear, IN.UV) : Albedo;
    const float3 biTangent          = normalize(IN.Bitangent);
    const float4 roughMetal         = textureCount >= 3 ? SampleVirtualTexture(roughnessMetalTexture, BiLinear, IN.UV) : float4(IOR, Roughness, Metallic, Anisotropic);
    const float3 normalSample       = textureCount >= 2 ? SampleVirtualTexture(normalTexture, BiLinear, IN.UV).xyz : float3(0.5f, 0.5f, 1.0f);
    const float3 normalCorrected    = float3(normalSample.x, normalSample.y, normalSample.z);
    
    float3x3 inverseTBN = float3x3(normalize(IN.Tangent), normalize(biTangent), normalize(IN.Normal));
    float3x3 TBN        = transpose(inverseTBN);
    const float3 normal = mul(TBN, normalCorrected * 2.0f - 1.0f);

    gbuffer.Albedo      = float4(albedo.xyz, Ks);
    gbuffer.MRIA        = roughMetal.zyxw * float4(Metallic, Roughness, IOR, Anisotropic);
    gbuffer.Normal      = mul(View, float4(normalize(normal), 0)).xy;
    gbuffer.Depth       = IN.depth;

    return gbuffer;
}

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
