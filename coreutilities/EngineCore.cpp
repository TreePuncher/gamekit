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

#include "EngineCore.h"
#include "Transforms.h"

namespace FlexKit
{   /************************************************************************************************/


	bool CreateRenderWindow(EngineCore* Game, uint32_t height, uint32_t width, bool fullscreen = false);


	/************************************************************************************************/


	EngineMemory* CreateEngineMemory()
	{
		bool ThrowAway;
		return CreateEngineMemory(ThrowAway);
	}


	/************************************************************************************************/


	EngineMemory* CreateEngineMemory(bool& Success)
	{
        Success = false;

        auto* Memory = new(_aligned_malloc(sizeof(EngineMemory), 0x40)) EngineMemory{};

		FK_ASSERT(Memory != nullptr, "Memory Allocation Error!");

        if (Memory == nullptr) {
            return nullptr;
        }

        Success = true;
		return Memory;
	}


	/************************************************************************************************/


	void ReleaseEngineMemory(EngineMemory* Memory)
	{
		DEBUGBLOCK(PrintBlockStatus(&Memory->GetBlockMemory()));
		_aligned_free(Memory);
	}


	/************************************************************************************************/


	bool EngineCore::Initiate(EngineMemory* Memory)
	{
		Graphics_Desc	desc	= { 0 };
		desc.Memory			    = GetBlockMemory();
		desc.TempMemory			= GetTempMemory();

        if (!RenderSystem.Initiate(&desc)) {
            FK_LOG_ERROR("Failed to initiate renderSystem");

            return false;
        }

        InitiateAssetTable(GetBlockMemory());

        return true;
	}


	/************************************************************************************************/


	void EngineCore::Release()
	{
		for (auto Arg : CmdArguments)
			GetBlockMemory().free((void*)Arg);

		CmdArguments.Release();
		RenderSystem.Release();

		Threads.Release();

		Memory = nullptr;
	}


	/************************************************************************************************/


	void PushCmdArg(EngineCore* Engine, const char* str)
	{
		Engine->CmdArguments.push_back(str);
	}


}	/************************************************************************************************/
