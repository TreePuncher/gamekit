#include "common.hlsl"

struct PointLight
{
	float4 KI;	// Color + intensity in W
	float4 PR;	// XYZ + radius in W
};


cbuffer EntityConstants : register(b1)
{
	float4	 Albedo;
	float    Ks;
	float    IOR;
	float    Roughness;
	float    Anisotropic;
	float    Metallic;
	float4x4 WT;
	uint     textureCount;
}

StructuredBuffer<float4x4> Poses : register(t0);

struct Vertex
{
	float3 POS		: POSITION;
	float3 Normal	: NORMAL;
	float3 Tangent	: Tangent;
	float2 UV		: TEXCOORD;
};

struct Forward_VS_OUT
{
	float4 POS 		: SV_POSITION;
	float  depth    : DEPTH;
	float3 Normal	: NORMAL;
	float3 Tangent	: TANGENT;
	float2 UV		: TEXCOORD;
	float3 Bitangent : BITANGENT;
};


/************************************************************************************************/


Forward_VS_OUT Forward_VS(Vertex In)
{
	const float3 POS_WS = mul(WT, float4(In.POS, 1));
	const float3 POS_VS = mul(View, float4(POS_WS, 1));

	Forward_VS_OUT Out;
	Out.depth		= -POS_VS.z / MaxZ;
	Out.POS			= mul(PV, float4(POS_WS, 1));
	Out.Normal		= normalize(mul(WT, float4(In.Normal, 0.0f)));
	Out.Tangent		= normalize(mul(WT, float4(In.Tangent, 0.0f)));
	Out.Bitangent	= cross(Out.Tangent, Out.Normal);
	Out.UV			= In.UV;

	return Out;
}

/************************************************************************************************/


struct VertexSkinned
{
	float3 POS				: POSITION;
	float3 Normal			: NORMAL;
	float3 Tangent			: Tangent;
	float2 UV				: TEXCOORD;
	float3 Weights			: BLENDWEIGHT;
	uint4  Indices			: BLENDINDICES;
	float3 POS_Blend		: BLENDPOS;
	float3 Normal_Blend		: BLENDNORM;
	float3 Tangent_Blend	: BLENDTAN;
};

Forward_VS_OUT ForwardSkinned_VS(VertexSkinned In)
{
	//float4 P = float4(In.POS_Blend, 0.0f);
	//float4 N = float4(In.Normal_Blend, 0.0f);
	//float4 T = float4(In.Tangent_Blend, 0.0f);

	float4 P = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 N = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 T = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 W = float4(In.Weights.xyz, 1 - In.Weights.x - In.Weights.y - In.Weights.z);

	[unroll(4)]
	for (uint I = 0; I < 4; ++I)
	{
		P += mul(Poses[In.Indices[I]], float4(In.POS, 1)) * W[I];
		N += mul(Poses[In.Indices[I]], float4(In.Normal, 0)) * W[I];
		T += mul(Poses[In.Indices[I]], float4(In.Tangent, 0)) * W[I];
	}

	const float3 POS_WS = mul(WT, float4(P.xyz, 1));
	const float3 POS_VS = mul(View, float4(POS_WS, 1));

	Forward_VS_OUT Out;
	Out.depth		= -POS_VS.z / MaxZ;
	Out.POS			= mul(PV, float4(POS_WS, 1));
	Out.Normal		= normalize(mul(WT, float4(N.xyz, 0.0f)));
	Out.Tangent		= normalize(mul(WT, float4(T.xyz, 0.0f)));
	Out.Bitangent	= cross(Out.Tangent, Out.Normal);
	Out.UV			= In.UV;

	return Out;
}


/**********************************************************************

Copyright (c) 2015 - 2022 Robert May

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
