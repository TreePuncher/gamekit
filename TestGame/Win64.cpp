/**********************************************************************

Copyright (c) 2019 Robert May

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
#include "..\coreutilities\Logging.cpp"

#include "BaseState.h"
#include "client.cpp"
#include "Gameplay.cpp"
#include "host.cpp"
#include "lobbygui.cpp"
#include "MainMenu.cpp"
#include "MultiplayerState.cpp"
#include "MultiplayerGameState.cpp"
#include "TestScene.h"
//#include "TextureStreamingTest.cpp"

#include <iostream>


int main(int argc, char* argv[])
{
    enum class ApplicationMode
    {
        Client,
        Host,
        TextureStreamingTestMode,
        GraphicsTestMode,
        PlaygroundMode,
    }   applicationMode = ApplicationMode::GraphicsTestMode;

    std::string name;
    std::string server;

    FlexKit::InitLog(argc, argv);
    FlexKit::SetShellVerbocity(FlexKit::Verbosity_1);
    FlexKit::AddLogFile("GameState.log", FlexKit::Verbosity_INFO);

#ifdef _DEBUG
    FlexKit::AddLogFile("GameState_Detailed.log", 
        FlexKit::Verbosity_9, 
        false);
#endif

    FK_LOG_INFO("Logging initialized started.");

    uint2   WH          = uint2{ 1920, 1080 } * 1.4f;
    size_t threadCount  = max(std::thread::hardware_concurrency(), 1u) - 1;

    for (size_t I = 0; I < argc; ++I)
    {
        const char* TextureStreamingTestStr = "TextureStreamingTest";

        if (!strncmp("-WH", argv[I], 2))
        {
            const auto X_str = argv[I + 1];
            const auto Y_str = argv[I + 2];

            const uint32_t X = atoi(X_str);
            const uint32_t Y = atoi(Y_str);

            WH = { X, Y };

            I += 2;
        }
        if (!strncmp("--threadcount", argv[I], strlen("-threadCount")))
        {
            threadCount = atoi(argv[I + 1]);
            I += 1;
        }
        else if (!strncmp("-C", argv[I], 2))
        {
            try
            {
                std::cout << "Please Enter Player Name: \n";
                std::cin >> name;

                std::cout << "Please Enter Server: \n";
                std::cin >> server;
            }
            catch (...)
            {
                return -1;
            }
            applicationMode = ApplicationMode::Client;
        }
        else if (!strncmp("-S", argv[I], 2))
        {
            try
            {
                std::cout << "Please Enter Player Name: \n";
                std::cin >> name;
            }
            catch (...)
            {
                return -1;
            }
            applicationMode = ApplicationMode::Host;
        }
        else if (!strncmp(TextureStreamingTestStr, argv[I], strlen(TextureStreamingTestStr))) // 
            applicationMode = ApplicationMode::GraphicsTestMode;

        //app.PushArgument(argv[I]);
    }


    auto* Memory = CreateEngineMemory();
    EXITSCOPE(ReleaseEngineMemory(Memory));

    FlexKit::FKApplication app{ WH, Memory, threadCount };

    FK_LOG_INFO("Set initial PlayState state.");
    auto& base = app.PushState<BaseState>(app);

    switch (applicationMode)
    {
        case ApplicationMode::Client:
        {
            AddAssetFile("assets\\TestScenes.gameres");

            auto& NetState      = app.PushState<NetworkState>(base);
            auto& clientState   = app.PushState<GameClientState>(base, NetState, ClientGameDescription{ 1337, server.c_str(), name.c_str() });
        }   break;
        case ApplicationMode::Host:
        {
            AddAssetFile("assets\\TestScenes.gameres");

            auto& NetState  = app.PushState<NetworkState>(base);
            auto& hostState = app.PushState<GameHostState>(base, NetState);
        }   break;
        case ApplicationMode::GraphicsTestMode:
        {
            AddAssetFile("assets\\DemonGirl.gameres");

            StartTestState(app, base, TestScenes::AnimationTest);
        }   break;
        case ApplicationMode::PlaygroundMode:
        {
            AddAssetFile("assets\\TestScenes.gameres");

            auto& gameState = app.PushState<GameState>(base);

            auto& completed = app.GetFramework().core.GetBlockMemory().allocate_aligned<bool>(false);

            auto Task =
                [&]()
                {
                    auto& framework             = app.GetFramework();
                    auto& allocator             = framework.core.GetBlockMemory();
                    auto& renderSystem          = framework.GetRenderSystem();
                    UploadQueueHandle upload    = renderSystem.GetUploadQueue();

                    auto HDRStack = LoadHDR("assets/textures/lakeside_2k.hdr", 6, allocator);

                    base.irradianceMap = MoveTextureBuffersToVRAM(
                        renderSystem,
                        upload,
                        HDRStack.begin(),
                        HDRStack.size(),
                        FORMAT_2D::R32G32B32A32_FLOAT,
                        allocator);

                    renderSystem.SetDebugName(base.irradianceMap, "irradiance Map");
                    renderSystem.SetDebugName(base.irradianceMap, "ggx Map");
                    renderSystem.SubmitUploadQueues(&upload);

                    completed = true;
                };

            auto& workItem = CreateWorkItem(Task, app.GetFramework().core.GetBlockMemory());
            app.GetFramework().core.Threads.AddWork(workItem);

            // Setup test scene
            LoadScene(app.GetFramework().core, gameState.scene, 10000);

            auto OnLoadUpdate =
                [&](EngineCore& core, UpdateDispatcher& dispatcher, double dT)
                {
                    const auto completedState = completed;
                    if(completedState)
                    {
                        app.PopState();

                        auto& playState = app.PushState<LocalPlayerState>(base, gameState);
                        app.GetFramework().core.GetBlockMemory().free(&completed);
                    }
                };


            using LoadState = GameLoadScreenState<decltype(OnLoadUpdate)>;
            app.PushState<LoadState>(base, OnLoadUpdate);

        }   break;
        case ApplicationMode::TextureStreamingTestMode:
        {
            //auto& testState = app.PushState<TextureStreamingTest>(base);
        }   break;
    default:
        return -1;
    }

    FK_LOG_INFO("Running application...");
    app.Run();
    FK_LOG_INFO("Completed running application");

    FK_LOG_INFO("Started cleanup...");
    app.Release();
    FK_LOG_INFO("Completed cleanup.");

    return 0;
}
