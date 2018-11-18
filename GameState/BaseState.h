#pragma once



/**********************************************************************

Copyright (c) 2017 Robert May

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

#include "..\coreutilities\GameFramework.h"
#include "..\coreutilities\EngineCore.h"
#include "..\coreutilities\WorldRender.h"
#include "..\graphicsutilities\TextRendering.h"

using FlexKit::WorldRender;
using FlexKit::ReleaseCameraTable;
using FlexKit::FKApplication;


class BaseState : public FlexKit::FrameworkState
{
public:
	BaseState(	
		FlexKit::GameFramework* IN_Framework,
		FlexKit::FKApplication* IN_App	) :
			App				{IN_App},
			FrameworkState	{IN_Framework},
			depthBuffer		{IN_Framework->core->RenderSystem.CreateDepthBuffer({ 1920, 1080 },	true)},
			vertexBuffer	{IN_Framework->core->RenderSystem.CreateVertexBuffer(8096 * 64, false)},
			textBuffer		{IN_Framework->core->RenderSystem.CreateVertexBuffer(8096 * 64, false)},
			constantBuffer	{IN_Framework->core->RenderSystem.CreateConstantBuffer(	8096 * 2000, false)},

			render	{
				IN_Framework->core->GetTempMemory(),
				IN_Framework->core->RenderSystem}
	{
		InitiateCameraTable(framework->core->GetBlockMemory());

		IN_Framework->core->RenderSystem.RegisterPSOLoader(FlexKit::DRAW_SPRITE_TEXT_PSO, FlexKit::LoadSpriteTextPSO);

		IN_Framework->core->RenderSystem.QueuePSOLoad(FlexKit::DRAW_PSO);
		IN_Framework->core->RenderSystem.QueuePSOLoad(FlexKit::DRAW_LINE_PSO);
		IN_Framework->core->RenderSystem.QueuePSOLoad(FlexKit::DRAW_LINE3D_PSO);
		IN_Framework->core->RenderSystem.QueuePSOLoad(FlexKit::DRAW_SPRITE_TEXT_PSO);
	}

	~BaseState()
	{
		framework->core->RenderSystem.ReleaseVB(vertexBuffer);
		framework->core->RenderSystem.ReleaseVB(textBuffer);
		framework->core->RenderSystem.ReleaseCB(constantBuffer);
		framework->core->RenderSystem.ReleaseDB(depthBuffer);

		ReleaseCameraTable();
	}


	FKApplication* App;

	FlexKit::WorldRender				render;
	FlexKit::TextureHandle				depthBuffer;
	FlexKit::VertexBufferHandle			vertexBuffer;
	FlexKit::VertexBufferHandle			textBuffer;
	FlexKit::ConstantBufferHandle		constantBuffer;
};