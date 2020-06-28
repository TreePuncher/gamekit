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

#include "buildsettings.h"

#include <Windows.h>
#include <iostream>

#include "Application.h"

#if USING(DEBUGALLOCATOR)
#include <vld.h>
#endif



/************************************************************************************************/


class TestState : public FlexKit::FrameworkState
{
public:
	TestState(GameFramework* FW) :
		FrameworkState(FW)
	{}
};


int main( int argc, char* argv[] )
{
	FlexKit::FKApplication App;

	for (size_t I = 0; I < argc; ++I)
		App.PushArgument(argv[I]);

	App.SetInitialState<TestState>();
	App.Run();

	return 0;
}


/************************************************************************************************/