/**********************************************************************

Copyright (c) 2015 - 2018 Robert May

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

#ifndef DEFAULTPIPELINESTATES
#define DEFAULTPIPELINESTATES

#include "graphics.h"
#include "PipelineState.h"
#include "ResourceHandles.h"


namespace FlexKit
{
	/************************************************************************************************/


	inline constexpr PSOHandle TILEDSHADING_SHADE			= PSOHandle(GetTypeGUID(TILEDSHADING_SHADE));
	inline constexpr PSOHandle TILEDSHADING_LIGHTPREPASS	= PSOHandle(GetTypeGUID(TILEDSHADING_LIGHTPREPASS));
	inline constexpr PSOHandle TILEDSHADING_COPYOUT			= PSOHandle(GetTypeGUID(TILEDSHADING_COPYOUT));
	inline constexpr PSOHandle OCCLUSION_CULLING			= PSOHandle(GetTypeGUID(OCCLUSION_CULLING));
	inline constexpr PSOHandle SSREFLECTIONS				= PSOHandle(GetTypeGUID(SSREFLECTIONS));
	inline constexpr PSOHandle DRAW_PSO						= PSOHandle(GetTypeGUID(DRAW_PSO));
	inline constexpr PSOHandle DRAW_TEXTURED_PSO			= PSOHandle(GetTypeGUID(DRAW_TEXTURED_PSO));
	inline constexpr PSOHandle DRAW_TEXTURED_DEBUG_PSO		= PSOHandle(GetTypeGUID(DRAW_TEXTURED_DEBUG_PSO));
	inline constexpr PSOHandle DRAW_3D_PSO					= PSOHandle(GetTypeGUID(DRAW_3D_PSO));
	inline constexpr PSOHandle DRAW_LINE_PSO				= PSOHandle(GetTypeGUID(DRAW_LINE_PSO));
	inline constexpr PSOHandle DRAW_LINE3D_PSO				= PSOHandle(GetTypeGUID(DRAW_LINE3D_PSO));
	inline constexpr PSOHandle DRAW_SPRITE_TEXT_PSO			= PSOHandle(GetTypeGUID(DRAW_SPRITE_TEXT_PSO));
	inline constexpr PSOHandle CLEARRENDERTARGET_RG32		= PSOHandle(GetTypeGUID(CLEARRENDERTARGET_RG32));

	inline constexpr PSOHandle DRAW_TRI3D_PSO				= PSOHandle(GetTypeGUID(DRAW_TRI3D_PSO));

	ID3D12PipelineState* CreateDrawTriStatePSO		(RenderSystem* RS);
	ID3D12PipelineState* CreateTexturedTriStatePSO	(RenderSystem* RS);
	ID3D12PipelineState* CreateDrawLineStatePSO		(RenderSystem* RS);
	ID3D12PipelineState* CreateDraw2StatePSO		(RenderSystem* RS);

	ID3D12PipelineState* CreateDrawTri3DStatePSO	(RenderSystem* RS);


	ID3D12PipelineState* LoadClearRenderTarget_RG32 (RenderSystem* renderSystem);


	/************************************************************************************************/
	// TODO: move this to a frame graph function


	//FLEXKITAPI OcclusionCuller	CreateOcclusionCuller	( RenderSystem* RS, size_t Count, uint2 OcclusionBufferSize, bool UseFloat = true );
	//FLEXKITAPI void				OcclusionPass			( RenderSystem* RS, PVS* Set, OcclusionCuller* OC, ID3D12GraphicsCommandList* CL, GeometryTable* GT, Camera* C );
}

#endif
