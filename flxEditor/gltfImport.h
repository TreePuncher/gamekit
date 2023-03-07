#pragma once
#include "EditorImport.h"

class EditorProject;

class gltfImporter : public iEditorImportor
{
public:
	gltfImporter(EditorProject& IN_project) :
		project{ IN_project } {}

	bool Import(const std::string fileDir) override;

	std::string GetFileTypeName()	override { return "glTF"; }
	std::string GetFileExt()		override { return "glb"; }

	EditorProject&		project;
};

