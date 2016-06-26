/**********************************************************************

Copyright (c) 2015 - 2016 Robert May

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

/*
TODOs
- INPUT
	- Mouse Input
- NETWORK
	- Nothing
- TEXT RENDERING
	- Do Sprite Text Batcher?
- PHYSICS
	- Add forces
	- Add Kinematics
	- Add Character Kinematic Controller
- TERRAIN SYSTEM
- OBJECT LOADING
	- Move OBJ loader code to use a scratch space instead
- COLLISION DETECTION
- GAMEPLAY
- MEMORY
*/

#pragma warning( disable : 4251 )

#include "stdafx.h"

#include "..\coreutilities\AllSourceFiles.cpp"
#include "..\buildsettings.h"
#include "..\coreutilities\ProfilingUtilities.h"
#include "MultiplayerMode.h"
//#include "TestMode.h"
#include "GameMemory.h"

#include <Windows.h>
#include <iostream>


#if USING(DEBUGALLOCATOR)
#include <vld.h>
#endif


/************************************************************************************************/


struct GameCode
{
	HMODULE Lib;
	HANDLE	File;
};

void LoadGameCode(GameCode& Code, CodeTable* out)
{
	wchar_t DefaultGameState[] = L"TestGameState.dll";
	//DeleteFile(L"TestGameStateTemp.dll");
	//CopyFile(L"TestGameState.dll", L"TestGameStateTemp.dll", FALSE);
	Code.Lib = LoadLibrary(L"TestGameState.dll");

	auto GetStateTable	= reinterpret_cast<GetStateTableFN>(GetProcAddress(Code.Lib,	"GetStateTable"));
	GetStateTable(out);
}

bool CheckGameCode(GameCode& Code)
{
	return true;
}

void ReloadGameCode(GameCode& Code, CodeTable* out)
{
	FreeLibrary(Code.Lib);
	LoadGameCode(Code, out);
}

void UnloadGameCode(GameCode& Code)
{
	FreeLibrary(Code.Lib);
}


/************************************************************************************************/


void DLLGameLoop(EngineMemory* Engine, void* State, CodeTable* FNTable, GameCode* Code)
{
	using FlexKit::UpdateTransforms;
	using FlexKit::UpdateInput;
	using FlexKit::UpdatePointLightBuffer;
	using FlexKit::PresentWindow;

	Engine->Time.Clear();
	Engine->Time.PrimeLoop();

	const double StepSize = 1 / 60.0f;
	double T  = 0.0f;
	double CodeCheckTimer  = 0.0f;

	while (!Engine->End && !Engine->Window.Close)
	{
		Engine->Time.Before();

		double dt = Engine->Time.GetAveragedFrameTime();
		CodeCheckTimer += dt;

		FNTable->Update(Engine, State, dt);
		if (T > StepSize)
		{	// Game Tick  -----------------------------------------------------------------------------------
			UpdateInput();

			FNTable->UpdateFixed		(Engine, StepSize, State);
			FNTable->UpdateAnimations	(Engine->RenderSystem,	&Engine->TempAllocator.AllocatorInterface, StepSize,			State);
			FNTable->UpdatePreDraw		(Engine,				&Engine->TempAllocator.AllocatorInterface, StepSize,			State);
			FNTable->Draw				(Engine->RenderSystem,	&Engine->TempAllocator.AllocatorInterface, &Engine->Materials,	State);
			FNTable->PostDraw			(Engine,										StepSize,								State);

			if (CodeCheckTimer > 2.0)
			{
				//std::cout << "Checking Code\n";
				CodeCheckTimer = 0;
				//ReloadGameCode(*Code, FNTable);
			}

			// Memory -----------------------------------------------------------------------------------
			Engine->BlockAllocator.LargeBlockAlloc.Collapse(); // Coalesce blocks
			Engine->TempAllocator.clear();
		}

		if ( dt < StepSize )
			Sleep( 2 );

		Engine->Time.After();
		Engine->Time.Update();

		T += dt;
		// End Update  -----------------------------------------------------------------------------------------
	}
	// Clean-Up
	FNTable->Cleanup( Engine, State );
}


/************************************************************************************************/


int main( int argc, char* argv[] )
{
	GameCode	Code;
	CodeTable	FNTable;

	LoadGameCode(Code, &FNTable);

	if (Code.Lib)
	{
		EngineMemory* Engine = (EngineMemory*)_aligned_malloc(PRE_ALLOC_SIZE, 0x40);
		FNTable.InitEngine(Engine);

		void* State = FNTable.Init(Engine);

		DLLGameLoop(Engine, State, &FNTable, &Code);

		::_aligned_free(Engine);
	}

	UnloadGameCode(Code);
	return 0;
}


/************************************************************************************************/