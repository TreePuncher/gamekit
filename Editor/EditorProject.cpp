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


using namespace FlexKit;
using namespace std::filesystem;


/************************************************************************************************/
// Project Global Parameters
inline static std::string objectsDirectory				= "Objects/";
inline static const char defaultRunCommand[]			= R"(cmd /c "echo "Starting Visual Studio" && "@VCVARS" && @ProjectDrive && cd "@ProjectPath" && @Command")";
inline static const char defaultOpenIDE[]				= R"(cmd /c "echo "Starting Visual Studio" && "@VCVARS" && @ProjectDrive && cd "@ProjectPath" && devenv "@ProjectPath")";
inline static const char defaultOpenExplorer[]			= R"(cmd /c "echo "Starting Explorer" && @ProjectDrive && explorer "@ProjectPath")";
inline static const char defaultConfigure[]				= R"(cmd /c "echo "Starting Reconfigure" && "@VCVARS" && @ProjectDrive && cmake -B "@BuildPath" "@ProjectPath" --preset @Preset && echo "Done!"")";
inline static const char defaultBuildCommand[]			= R"(cmd /c "echo "Starting Build" && @ProjectDrive && cd "@ProjectPath" && "@VCVARS" && cmake --build "@BuildPath" && echo "Done!"")";
inline static const char debugPreset[]					= R"("x64-debug")";
inline static const char releasePreset[]				= R"("x64-release")";

inline static const char defaultGitSource[]				= R"(https://github.com/TreePuncher/gamekit.git)";
inline static const char defaultGitHash[]				= R"(editor)";

//inline static const char defaultGitSource[]			= R"(http://fedora/gamedev/flex.git)";
//inline static const char defaultGitHash[]				= R"(editor)";

inline static const char defaultVCVarsPath[]			= R"(C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat)";

inline static std::string cmakeTemplate = R"(
cmake_minimum_required(VERSION 3.23)

include(FetchContent)

project(@ProjectName  LANGUAGES CXX VERSION @Version)

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
)

target_include_directories(
	@AppName
	PUBLIC
	${PROJECT_SOURCE_DIR}/includes
	${PROJECT_SOURCE_DIR}/generated_headers)

set_property(TARGET @AppName PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_link_libraries(@AppName PRIVATE flex flex_optional)

Flex_CopyBinaries(@AppName)
Flex_CopyAssets(@AppName)
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

	path projectPath{ projectDir };
	if (!exists(projectPath))
		return false;

	auto f = fopen(projectDir.c_str(), "rb");
	if (!f)
		return false;

	ResetProject();

	size_t fileVersion;

	LoadFileArchiveContext archive{ f };

	archive& *this;

	fclose(f);

	const std::string fileName = projectPath.replace_extension().string();
	SetProjectResourceDir(fileName + R"(.objects/)");
	projectDirectory = path{ projectDir }.parent_path().string();

	delete fileWatcher;
	fileWatcher = new QFileSystemWatcher{};
	StartWatchingDirectories();

	return true;
}


/************************************************************************************************/



bool EditorProject::SaveProject(const std::string& projectDir)
{
	std::unique_lock sl{ m };

	path projectPath{ projectDir };
	const std::string fileName = projectPath.replace_extension().string();
	path objectDir{ fileName + R"(.objects)" };

	if(!exists(objectDir))
		create_directory(objectDir);

	SetProjectResourceDir(fileName + R"(.objects/)");

	if (!projectDir.size())
		return false;

	try
	{
		SaveArchiveContext archive;
		archive& *this;

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
	for (auto&& header : directory_iterator{ GetHeadersPath() })
	{
		headerFiles.insert(header.path().string());
		onHeaderAdded(header.path().string());

		if (fileWatcher->addPath(header.path().string().c_str()))
			QObject::connect(fileWatcher, &QFileSystemWatcher::directoryChanged,
				[this](const QString& path)
				{
					projectNeedsCMakeRebuild = true;
					onHeaderChanged(path.toStdString());
				});
	}

	for (auto&& srcFile : directory_iterator{ GetSourcesPath() })
	{
		projectNeedsCMakeRebuild = true;
		sourceFiles.insert(srcFile.path().filename().string());
	}

	if (fileWatcher->addPath(GetHeadersPath().c_str()))
	{
		QObject::connect(fileWatcher, &QFileSystemWatcher::directoryChanged,
			[this](const QString& path)
			{
				projectNeedsCMakeRebuild = true;
				HeaderAddedRemoved(path.toStdString());
			});
	}
}


/************************************************************************************************/


void EditorProject::HeaderChanged(const std::string& changedFile)
{
	projectNeedsCMakeRebuild = true;
	onHeaderChanged(changedFile);
}


/************************************************************************************************/


void EditorProject::HeaderAddedRemoved(const std::string& changedDirectory)
{
	for (auto&& file : directory_iterator{ changedDirectory })
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
				projectNeedsCMakeRebuild = true;
				onHeaderAdded(file.path().string());
			}
		}
	}

	for (auto&& header : headerFiles)
	{
		path path{ GetHeadersPath() + header };
		if (exists(path))
		{
			headerFiles.erase(header);
			projectNeedsCMakeRebuild = true;
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


ResourceList EditorProject::GetResources() const
{
	std::shared_lock sl{ const_cast<std::shared_mutex&>(m) };

	ResourceList out;

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
		[&](ProjectResource_ptr& projectRes)
		{
			return projectRes->resource->GetResourceGUID() == assetID;
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
		[&](ProjectResource_ptr& projectRes)
		{
			return projectRes->resource->GetResourceID() == id;
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
	create_directory(projectDirectory.string() + R"(\generated_headers)");
	create_directory(projectDirectory.string() + R"(\components)");
	create_directory(projectDirectory.string() + R"(\assets)");
	create_directory(projectDirectory.string() + R"(\assetPacks)");
	create_directory(projectDirectory.string() + R"(\includes)");
	create_directory(projectDirectory.string() + R"(\src)");

	copy_file(R"(resources\vcpkg.json)",		projectDirectory.string() + R"(\vcpkg.json)");
	copy_file(R"(resources\CMakePresets.json)", projectDirectory.string() + R"(\CMakePresets.json)");
	copy_file(R"(resources\main.cpp)",			projectDirectory.string() + R"(\src\main.cpp)");
	copy_file(R"(resources\.gitignore)",		projectDirectory.string() + R"(\.gitignore)");

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
	newCMakeTexts = SearchAndReplace(newCMakeTexts, "@Version", version);

	auto f = fopen(cmakeFile.string().c_str(), "w");

	if (!f)
	{
		FK_LOG_ERROR("Failed to create new cmakefile at: %s", cmakeFile.c_str());
		return;
	}

	for(size_t i = 0; i < newCMakeTexts.size(); i++)
		i += fwrite(newCMakeTexts.c_str() + i, 1, newCMakeTexts.size() - i, f);

	fclose(f);

	projectNeedsCMakeRebuild = false;
}


/************************************************************************************************/


void EditorProject::ReconfigureCMake() const
{
	std::string configCommand = defaultConfigure;
	configCommand = SearchAndReplace(configCommand, "@VCVARS", defaultVCVarsPath);
	configCommand = SearchAndReplace(configCommand, "@BuildPath", projectDirectory.string() + "/out/build/" + debugPreset);
	configCommand = SearchAndReplace(configCommand, "@ProjectPath", projectDirectory.string());
	configCommand = SearchAndReplace(configCommand, "@Preset", debugPreset);
	configCommand = SearchAndReplace(configCommand, "@ProjectDrive", std::string{} + projectDirectory.string()[0] + ":");

	std::print("{}\n", configCommand);

	try
	{
		boost::process::ipstream pipe_stream_out;
		boost::process::child c{ configCommand, boost::process::std_out > pipe_stream_out };

		std::string line;
		while (pipe_stream_out && std::getline(pipe_stream_out, line))
			std::print("{}\n", line);

		projectNeedsCMakeRebuild = false;
	}
	catch (const boost::process::process_error& error)
	{
		FK_LOG_ERROR("EditorProject::ReconfigureCMake: %s", error.what());
	}
}


/************************************************************************************************/


void EditorProject::BuildDebug() const
{
	if (projectNeedsCMakeRebuild)
		ReconfigureCMake();

	std::string buildCommand = defaultBuildCommand;
	buildCommand = SearchAndReplace(buildCommand, "@VCVARS", defaultVCVarsPath);
	buildCommand = SearchAndReplace(buildCommand, "@BuildPath", projectDirectory.string() + "/out/build/x64-debug");
	buildCommand = SearchAndReplace(buildCommand, "@Preset", debugPreset);
	buildCommand = SearchAndReplace(buildCommand, "@ProjectPath", projectDirectory.string());
	buildCommand = SearchAndReplace(buildCommand, "@ProjectDrive", std::string{} + projectDirectory.string()[0] + ":");

	std::print("{}\n", buildCommand);

	boost::process::ipstream pipe_stream;
	boost::process::child c{ buildCommand, boost::process::std_out > pipe_stream};

	std::string line;

	while (pipe_stream && std::getline(pipe_stream, line))
		std::print("{}\n", line);

	c.wait();
}


/************************************************************************************************/


int EditorProject::RunBuildCommand(const std::string& commandStr) const
{
	std::string command = defaultRunCommand;
	command = SearchAndReplace(command, "@VCVARS", defaultVCVarsPath);
	command = SearchAndReplace(command, "@ProjectDrive", std::string{} + projectDirectory.string()[0] + ":");
	command = SearchAndReplace(command, "@ProjectPath", path{ projectDirectory }.make_preferred().string());
	command = SearchAndReplace(command, "@Command", commandStr);

	std::print("{}\n", command);

	boost::process::ipstream pipe_stream;
	boost::process::child c{ command, boost::process::std_out > pipe_stream };

	std::string line;

	while (pipe_stream && std::getline(pipe_stream, line))
		std::print("{}\n", line);

	c.wait();
	return c.exit_code();
}


/************************************************************************************************/


void EditorProject::MoveAssets(const std::string& targetLocation) const
{
	auto assetPackLocation = GetGameAssetsPath();

	for (auto& itr : directory_iterator{ assetPackLocation })
	{
		if (is_regular_file(itr))
		{
			path targetPath{ targetLocation + itr.path().filename().string() };
			if (exists(targetPath))
			{
				auto targetLastWriteTime = last_write_time(targetPath);
				auto sourceLastWriteTime = last_write_time(itr);

				if (targetLastWriteTime == sourceLastWriteTime)
					continue;
				else
				{
					remove(targetPath);
					copy_file(itr, targetPath);

					last_write_time(targetPath, sourceLastWriteTime);
				}
			}
		}
	}
}


/************************************************************************************************/


void EditorProject::OpenIDE() const
{
	std::string command = defaultOpenIDE;
	command = SearchAndReplace(command, "@VCVARS", defaultVCVarsPath);
	command = SearchAndReplace(command, "@ProjectDrive", std::string{} + projectDirectory.string()[0] + ":");
	command = SearchAndReplace(command, "@ProjectPath", projectDirectory.string());

	system(command.c_str());
}


/************************************************************************************************/


void EditorProject::OpenExplorer() const
{
	std::string command = defaultOpenExplorer;
	command = SearchAndReplace(command, "@ProjectDrive", std::string{} + projectDirectory.string()[0] + ":");
	command = SearchAndReplace(command, "@ProjectPath", SearchAndReplace(projectDirectory.string(), "/", "\\"));

	system(command.c_str());
}


/************************************************************************************************/


void EditorProject::AddHeader(const std::string& name) const
{
	auto filePath = projectDirectory.string() + "/includes/" + name;
	if (exists(filePath))
		return;

	if (auto f = fopen(filePath.c_str(), "w"); f)
		fclose(f);
	else
		FK_LOG_ERROR("Failed to create file %s", filePath.c_str());

	projectNeedsCMakeRebuild = true;
}


/************************************************************************************************/


void EditorProject::AddSource(const std::string& name) const
{
	auto filePath = projectDirectory.string() + "/src/" + name;
	if (exists(filePath))
		return;

	if (auto f = fopen(filePath.c_str(), "w"); f)
		fclose(f);
	else
		FK_LOG_ERROR("Failed to create file %s", filePath.c_str());

	projectNeedsCMakeRebuild = true;
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


/************************************************************************************************/


std::string EditorProject::GetGameAssetsPath() const
{
	return projectDirectory.string() + R"(\assetPacks)";
}


/************************************************************************************************/


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
