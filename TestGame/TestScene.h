#ifndef TESTSCENE_H
#define TESTSCENE_H

#include "BaseState.h"
#include "MultiplayerGameState.h"

inline void SetupTestScene(
    FlexKit::GraphicScene&  scene,
    FlexKit::RenderSystem&  renderSystem,
    FlexKit::iAllocator*    allocator,
    GUID_t                  texture1,
    GUID_t                  texture2,
    CopyContextHandle       copyContext)
{
    /*
    const AssetHandle demonModel = 666;

    // Load Model
    auto model = GetMesh(renderSystem, demonModel, copyContext);

	static const size_t N = 10;
	static const float  W = (float)10;

    MaterialComponent& materials = MaterialComponent::GetComponent();
    MaterialHandle material = materials.CreateMaterial();
    materials.AddTexture(texture1, material);
    materials.AddTexture(texture2, material);

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
            gameObject.AddView<MaterialComponentView>(material);

			SetMaterialParams(
				gameObject,
				float3(1.0f, 1.0f, 1.0f), // albedo
				kS, // specular
				0.47f, // IOR
				anisotropic,
				roughness,
				0.0f);

            SetMaterialHandle(gameObject, GetMaterialHandle(gameObject));

			SetPositionW(node, float3{ (float)X * W, 0, (float)Y * W } - float3{ N * W / 2, 0, N * W / 2 });

            if (demonModel == 6666)
                Pitch(node, (float)pi / 2.0f);
            else
                Yaw(node, (float)pi);

			scene.AddGameObject(gameObject, node);
			SetBoundingSphereFromMesh(gameObject);
		}
	}

    materials.ReleaseMaterial(material);
    */
}


enum class TestScenes
{
    LocalIllumination,
	GlobalIllumination,
	ShadowTestScene,
    PhysXTest,
    AnimationTest
};


inline void StartTestState(FlexKit::FKApplication& app, BaseState& base, TestScenes scene = TestScenes::ShadowTestScene)
{
    AddAssetFile("assets\\TestScenes.gameres");
    AddAssetFile("assets\\ZeldaScene.gameres");
    AddAssetFile("assets\\CubeMapResource.gameres");
    AddAssetFile("assets\\skull.gameres");
    AddAssetFile("assets\\debugPlane.gameres");
    AddAssetFile("assets\\debugTexture.gameres");
    AddAssetFile("assets\\aRealDemon.gameres");
    AddAssetFile("assets\\primrose.gameres");

	auto& gameState     = app.PushState<GameState>(base);
	auto& renderSystem  = app.GetFramework().GetRenderSystem();

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

            if(0)
            {
                auto testPattern1       = 8001;
                auto testPattern2       = 8002;

                size_t          mipCount;
                uint2           WH;
                DeviceFormat    format;

                Vector<TextureBuffer> radiance   = LoadCubeMapAsset(2, mipCount, WH, format, allocator);
                base.GGXMap = MoveTextureBuffersToVRAM(
                    renderSystem,
                    upload,
                    radiance.begin(),
                    mipCount,
                    6,
                    format);

                Vector<TextureBuffer> irradience = LoadCubeMapAsset(1, mipCount, WH, format, allocator);
                base.irradianceMap = MoveTextureBuffersToVRAM(
                    renderSystem,
                    upload,
                    irradience.begin(),
                    mipCount,
                    6,
                    format);

                SetupTestScene(
                    gameState.scene,
                    app.GetFramework().GetRenderSystem(),
                    app.GetCore().GetBlockMemory(),
                    testPattern1, testPattern2, upload);

                renderSystem.SetDebugName(base.irradianceMap, "irradiance Map");
                renderSystem.SetDebugName(base.GGXMap, "GGX Map");
                renderSystem.SubmitUploadQueues(SYNC_Graphics, &upload);
            }
            else
            {
                const AssetHandle ShadowScene = 202;
                AddAssetFile("assets\\ShadowTestScene.gameres");
                FK_ASSERT(FlexKit::LoadScene(app.GetCore(), gameState.scene, ShadowScene), "Failed to load Scene!");
            }

            state.loaded = true;
		};

	auto& workItem = CreateWorkItem(loadTexturesTask, app.GetFramework().core.GetBlockMemory());
	app.GetFramework().core.Threads.AddWork(workItem);

	switch (scene)
	{
    case TestScenes::LocalIllumination:
        AddAssetFile("assets\\ShadowTestScene.gameres");

        break;
	case TestScenes::GlobalIllumination:
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
