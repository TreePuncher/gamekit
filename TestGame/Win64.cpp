#include "buildsettings.h"

#include "Logging.h"
#include "Application.h"
#include "AnimationComponents.h"

#include "BaseState.h"
#include "client.cpp"
#include "Gameplay.cpp"
#include "host.cpp"
#include "lobbygui.cpp"
#include "menuState.cpp"

#include "GraphicsTest.hpp"

#include "MultiplayerState.cpp"
#include "MultiplayerGameState.cpp"
#include "playgroundmode.hpp"

#include "allsourcefiles.cpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>

int main(int argc, char* argv[])
{
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
        if (!strncmp("-WH", argv[I], 2))
        {
            const auto X_str = argv[I + 1];
            const auto Y_str = argv[I + 2];

            const uint32_t X = atoi(X_str);
            const uint32_t Y = atoi(Y_str);

            WH = { X, Y };

            I += 2;
        }
    }

    auto* allocator = CreateEngineMemory();
    EXITSCOPE(ReleaseEngineMemory(allocator));

    try
    {
        FlexKit::FKApplication app{ allocator, Max(std::thread::hardware_concurrency() / 2, 1u) - 1 };
        app.GetCore().FrameLock = true;
        app.GetCore().FPSLimit  = 90;

        auto& base      = app.PushState<BaseState>(app, WH);
        auto& net       = app.PushState<NetworkState>(base);
        auto& menuState = app.PushState<MenuState>(base, net);

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
