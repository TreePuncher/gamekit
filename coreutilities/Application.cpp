/**********************************************************************

Copyright (c) 2015 - 2018 Robert May

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

#include "Application.h"

namespace FlexKit
{
	FKApplication::FKApplication(uint2 WindowResolution, EngineMemory* IN_Memory) :
		Memory		{ IN_Memory },
		Core		{ IN_Memory, WindowResolution },
		framework	{ &Core }{}


	/************************************************************************************************/


	FKApplication::~FKApplication()
	{
		Release();
	}


	/************************************************************************************************/


	void FKApplication::Release()
	{
		if (Memory)
		{
			framework.Cleanup();
			Core.Release();
			Memory = nullptr;
		}
	}


	/************************************************************************************************/


	void FKApplication::PushArgument(const char* Str)
	{
		Core.CmdArguments.push_back(Str);
	}


	/************************************************************************************************/


	void FKApplication::Run()
	{
		using FlexKit::UpdateInput;

		const double StepSize	= 1 / 60.0f;
		double T				= 0.0f;
		double FPSTimer			= 0.0;
		size_t FPSCounter		= 0;
		double CodeCheckTimer	= 0.0f;
		double dT				= StepSize;

		while (!Core.End && !Core.Window.Close && framework.subStates.size())
		{
			Core.Time.Before();

			auto FrameStart = std::chrono::system_clock::now();
			CodeCheckTimer += dT;
			FPSTimer += dT;
			FPSCounter++;

			//if (T > StepSize)
			{	// Game Tick  -----------------------------------------------------------------------------------
				UpdateInput();

				FK_LOG_9("Frame Begin");

				framework.Update			(dT);
				framework.UpdateFixed		(StepSize);
				framework.UpdatePreDraw		(Memory->GetTempMemory(), dT);
				framework.Draw				(Memory->GetTempMemory());
				framework.PostDraw			(Memory->GetTempMemory(), dT);

				FK_LOG_9("Frame End");

				// Memory -----------------------------------------------------------------------------------
				//Engine->GetBlockMemory().LargeBlockAlloc.Collapse(); // Coalesce blocks
				Memory->GetTempMemory().clear();
				T -= dT;
			}

			auto FrameEnd = std::chrono::system_clock::now();
			auto Duration = chrono::duration_cast<chrono::microseconds>(FrameEnd - FrameStart);

			if (Core.FrameLock)// FPS Locked
				std::this_thread::sleep_for(chrono::microseconds(16000) - Duration);

			FrameEnd = std::chrono::system_clock::now();
			Duration = chrono::duration_cast<chrono::microseconds>(FrameEnd - FrameStart);

			dT = double(Duration.count()) / 1000000.0;

			Core.Time.After();
			Core.Time.Update();

			T += dT;
		}
			// End Update  -----------------------------------------------------------------------------------------
	}


}	/************************************************************************************************/
