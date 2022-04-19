#ifndef MESHPROCESSING_H
#define MESHPROCESSING_H


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


#include "Common.h"
#include "MetaData.h"
#include "Geometry.h"
#include "Animation.h"
#include "MeshUtils.h"
#include "ResourceIDs.h"
#include "ResourceUtilities.h"

#include <array>
#include <optional>

#include "../flxEditor/Serialization.h"




using FlexKit::float2;
using FlexKit::float3;
using FlexKit::float4;
using FlexKit::float4x4;
using FlexKit::uint4_16;
using FlexKit::uint4_32;
using FlexKit::iAllocator;

using FlexKit::MeshUtilityFunctions::IndexList;
using FlexKit::MeshUtilityFunctions::CombinedVertexBuffer;


namespace FlexKit
{   /************************************************************************************************/


	struct FBXVertexLayout
	{
		float3 POS		= {0};
		float3 Normal	= {0};
		float3 Tangent	= {0};
		float3 TexCord1	= {0};
		float3 Weight	= {0}; 
	};


	struct MeshSkinDeformer
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

        MeshSkinDeformer                   skin;
		SkeletonResource_ptr               skeleton;

		MeshUtilityFunctions::TokenList     tokens;
        size_t                              faceCount;
	};


	/************************************************************************************************/


	using MeshKDBTree_ptr = std::shared_ptr<FlexKit::MeshUtilityFunctions::MeshKDBTree>;


    struct SubMeshResource
    {
        uint32_t    BaseIndex   = 0;
        uint32_t    IndexCount  = 0;
        AABB        aabb        = AABB{};
    };

    struct LevelOfDetail
    {
        static_vector<SubMesh, 32>                          submeshes;
        static_vector<std::shared_ptr<VertexBufferView>>    buffers;

        size_t IndexCount       = 0;
        size_t IndexBuffer_Idx  = 0;
    };


    /************************************************************************************************/


    template<class Archive>
    void Serialize(Archive& ar, FlexKit::AABB& aabb)
    {
        ar& aabb.Min;
        ar& aabb.Max;
    }


    template<class Archive>
    void Serialize(Archive& ar, TextureCoordinateToken& texcord)
    {
        ar& texcord.UV;
        ar& texcord.index;
    }


    void Serialize(auto& arc, FlexKit::MeshUtilityFunctions::MeshKDBTree::KDBNode& unoptimizedMesh)
    {
        arc& unoptimizedMesh.left;
        arc& unoptimizedMesh.right;

        arc& unoptimizedMesh.aabb;

        arc& unoptimizedMesh.begin;
        arc& unoptimizedMesh.end;
    }


    void Serialize(auto& ar, FlexKit::MeshUtilityFunctions::MeshKDBTree& meshKDBTree)
    {
        ar& meshKDBTree.mesh;
        ar& meshKDBTree.leafNodes;
        ar& meshKDBTree.root;
    }


    template<class Archive>
    void Serialize(Archive& ar, SubMesh& sm)
    {
        ar& sm.BaseIndex;
        ar& sm.IndexCount;
        ar& sm.aabb;
    }


    template<class Archive>
    void Serialize(Archive& ar, LevelOfDetail& lod)
    {
        ar& lod.submeshes;
        ar& lod.buffers;

        ar& lod.IndexCount;
        ar& lod.IndexBuffer_Idx;
    }


    /************************************************************************************************/


	class MeshResource :
        public Serializable<MeshResource, iResource, GetTypeGUID(MeshResource)>
	{
    public:
        ResourceBlob CreateBlob() const override;

        const ResourceID_t GetResourceTypeID()  const noexcept override { return MeshResourceTypeID; }
		const std::string& GetResourceID()      const noexcept override { return ID; }
		const uint64_t     GetResourceGUID()    const noexcept override { return TriMeshID; }

        void SetResourceID      (const std::string& newID)  noexcept final { ID = newID; }
        void SetResourceGUID    (uint64_t newGUID)          noexcept final { TriMeshID = newGUID; }

        /*
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
        */


		size_t AnimationData;
		size_t TriMeshID;


		struct SubDivInfo
		{
			size_t  numVertices;
			size_t  numFaces;
			int*	numVertsPerFace;
			int*	IndicesPerFace;

            template<class Archive>
            void serialize(Archive& ar, const unsigned int version)
            {
                ar& numVertices;
                ar& numFaces;
            }
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
        /*
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
        */
		}


        template<class Archive>
        void Serialize(Archive& ar)
        {
            ar& ID;

            ar& TriMeshID;
            ar& AnimationData;

            ar& Info.Offset;
            ar& Info.Min;
            ar& Info.Max;
            ar& Info.r;

            ar& kdbTree_0;
            ar& AABB;
            ar& BS;

            ar& Skeleton;
            ar& SkeletonGUID;

            ar& LODs;
            ar& subDivs;
        }


		// Visibility Information
		MeshKDBTree_ptr             kdbTree_0;
		AABB			            AABB;
		BoundingSphere	            BS;

		SkeletonResource_ptr	    Skeleton;
		size_t					    SkeletonGUID;


		std::vector<LevelOfDetail>  LODs;
        std::vector<SubDivInfo>		subDivs;
	};


	using MeshResource_ptr	= std::shared_ptr<MeshResource>;
	using GeometryList		= std::vector<MeshResource_ptr>;



	/************************************************************************************************/


    struct LODLevel
    {
        std::vector<MeshDesc> subMeshs;
    };
	
	MeshResource_ptr	CreateMeshResource(std::vector<LODLevel>& lods, const std::string& ID, const MetaDataList& MD = MetaDataList{}, const bool EnableSubDiv = false);


	/************************************************************************************************/
}
#endif
