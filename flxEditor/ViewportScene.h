#pragma once

#include "GraphicsComponents.h"
#include "EditorProject.h"
#include "physicsutilities.h"
#include "intersection.h"


/************************************************************************************************/


struct ViewportGameObject
{
    FlexKit::GameObject gameObject;
    uint64_t            objectID;

    operator FlexKit::GameObject& () { return gameObject; }

    std::vector<ProjectResource_ptr> resourceDependencies;
};

using ViewportGameObject_ptr    = std::shared_ptr<ViewportGameObject>;
using ViewportObjectList        = std::vector<ViewportGameObject_ptr>;


struct ViewportScene
{
    ViewportScene(EditorScene_ptr IN_sceneResource) :
            sceneResource   { IN_sceneResource  } {}

    ViewportObjectList  RayCast(FlexKit::Ray v) const;


    void Update();

    EditorScene_ptr                     sceneResource;
    std::vector<ViewportGameObject_ptr> sceneObjects;
    FlexKit::Scene                      scene           { FlexKit::SystemAllocator };
    FlexKit::LayerHandle                physicsLayer    = FlexKit::InvalidHandle_t;
};

struct ViewportSelection
{
    ViewportObjectList  viewportObjects;
    ViewportScene*      scene;
};


/************************************************************************************************/


class FlexKit::EntityComponent;

struct ViewportSceneContext
{
    ViewportScene& scene;

    struct Node
    {
        float3      position;
        Quaternion  orientation;
        float3      scale;
        int         parent = -1;
    };

    int MapNode(FlexKit::NodeHandle node)
    {
        if (node == FlexKit::InvalidHandle_t)
            return -1;

        if (auto res = nodeMap.find(node); res != nodeMap.end())
            return res->second;
        else
        {
            auto parent             = MapNode(FlexKit::GetParentNode(node));
            auto localPosition      = FlexKit::GetPositionL(node);
            auto localOrientation   = FlexKit::GetOrientation(node);
            auto localScale         = FlexKit::GetLocalScale(node);

            auto idx = nodes.size();
            nodes.emplace_back( localPosition, localOrientation, localScale );

            nodeMap[node] = idx;
            return idx;
        }
    }

    std::map<FlexKit::NodeHandle, int>  nodeMap = { { FlexKit::NodeHandle{ 0 }, 0 } };
    std::vector<Node>                   nodes   = { { { 0, 0, 0 }, { 0, 0, 0, 1 }, { 1, 1, 1 }, -1 }};
};


/************************************************************************************************/


class IEntityComponentRuntimeUpdater
{
public:
    inline static std::map<uint32_t, void (*)(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& runtime, ViewportSceneContext& scene)> updaters;

    template<typename TY, size_t UpdaterID>
    struct RegisterConstructorHelper
    {
        static bool Register()
        {
            IEntityComponentRuntimeUpdater::updaters[UpdaterID] = [](FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& runtime, ViewportSceneContext& scene)
            {
                TY::Update(component, runtime, scene);
            };

            return true;
        }

        inline static bool _registered = Register();
    };


    static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& runtime, ViewportSceneContext& scene)
    {
        if(updaters.find(runtime.ID) != updaters.end())
            updaters[runtime.ID](component, runtime, scene);
    }
};


/**********************************************************************

Copyright (c) 2021 Robert May

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
