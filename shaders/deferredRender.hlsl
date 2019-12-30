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
    float   lightCount;
}

Texture2D<float4> AlbedoBuffer      : register(t0);
Texture2D<float4> MRIABuffer        : register(t1); // metallic, roughness, IOR, anisotropic
Texture2D<float4> NormalBuffer      : register(t2);
Texture2D<float4> TangentBuffer     : register(t3);
Texture2D<float>  DepthBuffer       : register(t4);
ByteAddressBuffer pointLights       : register(t5);
TextureCube<float4> EnvMap          : register(t6);
TextureCube<float4> IrradianceMap   : register(t7);

sampler BiLinear     : register(s0); // Nearest point
sampler NearestPoint : register(s1); // Nearest point

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
#define ENV_SAMPLE_COUNT 1
#define SEED_SCALE 1000000
/*
float4 SampleSphericalLOD(in TextureCube<float4> tex, in float3 w, in float lod)
{
	float r = length(w);
	float theta = atan2(w.z, w.x);
	float phi = acos(w.y / r);
	
	float U = (theta / (2.0 * PI)) + 0.5;
	float V = phi / PI;

    float2 UV = saturate(float2(U, V));

    return tex.SampleLevel(BiLinear, UV, lod);
}

float4 SampleSpherical(in TextureCube<float4> tex, in float3 w)
{
	float r = length(w);
    float theta = atan2(w.z, w.x);
	float phi = acos(w.y / r);
	
	float U = (theta / (2.0 * PI)) + 0.5;
	float V = phi / PI;

    float2 UV = saturate(float2(U, V));

    uint W;
    uint H;
    tex.GetDimensions(W, H);

    return tex.Sample(BiLinear, UV);
}

float4 SampleSphericalRoughness(in TextureCube<float4> tex, in float3 w, in float alpha)
{
	float W, H, Elements, NumberOfLevels;
	EnvMap.GetDimensions(0, W, H, NumberOfLevels);
	float lod = alpha * NumberOfLevels;//EnvMap.CalculateLevelOfDetail(BiLinear, alpha);
    return SampleSphericalLOD(tex, w, lod);
}


float3 SampleCosWeighted(float U1, float U2)
{
    float r = sqrt(U1);
    float theta = PI * 2.0 * U2;

    float x = r * cos(theta);
    float y = r * sin(theta);

    return float3(x, y, sqrt(max(0.0, 1.0 - U1)));
}
*/
float RandUniform(float seed)
{
    return Hash(seed);
}

float CantorPairing(in float a, in float b)
{
    return 0.5 * (a + b) * (a + b + 1.0) + b;
}

#define SAMPLE_COUNT 32
float4 environment_PS(ENVIRONMENT_PS input) : SV_Target
{
    const float2 SampleCoord    = input.Position;
	const int3 Coord            = int3(SampleCoord.x, SampleCoord.y, 0);
    const float2 UV             = SampleCoord.xy / WH; // [0;1]
    float3 rayDir               = GetViewVector(UV);

    //const float4 Specular       = SpecularBuffer.Load(Coord);
    const float3 N              = normalize(NormalBuffer.Load(Coord).xyz);
    const float3 T              = normalize(cross(N, N.zxy));
    const float3 B              = normalize(cross(N, T));

    const float  depth          = DepthBuffer.Load(Coord);

	//return float4(T, 1.0);

    const float3x3 invView  = View;
    const float3 rayPos     = CameraPOS;

	// Calculate world position
    const float3 positionW = rayPos + rayDir * (depth * MaxZ);

	// Compute tangent space vectors
	const float3 worldV         = -rayDir;
    const float3x3 inverseTBN   = float3x3(T, B, N.xyz);
    const float3x3 TBN          = transpose(inverseTBN);
	const float3 V              = mul(inverseTBN, worldV);

   // float roughness     = 0.4;//cos(Time) * .5 + .5;
    //float ior           = IOR_ANISO.x;
    //float anisotropic   = 0.8;//IOR_ANISO.y;
	//float metallic      = Specular.w;
	//float3 albedo       = Albedo.rgb;

	if (depth < 1.0)
	{
		const float4 AlbedoSpecular = AlbedoBuffer.Load(Coord);
		const float3 albedo         = AlbedoSpecular.xyz;
		const float  kS             = AlbedoSpecular.w;
		const float4 MRIA           = MRIABuffer.Load(Coord);

		const float metallic = MRIA.x;
		const float roughness = MRIA.y;
		const float ior = MRIA.z;
		const float anisotropic = MRIA.w;
		const float kD = (1.0 - kS) * (1.0 - metallic);

		float3 specular = float3(0,0,0);

		uint W, H, NumberOfLevels;
		EnvMap.GetDimensions(0, W, H, NumberOfLevels);
		
		float3 diffuse = IrradianceMap.SampleLevel(BiLinear, N, NumberOfLevels);

		int seed = Hash(SampleCoord.y * W + SampleCoord.x) * 1000000;

		for (int i = 0; i < SAMPLE_COUNT; ++i)
		{
			// Eric Heitz GGX 2018 MIS sampling
			float alpha = roughness * roughness;
			float aspect = sqrt(1.0 - 0.9 * anisotropic);
			float alpha_x = alpha * aspect;
			float alpha_y = alpha / aspect;

			float U1 = RandUniform(seed++);
			float U2 = RandUniform(seed++);

			float3 Ni = EricHeitz2018GGXVNDF(V, alpha_x, alpha_y, U1, U2);
			float3 Li = reflect(-V, Ni);

			float cosT = max(dot(V,Li), 0.0);

			float3 F0 = abs((1.0 - ior) / (1.0 + ior));
			float3 F = SchlickFresnel(cosT, F0);
                
			float G2 = EricHeitz2018GGXG2(V, Li, alpha_x, alpha_y);
			float G1 = EricHeitz2018GGXG1(V, alpha_x, alpha_y);

			float3 brdfVal = (F * G2) / max(G1, 0.01);

			// Importance Sampling combined with Mipmap Filtering
			float pdfVal = EricHeitz2018GGXPDF(V, Ni, Li, alpha_x, alpha_y);
			float distortVal = 1.0;
			float omegaS = 1.0 / (SAMPLE_COUNT * pdfVal);
			float omegaP = distortVal / (W * H);

			int lod = max(0.5 * log2( (W * H) / SAMPLE_COUNT ) - 0.5 * log2(pdfVal * distortVal), 0.0);
			float3 sampleDir = mul(TBN, Li);
			float3 sampleIrradiance = EnvMap.SampleLevel(BiLinear, sampleDir, lod);
		
			specular += sampleIrradiance * brdfVal;
		}

		specular /= SAMPLE_COUNT;

		return float4(kS * specular + kD * diffuse, 1.0);
	}
	else
	{
        return EnvMap.Sample(BiLinear, rayDir);
    }

    /*
	// Calculate world position
    const float3 positionW = rayPos + rayDir * (depth * (MaxZ - MinZ) - MinZ);

	// Compute tangent space vectors
	const float3 worldV         = -rayDir;
    const float3x3 inverseTBN   = float3x3(T, B, N.xyz);
    const float3x3 TBN          = transpose(inverseTBN);
	const float3 V              = mul(inverseTBN, worldV);

    float roughness     = cos(Time) * .5 + .5;
    float ior           = IOR_ANISO.x;
    float anisotropic   = IOR_ANISO.y;
	float metallic      = Specular.w;
	float3 albedo       = Albedo.rgb;


	float3 R = reflect(-worldV, N);
	float3 irradiance = SampleEnvRoughness(R, roughness);
    
    return float4(cos(Time), roughness, roughness, 1.0);
#if 0
	float seed = Hash(SampleCoord.x + SampleCoord.y * WH.x) * SEED_SCALE;//(Hash(SampleCoord.x + SampleCoord.y * WH.x) + Time1 + Time2) * SEED_SCALE; // SampleCoord.x + SampleCoord.y * WH.x;// //Hash(SampleCoord * SEED_SCALE); //Hash(SampleCoord.x + SampleCoord.y * WH.x) * SEED_SCALE;
	float3 diffuseAccum = (0);
	float3 specularAccum = (0);

    [unroll(ENV_SAMPLE_COUNT)]
    for (int itr = 0; itr < ENV_SAMPLE_COUNT; itr++)
    {
		float U1 = RandUniform(seed += 4);
		float U2 = RandUniform(seed +=4 );
		float3 sampleDir = mul(TBN, SampleCosWeighted(U1, U2));
		diffuseAccum += SampleEnv(sampleDir);
		{
			// Eric Heitz GGX 2018 MIS sampling
			float alpha = roughness * roughness;
			float aspect = sqrt(1.0 - 0.9 * anisotropic);
			float alpha_x = alpha * aspect;
			float alpha_y = alpha / aspect;

			float U1 = RandUniform(seed++);
			float U2 = RandUniform(seed++);

			float3 Ni = EricHeitz2018GGXVNDF(V, alpha_x, alpha_y, U1, U2);
			float3 Li = reflect(-V, Ni);
			//vec3 L = SampleUniformHemisphere(U1, U2);

			float dotVL = dot(V, Li);
				
			float cosT = max(dotVL, 0.0);
			float tmpF0 = abs((1.0 - ior) / (1.0 + ior));
			float3 F0 = float3(tmpF0, tmpF0, tmpF0);
			//F0 = lerp(F0 * F0, albedo, metallic);
			float3 F = SchlickFresnel(cosT, F0);
                
			float G2 = EricHeitz2018GGXG2(V, Li, alpha_x, alpha_y);
			float G1 = EricHeitz2018GGXG1(V, alpha_x, alpha_y);

			//vec3 H = normalize(V + L);
			//float G = EricHeitz2018GGXG2(V, L, alpha_x, alpha_y);
			//float D = EricHeitz2018GGXD(H, alpha_x, alpha_y);

			float3 I = (F * G2) / max(G1, 0.01);
			//vec3 I = D * F * G;
			//outColor.rgb += Ni;
            specularAccum += I * SampleEnvRoughness(mul(TBN, Li), alpha);
        }
    }

	float3 diffuse = (diffuseAccum / ENV_SAMPLE_COUNT) * albedo;
	float3 specular = specularAccum / ENV_SAMPLE_COUNT;
    return float4(diffuse + specular, 1.0);
#endif*/
}

float4 DeferredShade_PS(Deferred_PS_IN IN) : SV_Target0
{
	discard;
    const float2 SampleCoord    = IN.Position;
    const float2 UV             = SampleCoord.xy / WH; // [0;1]
    const float2 P              = (2 * SampleCoord - WH) / WH.y;
    
    const float4 Albedo         = AlbedoBuffer.Sample(NearestPoint, UV);
    //const float4 Specular       = SpecularBuffer.Sample(NearestPoint, UV);
    const float4 N              = NormalBuffer.Sample(NearestPoint, UV);
    const float3 T              = normalize(cross(N, N.zxy));
    const float3 B              = normalize(cross(N, T));
    //const float2 IOR_ANISO      = IOR_ANISOBuffer.Sample(NearestPoint, UV);
    const float  depth          = DepthBuffer.Sample(NearestPoint, UV);


    if (depth == 1) discard;
    
	const float3x3 invView = View;
	const float3 rayDir = GetViewVector(UV); //mul(invView, normalize(float3(P, 0.5)));
    const float3 rayPos = CameraPOS;

    const float3 positionW = rayPos + rayDir * (depth * MaxZ);

    float roughness     = cos(Time) * .5 + .5;
    float ior           = 1.0;//IOR_ANISO.x;
    float anisotropic   = 0.0;//IOR_ANISO.y;
	float metallic      = 0.0;//Specular.w;
	float3 albedo       = Albedo.rgb;
    
    float3 color = 0;
    for(float I; I < lightCount; I++)
    {
        PointLight light = ReadPointLight(I);

        const float3 Lc			= light.KI.rgb;
        const float  Ld			= length(positionW - light.PR.xyz);
        const float  Li			= light.KI.w;
        const float  Lr			= light.PR.w;
        const float  ld_2		= Ld * Ld;
        const float  La			= (Li / ld_2) * saturate(1 - (pow(Ld, 10) / pow(Lr, 10)));

        // Compute tangent space vectors
        const float3 worldV = -rayDir;
        const float3 worldL = normalize(light.PR.xyz - positionW);
        const float3x3 inverseTBN = float3x3(T, B, N.xyz);
        const float3 L = mul(inverseTBN, worldL);
        const float3 V = mul(inverseTBN, worldV);

        // Constants for testing
            
        // Gold physical properties
        const float Ks = 1.0;
        const float Kd = (1.0 - Ks) * (1.0 - metallic);

        const float3 diffuse    = HammonEarlGGX(V, L, albedo, roughness);
        const float3 specular   = EricHeitz2018GGX(V, L, albedo, metallic, roughness, anisotropic, ior);

        color += saturate((diffuse * Kd + specular * Ks) * La);
    }
    return float4(color, 1);
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
