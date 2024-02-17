#include "PCH.h"
#include "DXRenderWindow.h"
#include "EditorPlayer.h"
#include "EditorPrefabEditor.h"
#include "EditorPrefabObject.h"
#include "EditorPrefabRenderer.h"
#include "EditorRenderer.h"
#include "EditorSelectedPrefabObject.h"

#define BOOST_ASIO_NO_WIN32_LEAN_AND_MEAN
#include <boost/process.hpp>

#include "Serialization.hpp"
#include <type_traits>

/************************************************************************************************/


FlexKit::LoadPipelineStateRes CreateFlatSkinnedPassPSO(RenderSystem* RS, iAllocator&)
{
	auto DrawRectVShader = RS->LoadShader("ForwardSkinned_VS",	"vs_6_0", "assets\\shaders\\forwardRender.hlsl");
	auto DrawRectPShader = RS->LoadShader("GreyPolys",			"ps_6_0", "assets\\shaders\\forwardRender.hlsl");

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
		{ "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 3, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

		{ "BLENDWEIGHT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    4, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BLENDINDICES",	0, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT,  5, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

		{ "BLENDPOS",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,  6, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BLENDNORM",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,  7, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BLENDTAN",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,  8, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};


	D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//Rast_Desc.FillMode = D3D12_FILL_MODE_WIREFRAME;
	Rast_Desc.CullMode = D3D12_CULL_MODE_NONE;

	D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
	Depth_Desc.DepthEnable	= false;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
		PSO_Desc.pRootSignature        = *RS->Library.RS6CBVs4SRVs;
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

	SETDEBUGNAME(PSO, "DrawGrayPrefab");

	return { PSO, RS->Library.RS6CBVs4SRVs };
}


/************************************************************************************************/


FlexKit::LoadPipelineStateRes CreateFlatPassPSO(RenderSystem* RS, iAllocator&)
{
	auto DrawRectVShader = RS->LoadShader("Forward_VS", "vs_6_0", R"(assets\shaders\forwardRender.hlsl)");
	auto DrawRectPShader = RS->LoadShader("GreyPolys",	"ps_6_0", R"(assets\shaders\forwardRender.hlsl)");

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
		{ "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 3, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};


	D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//Rast_Desc.FillMode = D3D12_FILL_MODE_WIREFRAME;
	Rast_Desc.CullMode = D3D12_CULL_MODE_NONE;

	D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
	Depth_Desc.DepthEnable	= false;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
		PSO_Desc.pRootSignature        = *RS->Library.RS6CBVs4SRVs;
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

	SETDEBUGNAME(PSO, "DrawFlatPrefab");

	return { PSO, RS->Library.RS6CBVs4SRVs };
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


template<typename TY>
concept IPCMessage = std::is_base_of_v<MessageInterface, typename TY::element_type>;

struct PlayerContext
{
	iAllocator&								sharedMemory;
	SharedEngineMemory*						shared;

	boost::process::child child;

	void push_message(IPCMessage auto& message)
	{
		shared->PushMessageToPlayer(message);
	}

	auto PollMessage()
	{
		return shared->PollEditorMessages();
	}
};


/************************************************************************************************/



EditorPrefabPreview::EditorPrefabPreview(EditorRenderer& IN_renderer, EditorSelectedPrefabObject* IN_selection, EditorProject& IN_project, QWidget* parent)
		: QWidget		{ parent }
		, layer			{ FlexKit::PhysXComponent::GetComponent().CreateLayer() }
		, renderer		{ IN_renderer }
		//, sharedWindow	{ this }
#if LOCALPLAYER
		, renderWindow	{ IN_renderer.CreateRenderWindow(this) }
		, depthBuffer	{ IN_renderer.GetRenderSystem(), renderWindow->WH() }
#endif
		, selection		{ IN_selection }
		, project		{ IN_project }
		, previewCamera	{ FlexKit::CameraComponent::GetComponent().CreateCamera() }
{
	auto node = FlexKit::GetZeroedNode();
	FlexKit::SetCameraNode(previewCamera, node);
	FlexKit::TranslateWorld(node, { 0, 0, 10 });

	auto& renderSystem = renderer.GetRenderSystem();
	renderSystem.RegisterPSOLoader(FLATSKINNED_PSO,	&CreateFlatSkinnedPassPSO);
	renderSystem.RegisterPSOLoader(FLAT_PSO,		&CreateFlatPassPSO);


#if LOCALPLAYER
	renderWindow->SetOnDraw(
		[&](FlexKit::UpdateDispatcher& dispatcher, double dT, TemporaryBuffers& temporaries, FlexKit::FrameGraph& frameGraph, FlexKit::ResourceHandle renderTarget, FlexKit::ThreadSafeAllocator& allocator)
		{
			if (isVisible())
				RenderAnimated(dispatcher, frameGraph, dT, temporaries, renderTarget, allocator);
		});
#else
	auto shared = renderer.GetSharedMemory();
	shared->targetWindow = sharedWindow.GetHWND();

	playerContext = std::make_unique<PlayerContext>(
		shared->blockAllocator,
		shared,
		boost::process::child(
			"flxEditor.exe",
			boost::process::args(std::format("--player", IN_renderer.GetSharedAddress())),
			boost::process::args(std::format("{}", IN_renderer.GetSharedAddress()))));

	while (!shared->PollEditorMessages());
#endif
}



/************************************************************************************************/


EditorPrefabPreview::~EditorPrefabPreview()
{
#if LOCALPLAYER == 0
	if (playerContext->child.running())
	{	// Kill Player!
		struct QuitMessage : public FlexKit::Serializable<QuitMessage, MessageInterface, GetTypeGUID(QuitMessage)>
		{
			void Do(EditorPlayerState& player) override
			{
				player.Shutdown();
			}

			void Serialize(auto& archive) {}
		};

		auto quiteMessage = std::make_shared<QuitMessage>();
		playerContext->push_message(quiteMessage);

		while (playerContext->child.running());
	}
#endif
}


/************************************************************************************************/


void EditorPrefabPreview::ProcessMessages()
{
	return;
	if (!playerContext->child.running())
	{	// Restart Player!
	}

	EditorContext context
	{
		.shared		= *playerContext->shared,
		.renderer	= renderer,
		.project	= project,
	};

	while (auto message = playerContext->PollMessage())
	{
		auto& temp		= message.value();
		
		if (!temp.buffer.size())
			continue;
		
		FlexKit::Blob					blob	{ (const char*)temp.buffer.data(), temp.buffer.size() };
		FlexKit::LoadBlobArchiveContext	loader	{ blob };
		
		std::shared_ptr<EditorMessageInterface> freshMessage;
		loader& freshMessage;

		freshMessage->Do(context);
	}
}


/************************************************************************************************/


struct PlayerResetMessage : public FlexKit::Serializable<PlayerResetMessage, MessageInterface, GetTypeGUID(ResetMessage)>
{
	void Do(EditorPlayerState& player) override
	{
		player.Reset();
	}

	void Serialize(auto& archive) {}
};

void EditorPrefabPreview::Reset()
{
	return;
	auto resetMsg = std::make_shared<PlayerResetMessage>();
	renderer.GetSharedMemory()->currentGameObject = nullptr;
	playerContext->push_message(resetMsg);
}


/************************************************************************************************/


FlexKit::GameObject* EditorPrefabPreview::GetGameObject()
{
	return &selection->gameObject;
}


/************************************************************************************************/


struct EditorSetBrushMessage : public FlexKit::Serializable<EditorSetBrushMessage, MessageInterface, GetTypeGUID(ResizeMessage)>
{
	void Do(EditorPlayerState& player) override
	{
		auto triMeshResource = FlexKit::FindMesh(meshHandle);

		if (triMeshResource == FlexKit::InvalidHandle)
			player.SendErrorMessage("Resource not found!");
		else
			player.gameObject->AddView<FlexKit::BrushView>(triMeshResource.value());
	}

	void Serialize(auto& archive)
	{
		archive& meshHandle;
	}

	FlexKit::AssetHandle	meshHandle;
};

void EditorPrefabPreview::SetBrush(FlexKit::AssetHandle handle)
{
	auto res = project.FindProjectResource(handle);
	if (!res)
		return;

	SendResource(*res->resource.get(), *playerContext->shared);

	auto brushMsg			= std::make_shared<EditorSetBrushMessage>();
	brushMsg->meshHandle	= handle;

	playerContext->push_message(brushMsg);
}


/************************************************************************************************/


void EditorPrefabPreview::resizeEvent(QResizeEvent* evt)
{
	QWidget::resizeEvent(evt);

	//auto size = evt->size();
	//FlexKit::uint2 newWH = { FlexKit::Max(1, evt->size().width() * 1.5), FlexKit::Max(1, evt->size().height() * 1.5) };
	//
	//struct ResizeMessage : public FlexKit::Serializable<ResizeMessage, MessageInterface, GetTypeGUID(ResizeMessage)>
	//{
	//	FlexKit::uint2 newWH;
	//
	//	void Do(EditorPlayerState& player) override
	//	{
	//		player.renderWindow.Resize(newWH);
	//		player.depthBuffer.Resize(newWH);
	//		FlexKit::SetCameraAspectRatio(player.activeCamera, (float)newWH[0] / (float)newWH[1]);
	//	}
	//
	//	void Serialize(auto& archive)
	//	{
	//		archive& newWH;
	//	}
	//};
	//
	//auto resize = std::make_shared<ResizeMessage>();
	//
	//resize->newWH = newWH;
	//
	//playerContext->push_message(resize);
	//sharedWindow.resize(evt->size());

	auto size = evt->size();
	FlexKit::uint2 newWH = {
		FlexKit::Max(evt->size().width() * 1.5, 1),
		FlexKit::Max(evt->size().height() * 1.5, 1)	};


	renderWindow->resizeEvent(evt);
	depthBuffer.Resize(newWH);

	FlexKit::SetCameraAspectRatio(previewCamera, float(evt->size().width()) / float(evt->size().height()));
}


/************************************************************************************************/


void EditorPrefabPreview::RenderStatic(
	FlexKit::UpdateDispatcher&		dispatcher,
	FlexKit::FrameGraph&			frameGraph,
	FlexKit::GameObject&			gameObject,
	double							dT,
	TemporaryBuffers&				temporaryBuffers,
	FlexKit::ResourceHandle			renderTarget,
	FlexKit::ThreadSafeAllocator&	allocator)
{
	struct Pass
	{
		FlexKit::ReserveConstantBufferFunction  reserveCB;
		FlexKit::ReserveVertexBufferFunction    reserveVB;

		FlexKit::FrameResourceHandle renderTarget;
		FlexKit::FrameResourceHandle depthTarget;
		FlexKit::FrameResourceHandle poseBuffer;
	};

	auto WH = frameGraph.GetRenderSystem().GetTextureWH(renderTarget);
	FlexKit::SetCameraAspectRatio(previewCamera, (float)WH[0] / (float)WH[1]);
	FlexKit::MarkCameraDirty(previewCamera);

	auto& transforms		= FlexKit::QueueTransformUpdateTask(dispatcher);
	auto& cameras			= FlexKit::CameraComponent::GetComponent().QueueCameraUpdate(dispatcher);

	cameras.AddInput(transforms);

	renderer.worldRender.AddMemoryPools(frameGraph);

	frameGraph.AddNode<Pass>(
		Pass{
			temporaryBuffers.ReserveConstantBuffer,
			temporaryBuffers.ReserveVertexBuffer,
		},
		[&](FlexKit::FrameGraphNodeBuilder& builder, Pass& data)
		{
			data.renderTarget	= builder.RenderTarget(renderTarget);
			data.depthTarget	= builder.DepthTarget(depthBuffer.Get());
			data.poseBuffer		= builder.AcquireVirtualResource(FlexKit::GPUResourceDesc::StructuredResource(64 * 1024), FlexKit::DASCopyDest);

			builder.AddDataDependency(cameras);

			if (animate || singleStep)
			{
				auto& animationUpdate = FlexKit::UpdateAnimations(dispatcher, singleStep ? stepSize : dT);
				builder.AddDataDependency(animationUpdate);
				singleStep = false;
			}
		},
		[=, &gameObject](Pass& data, const FlexKit::ResourceHandler& frameResources, FlexKit::Context& ctx, FlexKit::iAllocator& allocator)
		{
			using namespace FlexKit;
			auto brush = GetBrush(gameObject);

			if (!brush || brush->meshes.empty())
				return;

			auto materialHndl	= brush->material;
			auto constants		= brush->GetConstants();
			auto WH				= frameResources.GetTextureWH(data.renderTarget);

			auto skeleton	= FlexKit::GetSkeleton(gameObject);
			auto poseState	= FlexKit::GetPoseState(gameObject);

			struct ForwardDrawConstants
			{
				uint32_t	LightCount;
				float		t;
				uint2		WH;
			};

			const size_t entityBufferSize =
				AlignedSize<Brush::VConstantsLayout>();

			constexpr size_t passBufferSize =
				AlignedSize<Camera::ConstantBuffer>() +
				AlignedSize<ForwardDrawConstants>();

			struct EntityPoses
			{
				float4x4_GPU transforms[768];

				auto& operator [](size_t idx) { return transforms[idx]; }
			};

			const size_t poseBufferSize =
				AlignedSize<EntityPoses>();

			auto passConstantBuffer		= data.reserveCB(passBufferSize);
			auto entityConstantBuffer	= data.reserveCB(entityBufferSize);
			auto poseBuffer				= data.reserveCB(poseBufferSize);

			const auto cameraConstants	= ConstantBufferDataSet{ GetCameraConstants(previewCamera), passConstantBuffer};
			const auto passConstants	= ConstantBufferDataSet{ ForwardDrawConstants{ .LightCount = 1, .t = 1, .WH = WH }, passConstantBuffer };

			auto& rootSignature = frameResources.renderSystem().Library.RS6CBVs4SRVs;
			ctx.SetRootSignature(rootSignature);

			DescriptorHeap emptyHeap(ctx, rootSignature->GetDescHeap(0), allocator);
			emptyHeap.NullFill(ctx);
			ctx.SetGraphicsDescriptorTable(0, emptyHeap);

			if (poseState)
			{
				ctx.SetPipelineState(frameResources.GetPipelineState(FLATSKINNED_PSO, allocator));
				ctx.SetInputPrimitive(FlexKit::INPUTPRIMITIVETRIANGLELIST);

				ctx.SetScissorAndViewports({ renderTarget });
				ctx.SetRenderTargets(
					{ frameResources.GetResource({ data.renderTarget }) },
					true, frameResources.GetResource(data.depthTarget));

				auto poseSize	= poseState->JointCount * sizeof(float4x4_GPU);
				auto poseBuffer = ctx.ReserveDirectUploadSpace(poseSize);

				FlexKit::UpdatePose(*poseState, allocator);

				for (size_t I = 0; I < poseState->JointCount; I++)
					reinterpret_cast<float4x4*>(poseBuffer.buffer)[I] = skeleton->IPose[I] * poseState->CurrentPose[I];

				ctx.CopyBufferRegion(frameResources.GetResource(data.poseBuffer), poseBuffer.resource, poseSize, 0, poseBuffer.offset);

				ctx.SetGraphicsConstantBufferView(1, cameraConstants);
				ctx.SetGraphicsConstantBufferView(2, ConstantBufferDataSet(constants, entityConstantBuffer));
				ctx.SetGraphicsConstantBufferView(3, passConstants);

				ctx.SetGraphicsShaderResourceView(7, frameResources.PixelShaderResource(data.poseBuffer, ctx, FlexKit::DeviceSyncPoint::Sync_Copy, FlexKit::DeviceSyncPoint::Sync_VertexShader));

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
				ctx.SetPipelineState(frameResources.GetPipelineState(FLAT_PSO, allocator));
				ctx.SetInputPrimitive(FlexKit::INPUTPRIMITIVETRIANGLELIST);

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
	FlexKit::FrameGraph&			frameGraph,
	double							dT,
	TemporaryBuffers&				temporaryBuffers,
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
		{
			yaw += turnTableRate * dT;
			yaw = fmod(yaw, 1.0f);
		}

		FlexKit::SetOrientation(object.gameObject, Quaternion{ 0, yaw * 360.0f, 0 });

		RenderStatic(dispatcher, frameGraph, object.gameObject, dT, temporaryBuffers, renderTarget, allocator);
			
		if (const auto pose = FlexKit::GetPoseState(object.gameObject); skeletonOverlay && pose)
		{
			const auto		node	= FlexKit::GetSceneNode(object.gameObject);
			const float4x4	PV		= GetCameraConstants(previewCamera).PV;

			FlexKit::LineSegments lines = FlexKit::DEBUG_DrawPoseState(*pose, node, allocator);

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
			frameGraph,
			dT,
			temporaryBuffers,
			renderTarget,
			allocator);

	const auto HW			= frameGraph.GetRenderSystem().GetTextureWH(renderTarget);
	QPoint globalCursorPos	= QCursor::pos();
	auto localPosition		= renderWindow->mapFromGlobal(globalCursorPos);

	renderer.hud.Update({ (float)localPosition.x() * 1.5f, (float)localPosition.y() * 1.5f }, HW, dispatcher, dT);

	ImGui::NewFrame();

	if (playBackWindow)
	{
		if (ImGui::Begin("Playback", nullptr)) {
			ImGui::SetWindowPos({ 0, 0 });
			ImGui::Checkbox("Play", &animate);
			ImGui::SliderFloat("Step size", &stepSize, -1.0f, 1.0f);

			if (ImGui::Button("Single Step"))
				singleStep = !singleStep;

			ImGui::SliderFloat("Rotate View", &yaw, 0, 1.0f);
			ImGui::SliderFloat("TurnTable Rate", &turnTableRate, 0, 3.0f);
		}
		ImGui::End();
	}

	if (jointInfoWindow && selection && selection->ID != -1)
	{
		auto& object = *selection;

		if (const auto poseState = FlexKit::GetPoseState(object.gameObject); poseState && ImGui::Begin("Joint Info", nullptr))
		{
					FlexKit::Skeleton* S		= poseState->Sk;
			static	FlexKit::Skeleton* prev_S	 = nullptr;

			static std::vector<std::string>			strings;
			static std::vector<std::string_view>	labels;

			if (S != prev_S)
			{
				strings.clear();
				labels.clear();
				prev_S = S;

				for (size_t I = 0; I < poseState->JointCount; I++)
				{
					const auto& joint = S->Joints[I];

					if (joint.mID)
						labels.push_back(joint.mID);
					else
					{
						strings.push_back(fmt::format("Joint {}", I));
						labels.push_back(strings.back());
					}
				}
			}


			static const char*	selectedStr = nullptr;
			static size_t		selected = 0;

			if (ImGui::BeginCombo("Joint ", labels[selected].data()))
			{
				for (size_t I = 0; I < poseState->JointCount; I++)
				{
					const auto& joint = S->Joints[I];

					if (ImGui::Selectable(labels[I].data(), I == selected))
					{
						selected = I;
					}

					if (I == selected)
						ImGui::SetItemDefaultFocus();

				}

				ImGui::EndCombo();
			}

			const auto	jointState			= poseState->Joints[selected];
			std::string jointID				= fmt::format("Joint Handle: {}", selected);
			std::string jointOrientation	= fmt::format("Orientation: [{}, {}, {}, {}]",	jointState.r.x, jointState.r.y, jointState.r.z, jointState.r.w);
			std::string jointTranslation	= fmt::format("Position: [{}, {}, {}]",			jointState.ts.x, jointState.ts.y, jointState.ts.z);

			ImGui::Text(jointID.c_str());
			ImGui::Text(jointOrientation.c_str());
			ImGui::Text(jointTranslation.c_str());
			ImGui::End();
		}
	}

	ImGui::EndFrame();
	ImGui::Render();

	renderer.hud.DrawImGui(dT, dispatcher, frameGraph, temporaryBuffers.ReserveVertexBuffer, temporaryBuffers.ReserveConstantBuffer, renderTarget);
	FlexKit::PresentBackBuffer(frameGraph, renderTarget);
}


/************************************************************************************************/


void EditorPrefabPreview::RenderOverlays(
	FlexKit::UpdateDispatcher&		dispatcher,
	FlexKit::FrameGraph&			frameGraph,
	double							dT,
	TemporaryBuffers&				temporaryBuffers,
	FlexKit::ResourceHandle			renderTarget,
	FlexKit::ThreadSafeAllocator&	allocator)
{
	auto& object			= selection->gameObject;
	const auto node			= FlexKit::GetSceneNode(object);
	const auto constants	= GetCameraConstants(previewCamera);
	const auto PV			= constants.PV;
	const auto Q			= FlexKit::GetOrientation(object);
	const auto WT			= FlexKit::GetWT(object);
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
	return;

	auto& gameObject	= selection->gameObject;
	auto meshes			= FlexKit::GetTriMesh(gameObject);

	if (meshes.empty())
		return;

	auto aabb				= FlexKit::GetAABBFromMesh(gameObject);
	const FlexKit::Camera c = FlexKit::CameraComponent::GetComponent().GetCamera(previewCamera);

	const auto target			= aabb.MidPoint();
	const auto desiredDistance	= 2.5f * aabb.Span().magnitude() / std::tan(c.FOV);

	auto position_VS		= c.View	* float4 { target, 1 };
	auto updatedPosition_WS	= c.IV		* float4 { position_VS.x, position_VS.y, position_VS.z + desiredDistance, 1 };

	const auto node		= FlexKit::GetCameraNode(previewCamera);
	const Quaternion Q	= GetOrientation(node);
	auto forward		= (Q * float3{ 0.0f, 0.0f, -1.0f}).normal();

	FlexKit::SetPositionW(node, updatedPosition_WS.xyz());
	FlexKit::MarkCameraDirty(previewCamera);
}


/************************************************************************************************/


void EditorPrefabPreview::mousePressEvent(QMouseEvent* event)
{
	const auto screen = QGuiApplication::screenAt(event->pos());
	if (!screen)
		return;

	const auto pos		= event->localPos();
	const auto x		= pos.x();
	const auto y		= pos.y();
	const auto WH		= renderWindow->WH();
	const auto ratio	= screen->devicePixelRatio();

	FlexKit::Event mouseEvent;
	mouseEvent.InputSource		= FlexKit::Event::Mouse;
	mouseEvent.Action			= FlexKit::Event::Pressed;
	mouseEvent.mType			= FlexKit::Event::Input;
	mouseEvent.mData1.mKC[0]	= FlexKit::KC_MOUSELEFT;

	renderer.hud.HandleInput(mouseEvent);
}


/************************************************************************************************/


void EditorPrefabPreview::mouseReleaseEvent(QMouseEvent* event)
{
	const auto screen = QGuiApplication::screenAt(event->pos());
	if (!screen)
		return;

	const auto pos		= event->localPos();
	const auto x		= pos.x();
	const auto y		= pos.y();
	const auto WH		= renderWindow->WH();
	const auto ratio	= screen->devicePixelRatio();

	FlexKit::Event mouseEvent;
	mouseEvent.InputSource		= FlexKit::Event::Mouse;
	mouseEvent.Action			= FlexKit::Event::Release;
	mouseEvent.mType			= FlexKit::Event::Input;
	mouseEvent.mData1.mKC[0]	= FlexKit::KC_MOUSELEFT;

	renderer.hud.HandleInput(mouseEvent);
}



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
