#include "PCH.h"
#include "AnimationComponents.h"
#include "AnimationObject.h"
#include "DXRenderWindow.h"
#include "EditorAnimationEditor.h"
#include "EditorAnimatorComponent.h"
#include "EditorCodeEditor.h"
#include "EditorRenderer.h"
#include "EditorResourcePickerDialog.h"
#include "EditorScriptObject.h"
#include "EditorScriptEngine.h"
#include "SelectionContext.h"
#include "ResourceIDs.h"
#include "ScriptedAnimationObject.h"


/************************************************************************************************/


ID3D12PipelineState* CreateFlatSkinnedPassPSO(RenderSystem* RS)
{
	auto DrawRectVShader = RS->LoadShader("ForwardSkinned_VS",  "vs_6_0", "assets\\shaders\\forwardRender.hlsl");
	auto DrawRectPShader = RS->LoadShader("FlatWhite",          "ps_6_0", "assets\\shaders\\forwardRender.hlsl");

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
	D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
	Depth_Desc.DepthEnable	= false;

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
        objects.emplace_back(std::make_unique<AnimationObject>());
        selectedObject = objects.size() - 1;
    }

    void CreateAnimator()
    {
    }

    AnimationObject* GetSelection()
    {
        if (objects.size() && selectedObject != -1)
            return objects[selectedObject].get();

        return nullptr;
    }

    std::vector<std::unique_ptr<AnimationObject>>   objects;
    int                                             selectedObject = -1;
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

    void resizeEvent(QResizeEvent* evt) override
    {
        QWidget::resizeEvent(evt);

        auto size = evt->size();

        renderWindow->resize(size.width(), size.height());
        depthBuffer.Resize({ size.width(), size.height() });
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


        struct AnimationUpdate
        {
        };

        auto& scriptedAnimationUpdate = dispatcher.Add<AnimationUpdate>(
            [&](auto&, auto&)
            {
            },
            [dT = dT, this](auto&, iAllocator& temporaryAllocator)
            {
                AnimationObject* mainObj = selection->GetSelection();

                if(mainObj->script)
                    mainObj->script->Update(mainObj);
            });

        cameras.AddInput(transforms);
        animationUpdate.AddOutput(scriptedAnimationUpdate);

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
                builder.AddDataDependency(scriptedAnimationUpdate);
            },
            [=, &gameObject](Pass& data, const FlexKit::ResourceHandler& frameResources, FlexKit::Context& ctx, FlexKit::iAllocator& allocator)
            {
                using namespace FlexKit;
                auto brush = GetBrush(gameObject);

                if (!brush)
                    return;

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
                    float4x4 transforms[64];

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

        if (selection->selectedObject != -1)
        {
            auto& object = selection->objects[selection->selectedObject];

            RenderAnimatedModel(object->gameObject, dispatcher, dT, temporaryBuffers, frameGraph, renderTarget, allocator);
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

            const auto target           = float3(0, 0, 0);
            const auto desiredDistance  = (2.0f / std::sqrt(2.0f)) * boundingSphere.w / std::tan(c.FOV);

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
{
    static auto _REGISTERED = ScriptedAnimationObject::RegisterInterface(scriptEngine);

    ui.setupUi(this);
    ui.horizontalLayout->setMenuBar(menubar);
    ui.horizontalLayout->setContentsMargins(0, 0, 0, 0);
    ui.verticalLayout->setContentsMargins(0, 0, 0, 0);
    ui.verticalLayout->addWidget(codeEditor);
    ui.editorSection->setContentsMargins(0, 0, 0, 0);

    auto codeEditorMenu = codeEditor->GetMenuBar();
    auto animationMenu  = codeEditorMenu->addMenu("Animator");
    auto reloadObject = animationMenu->addAction("Reload Object");

    connect(reloadObject, &QAction::triggered,
        [&]()
        {
            codeEditor->SaveDocument();

            if(auto obj = localSelection->GetSelection(); obj && obj->script)
                obj->script->Reload(scriptEngine, obj);
        });

    auto layout = new QVBoxLayout{};
    ui.animationPreview->setLayout(layout);
    ui.animationPreview->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(previewWindow);

    auto fileMenu     = menubar->addMenu("Create");
    auto createObject = fileMenu->addAction("Object");

    auto createScript = fileMenu->addAction("AnimatorScript");
    connect(
        createScript, &QAction::triggered,
        [&]()
        {
            if(auto obj = localSelection->GetSelection(); obj)
            {
                auto scriptResource = std::make_shared<ScriptResource>();
                auto scriptObject   = new ScriptedAnimationObject{};
                auto context        = scriptEngine.BuildModule(scriptResource->source);

                scriptObject->script = scriptResource;

                obj->script = scriptObject;

                project.AddResource(scriptResource);
                codeEditor->SetResource(scriptResource);
                localSelection->GetSelection()->script = scriptObject;
            }
        });

    createScript->setEnabled(false);

    connect(
        createObject, &QAction::triggered,
        [&, createScript = createScript]()
        {
            localSelection->CreateObject();
            auto obj                    = localSelection->GetSelection();
            globalSelection.type        = AnimatorObject_ID;
            globalSelection.selection   = std::any{ obj };

            createScript->setEnabled(true);
        });

    auto createAnimator = fileMenu->addAction("Animator");
    createAnimator->setEnabled(false);

    connect(
        createAnimator, &QAction::triggered,
        [&]()
        {
            localSelection->CreateAnimator();
        });

    auto objectMenu = menubar->addMenu("Object");
    auto loadObject = objectMenu->addAction("Load");

    connect(
        loadObject, &QAction::triggered,
        [&]()
        {
            if (auto selection = localSelection->GetSelection(); selection && !selection->gameObject.hasView(FlexKit::BrushComponentID))
            {
                auto meshPicker = new EditorResourcePickerDialog(MeshResourceTypeID, IN_project, this);
                meshPicker->OnSelection(
                    [&](ProjectResource_ptr resource)
                    {
                        auto skeletonPicker = new EditorResourcePickerDialog(SkeletonResourceTypeID, IN_project, this);

                        skeletonPicker->OnSelection(
                            [=, &selection](ProjectResource_ptr skeleton)
                            {
                                auto meshResource   = std::static_pointer_cast<FlexKit::MeshResource>(resource->resource);
                                auto mesh           = renderer.LoadMesh(*meshResource);
                                auto selection      = localSelection->GetSelection();
                                auto& gameObject    = selection->gameObject;

                                gameObject.AddView<FlexKit::BrushView>(mesh);

                                auto blob       = skeleton->resource->CreateBlob();
                                auto buffer     = blob.buffer;
                                blob.buffer     = 0;
                                blob.bufferSize = 0;

                                FlexKit::AddAssetBuffer((FlexKit::Resource*)buffer);

                                gameObject.AddView<FlexKit::SkeletonView>(mesh, skeleton->resource->GetResourceGUID());
                                gameObject.AddView<FlexKit::AnimatorView>();
                            });

                        skeletonPicker->show();
                    });

                meshPicker->show();
            }
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
