#include "common.hlsl"
#include "pbr.hlsl"

// 0 - FROSTBYTE
// 1 - BURLEY
// 2 - DISABALED
#define DIFFUSETECHNIQUE 0

// 0 - FROSTBYTE
// 1 - SAIDs
// 2 - DISABALED
#define SPECULARTECHNIQUE 0



// 0 - off
// 1 - Cubemap PCF
// 2 - PCF
#define SHADOWTECHNIQUE 1


struct PointLight
{
	float4 KI;	// Color + intensity in W
	float4 PR;	// XYZ + radius in W
	float4 DS;	// Direction + Spread + Type
	uint4  TypeExtra;
};

struct ShadowMap
{
	float4x4 PV;
	float4x4 V;
};

struct Cluster
{
	float4 Min;	// Min + Offset
	float4 Max;	// Max + Count
};


cbuffer LocalConstants : register(b1)
{
	float2	WH;
	float2	WH_I;
	float	Time;
	uint	lightCount;
	float4	ambientLight;
}

Texture2D<float4> AlbedoBuffer	: register(t0);
Texture2D<float4> MRIABuffer	: register(t1); // metallic, roughness, IOR, anisotropic
Texture2D<float2> NormalBuffer	: register(t2);
Texture2D<float>  DepthBuffer	: register(t4);

// light and shadow map resources
Texture2D<uint> 				lightMap		: register(t5);
StructuredBuffer<uint2> 		lightLists		: register(t6);
StructuredBuffer<uint> 			lightListBuffer	: register(t7);
StructuredBuffer<PointLight> 	pointLights		: register(t8);

StructuredBuffer<float4x4> 		shadowMatrices	: register(t9);
TextureCube<float> 				shadowCubes[]	: register(t10, space0);
Texture2DArray<float2> 			shadowMaps[]	: register(t10, space1);

SamplerState BiLinear     : register(s0);
SamplerState NearestPoint : register(s1);
SamplerComparisonState ShadowSampler : register(s2);


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
};

Deferred_PS_IN ShadingPass_VS(float4 position : POSITION)
{
	Deferred_PS_IN OUT;
	OUT.Position = position;

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
#define PI 3.14159265359f

float RandUniform(float seed)
{
	return Hash(seed);
}

float4 environment_PS(ENVIRONMENT_PS input) : SV_Target {
	return float4(1, 0, 1, 1.0);
} 

uint GetSliceIdx(float z)
{
#if 1
	const float numSlices   = 24;                                      
	const float MinOverMax  = MaxZ / MinZ;
	const float LogMoM      = log(MinOverMax);

	return (log(z) * numSlices / LogMoM) - (numSlices * log(MinZ) / LogMoM);
#else
	return z / MaxZ * 24;
#endif
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

float3 AxisAngle(float3 e, float theta, float3 v)
{
	return (cos(theta) * v + sin(theta) * (cross(e, v)) + (1.0f - cos(theta)) * dot(v, e) * e) * theta;
}

bool IsNaN(float x)
{
	return (asuint(x) & 0x7fffffff) > 0x7f800000;
}

float3 VogelDiskSample3D(int sampleIndex, int samplesCount, float phi, float3 e, float f)
{
	if (samplesCount == 1)
		return e;

	float GoldenAngle = 2.4f;

	float r = sqrt(sampleIndex + 0.5f) / sqrt(samplesCount);
	float theta = sampleIndex * GoldenAngle + phi;

	float3 v = normalize(cross(float3(0, 1, 0), e)) * r;
	if (IsNaN(v.x) || IsNaN(v.y) || IsNaN(v.z))
		v = float3(1, 0, 0);

	return normalize(e + AxisAngle(e, theta, v) * f);
}

float2 VogelDiskSample2D(int sampleIndex, int samplesCount, float phi)
{
	if (samplesCount == 1)
		return float2(0.0f, 0.0f);

	float GoldenAngle = 2.4f;

	float r		= sqrt(sampleIndex + 0.5f) / sqrt(samplesCount);
	float theta	= sampleIndex * GoldenAngle + phi;

	float sine, cosine;
	sincos(theta, sine, cosine);

	return float2(r * cosine, r * sine);
}

float2 VogelDiskSample2D(int sampleIndex, int samplesCount, float phi)
{
	float GoldenAngle = 2.4f;

	float r		= sqrt(sampleIndex + 0.5f) / sqrt(samplesCount);
	float theta	= sampleIndex * GoldenAngle + phi;

	float sine, cosine;
	sincos(theta, sine, cosine);

	return float2(r * cosine, r * sine);
}

float2 ComputeReceiverPlaneDepthBias(float3 texCoordDX, float3 texCoordDY)
{
	float2 biasUV;
	biasUV.x = texCoordDY.y * texCoordDX.z - texCoordDX.y * texCoordDY.z;
	biasUV.y = texCoordDX.x * texCoordDY.z - texCoordDY.x * texCoordDX.z;
	biasUV *= 1.0f / ((texCoordDX.x * texCoordDY.y) - (texCoordDX.y * texCoordDY.x));
	return biasUV;
}

#if SHADOWTECHNIQUE == 2
float SampleShadowMap(in float2 base_uv, in float u, in float v, in float2 shadowMapSizeInv, in uint fieldID, in Texture2DArray<float> shadowMap, in float depth, in float2 receiverPlaneDepthBias)
{
	const float2 uv  = base_uv + float2(u, v) * shadowMapSizeInv;
	return saturate(shadowMap.SampleCmpLevelZero(ShadowSampler, float3(uv, fieldID), float2(depth, 0.0f)));
}
#endif

float GetLuminance(float3 RGB)
{
	return 0.2126f * RGB.x + 0.7152f * RGB.y + 0.0722f * RGB.z;
}

float linstep(float min, float max, float v)
{
	return clamp((v - min) / (max - min), 0.0f, 1);
}

float ReduceLightBleed(float p_max, float amount)
{
	return linstep(amount, 1.0f, p_max);
}

#define g_minVariance 0.0015f

float ChebyshevUpperBound(const float2 moments, const float t)
{
	const float p = step(t, moments.x);

	const float variance = max(moments.y - (moments.x * moments.x), g_minVariance);

	const float d = t - moments.x;
	const float p_max = variance / (variance + d * d);

	return ReduceLightBleed(saturate(max(p, p_max)), 0.01f);
}

float ExponentialShadowSample(const float exponentialDepth, const float t)
{
	return exp(-80.0f * t) * exponentialDepth;
}

float InterleavedGradientNoise(float2 position_screen)
{
	float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
	return frac(magic.z * frac(dot(position_screen, magic.xy)));
}

float2 VectorToSphere(float3 XYZ)
{
	const float theta	= atan(XYZ.y / XYZ.x);
	const float phi		= acos(XYZ.z);

	return float2( theta < 0.0f ? PI + theta : theta, phi );
}

float3 SphereToVector(float2 UV)
{
	float cosTheta;
	float sinTheta;
	float cosPhi;
	float sinPhi;

	sincos(UV.x, sinTheta, cosTheta);
	sincos(UV.y, sinPhi, cosPhi);

	return float3( sinPhi * cosTheta, sinPhi * sinTheta, cosPhi );
}

float smoothDistanceAtt(float squaredDistance, float invSqrAttRadius)
{
	float factor		= squaredDistance * invSqrAttRadius;
	float smoothFactor	= saturate(1.0f - factor * factor);

	return smoothFactor * smoothFactor;
}

float4 DeferredShade_PS(float4 Position : SV_Position) : SV_Target0
{
	const float2 SampleCoord	= Position;
	const uint2  px				= Position.xy;
	const float  depth			= DepthBuffer.Load(uint3(px.xy, 0));

	if (depth == 1.0f) 
		discard;

	const uint lightListKey		= lightMap.Load(uint3(px.xy, 0));
	const uint2 lightList		= lightLists[lightListKey];
	const uint localLightCount	= lightList.x;
	const uint localLightList	= lightList.y;
	const float4 Albedo			= AlbedoBuffer.Load(uint3(px.xy, 0));

	if (lightListKey == -1)
		return pow(float4(ambientLight * Albedo.xyz, 1), 2.2f);

	const float2 N_xy		= NormalBuffer.Load(uint3(px.xy, 0));
	const float4 MRIA		= MRIABuffer.Load(uint3(px.xy, 0));

	const float3 N			= float3(N_xy, sqrt(saturate(1.0f - dot(N_xy, N_xy))));
	const float3 N_WS		= mul(ViewI, N);

	const float2 UV			= SampleCoord * WH_I; // [0;1]
	const float3 V			= -GetViewVector_VS(UV);
	const float3 positionVS	= GetViewSpacePosition(UV, depth);
	const float3 positionWS = mul(ViewI, float4(positionVS, 1)); //GetWorldSpacePosition(UV, depth);

	const float ior			= MRIA.b;
	const float metallic	= 1 - MRIA.r > 0.1f ? 1.0f : 0.0f; 
	const float3 albedo		= pow(Albedo.rgb, 2.1f);
	const float roughness	= MRIA.g;

	const float Ks			= lerp(0, 0.4f, saturate(1.0f));
	const float Kd			= (1.0 - Ks) * (1.0 - metallic);
	const float NdotV		= saturate(dot(N.xyz, V));

	float4 color = float4(0, 0, 0, 1);
	for (uint I = 0; I < localLightCount; I++)
	{
		const uint lightIdx		= lightListBuffer[localLightList + I];
		const PointLight light	= pointLights[lightIdx];

		const float3 Lc			= light.KI.rgb;
		const float3 Lp			= mul(View, float4(light.PR.xyz, 1));
		const float3 L			= normalize(Lp - positionVS);
		const float  Ld			= length(Lp - positionVS);
		const float  Li			= light.KI.w;
		const float  Lr			= light.PR.w;
		const float  ld_2		= Ld * Ld;
		const float  La			= (Li / ld_2) * (1 - (pow(Ld, 10) / pow(Lr, 10)));

		const float  NdotL		= saturate(dot(N, L));
		const float3 H			= normalize(V + L);
		 
		#if DIFFUSETECHNIQUE == 0
			const float3 diffuse	= NdotL * albedo * INV_PI;
		#elif DIFFUSETECHNIQUE == 1
			const float3 diffuse	= max(albedo * F_d(V, H, L, N.xyz, roughness), 0.0f);
		#elif DIFFUSETECHNIQUE == 2
			const float3 diffuse	= float3(0, 0, 0);
		#endif 

		#if SPECULARTECHNIQUE == 0
			const float3 specular	= F_r(V, H, L, N.xyz, roughness);
		#elif SPECULARTECHNIQUE == 1
			const float3 specular	= F_r_said(V, H, L, N.xyz, albedo, roughness, 0);
		#elif SPECULARTECHNIQUE == 2
			const float3 specular	= float3(0, 0, 0);
		#endif

		switch (light.TypeExtra[0])
		{
		case 0:	// Point Light
		{
					float3	v_WS		= mul(ViewI, -L);
			const	float	depth		= length(Lp - positionVS) / Lr;

			float t = 0;
			static const int sampleCount = 16;
			for (int i = 0; i < sampleCount; i++)
			{
				const float3	sampleVector	= VogelDiskSample3D(i, sampleCount, InterleavedGradientNoise(px), v_WS, 0.00019f);
				const float		expDepth		= shadowCubes[NonUniformResourceIndex(lightIdx)].Sample(BiLinear, sampleVector);

				t += saturate(ExponentialShadowSample(expDepth, depth)) / sampleCount;
			}

			const float3 colorSample = (diffuse * Kd + specular * Ks) * NdotL * La * Lc;
			color += max(float4(colorSample, 0), 0) * t;
			break;
		}
		case 1: // Spot light
		{
			const float	dp					= saturate(dot(L, mul(View, light.DS.xyz)));
			const float lightAngleScale		= asfloat(light.TypeExtra[1]);
			const float lightAngleOffset	= asfloat(light.TypeExtra[2]);

			float a = saturate(dp * lightAngleScale + lightAngleOffset);
			a *= a;

			const float3	v_WS		= mul(ViewI, -L);
			const float4	SC			= mul(shadowMatrices[lightIdx], float4(positionWS, 1));
			const float3	N_DC		= SC.xyz / SC.w;
			const float2	shadowMapUV = float2(0.5f + N_DC.x / 2.0f, 0.5f - N_DC.y / 2.0f);

			if (shadowMapUV.x < 0.0f || shadowMapUV.x > 1.0f |
				shadowMapUV.y < 0.0f || shadowMapUV.y > 1.0f)
				continue;

			float		shadowing		= 0;
			float		gradientNoise	= InterleavedGradientNoise(px);
			const float	depth			= length(positionVS - Lp) / Lr;

			uint sampleCount = 16;
			for (uint i = 0; i < sampleCount; i++)
			{
				const float		penumbraFilterMaxSize	= 0.0025f;
				const float2	sampleUVOffset			= VogelDiskSample2D(i, sampleCount, gradientNoise);
				const float2	sampleUV				= clamp(shadowMapUV + sampleUVOffset * penumbraFilterMaxSize, 0.0f, 1.0f);

				const float	expDepth = shadowMaps[NonUniformResourceIndex(lightIdx)].Sample(BiLinear, float3(sampleUV, 0.0f));

				shadowing += saturate(ExponentialShadowSample(expDepth, depth)) / sampleCount;
			}

			const float3	colorSample = (diffuse * Kd + specular * Ks) * NdotL * INV_PI * La * a * dp;
			color += float4(max(colorSample, 0), 0) * shadowing;
		}	break;
		case 2:	// DirectionalLight
		{
			const float3	colorSample = (diffuse * Kd + specular * Ks) * NdotL * INV_PI;
			const float		shadowDepth = shadowMaps[NonUniformResourceIndex(lightIdx)].Gather(BiLinear, float3( 0.5, 0.5, 0.0f ));
			
			color += float4(max(colorSample, 0), 0) * shadowDepth;
		}	break;
		case 3: // PointLight no shadows
		{
			const float3 colorSample = (diffuse * Kd + specular * Ks) * NdotL * La * INV_PI;
			color += float4(max(colorSample, 0), 0);
		}	break;
		}

		#if SHADOWTECHNIQUE == 0 // Skip

		#elif SHADOWTECHNIQUE == 1 // CubeMap PCF

		#elif SHADOWTECHNIQUE == 2 // PCF

			const float3 colorSample = (diffuse * Kd + specular * Ks) * La * abs(NdotL) * INV_PI * Lc;

			const float3 mapVectors[] = {
				float3( 1,  0,  0), // right
				float3(-1,  0,  0), // left
				float3( 0,  1,  0), // top
				float3( 0, -1,  0), // bottom
				float3( 0,  0,  1), // forward
				float3( 0,  0, -1), // backward
			};

			int fieldID = 0;
			float a = -1.0f;
			const float3 L_WS = mul(ViewI, -L);

			for (int II = 0; II < 6; II++)
			{
				const float b = dot(mapVectors[II], L_WS);

				if (b > a)
				{
					fieldID = II;
					a = b;
				}
			}

			const float4 lightPosition_DC = mul(pl[6 * pointLightIdx + fieldID].PV, float4(positionWS.xyz, 1));
			const float4 lightPosition_VS = mul(pl[6 * pointLightIdx + fieldID].V, float4(positionWS.xyz, 1));
			const float3 lightPosition_PS = lightPosition_DC / lightPosition_DC.a;
			
			const float lightDepth	= -lightPosition_VS.z / Lr;

			const float2 momentSample = shadowMaps[pointLightIdx].Sample(BiLinear, L_WS);
			color += max(float4(colorSample * Lc * step(momentSample.x, lightDepth), 0), 0);


			for (int II = 0; II < 6; II++)
			{
				const float b = dot(mapVectors[II], L_WS);

				if (b > a)
				{
					fieldID = II;
					a = b;
				}
			}

			const float4 lightPosition_DC 	= mul(pl[6 * pointLightIdx + fieldID].PV,   float4(positionWS.xyz, 1));
			const float4 lightPosition_VS 	= mul(pl[6 * pointLightIdx + fieldID].V,    float4(positionWS.xyz, 1));
			const float3 lightPosition_PS 	= lightPosition_DC / lightPosition_DC.a;
			const float lightDepth			= -lightPosition_VS.z / Lr;

			const float2 shadowMapUV	= float2(0.5f + lightPosition_PS.x / 2.0f, 0.5f - lightPosition_PS.y / 2.0f);

			float Elements;
			float2 resolution;
			shadowMaps[pointLightIdx].GetDimensions(resolution.x, resolution.y, Elements);
			const float2 resolution_INV	= 1.0f / resolution;
			const float2 uv				= shadowMapUV * resolution;
			float2 base_uv;
			base_uv.x = floor(uv.x + 0.5);
			base_uv.y = floor(uv.y + 0.5);

			float s = (uv.x + 0.5 - base_uv.x);
			float t = (uv.y + 0.5 - base_uv.y);

			base_uv -= 0.5f;
			base_uv *= resolution_INV;

			const float uw0 = (5 * s - 6);
			const float uw1 = (11 * s - 28);
			const float uw2 = -(11 * s + 17);
			const float uw3 = -(5 * s + 1);

			const float u0 = (4 * s - 5) / uw0 - 3;
			const float u1 = (4 * s - 16) / uw1 - 1;
			const float u2 = -(7 * s + 5) / uw2 + 1;
			const float u3 = -s / uw3 + 3;

			const float vw0 = (5 * t - 6);
			const float vw1 = (11 * t - 28);
			const float vw2 = -(11 * t + 17);
			const float vw3 = -(5 * t + 1);

			const float v0 = (4 * t - 5) / vw0 - 3;
			const float v1 = (4 * t - 16) / vw1 - 1;
			const float v2 = -(7 * t + 5) / vw2 + 1;
			const float v3 = -t / vw3 + 3;

			const float3 shadowPosDX = ddx_fine(positionWS);
			const float3 shadowPosDY = ddy_fine(positionWS);
			const float2 receiverPlaneDepthBias = ComputeReceiverPlaneDepthBias(shadowPosDX, shadowPosDY);

			float sum = 0;
			sum += uw0 * vw0 * SampleShadowMap(base_uv, u0, v0, resolution_INV, fieldID, shadowMaps[pointLightIdx], lightDepth, receiverPlaneDepthBias);
			sum += uw1 * vw0 * SampleShadowMap(base_uv, u1, v0, resolution_INV, fieldID, shadowMaps[pointLightIdx], lightDepth, receiverPlaneDepthBias);
			sum += uw2 * vw0 * SampleShadowMap(base_uv, u2, v0, resolution_INV, fieldID, shadowMaps[pointLightIdx], lightDepth, receiverPlaneDepthBias);
			sum += uw3 * vw0 * SampleShadowMap(base_uv, u3, v0, resolution_INV, fieldID, shadowMaps[pointLightIdx], lightDepth, receiverPlaneDepthBias);

			sum += uw0 * vw1 * SampleShadowMap(base_uv, u0, v1, resolution_INV, fieldID, shadowMaps[pointLightIdx], lightDepth, receiverPlaneDepthBias);
			sum += uw1 * vw1 * SampleShadowMap(base_uv, u1, v1, resolution_INV, fieldID, shadowMaps[pointLightIdx], lightDepth, receiverPlaneDepthBias);
			sum += uw2 * vw1 * SampleShadowMap(base_uv, u2, v1, resolution_INV, fieldID, shadowMaps[pointLightIdx], lightDepth, receiverPlaneDepthBias);
			sum += uw3 * vw1 * SampleShadowMap(base_uv, u3, v1, resolution_INV, fieldID, shadowMaps[pointLightIdx], lightDepth, receiverPlaneDepthBias);

			sum += uw0 * vw2 * SampleShadowMap(base_uv, u0, v2, resolution_INV, fieldID, shadowMaps[pointLightIdx], lightDepth, receiverPlaneDepthBias);
			sum += uw1 * vw2 * SampleShadowMap(base_uv, u1, v2, resolution_INV, fieldID, shadowMaps[pointLightIdx], lightDepth, receiverPlaneDepthBias);
			sum += uw2 * vw2 * SampleShadowMap(base_uv, u2, v2, resolution_INV, fieldID, shadowMaps[pointLightIdx], lightDepth, receiverPlaneDepthBias);
			sum += uw3 * vw2 * SampleShadowMap(base_uv, u3, v2, resolution_INV, fieldID, shadowMaps[pointLightIdx], lightDepth, receiverPlaneDepthBias);

			sum += uw0 * vw3 * SampleShadowMap(base_uv, u0, v3, resolution_INV, fieldID, shadowMaps[pointLightIdx], lightDepth, receiverPlaneDepthBias);
			sum += uw1 * vw3 * SampleShadowMap(base_uv, u1, v3, resolution_INV, fieldID, shadowMaps[pointLightIdx], lightDepth, receiverPlaneDepthBias);
			sum += uw2 * vw3 * SampleShadowMap(base_uv, u2, v3, resolution_INV, fieldID, shadowMaps[pointLightIdx], lightDepth, receiverPlaneDepthBias);
			sum += uw3 * vw3 * SampleShadowMap(base_uv, u3, v3, resolution_INV, fieldID, shadowMaps[pointLightIdx], lightDepth, receiverPlaneDepthBias);

			sum *= 1.0f / 2704.0f;

			static float4 Colors[] = {
				float4(1, 0, 0, 0),
				float4(0, 1, 0, 0),
				float4(0, 0, 1, 0),
				float4(1, 1, 0, 1),
				float4(0, 1, 1, 1),
				float4(1, 0, 1, 1),
			};

			//color += max(float4(colorSample * Lc * sum * 1.0f / 2704.0f, 0), 0);
		#endif 
	}

#if 0
	static float4 Colors[] = {
		float4(0.5f, 0.5f, 0.5f, 0), 
		float4(1, 0, 0, 0), 
		float4(0, 1, 0, 0), 
		float4(0, 0, 1, 0), 
		float4(1, 1, 0, 1), 
		float4(0, 1, 1, 1), 
		float4(1, 0, 1, 1), 
		float4(1, 1, 1, 1), 
	};

	const uint clusterKey = GetSliceIdx(depth * MaxZ);

	//if (px.x > WH.x / 2)
		//return pow(float4(0, UV.y, 0, 1), 2);
	
		//return float4(positionWS * float3(0, 0, -0.01), 1);
		//return float4(N);
		//return float4(positionWS, 1);
		//return float4(positionVS, 1);
		//return pow(depth * 100, 1.0f);
		// 
		//return pow(Colors[localLightCount % 8], 2.0f) * color;
		//return pow(Colors[lightListKey % 8] * color, 1.0f);
		//return pow(Colors[clusterKey % 8] * (float(localLightCount) / float(lightCount)), 1.0f);
		//return color * (float(localLightCount) / float(lightCount));
		//return (float(localLightCount) / float(lightCount));
		// 
		//return pow(Colors[clusterKey % 8], 1.0f);
		//return pow(Colors[GetSliceIdx(-positionVS.z) % 8], 1.0f);
		//
		//return pow(-positionVS.z / 128, 10.0f);
		//return depth;
		//return float4(N / 2.0f + 0.5f);
	//return Albedo * Albedo;
	//return float4(positionWS, 0);
	//return pow(roughness, 2.2f);
	//return pow(MRIA, 2.2f);
	//return pow(float4(albedo, 1), 2.2f);
	//return float4(T.xyz, 1);
	//return float4(N.xyz, 1);
	//return pow(float4(roughness, metallic, 0, 0), 2.2f);
	//return float4(N_WS, 1);
	//return float4(N_WS / 2 + 0.5f, 1);
	// 
	//return float4(N_WS.xyz / 2 + 0.5f, 1) * smoothstep(0.2, 1.0f, (float(localLightCount) / float(lightCount)));
	//return color * (float(localLightCount) / float(lightCount));
	//return max(color, float4(0.3f, 0.3f, 0.3f, 1.0f)) * float4(N.xyz / 2 + 0.5f, 1) * (float(localLightCount) / float(lightCount));
	//return max(color, float4(0.3f, 0.3f, 0.3f, 1.0f)) * (float(localLightCount) / float(lightCount));
	return max(color, float4(0.3f, 0.3f, 0.3f, 1.0f)); * (float(localLightCount) / float(lightCount));
	//return lerp(float4(0, 0, 0, 0), float4(1, 1, 1, 0), float(localLightCount) / float(lightCount));
	//return float4(albedo, 1);
#endif
	
	return color;
}


/**********************************************************************

Copyright (c) 2022 Robert May

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
