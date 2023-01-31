#pragma once

#include <Application.h>
#include <Scene.h>
#include <physicsutilities.h>
#include <TextureStreamingUtilities.h>
#include <WorldRender.h>
#include <Win32Graphics.h>
#include <DebugUI.h>
#include <..\source\Signal.h>

struct GameObjectSignal
{
	using Signal_ty = FlexKit::Signal<void(FlexKit::GameObject&, void*)>;

	uint32_t	signalID;
	Signal_ty 	signal;
};

struct GameObjectSignals
{
	std::vector<GameObjectSignal> signals;
};

constexpr uint32_t SignalGroupID = GetTypeGUID(SignalGroupHandle);

using SignalGroupComponent = FlexKit::BasicComponent_t<GameObjectSignals, FlexKit::SignalGroupHandle, SignalGroupID>;

class PhysicsTest : public FlexKit::FrameworkState
{
public:
	PhysicsTest(FlexKit::GameFramework& IN_framework);
	~PhysicsTest() final;

	FlexKit::UpdateTask* Update(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT) final;
	FlexKit::UpdateTask* Draw(FlexKit::UpdateTask* update, FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT, FlexKit::FrameGraph& frameGraph) final;

	void PostDrawUpdate(FlexKit::EngineCore&, double dT) final;
	bool EventHandler(FlexKit::Event evt) final;

	FlexKit::AnimatorComponent				animators;
	FlexKit::CameraComponent				cameras;
	FlexKit::CameraControllerComponent		orbitCameras;
	FlexKit::SceneNodeComponent				sceneNodes;
	FlexKit::MaterialComponent				materials;
	FlexKit::SceneVisibilityComponent		visibilityComponent;
	FlexKit::BrushComponent					brushes;
	FlexKit::PointLightComponent			pointLights;
	FlexKit::PointLightShadowMap			pointLightShadowMaps;
	FlexKit::FABRIKComponent				ikComponent;
	FlexKit::SkeletonComponent				skeletons;
	FlexKit::StringIDComponent				stringIDs;

	SignalGroupComponent					signalGroups;

	FlexKit::PhysXComponent					physx;
	FlexKit::RigidBodyComponent				rigidBodies;
	FlexKit::StaticBodyComponent			staticBodies;
	FlexKit::CharacterControllerComponent	characterController;

	FlexKit::GBuffer						gbuffer;
	FlexKit::DepthBuffer					depthBuffer;
	FlexKit::Win32RenderWindow				renderWindow;
	FlexKit::ConstantBufferHandle			constantBuffer;
	FlexKit::VertexBufferHandle				vertexBuffer;

	FlexKit::WorldRender					renderer;
	FlexKit::TextureStreamingEngine			textureStreamingEngine;

	FlexKit::LayerHandle					layer;
	FlexKit::Scene							scene;
	FlexKit::CameraHandle					activeCamera = FlexKit::InvalidHandle;

	FlexKit::InputMap						inputMap;
	FlexKit::GameObject						cameraRig;

	FlexKit::GameObject						floorCollider;

	FlexKit::RunOnceQueue<void(FlexKit::UpdateDispatcher&, FlexKit::FrameGraph&)>	runOnceQueue;

	FlexKit::ImGUIIntegrator		debugUI;
};
