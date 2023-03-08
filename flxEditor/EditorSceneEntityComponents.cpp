#include "EditorSceneEntityComponents.h"
#include "ComponentBlobs.h"

namespace FlexKit
{	/************************************************************************************************/


	Blob CreateSceneNodeComponent(uint32_t nodeIdx)
	{
		SceneNodeComponentBlob nodeblob;
		nodeblob.excludeFromScene	= false;
		nodeblob.nodeIdx			= nodeIdx;

		return { nodeblob };
	}


	/************************************************************************************************/


	Blob CreateIDComponent(const std::string& string)
	{
		IDComponentBlob IDblob;
		strncpy_s(IDblob.ID, 64, string.c_str(), string.size());

		return { IDblob };
	}


	/************************************************************************************************/


	Blob CreateBrushComponent(std::span<uint64_t> meshGUIDs)
	{
		BrushComponentBlob brushComponent;
		brushComponent.meshCount			= (uint8_t)meshGUIDs.size();
		brushComponent.header.blockSize += sizeof(uint64_t) * meshGUIDs.size();
		Blob blob;
		blob += brushComponent;

		for (auto GUID : meshGUIDs)
			blob += GUID;

		return blob;
	}


	/************************************************************************************************/


	Blob EntityMaterial::CreateSubMaterialBlob() const noexcept
	{
		SubMaterialHeader header;
		header.propertyCount	= properties.size();
		header.textureCount		= textures.size();

		Blob propertyBlob;
		for (size_t itr = 0; itr < properties.size(); itr++)
		{
			const MaterialProperty& property = properties[itr];

			std::visit(
				Overloaded
				{
					[&](float x)
					{
						propertyBlob += (uint8_t)MaterialPropertyBlobValueType::FLOAT;
						propertyBlob += propertyGUID[itr];
						propertyBlob += x;
					},
					[&](float2 xy)
					{
						propertyBlob += (uint8_t)MaterialPropertyBlobValueType::FLOAT2;
						propertyBlob += propertyGUID[itr];
						propertyBlob += xy;
					},
					[&](float3 xyz)
					{
						propertyBlob += (uint8_t)MaterialPropertyBlobValueType::FLOAT3;
						propertyBlob += propertyGUID[itr];
						propertyBlob += xyz;
					},
					[&](float4 xyzw)
					{
						propertyBlob += (uint8_t)MaterialPropertyBlobValueType::FLOAT4;
						propertyBlob += propertyGUID[itr];
						propertyBlob += xyzw;
					},
					[&](uint32_t x)
					{
						propertyBlob += (uint8_t)MaterialPropertyBlobValueType::UINT;
						propertyBlob += propertyGUID[itr];
						propertyBlob += x;
					},
					[&](uint2 xy)
					{
						propertyBlob += (uint8_t)MaterialPropertyBlobValueType::UINT2;
						propertyBlob += propertyGUID[itr];
						propertyBlob += xy;
					},
					[&](uint3 xyz)
					{
						propertyBlob += (uint8_t)MaterialPropertyBlobValueType::UINT3;
						propertyBlob += propertyGUID[itr];
						propertyBlob += xyz;
					},
					[&](uint4 xyzw)
					{
						propertyBlob += (uint8_t)MaterialPropertyBlobValueType::UINT4;
						propertyBlob += propertyGUID[itr];
						propertyBlob += xyzw;
					},
					[](auto) {}
				},	property);
		}

		Blob textureBlob;
		for (auto texture : textures)
			textureBlob += texture;

		header.materialSize = sizeof(header) + propertyBlob.size() + textureBlob.size();

		return Blob{ header } + propertyBlob + textureBlob;
	}


	Blob EntityMaterialComponent::GetBlob()
	{
		Blob subMaterials;
		for (const auto& subMaterial : materials)
			subMaterials += subMaterial.CreateSubMaterialBlob();

		MaterialComponentBlob materialComponent;
		materialComponent.materialCount = materials.size();
		materialComponent.header.blockSize = sizeof(materialComponent) + subMaterials.size();

		return Blob{ materialComponent } + subMaterials;
	}


	/************************************************************************************************/


	Blob CreatePointLightComponent(float3 K, float2 IR)
	{
		PointLightComponentBlob blob;
		blob.IR		= IR;
		blob.K		= K;

		return blob;
	}


	/************************************************************************************************/


	uint64_t EntityStringIDComponent::GetStringHash() const
	{
		std::hash<std::string_view> hash;
		return hash(stringID);
	}


	/************************************************************************************************/


	Blob EntityTriggerComponent::GetBlob()
	{
		TriggerComponentBlob blob;
		return blob;
	}


	/************************************************************************************************/


	Blob EntitySkeletonComponent::GetBlob()
	{
		SkeletonComponentBlob blob;
		blob.assetID = skeletonResourceID;

		return { blob };
	}

}	/************************************************************************************************/


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
