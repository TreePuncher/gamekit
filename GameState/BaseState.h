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

using FlexKit::WorldRender;
using FlexKit::ReleaseCameraTable;
using FlexKit::FKApplication;

class BaseState : public FlexKit::FrameworkState
{
public:
	BaseState(	
		GameFramework* IN_Framework,
		FKApplication* IN_App	) :
			App				{IN_App},
			FrameworkState	{IN_Framework},
			DepthBuffer		{IN_Framework->Core->RenderSystem.CreateDepthBuffer({ 1920, 1080 },	true)},
			VertexBuffer	{IN_Framework->Core->RenderSystem.CreateVertexBuffer(8096 * 64, false)},
			TextBuffer		{IN_Framework->Core->RenderSystem.CreateVertexBuffer(8096 * 64, false)},
			ConstantBuffer	{IN_Framework->Core->RenderSystem.CreateConstantBuffer(	8096 * 2000, false)},

			Render	{
				IN_Framework->Core->GetTempMemory(),
				IN_Framework->Core->RenderSystem}
	{
		InitiateCameraTable(Framework->Core->GetBlockMemory());
		InitiateOrbitCameras(Framework->Core->GetBlockMemory());

		IN_Framework->Core->RenderSystem.QueuePSOLoad(FlexKit::DRAW_PSO);
		IN_Framework->Core->RenderSystem.QueuePSOLoad(FlexKit::DRAW_LINE_PSO);
		IN_Framework->Core->RenderSystem.QueuePSOLoad(FlexKit::DRAW_LINE3D_PSO);
		IN_Framework->Core->RenderSystem.QueuePSOLoad(FlexKit::DRAW_SPRITE_TEXT_PSO);
	}

	~BaseState()
	{
		Framework->Core->RenderSystem.ReleaseVB(VertexBuffer);
		Framework->Core->RenderSystem.ReleaseVB(TextBuffer);
		Framework->Core->RenderSystem.ReleaseCB(ConstantBuffer);
		Framework->Core->RenderSystem.ReleaseDB(DepthBuffer);

		ReleaseOrbitCameras(Framework->Core->GetBlockMemory());
		ReleaseCameraTable();
	}


	FKApplication* App;

	WorldRender				Render;
	TextureHandle			DepthBuffer;
	VertexBufferHandle		VertexBuffer;
	VertexBufferHandle		TextBuffer;
	ConstantBufferHandle	ConstantBuffer;
};