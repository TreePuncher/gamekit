#include "PCH.h"
#include "EditorProject.h"
#include "EditorResource.h"
#include "MeshResource.h"
#include "EditorTextureResources.h"

#include <fstream>
#include <filesystem>
#include <QFileSystemWatcher>


#include "Serialization.hpp"

/************************************************************************************************/
// Project Global Parameters
inline static std::string objectsDirectory = "Objects/";

/************************************************************************************************/


ProjectResource_ptr EditorScene::FindSceneResource(uint64_t resourceID)
{
	for (auto& resource : resources)
		if (resource->resource->GetResourceGUID() == resourceID)
			return resource;

	return nullptr;
}

/************************************************************************************************/


EditorProject::EditorProject() :
	fileWatcher{ new QFileSystemWatcher{} }
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


void EditorProject::BuildDebug() const
{
	//boost::process::system("cmake --help");

	int x = 0;
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
