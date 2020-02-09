#ifndef TESTSCENE_H
#define TESTSCENE_H

#include "BaseState.h"
#include "MultiplayerGameState.h"


inline void SetupTestScene(FlexKit::GraphicScene& scene, FlexKit::RenderSystem& renderSystem, FlexKit::iAllocator* allocator)
{
	const AssetHandle model = 1001;

	auto [triMesh, loaded] = FindMesh(model);

	if (!loaded)
		triMesh = LoadTriMeshIntoTable(renderSystem, renderSystem.GetImmediateUploadQueue(), model);

	static const size_t N = 10;
	static const float  W = (float)30;

	for (size_t Y = 0; Y < N; ++Y)
	{
		for (size_t X = 0; X < N; ++X)
		{
			float roughness     = ((float)X + 0.5f) / (float)N;
			float anisotropic   = 0.0f;//((float)Y + 0.5f) / (float)N;
			float kS            = ((float)Y + 0.5f) / (float)N;

			auto& gameObject = allocator->allocate<GameObject>();
			auto node = FlexKit::GetNewNode();

			gameObject.AddView<DrawableView>(triMesh, node);

			SetMaterialParams(
				gameObject,
				float3(1.0f, 1.0f, 1.0f), // albedo
				kS, // specular
				1.0f, // IOR
				anisotropic,
				roughness,
				0.0f);

			
			SetPositionW(node, float3{ (float)X * W, 0, (float)Y * W } - float3{ N * W / 2, 0, N * W / 2 });
			Scale(node, { 3, 3, 3 });
			Roll(node, (float)-pi / 2.0f);
			SetFlag(node, SceneNodes::StateFlags::SCALE);

			scene.AddGameObject(gameObject, node);
			SetBoundingSphereFromMesh(gameObject);
		}
	}
}


enum class TestScenes
{
	GlobalIllumination,
	ShadowTestScene,
    PhysXTest
};


inline void StartTestState(FlexKit::FKApplication& app, BaseState& base, TestScenes scene = TestScenes::ShadowTestScene)
{
    AddAssetFile("assets\\TestScenes.gameres");
    AddAssetFile("test.gameres");

	auto& gameState     = app.PushState<GameState>(base);
	auto& renderSystem  = app.GetFramework().GetRenderSystem();

	auto test = []{};

	auto testSynced = FlexKit::MakeSynchonized(test, app.GetCore().GetTempMemory());
	testSynced.Get()();

	struct LoadStateData
	{
		bool loaded                     = false;
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
			UploadQueueHandle upload    = renderSystem.GetUploadQueue();

            size_t      MIPCount;
            uint2       WH;
            FORMAT_2D   format;
            Vector<TextureBuffer> GGXStack = LoadCubeMapAsset(1, MIPCount, WH, format, allocator);

			//const auto WH       = HDRStack.front().WH;
            base.irradianceMap = renderSystem.CreateGPUResource(GPUResourceDesc::CubeMap({ 32, 32 }, FORMAT_2D::R32G32B32A32_FLOAT, 1, true));
			//base.GGXMap        = renderSystem.CreateGPUResource(GPUResourceDesc::CubeMap({ 1024, 1024}, FORMAT_2D::R32G32B32A32_FLOAT, 1, true));

            base.GGXMap = MoveTextureBuffersToVRAM(
				renderSystem,
                upload,
                GGXStack.begin(),
                MIPCount,
                6,
                format,
                allocator);
			renderSystem.SetDebugName(base.irradianceMap, "irradiance Map");
			renderSystem.SetDebugName(base.GGXMap,        "GGX Map");
			renderSystem.SubmitUploadQueues(&upload);

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
		LoadScene(app.GetCore(), gameState.scene, "ShadowsTestScene");

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
        auto [triMesh, loaded] = FindMesh(model);

        if (!loaded)
            triMesh = LoadTriMeshIntoTable(renderSystem, renderSystem.GetImmediateUploadQueue(), model);

        auto    floorShape = base.physics.CreateCubeShape({ 300, 1, 300 });
        auto    cubeShape  = base.physics.CreateCubeShape({ 1, 1, 1 });

        // Create static Box
        auto  staticCollider = base.physics.CreateStaticCollider(gameState.pScene, floorShape, { 0, 0, 0 });
        auto& staticBox = allocator.allocate<GameObject>();

        staticBox.AddView<StaticBodyView>(staticCollider, gameState.pScene);
        auto staticNode = GetStaticBodyNode(staticBox);
        staticBox.AddView<DrawableView>(triMesh, staticNode);
        staticBox.AddView<SceneNodeView<>>(staticNode);
        EnableScale(staticBox, true);
        SetScale(staticBox, { 300, 1, 300 });
        gameState.scene.AddGameObject(staticBox, staticNode);
        SetBoundingSphereRadius(staticBox, 400);

        // Create Rigid Body Box
        for (size_t y = 0; y < 20; y++)
        {
            for (size_t x = 0; x < 50; x++)
            {
                auto  rigidBody = base.physics.CreateRigidBodyCollider(gameState.pScene, cubeShape, { 2.1f * x + 1, 3.0f * y + 1.1f, 0 });
                auto& dynamicBox = allocator.allocate<GameObject>();

                dynamicBox.AddView<RigidBodyView>(rigidBody, gameState.pScene);
                auto dynamicNode = GetRigidBodyNode(dynamicBox);
                dynamicBox.AddView<DrawableView>(triMesh, dynamicNode);
                gameState.scene.AddGameObject(dynamicBox, dynamicNode);
            }
        }

        for (size_t y = 0; y < 0; y++)
        {
            auto  rigidBody = base.physics.CreateRigidBodyCollider(gameState.pScene, cubeShape, { -50 , 3.0f * y + 1.1f, 0 });
            auto& dynamicBox = allocator.allocate<GameObject>();

            dynamicBox.AddView<RigidBodyView>(rigidBody, gameState.pScene);
            auto dynamicNode = GetRigidBodyNode(dynamicBox);
            dynamicBox.AddView<DrawableView>(triMesh, dynamicNode);
            gameState.scene.AddGameObject(dynamicBox, dynamicNode);
        }
    }   break;
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
