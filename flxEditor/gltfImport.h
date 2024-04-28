#pragma once
#include "EditorImport.h"

class EditorProject;

class gltfImporter : public iEditorImportor
{
public:
	gltfImporter(EditorProject& IN_project, class FlexKit::ThreadManager& IN_threads) :
		project	{ IN_project },
		threads	{ IN_threads } {}

	bool Import(const std::string fileDir) override;

	std::string GetFileTypeName()	override { return "glTF"; }
	std::string GetFileExt()		override { return "glb"; }

	EditorProject&		project;

	class FlexKit::ThreadManager& threads;
};

