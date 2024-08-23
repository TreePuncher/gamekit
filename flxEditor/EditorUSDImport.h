#pragma once
#include "buildsettings.h"
#include "EditorImport.h"

class EditorProject;

namespace FlexKit
{
	class ThreadManager;
}

class USDImporter : public iEditorImportor
{
public:
	USDImporter(EditorProject& IN_project, class FlexKit::ThreadManager& IN_threads);

	bool Import(const std::string& fileDir) override;

	std::string GetFileTypeName()	override { return "USD"; }
	std::string GetFileExt()		override { return "usd"; }

	EditorProject& project;

	class FlexKit::ThreadManager& threads;
};


struct ObjectParserInterface_* GetAttributeParser(const std::string&);


/**********************************************************************

Copyright (c) 2019-2024 Robert May

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
