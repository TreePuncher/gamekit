static const float PI			= 3.14159265359f;
static const float INV_PI	    = 1 / PI;

float Square(float x) { return x * x; }
 
float Theta(float3 w)
{
    return acos(w.z / length(w));
}
 
float CosTheta(float3 w)
{
    return cos(Theta(w));
}

float EricHeitz2018GGXG1Lambda(float3 V, float alpha_x,  float alpha_y)
{
    float Vx2 = Square(V.x);
    float Vy2 = Square(V.y);
    float Vz2 = Square(V.z);
    float ax2 = Square(alpha_x);
    float ay2 = Square(alpha_y);
    return (-1.0 + sqrt(1.0 + (Vx2 * ax2 + Vy2 * ay2) / Vz2)) / 2.0;
}
 
float EricHeitz2018GGXG1(float3 V, float alpha_x, float alpha_y)
{
    return 1.0 / (1.0 + EricHeitz2018GGXG1Lambda(V, alpha_x, alpha_y));
}
 
// wm: microfacet normal in frame
float EricHeitz2018GGXD(float3 N,  float alpha_x, float alpha_y)
{
    float Nx2 = Square(N.x);
    float Ny2 = Square(N.y);
    float Nz2 = Square(N.z);
    float ax2 = Square(alpha_x);
    float ay2 = Square(alpha_y);
    return 1.0 / (PI * alpha_x * alpha_y * Square(Nx2 / ax2 + Ny2 / ay2 + Nz2));
}
 
float EricHeitz2018GGXG2(float3 V, float3 L, float alpha_x, float alpha_y)
{
    return EricHeitz2018GGXG1(V, alpha_x, alpha_y) * EricHeitz2018GGXG1(L, alpha_x, alpha_y);
}
 
float3 SchlickFresnel(float NoX, float3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - NoX, 5.0);
}

// L is in tangent space here
float3 Lambert(float3 L, float3 albedo)
{
	return CosTheta(L) * INV_PI * albedo;
}

float3 EricHeitz2018GGX(float3 V, float3 L, float3 albedo, float metallic, float roughness, float anisotropic, float ior)
{
    float alpha = roughness * roughness;
    float aspect = sqrt(1.0 - 0.9 * anisotropic);
    float alpha_x = alpha * aspect;
    float alpha_y = alpha / aspect;
 
    float3 H = normalize(L + V);
    float NoV = CosTheta(V);
    float NoL = CosTheta(L);
    if (NoV < 0.0 || NoL < 0.0) return float3(0, 0, 0);
 
    float VoH = dot(V, H);
    float NoH = CosTheta(H);
    float3 F0 = float3(1, 1, 1) * abs((1.0 - ior) / (1.0 + ior));
    F0 = F0 * F0;
	F0 = lerp(F0, albedo, metallic);
 
    float D = EricHeitz2018GGXD(H, alpha_x, alpha_y);
    float3 F = SchlickFresnel(max(NoV, 0.0), F0);
    float G = EricHeitz2018GGXG2(V, L, alpha_x, alpha_y);

    return (F * D * G) / (4.0 * VoH * NoH);
}

float3 HammonEarlGGX(float3 V, float3 L, float3 albedo, float roughness)
{
    float NoV = CosTheta(V);
    float NoL = CosTheta(L);
 
    if (NoV < 0.0 || NoL < 0.0) return float3(0, 0, 0);
 
	float3 H = normalize(V + L);
    float NoH = CosTheta(H);
    float VoL = dot(V, L);
 
    float alpha = roughness * roughness;
 
    float facing = 0.5 + 0.5 * VoL;
    float roughy = facing * (0.9 - 0.4 * facing) * (0.5 + NoH)/NoH;
    float smoothy = 1.05 * (1.0 - pow(1.0 - NoL, 5.0)) * (1.0 - pow(1.0 - NoV, 5.0));
    float single = INV_PI * lerp(smoothy, roughy, alpha);
    float multi = alpha * 0.1159;
    return albedo * (single + albedo * multi);
}

/*
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
	return saturate(0.5f / (Lambda_GGXV + Lambda_GGXL));
}


float D_GGX(float ndoth , float m)
{
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


float Fr(float3 l, float3 lc, float3 v, float3 WPOS, float3 Kd, float3 n, float3 Ks, float m, float r) // specular
{
	const float3 h  = normalize(v + l);
	const float  A  = saturate(pow(r, 2));

	const float ndotv = saturate(dot(n, v) + 1e-5);
	const float ndotl = saturate(dot(n, l));
	const float ndoth = saturate(dot(n, h));
	const float ldoth = saturate(dot(l, h));
	
	const float3 f0 = (m == 0) ? 0.04f : float3(0.549, 0.556, 0.554);

	const float3	F = F_Schlick(Ks, 1.0f, ldoth);
	const float		G = V_SmithGGXCorrelated(ndotv, ndotl, A);
	const float		D = D_GGX(ndoth, A);

	return D * G * F * ndotv * ndotl / 4;
}


float Fd(float3 l, float3 lc, float3 v, float3 WPOS, float3 Kd, float3 n, float3 Ks, float m, float r) // diffuse
{
	const float3 h = normalize(v + l);
	const float  A = saturate(pow(r, 2));

	const float ndotv = saturate(dot(n, v) + 1e-5);
	const float ndotl = saturate(dot(n, l));
	const float ndoth = saturate(dot(n, h));
	const float ldoth = saturate(dot(l, h));

	const float3 f0 = (m == 0) ? 0.04f : float3(0.549, 0.556, 0.554);

	const float3	F = F_Schlick(f0, 1.0f, ldoth);
	const float		G = V_SmithGGXCorrelated(ndotv, ndotl, A);
	const float		D = D_GGX(ndoth, A);

	return Fr_Disney(ndotv, ndotl, ldoth, A) * ndotl;
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
	float	G	= V_SmithGGXCorrelated(ndotv, ndotl, r);
	float	D	= D_GGX(ndotl, A);

	return 	(m == 1) ? Fd(l, lc, v, WPOS, Kd, n, Ks, m, r) : 0 +
			Fr(l, lc, v, WPOS, Kd, n, Ks, m, A);
}


float Square(float x) { return x * x; }
 
float Theta(float3 w)
{
    return acos(w.z / length(w));
}
 
float CosTheta(float3 w)
{
    return cos(Theta(w));
}
 
float EricHeitz2018GGXG1Lambda(float3 V, float alpha_x,  float alpha_y)
{
    float Vx2 = Square(V.x);
    float Vy2 = Square(V.y);
    float Vz2 = Square(V.z);
    float ax2 = Square(alpha_x);
    float ay2 = Square(alpha_y);
    return (-1.0 + sqrt(1.0 + (Vx2 * ax2 + Vy2 * ay2) / Vz2)) / 2.0;
}
 
float EricHeitz2018GGXG1(float3 V, float alpha_x, float alpha_y)
{
    return 1.0 / (1.0 + EricHeitz2018GGXG1Lambda(V, alpha_x, alpha_y));
}
 
// wm: microfacet normal in frame
float EricHeitz2018GGXD(float3 N,  float alpha_x, float alpha_y)
{
    float Nx2 = Square(N.x);
    float Ny2 = Square(N.y);
    float Nz2 = Square(N.z);
    float ax2 = Square(alpha_x);
    float ay2 = Square(alpha_y);
    return 1.0 / (pi * alpha_x * alpha_y * Square(Nx2 / ax2 + Ny2 / ay2 + Nz2));
}
 
float EricHeitz2018GGXG2(float3 V, float3 L, float alpha_x, float alpha_y)
{
    return EricHeitz2018GGXG1(V, alpha_x, alpha_y) * EricHeitz2018GGXG1(L, alpha_x, alpha_y);
}
 
float3 SchlickFresnel(float NoX, float3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - NoX, 5.0);
}

float3 EricHeitz2018GGX(float3 V, float3 L, float roughness, float anisotropic, float ior)
{
    float alpha = roughness * roughness;
    float aspect = sqrt(1.0 - 0.9 * anisotropic);
    float alpha_x = alpha * aspect;
    float alpha_y = alpha / aspect;
 
    float3 H = normalize(L + V);
    float NoV = CosTheta(V);
    float NoL = CosTheta(L);
    if (NoV < 0.0 || NoL < 0.0) return float3(0, 0, 0);
 
    float VoH = dot(V, H);
    float NoH = CosTheta(H);
    float3 F0 = float3(1, 1, 1) * abs((1.0 - ior) / (1.0 + ior));
    F0 = F0 * F0;
 
    float D = EricHeitz2018GGXD(H, alpha_x, alpha_y);
    float3 F = SchlickFresnel(max(NoV, 0.0), F0);
    float G = EricHeitz2018GGXG2(V, L, alpha_x, alpha_y);

    return (F * D * G) / (4.0f * VoH * NoH);
}
*/

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
