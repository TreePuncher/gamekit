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
	ShadowTestScene,
	PhysXTest,
};


inline void StartTestState(FlexKit::FKApplication& app, BaseState& base, TestScenes scene = TestScenes::ShadowTestScene)
{
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


	switch (scene)
	{
	case TestScenes::ShadowTestScene:
	{
		const AssetHandle ShadowScene = 2002;
		AddAssetFile("assets\\SuzanneShaderBallScene.gameres");

        //const AssetHandle ShadowScene = 1234;
        //AddAssetFile("assets\\lightTestScene.gameres");

        //const AssetHandle ShadowScene = 2082;
        //AddAssetFile("assets\\primRose.gameres");


		FK_ASSERT(FlexKit::LoadScene(app.GetCore(), gameState.scene, ShadowScene), "Failed to load Scene!");

		//DEBUG_ListSceneObjects(gameState.scene);

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
	default:
		break;
	}

    app.PushState<LocalPlayerState>(base, gameState);
}


#endif // Include guard
