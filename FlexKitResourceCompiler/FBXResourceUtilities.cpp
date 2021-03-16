#include "stdafx.h"
#include "buildsettings.h"

#include "Common.h"
#include "FBXResourceUtilities.h"

#include "AnimationUtilities.h"

#include "Scenes.h"
#include "MeshProcessing.h"

#include "fbxsdk.h"

namespace FlexKit::ResourceBuilder
{   /************************************************************************************************/

    
	size_t          GetNormalIndex	    (const int pIndex, const int vIndex, const int vID, fbxsdk::FbxMesh* Mesh);
	size_t          GetTexcordIndex	    (const int pIndex, const int vIndex, fbxsdk::FbxMesh* Mesh);
	size_t          GetVertexIndex	    (const int pIndex, const int vIndex, const int vID, fbxsdk::FbxMesh* Mesh);
	MeshDesc		TranslateToTokens	(fbxsdk::FbxMesh* Mesh, MeshUtilityFunctions::TokenList& TokensOut, SkeletonResource_ptr S = nullptr, bool SubDiv_Enabled = false);
	SkinDeformer	CreateSkin			(const fbxsdk::FbxMesh* Mesh);


    /************************************************************************************************/


    inline float3 TranslateToFloat3(const FbxVector4& in)
    {
        return float3(
            (float)in.mData[0],
            (float)in.mData[1],
            (float)in.mData[2]);
    }


    inline float3 TranslateToFloat3(const FbxDouble3& in)
    {
        return float3(
            (float)in.mData[0],
            (float)in.mData[1],
            (float)in.mData[2]);
    }


    inline float4 TranslateToFloat4(const FbxVector4& in)
    {
        return float4(
            (float)in.mData[0],
            (float)in.mData[1],
            (float)in.mData[2],
            (float)in.mData[3]);
    }


    /************************************************************************************************/


    inline XMMATRIX FBXMATRIX_2_XMMATRIX(const FbxAMatrix& AM)
    {
        XMMATRIX M; // Xmmatrix is Filled with 32-bit floats
        for (uint32_t I = 0; I < 4; ++I)
            for (uint32_t II = 0; II < 4; ++II)
                M.r[I].m128_f32[II] = (float)AM[I][II];

        return M;
    }


    inline FbxAMatrix XMMATRIX_2_FBXMATRIX(const XMMATRIX& M)
    {
        FbxAMatrix AM; // FBX Matrix is filled with 64-bit floats
        for (uint32_t I = 0; I < 4; ++I)
            for (uint32_t II = 0; II < 4; ++II)
                AM[I][II] = M.r[I].m128_f32[II];

        return AM;
    }


    inline float4x4 FBXMATRIX_2_FLOAT4X4(const FbxAMatrix& AM)
    {
        return FlexKit::XMMatrixToFloat4x4(FBXMATRIX_2_XMMATRIX(AM));
    }


    /************************************************************************************************/


    FbxAMatrix GetGeometryTransformation(FbxNode* inNode)
    {
        const FbxVector4 lT = inNode->GetGeometricTranslation(FbxNode::eSourcePivot);
        const FbxVector4 lR = inNode->GetGeometricRotation(FbxNode::eSourcePivot);
        const FbxVector4 lS = inNode->GetGeometricScaling(FbxNode::eSourcePivot);

        return FbxAMatrix(lT, lR, lS);
    }


    /************************************************************************************************/


    Pair<bool, fbxsdk::FbxScene*>   LoadFBXScene(std::string file, fbxsdk::FbxManager* lSdkManager, fbxsdk::FbxIOSettings* settings);
    ResourceList                    CreateSceneFromFBXFile(fbxsdk::FbxScene* scene, const CompileSceneFromFBXFile_DESC& Desc, const MetaDataList& metaData);


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


    uint32_t GetNormalIndex(const int pIndex, const int vIndex, const int vID, const fbxsdk::FbxMesh& mesh)
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


	uint32_t GetTexcordIndex(const int pIndex, const int vIndex, fbxsdk::FbxMesh& mesh)
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


    uint32_t GetVertexIndex(const int pIndex, const int vIndex, const int vID, fbxsdk::FbxMesh& mesh)
	{ 
		return (uint32_t)mesh.GetPolygonVertex(pIndex, vIndex);
	}


	/************************************************************************************************/


	SkinDeformer CreateSkin(const fbxsdk::FbxMesh& mesh)
	{	// Get Weights
		SkinDeformer	Out = {};

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

					SkinDeformer::BoneWeights bone;
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


	MeshDesc TranslateToTokens(fbxsdk::FbxMesh& mesh, SkeletonResource_ptr skeleton, bool SubDiv_Enabled)
	{
        using FlexKit::MeshUtilityFunctions::OBJ_Tools::AddNormalToken;
		using FlexKit::MeshUtilityFunctions::OBJ_Tools::AddIndexToken;
		using FlexKit::MeshUtilityFunctions::OBJ_Tools::AddTexCordToken;
		using FlexKit::MeshUtilityFunctions::OBJ_Tools::AddVertexToken;
		using FlexKit::MeshUtilityFunctions::OBJ_Tools::AddWeightToken;
		using fbxsdk::FbxLayerElement;

		MeshDesc out = {};
        out.skeleton = skeleton;

        FlexKit::MeshUtilityFunctions::TokenList tokens{ SystemAllocator };

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

				MinV.x = Min(V2.x, MinV.x);
				MinV.y = Min(V2.y, MinV.y);
				MinV.z = Min(V2.z, MinV.z);

				MaxV.x = Max(V2.x, MaxV.x);
				MaxV.y = Max(V2.y, MaxV.y);
				MaxV.z = Max(V2.z, MaxV.z);
				R = Max(R, V2.magnitude());

				AddVertexToken(V2, tokens);
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
                        tokens);

                out.Tangents    = true;
                out.Normals     = true;
            }
            else if(Normals)
            {
                for (int itr = 0; itr < NCount; ++itr)
                    AddNormalToken(
                        TranslateToFloat3(Normals->GetDirectArray().GetAt(itr)),
                        tokens);

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
					AddTexCordToken({ float(UV[0]), float(UV[1]), 0.0f }, tokens);
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
				AddWeightToken({ Weights[I].pFloats, boneIndices[I] }, tokens);

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
					const uint32_t VertexIndex1 = GetVertexIndex(triID, 0, IndexCount, mesh);
                    const uint32_t NormalIndex1 = out.Normals ? GetNormalIndex(triID, 0, IndexCount, mesh) : 0;
                    const uint32_t UVCordIndex1 = out.UV ? GetTexcordIndex(triID, 0, mesh) : 0;

					const uint32_t VertexIndex2 = GetVertexIndex(triID, 1, IndexCount + 1, mesh);
                    const uint32_t NormalIndex2 = out.Normals ? GetNormalIndex(triID, 1, IndexCount + 1, mesh) : 0;
                    const uint32_t UVCordIndex2 = out.UV ? GetTexcordIndex(triID, 1, mesh) : 0;

					const uint32_t VertexIndex3 = GetVertexIndex(triID, 2, IndexCount + 2, mesh);
                    const uint32_t NormalIndex3 = out.Normals ? GetNormalIndex(triID, 2, IndexCount + 2, mesh) : 0;
                    const uint32_t UVCordIndex3 = out.UV ? GetTexcordIndex(triID, 2, mesh) : 0;

					AddIndexToken(VertexIndex1, NormalIndex1, NormalIndex1, UVCordIndex1, tokens);
                    AddIndexToken(VertexIndex3, NormalIndex3, NormalIndex3, UVCordIndex3, tokens);
                    AddIndexToken(VertexIndex2, NormalIndex2, NormalIndex2, UVCordIndex2, tokens);

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

						AddIndexToken(VertexIndex1, NormalIndex1, UVCordIndex1, tokens);
						AddIndexToken(VertexIndex2, NormalIndex2, UVCordIndex2, tokens);
						AddIndexToken(VertexIndex3, NormalIndex3, UVCordIndex3, tokens);
						AddIndexToken(VertexIndex4, NormalIndex4, UVCordIndex4, tokens);

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

						AddIndexToken(VertexIndex1, NormalIndex1, NormalIndex1, UVCordIndex1, tokens);
                        AddIndexToken(VertexIndex3, NormalIndex3, NormalIndex3, UVCordIndex3, tokens);
                        AddIndexToken(VertexIndex2, NormalIndex2, NormalIndex2, UVCordIndex2, tokens);

                        AddIndexToken(VertexIndex1, NormalIndex1, NormalIndex1, UVCordIndex1, tokens);
                        AddIndexToken(VertexIndex4, NormalIndex4, NormalIndex4, UVCordIndex4, tokens);
                        AddIndexToken(VertexIndex3, NormalIndex3, NormalIndex3, UVCordIndex3, tokens);

						IndexCount += 4;
                        faceCount += 2;
					}

				}

				//if (SubDiv_Enabled)	AddPatchEndToken(out.tokens);
			}

            out.tokens      = tokens;
            out.faceCount   = faceCount;
		}

		out.MinV	= MinV;
		out.MaxV	= MaxV;
		out.R		= R;

		return out;
	}


	/************************************************************************************************/


    fbxsdk::FbxNode* FindSkeletonRoot(const fbxsdk::FbxMesh& mesh)
    {
        auto DeformerCount  = mesh.GetDeformerCount();
        for (int32_t I = 0; I < DeformerCount; ++I)
        {
            fbxsdk::FbxStatus S;
            const auto D	= mesh.GetDeformer(I, &S);  FK_ASSERT(D != nullptr);
            const auto Type	= D->GetDeformerType();

            switch (Type)
            {
            case fbxsdk::FbxDeformer::EDeformerType::eSkin:
            {
                const auto Skin			= (FbxSkin*)D;
                const auto ClusterCount	= Skin->GetClusterCount();

                const auto Cluster		= Skin->GetCluster(0);
                const auto CLBone		= Cluster->GetLink();
                const auto CLBoneAttrib	= CLBone->GetNodeAttribute();
                const auto CLBoneName	= CLBone->GetName();
                auto* I				    = CLBone;

                while (true)
                {
                    if (I->GetParent()->GetSkeleton())
                        I = I->GetParent();
                    else
                        return I;
                }

            }	break;
            default:
                break;
            }
        }

        return nullptr;
    }

    
    /************************************************************************************************/


    void GetJointTransforms(JointList& Out, const FbxMesh& mesh)
    {
        using DirectX::XMMatrixRotationQuaternion;

        FbxAMatrix Identity;
        Identity.FbxAMatrix::SetIdentity();

        const auto DeformerCount = mesh.GetDeformerCount();
        for (int I = 0; I < DeformerCount; ++I)
        {
            const auto D = mesh.GetDeformer(I);
            if (D->GetDeformerType() == FbxDeformer::EDeformerType::eSkin)
            {
                const auto Skin = (FbxSkin*)D;
                auto root = Skin->GetCluster(0);
                auto rootLinkMatrix = FbxAMatrix{};

                FbxAMatrix G = GetGeometryTransformation(root->GetLink());
                root->GetTransformLinkMatrix(rootLinkMatrix);

                for (int II = 0; Skin->GetClusterCount() > II; ++II)
                {
                    const auto Cluster  = Skin->GetCluster(II);
                    const auto ID       = Cluster->GetLink()->GetName();

                    JointHandle Handle              = GetJoint(Out, ID);
                    FbxAMatrix G                    = GetGeometryTransformation(Cluster->GetLink());
                    FbxAMatrix transformMatrix;
                    FbxAMatrix transformLinkMatrix;

                    Cluster->GetTransformLinkMatrix(transformLinkMatrix);
                    Cluster->GetTransformMatrix(transformMatrix);

                    const FbxAMatrix globalBindposeInverseMatrix = transformLinkMatrix.Inverse() * transformMatrix * G;
                    const XMMATRIX Inverse  = FBXMATRIX_2_XMMATRIX(globalBindposeInverseMatrix);

                    Out[Handle].Inverse = Inverse;
                }
            }
        }
    }


    /************************************************************************************************/


    SkeletonJointAnimation GetJointAnimation(const FbxNode* node, const AnimationProperties properties = {})
    {
        const auto Scene            = node->GetScene();
        const auto AnimationStack   = Scene->GetCurrentAnimationStack();
        const auto TakeInfo         = Scene->GetTakeInfo(AnimationStack->GetName());
        const auto Begin            = TakeInfo->mLocalTimeSpan.GetStart();
        const auto End              = TakeInfo->mLocalTimeSpan.GetStop();
        const auto Duration         = (End - Begin).GetFrameCount(FbxTime::ConvertFrameRateToTimeMode(properties.frameRate));
        const auto FrameRate        = properties.frameRate;

        SkeletonJointAnimation A;
        A.FPS = (uint32_t)properties.frameRate;
        A.FrameCount = Duration;
        A.Poses.resize(Duration);

        for (size_t I = 0; I < size_t(Duration); ++I)
        {
            FbxTime	CurrentFrame;
            CurrentFrame.SetFrame(I, FbxTime::ConvertFrameRateToTimeMode(properties.frameRate));
            A.Poses[I].JPose = GetPose(FBXMATRIX_2_XMMATRIX(const_cast<FbxNode*>(node)->EvaluateLocalTransform(CurrentFrame)));
        }

        return A;
    }


    /************************************************************************************************/


    void FindAllJoints(JointList& Out, const FbxNode* N, const size_t Parent = 0xFFFF)
    {
        if (N->GetNodeAttribute() && N->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton )
        {
            fbxsdk::FbxSkeleton* Sk = (fbxsdk::FbxSkeleton*)N->GetNodeAttribute();
            
            const int JointIndex = (int)Out.size();
            const int ChildCount = N->GetChildCount();

            SkeletonJoint NewJoint;
            NewJoint.mID	    = N->GetName();
            NewJoint.mParent	= JointHandle(Parent);

            Out.emplace_back(SkeletonJointInfo{ { NewJoint }, GetJointAnimation(N), DirectX::XMMatrixIdentity() });

            for ( int I = 0; I < ChildCount; ++I )
                FindAllJoints(Out, N->GetChild( I ), JointIndex);
        }
    }


    /************************************************************************************************/


    SkeletonResource_ptr CreateSkeletonResource(FbxMesh& mesh, const std::string& parentID, const MetaDataList& MD)
    {
        using FlexKit::AnimationClip;
        using FlexKit::Skeleton;

        auto skeleton = std::make_unique<SkeletonResource>();

        // Gather MetaData
        auto Related		= FindRelatedMetaData(MD, MetaData::EMETA_RECIPIENT_TYPE::EMR_SKELETON, parentID);
        auto SkeletonInfo	= GetSkeletonMetaData(MD);
        skeleton->metaData  = Related;

        auto root	= FindSkeletonRoot(mesh);
        if (!root || !SkeletonInfo)
            return nullptr;

        std::vector<SkeletonJointInfo> joints;
        FindAllJoints		(joints, root);
        GetJointTransforms	(joints, mesh);

        for (auto j : joints)
            skeleton->AddJoint(j.Joint, j.Inverse);
        
        const std::string ID = SkeletonInfo->SkeletonID;
        skeleton->guid  = SkeletonInfo->SkeletonGUID;
        skeleton->ID    = ID;

        for (size_t joint = 0; joint < joints.size(); ++joint)
            skeleton->joints[joint].mID = skeleton->joints[joint].mID;

        const CutList cuts = GetAnimationCuts(MD, ID);
        for(const auto& cut : cuts)
        {
            const auto begin	= (size_t)(cut.T_Start);
            const auto end		= (size_t)(cut.T_End);

            /*
            AnimationClipResource clip;
            clip.FPS		= 60;
            clip.mID		= cut.ID;
            clip.guid		= cut.guid;
            clip.isLooping	= false;

            const size_t clipFrameCount = end - begin;

            for (size_t frame = 0; frame < clipFrameCount; ++frame)
            {
                AnimationKeyFrame keyFrame;
                keyFrame.FrameNumber = frame;

                for (size_t jointIdx = 0; jointIdx < joints.size(); ++jointIdx)
                {
                    const auto& joint       = joints[jointIdx];
                    const auto  inverse		= DirectX::XMMatrixInverse(nullptr, Float4x4ToXMMATIRX(GetPoseTransform(skeleton->jointPoses[jointIdx])));
                    const auto  pose		= Float4x4ToXMMATIRX(GetPoseTransform(joints[jointIdx].Animation.Poses[(frame + begin) % (joint.Animation.FrameCount - 1)].JPose));
                    const auto  localPose   = GetPose(pose * inverse);

                    keyFrame.AddJointPose(JointHandle(jointIdx), localPose);
                }

                clip.AddKeyFrame(keyFrame, frame * 1.0 / 60.0f);
            }
            */
            //clip.Compress();
        }

        return skeleton;
    }


    /************************************************************************************************/


    inline uint32_t		FetchIndex(uint32_t itr, fbxsdk::FbxMesh* Mesh) { return Mesh->GetPolygonVertex(itr / 3, itr % 3); }


    /************************************************************************************************/


    MeshResource_ptr FindGeoByID(GeometryList& geometry, size_t ID)
    {
        auto res = std::find_if(begin(geometry), end(geometry), [&](auto& v) { return v->TriMeshID == ID; });

        return (res != geometry.end()) ? *res : MeshResource_ptr{ nullptr };
    }


    /************************************************************************************************/


    void GatherAllGeometry(
        GeometryList&			geometry,
        fbxsdk::FbxNode*		node, 
        IDTranslationTable&	    Table, 
        const MetaDataList&		MD			= MetaDataList{}, 
        bool					subDiv		= false)
    {
        using FlexKit::AnimationClip;
        using FlexKit::Skeleton;

        auto AttributeCount = node->GetNodeAttributeCount();

        for (int itr = 0; itr < AttributeCount; ++itr)
        {
            auto Attr		= node->GetNodeAttributeByIndex(itr);
            auto nodeName	= node->GetName();

            switch (Attr->GetAttributeType())
            {
            case fbxsdk::FbxNodeAttribute::EType::eMesh:
            {
                const char* MeshName    = node->GetName();
                auto Mesh		        = (fbxsdk::FbxMesh*)Attr;
                bool found		        = false;
                bool LoadMesh	        = false;
                size_t uniqueID	        = (size_t)Mesh->GetUniqueID();
                auto Geo		        = FindGeoByID(geometry, uniqueID);

                MetaDataList RelatedMetaData;

    #if USING(RESCOMPILERVERBOSE)
                std::cout << "Found Mesh: " << MeshName << "\n";
    #endif
                RelatedMetaData = FindRelatedMetaData(MD, MetaData::EMETA_RECIPIENT_TYPE::EMR_MESH, MeshName);

                if(!RelatedMetaData.size())
                    LoadMesh = true;

                const auto MeshInfo	= GetMeshMetaData(RelatedMetaData);
                const auto Name		= MeshInfo ? MeshInfo->MeshID : Mesh->GetName();

                if (!IDPresentInTable(Mesh->GetUniqueID(), Table))
                {
                    std::cout << "Building Mesh: " << MeshName << "\n";

                    auto skeleton       = CreateSkeletonResource(*Mesh, Name, MD);
                    MeshDesc meshDesc   = TranslateToTokens(*Mesh, skeleton, false);

                    std::vector<LODLevel> lods = {
                        LODLevel{
                            .subMeshs = { std::move(meshDesc) }
                        }
                    };

                    const FbxAMatrix transform  = node->EvaluateGlobalTransform();
                    MeshResource_ptr resource   = CreateMeshResource(lods, Name, MD, false);

                    resource->BakeTransform(FBXMATRIX_2_FLOAT4X4((transform)));
                    
                    if(MeshInfo)
                    {
                        resource->TriMeshID		= MeshInfo->guid;
                        resource->ID			= MeshInfo->MeshID;
                    }
                    else
                    {
                        resource->TriMeshID		= CreateRandomID();
                        resource->ID			= Mesh->GetName();
                    }

                    Table.push_back({ Mesh->GetUniqueID(), resource->TriMeshID });

                    if constexpr (USING(RESCOMPILERVERBOSE))
                        std::cout << "Compiled Resource: " << Name << "\n";

                    geometry.push_back(resource);
                }
            }	break;
            }
        }

        const size_t NodeCount = node->GetChildCount();
        for(int itr = 0; itr < NodeCount; ++itr)
            GatherAllGeometry(geometry, node->GetChild(itr), Table, MD, subDiv);
    }


    /************************************************************************************************/


    ResourceList GatherSceneResources(fbxsdk::FbxScene* S, physx::PxCooking* Cooker, IDTranslationTable& Table, const bool LoadSkeletalData = false, const MetaDataList& MD = MetaDataList{}, const bool subDivEnabled = false)
    {
        GeometryList geometryFound;
        ResourceList resources;

        GatherAllGeometry(geometryFound, S->GetRootNode(), Table, MD, subDivEnabled);

#if USING(RESCOMPILERVERBOSE)
        std::cout << "CompileAllGeometry Compiled " << geometryFound.size() << " Resources\n";
#endif
        for (auto geometry : geometryFound)
        {
            resources.push_back(geometry);

            if (geometry->Skeleton)
            {
                resources.push_back(geometry->Skeleton);

                // TODO: scan metadata for animation clips, then add those to the resource list, everything in the resource list gets output in the resource file
                auto pred = [&](MetaData_ptr metaInfo) -> bool
                {
                    return
                        metaInfo->UserType == MetaData::EMETA_RECIPIENT_TYPE::EMR_SKELETON &&
                        metaInfo->ID == geometry->ID;
                };

                auto results = filter(MD, pred);

                for (const auto r : results)
                {
                    switch (r->type)
                    {
                    case MetaData::EMETAINFOTYPE::EMI_ANIMATIONCLIP:    break;
                    case MetaData::EMETAINFOTYPE::EMI_ANIMATIONEVENT:   break;
                    case MetaData::EMETAINFOTYPE::EMI_SKELETALANIMATION:
                    {
                        auto skeletonMetaData = static_pointer_cast<Skeleton_MetaData>(r);
                    }   break;
                    default:
                        break;
                    }
                }
            }
            /*
            auto RelatedMD	= FindRelatedMetaData(MD, MetaData::EMETA_RECIPIENT_TYPE::EMR_NONE, geometry.ID);

            for(size_t J = 0; J < RelatedMD.size(); ++J)
            {
                switch (RelatedMD[J]->type)
                {
                case MetaData::EMETAINFOTYPE::EMI_COLLIDER:
                {
                    if (!Cooker)
                        continue;

                    Collider_MetaData*	ColliderInfo	= (Collider_MetaData*)RelatedMD[J];
                    ColliderStream		Stream			= ColliderStream(SystemAllocator, 2048);

                    using physx::PxTriangleMeshDesc;
                    PxTriangleMeshDesc meshDesc;
                    meshDesc.points.count     = geometry.Buffers[0]->GetBufferSizeUsed();
                    meshDesc.points.stride    = geometry.Buffers[0]->GetElementSize();
                    meshDesc.points.data      = geometry.Buffers[0]->GetBuffer();

                    uint32_t* Indexes = (uint32_t*)SystemAllocator._aligned_malloc(geometry.Buffers[15]->GetBufferSizeRaw());

                    {
                        struct Tri {
                            uint32_t Indexes[3];
                        };

                        auto Proxy		= geometry.Buffers[15]->CreateTypedProxy<Tri>();
                        size_t Position = 0;
                        auto itr		= Proxy.begin();
                        auto end		= Proxy.end();

                        while(itr < end) {
                            Indexes[Position + 0] = (*itr).Indexes[1];
                            Indexes[Position + 2] = (*itr).Indexes[2];
                            Indexes[Position + 1] = (*itr).Indexes[0];
                            Position += 3;
                            itr++;
                        }
                    }

                    auto IndexBuffer		  = geometry.IndexBuffer_Idx;
                    meshDesc.triangles.count  = geometry.IndexCount / 3;
                    meshDesc.triangles.stride = geometry.Buffers[15]->GetElementSize() * 3;
                    meshDesc.triangles.data   = Indexes;

    #if USING(RESCOMPILERVERBOSE)
                    printf("BEGINNING MODEL BAKING!\n");
                    std::cout << "Baking: " << geometry.ID << "\n";
    #endif
                    bool success = false;
                    if(Cooker) success = Cooker->cookTriangleMesh(meshDesc, Stream);

    #if USING(RESCOMPILERVERBOSE)
                    if(success)
                        printf("MODEL FINISHED BAKING!\n");
                    else
                        printf("MODEL FAILED BAKING!\n");

    #else
                    FK_ASSERT(success, "FAILED TO COOK MESH!");
    #endif

                    if(success)
                    {
                        auto Blob = CreateColliderResourceBlob(Stream.Buffer, Stream.used, ColliderInfo->Guid, ColliderInfo->ColliderID, SystemAllocator);
                        ResourcesFound.push_back(Blob);
                    }
                }	break;
                default:
                    break;
                }
            }
            */
        }

#if USING(RESCOMPILERVERBOSE)
        std::cout << "Created " << resources.size() << " Resource Blobs\n";
#endif

        return resources;
    }




    void ProcessNodes(fbxsdk::FbxNode* Node, SceneResource_ptr scene, const MetaDataList& MD = MetaDataList{}, size_t Parent = -1)
    {
        bool SkipChildren = false;
        auto AttributeCount = Node->GetNodeAttributeCount();

        const auto Position = Node->LclTranslation.Get();
        const auto LclScale = Node->LclScaling.Get();
        const auto rotation = Node->LclRotation.Get();
        const auto NodeName = Node->GetName();

        SceneNode NewNode;
        NewNode.parent = Parent;
        NewNode.position = TranslateToFloat3(Position);
        NewNode.scale = TranslateToFloat3(LclScale);
        NewNode.Q = Quaternion((float)rotation.mData[0], (float)rotation.mData[1], (float)rotation.mData[2]);

        const uint32_t Nodehndl = scene->AddSceneNode(NewNode);

        for (int i = 0; i < AttributeCount; ++i)
        {
            auto Attr = Node->GetNodeAttributeByIndex(i);
            auto AttrType = Attr->GetAttributeType();

            switch (AttrType)
            {
            case FbxNodeAttribute::eMesh:
            {
#if USING(RESCOMPILERVERBOSE)
                std::cout << "Entity Found: " << Node->GetName() << "\n";
#endif
                auto FBXMesh = static_cast<fbxsdk::FbxMesh*>(Attr);
                auto UniqueID = FBXMesh->GetUniqueID();
                auto name = Node->GetName();

                const auto materialCount = Node->GetMaterialCount();
                const auto shadingMode = Node->GetShadingMode();

                for (int I = 0; I < materialCount; I++)
                {
                    auto classID = Node->GetMaterial(I)->GetClassId();
                    auto material = Node->GetSrcObject<FbxSurfacePhong>(I);
                    auto materialName = material->GetName();
                    auto diffuse = material->sDiffuse;
                    auto normal = material->sNormalMap;
                    bool multilayer = material->MultiLayer;
                }

                SceneEntity entity;
                entity.components.push_back(std::make_shared<DrawableComponent>(UniqueID));
                entity.Node = Nodehndl;
                entity.id = name;
                entity.id = std::string(name);

                scene->AddSceneEntity(entity);
            }	break;
            case FbxNodeAttribute::eLight:
            {
#if USING(RESCOMPILERVERBOSE)
                std::cout << "Light Found: " << Node->GetName() << "\n";
#endif
                const auto FBXLight = static_cast<fbxsdk::FbxLight*>(Attr);
                const auto Type = FBXLight->LightType.Get();
                const auto Cast = FBXLight->CastLight.Get();
                const auto I = (float)FBXLight->Intensity.Get() / 10;
                const auto K = FBXLight->Color.Get();
                const auto R = FBXLight->OuterAngle.Get();
                const auto radius = FBXLight->FarAttenuationStart.Get();
                const auto decay = FBXLight->DecayStart.Get();

                SceneEntity entity;
                entity.Node = Nodehndl;
                entity.id = std::string(Node->GetName());

                entity.components.push_back(std::make_shared<PointLightComponent>(TranslateToFloat3(K), float2{ 40, 40 }));

                scene->AddSceneEntity(entity);
            }	break;
            case FbxNodeAttribute::eMarker:
            case FbxNodeAttribute::eUnknown:
            default:
                break;
            }
        }

        if (!SkipChildren)
        {
            const auto ChildCount = Node->GetChildCount();
            for (int I = 0; I < ChildCount; ++I)
                ProcessNodes(Node->GetChild(I), scene, MD, Nodehndl);
        }
    }


    /************************************************************************************************/


    void ScanChildrenNodesForScene(
        fbxsdk::FbxNode*    Node,
        const MetaDataList& MetaData,
        IDTranslationTable& translationTable,
        ResourceList&       Out)
    {
        const auto nodeName = Node->GetName();
        const auto RelatedMetaData = FindRelatedMetaData(MetaData, MetaData::EMETA_RECIPIENT_TYPE::EMR_NODE, Node->GetName());
        const auto NodeCount = Node->GetChildCount();

        if (RelatedMetaData.size())
        {
            for (auto& i : RelatedMetaData)
            {
                if (i->type == MetaData::EMETAINFOTYPE::EMI_SCENE)
                {
                    const auto			MD = std::static_pointer_cast<Scene_MetaData>(i);
                    SceneResource_ptr	scene = std::make_shared<SceneResource>();

                    scene->GUID = MD->Guid;
                    scene->ID = MD->SceneID;
                    scene->translationTable = translationTable;

                    ProcessNodes(Node, scene, MD->sceneMetaData);
                    Out.push_back(scene);
                }
            }
        }
        else
        {
            for (int itr = 0; itr < NodeCount; ++itr) {
                auto Child = Node->GetChild(itr);
                ScanChildrenNodesForScene(Child, MetaData, translationTable, Out);
            }
        }
    }


    /************************************************************************************************/


    void GetScenes(fbxsdk::FbxScene* S, const MetaDataList& MetaData, IDTranslationTable& translationTable, ResourceList& Out)
    {
        auto Root = S->GetRootNode();
        ScanChildrenNodesForScene(Root, MetaData, translationTable, Out);
    }


    /************************************************************************************************/


    void MakeScene(fbxsdk::FbxScene* S, IDTranslationTable& translationTable, ResourceList& Out)
    {
        auto Root = S->GetRootNode();

        SceneResource_ptr	scene = std::make_shared<SceneResource>();

        scene->ID = Root->GetName();
        scene->translationTable = translationTable;

        ProcessNodes(Root, scene, {});
        Out.push_back(scene);
    }


    /************************************************************************************************/


    ResourceList CreateSceneFromFBXFile(std::string file, const MetaDataList& metaData)
    {
	    fbxsdk::FbxManager*		Manager		= fbxsdk::FbxManager::Create();
	    fbxsdk::FbxIOSettings*	Settings	= fbxsdk::FbxIOSettings::Create(Manager, IOSROOT);

        auto [res, scene] = LoadFBXScene(file, Manager, Settings);

	    Manager->SetIOSettings(Settings);

        if (res)
        {
            IDTranslationTable  translationTable;
            ResourceList	    resources = GatherSceneResources(scene, nullptr, translationTable, true, metaData);

            GetScenes(scene, metaData, translationTable, resources);

            return resources;
        }
        else
        {
            std::cout << "Failed to Open FBX File: " << file << "\n";
            MessageBox(0, L"Failed to Load File!", L"ERROR!", MB_OK);

            return {};
        }
    }


    /************************************************************************************************/


    std::pair<ResourceList, std::shared_ptr<SceneResource>>  CreateSceneFromFBXFile2(fbxsdk::FbxScene* scene, const CompileSceneFromFBXFile_DESC& Desc)
    {
        IDTranslationTable	    translationTable;
        ResourceList			resources = GatherSceneResources(scene, Desc.Cooker, translationTable, true, {});
        ResourceList            sceneRes;

        MakeScene(scene, translationTable, sceneRes);
        // Translate ID's right now
        auto localScene = std::dynamic_pointer_cast<SceneResource>(sceneRes.back());
        for (auto& entity : localScene->entities)
        {
            for (auto& component : entity.components)
            {
                if (component->id == GetTypeGUID(DrawableComponent))
                {
                    auto drawable = std::dynamic_pointer_cast<DrawableComponent>(component);
                    drawable->MeshGuid = TranslateID(drawable->MeshGuid, translationTable);
                }
            }
        }
        return { resources, localScene };
    }


    /************************************************************************************************/


    Pair<bool, fbxsdk::FbxScene*>
    LoadFBXScene(std::string file, fbxsdk::FbxManager* lSdkManager, fbxsdk::FbxIOSettings* settings)
    {
        fbxsdk::FbxNode* node = nullptr;
        fbxsdk::FbxImporter* importer = fbxsdk::FbxImporter::Create(lSdkManager, "");

        if (!importer->Initialize(file.c_str(), -1, lSdkManager->GetIOSettings()))
        {
            printf("Failed to Load: %s\n", file.c_str());
            printf("Error Returned: %s\n", importer->GetStatus().GetErrorString());
            return{ false, nullptr };
        }

        fbxsdk::FbxScene* scene = FbxScene::Create(lSdkManager, "Scene");
        if (!importer->Import(scene))
        {
            printf("Failed to Load: %s\n", file.c_str());
            printf("Error Returned: %s\n", importer->GetStatus().GetErrorString());
            return{ false, nullptr };
        }

        if (const auto& axisSystem = scene->GetGlobalSettings().GetAxisSystem();
            axisSystem != FbxAxisSystem(FbxAxisSystem::EUpVector::eYAxis, FbxAxisSystem::EFrontVector::eParityOdd, FbxAxisSystem::ECoordSystem::eRightHanded))
        {
            std::cout << "Converting scene axis system\n";

            FbxAxisSystem newAxisSystem(FbxAxisSystem::EUpVector::eYAxis, FbxAxisSystem::EFrontVector::eParityOdd, FbxAxisSystem::ECoordSystem::eRightHanded);
            newAxisSystem.DeepConvertScene(scene);
        }

        return{ true, scene };
    }

}   /************************************************************************************************/


/**********************************************************************

Copyright (c) 2019-2021 Robert May

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
