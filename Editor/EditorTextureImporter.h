#pragma once
#include "ui_EditorTextureImport.h"
#include "EditorImport.h"


class EditorProject;
class EditorRenderer;

class EditorTextureImporter : public iEditorImportor
{
public:
	EditorTextureImporter(EditorProject&, EditorRenderer& renderer);

	virtual bool            Import(const std::string& fileDir) override;
	virtual std::string     GetFileTypeName()	{ return "Image"; }
	virtual std::string     GetFileExt()		{ return "JPEG (*.jpg *.jpeg);; TIFF (*.tif);; HDR (*.hdr)"; }

	EditorProject&	project;
	EditorRenderer& renderer;
};

