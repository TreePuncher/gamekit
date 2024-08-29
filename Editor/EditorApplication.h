#pragma once

#include "DXRenderWindow.h"
#include "EditorConfig.h"
#include "EditorProject.h"
#include "EditorImport.h"
#include "EditorMainWindow.h"
#include "EditorRenderer.h"
#include "EditorScriptEngine.h"
#include "EditorInspectors.h"
#include "EditorViewport.h"


#include "EditorSceneResource.h"
#include "EditorTextureResources.h"

#include <Application.hpp>
#include <QtWidgets/qstylefactory.h>

#include "SharedEngineMemory.hpp"

/************************************************************************************************/


class EditorProjectScriptConnector;
class EditorScriptEngine;

class gltfImporter;
class USDImporter;
class GameResExporter;
class EditorTextureImporter;

using EditorProjectScriptConnector_ptr	= std::unique_ptr<EditorProjectScriptConnector>;
using EditorScriptEngine_ptr			= std::unique_ptr<EditorScriptEngine>;


using gltfImporter_ptr		= std::unique_ptr<gltfImporter>;
using USDImporter_ptr		= std::unique_ptr<USDImporter>;
using TextureImporter_ptr	= std::unique_ptr<EditorTextureImporter>;
using GameResExporter_ptr	= std::unique_ptr<GameResExporter>;

class EditorApplication
{
public:
	EditorApplication(QApplication& IN_qtApp);
	~EditorApplication();

	QApplication&					qtApp;

#if DEBUG
	FlexKit::FKApplication			fkApplication{ FlexKit::CreateEngineMemory(), { .threadCount = 6, .GPUdebugMode = true } };
#else
	FlexKit::FKApplication			fkApplication{ FlexKit::CreateEngineMemory() };
#endif

	EditorProjectScriptConnector_ptr	projectConnector;
	EditorScriptEngine_ptr				scripts;

	EditorProject					project;
	EditorRenderer&					editorRenderer;
	EditorMainWindow				mainWindow;


	gltfImporter_ptr				gltfImporter;
	USDImporter_ptr					usdImporter;
	TextureImporter_ptr				textureImporter;
	TextureImporter_ptr				cubeMapImporter;

	GameResExporter_ptr				gameResExporter;


	std::string	currentProjectFile = "";

	inline static	EditorProject* currentProject = nullptr;
	static			EditorProject* GetCurrentProject();
};


/************************************************************************************************/
