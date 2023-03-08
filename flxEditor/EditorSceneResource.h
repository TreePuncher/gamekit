#ifndef SCENERESOURCES_H
#define SCENERESOURCES_H

#include "buildsettings.h"

#include "EditorResource.h"
#include "ResourceUtilities.h"

#include "containers.h"
#include "RuntimeComponentIDs.h"

#include <filesystem>


namespace FlexKit
{   /************************************************************************************************/


	typedef FlexKit::Handle_t<16> ResSceneHandle;
	typedef FlexKit::Handle_t<16> ResNodeHandle;


	/************************************************************************************************/


	struct Scene_Desc
	{
		size_t				MaxTriMeshCount		= 0;
		size_t				MaxEntityCount		= 0;
		size_t				MaxPointLightCount	= 0;
		size_t				MaxSkeletonCount	= 0;
		iAllocator*			SceneMemory			= nullptr;
		iAllocator*			AssetMemory			= nullptr;
		ResNodeHandle		Root;
		ShaderSetHandle		DefaultMaterial;
	};


	struct SceneStats
	{
		size_t EntityCount	= 0;
		size_t LightCount	= 0;

		SceneStats& operator += (const SceneStats& in)
		{
			EntityCount += in.EntityCount;
			LightCount	+= in.LightCount;
			return *this;
		}
	};


	/************************************************************************************************/


	struct CompileSceneFromFBXFile_DESC
	{
		ResNodeHandle           SceneRoot;

		//physx::PxCooking*		Cooker		= nullptr;
		//physx::PxFoundation*	Foundation	= nullptr;

		bool					 SUBDIVEnabled;
		bool					 CloseFBX;
		bool					 IncludeShaders;
		bool					 CookingEnabled;
	};


	/************************************************************************************************/


	struct SceneNode
	{
		Quaternion	orientation;
		float3		position;
		float3		scale;
		size_t		parent;
		size_t		pad;

		std::string		id;
		MetaDataList	metaData;

		void Serialize(auto& ar)
		{
			ar& orientation;
			ar& position;
			ar& scale;
			ar& parent;
			ar& id;
		}
	};



	/************************************************************************************************/


	class EntityComponent : public SerializableInterface<GetTypeGUID(EntityComponent)>
	{
	public:
		EntityComponent(const uint32_t IN_id = -1) : id{ IN_id } {}
		~EntityComponent() = default;


		virtual Blob GetBlob()					{ return {}; }
		virtual bool ExportComponent() const	{ return true; }

		uint32_t id;

		static EntityComponent* CreateComponent(uint32_t ID)
		{
			if (constructors.find(ID) != constructors.end())
				return constructors[ID]();
			else
				return nullptr;
		}

		void Serialize(auto& archive)
		{
			archive& id;
		}

	protected:
		inline static std::map<uint32_t, EntityComponent* (*)()> constructors;

		template<typename TY, size_t ConstructorID>
		struct RegisterConstructorHelper
		{
			static bool Register()
			{
				EntityComponent::constructors[ConstructorID] = []() -> EntityComponent*
				{
					return new TY();
				};

				return true;
			}

			inline static bool _registered = Register();
		};
	};



	/************************************************************************************************/


	using EntityComponent_ptr	= std::shared_ptr<EntityComponent>;
	using ComponentVector		= std::vector<EntityComponent_ptr>;


	/************************************************************************************************/


	struct SceneEntity
	{
		uint64_t		objectID = rand();
		std::string		id;
		//uint32_t	    Node;
		ComponentVector components;
		MetaDataList	metaData;

		EntityComponent_ptr FindComponent(ComponentID id);

		void Serialize(auto& ar)
		{
			ar& objectID;
			ar& id;
			//ar& Node;
			ar& components;
			//ar& metaData;
		}
	};

	struct ScenePointLight
	{
		float	I, R;
		float3	K;
		size_t	Node;

		void Serialize(auto& ar)
		{
			ar& I;
			ar& R;
			ar& K;
			ar& Node;
		}

		operator CompiledScene::PointLight() const
		{
			return {
					.I		= I, 
					.R		= R,
					.K		= K,
					.Node	= Node
			};
		}
	};


	/************************************************************************************************/


	class SceneObject : public FlexKit::Serializable<SceneObject, iResource, GetTypeGUID(SceneResource)>
	{
	public:
		ResourceBlob CreateBlob() const override;

		uint32_t AddSceneEntity(SceneEntity entity)
		{
			const auto idx = (uint32_t)entities.size();
			entities.push_back(entity);

			return idx;
		}

		uint32_t AddSceneNode(SceneNode node)
		{
			const auto idx = (uint32_t)nodes.size();
			nodes.push_back(node);

			return idx;
		}

				ResourceID_t	GetResourceTypeID()	const noexcept final { return SceneResourceTypeID; }
		const	std::string&	GetResourceID()		const noexcept final { return ID; }
				uint64_t		GetResourceGUID()	const noexcept final { return guid; }


		void SetResourceID		(const std::string& newID)	noexcept final { ID		= newID; }
		void SetResourceGUID	(uint64_t newGUID)			noexcept final { guid	= newGUID; }

		template<class Archive>
		void Serialize(Archive& ar)
		{
			ar& translationTable;
			ar& pointLights;
			ar& nodes;
			ar& entities;
			ar& staticEntities;

			ar& guid;
			ar& ID;
		}


		IDTranslationTable				translationTable;
		std::vector<ScenePointLight>	pointLights;
		std::vector<SceneNode>			nodes;
		std::vector<SceneEntity>		entities;
		std::vector<SceneEntity>		staticEntities;

		size_t		guid	= rand();
		std::string	ID;
	};


	using SceneResource		= FileObjectResource<SceneObject, GetTypeGUID(SceneResource)>;
	using SceneResource_ptr = std::shared_ptr<SceneResource>;


	/************************************************************************************************/


	struct gltfImportOptions
	{
		bool importAnimations	= true;
		bool importDeformers	= true;
		bool importMeshes		= true;
		bool importMaterials	= true;
		bool importScenes		= true;
		bool importTextures		= true;
	};

	ResourceList CreateSceneFromGlTF(const std::filesystem::path& fileDir, const gltfImportOptions& options, MetaDataList&);


}   /************************************************************************************************/


#endif


/**********************************************************************

Copyright (c) 2015 - 2022 Robert May

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
