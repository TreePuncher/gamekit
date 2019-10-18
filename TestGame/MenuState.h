#ifndef MENUSTATE_H
#define MENUSTATE_H

/**********************************************************************

Copyright (c) 2019 Robert May

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
#include "..\coreutilities\CameraUtilities.h"

using FlexKit::iAllocator;
using FlexKit::FrameworkState;
using FlexKit::GameFramework;
using FlexKit::EngineCore;

struct MenuState;

struct CBArguements
{
	EngineCore*		Engine;
	MenuState*		State;
};


struct MenuState : public FrameworkState
{
	MenuState(GameFramework* framework, iAllocator* Memory) : 
		FrameworkState	{ framework },
		UISystem		{ FlexKit::GuiSystem_Desc{}, Memory }
	{
	}

	MenuState(MenuState& rhs) :
		FrameworkState	(std::move(rhs.framework)),
		UISystem		(std::move(rhs.UISystem))
	{
		CursorSize		= rhs.CursorSize;
	}


	FlexKit::float2			CursorSize;
	FlexKit::GuiSystem		UISystem;

	double T;

	CBArguements CBArgs;
};


MenuState* CreateMenuState(GameFramework* framework, EngineCore* Engine);


struct JoinScreen : public FrameworkState
{
	JoinScreen(GameFramework* framework, iAllocator* Memory) : 
		FrameworkState	{ framework },
		UISystem		{ FlexKit::GuiSystem_Desc{}, Memory }
	{
		memset(Name, '\0', 32);
		memset(Server, '\0', 64);
	}

	JoinScreen(JoinScreen&& rhs) : 
		FrameworkState	{rhs.framework},
		UISystem		{std::move(rhs.UISystem)}
	{
		CursorSize = rhs.CursorSize;
	}

	FlexKit::float2			CursorSize;
	FlexKit::GuiSystem		UISystem;

	char	Name	[32];
	char	Server	[64];

	enum SubState
	{
		ENTERSERVERINFO,
		ENTERNAMEINFO,
		NONE
	}State;

	CBArguements CBArgs;
};


JoinScreen* CreateJoinScreenState(GameFramework* framework, EngineCore* Engine);


#endif