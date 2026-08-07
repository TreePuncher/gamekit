#include <ExampleFramework.hpp>
#include <WorldRender.hpp>
#include <TextureStreaming/TextureStreamingUtilities.hpp>
#include <ShadowMapping.hpp>
#include <Transforms.hpp>
#include "CameraComponent.hpp"


using namespace FlexKit;

struct HighQualityRenderingState : ExampleState
{
    HighQualityRenderingState() : 
        textureStreamingEngine  { GetRenderSystem(), GetThreads(), GetAllocatorMT() },
        worldRender             { GetRenderSystem(), GetAllocatorMT(), {}, {}}, 
        shadowMapper            { GetRenderSystem(), GetAllocator() },

        visibilityComponent     { GetAllocatorMT() },
        cameras                 { GetAllocator() },
        materials               { GetRenderSystem(), GetAllocatorMT(), &textureStreamingEngine },
        shadowMaps              { GetAllocatorMT() },
        lights                  { GetAllocator() },
        scene                   { GetAllocator() }, 

        gbuffer                 { GetRenderWindow().GetWH(), GetRenderSystem() },
        depthBuffer             { GetRenderSystem(), GetRenderWindow().GetWH() } {}


    UpdateTask* Update(EngineCore& core, UpdateDispatcher& dispatcher, double dt) override { return nullptr; }

    UpdateTask* Draw(UpdateTask* updateTask, EngineCore& core, UpdateDispatcher& dispatcher, double dt, FrameGraph& frameGraph) override
    {
        auto& cameraUpdate      = cameras.QueueCameraUpdate(dispatcher);
        auto& transformUpdate   = QueueTransformUpdateTask(dispatcher);

        cameraUpdate.AddInput(transformUpdate);

        DrawSceneDescription sceneDesc{
            .camera                 = activeCamera,
            .scene                  = scene,
            .dt                     = dt, 
            .t                      = GetRunningTime(),
            
            .gbuffer                = gbuffer,

            .transformDependency    = transformUpdate,
            .cameraDependency       = cameraUpdate,

        };

        WorldRender_Targets targets{
            .RenderTarget   = GetRenderWindow().GetBackBuffer(),
            .DepthTarget    = depthBuffer,
        };

        auto res = worldRender.DrawScene(dispatcher, frameGraph, sceneDesc, targets, GetAllocatorMT(), GetTempAllocatorMT());

        return nullptr;
    }


    SceneVisibilityComponent    visibilityComponent;
    SceneNodeComponent          transformComponent;
    CameraComponent             cameras;
    MaterialComponent           materials;
    ShadowMapComponent          shadowMaps;
    LightComponent              lights;

    WorldRender                 worldRender;
    TextureStreamingEngine      textureStreamingEngine;

    ShadowMapper                shadowMapper;

    GBuffer                     gbuffer;
    DepthBuffer                 depthBuffer;

    Scene                       scene;

    CameraHandle                activeCamera = InvalidHandle;

};

int main()
{
    return RunExample<HighQualityRenderingState>();
}
