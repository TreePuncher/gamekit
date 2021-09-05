#include "common.hlsl"

struct ParticleMeshInstanceVS_Input
{
    float3 POS		: POSITION;
    float3 Normal	: NORMAL;
    float3 Tangent	: Tangent;

    float4 instancePosition : INSTANCEPOS;
    float4 instanceArgs     : INSTANCEARGS; 
};

struct ParticleVertex
{
    centroid    float4 POS		: SV_POSITION;
                float3 WPOS		: POSITION;
                float3 Normal	: NORMAL;
                float3 Tangent	: Tangent;
};

ParticleVertex ParticleMeshInstanceVS(ParticleMeshInstanceVS_Input input)
{
    ParticleVertex output;
    output.WPOS     = input.POS + input.instancePosition;
    output.POS      = mul(PV, float4(output.WPOS, 1));
    output.Normal   = mul(PV, input.Normal); 
    output.Tangent  = mul(PV, input.Tangent);

    return output;
}

struct PS_Out
{
    float4  Albedo  : SV_TARGET0;
    float4  MRIA    : SV_TARGET1;
    float4  Normal  : SV_TARGET2;

    float   Depth   : SV_DepthLessEqual;
};

PS_Out ParticleMeshInstancePS(ParticleVertex input)
{
    PS_Out output;

    output.Albedo = float4(1.0f, 1.0f, 1.0f, 1.0f);
    output.MRIA   = float4(1.0f, 0.1f, 0.0f, 0.0f);
    output.Normal = float4(normalize(input.Normal), 1);
    output.Depth  = length(CameraPOS - input.WPOS) / MaxZ;

    return output;
}


/**********************************************************************

Copyright (c) 2015 - 2021 Robert May

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
