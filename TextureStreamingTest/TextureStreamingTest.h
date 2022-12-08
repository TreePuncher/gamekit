#pragma once
#include <Application.h>
#include <Scene.h>
#include <TextureStreamingUtilities.h>
#include <WorldRender.h>
#include <Win32Graphics.h>

class TextureStreamingTest : public FlexKit::FrameworkState
{
public:
	TextureStreamingTest(FlexKit::GameFramework& IN_framework);
	~TextureStreamingTest() final;

	FlexKit::UpdateTask* Update(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT) final;
	FlexKit::UpdateTask* Draw(FlexKit::UpdateTask* update, FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT, FlexKit::FrameGraph& frameGraph) final;

	void PostDrawUpdate(FlexKit::EngineCore&, double dT) final;
	bool EventHandler(FlexKit::Event evt) final;



	FlexKit::CameraComponent			cameras;
	FlexKit::SceneNodeComponent			sceneNodes;
	FlexKit::MaterialComponent			materials;
	FlexKit::SceneVisibilityComponent	visibilityComponent;
	FlexKit::BrushComponent				brushes;
	FlexKit::PointLightComponent		pointLights;

	FlexKit::Win32RenderWindow		renderWindow;
	FlexKit::ConstantBufferHandle	constantBuffer;
	FlexKit::VertexBufferHandle		vertexBuffer;

	FlexKit::WorldRender			renderer;
	FlexKit::TextureStreamingEngine	textureStreamingEngine;
	FlexKit::Scene					scene;

	FlexKit::RunOnceQueue<void(FlexKit::UpdateDispatcher&, FlexKit::FrameGraph&)>	runOnceQueue;
};
