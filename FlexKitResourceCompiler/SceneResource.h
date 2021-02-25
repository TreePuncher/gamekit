#ifndef SCENERESOURCES_H
#define SCENERESOURCES_H

/**********************************************************************

Copyright (c) 2015 - 2020 Robert May

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

#include "buildsettings.h"

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_INCLUDE_JSON

#include <stb_image.h>
#include <nlohmann\json.hpp>
#include <tiny_gltf.h>

#include "Common.h"
#include "MetaData.h"
#include "ResourceUtilities.h"

#include "containers.h"
#include "memoryutilities.h"
#include "Assets.h"
#include "AnimationUtilities.h"

#include <filesystem>


namespace FlexKit::ResourceBuilder
{   /************************************************************************************************/


    typedef FlexKit::Handle_t<16> SceneHandle;
    typedef FlexKit::Handle_t<16> NodeHandle;


    /************************************************************************************************/


    struct Scene_Desc
    {
        size_t			    MaxTriMeshCount		= 0;
        size_t			    MaxEntityCount		= 0;
        size_t			    MaxPointLightCount	= 0;
        size_t			    MaxSkeletonCount	= 0;
        iAllocator*		    SceneMemory			= nullptr;
        iAllocator*		    AssetMemory			= nullptr;
        NodeHandle	        Root;
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
        NodeHandle				SceneRoot;

        physx::PxCooking*		Cooker		= nullptr;
        physx::PxFoundation*	Foundation	= nullptr;

        bool					 SUBDIVEnabled;
        bool					 CloseFBX;
        bool					 IncludeShaders;
        bool					 CookingEnabled;
    };


    /************************************************************************************************/


    struct SceneNode
    {
        Quaternion	Q;
        float3		position;
        float3		scale;
        size_t		parent;
        size_t		pad;

        std::string		id;
        MetaDataList	metaData;
    };


    /************************************************************************************************/


    class EntityComponent
    {
    public:
        EntityComponent(const uint32_t IN_id = -1) : id{ IN_id } {}
        ~EntityComponent() = default;

        virtual Blob GetBlob() { return {}; }

        uint32_t id;
    };


    /************************************************************************************************/


    struct DrawableMaterial
    {
        float4 albedo;
        float4 specular;

        std::vector<uint64_t>           textures;
        std::vector<DrawableMaterial>   subMaterials;
    };


    /************************************************************************************************/


    Blob CreateSceneNodeComponent   (uint32_t nodeIdx);
    Blob CreateIDComponent          (std::string& string);
    Blob CreateDrawableComponent    (GUID_t meshGUID, const float4 albedo, const float4 specular);
    Blob CreateMaterialComponent    (DrawableMaterial material);
    Blob CreatePointLightComponent  (float3 K, float2 IR);


    /************************************************************************************************/


    class DrawableComponent : public EntityComponent
    {
    public:
        DrawableComponent(GUID_t IN_MGUID = INVALIDHANDLE, float4 IN_albedo = { 0, 1, 0, 0.5f }, float4 specular = { 1, 0, 1, 0 }) :
            EntityComponent { GetTypeGUID(DrawableComponent) },
            MeshGuid        { IN_MGUID }
        {
        }

        Blob GetBlob() override
        {
            return CreateDrawableComponent(MeshGuid, material.albedo, material.specular);
        }

        GUID_t MeshGuid = INVALIDHANDLE;
        GUID_t Collider = INVALIDHANDLE;

        DrawableMaterial material;
    };



    /************************************************************************************************/


    class SkeletonEntityComponent : public EntityComponent
    {
    public:
        SkeletonEntityComponent(GUID_t IN_skeletonResourceID) :
            EntityComponent     { GetTypeGUID(Skeleton) },
            skeletonResourceID  { IN_skeletonResourceID }{}

        Blob GetBlob() override;

        GUID_t skeletonResourceID;
    };


    /************************************************************************************************/


    struct Material
    {
        using MaterialProperty = std::variant<std::string, float4, float3, float2, float>;

        std::vector<MaterialProperty>   properties;
        std::vector<std::string>        propertyID;
        std::vector<uint32_t>           propertyGUID;
    };


    /************************************************************************************************/


    class MaterialComponent : public EntityComponent
    {
    public:
        MaterialComponent() :
            EntityComponent{ GetTypeGUID(DrawableComponent) } {}


        Blob GetBlob() override
        {
            return {};
        }

        std::vector<Material> materials;
    };


    using DrawableComponent_ptr = std::shared_ptr<DrawableComponent>;


    /************************************************************************************************/


    class PointLightComponent : public EntityComponent
    {
    public:
        PointLightComponent(const FlexKit::float3 IN_K, const float2 IR) :
            EntityComponent { GetTypeGUID(PointLight) },
            K               ( IN_K ),
            I               { IR.x },
            R               { IR.y } {}

        Blob GetBlob() override
        {
            return CreatePointLightComponent(K, float2{ I, R });
        }

        float	I;
        float   R;
        float3	K;
    };


    using EntityComponent_ptr   = std::shared_ptr<EntityComponent>;
    using ComponentVector       = std::vector<EntityComponent_ptr>;


    /************************************************************************************************/


    struct SceneEntity
    {
        uint32_t	    Node;
        std::string		id;
        ComponentVector components;
        MetaDataList	metaData;
    };


    struct ScenePointLight
    {
        float	I, R;
        float3	K;
        size_t	Node;

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


    class SceneResource : public iResource
    {
    public:
        ResourceBlob CreateBlob() override;
        const std::string& GetResourceID() const override { return ID;  }

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

        const ResourceID_t GetResourceTypeID() const override { return SkeletonResourceTypeID; }

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
