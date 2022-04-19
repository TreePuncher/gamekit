#include "PCH.h"
#include "AnimationComponents.h"
#include "AnimationEditorObject.h"
#include "DXRenderWindow.h"
#include "EditorAnimationEditor.h"
#include "EditorAnimatorComponent.h"
#include "EditorCodeEditor.h"
#include "EditorPrefabObject.h"
#include "EditorRenderer.h"
#include "EditorResourcePickerDialog.h"
#include "EditorScriptEngine.h"
#include "SelectionContext.h"
#include "ResourceIDs.h"
#include "EditorAnimationInputTab.h"
#include "EditorAnimatorComponent.h"

#include <angelscript.h>

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


class AnimatorSelectionContext
{
public:

    void CreateObject()
    {
    }

    void CreateAnimator()
    {
    }

    AnimationEditorObject* GetSelection()
    {
        return &object;
    }

    void Clear()
    {

    }

    AnimationEditorObject object;
};


/************************************************************************************************/


constexpr FlexKit::PSOHandle FLATSKINNED_PSO = FlexKit::PSOHandle(GetTypeGUID(FlatSkinned));


class EditorAnimationPreview : public QWidget
{
public:
    EditorAnimationPreview(EditorRenderer& IN_renderer, AnimatorSelectionContext* IN_selection, QWidget* parent = nullptr)
        : QWidget       { parent }
        , renderer      { IN_renderer }
        , renderWindow  { IN_renderer.CreateRenderWindow(this) }
        , depthBuffer   { IN_renderer.GetRenderSystem(), renderWindow->WH() }
        , selection     { IN_selection }
        , previewCamera { FlexKit::CameraComponent::GetComponent().CreateCamera() }
    {
        FlexKit::SetCameraNode(previewCamera, FlexKit::GetZeroedNode());

        auto& renderSystem = renderer.GetRenderSystem();
        renderSystem.RegisterPSOLoader(FLATSKINNED_PSO, { &renderSystem.Library.RS6CBVs4SRVs, &CreateFlatSkinnedPassPSO });

        renderWindow->SetOnDraw(
            [&](FlexKit::UpdateDispatcher& dispatcher, double dT, TemporaryBuffers& temporaries, FlexKit::FrameGraph& frameGraph, FlexKit::ResourceHandle renderTarget, FlexKit::ThreadSafeAllocator& allocator)
            {
                if (isVisible())
                    RenderAnimationPreview(dispatcher, dT, temporaries, frameGraph, renderTarget, allocator);
            });
    }

    struct LoadEntityContext : public LoadEntityContextInterface
    {
        LoadEntityContext(
            std::vector<FlexKit::NodeHandle>&   IN_nodes,
            EditorScene_ptr                     IN_viewportscene,
            EditorViewport&                     IN_viewport,
            FlexKit::Scene&                     IN_scene,
            FlexKit::GameObject&                IN_gameObject,
            FlexKit::MaterialHandle             IN_defaultMaterial)
            : nodes             { IN_nodes              }
            , viewportscene     { IN_viewportscene      }
            , viewport          { IN_viewport           }
            , scene             { IN_scene              }
            , gameObject        { IN_gameObject         }
            , defaultMaterial   { IN_defaultMaterial    } {}


        std::vector<FlexKit::NodeHandle>&   nodes;
        EditorScene_ptr                     viewportscene;
        EditorViewport&                     viewport;
        FlexKit::Scene&                     scene;
        FlexKit::GameObject&                gameObject;
        FlexKit::MaterialHandle             defaultMaterial;

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
            return FlexKit::InvalidHandle_t;
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


    void resizeEvent(QResizeEvent* evt) override
    {
        QWidget::resizeEvent(evt);

        auto size = evt->size();
        FlexKit::uint2 newWH = { evt->size().width() * 1.5, evt->size().height() * 1.5 };


        renderWindow->resizeEvent(evt);
        depthBuffer.Resize(newWH);

        FlexKit::SetCameraAspectRatio(previewCamera, float(evt->size().width()) / float(evt->size().height()));
    }

    void RenderAnimatedModel(
        FlexKit::GameObject&            gameObject,
        FlexKit::UpdateDispatcher&      dispatcher,
        double                          dT,
        TemporaryBuffers&               temporaryBuffers,
        FlexKit::FrameGraph&            frameGraph,
        FlexKit::ResourceHandle         renderTarget,
        FlexKit::ThreadSafeAllocator&   allocator)
    {
        struct Pass
        {
            FlexKit::ReserveConstantBufferFunction  reserveCB;
            FlexKit::ReserveVertexBufferFunction    reserveVB;

            FlexKit::FrameResourceHandle renderTarget;
            FlexKit::FrameResourceHandle depthTarget;
        };


        auto& animationUpdate   = FlexKit::UpdateAnimations(dispatcher, dT);
        auto& transforms        = FlexKit::QueueTransformUpdateTask(dispatcher);
        auto& cameras           = FlexKit::CameraComponent::GetComponent().QueueCameraUpdate(dispatcher);

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
                builder.AddDataDependency(animationUpdate);
            },
            [=, &gameObject](Pass& data, const FlexKit::ResourceHandler& frameResources, FlexKit::Context& ctx, FlexKit::iAllocator& allocator)
            {
                using namespace FlexKit;
                auto brush = GetBrush(gameObject);

                if (!brush)
                    return;

                FlexKit::Yaw(brush->Node, 1.0f/60.0f);

                auto mesh           = brush->MeshHandle;
                auto materialHndl   = brush->material;
                auto triMesh        = GetMeshResource(mesh);
                auto constants      = brush->GetConstants();

                if (!triMesh)
                    return;

                auto& materials         = MaterialComponent::GetComponent();
                const auto materialData = MaterialComponent::GetComponent()[materialHndl];
                auto skeleton           = FlexKit::GetSkeleton(gameObject);
                auto poseState          = FlexKit::GetPoseState(gameObject);

                auto lodLevel = triMesh->GetHighestLoadedLodIdx();

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
                    float4x4 transforms[512];

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
                ctx.SetPipelineState(frameResources.GetPipelineState(FLATSKINNED_PSO));
                ctx.SetPrimitiveTopology(FlexKit::EIT_TRIANGLELIST);

                ctx.SetScissorAndViewports({ renderTarget });
                ctx.SetRenderTargets(
                    { frameResources.GetResource({ data.renderTarget }) },
                    true, frameResources.GetResource(data.depthTarget));

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

                auto& poses     = allocator.allocate<EntityPoses>();

                FlexKit::UpdatePose(*poseState, allocator);

                for (size_t I = 0; I < poseState->JointCount; I++)
                    poses[I] = skeleton->IPose[I] * poseState->CurrentPose[I];

                ctx.SetGraphicsConstantBufferView(1, cameraConstants);
                ctx.SetGraphicsConstantBufferView(2, ConstantBufferDataSet(constants, entityConstantBuffer));
                ctx.SetGraphicsConstantBufferView(3, passConstants);
                ctx.SetGraphicsConstantBufferView(4, ConstantBufferDataSet(poses, poseBuffer));

                ctx.DrawIndexed(triMesh->GetHighestLoadedLod().GetIndexCount());
            });
    }


    void RenderAnimationPreview(
        FlexKit::UpdateDispatcher&      dispatcher,
        double                          dT,
        TemporaryBuffers&               temporaryBuffers,
        FlexKit::FrameGraph&            frameGraph,
        FlexKit::ResourceHandle         renderTarget,
        FlexKit::ThreadSafeAllocator&   allocator)
    {
        FlexKit::ClearBackBuffer(frameGraph, renderTarget, FlexKit::float4{ 0, 0, 0, 0 });
        FlexKit::ClearDepthBuffer(frameGraph, depthBuffer.Get(), 1.0f);

        if (selection->object.ID != -1)
        {
            auto& object = selection->object;

            RenderAnimatedModel(object.gameObject, dispatcher, dT, temporaryBuffers, frameGraph, renderTarget, allocator);
            
            if (const auto pose = FlexKit::GetPoseState(object.gameObject); pose)
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

        FlexKit::PresentBackBuffer(frameGraph, renderTarget);
    }


    void CenterCamera()
    {
        auto& gameObject = selection->GetSelection()->gameObject;
        auto mesh = FlexKit::GetTriMesh(gameObject);

        if (mesh == FlexKit::InvalidHandle_t)
            return;

        if (auto resource = FlexKit::GetMeshResource(mesh); resource)
        {
            auto boundingSphere     = resource->BS;
            const FlexKit::Camera c = FlexKit::CameraComponent::GetComponent().GetCamera(previewCamera);

            const auto target           = boundingSphere.xyz();
            const auto desiredDistance  = 5 * boundingSphere.w / std::tan(c.FOV);

            auto position_VS        = c.View.Transpose() * float4 { target, 1 };
            auto updatedPosition_WS = c.IV.Transpose() * float4 { position_VS.x, position_VS.y, position_VS.z + desiredDistance, 1 };

            const auto node     = FlexKit::GetCameraNode(previewCamera);
            const Quaternion Q  = GetOrientation(node);
            auto forward        = (Q * float3{ 0.0f, 0.0f, -1.0f}).normal();

            FlexKit::SetPositionW(node, updatedPosition_WS.xyz());
            FlexKit::MarkCameraDirty(previewCamera);
        }
    }


private:
    FlexKit::CameraHandle       previewCamera;
    EditorRenderer&             renderer;
    DXRenderWindow*             renderWindow;
    AnimatorSelectionContext*   selection;
    FlexKit::DepthBuffer        depthBuffer;
};


/************************************************************************************************/


FlexKit::Animation* EditorAnimationEditor::LoadAnimation(std::string& id, bool)
{
    auto resource   = project.FindProjectResource(id)->resource;
    auto blob       = resource->CreateBlob();

    FlexKit::AddAssetBuffer((FlexKit::Resource*)blob.buffer);
    return FlexKit::LoadAnimation(resource->GetResourceGUID(), FlexKit::SystemAllocator);
}


/************************************************************************************************/


void EditorAnimationEditor::ReleaseAnimation(FlexKit::Animation* anim)
{
    FlexKit::SystemAllocator.release(anim);
}


/************************************************************************************************/


EditorAnimationEditor::EditorAnimationEditor(SelectionContext& IN_selection, EditorScriptEngine& IN_engine, EditorRenderer& IN_renderer, EditorProject& IN_project, QWidget* parent)
    : QWidget           { parent }
    , project           { IN_project }
    , previewWindow     { new EditorAnimationPreview{ IN_renderer, localSelection } }
    , menubar           { new QMenuBar{} }
    , localSelection    { new AnimatorSelectionContext{} }
    , codeEditor        { new EditorCodeEditor{ IN_project, IN_engine, this } }
    , globalSelection   { IN_selection }
    , renderer          { IN_renderer }
    , scriptEngine      { IN_engine }
    , inputVariables    { new EditorAnimationInputTab }
{
    static auto _REGISTERED = AnimationEditorObject::RegisterInterface(scriptEngine);

    auto engine     = FlexKit::GetScriptEngine();

    int res;
    res = engine->RegisterGlobalFunction("Animation@ LoadAnimation(string& in)", asMETHOD(EditorAnimationEditor, LoadAnimation), asCALL_THISCALL_ASGLOBAL, this);
    res = engine->RegisterGlobalFunction("void ReleaseAnimation(Animation@)", asMETHOD(EditorAnimationEditor, ReleaseAnimation), asCALL_THISCALL_ASGLOBAL, this);

    codeEditor->GetTabs()->addTab(inputVariables, "Input Variables");

    ui.setupUi(this);
    ui.horizontalLayout->setMenuBar(menubar);
    ui.horizontalLayout->setContentsMargins(0, 0, 0, 0);
    ui.verticalLayout->setContentsMargins(0, 0, 0, 0);
    ui.verticalLayout->addWidget(codeEditor);
    ui.editorSection->setContentsMargins(0, 0, 0, 0);

    auto codeEditorMenu = codeEditor->GetMenuBar();
    auto animationMenu  = codeEditorMenu->addMenu("Animator");
    auto reloadObject   = animationMenu->addAction("Reload Object");

    connect(reloadObject, &QAction::triggered,
        [&]()
        {
            codeEditor->SaveDocument();

            if(auto obj = localSelection->GetSelection(); obj && obj->resource)
                obj->Reload(scriptEngine);
        });

    auto layout = new QVBoxLayout{};
    ui.animationPreview->setLayout(layout);
    ui.animationPreview->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(previewWindow);

    auto fileMenu       = menubar->addMenu("Object");
    auto createObject   = fileMenu->addAction("Create Animated Prefab");

    connect(
        createObject, &QAction::triggered,
        [&]
        {
            auto meshPicker = new EditorResourcePickerDialog(MeshResourceTypeID, IN_project, this);

            meshPicker->OnSelection(
                [&](ProjectResource_ptr resource)
                {
                    auto skeletonPicker = new EditorResourcePickerDialog(SkeletonResourceTypeID, IN_project, this);

                    skeletonPicker->OnSelection(
                        [&, resource](ProjectResource_ptr skeleton)
                        {
                            localSelection->CreateObject();
                            auto obj            = localSelection->GetSelection();

                            // Load Tri Mesh
                            auto meshResource   = std::static_pointer_cast<FlexKit::MeshResource>(resource->resource);
                            auto mesh           = renderer.LoadMesh(*meshResource);
                            auto& gameObject    = obj->gameObject;

                            // Load Skeleton
                            auto blob           = skeleton->resource->CreateBlob();
                            auto buffer         = blob.buffer;
                            blob.buffer         = 0;
                            blob.bufferSize     = 0;

                            FlexKit::AddAssetBuffer((FlexKit::Resource*)buffer);

                            // Create Script Object
                            auto scriptResource = std::make_shared<ScriptResource>();
                            auto context        = scriptEngine.BuildModule(scriptResource->source);

                            project.AddResource(scriptResource);
                            codeEditor->SetResource(scriptResource);

                            // Build Editor object
                            gameObject.AddView<FlexKit::SceneNodeView<>>();
                            gameObject.AddView<FlexKit::BrushView>(mesh);
                            gameObject.AddView<FlexKit::SkeletonView>(mesh, skeleton->resource->GetResourceGUID());
                            gameObject.AddView<FlexKit::AnimatorView>();

                            // serialize object
                            PrefabGameObjectResource_ptr objectResource = std::make_shared<PrefabGameObjectResource>();
                            project.AddResource(objectResource);

                            auto brushComponent     = std::make_shared<FlexKit::EntityBrushComponent>();
                            auto skeletonComponent  = std::make_shared<FlexKit::EntitySkeletonComponent>();
                            auto animatorComponent  = std::make_shared<AnimatorComponent>();

                            brushComponent->MeshGuid                = meshResource->GetResourceGUID();
                            skeletonComponent->skeletonResourceID   = skeleton->resource->GetResourceGUID();
                            animatorComponent->scriptResource       = scriptResource->GetResourceGUID();

                            objectResource->components.push_back(brushComponent);
                            objectResource->components.push_back(skeletonComponent);
                            objectResource->components.push_back(animatorComponent);

                            obj->resource                   = scriptResource;
                            obj->resourceID                 = objectResource->GetResourceGUID();
                            localSelection->object.ID       = obj->resourceID;
                            localSelection->object.animator = animatorComponent.get();

                            globalSelection.Clear();
                            globalSelection.type        = AnimatorObject_ID;
                            globalSelection.selection   = std::any{ obj };

                            previewWindow->CenterCamera();
                        });

                    skeletonPicker->show();
                });

            meshPicker->show();
        });

    auto loadObject = fileMenu->addAction("Load");
    connect(
        loadObject, &QAction::triggered,
        [&]
        {
            auto resourcePicker = new EditorResourcePickerDialog(FlexKit::EResource_Prefab, IN_project, this);
            resourcePicker->OnSelection(
                [&](ProjectResource_ptr projectObj)
                {
                    localSelection->CreateObject();
                    auto obj = localSelection->GetSelection();
                    obj->Release();

                    auto prefabObjectRes    = std::static_pointer_cast<PrefabGameObjectResource>(projectObj->resource);

                    struct Context : public LoadEntityContextInterface
                    {
                        Context(FlexKit::GameObject& IN_obj, EditorRenderer& IN_renderer, EditorProject& IN_project)
                            : gameObject    { IN_obj        }
                            , renderer      { IN_renderer   }
                            , project       { IN_project    } {}

                        FlexKit::GameObject&    gameObject;
                        EditorRenderer&         renderer;
                        EditorProject&          project;

                        FlexKit::GameObject&    GameObject() override { return gameObject; }
                        FlexKit::NodeHandle     GetNode(uint32_t idx) { return FlexKit::GetZeroedNode(); }

                        ProjectResource_ptr     FindSceneResource(uint64_t assetID) { return project.FindProjectResource(assetID); }
                        FlexKit::TriMeshHandle  LoadTriMeshResource (ProjectResource_ptr resource)
                        {
                            auto meshResource = std::static_pointer_cast<FlexKit::MeshResource>(resource->resource);
                            return renderer.LoadMesh(*meshResource);
                        }

                        FlexKit::MaterialHandle DefaultMaterial() const { return FlexKit::InvalidHandle_t; }
                        FlexKit::Scene*         Scene()                 { return nullptr; }
                    } context{ obj->gameObject, renderer, project };

                    auto loadRes =
                        [&](auto& resourceID)
                        {
                            auto sceneRes   = context.FindSceneResource(resourceID);

                            auto blob       = sceneRes->resource->CreateBlob();
                            auto buffer     = blob.buffer;
                            blob.buffer     = nullptr;
                            blob.bufferSize = 0;
                            FlexKit::AddAssetBuffer((Resource*)buffer);

                            return sceneRes;
                        };


                    auto& components = prefabObjectRes->components;
                    auto FindComponent =
                        [&](uint32_t componentID) -> std::optional<FlexKit::EntityComponent_ptr>
                        {
                            auto res = std::find_if(
                                components.begin(), components.end(),
                                [&](FlexKit::EntityComponent_ptr& component)
                                {
                                    return component->GetTypeID() == componentID;
                                });

                            if (res != components.end())
                                return { *res };
                            else
                                return {};
                        };

                    if (auto res = FindComponent(FlexKit::AnimatorComponentID); res)
                    {
                        auto animator = std::static_pointer_cast<AnimatorComponent>(*res);
                        obj->animator = animator.get();

                        auto resource   = loadRes(animator->scriptResource);
                        auto scriptRes  = std::static_pointer_cast<ScriptResource>(resource->resource);
                        obj->resource   = scriptRes;

                        codeEditor->SetResource(scriptRes);
                    }

                    if (auto res = FindComponent(GetTypeGUID(EntitySkeletonComponent)); res)
                    {
                        auto sk = std::static_pointer_cast<FlexKit::EntitySkeletonComponent>(*res);

                        loadRes(sk->skeletonResourceID);
                    }

                    LoadEntity(prefabObjectRes->components, context);

                    obj->ID = prefabObjectRes->GetResourceGUID();

                    globalSelection.Clear();
                    globalSelection.type        = AnimatorObject_ID;
                    globalSelection.selection   = std::any{ obj };

                    previewWindow->CenterCamera();
                });

            resourcePicker->show();
        });

    auto createAnimator = fileMenu->addAction("Animator");
    createAnimator->setEnabled(false);

    connect(
        createAnimator, &QAction::triggered,
        [&]()
        {
            localSelection->CreateAnimator();
        });


    auto viewMenu   = menubar->addMenu("View");
    auto centerView = viewMenu->addAction("Center View On Object");

    connect(
        centerView, &QAction::triggered,
        [&]()
        {
            if(localSelection->GetSelection())
                previewWindow->CenterCamera();
        });

    inputVariables->SetOnCreateEvent(
        [&](size_t typeID, const std::string& ID)
        {
            if (auto selection = localSelection->GetSelection(); selection)
            {
                auto& gameObject = selection->gameObject;
                selection->AddInputValue(ID, typeID);
            }
        });

    inputVariables->SetOnChangeEvent(
        [&](size_t idx, const std::string& ID, const std::string& value, const std::string& defaultValue)
        {
            if (auto selection = localSelection->GetSelection(); selection)
            {
                auto& inputs        = selection->animator->inputs;
                auto& valueEntry    = inputs[idx];
                valueEntry.stringID = ID;

                selection->UpdateValue(idx, value);
                selection->UpdateDefaultValue(idx, defaultValue);
            }
        });

    timer = new QTimer{ this };
    connect(timer, &QTimer::timeout,
        [&]()
        {
            if (auto selection = localSelection->GetSelection(); selection && selection->ID != -1)
            {
                if (auto inputs = selection->animator->inputs.size(); inputs)
                {
                    inputVariables->Update(
                        inputs,
                        [&](size_t idx, std::string& ID, std::string& value, std::string& defaultValue)
                        {
                            auto& inputID   = selection->animator->inputs[idx];
                            ID              = inputID.stringID;
                            value           = selection->ValueString(idx, (uint32_t)inputID.type);
                            defaultValue    = selection->DefaultValueString(idx);
                        });
                }
            }

            timer->start(500ms);
        });

    timer->setTimerType(Qt::PreciseTimer);
    timer->start(500ms);
}


/************************************************************************************************/


EditorAnimationEditor::~EditorAnimationEditor()
{
    delete localSelection;
}


/**********************************************************************

Copyright (c) 2021 - 2022 Robert May

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
