/**********************************************************************

Copyright (c) 2015 - 2017 Robert May

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

//#include "..\graphicsutilities\graphics.cpp"

#include "..\coreutilities\AllSourceFiles.cpp"
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


void DLLGameLoop(EngineCore* Engine, void* State, CodeTable* FNTable, GameCode* Code)
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

			FNTable->Update				(Engine,								State, dt);
			FNTable->UpdateFixed		(Engine, StepSize,						State);
			FNTable->UpdateAnimations	(Engine, Engine->GetTempMemory(), dt,	State);
			FNTable->UpdatePreDraw		(Engine, Engine->GetTempMemory(), dt,	State);
			FNTable->Draw				(Engine, Engine->GetTempMemory(),		State);
			FNTable->PostDraw			(Engine, Engine->GetTempMemory(), dt,	State);

			/*
			if (CodeCheckTimer > 2.0)
			{
				//std::cout << "Checking Code\n";
				CodeCheckTimer = 0;
				//ReloadGameCode(*Code, FNTable);
			}
			*/

			// Memory -----------------------------------------------------------------------------------
			//Engine->GetBlockMemory().LargeBlockAlloc.Collapse(); // Coalesce blocks
			Engine->GetTempMemory().clear();
			T -= dt;
		}

		auto FrameEnd = std::chrono::system_clock::now();
		auto Duration = chrono::duration_cast<chrono::microseconds>(FrameEnd - FrameStart);

		if(Engine->FrameLock)// FPS Locked
			std::this_thread::sleep_for(chrono::microseconds(16000) - Duration);

		FrameEnd = std::chrono::system_clock::now();
		Duration = chrono::duration_cast<chrono::microseconds>(FrameEnd - FrameStart);

		dt = double(Duration.count()) / 1000000.0;

		Engine->Time.After();
		Engine->Time.Update();

		T += dt;
		// End Update  -----------------------------------------------------------------------------------------
	}
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
		EngineMemory*	Memory	= nullptr;//
		EngineCore*		Core	= nullptr;//&Memory->BlockAllocator.allocate_aligned<EngineCore>(Memory);

		if (!FNTable.InitEngine(Core, Memory))
			return -1;


		for (size_t I = 0; I < argc; ++I) {
			Core->CmdArguments.push_back(argv[I]);
		}

		void* State = FNTable.Init(Core);

		DLLGameLoop(Core, State, &FNTable, &Code);
		FNTable.Cleanup(Core, State);

		::_aligned_free(Memory);
	}
	else
		return -1;

	
	return 0;
}


/************************************************************************************************/