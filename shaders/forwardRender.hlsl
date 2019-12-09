#include "common.hlsl"
#include "pbr.hlsl"

static const float PI			= 3.14159265359f;
static const float PIInverse	= 1 / PI;

struct PointLight
{
    float4 KI;	// Color + intensity in W
    float4 PR;	// XYZ + radius in W
};

cbuffer LocalConstants : register(b1)
{
    float4	 Albedo; // + roughness
    float4	 Specular;
    float4x4 WT;
}

cbuffer ShadingConstants : register(b2)
{
    float lightCount;
}

Texture2D<uint2>			    lightMap	: register(t0);
StructuredBuffer<uint>		    lightLists	: register(t1);
ByteAddressBuffer               pointLights : register(t2);

PointLight ReadPointLight(uint idx)
{
    PointLight pointLight;

    uint4 load1 = pointLights.Load4(idx * 32 + 00);
    uint4 load2 = pointLights.Load4(idx * 32 + 16);

    pointLight.KI   = asfloat(load1);
    pointLight.PR   = asfloat(load2);

    return pointLight;
}

sampler NearestPoint : register(s1); // Nearest point

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

float4 Forward_PS(Forward_PS_IN IN) : SV_TARGET
{
    // surface parameters
    const float  m			= 0;//Specular.w;
    const float  r			= 0.05;//Albedo.w;
    const float3 Kd			= float3(0.5f, 0.5f, 0.5f);Albedo;
    const float3 N			= normalize(IN.Normal);
    const float3 T			= normalize(cross(N, N.zxy));
    const float3 B			= normalize(cross(N, T));

    const float3 Ks				= float3(1, 1, 1);
    const float2 UV				= IN.UV;
    const float3x3 TBN			= float3x3(T, B, N);
    const float3x3 inverseTBN	= transpose(TBN);

    const float3 positionW	= IN.WPOS;
    const float3 L			= normalize(CameraPOS - positionW);

    const uint2	lightMapCord	= IN.POS / 10.0f;// - uint2( 1, 1 );
    const uint2	lightList		= lightMap.Load(int3(lightMapCord, 0));
    const int	lightBegin		= lightList.y;
    const int	localLightCount	= lightList.x;

    float3 Color      = float3(0, 0, 0);
    for(int i = 0; i < localLightCount; i++)
    {
        int lightIdx            = lightLists[lightBegin + i];
        const PointLight light	= ReadPointLight(lightIdx);

        const float3 Lc			= light.KI.rgb;
        const float3 V			= normalize(light.PR.xyz - positionW);
        const float  Ld			= length(positionW - light.PR.xyz);
        const float  Li			= light.KI.w;
        const float  Lr			= light.PR.w;
        const float  ld_2		= Ld * Ld;
        const float  La			= (Li / ld_2) * saturate(1 - (pow(Ld, 10) / pow(Lr, 10)));

		Color += max(dot(L, N), 0.0) * float3(1, 0, 0);

		/*
#if 0
        const float3 Fr = 
            BRDF(
                L,
                Lc,
                V,
                positionW,
                Kd,
                N,
                Ks,
                m,
                r)
            * La * dot(N, V);
#else
        const float3 L2 = normalize(mul(TBN, CameraPOS - positionW));
        const float3 V2 = normalize(mul(TBN, light.PR.xyz - positionW));
        float3 Fr = EricHeitz2018GGX(
            L2, 
            V2, 
            r, 
            0, 
            1) * La
            + saturate(CosTheta(L2)) * La  * dot(N, V);
#endif

        Color += saturate(Fr * Kd);*/
    }

    //return pow(float4(1, 0, 0, 0) * localLightCount / lightCount, 1.0f / 2.1f); // Light Tiles
    //return pow(float4(Color * (localLightCount / lightCount) * (localLightCount / lightCount), 1), 1.0f / 2.1f); // Light tiles overlayed on render
    //return localLightCount == 0 ? float4(1, 0, 1, 1) : float4(0, 1, 1, 1);
    //return float4(pow(pi * Color, 1.0f / 2.1f), 1);
    //return float4(n * 0.5 + float3(0.5, 0.5f, 0.5f), 1);									        // Normal debug vis
    //return float4(pow(tangent * 0.5 + float3(0.5, 0.5f, 0.5f), 0.45f), 1);	
    //return float4(UV, 0, 1);	
    return float4(pow(Color, 1/2.1), 1);
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
