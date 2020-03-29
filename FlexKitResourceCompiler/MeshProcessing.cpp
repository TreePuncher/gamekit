

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

#include "stdafx.h"
#include "..\buildsettings.h"

#include "MeshProcessing.h"
#include "Animation.h"
#include "..\coreutilities\intersection.h"

#include <iostream>

#if USING(TOOTLE)
#include <tootlelib.h>

#ifdef _DEBUG
#pragma comment(lib, "TootleSoftwareOnlyDLL_2015_d64.lib")

#else 
#pragma comment(lib, "TootleSoftwareOnlyDLL_2015_64.lib")
#endif
#endif

namespace FlexKit
{

	/************************************************************************************************/


	FbxVector4 ReadNormal(int index, fbxsdk::FbxMesh* Mesh)	
	{ 
		FbxVector4 out;
		auto Normals = Mesh->GetElementNormal(index); 
		auto Mapping = Normals->GetMappingMode();

		out = Normals->GetDirectArray().GetAt(index);
		switch (Mapping)
		{
		case fbxsdk::FbxLayerElement::eNone:
			break;
		case fbxsdk::FbxLayerElement::eByControlPoint:
				{
					switch (Normals->GetReferenceMode())
					{
					case fbxsdk::FbxLayerElement::eDirect:
						out = Normals->GetDirectArray().GetAt(index);
						break;
					case fbxsdk::FbxLayerElement::eIndexToDirect:
					{
						int normalIndex = Normals->GetIndexArray().GetAt(index);
						out = Normals->GetDirectArray().GetAt(normalIndex);
					}	break;
					default:
						break;
					}
				}
			break;
		case fbxsdk::FbxLayerElement::eByPolygonVertex:
		{
			auto NormalIndex = Normals->GetIndexArray();
			auto NIndex = NormalIndex.GetAt(index);
			{
				switch (Normals->GetReferenceMode())
				{
				case fbxsdk::FbxLayerElement::eDirect:
				{
					out = Normals->GetDirectArray().GetAt(index);
				}	break;
				case fbxsdk::FbxLayerElement::eIndexToDirect:
				{
					int x = 0;
				}	break;
				default:
					break;
				}
			}
		}
		default:
			break;
		}
		return out;
	}


    /************************************************************************************************/


    FbxVector4 ReadTangent(int index, fbxsdk::FbxMesh* Mesh)
    {
        FbxVector4 out;
        auto Tangents = Mesh->GetElementTangent(index);
        auto Mapping = Tangents->GetMappingMode();

        out = Tangents->GetDirectArray().GetAt(index);
        switch (Mapping)
        {
        case fbxsdk::FbxLayerElement::eNone:
            break;
        case fbxsdk::FbxLayerElement::eByControlPoint:
        {
            switch (Tangents->GetReferenceMode())
            {
            case fbxsdk::FbxLayerElement::eDirect:
                out = Tangents->GetDirectArray().GetAt(index);
                break;
            case fbxsdk::FbxLayerElement::eIndexToDirect:
            {
                int TangentIndex = Tangents->GetIndexArray().GetAt(index);
                out = Tangents->GetDirectArray().GetAt(TangentIndex);
            }	break;
            default:
                break;
            }
        }
        break;
        case fbxsdk::FbxLayerElement::eByPolygonVertex:
        {
            auto TangentIndex = Tangents->GetIndexArray();
            auto NIndex = TangentIndex.GetAt(index);
            {
                switch (Tangents->GetReferenceMode())
                {
                case fbxsdk::FbxLayerElement::eDirect:
                {
                    out = Tangents->GetDirectArray().GetAt(index);
                }	break;
                case fbxsdk::FbxLayerElement::eIndexToDirect:
                {
                    int x = 0;
                }	break;
                default:
                    break;
                }
            }
        }
        default:
            break;
        }
        return out;
    }


	/************************************************************************************************/


	uint4_16 Writeuint4(uint4_16 in)
	{
		return uint4_16{
			(unsigned short)((in[0] == -1) ? 0 : in[0]),
			(unsigned short)((in[1] == -1) ? 0 : in[1]),
			(unsigned short)((in[2] == -1) ? 0 : in[2]),
			(unsigned short)((in[3] == -1) ? 0 : in[3]) };
	}


	/************************************************************************************************/


	size_t GetNormalIndex(const int pIndex, const int vIndex, const int vID, const fbxsdk::FbxMesh& mesh)
	{
		using FlexKit::MeshUtilityFunctions::TokenList;

		int CPIndex     = mesh.GetPolygonVertex(pIndex, vIndex);
		auto NElement   = mesh.GetElementNormal(0);

        const auto MappingMode    = NElement->GetMappingMode();
        const auto referenceMode  = NElement->GetReferenceMode();

		switch (MappingMode)
		{
			case FbxGeometryElement::eByPolygonVertex:
			{
				auto ReferenceMode = NElement->GetReferenceMode();
				switch (referenceMode)
				{
				case FbxGeometryElement::eDirect:
				{
					return vID;
				}	break;
				case FbxGeometryElement::eIndexToDirect:
				{
					return NElement->GetIndexArray().GetAt(vID);
				}
				break;
				default:
					break; // other reference modes not shown here!
				}
			}	break;
			case FbxGeometryElement::eByControlPoint:
			{
				switch (NElement->GetReferenceMode())
				{
				case FbxGeometryElement::eDirect:
					return CPIndex;
				case FbxGeometryElement::eIndexToDirect:
					return NElement->GetIndexArray().GetAt(CPIndex);
					break;
				default:
					break;
				}
			}	break;
		}
		return -1;
	}


	/************************************************************************************************/


	size_t GetTexcordIndex(const int pIndex, const int vIndex, fbxsdk::FbxMesh& mesh)
	{
		const int CPIndex       = mesh.GetPolygonVertex(pIndex, vIndex);
		const auto UVElement    = mesh.GetElementUV(0);

		if ( !UVElement )
			return -1;

		switch (UVElement->GetMappingMode())
		{
			case FbxGeometryElement::eByControlPoint:
			{
				switch (UVElement->GetReferenceMode())
				{
				case FbxGeometryElement::eDirect:
					return CPIndex;
				case FbxGeometryElement::eIndexToDirect:
					return UVElement->GetIndexArray().GetAt(CPIndex);
					break;
				default:
					break;
				}
			}break;
				return -1;

			case FbxGeometryElement::eByPolygonVertex:
				switch (UVElement->GetReferenceMode())
				{
				case FbxGeometryElement::eDirect:
				case FbxGeometryElement::eIndexToDirect:
				{
					const int lTextureUVIndex = mesh.GetTextureUVIndex(pIndex, vIndex);
					return (size_t)lTextureUVIndex;
				}
				break;
				default:
					break; // other reference modes not shown here!
				}

			case FbxGeometryElement::eByPolygon: // doesn't make much sense for UVs
			case FbxGeometryElement::eAllSame:   // doesn't make much sense for UVs
			case FbxGeometryElement::eNone:      // doesn't make much sense for UVs
				return -1;
		}
		return -1;
	}


	/************************************************************************************************/


	size_t GetVertexIndex(const int pIndex, const int vIndex, const int vID, fbxsdk::FbxMesh& mesh)
	{ 
		return mesh.GetPolygonVertex(pIndex, vIndex); 
	}


	/************************************************************************************************/


	FBXSkinDeformer CreateSkin(const fbxsdk::FbxMesh& mesh)
	{	// Get Weights
		FBXSkinDeformer	Out = {};

		const auto DeformerCount  = mesh.GetDeformerCount();
		for ( int I = 0; I < DeformerCount; ++I )
		{
			fbxsdk::FbxStatus S;
			auto D		= mesh.GetDeformer(I, &S);
			auto Type	= D->GetDeformerType();
			switch ( Type ) 
			{
			case fbxsdk::FbxDeformer::EDeformerType::eSkin:
			{
				auto Skin			= (FbxSkin*)D;
				auto ClusterCount	= Skin->GetClusterCount();
				
				Out.bones.reserve(ClusterCount);
				for ( int II = 0; II < ClusterCount; ++II)
				{
					const auto Cluster			= Skin->GetCluster( II );
					const auto ClusterName		= Cluster->GetLink()->GetName();
					const auto CPIndices		= Cluster->GetControlPointIndices();
					const auto Weights			= Cluster->GetControlPointWeights();
					const size_t CPICount		= Cluster->GetControlPointIndicesCount();

					auto* const fbxBone			= Cluster->GetLink();
					const auto	fbxBoneAttrib	= fbxBone->GetNodeAttribute();

					FBXSkinDeformer::BoneWeights bone;
					bone.name = ClusterName;

					for ( size_t III = 0; III < CPICount; ++III )
					{
						bone.weights.push_back((float)Weights[III]);
						bone.indices.push_back(CPIndices[III]);
					}

					Out.bones.emplace_back(std::move(bone));
				}
			}	break;
			}
		}
		return Out;
	}


	/************************************************************************************************/


	FBXMeshDesc TranslateToTokens(fbxsdk::FbxMesh& mesh, SkeletonResource_ptr skeleton, bool SubDiv_Enabled)
	{
        using FlexKit::MeshUtilityFunctions::OBJ_Tools::AddNormalToken;
		using FlexKit::MeshUtilityFunctions::OBJ_Tools::AddIndexToken;
		using FlexKit::MeshUtilityFunctions::OBJ_Tools::AddTexCordToken;
		using FlexKit::MeshUtilityFunctions::OBJ_Tools::AddVertexToken;
		using FlexKit::MeshUtilityFunctions::OBJ_Tools::AddWeightToken;
		using fbxsdk::FbxLayerElement;

		FBXMeshDesc	out = {};
        out.tokens      = FlexKit::MeshUtilityFunctions::TokenList{ SystemAllocator };

		float3 MinV(INFINITY);
		float3 MaxV(-INFINITY);
		float  R = 0;

		{	// Get Vertices
			const size_t VCount   = mesh.GetControlPointsCount();
			const auto CPs        = mesh.mControlPoints;

			for (int itr = 0; itr < VCount; ++itr) 
			{
				auto V	= CPs[itr];
				auto V2 = TranslateToFloat3(V);

				MinV.x = min(V2.x, MinV.x);
				MinV.y = min(V2.y, MinV.y);
				MinV.z = min(V2.z, MinV.z);

				MaxV.x = max(V2.x, MaxV.x);
				MaxV.y = max(V2.y, MaxV.y);
				MaxV.z = max(V2.z, MaxV.z);
				R = max(R, V2.magnitude());

				AddVertexToken(V2, out.tokens);
			}
		}
		{	// Get Normals
			size_t	NormalCount = mesh.GetElementNormalCount();
			auto	Normals     = mesh.GetElementNormal();
			auto	Tangents    = mesh.GetElementTangent();
			size_t	VCount	    = mesh.GetPolygonVertexCount();
			size_t	NCount	    = Normals->GetDirectArray().GetCount();

            auto	mapping     = Normals->GetMappingMode();
            auto	reference   = Normals->GetReferenceMode();

            if (!Tangents && mesh.GetElementUV(0))
            {
                auto UVElement = mesh.GetElementUV(0);

                mesh.GenerateTangentsData(UVElement->GetName());
                Tangents = mesh.GetElementTangent();
            }

			if (NormalCount)
				out.Normals = true;

            if (Tangents)
            {
                for (int itr = 0; itr < NCount; ++itr)
                    AddNormalToken(
                        TranslateToFloat3(Normals->GetDirectArray().GetAt(itr)),
                        TranslateToFloat3(Tangents->GetDirectArray().GetAt(itr)),
                        out.tokens);

                out.Tangents    = true;
                out.Normals     = true;
            }
            else if(Normals)
            {
                for (int itr = 0; itr < NCount; ++itr)
                    AddNormalToken(
                        TranslateToFloat3(Normals->GetDirectArray().GetAt(itr)),
                        out.tokens);

                out.Tangents    = false;
                out.Normals     = true;
            }
		}
		{	// Get UV's
			auto UVElement = mesh.GetElementUV(0);
			if (UVElement) {
				out.UV = true;
				auto size = UVElement->GetDirectArray().GetCount();
				for (int I = 0; I < size; ++I) {
					auto UV = UVElement->GetDirectArray().GetAt(I);
					AddTexCordToken({ float(UV[0]), float(UV[1]), 0.0f }, out.tokens);
				}
			}
		}	// Get Use-able Deformers
		if (mesh.GetDeformerCount() && skeleton)
		{
			const size_t			VCount		= (size_t)mesh.GetControlPointsCount();
			std::vector<float4>		Weights;
			std::vector<uint4_16>	boneIndices;

			boneIndices.resize(VCount);
			Weights.resize(VCount);

			for (size_t I = 0; I < VCount; ++I)	Weights[I]		= { 0.0f, 0.0f, 0.0f, 0.0f };
			for (size_t I = 0; I < VCount; ++I)	boneIndices[I]	= { -1, -1, -1, -1 };

			auto skin		= CreateSkin(mesh);

			for (size_t I = 0; I < skin.bones.size(); ++I)
			{
				JointHandle Handle = skeleton->FindJoint(skin.bones[I].name);

				for (size_t II = 0; II < skin.bones[I].weights.size(); ++II)
				{
					size_t III = 0;
					for (; III < 4; ++III)
					{
						size_t Index = skin.bones[I].indices[II];
						if (boneIndices[Index][III] == -1)
							break;
					}

					if (III != 4) {
						Weights[skin.bones[I].indices[II]][III] = skin.bones[I].weights[II];
						boneIndices[skin.bones[I].indices[II]][III] = Handle;
					}
				}
			}

			for (size_t I = 0; I < VCount; ++I)
				AddWeightToken({ Weights[I].pFloats, boneIndices[I] }, out.tokens);

			out.Weights		= true;
			skin.indices	= std::move(boneIndices);
			out.skin		= std::move(skin);
		}
		{	// Calculate Indices
			const auto Normals	= mesh.GetElementNormal();
			const auto UVs		= mesh.GetElementUV(0);

			const size_t  NormalCount	= mesh.GetPolygonVertexCount();
			const size_t  TriCount	    = mesh.GetPolygonCount();

			size_t  faceCount	= 0;
			int     IndexCount	= 0;

			// Iterate through each Tri
			for (int triID = 0; triID < TriCount; ++triID)
			{	// Process each Vertex in tri
				const int size = mesh.GetPolygonSize(triID);
				const size_t NC = mesh.GetElementNormal()->GetDirectArray().GetCount();

				if (size == 3)
				{
					const size_t VertexIndex1 = GetVertexIndex(triID, 0, IndexCount, mesh);
					const size_t NormalIndex1 = out.Normals ? GetNormalIndex(triID, 0, IndexCount, mesh) : 0;
					const size_t UVCordIndex1 = out.UV ? GetTexcordIndex(triID, 0, mesh) : 0;

					const size_t VertexIndex2 = GetVertexIndex(triID, 1, IndexCount + 1, mesh);
					const size_t NormalIndex2 = out.Normals ? GetNormalIndex(triID, 1, IndexCount + 1, mesh) : 0;
					const size_t UVCordIndex2 = out.UV ? GetTexcordIndex(triID, 1, mesh) : 0;

					const size_t VertexIndex3 = GetVertexIndex(triID, 2, IndexCount + 2, mesh);
					const size_t NormalIndex3 = out.Normals ? GetNormalIndex(triID, 2, IndexCount + 2, mesh) : 0;
					const size_t UVCordIndex3 = out.UV ? GetTexcordIndex(triID, 2, mesh) : 0;

                    if (NormalIndex1 > NormalCount || NormalIndex2 > NormalCount || NormalIndex3 > NormalCount)
                        __debugbreak();

					AddIndexToken(VertexIndex1, NormalIndex1, NormalIndex1, UVCordIndex1, out.tokens);
                    AddIndexToken(VertexIndex3, NormalIndex3, NormalIndex3, UVCordIndex3, out.tokens);
                    AddIndexToken(VertexIndex2, NormalIndex2, NormalIndex2, UVCordIndex2, out.tokens);

					IndexCount += 3;
                    ++faceCount;
				}
				else if (size == 4)
				{	// Quads

					if constexpr(false)
					{
						auto VertexIndex1 = GetVertexIndex(triID, 0, IndexCount, mesh);
						auto NormalIndex1 = out.Normals ? GetNormalIndex(triID, 0, IndexCount, mesh) : 0;
						auto UVCordIndex1 = out.UV ? GetTexcordIndex(triID, 0, mesh) : 0;

						auto VertexIndex2 = GetVertexIndex(triID, 1, IndexCount + 1, mesh);
						auto NormalIndex2 = out.Normals ? GetNormalIndex(triID, 1, IndexCount + 1, mesh) : 0;
						auto UVCordIndex2 = out.UV ? GetTexcordIndex(triID, 1, mesh) : 0;

						auto VertexIndex3 = GetVertexIndex(triID, 2, IndexCount + 2, mesh);
						auto NormalIndex3 = out.Normals ? GetNormalIndex(triID, 2, IndexCount + 2, mesh) : 0;
						auto UVCordIndex3 = out.UV ? GetTexcordIndex(triID, 2, mesh) : 0;

						auto VertexIndex4 = GetVertexIndex(triID, 3, IndexCount + 3, mesh);
						auto NormalIndex4 = out.Normals ? GetNormalIndex(triID, 3, IndexCount + 3, mesh) : 0;
						auto UVCordIndex4 = out.UV ? GetTexcordIndex(triID, 3, mesh) : 0;

                        if (NormalIndex1 > NormalCount || NormalIndex2 > NormalCount || NormalIndex3 > NormalCount || NormalIndex4 > NormalCount)
                            __debugbreak();

						AddIndexToken(VertexIndex1, NormalIndex1, UVCordIndex1, out.tokens);
						AddIndexToken(VertexIndex2, NormalIndex2, UVCordIndex2, out.tokens);
						AddIndexToken(VertexIndex3, NormalIndex3, UVCordIndex3, out.tokens);
						AddIndexToken(VertexIndex4, NormalIndex4, UVCordIndex4, out.tokens);

						IndexCount += 4;
					}
					else
					{
						auto VertexIndex1 = GetVertexIndex(triID, 0, IndexCount, mesh);
						auto NormalIndex1 = out.Normals ? GetNormalIndex(triID, 0, IndexCount, mesh) : 0;
						auto UVCordIndex1 = out.UV ? GetTexcordIndex(triID, 0, mesh) : 0;

						auto VertexIndex2 = GetVertexIndex(triID, 1, IndexCount + 1, mesh);
						auto NormalIndex2 = out.Normals ? GetNormalIndex(triID, 1, IndexCount + 1, mesh) : 0;
						auto UVCordIndex2 = out.UV ? GetTexcordIndex(triID, 1, mesh) : 0;

						auto VertexIndex3 = GetVertexIndex(triID, 2, IndexCount + 2, mesh);
						auto NormalIndex3 = out.Normals ? GetNormalIndex(triID, 2, IndexCount + 2, mesh) : 0;
						auto UVCordIndex3 = out.UV ? GetTexcordIndex(triID, 2, mesh) : 0;

						auto VertexIndex4 = GetVertexIndex(triID, 3, IndexCount + 3, mesh);
						auto NormalIndex4 = out.Normals ? GetNormalIndex(triID, 3, IndexCount + 3, mesh) : 0;
						auto UVCordIndex4 = out.UV ? GetTexcordIndex(triID, 3, mesh) : 0;

                        if (NormalIndex1 > NormalCount || NormalIndex2 > NormalCount || NormalIndex3 > NormalCount || NormalIndex4 > NormalCount)
                            __debugbreak();

						AddIndexToken(VertexIndex1, NormalIndex1, NormalIndex1, UVCordIndex1, out.tokens);
                        AddIndexToken(VertexIndex3, NormalIndex3, NormalIndex3, UVCordIndex3, out.tokens);
                        AddIndexToken(VertexIndex2, NormalIndex2, NormalIndex2, UVCordIndex2, out.tokens);

                        AddIndexToken(VertexIndex1, NormalIndex1, NormalIndex1, UVCordIndex1, out.tokens);
                        AddIndexToken(VertexIndex4, NormalIndex4, NormalIndex4, UVCordIndex4, out.tokens);
                        AddIndexToken(VertexIndex3, NormalIndex3, NormalIndex3, UVCordIndex3, out.tokens);

						IndexCount += 4;
                        faceCount += 2;
					}

				}

				//if (SubDiv_Enabled)	AddPatchEndToken(out.tokens);
			}

			out.faceCount = faceCount;
		}

		out.MinV	= MinV;
		out.MaxV	= MaxV;
		out.R		= R;

		return out;
	}


	/************************************************************************************************/


	MeshResource_ptr CreateMeshResource(FbxMesh& mesh, const std::string& ID, const MetaDataList& metaData, const bool enableSubDiv)
	{
#if USING(TOOTLE)
		Memory  = SystemAllocator;// It will Leak, I know
		TempMem = SystemAllocator;
#endif

		using FlexKit::FillBufferView;
		using FlexKit::AnimationClip;
		using FlexKit::Skeleton;
		using MeshUtilityFunctions::BuildVertexBuffer;
		using MeshUtilityFunctions::CombinedVertexBuffer;
		using MeshUtilityFunctions::IndexList;
		using MeshUtilityFunctions::TokenList;
		using MeshUtilityFunctions::MeshBuildInfo;

		MeshResource_ptr meshOut		= std::make_shared<MeshResource>();
		SkeletonResource_ptr skeleton	= CreateSkeletonResource(mesh, ID, metaData);
		const auto transformedMesh		= TranslateToTokens(mesh, skeleton, enableSubDiv);
		auto optimizedbuffer            = MeshUtilityFunctions::BuildVertexBuffer(transformedMesh.tokens);

		static_vector<Pair<VERTEXBUFFER_TYPE, VERTEXBUFFER_FORMAT>> BuffersFound = {
			{VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32}
		};

        size_t i = 0;
        CreateBufferView(
            optimizedbuffer.points.data(), optimizedbuffer.points.size(), meshOut->buffers[i++],
            VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32, SystemAllocator);

        if (transformedMesh.UV)
            CreateBufferView(
                optimizedbuffer.textureCoordinates.data(), optimizedbuffer.textureCoordinates.size(), meshOut->buffers[i++],
                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32, SystemAllocator);

        if (transformedMesh.Normals)
            CreateBufferView(
                optimizedbuffer.normals.data(), optimizedbuffer.normals.size(), meshOut->buffers[i++],
                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32, SystemAllocator);

        if(transformedMesh.Tangents)
            CreateBufferView(
                optimizedbuffer.tangents.data(), optimizedbuffer.tangents.size(), meshOut->buffers[i++],
                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32, SystemAllocator);

		if (transformedMesh.Weights)
        {
            CreateBufferView(
                optimizedbuffer.jointWeights.data(), optimizedbuffer.tangents.size(), meshOut->buffers[i++],
                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32, SystemAllocator);

            CreateBufferView(
                optimizedbuffer.jointIndexes.data(), optimizedbuffer.tangents.size(), meshOut->buffers[i++],
                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R16G16B16A16, SystemAllocator);

		}

        CreateBufferView(
            optimizedbuffer.indexes.data(), optimizedbuffer.indexes.size(), meshOut->buffers[i++],
            VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32, SystemAllocator);


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

		meshOut->IndexBuffer_Idx	= BuffersFound.size();
		meshOut->IndexCount			= optimizedbuffer.IndexCount();
		meshOut->Skeleton			= skeleton;
		meshOut->AnimationData		= transformedMesh.Weights ? EAnimationData::EAD_Skin : 0;
		meshOut->Info.max			= transformedMesh.MaxV;
		meshOut->Info.min			= transformedMesh.MinV;
		meshOut->Info.r				= transformedMesh.R;
		meshOut->TriMeshID			= mesh.GetUniqueID();
		meshOut->ID					= ID;
		meshOut->SkeletonGUID		= skeleton ? skeleton->guid : -1;
		meshOut->BS					= optimizedbuffer.bs;
		meshOut->AABB				= optimizedbuffer.aabb;
        
		return meshOut;
	}


	/************************************************************************************************/
}// Namespace FlexKit
