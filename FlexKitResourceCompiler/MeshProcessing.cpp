

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


	uint4_16 Writeuint4(uint4_16 in)
	{
		return uint4_16{
			(unsigned short)((in[0] == -1) ? 0 : in[0]),
			(unsigned short)((in[1] == -1) ? 0 : in[1]),
			(unsigned short)((in[2] == -1) ? 0 : in[2]),
			(unsigned short)((in[3] == -1) ? 0 : in[3]) };
	}


	/************************************************************************************************/


	size_t GetNormalIndex(const int pIndex, const int vIndex, const int vID, fbxsdk::FbxMesh* Mesh)
	{
		using FlexKit::MeshUtilityFunctions::TokenList;

		int CPIndex     = Mesh->GetPolygonVertex(pIndex, vIndex);
		auto NElement   = Mesh->GetElementNormal(0);

		auto MappingMode = NElement->GetMappingMode();
		switch (NElement->GetMappingMode())
		{
			case FbxGeometryElement::eByPolygonVertex:
			{
				auto ReferenceMode = NElement->GetReferenceMode();
				switch (NElement->GetReferenceMode())
				{
				case FbxGeometryElement::eDirect:
				{
					return vID;
				}	break;
				case FbxGeometryElement::eIndexToDirect:
				{
					int index = NElement->GetIndexArray().GetAt(vID);
					return index;
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


	size_t GetTexcordIndex(const int pIndex, const int vIndex, fbxsdk::FbxMesh* Mesh)
	{
		FK_ASSERT(Mesh);

		int CPIndex = Mesh->GetPolygonVertex(pIndex, vIndex);
		auto UVElement = Mesh->GetElementUV(0);
		if ( !UVElement )
			return 0;

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
					const int lTextureUVIndex = Mesh->GetTextureUVIndex(pIndex, vIndex);
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


	size_t GetVertexIndex(int pIndex, int vIndex, int vID, fbxsdk::FbxMesh* Mesh)
	{ 
		FK_ASSERT(Mesh);
		return Mesh->GetPolygonVertex(pIndex, vIndex); 
	}


	/************************************************************************************************/


	FBXSkinDeformer CreateSkin(const fbxsdk::FbxMesh* Mesh)
	{	// Get Weights
		FBXSkinDeformer	Out = {};

		auto DeformerCount  = Mesh->GetDeformerCount();
		for ( int I = 0; I < DeformerCount; ++I )
		{
			fbxsdk::FbxStatus S;
			auto D		= Mesh->GetDeformer(I, &S);
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

					//if (fbxBoneAttrib->GetAttributeType() == FbxNodeAttribute::eSkeleton)
					//	auto Name	= Bone->GetName();

					FBXSkinDeformer::BoneWeights bone;
					bone.name			= ClusterName;

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


	FBXMeshDesc TranslateToTokens(fbxsdk::FbxMesh* Mesh, FlexKit::MeshUtilityFunctions::TokenList& TokensOut, Skeleton* S, bool SubDiv_Enabled)
	{
		using FlexKit::MeshUtilityFunctions::OBJ_Tools::AddNormalToken;
		using FlexKit::MeshUtilityFunctions::OBJ_Tools::AddIndexToken;
		using FlexKit::MeshUtilityFunctions::OBJ_Tools::AddTexCordToken;
		using FlexKit::MeshUtilityFunctions::OBJ_Tools::AddVertexToken;
		using FlexKit::MeshUtilityFunctions::OBJ_Tools::AddWeightToken;
		using FlexKit::MeshUtilityFunctions::OBJ_Tools::AddPatchBeginToken;
		using FlexKit::MeshUtilityFunctions::OBJ_Tools::AddPatchEndToken;
		using fbxsdk::FbxLayerElement;

		FBXMeshDesc	out = {};
		float3 MinV(INFINITY);
		float3 MaxV(-INFINITY);
		float  R = 0;

		{	// Get Vertices
			size_t VCount = Mesh->GetControlPointsCount();
			auto CPs = Mesh->mControlPoints;

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

				AddVertexToken(V2, TokensOut);
			}
		}
		{	// Get Normals
			size_t	NormalCount = Mesh->GetElementNormalCount();
			auto	Normals = Mesh->GetElementNormal();
			size_t	NCount	= Mesh->GetPolygonVertexCount();
			auto	mapping = Normals->GetMappingMode();

			if (NormalCount)
				out.Normals = true;

			for (int itr = 0; itr < NCount; ++itr)
			{
				auto N = Normals->GetDirectArray().GetAt(itr);
				AddNormalToken(TranslateToFloat3(N), TokensOut);
			}
		}
		{	// Get UV's
			auto UVElement = Mesh->GetElementUV(0);
			if (UVElement) {
				out.UV = true;
				auto size = UVElement->GetDirectArray().GetCount();
				for (int I = 0; I < size; ++I) {
					auto UV = UVElement->GetDirectArray().GetAt(I);
					AddTexCordToken({ float(UV[0]), float(UV[1]), 0.0f }, TokensOut);
				}
			}
		}	// Get Use-able Deformers
		if (Mesh->GetDeformerCount() && S)
		{
			const size_t			VCount		= size_t(Mesh->GetControlPointsCount());
			std::vector<float4>		Weights;
			std::vector<uint4_32>	boneIndices;

			boneIndices.resize(VCount);
			Weights.resize(VCount);

			for (size_t I = 0; I < VCount; ++I)	Weights[I]		= { 0.0f, 0.0f, 0.0f, 0.0f };
			for (size_t I = 0; I < VCount; ++I)	boneIndices[I]	= { -1, -1, -1, -1 };

			auto skin		= CreateSkin(Mesh);

			for (size_t I = 0; I < skin.bones.size(); ++I)
			{
				JointHandle Handle = S->FindJoint(skin.bones[I].name);

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
				AddWeightToken({ Weights[I].pFloats, boneIndices[I] }, TokensOut);

			out.Weights		= true;
			skin.indices	= std::move(boneIndices);
			out.Skin		= std::move(skin);
		}
		{	// Calculate Indices
			auto Normals	= Mesh->GetElementNormal();
			auto UVs		= Mesh->GetElementUV(0);

			size_t  NormalCount	= Mesh->GetElementNormalCount();
			size_t  TriCount		= Mesh->GetPolygonCount();
			size_t  FaceCount	= 0;
			int     IndexCount	= 0;

			// Iterate through each Tri
			for (int I = 0; I < TriCount; ++I)
			{	// Process each Vertex in tri
				const int size = Mesh->GetPolygonSize(I);
				++FaceCount;

				size_t	NC = Mesh->GetElementNormal()->GetDirectArray().GetCount();
				if (SubDiv_Enabled)	
					AddPatchBeginToken(TokensOut);

				if (size == 3)
				{
					size_t VertexIndex1 = GetVertexIndex(I, 0, IndexCount, Mesh);
					size_t NormalIndex1 = out.Normals ? GetNormalIndex(I, 0, IndexCount, Mesh) : 0;
					size_t UVCordIndex1 = out.UV ? GetTexcordIndex(I, 0, Mesh) : 0;

					size_t VertexIndex2 = GetVertexIndex(I, 1, IndexCount + 1, Mesh);
					size_t NormalIndex2 = out.Normals ? GetNormalIndex(I, 1, IndexCount + 1, Mesh) : 0;
					size_t UVCordIndex2 = out.UV ? GetTexcordIndex(I, 1, Mesh) : 0;

					size_t VertexIndex3 = GetVertexIndex(I, 2, IndexCount + 2, Mesh);
					size_t NormalIndex3 = out.Normals ? GetNormalIndex(I, 2, IndexCount + 2, Mesh) : 0;
					size_t UVCordIndex3 = out.UV ? GetTexcordIndex(I, 2, Mesh) : 0;

					AddIndexToken(VertexIndex1, NormalIndex1, UVCordIndex1, TokensOut);
					AddIndexToken(VertexIndex3, NormalIndex3, UVCordIndex3, TokensOut);
					AddIndexToken(VertexIndex2, NormalIndex2, UVCordIndex2, TokensOut);

					IndexCount += 3;
				}
				else if (size == 4)
				{	// Quads
					bool QuadsEnabled = false | SubDiv_Enabled;
					if (!QuadsEnabled) {
						std::cout << "Quads Disabled!\n";
					}

					if(QuadsEnabled)
					{
						auto VertexIndex1 = GetVertexIndex(I, 0, IndexCount, Mesh);
						auto NormalIndex1 = out.Normals ? GetNormalIndex(I, 0, IndexCount, Mesh) : 0;
						auto UVCordIndex1 = out.UV ? GetTexcordIndex(I, 0, Mesh) : 0;

						auto VertexIndex2 = GetVertexIndex(I, 1, IndexCount + 1, Mesh);
						auto NormalIndex2 = out.Normals ? GetNormalIndex(I, 1, IndexCount + 1, Mesh) : 0;
						auto UVCordIndex2 = out.UV ? GetTexcordIndex(I, 1, Mesh) : 0;

						auto VertexIndex3 = GetVertexIndex(I, 2, IndexCount + 2, Mesh);
						auto NormalIndex3 = out.Normals ? GetNormalIndex(I, 2, IndexCount + 2, Mesh) : 0;
						auto UVCordIndex3 = out.UV ? GetTexcordIndex(I, 2, Mesh) : 0;

						auto VertexIndex4 = GetVertexIndex(I, 3, IndexCount + 3, Mesh);
						auto NormalIndex4 = out.Normals ? GetNormalIndex(I, 3, IndexCount + 3, Mesh) : 0;
						auto UVCordIndex4 = out.UV ? GetTexcordIndex(I, 3, Mesh) : 0;

						AddIndexToken(VertexIndex1, NormalIndex1, 0, TokensOut);
						AddIndexToken(VertexIndex2, NormalIndex2, 0, TokensOut);
						AddIndexToken(VertexIndex3, NormalIndex3, 0, TokensOut);
						AddIndexToken(VertexIndex4, NormalIndex4, 0, TokensOut);

						IndexCount += 4;
					}
					else
					{
						auto VertexIndex1 = GetVertexIndex(I, 0, IndexCount, Mesh);
						auto NormalIndex1 = out.Normals ? GetNormalIndex(I, 0, IndexCount, Mesh) : 0;
						auto UVCordIndex1 = out.UV ? GetTexcordIndex(I, 0, Mesh) : 0;

						auto VertexIndex2 = GetVertexIndex(I, 1, IndexCount + 1, Mesh);
						auto NormalIndex2 = out.Normals ? GetNormalIndex(I, 1, IndexCount + 1, Mesh) : 0;
						auto UVCordIndex2 = out.UV ? GetTexcordIndex(I, 1, Mesh) : 0;

						auto VertexIndex3 = GetVertexIndex(I, 2, IndexCount + 2, Mesh);
						auto NormalIndex3 = out.Normals ? GetNormalIndex(I, 2, IndexCount + 2, Mesh) : 0;
						auto UVCordIndex3 = out.UV ? GetTexcordIndex(I, 2, Mesh) : 0;

						auto VertexIndex4 = GetVertexIndex(I, 3, IndexCount + 3, Mesh);
						auto NormalIndex4 = out.Normals ? GetNormalIndex(I, 3, IndexCount + 3, Mesh) : 0;
						auto UVCordIndex4 = out.UV ? GetTexcordIndex(I, 3, Mesh) : 0;

						AddIndexToken(VertexIndex1, NormalIndex1, 0, TokensOut);
						AddIndexToken(VertexIndex2, NormalIndex2, 0, TokensOut);
						AddIndexToken(VertexIndex3, NormalIndex3, 0, TokensOut);

						IndexCount += 3;
					}

				}

				if (SubDiv_Enabled)	AddPatchEndToken(TokensOut);
			}

			out.FaceCount = FaceCount;
		}

		out.MinV	= MinV;
		out.MaxV	= MaxV;
		out.R		= R;

		return out;
	}


	/************************************************************************************************/


	MeshResource_ptr CompileMeshResource(FbxMesh* Mesh, const std::string& ID, MetaDataList& MD, bool EnableSubDiv = false)
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

		SkeletonResource_ptr skeleton	= LoadSkeletonResource(Mesh, ID, MD);
		TokenList Tokens				= TokenList{ SystemAllocator };
		auto MeshInfo					= TranslateToTokens(Mesh, Tokens, &skeleton.get()->CreateSkeletonProxy());

		CombinedVertexBuffer	CVB	{ SystemAllocator };
		IndexList				IB	{ SystemAllocator, MeshInfo.FaceCount * 4 };

		auto BuildRes = MeshUtilityFunctions::BuildVertexBuffer(Tokens, CVB, IB, SystemAllocator, SystemAllocator, MeshInfo.Weights);
		FK_ASSERT(BuildRes.V1 == true, "Mesh Failed to Build");

		size_t IndexCount  = GetByType<MeshBuildInfo>(BuildRes).IndexCount;
		size_t VertexCount = GetByType<MeshBuildInfo>(BuildRes).VertexCount;

		static_vector<Pair<VERTEXBUFFER_TYPE, VERTEXBUFFER_FORMAT>> BuffersFound = {
			{VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32}
		};

		if ((MeshInfo.UV))
			BuffersFound.push_back({ VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32 });

		if ((MeshInfo.Normals))
			BuffersFound.push_back({ VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32 });

		if ((MeshInfo.Weights)) {
			BuffersFound.push_back({ VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32 });
			BuffersFound.push_back({ VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R16G16B16A16 });
		}

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


		for (size_t i = 0; i < BuffersFound.size(); ++i) 
		{
			CreateBufferView(VertexCount, SystemAllocator, meshOut->buffers[i], (VERTEXBUFFER_TYPE)BuffersFound[i], (VERTEXBUFFER_FORMAT)BuffersFound[i]);

			switch ((FlexKit::VERTEXBUFFER_TYPE)BuffersFound[i])
			{
			case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION:
				FillBufferView(&CVB, CVB.size(), meshOut->buffers[i], WriteVertex, FetchVertexPOS);		break;
			case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL:
				FillBufferView(&CVB, CVB.size(), meshOut->buffers[i], WriteVertex, FetchVertexNormal);	break;
			case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV:
				FillBufferView(&CVB, CVB.size(), meshOut->buffers[i], WriteUV, FetchVertexUV);			break;
			case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT:
				FillBufferView(&CVB, CVB.size(), meshOut->buffers[i], WriteVertex, FetchFloat3ZERO);	break;
			case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1:
				FillBufferView(&CVB, CVB.size(), meshOut->buffers[i], WriteVertex, FetchWeights);		break;
			case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2:
				FillBufferView(&CVB, CVB.size(), meshOut->buffers[i], Writeuint4, FetchWeightIndices);	break;
			default:
				break;
			}
		}

		CreateBufferView(IB.size(), SystemAllocator, meshOut->buffers[BuffersFound.size()], VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32);
		FillBufferView(&IB, IB.size(), meshOut->buffers[BuffersFound.size()], WriteIndex, FetchIndex2);

		//Calculate Bounding Sphere
		const float3	BSCenter = (MeshInfo.MaxV + MeshInfo.MinV) / 2;
		float			BSRadius = 0;//(MeshInfo.MaxV - MeshInfo.MinV).magnitude() / 2;
		const float		BSR_Ref  = (MeshInfo.MaxV - MeshInfo.MinV).magnitude() / 2;

		ProcessBufferView(
			&CVB, 
			(uint32_t)CVB.size(),
			WriteVertex, 
			FetchVertexPOS, 
			[&](auto V)
			{
				V[3]		= 0;
				auto Temp	= (V - BSCenter);
				auto L		= Temp.magnitude();
				BSRadius	= (BSRadius > L)? BSRadius : L;
			});

		//Calculate AABB
		FlexKit::AABB AxisAlignedBoundingBox;
		AxisAlignedBoundingBox.BottomLeft	= MeshInfo.MinV;
		AxisAlignedBoundingBox.TopRight		= MeshInfo.MaxV;

		meshOut->IndexBuffer_Idx	= BuffersFound.size();
		meshOut->IndexCount			= IndexCount;
		meshOut->Skeleton			= skeleton;
		meshOut->AnimationData		= MeshInfo.Weights ? EAnimationData::EAD_Skin : 0;
		meshOut->Info.max			= MeshInfo.MaxV;
		meshOut->Info.min			= MeshInfo.MinV;
		meshOut->Info.r				= MeshInfo.R;
		meshOut->TriMeshID			= Mesh->GetUniqueID();
		meshOut->ID					= ID;
		meshOut->SkeletonGUID		= skeleton ? skeleton->guid : -1;
		meshOut->BS					= BoundingSphere(BSCenter, BSRadius);
		meshOut->AABB				= AxisAlignedBoundingBox;

		return meshOut;
	}


	/************************************************************************************************/
}// Namespace FlexKit
