/**********************************************************************

Copyright (c) 2015 Robert May

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

#include "common.hlsl"

/************************************************************************************************/


cbuffer LocalConstants : register( b1 )
{
	float4	 Albedo;    // + roughness
	float4	 Specular;  // + Metal
	float4x4 WT;
	//uint     MatID;
}

cbuffer TextureConstants : register(b2)
{
	uint2 PageDimension;
	uint2 AtlasDimension;
}

struct GBuffer
{
	float4 Albedo	    : SV_TARGET0; // Albedo    + MatID
	float4 Specular	    : SV_TARGET1; // Specular  
	float4 Emissive     : SV_TARGET2; // Emissive 
	float2 RoughMetal   : SV_TARGET3; // Roughness + Metal
	float4 NORMAL 	    : SV_TARGET4; // Normal    + W Depth
	//float4 WPOS 	    : SV_TARGET5;
};




/************************************************************************************************/

Texture2D AlbedoTexture      : register(t0);
Texture2D RandSTexture	     : register(t1);
Texture2D AlbedoPageTable    : register(t2); //
Texture2D RoughnessPageTable : register(t3); //
Texture2D TextureVAtlas      : register(t4); // Global Texture Resource

SamplerState DefaultSampler;

/************************************************************************************************/


GBuffer Write2GBuffer(
	float3  Color,
	int     MATID,
	float3  Spec,
	float   Roughness,
	float   Metal,
	float3  Emissive,
	float3  Normal,
	float3  WPOS,
	float   W )

{
	GBuffer Out;

	Out.Albedo      = float4(Color, MATID);
	Out.Specular    = float4(Spec, 0);
	Out.RoughMetal  = float2(Roughness, Metal);
	Out.Emissive    = float4(Emissive, 0);
	Out.NORMAL      = float4(normalize(Normal), length(WPOS - CameraPOS));
	//Out.WPOS        = float4(WPOS, length(WPOS - CameraPOS));

	return Out;
}


/************************************************************************************************/


float4 DrawTextureCoordinates(PS_IN IN) : SV_Target
{
	float4 Out        = float4(1, 1, 1, 1);
	float2 Coordinate = IN.UV * PageDimension * AtlasDimension;
	float2 Offset     = frac(Coordinate) / PageDimension;
	float2 Page       = (Coordinate - Offset) / (PageDimension * AtlasDimension);

	return float4(Offset, Page);
}

GBuffer DebugPaint(PS_IN IN)
{
}


/************************************************************************************************/


GBuffer PMain(PS_IN IN)
{
	return Write2GBuffer(
		Albedo.xyz, 
		1, 
		Specular.xyz,
		Albedo.w,
		Specular.w,
		float3(0, 0, 0), 
		IN.N, 
		IN.WPOS,
		IN.Depth
	);
}

GBuffer PMainNormalMapped(PS_IN IN)
{
	return Write2GBuffer(
		Albedo.xyz,
		1,
		Specular.xyz,
		Albedo.w,
		Specular.w,
		float3(0, 0, 0),
		IN.N,
		IN.WPOS,
		IN.Depth);
}


/************************************************************************************************/


struct LinePointPS
{
	float4 Colour	: COLOUR;
};

float4 DrawLine(LinePointPS IN) : SV_TARGET
{
	return float4(IN.Colour.xyz, 1);
}


/************************************************************************************************/


float4 DrawRect(RectPoint_PS IN) : SV_TARGET
{
	return IN.Color;
}


float4 DrawRectTextured(RectPoint_PS IN) : SV_TARGET
{
	float4 Sample = float4(0, 0, 0, 1);// AlbedoTexture.Sample(DefaultSampler, float3(IN.UV.xy, 1));
	Sample += float4(0, 1, 1, 1);
	return Sample * IN.Color;
}


/************************************************************************************************/



float4 DrawRectTextured_DEBUGUV(RectPoint_PS IN) : SV_TARGET
{
	//float4 Sample = float4(0, 0, 0, 1);
	float4 Sample = AlbedoTexture.Sample(DefaultSampler, float3(IN.UV.xy, 1));
	//Sample += float4(0, 1, 1, 1);
	return Sample;//  *float4(IN.UV.xy, 1, 1);
}


/************************************************************************************************/


GBuffer TerrainPaint(PS_Colour_IN IN)
{
	return Write2GBuffer(
		IN.Colour,
		0x01,
		float3(1, 1, 1),
		0.5f,
		0.0f,
		float3(0, 1, 0),
		IN.N,
		IN.WPOS,
		length(IN.WPOS - CameraPOS));
}


GBuffer DebugTerrainPaint(PS_Colour_IN IN)
{
	return Write2GBuffer(
		IN.Colour,
		0x01,
		float3(1, 1, 1),
		0.5f,
		0.0f,
		float3(0, 1, 0),
		IN.N,
		IN.WPOS,
		length(IN.WPOS - CameraPOS));
}


GBuffer DebugTerrainPaint_2(PS_Colour_IN IN)
{
	return Write2GBuffer(
		float3(0, 0, 0),
		0x01,
		float3(1, 1, 1),
		0.5f,
		0.0f,
		float3(0, 0, 0),
		IN.N,
		IN.WPOS,
		length(IN.WPOS - CameraPOS));
}


struct PS_TEXURED_IN
{
	float3 WPOS 	: TEXCOORD0;
	float3 N 		: TEXCOORD1;
	float2 UV		: TEXCOORD2;
};


GBuffer PMain_TEXTURED(PS_TEXURED_IN IN )
{
	float3 A 		= AlbedoTexture.Sample(DefaultSampler, IN.UV) * Albedo;
	float3 Spec		= AlbedoTexture.Sample(DefaultSampler, IN.UV) * Specular;
	float2 RM		= RandSTexture.Sample (DefaultSampler, IN.UV);

	return Write2GBuffer(
		A,
		1,
		Spec,
		RM.x,
		RM.y,
		float3(0, 0, 0),
		IN.N,
		IN.WPOS,
		1);
}


/************************************************************************************************/


struct PSIN_Shadow {
	float4 POS_H	: Position;
};

struct ShadowSample {
	float D         : SV_Depth;
};

ShadowSample PMain_ShadowMapping(PSIN_Shadow In) {
	ShadowSample Out;
	Out.D = In.POS_H.w;
	return Out;
}


/************************************************************************************************/


void PMain_FindSamples(PS_TEXURED_IN IN)
{
	IN.UV * PageDimension;
}


/************************************************************************************************/