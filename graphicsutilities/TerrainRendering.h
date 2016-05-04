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

#include <d3d11.h>
#include <DirectXMath.h>


/************************************************************************************************/


namespace FlexKit
{
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

		char				SplitCount;
		size_t				OutputBuffer;

		bool				Dirty;
		// V Shaders
		Shader PassThroughShader;
		Shader VShader;
		Shader Visualiser;

		// G Shaders
		Shader GSubdivShader;
		Shader RegionToTri;

		// P Shaders
		Shader PShader;

		StreamOut RegionBuffers[2];

		size_t				InputCount;
		ID3D12Resource*		InputBuffer;
		ID3D12Resource*		UAVCounter;
		ConstantBuffer		ConstantBuffer;
		static_vector<UINT>	Strides;
		static_vector<UINT>	Offsets;

		fixed_vector<ViewableRegion>* Regions;

		float4		Albedo;
		float4		Specular;

		NodeHandle Node;
	};


	/************************************************************************************************/
	

	FLEXKITAPI void CreateTerrain		(RenderSystem* RS, FlexKit::Landscape* out, NodeHandle node);
	FLEXKITAPI void CleanUpTerrain		(SceneNodes* Nodes, Landscape* ls);

	FLEXKITAPI void DrawLandscape		(RenderSystem* RS,  Landscape* ls, Camera* c, size_t splitcount, void* _ptr);
	FLEXKITAPI void UpdateLandscape		(RenderSystem* RS, Landscape* ls, SceneNodes* Nodes, Camera* Camera);


	/************************************************************************************************/

}

#endif