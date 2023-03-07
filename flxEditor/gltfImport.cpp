#include <gltfImport.h>
#include "ui_EditorImportGLTF.h"
#include <string>


/************************************************************************************************/


class Dialog : public QWidget
{
public:
	Dialog(std::string IN_fileDir, EditorProject& IN_project) :
		fileDir	{ std::move(IN_fileDir) },
		project	{ IN_project }
	{
		ui.setupUi(this);
		setAttribute(Qt::WA_DeleteOnClose, true);

		connect(ui.cancelButton, &QPushButton::clicked, this, &Dialog::close);
		connect(ui.importButton, &QPushButton::clicked, this, &Dialog::OnImport);

		show();
	}

	void OnImport()
	{
		hide();

		FlexKit::gltfImportOptions	options;
		FlexKit::MetaDataList		meta;

		options.importAnimations	= ui.importAnimations->isChecked();
		options.importDeformers		= ui.importDeformers->isChecked();
		options.importMaterials		= ui.importMaterials->isChecked();
		options.importMeshes		= ui.importMeshes->isChecked();
		options.importScenes		= ui.importScenes->isChecked();
		options.importTextures		= ui.importTextures->isChecked();

		auto resources = FlexKit::CreateSceneFromGlTF(fileDir, options, meta);

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

		close();
	}

	std::string			fileDir;
	EditorProject&		project;
	Ui_ImportglTFDialog ui;
};


/************************************************************************************************/


bool gltfImporter::Import(const std::string fileDir)
{
	if (!std::filesystem::exists(fileDir))
		return false;

	new Dialog{ fileDir, project };

	return true;
}


/**********************************************************************

Copyright (c) 2015 - 2023 Robert May

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
