#pragma once
#include <Application.hpp>
#include <Scene.hpp>
#include <PhysicsUtilities.hpp>
#include <PlanetComponent.hpp>
#include <TextureStreamingUtilities.hpp>
#include <WorldRender.hpp>
#include <Win32Graphics.hpp>
#include <DebugUI.hpp>
#include <TriggerComponent.hpp>


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

	FlexKit::PlanetComponent				planets;

	FlexKit::GBuffer				gbuffer;
	FlexKit::DepthBuffer			depthBuffer;
	FlexKit::ConstantBufferHandle	constantBuffer;
	FlexKit::VertexBufferHandle		vertexBuffer;
	FlexKit::Win32RenderWindow*		renderWindow;

	FlexKit::WorldRender			renderer;
	FlexKit::TextureStreamingEngine	textureStreamingEngine;

	FlexKit::LayerHandle			layer;
	FlexKit::Scene					scene;
	FlexKit::CameraHandle			activeCamera;

	FlexKit::GameObject				orbitCamera;
	FlexKit::GameObject				testPlanet;

	FlexKit::RunOnceQueue<void(FlexKit::UpdateDispatcher&, FlexKit::FrameGraph&)>	runOnceQueue;
};
