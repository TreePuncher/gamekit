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

#include "SceneResource.h"
#include "TextureResourceUtilities.h"
#include <QtWidgets/qstylefactory.h>


/************************************************************************************************/


class EditorProjectScriptConnector;
class EditorScriptEngine;

class gltfImporter;
class GameResExporter;

using EditorProjectScriptConnector_ptr  = std::unique_ptr<EditorProjectScriptConnector>;
using EditorScriptEngine_ptr            = std::unique_ptr<EditorScriptEngine>;


using gltfImporter_ptr      = std::unique_ptr<gltfImporter>;
using GameResExporter_ptr   = std::unique_ptr<GameResExporter>;

class EditorApplication
{
public:
    EditorApplication(QApplication& IN_qtApp);
    ~EditorApplication();

	QApplication&                   qtApp;
	FlexKit::FKApplication          fkApplication{ FlexKit::CreateEngineMemory() };

    EditorProjectScriptConnector_ptr    projectConnector;
    EditorScriptEngine_ptr              scripts;

    gltfImporter_ptr                gltfImporter;
    GameResExporter_ptr             gameResExporter;

	EditorProject                   project;
	EditorRenderer&                 editorRenderer;
    EditorMainWindow                mainWindow;

    inline static   EditorProject* currentProject = nullptr;
    static          EditorProject* GetCurrentProject();
};


/************************************************************************************************/
