#ifndef MESHPROCESSING_H
#define MESHPROCESSING_H


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
#include "..\graphicsutilities\Geometry.h"
#include "Animation.h"

#include <array>


namespace FlexKit
{
	/************************************************************************************************/



	struct FBXVertexLayout
	{
		float3 POS		= {0};
		float3 Normal	= {0};
		float3 Tangent	= {0};
		float3 TexCord1	= {0};
		float3 Weight	= {0}; 
	};


	struct FBXSkinDeformer
	{
		struct BoneWeights
		{
			const char*				name;
			std::vector<float>		weights = std::vector<float>	{ 4 };
			std::vector<size_t>		indices = std::vector<size_t>	{ 4 };
		};

		std::vector<BoneWeights>	bones;
		std::vector<uint4_32>		indices;

		size_t						size;
	};

	struct FBXMeshDesc
	{
		bool UV         = false;
		bool Normals    = false;
        bool Tangents   = false;
		bool Weights    = false;

		float3			MinV    = float3(0, 0, 0);
		float3			MaxV    = float3(0, 0, 0);
		float			R       = 0;

		size_t			                            faceCount = 0;
		FBXSkinDeformer                             skin;
        FlexKit::MeshUtilityFunctions::TokenList    tokens;
	};


	/************************************************************************************************/


	struct MeshResource : public iResource
	{
		ResourceBlob CreateBlob() override 
		{ 
			size_t bufferSize			= CalculateResourceSize();
			TriMeshResourceBlob* blob	= reinterpret_cast<TriMeshResourceBlob*>(malloc(bufferSize));

			blob->GUID				= TriMeshID;
			blob->HasAnimation		= AnimationData > 0;
			blob->HasIndexBuffer	= true;
			blob->BufferCount		= 0;
			blob->SkeletonGuid		= SkeletonGUID;
			blob->Type				= EResourceType::EResource_TriMesh;
			blob->ResourceSize		= bufferSize;

			blob->IndexCount	= IndexCount;
			blob->IndexBuffer	= IndexBuffer_Idx;
			blob->Info.minx		= Info.min.x;
			blob->Info.miny		= Info.min.y;
			blob->Info.minz		= Info.min.z;
			blob->Info.maxx		= Info.max.x;
			blob->Info.maxy		= Info.max.y;
			blob->Info.maxz		= Info.max.z;
			blob->Info.r		= Info.r;
	
			memcpy(blob->BS, &BS, sizeof(BoundingSphere));
			blob->AABB[0] = AABB.BottomLeft[0];
			blob->AABB[1] = AABB.BottomLeft[1];
			blob->AABB[2] = AABB.BottomLeft[2];
			blob->AABB[3] = AABB.TopRight[0];
			blob->AABB[4] = AABB.TopRight[1];
			blob->AABB[5] = AABB.TopRight[2];

			strcpy_s(blob->ID, ID_LENGTH > ID.size() ? ID_LENGTH : ID.size(), ID.c_str());

			size_t bufferPosition = 0;
			for (size_t I = 0; I < 16; ++I)
			{
				if (buffers[I])
				{
					memcpy(blob->Memory + bufferPosition, buffers[I]->GetBuffer(), buffers[I]->GetBufferSizeRaw());

					auto View = buffers[I];
					blob->Buffers[I].Begin  = bufferPosition;
					blob->Buffers[I].Format = (uint16_t)buffers[I]->GetElementSize();
					blob->Buffers[I].Type   = (uint16_t)buffers[I]->GetBufferType();
					blob->Buffers[I].size   = (size_t)buffers[I]->GetBufferSizeRaw();
					bufferPosition		  += buffers[I]->GetBufferSizeRaw();

					blob->BufferCount++;
				}
			}

			ResourceBlob out;
			out.GUID			= TriMeshID;
			out.ID				= ID;
			out.resourceType	= EResourceType::EResource_TriMesh;
			out.buffer			= reinterpret_cast<char*>(blob);
			out.bufferSize		= bufferSize;

			return out;
		}

		size_t CalculateResourceSize()
		{
			size_t Size = 0;
			for (auto B : buffers)
				Size += B ? B->GetBufferSizeRaw() : 0;

			return Size + sizeof(TriMeshResourceBlob);
		}

		size_t AnimationData;
		size_t IndexCount;
		size_t TriMeshID;
		size_t IndexBuffer_Idx;

		struct SubDivInfo
		{
			size_t  numVertices;
			size_t  numFaces;
			int*	numVertsPerFace;
			int*	IndicesPerFace;
		};

		std::string			ID;

		struct RInfo
		{
			float3 Offset;
			float3 min, max;
			float  r;
		}Info;

		// Visibility Information
		AABB			AABB;
		BoundingSphere	BS;

		SkeletonResource_ptr	Skeleton;
		size_t					SkeletonGUID;

		std::vector<SubDivInfo>				subDivs;
		std::array<VertexBufferView*, 16>	buffers;
	};


	using MeshResource_ptr	= std::shared_ptr<MeshResource>;
	using GeometryList		= std::vector<MeshResource_ptr>;


	/************************************************************************************************/


	size_t GetNormalIndex	(const int pIndex, const int vIndex, const int vID, fbxsdk::FbxMesh* Mesh);
	size_t GetTexcordIndex	(const int pIndex, const int vIndex, fbxsdk::FbxMesh* Mesh);
	size_t GetVertexIndex	(const int pIndex, const int vIndex, const int vID, fbxsdk::FbxMesh* Mesh);


	/************************************************************************************************/


	FBXSkinDeformer		CreateSkin			(const fbxsdk::FbxMesh* Mesh);
	FBXMeshDesc			TranslateToTokens	(fbxsdk::FbxMesh* Mesh, MeshUtilityFunctions::TokenList& TokensOut, SkeletonResource_ptr S = nullptr, bool SubDiv_Enabled = false);
	
	MeshResource_ptr	CreateMeshResource	(FbxMesh& Mesh, const std::string& ID = std::string{}, const MetaDataList& MD = MetaDataList{}, const bool EnableSubDiv = false);


	/************************************************************************************************/
}
#endif
