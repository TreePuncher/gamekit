#ifndef SCENERESOURCES_H
#define SCENERESOURCES_H

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

#include "Common.h"
#include "MetaData.h"
#include "ResourceUtilities.h"
#include "sceneResource.h"

#include "..\buildsettings.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\coreutilities\Resources.h"
#include "..\graphicsutilities\AnimationUtilities.h"


/************************************************************************************************/


typedef FlexKit::Handle_t<16> SceneHandle;
typedef FlexKit::Handle_t<16> NodeHandle;


/************************************************************************************************/


struct Scene_Desc
{
	size_t			MaxTriMeshCount		= 0;
	size_t			MaxEntityCount		= 0;
	size_t			MaxPointLightCount	= 0;
	size_t			MaxSkeletonCount	= 0;
	iAllocator*		SceneMemory			= nullptr;
	iAllocator*		AssetMemory			= nullptr;
	NodeHandle		Root;
	ShaderSetHandle DefaultMaterial;
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
	float4		TS;
	size_t		Parent;
	size_t		pad;

	std::string ID;

	operator CompiledScene::SceneNode() const
	{
		return {
			.Q		= Q,
			.TS		= TS,
			.Parent	= Parent
		};
	}
};


struct SceneEntity
{
	GUID_t		MeshGuid;
	size_t		Node;
	GUID_t		Collider	= INVALIDHANDLE;
	float4		albedo;
	float4		specular;
	std::string	id;

	operator CompiledScene::Entity() const
	{
		return {
			.MeshGuid	= MeshGuid,
			.TextureSet	= INVALIDHANDLE,
			.Node		= Node,
			.Collider	= Collider,
			.idlength	= id.size(),
			.albedo		= albedo,
			.specular	= specular,
			.id			= id.c_str()
		};
	}
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
	ResourceBlob CreateBlob() override 
	{ 
		const size_t bufferSize = CalculateResourceSize();
		const size_t sceneTableOffset		= 0;
		const size_t pointLightTableOffset	= entities.size()								* sizeof(CompiledScene::Entity);;
		const size_t nodeTableOffset		= pointLightTableOffset + pointLights.size()	* sizeof(CompiledScene::PointLight);
		const size_t staticsTableOffset		= nodeTableOffset		+ nodes.size()			* sizeof(CompiledScene::SceneNode);
		const size_t stringTableOffset		= staticsTableOffset	+ staticEntities.size()	* sizeof(CompiledScene::Entity);;

		SceneResourceBlob* blob	= (SceneResourceBlob*)malloc(bufferSize);
		auto buffer				= blob->Buffer;

		std::vector<CompiledScene::Entity>		convertedEntities;
		std::vector<CompiledScene::Entity>		convertedStaticEntities;
		std::vector<CompiledScene::PointLight>	convertedPointLights;
		std::vector<CompiledScene::SceneNode>	convertedNodes;

		for (auto& entity : entities)	convertedEntities.push_back(entity);
		for (auto& light : pointLights)	convertedPointLights.push_back(light);
		for (auto& node : nodes)		convertedNodes.push_back(node);

		// Copy Strings
		size_t strOffset	= stringTableOffset;
		size_t strCount		= 0;
		for (auto& entity : convertedEntities)
		{
			entity.MeshGuid = TranslateID(entity.MeshGuid, translationTable);

			if (entity.idlength) {
				strCount++;
				memcpy(buffer + strOffset, entity.id, entity.idlength);// discarding the null terminator

				entity.id = reinterpret_cast<const char*>(strOffset);
				strOffset += entity.idlength;
			}
		}

		memset(blob->ID, 0, 64);
		strncpy(blob->ID, ID.c_str(), ID.size());

		blob->SceneTable.EntityCount		= entities.size();
		blob->SceneTable.NodeCount			= nodes.size();
		blob->SceneTable.LightCount			= pointLights.size();
		blob->ResourceSize					= bufferSize;
		blob->GUID							= GUID;
		blob->RefCount						= 0;
		blob->State							= Resource::EResourceState_UNLOADED;
		blob->Type							= EResource_Scene;

		blob->SceneTable.SceneStringCount	= strCount;
		blob->SceneTable.SceneStringsOffset	= stringTableOffset;

		blob->SceneTable.EntityOffset	= sceneTableOffset;
		blob->SceneTable.LightOffset	= pointLightTableOffset;
		blob->SceneTable.NodeOffset		= nodeTableOffset;
		blob->SceneTable.StaticsOffset	= staticsTableOffset;

		if(convertedEntities.size())		memcpy(buffer + sceneTableOffset,		&convertedEntities.front(),			convertedEntities.size()		* sizeof(CompiledScene::Entity));
		if(convertedPointLights.size())		memcpy(buffer + pointLightTableOffset,	&convertedPointLights.front(),		convertedPointLights.size()		* sizeof(CompiledScene::PointLight));
		if(convertedNodes.size())			memcpy(buffer + nodeTableOffset,		&convertedNodes.front(),			convertedNodes.size()			* sizeof(CompiledScene::SceneNode));
		if(convertedStaticEntities.size())	memcpy(buffer + staticsTableOffset,		&convertedStaticEntities.front(),	convertedStaticEntities.size()	* sizeof(CompiledScene::Entity));

		ResourceBlob out;
		out.GUID			= GUID;
		out.ID				= ID;
		out.resourceType	= EResourceType::EResource_Scene;
		out.buffer			= reinterpret_cast<char*>(blob);
		out.bufferSize		= bufferSize;

		return out;
	}


	size_t CalculateResourceSize() const
	{
		size_t BlobSize = sizeof(SceneResourceBlob);

		BlobSize += nodes.size()				* sizeof(CompiledScene::SceneNode);
		BlobSize += entities.size()				* sizeof(CompiledScene::Entity);
		//BlobSize += .SceneGeometry.size()		* sizeof(CompiledScene::SceneGeometry);
		BlobSize += pointLights.size()			* sizeof(CompiledScene::PointLight);
		BlobSize += staticEntities.size()		* sizeof(CompiledScene::Entity);

		for (auto E : entities)
			BlobSize += E.id.size() + 1;

		return BlobSize;
	}


	size_t AddSceneEntity(SceneEntity entity)
	{
		auto idx = entities.size();
		entities.push_back(entity);

		return idx;
	}


	size_t AddSceneNode(SceneNode node)
	{
		auto idx = nodes.size();
		nodes.push_back(node);

		return idx;
	}


	size_t AddPointLight(ScenePointLight pointLight)
	{
		auto idx = pointLights.size();
		pointLights.push_back(pointLight);

		return idx;
	}


	FBXIDTranslationTable			translationTable;
	std::vector<ScenePointLight>	pointLights;
	std::vector<SceneNode>			nodes;
	std::vector<SceneEntity>		entities;
	std::vector<SceneEntity>		staticEntities;

	size_t		GUID;
	std::string	ID;
};


using SceneResource_ptr = std::shared_ptr<SceneResource>;


/************************************************************************************************/


ResourceList CompileSceneFromFBXFile(fbxsdk::FbxScene* scene, const CompileSceneFromFBXFile_DESC& Desc, MetaDataList& METAINFO);
#endif