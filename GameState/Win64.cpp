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

#include "..\coreutilities\Logging.h"
#include "..\buildsettings.h"
#include "..\coreutilities\Application.h"

#include "HostState.cpp"
#include "ClientState.cpp"
#include "MenuState.cpp"
#include "PlayState.cpp"
#include "BaseState.h"
#include "Gameplay.cpp"
#include <iostream>


int main(int argc, char* argv[])
{
	FlexKit::InitLog(argc, argv);
	FlexKit::SetShellVerbocity(FlexKit::Verbosity_1);
	FlexKit::AddLogFile("GameState.log", 
		FlexKit::Verbosity_INFO);

#ifdef _DEBUG
	FlexKit::AddLogFile("GameState_Detailed.log", 
		FlexKit::Verbosity_9, 
		false);
#endif

	FK_LOG_INFO("Logging initialized started.");

	auto* Memory = CreateEngineMemory();
	EXITSCOPE(ReleaseEngineMemory(Memory));

	FlexKit::FKApplication App{ { 1920, 1080 }, Memory };

	for (size_t I = 0; I < argc; ++I)
		App.PushArgument(argv[I]);

	FK_LOG_INFO("Set initial PlayState state.");
	auto& GameBase = App.PushState<BaseState>(&App);
	App.PushState<PlayState>(&GameBase);

	FK_LOG_INFO("Running application...");
	App.Run();
	FK_LOG_INFO("Completed running application");

	FK_LOG_INFO("Started cleanup...");
	App.Release();
	FK_LOG_INFO("Completed cleanup.");

	return 0;
}