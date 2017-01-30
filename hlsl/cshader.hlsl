/**********************************************************************

Copyright (c) 2015 - 2016 Robert May

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


/************************************************************************************************/


#include "common.hlsl"

static const float PI		 = 3.14159265359f;
static const float PIInverse = 1/PI;

// Outputs
RWTexture2D<float4> BB : register(u0); // out


/************************************************************************************************/
// OUTPUT STRUCTS

void WriteOut( float4 C, uint2 PixelCord, uint2 offset ) {BB[PixelCord + offset] = C;}


/************************************************************************************************/
// INPUT STRUCTS

cbuffer GBufferConstantsLayout : register(b1)
{
	uint DLightCount;
	uint DeferredPointLightCount;
	uint DeferredSpotLightCount;
	uint Height;
	uint Width;
	uint DebugRenderMode;
	uint Padding[2];
	float4 AmbientLight;
}

struct PointLight
{
	float4 P; // Position + R
	float4 K; // Color + I
};

struct SpotLight
{
	float4 P; // Position + R
	float4 K; // Color + I
	float4 D; // Direction;
};

struct DirectionalLight
{
	float4 D; // Direction
	float4 K; // Color + I
};

// Inputs
sampler Sampler : register(s0);

/************************************************************************************************/


StructuredBuffer<PointLight>	PointLights		: register(t0);
StructuredBuffer<SpotLight>		SpotLights		: register(t1);

Texture2D<float4>		 		Color			: register(t2);
Texture2D<float4>	 			Specular		: register(t3);
Texture2D<float4>               Emissive        : register(t4);
Texture2D<float2>               RoughMetal      : register(t5);
Texture2D<float4>	 			Normal			: register(t6);
Texture2D<float4>	 			Position 		: register(t7);
Texture2D<float>	 			DepthView 		: register(t8);


/************************************************************************************************/


struct SurfaceProperties
{
	float A;  // Roughness
	float Ks; //
	float kd; //
	float Kr; //
};


/************************************************************************************************/

// Inverse Depth
float3 TopLeft		= float3(-1,  1,  0);
float3 TopRight		= float3( 1,  1,  0);
float3 BottomLeft	= float3(-1, -1,  0);
float3 BottomRight	= float3( 1, -1,  0);

float3 GetViewVector(float3 pos)
{
	return normalize(CameraPOS - pos);
}

float3 GetVectorToBack(float3 pos)
{
	return normalize(CameraPOS - pos);
}

float PL(float3 LightPOS, float3 POS, float r, float lm) // Lighting Function Point Light
{
	float L = length(POS-LightPOS);
	float E = (lm/(L*L)) * saturate((1-(L/r)));
	return E;
}

float cot(float theta)
{
	return cos(theta)/sin(theta);
}

float3 F_Schlick(in float3 f0, in float3 f90, in float u)
{
	return f0 + (f90 - f0) * pow(1.0f - u, 5.0f);
}

float Fr_Disney(float ndotv, float ndotl, float ldoth, float linearRoughness) // Fresenal Factor
{
	float energyBias	= lerp(0, 0.5, linearRoughness);
	float energyFactor	= lerp(1.0, 1.0 / 1.51, linearRoughness);
	float fd90 			= energyBias + 2.0 * ldoth * ldoth * linearRoughness;
	float3 f0 			= float3(1.0f, 1.0f, 1.0f);

	float lightScatter	= F_Schlick(f0, fd90, ndotl).r;
	float viewScatter	= F_Schlick(f0, fd90, ndotv).r;
	return lightScatter * viewScatter * energyFactor;
}

float V_SmithGGXCorrelated(float ndotl, float ndotv, float alphaG)
{
	float alphaG2 = alphaG * alphaG;
	float Lambda_GGXV = ndotl * sqrt((-ndotv * alphaG2 + ndotv) * ndotv + alphaG2);
	float Lambda_GGXL = ndotv * sqrt((-ndotl * alphaG2 + ndotl) * ndotl + alphaG2);
	return 0.5f / (Lambda_GGXV + Lambda_GGXL);
}

float D_GGX(float ndoth , float m)
{
	//Divide by PI is apply later
	float m2 = m * m;
	float f = (ndoth * m2 - ndoth) * ndoth + 1;
	return m2 / (f * f);
} 

struct SphereAreaLight
{
	float3	POSITION;
	float 	R;	
};

float3 Frd(float3 l, float3 lc, float3 v, float3 WPOS, float3 Kd, float3 n, float3 Ks, float m, float r)
{
	float3 h = normalize(v + l);
	float  A  = saturate(pow(r, 2));

	float ndotv = saturate(dot(n, v) + 1e-5);
	float ndotl = saturate(dot(n, l));
	float ndoth = saturate(dot(n, h));
	float ldoth = saturate(dot(l, h));
	
	//	Specular BRDF
	float3 F 	= F_Schlick(Ks, 0.0f, ldoth);
	float Vis 	= V_SmithGGXCorrelated(ndotv, ndotl, A);
	float D 	= D_GGX(ndoth, A);
	float3 Fr 	= D * F * Vis;

	// 	Diffuse BRDF
	float Fd = Fr_Disney(ndotv, ndotl, ldoth, A);
	return  float3(Fr + (Fd * lc * Kd * (1 - m))) * ndotl;
}


/************************************************************************************************/


//groupshared PointLight Lights[256];

[numthreads(64, 2, 1)]
void cmain( uint3 ID : SV_DispatchThreadID, uint3 TID : SV_GroupThreadID)
{
	uint ThreadID = TID.x % 64 + TID.y * 128;
	float3 n = Normal.Load(int3(ID.xy, 0)).xyz;

	//Lights[ThreadID] = PointLights[ThreadID];
	//GroupMemoryBarrierWithGroupSync();

	if(!any(n))
		return;

	float3 Kd 	= Color.Load(int3(ID.xy, 0)).xyz;
	float3 WPOS = Position.Load(int3(ID.xy, 0)).xyz;
	float3 Ks 	= Specular.Load(int3(ID.xy, 0)).xyz;
	float  m 	= RoughMetal.Load(int3(ID.xy, 0)).y;
	float  r    = RoughMetal.Load(int3(ID.xy, 0)).x;
	float3 vdir = GetVectorToBack(WPOS);

    float3 ColorOut = float4(Kd * AmbientLight.xyz, 0.0f);

   
    #if 1
	switch (DebugRenderMode)
	{
		case 0:
			{
                for (int I = 0; I < DeferredPointLightCount; ++I)
                {
                    PointLight Light = PointLights[I];
                    float3 Lp = Light.P;
                    float3 Lc = Light.K;
                    float3 Lv = normalize(Lp - WPOS);
                    float La = PL(Lp, WPOS, Light.P[3], Light.K[3]); // Attenuation

                    ColorOut += Frd(Lv, Lc, vdir, WPOS, Kd, n, Ks, m, r) * La * PIInverse;
                }

				for (int II = 0; II < DeferredSpotLightCount; ++II)
				{
					float3 Lp = SpotLights[II].P;
					float3 Ld = SpotLights[II].D;
					float3 Lv = normalize(Lp - WPOS);
					float3 Lk = SpotLights[II].K;
					float La  = pow(max(dot(-Ld, Lv), 0), 10);

					ColorOut += float4(Frd(Lv, Lk, vdir, WPOS, Kd, n, Ks, m, r) * La * PIInverse, 0);
				}
			}   break;
		case 1: // Display Albedo Buffer
			{
				ColorOut = Kd;
			}   break;
		case 2: // Display Roughness
			{
				ColorOut = float3(r, r, r);
			}   break;
		case 3: // Display Metal
			{
				ColorOut = float3(m, m, m);
			}
			break;
		case 4: // Display Normal Buffer
			{
				ColorOut = n;
			}   break;
		case 5: // Lights used
			{
				float r = float(PointLightCount) / 64.0f;
				ColorOut = float3(r, r, r);
			}   break;
		default:
			break;
	}
    #endif

	WriteOut(float4(pow(ColorOut, 1.0f / 2.1f), 1), ID.xy, uint2(0, 0));
}


/************************************************************************************************/