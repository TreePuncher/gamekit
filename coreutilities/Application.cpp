#include "Application.h"

namespace FlexKit
{
	FKApplication::FKApplication(uint2 WindowResolution, EngineMemory* IN_Memory, size_t threadCount) :
		Memory		{ IN_Memory },
		Core		{ IN_Memory, WindowResolution, threadCount },
		framework	{ Core } {}


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
            Core.RenderSystem.WaitforGPU();
			framework.Release();
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

		double T				= 0.0f;
		double FPSTimer			= 0.0;
		double CodeCheckTimer	= 0.0f;
		double dT				= 1.0/60.0;


		while (!Core.End && !Core.Window.Close && framework.subStates.size())
		{
            FK_LOG_2("BEGIN FRAME");

			Core.Time.Before();

			const auto frameStart = std::chrono::high_resolution_clock::now();
			CodeCheckTimer += dT;
			FPSTimer += dT;

			framework.DrawFrame(dT);

			const auto frameEnd         = std::chrono::high_resolution_clock::now();
			const auto updateDuration   = frameEnd - frameStart;
            const auto desiredFrameTime = std::chrono::microseconds(1000000) / 60;

            if (Core.FrameLock)// FPS Locked
            {
                auto sleepTime  = desiredFrameTime - updateDuration;
                auto timePointA = std::chrono::high_resolution_clock::now();

                // Give up timeslice for 1ms increments
                while (true)
                {
                    const auto timepointB        = std::chrono::high_resolution_clock::now();
                    const auto durationRemaining = sleepTime - (timepointB - timePointA);

                    if (durationRemaining < 1.ms)
                        break;

                    std::this_thread::sleep_for(1ms);
                }
                // Spin for remaining time
                while (true)
                {
                    const auto timepointB   = std::chrono::high_resolution_clock::now();
                    const auto duration     = timepointB - frameStart;

                    if (duration > desiredFrameTime)
                        break;
                }
            }

			const auto sleepEnd      = std::chrono::high_resolution_clock::now();
			const auto totalDuration = chrono::duration_cast<chrono::nanoseconds>(sleepEnd - frameStart);

			dT = double(totalDuration.count() ) / 1000000000.0;
            T += dT;

			Core.Time.After();
			Core.Time.Update();

		}
			// End Update  -----------------------------------------------------------------------------------------
	}


}	/************************************************************************************************/

/**********************************************************************

Copyright (c) 2015 - 2019 Robert May

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
