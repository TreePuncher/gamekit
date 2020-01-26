#ifndef EDITORBASE_H
#define EDITORBASE_H

#include "..\graphicsutilities\AnimationComponents.h"
#include "..\coreutilities\Application.h"
#include "..\coreutilities\GameFramework.h"
#include "..\coreutilities\EngineCore.h"
#include "..\coreutilities\WorldRender.h"
#include "..\graphicsutilities\TextRendering.h"
#include "..\graphicsutilities\defaultpipelinestates.h"
#include "graphics.h"

#include <imgui/imgui.h>


auto CreateVertexBufferReserveObject(
    FlexKit::VertexBufferHandle vertexBuffer,
    FlexKit::RenderSystem*      renderSystem,
    FlexKit::iAllocator*        allocator)
{
    return FlexKit::MakeSynchonized(
        [=](size_t reserveSize) -> FlexKit::VBPushBuffer
        {
            return FlexKit::VBPushBuffer(vertexBuffer, reserveSize, *renderSystem);
        },
        allocator);
}

auto CreateConstantBufferReserveObject(
    FlexKit::ConstantBufferHandle   vertexBuffer,
    FlexKit::RenderSystem*          renderSystem,
    FlexKit::iAllocator*            allocator)
{
    return FlexKit::MakeSynchonized(
        [=](size_t reserveSize) -> FlexKit::CBPushBuffer
        {
            return FlexKit::CBPushBuffer(vertexBuffer, reserveSize, *renderSystem);
        },
        allocator);
}

using ReserveVertexBufferFunction   = decltype(CreateVertexBufferReserveObject(FlexKit::InvalidHandle_t, nullptr, nullptr));
using ReserveConstantBufferFunction = decltype(CreateConstantBufferReserveObject(FlexKit::InvalidHandle_t, nullptr, nullptr));

inline ImTextureID TextreHandleToIM(FlexKit::ResourceHandle texture)
{
    return reinterpret_cast<ImTextureID>(texture.INDEX);
}

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

    FlexKit::ResourceHandle             imGuiFont;

    FlexKit::WorldRender				render;
    FlexKit::GBuffer                    gbuffer;
    FlexKit::ResourceHandle				depthBuffer;
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

private:



    void DrawImGui(FlexKit::UpdateDispatcher&, FlexKit::FrameGraph&, ReserveVertexBufferFunction, ReserveConstantBufferFunction, FlexKit::ResourceHandle, ImDrawData*);


    struct
    {

    }InputStates;

    struct gui_Atlas
    {
        unsigned char*  tex_pixels = nullptr;
        int             tex_w;
        int             tex_h;
    }atlas;
};


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
