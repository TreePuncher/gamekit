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

void    WriteOut( float4 C, uint2 PixelCord, uint2 offset ) { BB[PixelCord + offset] = C; }
float4  ReadIn  (uint2 PixelCord)                           { return BB[PixelCord]; }


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

struct LightTile
{
	uint Offset;
	uint Count;
};

// Inputs
sampler Sampler : register(s0);

/************************************************************************************************/


StructuredBuffer<PointLight>	PointLights		: register(t0);
StructuredBuffer<SpotLight>		SpotLights		: register(t1);
StructuredBuffer<LightTile>		LightTiles		: register(t7);

Texture2D<float4>		 		Color			: register(t2);
Texture2D<float4>	 			Specular		: register(t3);
Texture2D<float4>               Emissive        : register(t4);
Texture2D<float2>               RoughMetal      : register(t5);
Texture2D<float4>	 			Normal			: register(t6);
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


groupshared PointLight Lights[256];


[numthreads(32, 12, 1)]
void Tiled_Shading( uint3 ID : SV_DispatchThreadID, uint3 TID : SV_GroupThreadID)
{
	uint ThreadID = TID.x % 16 + TID.y * 8;
	float3 Kd = Color.Load(int3(ID.xy, 0)).xyz;
	int MatID = Color.Load(int3(ID.xy, 0)).w;

	// Pre-Load Lights
	int Itr = 0;
	while (Itr < PointLightCount)
	{
		Lights[Itr + ThreadID] = PointLights[ThreadID];
		Itr += 16 * 8;
	}

	GroupMemoryBarrierWithGroupSync();

	if (MatID == 0)
		return;

	float U = float(ID.x) / WindowWidth;
	float V = float(ID.y) / WindowHeight;

	float3 Temp1    = lerp(WSTopLeft, WSBottomLeft,  V);
	float3 Temp2    = lerp(WSTopRight, WSBottomRight, V);
	float3 FarPos   = lerp(Temp1, Temp2, U);

	float3 Temp3    = lerp(WSTopLeft_Near, WSBottomLeft_Near, V);
	float3 Temp4    = lerp(WSTopRight_Near, WSBottomRight_Near, V);
	float3 NearPos  = lerp(Temp3, Temp4, U);

	
	float3 n = Normal.Load(int3(ID.xy, 0)).xyz;
	float  l = Normal.Load(int3(ID.xy, 0)).w;

	/*
	float2 ProjRatio = float2(MaxZ/(MaxZ - MinZ), MinZ/(MinZ-MaxZ));
	float LinearDepth = ProjRatio.y / (l - ProjRatio.x);
	*/

	float Distance = l;
    float LinearNormlizedDisance = (Distance - MinZ) / (500 - MinZ);

	float3 ViewRay = normalize(FarPos - CameraPOS);
	float3 WPOS    = CameraPOS.xyz + (ViewRay * Distance);
	float3 Ks 	   = Specular.Load(int3(ID.xy, 0)).xyz;
	float  m 	   = RoughMetal.Load(int3(ID.xy, 0)).y;
	float  r       = RoughMetal.Load(int3(ID.xy, 0)).x;
	float3 vdir    = -ViewRay;

    //float3 ColorOut = float4(Kd * float3(1, 1, 1), 0.0f) * dot(n, float3(0, 1, 0));
    float3 ColorOut = 0;

    #if 1
	switch (MatID)
	{
		case 1:
			{
				switch (DebugRenderMode)
				{
					case 0:
						{
							[loop]
							for (int I = 0; I < DeferredPointLightCount; ++I)
							{
								PointLight Light = Lights[I];
								float3 Lp = Light.P;
								float3 Lc = Light.K;
								float3 Lv = normalize(Lp - WPOS);
								float La  = PL(Lp, WPOS, Light.P[3], Light.K[3]); // Attenuation

								ColorOut += Frd(Lv, Lc, vdir, WPOS, Kd, n, Ks, m, r) * La * PIInverse;
                            }

							[loop]
							for (int II = 0; II < DeferredSpotLightCount; ++II)
							{
								float3 Lp = SpotLights[II].P;
								float3 Ld = SpotLights[II].D;
								float3 Lv = normalize(Lp - WPOS);
								float3 Lk = SpotLights[II].K;
								float La = pow(Max(dot(-Ld, Lv), 0), 10);

								ColorOut += float4(Frd(Lv, Lk, vdir, WPOS, Kd, n, Ks, m, r) * La * PIInverse, 0);
							}
						}
						break;
					case 1: // Display Albedo Buffer
						{
							ColorOut = Kd;
						}
						break;
					case 2: // Display Roughness
						{
							ColorOut = float3(r, r, r);
						}
						break;
					case 3: // Display Metal
						{
							ColorOut = float3(m, m, m);
						}
						break;
					case 4: // Display WorldSpace Normals
						{
							ColorOut = n;
						}
						break;
                    case 5: // Display Screeen Space Normal Buffer
						{
                            ColorOut = mul(PV, n);
                        }
                        break;
					case 6: // World Position
						{
							ColorOut = float3(WPOS);
							break;
						}
					case 7: // Depth Debug
						{
							//float4 ActualPosition = Position.Load(int3(ID.xy, 0));
							//float e = length(WPOS - ActualPosition);
                            ColorOut = LinearNormlizedDisance;
							break;
						}
                    case 8:
                    {
                    /*
                        [loop]
                        for (int I = 0; I < DeferredPointLightCount; ++I)
                        {
                            PointLight Light = Lights[I];
                            float3 Lp   = Light.P;
                            float3 Lc   = Light.K;
                            float3 Lv   = normalize(Lp - WPOS);
                            float La    = PL(Lp, WPOS, Light.P[3], Light.K[3]); // Attenuation

                            ColorOut += La * Lc;
                        }
                    */
                            ColorOut = 0;

                        }   break;
					default:
						break;
				}
			}
			break;
		case 2:
			{
				break;
			}
		default:
			break;
	}

#else
    [loop]
	for (int I = 0; I < DeferredPointLightCount; ++I)
	{
		PointLight Light = Lights[I];
		float3 Lp = Light.P;
		float3 Lc = Light.K;
		float3 Lv = normalize(Lp - WPOS);
		float La  = PL(Lp, WPOS, Light.P[3], Light.K[3]); // Attenuation

		ColorOut += Frd(Lv, Lc, vdir, WPOS, Kd, n, Ks, m, r) * La * PIInverse;
	}

	[loop]
	for (int II = 0; II < DeferredSpotLightCount; ++II)
	{
		float3 Lp = SpotLights[II].P;
		float3 Ld = SpotLights[II].D;
		float3 Lv = normalize(Lp - WPOS);
		float3 Lk = SpotLights[II].K;
		float La = pow(Max(dot(-Ld, Lv), 0), 10);

		ColorOut += float4(Frd(Lv, Lk, vdir, WPOS, Kd, n, Ks, m, r) * La * PIInverse, 0);
	}
#endif
	WriteOut(float4(ColorOut, 1), ID.xy, uint2(0, 0));

	//WriteOut(float4(pow(ColorOut, 1.0f / 2.1f), 1), ID.xy, uint2(0, 0));
}


/************************************************************************************************/


[numthreads(32, 12, 1)]
void ConvertOut(uint3 ID : SV_DispatchThreadID, uint3 TID : SV_GroupThreadID)
{
    WriteOut(pow(ReadIn(ID.xy), 1.0f / 2.1f), ID.xy, uint2(0, 0));
}


/************************************************************************************************/