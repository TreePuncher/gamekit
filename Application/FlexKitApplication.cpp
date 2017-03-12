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

#include "..\buildsettings.h"
#include "..\coreutilities\memoryutilities.cpp"
#include "..\coreutilities\timeutilities.cpp"

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

void LoadGameCode(GameCode& Code, CodeTable* out, char* DllLocation)
{
	//DeleteFile(L"TestGameStateTemp.dll");
	//CopyFile(L"TestGameState.dll", L"TestGameStateTemp.dll", FALSE);

	wchar_t MBString[512];
	mbstowcs(MBString, DllLocation, 512);
	Code.Lib = LoadLibrary(MBString);

	auto GetStateTable	= reinterpret_cast<GetStateTableFN>(GetProcAddress(Code.Lib,	"GetStateTable"));
	if(GetStateTable)
		GetStateTable(out);
}

bool CheckGameCode(GameCode& Code)
{
	return true;
}

void ReloadGameCode(GameCode& Code, CodeTable* out, char* DllLocation)
{
	FreeLibrary(Code.Lib);
	LoadGameCode(Code, out, DllLocation);
}

void UnloadGameCode(GameCode& Code)
{
	FreeLibrary(Code.Lib);
}


/************************************************************************************************/

void UpdateInput()
{
	MSG  msg;
	while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}


/************************************************************************************************/


void DLLGameLoop(EngineMemory* Engine, void* State, CodeTable* FNTable, GameCode* Code)
{
	using FlexKit::UpdateTransforms;
	using FlexKit::UpdateInput;
	using FlexKit::UpdatePointLightBuffer;
	using FlexKit::PresentWindow;

	const double StepSize = 1 / 60.0f;
	double T  = 0.0f;
	double FPSTimer = 0.0;
	size_t FPSCounter = 0;
	double CodeCheckTimer  = 0.0f;
	double dt = StepSize;// Engine->Time.GetAveragedFrameTime();


	while (!Engine->End && !Engine->Window.Close)
	{
		Engine->Time.Before();

		auto FrameStart = std::chrono::system_clock::now();
		CodeCheckTimer += dt;
		FPSTimer	   += dt;
		FPSCounter++;




		//if (T > StepSize)
		{	// Game Tick  -----------------------------------------------------------------------------------
			::UpdateInput();

			FNTable->Update				(Engine, State, dt);
			FNTable->UpdateFixed		(Engine, StepSize, State);
			FNTable->UpdateAnimations	(Engine,				&Engine->TempAllocator.AllocatorInterface, dt,			State);
			FNTable->UpdatePreDraw		(Engine,				&Engine->TempAllocator.AllocatorInterface, dt,			State);
			FNTable->Draw				(Engine,				&Engine->TempAllocator.AllocatorInterface,				State);
			FNTable->PostDraw			(Engine,				&Engine->TempAllocator.AllocatorInterface, dt,			State);

			if (FPSTimer > 1.0) {
				std::cout << FPSCounter << "\n";
				//std::cout << "Current VRam Usage: " << GetVidMemUsage(Engine->RenderSystem) / MEGABYTE << "MBs\n";
				FPSCounter = 0;
				FPSTimer = 0;
			}

			/*
			if (CodeCheckTimer > 2.0)
			{
				//std::cout << "Checking Code\n";
				CodeCheckTimer = 0;
				//ReloadGameCode(*Code, FNTable);
			}
			*/

			// Memory -----------------------------------------------------------------------------------
			//Engine->BlockAllocator.LargeBlockAlloc.Collapse(); // Coalesce blocks
			Engine->TempAllocator.clear();
			T -= dt;
		}

		auto FrameEnd = std::chrono::system_clock::now();
		auto Duration = chrono::duration_cast<chrono::microseconds>(FrameEnd - FrameStart);

		if(true)// FPS Locked
			std::this_thread::sleep_for(chrono::microseconds(16000) - Duration);

		FrameEnd = std::chrono::system_clock::now();
		Duration = chrono::duration_cast<chrono::microseconds>(FrameEnd - FrameStart);

		dt = double(Duration.count()) / 1000000.0;

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

	char* GameStateLocation = "GameState.dll";

	for (size_t I = 0; I < argc; ++I) {
		char* Str = argv[I];
		if (!strcmp(Str, "-L") && I  + 1 < argc)
			GameStateLocation = argv[I + 1];
	}

	LoadGameCode(Code, &FNTable, GameStateLocation);
	FINALLY
	{
		UnloadGameCode(Code);
	}FINALLYOVER;

	if (Code.Lib)
	{
		EngineMemory* Engine = (EngineMemory*)_aligned_malloc(PRE_ALLOC_SIZE, 0x40);
		if (!FNTable.InitEngine(Engine))
			return -1;


		for (size_t I = 0; I < argc; ++I) {
			Engine->CmdArguments.push_back(argv[I]);
		}

		void* State = FNTable.Init(Engine);

		DLLGameLoop(Engine, State, &FNTable, &Code);

		::_aligned_free(Engine);
	}
	else
		return -1;

	
	return 0;
}


/************************************************************************************************/