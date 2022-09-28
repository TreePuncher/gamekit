#include "PCH.h"
#include "DXRenderWindow.h"
#include "EditorPrefabEditor.h"
#include "EditorPrefabObject.h"
#include "EditorPrefabRenderer.h"
#include "EditorRenderer.h"
#include "EditorSelectedPrefabObject.h"

/************************************************************************************************/


ID3D12PipelineState* CreateFlatSkinnedPassPSO(RenderSystem* RS)
{
	auto DrawRectVShader = RS->LoadShader("ForwardSkinned_VS",  "vs_6_0", "assets\\shaders\\forwardRender.hlsl");
	auto DrawRectPShader = RS->LoadShader("ColoredPolys",       "ps_6_0", "assets\\shaders\\forwardRender.hlsl");

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
		{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 3, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

		{ "BLENDWEIGHT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    4, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BLENDINDICES",	0, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT,  5, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};


	D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//Rast_Desc.FillMode = D3D12_FILL_MODE_WIREFRAME;
	//Rast_Desc.CullMode = D3D12_CULL_MODE_NONE;

	D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
	Depth_Desc.DepthEnable	= true;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
		PSO_Desc.pRootSignature        = RS->Library.RS6CBVs4SRVs;
		PSO_Desc.VS                    = DrawRectVShader;
		PSO_Desc.PS                    = DrawRectPShader;
		PSO_Desc.RasterizerState       = Rast_Desc;
		PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		PSO_Desc.SampleMask            = UINT_MAX;
		PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		PSO_Desc.NumRenderTargets      = 1;
		PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // Albedo
		PSO_Desc.SampleDesc.Count      = 1;
		PSO_Desc.SampleDesc.Quality    = 0;
		PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
		PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
		PSO_Desc.DepthStencilState     = Depth_Desc;
		PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
	}

	ID3D12PipelineState* PSO = nullptr;
	auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
	FK_ASSERT(SUCCEEDED(HR));

	return PSO;
}


/************************************************************************************************/


ID3D12PipelineState* CreateFlatPassPSO(RenderSystem* RS)
{
	auto DrawRectVShader = RS->LoadShader("Forward_VS", "vs_6_0", R"(assets\shaders\forwardRender.hlsl)");
	auto DrawRectPShader = RS->LoadShader("ColoredPolys", "ps_6_0", R"(assets\shaders\forwardRender.hlsl)");

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
		{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 3, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};


	D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//Rast_Desc.FillMode = D3D12_FILL_MODE_WIREFRAME;
	//Rast_Desc.CullMode = D3D12_CULL_MODE_NONE;

	D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
	Depth_Desc.DepthEnable	= true;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
		PSO_Desc.pRootSignature        = RS->Library.RS6CBVs4SRVs;
		PSO_Desc.VS                    = DrawRectVShader;
		PSO_Desc.PS                    = DrawRectPShader;
		PSO_Desc.RasterizerState       = Rast_Desc;
		PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		PSO_Desc.SampleMask            = UINT_MAX;
		PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		PSO_Desc.NumRenderTargets      = 1;
		PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // Albedo
		PSO_Desc.SampleDesc.Count      = 1;
		PSO_Desc.SampleDesc.Quality    = 0;
		PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
		PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
		PSO_Desc.DepthStencilState     = Depth_Desc;
		PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
	}

	ID3D12PipelineState* PSO = nullptr;
	auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
	FK_ASSERT(SUCCEEDED(HR));

	return PSO;
}


/************************************************************************************************/


constexpr FlexKit::PSOHandle FLATSKINNED_PSO    = FlexKit::PSOHandle(GetTypeGUID(FlatSkinned));
constexpr FlexKit::PSOHandle FLAT_PSO           = FlexKit::PSOHandle(GetTypeGUID(Flat));


struct LoadEntityContext : public LoadEntityContextInterface
{
	LoadEntityContext(
		std::vector<FlexKit::NodeHandle>&	IN_nodes,
		EditorScene_ptr						IN_viewportscene,
		EditorViewport&						IN_viewport,
		FlexKit::Scene&						IN_scene,
		FlexKit::GameObject&				IN_gameObject,
		FlexKit::MaterialHandle				IN_defaultMaterial)
		: nodes				{ IN_nodes				}
		, viewportscene		{ IN_viewportscene		}
		, viewport			{ IN_viewport			}
		, scene				{ IN_scene				}
		, gameObject		{ IN_gameObject			}
		, defaultMaterial	{ IN_defaultMaterial	} {}

	std::vector<FlexKit::NodeHandle>&	nodes;
	EditorScene_ptr						viewportscene;
	EditorViewport&						viewport;
	FlexKit::Scene&						scene;
	FlexKit::GameObject&				gameObject;
	FlexKit::MaterialHandle				defaultMaterial;

	FlexKit::GameObject& GameObject() override
	{
		return gameObject;
	}

	FlexKit::NodeHandle GetNode(uint32_t idx) override
	{
		return nodes[idx];
	}

	ProjectResource_ptr FindSceneResource(uint64_t assetIdx) override
	{
		return viewportscene->FindSceneResource(assetIdx);
	}

	FlexKit::TriMeshHandle LoadTriMeshResource(ProjectResource_ptr resource) override
	{
		return FlexKit::InvalidHandle;
	}

	FlexKit::MaterialHandle DefaultMaterial() const override
	{
		return defaultMaterial;
	}

	FlexKit::Scene* Scene()
	{
		return nullptr;
	}
};


/************************************************************************************************/


EditorPrefabPreview::EditorPrefabPreview(EditorRenderer& IN_renderer, EditorSelectedPrefabObject* IN_selection, EditorProject& IN_project, QWidget* parent)
		: QWidget		{ parent }
		, layer			{ FlexKit::PhysXComponent::GetComponent().CreateLayer() }
		, renderer		{ IN_renderer }
		, renderWindow	{ IN_renderer.CreateRenderWindow(this) }
		, depthBuffer	{ IN_renderer.GetRenderSystem(), renderWindow->WH() }
		, selection		{ IN_selection }
		, project		{ IN_project }
		, previewCamera	{ FlexKit::CameraComponent::GetComponent().CreateCamera() }
	{
		FlexKit::SetCameraNode(previewCamera, FlexKit::GetZeroedNode());

		auto& renderSystem = renderer.GetRenderSystem();
		renderSystem.RegisterPSOLoader(FLATSKINNED_PSO, { &renderSystem.Library.RS6CBVs4SRVs, &CreateFlatSkinnedPassPSO });
		renderSystem.RegisterPSOLoader(FLAT_PSO, { &renderSystem.Library.RS6CBVs4SRVs, &CreateFlatPassPSO });

		renderWindow->SetOnDraw(
			[&](FlexKit::UpdateDispatcher& dispatcher, double dT, TemporaryBuffers& temporaries, FlexKit::FrameGraph& frameGraph, FlexKit::ResourceHandle renderTarget, FlexKit::ThreadSafeAllocator& allocator)
			{
				if (isVisible())
					RenderAnimated(dispatcher, dT, temporaries, frameGraph, renderTarget, allocator);
			});
	}


/************************************************************************************************/


void EditorPrefabPreview::resizeEvent(QResizeEvent* evt)
{
	QWidget::resizeEvent(evt);

	auto size = evt->size();
	FlexKit::uint2 newWH = { evt->size().width() * 1.5, evt->size().height() * 1.5 };


	renderWindow->resizeEvent(evt);
	depthBuffer.Resize(newWH);

	FlexKit::SetCameraAspectRatio(previewCamera, float(evt->size().width()) / float(evt->size().height()));
}


/************************************************************************************************/


void EditorPrefabPreview::RenderStatic(
	FlexKit::GameObject&			gameObject,
	FlexKit::UpdateDispatcher&		dispatcher,
	double							dT,
	TemporaryBuffers&				temporaryBuffers,
	FlexKit::FrameGraph&			frameGraph,
	FlexKit::ResourceHandle			renderTarget,
	FlexKit::ThreadSafeAllocator&	allocator)
{
	struct Pass
	{
		FlexKit::ReserveConstantBufferFunction  reserveCB;
		FlexKit::ReserveVertexBufferFunction    reserveVB;

		FlexKit::FrameResourceHandle renderTarget;
		FlexKit::FrameResourceHandle depthTarget;
	};

	auto WH = frameGraph.GetRenderSystem().GetTextureWH(renderTarget);
	FlexKit::SetCameraAspectRatio(previewCamera, (float)WH[0] / (float)WH[1]);
	FlexKit::MarkCameraDirty(previewCamera);

	auto& transforms		= FlexKit::QueueTransformUpdateTask(dispatcher);
	auto& cameras			= FlexKit::CameraComponent::GetComponent().QueueCameraUpdate(dispatcher);

	cameras.AddInput(transforms);

	frameGraph.AddNode<Pass>(
		Pass{
			temporaryBuffers.ReserveConstantBuffer,
			temporaryBuffers.ReserveVertexBuffer,
		},
		[&](FlexKit::FrameGraphNodeBuilder& builder, Pass& data)
		{
			data.renderTarget   = builder.RenderTarget(renderTarget);
			data.depthTarget    = builder.DepthTarget(depthBuffer.Get());

			builder.AddDataDependency(cameras);

			if (animate)
			{
				auto& animationUpdate = FlexKit::UpdateAnimations(dispatcher, dT);
				builder.AddDataDependency(animationUpdate);
			}
		},
		[=, &gameObject](Pass& data, const FlexKit::ResourceHandler& frameResources, FlexKit::Context& ctx, FlexKit::iAllocator& allocator)
		{
			using namespace FlexKit;
			auto brush = GetBrush(gameObject);

			if (!brush || brush->meshes.empty() || brush->material == FlexKit::InvalidHandle)
				return;

			auto materialHndl	= brush->material;
			auto constants		= brush->GetConstants();

			auto& materials			= MaterialComponent::GetComponent();
			const auto materialData	= MaterialComponent::GetComponent()[materialHndl];
			auto skeleton			= FlexKit::GetSkeleton(gameObject);
			auto poseState			= FlexKit::GetPoseState(gameObject);

			struct ForwardDrawConstants
			{
				float LightCount;
				float t;
				uint2 WH;
			};

			const size_t entityBufferSize =
				AlignedSize<Brush::VConstantsLayout>();

			constexpr size_t passBufferSize =
				AlignedSize<Camera::ConstantBuffer>() +
				AlignedSize<ForwardDrawConstants>();

			struct EntityPoses
			{
				float4x4 transforms[768];

				auto& operator [](size_t idx)
				{
					return transforms[idx];
				}
			};

			const size_t poseBufferSize =
				AlignedSize<EntityPoses>();

			auto passConstantBuffer   = data.reserveCB(passBufferSize);
			auto entityConstantBuffer = data.reserveCB(entityBufferSize);
			auto poseBuffer           = data.reserveCB(poseBufferSize);

			const auto cameraConstants  = ConstantBufferDataSet{ GetCameraConstants(previewCamera), passConstantBuffer };
			const auto passConstants    = ConstantBufferDataSet{ ForwardDrawConstants{ 1, 1 }, passConstantBuffer };

			ctx.SetRootSignature(frameResources.renderSystem().Library.RS6CBVs4SRVs);

			if (poseState)
			{
				ctx.SetPipelineState(frameResources.GetPipelineState(FLATSKINNED_PSO));
				ctx.SetPrimitiveTopology(FlexKit::EIT_TRIANGLELIST);

				ctx.SetScissorAndViewports({ renderTarget });
				ctx.SetRenderTargets(
					{ frameResources.GetResource({ data.renderTarget }) },
					true, frameResources.GetResource(data.depthTarget));

				auto& poses = allocator.allocate<EntityPoses>();

				FlexKit::UpdatePose(*poseState, allocator);

				for (size_t I = 0; I < poseState->JointCount; I++)
					poses[I] = skeleton->IPose[I] * poseState->CurrentPose[I];

				ctx.SetGraphicsConstantBufferView(1, cameraConstants);
				ctx.SetGraphicsConstantBufferView(2, ConstantBufferDataSet(constants, entityConstantBuffer));
				ctx.SetGraphicsConstantBufferView(3, passConstants);
				ctx.SetGraphicsConstantBufferView(4, ConstantBufferDataSet(poses, poseBuffer));

				for (auto mesh : brush->meshes)
				{
					auto triMesh	= GetMeshResource(mesh);
					auto lodLevel	= triMesh->GetHighestLoadedLodIdx();

					ctx.AddIndexBuffer(triMesh, lodLevel);

					ctx.AddVertexBuffers(
						triMesh,
						lodLevel,
						{
							VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
							VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
							VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT,
							VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,
							VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1,
							VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2,
						}
					);

					ctx.DrawIndexed(triMesh->GetHighestLoadedLod().GetIndexCount());
				}
			}
			else
			{
				ctx.SetPipelineState(frameResources.GetPipelineState(FLAT_PSO));
				ctx.SetPrimitiveTopology(FlexKit::EIT_TRIANGLELIST);

				ctx.SetScissorAndViewports({ renderTarget });
				ctx.SetRenderTargets(
					{ frameResources.GetResource({ data.renderTarget }) },
					true, frameResources.GetResource(data.depthTarget));

				ctx.SetGraphicsConstantBufferView(1, cameraConstants);
				ctx.SetGraphicsConstantBufferView(2, ConstantBufferDataSet(constants, entityConstantBuffer));
				ctx.SetGraphicsConstantBufferView(3, passConstants);

				for (auto mesh : brush->meshes)
				{
					auto triMesh	= GetMeshResource(mesh);
					auto lodLevel	= triMesh->GetHighestLoadedLodIdx();
					ctx.AddIndexBuffer(triMesh, lodLevel);

					ctx.AddVertexBuffers(
						triMesh,
						lodLevel,
						{
							VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
							VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
							VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT,
							VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,
						}
					);

					ctx.DrawIndexed(triMesh->GetHighestLoadedLod().GetIndexCount());
				}
			}
		});
}


/************************************************************************************************/


void EditorPrefabPreview::RenderAnimated(
	FlexKit::UpdateDispatcher&		dispatcher,
	double							dT,
	TemporaryBuffers&				temporaryBuffers,
	FlexKit::FrameGraph&			frameGraph,
	FlexKit::ResourceHandle			renderTarget,
	FlexKit::ThreadSafeAllocator&	allocator)
{
	if (!isVisible() || renderer.GetRenderSystem().GetTextureWH(renderTarget).Product() == 0)
		return;

	FlexKit::ClearBackBuffer(frameGraph, renderTarget, FlexKit::float4{ 0, 0, 0, 0 });
	FlexKit::ClearDepthBuffer(frameGraph, depthBuffer.Get(), 1.0f);

	if (selection && selection->ID != -1)
	{
		auto& object = *selection;

		if (turnTable)
			FlexKit::Yaw(object.gameObject, 1.0f / 60.0f);

		RenderStatic(object.gameObject, dispatcher, dT, temporaryBuffers, frameGraph, renderTarget, allocator);
			
		if (const auto pose = FlexKit::GetPoseState(object.gameObject); skeletonOverlay && pose)
		{
			const auto node = FlexKit::GetSceneNode(object.gameObject);
			const auto PV   = GetCameraConstants(previewCamera).PV;

			FlexKit::LineSegments lines(FlexKit::SystemAllocator);
			lines = FlexKit::DEBUG_DrawPoseState(*pose, node, FlexKit::SystemAllocator);

			for (auto& line : lines)
			{
				const auto tempA = PV * float4{ line.A, 1 };
				const auto tempB = PV * float4{ line.B, 1 };

				if (tempA.w <= 0 || tempB.w <= 0)
				{
					line.A = { 0, 0, 0 };
					line.B = { 0, 0, 0 };
				}
				else
				{
					line.A = tempA.xyz() / tempA.w;
					line.B = tempB.xyz() / tempB.w;
				}
			}

			DrawShapes(
				FlexKit::DRAW_LINE_PSO,
				frameGraph,
				temporaryBuffers.ReserveVertexBuffer,
				temporaryBuffers.ReserveConstantBuffer,
				renderTarget,
				allocator,
				FlexKit::LineShape{ lines });
		}

	}

	if (QDTreeOverlay || boundingVolume || SMboundingVolumes)
		RenderOverlays(
			dispatcher,
			dT,
			temporaryBuffers,
			frameGraph,
			renderTarget,
			allocator);

	FlexKit::PresentBackBuffer(frameGraph, renderTarget);
}


/************************************************************************************************/


void EditorPrefabPreview::RenderOverlays(
	FlexKit::UpdateDispatcher&		dispatcher,
	double							dT,
	TemporaryBuffers&				temporaryBuffers,
	FlexKit::FrameGraph&			frameGraph,
	FlexKit::ResourceHandle			renderTarget,
	FlexKit::ThreadSafeAllocator&	allocator)
{
	auto& object			= selection->gameObject;
	const auto node			= FlexKit::GetSceneNode(object);
	const auto constants	= GetCameraConstants(previewCamera);
	const auto PV			= constants.PV;
	const auto Q			= FlexKit::GetOrientation(object);
	const auto WT			= FlexKit::GetWT(object).Transpose();
	const auto brush		= FlexKit::GetBrush(object);

	if (!brush)
		return;

	FlexKit::LineSegments lines(allocator);

	using KDBNode = FlexKit::MeshUtilityFunctions::MeshKDBTree::KDBNode;

	const static float3 Colors[] = {
		{ 1, 0, 0 },
		{ 0, 1, 0 },
		{ 0, 0, 1 },
		{ 1, 0, 1 },
		{ 1, 1, 1 },
	};

	size_t colorIdx = 0;

	auto AABBtoLines = [&](FlexKit::AABB aabb, float3 color)
	{
		const auto min = aabb.Min;
		const auto max = aabb.Max;

		const float3 p1 = float3{ min.x, min.y, min.z };
		const float3 p2 = float3{ max.x, min.y, min.z };
		const float3 p3 = float3{ max.x, min.y, max.z };
		const float3 p4 = float3{ min.x, min.y, max.z };

		const float3 p5 = float3{ min.x, max.y, min.z };
		const float3 p6 = float3{ max.x, max.y, min.z };
		const float3 p7 = float3{ max.x, max.y, max.z };
		const float3 p8 = float3{ min.x, max.y, max.z };


		lines.emplace_back(
			p1, color,
			p2, color);

		lines.emplace_back(
			p2, color,
			p3, color);

		lines.emplace_back(
			p3, color,
			p4, color);

		lines.emplace_back(
			p4, color,
			p1, color);

		lines.emplace_back(
			p5, color,
			p6, color);

		lines.emplace_back(
			p6, color,
			p7, color);

		lines.emplace_back(
			p7, color,
			p8, color);

		lines.emplace_back(
			p8, color,
			p5, color);

		lines.emplace_back(
			p1, color,
			p5, color);

		lines.emplace_back(
			p2, color,
			p6, color);

		lines.emplace_back(
			p3, color,
			p7, color);

		lines.emplace_back(
			p4, color,
			p8, color);
	};

	auto ProcessNode =
		[&](this auto& self, KDBNode* node, FlexKit::MeshUtilityFunctions::MeshKDBTree* tree) -> void
		{
			if(node->left == nullptr && node->right == nullptr)
				AABBtoLines(node->aabb, Colors[colorIdx++ % 5]);

			if (node->left.get())
				self(node->left.get(), tree);

			if(node->right.get())
				self(node->right.get(), tree);
		};

	for (auto& mesh : brush->meshes)
	{
		auto guid		= FlexKit::GetMeshResource(mesh)->assetHandle;
		auto projectRes = project.FindProjectResource(guid);

		auto meshResource	= static_cast<FlexKit::MeshResource*>(projectRes->resource.get());

		if (boundingVolume)
			AABBtoLines(FlexKit::GetAABBFromMesh(object), float3{237.0f / 256.0f, 231.0f / 256.0f, 107.0f / 256.0f});

		if (SMboundingVolumes)
		{
			for (auto& sm : meshResource->data->LODs[0].submeshes)
				AABBtoLines(sm.aabb, Colors[colorIdx++ % 5]);
		}

		if (QDTreeOverlay)
		{
			for (auto& tree : meshResource->data->kdbTrees)
			{
				auto* tree_ptr = tree.get();
				ProcessNode(tree_ptr->root.get(), tree_ptr);
			}
		}
	}

	static const float4 points[] = {
		{ 0, 0, 0, 1 },
		{ 0, 0, 0, 1 },
		{ 0, 0, 0, 1 },
		{ 0, 0, 0, 1 },
	};

	for (auto& line : lines)
	{
		const auto tempA1 = WT * float4{ line.A, 1 };
		const auto tempB1 = WT * float4{ line.B, 1 };

		const auto tempA2 = PV * float4{ tempA1.xyz(), 1 };
		const auto tempB2 = PV * float4{ tempB1.xyz(), 1 };

		if (tempA2.w <= 0 || tempB2.w <= 0)
		{
			line.A = { 0, 0, 0 };
			line.B = { 0, 0, 0 };
		}
		else
		{
			line.A = tempA2.xyz() / tempA2.w;
			line.B = tempB2.xyz() / tempB2.w;
		}
	}

	DrawShapes(
		FlexKit::DRAW_LINE_PSO,
		frameGraph,
		temporaryBuffers.ReserveVertexBuffer,
		temporaryBuffers.ReserveConstantBuffer,
		renderTarget,
		allocator,
		FlexKit::LineShape{ lines });
}


/************************************************************************************************/


void EditorPrefabPreview::CenterCamera()
{
	auto& gameObject	= selection->gameObject;
	auto meshes			= FlexKit::GetTriMesh(gameObject);

	if (meshes.empty())
		return;

	auto aabb				= FlexKit::GetAABBFromMesh(gameObject);
	const FlexKit::Camera c = FlexKit::CameraComponent::GetComponent().GetCamera(previewCamera);

	const auto target			= aabb.MidPoint();
	const auto desiredDistance	= 2.5f * aabb.Span().magnitude() / std::tan(c.FOV);

	auto position_VS		= c.View.Transpose() * float4 { target, 1 };
	auto updatedPosition_WS	= c.IV.Transpose() * float4 { position_VS.x, position_VS.y, position_VS.z + desiredDistance, 1 };

	const auto node		= FlexKit::GetCameraNode(previewCamera);
	const Quaternion Q	= GetOrientation(node);
	auto forward		= (Q * float3{ 0.0f, 0.0f, -1.0f}).normal();

	FlexKit::SetPositionW(node, updatedPosition_WS.xyz());
	FlexKit::MarkCameraDirty(previewCamera);
}


/************************************************************************************************/
