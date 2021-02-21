#include "buildsettings.h"

//#include <windows.h>
//#include <WinSock2.h>

#include "Logging.h"
#include "Application.h"
#include "AnimationComponents.h"

#include "BaseState.h"
#include "client.cpp"
#include "Gameplay.cpp"
#include "host.cpp"
#include "lobbygui.cpp"
#include "menuState.cpp"
#include "TestScene.h"

#include "GraphicsTest.hpp"

#include "MultiplayerState.cpp"
#include "MultiplayerGameState.cpp"
#include "playgroundmode.hpp"
//#include "DebugUI.cpp"

#include "allsourcefiles.cpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>

int main(int argc, char* argv[])
{
    enum class ApplicationMode
    {
        Client,
        Host,
        TextureStreamTestMode,
        ACCTestMode,
        PlaygroundMode,
    }   applicationMode = ApplicationMode::TextureStreamTestMode;

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

    uint2   WH = uint2{ (uint)(1920), (uint)(1080) };

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
            applicationMode = ApplicationMode::TextureStreamTestMode;

        //app.PushArgument(argv[I]);
    }

    try
    {
        auto* allocator = CreateEngineMemory();
        EXITSCOPE(ReleaseEngineMemory(allocator));

        FlexKit::FKApplication app{ allocator, Max(std::thread::hardware_concurrency(), 1u) - 1 };
        app.GetCore().FrameLock = false;


        auto& base      = app.PushState<BaseState>(app, WH);

#if 1

        FK_LOG_INFO("Set initial PlayState state.");
        auto& net       = app.PushState<NetworkState>(base);
        auto& menuState = app.PushState<MenuState>(base, net);

#else
        switch (applicationMode)
        {
        case ApplicationMode::TextureStreamTestMode:
        {
            StartTestState(app, base, TestScenes::ShadowTestScene);
        }   break;
        case ApplicationMode::PlaygroundMode:
        {

        }   break;
        case ApplicationMode::ACCTestMode:
        {
            app.PushState<GraphicsTest>(base);
        }   break;
        default:
            return -1;
        }
#endif

        try
        {
            FK_LOG_2("Running application.");
            app.Run();
            FK_LOG_2("Application shutting down.");

            FK_LOG_2("Cleanup Startup.");
            app.Release();
            FK_LOG_2("Cleanup Finished.");
        }
        catch (...)
        {
            return -1;
        };
    }
    catch (...)
    {
        return -2;
    };

    return 0;
}
