#pragma once
#include "EditorResource.h"
#include "EditorSceneResource.h"
#include <RuntimeComponentIDs.h>
#include <span>

namespace FlexKit
{	/************************************************************************************************/


	Blob CreateSceneNodeComponent	(uint32_t nodeIdx);
	Blob CreateIDComponent			(const std::string& string);
	Blob CreateBrushComponent		(std::span<GUID_t> meshGUIDs);
	Blob CreateLightComponent	(float3 K, float2 IR);


	/************************************************************************************************/


	class EntityStringIDComponent :
		public Serializable<EntityStringIDComponent, EntityComponent, FlexKit::StringComponentID>
	{
	public:
		EntityStringIDComponent() :
			Serializable{ FlexKit::StringComponentID } {}

		void Serialize(auto& ar)
		{
			EntityComponent::Serialize(ar);

			uint32_t version = 1;
			ar& version;

			ar& stringID;
		}

		Blob GetBlob() override
		{
			return CreateIDComponent(stringID);
		}

		uint64_t GetStringHash() const;

		std::string stringID;

		inline static RegisterConstructorHelper<EntityStringIDComponent, FlexKit::StringComponentID> registered{};
	};

	/************************************************************************************************/


	class EntitySceneNodeComponent :
		public Serializable<EntitySceneNodeComponent, EntityComponent, FlexKit::TransformComponentID>
	{
	public:
		EntitySceneNodeComponent(size_t IN_nodeIdx = -1 ) :
			Serializable	{ FlexKit::TransformComponentID },
			nodeIdx			{ IN_nodeIdx } {}

		void Serialize(auto& ar)
		{
			uint32_t version = 1;
			ar& version;

			ar& nodeIdx;
		}

		Blob GetBlob() override
		{
			return CreateSceneNodeComponent(nodeIdx);
		}

		size_t		nodeIdx;

		inline static RegisterConstructorHelper<EntitySceneNodeComponent, FlexKit::TransformComponentID> registered{};
	};


	/************************************************************************************************/


	class EntityBrushComponent :
		public Serializable<EntityBrushComponent, EntityComponent, GetTypeGUID(EntityBrushComponent)>
	{
	public:
		EntityBrushComponent(GUID_t IN_MGUID = INVALIDHANDLE, float4 IN_albedo = { 0, 1, 0, 0.5f }, float4 specular = { 1, 0, 1, 0 }) :
			Serializable	{ GetTypeGUID(Brush) }
		{
			if(IN_MGUID != INVALIDHANDLE)
				meshes.push_back(IN_MGUID);
		}

		void Serialize(auto& ar)
		{
			EntityComponent::Serialize(ar);

			uint32_t version = 1;
			ar& version;

			ar& meshes;
			ar& Collider;
		}

		Blob GetBlob() override
		{
			return CreateBrushComponent(meshes);
		}

		GUID_t					Collider = INVALIDHANDLE;
		std::vector<GUID_t>		meshes;

		inline static RegisterConstructorHelper<EntityBrushComponent, FlexKit::BrushComponentID> registered{};
	};



	/************************************************************************************************/


	class EntitySkeletonComponent :
		public Serializable<EntitySkeletonComponent, EntityComponent, FlexKit::SkeletonComponentID>
	{
	public:
		EntitySkeletonComponent(GUID_t IN_skeletonResourceID = -1) :
			Serializable		{ FlexKit::SkeletonComponentID },
			skeletonResourceID	{ IN_skeletonResourceID } {}

		void Serialize(auto& ar)
		{
			EntityComponent::Serialize(ar);

			uint32_t version = 1;
			ar& version;

			ar& skeletonResourceID;
		}

		Blob GetBlob() override;

		GUID_t		skeletonResourceID;

		inline static RegisterConstructorHelper<EntitySkeletonComponent, FlexKit::SkeletonComponentID> registered{};
	};


	/************************************************************************************************/


	struct EntityMaterial
	{
		using MaterialProperty = std::variant<std::string, float, float2, float3, float4, uint, uint2, uint3, uint4>;

		GUID_t							resource;		// Resource
		std::vector<uint64_t>			textures;
		std::vector<MaterialProperty>	properties;		// Properties override data in the resource
		std::vector<std::string>		propertyID;
		std::vector<uint32_t>			propertyGUID;
		std::vector<EntityMaterial>		subMaterials;

		/*
		struct Version1
		{
			using MaterialProperty = std::variant<std::string, float4, float3, float2, float>;

			GUID_t							resource;		// Resource
			std::vector<uint64_t>			textures;
			std::vector<MaterialProperty>	properties;		// Properties override data in the resource
			std::vector<std::string>		propertyID;
			std::vector<uint32_t>			propertyGUID;
			std::vector<EntityMaterial>		subMaterials;
		};
		*/

		Blob CreateSubMaterialBlob() const noexcept;

		template<class Archive>
		void Serialize(Archive& ar)
		{
			uint32_t version = 1;
			ar& version;

			switch(version)
			{
				case 1:
				{	ar& resource;
					ar& textures;
					ar& properties;
					ar& propertyID;
					ar& propertyGUID;
					ar& subMaterials;
				}	break;

				default:
					FK_LOG_ERROR("Serialization Error");
					throw std::runtime_error("Serialization Error");
			}
		}
	};


	/************************************************************************************************/


	class EntityMaterialComponent :
		public Serializable<EntityMaterialComponent, EntityComponent, GetTypeGUID(EntityMaterialComponent)>
	{
	public:
		EntityMaterialComponent() :
			Serializable{ GetTypeGUID(Material) } {}

		Blob GetBlob() override;

		void Serialize(auto& ar)
		{
			EntityComponent::Serialize(ar);

			uint32_t version = 1;
			ar& version;

			ar& materials;
		}

		bool ExportComponent() const final { return false; }

		std::vector<EntityMaterial>	materials;

		inline static RegisterConstructorHelper<EntityMaterialComponent, FlexKit::MaterialComponentID> registered{};
	};


	using BrushComponent_ptr = std::shared_ptr<EntityBrushComponent>;


	/************************************************************************************************/


	class EntityLightComponent : 
		public Serializable<EntityLightComponent, EntityComponent, GetTypeGUID(EntityLightComponent)>
	{
	public:
		EntityLightComponent(const FlexKit::float3 IN_K = {}, const float2 IR = {}) :
			Serializable	{ GetTypeGUID(PointLight) },
			K				{ IN_K },
			I				{ IR.x },
			R				{ IR.y } {}

		Blob GetBlob() override
		{
			return CreateLightComponent(K, float2{ I, R });
		}

		void Serialize(auto& ar)
		{
			EntityComponent::Serialize(ar);

			uint32_t version = 1;
			ar& version;

			ar& I;
			ar& R;
			ar& K;
		}

		float	I;
		float   R;
		float3	K;

	private:

		inline static RegisterConstructorHelper<EntityLightComponent, FlexKit::LightComponentID> registered{};
	};


	/************************************************************************************************/


	class EntityTriggerComponent : 
		public Serializable<EntityTriggerComponent, EntityComponent, GetTypeGUID(EntityTriggerComponent)>
	{
	public:
		EntityTriggerComponent() :
			Serializable	{ GetTypeGUID(EntityTriggerComponent) } {}

		Blob GetBlob() override;

		void Serialize(auto& ar)
		{
			EntityComponent::Serialize(ar);

			uint32_t version = 1;
			ar& version;

			ar& triggerID;
			ar& stringID;
		}

		std::vector<uint32_t>		triggerID;
		std::vector<std::string>	stringID; // human readable

	private:

		inline static RegisterConstructorHelper<EntityTriggerComponent, FlexKit::TriggerComponentID> registered{};
	};


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
