#include "common.hlsl"
#include "pbr.hlsl"

static const float PI			= 3.14159265359f;
static const float PIInverse	= 1 / PI;

struct PointLight
{
	float4 KI;	// Color + intensity in W
	float4 P;	//
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
    pointLight.P    = asfloat(load2);

    return pointLight;
}

sampler NearestPoint : register(s1); // Nearest point

struct Vertex
{
	float3 POS		: POSITION;
	float3 Normal	: NORMAL;
	//float2 UV		: TEXCOORD;

};

struct Forward_VS_OUT
{
	float4 POS 		: SV_POSITION;
	float3 WPOS 	: POSITION;
	float3 Normal	: NORMAL;
};

Forward_VS_OUT Forward_VS(Vertex In)
{
	Forward_VS_OUT Out;
	Out.WPOS	= mul(WT, float4(In.POS, 1));
	Out.POS		= mul(PV, mul(WT, float4(In.POS, 1)));
	Out.Normal  = normalize(mul(WT, float4(In.Normal, 0.0f)));

	return Out;
}

struct Forward_PS_IN
{
	float4	POS 	: SV_POSITION;
	float3	WPOS 	: POSITION;
	float3	Normal	: NORMAL;
};

float4 Forward_PS(Forward_PS_IN IN) : SV_TARGET
{
	float3 positionW	= IN.WPOS;
	float3 vdir			= normalize(CameraPOS - positionW);

	// surface parameters
	const float  m			= Specular.w;
	const float  r			= Albedo.w;
	const float3 Kd			= Albedo;
	const float3 n			= normalize(IN.Normal);
	const float3 Ks			= Specular;//float3(1, 1, 1);


	const uint2	lightMapCord	= IN.POS / 10.0f;// - uint2( 1, 1 );
	const uint2	lightList		= lightMap.Load(int3(lightMapCord, 0));
	const int	lightBegin		= lightList.y;
	const int	localLightCount	= lightList.x;

	float3 Color      = float3(0, 0, 0);
	for(int i = 0; i < localLightCount; i++)
	{
        int lightIdx        = lightLists[lightBegin + i];
		PointLight light	= ReadPointLight(lightIdx);

		float3 Lc			= light.KI.xyz;
		float3 Lv			= normalize(light.P.xyz - positionW);
		float  ld			= length(positionW - light.P.xyz);
		float  Li			= light.KI.w;
		float  Lr			= light.KI.w;
		float  La			= Li / (ld * ld) * saturate(1 - (pow(ld, 10) / pow(Lr, 10)));

		float3 Fr = 
			BRDF(
				Lv,
				Lc,
				vdir,
				positionW,
				Kd,
				n,
				Ks,
				m,
				r);

        Color += saturate(light.KI.xyz * La * Fr * dot(n, Lv) * dot(n, vdir) * saturate(dot(n, vdir)));
    }


    //return pow(float4(1, 0, 0, 0) * localLightCount / lightCount, 1.0f / 2.1f); // Light Tiles
    //return pow(float4(Color * (localLightCount / lightCount), 1), 1.0f / 2.1f); // Light tiles overlayed on render
	//return float4(pow(n * 0.5 + float3(0.1, 0.1f, 0.1f), 0.45f), 1);									        // Normal debug vis
    //return localLightCount == 0 ? float4(1, 0, 1, 1) : float4(0, 1, 1, 1);
    return float4(pow(pi * Color, 1.0f / 2.1f), 1);
}

float4 FlatWhite(Forward_PS_IN IN) : SV_TARGET
{
    return float4(1, 0, 1, 1);
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