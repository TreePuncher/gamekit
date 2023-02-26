#pragma once

#ifndef COMPONENTBLOBS_H
#define COMPONENTBLOBS_H

#include "buildsettings.h"
#include "MathUtils.h"

namespace FlexKit
{
	enum SceneBlockType
	{
		NodeTable = 1337,
		ComponentRequirementTable,
		EntityBlock,
		EntityComponentBlock,
		MaterialComponentBlock,
	};


	struct SceneBlock
	{
		uint32_t CRC32;
		uint32_t blockType;
		uint32_t blockSize;
	};

	// SceneNodeBlock must always be defined before any entity Blocks
	struct SceneNodeBlock
	{
		struct Header
		{
			uint32_t CRC32;
			uint32_t blockType = NodeTable;
			uint32_t blockSize;
			uint32_t nodeCount;
		} header;
		static const size_t GetHeaderSize() noexcept { return sizeof(Header); }

		struct SceneNode
		{
			float3		position;		// 16
			Quaternion	orientation;	// 16
			float3		scale;			// 16
			size_t		parent;			// 8
			size_t		pad;			// 8
		};
	};


	struct ComponentRequirementBlock
	{
		uint32_t CRC32;
		uint32_t blockType = ComponentRequirementTable;
		uint32_t blockSize;

		uint32_t count;
	};


	struct ComponentBlock
	{
		struct Header
		{
			uint32_t CRC32;
			uint32_t blockType = EntityComponentBlock;
			uint32_t blockSize;
			uint32_t componentID;
		} header;
	};


	struct EntityBlock
	{
		struct Header
		{
			uint32_t CRC32;
			uint32_t blockType = EntityComponentBlock;
			size_t	 blockSize;

			// Temporary Values
			char	ID[64];

			size_t componentCount = 0;
		} header;
	};


	struct PointLightComponentBlob
	{
		ComponentBlock::Header  header = {
		   0,
		   EntityComponentBlock,
		   sizeof(PointLightComponentBlob),
		   GetTypeGUID(PointLight)
		};

		float2 IR;
		float3 K;
	};


	struct InterObjectConnection
	{
		uint32_t foreignObjectID;
		uint32_t triggerID;
		uint32_t slotID;
	};


	struct TriggerComponentBlob
	{
		ComponentBlock::Header  header = {
			0,
			EntityComponentBlock,
			sizeof(TriggerComponentBlob),
			FlexKit::TriggerComponentID
		};

		uint32_t TriggerOffset()	const noexcept { return sizeof(header); }
		uint32_t SlotOffset()		const noexcept { return TriggerOffset() + sizeof(uint32_t) * triggerCount; }
		uint32_t ConnectionOffset()	const noexcept { return SlotOffset()	+ sizeof(uint32_t) * slotCount; }

		uint32_t triggerCount		= 0;
		uint32_t slotCount			= 0;
		uint32_t connectionCount	= 0;
	};


	struct SubMaterial
	{
		static_vector<uint64_t>  textures;
	};


	struct BrushComponentBlob
	{
		ComponentBlock::Header  header = {
			0,
			EntityComponentBlock,
			sizeof(BrushComponentBlob),
			GetTypeGUID(Brush)
		};

		float4 albedo_smoothness;
		float4 specular_metal;
		uint8_t	meshCount;
	};


	struct MaterialComponentBlob
	{
		ComponentBlock::Header  header = {
			0,
			EntityComponentBlock,
			sizeof(MaterialComponentBlob),
			MaterialComponentID
		};

		GUID_t							resourceID;
		static_vector<SubMaterial, 32>	materials;

		float4 albedo_smoothness;
		float4 specular_metal;
	};


	struct IDComponentBlob
	{
		ComponentBlock::Header  header = {
			0,
			EntityComponentBlock,
			sizeof(IDComponentBlob),
			GetTypeGUID(StringID)
		};

		char ID[64];
	};


	struct SceneNodeComponentBlob
	{
		ComponentBlock::Header  header = {
			0,
			EntityComponentBlock,
			sizeof(SceneNodeComponentBlob),
			GetTypeGUID(TransformComponent)
		};

		uint32_t	nodeIdx				= 0;
		bool		excludeFromScene	= false;
	};


	struct SkeletonComponentBlob
	{
		ComponentBlock::Header  header = {
			0,
			EntityComponentBlock,
			sizeof(SkeletonComponentBlob),
			GetTypeGUID(Skeleton),
		};

		uint64_t assetID; // SkeletonAsset ID
	};


	struct PortalComponentBlob
	{
		ComponentBlock::Header  header = {
			0,
			EntityComponentBlock,
			sizeof(PortalComponentBlob),
			GetTypeGUID(PortalComponentID)
		};

		uint64_t spawnObjectID;
		uint64_t sceneID;
	};


	struct SpawnComponentBlob
	{
		ComponentBlock::Header  header = {
			0,
			EntityComponentBlock,
			sizeof(SpawnComponentBlob),
			GetTypeGUID(SpawnCompomentID),
		};

		float3		spawnOffset;
		Quaternion	spawnDirection;
	};
}	// namespace FlexKit


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

#endif
