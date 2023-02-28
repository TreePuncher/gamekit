#pragma once

#include <Application.h>
#include <Scene.h>
#include <physicsutilities.h>
#include <TextureStreamingUtilities.h>
#include <TriggerComponent.h>
#include <WorldRender.h>
#include <Win32Graphics.h>
#include <DebugUI.h>
#include <..\source\Signals.h>

struct Portal
{
	uint64_t levelID;
	uint64_t spawnPointID;
};


class PhysicsTest;

struct PortalFactory
{
	PortalFactory(PhysicsTest*);

	void OnCreateView(
		FlexKit::GameObject&	gameObject,
		FlexKit::ValueMap		user_ptr,
		const std::byte*		buffer,
		const size_t			bufferSize,
		FlexKit::iAllocator*	allocator);

	PhysicsTest* test;
};

using PortalHandle		= FlexKit::Handle_t<32, GetTypeGUID(PortalComponentID)>;
using PortalComponent	= FlexKit::BasicComponent_t<Portal, PortalHandle, GetTypeGUID(PortalComponentID), PortalFactory>;
using PortalView		= PortalComponent::View;

struct Spawn
{
};


struct SpawnFactory
{
	static void OnCreateView(
		FlexKit::GameObject&	gameObject,
		FlexKit::ValueMap		user_ptr,
		const std::byte*		buffer,
		const size_t			bufferSize,
		FlexKit::iAllocator*	allocator);
};

using SpawnHandle		= FlexKit::Handle_t<32, GetTypeGUID(SpawnComponentBlob)>;
using SpawnComponent	= FlexKit::BasicComponent_t<Spawn, SpawnHandle, GetTypeGUID(SpawnComponentBlob), SpawnFactory>;
using SpawnView			= PortalComponent::View;

class PhysicsTest : public FlexKit::FrameworkState
{
public:
	PhysicsTest(FlexKit::GameFramework& IN_framework);
	~PhysicsTest() final;

	FlexKit::UpdateTask* Update(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT) final;
	FlexKit::UpdateTask* Draw(FlexKit::UpdateTask* update, FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT, FlexKit::FrameGraph& frameGraph) final;

	void PostDrawUpdate(FlexKit::EngineCore&, double dT) final;
	bool EventHandler(FlexKit::Event evt) final;

	void Action();

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
	FlexKit::TriggerComponent				triggers;

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

	FlexKit::CameraHandle					activeCamera = FlexKit::InvalidHandle;

	FlexKit::InputMap						inputMap;
	FlexKit::GameObject						cameraRig;
	FlexKit::GameObject						character;

	FlexKit::GameObject						floorCollider;

	FlexKit::RunOnceQueue<void(FlexKit::UpdateDispatcher&, FlexKit::FrameGraph&)>	runOnceQueue;

	FlexKit::ImGUIIntegrator		debugUI;

	FlexKit::float3 A = { 0, 0, 0 };
	FlexKit::float3 B = { 0, 10, 0 };

	PortalComponent	portalComponent;
	SpawnComponent	spawnComponent;
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
