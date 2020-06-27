/**********************************************************************

Copyright (c) 2015 - 2019 Robert May

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

#include "..\buildsettings.h"



#include "..\coreutilities\CameraUtilities.cpp"
#include "..\coreutilities\Console.cpp"
#include "..\coreutilities\DebugPanel.cpp"
#include "..\coreutilities\Logging.h"
//#include "..\coreutilities\DungeonGen.cpp"
#include "..\coreutilities\Components.cpp"
#include "..\coreutilities\EngineCore.cpp"
#include "..\coreutilities\GameFramework.cpp"
#include "..\coreutilities\GraphicsComponents.cpp"
#include "..\coreutilities\GraphicScene.cpp"
#include "..\coreutilities\Handle.cpp"
#include "..\coreutilities\intersection.cpp"
#include "..\coreutilities\MathUtils.cpp"
#include "..\coreutilities\Materials.cpp"
#include "..\coreutilities\memoryutilities.cpp"
#include "..\coreutilities\ProfilingUtilities.cpp"
#include "..\coreutilities\assets.cpp"
#include "..\coreutilities\ThreadUtilities.cpp"
#include "..\coreutilities\Transforms.cpp"
#include "..\coreutilities\timeutilities.cpp"
#include "..\coreutilities\type.cpp"
#include "..\coreutilities\WorldRender.cpp"

#include "..\graphicsutilities\AnimationUtilities.cpp"
#include "..\graphicsutilities\AnimationComponents.cpp"
#include "..\graphicsutilities\AnimationRuntimeUtilities.cpp"
#include "..\graphicsutilities\CoreSceneObjects.cpp"
#include "..\graphicsutilities\DDSUtilities.cpp"
#include "..\graphicsutilities\defaultpipelinestates.cpp"
#include "..\graphicsutilities\geometry.cpp"
#include "..\graphicsutilities\FrameGraph.cpp"
#include "..\graphicsutilities\Fonts.cpp"
#include "..\graphicsutilities\graphics.cpp"
#include "..\graphicsutilities\GuiUtilities.cpp"
#include "..\graphicsutilities\TextureUtilities.cpp"
#include "..\graphicsutilities\TextureStreamingUtilities.cpp"
#include "..\graphicsutilities\TextRendering.cpp"
#include "..\graphicsutilities\Meshutils.cpp"
#include "..\graphicsutilities\PipelineState.cpp"
#include "..\graphicsutilities\TerrainRendering.cpp"

#include "..\PhysicsUtilities\physicsutilities.cpp"
