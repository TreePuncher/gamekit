#include <QtWidgets/qapplication>
#include <QtWidgets/qwidget>
#include <QtWidgets/qmainwindow>
#include <QtWidgets/qtextedit.h>
#include <QtWidgets/qdockwidget.h>

#include <QtGui/qwindow>
#include <qthread>

#include <iostream>
#include <Win32Graphics.h>
#include <qtimer>
#include <qdebug>
#include <qevent.h>
#include <fmt/format.h>

#include "DXRenderWindow.h"
#include "EditorProject.h"
#include "EditorConfig.h"
#include "EditorMainWindow.h"
#include "EditorApplication.h"

#include <allsourcefiles.cpp>
#include "..\FlexKitResourceCompiler\MetaData.cpp"
#include "..\FlexKitResourceCompiler\MeshProcessing.cpp"
#include "..\FlexKitResourceCompiler\SceneResource.cpp"
#include "..\FlexKitResourceCompiler\TextureResourceUtilities.cpp"
#include "..\FlexKitResourceCompiler\ResourceUtilities.cpp"
#include "..\FlexKitResourceCompiler\Animation.cpp"

#include <angelscript/scriptbuilder/scriptbuilder.cpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

int main(int argc, char* argv[])
{
    auto qtApplication      = new QApplication{ argc, argv };
    auto editor             = new EditorApplication{ *qtApplication };

    return qtApplication->exec();
}


/**********************************************************************

Copyright (c) 2021 Robert May

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
