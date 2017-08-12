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
#include "Client.h"
#include "HostState.h"
#include "MenuState.h"
#include "PlayState.h"


using namespace FlexKit;


struct CodeExports
{
	typedef GameFramework*	(*InitiateGameStateFN)	(EngineCore* Engine);
	typedef void			(*UpdateFixedIMPL)		(EngineCore* Engine,	double dt, GameFramework* _ptr);
	typedef void			(*UpdateIMPL)			(EngineCore* Engine,	GameFramework* _ptr, double dt);
	typedef void			(*UpdateAnimationsFN)	(EngineCore* RS,		iAllocator* TempMemory, double dt, GameFramework* _ptr);
	typedef void			(*UpdatePreDrawFN)		(EngineCore* Engine,	iAllocator* TempMemory, double dt, GameFramework* _ptr);
	typedef void			(*DrawFN)				(EngineCore* RS,		iAllocator* TempMemory,			   GameFramework* _ptr);
	typedef void			(*PostDrawFN)			(EngineCore* Engine,	iAllocator* TempMemory, double dt, GameFramework* _ptr);
	typedef void			(*CleanUpFN)			(EngineCore* Engine,	GameFramework* _ptr);
	typedef void			(*PostPhysicsUpdate)	(GameFramework*);
	typedef void			(*PrePhysicsUpdate)		(GameFramework*);

	InitiateGameStateFN		Init;
	InitiateEngineFN		InitEngine;
	UpdateIMPL				Update;
	UpdateFixedIMPL			UpdateFixed;
	UpdateAnimationsFN		UpdateAnimations;
	UpdatePreDrawFN			UpdatePreDraw;
	DrawFN					Draw;
	PostDrawFN				PostDraw;
	CleanUpFN				Cleanup;
};


extern "C"
{
	GameFramework* InitateState(EngineCore* Engine)
	{
		auto* Framework = InitiateFramework(Engine);

		enum Mode
		{
			Menu, 
			Host,
			Client,
			Play,
		}CurrentMode = Play;

		const char* Name	= nullptr;
		const char* Server	= nullptr;
		

		for (size_t I = 0; I < Engine->CmdArguments.size(); ++I)
		{
			auto Arg = Engine->CmdArguments[I];
			if (!strncmp(Arg, "-H", strlen("-H")))
			{
				CurrentMode = Host;
			}
			else if(!strncmp(Arg, "-C", strlen("-C")) && I + 2 < Engine->CmdArguments.size())
			{
				CurrentMode = Client;
				Name	= Engine->CmdArguments[I + 1];
				Server	= Engine->CmdArguments[I + 2];
			}
		}

		switch (CurrentMode)
		{
		case Menu:{
			auto MenuSubState = CreateMenuState(Framework, Engine);
			PushSubState(Framework, MenuSubState);
		}	break;
		case Host: {
			PushSubState(Framework, &Engine->GetBlockMemory().allocate_aligned<HostState>(Engine, Framework));
		}	break;
		case Client: {
			PushSubState(Framework, &Engine->GetBlockMemory().allocate_aligned<ClientState>(Framework, Engine, Name, Server));
		}	break;
		case Play: {
			auto PlayState = CreatePlayState(Engine, Framework);
			PushSubState(Framework, PlayState);
		}
		default:
			break;
		}

		return Framework;
	}


	GAMESTATEAPI void GetStateTable(CodeTable* out)
	{
		CodeExports* Table = reinterpret_cast<CodeExports*>(out);

		Table->InitEngine		= &InitEngine;
		Table->Init				= &InitateState;
		Table->Update			= &FlexKit::Update;
		Table->UpdateFixed		= &FlexKit::UpdateFixed;
		Table->UpdateAnimations	= &FlexKit::UpdateAnimations;
		Table->UpdatePreDraw	= &FlexKit::UpdatePreDraw;
		Table->Draw				= &FlexKit::Draw;
		Table->PostDraw			= &FlexKit::PostDraw;
		Table->Cleanup			= &FlexKit::Cleanup;
	}
}