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

#include "..\coreutilities\GameFramework.h"

const size_t lookupTableSize	= 128;

enum TestEvents
{
	ReloadShaders_1,
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


void DEBUG_DrawQuadTree(
	GraphicScene&			scene, 
	FlexKit::FrameGraph&	graph, 
	CameraHandle			camera,
	VertexBufferHandle		vertexBuffer, 
	ConstantBufferHandle	constantBuffer,
	TextureHandle			renderTarget,
	iAllocator*				tempMemory)
{
	Vector<FlexKit::Rectangle> rects{ tempMemory };

	const float4 colors[] =
	{
		{0, 0, 1, 1},
		{0, 1, 1, 1},
		{1, 1, 0, 1},
		{1, 0, 0, 1}
	};

	const auto area		= scene.SceneManagement.GetArea();
	const auto areaLL	= (area.y <  area.z) ? float2{ area.x, area.y } : float2{ area.z, area.w };
	const auto areaUR	= (area.y >= area.z) ? float2{ area.x, area.y } : float2{ area.z, area.w };
	const auto areaSpan = (areaUR - areaLL);
	int itr = 0;

	auto drawNode = [&](auto& self, const QuadTreeNode& Node, const int depth) -> void
	{
		auto nodeArea = Node.GetArea();

		//const auto ll = (float2{ nodeArea.x, nodeArea.y } - areaLL) / areaSpan;
		//const auto ur = (float2{ nodeArea.z, nodeArea.w } - areaLL) / areaSpan;

		const auto ll = float2{nodeArea.x, nodeArea.y};// lower left
		const auto ur = float2{nodeArea.z, nodeArea.w};// upper right

		FlexKit::Rectangle rect;
		rect.Color		= colors[depth % 4];
		rect.Position	= ll;
		rect.WH			= ur - ll;
		rects.push_back(rect);

		for (auto& child : Node.ChildNodes)
			self(self, *child, depth + 1 + itr++);
	};

	drawNode(drawNode, scene.SceneManagement.root, 0);

	DrawWireframeRectangle_Desc drawDesc =
	{
		renderTarget,
		vertexBuffer,
		constantBuffer,
		camera,
		DRAW_LINE3D_PSO
	};

	WireframeRectangleList(
		graph,
		drawDesc,
		rects,
		tempMemory);
}


/************************************************************************************************/


struct TileMaps
{
	FlexKit::TextureHandle heightMap;
	const char*	File;
};


struct Tile
{
	int2	tileID;
	size_t	textureMaps;
};


using TerrainTileHandle = FlexKit::Handle_t<16, GetTypeGUID(TerrainTileHandle)>;

class TerrainEngine
{
public:
	TerrainEngine(iAllocator* IN_allocator, size_t IN_tileEdgeSize = 128, size_t IN_tileHeight = 128, uint2 IN_tileOffset = {0, 0}) :
		tiles			{IN_allocator},
		tileTextures	{IN_allocator},
		tileHeight		{IN_tileHeight}, 
		tileOffset		{IN_tileOffset},
		allocator		{IN_allocator}
	{}


	void SetTileSize(size_t tileEdgeSize)
	{

	}


	TerrainTileHandle AddTile(int2 tileID, const char* HeightMap, RenderSystem* RS, iAllocator* tempMemory)
	{
		static_vector<TextureBuffer> Textures;
		Textures.push_back(TextureBuffer{});
		LoadBMP(HeightMap, tempMemory, &Textures.back());

		//Build Mip Maps
		auto BuildMipMap = [](TextureBuffer& sourceMap, iAllocator* memory) -> TextureBuffer
		{
			using RBGA = Vect<4, uint8_t>;

			TextureBuffer		MIPMap	= TextureBuffer( sourceMap.WH / 2, sizeof(RGBA), memory );
			TextureBufferView	View	= TextureBufferView<RBGA>(&sourceMap);
			TextureBufferView	MipView = TextureBufferView<RBGA>(&MIPMap);

			const auto WH = MIPMap.WH;
			for (size_t Y = 0; Y < WH[0]; Y++)
			{
				for (size_t X = 0; X < WH[1]; X++)
				{
					uint2 Cord = uint2{ min(X, WH[0] - 1), min(Y, WH[1] - 1) };

					auto Sample = 
						View[Cord + uint2{0, 0}] +
						View[Cord + uint2{0, 1}] +
						View[Cord + uint2{1, 0}] +
						View[Cord + uint2{1, 1}];

					Sample = Sample / 4;

					MipView[Cord] = Sample;
				}
			}

			return MIPMap;
		};


		Textures.push_back(BuildMipMap(Textures.back(), tempMemory));
		Textures.push_back(BuildMipMap(Textures.back(), tempMemory));
		Textures.push_back(BuildMipMap(Textures.back(), tempMemory));
		Textures.push_back(BuildMipMap(Textures.back(), tempMemory));

		auto textureHandle	= MoveTextureBufferToVRAM(&Textures.front(), RS, allocator);
		auto tileMaps		= tileTextures.push_back(TileMaps{textureHandle, HeightMap});
		tiles.push_back(Tile{ tileID, tileMaps });

		return InvalidHandle_t;
	}

	void SetTileOffset(int2 IN_tileOffset)
	{
		tileOffset = IN_tileOffset;
	}

	// No update dependencies
	FlexKit::UpdateTask* Update(FlexKit::UpdateDispatcher& Dispatcher)
	{
		// Make sure textures are loaded
		return nullptr;
	}


	Vector<Tile>		tiles;
	Vector<TileMaps>	tileTextures;

	int2		tileOffset;
	size_t		tileEdgeSize;
	size_t		tileHeight;
	iAllocator* allocator;
};


/************************************************************************************************/


void DrawTerrain_Forward(
	TerrainEngine&					terrainEngine,
	FlexKit::FrameGraph&			frameGraph,
	FlexKit::CameraHandle			camera,
	FlexKit::VertexBufferHandle		vertexBuffer,
	FlexKit::ConstantBufferHandle	constantBuffer,
	FlexKit::TextureHandle			renderTarget,
	FlexKit::iAllocator*			tempMemory)
{
	if (terrainEngine.tileTextures.size())
	{
		RectangleList rects{ tempMemory };
		TextureList	  textures{ tempMemory };

		rects.push_back(Rectangle::FullScreenQuad());
		textures.push_back(terrainEngine.tileTextures.back().heightMap);

		DrawShapes(
			DRAW_TEXTURED_DEBUG_PSO,
			frameGraph,
			vertexBuffer,
			constantBuffer,
			renderTarget,
			tempMemory,
			TexturedRectangleListShape{
				std::move(rects),
				std::move(textures) });
	}
}


/************************************************************************************************/


class GraphicsTest : public FrameworkState
{
public:

	GraphicsTest(GameFramework* IN_framework) :
		FrameworkState(IN_framework),
		render			{IN_framework->core->GetBlockMemory(), IN_framework->core->RenderSystem},
		depthBuffer		{IN_framework->core->RenderSystem.CreateDepthBuffer		({ 1920, 1080 },	true)},
		vertexBuffer	{IN_framework->core->RenderSystem.CreateVertexBuffer	(8096 * 64,			false)},
		textBuffer		{IN_framework->core->RenderSystem.CreateVertexBuffer	(8096 * 64,			false)},
		constantBuffer	{IN_framework->core->RenderSystem.CreateConstantBuffer	(8096 * 2000,		false)},
		eventMap		{IN_framework->core->GetBlockMemory()},
		terrain			{IN_framework->core->GetBlockMemory()},
		scene			{	
			IN_framework->core->RenderSystem, 
			IN_framework->core->GetBlockMemory(),
			IN_framework->core->GetTempMemory()}
		//ltcLookup_1{ IN_framework->Core->RenderSystem.CreateTexture2D(FlexKit::uint2{lookupTableSize, lookupTableSize}, FlexKit::FORMAT_2D::R32G32B32A32_FLOAT)},
		//ltcLookup_2{ IN_framework->Core->RenderSystem.CreateTexture2D(FlexKit::uint2{lookupT's ableSize, lookupTableSize}, FlexKit::FORMAT_2D::R32G32B32A32_FLOAT)}
	{
		auto& RS = IN_framework->core->RenderSystem;
		RS.RegisterPSOLoader(DRAW_PSO,					{&RS.Library.RS4CBVs4SRVs, CreateDrawTriStatePSO});
		RS.RegisterPSOLoader(DRAW_TEXTURED_PSO,			{&RS.Library.RS4CBVs4SRVs, CreateTexturedTriStatePSO});
		RS.RegisterPSOLoader(DRAW_TEXTURED_DEBUG_PSO,	{&RS.Library.RS4CBVs4SRVs, CreateTexturedTriStateDEBUGPSO});
		RS.RegisterPSOLoader(DRAW_LINE_PSO,				{&RS.Library.RS4CBVs4SRVs, CreateDrawLineStatePSO});
		RS.RegisterPSOLoader(DRAW_LINE3D_PSO,			{&RS.Library.RS4CBVs4SRVs, CreateDraw2StatePSO});
		RS.RegisterPSOLoader(DRAW_SPRITE_TEXT_PSO,		{&RS.Library.RS4CBVs4SRVs, LoadSpriteTextPSO });

		RS.QueuePSOLoad(DRAW_PSO);
		RS.QueuePSOLoad(DRAW_LINE3D_PSO);
		RS.QueuePSOLoad(DRAW_TEXTURED_DEBUG_PSO);

		AddResourceFile("testScene.gameres");

		if (!FlexKit::LoadScene(IN_framework->core, &scene, 6001))
		{
			std::cerr << "Failed to Load Scene!\n";
			IN_framework->quit = true;
		}

		std::cout << "Objects in Scene: "	<< scene.Drawables.size() << "\n";
		std::cout << "Lights in Scene: "	<< scene.PLights.size() << "\n";

		scene.ListEntities();

		orbitCamera.TranslateWorld({0, 2.5, 0});

		eventMap.MapKeyToEvent(KEYCODES::KC_R, ReloadShaders_1);
		eventMap.MapKeyToEvent(KEYCODES::KC_W, OCE_MoveForward);
		eventMap.MapKeyToEvent(KEYCODES::KC_S, OCE_MoveBackward);
		eventMap.MapKeyToEvent(KEYCODES::KC_A, OCE_MoveLeft);
		eventMap.MapKeyToEvent(KEYCODES::KC_D, OCE_MoveRight);

		const int TileSize = 1024;
		terrain.SetTileOffset	(int2{-TileSize/2, -TileSize/2});
		terrain.SetTileSize		(TileSize); // will assume square tiles
		terrain.AddTile			({ 0, 0 }, "assets/textures/tiles/tile_0_0.bmp", framework->GetRenderSystem(), framework->core->GetTempMemory());

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
		FlexKit::EngineCore*		Engine, 
		FlexKit::UpdateDispatcher&	Dispatcher, 
		double						dT) override
	{
		auto transformTask = QueueTransformUpdateTask	(Dispatcher);
		auto cameraUpdate  = QueueCameraUpdate			(Dispatcher, transformTask);
		auto orbitUpdate   = QueueOrbitCameraUpdateTask	(Dispatcher, transformTask, cameraUpdate, orbitCamera, framework->MouseState, dT);
		auto sceneUpdate   = scene.Update				(Dispatcher, transformTask);
		auto terrainUpdate = terrain.Update				(Dispatcher);

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

		if (true)
			DrawTerrain_Forward(
				terrain,
				frameGraph,
				activeCamera,
				vertexBuffer,
				constantBuffer,
				core->Window.GetBackBuffer(),
				core->GetTempMemory());

		if(true)
			render.DefaultRender(
				solidDrawables,
				activeCamera,
				targets,
				frameGraph,
				core->GetTempMemory());

		if (true)
			DEBUG_DrawQuadTree(
				scene,
				frameGraph,
				activeCamera,
				vertexBuffer,
				constantBuffer,
				core->Window.GetBackBuffer(),
				core->GetTempMemory());



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
			});

		return true; 
	}


	/************************************************************************************************/


private:
	TerrainEngine						terrain;

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
};


/************************************************************************************************/
#endif