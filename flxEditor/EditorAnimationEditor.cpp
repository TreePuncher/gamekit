#include "AnimationComponents.h"
#include "AnimationObject.h"
#include "DXRenderWindow.h"
#include "EditorAnimationEditor.h"
#include "EditorAnimatorComponent.h"
#include "EditorCodeEditor.h"
#include "EditorRenderer.h"
#include "EditorResourcePickerDialog.h"
#include "qboxlayout.h"
#include "qevent.h"
#include "qmenubar.h"
#include "SelectionContext.h"
#include "ResourceIDs.h"


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
        if(objects.size() && selectedObject != -1)
            return objects[selectedObject].get();
    }

    std::vector<std::unique_ptr<AnimationObject>>   objects;
    int                                             selectedObject = -1;
};


/************************************************************************************************/


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

    void RenderAnimationPreview(FlexKit::UpdateDispatcher& Dispatcher, double dT, TemporaryBuffers&, FlexKit::FrameGraph& graph, FlexKit::ResourceHandle renderTarget, FlexKit::ThreadSafeAllocator& allocator)
    {
        FlexKit::ClearBackBuffer(graph, renderTarget, FlexKit::float4{ 120.0f / 225.0f, 130.0f / 255.0f, 164.0f / 255.0f, 1.0f });

        if (selection->selectedObject != -1)
        {
            auto& object = selection->objects[selection->selectedObject];
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
{
    ui.setupUi(this);
    ui.horizontalLayout->setMenuBar(menubar);
    ui.horizontalLayout->setContentsMargins(0, 0, 0, 0);
    ui.verticalLayout->setContentsMargins(0, 0, 0, 0);
    ui.verticalLayout->addWidget(codeEditor);
    ui.editorSection->setContentsMargins(0, 0, 0, 0);

    auto layout = new QVBoxLayout{};
    ui.animationPreview->setLayout(layout);
    ui.animationPreview->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(previewWindow);

    auto fileMenu     = menubar->addMenu("Create");
    auto createObject = fileMenu->addAction("Object");

    connect(
        createObject, &QAction::triggered,
        [&]()
        {
            localSelection->CreateObject();
            auto obj                    = localSelection->GetSelection();
            globalSelection.type        = AnimatorObject_ID;
            globalSelection.selection   = std::any{ obj };
        });

    auto createAnimator = fileMenu->addAction("Animator");
    createAnimator->setEnabled(false);

    connect(
        createAnimator, &QAction::triggered,
        [&]()
        {
            localSelection->CreateAnimator();
        });

    auto createScript = fileMenu->addAction("AnimatorScript");
    createScript->setEnabled(false);

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
                        auto meshResource   = std::static_pointer_cast<FlexKit::MeshResource>(resource->resource);
                        auto mesh           = renderer.LoadMesh(*meshResource);
                        auto selection      = localSelection->GetSelection();

                        selection->gameObject.AddView<FlexKit::BrushView>(mesh);

                        auto blob       = meshResource->Skeleton->CreateBlob();
                        auto buffer     = blob.buffer;
                        blob.buffer     = 0;
                        blob.bufferSize = 0;

                        FlexKit::AddAssetBuffer((FlexKit::Resource*)buffer);

                        selection->gameObject.AddView<FlexKit::SkeletonView>(mesh, meshResource->Skeleton->GetID());
                    });

                meshPicker->show();
            }
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
