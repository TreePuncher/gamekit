#pragma once

#include "Components.hpp"
#include "Type.hpp"
#include "ResourceHandles.hpp"

namespace FlexKit
{
	struct PlanetComponentData
	{
		float		radius;
		NodeHandle	node;
	};

	struct PlanetComponentEventHandler
	{
		static PlanetComponentData OnCreate(GameObject& gameObject);

		static void OnCreateView(GameObject& gameObject, ValueMap user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) {}
	};

	constexpr ComponentID PlanetComponentID = GetCRCGUID(PlanetComponent);
	using PlanetHandle		= FlexKit::Handle_t<32, PlanetComponentID>; 
	using PlanetComponent	= BasicComponent_t<PlanetComponentData, PlanetHandle, PlanetComponentID, PlanetComponentEventHandler>;
	using PlanetView		= PlanetComponent::View;

	struct ExtraGBufferPassInputs;
	struct ExtraForwardPassInputs;
	class RenderSystem;

	constexpr PSOHandle PlanetTestPSO		= PSOHandle(GetTypeGUID(PLANETTESTPSO));
	constexpr PSOHandle SumReductionCBTPSO	= PSOHandle(GetTypeGUID(SUMREDUCTIONCBT));


	void RegisterPlanetRenderingPipelineStates(IRenderSystem&);
	void PlanetGBufferPass(ExtraGBufferPassInputs& inputs);
	void PlanetTestPass(ExtraForwardPassInputs& inputs);
}


/**********************************************************************

Copyright (c) 2016-2024 Robert May

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
