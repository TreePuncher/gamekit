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

#include "../graphicsutilities/PipelineState.h"

namespace FlexKit
{
	const PSOHandle TERRAIN_DRAW_PSO				= PSOHandle(GetTypeGUID(TERRAIN_DRAW_PSO));
	const PSOHandle TERRAIN_DRAW_PSO_DEBUG			= PSOHandle(GetTypeGUID(TERRAIN_DRAW_PSO_DEBUG));
	const PSOHandle TERRAIN_DRAW_WIRE_PSO			= PSOHandle(GetTypeGUID(TERRAIN_DRAW_WIRE_PSO));
	const PSOHandle TERRAIN_CULL_PSO				= PSOHandle(GetTypeGUID(TERRAIN_CULL_PSO));
	const PSOHandle TILEDSHADING_SHADE				= PSOHandle(GetTypeGUID(TILEDSHADING_SHADE));
	const PSOHandle TILEDSHADING_LIGHTPREPASS		= PSOHandle(GetTypeGUID(TILEDSHADING_LIGHTPREPASS));
	const PSOHandle TILEDSHADING_COPYOUT			= PSOHandle(GetTypeGUID(TILEDSHADING_COPYOUT));
	const PSOHandle OCCLUSION_CULLING				= PSOHandle(GetTypeGUID(OCCLUSION_CULLING));
	const PSOHandle SSREFLECTIONS					= PSOHandle(GetTypeGUID(SSREFLECTIONS));
	const PSOHandle DRAW_PSO						= PSOHandle(GetTypeGUID(DRAW_PSO));
	const PSOHandle DRAW_3D_PSO						= PSOHandle(GetTypeGUID(DRAW_3D_PSO));
	const PSOHandle DRAW_LINE_PSO					= PSOHandle(GetTypeGUID(DRAW_LINE_PSO));
	const PSOHandle DRAW_LINE3D_PSO					= PSOHandle(GetTypeGUID(DRAW_LINE3D_PSO));
	const PSOHandle DRAW_SPRITE_TEXT_PSO			= PSOHandle(GetTypeGUID(DRAW_SPRITE_TEXT_PSO));


	ID3D12PipelineState* CreateDrawTriStatePSO	(RenderSystem* RS);
	ID3D12PipelineState* CreateDrawLineStatePSO	(RenderSystem* RS);
	ID3D12PipelineState* CreateDraw2StatePSO	(RenderSystem* RS);

}

#endif