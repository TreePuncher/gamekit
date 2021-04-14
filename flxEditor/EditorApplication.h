#pragma once

#include "Application.h"
#include "DXRenderWindow.h"
#include "EditorProject.h"
#include "EditorConfig.h"
#include "EditorImport.h"
#include "EditorRenderer.h"
#include "EditorScriptEngine.h"
#include "EditorMainWindow.h"
#include "EditorViewport.h"

#include "..\FlexKitResourceCompiler\SceneResource.h"
#include "..\FlexKitResourceCompiler\TextureResourceUtilities.h"

class gltfImporter : public iEditorImportor
{
public:
	gltfImporter(EditorProject& IN_project) :
		project         { IN_project }{}

	bool Import(const std::string fileDir) override
	{
		if (!std::filesystem::exists(fileDir))
			return false;

		FlexKit::ResourceBuilder::MetaDataList meta;
		auto resources = FlexKit::ResourceBuilder::CreateSceneFromGlTF(fileDir, meta);

		for (auto& resource : resources)
		{
            if (SceneResourceTypeID == resource->GetResourceTypeID())
            {
                EditorScene_ptr gameScene = std::make_shared<EditorScene>();

                gameScene->sceneResource = std::static_pointer_cast<FlexKit::ResourceBuilder::SceneResource>(resource);

                for (auto& dependentResource : resources)
                    if (dependentResource != resource)
                        gameScene->sceneResources.push_back(std::make_shared<ProjectResource>(dependentResource));


                project.AddScene(gameScene);
                project.AddResource(resource);
            }
			else
				project.AddResource(resource);
		}

		return false;
	}

	std::string GetFileTypeName() override
	{
		return "glTF";
	}

	std::string GetFileExt() override
	{
		return "glb";
	}

	EditorProject& project;
};


class GameResExporter : public iEditorExporter
{
public:
    GameResExporter(EditorProject& IN_project) :
		project         { IN_project }{}

	bool Export(const std::string fileDir, const FlexKit::ResourceBuilder::ResourceList& resourceList) override
	{
        return FlexKit::ResourceBuilder::ExportGameRes(fileDir, resourceList);
	}

	std::string GetFileTypeName() override
	{
		return "gameresource";
	}

	std::string GetFileExt() override
	{
		return "gameres";
	}

	EditorProject& project;
};


class EditorProjectScriptConnector
{
public:
	EditorProjectScriptConnector(EditorProject& project);
	void Register(EditorScriptEngine& engine);

	EditorProject& project;

private:

	int GetResourceCount()
	{
		return project.resources.size();
	}

    void CreateTexture2DResource(FlexKit::TextureBuffer* buffer, uint32_t resourceID, std::string* resourceName, std::string* format);
};


struct TextureResourceViewer : public IResourceViewer
{
    TextureResourceViewer(EditorRenderer& IN_renderer, QWidget* parent_widget) :
        IResourceViewer { TextureResourceTypeID },
        renderer        { IN_renderer           },
        parent          { parent_widget         } {}

    EditorRenderer&     renderer;
    QWidget*            parent;

    void operator () (FlexKit::ResourceBuilder::Resource_ptr resource) override
    {
        auto textureResource    = std::static_pointer_cast<FlexKit::ResourceBuilder::TextureResource>(resource);

        void* textureBuffer     = textureResource->buffer;
        const auto format       = textureResource->format;
        const auto WH           = textureResource->WH;

        FlexKit::TextureBuffer  uploadBuffer{ WH, (FlexKit::byte*)textureBuffer, textureResource->bufferSize, 16, nullptr };

        auto& renderSystem  = renderer.framework.GetRenderSystem();
        auto queue          = renderSystem.GetImmediateUploadQueue();
        auto texture        = FlexKit::MoveTextureBufferToVRAM(renderSystem, queue, &uploadBuffer, format);

                    
        auto docklet        = new QDockWidget{ parent };
        auto textureViewer  = new TextureViewer(renderer, parent, texture);

        docklet->setWidget(textureViewer);
        docklet->resize({ 400, 300 });
        docklet->show();
    }
};

struct SceneResourceViewer : public IResourceViewer
{
    SceneResourceViewer(EditorRenderer& IN_renderer, EditorProject& IN_project, EditorMainWindow& IN_mainWindow) :
        IResourceViewer { SceneResourceTypeID   },
        project         { IN_project            },
        renderer        { IN_renderer           },
        mainWindow      { IN_mainWindow         } {}

    void operator () (FlexKit::ResourceBuilder::Resource_ptr resource) override
    {
        auto& view = mainWindow.Get3DView();

        for(auto& scene : project.scenes)
            if(scene->sceneResource == resource)
                view.SetScene(scene);
    }

    EditorProject&      project;
    EditorRenderer&     renderer;
    EditorMainWindow&   mainWindow;
};

class EditorApplication
{
public:
	EditorApplication(QApplication& IN_qtApp) :
		qtApp               { IN_qtApp },
		editorRenderer      { fkApplication.PushState<EditorRenderer>(fkApplication, IN_qtApp) },
		mainWindow          { editorRenderer, scripts, project, qtApp },

		visibilityComponent     { FlexKit::SystemAllocator },
		pointLights             { FlexKit::SystemAllocator },
        cameras                 { FlexKit::SystemAllocator },
        pointLightShadowMaps    { FlexKit::SystemAllocator },
        materials               { editorRenderer.framework.GetRenderSystem(), editorRenderer.textureEngine, FlexKit::SystemAllocator },

        ikTargetComponent       { FlexKit::SystemAllocator },
        ikComponent             { FlexKit::SystemAllocator },

		gltfImporter        { project },
        gameResExporter     { project },
		projectConnector    { project }
	{
        
		mainWindow.AddImporter(&gltfImporter);
        mainWindow.AddExporter(&gameResExporter);

		projectConnector.Register(scripts);
		scripts.LoadModules();

        mainWindow.AddFileAction(
            "Save",
            [&]()
            {
                project.SaveProject("Test.proj");
            });

        mainWindow.AddFileAction(
            "Load",
            [&]()
            {
                project.LoadProject("Test.proj");
            });

        ResourceBrowserWidget::AddResourceViewer(std::make_shared<TextureResourceViewer>(editorRenderer, (QWidget*)&mainWindow));
        ResourceBrowserWidget::AddResourceViewer(std::make_shared<SceneResourceViewer>(editorRenderer, project, mainWindow));

		// connect script gadgets
		for (auto& gadget : scripts.GetGadgets())
			mainWindow.RegisterGadget(gadget);
	}

	~EditorApplication()
	{
		fkApplication.Release();
	}

	QApplication&                       qtApp;
	FlexKit::FKApplication              fkApplication{ FlexKit::CreateEngineMemory() };

	EditorScriptEngine                  scripts;

	EditorProject                   project;
	EditorRenderer&                 editorRenderer;

	EditorProjectScriptConnector    projectConnector;

    gltfImporter                    gltfImporter;
    GameResExporter                 gameResExporter;

    FlexKit::SceneNodeComponent         sceneNodes;
	FlexKit::SceneVisibilityComponent   visibilityComponent;
	FlexKit::PointLightComponent        pointLights;
    FlexKit::CameraComponent            cameras;
    FlexKit::PointLightShadowMap        pointLightShadowMaps;
    FlexKit::FABRIKTargetComponent      ikTargetComponent;
    FlexKit::FABRIKComponent            ikComponent;
    FlexKit::MaterialComponent          materials;

    EditorMainWindow                mainWindow;
};


