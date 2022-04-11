#include "PCH.h"
#include "EditorApplication.h"
#include "TextureUtilities.h"
#include "angelscript.h"
#include "TextureResourceUtilities.h"
#include <stb_image_write.h>
#include <filesystem>
#include <QKeySequence>
#include "EditorColliderComponent.h"

using namespace std::chrono_literals;


/************************************************************************************************/


bool gltfImporter::Import(const std::string fileDir) 
{
    if (!std::filesystem::exists(fileDir))
        return false;

    FlexKit::MetaDataList meta;
    auto resources = FlexKit::CreateSceneFromGlTF(fileDir, meta);

    for (auto& resource : resources)
    {
        if (resource == nullptr)
            continue;

        if (SceneResourceTypeID == resource->GetResourceTypeID())
        {
            EditorScene_ptr gameScene = std::make_shared<EditorScene>();

            gameScene->sceneResource = std::static_pointer_cast<FlexKit::SceneResource>(resource);

            for (auto& dependentResource : resources)
                if (dependentResource != resource)
                    gameScene->sceneResources.push_back(std::make_shared<ProjectResource>(dependentResource));


            project.AddScene(gameScene);
            project.AddResource(resource);
        }
        else
            project.AddResource(resource);
    }

    return true;
}


/************************************************************************************************/



EditorApplication::EditorApplication(QApplication& IN_qtApp) :
	qtApp               { IN_qtApp },
	editorRenderer      { fkApplication.PushState<EditorRenderer>(fkApplication, IN_qtApp) },
	mainWindow          { editorRenderer, scripts, project, qtApp },
	gltfImporter        { project },
    gameResExporter     { project },
	projectConnector    { &mainWindow.Get3DView(), mainWindow.GetSelectionCtx(), project }
{
    qApp->setStyle(QStyleFactory::create("fusion"));

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(53, 53, 53));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor(15, 15, 15));
    palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor(53, 53, 53));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::BrightText, Qt::red);

    palette.setColor(QPalette::Highlight, QColor(142, 45, 197).lighter());
    palette.setColor(QPalette::HighlightedText, Qt::black);

    palette.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);

    qApp->setPalette(palette);


	mainWindow.AddImporter(&gltfImporter);
    mainWindow.AddExporter(&gameResExporter);

	projectConnector.Register(scripts);
	scripts.LoadModules();

    mainWindow.AddFileAction(
        "Save",
        [&]()
        {
            if(auto& scene = mainWindow.Get3DView().GetScene(); scene)
                scene->Update();

            const auto saveText     = std::string{ "Save Project" };
            const auto fileMenuText = std::string{ "Files (*.proj)" };
            const auto fileDir      = QFileDialog::getSaveFileName(nullptr, saveText.c_str(), QDir::currentPath(), fileMenuText.c_str());
            const auto fileStr      = fileDir.toStdString();

            project.SaveProject(fileStr);
        });

    mainWindow.AddFileAction(
        "Load",
        [&]()
        {
            const auto loadText     = std::string{ "Load Project" };
            const auto fileMenuText = std::string{ "Files (*.proj)" };
            const auto fileDir      = QFileDialog::getOpenFileName(nullptr, loadText.c_str(), QDir::currentPath(), fileMenuText.c_str());
            const auto fileStr      = fileDir.toStdString();

            project.LoadProject(fileStr);
        });

    auto quitAction = mainWindow.GetFileMenu()->addAction("Quit");
    quitAction->setShortcut(QKeySequence::Quit);
    
    mainWindow.connect(quitAction, &QAction::triggered, &mainWindow, &EditorMainWindow::Close, Qt::QueuedConnection);

    ResourceBrowserWidget::AddResourceViewer(std::make_shared<TextureResourceViewer>(editorRenderer, (QWidget*)&mainWindow));
    ResourceBrowserWidget::AddResourceViewer(std::make_shared<SceneResourceViewer>(editorRenderer, project, mainWindow));

	// connect script gadgets
	for (auto& gadget : scripts.GetGadgets())
		mainWindow.RegisterGadget(&gadget);

    EditorInspectorView::AddComponentInspector<TransformInspector>();
    EditorInspectorView::AddComponentInspector<VisibilityInspector>();
    EditorInspectorView::AddComponentInspector<PointLightInspector>();
    EditorInspectorView::AddComponentInspector<PointLightShadowInspector>();
    EditorInspectorView::AddComponentInspector<SceneBrushInspector>(project, mainWindow.Get3DView());

    RegisterCSGInspector(mainWindow.Get3DView());
    RegisterColliderInspector(mainWindow.Get3DView(), project);
}


/************************************************************************************************/


EditorApplication::~EditorApplication()
{
    fkApplication.Release();
}


/************************************************************************************************/


void SceneReference::Register(asIScriptEngine* engine)
{
    int c = engine->RegisterObjectType("Scene", 0, asOBJ_REF);                   assert(c >= 0);

    // Life Cycle behaviors
    //c = engine->RegisterObjectBehaviour("Scene@", asBEHAVE_FACTORY,   "void f()", asFunctionPtr(SceneReference::Factory),   asCALL_CDECL_OBJLAST);    assert(c >= 0);

    c = engine->RegisterObjectBehaviour("Scene", asBEHAVE_ADDREF,     "void f()", asFunctionPtr(SceneReference::AddRef),    asCALL_CDECL_OBJLAST);    assert(c >= 0);
    c = engine->RegisterObjectBehaviour("Scene", asBEHAVE_RELEASE,    "void f()", asFunctionPtr(SceneReference::Release),   asCALL_CDECL_OBJLAST);    assert(c >= 0);

    // Methods
    c = engine->RegisterObjectMethod("Scene", "bool RayCast(float, float, float)",    asFunctionPtr(SceneReference::RayCast),     asCALL_CDECL_OBJLAST); assert(c >= 0);
    c = engine->RegisterObjectMethod("Scene", "bool IsValid()",                       asFunctionPtr(SceneReference::IsValid),     asCALL_CDECL_OBJLAST); assert(c >= 0);
}

SceneReference* SceneReference::Factory()
{
    return new SceneReference{};
}

void SceneReference::AddRef(SceneReference* ref)
{
    ref->ref_count++;
}

void SceneReference::Release(SceneReference* ref)
{
    ref->ref_count--;

    if (!ref->ref_count)
        delete ref;
}

bool SceneReference::IsValid(SceneReference rhs)
{
    return rhs.scene != nullptr;
}

bool SceneReference::RayCast(float x, float y, float z, SceneReference scene)
{
    return false;
}


/************************************************************************************************/



EditorProjectScriptConnector::EditorProjectScriptConnector(EditorViewport* IN_viewport, SelectionContext& IN_ctx, EditorProject& IN_project) :
    ctx         { IN_ctx },
	project     { IN_project },
    viewport    { IN_viewport }
{
}

void CreateTextureBufferDefault(void* _ptr)
{
    new(_ptr) FlexKit::TextureBuffer({ 4, 4 }, 4, FlexKit::SystemAllocator);
}

void CreateTextureBuffer(uint32_t width, uint32_t height, uint32_t format, void* _ptr)
{
	new(_ptr) FlexKit::TextureBuffer({width, height}, format, FlexKit::SystemAllocator);
}

void ReleaseTextureBuffer(void* _ptr)
{
	((FlexKit::TextureBuffer*)_ptr)->FlexKit::TextureBuffer::~TextureBuffer();
}

void OpAssignTextureBuffer(FlexKit::TextureBuffer* rhs, FlexKit::TextureBuffer* lhs)
{
    *lhs = *rhs;
}

void WritePixel(uint32_t x, uint32_t y, float r, float g, float b, float a, FlexKit::TextureBuffer* buffer)
{
    struct RGBA
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };

    FlexKit::TextureBufferView<RGBA> view(*buffer);
    view[FlexKit::uint2{x, y}] = RGBA{
        (uint8_t)r,
        (uint8_t)g,
        (uint8_t)b,
        (uint8_t)a
    };
}


/************************************************************************************************/


void DoNothing(void* lhs, void* rhs)
{

}


void EditorProjectScriptConnector::Register(EditorScriptEngine& engine)
{
	auto scriptEngine = engine.GetScriptEngine();
	scriptEngine->RegisterGlobalFunction("int GetResourceCount()", asMETHOD(EditorProjectScriptConnector, GetResourceCount), asCALL_THISCALL_ASGLOBAL, this);


	// Register TextureBuffer Type
	auto c = scriptEngine->RegisterObjectType("TextureBuffer", sizeof(FlexKit::TextureBuffer), asOBJ_VALUE | asGetTypeTraits<FlexKit::TextureBuffer>());                                               assert(c >= 0);
	c = scriptEngine->RegisterObjectBehaviour("TextureBuffer", asBEHAVE_CONSTRUCT,  "void f()",                                     asFunctionPtr(CreateTextureBufferDefault),  asCALL_CDECL_OBJLAST); assert(c >= 0);
	c = scriptEngine->RegisterObjectBehaviour("TextureBuffer", asBEHAVE_CONSTRUCT,  "void f(uint width, uint height, uint format)", asFunctionPtr(CreateTextureBuffer),         asCALL_CDECL_OBJLAST); assert(c >= 0);
	c = scriptEngine->RegisterObjectBehaviour("TextureBuffer", asBEHAVE_DESTRUCT,   "void f()",                                     asFunctionPtr(ReleaseTextureBuffer),        asCALL_CDECL_OBJLAST); assert(c >= 0);
    c = scriptEngine->RegisterObjectMethod("TextureBuffer", "void WritePixel(uint x, uint y, float, float, float, float)",          asFunctionPtr(WritePixel),                  asCALL_CDECL_OBJLAST); assert(c >= 0);
    c = scriptEngine->RegisterObjectMethod("TextureBuffer", "void opAssign(const TextureBuffer& in)",                               asFunctionPtr(OpAssignTextureBuffer),       asCALL_CDECL_OBJLAST); assert(c >= 0);

    SceneReference::Register(scriptEngine);

    // Register Global project functions
    c = scriptEngine->RegisterGlobalFunction(
        "void Create2DTextureResource(const TextureBuffer& in, uint ResourceID, string resourceName, string format)",
        asMETHOD(EditorProjectScriptConnector, CreateTexture2DResource), asCALL_THISCALL_ASGLOBAL, this);       assert(c >= 0);

    c = scriptEngine->RegisterGlobalFunction(
        "Scene@ GetViewportScene()",
        asMETHOD(EditorProjectScriptConnector, EditorProjectScriptConnector::GetEditorScene), asCALL_THISCALL_ASGLOBAL, this);    assert(c >= 0);
}


/************************************************************************************************/


void EditorProjectScriptConnector::CreateTexture2DResource(FlexKit::TextureBuffer* buffer, uint32_t resourceID, std::string* resourceName, std::string* format)
{
    auto newResource    = FlexKit::CreateTextureResource(*buffer, *format);
    auto texture        = std::static_pointer_cast<FlexKit::TextureResource>(newResource);

    texture->ID             = *resourceName;
    texture->assetHandle    = resourceID;

    project.AddResource(newResource);
}



/**********************************************************************

Copyright (c) 2021 Robert May

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
