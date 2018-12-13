/**********************************************************************

Copyright (c) 2018 Robert May

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


#define _CRT_SECURE_NO_WARNINGS

#include "..\buildsettings.h"
#include "..\coreutilities\Application.h"
#include "..\coreutilities\Logging.h"
#include "..\coreutilities\Logging.cpp"

#include "GraphicsTest.hpp"


using namespace FlexKit;

int main(int argc, char* argv[])
{
	InitLog(argc, argv);
	SetShellVerbocity(FlexKit::Verbosity_1);
	AddLogFile("GraphicsTests.log", FlexKit::Verbosity_INFO);

	auto* Memory = CreateEngineMemory();
	FKApplication App({1920, 1080}, Memory);

	InitiateCameraTable(Memory->GetBlockMemory());

	App.PushState<GraphicsTest>();
	App.Run();
	App.Release();

	return 0;
}