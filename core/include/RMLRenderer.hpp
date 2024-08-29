#pragma once

#include "MathUtilities.hpp"
#include "MemoryUtilities.hpp"
#include "ResourceHandles.hpp"
#include <chrono>

namespace Rml
{
	class Context;
}

namespace FlexKit
{
	constexpr FlexKit::PSOHandle RMLDrawPSO		= FlexKit::PSOHandle{ GetCRCGUID(RMLDrawPSO) };
	constexpr FlexKit::PSOHandle RMLDraw2PSO	= FlexKit::PSOHandle{ GetCRCGUID(RMLDraw2PSO) };

	class Context;
	class Event;
	class RenderSystem;

	struct RmlPassData
	{
		struct FlexKit::ReserveConstantBufferFunction&	constantBuffer;
		struct FlexKit::ReserveVertexBufferFunction&	vertexBuffer;
		FlexKit::ResourceHandle							renderTarget;
	};


	struct RMLVertex
	{
		uint16_t point[2];
		uint16_t UV[2];
		uint8_t color[4];

	};

	class RmlIntegrator : NoCopy
	{
	public:
		RmlIntegrator(RenderSystem& renderSystem, iAllocator& allocator);
		~RmlIntegrator();

		class UpdateTask*	Update	(class EngineCore&, class UpdateDispatcher&, double dT);
		void*				Draw	(class UpdateTask* update, class EngineCore& core, RmlPassData& passData, double dT, class FrameGraph& frameGraph);

		class UpdateTask*	Update	(Rml::Context* ctx, class EngineCore&,			class UpdateDispatcher&, double dT);
		void*				Draw	(Rml::Context* ctx, class UpdateTask* update,	class EngineCore& core, RmlPassData& passData, double dT, class FrameGraph& frameGraph);

		void				HandleEvent(const FlexKit::Event& evt);
		void				HandleEvent(Rml::Context* uiCtx, const FlexKit::Event& evt);

		Rml::Context*	GetMainContext();
		Rml::Context*	CreateContext(const char* id, const uint2& WH);
	private:
		struct RmlUI*	impl		= nullptr;
		iAllocator*		allocator	= nullptr;
	};
}	// namespace FlexKit;



/**********************************************************************

Copyright (c) 2014-2024 Robert May

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
