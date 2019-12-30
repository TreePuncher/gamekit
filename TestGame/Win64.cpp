/**********************************************************************

Copyright (c) 2018 Robert May

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

#include "..\buildsettings.h"
#include "..\coreutilities\Logging.h"
#include "..\coreutilities\Application.h"
#include "..\graphicsutilities\AnimationComponents.h"

#include "BaseState.h"
#include "client.cpp"
#include "Gameplay.cpp"
#include "host.cpp"
#include "lobbygui.cpp"
#include "MainMenu.cpp"
#include "MultiplayerState.cpp"
#include "MultiplayerGameState.cpp"
//#include "TextureStreamingTest.cpp"

#include <iostream>


void SetupTestScene(FlexKit::GraphicScene& scene, RenderSystem& renderSystem, iAllocator* allocator)
{
    const AssetHandle sphere    = 4967718927826386829;

    float3x3 m;
    m[0] = { 0, 0, 0 };
    m[1] = { 0, 0, 0 };
    m[2] = { 0, 0, 0 };
    m[3] = { 0, 0, 0 };

    auto [triMesh, loaded] = FindMesh(sphere);

    if (!loaded)
        triMesh = LoadTriMeshIntoTable(renderSystem, renderSystem.GetImmediateUploadQueue(), sphere);

    static const size_t N = 10;

    for (size_t Y = 0; Y < N; ++Y)
    {
        for (size_t X = 0; X < N; ++X)
        {
            float roughness = ((float)X + 0.5f) / (float)N;
            float anisotropic = 0.0f;//((float)Y + 0.5f) / (float)N;
            float kS = ((float)Y + 0.5f) / (float)N;

            auto& gameObject = allocator->allocate<GameObject>();
            auto node = FlexKit::GetNewNode();

            gameObject.AddView<DrawableView>(triMesh, node);

            SetMaterialParams(gameObject, float3(1.0f, 1.0f, 1.0f), kS, 1.0f, roughness, anisotropic, 0.0f);

            float W = (float)N * 1.5f;
            FlexKit::SetPositionW(node, float3{ (float)X * W, 0, (float)Y * W } - float3{ W * 0.5f, 0, W * 0.5f });

            scene.AddGameObject(gameObject, node);
        }
    }

}


int main(int argc, char* argv[])
{
    enum class ApplicationMode
    {
        Client,
        Host,
        TextureStreamingTestMode,
        GraphicsTestMode,
        PlaygroundMode,
    }   applicationMode = ApplicationMode::GraphicsTestMode;

    std::string name;
    std::string server;

    FlexKit::InitLog(argc, argv);
    FlexKit::SetShellVerbocity(FlexKit::Verbosity_1);
    FlexKit::AddLogFile("GameState.log", FlexKit::Verbosity_INFO);

#ifdef _DEBUG
    FlexKit::AddLogFile("GameState_Detailed.log", 
        FlexKit::Verbosity_9, 
        false);
#endif

    FK_LOG_INFO("Logging initialized started.");

    auto* Memory = CreateEngineMemory();
    EXITSCOPE(ReleaseEngineMemory(Memory));

    uint2 WH = uint2{ 1920, 1080 } * 1.4;

    for (size_t I = 0; I < argc; ++I)
    {
        const char* TextureStreamingTestStr = "TextureStreamingTest";

        if (!strncmp("-WH", argv[I], 2))
        {
            const auto X_str = argv[I + 1];
            const auto Y_str = argv[I + 2];

            const uint32_t X = atoi(X_str);
            const uint32_t Y = atoi(Y_str);

            WH = { X, Y };

            I += 2;
        }
        else if (!strncmp("-C", argv[I], 2))
        {
            try
            {
                std::cout << "Please Enter Player Name: \n";
                std::cin >> name;

                std::cout << "Please Enter Server: \n";
                std::cin >> server;
            }
            catch (...)
            {
                return -1;
            }
            applicationMode = ApplicationMode::Client;
        }
        else if (!strncmp("-S", argv[I], 2))
        {
            try
            {
                std::cout << "Please Enter Player Name: \n";
                std::cin >> name;
            }
            catch (...)
            {
                return -1;
            }
            applicationMode = ApplicationMode::Host;
        }
        else if (!strncmp(TextureStreamingTestStr, argv[I], strlen(TextureStreamingTestStr))) // 
            applicationMode = ApplicationMode::TextureStreamingTestMode;

        //app.PushArgument(argv[I]);
    }


    FlexKit::FKApplication app{ WH, Memory, 8 };

    FK_LOG_INFO("Set initial PlayState state.");
    auto& base = app.PushState<BaseState>(app);

    switch (applicationMode)
    {
        case ApplicationMode::Client:
        {
            AddAssetFile("assets\\Scene1.gameres");

            auto& NetState      = app.PushState<NetworkState>(base);
            auto& clientState   = app.PushState<GameClientState>(base, NetState, ClientGameDescription{ 1337, server.c_str(), name.c_str() });
        }   break;
        case ApplicationMode::Host:
        {
            AddAssetFile("assets\\Scene1.gameres");

            auto& NetState  = app.PushState<NetworkState>(base);
            auto& hostState = app.PushState<GameHostState>(base, NetState);
        }   break;
        case ApplicationMode::GraphicsTestMode:
        {
            AddAssetFile("assets\\Scene1.gameres");

            auto& gameState     = app.PushState<GameState>(base);
            auto& renderSystem  = app.GetFramework().GetRenderSystem();
            struct LoadStateData
            {
                bool Finished           = false;
                bool textureLoaded      = false;
                ResourceHandle cubeMap  = InvalidHandle_t;
                ResourceHandle hdrMap   = InvalidHandle_t;

                VertexBufferHandle  vertexBuffer;
            }&state = app.GetFramework().core.GetBlockMemory().allocate<LoadStateData>();

            state.vertexBuffer = renderSystem.CreateVertexBuffer(2048, false);
            renderSystem.RegisterPSOLoader(TEXTURE2CUBEMAP, { &renderSystem.Library.RSDefault, CreateTexture2CubeMapPSO });
            renderSystem.QueuePSOLoad(TEXTURE2CUBEMAP);

            auto Task =
                [&]()
                {
                    auto& framework             = app.GetFramework();
                    auto& allocator             = framework.core.GetBlockMemory();
                    auto& renderSystem          = framework.GetRenderSystem();
                    UploadQueueHandle upload    = renderSystem.GetUploadQueue();

                    auto HDRStack   = LoadHDR("assets/textures/lakeside_2k.hdr", 0, allocator);

                    const auto WH   = HDRStack.front().WH;
                    state.cubeMap   = renderSystem.CreateGPUResource(GPUResourceDesc::CubeMap({ 256, 256 }, FORMAT_2D::R16G16B16A16_FLOAT, 0, true));
                    base.cubeMap    = state.cubeMap;

                    renderSystem.SetDebugName(state.cubeMap, "Cube Map");

                    state.hdrMap    = MoveTextureBuffersToVRAM(
                        renderSystem,
                        upload,
                        HDRStack.begin(),
                        HDRStack.size(),
                        allocator,
                        FORMAT_2D::R32G32B32A32_FLOAT);

                    renderSystem.SetDebugName(state.hdrMap, "HDR Map");
                    renderSystem.SubmitUploadQueues(&upload);

                    state.textureLoaded = true;
                };

            auto& workItem = CreateLambdaWork(Task, app.GetFramework().core.GetBlockMemory());
            app.GetFramework().core.Threads.AddWork(workItem);

            // Setup test scene
            SetupTestScene(gameState.scene, app.GetFramework().GetRenderSystem(), app.GetFramework().core.GetBlockMemory());


            auto OnLoadUpdate =
                [&](EngineCore& core, UpdateDispatcher& dispatcher, double dT)
                {
                    const auto completedState = state.textureLoaded;
                    if(state.Finished)
                    {
                        app.PopState();

                        auto& playState = app.PushState<LocalPlayerState>(base, gameState);
                        //app.GetFramework().GetRenderSystem().ReleaseVB(state.vertexBuffer);
                        app.GetFramework().core.GetBlockMemory().free(&state);
                    }
                };

            auto OnLoadDraw = [&](EngineCore& core, UpdateDispatcher& dispatcher, FrameGraph& frameGraph, double dT)
            {
                if (state.textureLoaded)
                {
                    struct RenderTexture2CubeMap
                    {
                        FrameResourceHandle renderTarget;
                        ResourceHandle      hdrMap;
                        ResourceHandle      cubeMap;

                        DescriptorHeap      descHeap;
                        VBPushBuffer        vertexBuffer;
                    };

                    ClearVertexBuffer(frameGraph, state.vertexBuffer);

                    frameGraph.AddRenderTarget(state.cubeMap);
                    frameGraph.AddNode<RenderTexture2CubeMap>(
                        RenderTexture2CubeMap{},
                        [&](FrameGraphNodeBuilder& builder, RenderTexture2CubeMap& data)
                        {
                            data.renderTarget   = builder.WriteRenderTarget(state.cubeMap);
                            data.hdrMap         = state.hdrMap;
                            data.cubeMap        = state.cubeMap;

                            auto& allocator     = core.GetBlockMemory();
                            auto& renderSystem  = frameGraph.GetRenderSystem();

                            data.vertexBuffer = VBPushBuffer(state.vertexBuffer, sizeof(float4) * 6, renderSystem);
                            data.descHeap.Init2(renderSystem, renderSystem.Library.RSDefault.GetDescHeap(0), 20, allocator);
                            data.descHeap.NullFill(renderSystem, 20);
                        },
                        [](RenderTexture2CubeMap& data, FrameResources& resources, Context* ctx)
                        {
                            data.descHeap.SetSRV(resources.renderSystem, 0, data.hdrMap);

                            struct
                            {
                                float4 position;
                            }   vertices[] =
                            {
                                // Front
                                { {-1,   1,  1, 0} },
                                { { 1,   1,  1, 0} },
                                { {-1,  -1,  1, 0} },

                                { {-1,  -1,  1, 0} },
                                { {1,    1,  1, 0} },
                                { {1,   -1,  1, 0} },
                            };

                            ctx->SetRootSignature(resources.renderSystem.Library.RSDefault);
                            ctx->SetPipelineState(resources.GetPipelineState(TEXTURE2CUBEMAP));
                            ctx->SetScissorAndViewports({ data.cubeMap });
                            ctx->SetPrimitiveTopology(EInputTopology::EIT_POINT);
                            //ctx->SetVertexBuffers({ VertexBufferDataSet{ vertices, data.vertexBuffer } });

                            ctx->SetGraphicsDescriptorTable(4, data.descHeap);

                            for (size_t I = 0; I < 1; ++I)
                            {
                                ctx->SetRenderTargets({ resources.GetRenderTargetObject(data.renderTarget) }, false);
                                ctx->Draw(1);
                            }
                        });

                    struct Dummy {};
                    frameGraph.AddNode<Dummy>(
                        Dummy{},
                        [&](FrameGraphNodeBuilder& builder, Dummy& data)
                        {
                            builder.ReadShaderResource(state.cubeMap);
                        },
                        [](Dummy& data, FrameResources& resources, Context* ctx) {});

                    state.Finished = true;
                }
            };


            using LoadState = GameLoadScreenState<decltype(OnLoadUpdate), decltype(OnLoadDraw)>;
            app.PushState<LoadState>(base, OnLoadUpdate, OnLoadDraw);
        }   break;
        case ApplicationMode::PlaygroundMode:
        {
            AddAssetFile("assets\\Scene1.gameres");

            auto& gameState = app.PushState<GameState>(base);

            auto& completed = app.GetFramework().core.GetBlockMemory().allocate_aligned<bool>(false);

            auto Task =
                [&]()
                {
                    auto& framework             = app.GetFramework();
                    auto& allocator             = framework.core.GetBlockMemory();
                    auto& renderSystem          = framework.GetRenderSystem();
                    UploadQueueHandle upload    = renderSystem.GetUploadQueue();

                    auto HDRStack = LoadHDR("assets/textures/lakeside_2k.hdr", 6, allocator);

                    base.cubeMap = MoveTextureBuffersToVRAM(
                        renderSystem,
                        upload,
                        HDRStack.begin(),
                        HDRStack.size(),
                        allocator,
                        FORMAT_2D::R32G32B32A32_FLOAT);

                    renderSystem.SetDebugName(base.cubeMap, "HDR Map");
                    renderSystem.SubmitUploadQueues(&upload);

                    completed = true;
                };

            auto& workItem = CreateLambdaWork(Task, app.GetFramework().core.GetBlockMemory());
            app.GetFramework().core.Threads.AddWork(workItem);

            // Setup test scene
            LoadScene(app.GetFramework().core, gameState.scene, 10000);

            auto OnLoadUpdate =
                [&](EngineCore& core, UpdateDispatcher& dispatcher, double dT)
                {
                    const auto completedState = completed;
                    if(completedState)
                    {
                        app.PopState();

                        auto& playState = app.PushState<LocalPlayerState>(base, gameState);
                        //app.GetFramework().core.GetBlockMemory().free(&completed);
                    }
                };


            using LoadState = GameLoadScreenState<decltype(OnLoadUpdate)>;
            app.PushState<LoadState>(base, OnLoadUpdate);

        }   break;
        case ApplicationMode::TextureStreamingTestMode:
        {
            //auto& testState = app.PushState<TextureStreamingTest>(base);
        }   break;
    default:
        return -1;
    }

    FK_LOG_INFO("Running application...");
    app.Run();
    FK_LOG_INFO("Completed running application");

    FK_LOG_INFO("Started cleanup...");
    app.Release();
    FK_LOG_INFO("Completed cleanup.");

    return 0;
}
