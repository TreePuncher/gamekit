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

#include "SceneInspectors.h"

#include "..\FlexKitResourceCompiler\SceneResource.h"
#include "..\FlexKitResourceCompiler\TextureResourceUtilities.h"
#include <QtWidgets/qstylefactory.h>


/************************************************************************************************/


class gltfImporter : public iEditorImportor
{
public:
	gltfImporter(EditorProject& IN_project) :
		project         { IN_project }{}

	bool Import(const std::string fileDir) override
	{
		if (!std::filesystem::exists(fileDir))
			return false;

		FlexKit::MetaDataList meta;
		auto resources = FlexKit::CreateSceneFromGlTF(fileDir, meta);

		for (auto& resource : resources)
		{
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


/************************************************************************************************/


class GameResExporter : public iEditorExporter
{
public:
    GameResExporter(EditorProject& IN_project) :
		project         { IN_project }{}

	bool Export(const std::string fileDir, const FlexKit::ResourceList& resourceList) override
	{
        return FlexKit::ExportGameRes(fileDir, resourceList);
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


/************************************************************************************************/


struct SceneReference
{
    std::shared_ptr<ViewportScene>  scene;
    std::atomic_int                 ref_count = 1;

    static void Register(asIScriptEngine* engine);

    static SceneReference*  Factory();

    static void AddRef  (SceneReference*);
    static void Release (SceneReference*);

    static bool IsValid(SceneReference rhs);
    static bool RayCast(float x, float y, float z, SceneReference scene);
};


/************************************************************************************************/


class EditorProjectScriptConnector
{
public:
	EditorProjectScriptConnector(EditorViewport* viewport, SelectionContext& ctx, EditorProject& project);

	void Register(EditorScriptEngine& engine);

	EditorProject&      project;
    SelectionContext&   ctx;
    EditorViewport*     viewport = nullptr;

private:

	int GetResourceCount()
	{
		return project.resources.size();
	}


    void            CreateTexture2DResource (FlexKit::TextureBuffer* buffer, uint32_t resourceID, std::string* resourceName, std::string* format);


    SceneReference* GetEditorScene()
    {
        auto&& scene = viewport->GetScene();

        auto ref = SceneReference::Factory();
        ref->scene = scene;

        return ref;
    }
};


/************************************************************************************************/


struct TextureResourceViewer : public IResourceViewer
{
    TextureResourceViewer(EditorRenderer& IN_renderer, QWidget* parent_widget) :
        IResourceViewer { TextureResourceTypeID },
        renderer        { IN_renderer           },
        parent          { parent_widget         } {}

    EditorRenderer&     renderer;
    QWidget*            parent;

    void operator () (FlexKit::Resource_ptr resource) override
    {
        auto textureResource    = std::static_pointer_cast<FlexKit::TextureResource>(resource);

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
        docklet->show();
    }
};


/************************************************************************************************/


struct SceneResourceViewer : public IResourceViewer
{
    SceneResourceViewer(EditorRenderer& IN_renderer, EditorProject& IN_project, EditorMainWindow& IN_mainWindow) :
        IResourceViewer { SceneResourceTypeID   },
        project         { IN_project            },
        renderer        { IN_renderer           },
        mainWindow      { IN_mainWindow         } {}

    void operator () (FlexKit::Resource_ptr resource) override
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


/************************************************************************************************/


class EditorApplication
{
public:
    EditorApplication(QApplication& IN_qtApp);
    ~EditorApplication();

	QApplication&                   qtApp;
	FlexKit::FKApplication          fkApplication{ FlexKit::CreateEngineMemory() };


    gltfImporter                    gltfImporter;
    GameResExporter                 gameResExporter;

	EditorProject                   project;
	EditorRenderer&                 editorRenderer;
    EditorMainWindow                mainWindow;

    EditorProjectScriptConnector    projectConnector;
    EditorScriptEngine              scripts;
};


/************************************************************************************************/
