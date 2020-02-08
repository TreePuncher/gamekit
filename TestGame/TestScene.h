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

	auto& gameState     = app.PushState<GameState>(base);
	auto& renderSystem  = app.GetFramework().GetRenderSystem();

	auto test = []{};

	auto testSynced = FlexKit::MakeSynchonized(test, app.GetCore().GetTempMemory());
	testSynced.Get()();

	struct LoadStateData
	{
		bool Finished                   = false;
		bool textureLoaded              = false;
		ResourceHandle irradianceMap    = InvalidHandle_t;
		ResourceHandle GGXMap           = InvalidHandle_t;
		ResourceHandle sourceMap        = InvalidHandle_t;

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

			auto HDRStack   = LoadHDR("assets/textures/lebombo_1k.hdr", 0, allocator);

			const auto WH       = HDRStack.front().WH;
			state.irradianceMap = renderSystem.CreateGPUResource(GPUResourceDesc::CubeMap({ 32, 32 }, FORMAT_2D::R16G16B16A16_FLOAT, 1, true));
			state.GGXMap        = renderSystem.CreateGPUResource(GPUResourceDesc::CubeMap({ 128, 128 }, FORMAT_2D::R16G16B16A16_FLOAT, 6, true));
			base.irradianceMap  = state.irradianceMap;
			base.GGXMap         = state.GGXMap;

			state.sourceMap     = MoveTextureBuffersToVRAM(
				renderSystem,
				upload,
				HDRStack.begin(),
				HDRStack.size(),
				allocator,
				FORMAT_2D::R32G32B32A32_FLOAT);

			renderSystem.SetDebugName(state.sourceMap,     "Cube Map");
			renderSystem.SetDebugName(state.irradianceMap, "irradiance Map");
			renderSystem.SetDebugName(state.GGXMap,        "GGX Map");
			renderSystem.SubmitUploadQueues(&upload);

			state.textureLoaded = true;
			HDRStack.Release();
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


        // Create Rigid Body Box
        for (size_t itr = 0; itr < 30; itr++)
        {
            auto  rigidBody = base.physics.CreateRigidBodyCollider(gameState.pScene, cubeShape, { 0, 2.1f * itr + 1, 0 });
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
			const auto completedState = state.textureLoaded;
			if(state.Finished)
			{
				core.RenderSystem.ReleaseVB(state.vertexBuffer);
				core.GetBlockMemory().release_allocation(state);

				app.PopState();

				auto& playState = app.PushState<LocalPlayerState>(base, gameState);
			}
		};

	auto OnLoadDraw = [&](EngineCore& core, UpdateDispatcher& dispatcher, FrameGraph& frameGraph, double dT)
	{
		if (state.textureLoaded)
		{
			struct RenderTexture2CubeMap
			{
				FrameResourceHandle irradianceTarget;
				FrameResourceHandle ggxTarget;

				ResourceHandle      irradianceMap;
				ResourceHandle      ggxMap;
				ResourceHandle      irradianceSource;

				VBPushBuffer        vertexBuffer;
			};

			ClearVertexBuffer(frameGraph, state.vertexBuffer);

			frameGraph.AddRenderTarget(state.irradianceMap);
			frameGraph.AddRenderTarget(state.GGXMap);
			frameGraph.AddNode<RenderTexture2CubeMap>(
				RenderTexture2CubeMap{},
				[&](FrameGraphNodeBuilder& builder, RenderTexture2CubeMap& data)
				{
					data.irradianceTarget   = builder.WriteRenderTarget(state.irradianceMap);
					data.ggxTarget          = builder.WriteRenderTarget(state.GGXMap);
					data.ggxMap             = state.GGXMap;
					data.irradianceMap      = state.irradianceMap;
					data.irradianceSource   = state.sourceMap;

					auto& allocator     = core.GetBlockMemory();
					auto& renderSystem  = frameGraph.GetRenderSystem();

					data.vertexBuffer = VBPushBuffer(state.vertexBuffer, sizeof(float4) * 6, renderSystem);
				},
				[](RenderTexture2CubeMap& data, FrameResources& resources, Context& ctx, iAllocator& allocator)
				{
					DescriptorHeap descHeap;
					descHeap.Init2(ctx, resources.renderSystem.Library.RSDefault.GetDescHeap(0), 20, &allocator);
					descHeap.SetSRV(ctx, 0, data.irradianceSource);
					descHeap.NullFill(ctx, 20);

					ctx.SetRootSignature(resources.renderSystem.Library.RSDefault);
					ctx.SetPipelineState(resources.GetPipelineState(TEXTURE2CUBEMAP_IRRADIANCE));
					ctx.SetScissorAndViewports({ data.irradianceMap });
					ctx.SetPrimitiveTopology(EInputTopology::EIT_POINT);

					ctx.SetGraphicsDescriptorTable(3, descHeap);

					ctx.SetRenderTargets({ resources.GetRenderTarget(data.irradianceTarget) }, false);
					ctx.Draw(1);


					ctx.SetPipelineState(resources.GetPipelineState(TEXTURE2CUBEMAP_GGX));

					for (size_t I = 0; I < 6; I++)
					{
						ctx.SetScissorAndViewports2({ data.ggxMap }, I);
						ctx.SetRenderTargets({ data.ggxMap }, false, InvalidHandle_t, I);
						ctx.Draw(1);
					}
				});

			struct Dummy {};
			frameGraph.AddNode<Dummy>(
				Dummy{},
				[&](FrameGraphNodeBuilder& builder, Dummy& data)
				{
					builder.ReadShaderResource(state.irradianceMap);
					builder.ReadShaderResource(state.GGXMap);
				},
				[](Dummy& data, FrameResources& resources, Context& ctx, iAllocator&) {});

			state.Finished = true;
		}
	};


	using LoadState = GameLoadScreenState<decltype(OnLoadUpdate), decltype(OnLoadDraw)>;
	app.PushState<LoadState>(base, OnLoadUpdate, OnLoadDraw);
}


#endif // Include guard
