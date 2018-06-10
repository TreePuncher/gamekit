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

#ifndef TERRAINRENDERING
#define TERRAINRENDERING

#include "..\buildsettings.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\MathUtils.h"
#include "..\graphicsutilities\graphics.h"


#include <DirectXMath.h>


/************************************************************************************************/


namespace FlexKit
{
	struct Landscape_Desc
	{
		Texture2D	HeightMap;
	};


	// TODO: update to use Frame Graph System
	struct Landscape
	{
		struct ViewableRegion
		{
			int32_t xyz[4];
			int32_t unused[3];
			int32_t BitID;
			float2  UV_TL; // Top	 Left
			float2  UV_BR; // Bottom Right
			int32_t TerrainInfo[4]; // Texture Index
		};

		struct ConstantBufferLayout
		{
			float4	 Albedo;   // + roughness
			float4	 Specular; // + metal factor
			float2	 RegionDimensions;
			Frustum	 Frustum;
			int      PassCount;
		};

		uint16_t			  SplitCount;
		size_t				  OutputBuffer;

		/*
		ID3D12PipelineState*  SplitState;
		ID3D12PipelineState*  GenerateState;
		ID3D12PipelineState*  GenerateStateDebug;
		ID3D12PipelineState*  WireFrameState;
		*/

		StreamOutBuffer			RegionBuffers[2];// Ping Pongs Back and Forth
		QueryResource			SOQuery;

		size_t					InputCount;
		ID3D12Resource*			InputBuffer;
		ID3D12CommandSignature*	CommandSignature;
		ID3D12Resource*			ZeroValues;

		Texture2D				HeightMap;

		StreamOutBuffer	FinalBuffer;
		StreamOutBuffer	TriBuffer;
		StreamOutBuffer	IndirectOptions1;
		StreamOutBuffer	IndirectOptions2;
		StreamOutBuffer	SOCounter_1;
		StreamOutBuffer	SOCounter_2;
		StreamOutBuffer	FB_Counter;

		ConstantBuffer	ConstantBuffer;

		Vector<ViewableRegion> Regions;

		float4 Albedo;
		float4 Specular;

		NodeHandle Node;
	};


	/************************************************************************************************/
	

	FLEXKITAPI void InitiateLandscape	( RenderSystem* RS, NodeHandle node, Landscape_Desc* desc, iAllocator* alloc, Landscape* ls );
	FLEXKITAPI void ReleaseTerrain		( Landscape* ls );
	FLEXKITAPI void PushRegion			( Landscape* ls, Landscape::ViewableRegion R );
	FLEXKITAPI void UploadLandscape		( RenderSystem* RS, Landscape* ls, Camera* c, bool UploadRegions = false, bool UploadConstants = true, int PassCount = 12 );
	FLEXKITAPI void UploadLandscape2	( RenderSystem* RS, Landscape* ls, Camera* c, Frustum F, bool UploadRegions = false, bool UploadConstants = true, int PassCount = 12 );
	//FLEXKITAPI void DrawLandscape		( RenderSystem* RS, Landscape* ls, TiledDeferredRender* PS, size_t splitcount, Camera* C, bool DrawWireframe = false );


	/************************************************************************************************/

}

#endif