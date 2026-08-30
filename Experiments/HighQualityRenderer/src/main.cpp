#include <ExampleFramework.hpp>
#include <WorldRender.hpp>
#include <TextureStreaming/TextureStreamingUtilities.hpp>
#include <ShadowMapping.hpp>
#include <Transforms.hpp>
#include "CameraComponent.hpp"
#include <objloader.hpp>
#include <imgui.h>

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
        const double offset = 123'123'123;
        //const double offset = 0;
        auto cameraNode = GetZeroedNode();
        auto& camera = cameraObject->AddView<CameraView>();
        camera.SetCameraNode(cameraNode);
        camera.SetCameraAspectRatio(GetRenderWindow().GetAspectRatio());
        auto& cameraNodeView = cameraObject->AddView<SceneNodeView>(cameraNode);
        cameraNodeView.TranslateWorld({ offset, offset + 4, 10 });
        activeCamera = camera;

        auto test       = LoadObj(R"(assets\test.obj)");
        auto light      = LoadObj(R"(assets\light.obj)");
        auto room       = LoadObj(R"(assets\room.obj)");
        auto suzanne    = LoadObj(R"(assets\goober.obj)");

        auto defaultMaterial = materials.CreateMaterial();
        materials.Add2Pass(defaultMaterial, GBufferStaticPassID);

        auto& testObj       = AllocateGameObject();
        auto& testNode      = testObj.AddView<SceneNodeView>(GetZeroedNode());
        auto& testObjMat    = testObj.AddView<MaterialView>(defaultMaterial);
        auto& testBrush     = testObj.AddView<BrushView>(test);
        testBrush.SetMaterial(testObjMat);
        testNode.SetPosition({ offset, offset, 0 });
        scene.AddGameObject(testObj);

        auto& lightObj      = AllocateGameObject();
        auto& lightNode     = lightObj.AddView<SceneNodeView>(GetZeroedNode());
        auto& lightObjMat   = lightObj.AddView<MaterialView>(defaultMaterial);
        auto& lightBrush    = lightObj.AddView<BrushView>(light);
        lightBrush.SetMaterial(lightObjMat);
        lightNode.SetPosition({ offset, offset, 0 });
        scene.AddGameObject(lightObj);

        auto& roomObj       = AllocateGameObject();
        auto& roomNode      = roomObj.AddView<SceneNodeView>(GetZeroedNode());
        auto& roomObjMat    = roomObj.AddView<MaterialView>(defaultMaterial);
        auto& roomBrush     = roomObj.AddView<BrushView>(room);
        roomBrush.SetMaterial(roomObjMat);
        roomNode.SetPosition({ offset, offset, 0 });
        scene.AddGameObject(roomObj);

        auto& suzanneObj    = AllocateGameObject();
        auto& suzanneNode   = suzanneObj.AddView<SceneNodeView>(GetZeroedNode());
        auto& suzanneMat    = suzanneObj.AddView<MaterialView>(defaultMaterial);
        auto& suzanneBrush  = suzanneObj.AddView<BrushView>(suzanne);
        suzanneBrush.SetMaterial(suzanneMat);
        suzanneNode.SetPosition({ offset, offset, 0 });
        scene.AddGameObject(suzanneObj);
        
        obj1 = &suzanneObj;

        const uint32_t end = 00;
        const float3 start  { offset + -1 * 1.5 * float(end) / 2.0f, offset, -20 };
        const float3 step   { 1.5, 1.5f, -1.5};
        for (uint32_t y = 0; y < end; y++)
        {
            for (uint32_t x = 0; x < end; x++)
            {
                for (uint32_t z = 0; z < end; z++)
                {
                    auto& suzanneObj1       = AllocateGameObject();
                    auto& sceneNodeView1    = suzanneObj1.AddView<SceneNodeView>(GetZeroedNode());
                    auto& suzanneMat1       = suzanneObj1.AddView<MaterialView>(defaultMaterial);
                    auto& suzanneBrush1     = suzanneObj1.AddView<BrushView>(suzanne);

                    suzanneBrush1->node = sceneNodeView1;
                    sceneNodeView1.SetPosition(start + step * float3{ x, y, z });
                    suzanneBrush1.SetMaterial(suzanneMat1);

                    scene.AddGameObject(suzanneObj1, sceneNodeView1);
                }
            }
        }

        worldRender.occlusionCulling = true;
    }

    ~HighQualityRenderingState()
    {
        scene.ClearScene();
        ReleaseGameObject(*cameraObject);
    }

    UpdateTask* Update(EngineCore& core, UpdateDispatcher& dispatcher, double dt) override
    {
        a = Clock.now();
        Yaw(*obj1, -pi * dt);

        return nullptr;
    }


    void DrawUI() override final
    {
        double3 xyz = GetWorldPosition(*cameraObject);

        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ 800, 200 });
        if (ImGui::Begin("FPS Counter"))
        {
            ImGui::Text("FPS: %u", GetFPS());
            ImGui::Text("FPS Time: %f ms", 1000.0f / GetFPS());
            ImGui::Text("CPU Time: %f ms", (float)d.count() / 1000.0f);
            ImGui::Text("Camera Position : %f, %f, %f", xyz.x, xyz.y, xyz.z);
        }   ImGui::End();
        
    }


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
        b = Clock.now();
        d = std::chrono::duration_cast<std::chrono::microseconds>(b - a);
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
    GameObject*                 obj1    = nullptr;

    CameraHandle                activeCamera = InvalidHandle;

    
    static const std::chrono::high_resolution_clock Clock;
    std::chrono::high_resolution_clock::time_point a;
    std::chrono::high_resolution_clock::time_point b;
    std::chrono::microseconds d;
};

struct Blank : ExampleState{};

int main()
{
    return RunExample<HighQualityRenderingState>();
}
