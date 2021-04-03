#ifndef MESHPROCESSING_H
#define MESHPROCESSING_H


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


#include "Common.h"
#include "MetaData.h"
#include "Geometry.h"
#include "Animation.h"
#include "ResourceIDs.h"
#include "ResourceUtilities.h"

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

		SkinDeformer                       skin;
		SkeletonResource_ptr               skeleton;

		MeshUtilityFunctions::TokenList     tokens;
        size_t                              faceCount;
	};


	/************************************************************************************************/


	using MeshKDBTree_ptr = std::shared_ptr<FlexKit::MeshUtilityFunctions::MeshKDBTree>;

    struct SubMeshResource
    {
        uint32_t    BaseIndex;
        uint32_t    IndexCount;
        AABB        aabb;
    };


    struct LevelOfDetail
    {
        static_vector<SubMesh, 32>          submeshes;
        static_vector<VertexBufferView*>    buffers;

        size_t IndexCount;
        size_t IndexBuffer_Idx;
    };

	struct MeshResource : public iResource
	{
        ResourceBlob CreateBlob() override;

        const ResourceID_t GetResourceTypeID()  const override { return MeshResourceTypeID; }
		const std::string& GetResourceID()      const override { return ID; }
		const uint64_t     GetResourceGUID()    const override { return TriMeshID; }


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

		// Visibility Information
		MeshKDBTree_ptr kdbTree;
		AABB			AABB;
		BoundingSphere	BS;

		SkeletonResource_ptr	Skeleton;
		size_t					SkeletonGUID;


		std::vector<LevelOfDetail>			LODs;
        std::vector<SubDivInfo>				subDivs;
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
