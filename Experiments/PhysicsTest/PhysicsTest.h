#pragma once

#include "Player.hpp"

#include <Application.hpp>
#include <DebugUI.hpp>
#include <GameplayComponents.hpp>
#include <physicsutilities.hpp>
#include <Scene.hpp>
#include <Signals.hpp>
#include <TextureStreamingUtilities.hpp>
#include <TriggerComponent.hpp>
#include <WorldRender.hpp>
#include <Win32Graphics.hpp>


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
	FlexKit::LightComponent					pointLights;
	FlexKit::ShadowMapComponent				shadowMaps;
	FlexKit::FABRIKComponent				ikComponent;
	FlexKit::SkeletonComponent				skeletons;
	FlexKit::StringIDComponent				stringIDs;
	FlexKit::TriggerComponent				triggers;

	FlexKit::PhysXComponent					physx;
	FlexKit::RigidBodyComponent				rigidBodies;
	FlexKit::StaticBodyComponent			staticBodies;
	FlexKit::CharacterControllerComponent	characterController;

	FlexKit::GBuffer						gbuffer;
	FlexKit::DepthBuffer					depthBuffer;
	FlexKit::Win32RenderWindow*				renderWindow;
	FlexKit::ConstantBufferHandle			constantBuffer;
	FlexKit::VertexBufferHandle				vertexBuffer;

	FlexKit::WorldRender					renderer;
	FlexKit::TextureStreamingEngine			textureStreamingEngine;

	FlexKit::CameraHandle					activeCamera = FlexKit::InvalidHandle;

	FlexKit::InputMap						inputMap;
	FlexKit::GameObject						playerObject;
	FlexKit::GameObject						character;

	FlexKit::GameObject						floorCollider;

	FlexKit::RunOnceQueue<void(FlexKit::UpdateDispatcher&, FlexKit::FrameGraph&)>	runOnceQueue;

	struct DebugRayCast
	{
		FlexKit::float3 A = { 0, 0, 0 };
		FlexKit::float3 B = { 0, 10, 0 };
	};

	FlexKit::Vector<DebugRayCast> debugRays;

	PortalComponent	portalComponent;
	SpawnComponent	spawnComponent;
	PlayerComponent	playerComponent;
};



/**********************************************************************

Copyright (c) 2015 - 2023 Robert May

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
