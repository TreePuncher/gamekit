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

#include "..\buildsettings.h"
#include "..\coreutilities\Logging.h"
#include "..\coreutilities\Application.h"
#include "..\graphicsutilities\AnimationComponents.h"

#include "BaseState.h"
#include "client.cpp"
#include "Gameplay.cpp"
#include "host.cpp"
#include "lobbygui.cpp"
#include "MainMenu.cpp"
#include "MultiplayerState.cpp"
#include "MultiplayerGameState.cpp"
#include "PlayState.cpp"

#include <iostream>


int main(int argc, char* argv[])
{
	bool multiplayerMode	= false;
	bool host				= true;
	bool quit				= false;
	std::string name;
	std::string server;


	[&]
	{
		while (true)
		{
			std::cout << "Main Menu: \n1 - Play State\n2 - Multiplayer State\n3 - Quit\n";
			int x = 0;
			std::cin >> x;

			switch (x)
			{
				case 1:
				multiplayerMode = false;
				host = false;
				return;
				case 2: 
				{
					multiplayerMode = true;
					std::cout << "Multiplayer menu\n1 - Host\n2 - Join\n";
					std::cin >> x;
					switch (x)
					{
					case 1:
						std::cout << "hosting game\n";
						host	= true;
						return;
					case 2:
					{
						std::cout << "Joining game\nEnter name: ";
						std::cin >> name;
						std::cout << "Enter server address: ";
						std::cin >> server;

						name.push_back('\0'); 
						// TODO: better input scrubbing
						if (!name.size() && !server.size()) // if they entered invalid inputs
							continue;						// Goes back to main menu to try again

						multiplayerMode		= true;
						host				= false;
						return;
					}	break;
					default:
						break;
					}
				}
				case 3:
				quit = true;
				return;
			default:
				continue;
				return;
			}
		}
	}();


	if (quit)
		return 0;

	FlexKit::InitLog(argc, argv);
	FlexKit::SetShellVerbocity(FlexKit::Verbosity_1);
	FlexKit::AddLogFile("GameState.log", FlexKit::Verbosity_INFO);

#ifdef _DEBUG
	FlexKit::AddLogFile("GameState_Detailed.log", 
		FlexKit::Verbosity_9, 
		false);
#endif

	FK_LOG_INFO("Logging initialized started.");

	auto* Memory = CreateEngineMemory();
	EXITSCOPE(ReleaseEngineMemory(Memory));

	FlexKit::FKApplication app{ uint2{ 1920, 1080 }, Memory };

	for (size_t I = 0; I < argc; ++I)
		app.PushArgument(argv[I]);

	FK_LOG_INFO("Set initial PlayState state.");
	auto& gameBase = app.PushState<BaseState>(app);


	if (multiplayerMode)
	{
        AddAssetFile("assets\\Scene1.gameres");

		auto& NetState = app.PushState<NetworkState>(gameBase);
		if (host)
			auto& hostState		= app.PushState<GameHostState>(gameBase, NetState);
		else
			auto& clientState	= app.PushState<GameClientState>(gameBase, NetState, ClientGameDescription{ 1337, server.c_str(), name.c_str() });
	}
	else
	{
        AddAssetFile("assets\\Scene1.gameres");

		auto& gameState = app.PushState<GameState>(gameBase);
		auto& playState = app.PushState<LocalPlayerState>(gameBase, gameState);

        LoadScene(app.GetFramework().core, gameState.scene, 10000);
        DEBUG_ListSceneObjects(gameState.scene);


        if (auto [Cube_003, found] = FindGameObject(gameState.scene, "Cube.003"); found)
            SetVisable(*Cube_003, false);

        if (auto [gameObject, found] = FindGameObject(gameState.scene, "object1"); found)
        {
            const auto parent       = GetParentNode(*gameObject);
            const auto parentQ      = GetOrientation(parent);
            const auto orientation  = GetOrientation(*gameObject);

            ClearParent(*gameObject);
            AddSkeletonComponent(*gameObject, app.GetFramework().core.GetBlockMemory());
            SetScale(*gameObject, { 1, 1, 1 });
        }
	}

	FK_LOG_INFO("Running application...");
	app.Run();
	FK_LOG_INFO("Completed running application");

	FK_LOG_INFO("Started cleanup...");
	app.Release();
	FK_LOG_INFO("Completed cleanup.");

	return 0;
}
