#pragma once

#include <string>
#include <vector>
#include <memory>
#include <any>

#include "EditorSceneResource.h"
#include "Scene.h"
#include "Serialization.hpp"


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
	void					AddScene	(EditorScene_ptr scene);
	ProjectResource_ptr		AddResource	(FlexKit::Resource_ptr resource);

	FlexKit::ResourceList	GetResources() const;

	void					RemoveResource(FlexKit::Resource_ptr resource);
	ProjectResource_ptr		FindProjectResource(uint64_t assetID);
	ProjectResource_ptr		FindProjectResource(const std::string& id);

	bool LoadProject(const std::string& projectDir);
	bool SaveProject(const std::string& projectDir);

	std::vector<EditorScene_ptr>		scenes;
	std::vector<ProjectResource_ptr>	resources;
	ProjectLayout						layout;
};


/************************************************************************************************/


/**********************************************************************

Copyright (c) 2019-2021 Robert May

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
