#pragma once

#include <cstdint>
#include <Components.h>
#include <EngineCore.h>

struct Portal
{
	uint64_t levelID;
	uint64_t spawnPointID;
};


class AnimationTest;


/************************************************************************************************/


struct PortalFactory
{
	PortalFactory(FlexKit::EngineCore& IN_core, FlexKit::GameObject& IN_player) : core{ &IN_core }, playerObject{ &IN_player } {}

	void OnCreateView(
		FlexKit::GameObject&	gameObject,
		FlexKit::ValueMap		user_ptr,
		const std::byte*		buffer,
		const size_t			bufferSize,
		FlexKit::iAllocator*	allocator);

	FlexKit::GameObject* playerObject;
	FlexKit::EngineCore* core;
};

using PortalHandle		= FlexKit::Handle_t<32, GetTypeGUID(PortalComponentID)>;
using PortalComponent	= FlexKit::BasicComponent_t<Portal, PortalHandle, GetTypeGUID(PortalComponentID), PortalFactory>;
using PortalView		= PortalComponent::View;


/************************************************************************************************/


struct Spawn
{
};

struct SpawnFactory
{
	static void OnCreateView(
		FlexKit::GameObject&	gameObject,
		FlexKit::ValueMap		user_ptr,
		const std::byte*		buffer,
		const size_t			bufferSize,
		FlexKit::iAllocator*	allocator);
};

using SpawnHandle		= FlexKit::Handle_t<32, GetTypeGUID(SpawnComponentBlob)>;
using SpawnComponent	= FlexKit::BasicComponent_t<Spawn, SpawnHandle, GetTypeGUID(SpawnComponentBlob), SpawnFactory>;
using SpawnView			= SpawnComponent::View;


/**********************************************************************

Copyright (c) 2015 - 2023 Robert May

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
