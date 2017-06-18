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

		void SetID(char* Str, size_t StrSize)
		{
			memset(ID, 0x00, ID_LENGTH);

		
			strncpy(ID, Str, StrSize);

			for (auto I = StrSize; I > 0; --I)
			{
				if (ID[I] == ' ')
				{
					ID[I] = '\0';
					StrSize--;
				}
				else if (ID[I] == '\n')
				{
					ID[I] = '\0';
					StrSize--;
				}
			}
			ID[StrSize] = '\0';
			size = StrSize;
		}

		size_t					size;
		EMETA_RECIPIENT_TYPE	UserType;
		EMETAINFOTYPE			type;
		char					ID[ID_LENGTH];	// Specifies the Asset that uses the meta data
	};

	typedef FlexKit::Vector<MetaData*> MD_Vector;


	/************************************************************************************************/


	struct Skeleton_MetaData : public MetaData
	{
		Skeleton_MetaData(){
			UserType	= MetaData::EMETA_RECIPIENT_TYPE::EMR_SKELETON;
			type		= MetaData::EMETAINFOTYPE::EMI_SKELETAL;
			size		= 0;
		}

		char	SkeletonID[ID_LENGTH];
		GUID_t	SkeletonGUID;			
	};


	/************************************************************************************************/


	struct AnimationClip_MetaData : public MetaData
	{
		AnimationClip_MetaData() {
			UserType = MetaData::EMETA_RECIPIENT_TYPE::EMR_SKELETALANIMATION;
			type	 = MetaData::EMETAINFOTYPE::EMI_ANIMATIONCLIP;
			size	 = 0;
		}

		char	ClipID[ID_LENGTH];// Mesh Name
		double	T_Start;
		double	T_End;
		GUID_t	guid;
	};


	/************************************************************************************************/


	struct AnimationEvent_MetaData : public MetaData
	{
		AnimationEvent_MetaData() {
			UserType = MetaData::EMETA_RECIPIENT_TYPE::EMR_SKELETALANIMATION;
			type	 = MetaData::EMETAINFOTYPE::EMI_ANIMATIONEVENT;
			size	 = 0;
		}

		char		ClipID[ID_LENGTH];//
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
			size		= 0;
		}

		GUID_t	guid;
		GUID_t	ColliderGUID;
		char	MeshID[ID_LENGTH];//
	};


	/************************************************************************************************/


	struct Scene_MetaData : public MetaData
	{
		Scene_MetaData(){
			UserType	= MetaData::EMETA_RECIPIENT_TYPE::EMR_NODE;
			type		= MetaData::EMETAINFOTYPE::EMI_SCENE;
			size		= 0;
			SceneIDSize = 0;
			Guid		= INVALIDHANDLE;
		}

		GUID_t	Guid;
		char	SceneID		[64];
		size_t	SceneIDSize;
	};


	/************************************************************************************************/


	struct Collider_MetaData : public MetaData
	{
		Collider_MetaData(){
			UserType		= MetaData::EMETA_RECIPIENT_TYPE::EMR_NONE;
			type			= MetaData::EMETAINFOTYPE::EMI_COLLIDER;
			size			= 0;
			ColliderIDSize	= 0;
			Guid			= INVALIDHANDLE;
		}

		GUID_t	Guid;
		char	ColliderID		[64];
		size_t	ColliderIDSize;
	};


	/************************************************************************************************/


	struct TerrainCollider_MetaData : public MetaData
	{
		TerrainCollider_MetaData() {
			UserType	      = MetaData::EMETA_RECIPIENT_TYPE::EMR_NONE;
			type		      = MetaData::EMETAINFOTYPE::EMI_TERRAINCOLLIDER;
			size              = 0;
			ColliderIDSize    = 0;
			BitmapFileLocSize = 0;
			Guid              = INVALIDHANDLE;
		}


		GUID_t	Guid;
		char	ColliderID[64];
		size_t	ColliderIDSize;
		char	BitmapFileLoc[256];
		size_t	BitmapFileLocSize;

	};


	/************************************************************************************************/


	struct Font_MetaData : public MetaData
	{
		GUID_t	Guid;
		char	FontID[64];
		size_t	FontIDSize;

		char FontFile[256];
	};


	/************************************************************************************************/


	struct TextureSet_MetaData : public MetaData
	{
		TextureSet_MetaData() {
			UserType = MetaData::EMETA_RECIPIENT_TYPE::EMR_NONE;
			type     = MetaData::EMETAINFOTYPE::EMI_TEXTURESET;
			size     = 0;
			Guid     = 0;
			memset(&Textures, 0, sizeof(Textures));
		}

		GUID_t Guid;
		FlexKit::TextureSet_Locations Textures;
	};

	typedef Vector<size_t> RelatedMetaData;


	/************************************************************************************************/


	bool			ReadMetaData		( const char* Location, iAllocator* Memory, iAllocator* TempMemory, MD_Vector& MD_Out );
	RelatedMetaData	FindRelatedMetaData ( MD_Vector* MetaData, MetaData::EMETA_RECIPIENT_TYPE Type, const char* ID, iAllocator* TempMem );
	MetaData*		ScanForRelated		( MD_Vector* MetaData, RelatedMetaData& Related, MetaData::EMETAINFOTYPE Type );
	Resource*		MetaDataToBlob		( MetaData* Meta, iAllocator* Mem );

	Resource*		MetaDataToBlob		( MetaData* Meta, iAllocator* Mem );

	Mesh_MetaData*	GetMeshMetaData		( MD_Vector* MetaData, Vector<size_t>& related );
	Vector<size_t>	FindRelatedMetaData	( MD_Vector* MetaData, MetaData::EMETA_RECIPIENT_TYPE Type, const char* ID, iAllocator* TempMem );
	MetaData*		ScanForRelated		( MD_Vector* MetaData, RelatedMetaData& Related, MetaData::EMETAINFOTYPE Type );

	void			PrintMetaDataList(MD_Vector& MD);
	/************************************************************************************************/
}

#endif
