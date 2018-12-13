/**********************************************************************

Copyright (c) 2014-2018 Robert May

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


#ifdef _WIN32
#pragma once
#endif


#ifndef WORLDRENDER_H_INCLUDED
#define WORLDRENDER_H_INCLUDED


#include "../buildsettings.h"
#include "../graphicsutilities/FrameGraph.h"
#include "../graphicsutilities/graphics.h"
#include "../graphicsutilities/CoreSceneObjects.h"

namespace FlexKit
{	/************************************************************************************************/


	const PSOHandle FORWARDDRAW				= PSOHandle(GetTypeGUID(FORWARDDRAW));
	const PSOHandle FORWARDDRAWINSTANCED	= PSOHandle(GetTypeGUID(FORWARDDRAWINSTANCED));
	const PSOHandle FORWARDDRAW_OCCLUDE		= PSOHandle(GetTypeGUID(FORWARDDRAW_OCCLUDE));

	ID3D12PipelineState* CreateForwardDrawPSO			(RenderSystem* RS);
	ID3D12PipelineState* CreateForwardDrawInstancedPSO	(RenderSystem* RS);
	ID3D12PipelineState* CreateOcclusionDrawPSO			(RenderSystem* RS);

	struct WorldRender_Targets
	{
		TextureHandle RenderTarget;
		TextureHandle DepthTarget;
	};


	struct ObjectDrawState
	{
		bool transparent	: 1;
		bool posed			: 1;
	};


	struct ObjectDraw
	{
		TriMeshHandle	mesh;
		TriMeshHandle	occluder;
		ObjectDrawState states;
		byte*			constantBuffers[4];
	};


	/************************************************************************************************/


	class FLEXKITAPI WorldRender
	{
	public:
		WorldRender(iAllocator* Memory, RenderSystem* RS_IN) :
			RS(RS_IN),
			ConstantBuffer		{ RS->CreateConstantBuffer(64 * MEGABYTE, false) },
			OcclusionBuffer		{ RS->CreateDepthBuffer({1024, 1024}, true) },
			OcclusionCulling	{ false },
			OcclusionQueries	{ RS->CreateOcclusionBuffer(4096) }
		{
			RS_IN->RegisterPSOLoader(FORWARDDRAW,			{ &RS_IN->Library.RS4CBVs4SRVs, CreateForwardDrawPSO,			});
			RS_IN->RegisterPSOLoader(FORWARDDRAWINSTANCED,	{ &RS_IN->Library.RS4CBVs4SRVs, CreateForwardDrawInstancedPSO	});
			RS_IN->RegisterPSOLoader(FORWARDDRAW_OCCLUDE,	{ &RS_IN->Library.RS4CBVs4SRVs, CreateOcclusionDrawPSO			});

			RS_IN->QueuePSOLoad(FORWARDDRAW);
			RS_IN->QueuePSOLoad(FORWARDDRAWINSTANCED);
		}

		~WorldRender()
		{
			RS->ReleaseCB(ConstantBuffer);
		}

		void DefaultRender				(PVS& Objects, CameraHandle Camera, WorldRender_Targets& Target, FrameGraph& Graph, iAllocator* Memory);
		void RenderDrawabledPBR_Forward	(PVS& Objects, CameraHandle Camera, WorldRender_Targets& Target, FrameGraph& Graph, iAllocator* Memory);

		void ClearGBuffer				(FrameGraph& Graph);
		void RenderDrawabledPBR_Main	(PVS& Objects, FrameGraph& Graph);
		void RenderDrawabledPBR_Shadow	(PVS& Objects, FrameGraph& Graph);
		void ShadePBRPass				(FrameGraph& Graph);

	private:
		RenderSystem*			RS;
		ConstantBufferHandle	ConstantBuffer;
		QueryBufferHandle		OcclusionQueries;
		TextureHandle			OcclusionBuffer;
		uint2					WH;// Output Size

		bool OcclusionCulling;
		// GBuffer
		//RenderTargetHandle		Albedo;
		//RenderTargetHandle		Color;
		//RenderTargetHandle		Normal;
		//RenderTargetHandle		Depth;
	};


}	/************************************************************************************************/

#endif
