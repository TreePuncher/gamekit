#pragma once
#include <Application.h>
#include <Scene.h>
#include <physicsutilities.h>
#include <TextureStreamingUtilities.h>
#include <WorldRender.h>
#include <Win32Graphics.h>
#include <DebugUI.h>
#include <TriggerComponent.h>


class TextureStreamingTest : public FlexKit::FrameworkState
{
public:
	TextureStreamingTest(FlexKit::GameFramework& IN_framework);
	~TextureStreamingTest() final;

	FlexKit::UpdateTask* Update(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT) final;
	FlexKit::UpdateTask* Draw(FlexKit::UpdateTask* update, FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT, FlexKit::FrameGraph& frameGraph) final;

	void PostDrawUpdate(FlexKit::EngineCore&, double dT) final;
	bool EventHandler(FlexKit::Event evt) final;

	bool rotate = false;
	bool streamingUpdates = true;

	FlexKit::AnimatorComponent				animators;
	FlexKit::CameraComponent				cameras;
	FlexKit::CameraControllerComponent		orbitCameras;
	FlexKit::SceneNodeComponent				sceneNodes;
	FlexKit::MaterialComponent				materials;
	FlexKit::SceneVisibilityComponent		visibilityComponent;
	FlexKit::BrushComponent					brushes;
	FlexKit::LightComponent					pointLights;
	FlexKit::ShadowMapComponent				pointLightShadowMaps;
	FlexKit::FABRIKComponent				ikComponent;
	FlexKit::SkeletonComponent				skeletons;
	FlexKit::TriggerComponent				triggers;

	FlexKit::PhysXComponent					physx;
	FlexKit::RigidBodyComponent				rigidBodies;
	FlexKit::StaticBodyComponent			staticBodies;

	FlexKit::GBuffer				gbuffer;
	FlexKit::DepthBuffer			depthBuffer;
	FlexKit::Win32RenderWindow		renderWindow;
	FlexKit::ConstantBufferHandle	constantBuffer;
	FlexKit::VertexBufferHandle		vertexBuffer;

	FlexKit::WorldRender			renderer;
	FlexKit::TextureStreamingEngine	textureStreamingEngine;

	FlexKit::LayerHandle			layer;
	FlexKit::Scene					scene;
	FlexKit::CameraHandle			activeCamera;

	FlexKit::GameObject				orbitCamera;

	FlexKit::RunOnceQueue<void(FlexKit::UpdateDispatcher&, FlexKit::FrameGraph&)>	runOnceQueue;

	FlexKit::ImGUIIntegrator		debugUI;
};
