#pragma once

#include "Components.h"
#include "EditorSceneResource.h"


/************************************************************************************************/


constexpr uint32_t PortalComponentID		= GetTypeGUID(PortalComponentID);
constexpr uint32_t SpawnPointComponentID	= GetTypeGUID(SpawnPointComponentID);
constexpr uint32_t KeyComponentID			= GetTypeGUID(KeyComponentID);


/************************************************************************************************/


class PortalEntityComponent :
	public FlexKit::Serializable<PortalEntityComponent, FlexKit::EntityComponent, PortalComponentID>
{
public:
	PortalEntityComponent() :
		Serializable{ PortalComponentID } {}

	void Serialize(auto& ar)
	{
		EntityComponent::Serialize(ar);
	}


	FlexKit::Blob GetBlob() override;

	uint64_t spawnObjectID	= INVALIDHANDLE;	// Object String ID hash
	uint64_t sceneID		= INVALIDHANDLE;			// asset ID

	inline static RegisterConstructorHelper<PortalEntityComponent, PortalComponentID> registered{};
};


struct PortalComponentEventHandler
{
	static void OnCreateView(FlexKit::GameObject& gameObject, FlexKit::ValueMap user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator);
};


struct PortalComponentData
{
	uint64_t spawnObjectID	= INVALIDHANDLE; // Object String ID hash
	uint64_t sceneID		= INVALIDHANDLE; // asset ID
};


using PortalHandle			= FlexKit::Handle_t<32, PortalComponentID>;
using EditorPortalComponent = FlexKit::BasicComponent_t<PortalComponentData, PortalHandle, PortalComponentID, PortalComponentEventHandler>;
using EditorPortalView		= EditorPortalComponent::View;


/************************************************************************************************/


class EntitySpawnComponent :
	public FlexKit::Serializable<EntitySpawnComponent, FlexKit::EntityComponent, SpawnPointComponentID>
{
public:
	EntitySpawnComponent() :
		Serializable{ SpawnPointComponentID } {}

	void Serialize(auto& ar)
	{
		EntityComponent::Serialize(ar);

		ar& spawnArea;
	}

	FlexKit::Blob GetBlob() override;

	FlexKit::float3 spawnArea = float3{ 1, 1, 1 };

	inline static RegisterConstructorHelper<EntitySpawnComponent, SpawnPointComponentID> registered{};
};


struct SpawnComponentEventHandler
{
	static void OnCreateView(FlexKit::GameObject& gameObject, FlexKit::ValueMap user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator);
};


struct SpawnComponentData
{
	float radius = 0;
};


using SpawnHandle			= FlexKit::Handle_t<32, SpawnPointComponentID>;
using EditorSpawnComponent	= FlexKit::BasicComponent_t<SpawnComponentData, SpawnHandle, SpawnPointComponentID, SpawnComponentEventHandler>;
using EditorSpawnView		= EditorSpawnComponent::View;


class EditorProject;

void RegisterPortalComponent(EditorProject&);


/************************************************************************************************/


/**********************************************************************

Copyright (c) 2023 Robert May

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
