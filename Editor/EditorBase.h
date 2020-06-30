#ifndef EDITORBASE_H
#define EDITORBASE_H

#include "AnimationComponents.h"
#include "Application.h"
#include "GameFramework.h"
#include "EngineCore.h"
#include "WorldRender.h"
#include "TextRendering.h"
#include "defaultpipelinestates.h"
#include "GraphicScene.h"

#include "EditorPanels.h"
#include "graphics.h"

#include <imgui/imgui.h>
#include <memory>


#include "ResourceHandles.h"

#include "..\FlexKitResourceCompiler\Common.h"
#include "..\FlexKitResourceCompiler\SceneResource.h"
#include "..\FlexKitResourceCompiler\ResourceUtilities.h"


/************************************************************************************************/


inline ImTextureID TextreHandleToIM(FlexKit::ResourceHandle texture)
{
    return reinterpret_cast<ImTextureID>((size_t)texture.INDEX);
}


/************************************************************************************************/


struct EditorResourceTable
{
    std::vector<std::shared_ptr<FlexKit::ResourceBuilder::SceneResource>> scenes;
    std::vector<std::shared_ptr<FlexKit::ResourceBuilder::iResource>>     resources;
};


/************************************************************************************************/


struct SceneContext
{
    std::shared_ptr<FlexKit::GraphicScene> scene = std::make_shared<FlexKit::GraphicScene>();
};


class EditorBase : public FlexKit::FrameworkState
{
public:
    EditorBase(FlexKit::GameFramework& IN_framework);

    ~EditorBase();

	void Update			(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT);
	void DebugDraw		(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT) override {}
	void PreDrawUpdate	(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT) override {}
	void Draw			(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT, FlexKit::FrameGraph&);
	void PostDrawUpdate	(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT, FlexKit::FrameGraph&);

	bool EventHandler	(FlexKit::Event evt) override;

    void ImportFbx(const char* str);
    void LoadScene(std::shared_ptr<FlexKit::ResourceBuilder::SceneResource> scene);

    void Resize(const FlexKit::uint2 WH);

    std::unique_ptr<SceneContext>       currentScene;
    EditorResourceTable                 resourceTable;

    FlexKit::ResourceHandle             imGuiFont;

    FlexKit::VertexBufferHandle			vertexBuffer;
    FlexKit::ConstantBufferHandle		constantBuffer;

    FlexKit::SceneNodeComponent			transforms;
    FlexKit::CameraComponent			cameras;
    FlexKit::StringIDComponent			ids;
    FlexKit::DrawableComponent			drawables;
    FlexKit::SceneVisibilityComponent	visables;
    FlexKit::PointLightComponent		pointLights;
    FlexKit::SkeletonComponent          skeletonComponent;
    FlexKit::PointLightShadowCaster     shadowCasters;

    FlexKit::TextureStreamingEngine		streamingEngine;

    FlexKit::Vector<iEditorPanel*>      panels;
    UIGrid                              UI;

private:

    void DrawImGui(FlexKit::UpdateDispatcher&, FlexKit::FrameGraph&, FlexKit::ReserveVertexBufferFunction, FlexKit::ReserveConstantBufferFunction, FlexKit::ResourceHandle, ImDrawData*);
};


/************************************************************************************************/


void CreateDefaultLayout(EditorBase&);


/**********************************************************************

Copyright (c) 2019 Robert May

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


#endif EDITORBASE_H
