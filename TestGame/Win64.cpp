#include "buildsettings.h"
#include "Logging.h"
#include "Application.h"
#include "BaseState.h"

#include "MultiplayerState.h"
#include "MenuState.h"

#ifdef UNITY
#include "BaseState.cpp"
#include "Enemy1.cpp"
#include "client.cpp"
#include "Gameplay.cpp"
#include "host.cpp"
#include "lobbygui.cpp"
#include "menuState.cpp"

#include "GraphicsTest.hpp"

#include "MultiplayerState.cpp"
#include "MultiplayerGameState.cpp"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include <cpptoml.h>


/************************************************************************************************/


int main(int argc, char* argv[])
{
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
			app.Release();
			FK_LOG_2("shut down Finished.");
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
	f << *config;
	f.close();

	return 0;
}


/**********************************************************************

Copyright (c) 2022 Robert May

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
