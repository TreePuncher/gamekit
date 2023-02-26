#pragma once

#include "AnimationComponents.h"
#include "Application.h"
#include "GameFramework.h"
#include "EngineCore.h"
#include "Materials.h"
#include "WorldRender.h"
#include "TextRendering.h"
#include "defaultpipelinestates.h"
#include "Win32Graphics.h"
#include "TextureStreamingUtilities.h"
#include "RayTracingUtilities.h"
#include "DebugUI.h"
#include "GamepadInput.h"

#include <angelscript.h>
#include <angelscript/scriptstdstring/scriptstdstring.h>
#include <angelscript/scriptbuilder/scriptbuilder.h>
#include <imgui.h>
#include <TriggerComponent.hpp>

#include <fmt/printf.h>

/************************************************************************************************/


using namespace FlexKit;

class BaseState : public FrameworkState
{
public:
	BaseState(
		GameFramework& IN_Framework,
		FKApplication& IN_App,
		uint2 WH = { 1920, 1080 },
		bool fullscreen = false);

	~BaseState();

	UpdateTask* Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT);

	void PostDrawUpdate(EngineCore& core, double dT) override;

	void Resize(const uint2 WH);

	void DrawDebugHUD(EngineCore& core, UpdateDispatcher& dispatcher, FrameGraph& frameGraph, ReserveVertexBufferFunction& reserveVB, ReserveConstantBufferFunction& reserveCB, ResourceHandle renderTarget, double dT);
	void DEBUG_PrintDebugStats(EngineCore& core);


	void PixCapture();
	void BeginPixCapture();

	void EndPixCapture();

	asIScriptEngine* asEngine;

	FKApplication&  App;


	// Debug hud
	enum EHudMode
	{
		FPS_Counter,
		PhysXOverlay,
		Profiler,
		Disabled,
		ModeCount
	} HUDmode = EHudMode::FPS_Counter;
	bool						enableHud = true;

	// counters, timers
	float						t			= 0.0f;
	size_t						counter		= 0;

	// Pix capture
	bool						PIX_SingleFrameCapture	= false;
	bool						PIX_CaptureInProgress	= false;
	uint32_t					PIX_frameCaptures		= 0;

	// Default passes
	MaterialHandle				gbufferPass;
	MaterialHandle				gbufferAnimatedPass;

	// Graphics resources
	Win32RenderWindow			renderWindow;
	WorldRender					render;
	GBuffer						gbuffer;
	DepthBuffer					depthBuffer;
	VertexBufferHandle			vertexBuffer;
	ConstantBufferHandle		constantBuffer;

	// Components
	AnimatorComponent				animations;
	FABRIKComponent					IK;
	FABRIKTargetComponent			IKTargets;
	SceneNodeComponent				transforms;
	CameraComponent					cameras;
	StringIDComponent				ids;
	BrushComponent					brushes;
	MaterialComponent				materials;
	SceneVisibilityComponent		visables;
	PointLightComponent				pointLights;
	SkeletonComponent				skeletonComponent;
	PointLightShadowMap				shadowCasters;
	PhysXComponent  				physics;
	RigidBodyComponent				rigidBodies;
	StaticBodyComponent				staticBodies;
	CharacterControllerComponent	characterControllers;
	CameraControllerComponent		orbitCameras;
	//SoundSystem			            sounds;

	GamepadInput					gamepads;
	ImGUIIntegrator					debugUI;
	TextureStreamingEngine			streamingEngine;
	iRayTracer&						rtEngine;
};


/**********************************************************************

Copyright (c) 2023 Robert May

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
