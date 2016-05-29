/**********************************************************************

Copyright (c) 2015 - 2016 Robert May

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
		void*	PShaderCode;
		size_t	PShaderSize;
	};

	struct Landscape
	{
		struct ViewableRegion
		{
			int32_t xyz[4];
			int32_t unused[3];
			int32_t BitID;
		};

		struct ConstantBufferLayout
		{
			float4		Albedo;
			float4		Specular;
			DirectX::XMMATRIX WT;
		};

		uint16_t			  SplitCount;
		size_t				  OutputBuffer;
		ID3D12PipelineState*  SplitState;
		ID3D12PipelineState*  GenerateState;
		ID3D12PipelineState*  TessellateState;

		StreamOutBuffer			RegionBuffers[2];// Ping Pongs Back and Forth
		QueryResource			SOQuery;

		size_t					InputCount;
		ID3D12Resource*			InputBuffer;
		ID3D12CommandSignature*	CommandSignature;
		ID3D12Resource*			ZeroValues;

		StreamOutBuffer	FinalBuffer;
		StreamOutBuffer	TriBuffer;
		StreamOutBuffer	IndirectOptions1;
		StreamOutBuffer	IndirectOptions2;
		StreamOutBuffer	SOCounter_1;
		StreamOutBuffer	SOCounter_2;
		StreamOutBuffer	FB_Counter;

		ConstantBuffer	ConstantBuffer;

		DynArray<ViewableRegion> Regions;

		float4 Albedo;
		float4 Specular;

		NodeHandle Node;

		// V Shaders
		Shader PassThroughShader;
		Shader VShader;
		Shader Visualiser;

		// G Shaders
		Shader GSubdivShader;
		Shader RegionToTri;

		// P Shaders
		Shader PShader;

		// Tessellation Related Shaders
		Shader HullShader;
		Shader DomainShader;

	};


	/************************************************************************************************/
	

	FLEXKITAPI void InitiateLandscape	( RenderSystem* RS, NodeHandle node, Landscape_Desc* desc, iAllocator* alloc, Landscape* ls );
	FLEXKITAPI void CleanUpTerrain		( SceneNodes* Nodes, Landscape* ls );
	FLEXKITAPI void PushRegion			( Landscape* ls, Landscape::ViewableRegion R );
	FLEXKITAPI void UploadLandscape		( RenderSystem* RS, Landscape* ls, SceneNodes* Nodes, Camera* c, bool UploadRegions = false, bool UploadConstants = true );
	FLEXKITAPI void DrawLandscape		( RenderSystem* RS, Landscape* ls, size_t splitcount, Camera* C );


	/************************************************************************************************/

}

#endif