#include "common.hlsl"
#include "pbr.hlsl"


struct PointLight
{
    float4 KI;	// Color + intensity in W
    float4 PR;	// XYZ + radius in W
};


struct Cluster
{
    float4 Min;	// Min + Offset
    float4 Max;	// Max + Count
};


cbuffer LocalConstants : register(b1)
{
    float2  WH;
    float2  WH_I;
    float   Time;
    uint   	lightCount;
    float4  ambientLight;

	float4x4 pl_PV[1022];
}

#define ARRAY 1

Texture2D<float4> AlbedoBuffer      : register(t0);
Texture2D<float4> MRIABuffer        : register(t1); // metallic, roughness, IOR, anisotropic
Texture2D<float4> NormalBuffer      : register(t2);
Texture2D<float>  DepthBuffer       : register(t4);

// light and shadow map resources
Texture2D<uint> 			    lightMap        : register(t5);
StructuredBuffer<uint2> 	    lightLists	    : register(t6);
StructuredBuffer<uint> 		    lightListBuffer : register(t7);
StructuredBuffer<PointLight> 	pointLights	    : register(t8);
Texture2DArray<float> 			shadowMaps[]    : register(t10);


sampler BiLinear     : register(s0);
sampler NearestPoint : register(s1); 


struct Vertex
{
    float3 POS		: POSITION;
    float3 Normal	: NORMAL;
    float3 Tangent	: Tangent;
    float2 UV		: TEXCOORD;

};

struct Deferred_PS_IN
{
    float4 Position   : SV_Position;
    float2 PixelCoord : PIXELCOORD;
    float2 UV         : UVCOORD;
};

Deferred_PS_IN ShadingPass_VS(float4 position : POSITION)
{
    Deferred_PS_IN OUT;
    OUT.Position        = position;
    OUT.PixelCoord      = ((float2(position.xy) + float2(1, 1)) / 2) * WH;
    OUT.PixelCoord.y    = WH.y - OUT.PixelCoord.y - 1;
    
    return OUT;
}

struct ENVIRONMENT_PS
{
    float4 Position : SV_POSITION;
};

ENVIRONMENT_PS passthrough_VS(float4 position : POSITION)
{
    ENVIRONMENT_PS OUT;
    OUT.Position = position;
    return OUT;
}

#define MAX_ENV_LOD 2.0
#define MAX_CUBEMAP_SAMPLES 8
#define SEED_SCALE 1000000


float RandUniform(float seed)
{
    return Hash(seed);
}


float4 environment_PS(ENVIRONMENT_PS input) : SV_Target
{
	return float4(1, 0, 1, 1.0);
}


uint GetSliceIdx(float z)
{            
    const float numSlices   = 24;                                      
    const float MinOverMax  = MaxZ / MinZ;
    const float LogMoM      = log(MinOverMax);

    return (log(z) * numSlices / LogMoM) - (numSlices * log(MinZ) / LogMoM);
}

float rand_1_05(in float2 uv)
{
    float2 noise = (frac(sin(dot(uv, float2(12.9898, 78.233) * 2.0)) * 43758.5453));
    return abs(noise.x + noise.y) * 0.5;
}

float RandFloatInRange(const float lowerBound, const float upperBound, float2 r)
{
    return lowerBound + (rand_1_05(r) % (upperBound - lowerBound));
}

float RandUniform(float2 r)
{
    return RandFloatInRange(0, 1.0f, r);
}

float3 SampleUniformHemisphere(float2 seed)
{
    const float u1    = RandFloatInRange(0.0f, 1.0f, seed + float2(1, 2));
    const float u2    = RandFloatInRange(0.0f, 1.0f, seed + float2(3, 4));
    const float r     = sqrt(1.0f - u1 * u1);
    const float phi   = 2.0f * PI * u2;

    return float3(cos(phi) * r, sin(phi) * r, u1);
}

float3 CosineSampleHemisphere(float2 seed)
{
    const float u1 = RandUniform(seed + float2(1.0f, 2.0f)), u2 = RandUniform(seed + float2(3.0f, 4.0f));

    const float r = sqrt(u1);
    const float theta = u2 * 2.0f * PI;

    const float x = r * cos(theta);
    const float y = r * sin(theta);

    return float3( x, y, sqrt(1.0f - u1) );
}

float3 SampleFocusedRay(float theta, float2 seed)
{
    float x0 = sin(PI / 2.0f - theta);
    float x1 = 1.0f;
    float u1 = RandUniform(seed) * (x1 - x0) + x0;

    float u2 = RandUniform(seed + 0.1f);

    float r = sqrt(1.0f - u1 * u1);
    float phi = 2.0f * PI * u2;
    return (cos(phi) * r, sin(phi) * r, u1 );
}

float4 DeferredShade_PS(Deferred_PS_IN IN) : SV_Target0
{
    const float2 SampleCoord    = IN.Position;
    const uint2  px             = IN.Position.xy;
    const float  depth          = DepthBuffer.Load(uint3(px.xy, 0));

    if (depth == 1.0f) 
        discard;

    const uint lightListKey     = lightMap.Load(uint3(px.xy, 0));
    const uint2 lightList       = lightLists[lightListKey];
    const uint localLightCount  = lightList.x;
    const uint localLightList   = lightList.y;

    const float4 Albedo         = AlbedoBuffer.Load(uint3(px.xy, 0));

    if (lightListKey == -1)
        return pow(float4(ambientLight * Albedo.xyz, 1), 2.2f);

    const float4 N              = NormalBuffer.Load(uint3(px.xy, 0));
	const float4 MRIA			= MRIABuffer.Load(uint3(px.xy, 0));
    const float4 N_WT           = mul(ViewI, N.xyz);

    const float2 UV             = SampleCoord * WH_I; // [0;1]
    const float3 V              = -GetViewVector_VS(UV);
    const float3 positionVS     = GetViewSpacePosition(UV, depth);
    const float3 positionWS     = GetWorldSpacePosition(UV, depth);

    const float ior           = MRIA.b;
    const float metallic      = MRIA.r > 0.1f ? 1.0f : 0.0f;
	const float3 albedo       = pow(Albedo.rgb, 2);
    const float roughness     = 0.9;//MRIA.g;

    const float Ks              = lerp(0, 0.4f, saturate(Albedo.w));
    const float Kd              = (1.0 - Ks) * (1.0 - metallic);
    const float NdotV           = saturate(dot(N.xyz, V));

    float4 color = float4(ambientLight * albedo, 1);
    for(uint I = 0; I < localLightCount; I++)
    {
        const uint pointLightIdx    = lightListBuffer[localLightList + I];
        const PointLight light      = pointLights[pointLightIdx];

        const float3 Lc			= light.KI.rgb;
        const float3 Lp         = mul(View, float4(light.PR.xyz, 1));
        const float3 L		    = normalize(Lp - positionVS);
        const float  Ld			= length(positionVS - Lp);
        const float  Li			= light.KI.w;
        const float  Lr			= light.PR.w;
        const float  ld_2		= Ld * Ld;
        const float  La			= (Li / ld_2) * (1 - (pow(Ld, 10) / pow(Lr, 10)));

        const float  NdotL      = saturate(dot(N.xyz, L));
        const float3 H          = normalize(V + L);
         
        #if 1
            const float3 diffuse    = saturate(albedo * F_d(V, H, L, N.xyz, roughness));
        #else
            const float3 diffuse    = NdotL * albedo * INV_PI;
        #endif

        #if 1
            const float3 specular = F_r_said(V, H, L, N.xyz, albedo, roughness, metallic);
        #else
            const float3 specular = F_r(V, H, L, N.xyz, roughness);
        #endif

        #if 1// skip shadowmaping
            const float3 colorSample = (diffuse * Kd + specular * Ks) * La * abs(NdotL) * INV_PI;
            color += max(float4(colorSample, 0 ), 0.0f);
        #else
            const float3 colorSample = (diffuse * Kd + specular * Ks) * La * abs(NdotL) * INV_PI;

            const float3 mapVectors[] = {
                float3(-1,  0,  0), // left
                float3( 1,  0,  0), // right
                float3( 0,  1,  0), // top
                float3( 0, -1,  0), // bottom
                float3( 0,  0,  1), // forward
                float3( 0,  0, -1), // backward
            };

            float3 temp = dot(L, float3(1, 0, 0)) < 0.9f ?
                cross(L, float3(0, 1, 0)) : cross(L, float3(1, 0, 0));

            const float3 R  = cross(L, temp);
            const float3 Up = cross(L, R);

            const float3x3 m = transpose(float3x3(R, Up, L));

            const float2 sampleVectors[] =
            {
                float2( 0.0f, 0.0f),

                float2( 0, 1),
                float2( 0,-1),
                float2( 1, 0),
                float2(-1, 0),

                float2( 1, 1),
                float2(-1,-1),
                float2(-1, 1),
                float2( 1,-1),
                float2( 1,-1),
                float2(-1, 1),
            };

            const float3 sampleVector = normalize(mul(m, float3(0, 0, 1)));

            int fieldID = 0;
            float a = -1.0f;

            for (int II = 0; II < 6; II++) {
                const float b = dot(mapVectors[II], mul(ViewI, -sampleVector));

                if (b > a)
                {
                    fieldID = II;
                    a = b;
                }
            }

            const float4 lightPosition_DC 	= mul(pl_PV[6 * pointLightIdx + fieldID], float4(positionWS.xyz, 1));
            const float3 lightPosition_PS 	= lightPosition_DC / lightPosition_DC.a;

            const float3 L_WS               = mul(ViewI, sampleVector) * float3(1, -1, -1);
            const float3 L_Offset           = L_WS;

            const float2 shadowMapUV        = float2(0.5f + lightPosition_PS.x / 2.0f, 0.5f - lightPosition_PS.y / 2.0f);
            const float shadowSample        = shadowMaps[pointLightIdx].Sample(NearestPoint, float3(shadowMapUV, fieldID));
            const float depth               = lightPosition_PS.z;

            const float minBias             = 0.0000015f;
            const float maxBias             = 0.00000750f;
            const float bias                = maxBias * saturate(tan(acos(dot(N_WT, L_WS)))) + minBias;

            const float visibility = saturate(depth < (shadowSample + bias) ? 1.0f : 0.0f);

            const float4 Colors[] = {
                float4(1, 0, 0, 0),
                float4(0, 1, 0, 0),
                float4(0, 0, 1, 0),
                float4(1, 1, 0, 1),
                float4(0, 1, 1, 1),
                float4(1, 0, 1, 1),
            };

            color += float4(saturate(colorSample * Lc * visibility), 0);
        #endif
    }

#if 1
    float4 Colors[] = {
        float4(0, 0, 0, 0), 
        float4(1, 0, 0, 0), 
        float4(0, 1, 0, 0), 
        float4(0, 0, 1, 0), 
        float4(1, 1, 0, 1), 
        float4(0, 1, 1, 1), 
        float4(1, 0, 1, 1), 
        float4(1, 1, 1, 1), 
    };

    const uint clusterKey = GetSliceIdx(depth * MaxZ);

    //if (px.x % (1920 / 38) == 0 || px.y % (1080 / 18) == 0)
    //    return color * color;
    //else
    //
    //if (px.x > WH.x / 2)
        //return pow(float4(0, UV.y, 0, 1), 2);
    
        //return float4(positionWS * float3(0, 0, -0.01), 1);
        //return float4(N);
        //return float4(positionWS, 1);
        //return pow(depth * 100, 1.0f);
        // 
        //return pow(Colors[localLightCount % 8], 1.0f);
        //return pow(Colors[lightListKey % 8] * color, 1.0f);
        //return (float(localLightCount) / float(lightCount));
        // 
        //return pow(Colors[clusterKey % 8], 1.0f);
        //return pow(Colors[GetSliceIdx(-positionVS.z) % 8], 1.0f);
        //
        //return pow(-positionVS.z / 128, 10.0f);
        //return depth;
        //return float4(N / 2.0f + 0.5f);
    return Albedo * Albedo;
    //return float4(positionW, 0);
    //return pow(roughness, 2.2f);
    //return pow(MRIA, 2.2f);
    //return pow(float4(albedo, 1), 2.2f);
    //return float4(T.xyz, 1);
    //return float4(N.xyz, 1);
    //return pow(float4(roughness, metallic, 0, 0), 2.2f);
    //return float4(N.xyz / 2 + 0.5f, 1);
    //return float4(T.xyz / 2 + 0.5f, 1);
    //return float4(Albedo.xyz, 1);

    //return pow(UV.y, 1.0f); 
    //return pow(1 - UV.y, 2.2f); 
#endif
    
	return pow(color, 2.2f);
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
