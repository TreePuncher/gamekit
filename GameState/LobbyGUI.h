/**********************************************************************

Copyright (c) 2018 Robert May

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

#ifndef LOBBYGUI_H_INCLUDED
#define LOBBYGUI_H_INCLUDED


#include "..\coreutilities\EngineCore.h"
#include "..\graphicsutilities\FrameGraph.h"
#include "..\graphicsutilities\GuiUtilities.h"
#include "..\graphicsutilities\Fonts.h"


/************************************************************************************************/


typedef size_t LobbyRowID;

struct LobbyScreenDrawDesc
{
	FlexKit::VertexBufferHandle		vertexBuffer;
	FlexKit::VertexBufferHandle		textBuffer;
	FlexKit::ConstantBufferHandle	constantBuffer;
	FlexKit::TextureHandle			renderTarget;
	FlexKit::iAllocator*			allocator;
};


/************************************************************************************************/


class MultiplayerLobbyScreen
{
public:
	MultiplayerLobbyScreen(FlexKit::iAllocator* memory, FlexKit::SpriteFontAsset* IN_font);

	void Update	(double dt, FlexKit::WindowInput& input, FlexKit::float2 pixelSize, FlexKit::iAllocator* allocator);
	void Draw	(LobbyScreenDrawDesc& desc, FlexKit::UpdateDispatcher& Dispatcher, FlexKit::FrameGraph& frameGraph);

	void ClearRows		();
	void CreateRow		(LobbyRowID id);
	void SetPlayerName	(LobbyRowID id, const char*	name	);
	void SetPlayerReady	(LobbyRowID id, bool		ready	);

	struct Row
	{
		LobbyRowID			id;
		FlexKit::GUIButton* name;
		FlexKit::GUIButton* ready;
	};

	FlexKit::Vector<Row>		rows;

	FlexKit::GuiSystem			gui;
	FlexKit::GUIGrid*			playerList;
	FlexKit::GUIGrid*			playerScreen;
	FlexKit::SpriteFontAsset*	font;
};


/************************************************************************************************/
#endif