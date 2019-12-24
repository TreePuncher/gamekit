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
    float   Time1;
    
}

cbuffer ShadingConstants : register(b2)
{
    PointLight  light;
    float       Time2;
}

Texture2D<float4> AlbedoBuffer      : register(t0);
Texture2D<float4> SpecularBuffer    : register(t1);
Texture2D<float4> NormalBuffer      : register(t2);
Texture2D<float4> TangentBuffer     : register(t3);
Texture2D<float2> IOR_ANISOBuffer   : register(t4);
Texture2D<float>  DepthBuffer       : register(t5);
Texture2D<float4> HDRMap            : register(t6);

sampler BiLinear     : register(s0); // Nearest point
sampler NearestPoint : register(s1); // Nearest point


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

float4 SampleEnvLod(in float3 w, in float level)
{
	float r = length(w);
	float theta = atan2(w.z, w.x);
	float phi = acos(w.y / r);
	
	float U = (theta / (2.0 * PI)) + 0.5;
	float V = phi / PI;

    float2 UV = saturate(float2(U, V));

    return HDRMap.SampleLevel(BiLinear, UV, 0);
}

float4 SampleEnv(in float3 w)
{
	float r = length(w);
    float theta = atan2(w.z, w.x);
	float phi = acos(w.y / r);
	
	float U = (theta / (2.0 * PI)) + 0.5;
	float V = phi / PI;

    float2 UV = saturate(float2(U, V));

    uint W;
    uint H;
    HDRMap.GetDimensions(W, H);

    return HDRMap.Sample(BiLinear, UV);
}

float4 SampleEnvRoughness(in float3 w, in float alpha)
{
	float r = length(w);
    float theta = atan2(w.z, w.x);
	float phi = acos(w.y / r);
	
	float U = (theta / (2.0 * PI)) + 0.5;
	float V = phi / PI;

    float2 UV = saturate(float2(U, V));
	return HDRMap.SampleBias(BiLinear, UV, alpha);
}


#define ENV_SAMPLE_COUNT 0
#define SEED_SCALE 1000000

float3 SampleCosWeighted(float U1, float U2)
{
    float r = sqrt(U1);
    float theta = PI * 2.0 * U2;

    float x = r * cos(theta);
    float y = r * sin(theta);

    return float3(x, y, sqrt(max(0.0, 1.0 - U1)));
}

float RandUniform(float seed)
{
    return Hash(seed);
}

float CantorPairing(in float a, in float b)
{
    return 0.5 * (a + b) * (a + b + 1.0) + b;
}

float3 EricHeitz2018GGXVNDF(in float3 Ve, in float alpha_x, in float alpha_y, in float U1, in float U2)
{
	// Section 3.2: transforming the view direction to the hemisphere configuration
    float3 Vh = normalize(float3(alpha_x * Ve.x, alpha_y * Ve.y, Ve.z));
	// Section 4.1: orthonormal basis (with special case if cross product is zero)
    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    float3 T1 = lensq > 0.0 ? float3(-Vh.y, Vh.x, 0.0) * rsqrt(lensq) : float3(1.0, 0.0, 0.0);
    float3 T2 = cross(Vh, T1);
	// Section 4.2: parameterization of the projected area
    float r = sqrt(U1);
    float phi = PI * 2.0 * U2;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5 * (1.0 + Vh.z);
    t2 = (1.0 - s) * sqrt(1.0 - t1 * t1) + s * t2;
	// Section 4.3: reprojection onto hemisphere
    float3 Nh = T1 * t1 + T2 * t2 + sqrt(max(0.0, 1.0 - t1 * t1 - t2 * t2)) * Vh;
	// Section 3.4: transforming the normal back to the ellipsoid configuration
    float3 Ne = normalize(float3(alpha_x * Nh.x, alpha_y * Nh.y, max(0.0, Nh.z)));
    return Ne;
}


float4 environment_PS(ENVIRONMENT_PS input) : SV_Target
{
    const float2 SampleCoord    = input.Position;
    const float2 UV             = SampleCoord.xy / WH; // [0;1]
    float3 rayDir               = GetViewVector(UV);

    float3 environment          = SampleEnv(rayDir);

    const float4 Albedo         = AlbedoBuffer.Sample(NearestPoint, UV);
    const float4 Specular       = SpecularBuffer.Sample(NearestPoint, UV);
    const float3 N              = normalize(NormalBuffer.Sample(NearestPoint, UV).xyz);
    const float3 T              = normalize(cross(N, N.zxy));
    const float3 B              = normalize(cross(N, T));
    const float2 IOR_ANISO      = IOR_ANISOBuffer.Sample(NearestPoint, UV);
    const float  depth          = DepthBuffer.Sample(NearestPoint, UV);

    const float3x3 invView  = View;
    const float3 rayPos     = CameraPOS;

    if (depth == 1)
        return SampleEnv(rayDir);
    
	// Calculate world position
    const float3 positionW = rayPos + rayDir * (depth * (MaxZ - MinZ) - MinZ);

    return float4(positionW, 0);
    
	// Compute tangent space vectors
	const float3 worldV         = -rayDir;
	const float3 worldL         = normalize(light.PR.xyz - positionW);
    const float3x3 inverseTBN   = float3x3(T, B, N.xyz);
    const float3x3 TBN          = transpose(inverseTBN);
	const float3 L              = mul(inverseTBN, worldL);
	const float3 V              = mul(inverseTBN, worldV);

    float seed = Hash(SampleCoord.x + SampleCoord.y * WH.x) * SEED_SCALE;//(Hash(SampleCoord.x + SampleCoord.y * WH.x) + Time1 + Time2) * SEED_SCALE; // SampleCoord.x + SampleCoord.y * WH.x;// //Hash(SampleCoord * SEED_SCALE); //Hash(SampleCoord.x + SampleCoord.y * WH.x) * SEED_SCALE;
	float3 diffuseAccum = (0);
	float3 specularAccum = (0);

    float roughness     = Albedo.w;
    float ior           = IOR_ANISO.x;
    float anisotropic   = IOR_ANISO.y;
	float metallic      = Specular.w;
	float3 albedo       = Albedo.rgb;

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

    
}

float4 DeferredShade_PS(Deferred_PS_IN IN) : SV_Target0
{
    const float2 SampleCoord    = IN.Position;
    const float2 UV             = SampleCoord.xy / WH; // [0;1]
    const float2 P              = (2 * SampleCoord - WH) / WH.y;
    
#if 0
    const float4 Albedo         = AlbedoBuffer.Sample(NearestPoint, UV);
    const float4 Specular       = SpecularBuffer.Sample(NearestPoint, UV);
    const float3 N              = normalize(NormalBuffer.Sample(NearestPoint, UV).xyz);
    const float3 T              = normalize(cross(N, N.zxy));
    const float3 B              = normalize(cross(N, T));
    const float2 IOR_ANISO      = IOR_ANISOBuffer.Sample(NearestPoint, UV);
    const float  depth          = DepthBuffer.Sample(NearestPoint, UV);

    const float3x3 invView = View;
	const float3 rayDir = GetViewVector(UV); //mul(invView, normalize(float3(P, 0.5)));
    const float3 rayPos = CameraPOS;

#else
    const float4 Albedo         = AlbedoBuffer.Sample(NearestPoint, UV);
    const float4 Specular       = SpecularBuffer.Sample(NearestPoint, UV);
    const float4 N              = NormalBuffer.Sample(NearestPoint, UV);
    const float3 T              = normalize(cross(N, N.zxy));
    const float3 B              = normalize(cross(N, T));
    const float2 IOR_ANISO      = IOR_ANISOBuffer.Sample(NearestPoint, UV);
    const float  depth          = DepthBuffer.Sample(NearestPoint, UV);
	
	const float3x3 invView = View;
	const float3 rayDir = GetViewVector(UV); //mul(invView, normalize(float3(P, 0.5)));
    const float3 rayPos = CameraPOS;

    const float3 positionW = rayPos + rayDir * (depth * (MaxZ - MinZ) - MinZ);

    const float3 Lc			= light.KI.rgb;
    const float  Ld			= length(positionW - light.PR.xyz);
    const float  Li			= light.KI.w * 100;
    const float  Lr			= light.PR.w;
    const float  ld_2		= Ld * Ld;
    const float  La			= (Li / ld_2);// * saturate(1 - (pow(Ld, 10) / pow(Lr, 10)));

	// Compute tangent space vectors
	const float3 worldV = -rayDir;
	const float3 worldL = normalize(light.PR.xyz - positionW);
    const float3x3 inverseTBN = float3x3(T, B, N.xyz);
	const float3 L = mul(inverseTBN, worldL);
	const float3 V = mul(inverseTBN, worldV);

	// Constants for testing
		
	// Gold physical properties
#if 1
	const float3 albedo = float3(1.0, 0.765, 0.336);
	const float metallic = 1.0;
    const float roughness = Albedo.w;// cos(globalTime * 0.25f * PI) * .5 + .5;// fmod(globalTime * 0.25, 1.0);
	const float anisotropic = 0.0;
	const float ior = 0.475;
#else
	const float3 albedo = float3(1, 1.0f, 1);
	const float metallic = 1.0;
    const float roughness = cos(globalTime * 0.25f * PI) * .5 + .5;// fmod(globalTime * 0.25, 1.0);
	const float anisotropic = 1.0;
	const float ior = 1.0;
#endif
	const float Ks = 1.0;
	const float Kd = (1.0 - Ks) * (1.0 - metallic);

	const float3 diffuse    = HammonEarlGGX(V, L, albedo, roughness);
	const float3 specular   = EricHeitz2018GGX(V, L, albedo, metallic, roughness, anisotropic, ior);

	const float r = distance(light.PR.xyz, positionW);
	const float K = Li / (4.0 * PI * r * r); // Inverse Square law


    return float4(saturate((diffuse * Kd + specular * Ks) * La), 1);
#endif
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
