#pragma once

#include <string>
#include <vector>
#include <memory>
#include <GraphicScene.h>

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
    FlexKit::ResourceBuilder::Resource_ptr resource;
};


/************************************************************************************************/


class ProjectScene
{
public:
    ProjectScene(FlexKit::ResourceBuilder::SceneResource_ptr IN_scene = nullptr) : sceneResource{ IN_scene } {}

    FlexKit::ResourceBuilder::SceneResource_ptr sceneResource;
    FlexKit::GraphicScene                       scene{ FlexKit::SystemAllocator };

    std::string sceneName;
};


/************************************************************************************************/


class EditorProject
{
public:
    void AddScene(FlexKit::ResourceBuilder::SceneResource_ptr scene)
    {
        scenes.emplace_back(ProjectScene{ scene });
    }

    void AddResource(FlexKit::ResourceBuilder::Resource_ptr resource)
    {
        resources.emplace_back(ProjectResource{ resource });
    }

    std::vector<ProjectScene>       scenes;
    std::vector<ProjectResource>    resources;
    ProjectLayout                   layout;
};


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
