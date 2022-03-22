#include "EditorAnimationEditor.h"
#include "DXRenderWindow.h"
#include "EditorRenderer.h"
#include "qboxlayout.h"
#include "qevent.h"
#include "EditorRenderer.h"
#include "qmenubar.h"
#include "EditorAnimatorComponent.h"
#include "AnimationComponents.h"
#include "EditorResourcePickerDialog.h"
#include "..\FlexKitResourceCompiler\ResourceIDs.h"
#include "EditorCodeEditor.h"


/************************************************************************************************/


class AnimatorSelectionContext
{
public:

    struct AnimationObject
    {
        FlexKit::GameObject gameObject;
    };


    void CreateObject()
    {
        objects.emplace_back(std::make_unique<AnimationObject>());
        selectedObject = objects.size() - 1;
    }


    void CreateAnimator()
    {

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


EditorAnimationEditor::EditorAnimationEditor(EditorScriptEngine& IN_engine, EditorRenderer& IN_renderer, EditorProject& IN_project, QWidget* parent)
    : QWidget       { parent }
    , project       { IN_project }
    , previewWindow { new EditorAnimationPreview{ IN_renderer, selection } }
    , menubar       { new QMenuBar{} }
    , selection     { new AnimatorSelectionContext{} }
    , codeEditor    { new EditorCodeEditor{ IN_engine, this } }
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

    auto fileMenu           = menubar->addMenu("Create");
    auto createObject       = fileMenu->addAction("Object");

    connect(
        createObject, &QAction::triggered,
        [&]()
        {
            selection->CreateObject();
        });

    auto createAnimator = fileMenu->addAction("Animator");
    createAnimator->setEnabled(false);

    connect(
        createAnimator, &QAction::triggered,
        [&]()
        {
            selection->CreateAnimator();
        });

    auto createScript = fileMenu->addAction("AnimatorScript");
    createScript->setEnabled(false);

    auto objectMenu         = menubar->addMenu("Object");
    auto loadObject         = objectMenu->addAction("Load");

    connect(
        loadObject, &QAction::triggered,
        [&]()
        {
            auto meshPicker = new EditorResourcePickerDialog(MeshResourceTypeID, IN_project, this);
            meshPicker->OnSelection(
                [&](auto resource)
                {
                    auto skeletonPicker = new EditorResourcePickerDialog(SkeletonResourceTypeID, IN_project, this);

                    meshPicker->OnSelection(
                        [&](auto resource)
                        {
                        });
                });
        });
}


/************************************************************************************************/


EditorAnimationEditor::~EditorAnimationEditor()
{
    delete selection;
}


/************************************************************************************************/

