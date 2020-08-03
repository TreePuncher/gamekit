#include "common.hlsl"
#include "pbr.hlsl"

struct PointLight
{
    float4 KI;	// Color + intensity in W
    float4 PR;	// XYZ + radius in W
};

cbuffer LocalConstants : register(b1)
{
    float2  WH;
    float   Time;
    uint   	lightCount;

	float4x4 pl_PV[512];
}

Texture2D<float4> AlbedoBuffer      : register(t0);
Texture2D<float4> MRIABuffer        : register(t1); // metallic, roughness, IOR, anisotropic
Texture2D<float4> NormalBuffer      : register(t2);
Texture2D<float4> TangentBuffer     : register(t3);
Texture2D<float>  DepthBuffer       : register(t4);

// light and shadow map resources
StructuredBuffer<uint> 			lightBuckets : register(t6);
StructuredBuffer<PointLight> 	pointLights	 : register(t7);
TextureCube<float> 				shadowMaps[] : register(t8);

sampler BiLinear     : register(s0); // Nearest point
sampler NearestPoint : register(s1); // Nearest point

bool isLightContributing(const uint idx, const uint2 xy)
{
    const uint offset       = (xy.x + xy.y * WH[0] / 10) * (1024 / 32);
    const uint wordOffset   = idx / 32;
    const uint bitMask      = 0x1 << (idx % 32);

    return lightBuckets[wordOffset + offset] & bitMask;
}

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

float4 DeferredShade_PS(Deferred_PS_IN IN, float4 ScreenPOS : SV_POSITION) : SV_Target0
{
    const float2 SampleCoord    = IN.Position;
    const float2 UV             = SampleCoord.xy / WH; // [0;1]
    const float  depth          = DepthBuffer.Sample(NearestPoint, UV);

    if (depth >= 1.0f) discard;

    const float4 Albedo         = AlbedoBuffer.Sample(NearestPoint, UV);
    const float4 N              = NormalBuffer.Sample(NearestPoint, UV);
    const float3 T              = TangentBuffer.Sample(NearestPoint, UV);
    const float3 B              = normalize(cross(N, T));
	const float4 MRIA			= MRIABuffer.Sample(NearestPoint, UV);

    const float3 positionW 	= GetWorldSpacePosition(UV, depth);

    const float3 V          = -GetViewVector_VS(UV);
    const float3 positionVS =  GetViewSpacePosition(UV, depth);

    const float roughness     = MRIA.y;
    const float ior           = MRIA.b;
	const float metallic      = MRIA.r;
	const float3 albedo       = Albedo.rgb;
    
    const float Ks = 0.0f;
    const float Kd = (1.0 - Ks) * (1.0 - metallic);

	const uint2	lightBin	    = UV * WH / 10.0f;// - uint2( 1, 1 );
    uint        localLightCount = 0;


    float4 color = 0;
	[unroll(16)]
    for(float I = 0; I < lightCount; I++)
    {
		if(!isLightContributing(I, lightBin))
			continue;

		localLightCount++;

        const PointLight light  = pointLights[I];

        const float3 Lc			= light.KI.rgb;
        const float3 Lp         = mul(View, float4(light.PR.xyz, 1)).xyz;
        const float3 L		    = normalize(Lp - positionVS);
        
        const float  Ld			= length(positionVS - Lp);
        const float  Li			= light.KI.w * 10;
        const float  Lr			= light.PR.w;
        const float  ld_2		= Ld * Ld;
        const float  La			= (Li / ld_2) * saturate(1 - (pow(Ld, 10) / pow(Lr, 10)));

        const float  NdotV      = dot(N.xyz, -V);
        const float  NdotL      = dot(N.xyz, L);
        const float3 H          = normalize(V + L);
        const float3 diffuse    = F_d(V, H, L, N.xyz, roughness) * albedo;//pow(Albedo.xyz, 1.0f) * INV_PI;
        //const float3 diffuse    = albedo * INV_PI;
        const float3 specular   = F_r(V, H, L, N.xyz, roughness);

        const float3 colorSample = (diffuse * Kd + specular * Ks) * La * NdotL;

		const float3 mapVectors[] = {
			float3(-1,  0,  0), // left
			float3( 1,  0,  0), // right
			float3( 0,  1,  0), // top
			float3( 0, -1,  0), // bottom
			float3( 0,  0,  1), // forward
			float3( 0,  0, -1), // backward
		};

		float4 Colors[] = {
			float4(1, 0, 0, 0), // Left
			float4(0, 1, 0, 0), // Right
			float4(0, 0, 1, 0), // Top
			float4(1, 1, 1, 1), // Bottom
			float4(1, 1, 0, 1), // forward
			float4(1, 0, 1, 1), // backward
		};

		int fieldID = 0;
		float a = -1.0f;
		
		[unroll(6)]
		for(int II = 0; II < 6; II++){
			const float b = dot(mapVectors[II], mul(ViewI, -L));

			if(b > a){
				fieldID = II;
				a = b;
			}
		}

		const float4 lightPosition_DC 	= mul(pl_PV[6 * I + fieldID], float4(positionW.xyz, 1));
		const float3 lightPosition_PS 	= lightPosition_DC / lightPosition_DC.a;

		if( lightPosition_PS.z < 0.0f || lightPosition_PS.z > 1.0f || 
			lightPosition_PS.x > 1.0f || lightPosition_PS.x < -1.0f ||
			lightPosition_PS.y > 1.0f || lightPosition_PS.y < -1.0f )
			continue;

		const float2 shadowMapUV 	  	= float2( lightPosition_PS.x / 2 + 0.5f, 0.5f - lightPosition_PS.y / 2);
		const float shadowSample 		= shadowMaps[I].Sample(BiLinear, mul(ViewI, L) * float3( 1, -1, -1));
		const float depth 				= lightPosition_PS.z;

		const float bias        = clamp(0.00001 * tan(acos(dot(N.xyz, L))), 0.0, 0.01);
		const float visibility  = 1.0f;//saturate(depth < shadowSample + bias ? 1.0f : 0.0f);
		color +=  float4(colorSample * Lc * La * visibility, 0 );
    }

	//return float4(N.xyz / 2 + 0.5f, 1);
	return pow(color, 2.1f);
}


/**********************************************************************

Copyright (c) 2015 - 2020 Robert May

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
