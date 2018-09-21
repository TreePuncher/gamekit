#pragma once

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


#ifndef MAINMENU_H_INCLUDED
#define MAINMENU_H_INCLUDED

#include "..\coreutilities\GameFramework.h"
#include "..\graphicsutilities\GuiUtilities.h"

#include "BaseState.h"

class MainMenu : public FlexKit::FrameworkState
{
public:
	MainMenu(
		FlexKit::GameFramework*		IN_Framework,
		BaseState*					IN_Base	);

	bool Update			(FlexKit::EngineCore* Engine, FlexKit::UpdateDispatcher& Dispatcher, double dT) override;
	bool Draw			(FlexKit::EngineCore* Engine, FlexKit::UpdateDispatcher& Dispatcher, double dT, FlexKit::FrameGraph& Graph) override;
	bool PostDrawUpdate	(FlexKit::EngineCore* Core, FlexKit::UpdateDispatcher& Dispatcher, double dT, FlexKit::FrameGraph& Graph) override;

private:
	FlexKit::ConstantBufferHandle	constantBuffer;
	FlexKit::VertexBufferHandle		vertexBuffer;
	FlexKit::VertexBufferHandle		textBuffer;


	FlexKit::GuiSystem	gui;
	FlexKit::GUIGrid*	menuGrid;
	FlexKit::GUIButton*	menuQuitButton;
	FlexKit::GUIButton*	menuJoinButton;
	FlexKit::GUIButton*	menuHostButton;
};

#endif