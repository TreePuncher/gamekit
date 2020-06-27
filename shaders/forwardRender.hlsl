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
}

cbuffer ShadingConstants : register(b2)
{
    float lightCount;
    float globalTime;
    uint2 WH;
}

cbuffer Poses : register(b3)
{
    float4x4 Poses[128];
}

Texture2D<float4> albedoTexture : register(t0);
Texture2D<float4> testTexture   : register(t1);

TextureCube<float4>             HDRMap          : register(t3);
Texture2D<float4>		        MRIATexture     : register(t4);

StructuredBuffer<uint>		    lightLists	    : register(t5);
ByteAddressBuffer               pointLights     : register(t6);

sampler BiLinear : register(s0); // Nearest point
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
    float3 WPOS 	: POSITION;
    float3 Normal	: NORMAL;
    float3 Tangent	: TANGENT;
    float2 UV		: TEXCOORD;
};

Forward_VS_OUT Forward_VS(Vertex In)
{
    Forward_VS_OUT Out;
    Out.WPOS	= mul(WT, float4(In.POS, 1));
    Out.POS		= mul(PV, mul(WT, float4(In.POS, 1)));
    Out.Normal  = normalize(mul(WT, float4(In.Normal, 0.0f)));
    Out.Tangent = normalize(mul(WT, float4(In.Tangent, 0.0f)));
    Out.UV		= In.UV % 1.0f;

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
        #if 0
        V += mul(float4(In.POS, 1),     MTs[I]) * W[I];
        N += mul(float4(In.Normal, 0),  MTs[I]) * W[I];
        T += mul(float4(In.Tangent, 0), MTs[I]) * W[I];
        #else
        V += mul(MTs[I], float4(In.POS, 1)) * W[I];
        N += mul(MTs[I], float4(In.Normal, 0)) * W[I];
        T += mul(MTs[I], float4(In.Tangent, 0)) * W[I];
        #endif
    }
    
    Forward_VS_OUT Out;
    Out.WPOS	= mul(WT, float4(V.xyz, 1));
    Out.POS		= mul(PV, mul(WT, float4(V.xyz, 1)));
    Out.Normal  = normalize(mul(WT, float4(N.xyz, 0.0f)));
    Out.Tangent = normalize(mul(WT, float4(T.xyz, 0.0f)));
    Out.UV		= In.UV;

    return Out;
}

float4 DepthPass_VS(float3 POS : POSITION) : SV_POSITION
{
    return mul(PV, mul(WT, float4(POS, 1)));
}

struct Forward_PS_IN
{
    float4	POS 	: SV_POSITION;
    float3	WPOS 	: POSITION;
    float3	Normal	: NORMAL;
    float3	Tangent	: TANGENT;
    float2	UV		: TEXCOORD;
};

struct Deferred_OUT
{
    float4 Albedo       : SV_TARGET0;
    float4 MRIA         : SV_TARGET1;
    float4 Normal       : SV_TARGET2;
    float4 Tangent      : SV_TARGET3;

    float Depth : SV_DEPTH;
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

    const float lod = source.CalculateLevelOfDetail(textureSampler, UV);
    float mip = lod;

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

    gbuffer.Normal      = float4(IN.Normal,     1);
    gbuffer.Tangent     = float4(normalize(cross(IN.Normal, IN.Normal.zxy)),    1);
    gbuffer.Albedo      = float4(Albedo.xyz *
        SampleVirtualTexture(albedoTexture, BiLinear, IN.UV).xyz * 
        SampleVirtualTexture(testTexture, BiLinear, IN.UV).xyz,
        Ks);

    gbuffer.MRIA        = float4(Metallic, Roughness, IOR, Anisotropic) * SampleVirtualTexture(MRIATexture, BiLinear, IN.UV);
    gbuffer.Depth       = length(IN.WPOS - CameraPOS.xyz) / MaxZ;

    return gbuffer;
}

float4 Forward_PS(Forward_PS_IN IN) : SV_TARGET
{
    // surface parameters
    const float3 N			    = normalize(IN.Normal);
    const float3 T			    = normalize(cross(N, N.zxy));
    const float3 B			    = normalize(cross(N, T));

    const float2 UV				= IN.UV;
    const float3x3 inverseTBN	= float3x3(T, B, N);
    const float3x3 TBN	        = transpose(TBN);

    float3 positionW	        = IN.WPOS;
    float3 worldV   	        = normalize(CameraPOS - positionW);

    const uint2	lightBin	    = IN.POS / 10.0f;// - uint2( 1, 1 );
    uint        localLightCount = 0;

    float3 Color = float3(0, 0, 0);
    for (int i = 0; i < lightCount; i++)
    {
        if(!isLightContributing(i, lightBin))
            continue;

        PointLight light	= ReadPointLight(i);
        localLightCount++;

        const float3 Lc			= light.KI.rgb;
        const float  Ld			= length(positionW - light.PR.xyz);
        const float  Li			= light.KI.w;
        const float  Lr			= light.PR.w;
        const float  ld_2		= (Ld * Ld);
        const float  La			= (Li / ld_2) * saturate(1 - (pow(Ld, 10) / pow(Lr, 10)));

		// Compute tangent space vectors
		const float3 worldL = normalize(light.PR.xyz - positionW);
		const float3 L = mul(inverseTBN, worldL);
		const float3 V = mul(inverseTBN, worldV);

		const float3 diffuse    = HammonEarlGGX(V, L, Albedo, .01);
		const float3 specular   = EricHeitz2018GGX(V, L, Albedo, Metallic, .01, Anisotropic, IOR);

		Color += saturate(diffuse * (1 - Ks) + specular * Ks) * La;
    }

    //return pow(float4(1, 0, 0, 0) * localLightCount / lightCount, 1.0f / 2.1f); // Light Tiles
    //return pow(float4(Color * (localLightCount / lightCount) * (localLightCount / lightCount), 1), 1.0f / 2.1f); // Light tiles overlayed on render
    //return localLightCount == 0 ? float4(1, 0, 1, 1) : float4(0, 1, 1, 1);
    //return float4(pow(pi * Color, 1.0f / 2.1f), 1);
    //return float4(n * 0.5 + float3(0.5, 0.5f, 0.5f), 1);									        // Normal debug vis
    //return float4(pow(tangent * 0.5 + float3(0.5, 0.5f, 0.5f), 0.45f), 1);	
    //return float4(UV, 0, 1);	
    //return float4(pow(Color, 1/2.1), 1);
	return float4(Color, 1);
	//return float4(Color.xy,pow(localLightCount / lightCount, 1), 1);
	//return float4(Color.xy, pow(localLightCount / lightCount, 1), 1);
}

float4 FlatWhite(Forward_PS_IN IN) : SV_TARGET
{
    return float4(1, 1, 1, 1);
}


/**********************************************************************

Copyright (c) 2015 - 2019 Robert May

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
