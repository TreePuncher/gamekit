/**********************************************************************

Copyright (c) 2015 - 2017 Robert May

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

#ifndef TILEDRENDER_H
#define TILEDRENDER_H

#include "..\buildsettings.h"
#include "graphics.h"

namespace FlexKit
{
	enum EDEFERREDPASSMODE
	{
		EDPM_DEFAULT	= 0,
		EDPM_ALBEDO,
		EDPM_ROUGHNESS,
		EDPM_METAL,
		EDPM_NORMALS,
		EDPM_POSITION,
		EDPM_CONSTRUCTPOSITION,
		EDPM_LIGHTCONTRIBUTION,
		EDPM_COUNT,
	};


	struct DeferredPass_Parameters
	{
		float4x4 InverseZ;
		uint64_t PointLightCount;
		uint64_t SpotLightCount;
		uint2	 WH;
		EDEFERREDPASSMODE Mode;// Only useable in Debug mode
	};


	struct TiledRendering_Desc
	{
		DepthBuffer*	DepthBuffer;
		RenderWindow*	RenderWindow;
		byte* _ptr;
	};


	struct GBufferConstantsLayout
	{
		uint32_t DLightCount;
		uint32_t PLightCount;
		uint32_t SLightCount;
		uint32_t Height;
		uint32_t Width;
		uint32_t DebugRenderEnabled;
		uint32_t Padding[2];
		float4	 AmbientLight;
		//char padding[1019];
	};


	enum DeferredShadingRootParam
	{
		DSRP_DescriptorTable  = 0,
		DSRP_CameraConstants  = 1,
		DSRP_ShadingConstants = 2,
		DSRP_COUNT,
	};


	enum DeferredFillingRootParam
	{
		DFRP_CameraConstants	= 0,
		DFRP_ShadingConstants	= 1,
		DFRP_TextureConstants	= 2,
		DFRP_AnimationResources = 3,
		DFRP_Textures			= 4,
		DFRP_COUNT,
	};


	struct TiledDeferredRender
	{
		// GBuffer
		struct GBuffer
		{
			Texture2D ColorTex;
			Texture2D SpecularTex;
			Texture2D NormalTex;
			//Texture2D PositionTex;
			Texture2D OutputBuffer;
			Texture2D DepthBuffer;
			Texture2D EmissiveTex;
			Texture2D RoughnessMetal;
			Texture2D LightTilesBuffer;
		}GBuffers[3];
		size_t CurrentBuffer;

		// Shading
		struct
		{
			ConstantBuffer			ShaderConstants;
			Shader					Shade;		// Compute Shader
		}Shading;

		// GBuffer Filling
		struct
		{
			ID3D12PipelineState*	PSO;
			ID3D12PipelineState*	PSOTextured;
			ID3D12PipelineState*	PSOAnimated;
			ID3D12PipelineState*	PSOAnimatedTextured;

			ID3D12RootSignature*	FillRTSig;
			Shader					NormalMesh;
			Shader					AnimatedMesh;
			Shader					NoTexture;
			Shader					Textured;
		}Filling;
	};

	FLEXKITAPI void InitiateTiledDeferredRender	( RenderSystem* RenderSystem,  TiledRendering_Desc* GBdesc, TiledDeferredRender* out );
	FLEXKITAPI void TiledRender_LightPrePass	( RenderSystem* RS, PVS* _PVS, TiledDeferredRender* Pass, const Camera* C, const PointLightBuffer* PLB, const SpotLightBuffer* SPLB, uint2 WH);
	FLEXKITAPI void TiledRender_Fill			( RenderSystem* RS, PVS* _PVS, TiledDeferredRender* Pass, Texture2D Target, const Camera* C, TextureManager* TM, GeometryTable* GT, TextureVTable* Texture, OcclusionCuller* OC = nullptr );
	FLEXKITAPI void TiledRender_Shade			( RenderSystem* RS, PVS* _PVS, TiledDeferredRender* Pass, Texture2D Target, const Camera* C, const PointLightBuffer* PLB, const SpotLightBuffer* SPLB );
	FLEXKITAPI void ReleaseTiledRender			( TiledDeferredRender* gb );
	FLEXKITAPI void ClearGBuffer				( RenderSystem* RS, TiledDeferredRender* gb, const float4& ClearColor, size_t Idx );
	FLEXKITAPI void UpdateTiledBufferConstants	( RenderSystem* RS, TiledDeferredRender* gb, size_t PLightCount, size_t SLightCount );
	FLEXKITAPI void UploadDeferredPassConstants	( RenderSystem* RS, DeferredPass_Parameters* in, float4 A, TiledDeferredRender* Pass );
	FLEXKITAPI void	IncrementPassIndex			( TiledDeferredRender* Pass );
	FLEXKITAPI void ClearTileRenderBuffers		( RenderSystem* RS, TiledDeferredRender* );

	FLEXKITAPI void PresentBufferToTarget		();
}

#endif