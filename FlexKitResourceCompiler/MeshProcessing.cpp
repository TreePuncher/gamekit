

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

#include "stdafx.h"
#include "buildsettings.h"

#include "MeshProcessing.h"
#include "Animation.h"
#include "intersection.h"

#include <iostream>

#if USING(TOOTLE)
#include <tootlelib.h>

#ifdef _DEBUG
#pragma comment(lib, "TootleSoftwareOnlyDLL_2015_d64.lib")

#else 
#pragma comment(lib, "TootleSoftwareOnlyDLL_2015_64.lib")
#endif
#endif

namespace FlexKit::ResourceBuilder
{   /************************************************************************************************/


    uint4_16 Writeuint4(uint4_16 in)
    {
        return uint4_16{
            (unsigned short)((in[0] == -1) ? 0 : in[0]),
            (unsigned short)((in[1] == -1) ? 0 : in[1]),
            (unsigned short)((in[2] == -1) ? 0 : in[2]),
            (unsigned short)((in[3] == -1) ? 0 : in[3]) };
    }


    /************************************************************************************************/


	MeshResource_ptr CreateMeshResource(MeshDesc* meshes, const size_t meshCount, const std::string& ID, const MetaDataList& metaData, const bool enableSubDiv)
	{
		using FlexKit::FillBufferView;
		using FlexKit::AnimationClip;
		using FlexKit::Skeleton;

        using MeshUtilityFunctions::OptimizedBuffer;
        using MeshUtilityFunctions::OptimizedMesh;
		using MeshUtilityFunctions::TokenList;
        using MeshUtilityFunctions::MeshBuildInfo;
        using MeshUtilityFunctions::MeshKDBTree;
        using MeshUtilityFunctions::CreateOptimizedMesh;

        static_vector<FlexKit::SubMesh, 32> submeshes;

        MeshResource_ptr meshOut = std::make_shared<MeshResource>();
        size_t bufferCount  = 0;

        FlexKit::BoundingSphere boundingSphere = { 0, 0, 0, 1000 };
        FlexKit::AABB           aabb = {};

        OptimizedMesh optimizedMesh;

        for(size_t I = 0; I < meshCount; I++)
        {
            auto kdbTree    = std::make_shared<MeshKDBTree>(meshes[I].tokens);
            auto submesh    = CreateOptimizedMesh(*kdbTree);

            submeshes.push_back({ (uint32_t)optimizedMesh.indexes.size(), (uint32_t)submesh.indexes.size() });

            optimizedMesh  += submesh;

            aabb += kdbTree->root->aabb;
        }

        auto optimizedBuffer = OptimizedBuffer(optimizedMesh);

        auto AddSubMeshBuffer =
            [&](const VERTEXBUFFER_TYPE type, const VERTEXBUFFER_FORMAT format, auto& source)
            {
                VertexBufferView* view;
                CreateBufferView((byte*)source.data(), source.size(), view,
                    type, format, SystemAllocator);

               meshOut->buffers[bufferCount++] = view;
            };


        AddSubMeshBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32, optimizedBuffer.points);

        if (meshes[0].UV)
            AddSubMeshBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32, optimizedBuffer.textureCoordinates);

        if (meshes[0].Normals)
            AddSubMeshBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32, optimizedBuffer.normals);

        if (meshes[0].Tangents)
            AddSubMeshBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32, optimizedBuffer.tangents);

        if (meshes[0].Weights)
        {
            AddSubMeshBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32, optimizedBuffer.jointWeights);
            AddSubMeshBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R16G16B16A16, optimizedBuffer.jointIndexes);
        }

        AddSubMeshBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32, optimizedBuffer.indexes);

#if USING(TOOTLE)
		// Re-Order Buffers
		if (!EnableSubDiv)
		{
			float Efficiency = 0;
			auto RES = TootleMeasureCacheEfficiency(IB.begin(), IndexCount / 3, TOOTLE_DEFAULT_VCACHE_SIZE, &Efficiency);
			std::cout << "Mesh Efficiency: " << Efficiency << "\n";

			if (Efficiency > 0.8f) {
				std::cout << "Optimizing Mesh!\n";

				CombinedVertexBuffer	NewCVB	{ SystemAllocator, CVB.size() };	NewCVB.resize(CVB.size());
				IndexList				NewIB	{ SystemAllocator, IB.size() };		NewIB.resize(IB.size());

				auto OptimizeRes = TootleOptimize(CVB.begin(), IB.begin(), VertexCount, IndexCount / 3, sizeof(CombinedVertex), TOOTLE_DEFAULT_VCACHE_SIZE, nullptr, 0, TOOTLE_CW, NewIB.begin(), nullptr, TOOTLE_VCACHE_LSTRIPS, TOOTLE_OVERDRAW_FAST);
				auto Res = TootleOptimizeVertexMemory(CVB.begin(), IB.begin(), IndexCount / 3, VertexCount, sizeof(CombinedVertex), NewCVB.begin(), NewIB.begin(), nullptr);

				auto RES = TootleMeasureCacheEfficiency(NewIB.begin(), IndexCount / 3, TOOTLE_DEFAULT_VCACHE_SIZE, &Efficiency);
				std::cout << "New Mesh Efficiency: " << Efficiency << "\n";

				IB	= std::move(NewIB);
				CVB = std::move(NewCVB);
			}
		}
#endif

		meshOut->IndexBuffer_Idx	= meshOut->GetIndexBufferIndex();
		meshOut->IndexCount			= optimizedBuffer.indexes.size() / 4;
		meshOut->Skeleton			= meshes[0].skeleton;
		meshOut->AnimationData		= meshes[0].Weights ? EAnimationData::EAD_Skin : 0;
		meshOut->Info.Max			= aabb.Max;
		meshOut->Info.Min			= aabb.Min;
		meshOut->Info.r				= (aabb.Max - aabb.Min).magnitude() / 2;
		meshOut->TriMeshID			= meshes[0].ID;
		meshOut->ID					= ID;
		meshOut->SkeletonGUID		= meshes[0].skeleton ? meshes[0].skeleton->guid : -1;
		meshOut->BS					= { aabb.MidPoint(), (aabb.Max - aabb.Min).magnitude() / 2 };
  		meshOut->AABB				= aabb;
        meshOut->submeshes          = submeshes;

		return meshOut;
	}


	/************************************************************************************************/
}// Namespace FlexKit
