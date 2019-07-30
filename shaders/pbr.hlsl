#define pi 3.14159265


float3 F_Schlick(in float3 f0, in float3 f90, in float u)
{
	return f0 + (f90 - f0) * pow(f90 - saturate(u), 5.0f);
}


float Fr_Disney(float ndotv, float ndotl, float ldoth, float linearRoughness) // Fresnel Factor
{
	float energyBias	= lerp(0, 0.5, linearRoughness);
	float energyFactor	= lerp(1.0, 1.0 / 1.51, linearRoughness);
	float fd90 			= energyBias + 2.0 * ldoth * ldoth * linearRoughness;
	float3 f0 			= 1.0f;

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


float Fr(float3 l, float3 lc, float3 v, float3 WPOS, float3 Kd, float3 n, float3 Ks, float m, float r) // Fresnel Factor
{
	float3 h  = normalize(v + l);
	float  A  = saturate(pow(r, 2));

	float ndotv = saturate(dot(n, v) + 1e-5);
	float ndotl = saturate(dot(n, l));
	float ndoth = saturate(dot(n, h));
	float ldoth = saturate(dot(l, h));
	
	//	Specular BRDF
	float3	F	= F_Schlick(Ks, 0.0f, ldoth);
	float	G	= V_SmithGGXCorrelated(ndotv, ndotl, A);
	float	D	= D_GGX(ndoth, A);

	return D * G * F / (4 * ndotv * ndotl);
}


float Fd(float3 l, float3 lc, float3 v, float3 WPOS, float3 Kd, float3 n, float3 Ks, float m, float r) // Fresnel Factor
{
	float3 h = normalize(v + l);
	float  A = saturate(pow(r, 2));

	float ndotv = saturate(dot(n, v) + 1e-5);
	float ndotl = saturate(dot(n, l));
	float ndoth = saturate(dot(n, h));
	float ldoth = saturate(dot(l, h));

	//	Specular BRDF
	float3	F = F_Schlick(Ks, 1.0f, ldoth);
	float	G = V_SmithGGXCorrelated(ndotv, ndotl, A);
	float	D = D_GGX(ndoth, A);

	return Kd * D * G * F / (pi * ndotv * ndotl);
}

float3 BRDF(float3 l, float3 lc, float3 v, float3 WPOS, float3 Kd, float3 n, float3 Ks, float m, float r)
{
	float3 h  = normalize(v + l);
	float  A  = saturate(pow(r, 2));

	float ndotv = (dot(n, v) + 1e-5);
	float ndotl = (dot(n, l));
	float ndoth = (dot(n, h));
	float ldoth = (dot(l, h));
	
	//const float ior = 1.505f;
	//float f0 = (ior - 1)/(ior + 1);
	//f0 *= f0;
	const float3 f0 = (m == 0) ? 0.04f : float3(0.549, 0.556, 0.554);
	
	//	Specular BRDF
	float3	F	= F_Schlick(f0, 1.0f, ndotv);
	float	G	= V_SmithGGXCorrelated(ndotv, ndotl, A);
	float	D	= D_GGX(ndoth, A);
	// 	Diffuse BRDF
	float Fd	= (m <= 0.5f) ? Fr_Disney(ndotv, ndotl, ldoth, A) : 0.0f;

	return saturate((Kd * Fd) + (Ks * D * G * F));
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