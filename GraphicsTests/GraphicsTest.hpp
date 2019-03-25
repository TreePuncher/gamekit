/**********************************************************************

Copyright (c) 2018 Robert May

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


#ifndef GRAPHICSTEST_INCLUDED
#define GRAPHICSTEST_INCLUDED

#include "../coreutilities/GameFramework.h"
#include "../physicsUtilities/physicsUtilities.h"

const size_t lookupTableSize	= 128;

enum TestEvents
{
	ReloadShaders_1 = 20,
	ToggleWireframe,
	Forward,
	Backward,
	Left,
	Right
};

using namespace FlexKit;


void NelderMead()
{

}

struct BRDF_ggx
{
	static float3 F_Schlick(float3 f0, float3 f90, float u)
	{
		return f0 + (f90 - f0) * pow(1.0f - u, 5.0f);
	}

	static float Fr_Disney(float ndotv, float ndotl, float ldoth, float linearRoughness) // Fresnel Factor
	{
		float energyBias	= lerp(0.0f, 0.5f,			linearRoughness);
		float energyFactor	= lerp(1.0f, 1.0f / 1.51f,	linearRoughness);
		float fd90 			= energyBias + 2.0 * ldoth * ldoth * linearRoughness;
		float3 f0 			= float3(1.0f, 1.0f, 1.0f);

		float lightScatter	= F_Schlick(f0, fd90, ndotl).x;
		float viewScatter	= F_Schlick(f0, fd90, ndotv).x;

		return lightScatter * viewScatter * energyFactor;
	}

	static float V_SmithGGXCorrelated(float ndotl, float ndotv, float alphaG)
	{
		float alphaG2       = alphaG * alphaG;
		float Lambda_GGXV   = ndotl * sqrt((-ndotv * alphaG2 + ndotv) * ndotv + alphaG2);
		float Lambda_GGXL   = ndotv * sqrt((-ndotl * alphaG2 + ndotl) * ndotl + alphaG2);
		return 0.5f / (Lambda_GGXV + Lambda_GGXL);
	}

	static float D_GGX(float ndoth , float m)
	{
		//Divide by PI is apply later
		float m2    = m * m;
		float f     = (ndoth * m2 - ndoth) * ndoth + 1;
		return m2 / (f * f);
	} 

	static float DistanceSquared2(float2 a, float2 b)
	{
 		a -= b; 
 		return DotProduct2(a, a);
	}

	static float DistanceSquared3(float2 a, float2 b)
	{
 		a -= b;
 		return DotProduct2(a, a);
	}

	static float3 Frd(float3 l, float3 v, float3 n, float3 Ks, float m, float r)
	{
		float3 h  = (v + l).normal();
		float  A  = saturate(pow(r, 2));

		float ndotv = saturate(dot(n, v) + 1e-5);
		float ndotl = saturate(dot(n, l));
		float ndoth = saturate(dot(n, h));
		float ldoth = saturate(dot(l, h));
	
		//	Specular BRDF
		float3 F    = F_Schlick(Ks, 0.0f, ldoth);
		float Vis   = V_SmithGGXCorrelated(ndotv, ndotl, A);
		float D     = D_GGX(ndoth, A);
		float3 Fr   = D * F * Vis;

		// 	Diffuse BRDF
		float Fd = Fr_Disney(ndotv, ndotl, ldoth, A) / (4.0f * pi * pi);

		return float3(Fr + (Fd * (1 - m))) * ndotl;
	}

	static float	Eval	(const float3& V, const float3& L,	float3 N, const float alpha, float& pdf)
	{
		pdf = V.z / pi;
		return Frd(L, V, N, float3(1), 0, alpha).magnitude();
	}
	
	static float3	Sample	(const float3& V, const float alpha,	const float U1,		const	float U2)
	{
		const float r	= sqrtf(U1);
		const float phi = 2.0f*3.14159f * U2;
		const float3 L	= float3(r*cosf(phi), r*sinf(phi), sqrtf(1.0f - r * r));
		return L;
	};
};


/************************************************************************************************/


class BrdfDisneyDiffuse
{
public:

	static float Eval(const float3 V, const float3 L, const float alpha, float& pdf)
	{
		if(V.z <= 0 || L.z <= 0)
		{
			pdf = 0;
			return 0;
		}

		pdf = L.z / 3.14159f;

		float NdotV = V.z;
		float NdotL = L.z;
		float LdotH = dot(L, (V+L).normal());
		float perceptualRoughness = sqrtf(alpha);
		float fd90 = 0.5 + 2 * LdotH * LdotH * perceptualRoughness;
		float lightScatter    = (1 + (fd90 - 1) * powf(1 - NdotL, 5.0f));
		float viewScatter    = (1 + (fd90 - 1) * powf(1 - NdotV, 5.0f));
		return lightScatter * viewScatter * L.z / 3.14159f;
	}

	static float3 Sample(const float3 V, const float alpha, const float U1, const float U2)
	{
		const float r	= sqrtf(U1);
		const float phi = 2.0f*3.14159f * U2;
		const float3 L	= float3(r*cosf(phi), r*sinf(phi), sqrtf(1.0f - r*r));
		return L;
	}	
};


/************************************************************************************************/


float ComputeBRDFNorm(const float3& v, const float alpha, const int sampleCount = 64)
{
	float albedo = 0.0f;
	for (size_t i = 0; i < sampleCount; ++i)
	{
		for (size_t j = 0; j < sampleCount; ++j)
		{
			const float U1 = (i + 0.5f) / (float)sampleCount;
			const float U2 = (j + 0.5f) / (float)sampleCount;

			const float3 l	= BrdfDisneyDiffuse::Sample(v, alpha, U1, U2);
			float pdf		= 0;

			float eval1 = BRDF_ggx::Eval(v, l, {0, 0, 1}, alpha, pdf);
			float eval2	= BrdfDisneyDiffuse::Eval(v, l, alpha, pdf);

			albedo += (pdf > 0.0f ) ? eval2 / pdf : 0.0f;
		}
	}

	return albedo / (sampleCount * sampleCount);
}


/************************************************************************************************/

const size_t CubeCount = 1;

class GraphicsTest : public FrameworkState
{
public:
	GraphicsTest(GameFramework* IN_framework) :
		FrameworkState(IN_framework),
		render			{ IN_framework->core->GetBlockMemory(), IN_framework->core->RenderSystem						},
		depthBuffer		{ IN_framework->core->RenderSystem.CreateDepthBuffer	(GetWindowWH(IN_framework->core), true)	},
		vertexBuffer	{ IN_framework->core->RenderSystem.CreateVertexBuffer	(8096 * 32,			false)				},
		textBuffer		{ IN_framework->core->RenderSystem.CreateVertexBuffer	(8096 * 32,			false)				},
		constantBuffer	{ IN_framework->core->RenderSystem.CreateConstantBuffer	(MEGABYTE * 256,	false)				},
		eventMap		{ IN_framework->core->GetBlockMemory()															},
		terrain			{ IN_framework->GetRenderSystem(), IN_framework->core->GetBlockMemory()							},
		scene			{	
			IN_framework->core->RenderSystem, 
			IN_framework->core->GetBlockMemory(),
			IN_framework->core->GetTempMemory()	},
		PScene			{ IN_framework->GetPhysx()->CreateScene()														},
		floor			{ IN_framework->GetPhysx()->CreateStaticBoxCollider		(PScene, {1000, 10, 1000}, {0, -10, 0})	},
		orbitCamera		{ CreateCamera(pi / 3.0f, GetWindowAspectRatio(IN_framework->core), 0.1f, 100000.0f), 3000, {0, 50, 50} }
		//ltcLookup_1{ IN_framework->Core->RenderSystem.CreateTexture2D(FlexKit::uint2{lookupTableSize, lookupTableSize}, FlexKit::FORMAT_2D::R32G32B32A32_FLOAT)},
		//ltcLookup_2{ IN_framework->Core->RenderSystem.CreateTexture2D(FlexKit::uint2{lookupT's ableSize, lookupTableSize}, FlexKit::FORMAT_2D::R32G32B32A32_FLOAT)}
	{
		AddResourceFile("testScene.gameres");
		AddResourceFile("testHead.gameres");
		
		obj = GetMesh(GetRenderSystem(), 10000);

		for (size_t itr = 0; itr < CubeCount; itr++) {
			box[itr] = std::move(CreateRBCube(&scene, obj, IN_framework->GetPhysx(), PScene, 1, {0, 1.1f  + 1.1f * itr * 2, 0}));
			box[itr].SetVisable(true);
			box[itr].SetMeshScale(10);
			box[itr].SetMass(10);
		}

		auto& RS = IN_framework->core->RenderSystem;
		RS.RegisterPSOLoader(DRAW_PSO,					{ &RS.Library.RS6CBVs4SRVs, CreateDrawTriStatePSO			});
		RS.RegisterPSOLoader(DRAW_TEXTURED_PSO,			{ &RS.Library.RS6CBVs4SRVs, CreateTexturedTriStatePSO		});
		RS.RegisterPSOLoader(DRAW_TEXTURED_DEBUG_PSO,	{ &RS.Library.RS6CBVs4SRVs, CreateTexturedTriStateDEBUGPSO	});
		RS.RegisterPSOLoader(DRAW_LINE_PSO,				{ &RS.Library.RS6CBVs4SRVs, CreateDrawLineStatePSO			});
		RS.RegisterPSOLoader(DRAW_LINE3D_PSO,			{ &RS.Library.RS6CBVs4SRVs, CreateDraw2StatePSO				});
		RS.RegisterPSOLoader(DRAW_SPRITE_TEXT_PSO,		{ &RS.Library.RS6CBVs4SRVs, LoadSpriteTextPSO				});

		RS.QueuePSOLoad(DRAW_PSO);
		RS.QueuePSOLoad(DRAW_LINE3D_PSO);
		RS.QueuePSOLoad(DRAW_TEXTURED_DEBUG_PSO);


		/*
		if (!LoadScene(IN_framework->core, &scene, 6001))
		{
			std::cerr << "Failed to Load Scene!\n";
			IN_framework->quit = true;
		}

		std::cout << "Objects in Scene: "	<< scene.Drawables.size() << "\n";
		std::cout << "Lights in Scene: "	<< scene.PLights.size() << "\n";

		scene.ListEntities();
		*/

		orbitCamera.TranslateWorld({0, 50, 50});

		eventMap.MapKeyToEvent(KEYCODES::KC_R, ReloadShaders_1);
		eventMap.MapKeyToEvent(KEYCODES::KC_W, OCE_MoveForward);
		eventMap.MapKeyToEvent(KEYCODES::KC_S, OCE_MoveBackward);
		eventMap.MapKeyToEvent(KEYCODES::KC_A, OCE_MoveLeft);
		eventMap.MapKeyToEvent(KEYCODES::KC_D, OCE_MoveRight);
		eventMap.MapKeyToEvent(KEYCODES::KC_Q, ToggleWireframe);

		const int TileSize = 1024 * 8;
		terrain.SetTileOffset	(int2{-TileSize/2, -TileSize/2});
		terrain.SetTileSize		(TileSize); // will assume square tiles
		terrain.AddTile			({ 0, 0 }, "assets/textures/heightmap.dds", framework->GetRenderSystem(), framework->core->GetTempMemory());
		terrain.DebugMode	= TerrainEngine::WIREFRAME;

		/*
		float3x3*	ltc_Table		= (float3x3*)	IN_framework->Core->GetTempMemory()._aligned_malloc(sizeof(FlexKit::float3x3[lookupTableSize*lookupTableSize]));
		float*		ltc_Amplitude	= (float*)		IN_framework->Core->GetTempMemory()._aligned_malloc(sizeof(float [lookupTableSize*lookupTableSize]));

		const float MinAlpha = 0.0001f;

		for (size_t a = lookupTableSize; a >= 0; --a)// steps roughness
		{
			for (size_t t = 0; t < lookupTableSize; ++t)// steps angle
			{
				size_t idx = a * lookupTableSize + t;

				float roughness		= a / (lookupTableSize - 1);
				float alpha			= FlexKit::max(roughness * roughness, MinAlpha);
				float theta			= t *  1.0f / float(lookupTableSize - 1) * pi / 2;

				float3 v{ std::sin(theta), 0, cos(theta) };
				
				float amplitude = ComputeBRDFNorm(v, alpha);

				ltc_Table[idx][0][1] = 0;
				ltc_Table[idx][1][0] = 0;
				ltc_Table[idx][2][1] = 0;
				ltc_Table[idx][1][2] = 0;

				ltc_Table[idx] = ltc_Table[idx] * (1 / ltc_Table[idx][2][2]);
			}
		}
		*/
		//framework->Core->RenderSystem.
	}


	/************************************************************************************************/


	~GraphicsTest()
	{
		auto rendersystem = framework->GetRenderSystem();
		rendersystem->ReleaseVB(vertexBuffer);
		rendersystem->ReleaseVB(textBuffer);
		rendersystem->ReleaseCB(constantBuffer);
		rendersystem->ReleaseDB(depthBuffer);

		FlexKit::ReleaseCameraTable();
	}


	/************************************************************************************************/


	bool Update(
		EngineCore*			Engine, 
		UpdateDispatcher&	dispatcher, 
		double				dT) override
	{
		FK_LOG_1("Begin Update");

		auto transformTask = QueueTransformUpdateTask	(dispatcher);
		auto cameraUpdate  = QueueCameraUpdate			(dispatcher, transformTask);
		auto orbitUpdate   = QueueOrbitCameraUpdateTask	(dispatcher, transformTask, cameraUpdate, orbitCamera, framework->MouseState, dT);
		auto sceneUpdate   = scene.Update				(dispatcher, transformTask);
		auto terrainUpdate = terrain.Update				(dispatcher);

		return true;
	}


	/************************************************************************************************/


	bool DebugDraw(
		EngineCore*			core, 
		UpdateDispatcher&	dispatcher, 
		double dT) override
	{
		return true;
	}


	/************************************************************************************************/


	bool PreDrawUpdate(
		EngineCore*			engine, 
		UpdateDispatcher&	dispatcher, 
		double				dT) override
	{ 
		return true; 
	}


	/************************************************************************************************/


	bool Draw(
		EngineCore*			core, 
		UpdateDispatcher&	dispatcher, 
		double				dT, 
		FrameGraph&			frameGraph) override
	{
		FK_LOG_1("End Update");

		FK_LOG_1("Begin Draw");

		frameGraph.Resources.AddDepthBuffer(depthBuffer);


		FlexKit::WorldRender_Targets targets = {
			GetCurrentBackBuffer(&core->Window),
			depthBuffer
		};

		PVS	solidDrawables			(core->GetTempMemory());
		PVS	transparentDrawables	(core->GetTempMemory());

		ClearVertexBuffer	(frameGraph, vertexBuffer);
		ClearVertexBuffer	(frameGraph, textBuffer);

		ClearBackBuffer		(frameGraph, 0.0f);
		ClearDepthBuffer	(frameGraph, depthBuffer, 1.0f);


		DrawUI_Desc DrawDesk
		{
			&frameGraph, 
			targets.RenderTarget,
			vertexBuffer, 
			textBuffer, 
			constantBuffer
		};

		CameraHandle activeCamera = (CameraHandle)orbitCamera;

		GetGraphicScenePVS(scene, activeCamera, &solidDrawables, &transparentDrawables);

		const bool renderTerrainEnabled = true;
		TerrainCullerData* culledTerrain = nullptr;

		if (renderTerrainEnabled) {
			culledTerrain = CullTerrain(
				terrain,
				core->RenderSystem,
				frameGraph,
				activeCamera,
				vertexBuffer,
				constantBuffer,
				core->GetTempMemory());
		}

		if (renderTerrainEnabled)
		{
			DrawTerrain_Forward(
				culledTerrain,
				terrain,
				core->RenderSystem,
				frameGraph,
				activeCamera,
				vertexBuffer,
				constantBuffer,
				core->Window.GetBackBuffer(),
				depthBuffer,
				core->GetTempMemory());
		}

		/*
		if (true)
			DEBUG_DrawQuadTree(
				scene,
				frameGraph,
				activeCamera,
				vertexBuffer,
				constantBuffer,
				core->Window.GetBackBuffer(),
				core->GetTempMemory());
		*/

		return true; 
	}


	/************************************************************************************************/


	bool PostDrawUpdate(
		EngineCore*			core, 
		UpdateDispatcher&	dispatcher, 
		double				dT, 
		FrameGraph&			frameGraph)
	{ 

		if (framework->drawDebug)
			framework->DrawDebugHUD(dT, textBuffer, frameGraph);

		PresentBackBuffer(frameGraph, &core->Window);
		FK_LOG_1("End Draw");

		return true; 
	}


	/************************************************************************************************/


	bool EventHandler	(Event evt)
	{ 
		eventMap.Handle(
			evt, 
			[&](auto evt)
			{
				orbitCamera.HandleEvent(evt);

				if (evt.mData1.mINT[0] == ReloadShaders_1)
				{
					framework->GetRenderSystem()->QueuePSOLoad(TERRAIN_COMPUTE_CULL_PSO);
					framework->GetRenderSystem()->QueuePSOLoad(TERRAIN_RENDER_FOWARD_PSO);
					framework->GetRenderSystem()->QueuePSOLoad(TERRAIN_RENDER_FOWARD_WIREFRAME_PSO);
				}
				if (	evt.InputSource		== Event::InputType::Keyboard &&
						evt.Action			== Event::InputAction::Release &&
						evt.mData1.mINT[0]	== ToggleWireframe)
					terrain.DebugMode = terrain.DebugMode == 
								TerrainEngine::TERRAINDEBUGRENDERMODE::DISABLED ?
								TerrainEngine::TERRAINDEBUGRENDERMODE::WIREFRAME :
								TerrainEngine::TERRAINDEBUGRENDERMODE::DISABLED;
			});

		return true; 
	}


	/************************************************************************************************/


private:
	FlexKit::InputMap					eventMap;

	FlexKit::TextureHandle				ltcLookup_1;
	FlexKit::TextureHandle				ltcLookup_2;

	FlexKit::OrbitCameraBehavior		orbitCamera;
	FlexKit::GraphicScene				scene;
	FlexKit::WorldRender				render;
	FlexKit::TextureHandle				depthBuffer;
	FlexKit::VertexBufferHandle			vertexBuffer;
	FlexKit::VertexBufferHandle			textBuffer;
	FlexKit::ConstantBufferHandle		constantBuffer;
	FlexKit::TriMeshHandle				obj;

	TerrainEngine						terrain;

	PhysicsSceneHandle					PScene;
	StaticColliderHandle				floor;
	RigidBodyDrawableBehavior			box[CubeCount];
};
/************************************************************************************************/
#endif