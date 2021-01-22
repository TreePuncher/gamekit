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

#ifndef METADATA_H
#define METADATA_H

#include "memoryutilities.h"
#include "Assets.h"

using FlexKit::iAllocator;

namespace FlexKit::ResourceBuilder
{
	// Resource Compiler Functions
	struct MetaData
	{
		enum class EMETAINFOTYPE
		{
			EMI_STRING,
			EMI_INT,
			EMI_GUID,
			EMI_FLOAT,
			EMI_FONT,
			EMI_COLLIDER,
			EMI_DOUBLE,
			EMI_MESH,
			EMI_SCENE,
			EMI_SKELETAL,
			EMI_SKELETALANIMATION,
			EMI_ANIMATIONCLIP,
			EMI_ANIMATIONEVENT,
			EMI_TERRAINCOLLIDER,
            EMI_TEXTURESET,
            EMI_CUBEMAPTEXTURE,
            EMI_TEXTURE,
		};

		enum class EMETA_RECIPIENT_TYPE
		{
			EMR_MESH,
			EMR_SKELETON,
			EMR_SKELETALANIMATION,
			EMR_NODE,
			EMR_ENTITY,
			EMR_NONE,
		};

		EMETA_RECIPIENT_TYPE	UserType;
		EMETAINFOTYPE			type;
		std::string				ID;	// Specifies the Asset that uses the meta data
	};

	using MetaData_ptr = std::shared_ptr<MetaData>;
	using MetaDataList = std::vector<MetaData_ptr>;

	auto SkeletonFilter = [](MetaData* const in) -> bool{
		return (in->type == MetaData::EMETAINFOTYPE::EMI_SKELETAL);
	};

	template<typename TY_Filter>
	MetaDataList FilterList(const TY_Filter& filter, const MetaDataList& metaDatas)
	{
		MetaDataList out;
		for (auto meta : metaDatas)
			if (filter(meta))
				out.push_back(meta);

		return out;
	}


	/************************************************************************************************/


	struct Skeleton_MetaData : public MetaData
	{
		Skeleton_MetaData(){
			UserType	= MetaData::EMETA_RECIPIENT_TYPE::EMR_SKELETON;
			type		= MetaData::EMETAINFOTYPE::EMI_SKELETAL;
		}

		std::string	SkeletonID;
		GUID_t		SkeletonGUID;			
	};


    using SkeletonMetaData_ptr = std::shared_ptr<Skeleton_MetaData>;

	/************************************************************************************************/


	struct SkeletalAnimationClip_MetaData : public MetaData
	{
        SkeletalAnimationClip_MetaData() {
			UserType = MetaData::EMETA_RECIPIENT_TYPE::EMR_SKELETON;
			type	 = MetaData::EMETAINFOTYPE::EMI_SKELETALANIMATION;
		}

		std::string	ClipID;// Mesh Name
		double		T_Start;
        double		T_End;
        double		frameRate = 60;
		GUID_t		guid;
	};


	/************************************************************************************************/


	struct AnimationEvent_MetaData : public MetaData
	{
		AnimationEvent_MetaData() {
			UserType = MetaData::EMETA_RECIPIENT_TYPE::EMR_SKELETALANIMATION;
			type	 = MetaData::EMETAINFOTYPE::EMI_ANIMATIONEVENT;
		}

		std::string	ClipID;//
		uint32_t	EventID;//
		double		EventT;// Location of Event relative to Beginning of Clip
	};


	/************************************************************************************************/

	// Replaces provided GUID with a specific one
	struct Mesh_MetaData : public MetaData
	{
		Mesh_MetaData(){
			UserType	= MetaData::EMETA_RECIPIENT_TYPE::EMR_MESH;
			type		= MetaData::EMETAINFOTYPE::EMI_MESH;
		}

		GUID_t		guid;
		GUID_t		ColliderGUID;
		std::string MeshID;//
	};


	/************************************************************************************************/


	struct Scene_MetaData : public MetaData
	{
		Scene_MetaData(){
			UserType	= MetaData::EMETA_RECIPIENT_TYPE::EMR_NODE;
			type		= MetaData::EMETAINFOTYPE::EMI_SCENE;
			Guid		= INVALIDHANDLE;
		}

		GUID_t		Guid;
		std::string	SceneID;

		MetaDataList sceneMetaData;
	};


    /************************************************************************************************/


    struct TextureCubeMapMipLevel_MetaData : public MetaData
    {
        TextureCubeMapMipLevel_MetaData() {
            UserType    = MetaData::EMETA_RECIPIENT_TYPE::EMR_NONE;
            type        = MetaData::EMETAINFOTYPE::EMI_CUBEMAPTEXTURE;
        }

        uint32_t        level;
        std::string	    TextureFiles[6];
    };


    struct TextureCubeMap_MetaData : public MetaData
	{
        TextureCubeMap_MetaData(){
			UserType	= MetaData::EMETA_RECIPIENT_TYPE::EMR_NONE;
			type		= MetaData::EMETAINFOTYPE::EMI_CUBEMAPTEXTURE;
			Guid		= INVALIDHANDLE;
		}

		GUID_t		           Guid;
        AssetHandle            AssetID;
        std::string            ID;
        MetaDataList           mipLevels;
        std::string            format;
	};


    /************************************************************************************************/


    struct TextureMipLevel_MetaData : public MetaData
    {
        TextureMipLevel_MetaData() {
            UserType = MetaData::EMETA_RECIPIENT_TYPE::EMR_NONE;
            type = MetaData::EMETAINFOTYPE::EMI_CUBEMAPTEXTURE;
        }

        uint32_t        level;
        std::string	    file;
    };


    struct Texture_MetaData : public MetaData
    {
        Texture_MetaData() {
            UserType    = MetaData::EMETA_RECIPIENT_TYPE::EMR_NONE;
            type        = MetaData::EMETAINFOTYPE::EMI_TEXTURE;
        }

        AssetHandle            assetID              = rand();
        bool                   generateMipMaps      = true;
        bool                   compressTexture      = true;
        float                  compressionQuality   = 0.8f;
        std::string            stringID             = "";
        MetaDataList           mipLevels;
        std::string            format               = "";
        std::string            file;
    };



	/************************************************************************************************/


	struct Collider_MetaData : public MetaData
	{
		Collider_MetaData(){
			UserType		= MetaData::EMETA_RECIPIENT_TYPE::EMR_NONE;
			type			= MetaData::EMETAINFOTYPE::EMI_COLLIDER;
			Guid			= INVALIDHANDLE;
		}

		GUID_t		Guid;
		std::string	ColliderID;
	};


	/************************************************************************************************/


	struct TerrainCollider_MetaData : public MetaData
	{
		TerrainCollider_MetaData() {
			UserType	      = MetaData::EMETA_RECIPIENT_TYPE::EMR_NONE;
			type		      = MetaData::EMETAINFOTYPE::EMI_TERRAINCOLLIDER;
			Guid              = INVALIDHANDLE;
		}


		GUID_t		Guid;
		std::string	ColliderID;
		std::string	BitmapFileLoc;

	};


	/************************************************************************************************/


	struct Font_MetaData : public MetaData
	{
		GUID_t	Guid;
		std::string	FontID;
		std::string	FontFile;
	};


	/************************************************************************************************/


	struct TextureSet_MetaData : public MetaData
	{
		TextureSet_MetaData() {
			UserType = MetaData::EMETA_RECIPIENT_TYPE::EMR_NONE;
			type     = MetaData::EMETAINFOTYPE::EMI_TEXTURESET;
			Guid     = 0;
			memset(&Textures, 0, sizeof(Textures));
		}

		GUID_t Guid;
		FlexKit::TextureSet_Locations Textures;
	};


	/************************************************************************************************/


	class ScenePointLightMeta : public MetaData
	{
	public:
		float3	color;
		float	intensity;
		float	radius;
	};

	class SceneElementMeta : public MetaData
	{
	public:
		static SceneElementMeta Node() {
			SceneElementMeta element;
			element.UserType	= MetaData::EMETA_RECIPIENT_TYPE::EMR_NODE;
			element.type		= MetaData::EMETAINFOTYPE::EMI_SCENE;

			return element;
		}

		static SceneElementMeta Entity() {
			SceneElementMeta element;
			element.UserType	= MetaData::EMETA_RECIPIENT_TYPE::EMR_ENTITY;
			element.type		= MetaData::EMETAINFOTYPE::EMI_SCENE;

			return element;
		}

		std::string		id = "";
		MetaDataList	metaData;
	};


	/************************************************************************************************/


	class SceneComponentMeta;
	using FNComponentBlobFormatter = std::vector<byte> (*)(SceneComponentMeta*);

	inline std::vector<byte> createTransformComponentBlob(SceneComponentMeta*)
	{
		return {};
	}

	inline const std::map<std::string, uint32_t> ComponentIDMap =
	{
		{ "Transform", GetTypeGUID(TransformComponent)	},
	};

	inline const std::map<uint32_t, FNComponentBlobFormatter> ComponentBlobFormatters =
	{
		{ GetTypeGUID(TransformComponent), createTransformComponentBlob },
	};


	class SceneComponentMeta : public MetaData
	{
	public:
		FNComponentBlobFormatter CreateBlob;

		std::string		id = "";
		MetaDataList	metaData;
	};


	/************************************************************************************************/


	enum class ValueType
	{
		INT,
		UINT,
		STRING,
		FLOAT,
		INVALID
	};


	union ValueTypeUnion
	{
		ValueTypeUnion() : S{ nullptr, 0 } {}

		explicit ValueTypeUnion(float f)					: F{ f } {}
		//explicit ValueTypeUnion(float3 f3 = { 0, 0, 0 })	: F3{ f3 } {}
		explicit ValueTypeUnion(int i)				: I{ i } {}
		explicit ValueTypeUnion(std::string_view s) : S{ s } {}

		float				F;
		float				F3[3];
		int32_t				I;
		uint32_t			UI;
		std::string_view	S;
	};


	struct Value
	{
		ValueType Type = ValueType::INVALID;
		ValueTypeUnion			Data;
		std::string_view		ID;
	};



	/************************************************************************************************/


	typedef std::vector<Value> ValueList;
	typedef std::vector<std::string_view> MetaTokenList;


	/************************************************************************************************/


	using MetaDataParserFN_ptr	= MetaData* (*)(const MetaTokenList& Tokens, const ValueList& values, const size_t begin, const size_t end);
	using MetaDataParserTable	= std::map<std::string, MetaDataParserFN_ptr>;

    const MetaDataParserTable CreateDefaultParser();
    const MetaDataParserTable CreateCubeMapParser();
    const MetaDataParserTable CreateTextureParser();
    const MetaDataParserTable CreateSceneParser();
	const MetaDataParserTable CreateNodeParser();
	const MetaDataParserTable CreateEntityParser();


	inline const MetaDataParserTable DefaultParser	= CreateDefaultParser();
    inline const MetaDataParserTable CubeMapParser  = CreateCubeMapParser();
    inline const MetaDataParserTable TextureParser  = CreateTextureParser();
	inline const MetaDataParserTable SceneParser	= CreateSceneParser();
	inline const MetaDataParserTable NodeParser		= CreateNodeParser();
	inline const MetaDataParserTable EntityParser	= CreateEntityParser();


	/************************************************************************************************/


	bool							ParseTokens(const MetaDataParserTable& parser, const MetaTokenList& Tokens, MetaDataList& MD_Out, size_t begin, size_t end);

	std::pair<MetaDataList, bool>	ReadMetaData		(const char* Location);
	MetaDataList					FindRelatedMetaData (const MetaDataList& MetaData, const MetaData::EMETA_RECIPIENT_TYPE Type, const std::string& ID);
	MetaDataList					ScanForRelated		(const MetaDataList& MetaData, const MetaData::EMETAINFOTYPE Type);

	Resource*						MetaDataToBlob		(MetaData* Meta, iAllocator* Mem);

	std::shared_ptr<Mesh_MetaData>	GetMeshMetaData		(MetaDataList& relatedMetaData);

	void							PrintMetaDataList	(const MetaDataList& MD);


}	/************************************************************************************************/

#endif
