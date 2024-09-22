#pragma once

#include <any>
#include <filesystem>
#include <memory>
#include <unordered_set>
#include <string>
#include <vector>

#include "EditorSceneResource.h"
#include <Scene.hpp>
#include <Serialization.hpp>
#include <Signals.hpp>

class ProjectWidget
{
public:
	enum class WidgetType
	{

	};

	std::vector<std::shared_ptr<ProjectWidget>> children;
};


/************************************************************************************************/


class ProjectLayout
{
public:
	void Restore();
	void Save(std::string& file);
};


/************************************************************************************************/


using ResourcePropertyID = uint32_t;

class ProjectResource
{
public:
	void Serialize(auto& ar)
	{
		std::string tag = "ProjectResource";
		ar& tag;

		ar& resource;
		//ar& properties;
	}


	FlexKit::Resource_ptr  resource;
	std::map<ResourcePropertyID, std::any>  properties;
};


/************************************************************************************************/


using GameObject_ptr		= std::shared_ptr<FlexKit::GameObject>;
using ProjectResource_ptr	= std::shared_ptr<ProjectResource>;


class EditorScene
{
public:
	EditorScene(FlexKit::SceneResource_ptr IN_scene = nullptr) : resource{ IN_scene } {}

	ProjectResource_ptr	FindSceneResource(uint64_t resourceID);

	void Serialize(auto& archive)
	{
		archive& name;
		archive& resources;

		if (archive.Loading())
		{
			std::shared_ptr<FlexKit::iResource> resource_temp;
			archive& resource_temp;

			resource = std::static_pointer_cast<FlexKit::SceneResource>(resource_temp);
		}
		else
		{
			auto casted = std::static_pointer_cast<FlexKit::SceneResource>(resource);
			archive& casted;
		}

	}

	std::string							name;
	FlexKit::SceneResource_ptr			resource;
	std::vector<ProjectResource_ptr>	resources;
};


using EditorScene_ptr = std::shared_ptr<EditorScene>;


/************************************************************************************************/


class EditorProject
{
public:
	EditorProject();
	~EditorProject();

	void					AddScene	(EditorScene_ptr scene);
	ProjectResource_ptr		AddResource	(FlexKit::Resource_ptr resource);

	FlexKit::ResourceList	GetResources() const;

	void					RemoveResource(FlexKit::Resource_ptr resource);
	ProjectResource_ptr		FindProjectResource(uint64_t assetID);
	ProjectResource_ptr		FindProjectResource(const std::string& id);

	void CreateProjectFileStructure(const std::string& projectDir);

	bool LoadProject(const std::string& projectDir);
	bool SaveProject(const std::string& projectDir);

	void lock();
	void unlock();

	void StartWatchingDirectories();
	void HeaderChanged(const std::string& changedFile);
	void HeaderAddedRemoved(const std::string& changedFile);

	void ResetProject();

	void RegenerateCMake() const;
	void ReconfigureCMake() const;

	void BuildDebug() const;

	void MoveAssets(const std::string& targetLocation) const;

	void OpenIDE() const;
	void OpenExplorer() const;

	void AddHeader(const std::string& name) const;
	void AddSource(const std::string& name) const;

	void Serialize(auto&& archive)
	{
		uint64_t fileVersion = 1;

		archive& fileVersion;
		archive& resources;
		archive& scenes;

		switch (fileVersion)
		{
		case 1:
		{
			archive& projectName;
			archive& version;
			archive& gitSource;
			archive& gitSourceHash;
			archive& debugBuildCMakeCommand;

			headerFiles.clear();
			sourceFiles.clear();
		}	break;
		}
	}

	std::string GetHeadersPath() const;
	std::string GetSourcesPath() const;
	std::string GetAssetsPath() const;
	std::string GetGameAssetsPath() const;
	std::string GetGeneratedPath() const;

	// Project Variables
	std::string projectName		= "flexApplication";
	std::string version			= "0.0.1";

	// Engine Source variables
	std::string gitSource;
	std::string gitSourceHash;
	std::string debugBuildCMakeCommand;

	std::shared_mutex m;

	std::filesystem::path				projectDirectory;
	std::vector<EditorScene_ptr>		scenes;
	std::vector<ProjectResource_ptr>	resources;

	std::unordered_set<std::string>		headerFiles;
	std::unordered_set<std::string>		sourceFiles;

	ProjectLayout						layout;
	class QFileSystemWatcher*			fileWatcher = nullptr;

	FlexKit::Signal<void (const std::string&)>	onHeaderRemoved;
	FlexKit::Signal<void (const std::string&)>	onHeaderAdded;
	FlexKit::Signal<void (const std::string&)>	onHeaderChanged;

	mutable bool projectNeedsCMakeRebuild = false;
};

std::string ProjectGetObjectDirectory();


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
