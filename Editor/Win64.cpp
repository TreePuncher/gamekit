#include "buildsettings.h"
#include "Logging.h"
#include "Application.h"
#include "AnimationComponents.h"
#include "Logging.cpp"

#include "EditorPanels.cpp"
#include "EditorBase.cpp"


#include "..\FlexKitResourceCompiler\Animation.cpp"
#include "..\FlexKitResourceCompiler\MetaData.cpp"
#include "..\FlexKitResourceCompiler\MeshProcessing.cpp"
#include "..\FlexKitResourceCompiler\ResourceUtilities.cpp"
#include "..\FlexKitResourceCompiler\SceneResource.cpp"


int main(int argc, char* argv[])
{
    FlexKit::InitLog(argc, argv);
    FlexKit::SetShellVerbocity(FlexKit::Verbosity_1);
    FlexKit::AddLogFile("Editor.log", FlexKit::Verbosity_INFO);


    const auto        WH          = FlexKit::uint2{ 1920, 1080 };
    const uint32_t    threadCount = max(std::thread::hardware_concurrency(), 1u) - 1;

    auto* memory = FlexKit::CreateEngineMemory();
    EXITSCOPE(FlexKit::ReleaseEngineMemory(memory));

    FlexKit::FKApplication app{ WH, memory, threadCount };

    auto& editor = app.PushState<EditorBase>();
    CreateDefaultLayout(editor);

    app.Run();
    app.Release();


    return 0;
}


/**********************************************************************

Copyright (c) 2020 Robert May

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
