#include "EditorApplication.h"
#include "TextureUtilities.h"
#include "angelscript.h"
#include "../FlexKitResourceCompiler/TextureResourceUtilities.h"
#include <stb_image_write.h>
#include <filesystem>

EditorProjectScriptConnector::EditorProjectScriptConnector(EditorProject& IN_project) :
	project{ IN_project }
{

}

void CreateTextureBufferDefault(void* _ptr)
{
    new(_ptr) FlexKit::TextureBuffer({ 4, 4 }, 4, FlexKit::SystemAllocator);
}

void CreateTextureBuffer(uint32_t width, uint32_t height, uint32_t format, void* _ptr)
{
	new(_ptr) FlexKit::TextureBuffer({width, height}, format, FlexKit::SystemAllocator);
}

void ReleaseTextureBuffer(void* _ptr)
{
	((FlexKit::TextureBuffer*)_ptr)->FlexKit::TextureBuffer::~TextureBuffer();
}

void OpAssignTextureBuffer(FlexKit::TextureBuffer* rhs, FlexKit::TextureBuffer* lhs)
{
    *lhs = *rhs;
}

void WritePixel(uint32_t x, uint32_t y, float r, float g, float b, float a, FlexKit::TextureBuffer* buffer)
{
    struct RGBA
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };

    FlexKit::TextureBufferView<RGBA> view(*buffer);
    view[FlexKit::uint2{x, y}] = RGBA{
        (uint8_t)r,
        (uint8_t)g,
        (uint8_t)b,
        (uint8_t)a
    };
}


/************************************************************************************************/


void EditorProjectScriptConnector::Register(EditorScriptEngine& engine)
{
	auto scriptEngine = engine.GetScriptEngine();
	scriptEngine->RegisterGlobalFunction("int GetResourceCount()", asMETHOD(EditorProjectScriptConnector, GetResourceCount), asCALL_THISCALL_ASGLOBAL, this);


	// Register TextureBuffer Type
	auto c = scriptEngine->RegisterObjectType("TextureBuffer", sizeof(FlexKit::TextureBuffer), asOBJ_VALUE | asGetTypeTraits<FlexKit::TextureBuffer>());                                                assert(c >= 0);
	c = scriptEngine->RegisterObjectBehaviour("TextureBuffer", asBEHAVE_CONSTRUCT,  "void f()", asFunctionPtr(CreateTextureBufferDefault), asCALL_CDECL_OBJLAST);                                       assert(c >= 0);
	c = scriptEngine->RegisterObjectBehaviour("TextureBuffer", asBEHAVE_CONSTRUCT,  "void f(uint width, uint height, uint format)", asFunctionPtr(CreateTextureBuffer), asCALL_CDECL_OBJLAST);          assert(c >= 0);
	c = scriptEngine->RegisterObjectBehaviour("TextureBuffer", asBEHAVE_DESTRUCT,   "void f()", asFunctionPtr(ReleaseTextureBuffer), asCALL_CDECL_OBJLAST);                                             assert(c >= 0);
    c = scriptEngine->RegisterObjectMethod("TextureBuffer", "void WritePixel(uint x, uint y, float, float, float, float)", asFunctionPtr(WritePixel), asCALL_CDECL_OBJLAST);                            assert(c >= 0);
    c = scriptEngine->RegisterObjectMethod("TextureBuffer", "void opAssign(const TextureBuffer& in)", asFunctionPtr(OpAssignTextureBuffer), asCALL_CDECL_OBJLAST);                                            assert(c >= 0);

    // Register Global project functions
    c = scriptEngine->RegisterGlobalFunction(
        "void Create2DTextureResource(const TextureBuffer& in, uint ResourceID, string resourceName, string format)",
        asMETHOD(EditorProjectScriptConnector, CreateTexture2DResource), asCALL_THISCALL_ASGLOBAL, this);
}


/************************************************************************************************/


void EditorProjectScriptConnector::CreateTexture2DResource(FlexKit::TextureBuffer* buffer, uint32_t resourceID, std::string* resourceName, std::string* format)
{
    stbi_write_png("temp.png", (int)buffer->WH[0], (int)buffer->WH[1], 4, buffer->Buffer, 4096);
    auto newResource = FlexKit::ResourceBuilder::CreateTextureResource("temp.png", *format);
    auto texture = std::static_pointer_cast<FlexKit::ResourceBuilder::TextureResource>(newResource);

    texture->ID             = *resourceName;
    texture->assetHandle    = resourceID;

    project.AddResource(newResource);
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
