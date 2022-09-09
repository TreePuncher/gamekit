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

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include <cpptoml.h>

int main(int argc, char* argv[])
{

	Vector<uint32_t, 4> testA{ SystemAllocator };
	testA.push_back(1);
	testA.push_back(2);
	testA.push_back(3);
	testA.push_back(4);

	Vector<uint32_t> testB{ SystemAllocator };
	testB = std::move(testA);


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

	auto config = cpptoml::parse_file("config.toml");
	if (config->empty())
	{
		auto engineParams = cpptoml::make_table();
		engineParams->insert("ThreadCount", Max(std::thread::hardware_concurrency(), 2u) - 1);

		config->insert("Engine", engineParams);
	}

	auto engineParams = config->get_table("Engine");

	if (!engineParams)
		return -1;

	const int threadCount = engineParams->get_as<int>("ThreadCount").value_or(1);


	auto* allocator = CreateEngineMemory();
	EXITSCOPE(ReleaseEngineMemory(allocator));

	try
	{
		FlexKit::FKApplication app{ allocator, Max(std::thread::hardware_concurrency(), 2u) - 1 };

		auto assets = config->get_table("Assets");
		if (assets)
		{
			auto assetFolder = assets->get_as<std::string>("Folder");
			auto assetsFiles = assets->get_array_of<std::string>("Files");

			for (auto file : *assetsFiles)
			{
				std::string assetFile = std::string(*assetFolder) + std::string(file);
				FlexKit::AddAssetFile(assetFile.c_str());
			}
		}

		app.GetCore().FrameLock = engineParams->get_as<bool>("FrameLock").value_or(false);
		app.GetCore().vSync		= engineParams->get_as<bool>("VerticalSync").value_or(false);
		app.GetCore().FPSLimit  = engineParams->get_as<int>("FPSLimit").value_or(60);

		auto& base		= app.PushState<BaseState>(app, WH);
		auto& net		= app.PushState<NetworkState>(base);
		auto& menuState	= app.PushState<MenuState>(base, net);

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

	std::fstream f{ "config.toml", 'w' };
	std::cout << *config;
	f << *config;
	f.close();

	return 0;
}
