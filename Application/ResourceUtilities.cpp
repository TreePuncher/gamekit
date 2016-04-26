
/**********************************************************************

Copyright (c) 2015 - 2016 Robert May

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
#include "ResourceUtilities.h"
#include "GameUtilities.h"

#include "..\buildsettings.h"
#include "..\graphicsutilities\AnimationUtilities.h"
#include "..\graphicsutilities\graphics.h"
#include "..\graphicsutilities\MeshUtils.h"

#include <inttypes.h>
#include <Windows.h>
#include <shobjidl.h> 

#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "Shell32.lib")

// FBX Loading
#include				"fbxsdk/include/fbxsdk.h"
#pragma comment	(lib,	"libfbxsdk.lib")


using namespace FlexKit;



/************************************************************************************************/


void InitiateScene( Scene* out, RenderSystem* RS, BlockAllocator* memory, Scene_Desc* desc )
{
	desc->MaxPointLightCount = FlexKit::max(1, desc->MaxPointLightCount);

	out->Root			= desc->Root;
	out->MaxEntities	= desc->MaxEntityCount;
	out->MaxObjects		= desc->MaxTriMeshCount;
	out->Entities		= (Entity*)memory->_aligned_malloc( sizeof(Entity)  * desc->MaxEntityCount	);

	if(desc->MaxTriMeshCount)
		out->Geometry = (TriMesh*)memory->_aligned_malloc( sizeof(TriMesh) * desc->MaxTriMeshCount	);
	
	char*  IDs		   = (char*)memory->_aligned_malloc (out->MaxEntities * 64);
	out->EntityIDs	   = IDs;
	memset( IDs, 0x00, out->MaxEntities * 64 );

	byte*  LightBuffer = (byte*)memory->_aligned_malloc (( sizeof(Entity) + 9 ) * desc->MaxPointLightCount );
	CreatePointLightBuffer(		
		RS, 
		&out->PLightBuffer, 
		{ desc->MaxPointLightCount }, memory);

	out->Alloc = memory;
}


/************************************************************************************************/


void UpdateScene( Scene* In, RenderSystem* RS, SceneNodes* Nodes )	
{ 
	//UpdatePointLightBuffer( *RS, Nodes, &In->PLightBuffer ); 
}


/************************************************************************************************/


void CleanUpScene( Scene* scn, EngineMemory* memory )
{
	if (!scn)
		return;

	for( size_t itr = 0; itr < scn->EntitiesUsed; ++itr)
		CleanUpEntity(scn->Entities + itr);
	for( size_t itr = 0; itr < scn->GeometryUsed; ++itr)
		CleanUpTriMesh( scn->Geometry + itr );

	CleanUp( &scn->PLightBuffer, &memory->BlockAllocator );

	if(scn->Entities)				scn->Alloc->_aligned_free(scn->Entities);
	if(scn->PLightBuffer.Lights)	scn->Alloc->_aligned_free(scn->PLightBuffer.Lights);
	if(scn->Geometry)				scn->Alloc->_aligned_free(scn->Geometry);
	if(scn->EntityIDs)				scn->Alloc->_aligned_free(scn->EntityIDs);

	scn->Entities				= nullptr;
	scn->PLightBuffer.Lights	= nullptr;
	scn->Geometry				= nullptr;
	scn->EntityIDs				= nullptr;
}


/************************************************************************************************/


Pair<bool, SceneHandle> 
SearchForMesh(Scene* Scn, size_t TriMeshID)
{
	for ( size_t itr = 0; itr < Scn->GeometryUsed; ++itr )
		if ( Scn->Geometry[ itr ].TriMeshID == TriMeshID )
			return{ true, SceneHandle( itr ) };

	return{ false, SceneHandle(0) };
}

Pair<bool, SceneHandle>	
SearchForEntity( Scene* Scn, char* ID )
{
	for( size_t itr = 0; itr < Scn->EntitiesUsed; ++itr )
		if( strcmp( Scn->EntityIDs + itr * 64, ID ) == 0 )
			return{true, SceneHandle( itr ) };

	return {false, SceneHandle (-1)};
}


/************************************************************************************************/


TriMesh* SearchForMesh(Scene* Scene, const char* str)
{
	for (int I = 0; I < Scene->GeometryUsed; ++I)
	{
		if(Scene->Geometry[I].ID)
			if( strcmp(Scene->Geometry[I].ID, str) == 0 )
				return Scene->Geometry + I;
	}
	return nullptr;
}


/************************************************************************************************/


Pair<bool, fbxsdk::FbxScene*>  
LoadFBXScene(char* file, fbxsdk::FbxManager* lSdkManager, fbxsdk::FbxIOSettings* settings)
{
	fbxsdk::FbxNode*	 node	  = nullptr;
	fbxsdk::FbxImporter* importer = fbxsdk::FbxImporter::Create(lSdkManager, "");

	if (!importer->Initialize(file, -1, lSdkManager->GetIOSettings()))
	{
		printf("Failed to Load: %s\n", file);
		printf("Error Returned: %s\n", importer->GetStatus().GetErrorString());
		return{ false, nullptr };
	}

	fbxsdk::FbxScene* scene = FbxScene::Create(lSdkManager, "Scene");
	if (!importer->Import(scene))
	{
		printf("Failed to Load: %s\n", file);
		printf("Error Returned: %s\n", importer->GetStatus().GetErrorString());
		return{ false, nullptr };
	}
	return{ true, scene };
}


/************************************************************************************************/


float3 TranslateToFloat3(FbxVector4& in)
{
	return float3(
		in.mData[0], 
		in.mData[1], 
		in.mData[2]);
}

float3 TranslateToFloat3(FbxDouble3& in)
{
	return float3(
		in.mData[0],
		in.mData[1],
		in.mData[2]);
}

float4 TranslateToFloat4(FbxVector4& in)
{
	return float4(
		in.mData[0],
		in.mData[1],
		in.mData[2],
		in.mData[3]);
}

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

		//Normals->GetReferenceMode();
	}
	default:
		break;
	}
	return out;
}


/************************************************************************************************/


XMMATRIX FBXMATRIX_2_XMMATRIX(FbxAMatrix& AM)
{
	XMMATRIX M;
	for (size_t I = 0; I < 4; ++I)
		for (size_t II = 0; II < 4; ++II)
			M.r[I].m128_f32[II] = AM[I][II];

	return M;
}

FbxAMatrix XMMATRIX_2_FBXMATRIX(XMMATRIX& M)
{
	FbxAMatrix AM;
	for (size_t I = 0; I < 4; ++I)
		for (size_t II = 0; II < 4; ++II)
			AM[I][II] = M.r[I].m128_f32[II];

	return AM;
}


/************************************************************************************************/


struct Engine
{
	BlockAllocator* MemoryOut;
	StackAllocator* TempMem;
	StackAllocator* LevelMem;
	RenderSystem*	RS;
	ShaderTable*	Materials;
};

struct Compiler
{
	ShaderTable*	Materials;
};

struct FBXVertexLayout
{
	float3 POS		= {0};
	float3 Normal	= {0};
	float3 Tangent	= {0};
	float3 TexCord1	= {0};
	float3 Weight	= {0}; 
};


/************************************************************************************************/


uint32_t FetchIndex(size_t itr, fbxsdk::FbxMesh* Mesh)
{
#ifdef _DEBUG 
	int PolygonID	= itr / 3;
	int IndexID		= itr % 3;
	
	size_t VI = Mesh->GetPolygonVertex(itr / 3, itr % 3);
#endif

	return Mesh->GetPolygonVertex(itr/3, itr % 3);
}


/************************************************************************************************/


uint32_t FetchIndex2(size_t itr, FlexKit::MeshUtilityFunctions::IndexList* IL)
{
	return IL->operator[](itr);
}


/************************************************************************************************/


float3 FetchVertexPOS(size_t itr, FlexKit::MeshUtilityFunctions::CombinedVertexBuffer* Buff)
{ 
	return Buff->operator[](itr).POS;
}


/************************************************************************************************/


float3 FetchWeights(size_t itr, FlexKit::MeshUtilityFunctions::CombinedVertexBuffer* Buff)
{ 
	return Buff->operator[](itr).WEIGHTS;
}


/************************************************************************************************/


uint4_32 FetchWeightIndices( size_t itr, FlexKit::MeshUtilityFunctions::CombinedVertexBuffer* Buff )
{
	return Buff->operator[](itr).WIndices;
}


/************************************************************************************************/


float3 FetchVertexNormal(size_t itr, FlexKit::MeshUtilityFunctions::CombinedVertexBuffer* Buff)
{
	return Buff->operator[](itr).NORMAL;
}


/************************************************************************************************/


float3 FetchFloat3ZERO(size_t itr, FlexKit::MeshUtilityFunctions::CombinedVertexBuffer* Buff)
{
	return{ 0.0f, 0.0f, 0.0f };
}


/************************************************************************************************/


float2 FetchVertexUV(size_t itr, FlexKit::MeshUtilityFunctions::CombinedVertexBuffer* Buff)
{
	auto temp = Buff->operator[](itr).TEXCOORD.xy();
	return {temp.x, temp.y};
}


/************************************************************************************************/


uint32_t WriteIndex(uint32_t in)
{
	return in;
}


/************************************************************************************************/


float3 WriteVertex(float3 in)
{
	return in;
}


/************************************************************************************************/
 

float2 WriteUV(float2 in)
{
	return in;
}


/************************************************************************************************/


uint4_32 Writeuint4(uint4_32 in)
{
	return uint4_32{ 
		(in[0] == -1) ? 0 : in[0], 
		(in[1] == -1) ? 0 : in[1], 
		(in[2] == -1) ? 0 : in[2], 
		(in[3] == -1) ? 0 : in[3] };
}


/************************************************************************************************/


size_t GetVertexIndex(size_t pIndex, size_t vIndex, size_t vID, fbxsdk::FbxMesh* Mesh)	
{ 
	return Mesh->GetPolygonVertex(pIndex, vIndex);
}


/************************************************************************************************/


size_t GetNormalIndex(size_t pIndex, size_t vIndex,size_t vID, fbxsdk::FbxMesh* Mesh)
{
	using FlexKit::MeshUtilityFunctions::TokenList;

	int CPIndex = Mesh->GetPolygonVertex(pIndex, vIndex);
	auto NElement = Mesh->GetElementNormal(0);

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


size_t GetTexcordIndex(size_t pIndex, size_t vIndex, fbxsdk::FbxMesh* Mesh)
{
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
				int lTextureUVIndex = Mesh->GetTextureUVIndex(pIndex, vIndex);
				return lTextureUVIndex;
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


struct FBXSkinDeformer
{
	struct BoneWeights
	{
		const char*	Name;
		float*		Weights;
		size_t*		WeightIndices;
		size_t		WeightCount;
	}* Bones;

	uint4_32* WeightIndices;
	size_t size;

	size_t BoneCount;
};

struct FBXMeshDesc
{
	bool UV;
	bool Normals;
	bool Weights;

	float3			MinV;
	float3			MaxV;
	float			R;

	size_t			FaceCount;
	FBXSkinDeformer Skin;
};

FBXSkinDeformer CreateSkin(fbxsdk::FbxMesh* Mesh, StackAllocator* TempMem)
{	// Get Weights
	FBXSkinDeformer	Out = {};

	auto DeformerCount  = Mesh->GetDeformerCount();
	for ( size_t I = 0; I < DeformerCount; ++I )
	{
		fbxsdk::FbxStatus S;
		auto D		= Mesh->GetDeformer(I, &S);
		auto Type	= D->GetDeformerType();
		switch ( Type ) {
		case fbxsdk::FbxDeformer::EDeformerType::eSkin:
		{
			auto Skin			= (FbxSkin*)D;
			auto ClusterCount	= Skin->GetClusterCount();
			
			Out.Bones = (FBXSkinDeformer::BoneWeights*)TempMem->malloc( sizeof( FBXSkinDeformer::BoneWeights ) * ClusterCount );
			for ( size_t II = 0; II < ClusterCount; ++II)
			{
				Out.BoneCount++;

				auto Cluster		= Skin->GetCluster( II );
				auto ClusterName	= Cluster->GetLink()->GetName();
				size_t CPICount		= Cluster->GetControlPointIndicesCount();
				auto CPIndices		= Cluster->GetControlPointIndices();
				auto Weights		= Cluster->GetControlPointWeights();

				auto Bone = Cluster->GetLink();
				auto BoneAttrib = Bone->GetNodeAttribute();
				if (BoneAttrib->GetAttributeType() == FbxNodeAttribute::eSkeleton)
					auto Name	= Bone->GetName();

				Out.Bones[II].Name			= ClusterName;
				Out.Bones[II].Weights		= (float*)TempMem->malloc( sizeof( float ) * CPICount );
				Out.Bones[II].WeightIndices	= (size_t*)TempMem->malloc( sizeof( size_t ) * CPICount );
				Out.Bones[II].WeightCount	= CPICount;

				for ( size_t III = 0; III < CPICount; ++III ){
					Out.Bones[II].Weights		[III] = 0.0f;
					Out.Bones[II].WeightIndices	[III] = 0;
				}

				for ( size_t III = 0; III < CPICount; ++III ){
					Out.Bones[II].Weights		[III] = Weights		[III];
					Out.Bones[II].WeightIndices	[III] = CPIndices	[III];
				}
			}

		}	break;
		}
	}
	return Out;
}


/************************************************************************************************/


FBXMeshDesc TranslateToTokens(fbxsdk::FbxMesh* Mesh, StackAllocator* TempMem, MeshUtilityFunctions::TokenList& TokensOut, Skeleton* S = nullptr, bool SubDiv_Enabled = false)
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
	float3 MinV(0);
	float3 MaxV(0);
	float  R = 0;

	{	// Get Vertices
		size_t VCount = Mesh->GetControlPointsCount();
		auto CPs = Mesh->mControlPoints;

		for (int itr = 0; itr < VCount; ++itr){
			auto V = CPs[itr];
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
		auto	Normals		= Mesh->GetElementNormal();
		size_t	NCount		= Mesh->GetPolygonVertexCount();
		auto	mapping		= Normals->GetMappingMode();

		if ( NormalCount )
			out.Normals = true;

		for (int itr = 0; itr < NCount; ++itr)
		{
			auto N = Normals->GetDirectArray().GetAt(itr);
			AddNormalToken(TranslateToFloat3(N), TokensOut);
		}
	}
	{	// Get UV's
		auto UVElement = Mesh->GetElementUV(0);
		if ( UVElement ) {
			out.UV = true;
			auto size = UVElement->GetDirectArray().GetCount();
			for ( size_t I = 0; I < size; ++I ){
				auto UV = UVElement->GetDirectArray().GetAt( I );
				AddTexCordToken({float(UV[0]), float(UV[1]), 0.0f}, TokensOut);
			}
		}
	}
	// Get Use-able Deformers
	if (Mesh->GetDeformerCount() && S)
	{
		size_t		VCount		= Mesh->GetControlPointsCount();
		float4*		Weights		= (float4*)  TempMem->_aligned_malloc( sizeof( float4 )    * VCount );
		uint4_32*	BoneIndices = (uint4_32*)TempMem->_aligned_malloc( sizeof( uint4_32  ) * VCount );

		for (size_t I = 0; I < VCount; ++I)	Weights[I] = {0.0f, 0.0f, 0.0f, 0.0f};
		for (size_t I = 0; I < VCount; ++I)	BoneIndices[I] = {-1, -1, -1, -1};

		auto Skin			= CreateSkin(Mesh, TempMem);
		Skin.WeightIndices	= BoneIndices;

		for (size_t I = 0; I < Skin.BoneCount; ++I)
		{
			JointHandle Handle = S->FindJoint(Skin.Bones[I].Name);
			for (size_t II = 0; II < Skin.Bones[I].WeightCount; ++II)
			{
				size_t III = 0;
				for (; III < 4; ++III)
				{
					size_t Index = Skin.Bones[I].WeightIndices[II];
					if (BoneIndices[Index][III] == -1)
						break;
				}

				if (III != 4) {
					Weights		[Skin.Bones[I].WeightIndices[II]][III] = Skin.Bones[I].Weights[II];
					BoneIndices	[Skin.Bones[I].WeightIndices[II]][III] = Handle;
				}
			}
		}

		for (size_t I = 0; I < VCount; ++I)
			AddWeightToken({Weights[I].pFloats, BoneIndices[I]}, TokensOut);
		
		out.Weights = true;
		out.Skin = Skin;
	}
	{	// Calculate Indices
		auto Normals		= Mesh->GetElementNormal();
		auto UVs			= Mesh->GetElementUV(0);

		size_t NormalCount = Mesh->GetElementNormalCount();
		size_t TriCount	   = Mesh->GetPolygonCount();
		size_t IndexCount  = 0;

		// Iterate through each Tri
		for (size_t I = 0; I < TriCount; ++I)
		{	// Process each Vertex in tri
			unsigned int size = Mesh->GetPolygonSize( I );

			size_t	NC = Mesh->GetElementNormal()->GetDirectArray().GetCount();
			if (SubDiv_Enabled)	AddPatchBeginToken(TokensOut);

			if (size == 3)
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
				
				AddIndexToken(VertexIndex1, NormalIndex1, 0, TokensOut);
				AddIndexToken(VertexIndex3, NormalIndex3, 0, TokensOut);
				AddIndexToken(VertexIndex2, NormalIndex2, 0, TokensOut);

				IndexCount += 3;
			} else if (size == 4)
			{	// Quads
				FK_ASSERT(false | SubDiv_Enabled);

				auto VertexIndex1 =				  GetVertexIndex(I, 0, IndexCount, Mesh);
				auto NormalIndex1 = out.Normals ? GetNormalIndex(I, 0, IndexCount, Mesh) : 0;
				auto UVCordIndex1 = out.UV ?	  GetTexcordIndex(I, 0, Mesh) : 0;

				auto VertexIndex2 =				  GetVertexIndex(I, 1, IndexCount + 1, Mesh);
				auto NormalIndex2 = out.Normals ? GetNormalIndex(I, 1, IndexCount + 1, Mesh) : 0;
				auto UVCordIndex2 = out.UV ?	  GetTexcordIndex(I, 1, Mesh) : 0;

				auto VertexIndex3 =				  GetVertexIndex(I, 2, IndexCount + 2, Mesh);
				auto NormalIndex3 = out.Normals ? GetNormalIndex(I, 2, IndexCount + 2, Mesh) : 0;
				auto UVCordIndex3 = out.UV ?	  GetTexcordIndex(I, 2, Mesh) : 0;

				auto VertexIndex4 =				  GetVertexIndex(I, 3, IndexCount + 3, Mesh);
				auto NormalIndex4 = out.Normals ? GetNormalIndex(I, 3, IndexCount + 3, Mesh) : 0;
				auto UVCordIndex4 = out.UV ?	  GetTexcordIndex(I, 3, Mesh) : 0;

				AddIndexToken(VertexIndex1, NormalIndex1, 0, TokensOut);
				AddIndexToken(VertexIndex3, NormalIndex3, 0, TokensOut);
				AddIndexToken(VertexIndex2, NormalIndex2, 0, TokensOut);
				AddIndexToken(VertexIndex4, NormalIndex4, 0, TokensOut);

				IndexCount += 6;
			}

			if (SubDiv_Enabled)	AddPatchEndToken(TokensOut);
		} 
	}

	out.MinV = MinV;
	out.MaxV = MaxV;
	out.R	 = R;

	return out;
}


/************************************************************************************************/


struct GeometryBlock
{
	static const size_t	BlockSize	= 10;
	size_t				MeshUsed	= 0;
	TriMesh				Meshes[BlockSize];

	GeometryBlock*	Next	= nullptr;
};


/************************************************************************************************/


template<typename TY_ALLOC>
size_t PushGeo(GeometryBlock* GB, TriMesh& Mesh, TY_ALLOC Alloc)
{
	if (GB->MeshUsed > GeometryBlock::BlockSize )
	{
		GB->Next = CreateGB( Alloc );
		return PushGeo( GB->Next, Mesh, Alloc );
	}
	GB->Meshes[ GB->MeshUsed ] = Mesh;
	return GB->MeshUsed++;
};


/************************************************************************************************/


size_t GetGeoCount(GeometryBlock* GB)
{
	size_t Count = GB->MeshUsed;
	if(Count > GeometryBlock::BlockSize)
		Count += GetGeoCount(GB->Next);

	return Count;
};


/************************************************************************************************/


template<typename TY_ALLOC>
GeometryBlock* CreateGB(TY_ALLOC* Alloc)
{
	auto GB = (GeometryBlock*)Alloc->_aligned_malloc(sizeof(GeometryBlock));
	GB->MeshUsed	= 0;
	GB->Next		= 0;
	return GB;
}


/************************************************************************************************/


TriMesh* FindGeoByID(GeometryBlock* GB, size_t ID)
{
	for(size_t i = 0; i < GB->MeshUsed; i++)
	{
		if(GB->Meshes[i].TriMeshID == ID)
			return &GB->Meshes[i];
	}
	if(GB->MeshUsed > GB->BlockSize)
		return FindGeoByID(GB->Next, ID);
	else
		return nullptr;
}


/************************************************************************************************/


void MoveGeo(GeometryBlock* GB, TriMesh* out, size_t I = 0)
{
	for (;I < GB->MeshUsed; ++I)
		out[I] = GB->Meshes[I%GeometryBlock::BlockSize];

	if (GB->MeshUsed > GeometryBlock::BlockSize)
		MoveGeo( GB, out, I );
}


/************************************************************************************************/


fbxsdk::FbxNode* FindSkeletonRoot(fbxsdk::FbxMesh* M)
{
	auto DeformerCount  = M->GetDeformerCount();
	for (size_t I = 0; I < DeformerCount; ++I)
	{
		fbxsdk::FbxStatus S;
		auto D		= M->GetDeformer(I, &S);
		auto Type	= D->GetDeformerType();

		switch (Type)
		{
		case fbxsdk::FbxDeformer::EDeformerType::eSkin:
		{
			auto Skin			= (FbxSkin*)D;
			auto ClusterCount	= Skin->GetClusterCount();

			auto Cluster		= Skin->GetCluster(0);
			auto CLBone			= Cluster->GetLink();
			auto CLBoneAttrib	= CLBone->GetNodeAttribute();
			auto CLBoneName		= CLBone->GetName();
			auto* I				= CLBone;

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


struct	JointAnimation
{
	struct Pose
	{
		JointPose	JPose;
		double		T;
		double		_PAD;
	}* Poses;

	size_t FPS;
	size_t FrameCount;
	size_t _PAD;
};

struct JointInfo
{
	JointAndPose	Joint;
	JointAnimation	Animation;
	XMMATRIX		Inverse;
};


/************************************************************************************************/


template<typename TY_MEM>
JointAnimation GetJointAnimation(FbxNode* N, TY_MEM* M)
{
	auto Scene          = N->GetScene();
	auto AnimationStack = Scene->GetCurrentAnimationStack();
	auto TakeInfo       = Scene->GetTakeInfo(AnimationStack->GetName());
	auto Begin		    = TakeInfo->mLocalTimeSpan.GetStart();
	auto End		    = TakeInfo->mLocalTimeSpan.GetStop();
	auto Duration		= End - Begin;
	auto FrameRate		= 1.0f / 60.0f;

	JointAnimation	A; 
	A.FPS			= 60;
	A.Poses			= (JointAnimation::Pose*)M->_aligned_malloc(sizeof(JointAnimation::Pose) * Duration.GetFrameCount(FbxTime::eFrames60));
	A.FrameCount	= Duration.GetFrameCount(FbxTime::eFrames60);

	for(size_t I = 0; I < size_t(Duration.GetFrameCount(FbxTime::eFrames60)); ++I)
	{
		FbxTime	CurrentFrame;
		CurrentFrame.SetFrame(I, FbxTime::eFrames60);
		A.Poses[I].JPose = GetPose(FBXMATRIX_2_XMMATRIX(N->EvaluateLocalTransform(CurrentFrame)));
	}

	return A;
}


/************************************************************************************************/


JointHandle GetJoint(static_vector<JointInfo, 1024>& Out, const char* ID)
{
	for (size_t I = 0; I < Out.size(); ++I)
		if (!strcmp(Out[I].Joint.Linkage.mID, ID))
			return I;

	return 0XFFFF;
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


void GetJointTransforms(static_vector<JointInfo, 1024>& Out, FbxMesh* M, StackAllocator* MEM)
{
	auto DeformerCount = M->GetDeformerCount();
	for (size_t I = 0; I < DeformerCount; ++I)
	{
		auto D = M->GetDeformer(I);
		if (D->GetDeformerType() == FbxDeformer::EDeformerType::eSkin)
		{
			auto Skin = (FbxSkin*)D;
			for (size_t II = 0; Skin->GetClusterCount() > II; ++II)
			{
				auto Cluster = Skin->GetCluster(II);
				auto ID = Cluster->GetLink()->GetName();

				JointHandle Handle = GetJoint(Out, ID);
				FbxAMatrix G = GetGeometryTransformation(Cluster->GetLink());
				FbxAMatrix transformMatrix;						
				FbxAMatrix transformLinkMatrix;					
				FbxAMatrix globalBindposeInverseMatrix;

				Cluster->GetTransformMatrix(transformMatrix);			
				Cluster->GetTransformLinkMatrix(transformLinkMatrix);

				globalBindposeInverseMatrix = transformLinkMatrix.Inverse() * transformMatrix * G;
				Out[Handle].Inverse			= FBXMATRIX_2_XMMATRIX(globalBindposeInverseMatrix);
				//Out[Handle].Animation		= ;
			}
		}
	}
}


/************************************************************************************************/


template<typename TY_Out, typename TY_MEM>
void FindAllJoints(TY_Out& Out, FbxNode* N, TY_MEM* MEM, size_t Parent = 0xFFFF )
{
	if (N->GetNodeAttribute() && N->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton )
	{
		fbxsdk::FbxSkeleton* Sk = (fbxsdk::FbxSkeleton*)N->GetNodeAttribute();
		
		int JointIndex = Out.size();
		int ChildCount = N->GetChildCount();

		Joint NewJoint;
		NewJoint.mID		= N->GetName();
		NewJoint.mParent	= JointHandle(Parent);

		Out.push_back({ {NewJoint}, GetJointAnimation(N, MEM), DirectX::XMMatrixIdentity() });

		for ( size_t I = 0; I < ChildCount; ++I )
			FindAllJoints(Out, N->GetChild( I ), MEM, JointIndex);
	}
}


/************************************************************************************************/


struct AnimationCut
{
	double T_Start;
	double T_End;
	char*  ID;
};

typedef static_vector<AnimationCut> CutList;


void GetAnimationCuts(CutList* out, MetaData_list* MD, const char* ID)
{
	if (MD)
	{
		for (auto e : *MD)
		{
			if (!strncmp(e->ID, ID, 64))
			{
				auto Clip = (Animation_Clip*)e;
				AnimationCut NewCut;
				NewCut.T_Start	= Clip->T_Start;
				NewCut.T_End	= Clip->T_End;
				NewCut.ID		= Clip->ClipID;
			}
		}
	}
}


/************************************************************************************************/


FlexKit::Skeleton* LoadSkeleton(FbxMesh* M, BlockAllocator* Mem, StackAllocator* Temp, const char* ID = nullptr, MetaData_list* MD = nullptr)
{
	using FlexKit::AnimationClip;
	using FlexKit::Skeleton;

	auto Root	= FindSkeletonRoot(M);
	if (!Root)
		return nullptr;

	auto& Joints = Mem->allocate_aligned<static_vector<JointInfo, 1024>>();
	FindAllJoints		(Joints, Root, Temp);
	GetJointTransforms	(Joints, M, Temp);

	Skeleton* S = (Skeleton*)Mem->_aligned_malloc(0x40);
	S->InitiateSkeleton(Mem, Joints.size());

	for (auto J : Joints)
		S->AddJoint(J.Joint.Linkage, J.Inverse);
	
	for (size_t I = 0; I < Joints.size(); ++I)
	{
		size_t ID_Length = strnlen_s(S->Joints[I].mID, 64) + 1;
		char* ID		 = (char*)Mem->malloc(ID_Length);

		strcpy_s(ID, ID_Length, S->Joints[I].mID);
		S->Joints[I].mID = ID;
	}
	static_vector<AnimationCut> Cuts;
	GetAnimationCuts(&Cuts, MD, ID);

	if (!Cuts.size())
	{
		AnimationCut Cut;
		Cut.T_Start	= 0;
		Cut.T_End	= Joints.front().Animation.FrameCount * 1.0/Joints.front().Animation.FPS;
		Cut.ID		= "ANIMATION";
		Cuts.push_back(Cut);
	}

	for(auto Cut : Cuts)
	{
		size_t Begin	= Cut.T_Start / (1.0f / 60.0f);
		size_t End		= Cut.T_End / (1.0f / 60.0f) + 1;
		size_t test		= Joints.front().Animation.FrameCount;
		AnimationClip Clip;
		Clip.Skeleton	= S;
		Clip.FPS		= 60;
		Clip.FrameCount	= End - Begin;
		Clip.mID		= Cut.ID;
		Clip.isLooping	= true;

		{// Single Clips for Now
			Clip.Frames	= (AnimationClip::KeyFrame*)Mem->_aligned_malloc(Clip.FrameCount * sizeof(AnimationClip::KeyFrame));

			for (size_t I = 0; I < Clip.FrameCount; ++I)
			{
				Clip.Frames[I].Joints		= (JointHandle*)Mem->_aligned_malloc(sizeof(JointHandle) * Joints.size());
				Clip.Frames[I].Poses		=   (JointPose*)Mem->_aligned_malloc(sizeof(JointPose)	 * Joints.size());
				Clip.Frames[I].JointCount	= Joints.size();

				for (size_t II = 0; II < Joints.size(); ++II)
				{
					Clip.Frames[I].Joints[II]	= JointHandle(II);
					Clip.Frames[I].Poses[II]	= Joints[II].Animation.Poses[I + Begin].JPose;
				}
			}
		}

		Skeleton_PushAnimation(S, Mem, Clip);
	}
	Temp->clear();

	return S;
}


/************************************************************************************************/

struct CompileMeshInfo
{
	bool   Success;
	size_t BuffersFound;
};

CompileMeshInfo CompileMeshResource(TriMesh& out, StackAllocator* TempMem, BlockAllocator* Memory, FbxMesh* Mesh, bool EnableSubDiv = false, const char* ID = nullptr, MetaData_list* MD = nullptr)
{
	using FlexKit::FillBuffer;
	using FlexKit::AnimationClip;
	using FlexKit::Skeleton;
	using MeshUtilityFunctions::BuildVertexBuffer;
	using MeshUtilityFunctions::CombinedVertexBuffer;
	using MeshUtilityFunctions::IndexList;
	using MeshUtilityFunctions::TokenList;
	using MeshUtilityFunctions::MeshBuildInfo;

	Skeleton*	S		= LoadSkeleton(Mesh, Memory, TempMem, ID, MD);
	TokenList& Tokens	= TokenList::Create_Aligned(256000, TempMem);
	auto MeshInfo		= TranslateToTokens(Mesh, TempMem, Tokens, S);

	CombinedVertexBuffer& CVB = CombinedVertexBuffer::Create_Aligned(128000, TempMem);
	IndexList& IB			  = IndexList::Create_Aligned(128000, TempMem);

	auto BuildRes = MeshUtilityFunctions::BuildVertexBuffer(Tokens, CVB, IB, *TempMem, *TempMem, MeshInfo.Weights);
	FK_ASSERT(BuildRes.V1 == true, "Mesh Failed to Build");

	size_t IndexCount  = GetByType<MeshBuildInfo>(BuildRes).IndexCount;
	size_t VertexCount = GetByType<MeshBuildInfo>(BuildRes).VertexCount;

	static_vector<Pair<VERTEXBUFFER_TYPE, VERTEXBUFFER_FORMAT>> BuffersFound ={
		{VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32}
	};

	if ((MeshInfo.UV))
		BuffersFound.push_back({ VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32 });

	if ((MeshInfo.Normals))
		BuffersFound.push_back({ VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32 });

	if ((MeshInfo.Weights)) {
		BuffersFound.push_back({ VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32 });
		BuffersFound.push_back({ VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32A32 });
	}

	for (size_t i = 0; i < BuffersFound.size(); ++i) {
		CreateBuffer(VertexCount, Memory, out.Buffers[i], (VERTEXBUFFER_TYPE)BuffersFound[i], (VERTEXBUFFER_FORMAT)BuffersFound[i]);

		switch ((VERTEXBUFFER_TYPE)BuffersFound[i])
		{
		case  VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION:
			FillBuffer(&CVB, CVB.size(), out.Buffers[i], WriteVertex, FetchVertexPOS);		break;
		case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL:
			FillBuffer(&CVB, CVB.size(), out.Buffers[i], WriteVertex, FetchVertexNormal);	break;
		case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV:
			FillBuffer(&CVB, CVB.size(), out.Buffers[i], WriteUV, FetchVertexUV);			break;
		case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT:
			FillBuffer(&CVB, CVB.size(), out.Buffers[i], WriteVertex, FetchFloat3ZERO);		break;
		case  VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1:
			FillBuffer(&CVB, CVB.size(), out.Buffers[i], WriteVertex, FetchWeights);		break;
		case  VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2:
			FillBuffer(&CVB, CVB.size(), out.Buffers[i], Writeuint4, FetchWeightIndices);	break;
		default:
			break;
		}
	}

	CreateBuffer(IB.size(), Memory, out.Buffers[15], VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32);
	FillBuffer(&IB, IB.size(), out.Buffers[15], WriteIndex, FetchIndex2);

	
	if (EnableSubDiv)
	{

	}

	TempMem->clear();

	out.IndexCount    = IndexCount;
	out.Skeleton      = S;
	out.AnimationData = MeshInfo.Weights ? TriMesh::EAD_Skin : 0;
	out.Info.max	  = MeshInfo.MaxV;
	out.Info.min	  = MeshInfo.MinV;
	out.Info.r		  = MeshInfo.R;
	out.TriMeshID	  = Mesh->GetUniqueID();
	out.ID			  = ID;

	return {true, BuffersFound.size()};
}


/************************************************************************************************/


typedef Pair<GeometryBlock*, StackAllocator*> GBAPair;
Pair<size_t, GBAPair> 
FindAllGeometry(fbxsdk::FbxNode* node, Engine* E, GeometryBlock* GL = nullptr, StackAllocator* GLAlloc = nullptr, bool SubDiv = false)
{
	using FlexKit::FillBuffer;
	using FlexKit::AnimationClip;
	using FlexKit::Skeleton;
	using MeshUtilityFunctions::BuildVertexBuffer;
	using MeshUtilityFunctions::CombinedVertexBuffer;
	using MeshUtilityFunctions::IndexList;
	using MeshUtilityFunctions::TokenList;
	using MeshUtilityFunctions::MeshBuildInfo;

	
	if ( !GLAlloc )
	{
		GLAlloc = ( StackAllocator* )E->MemoryOut->malloc( MEGABYTE );
		GLAlloc->Init( (byte*)(GLAlloc + 1), MEGABYTE - sizeof(StackAllocator) );
	}
	if(!GL)
		GL = CreateGB(GLAlloc);

	auto AttributeCount = node->GetNodeAttributeCount();
	for (int itr = 0; itr < AttributeCount; ++itr){
		auto Attr = node->GetNodeAttributeByIndex(itr);
		switch (Attr->GetAttributeType())
		{
		case fbxsdk::FbxNodeAttribute::EType::eMesh:
		{
			const char* MeshName = node->GetName();
			auto test   = Attr->GetUniqueID();
			auto Mesh	= (fbxsdk::FbxMesh*)Attr;
			bool found	= false;
			size_t ID	= (size_t)Mesh;
			auto Geo	= FindGeoByID( GL, ID );

			if ( !Geo )
			{
				TriMesh	out;
				auto	Res		= CompileMeshResource(out, E->TempMem, E->MemoryOut, Mesh);
				
				auto Material	= E->Materials->GetDefaultMaterial();
				auto Shader		= (out.AnimationData & TriMesh::EAD_Skin) ? E->Materials->GetVertexShader_Animated(Material) : E->Materials->GetVertexShader(Material);
				FlexKit::CreateVertexBuffer(E->RS, out.Buffers, Res.BuffersFound, out.VertexBuffer);

				bool res = FlexKit::CreateInputLayout(E->RS, out.Buffers, Res.BuffersFound, Shader, &out.VertexBuffer);
				FK_ASSERT(res, "Failed to Create Input Layout!" );

				E->TempMem->clear();
				ClearTriMeshVBVs( &out );

				auto Name		= Mesh->GetName();
				size_t NameLen	= strlen( Name );
				if ( NameLen++ ) {
					out.ID = (char*)E->LevelMem->malloc( NameLen );
					strcpy_s( (char*)out.ID, NameLen, Name );
				}

				PushGeo( GL, out, E->TempMem );
			}
		}	break;
		}
	}

	size_t NodeCount = node->GetChildCount();
	for(int itr = 0; itr < NodeCount; ++itr)
		FindAllGeometry(node->GetChild(itr), E, GL, GLAlloc);

	return{ GetGeoCount(GL), {GL, GLAlloc} };
}


/************************************************************************************************/

struct CompiledMeshInfo
{
	FBXMeshDesc	MeshInfo;
	Skeleton*	S;
};

Pair<size_t, GBAPair> 
CompileAllGeometry(fbxsdk::FbxNode* node, BlockAllocator* Memory, GeometryBlock* GL, StackAllocator* TempMem, MetaData_list* MD = nullptr, bool SubDiv = false)
{
	using FlexKit::FillBuffer;
	using FlexKit::AnimationClip;
	using FlexKit::Skeleton;
	using MeshUtilityFunctions::BuildVertexBuffer;
	using MeshUtilityFunctions::CombinedVertexBuffer;
	using MeshUtilityFunctions::IndexList;
	using MeshUtilityFunctions::TokenList;
	using MeshUtilityFunctions::MeshBuildInfo;

	if(!GL)	GL = CreateGB(Memory);

	auto AttributeCount = node->GetNodeAttributeCount();
	for (int itr = 0; itr < AttributeCount; ++itr){
		auto Attr = node->GetNodeAttributeByIndex(itr);
		switch (Attr->GetAttributeType())
		{
		case fbxsdk::FbxNodeAttribute::EType::eMesh:
		{
			const char* MeshName = node->GetName();
			auto test   = Attr->GetUniqueID();
			auto Mesh	= (fbxsdk::FbxMesh*)Attr;
			bool found	= false;
			size_t ID	= (size_t)Mesh->GetUniqueID();
			auto Geo	= FindGeoByID( GL, ID );

			if ( !Geo )
			{
				auto Name		= Mesh->GetName();
				size_t NameLen	= strlen( Name );
				if (!NameLen) {
					Name = node->GetName();
					NameLen	= strlen( Name );
				}

				TriMesh	out;
				memset(&out, 0, sizeof(out));

				if ( NameLen++ ) {
					out.ID = (char*)Memory->malloc( NameLen );
					strcpy_s( (char*)out.ID, NameLen, Name );
				}

				auto Res = CompileMeshResource(out, TempMem, Memory, Mesh);

				PushGeo(GL, out, Memory);
			}
		}	break;
		}
	}

	size_t NodeCount = node->GetChildCount();
	for(int itr = 0; itr < NodeCount; ++itr)
		CompileAllGeometry(node->GetChild(itr), Memory, GL, TempMem);

	return{ GetGeoCount(GL), {GL, TempMem} };
}


/************************************************************************************************/


template<typename TY, size_t ALIGNMENT = 0x40>
TY& _AlignedAllocate()
{
	TY& V = new(_aligned_malloc(sizeof(TY, ALIGNMENT))) TY();
	return V;
}


/************************************************************************************************/


Pair<bool, GBAPair> LoadFBXGeometry(fbxsdk::FbxScene* S, BlockAllocator* MemoryOut, StackAllocator* MemoryTemp, StackAllocator* LevelMem, RenderSystem* RS, ShaderTable* MT,bool LoadSkeletalData = false, bool SUBDIV = false)
{
	Engine E = { MemoryOut, MemoryTemp, LevelMem, RS, MT };
	auto Res = FindAllGeometry(S->GetRootNode(), &E);

	return{ true, Res };
}


/************************************************************************************************/


size_t CalculateTriMeshSize(TriMesh* TriMesh)
{
	size_t Size = 0;
	for (auto B : TriMesh->Buffers)
		Size += B ? B->GetBufferSizeRaw() : 0;

	return Size;
}


/************************************************************************************************/


void FillTriMeshBlob(TriMeshResourceBlob* out, TriMesh* Mesh)
{
	size_t BufferPosition = 0;
	out->GUID = Mesh->TriMeshID;
	out->HasAnimation = Mesh->AnimationData > 0;
	out->HasIndexBuffer = true;
	out->BufferCount = 0;

	out->IndexCount = Mesh->IndexCount;
	out->Info.minx  = Mesh->Info.min.x;
	out->Info.miny  = Mesh->Info.min.y;
	out->Info.minz  = Mesh->Info.min.z;
	out->Info.maxx  = Mesh->Info.max.x;
	out->Info.maxy  = Mesh->Info.max.y;
	out->Info.maxz  = Mesh->Info.max.z;
	out->Info.r	    = Mesh->Info.r;

	if(Mesh->ID)
		strcpy_s(out->ID, ID_LENGTH, Mesh->ID);

	for (size_t I = 0; I < 16; ++I)
	{
		if (Mesh->Buffers[I])
		{
			memcpy(out->Memory + BufferPosition, Mesh->Buffers[I]->GetBuffer(), Mesh->Buffers[I]->GetBufferSizeRaw());

			auto View = Mesh->Buffers[I];
			out->Buffers[I].Begin  = BufferPosition;
			out->Buffers[I].Format = (size_t)Mesh->Buffers[I]->GetElementSize();
			out->Buffers[I].Type   = (size_t)Mesh->Buffers[I]->GetBufferType();
			out->Buffers[I].size   = (size_t)Mesh->Buffers[I]->GetBufferSizeRaw();
			BufferPosition		  += Mesh->Buffers[I]->GetBufferSizeRaw();

			out->BufferCount++;
		}
	}
}


/************************************************************************************************/


Resource* CreateTriMeshResourceBlob(TriMesh* Mesh, BlockAllocator* MemoryOut)
{
	size_t BlobSize          = CalculateTriMeshSize(Mesh);
	BlobSize				+= sizeof(TriMeshResourceBlob);
	char* Blob               = MemoryOut->_aligned_malloc(BlobSize);
	TriMeshResourceBlob* Res = (TriMeshResourceBlob*)Blob;


	memset(Blob, 0, BlobSize);
	Res->Type         = EResource_TriMesh;
	Res->ResourceSize = BlobSize;

	FillTriMeshBlob(Res, Mesh);

	return (Resource*)Res;
}


/************************************************************************************************/


size_t CalculateAnimationSize(Skeleton* S)
{
	size_t Size = 0;
	auto I = S->Animations;

	while (I)
	{
		for (size_t II = 0; II < I->Clip.FrameCount; ++II)
		{
			Size += I->Clip.Frames[II].JointCount * sizeof(JointPose);
			Size += I->Clip.Frames[II].JointCount * sizeof(JointHandle);
		}
		I = I->Next;
	}
	return Size;
}


/************************************************************************************************/


size_t CalculateSkeletonSize(Skeleton* S)
{
	size_t Size = 0;
	Size += S->JointCount * sizeof(SkeletonResourceBlob::JointEntry);

	for (size_t I = 0; I < S->JointCount; ++I)
		Size += strlen(S->Joints[I].mID);

	return Size;
}

Resource* CreateSkeletonResourceBlob(Skeleton* S, BlockAllocator* MemoryOut, size_t GUID)
{
	size_t Size = CalculateSkeletonSize(S);
	Size += sizeof(SkeletonResourceBlob);

	SkeletonResourceBlob* R = (SkeletonResourceBlob*)MemoryOut->_aligned_malloc(Size);
	R->GUID					= GUID;
	R->ResourceSize			= Size;
	R->Type					= EResource_Skeleton;
	R->JointCount			= S->JointCount;

	strcpy_s(R->ID, 64, "SKELETON");

	for (size_t I= 0; I < S->JointCount; ++I)
	{
		strcpy_s(R->Joints[I].ID, S->Joints[I].mID);
		R->Joints[I].Parent = S->Joints[I].mParent;
		memcpy(&R->Joints[I].IPose, &S->IPose[I], sizeof(float4x4));
		memcpy(&R->Joints[I].Pose, &S->JointPoses[I], sizeof(JointPose));
	}

	return (Resource*)R;
}


/************************************************************************************************/


size_t CalculateAnimationSize(AnimationClip* AC)
{
	size_t Size = 0;
	Size += AC->FrameCount * sizeof(AnimationResourceBlob::FrameEntry);
	Size += strlen(AC->mID);

	for (size_t I = 0; I < AC->FrameCount; ++I)
	{
		Size += AC->Frames[I].JointCount * sizeof(JointHandle);
		Size += AC->Frames[I].JointCount * sizeof(JointPose);
	}

	return Size;
}

Resource* CreateSkeletalAnimationResourceBlob(AnimationClip* AC, GUID_t Skeleton, BlockAllocator* MemoryOut)
{
	size_t Size = CalculateAnimationSize(AC);
	Size += sizeof(AnimationResourceBlob);

	AnimationResourceBlob* R = (AnimationResourceBlob*)MemoryOut->_aligned_malloc(Size);
	R->Skeleton		= Skeleton;
	R->FrameCount	= AC->FrameCount;
	R->FPS			= AC->FPS;
	R->ResourceSize = Size;
	R->GUID			= 0x0123;
	R->IsLooping	= AC->isLooping;
	R->Type			= EResourceType::EResource_SkeletalAnimation;
	strcpy_s(R->ID, AC->mID);

	AnimationResourceBlob::FrameEntry*	Frames = (AnimationResourceBlob::FrameEntry*)R->Buffer;
	size_t Position = sizeof(AnimationResourceBlob::FrameEntry) * R->FrameCount;

	for (size_t I = 0; I < R->FrameCount; ++I)
	{
		Frames[I].JointCount  = AC->Frames[I].JointCount;
		
		JointHandle* Joints   = (JointHandle*)(R->Buffer + Position);
		Frames[I].JointStarts = Position;
		Position += sizeof(JointHandle) * AC->Frames[I].JointCount;
		memcpy(Joints, AC->Frames[I].Joints, sizeof(JointHandle) * AC->Frames[I].JointCount);
	
		JointPose* Poses      = (JointPose*)(Joints + AC->Frames[I].JointCount);
		Frames[I].PoseStarts  = Position;
		Position += sizeof(JointPose) * AC->Frames[I].JointCount;
		memcpy(Poses, AC->Frames[I].Poses, sizeof(JointPose) * AC->Frames[I].JointCount);

		int x = 0;// Debug Point
	}

	return (Resource*)R;
}


/************************************************************************************************/


ResourceList CompileFBXGeometry(fbxsdk::FbxScene* S, BlockAllocator* MemoryOut, bool LoadSkeletalData = false, MetaData_list* MD = nullptr, bool SUBDIV = false)
{
	size_t TempMemorySize = MEGABYTE * 32;
	StackAllocator TempMemory;
	TempMemory.Init((byte*)_aligned_malloc(TempMemorySize, 0x40), TempMemorySize);

	auto Res = CompileAllGeometry(S->GetRootNode(), MemoryOut, nullptr, &TempMemory);

	ResourceList	ResourcesFound;
	if ((size_t)Res > 0)
	{
		auto G = (GeometryBlock*)(GBAPair)Res;
		while (G)
		{
			for (size_t I = 0; I < G->MeshUsed; ++I)
			{
				ResourcesFound.push_back(CreateTriMeshResourceBlob(G->Meshes + I, MemoryOut));

				if (G->Meshes[I].Skeleton) {
					auto Res = CreateSkeletonResourceBlob(G->Meshes[I].Skeleton, MemoryOut, G->Meshes[I].TriMeshID);
					ResourcesFound.push_back(Res);

					auto CurrentClip = G->Meshes[I].Skeleton->Animations;
					auto SkeletonID = ResourcesFound.back()->GUID;

					while (true)
					{
						if (CurrentClip)
						{
							auto R = CreateSkeletalAnimationResourceBlob(&CurrentClip->Clip, SkeletonID, MemoryOut);
							ResourcesFound.push_back(R);
						} else
							break;
						CurrentClip = CurrentClip->Next;
					}
				}

				if (G->Meshes[I].AnimationData) {
				}
			}
			if (G->MeshUsed == 10)
				G = G->Next;
			else
				break;
		}
	}

	return ResourcesFound;
}


/************************************************************************************************/


struct LoadGeometry_RES
{
	fbxsdk::FbxManager*		Manager;
	fbxsdk::FbxIOSettings*	Settings;
	fbxsdk::FbxScene*		INScene;

	ResourceList Resources;
};


LoadGeometryRES_ptr CompileGeometryFromFBXFile(char* AssetLocation, CompileSceneFromFBXFile_DESC* Desc, MetaData_list* METAINFO)
{
	size_t MaxMeshCount					= 100;
	fbxsdk::FbxManager*		Manager     = fbxsdk::FbxManager::Create();
	fbxsdk::FbxIOSettings*	Settings	= fbxsdk::FbxIOSettings::Create(Manager, IOSROOT);
	fbxsdk::FbxScene*		Scene       = nullptr;

	Manager->SetIOSettings(Settings);
	FINALLY{
		Manager->Destroy();
	}FINALLYOVER;

	auto res = LoadFBXScene( AssetLocation, Manager, Settings );
	if (res)
	{
		auto LoadRes = CompileFBXGeometry((FbxScene*)res, Desc->BlockMemory,true, METAINFO);

		size_t ResourceCount = 0;
		size_t FileSize		 = 0;
		for (auto& G : LoadRes)
		{
			ResourceCount++;
			FileSize += G->ResourceSize;
		}
		//{ Manager, Settings, Scene, std::move(LoadRes) }
		auto G = &Desc->BlockMemory->allocate_aligned<LoadGeometry_RES>();
		G->Manager = Manager;
		G->INScene = Scene;
		G->Resources = LoadRes;
		G->Settings = Settings;
		return  LoadGeometryRES_ptr(G);
	}
	return LoadGeometryRES_ptr(nullptr);
}


/************************************************************************************************/

using FlexKit::ShaderTable;
using FlexKit::ShaderSetHandle;

SceneStats ProcessSceneNodes(fbxsdk::FbxScene* scene, Scene* SceneOut, fbxsdk::FbxNode* Node, NodeHandle ParentNode, SceneNodes* Nodes, ShaderSetHandle DefaultMaterial, RenderSystem* RS, bool CreateEntities = true)
{
	using FlexKit::SetParentNode;
	SceneStats Stats = {};

	if (Node)
	{
		int AttributeCount	= Node->GetNodeAttributeCount();
		int ChildCount		= Node->GetChildCount();
		if (ChildCount || AttributeCount)
		{
			NodeHandle Nodehndl = GetZeroedNode(Nodes);
			SetParentNode(Nodes, ParentNode, Nodehndl);

			auto Position = Node->LclTranslation.	Get();
			auto LclScale = Node->LclScaling.		Get();
			auto rotation = Node->LclRotation.		Get();

			TranslateWorld	(Nodes, Nodehndl,				{Position.mData[0], Position.mData[1], Position.mData[2]});
			Scale			(Nodes, Nodehndl,				{LclScale.mData[0], LclScale.mData[1], LclScale.mData[2]});
			SetOrientation(Nodes, Nodehndl,		Quaternion	{rotation.mData[0], rotation.mData[1], rotation.mData[2]});

			size_t itr = 0;
			for (; itr < AttributeCount; ++itr)
			{
				auto Attr = Node->GetNodeAttributeByIndex(itr);
				auto AttrType = Attr->GetAttributeType();
				switch (AttrType)
				{
				case FbxNodeAttribute::eMesh:
				{
					std::cout << "Entity Found: " << Node->GetName() << "\n";
					
					auto EntityHandle = SceneOut->GetFreeEntity();
					if (EntityHandle) {
						auto NodeName	 = Node->GetName();
						auto Entity		 = SceneOut->GetEntity(EntityHandle);
						Entity->Posed = false;

						if ( NodeName ){
							size_t NodeStrLen = strnlen( NodeName, 0x40 );
							strncpy_s( SceneOut->GetEntityID( EntityHandle ), 64, NodeName, NodeStrLen);
						}
						
						if (CreateEntities)
						{
							CreateEntity(RS, Entity, EntityDesc{ DefaultMaterial });
							auto TriHandle = SearchForMesh(SceneOut, Attr->GetUniqueID());
							if ( TriHandle ) {
								Entity->Node		= Nodehndl;
								Entity->Mesh		= SceneOut->GetTriMesh(TriHandle);
							}
						}

						++Stats.EntityCount;
					}
#ifdef _DEBUG
					else
						FK_ASSERT(0, "MET MAX ENTITY COUNT/n");
#endif				
				}	break;
				case FbxNodeAttribute::eLight:
				{
					std::cout << "Light Found: " << Node->GetName() << "\n";

					auto Light				= static_cast<fbxsdk::FbxLight*>(Attr);
					auto Type				= Light->LightType.	Get();
					auto Cast				= Light->CastLight.	Get();
					auto I					= Light->Intensity.	Get();
					auto K					= Light->Color.		Get();
					auto R					= Light->OuterAngle.Get();

					auto Lighthndl			= SceneOut->GetFreeLight();
					auto SceneLight			= SceneOut->GetPLight	( Lighthndl );
					SceneLight->K			= TranslateToFloat3		( K );			// COLOR for the Confused
					SceneLight->I			= I;
					SceneLight->Position	= Nodehndl;
					SceneLight->R			= I * 100;

					++Stats.LightCount;
				}	break;
				case FbxNodeAttribute::eSkeleton:
				{

				}	break;
				case FbxNodeAttribute::eMarker:
				case FbxNodeAttribute::eUnknown:
				default:
					break;
				}
			}

			for (size_t itr = 0; itr < ChildCount; ++itr)
				Stats += ProcessSceneNodes(scene, SceneOut, Node->GetChild(itr), Nodehndl, Nodes, DefaultMaterial, RS, CreateEntities);

		}
	}
	return Stats;
}


/************************************************************************************************/


struct SceneGeometry
{
	TriMesh*	Meshes;
	size_t		MeshCount;
};

void CountNodeContents(fbxsdk::FbxNode* N, Scene_Desc& Desc)
{
	size_t AttribCount = N->GetNodeAttributeCount();
	for (size_t I = 0; I < AttribCount; ++I)
	{
		auto A = N->GetNodeAttributeByIndex(I);
		switch (A->GetAttributeType())
		{
		case fbxsdk::FbxNodeAttribute::EType::eLight:
			Desc.MaxPointLightCount++; 	break;
		case fbxsdk::FbxNodeAttribute::EType::eSkeleton:
			Desc.MaxSkeletonCount++;	break;
		case fbxsdk::FbxNodeAttribute::EType::eMesh:
			Desc.MaxTriMeshCount++;		break;
		}
	}

	size_t NodeCount = N->GetChildCount();
	for (size_t I = 0; I < NodeCount; ++I)
		CountNodeContents(N->GetChild(I), Desc);
}


/************************************************************************************************/


Scene_Desc CountSceneContents(fbxsdk::FbxScene* S)
{
	Scene_Desc	Desc = {};
	size_t NodeCount = S->GetNodeCount();
	for (size_t I = 0; I < NodeCount; ++I)
		CountNodeContents(S->GetNode(I), Desc);

	return Desc;
}


/************************************************************************************************/


Pair<bool, SceneStats>
TranslateFBXScene(fbxsdk::FbxScene* S, Scene* SceneOut, NodeHandle SceneRoot, SceneNodes* Nodes, ShaderSetHandle DefaultMat, RenderSystem* RS)
{
	fbxsdk::FbxNode* node = S->GetRootNode();
	auto SceneInfo = ProcessSceneNodes(S, SceneOut, node, SceneRoot,  Nodes, DefaultMat, RS, true);
	return{ true, SceneInfo };
}


/************************************************************************************************/


void CleanUp(Scene* Scene)
{
	for (size_t I = 0; I < Scene->EntitiesUsed; ++I) CleanUpEntity(Scene->Entities + I);
	for (size_t I = 0; I < Scene->GeometryUsed; ++I) CleanUpTriMesh(Scene->Geometry + I);
}


/************************************************************************************************/


Scene* LoadSceneFromFBXFile(char* AssetLocation, SceneNodes* Nodes, RenderSystem* RS, LoadSceneFromFBXFile_DESC* Desc)
{
	size_t MaxMeshCount					= 10;
	fbxsdk::FbxManager*		Manager     = fbxsdk::FbxManager::Create();
	fbxsdk::FbxIOSettings*	Settings	= fbxsdk::FbxIOSettings::Create(Manager, IOSROOT);
	fbxsdk::FbxScene*		INScene     = nullptr;

	Manager->SetIOSettings(Settings);

	auto res = LoadFBXScene( AssetLocation, Manager, Settings );
	if (res)
	{
		Scene* NewScene = &Desc->LevelMem->allocate_aligned<Scene>();
		auto LoadRes = LoadFBXGeometry((FbxScene*)res, Desc->BlockMemory, Desc->TempMem, Desc->LevelMem, Desc->RS, Desc->ST);

		Scene_Desc desc = CountSceneContents(res);

		desc.Root				= Desc->SceneRoot;
		desc.SceneMemory		= Desc->BlockMemory;
		desc.MaxTriMeshCount	= GetGeoCount((GBAPair)LoadRes);
		desc.MaxEntityCount		= 24;
		desc.MaxSkeletonCount	= desc.MaxSkeletonCount;
		desc.MaxPointLightCount	= desc.MaxPointLightCount;
		
		InitiateScene(NewScene, Desc->RS, Desc->BlockMemory, &desc);

		NewScene->GeometryUsed = GetGeoCount((GBAPair)LoadRes);
		MoveGeo((GBAPair)LoadRes, NewScene->Geometry);
		Desc->BlockMemory->free((StackAllocator*)(GBAPair)LoadRes); // Need to Handle Aligned Cases first

		auto SceneData  = TranslateFBXScene(GetByType<FbxScene*>(res), NewScene, Desc->SceneRoot, Nodes, Desc->DefaultMaterial, RS);
		Manager->Destroy();

		return NewScene;
	}
	return nullptr;
}


/************************************************************************************************/


FileDir SelectFile()
{
	IFileDialog* pFD = nullptr;
	FK_ASSERT( SUCCEEDED( CoInitialize( nullptr ) ) );
	auto HRES = CoCreateInstance( CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFD) );

	if ( SUCCEEDED( HRES ) )
	{
		DWORD Flags;
		HRES = pFD->GetOptions( &Flags );

		if ( SUCCEEDED( HRES ) )
		{
			pFD->SetOptions( FOS_FILEMUSTEXIST | FOS_FORCEFILESYSTEM );
		} else return FileDir();
		
		FINALLY{
			if(pFD)
				pFD->Release();
		}FINALLYOVER;

		IShellItem* pItem = nullptr;

		WCHAR CurrentDirectory[ 256 ];
		GetCurrentDirectoryW( 256, CurrentDirectory );
			
		HRES = pFD->GetFolder(&pItem);
		if ( SUCCEEDED( HRES ) )
		{
			HRES = SHCreateItemFromParsingName( CurrentDirectory, nullptr, IID_PPV_ARGS( &pItem ) );
			pFD->SetDefaultFolder(pItem);
		}

		FINALLY{
			if(pItem)
				pItem->Release();
		}FINALLYOVER;

		HRES = pFD->Show( nullptr );
		if ( SUCCEEDED( HRES ) )
		{
			IShellItem* pItem = nullptr;
			HRES = pFD->GetResult( &pItem );
			if(SUCCEEDED(HRES))
			{
				FINALLY{
				if(pItem)
					pItem->Release();
				}FINALLYOVER;

				PWSTR FilePath;
				HRES = pItem->GetDisplayName( SIGDN_FILESYSPATH, &FilePath );
				if ( SUCCEEDED( HRES ) )
				{
					size_t length = wcslen( FilePath );
					FileDir	Dir;
					BOOL DefaultCharUsed = 0;
					CCHAR	DChar = ' ';
					auto res = WideCharToMultiByte( CP_UTF8, 0, FilePath, length, Dir.str, 256, nullptr, nullptr);
					if ( !res )
					{
						IErrorInfo*	INFO = nullptr;
						auto Err = GetLastError();
					}
					Dir.str[length] = '\0';
					Dir.Valid = true;
					return Dir;
				}
			}
		}
	}
	return FileDir();
}


/************************************************************************************************/


namespace FontUtilities
{
	/************************************************************************************************/


	struct INFOBLOCK
	{
		int16_t		FontSize;
		uint8_t		BitField;
		uint8_t		CharSet;
		uint16_t	stretchH;
		uint8_t		AA;
		uint8_t		PaddingUp;
		uint8_t		PaddingRight;
		uint8_t		PaddingDown;
		uint8_t		PaddingLeft;
		uint8_t		SpacingHoriz;
		uint8_t		SpacingVert;
		uint8_t		Outline;
		char		fontName;
	};


	/************************************************************************************************/


	struct COMMONBLOCK
	{
		uint16_t	lineHeight;
		uint16_t	Base;
		uint16_t	ScaleW;
		uint16_t	ScaleH;
		uint16_t	Pages;
		uint8_t		BitField;
		uint8_t		alphaChannel;
		uint8_t		RedChannel;
		uint8_t		GreenChannel;
		uint8_t		BlueChannel;
	};


	/************************************************************************************************/

	struct CHARARRAYBLOCK
	{
		#pragma pack(push, 1)
		struct CHAR
		{
			uint32_t	id;
			uint16_t	x;
			uint16_t	y;
			uint16_t	width;
			uint16_t	height;
			uint16_t	xoffset;
			uint16_t	yoffset;
			uint16_t	xadvance;
			uint8_t		page;
			uint8_t		chnl;
		}First;
		#pragma pack(pop)
	};


	/************************************************************************************************/


	struct KERNINGENTRY
	{
		uint32_t FirstChar;
		uint32_t SecondChar;
		uint16_t Amount;
	};

	struct KERNINGARRAYBLOCK
	{
		KERNINGENTRY First;
	};


	/************************************************************************************************/


	LoadFontResult LoadFontAsset(char* dir, char* file, RenderSystem* RS, StackAllocator* tempMem, StackAllocator* outMem)
	{
		char TEMP[256];
		strcpy_s(TEMP, dir);
		strcat_s(TEMP, file);

		size_t Size = 1 + GetFileSize(TEMP);
		byte* mem = (byte*)tempMem->malloc(Size);
		FlexKit::LoadFileIntoBuffer(TEMP, mem, Size);

		char*	FontPath	= nullptr;
		size_t  PathLength	= strlen(dir);

		INFOBLOCK	Info;
		COMMONBLOCK	CB;

		FontAsset*		Fonts		= nullptr;
		size_t			FontCount	= 0;

		KERNINGENTRY*	KBlocks		= nullptr;
		size_t			KBlockUsed	= 0;

		FlexKit::uint4	Padding = 0;// Ordering { Top, Left, Bottom, Right }

		char ID[128];
		#pragma pack(push, 1)
		struct BlockDescriptor
		{
			char	 ID;
			uint32_t BlockSize;
		};
		#pragma pack(pop)

		if (mem[0] == 'B' &&
			mem[1] == 'M' &&
			mem[2] == 'F' &&
			mem[3] == 0x03)
		{
			size_t Position = 4;
			while (Position < Size)
			{
				BlockDescriptor* Desc = (BlockDescriptor*)(mem + Position);
				switch (Desc->ID)
				{
				case 0x01:
				{
					auto test = sizeof(BlockDescriptor);
					INFOBLOCK* IB = (INFOBLOCK*)(mem + Position + sizeof(BlockDescriptor));
					char* FontName = &IB->fontName;
					strcpy_s(ID, FontName);

					Padding[0] = IB->PaddingUp;
					Padding[1] = IB->PaddingLeft;
					Padding[2] = IB->PaddingDown;
					Padding[3] = IB->PaddingRight;

					Info = *IB;
				}break;
				case 0x02:
				{
					COMMONBLOCK* pCB	= (COMMONBLOCK*)(mem + Position + sizeof(BlockDescriptor));

					FontCount	= pCB->Pages;
					Fonts		= (FontAsset*)outMem->malloc(sizeof(FontAsset) * FontCount);

					for (size_t I = 0; I < FontCount; ++I) {
						Fonts[I].Padding = Padding;
						strcpy_s(Fonts[I].FontName, ID);
					}

					CB = *pCB;
				}break;
				case 0x03:
				{
					int32_t BlockSize	= Desc->BlockSize;
					char*	FONTPATH	= (char*)(mem + Position + sizeof(BlockDescriptor));
					size_t	FontPathLen	= strnlen_s(FONTPATH, BlockSize);

					for (size_t I = 0; I < FontCount; ++I) {
						size_t BufferSize = FontPathLen + PathLength + 1;
						Fonts[I].FontDir = (char*)outMem->malloc(BufferSize);

						strcpy_s(Fonts[I].FontDir, BufferSize, dir);
						strcat_s(Fonts[I].FontDir, BufferSize, FONTPATH + I * FontPathLen);

						auto res = LoadTextureFromFile(Fonts[I].FontDir, RS);
						Fonts[I].Text = res;
						Fonts[I].TextSheetDimensions = {CB.ScaleW, CB.ScaleH};
					}
				}break;
				case 0x04:
				{
					CHARARRAYBLOCK* pCAB = (CHARARRAYBLOCK*)(mem + Position + sizeof(BlockDescriptor));
					size_t End = Desc->BlockSize / sizeof(CHARARRAYBLOCK::CHAR);
					auto Begin = &pCAB->First;
					for (size_t I = 0; I < End; ++I)
					{
						auto pCB = Begin + I;
						Fonts[pCB->page].GlyphTable[pCB->id].Channel  = pCB->chnl;
						Fonts[pCB->page].GlyphTable[pCB->id].Offsets  = { float(pCB->xoffset),	float(pCB->yoffset) };
						Fonts[pCB->page].GlyphTable[pCB->id].WH       = { float(pCB->width),	float(pCB->height) };
						Fonts[pCB->page].GlyphTable[pCB->id].XY       = { float(pCB->x),		float(pCB->y) };
						Fonts[pCB->page].GlyphTable[pCB->id].Xadvance = pCB->xadvance;
					}
				}break;
				case 0x05:
				{
					KERNINGARRAYBLOCK* pKAB = (KERNINGARRAYBLOCK*)(mem + Position + sizeof(BlockDescriptor));
				}break;
				default:
					break;
				}
				Position = Position + sizeof(BlockDescriptor) + Desc->BlockSize;

			}
		}

		//LoadTextureFromFile();
		return{ FontCount, Fonts };
	}


	/************************************************************************************************/


	void FreeFontAsset(FontAsset* asset)
	{
		FreeTexture(&asset->Text);
	}


	/************************************************************************************************/
}// namespace FontUtilities