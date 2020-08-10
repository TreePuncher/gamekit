static const float PI			= 3.14159265359f;
static const float INV_PI	    = 1 / PI;

float Square(const float x) { return x * x; }
 
float Theta(const float3 w)
{
    return acos(w.z / length(w));
}
 
float CosTheta(const float3 w)
{
    return cos(Theta(w));
}

float F_Schlick(float LdotH, float3 f0, float3 f90)
{
    return f0 + ( f90 - f0 ) * pow(1.0f - LdotH , 5.0f);
}

float V_SmithGGXCorrelated(float NdotL, float NdotV, float alphaG )
{
    // This is the optimize version
    float alphaG2 = alphaG * alphaG;
    // Caution : the " NdotL *" and " NdotV *" are explicitely inversed , this is not a mistake .
    float Lambda_GGXV = NdotL * sqrt (( -NdotV * alphaG2 + NdotV ) * NdotV + alphaG2 );
    float Lambda_GGXL = NdotV * sqrt (( -NdotL * alphaG2 + NdotL ) * NdotL + alphaG2 );

    return 0.5f / ( Lambda_GGXV + Lambda_GGXL );
}

float G1(const float NdotV, const float K)
{
    return NdotV / (NdotV * (NdotV * (1 - K) + K));
}

float G_UE4(float NdotL, float NdotV, float r)
{
    const float K = Square(r + 1) / 8;
    return G1(NdotV, K) * G1(NdotL, K);
}


float D_GGX(const float NdotH, const float m)
{
    const float m2 = m * m;
    return INV_PI * m2 / Square(NdotH * NdotH * (m2 - 1) + 1);
}

float F_r(float3 V, float3 H, float3 L, float3 N, float roughness)
{
    const float NdotV = abs(dot(N, V)) + 0.00001f; 
    const float LdotH = saturate(dot(L, H));
    const float NdotH = saturate(dot(N, H)); 
    const float NdotL = saturate(dot(N, L));

    const float3 f0  = 2.0 * LdotH * LdotH * roughness;
    const float3 f90 = 1.0f;

    const float F   = F_Schlick(f0, f90, LdotH);
    const float Vis = V_SmithGGXCorrelated(NdotV, NdotL, roughness);
    const float D   = D_GGX(NdotH, roughness);

    return (D * F * Vis * INV_PI);
}

float Fr_DisneyDiffuse(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
    float energyBias    = lerp (0 , 0.5 , linearRoughness );
    float energyFactor  = lerp (1.0 , 1.0 / 1.51 , linearRoughness );
    float fd90          = energyBias + 2.0 * LdotH * LdotH * linearRoughness ;
    float3 f0           = float3 (1.0f , 1.0f , 1.0f);
    float lightScatter  = F_Schlick ( f0 , fd90 , NdotL ).r;
    float viewScatter   = F_Schlick (f0 , fd90 , NdotV ).r;

    return lightScatter * viewScatter * energyFactor;
}


float F_d(float3 V, float3 H, float3 L, float3 N, float roughness)
{
    const float NdotV = abs(dot(N, V)) + 0.00001f; 
    const float NdotL = saturate(dot(N, L));
    const float LdotH = saturate(dot(L, H));

    return Fr_DisneyDiffuse( NdotV , NdotL , LdotH , roughness ) * INV_PI;
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
