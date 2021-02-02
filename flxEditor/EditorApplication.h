#pragma once

#include "Application.h"
#include "DXRenderWindow.h"
#include "EditorProject.h"
#include "EditorConfig.h"
#include "EditorImport.h"
#include "EditorRenderer.h"
#include "EditorScriptEngine.h"
#include "EditorMainWindow.h"
#include "..\FlexKitResourceCompiler\SceneResource.h"

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
				project.AddScene(std::static_pointer_cast<FlexKit::ResourceBuilder::SceneResource>(resource));
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


class EditorApplication
{
public:
	EditorApplication(QApplication& IN_qtApp) :
		qtApp               { IN_qtApp },
		editorRenderer      { fkApplication.PushState<EditorRenderer>(fkApplication, IN_qtApp) },
		mainWindow          { editorRenderer, scripts, project, qtApp },
		visibilityComponent { FlexKit::SystemAllocator },
		pointLights         { FlexKit::SystemAllocator },
		gltfImporter        { project },
        gameResExporter     { project },
		projectConnector    { project }
	{
		auto viewportWidget = editorRenderer.CreateRenderWindow();

		viewportWidget->ResizeEventHandler =
			[&](DXRenderWindow* renderWindow)
			{
				//editorRenderer.DrawRenderWindow(renderWindow);
			};

		mainWindow.AddImporter(&gltfImporter);
        mainWindow.AddExporter(&gameResExporter);
		mainWindow.setCentralWidget(viewportWidget);

		projectConnector.Register(scripts);
		scripts.LoadModules();

		// connect script gadgets
		for (auto& gadget : scripts.GetGadgets())
			mainWindow.RegisterGadget(gadget);
	}

	~EditorApplication()
	{
		fkApplication.Release();
	}

	void LoadSceneResource(const std::string& file)
	{

	}

	void LoadSceneResource(FlexKit::GraphicScene& sceneOut, FlexKit::ResourceBuilder::SceneResource& sceneResource)
	{
		std::vector<FlexKit::NodeHandle> nodes;

		for (const auto node : sceneResource.nodes)
		{
			auto handle = FlexKit::GetNewNode();
			SetPositionL(handle, node.position);
			SetOrientationL(handle, node.Q);

			if (node.parent != -1)
				SetParentNode(handle, nodes[node.parent]);
		}

		for (const auto entity : sceneResource.entities)
		{
			auto newEntity = new FlexKit::GameObject;
			sceneOut.AddGameObject(*newEntity, nodes[entity.Node]);

			for (auto component : entity.components)
			{
				switch (component->id)
				{
				case FlexKit::TransformComponentID:
				{
					newEntity->AddView<FlexKit::SceneNodeView<>>(nodes[entity.Node]);
				}   break;
				default:
				{
					auto Blob               = component->GetBlob();
					auto& componentSystem   = FlexKit::GetComponent(component->id);

					componentSystem.AddComponentView(*newEntity, Blob, Blob.size(), FlexKit::SystemAllocator);
				}   break;
				}
			}
		}
	}


	QApplication&                       qtApp;
	FlexKit::FKApplication              fkApplication{ FlexKit::CreateEngineMemory() };
	FlexKit::SceneNodeComponent         sceneNodes;
	FlexKit::SceneVisibilityComponent   visibilityComponent;
	FlexKit::PointLightComponent        pointLights;

	EditorScriptEngine                  scripts;


	EditorProject                   project;
	EditorRenderer&                 editorRenderer;
	EditorMainWindow                mainWindow;

	EditorProjectScriptConnector    projectConnector;

    gltfImporter                    gltfImporter;
    GameResExporter                 gameResExporter;
};


