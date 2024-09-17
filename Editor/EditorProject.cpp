#include "PCH.h"
#include "EditorMeshResource.hpp"
#include "EditorProject.h"
#include "EditorResource.h"
#include "EditorTextureResources.h"

#include <boost/process.hpp>
#include <fstream>
#include <filesystem>
#include <print>
#include <QFileSystemWatcher>
#include <regex>
#include <Serialization.hpp>


using namespace std::filesystem;


/************************************************************************************************/
// Project Global Parameters
inline static std::string objectsDirectory				= "Objects/";
//inline static const char defaultConfigure[]				= R"(cmd /r 'echo hello & "@vcvars" & cmake -B "@buildPath" "@projectPath" --preset @preset')";
inline static const char defaultConfigure[]				= R"(cmd /c "echo "Starting Reconfigure" && "@VCVARS" && @ProjectDrive && cmake -B "@BuildPath" "@ProjectPath" --preset @Preset && echo "Done!"")";
inline static const char defaultBuildCommand[]			= R"(cmd /c "echo "Starting Build" && @ProjectDrive && cd "@ProjectPath" && "@VCVARS" && cmake --build "@BuildPath" && echo "Done!"")";
inline static const char debugPreset[]					= R"("x64-debug")";
inline static const char releasePreset[]				= R"("x64-release")";

//inline static const char defaultGitSource[]			= R"(https://github.com/TreePuncher/gamekit.git)";
//inline static const char defaultGitHash[]				= R"(1324dcaad01af6763909ba73238eb1213a2124db)";

inline static const char defaultGitSource[]				= R"(http://fedora/gamedev/flex.git)";
inline static const char defaultGitHash[]				= R"(3c6ab3ec7ca4834c7cd47859c7f6a6802fb15e70)";

inline static const char defaultVCVarsPath[]			= R"(C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat)";

inline static std::string cmakeTemplate = R"(
cmake_minimum_required(VERSION 3.23)

include(FetchContent)

FetchContent_Declare(
  vcpkg 
  GIT_REPOSITORY "https://github.com/microsoft/vcpkg"
  GIT_TAG        "91d888703f251c13111c1b889be1f350c4ceb7ab"
)

FetchContent_MakeAvailable(vcpkg)
set(CMAKE_TOOLCHAIN_FILE "${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "")

project(@ProjectName  LANGUAGES CXX VERSION 0.0.1)

FetchContent_Declare(
  flex
  GIT_REPOSITORY "@SourceRepo"
  GIT_TAG        "@SourceHash"
)

FetchContent_MakeAvailable(flex)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file (GLOB CPP_FILES src/*.cpp)
file (GLOB HPP_FILES includes/*.hpp)

add_executable(
    @AppName
    ${CPP_FILES}
	${HPP_FILES}
)

target_include_directories(
    @AppName
    PUBLIC
    ${PROJECT_SOURCE_DIR})

set_property(TARGET @AppName PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_link_libraries(@AppName PRIVATE flex flex_optional)
)";


/************************************************************************************************/


std::string SearchAndReplace(const std::string& in, const std::string& from, const std::string& to)
{
	return std::regex_replace(in, std::regex(from), to);
}


ProjectResource_ptr EditorScene::FindSceneResource(uint64_t resourceID)
{
	for (auto& resource : resources)
		if (resource->resource->GetResourceGUID() == resourceID)
			return resource;

	return nullptr;
}

/************************************************************************************************/


EditorProject::EditorProject() :
	fileWatcher				{ new QFileSystemWatcher{} },
	debugBuildCMakeCommand	{ defaultBuildCommand },
	gitSource				{ defaultGitSource },
	gitSourceHash			{ defaultGitHash }
{

}


EditorProject::~EditorProject()
{
	delete fileWatcher;
}


/************************************************************************************************/


bool EditorProject::LoadProject(const std::string& projectDir)
{
	std::unique_lock sl{m};

	std::filesystem::path projectPath{ projectDir };
	if (!std::filesystem::exists(projectPath))
		return false;

	auto f = fopen(projectDir.c_str(), "rb");
	if (!f)
		return false;

	ResetProject();

	FlexKit::LoadFileArchiveContext archive{ f };
	archive& resources;
	archive& scenes;

	fclose(f);

	const std::string fileName = projectPath.replace_extension().string();
	FlexKit::SetProjectResourceDir(fileName + R"(.objects/)");
	projectDirectory = std::filesystem::path{ projectDir }.parent_path().string();

	delete fileWatcher;
	fileWatcher = new QFileSystemWatcher{};
	StartWatchingDirectories();

	return true;
}


/************************************************************************************************/



bool EditorProject::SaveProject(const std::string& projectDir)
{
	std::unique_lock sl{ m };

	std::filesystem::path projectPath(projectDir);
	const std::string fileName = projectPath.replace_extension().string();
	std::filesystem::create_directory(fileName + R"(.objects)");
	FlexKit::SetProjectResourceDir(fileName + R"(.objects/)");

	if (!projectDir.size())
		return false;

	try
	{
		FlexKit::SaveArchiveContext archive;
		archive& resources;
		archive& scenes;

		auto blob = archive.GetBlob();

		auto f = fopen(projectDir.c_str(), "wb");
		WriteBlob(blob, f);
		fclose(f);

		return true;
	}
	// swallow any exceptions
	// TODO(R.M): log this?
	catch (...) 
	{
		return false;
	}
}


/************************************************************************************************/


void EditorProject::lock()
{
	m.lock();
}


/************************************************************************************************/


void EditorProject::unlock()
{
	m.unlock();
}


/************************************************************************************************/


void EditorProject::StartWatchingDirectories()
{
	for (auto&& header : std::filesystem::directory_iterator{ GetHeadersPath() })
	{
		headerFiles.insert(header.path().string());
		onHeaderAdded(header.path().string());

		if (fileWatcher->addPath(header.path().string().c_str()))
			QObject::connect(fileWatcher, &QFileSystemWatcher::directoryChanged,
				[this](const QString& path)
				{
					onHeaderChanged(path.toStdString());
				});
	}
	for (auto&& srcFile : std::filesystem::directory_iterator{ GetSourcesPath() })
		sourceFiles.insert(srcFile.path().filename().string());

	if (fileWatcher->addPath(GetHeadersPath().c_str()))
	{
		QObject::connect(fileWatcher, &QFileSystemWatcher::directoryChanged,
			[this](const QString& path)
			{
				HeaderAddedRemoved(path.toStdString());
			});
	}
}


/************************************************************************************************/


void EditorProject::HeaderChanged(const std::string& changedFile)
{
	onHeaderChanged(changedFile);
}


/************************************************************************************************/


void EditorProject::HeaderAddedRemoved(const std::string& changedDirectory)
{
	for (auto&& file : std::filesystem::directory_iterator{ changedDirectory })
	{
		if (file.is_regular_file() && file.path().extension() == ".hpp")
		{
			auto fileName = file.path().filename().string();
			if (!headerFiles.contains(fileName))
			{
				if (fileWatcher->addPath(file.path().string().c_str()))
					QObject::connect(fileWatcher, &QFileSystemWatcher::directoryChanged,
						[this](const QString& path)
						{
							onHeaderChanged(path.toStdString());
						});

				headerFiles.insert(fileName);
				onHeaderAdded(file.path().string());
			}
		}
	}

	for (auto&& header : headerFiles)
	{
		std::filesystem::path path{ GetHeadersPath() + header };
		if (!std::filesystem::exists(path))
		{
			headerFiles.erase(header);
			onHeaderRemoved(header);
		}
	}
}


/************************************************************************************************/


void EditorProject::AddScene(EditorScene_ptr scene)
{
	std::unique_lock sl{ m };

	scenes.emplace_back(scene);
}


/************************************************************************************************/


ProjectResource_ptr EditorProject::AddResource(FlexKit::Resource_ptr resource)
{
	std::unique_lock sl{ m };

	auto projectResource = std::make_shared<ProjectResource>(resource);
	resources.emplace_back(projectResource);

	return projectResource;
}


/************************************************************************************************/


FlexKit::ResourceList EditorProject::GetResources() const
{
	std::shared_lock sl{ const_cast<std::shared_mutex&>(m) };

	FlexKit::ResourceList out;

	for (auto& r : resources)
		out.push_back(r->resource);

	return out;
}


/************************************************************************************************/


void EditorProject::RemoveResource(FlexKit::Resource_ptr resource)
{
	std::unique_lock sl{ m };

	if (resource->GetResourceTypeID() == SceneResourceTypeID)
		std::erase_if(scenes, [&](auto& res) -> bool { return (res->resource == resource); });

	std::erase_if(resources, [&](auto& res) -> bool { return (res->resource == resource); });
}


/************************************************************************************************/


ProjectResource_ptr EditorProject::FindProjectResource(uint64_t assetID)
{
	std::shared_lock sl{ m };

	auto res = std::find_if(
		resources.begin(),
		resources.end(),
		[&](ProjectResource_ptr& resource)
		{
			return resource->resource->GetResourceGUID() == assetID;
		}
	);

	if (res != resources.end())
		return (*res);
	else
		return nullptr;
}


/************************************************************************************************/


ProjectResource_ptr EditorProject::FindProjectResource(const std::string& id)
{
	std::shared_lock sl{ m };

	auto res = std::find_if(
		resources.begin(),
		resources.end(),
		[&](ProjectResource_ptr& resource)
		{
			return resource->resource->GetResourceID() == id;
		}
	);

	if (res != resources.end())
		return (*res);
	else
		return nullptr;
}


/************************************************************************************************/


std::string ProjectGetObjectDirectory()
{
	return objectsDirectory;
}


/************************************************************************************************/


void EditorProject::CreateProjectFileStructure(const std::string& projectDir)
{
	projectDirectory = projectDir; 
	std::filesystem::create_directory(projectDirectory.string() + R"(\generated_headers)");
	std::filesystem::create_directory(projectDirectory.string() + R"(\components)");
	std::filesystem::create_directory(projectDirectory.string() + R"(\build)");
	std::filesystem::create_directory(projectDirectory.string() + R"(\assets)");
	std::filesystem::create_directory(projectDirectory.string() + R"(\includes)");
	std::filesystem::create_directory(projectDirectory.string() + R"(\src)");

	SaveProject(projectDirectory.string() + R"(\flex.proj)");
}


/************************************************************************************************/


void EditorProject::ResetProject()
{
	scenes.clear();
	resources.clear();
	headerFiles.clear();
	sourceFiles.clear();
}


/************************************************************************************************/


void EditorProject::RegenerateCMake() const
{
	path cmakeFile = projectDirectory.string() + R"(\CMakeLists.txt)";

	if (exists(path(cmakeFile)))
		remove(cmakeFile);

	std::string newCMakeTexts = cmakeTemplate;

	newCMakeTexts = SearchAndReplace(newCMakeTexts, "@ProjectName", projectName);
	newCMakeTexts = SearchAndReplace(newCMakeTexts, "@SourceRepo", gitSource);
	newCMakeTexts = SearchAndReplace(newCMakeTexts, "@SourceHash", gitSourceHash);
	newCMakeTexts = SearchAndReplace(newCMakeTexts, "@AppName", "TestApp");

	auto f = fopen(cmakeFile.string().c_str(), "w");

	if (!f)
	{
		FK_LOG_ERROR("Failed to create new cmakefile at: %s", cmakeFile.c_str());
		return;
	}

	for(size_t i = 0; i < newCMakeTexts.size(); i++)
		i += fwrite(newCMakeTexts.c_str() + i, 1, newCMakeTexts.size() - i, f);

	fclose(f);
}


/************************************************************************************************/


void EditorProject::ReconfigureCMake() const
{
	std::string configCommand = defaultConfigure;
	configCommand = SearchAndReplace(configCommand, "@VCVARS", defaultVCVarsPath);
	configCommand = SearchAndReplace(configCommand, "@BuildPath", projectDirectory.string() + "/out");
	configCommand = SearchAndReplace(configCommand, "@ProjectPath", projectDirectory.string());
	configCommand = SearchAndReplace(configCommand, "@Preset", debugPreset);
	configCommand = SearchAndReplace(configCommand, "@ProjectDrive", std::string{} + projectDirectory.string()[0] + ":");

	std::print("{}\n", configCommand);

	try
	{
		boost::process::ipstream pipe_stream_out;
		//boost::process::child c{ configCommand, boost::process::std_out > pipe_stream_out };
		boost::process::child c{ configCommand, boost::process::std_out > pipe_stream_out };


		std::string output;
		while (c.running())
		{
			std::this_thread::sleep_for(1s);

			char buffer[1024];
			pipe_stream_out.gcount();
			pipe_stream_out.read(buffer, 1024);
		}
	}
	catch (const boost::process::process_error& error)
	{
		FK_LOG_ERROR("EditorProject::ReconfigureCMake: %s", error.what());
	}
}


/************************************************************************************************/


void EditorProject::BuildDebug() const
{
	std::string buildCommand = defaultBuildCommand;
	buildCommand = SearchAndReplace(buildCommand, "@VCVARS", defaultVCVarsPath);
	buildCommand = SearchAndReplace(buildCommand, "@BuildPath", projectDirectory.string() + "/out");
	buildCommand = SearchAndReplace(buildCommand, "@Preset", debugPreset);
	buildCommand = SearchAndReplace(buildCommand, "@ProjectPath", projectDirectory.string());
	buildCommand = SearchAndReplace(buildCommand, "@ProjectDrive", std::string{} + projectDirectory.string()[0] + ":");

	std::print("{}\n", buildCommand);

	boost::process::ipstream pipe_stream;
	boost::process::child c{ buildCommand, boost::process::std_out > pipe_stream};

	std::string line;

	while (c.running())
	{
		while (pipe_stream && std::getline(pipe_stream, line))
			std::print("{}\n", line);
	}

	c.wait();
}


/************************************************************************************************/


void EditorProject::StartEditor() const
{
	auto command = std::string{ R"(cmd /c ")" } + defaultVCVarsPath + " | " + "devenv " + R"(" & echo "Done!")";
	system(command.c_str());
}



/************************************************************************************************/


std::string EditorProject::GetHeadersPath() const
{
	return projectDirectory.string() + R"(\includes)";
}


/************************************************************************************************/


std::string EditorProject::GetSourcesPath() const
{
	return projectDirectory.string() + R"(\src)";
}


/************************************************************************************************/


std::string EditorProject::GetAssetsPath() const
{
	return projectDirectory.string() + R"(\assets)";
}


std::string EditorProject::GetGeneratedPath() const
{
	return projectDirectory.string() + R"(\generated_headers)";
}


/**********************************************************************

Copyright (c) 2019-2024 Robert May

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
