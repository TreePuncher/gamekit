[[fk::BeginRootSignatureDef(id=rootsig1)]]
[[fk::RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)]]
[[fk::DescriptorSet(SRVTexture(num=unbounded))]]


#define TopLeft float3(-1,  1,  0);
#define TopRight float3( 1,  1,  0);
#define BottomLeft float3(-1, -1,  0);
#define BottomRight float3( 1, -1,  0);


[[fk::CBV(binding=0)]]
{
	float4x4 View;
	float4x4 ViewI;
	float4x4 Proj;
	float4x4 PV; // Projection x View
	float4x4 PVI;
	float4 CameraPOS;
	float MinZ;
	float MaxZ;
	float AspectRatio;
	float FOV;
	
	float3 TLCorner_VS;
	float3 TRCorner_VS;

	float3 BLCorner_VS;
	float3 BRCorner_VS;
};

struct PS_IN
{
	float3 WPOS : TEXCOORD0;
	float3 N : TEXCOORD1;
	float2 UV : TEXCOORD2;
	float4 POS : SV_POSITION;
	float Depth : TEXCOORD3;
};

struct PS_IN2
{
	float3 WPOS : TEXCOORD0;
	float4 N : TEXCOORD1;
	float3 T : TEXCOORD2;
	float3 B : TEXCOORD3;
	float4 POS : SV_POSITION;
	float Depth : TEXCOORD4;
};

struct RectPoint_PS
{
	float4 Color : COLOR;
	float2 UV : TEXCOORD;
	float4 POS : SV_POSITION;
};

struct RectPointDepth_PS
{
	float4 Color : COLOR;
	float2 UV : TEXCOORD;
	float4 POS : SV_POSITION;
	float depth : DEPTH;
};

struct DrawFlatTri3D_IN
{
	float4 Color : COLOR;
	float2 UV : TEXCOORD;
	float4 POS : SV_POSITION;
	float Depth : DEPTH;
};

struct PS_Colour_IN
{
	float3 WPOS : TEXCOORD0;
	float3 Colour : TEXCOORD1;
	float3 N : TEXCOORD2;
};

struct NormalMapped_IN
{
	float3 WPOS : TEXCOORD0;
	float4 N : TEXCOORD1;
	float3 T : TEXCOORD2;
	float3 B : TEXCOORD3;
};

struct Plane
{
	float4 Normal;
	float4 Orgin;
};

float Hash(in float x)
{
	uint2 n = uint(x) * uint2(1597334673U, 3812015801U);
	uint q = (n.x ^ n.y) * 1597334673U;
	return float(q) * (1.0 / float(0xFFFFFFFFU));
}

float2 Hash2(in uint q)
{
	uint2 n = q * uint2(1597334673U, 3812015801U);
	n = (n.x ^ n.y) * uint2(1597334673U, 3812015801U);
	return float2(n) * (1.0 / float(0xFFFFFFFFU));
}

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

float3 GetVectorToBack(float3 pos)
{
	return CameraPOS.xyz - pos;
}

float PL(float3 LightPOS, float3 POS, float r, float lm) // Lighting Function Point Light
{
	float L = length(POS - LightPOS);
	float E = (lm / (L * L)) * saturate((1 - (L / r)));
	return E;
}

float cot(float theta)
{
	return cos(theta) / sin(theta);
}

float2 PixelCordToCameraCord(int2 UV, float2 WH)
{
	float2 Out = float2(UV) / WH;
	return Out * 2 - 1.0;
}

float2 GetTextureSpaceCord(float2 UV, float2 WH)
{
	return UV / WH;
}

float2 ViewToTextureSpace(float2 CoordDS, float2 WH)
{
	return float2((CoordDS.x + 1) / 2, (1 - CoordDS.y) / 2) * WH;
}

float NormalizeAndRescaleZ(float Z_in, float Scale)
{
	return saturate((Z_in - MinZ) / (Scale - MinZ));
}

float3 GetViewVector_VS(const float2 UV) // View Space Vector
{
	const float3 LeftPoint = lerp(TLCorner_VS, BLCorner_VS, UV.y); // Left Edge
	const float3 RightPoint = lerp(TRCorner_VS, BRCorner_VS, UV.y); // Right Edge
	const float3 FarPos = lerp(LeftPoint, RightPoint, UV.x);

	return normalize(FarPos);
}

float3 GetViewVector(const float2 UV)
{
	const float3 View_VS = GetViewVector_VS(UV);
	return mul(ViewI, View_VS);
}

float3 GetViewSpacePosition(float2 UV, float D)
{
	const float3 LeftPoint = lerp(TLCorner_VS, BLCorner_VS, UV.y); // Left Edge
	const float3 RightPoint = lerp(TRCorner_VS, BRCorner_VS, UV.y); // Right Edge
	const float3 V = lerp(LeftPoint, RightPoint, UV.x) * D;

	return V;
}

void GetViewSpacePOSanDIR(float2 UV, float D, out float3 POS_VS, out float3 V)
{
	const float3 LeftPoint = lerp(TLCorner_VS, BLCorner_VS, UV.y); // Left Edge
	const float3 RightPoint = lerp(TRCorner_VS, BRCorner_VS, UV.y); // Right Edge
	const float3 farPoint = lerp(LeftPoint, RightPoint, UV.x);

	POS_VS = farPoint * D;
	V = normalize(farPoint);
}

float3 GetWorldSpacePosition(float2 UV, float D)
{
	const float3 V = GetViewSpacePosition(UV, D);

	return mul(ViewI, float4(V, 1));
}

void GetWorldSpacePositionAndViewDir(float2 UV, float D, out float3 POS_WS, out float3 VWS)
{
	GetViewSpacePOSanDIR(UV, D, POS_WS, VWS);

	POS_WS = mul(ViewI, float4(POS_WS, 1));
	VWS = mul(ViewI, float4(VWS, 1));
}

[[fk::CBV(binding=1)]]
{
	float4	 Albedo;
	float    Ks;
	float    IOR;
	float    Roughness;
	float    Anisotropic;
	float    Metallic;
	uint     textureCount;
	uint     textureChannels;
}

[[fk::PushConstants(num=20)]] 
{
	float4x4	WT;
	float		LightCount;
	float		t;
	uint2		WH;
};

[[fk::Texture2DArray(id=textures, offset=0, set=0, type=float4)]] 


sampler BiLinear		: register(s0); 
sampler NearestPoint	: register(s1); // Nearest point


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

Forward_VS_OUT Forward_VS(Vertex In, uint ID : SV_VertexID)
{
	const float3 POS_WS = mul(WT, float4(In.POS, 1));
	const float3 POS_VS = mul(View, float4(POS_WS, 1));

	Forward_VS_OUT Out;
	Out.depth		= -POS_VS.z / MaxZ;
	Out.POS			= mul(PV, float4(POS_WS, 1));
	Out.Normal		= normalize(mul(View, mul(WT, float4(In.Normal, 0.0f))));
	Out.Tangent		= normalize(mul(View, mul(WT, float4(In.Tangent, 0.0f))));
	Out.Bitangent	= cross(Out.Tangent, Out.Normal);
	Out.UV			= In.UV;
	
	return Out;
}


/************************************************************************************************/

/*
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

//StructuredBuffer<float4x4> Poses : register(t7);
//[[fk::StructuredBuffer(id=poses, binding=0, set=1, type=float4)

Forward_VS_OUT ForwardSkinned_VS(VertexSkinned In)
{
	float4 P = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 N = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 T = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 W = float4(In.Weights.xyz, 1 - In.Weights.x - In.Weights.y - In.Weights.z);

	[unroll(4)]
	for (uint I = 0; I < 4; ++I)
	{
		P += mul(poses[In.Indices[I]], float4(In.POS, 1)) * W[I];
		N += mul(poses[In.Indices[I]], float4(In.Normal, 0)) * W[I];
		T += mul(poses[In.Indices[I]], float4(In.Tangent, 0)) * W[I];
	}

	const float3 POS_WS = mul(WT, float4(P.xyz, 1));
	const float3 POS_VS = mul(View, float4(POS_WS, 1));

	Forward_VS_OUT Out;
	Out.depth       = -POS_VS.z / MaxZ;
	Out.POS		    = mul(PV, float4(POS_WS, 1));
	Out.Normal      = normalize(mul(View, mul(WT, float4(N.xyz, 0.0f))));
	Out.Tangent     = normalize(mul(View, mul(WT, float4(T.xyz, 0.0f))));
	Out.Bitangent   = cross(Out.Tangent, Out.Normal);
	Out.UV		    = In.UV;

	return Out;
}
*/

float4 DepthPass_VS(float3 POS : POSITION) : SV_POSITION
{
	return mul(PV, mul(WT, float4(POS, 1)));
}


/************************************************************************************************/


struct Forward_PS_IN
{
	centroid	float4  POS         : SV_POSITION;
				float   depth       : DEPTH;
				float3	Normal	    : NORMAL;
				float3	Tangent	    : TANGENT;
				float2	UV		    : TEXCOORD;
				float3	Bitangent	: BITANGENT;
};

struct Deferred_OUT
{
	float4 Albedo   : SV_TARGET0;
	float4 MRIA     : SV_TARGET1;
	float2 Normal   : SV_TARGET2;

	float Depth : SV_DepthLessEqual;
};


float4 SampleVirtualTexture(Texture2D source, in sampler textureSampler, in float2 UV)
{
	const float4 MIPColors[4] = 
	{
		float4(1, 1, 1, 1),
		float4(1, 0, 0, 1),
		float4(0, 1, 0, 1),
		float4(0, 0, 1, 1),
	};


	float MIPCount		= 1;
	float width			= 0;
	float height		= 0;
	source.GetDimensions(0u, width, height, MIPCount);

	float mip = source.CalculateLevelOfDetail(textureSampler, UV);
	uint state1;

	float4 texel = source.Sample(textureSampler, UV, 0, 0.0, state1);

	if (CheckAccessFullyMapped(state1))
		return texel;

	mip = floor(mip);

	while(mip < MIPCount)
	{
		float4 texel = source.SampleLevel(textureSampler, UV, trunc(mip), 0, state1);

		if(CheckAccessFullyMapped(state1))
			return texel;

		mip += 1;
	}
	
	return float4(1.0f, 0.0f, 1.0f, 0.0f); // NO PAGES LOADED!
}


float4 VirtualTextureDebug(Texture2D source, in sampler textureSampler, in float2 UV)
{
	float MIPCount		= 1;
	float width			= 0;
	float height		= 0;
	source.GetDimensions(0u, width, height, MIPCount);

	float4 colors[] = {
		float4(0.25f, 0.25f, 0.25f, 1),
		float4(0.5f, 0.5f, 0.5f, 1),
		float4(1.0f, 1.0f, 0.0f, 1),
		float4(0.0f, 1.0f, 1.0f, 1),
		float4(0.0f, 0.0f, 1.0f, 1),
		float4(1.0f, 0.0f, 1.0f, 1),
		float4(0.75f, 0.75f, 0.75f, 1),
		float4(1.0f, 1.0f, 1.0f, 1),
	};

	int mip = 0;
	while (mip < MIPCount)
	{
		uint state1;
		float4 texel = source.SampleLevel(textureSampler, UV, trunc(mip), 0, state1);

		if (CheckAccessFullyMapped(state1))
			return colors[mip % 8];

		mip += 1;
	}

	return float4(1, 0, 1, 0);
}

/************************************************************************************************/

#define ALBEDO 0x01
#define NORMAL 0x02
#define ROUGHNESS 0x04

/************************************************************************************************/


float2 SignNotZero(float2 v)
{
	return float2((v.x >= 0.0) ? +1.0 : -1.0, (v.y >= 0.0) ? +1.0 : -1.0);
}


float2 Encode(float3 v)
{
	float2 p = v.xy * (1.0 / (abs(v.x) + abs(v.y) + abs(v.z)));
	return (v.z <= 0.0) ? ((1.0 - abs(p.yx)) * SignNotZero(p)) : p;
}


/************************************************************************************************/

[[fk::RootSignature(id=rootsig1)]]
Deferred_OUT GBufferFill_PS(Forward_PS_IN IN)
{
	Deferred_OUT gbuffer;

	uint textureIdx = 0;

	float4 roughMetal;

	if ((textureChannels & ALBEDO) != 0x00)
		gbuffer.Albedo = SampleVirtualTexture(textures[NonUniformResourceIndex(textureIdx++)], BiLinear, IN.UV);
	else
		gbuffer.Albedo = Albedo;

	if ((textureChannels & ROUGHNESS) != 0x00)
		roughMetal = SampleVirtualTexture(textures[NonUniformResourceIndex(textureIdx++)], BiLinear, IN.UV);
	else
		roughMetal = float4(Metallic, Roughness, IOR, Anisotropic);

	if((textureChannels & NORMAL) != 0x00 && false)
	{
		const float3 normalSample		= SampleVirtualTexture(textures[NonUniformResourceIndex(textureIdx++)], BiLinear, IN.UV).xyz;
		const float3 normalCorrected	= float3(normalSample.x, normalSample.y, normalSample.z);
		const float3 biTangent			= normalize(IN.Bitangent);
		float3x3 inverseTBN				= float3x3(normalize(IN.Tangent), normalize(biTangent), normalize(IN.Normal));
		float3x3 TBN					= transpose(inverseTBN);
		const float3 normal				= mul(TBN, normalCorrected * 2.0f - 1.0f);
		gbuffer.Normal					= Encode(normal.xyz);
	}
	else
		gbuffer.Normal = Encode(IN.Normal);

	gbuffer.MRIA	= roughMetal;
	gbuffer.Depth	= IN.depth;

	return gbuffer;
}


/**********************************************************************

Copyright (c) 2015 - 2023 Robert May

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
