#include <ExampleFramework.hpp>
#include <WorldRender.hpp>
#include <TextureStreaming/TextureStreamingUtilities.hpp>
#include <ShadowMapping.hpp>
#include <Transforms.hpp>
#include "CameraComponent.hpp"
#include <objloader.hpp>

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
        brushes                 { GetAllocator() },

        gbuffer                 { GetRenderWindow().GetWH(), GetRenderSystem() },
        depthBuffer             { GetRenderSystem(), GetRenderWindow().GetWH() },

        cameraObject            { &AllocateGameObject() }
    {
        auto cameraNode = GetZeroedNode();
        auto& camera = cameraObject->AddView<CameraView>();
        camera.SetCameraNode(cameraNode);
        camera.SetCameraAspectRatio(GetRenderWindow().GetAspectRatio());
        auto& cameraNodeView = cameraObject->AddView<SceneNodeView>(cameraNode);
        cameraNodeView.TranslateWorld({ 0, 0, 10 });
        activeCamera = camera;

        auto test       = LoadObj(R"(assets\test.obj)");
        auto light      = LoadObj(R"(assets\light.obj)");
        auto room       = LoadObj(R"(assets\room.obj)");
        auto suzanne    = LoadObj(R"(assets\suzanne.obj)");

        auto defaultMaterial = materials.CreateMaterial();
        materials.Add2Pass(defaultMaterial, GBufferStaticPassID);

        auto& testObj = AllocateGameObject();
        testObj.AddView<SceneNodeView>(GetZeroedNode());
        auto& testObjMat = testObj.AddView<MaterialView>(defaultMaterial);
        auto& testBrush = testObj.AddView<BrushView>(test);
        testBrush.SetMaterial(testObjMat);
        scene.AddGameObject(testObj);

        auto& lightObj = AllocateGameObject();
        lightObj.AddView<SceneNodeView>(GetZeroedNode());
        auto& lightObjMat = lightObj.AddView<MaterialView>(defaultMaterial);
        auto& lightBrush = lightObj.AddView<BrushView>(light);
        lightBrush.SetMaterial(lightObjMat);
        scene.AddGameObject(lightObj);

        auto& roomObj = AllocateGameObject();
        roomObj.AddView<SceneNodeView>(GetZeroedNode());
        auto& roomObjMat = roomObj.AddView<MaterialView>(defaultMaterial);
        auto& roomBrush = roomObj.AddView<BrushView>(room);
        roomBrush.SetMaterial(roomObjMat);
        scene.AddGameObject(roomObj);

        auto& suzanneObj = AllocateGameObject();
        suzanneObj.AddView<SceneNodeView>(GetZeroedNode());
        auto& suzanneMat = suzanneObj.AddView<MaterialView>(defaultMaterial);
        auto& suzanneBrush = suzanneObj.AddView<BrushView>(suzanne);
        suzanneBrush.SetMaterial(suzanneMat);
        scene.AddGameObject(suzanneObj);

        worldRender.occlusionCulling = true;
    }

    ~HighQualityRenderingState()
    {
        scene.ClearScene();
        ReleaseGameObject(*cameraObject);
    }

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

        ClearDepthBuffer(frameGraph, depthBuffer.Get(), 1.0f);
        auto res = worldRender.DrawScene(dispatcher, frameGraph, sceneDesc, targets, GetAllocatorMT(), GetTempAllocatorMT());

        return nullptr;
    }

    void PostDraw(EngineCore& core, double dt) override final
    {
        worldRender.passHistories.GetHistory(GetRenderSystem(), activeCamera)->EndFrame();
    }


    TextureStreamingEngine      textureStreamingEngine;
    SceneVisibilityComponent    visibilityComponent;
    SceneNodeComponent          transformComponent;
    CameraComponent             cameras;
    MaterialComponent           materials;
    ShadowMapComponent          shadowMaps;
    LightComponent              lights;
    BrushComponent              brushes;

    WorldRender                 worldRender;

    ShadowMapper                shadowMapper;

    GBuffer                     gbuffer;
    DepthBuffer                 depthBuffer;

    Scene                       scene;

    GameObject*                 cameraObject = nullptr;

    CameraHandle                activeCamera = InvalidHandle;
};

int main()
{
    return RunExample<HighQualityRenderingState>();
}
