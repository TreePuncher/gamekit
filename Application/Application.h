/**********************************************************************
, 1080
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


#ifndef FKAPPLICATION_H
#define FKAPPLICATION_H

#pragma warning( disable : 4251 )

#include "stdafx.h"

#include "..\buildsettings.h"
#include "..\coreutilities\AllSourceFiles.cpp"
#include "FrameworkSourceFiles.cpp"

#include "GameMemory.h"
#include "GameFramework.h"

#include <Windows.h>
#include <iostream>


namespace FlexKit
{
	class FKApplication
	{
	public:
		FKApplication(uint2 WindowResolution = { 1920, 1080 });
		~FKApplication();

		template<typename TY_STATE, typename ... TY_ARGS>
		TY_STATE& PushState(TY_ARGS&& ... ARGS)
		{
			return Framework.PushState<TY_STATE>(std::forward<TY_ARGS>(ARGS)...);
		}

		void Run();
		void Cleanup();

		void PushArgument(const char* Str);

		GameFramework&	GetFramework() { return Framework; }

	private:
		EngineMemory*	Memory;
		EngineCore*		Core;
		GameFramework	Framework;
	};

}

#include "Application.cpp"

#endif