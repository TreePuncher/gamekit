/**********************************************************************

Copyright (c) 2015 - 2017 Robert May

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

#include "..\coreutilities\memoryutilities.h"
#include "..\coreutilities\Resources.h"

using FlexKit::iAllocator;

namespace FlexKit
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
		};

		enum class EMETA_RECIPIENT_TYPE
		{
			EMR_MESH,
			EMR_SKELETON,
			EMR_SKELETALANIMATION,
			EMR_NODE,
			EMR_NONE,
		};

		EMETA_RECIPIENT_TYPE	UserType;
		EMETAINFOTYPE			type;
		std::string				ID;	// Specifies the Asset that uses the meta data
	};

	typedef std::vector<MetaData*> MetaDataList;

	auto SkeletonFilter = [](MetaData* const in) {
		return (in->type == MetaData::EMETAINFOTYPE::EMI_SKELETAL);
	};

	template<typename TY_Filter>
	MetaDataList FilterList(TY_Filter& filter, const MetaDataList& metaDatas)
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


	/************************************************************************************************/


	struct AnimationClip_MetaData : public MetaData
	{
		AnimationClip_MetaData() {
			UserType = MetaData::EMETA_RECIPIENT_TYPE::EMR_SKELETALANIMATION;
			type	 = MetaData::EMETAINFOTYPE::EMI_ANIMATIONCLIP;
		}

		std::string	ClipID;// Mesh Name
		double		T_Start;
		double		T_End;
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


	std::pair<MetaDataList, bool>	ReadMetaData		(const char* Location);
	MetaDataList					FindRelatedMetaData (const MetaDataList& MetaData, const MetaData::EMETA_RECIPIENT_TYPE Type, const std::string& ID);
	MetaDataList					ScanForRelated		(const MetaDataList& MetaData, const MetaData::EMETAINFOTYPE Type);

	Resource*						MetaDataToBlob		(MetaData* Meta, iAllocator* Mem);

	Mesh_MetaData*					GetMeshMetaData		(MetaDataList& relatedMetaData);

	void							PrintMetaDataList	(const MetaDataList& MD);


}	/************************************************************************************************/

#endif
