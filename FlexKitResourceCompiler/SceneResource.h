#ifndef SCENERESOURCES_H
#define SCENERESOURCES_H

#include "buildsettings.h"

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_INCLUDE_JSON

#include <stb_image.h>
#include <nlohmann\json.hpp>
#include <tiny_gltf.h>
#include <filesystem>

#include "Common.h"
#include "MetaData.h"
#include "ResourceUtilities.h"

#include "containers.h"
#include "memoryutilities.h"
#include "Assets.h"
#include "AnimationUtilities.h"

#include "Serialization.hpp"
#include "RuntimeComponentIDs.h"


namespace FlexKit
{   /************************************************************************************************/


    typedef FlexKit::Handle_t<16> ResSceneHandle;
    typedef FlexKit::Handle_t<16> ResNodeHandle;


    /************************************************************************************************/


    struct Scene_Desc
    {
        size_t			    MaxTriMeshCount		= 0;
        size_t			    MaxEntityCount		= 0;
        size_t			    MaxPointLightCount	= 0;
        size_t			    MaxSkeletonCount	= 0;
        iAllocator*		    SceneMemory			= nullptr;
        iAllocator*		    AssetMemory			= nullptr;
        ResNodeHandle       Root;
        ShaderSetHandle     DefaultMaterial;
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


        virtual Blob GetBlob() { return {}; }

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


    struct BrushMaterial
    {
        float4 albedo;
        float4 specular;

        std::vector<uint64_t>       textures;
        std::vector<BrushMaterial>  subMaterials;
    };


    void Serialize(auto& ar, BrushMaterial& material)
    {
        ar& material.albedo;
        ar& material.specular;
        ar& material.textures;
        ar& material.subMaterials;
    }


    /************************************************************************************************/


    Blob CreateSceneNodeComponent   (uint32_t nodeIdx);
    Blob CreateIDComponent          (const std::string& string);
    Blob CreateBrushComponent       (GUID_t meshGUID, const float4 albedo, const float4 specular);
    Blob CreateMaterialComponent    (BrushMaterial material);
    Blob CreatePointLightComponent  (float3 K, float2 IR);


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

            ar& stringID;
        }

        Blob GetBlob() override
        {
            return CreateIDComponent(stringID);
        }

        std::string stringID;

        inline static RegisterConstructorHelper<EntityStringIDComponent, FlexKit::StringComponentID> registered{};
    };

    /************************************************************************************************/


    class EntitySceneNodeComponent :
        public Serializable<EntitySceneNodeComponent, EntityComponent, FlexKit::TransformComponentID>
    {
    public:
        EntitySceneNodeComponent(size_t IN_nodeIdx = -1 ) :
            Serializable    { FlexKit::TransformComponentID },
            nodeIdx         { IN_nodeIdx } {}

        void Serialize(auto& ar)
        {
            ar& nodeIdx;
        }

        Blob GetBlob() override
        {
            return CreateSceneNodeComponent(nodeIdx);
        }

        size_t nodeIdx;

        inline static RegisterConstructorHelper<EntitySceneNodeComponent, FlexKit::TransformComponentID> registered{};
    };


    /************************************************************************************************/


    class EntityBrushComponent :
        public Serializable<EntityBrushComponent, EntityComponent, GetTypeGUID(EntityBrushComponent)>
    {
    public:
        EntityBrushComponent(GUID_t IN_MGUID = INVALIDHANDLE, float4 IN_albedo = { 0, 1, 0, 0.5f }, float4 specular = { 1, 0, 1, 0 }) :
            Serializable    { GetTypeGUID(Brush) },
            MeshGuid        { IN_MGUID } {}

        void Serialize(auto& ar)
        {
            EntityComponent::Serialize(ar);

            ar& MeshGuid;
            ar& Collider;
            ar& material;
        }

        Blob GetBlob() override
        {
            return CreateBrushComponent(MeshGuid, material.albedo, material.specular);
        }

        GUID_t MeshGuid = INVALIDHANDLE;
        GUID_t Collider = INVALIDHANDLE;

        BrushMaterial material;

        inline static RegisterConstructorHelper<EntityBrushComponent, FlexKit::BrushComponentID> registered{};
    };



    /************************************************************************************************/


    class EntitySkeletonComponent :
        public Serializable<EntitySkeletonComponent, EntityComponent, GetTypeGUID(EntitySkeletonComponent)>
    {
    public:
        EntitySkeletonComponent(GUID_t IN_skeletonResourceID = -1) :
            Serializable        { GetTypeGUID(Skeleton) },
            skeletonResourceID  { IN_skeletonResourceID } {}

        void Serialize(auto& ar)
        {
            EntityComponent::Serialize(ar);

            ar& skeletonResourceID;
        }

        Blob GetBlob() override;

        GUID_t skeletonResourceID;

        inline static RegisterConstructorHelper<EntitySkeletonComponent, FlexKit::SkeletonComponentID> registered{};
    };


    /************************************************************************************************/


    struct EntityMaterial
    {
        using MaterialProperty = std::variant<std::string, float4, float3, float2, float>;

        std::vector<MaterialProperty>   properties;
        std::vector<std::string>        propertyID;
        std::vector<uint32_t>           propertyGUID;

        template<class Archive>
        void Serialize(Archive& ar)
        {
            ar& properties;
            ar& propertyID;
            ar& propertyGUID;
        }
    };


    /************************************************************************************************/


    class EntityMaterialComponent :
        public Serializable<EntityMaterialComponent, EntityComponent, GetTypeGUID(EntityMaterialComponent)>
    {
    public:
        EntityMaterialComponent() :
            Serializable{ GetTypeGUID(Material) } {}


        Blob GetBlob() override
        {
            return {};
        }

        void Serialize(auto& ar)
        {
            EntityComponent::Serialize(ar);

            ar& materials;
        }

        std::vector<EntityMaterial> materials;

        inline static RegisterConstructorHelper<EntityMaterialComponent, FlexKit::MaterialComponentID> registered{};
    };


    using BrushComponent_ptr = std::shared_ptr<EntityBrushComponent>;


    /************************************************************************************************/


    class EntityPointLightComponent : 
        public Serializable<EntityPointLightComponent, EntityComponent, GetTypeGUID(EntityPointLightComponent)>
    {
    public:
        EntityPointLightComponent(const FlexKit::float3 IN_K = {}, const float2 IR = {}) :
            Serializable    { GetTypeGUID(PointLight) },
            K               { IN_K },
            I               { IR.x },
            R               { IR.y } {}

        Blob GetBlob() override
        {
            return CreatePointLightComponent(K, float2{ I, R });
        }

        void Serialize(auto& ar)
        {
            EntityComponent::Serialize(ar);

            ar& I;
            ar& R;
            ar& K;
        }


        float	I;
        float   R;
        float3	K;

    private:

        inline static RegisterConstructorHelper<EntityPointLightComponent, FlexKit::PointLightComponentID> registered{};
    };


    using EntityComponent_ptr   = std::shared_ptr<EntityComponent>;
    using ComponentVector       = std::vector<EntityComponent_ptr>;


    /************************************************************************************************/


    struct SceneEntity
    {
        uint64_t        objectID = rand();
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


    class SceneResource : public FlexKit::Serializable<SceneResource, iResource, GetTypeGUID(SceneResource)>
    {
    public:
        ResourceBlob CreateBlob() const override;
        const std::string& GetResourceID() const noexcept override { return ID;  }

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

        const ResourceID_t GetResourceTypeID() const noexcept override { return SceneResourceTypeID; }


        template<class Archive>
        void Serialize(Archive& ar)
        {

            ar& translationTable;
            ar& pointLights;
            ar& nodes;
            ar& entities;
            ar& staticEntities;

            ar& GUID;
            ar& ID;
        }


        IDTranslationTable			    translationTable;
        std::vector<ScenePointLight>	pointLights;
        std::vector<SceneNode>			nodes;
        std::vector<SceneEntity>		entities;
        std::vector<SceneEntity>		staticEntities;

        size_t		GUID    = rand();
        std::string	ID;
    };


    using SceneResource_ptr = std::shared_ptr<SceneResource>;


    /************************************************************************************************/


    ResourceList CreateSceneFromGlTF(const std::filesystem::path& fileDir, MetaDataList&);


}   /************************************************************************************************/


#endif


/**********************************************************************

Copyright (c) 2015 - 2021 Robert May

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
