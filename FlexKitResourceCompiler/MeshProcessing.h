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
#include "Geometry.h"
#include "Animation.h"
#include "ResourceIDs.h"

#include <array>
#include <optional>

namespace FlexKit::ResourceBuilder
{   /************************************************************************************************/



	struct FBXVertexLayout
	{
		float3 POS		= {0};
		float3 Normal	= {0};
		float3 Tangent	= {0};
		float3 TexCord1	= {0};
		float3 Weight	= {0}; 
	};


	struct SkinDeformer
	{
		struct BoneWeights
		{
			const char*				name;
			std::vector<float>		weights = std::vector<float>	{ 4 };
			std::vector<size_t>		indices = std::vector<size_t>	{ 4 };
		};

		std::vector<BoneWeights>	bones;
		std::vector<uint4_16>		indices;

		size_t						size;
	};

	struct MeshDesc
	{
		bool UV         = false;
		bool Normals    = false;
		bool Tangents   = false;
		bool Weights    = false;

		float3			MinV    = float3(0, 0, 0);
		float3			MaxV    = float3(0, 0, 0);
		float			R       = 0;

		size_t          ID      = (uint32_t)rand();

		size_t			                   faceCount = 0;
		SkinDeformer                       skin;
		SkeletonResource_ptr               skeleton;
		MeshUtilityFunctions::TokenList    tokens;
	};


	/************************************************************************************************/


	using MeshKDBTree_ptr = std::shared_ptr<FlexKit::MeshUtilityFunctions::MeshKDBTree>;


	struct MeshResource : public iResource
	{
		ResourceBlob CreateBlob() override
		{ 
			size_t bufferSize		= CalculateResourceSize();
			TriMeshAssetBlob* blob	= reinterpret_cast<TriMeshAssetBlob*>(malloc(bufferSize));

			blob->GUID				= TriMeshID;
			blob->HasAnimation		= AnimationData > 0;
			blob->HasIndexBuffer	= true;
			blob->BufferCount		= 0;
			blob->SkeletonGuid		= SkeletonGUID;
			blob->Type				= EResourceType::EResource_TriMesh;
			blob->ResourceSize		= bufferSize;

			blob->IndexCount	= IndexCount;
			blob->IndexBuffer	= IndexBuffer_Idx;
			blob->Info.minx		= Info.Min.x;
			blob->Info.miny		= Info.Min.y;
			blob->Info.minz		= Info.Min.z;
			blob->Info.maxx		= Info.Max.x;
			blob->Info.maxy		= Info.Max.y;
			blob->Info.maxz		= Info.Max.z;
			blob->Info.r		= Info.r;
	
			memcpy(blob->BS, &BS, sizeof(BoundingSphere));
			blob->AABB[0] = AABB.Min[0];
			blob->AABB[1] = AABB.Min[1];
			blob->AABB[2] = AABB.Min[2];
			blob->AABB[3] = AABB.Max[0];
			blob->AABB[4] = AABB.Max[1];
			blob->AABB[5] = AABB.Max[2];

			strcpy_s(blob->ID, ID_LENGTH > ID.size() ? ID_LENGTH : ID.size(), ID.c_str());

            blob->submeshes = submeshes;

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

			ResourceBlob        out;
			out.GUID			= TriMeshID;
			out.ID				= ID;
			out.resourceType	= EResourceType::EResource_TriMesh;
			out.buffer			= reinterpret_cast<char*>(blob);
			out.bufferSize		= bufferSize;

			return out;
		}

        const ResourceID_t GetResourceTypeID()  const override { return MeshResourceTypeID; }
		const std::string& GetResourceID()      const override { return ID; }
		const uint64_t     GetResourceGUID()    const override { return TriMeshID; }

        std::optional<VertexBufferView*> GetBuffer(VERTEXBUFFER_TYPE type) const
        {
            for (auto buffer : buffers)
                if (buffer && buffer->GetBufferType() == type)
                    return buffer;

            return {};
        }

        size_t GetIndexBufferIndex() const
        {
            for (size_t I = 0; I < buffers.size(); I++)
                if (buffers[I] && buffers[I]->GetBufferType() == VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX)
                    return I;

            return -1;
        }

		size_t CalculateResourceSize()
		{
			size_t Size = 0;
			for (auto B : buffers)
				Size += B ? B->GetBufferSizeRaw() : 0;

			return Size + sizeof(TriMeshAssetBlob);
		}

		size_t AnimationData;
		size_t IndexCount;
		size_t TriMeshID;
		size_t IndexBuffer_Idx;

        static_vector<FlexKit::SubMesh, 32> submeshes;

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
			float3 Min, Max;
			float  r;
		}Info;


		void BakeTransform(const float4x4 transform)
		{
			return;
			for (auto view : buffers)
			{
				if (view)
				{
					switch (view->GetBufferType())
					{
						case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL:
						case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT:
                        {
                            auto proxy = view->CreateTypedProxy<float3>();
                            TransformBuffer(
                                proxy,
                                [&](auto V) -> float3
                                {
                                    return V;
                                    //return (transform * float4(V, 0)).xyz().normal(); // TODO: Handle scaling correctly
                                });
                        }	break;
						case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION:
                        {	struct Vertex
							{
								float pos[3];
							};
                            auto proxy = view->CreateTypedProxy<Vertex>();
							TransformBuffer(
								proxy,
								[&](auto V)
								{
									Vertex V_out;

									const float3 pos = float3::Load(V.pos);
									auto new_v = (transform * float4(pos, 1)).xyz();
									V_out.pos[0] = new_v.x;
									V_out.pos[1] = new_v.y;
									V_out.pos[2] = new_v.z;

									return V_out;
								});
						}	break;
					}
				}
			}
		}


		// Visibility Information
		MeshKDBTree_ptr kdbTree;
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


	SkinDeformer		CreateSkin			(const fbxsdk::FbxMesh* Mesh);
	MeshDesc			TranslateToTokens	(fbxsdk::FbxMesh* Mesh, MeshUtilityFunctions::TokenList& TokensOut, SkeletonResource_ptr S = nullptr, bool SubDiv_Enabled = false);
	
	MeshResource_ptr	CreateMeshResource	(MeshDesc*, const size_t meshCount, const std::string& ID, const MetaDataList& MD = MetaDataList{}, const bool EnableSubDiv = false);


	/************************************************************************************************/
}
#endif
