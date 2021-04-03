#pragma once

#include <string>
#include <vector>
#include <memory>

#include "GraphicScene.h"
#include "..\FlexKitResourceCompiler\SceneResource.h"



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


class ProjectResource
{
public:
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & resource;
    }


    FlexKit::ResourceBuilder::Resource_ptr resource;
};


/************************************************************************************************/


using GameObject_ptr = std::shared_ptr<FlexKit::GameObject>;


class EditorScene
{
public:
    EditorScene(FlexKit::ResourceBuilder::SceneResource_ptr IN_scene = nullptr) : sceneResource{ IN_scene } {}

    void LoadScene();

    FlexKit::ResourceBuilder::Resource_ptr FindSceneResource(uint64_t resourceID);



    FlexKit::ResourceBuilder::SceneResource_ptr sceneResource;

    std::string sceneName;

    std::vector<ProjectResource>    sceneResources;
    std::vector<GameObject_ptr>     sceneObjects;
};


using EditorScene_ptr = std::shared_ptr<EditorScene>;


/************************************************************************************************/


class EditorProject
{
public:
    void AddScene(EditorScene_ptr scene)
    {
        scenes.emplace_back(scene);
    }

    void AddResource(FlexKit::ResourceBuilder::Resource_ptr resource)
    {
        resources.emplace_back(ProjectResource{ resource });
    }

    FlexKit::ResourceBuilder::ResourceList GetResources() const
    {
        FlexKit::ResourceBuilder::ResourceList out;

        for (auto& r : resources)
            out.push_back(r.resource);

        return out;
    }

    void RemoveResource(FlexKit::ResourceBuilder::Resource_ptr resource)
    {
        std::erase_if(resources, [&](auto& res) -> bool { return (res.resource == resource); });
    }


    bool LoadProject(const std::string& projectDir);
    bool SaveProject(const std::string& projectDir);


    std::vector<EditorScene_ptr>    scenes;
    std::vector<ProjectResource>    resources;
    ProjectLayout                   layout;
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
