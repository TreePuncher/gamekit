/**********************************************************************

Copyright (c) 2015-2017 Robert May

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

float3 TopLeft		= float3(-1,  1,  0);
float3 TopRight		= float3( 1,  1,  0);
float3 BottomLeft	= float3(-1, -1,  0);
float3 BottomRight	= float3( 1, -1,  0);

cbuffer CameraConstants : register( b0 )
{
    float4x4 View;
    float4x4 ViewI;
    float4x4 Proj;
	float4x4 PV;				// Projection x View
	float4x4 PVI;				// Projection x View
	float4   CameraPOS;
	float  	 MinZ;
	float  	 MaxZ;

	float4  WSTopLeft;
	float4  WSTopRight;
	float4  WSBottomLeft;
	float4  WSBottomRight;

	float4  WSTopLeft_Near;
	float4  WSTopRight_Near;
	float4  WSBottomLeft_Near;
	float4  WSBottomRight_Near;
};


struct PS_IN
{
	float3 WPOS  : TEXCOORD0;
	float3 N     : TEXCOORD1;
	float2 UV    : TEXCOORD2;
	float4 POS   : SV_POSITION;
	float Depth  : TEXCOORD3;
};

struct PS_IN2
{
	float3 WPOS 	: TEXCOORD0;
	float4 N 		: TEXCOORD1;
	float3 T 		: TEXCOORD2;
	float3 B 		: TEXCOORD3;
	float4 POS 	 	: SV_POSITION;
	float Depth     : TEXCOORD4;
};

struct RectPoint_PS
{
	float4 Color	: COLOR;
	float2 UV		: TEXCOORD;
	float4 POS 		: SV_POSITION;
};

struct PS_Colour_IN
{
	float3 WPOS 	: TEXCOORD0;
	float3 Colour	: TEXCOORD1;
	float3 N 		: TEXCOORD2;
};

struct NormalMapped_IN
{
	float3 WPOS 	: TEXCOORD0;
	float4 N 		: TEXCOORD1;
	float3 T 		: TEXCOORD2;
	float3 B 		: TEXCOORD3;
};

struct Plane
{
	float4 Normal;
	float4 Orgin;
};

float Hash(float2 IN)// Thanks nVidia
{
    return frac(1.0e4 * sin(17.0f * IN.x + 0.1 * IN.y) *
                (0.1 + abs(sin(13.0 * IN.y + IN.x))));
}

float Hash3D(float3 IN)// Thanks nVidia
{
    return Hash(float2(Hash(IN.xy), IN.z));
}

float4 SampleVTexture(Texture2D OffsetTable, Texture2D TextureAtlas, float2 Coordinate)
{
	return float4(0, 0, 0, 0);
}

/*
float3 GetViewVectorWPOS(float3 pos)
{
	return normalize(pos - CameraPOS.xyz);
}
*/

float3 GetVectorToBack(float3 pos)
{
	return CameraPOS.xyz - pos;
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

float2 PixelCordToCameraCord(int2 UV, float2 WH)
{
    float2 Out = float2(UV) / WH;
	return Out * 2 - 1.0;
}


//float2 GetTextureSpaceCord(float2 UV)
//{
 //   return UV / float2(WindowWidth, WindowHeight);
//}

//float2 ViewToTextureSpace(float2 CoordDS)
//{
//   return float2((CoordDS.x + 1) / 2, (1 - CoordDS.y) / 2) * float2(WindowWidth, WindowHeight);
//}


float NormalizeAndRescaleZ(float Z_in, float Scale)
{
    return saturate((Z_in - MinZ) / (Scale - MinZ));
}

float3 GetViewVector(float2 UV)
{
    float3 Temp1    = lerp(WSTopLeft, WSBottomLeft, UV.y);
    float3 Temp2    = lerp(WSTopRight, WSBottomRight, UV.y);
    float3 FarPos   = lerp(Temp1, Temp2, UV.x);

    float3 Temp3    = lerp(WSTopLeft_Near, WSBottomLeft_Near, UV.y);
    float3 Temp4    = lerp(WSTopRight_Near, WSBottomRight_Near, UV.y);
    float3 NearPOS  = lerp(Temp3, Temp4, UV.x);

    return normalize(NearPOS - FarPos);
}

float3 GetWorldSpacePosition(float3 ViewVector, float3 Origin, float Z) 
{
    return Origin + ViewVector * Z;
}


/*
float3 GetWorldSpacePositionAndViewDir(float3 UVW, out float3 VWS)
{
    #if 1

    VWS = GetViewVector(UVW.xy);
    return CameraPOS + VWS * -UVW.z * MaxZ;
    #else

    /*
    float3 Temp1    = lerp(WSTopLeft, WSBottomLeft, UVW.y);
    float3 Temp2    = lerp(WSTopRight, WSBottomRight, UVW.y);
    float3 FarPos   = lerp(Temp1, Temp2, UVW.x);

    float3 Temp3    = lerp(WSTopLeft_Near, WSBottomLeft_Near, UVW.y);
    float3 Temp4    = lerp(WSTopRight_Near, WSBottomRight_Near, UVW.y);
    float3 NearPOS  = lerp(Temp3, Temp4, UVW.x);

    VWS = normalize(NearPOS - FarPos);
    return CameraPOS + VWS * UVW.z * MaxZ;
    VWS = 0;
    float4 POSVS = float4(PixelCordToCameraCord(UVW.xy), UVW.z, 1);
    float4 POSWS = mul(ViewI, POSVS);

    return POSWS;
    #endif
}
*/

// PBR Utility Functions
float3 F_Schlick(in float3 f0, in float3 f90, in float u)
{
	return f0 + (f90 - f0) * pow(1.0f - u, 5.0f);
}

float Fr_Disney(float ndotv, float ndotl, float ldoth, float linearRoughness) // Fresnel Factor
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
	float alphaG2       = alphaG * alphaG;
	float Lambda_GGXV   = ndotl * sqrt((-ndotv * alphaG2 + ndotv) * ndotv + alphaG2);
	float Lambda_GGXL   = ndotv * sqrt((-ndotl * alphaG2 + ndotl) * ndotl + alphaG2);
	return 0.5f / (Lambda_GGXV + Lambda_GGXL);
}

float D_GGX(float ndoth , float m)
{
	//Divide by PI is apply later
	float m2    = m * m;
	float f     = (ndoth * m2 - ndoth) * ndoth + 1;
	return m2 / (f * f);
} 

float DistanceSquared2(float2 a, float2 b) 
{
 	a -= b; 
 	return dot(a, a); 
}

float DistanceSquared3(float2 a, float2 b) 
{
 	a -= b; 
 	return dot(a, a); 
}

struct SphereAreaLight
{
	float3	POSITION;
	float 	R;	
};

float3 Frd(float3 l, float3 lc, float3 v, float3 WPOS, float3 Kd, float3 n, float3 Ks, float m, float r)
{
	float3 h  = normalize(v + l);
	float  A  = saturate(pow(r, 2));

	float ndotv = saturate(dot(n, v) + 1e-5);
	float ndotl = saturate(dot(n, l));
	float ndoth = saturate(dot(n, h));
	float ldoth = saturate(dot(l, h));
	
	//	Specular BRDF
	float3 F    = F_Schlick(Ks, 0.0f, ldoth);
	float Vis   = V_SmithGGXCorrelated(ndotv, ndotl, A);
	float D     = D_GGX(ndoth, A);
	float3 Fr   = D * F * Vis;

	// 	Diffuse BRDF
	float Fd = Fr_Disney(ndotv, ndotl, ldoth, A);
	return float3(Fr + (Fd * lc * Kd * (1 - m))) * ndotl;
}
