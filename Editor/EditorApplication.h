#pragma once

#include "DXRenderWindow.h"
#include "EditorComponentTable.hpp"
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

struct EditorOptions
{
	bool skipPrevious = false;
	bool enableAPIDebugging = [] () -> bool
		{
#if _DEBUG
			return true;
#else
			return false;
#endif
		}();
};


class EditorApplication
{
public:
	EditorApplication(QApplication& IN_qtApp, const EditorOptions& ops = {});
	~EditorApplication();

	QApplication&					qtApp;
	QSettings						settings;

	FlexKit::FKApplication			fkApplication;

	EditorProjectScriptConnector_ptr	projectConnector;
	EditorScriptEngine_ptr				scripts;

	EditorProject					project;
	EditorRenderer&					editorRenderer;
	EditorMainWindow				mainWindow;
	EditorComponentTable			components;

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


/**********************************************************************

Copyright (c) 2024 Robert May

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
