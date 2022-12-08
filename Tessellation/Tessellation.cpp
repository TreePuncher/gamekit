#include "pch.h"
#include "Gregory.h"

constexpr FlexKit::PSOHandle ACCQuad                = FlexKit::PSOHandle(GetTypeGUID(ACCQuad));
constexpr FlexKit::PSOHandle ACCQuadWireframe       = FlexKit::PSOHandle(GetTypeGUID(ACCQuadWireframe));
constexpr FlexKit::PSOHandle ACCDebugNormals        = FlexKit::PSOHandle(GetTypeGUID(ACCDebugNormals));
constexpr FlexKit::PSOHandle ACCDebugControlpoints  = FlexKit::PSOHandle(GetTypeGUID(ACCDebugControlpoints));

using namespace FlexKit;


/************************************************************************************************/


class TessellationTest : public FlexKit::FrameworkState
{
public:
	TessellationTest(FlexKit::GameFramework& IN_framework, int test) :
		FrameworkState{ IN_framework },
		renderWindow    {
			std::get<0>(CreateWin32RenderWindow(
				IN_framework.GetRenderSystem(),
				FlexKit::DefaultWindowDesc(uint2{ 1920, 1080 } * 2))) },
		rootSig         { IN_framework.core.GetBlockMemory() },
		vertexBuffer    { IN_framework.GetRenderSystem().CreateVertexBuffer(MEGABYTE, false) },
		constantBuffer  { IN_framework.GetRenderSystem().CreateConstantBuffer(MEGABYTE, false) },
		cameras         { IN_framework.core.GetBlockMemory() },
		depthBuffer     { IN_framework.GetRenderSystem().CreateDepthBuffer(uint2{ 1920, 1080 } * 2, true) },
		debug1Buffer    { IN_framework.GetRenderSystem().CreateUAVBufferResource(MEGABYTE * 16, false) },
		debug2Buffer    { IN_framework.GetRenderSystem().CreateUAVBufferResource(MEGABYTE, false) },
		indices         { IN_framework.core.GetBlockMemory() },
		orbitCamera     { gameObject, CameraComponent::GetComponent().CreateCamera(), 10.0f },
		nodes           {}
	{
		patches     = PreComputePatches(IN_framework.core.GetBlockMemory() );
		vertices    = GetGregoryVertexBuffer(shape, framework.core.GetBlockMemory());
		patchBuffer = MoveBufferToDevice(framework.GetRenderSystem(), (const char*)patches.data(), patches.ByteSize());

		rootSig.SetParameterAsSRV(0, 0, 0);
		rootSig.SetParameterAsCBV(1, 0, 0, PIPELINE_DEST_ALL);
		rootSig.SetParameterAsUINT(2, 16, 1, 0, PIPELINE_DEST_ALL);
		rootSig.SetParameterAsUINT(3, 1, 2, 0, PIPELINE_DEST_ALL);
		rootSig.SetParameterAsUAV(4, 0, 0, PIPELINE_DEST_ALL);
		rootSig.SetParameterAsUAV(5, 1, 0, PIPELINE_DEST_ALL);
		rootSig.SetParameterAsUAV(6, 2, 0, PIPELINE_DEST_ALL);

		rootSig.AllowIA = true;
		rootSig.AllowSO = false;
		FK_ASSERT(rootSig.Build(IN_framework.GetRenderSystem(), IN_framework.core.GetTempMemory()));

		framework.GetRenderSystem().RegisterPSOLoader(ACCQuad,                  { &rootSig, [&](auto* renderSystem) { return LoadPSO(renderSystem); } });
		framework.GetRenderSystem().RegisterPSOLoader(ACCQuadWireframe,         { &rootSig, [&](auto* renderSystem) { return LoadPSO(renderSystem, true); } });
		framework.GetRenderSystem().RegisterPSOLoader(ACCDebugControlpoints,    { &rootSig, [&](auto* renderSystem) { return LoadPSO2(renderSystem); } });
		framework.GetRenderSystem().RegisterPSOLoader(ACCDebugNormals,          { &rootSig, [&](auto* renderSystem) { return LoadPSO3(renderSystem); } });

		FlexKit::EventNotifier<>::Subscriber sub;
		sub.Notify  = &FlexKit::EventsWrapper;
		sub._ptr    = &framework;

		renderWindow.Handler->Subscribe(sub);
		renderWindow.SetWindowTitle("[Tessellation Test]");
		renderWindow.EnableCaptureMouse(true);
		//activeCamera    = CameraComponent::GetComponent().CreateCamera((float)pi/3.0f, 1920.0f / 1080.0f);
		node            = GetZeroedNode();
		orbitCamera.SetCameraAspectRatio(1920.0f / 1080.0f);
		orbitCamera.SetCameraFOV((float)pi / 3.0f);
		//TranslateWorld(node, float3( 0, -3, -20.0f ));
		//Pitch(node, pi / 6);

		//Yaw(node, pi);
		//Pitch(node, pi / 2);
	}


	/************************************************************************************************/


	static_vector<uint2> FindSharedEdges(QuadPatch& quadPatch, uint32_t vertexIdx)
	{
		static_vector<uint2> out;

		for(size_t I = 0; I < 4; I++)
		if (quadPatch.vertexIds[I] == vertexIdx)
		{
			out.emplace_back(quadPatch.vertexIds[I], quadPatch.vertexIds[(I + 1) % 4]);
			out.emplace_back(quadPatch.vertexIds[(I + 3) % 4], quadPatch.vertexIds[I]);
		}

		return out;
	}


	/************************************************************************************************/

	// V20 ------ V21 ----- V22 ------- V23 ------ V24
	// |          |          |           |          |
	// |          |          |           |          |
	// |   P12    |    p13   |    p14    |    p15   |
	// |          |          |           |          |
	// |          |          |           |          |
	// V7 ------- V6 ------- V11 ------ V15 ------ V19
	// |          |          |           |          |
	// |          |          |           |          |
	// |   P2     |    p5    |    p8     |    p11   |
	// |          |          |           |          |
	// |          |          |           |          |
	// V5 ------- V4 ------- V10 ------ V14 ------ V18
	// |          |          |           |          |
	// |          |          |           |          |
	// |   p1     |    p4    |    p7     |    p10   |
	// |          |          |           |          |
	// |          |          |           |          |
	// V3 ------- V2 ------- V9 ------- V13 ------ V17
	// |          |          |           |          |
	// |          |          |           |          |
	// |   p0     |    p3    |    p6     |    p9    |
	// |          |          |           |          |
	// |          |          |           |          |
	// V0 ------- V1 ------- V8 ------- V12 ------ V16


	/************************************************************************************************/


	Vector<GPUPatch> PreComputePatches(iAllocator& allocator)
	{
		// Patch 1
		shape.AddVertex(float3{ 0.0f, 0.0f, 0.0f }); // 0
		shape.AddVertex(float3{ 1.0f, 0.0f, 0.0f }); // 1
		shape.AddVertex(float3{ 1.0f, 1.0f, 1.0f }); // 2
		shape.AddVertex(float3{ 0.0f, 0.0f, 1.0f }); // 3

		shape.AddVertex(float3{ 1.0f, 1.0f, 2.0f }); // 4
		shape.AddVertex(float3{ 0.0f, 0.0f, 2.0f }); // 5
		shape.AddVertex(float3{ 1.0f, 0.0f, 3.0f }); // 6
		shape.AddVertex(float3{ 0.0f, 0.0f, 3.0f }); // 7

		shape.AddVertex(float3{ 2.0f, 0.0f, 0.0f }); // 8
		shape.AddVertex(float3{ 2.0f, 1.0f, 1.0f }); // 9
		shape.AddVertex(float3{ 2.0f, 1.0f, 2.0f }); // 10
		shape.AddVertex(float3{ 2.0f, 0.0f, 3.0f }); // 11

		shape.AddVertex(float3{ 3.0f, 0.0f, 0.0f }); // 12
		shape.AddVertex(float3{ 3.0f, 0.0f, 1.0f }); // 13
		shape.AddVertex(float3{ 3.0f, 0.0f, 2.0f }); // 14
		shape.AddVertex(float3{ 3.0f, 0.0f, 3.0f }); // 15

		shape.AddVertex(float3{ 4.0f, 0.0f, 0.0f }); // 16
		shape.AddVertex(float3{ 4.0f, 0.0f, 1.0f }); // 17
		shape.AddVertex(float3{ 4.0f, 0.0f, 2.0f }); // 18
		shape.AddVertex(float3{ 4.0f, 0.0f, 3.0f }); // 19

		shape.AddVertex(float3{ 0.0f, 0.0f, 4.0f }); // 20
		shape.AddVertex(float3{ 1.0f, 0.0f, 4.0f }); // 21
		shape.AddVertex(float3{ 2.0f, 0.0f, 4.0f }); // 22
		shape.AddVertex(float3{ 3.0f, 0.0f, 4.0f }); // 23
		shape.AddVertex(float3{ 4.0f, 0.0f, 4.0f }); // 24

		const uint32_t patch0[] = { 0, 1, 2, 3 };
		const uint32_t patch1[] = { 3, 2, 4, 5 };
		const uint32_t patch2[] = { 5, 4, 6, 7 };
		const uint32_t patch3[] = { 1, 8, 9, 2 };
		const uint32_t patch4[] = { 2, 9, 10, 4 };
		const uint32_t patch5[] = { 4, 10, 11, 6 };
		const uint32_t patch6[] = { 8, 12, 13, 9 };
		const uint32_t patch7[] = { 9, 13, 14, 10 };
		const uint32_t patch8[] = { 10, 14, 15, 11 };

		const uint32_t patch9[] = { 12, 16, 17, 13 };
		const uint32_t patch10[] = { 13, 17, 18, 14 };
		const uint32_t patch11[] = { 14, 18, 19, 15 };

		const uint32_t patch12[] = { 7, 6, 21, 20 };
		const uint32_t patch13[] = { 6, 11, 22, 21 };
		const uint32_t patch14[] = { 11, 15, 23, 22 };
		const uint32_t patch15[] = { 15, 19, 24, 23 };

		//shape.AddPolygon(patch0, patch0 + 4);
		shape.AddPolygon(patch1, patch1 + 4);
		shape.AddPolygon(patch2, patch2 + 4);
		shape.AddPolygon(patch3, patch3 + 4);
		shape.AddPolygon(patch4, patch4 + 4);
		shape.AddPolygon(patch5, patch5 + 4);
		shape.AddPolygon(patch6, patch6 + 4);
		shape.AddPolygon(patch7, patch7 + 4);
		shape.AddPolygon(patch8, patch8 + 4);
		shape.AddPolygon(patch9, patch9 + 4);
		shape.AddPolygon(patch10, patch10 + 4);
		shape.AddPolygon(patch11, patch11 + 4);

		shape.AddPolygon(patch12, patch12 + 4);
		shape.AddPolygon(patch13, patch13 + 4);
		shape.AddPolygon(patch14, patch14 + 4);
		shape.AddPolygon(patch15, patch15 + 4);

		auto classifiedPatches  = ClassifyPatches(shape);
		auto patches            = CreateGregoryPatches(classifiedPatches, shape);

		Vector<GPUPatch> GPUControlPoints{ allocator };

		for (auto& patch : patches)
			CreateGPUPatch(patch, shape, GPUControlPoints, indices);

		return GPUControlPoints;
	}


	/************************************************************************************************/


	Vector<GPUVertex> GetGregoryVertexBuffer(ModifiableShape& shape, iAllocator& allocator)
	{
		Vector<GPUVertex> vertices{ allocator };

		for (const auto& vertex : shape.wVertices)
		{
			GPUVertex p;
			memcpy(&p, &vertex.point, sizeof(p));
			vertices.push_back(p);
		}

		return vertices;
	}


	/************************************************************************************************/


	void DrawPatch(UpdateDispatcher& dispatcher, FlexKit::FrameGraph& frameGraph)
	{
		struct DrawPatch
		{
			FrameResourceHandle renderTarget;
			FrameResourceHandle depthTarget;
			FrameResourceHandle debug1Buffer;
			FrameResourceHandle debug2Buffer;
		};

		auto& cameraUpdate = CameraComponent::GetComponent().QueueCameraUpdate(dispatcher);

		frameGraph.AddNode<DrawPatch>(
			DrawPatch{},
			[&](FrameGraphNodeBuilder& builder, DrawPatch& draw)
			{
				builder.AddDataDependency(cameraUpdate);
				draw.renderTarget   = builder.RenderTarget(renderWindow.GetBackBuffer());
				draw.depthTarget    = builder.DepthTarget(depthBuffer);
				draw.debug1Buffer    = builder.UnorderedAccess(debug1Buffer);
				draw.debug2Buffer    = builder.UnorderedAccess(debug2Buffer);
			},
			[&](DrawPatch& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
				VBPushBuffer Vbuffer{ vertexBuffer, 8196, *ctx.renderSystem };
				CBPushBuffer Cbuffer{ constantBuffer, 1024 * 16, *ctx.renderSystem };

				const VertexBufferDataSet VB{ vertices, Vbuffer};
				const VertexBufferDataSet IB{ indices, Vbuffer};

				const float4x4 WT       = GetWT(node).Transpose();
				const auto constants    = GetCameraConstants(orbitCamera.camera);
				
				const ConstantBufferDataSet CB{ constants, Cbuffer };

				ctx.ClearDepthBuffer(depthBuffer, 1.0f);
				ctx.ClearUAVBufferRange(resources.UAV(data.debug1Buffer, ctx), 0, 64);
				ctx.ClearUAVBufferRange(resources.UAV(data.debug2Buffer, ctx), 0, 64);
				ctx.AddUAVBarrier(resources.UAV(data.debug1Buffer, ctx));
				ctx.AddUAVBarrier(resources.UAV(data.debug2Buffer, ctx));

				ctx.SetRootSignature(rootSig);
				ctx.SetPipelineState(resources.GetPipelineState(ACCQuad));
				ctx.SetPrimitiveTopology(EInputTopology::EIT_PATCH_CP_32);
				ctx.SetVertexBuffers({ VB });
				ctx.SetIndexBuffer(IB);
				ctx.SetScissorAndViewports({ resources.GetRenderTarget(data.renderTarget) });
				ctx.SetRenderTargets({ resources.GetRenderTarget(data.renderTarget) }, true, depthBuffer);

				const float maxExpansionRate = tesselationLevel;// 1 + cos(t) * 63.0f / 2 + 63.0f / 2.0f;

				ctx.SetGraphicsShaderResourceView(0, patchBuffer);
				ctx.SetGraphicsConstantBufferView(1, CB);
				ctx.SetGraphicsConstantValue(2, 16, &WT);
				ctx.SetGraphicsConstantValue(3, 1, &maxExpansionRate);

				ctx.SetGraphicsUnorderedAccessView(4, resources.UAV(data.debug1Buffer, ctx), 0);
				ctx.SetGraphicsUnorderedAccessView(5, resources.UAV(data.debug1Buffer, ctx), 64);
				ctx.SetGraphicsUnorderedAccessView(6, resources.UAV(data.debug2Buffer, ctx), 0);

				ctx.DrawIndexed(indices.size());

				if (wireframe)
				{
					ctx.SetPipelineState(resources.GetPipelineState(ACCQuadWireframe));
					ctx.DrawIndexed(indices.size());
				}


				if (markers)
				{
					const auto devicePointer1 = resources.GetDevicePointer(resources.VertexBuffer(data.debug2Buffer, ctx));
					ctx.SetPipelineState(resources.GetPipelineState(ACCDebugControlpoints));
					ctx.SetPrimitiveTopology(EInputTopology::EIT_POINT);
					ctx.SetVertexBuffers2({ D3D12_VERTEX_BUFFER_VIEW{ devicePointer1, MEGABYTE, 24 } });

					ctx.Draw(20 * indices.size() / 32);
				}
				if (normals)
				{
					const auto devicePointer2 = resources.GetDevicePointer(resources.VertexBuffer(data.debug1Buffer, ctx));
					ctx.SetPipelineState(resources.GetPipelineState(ACCDebugNormals));
					ctx.SetPrimitiveTopology(EInputTopology::EIT_LINE);
					ctx.SetVertexBuffers2({ D3D12_VERTEX_BUFFER_VIEW{ devicePointer2 + 64, MEGABYTE - 64, 12 } });
					ctx.Draw(maxExpansionRate * maxExpansionRate * 9 * 16);
				}
			});
	}


	/************************************************************************************************/


	FlexKit::UpdateTask* Draw(FlexKit::UpdateTask* updateTask, FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT, FlexKit::FrameGraph& frameGraph)
	{
		ClearBackBuffer(frameGraph, renderWindow.GetBackBuffer(), { 0.1f, 0.1f, 0.1f, 1.0f });

		renderWindow.UpdateCapturedMouseInput(dT);

		auto& orbitCameraUpdate = QueueOrbitCameraUpdateTask(dispatcher, orbitCamera, renderWindow.mouseState, dT);
		auto& transformUpdate   = QueueTransformUpdateTask(dispatcher);
		auto& cameraUpdate      = CameraComponent::GetComponent().QueueCameraUpdate(dispatcher);

		frameGraph.dataDependencies.push_back(&orbitCameraUpdate);

		DrawPatch(dispatcher, frameGraph);

		PresentBackBuffer(frameGraph, renderWindow.GetBackBuffer());

		t += dT;

		return nullptr;
	}


	/************************************************************************************************/


	FlexKit::UpdateTask* Update(FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT)
	{ 
		FlexKit::UpdateInput();
		renderWindow.UpdateCapturedMouseInput(dT);

		return nullptr;
	}


	/************************************************************************************************/


	void PostDrawUpdate(FlexKit::EngineCore& core, double dT) override
	{
		renderWindow.Present(core.vSync ? 1 : 0, 0);
	}


	/************************************************************************************************/
	

	bool EventHandler(FlexKit::Event evt) override
	{
		switch (evt.InputSource)
		{
		case FlexKit::Event::E_SystemEvent:
			if (evt.Action == FlexKit::Event::InputAction::Exit)
			{
				framework.quit = true;
				return true;
			}
			break;
		case FlexKit::Event::InputType::Keyboard:
		{
			switch (evt.mData1.mKC[0])
			{
			case KC_W:
			case KC_A:
			case KC_S:
			case KC_D:
			case KC_Q:
			case KC_E:
				orbitCamera.HandleEvent(evt);
				return true;
				break;
			case KC_M:
				if(evt.Action == Event::Release)
					renderWindow.ToggleMouseCapture();
				break;
			case KC_T:
				if (evt.Action == Event::Release)
					wireframe = !wireframe;
				break;
			case KC_Y:
				if (evt.Action == Event::Release)
					normals = !normals;
				break;
			case KC_2:
				if (evt.Action == Event::Release)
					tesselationLevel = clamp(1, tesselationLevel++, 64);
				break;
			case KC_1:
				if (evt.Action == Event::Release)
					tesselationLevel = clamp(1, tesselationLevel--, 64);
				break;
			case KC_3:
				if (evt.Action == Event::Release)
					markers = !markers;
				break;
			}

			if (evt.mType == Event::EventType::Input && evt.Action == Event::Release && evt.mData1.mKC[0] == KC_R)
			{
				framework.GetRenderSystem().QueuePSOLoad(ACCQuad);
				framework.GetRenderSystem().QueuePSOLoad(ACCQuadWireframe);
				framework.GetRenderSystem().QueuePSOLoad(ACCDebugControlpoints);
				framework.GetRenderSystem().QueuePSOLoad(ACCDebugNormals);
			}
		}   break;
		};

		return false;
	}


	/************************************************************************************************/


	ID3D12PipelineState* LoadPSO(FlexKit::RenderSystem* renderSystem, bool wireframe = false)
	{
		auto ACC_VShader = renderSystem->LoadShader("VS_Main", "vs_6_2", "assets\\shaders\\ACCQuad.hlsl", FlexKit::ShaderOptions{ .enable16BitTypes= true });
		auto ACC_DShader = renderSystem->LoadShader("DS_Main", "ds_6_2", "assets\\shaders\\ACCQuad.hlsl", FlexKit::ShaderOptions{ .enable16BitTypes= true });
		auto ACC_HShader = renderSystem->LoadShader("HS_Main", "hs_6_2", "assets\\shaders\\ACCQuad.hlsl", FlexKit::ShaderOptions{ .enable16BitTypes= true });
		auto ACC_PShader = renderSystem->LoadShader(
			wireframe ? "PS_Main2" : "PS_Main", "ps_6_2", "assets\\shaders\\ACCQuad.hlsl", FlexKit::ShaderOptions{ .enable16BitTypes = true });


		/*
		typedef struct D3D12_INPUT_ELEMENT_DESC
		{
		LPCSTR SemanticName;
		UINT SemanticIndex;
		DXGI_FORMAT Format;
		UINT InputSlot;
		UINT AlignedByteOffset;
		D3D12_INPUT_CLASSIFICATION InputSlotClass;
		UINT InstanceDataStepRate;
		} 	D3D12_INPUT_ELEMENT_DESC;
		*/

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//			{ "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//			{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		Rast_Desc.CullMode              = D3D12_CULL_MODE_NONE;
		if (wireframe)
			Rast_Desc.FillMode              = D3D12_FILL_MODE_WIREFRAME;
		//Rast_Desc.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		if(wireframe)
			Depth_Desc.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS_EQUAL;
		else
			Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = rootSig;
			PSO_Desc.VS                    = ACC_VShader;
			PSO_Desc.DS                    = ACC_DShader;
			PSO_Desc.HS                    = ACC_HShader;
			PSO_Desc.PS                    = ACC_PShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
			PSO_Desc.NumRenderTargets      = 1;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // renderTarget
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.DepthStencilState     = Depth_Desc;
			PSO_Desc.InputLayout           = { InputElements, 1 };
			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = renderSystem->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* LoadPSO2(FlexKit::RenderSystem* renderSystem)
	{
		auto Debug_VShader = renderSystem->LoadShader("VS_Main", "vs_6_2", "assets\\shaders\\ACCDebug.hlsl", FlexKit::ShaderOptions{ .enable16BitTypes = true });
		auto Debug_GShader = renderSystem->LoadShader("GS_Main", "gs_6_0", "assets\\shaders\\ACCDebug.hlsl");
		auto Debug_PShader = renderSystem->LoadShader("PS_Main", "ps_6_2", "assets\\shaders\\ACCDebug.hlsl", FlexKit::ShaderOptions{ .enable16BitTypes = true });

		/*
		typedef struct D3D12_INPUT_ELEMENT_DESC
		{
		LPCSTR SemanticName;
		UINT SemanticIndex;
		DXGI_FORMAT Format;
		UINT InputSlot;
		UINT AlignedByteOffset;
		D3D12_INPUT_CLASSIFICATION InputSlotClass;
		UINT InstanceDataStepRate;
		} 	D3D12_INPUT_ELEMENT_DESC;
		*/

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR",	    0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		Rast_Desc.CullMode              = D3D12_CULL_MODE_NONE;
		//Rast_Desc.FillMode              = D3D12_FILL_MODE_WIREFRAME;
		//Rast_Desc.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature         = rootSig;
			PSO_Desc.VS                     = Debug_VShader;
			PSO_Desc.GS                     = Debug_GShader;
			PSO_Desc.PS                     = Debug_PShader;
			PSO_Desc.RasterizerState        = Rast_Desc;
			PSO_Desc.BlendState             = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask             = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			PSO_Desc.NumRenderTargets       = 1;
			PSO_Desc.RTVFormats[0]          = DXGI_FORMAT_R16G16B16A16_FLOAT; // renderTarget
			PSO_Desc.SampleDesc.Count       = 1;
			PSO_Desc.SampleDesc.Quality     = 0;
			PSO_Desc.DSVFormat              = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.DepthStencilState      = Depth_Desc;
			PSO_Desc.InputLayout            = { InputElements, 2 };
			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = renderSystem->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	ID3D12PipelineState* LoadPSO3(FlexKit::RenderSystem* renderSystem)
	{
		auto Debug_VShader = renderSystem->LoadShader("VS2_Main", "vs_6_2", "assets\\shaders\\ACCDebug.hlsl", FlexKit::ShaderOptions{ .enable16BitTypes = true });
		auto Debug_PShader = renderSystem->LoadShader("PS2_Main", "ps_6_2",  "assets\\shaders\\ACCDebug.hlsl", FlexKit::ShaderOptions{ .enable16BitTypes = true });

		/*
		typedef struct D3D12_INPUT_ELEMENT_DESC
		{
		LPCSTR SemanticName;
		UINT SemanticIndex;
		DXGI_FORMAT Format;
		UINT InputSlot;
		UINT AlignedByteOffset;
		D3D12_INPUT_CLASSIFICATION InputSlotClass;
		UINT InstanceDataStepRate;
		} 	D3D12_INPUT_ELEMENT_DESC;
		*/

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		Rast_Desc.CullMode              = D3D12_CULL_MODE_NONE;

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature         = rootSig;
			PSO_Desc.VS                     = Debug_VShader;
			PSO_Desc.PS                     = Debug_PShader;
			PSO_Desc.RasterizerState        = Rast_Desc;
			PSO_Desc.BlendState             = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask             = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			PSO_Desc.NumRenderTargets       = 1;
			PSO_Desc.RTVFormats[0]          = DXGI_FORMAT_R16G16B16A16_FLOAT; // renderTarget
			PSO_Desc.SampleDesc.Count       = 1;
			PSO_Desc.SampleDesc.Quality     = 0;
			PSO_Desc.DSVFormat              = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.DepthStencilState      = Depth_Desc;
			PSO_Desc.InputLayout            = { InputElements, 1 };
			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = renderSystem->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}

	/************************************************************************************************/


	ModifiableShape         shape;
	Vector<GPUVertex>       vertices;
	Vector<uint32_t>        indices;
	Vector<GPUPatch>        patches;
	NodeHandle              node;

	
	ResourceHandle          patchBuffer;
	VertexBufferHandle      vertexBuffer;
	ConstantBufferHandle    constantBuffer;
	RootSignature           rootSig;
	Win32RenderWindow       renderWindow;
	ResourceHandle          depthBuffer;
	ResourceHandle          debug1Buffer;
	ResourceHandle          debug2Buffer;

	CameraComponent         cameras;
	SceneNodeComponent      nodes;

	double  t                   = 0.0f;
	int     tesselationLevel    = 16;
	bool    wireframe           = false;
	bool    markers             = false;
	bool    normals             = false;
	bool    forward             = false;
	bool    backward            = false;

	MouseInputState         mouseState;
	GameObject              gameObject;
	OrbitCameraBehavior     orbitCamera;
};


/************************************************************************************************/


int main()
{
	try
	{
		fmt::print("Controls:\n\t1: Decrease tessellation.\n\t2: Increase tessellation.\n\t3: Toggle markers.\n\tM: toggle mouse.\n\tR: Reload Shaders\n\tT: Toggle wireframe.\n\tY: Toggle Normal hairs.\n");

		auto* allocator = FlexKit::CreateEngineMemory();
		EXITSCOPE(ReleaseEngineMemory(allocator));

		FlexKit::FKApplication app{ allocator, FlexKit::Max(std::thread::hardware_concurrency() / 2, 1u) - 1 };

		auto& state = app.PushState<TessellationTest>(0);

		app.GetCore().FPSLimit  = 90;
		app.GetCore().FrameLock = true;
		app.GetCore().vSync     = true;
		app.Run();
	}
	catch (...) { }

	return 0;
}


/**********************************************************************

Copyright (c) 2015 - 2022 Robert May

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
