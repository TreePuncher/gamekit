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

#include "..\Application\GameFramework.h"
#include "..\Application\GameMemory.h"

class BaseState : public FlexKit::FrameworkState
{
public:
	BaseState(	
		GameFramework*			Framework,
		FlexKit::FKApplication* IN_App	) :
			App				{IN_App},
			FrameworkState	{Framework},
			DepthBuffer		{Framework->Core->RenderSystem.CreateDepthBuffer({ 1920, 1080 },	true)},
			VertexBuffer	{Framework->Core->RenderSystem.CreateVertexBuffer(8096 * 64,		false)},
			TextBuffer		{Framework->Core->RenderSystem.CreateVertexBuffer(8096 * 64,		false)},
			ConstantBuffer	{Framework->Core->RenderSystem.CreateConstantBuffer(8096 * 2000,	false)},

			Render	{
				Framework->Core->GetTempMemory(),
				Framework->Core->RenderSystem}
	{
		InitiateCameraTable(Framework->Core->GetBlockMemory());
		InitiateOrbitCameras(Framework->Core->GetBlockMemory());
	}

	~BaseState()
	{
		Framework->Core->RenderSystem.ReleaseVB(VertexBuffer);
		Framework->Core->RenderSystem.ReleaseVB(TextBuffer);
		Framework->Core->RenderSystem.ReleaseCB(ConstantBuffer);

		ReleaseOrbitCameras(Framework->Core->GetBlockMemory());
		ReleaseCameraTable();

		std::cout << "TESTING\n";
		// TODO: Release this Depth Buffer
		//Framework->Core->RenderSystem.ReleaseDp
	}


	FlexKit::FKApplication* App;

	WorldRender				Render;
	TextureHandle			DepthBuffer;
	VertexBufferHandle		VertexBuffer;
	VertexBufferHandle		TextBuffer;
	ConstantBufferHandle	ConstantBuffer;
};