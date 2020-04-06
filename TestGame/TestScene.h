#ifndef TESTSCENE_H
#define TESTSCENE_H

#include "BaseState.h"
#include "MultiplayerGameState.h"


inline void SetupTestScene(FlexKit::GraphicScene& scene, FlexKit::RenderSystem& renderSystem, FlexKit::iAllocator* allocator)
{
	const AssetHandle demonModel = 40000;

    // Load Model
    auto model = GetMesh(renderSystem, demonModel);

	static const size_t N = 10;
	static const float  W = (float)30;

	for (size_t Y = 0; Y < N; ++Y)
	{
		for (size_t X = 0; X < N; ++X)
		{
			const float roughness     = ((float)X + 0.5f) / (float)N;
			const float anisotropic   = ((float)Y + 0.5f) / (float)N;
			const float kS            = 0.0f;//((float)Y + 0.5f) / (float)N;

			auto& gameObject = allocator->allocate<FlexKit::GameObject>();
			auto node = FlexKit::GetNewNode();

			gameObject.AddView<DrawableView>(model, node);

			SetMaterialParams(
				gameObject,
				float3(1.0f, 1.0f, 1.0f), // albedo
				kS, // specular
				0.47f, // IOR
				anisotropic,
				roughness,
				0.0f);

			
			SetPositionW(node, float3{ (float)X * W, 0, (float)Y * W } - float3{ N * W / 2, 0, N * W / 2 });

            if (demonModel == 6666)
                Pitch(node, (float)pi / 2.0f);
            else
                Yaw(node, (float)pi);

			scene.AddGameObject(gameObject, node);
			SetBoundingSphereFromMesh(gameObject);
		}
	}
}


enum class TestScenes
{
	GlobalIllumination,
	ShadowTestScene,
    PhysXTest,
    AnimationTest
};


inline void StartTestState(FlexKit::FKApplication& app, BaseState& base, TestScenes scene = TestScenes::ShadowTestScene)
{
    AddAssetFile("assets\\TestScenes.gameres");
    AddAssetFile("assets\\ZeldaScene.gameres");
    AddAssetFile("assets\\aRealDemon.gameres");
    AddAssetFile("assets\\CubeMapResource.gameres");
    AddAssetFile("assets\\skull.gameres");
    AddAssetFile("assets\\debugPlane.gameres");

	auto& gameState     = app.PushState<GameState>(base);
	auto& renderSystem  = app.GetFramework().GetRenderSystem();

	auto test = []{};

	auto testSynced = FlexKit::MakeSynchonized(test, app.GetCore().GetTempMemory());
	testSynced.Get()();

	struct LoadStateData
	{
		bool loaded = false;
		VertexBufferHandle  vertexBuffer;
	}&state = app.GetFramework().core.GetBlockMemory().allocate<LoadStateData>();

	state.vertexBuffer = renderSystem.CreateVertexBuffer(2048, false);
	renderSystem.RegisterPSOLoader(TEXTURE2CUBEMAP_IRRADIANCE,  { &renderSystem.Library.RSDefault, CreateTexture2CubeMapIrradiancePSO });
	renderSystem.RegisterPSOLoader(TEXTURE2CUBEMAP_GGX,         { &renderSystem.Library.RSDefault, CreateTexture2CubeMapGGXPSO});
	renderSystem.QueuePSOLoad(TEXTURE2CUBEMAP_GGX);
	renderSystem.QueuePSOLoad(TEXTURE2CUBEMAP_IRRADIANCE);

	auto loadTexturesTask =
		[&]()
		{
			auto& framework             = app.GetFramework();
			auto& allocator             = framework.core.GetBlockMemory();
			auto& renderSystem          = framework.GetRenderSystem();
			CopyContextHandle upload    = renderSystem.OpenUploadQueue();


            auto textureHandle      = 8000;
            auto DDSTexture         = UploadDDSFromAsset(textureHandle, renderSystem, upload, allocator);

            base.TestImage          = DDSTexture;
            base.virtualResource    = renderSystem.CreateGPUResource(
                GPUResourceDesc::ShaderResource(
                    { 2048, 2048 },
                    DeviceFormat::BC3_UNORM,
                    4,
                    1, true ));

            base.streamingEngine.BindAsset(textureHandle, base.virtualResource);

            size_t          MIPCount;
            uint2           WH;
            DeviceFormat    format;

            Vector<TextureBuffer> radiance   = LoadCubeMapAsset(2, MIPCount, WH, format, allocator);
            base.GGXMap = MoveTextureBuffersToVRAM(
                renderSystem,
                upload,
                radiance.begin(),
                MIPCount,
                6,
                format);

            Vector<TextureBuffer> irradience = LoadCubeMapAsset(1, MIPCount, WH, format, allocator);
            base.irradianceMap = MoveTextureBuffersToVRAM(
                renderSystem,
                upload,
                irradience.begin(),
                MIPCount,
                6,
                format);

			renderSystem.SetDebugName(base.irradianceMap, "irradiance Map");
			renderSystem.SetDebugName(base.GGXMap,        "GGX Map");
			renderSystem.SubmitUploadQueues(SYNC_Graphics, &upload);

            state.loaded = true;
		};

	auto& workItem = CreateWorkItem(loadTexturesTask, app.GetFramework().core.GetBlockMemory());
	app.GetFramework().core.Threads.AddWork(workItem);

	switch (scene)
	{
	case TestScenes::GlobalIllumination:
		SetupTestScene(gameState.scene, app.GetFramework().GetRenderSystem(), app.GetCore().GetBlockMemory());
		break;
	case TestScenes::ShadowTestScene:
	{
		iAllocator* allocator = app.GetCore().GetBlockMemory();
		LoadScene(app.GetCore(), gameState.scene, "ZeldaScene");

		/*
		static const size_t N = 30;
		static const float  W = (float)5;

		for (size_t Y = 0; Y < N; ++Y)
		{
			for (size_t X = 0; X < N; ++X)
			{
				auto node           = FlexKit::GetNewNode();
				auto& gameObject    = allocator->allocate<GameObject>();
				gameObject.AddView<PointLightView>(float3(1, 1, 1), 20, node);

				SetPositionW(node, float3{ (float)X * W, 10, (float)Y * W } -float3{ N * W / 2, 0, N * W / 2 });

				gameState.scene.AddGameObject(gameObject, node);
			}
		}
		*/
	 }  break;
    case TestScenes::PhysXTest:
    {
        auto& allocator = app.GetCore().GetBlockMemory();

        // Load Model
        const AssetHandle model = 1002;
        auto [triMesh, loaded] = FindMesh("Cube1x1x1");

        if (!loaded)
            triMesh = LoadTriMeshIntoTable(renderSystem, renderSystem.GetImmediateUploadQueue(), "Cube1x1x1");

        auto    floorShape = base.physics.CreateCubeShape({ 300, 1, 300 });
        auto    cubeShape  = base.physics.CreateCubeShape({ 0.5f, 0.5f, 0.5f });

        // Create static Box
        auto  staticCollider = base.physics.CreateStaticCollider(gameState.pScene, floorShape, { 0, -1.0f, 0 });
        auto& staticBox = allocator.allocate<GameObject>();

        staticBox.AddView<StaticBodyView>(staticCollider, gameState.pScene);
        auto staticNode = GetStaticBodyNode(staticBox);
        staticBox.AddView<DrawableView>(triMesh, staticNode);
        staticBox.AddView<SceneNodeView<>>(staticNode);
        EnableScale(staticBox, true);
        SetScale(staticBox, { 60, 0.01f, 60 });
        gameState.scene.AddGameObject(staticBox, staticNode);
        SetBoundingSphereRadius(staticBox, 400);


        // Create Rigid Body Box
        for (size_t y = 0; y < 1; y++)
        {
            for (size_t x = 0; x < 10; x++)
            {
                auto  rigidBody = base.physics.CreateRigidBodyCollider(gameState.pScene, cubeShape, { (float)x * 1.05f, (float)y * 1.05f, 0 });
                auto& dynamicBox = allocator.allocate<GameObject>();

                dynamicBox.AddView<RigidBodyView>(rigidBody, gameState.pScene);
                auto dynamicNode = GetRigidBodyNode(dynamicBox);
                dynamicBox.AddView<DrawableView>(triMesh, dynamicNode);

                dynamicBox.AddView<SceneNodeView<>>(dynamicNode);
                EnableScale(dynamicBox, true);
                SetScale(dynamicBox, { 0.1f, 0.1f, 0.1f });

                gameState.scene.AddGameObject(dynamicBox, dynamicNode);

                SetMaterialParams(
                    dynamicBox,
                    float3(0.5f, 0.5f, 0.5f), // albedo
                    (rand() % 1000) / 1000.0f, // specular
                    1.0f, // IOR
                    (rand() % 1000) / 1000.0f,
                    (rand() % 1000) / 1000.0f,
                    0.0f);
            }
        }

        /*
        for (size_t y = 0; y < 0; y++)
        {
            auto  rigidBody = base.physics.CreateRigidBodyCollider(gameState.pScene, cubeShape, { -50 , 3.0f * y + 1.0f, 0 });
            auto& dynamicBox = allocator.allocate<GameObject>();

            dynamicBox.AddView<RigidBodyView>(rigidBody, gameState.pScene);
            auto dynamicNode = GetRigidBodyNode(dynamicBox);
            dynamicBox.AddView<DrawableView>(triMesh, dynamicNode);
            gameState.scene.AddGameObject(dynamicBox, dynamicNode);
        }
        */
    }   break;
    case TestScenes::AnimationTest:
    {
        auto& allocator = app.GetCore().GetBlockMemory();

        if(1)
        {
            // Load cube Model
            const AssetHandle model = 1002;
            auto triMesh = GetMesh(renderSystem, model);
            auto& floor  = allocator.allocate<GameObject>();

            auto node = GetZeroedNode();
            floor.AddView<DrawableView>(triMesh, node);
            floor.AddView<SceneNodeView<>>(node);

            gameState.scene.AddGameObject(floor, node);

            EnableScale(floor, true);
            SetScale(floor, { 600, 1, 600 });
            SetBoundingSphereRadius(floor, 900);
        }

        // Load rig Model
        const AssetHandle riggedModel   = 1005;
        const AssetHandle skeleton      = 2000;
        auto triMesh                    = GetMesh(renderSystem, riggedModel);

        auto& GO = allocator.allocate<GameObject>();
        auto node = GetZeroedNode();
        GO.AddView<DrawableView>(triMesh, node);
        GO.AddView<SceneNodeView<>>(node);
        GO.AddView<SkeletonView>(GetTriMesh(GO), 2000);
        GO.AddView<StringIDView>("object1", strlen("object1"));

        ToggleSkinned(GO, true);

        gameState.scene.AddGameObject(GO, node);
    }
	default:
		break;
	}


	auto OnLoadUpdate =
		[&](EngineCore& core, UpdateDispatcher& dispatcher, double dT)
		{
			if(state.loaded)
			{
				core.RenderSystem.ReleaseVB(state.vertexBuffer);
				core.GetBlockMemory().release_allocation(state);

				app.PopState();

				auto& playState = app.PushState<LocalPlayerState>(base, gameState);
			}
		};

	using LoadState = GameLoadScreenState<decltype(OnLoadUpdate)>;
	app.PushState<LoadState>(base, OnLoadUpdate);
}


#endif // Include guard
