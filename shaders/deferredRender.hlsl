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

ByteAddressBuffer pointLights       : register(t7);

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
#define MAX_CUBEMAP_SAMPLES 8
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

float RadicalInverse_VdC(in uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
float2 HammersleySeq(in uint i, in uint N)
{
    return float2(float(i)/float(N), RadicalInverse_VdC(i));
}

float DualParabloidJacobianDeterminant(in float z, in float b)
{
	float b2 = b * b;
	float zi = abs(z) + 1.0;
	float zi2 = zi * zi;
	return 4.0 * b2 * zi2;
}

float PickCubemapLOD(in TextureCube cubemap, in float pdf, in float3 w, in uint N)
{
	uint W, H;
	cubemap.GetDimensions(W, H);
	
	float det = DualParabloidJacobianDeterminant( w.z, 1.2 );

	float omega_s = 1.0 / (N * pdf);
	float omega_p = det / (W * H);

	float level = max(0.5 * log2(omega_s / omega_p), 0.0);

	return level;
}

float3 SchlickFresnelRoughness(in float NoX, in float3 F0, in float roughness)
{
    return F0 + (max(1.0 - roughness, F0) - F0) * pow(1.0 - NoX, 5.0);
}

float4 environment_PS(ENVIRONMENT_PS input) : SV_Target
{
    const float2 SampleCoord    = input.Position;
	const int3 Coord            = int3(SampleCoord.x, SampleCoord.y, 0);
    const float2 UV             = SampleCoord.xy / WH; // [0;1]
    float3 rayDir_VS            = GetViewVector_VS(UV);
    float3 rayDir               = mul(ViewI, rayDir_VS);
    
    //const float4 Specular       = SpecularBuffer.Load(Coord);
    const float3 normal          = normalize(NormalBuffer.Load(Coord).xyz);
    const float3 tangent         = normalize(TangentBuffer.Load(Coord).xyz);
    const float3 bitangent       = normalize(cross(normal, tangent));

    const float  depth           = DepthBuffer.Load(Coord);

    const float3x3 invView  = View;
    const float3 rayPos     = CameraPOS;

	// Calculate world position
    //const float3 position_VS    = rayDir_VS * (depth * MaxZ); // View Space
    //const float3 positionW      = GetWorldSpacePosition(UV, depth);//mul(ViewI, float4(position_VS, 1));//rayPos + rayDir * (depth * MaxZ);

	// Compute tangent space vectors
	const float3 worldV         = -rayDir;
    const float3x3 inverseTBN   = float3x3(tangent, bitangent, normal);
    const float3x3 TBN          = transpose(inverseTBN);
	const float3 V              = mul(inverseTBN, worldV);

   // float roughness     = 0.4;//cos(Time) * .5 + .5;
    //float ior           = IOR_ANISO.x;
    //float anisotropic   = 0.8;//IOR_ANISO.y;
	//float metallic      = Specular.w;
	//float3 albedo       = Albedo.rgb;

	//return float4(normal, 1.0);


/*
	float3 color;
	if (depth < 1.0)
	{
		const float4 AlbedoSpecular = AlbedoBuffer.Load(Coord);
		const float3 albedo         = AlbedoSpecular.xyz;
		const float sF              = 0.3;//AlbedoSpecular.w;
		const float4 MRIA           = MRIABuffer.Load(Coord);

		const float metallic  = MRIA.x;
		const float roughness = MRIA.y;
		const float ior       = 1.5;//MRIA.z;
		const float aniso     = MRIA.w;
		
		float alpha = roughness;
		float aspect = sqrt(1.0 - 0.9 * aniso);
		float alpha_x = alpha * aspect;
		float alpha_y = alpha / aspect;

		float NdotV = max(CosTheta(V), 0.0);

		float3 specular = float3(0,0,0);
		
		float3 diffuse = float3(1, 0, 1);//albedo * diffuseMap.Sample(BiLinear, normal) / PI;

		const uint N = MAX_CUBEMAP_SAMPLES;//max(pow(alpha,0.25) * MAX_CUBEMAP_SAMPLES, 8);

		uint Width, Height, NumberOfLevels;
		//ggxMap.GetDimensions(0, Width, Height, NumberOfLevels);
		//uint pixelIndex = uint(SampleCoord.y * Width + SampleCoord.x);//Hash() * 1000000;
		float kS = 0.0;

		//float NdotL = CosTheta(mul(TBN, normalize(float3(1,1,1))) );

		//return float4(NdotL, NdotL, NdotL, 0.0);
		// Precalculate Fresnel F0

		float3 F0 = (1.0 - ior) / (1.0 + ior);
		F0 = F0 * F0;
		F0 = lerp(F0, albedo, metallic);

		uint sampleCount = 0;

		for (uint i = 0; i < N; sampleCount = ++i)
		{
			// Eric Heitz GGX 2018 MIS sampling
			float2 Xi = frac(HammersleySeq(i, N) + Hash2(pixelIndex));
			float3 H = EricHeitz2018GGXVNDF(V, alpha_x, alpha_y, Xi.x, Xi.y);
			float3 L = reflect(-V, H);
			float dotVH = dot(V, H);
			float dotNL = CosTheta(L);

			if (dotNL <= 0.0) continue;

			// Fresnel
			float3 F = SchlickFresnel(max(dotVH, 0.0), F0);

			kS += F;

			// Geometry / Visibility
			float G2 = EricHeitz2018GGXG2(V, L, alpha_x, alpha_y);
			float G1 = EricHeitz2018GGXG1(V, alpha_x, alpha_y);

			float3 brdf = G2 / G1;

			// Importance Sampling combined with Mipmap Filtering
			float pdf = EricHeitz2018GGXPDF(V, H, L, alpha_x, alpha_y);
			//float lod = PickCubemapLOD(ggxMap, pdf, L, N);

			//return float4(lod, lod, lod, 0);
			//int lod = (1.0 / PDF) * NumberOfLevels;//max(0.875 * log2( (Width * Height) / N ) - 0.125 * log2(PDF * omegaS), 0.0);
			
			float3 sampleDir = mul(TBN, L);
			//float3 sampleIrradiance = ggxMap.SampleLevel(BiLinear, sampleDir, lod);
			
			//specular += (sampleIrradiance * brdf);
		}
		
		kS = saturate( kS / sampleCount );
		float kD = (1.0 - kS);// * (1.0 - metallic);

		specular /= sampleCount;

		//float3 reflVec = reflect(rayDir, normal);
		//specular = ggxMap.SampleLevel(BiLinear, reflVec, 6).rgb;
		color = kD * diffuse + kS * specular;
	}
	else
	*/
	{
        //color = ggxMap.Sample(BiLinear, rayDir);
    }

    //float gamma = 0.2;
    //color = color / (color + 1.0);
	//color = pow(color, 1.0/gamma);

    return float4(1, 0, 1, 1.0);
	
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

    if (depth >= 1.0f) discard;
    
	const float3x3 invView 	= View;
	const float3 rayDir 	= GetViewVector(UV);
    const float3 rayPos 	= CameraPOS;

    const float3 positionW 	= GetWorldSpacePosition(UV, depth);

    float roughness     = Albedo.a;
    float ior           = 1.0;//IOR_ANISO.x;
    float anisotropic   = 0.0;//IOR_ANISO.y;
	float metallic      = 1.0;//Specular.w;
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
	//return float4(N / 2 + float3(0.5f, 0.5f, 0.5f), 1);
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
