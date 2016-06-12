
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

#include "PxPhysicsAPI.h"

#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "Shell32.lib")

// FBX Loading
#include				"fbxsdk/include/fbxsdk.h"
#pragma comment	(lib,	"libfbxsdk.lib")

// PhysX Cooking Deps
#ifdef _DEBUG
#pragma comment(lib,	"legacy_stdio_definitions.lib"		)
#pragma comment(lib,	"PhysX3CommonDEBUG_x64.lib"			)
#pragma comment(lib,	"PhysX3CookingDEBUG_x64.lib"		)

#else
#pragma comment(lib,	"legacy_stdio_definitions.lib"		)
#pragma comment(lib,	"PhysX3Common_x64.lib"				)
#pragma comment(lib,	"PhysX3Cooking_x64.lib"				)
#endif

using namespace FlexKit;



/************************************************************************************************/


struct FBXIDTranslation
{
	size_t	FBXID;
	GUID_t	Guid;
};
typedef DynArray<FBXIDTranslation> FBXIDTranslationTable;


GUID_t	TranslateID(size_t FBXID, FBXIDTranslationTable& Table)
{
	for (auto ID : Table)
		if (ID.FBXID == FBXID)
			return ID.Guid;

	return INVALIDHANDLE;
}

bool FBXIDPresentInTable(size_t FBXID, FBXIDTranslationTable& Table)
{
	for (auto ID : Table)
		if (ID.FBXID == FBXID)
			return true;

	return false;
}

/************************************************************************************************/


void InitiateScene(ResourceScene* out, RenderSystem* RS, iAllocator* memory, Scene_Desc* desc )
{
	desc->MaxPointLightCount = FlexKit::max(1, desc->MaxPointLightCount);

	out->Root			= desc->Root;
	out->MaxDrawables	= desc->MaxEntityCount;
	out->MaxObjects		= desc->MaxTriMeshCount;
	out->Drawables		= (Drawable*)memory->_aligned_malloc( sizeof(Drawable)  * desc->MaxEntityCount	);

	if(desc->MaxTriMeshCount)
		out->Geometry = (TriMesh*)memory->_aligned_malloc( sizeof(TriMesh) * desc->MaxTriMeshCount	);
	
	char*  IDs		   = (char*)memory->_aligned_malloc (out->MaxDrawables * 64);
	out->EntityIDs	   = IDs;
	memset( IDs, 0x00, out->MaxDrawables * 64 );

	byte*  LightBuffer = (byte*)memory->_aligned_malloc (( sizeof(Drawable) + 9 ) * desc->MaxPointLightCount );
	CreatePointLightBuffer(		
		RS, 
		&out->PLightBuffer, 
		{ desc->MaxPointLightCount }, memory);

	out->Alloc = memory;
}


/************************************************************************************************/


void UpdateScene(ResourceScene* In, RenderSystem* RS, SceneNodes* Nodes )
{ 
	//UpdatePointLightBuffer( *RS, Nodes, &In->PLightBuffer ); 
}


/************************************************************************************************/


void CleanUpScene(ResourceScene* scn, EngineMemory* memory )
{
	if (!scn)
		return;

	for( size_t itr = 0; itr < scn->DrawablesUsed; ++itr)
		CleanUpDrawable(scn->Drawables + itr);
	for( size_t itr = 0; itr < scn->GeometryUsed; ++itr)
		CleanUpTriMesh( scn->Geometry + itr );

	CleanUp( &scn->PLightBuffer, memory->BlockAllocator );

	if(scn->Drawables)				scn->Alloc->_aligned_free(scn->Drawables);
	if(scn->PLightBuffer.Lights)	scn->Alloc->_aligned_free(scn->PLightBuffer.Lights);
	if(scn->Geometry)				scn->Alloc->_aligned_free(scn->Geometry);
	if(scn->EntityIDs)				scn->Alloc->_aligned_free(scn->EntityIDs);

	scn->Drawables				= nullptr;
	scn->PLightBuffer.Lights	= nullptr;
	scn->Geometry				= nullptr;
	scn->EntityIDs				= nullptr;
}


/************************************************************************************************/


Pair<bool, SceneHandle> 
SearchForMesh(ResourceScene* Scn, size_t TriMeshID)
{
	for ( size_t itr = 0; itr < Scn->GeometryUsed; ++itr )
		if ( Scn->Geometry[ itr ].TriMeshID == TriMeshID )
			return{ true, SceneHandle( itr ) };

	return{ false, SceneHandle(0) };
}

Pair<bool, SceneHandle>	
SearchForEntity(ResourceScene* Scn, char* ID )
{
	for( size_t itr = 0; itr < Scn->DrawablesUsed; ++itr )
		if( strcmp( Scn->EntityIDs + itr * 64, ID ) == 0 )
			return{true, SceneHandle( itr ) };

	return {false, SceneHandle (-1)};
}


/************************************************************************************************/


TriMesh* SearchForMesh(ResourceScene* Scene, const char* str)
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
	iAllocator*		MemoryOut;
	iAllocator*		TempMem;
	iAllocator*		LevelMem;
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


using FlexKit::MeshUtilityFunctions::IndexList;
using FlexKit::MeshUtilityFunctions::CombinedVertexBuffer;
// Simple Buffer Filling Utility Functions
uint32_t	FetchIndex			(size_t itr, fbxsdk::FbxMesh* Mesh)		{return Mesh->GetPolygonVertex(itr/3, itr % 3);}
uint32_t	FetchIndex2			(size_t itr, IndexList* IL)				{return IL->at(itr);}
float3		FetchVertexPOS		(size_t itr, CombinedVertexBuffer* Buff){return Buff->at(itr).POS;}
float3		FetchWeights		(size_t itr, CombinedVertexBuffer* Buff){return Buff->at(itr).WEIGHTS;}
float3		FetchVertexNormal	(size_t itr, CombinedVertexBuffer* Buff){return Buff->at(itr).NORMAL;}
float3		FetchFloat3ZERO		(size_t itr, CombinedVertexBuffer* Buff){return{ 0.0f, 0.0f, 0.0f };}
float2		FetchVertexUV		(size_t itr, CombinedVertexBuffer* Buff){auto temp = Buff->at(itr).TEXCOORD.xy();return {temp.x, temp.y};}
uint4_16	FetchWeightIndices	(size_t itr, CombinedVertexBuffer* Buff){return Buff->at(itr).WIndices;}
uint32_t	WriteIndex			(uint32_t in)	{return in;}
float3		WriteVertex			(float3 in)		{return in;}
float2		WriteUV				(float2 in)		{return in;}

/************************************************************************************************/
// Complexe Buffer Filling Utility Functions

uint4_16	Writeuint4			(uint4_16 in)
{
	return uint4_16{ 
		(unsigned short)((in[0] == -1) ? 0 : in[0]), 
		(unsigned short)((in[1] == -1) ? 0 : in[1]), 
		(unsigned short)((in[2] == -1) ? 0 : in[2]), 
		(unsigned short)((in[3] == -1) ? 0 : in[3]) };
}


/************************************************************************************************/


size_t		GetVertexIndex(size_t pIndex, size_t vIndex, size_t vID, fbxsdk::FbxMesh* Mesh) { return Mesh->GetPolygonVertex(pIndex, vIndex); }


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

FBXSkinDeformer CreateSkin(fbxsdk::FbxMesh* Mesh, iAllocator* TempMem)
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


FBXMeshDesc TranslateToTokens(fbxsdk::FbxMesh* Mesh, iAllocator* TempMem, MeshUtilityFunctions::TokenList& TokensOut, Skeleton* S = nullptr, bool SubDiv_Enabled = false)
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
					Weights[Skin.Bones[I].WeightIndices[II]][III] = Skin.Bones[I].Weights[II];
					BoneIndices[Skin.Bones[I].WeightIndices[II]][III] = Handle;
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
		size_t FaceCount   = 0;

		// Iterate through each Tri
		for (size_t I = 0; I < TriCount; ++I)
		{	// Process each Vertex in tri
			unsigned int size = Mesh->GetPolygonSize( I );
			++FaceCount;

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
				
				AddIndexToken(VertexIndex1, NormalIndex1, UVCordIndex1, TokensOut);
				AddIndexToken(VertexIndex3, NormalIndex3, UVCordIndex3, TokensOut);
				AddIndexToken(VertexIndex2, NormalIndex2, UVCordIndex2, TokensOut);

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

		out.FaceCount = FaceCount;
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


void GetJointTransforms(static_vector<JointInfo, 1024>& Out, FbxMesh* M, iAllocator* MEM)
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
		JointAnimation& Animations = GetJointAnimation(N, MEM);

		Out.push_back({ {NewJoint}, Animations, DirectX::XMMatrixIdentity() });

		for ( size_t I = 0; I < ChildCount; ++I )
			FindAllJoints(Out, N->GetChild( I ), MEM, JointIndex);
	}
}


/************************************************************************************************/


struct AnimationCut
{
	double	T_Start;
	double	T_End;
	char*	ID;
	GUID_t	guid;
};

typedef DynArray<AnimationCut> CutList;


RelatedMetaData GetAllAnimationClipMetaData(MD_Vector* MD, RelatedMetaData* RD, iAllocator* Memory)
{
	RelatedMetaData Out(Memory);

	for (auto I : *RD)
		if (MD->at(I)->type == MetaData::EMETAINFOTYPE::EMI_ANIMATIONCLIP)
			Out.push_back(I);

	return Out;
}

void GetAnimationCuts(CutList* out, MD_Vector* MD, const char* ID, iAllocator* Mem)
{
	if (MD)
	{
		auto Related		= FindRelatedMetaData(MD, MetaData::EMETA_RECIPIENT_TYPE::EMR_SKELETALANIMATION, ID, Mem);
		auto AnimationClips = GetAllAnimationClipMetaData(MD, &Related, Mem);

		for (auto I : AnimationClips)
		{
			AnimationClip_MetaData* Clip = (AnimationClip_MetaData*)MD->at(I);
			AnimationCut	NewCut = {};
			NewCut.ID		= Clip->ClipID;
			NewCut.T_Start	= Clip->T_Start;
			NewCut.T_End	= Clip->T_End;
			NewCut.guid		= Clip->guid;
			out->push_back(NewCut);
		}
	}
}


/************************************************************************************************/


Skeleton_MetaData* GetSkeletonMetaData(MD_Vector* MD, RelatedMetaData* RD)
{
	for (auto I : *RD)
		if (MD->at(I)->type == MetaData::EMETAINFOTYPE::EMI_SKELETAL)
			return (Skeleton_MetaData*)MD->at(I);

	return nullptr;
}

FlexKit::Skeleton* LoadSkeleton(FbxMesh* M, iAllocator* Mem, iAllocator* Temp, const char* ParentID = nullptr, MD_Vector* MD = nullptr)
{
	using FlexKit::AnimationClip;
	using FlexKit::Skeleton;

	// Gather MetaData
	auto Related		= FindRelatedMetaData(MD, MetaData::EMETA_RECIPIENT_TYPE::EMR_SKELETON, ParentID, Temp);
	auto SkeletonInfo	= GetSkeletonMetaData(MD, &Related);

	auto Root	= FindSkeletonRoot(M);
	if (!Root || !SkeletonInfo)
		return nullptr;

	auto& Joints = Mem->allocate_aligned<static_vector<JointInfo, 1024>>();
	FindAllJoints		(Joints, Root, Temp);
	GetJointTransforms	(Joints, M, Temp);

	Skeleton* S = (Skeleton*)Mem->_aligned_malloc(0x40);
	S->InitiateSkeleton(Mem, Joints.size());

	for (auto J : Joints)
		S->AddJoint(J.Joint.Linkage, J.Inverse);
	
	char* ID = SkeletonInfo->SkeletonID;
	S->guid  = SkeletonInfo->SkeletonGUID;

	for (size_t I = 0; I < Joints.size(); ++I)
	{
		size_t ID_Length = strnlen_s(S->Joints[I].mID, 64) + 1;
		char* ID		 = (char*)Mem->malloc(ID_Length);

		strcpy_s(ID, ID_Length, S->Joints[I].mID);
		S->Joints[I].mID = ID;
	}

	CutList Cuts(Mem);
	GetAnimationCuts(&Cuts, MD, ID, Mem);

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
		Clip.guid		= Cut.guid;
		Clip.isLooping	= true;
		Clip.Frames		= (AnimationClip::KeyFrame*)Mem->_aligned_malloc(Clip.FrameCount * sizeof(AnimationClip::KeyFrame));

		for (size_t I = 0; I < Clip.FrameCount; ++I)
		{
			Clip.Frames[I].Joints		= (JointHandle*)Mem->_aligned_malloc(sizeof(JointHandle) * Joints.size());
			Clip.Frames[I].Poses		=   (JointPose*)Mem->_aligned_malloc(sizeof(JointPose)	 * Joints.size());
			Clip.Frames[I].JointCount	= Joints.size();

			for (size_t II = 0; II < Joints.size(); ++II)
			{
				Clip.Frames[I].Joints[II]	= JointHandle(II);
				
				auto DEBUGVIEW = Joints[II].Animation.Poses[I + Begin].JPose;
				auto Pose					= GetTransform(&Joints[II].Animation.Poses[I + Begin].JPose);
				auto LocalPose				= GetPose(Joints[II].Inverse * Pose);
				Clip.Frames[I].Poses[II]	= LocalPose;
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

CompileMeshInfo CompileMeshResource(TriMesh& out, iAllocator* TempMem, iAllocator* Memory, FbxMesh* Mesh, bool EnableSubDiv = false, const char* ID = nullptr, MD_Vector* MD = nullptr)
{
	using FlexKit::FillBufferView;
	using FlexKit::AnimationClip;
	using FlexKit::Skeleton;
	using MeshUtilityFunctions::BuildVertexBuffer;
	using MeshUtilityFunctions::CombinedVertexBuffer;
	using MeshUtilityFunctions::IndexList;
	using MeshUtilityFunctions::TokenList;
	using MeshUtilityFunctions::MeshBuildInfo;

	Skeleton*	S		= LoadSkeleton(Mesh, Memory, TempMem, ID, MD);
	TokenList& Tokens	= TokenList::Create_Aligned(2048000, TempMem);
	auto MeshInfo		= TranslateToTokens(Mesh, TempMem, Tokens, S);

	CombinedVertexBuffer& CVB = CombinedVertexBuffer::Create_Aligned(1024000, TempMem);
	IndexList& IB			  = IndexList::Create_Aligned(MeshInfo.FaceCount * 8, TempMem);

	auto BuildRes = MeshUtilityFunctions::BuildVertexBuffer(Tokens, CVB, IB, TempMem, TempMem, MeshInfo.Weights);
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
		BuffersFound.push_back({ VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R16G16B16A16 });
	}

	for (size_t i = 0; i < BuffersFound.size(); ++i) {
		CreateBufferView(VertexCount, Memory, out.Buffers[i], (VERTEXBUFFER_TYPE)BuffersFound[i], (VERTEXBUFFER_FORMAT)BuffersFound[i]);

		switch ((VERTEXBUFFER_TYPE)BuffersFound[i])
		{
		case  VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION:
			FillBufferView(&CVB, CVB.size(), out.Buffers[i], WriteVertex, FetchVertexPOS);		break;
		case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL:
			FillBufferView(&CVB, CVB.size(), out.Buffers[i], WriteVertex, FetchVertexNormal);	break;
		case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV:
			FillBufferView(&CVB, CVB.size(), out.Buffers[i], WriteUV, FetchVertexUV);			break;
		case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT:
			FillBufferView(&CVB, CVB.size(), out.Buffers[i], WriteVertex, FetchFloat3ZERO);		break;
		case  VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1:
			FillBufferView(&CVB, CVB.size(), out.Buffers[i], WriteVertex, FetchWeights);		break;
		case  VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2:
			FillBufferView(&CVB, CVB.size(), out.Buffers[i], Writeuint4, FetchWeightIndices);	break;
		default:
			break;
		}
	}

	CreateBufferView(IB.size(), Memory, out.Buffers[15], VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32);
	FillBufferView(&IB, IB.size(), out.Buffers[15], WriteIndex, FetchIndex2);

	
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
	out.SkeletonGUID  = S ? S->guid : -1;

	return {true, BuffersFound.size()};
}


/************************************************************************************************/


struct CompiledMeshInfo
{
	FBXMeshDesc	MeshInfo;
	Skeleton*	S;
};

DynArray<size_t> FindRelatedMetaData(MD_Vector* MetaData, MetaData::EMETA_RECIPIENT_TYPE Type, const char* ID, iAllocator* TempMem)
{
	DynArray<size_t> RelatedData(TempMem);
	size_t IDLength = strlen(ID);

	for (size_t I = 0; I < MetaData->size(); ++I)
	{
		auto& MD = (*MetaData)[I];
		if (MD->UserType == Type)
			if (!strncmp(MD->ID, ID, IDLength) )
				RelatedData.push_back(I);
	}

	return RelatedData;
}

Mesh_MetaData* GetMeshMetaData(MD_Vector* MetaData, DynArray<size_t>& related)
{
	for (auto i : related)
	{
		if(MetaData->at(i)->type == MetaData::EMETAINFOTYPE::EMI_MESH)
			return (Mesh_MetaData*)MetaData->at(i);
	}
	return nullptr;
}


typedef Pair<GeometryBlock*, iAllocator*> GBAPair;
Pair<size_t, GBAPair> 
CompileAllGeometry(fbxsdk::FbxNode* node, iAllocator* Memory, GeometryBlock* GL, iAllocator* TempMem, FBXIDTranslationTable* Table, MD_Vector* MD = nullptr, bool SubDiv = false)
{
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
			bool LoadMesh = false;
			size_t ID	= (size_t)Mesh->GetUniqueID();
			auto Geo	= FindGeoByID( GL, ID );

			DynArray<size_t> RelatedMetaData;

#if USING(RESCOMPILERVERBOSE)
			std::cout << "Found Mesh: " << MeshName << "\n";
#endif
			if (MD)
			{
				MoveDynArray(	
					RelatedMetaData, 
					FindRelatedMetaData(MD, MetaData::EMETA_RECIPIENT_TYPE::EMR_MESH, MeshName, TempMem ));
			}
			else
				LoadMesh = true;

			auto MeshInfo = GetMeshMetaData(MD, RelatedMetaData);
			LoadMesh = MeshInfo != nullptr;

			if ( !Geo && LoadMesh)
			{
				auto Name		= MeshInfo ? MeshInfo->MeshID : Mesh->GetName();
				size_t NameLen	= strlen( Name );

				if (!NameLen) {
					Name = node->GetName();
					NameLen	= strlen( Name );
				}

				TriMesh	out;
				memset(&out, 0, sizeof(out));
				char* ID = nullptr;
				if ( NameLen++ ) {
					ID = (char*)Memory->malloc( NameLen );
					strcpy_s( (char*)ID, NameLen, Name );
				}

				auto Res = CompileMeshResource(out, TempMem, Memory, Mesh, false, ID, MD);
				
				if(MeshInfo){
					auto Info = MeshInfo;
					out.TriMeshID	= Info->guid;
					out.ID			= Info->MeshID;
				}

				if (!FBXIDPresentInTable(Mesh->GetUniqueID(), *Table))
					Table->push_back({ Mesh->GetUniqueID(), out.TriMeshID });

#if USING(RESCOMPILERVERBOSE)
				std::cout << "Compiled Resource: " << Name << "\n";
#endif

				PushGeo(GL, out, Memory);
			}
		}	break;
		}
	}

	size_t NodeCount = node->GetChildCount();
	for(int itr = 0; itr < NodeCount; ++itr)
		CompileAllGeometry(node->GetChild(itr), Memory, GL, TempMem, Table, MD);

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
	out->GUID			= Mesh->TriMeshID;
	out->HasAnimation	= Mesh->AnimationData > 0;
	out->HasIndexBuffer = true;
	out->BufferCount	= 0;
	out->SkeletonGuid	= Mesh->SkeletonGUID;
	out->Type			= EResourceType::EResource_TriMesh;

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


Resource* CreateTriMeshResourceBlob(TriMesh* Mesh, iAllocator* MemoryOut)
{
	size_t BlobSize          = CalculateTriMeshSize(Mesh);
	BlobSize				+= sizeof(TriMeshResourceBlob);
	char* Blob               = (char*)MemoryOut->_aligned_malloc(BlobSize);
	TriMeshResourceBlob* Res = (TriMeshResourceBlob*)Blob;


	memset(Blob, 0, BlobSize);
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

Resource* CreateSkeletonResourceBlob(Skeleton* S, iAllocator* MemoryOut)
{
	size_t Size = CalculateSkeletonSize(S);
	Size += sizeof(SkeletonResourceBlob);

	SkeletonResourceBlob* R = (SkeletonResourceBlob*)MemoryOut->_aligned_malloc(Size);
	R->GUID					= S->guid;
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

Resource* CreateSkeletalAnimationResourceBlob(AnimationClip* AC, GUID_t Skeleton, iAllocator* MemoryOut)
{
	size_t Size = CalculateAnimationSize(AC);
	Size += sizeof(AnimationResourceBlob);

	AnimationResourceBlob* R = (AnimationResourceBlob*)MemoryOut->_aligned_malloc(Size);
	R->Skeleton		= Skeleton;
	R->FrameCount	= AC->FrameCount;
	R->FPS			= AC->FPS;
	R->GUID			= AC->guid;
	R->IsLooping	= AC->isLooping;
	R->Type			= EResourceType::EResource_SkeletalAnimation;
	R->ResourceSize = Size;
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


Resource* CreateColliderResourceBlob(char* Buffer, size_t BufferSize, GUID_t ColliderID, const char* AssetID, iAllocator* MemoryOut)
{
	size_t ResourceSize = BufferSize + sizeof(ColliderResourceBlob);
	ColliderResourceBlob* R = (ColliderResourceBlob*)MemoryOut->_aligned_malloc(ResourceSize);
	
	memcpy(R->Buffer, Buffer, BufferSize);
	R->GUID			= ColliderID;
	R->ResourceSize = ResourceSize;
	R->RefCount		= 0;
	R->State		= Resource::EResourceState_UNLOADED;
	R->Type			= EResourceType::EResource_Collider;
	strncpy(R->ID, AssetID, 63);

	return (Resource*)R;
}


/************************************************************************************************/


class ColliderStream : public physx::PxOutputStream
{
public :
	ColliderStream(iAllocator* Allocator, size_t InitialSize) : Memory(Allocator), size(InitialSize)
	{
		Buffer = (char*)Memory->malloc(InitialSize);
	}

	~ColliderStream()
	{
		Memory->free(Buffer);
	}

	physx::PxU32 write(const void* src, physx::PxU32 count)
	{
		if (used + count > size) {
			char* NewBuffer = (char*)Memory->malloc(size * 2);
			FK_ASSERT(NewBuffer, "Ran Out Of Memory!");
			memcpy(NewBuffer, Buffer, used);
			Memory->free(Buffer);
			Buffer = NewBuffer;
			size = 2 * size;
		}

		memcpy(Buffer + used, src, count);
		used += count;

		return count;
	}

	size_t		size;
	size_t		used;
	char*		Buffer;
	iAllocator*	Memory;
};


/************************************************************************************************/


ResourceList GatherSceneResources(fbxsdk::FbxScene* S, physx::PxCooking* Cooker, iAllocator* MemoryOut, FBXIDTranslationTable* Table, bool LoadSkeletalData = false, MD_Vector* MD = nullptr, bool SUBDIV = false)
{
	size_t TempMemorySize = MEGABYTE * 256;
	StackAllocator TempMemory;
	TempMemory.Init((byte*)_aligned_malloc(TempMemorySize, 0x40), TempMemorySize);

	auto Res = CompileAllGeometry(S->GetRootNode(), MemoryOut, nullptr, TempMemory, Table, MD);

#if USING(RESCOMPILERVERBOSE)
	std::cout << "CompileAllGeometry Compiled " << (size_t)Res << " Resources\n";
#endif
	ResourceList ResourcesFound;
	if ((size_t)Res > 0)
	{
		auto G = (GeometryBlock*)(GBAPair)Res;
		while (G)
		{
			for (size_t I = 0; I < G->MeshUsed; ++I)
			{
				ResourcesFound.push_back(CreateTriMeshResourceBlob(G->Meshes + I, MemoryOut));

				if (G->Meshes[I].Skeleton) {
					auto Res = CreateSkeletonResourceBlob(G->Meshes[I].Skeleton, MemoryOut);
					ResourcesFound.push_back(Res);

					auto CurrentClip = G->Meshes[I].Skeleton->Animations;
					auto SkeletonID = Res->GUID;
					
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

				auto RelatedMD = FindRelatedMetaData(MD, MetaData::EMETA_RECIPIENT_TYPE::EMR_NONE, G->Meshes[I].ID, TempMemory);
				for(size_t J = 0; J < RelatedMD.size(); ++J)
				{
					switch (MD->at(RelatedMD[J])->type)
					{
					case MetaData::EMETAINFOTYPE::EMI_COLLIDER:
					{
						if (!Cooker)
							continue;

						Collider_MetaData* ColliderInfo = (Collider_MetaData*)MD->at(RelatedMD[J]);
						ColliderStream Stream = ColliderStream(MemoryOut, 2048);

						using physx::PxTriangleMeshDesc;
						PxTriangleMeshDesc meshDesc;
						meshDesc.points.count     = G->Meshes[I].Buffers[0]->GetBufferSizeUsed();
						meshDesc.points.stride    = G->Meshes[I].Buffers[0]->GetElementSize();
						meshDesc.points.data      = G->Meshes[I].Buffers[0]->GetBuffer();

						uint32_t* Indexes = (uint32_t*)TempMemory._aligned_malloc(G->Meshes[I].Buffers[15]->GetBufferSizeRaw());

						{
							struct Tri {
								uint32_t Indexes[3];
							};

							auto Proxy = G->Meshes[I].Buffers[15]->CreateTypedProxy<Tri>();
							size_t Position = 0;
							auto itr = Proxy.begin();
							auto end = Proxy.end();
							while(itr < end) {
								Indexes[Position + 0] = (*itr).Indexes[1];
								Indexes[Position + 2] = (*itr).Indexes[2];
								Indexes[Position + 1] = (*itr).Indexes[0];
								Position += 3;
								itr++;
							}
						}

						auto IndexBuffer		  = G->Meshes[I].VertexBuffer.MD.IndexBuffer_Index;
						meshDesc.triangles.count  = G->Meshes[I].IndexCount / 3;
						meshDesc.triangles.stride = G->Meshes[I].Buffers[15]->GetElementSize() * 3;
						meshDesc.triangles.data   = Indexes;

#if USING(RESCOMPILERVERBOSE)
						printf("BEGINNING MODEL BAKING!\n");
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
							auto Blob = CreateColliderResourceBlob(Stream.Buffer, Stream.used, ColliderInfo->Guid, ColliderInfo->ColliderID, MemoryOut);
							ResourcesFound.push_back(Blob);
						}
					}	break;
					default:
						break;
					} 
				}
			}
			if (G->MeshUsed == 10)
				G = G->Next;
			else
				break;
		}
	}

#if USING(RESCOMPILERVERBOSE)
	std::cout << "Created "<< ResourcesFound.size() << " Resource Blobs\n";
#endif

	return ResourcesFound;
}


/************************************************************************************************/


struct LoadGeometry_RES
{
	fbxsdk::FbxManager*		Manager;
	fbxsdk::FbxIOSettings*	Settings;
	fbxsdk::FbxScene*		LoadedFbxScene;

	ResourceList Resources;
};


/************************************************************************************************/


int	AddSceneNode(CompiledScene::SceneNode* Node, CompiledScene* Scene)
{
	auto index = Scene->Nodes.size();
	return index;
}


/************************************************************************************************/


void AddPointLight(CompiledScene::PointLight Light, CompiledScene* Scene)
{
	Scene->SceneLights.push_back(Light);
}


/************************************************************************************************/


void AddEntity(CompiledScene::Entity Entity, CompiledScene* Scene)
{
	Scene->SceneEntities.push_back(Entity);
}


/************************************************************************************************/


void AddStaticEntity(CompiledScene::Entity Static, CompiledScene* Scene)
{
	Scene->SceneStatics.push_back(Static);
}


/************************************************************************************************/


void InitiateCompiledScene(CompiledScene* Scene, iAllocator* Memory)
{
	Scene->Nodes.Allocator			= Memory;
	Scene->SceneEntities.Allocator	= Memory;
	Scene->SceneGeometry.Allocator	= Memory;
	Scene->SceneLights.Allocator	= Memory;
}


/************************************************************************************************/


size_t CalculateSceneResourceSize(CompiledScene* SceneIn)
{
	size_t BlobSize = sizeof(SceneResourceBlob);

	BlobSize += SceneIn->Nodes.size()			* sizeof(CompiledScene::SceneNode);
	BlobSize += SceneIn->SceneEntities.size()	* sizeof(CompiledScene::Entity);
	BlobSize += SceneIn->SceneGeometry.size()	* sizeof(CompiledScene::SceneGeometry);
	BlobSize += SceneIn->SceneLights.size()		* sizeof(CompiledScene::PointLight);
	BlobSize += SceneIn->SceneStatics.size()	* sizeof(CompiledScene::Entity);

	return BlobSize;
}


/************************************************************************************************/


Resource* CreateSceneResourceBlob(iAllocator* Memory, CompiledScene* SceneIn, FBXIDTranslationTable* Table)
{
	size_t ResourceSize = CalculateSceneResourceSize(SceneIn);
	SceneResourceBlob* SceneBlob		= (SceneResourceBlob*)Memory->malloc(ResourceSize);
	SceneBlob->SceneTable.EntityCount	= SceneIn->SceneEntities.size();
	SceneBlob->SceneTable.NodeCount		= SceneIn->Nodes.size();
	SceneBlob->SceneTable.LightCount	= SceneIn->SceneLights.size();
	SceneBlob->ResourceSize				= ResourceSize;
	SceneBlob->GUID						= SceneIn->Guid;
	SceneBlob->RefCount					= 0;
	SceneBlob->State					= Resource::EResourceState_UNLOADED;
	SceneBlob->Type						= EResource_Scene;
	
	for (auto& A : SceneIn->SceneEntities)
		A.MeshGuid = TranslateID(A.MeshGuid, *Table);

	memset(SceneBlob->ID, 0, 64);
	strncpy(SceneBlob->ID, SceneIn->ID, SceneIn->IDSize);
	
	size_t Offset = 0;
	auto Data = SceneBlob->Buffer;

	SceneBlob->SceneTable.EntityOffset = Offset;
	memcpy(Data + Offset, SceneIn->SceneEntities.begin(), SceneIn->SceneEntities.size() * sizeof(CompiledScene::Entity));
	Offset += SceneIn->SceneEntities.size() * sizeof(CompiledScene::Entity);

	SceneBlob->SceneTable.LightOffset = Offset;
	memcpy(Data + Offset, SceneIn->SceneLights.begin(), SceneIn->SceneLights.size() * sizeof(CompiledScene::PointLight));
	Offset += SceneIn->SceneLights.size() * sizeof(CompiledScene::PointLight);

	SceneBlob->SceneTable.NodeOffset = Offset;
	memcpy(Data + Offset, SceneIn->Nodes.begin(), SceneIn->Nodes.size() * sizeof(CompiledScene::SceneNode));
	Offset += SceneIn->Nodes.size() * sizeof(CompiledScene::SceneNode);

	return (Resource*)SceneBlob;
}


/************************************************************************************************/


void ProcessNodes(fbxsdk::FbxNode* Node, iAllocator* Memory, CompiledScene* SceneOut, size_t Parent = -1)
{
	bool SkipChildren = false;
	size_t AttributeCount = Node->GetNodeAttributeCount();
	CompiledScene::SceneNode NewNode;
	
	auto Position = Node->LclTranslation.	Get();
	auto LclScale = Node->LclScaling.		Get();
	auto rotation = Node->LclRotation.		Get();
	auto NodeName = Node->GetName();

	NewNode.Parent	= Parent;
	NewNode.TS		= float4(TranslateToFloat3(Position), LclScale.mData[0]);
	NewNode.Q		= Quaternion{rotation.mData[0], rotation.mData[1], rotation.mData[2]};

	size_t Nodehndl = AddSceneNode(&NewNode, SceneOut);
	SceneOut->Nodes.push_back(NewNode);

	for (size_t i= 0; i < AttributeCount; ++i)
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

			CompiledScene::Entity Entity;
			Entity.MeshGuid	= UniqueID;
			Entity.Node		= Nodehndl;

			AddEntity(Entity, SceneOut);
		}	break;
		case FbxNodeAttribute::eLight:
		{
#if USING(RESCOMPILERVERBOSE)
			std::cout << "Light Found: " << Node->GetName() << "\n";
#endif
			auto FBXLight    = static_cast<fbxsdk::FbxLight*>(Attr);
			auto Type        = FBXLight->LightType.Get();
			auto Cast        = FBXLight->CastLight.Get();
			auto I           = FBXLight->Intensity.Get();
			auto K           = FBXLight->Color.Get();
			auto R           = FBXLight->OuterAngle.Get();

			CompiledScene::PointLight Light;
			Light.K        = TranslateToFloat3(K);			// COLOR for the Confused
			Light.I        = I;
			Light.Node	   = Nodehndl;
			Light.R        = I * 100;

			AddPointLight(Light, SceneOut);
		}	break;
		case FbxNodeAttribute::eMarker:
		case FbxNodeAttribute::eUnknown:
		default:
			break;
		}
	}

	if (!SkipChildren)
	{
		size_t ChildCount = Node->GetChildCount();
		for (size_t I = 0; I < ChildCount; ++I)
			ProcessNodes(Node->GetChild(I), Memory, SceneOut, Nodehndl);
	}
}


/************************************************************************************************/


void ScanChildrenNodesForScene(fbxsdk::FbxNode* Node, MD_Vector* MetaData, iAllocator* Temp, iAllocator* Memory, SceneList* Out)
{
	auto nodeName = Node->GetName();
	auto RelatedMetaData	= FindRelatedMetaData(MetaData, MetaData::EMETA_RECIPIENT_TYPE::EMR_NODE, Node->GetName(), Temp);
	auto NodeCount			= Node->GetChildCount();

	if (RelatedMetaData.size())
	{
		for (auto& i : RelatedMetaData)
		{
			if (MetaData->at(i)->type == MetaData::EMETAINFOTYPE::EMI_SCENE)
			{
				Scene_MetaData* MD = (Scene_MetaData*)MetaData->at(i);
				CompiledScene& Scene	= Memory->allocate<CompiledScene>();
				InitiateCompiledScene(&Scene, Memory);
				Scene.Guid = MD->Guid;
				strncpy(Scene.ID, MD->SceneID, MD->SceneIDSize);
				Scene.IDSize = MD->SceneIDSize;

				ProcessNodes(Node, Temp, &Scene);
				Out->push_back(&Scene);
			}
		}
		// Get Scene
	}
	else
	{
		Temp->clear();
		for (int itr = 0; itr < NodeCount; ++itr) {
			auto Child = Node->GetChild(itr);
			ScanChildrenNodesForScene(Child, MetaData, Temp, Memory, Out);
		}
	}
}


/************************************************************************************************/


void GetScenes(fbxsdk::FbxScene* S, iAllocator* MemoryOut, iAllocator* TempMemory, MD_Vector* MetaData, SceneList* Out)
{
	auto Root = S->GetRootNode();
	ScanChildrenNodesForScene(Root, MetaData, TempMemory, MemoryOut, Out);
}


/************************************************************************************************/


LoadGeometryRES_ptr CompileSceneFromFBXFile(char* AssetLocation, CompileSceneFromFBXFile_DESC* Desc, MD_Vector* METAINFO)
{
	size_t MaxMeshCount							= 100;
	fbxsdk::FbxManager*				Manager     = fbxsdk::FbxManager::Create();
	fbxsdk::FbxIOSettings*			Settings	= fbxsdk::FbxIOSettings::Create(Manager, IOSROOT);
	fbxsdk::FbxScene*				Scene       = nullptr;
	physx::PxCooking*				Cooker		= nullptr;
	physx::PxFoundation*			Foundation	= nullptr;
	physx::PxDefaultErrorCallback	DefaultErrorCallback;
	physx::PxDefaultAllocator		DefaultAllocatorCallback;

	Manager->SetIOSettings(Settings);

	if (Desc->CookingEnabled)
	{
#if USING(RESCOMPILERVERBOSE)
		std::cout << "Physx Model Baking Enabled!\n";
#endif
		Foundation	= PxCreateFoundation(PX_PHYSICS_VERSION, DefaultAllocatorCallback, DefaultErrorCallback);
		FK_ASSERT(Foundation);

		Cooker		= PxCreateCooking(PX_PHYSICS_VERSION ,*Foundation, physx::PxCookingParams(physx::PxTolerancesScale()));
		FK_ASSERT(Cooker);
	}

	FINALLY{
		Manager->Destroy();
		if (Desc->CookingEnabled)
		{
			if (Foundation)
				Foundation->release();

			if (Cooker)
				Cooker->release();
		}
	}FINALLYOVER;

	auto res = LoadFBXScene( AssetLocation, Manager, Settings );
	if (res)
	{
		SceneList Scenes;
		FBXIDTranslationTable Table(*Desc->BlockMemory);
		GetScenes(res, *Desc->BlockMemory, *Desc->BlockMemory, METAINFO, &Scenes);
		ResourceList LoadRes = GatherSceneResources((FbxScene*)res, Cooker, *Desc->BlockMemory, &Table, true, METAINFO);

		for (auto Scene : Scenes){
			auto res = CreateSceneResourceBlob(*Desc->BlockMemory, Scene, &Table);
			LoadRes.push_back(res);
		}

		size_t ResourceCount = 0;
		size_t FileSize		 = 0;

		for (auto& G : LoadRes){
			ResourceCount++;
			FileSize += G->ResourceSize;
		}

		auto G = &Desc->BlockMemory->allocate_aligned<LoadGeometry_RES>();
		G->Manager			= Manager;
		G->LoadedFbxScene	= Scene;
		G->Resources		= LoadRes;
		G->Settings			= Settings;

		return  LoadGeometryRES_ptr(G);
	}
	return LoadGeometryRES_ptr(nullptr);
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


void CleanUp(ResourceScene* Scene)
{
	for (size_t I = 0; I < Scene->DrawablesUsed; ++I)	CleanUpDrawable(Scene->Drawables + I);
	for (size_t I = 0; I < Scene->GeometryUsed; ++I)	CleanUpTriMesh(Scene->Geometry + I);
}


/************************************************************************************************/


struct MD_Token
{
	char*	SubStr;
	size_t	size;
};


typedef DynArray<MD_Token> TokenList;


/************************************************************************************************/


TokenList* GetMetaDataTokens(char* Buffer, size_t BufferSize, iAllocator* Memory)
{
	DynArray<MD_Token>* Tokens = &Memory->allocate_aligned<DynArray<MD_Token>>(Memory);

	size_t StartPos = 0;
	size_t CurrentPos = 0;

	auto RemoveWhiteSpaces = [&]()
	{
		bool WhitespaceSkipped = false;
		while (CurrentPos < BufferSize && !WhitespaceSkipped)
		{
			char CurrentChar = Buffer[CurrentPos];
			switch (CurrentChar)
			{
			case 0x20:
			case 0x00:
			case '\t':
			case '\n':
				++CurrentPos;
				break;
			default:
				WhitespaceSkipped = true;
				break;
			}
		}
	};

	while (CurrentPos < BufferSize)
	{
		auto C = Buffer[CurrentPos];
		if (Buffer[CurrentPos] == ' ' || Buffer[CurrentPos] == '\n' || Buffer[CurrentPos] == '\t')
		{
			RemoveWhiteSpaces();

			char CurrentChar = Buffer[CurrentPos];

			MD_Token NewToken = { Buffer + StartPos, CurrentPos - StartPos - 1 }; // -1 to remove Whitespace at end

			if (NewToken.size)
				Tokens->push_back(NewToken);

			StartPos = CurrentPos;

			RemoveWhiteSpaces();
		}
		else if (Buffer[CurrentPos] == '\0')
			break;
		else
			++CurrentPos;
	}

	return Tokens;
}


/************************************************************************************************/


struct Value
{
	enum TYPE
	{
		INT,
		STRING,
		FLOAT,
	}Type;

	union
	{
		float	F;
		int		I;
		struct
		{
			char*	S;
			size_t	size;
		}S;
	}Data;

	char*	ID;
	size_t	ID_Size;
};

typedef static_vector<Value> ValueList;


/************************************************************************************************/


void MoveTokenStr(MD_Token T, char* out)
{
	memset(out, 0x00, T.size + 1);
	strncpy(out, T.SubStr, T.size);

	for (size_t I = T.size; I > 0; --I)
	{
		switch (out[I])
		{
		case '\n':
		case ' ':
			out[I] = '\0';
			T.size--;
		}
	}
}


/************************************************************************************************/


FlexKit::Pair<ValueList, size_t> ProcessDeclaration(iAllocator* Memory, iAllocator* TempMemory, TokenList* Tokens, size_t StartingPosition)
{
	ValueList Values;
	size_t itr2 = StartingPosition;

	for (; itr2 < Tokens->size(); ++itr2)
	{
		auto T = Tokens->at(itr2);

		if (T.size)
			if (!strncmp(T.SubStr, "int", 3))
			{
				Value NewValue;
				NewValue.Type = Value::INT;

				auto IDToken    = Tokens->at(itr2 - 2);
				auto ValueToken = Tokens->at(itr2 + 2);

				char ValueBuffer[16];
				MoveTokenStr(ValueToken, ValueBuffer);
				int V				= atoi(ValueBuffer);
				NewValue.Data.I		= V;
				NewValue.ID			= (char*)TempMemory->malloc(IDToken.size + 1); // 1 Extra for the Null Terminator
				NewValue.ID_Size	= ValueToken.size;
				MoveTokenStr(IDToken, NewValue.ID);

				Values.push_back(NewValue);
			}
			else if (!strncmp(T.SubStr, "string", 6))
			{
				Value NewValue;
				NewValue.Type = Value::STRING;

				auto IDToken		= Tokens->at(itr2 - 2);
				auto ValueToken		= Tokens->at(itr2 + 2);

				size_t IDSize		= IDToken.size;
				NewValue.ID			= (char*)TempMemory->malloc(IDSize + 1); // 
				NewValue.ID_Size	= IDSize;
				MoveTokenStr(IDToken, NewValue.ID);

				size_t StrSize       = ValueToken.size;
				NewValue.Data.S.S    = (char*)TempMemory->malloc(StrSize + 1); // 
				NewValue.Data.S.size = StrSize;
				MoveTokenStr(ValueToken, NewValue.Data.S.S);

				Values.push_back(NewValue);
			}
			else if (!strncmp(T.SubStr, "float", 5))
			{
				Value NewValue;
				NewValue.Type = Value::FLOAT;

				auto IDToken = Tokens->at(itr2 - 2);
				auto ValueToken = Tokens->at(itr2 + 2);

				char ValueBuffer[16];
				MoveTokenStr(ValueToken, ValueBuffer);
				float V         = atof(ValueBuffer);
				NewValue.Data.F = V;

				NewValue.ID = (char*)TempMemory->malloc(IDToken.size + 1); // 
				MoveTokenStr(IDToken, NewValue.ID);

				Values.push_back(NewValue);
			}
			else if (!strncmp(T.SubStr, "};", min(T.size, 2)))
				return{ Values, itr2 };
	}

	// Should Be Un-reachable
	return{ Values, itr2 };
}


/************************************************************************************************/


size_t SkipBrackets(TokenList* Tokens, size_t StartingPosition)
{
	size_t itr2 = StartingPosition;

	for (; itr2 < Tokens->size(); ++itr2)
	{
		auto T = Tokens->at(itr2);

		if (T.size && (!strncmp(T.SubStr, "};", min(T.size, 2))))
			return itr2;
	}

	// Malformed Document if code reaches here
	FK_ASSERT(0);
	return -1;
}


/************************************************************************************************/


Value* FindValue(static_vector<Value>& Values, char* ValueID)
{
	auto res = FlexKit::find(Values, [&](Value& V) {
		return (!strncmp(V.ID, ValueID, strlen(ValueID))); });

	return (res == Values.end()) ? nullptr : res;
}


/************************************************************************************************/


bool ProcessTokens(iAllocator* Memory, iAllocator* TempMemory, TokenList* Tokens, MD_Vector& MD_Out)
{
	struct Value
	{
		enum TYPE
		{
			INT,
			STRING,
			FLOAT,
		}Type;

		Token T;
	};

	// Metadata Format
	// {Identifier} : {type} = {Value(s)};

	ValueList Values;
	for (size_t itr = 0; itr < Tokens->size(); ++itr)
	{
		auto T = Tokens->at(itr);

		// TODO: Reform this into a table
		if		(T.size && !strncmp(T.SubStr, "AnimationClip",	max(strlen("AnimationClip"), T.size)))
		{
			auto res    = ProcessDeclaration(Memory, TempMemory, Tokens, itr);
			auto Values = res.V1;
			auto ID     = FindValue(Values, "ID");			
			auto begin  = FindValue(Values, "Begin");		
			auto end    = FindValue(Values, "End");			
			auto GUID   = FindValue(Values, "AssetGUID");	

			// Check for ill formed data
#if _DEBUG
			FK_ASSERT((ID != nullptr),		"MISSING ID		TAG!");
			FK_ASSERT((begin != nullptr),	"MISSING Begin	Value!");
			FK_ASSERT((end != nullptr),		"MISSING End	Value!");
			FK_ASSERT((GUID != nullptr),	"MISSING GUID!");
			FK_ASSERT((ID->Type    == Value::STRING));
			FK_ASSERT((begin->Type == Value::FLOAT));
			FK_ASSERT((end->Type   == Value::FLOAT));
			FK_ASSERT((GUID->Type  == Value::INT));
#else	
			if ((!ID	|| ID->Type		!= Value::STRING) ||
				(!begin || begin->Type	!= Value::FLOAT)  ||
				(!end	|| end->Type	!= Value::FLOAT)  ||
				(!GUID	|| GUID->Type	!= Value::INT))
				return false;
#endif

			AnimationClip_MetaData* NewAnimationClip = &Memory->allocate_aligned<AnimationClip_MetaData>();

			auto Target = Tokens->at(itr - 2);

			strncpy(NewAnimationClip->ClipID, ID->Data.S.S, ID->Data.S.size);

			NewAnimationClip->SetID(Target.SubStr, Target.size);
			NewAnimationClip->T_Start	= begin->Data.F;
			NewAnimationClip->T_End		= end->Data.F;
			NewAnimationClip->guid		= GUID->Data.I;

			MD_Out.push_back(NewAnimationClip);

			itr = res;
		}
		else if (T.size && !strncmp(T.SubStr, "Skeleton",		max(strlen("Skeleton"), T.size)))
		{
			auto res		= ProcessDeclaration(Memory, TempMemory, Tokens, itr);
			auto Values		= res.V1;
			auto AssetID	= FindValue(Values, "AssetID");		
			auto AssetGUID	= FindValue(Values, "AssetGUID");	

#if _DEBUG
			FK_ASSERT((AssetID			!= nullptr), "MISSING ID!");
			FK_ASSERT((AssetID->Type	== Value::STRING));

			FK_ASSERT((AssetGUID		!= nullptr), "MISSING GUID!");
			FK_ASSERT((AssetGUID->Type	== Value::INT));
#else
			if ((!AssetID || AssetID->Type != Value::STRING) || (!AssetGUID || AssetGUID->Type != Value::INT))
				return false;
#endif

			Skeleton_MetaData* Skeleton = &Memory->allocate_aligned<Skeleton_MetaData>();

			auto Target = Tokens->at(itr - 2);

			strncpy(Skeleton->SkeletonID, AssetID->Data.S.S, AssetID->Data.S.size);


			Skeleton->SetID(Target.SubStr, Target.size);
			Skeleton->SkeletonGUID	= AssetGUID->Data.I;

			MD_Out.push_back(Skeleton);

			itr = res;
		}
		else if (T.size && !strncmp(T.SubStr, "Model",			max(strlen("Model"), T.size)))
		{
			auto res			= ProcessDeclaration(Memory, TempMemory, Tokens, itr);
			auto Values			= res.V1;
			auto AssetID		= FindValue(Values, "AssetID");
			auto AssetGUID		= FindValue(Values, "AssetGUID");
			auto ColliderGUID	= FindValue(Values, "ColliderGUID");
			auto Target			= Tokens->at(itr - 2);

#if _DEBUG
			FK_ASSERT((AssetID != nullptr), "MISSING ID!");
			FK_ASSERT((AssetID->Type == Value::STRING));

			FK_ASSERT((AssetGUID != nullptr), "MISSING GUID!");
			FK_ASSERT((AssetGUID->Type == Value::INT));
#else
			if ((!AssetID || AssetID->Type != Value::STRING) || (!AssetGUID || AssetGUID->Type != Value::INT))
				return false;
#endif

			Mesh_MetaData* Model = &Memory->allocate_aligned<Mesh_MetaData>();

			if (ColliderGUID != nullptr && ColliderGUID->Type == Value::INT)
				Model->ColliderGUID = ColliderGUID->Data.I;
			else
				Model->ColliderGUID = INVALIDHANDLE;

			strncpy(Model->MeshID, AssetID->Data.S.S, AssetID->Data.S.size);

			for (size_t I = AssetID->Data.S.size; I > 0; --I)
			{
				switch (Model->MeshID[I])
				{
					case '\n':
					case ' ':
						Model->MeshID[I] = '\0';
						AssetID->Data.S.size--;
				}
			}

			Model->SetID(Target.SubStr, Target.size + 1);
			Model->guid = AssetGUID->Data.I;

			MD_Out.push_back(Model);

			itr = res;
		}
		else if (T.size && !strncmp(T.SubStr, "TextureSet",		max(strlen("TextureSet"), T.size)))
		{
			auto res		  = ProcessDeclaration(Memory, TempMemory, Tokens, itr);
			auto Values		  = res.V1;
			auto AssetID	  = Tokens->at(itr - 2);
			auto AssetGUID	  = FindValue(Values, "AssetGUID");
			auto Albedo		  = FindValue(Values, "Albedo");
			auto AlbedoID	  = FindValue(Values, "AlbedoGUID");
			auto RoughMetal	  = FindValue(Values, "RoughMetal");
			auto RoughMetalID = FindValue(Values, "RoughMetalGUID");

			TextureSet_MetaData* TextureSet_Meta = &Memory->allocate_aligned<TextureSet_MetaData>();

			if (AssetGUID && AssetGUID->Type == Value::INT) 
				TextureSet_Meta->Guid = AssetGUID->Data.I;

			if (Albedo && Albedo->Type == Value::STRING){
				auto dest = TextureSet_Meta->Textures.TextureLocation[ETT_ALBEDO].Directory;
				strncpy(dest, Albedo->Data.S.S, Albedo->Data.S.size);
			}

			if (AlbedoID && AlbedoID->Type == Value::INT) {
				TextureSet_Meta->Textures.TextureID[ETT_ALBEDO] = AlbedoID->Data.I;
			} else {
				TextureSet_Meta->Textures.TextureID[ETT_ALBEDO] = INVALIDHANDLE;
			}

			if (RoughMetal && RoughMetal->Type == Value::STRING){
				auto dest = TextureSet_Meta->Textures.TextureLocation[ETT_ROUGHSMOOTH].Directory;
				strncpy(dest, RoughMetal->Data.S.S, RoughMetal->Data.S.size);
			}

			if (RoughMetalID && RoughMetalID->Type == Value::INT) {
				TextureSet_Meta->Textures.TextureID[ETT_ROUGHSMOOTH] = RoughMetalID->Data.I;
			} else {
				TextureSet_Meta->Textures.TextureID[ETT_ROUGHSMOOTH] = INVALIDHANDLE;
			}

			MD_Out.push_back(TextureSet_Meta);
			itr = res;
		}
		else if (T.size && !strncmp(T.SubStr, "Scene",			max(strlen("Scene"), T.size)))
		{
			auto res        = ProcessDeclaration(Memory, TempMemory, Tokens, itr);
			auto Values     = res.V1;
			auto Target		= Tokens->at(itr - 2);
			auto AssetGUID  = FindValue(Values, "AssetGUID");
			auto AssetID	= FindValue(Values, "AssetID");

			Scene_MetaData& Scene		= Memory->allocate<Scene_MetaData>();
			Scene.SetID(Target.SubStr, Target.size);

			if(AssetGUID != nullptr && AssetGUID->Type == Value::INT)
				Scene.Guid = AssetGUID->Data.I;

			if (AssetID != nullptr && AssetID->Type == Value::STRING) {
				strncpy(Scene.SceneID, AssetID->Data.S.S, min(AssetID->Data.S.size, 64));
				Scene.SceneIDSize = AssetID->Data.S.size;
			}

			MD_Out.push_back(&Scene);
			itr = res;
		}
		else if (T.size && !strncmp(T.SubStr, "Collider",		max(strlen("Collider"), T.size)))
		{
			auto res		= ProcessDeclaration(Memory, TempMemory, Tokens, itr);
			auto Values		= res.V1;
			auto Target		= Tokens->at(itr - 2);
			auto AssetGUID	= FindValue(Values,	"AssetGUID");
			auto AssetID	= FindValue(Values,	"AssetID");

			Collider_MetaData& Collider = Memory->allocate<Collider_MetaData>();
			Collider.SetID(Target.SubStr, Target.size);

			if (AssetGUID != nullptr && AssetGUID->Type == Value::INT)
				Collider.Guid = AssetGUID->Data.I;

			if (AssetID != nullptr && AssetID->Type == Value::STRING) {
				strncpy(Collider.ColliderID, AssetID->Data.S.S, min(AssetID->Data.S.size, 64));
				Collider.ColliderIDSize = AssetID->Data.S.size;
			}

			MD_Out.push_back(&Collider);
			itr = res;
		}
		else if (T.size && !strncmp(T.SubStr, "{",				max(strlen("{"), T.size)))
			itr = SkipBrackets(Tokens, itr);
	}

	return true;
}


/************************************************************************************************/


bool ReadMetaData(const char* Location, iAllocator* Memory, iAllocator* TempMemory, MD_Vector& MD_Out)
{
	size_t BufferSize = FlexKit::GetFileSize(Location);
	char* Buffer = (char*)TempMemory->malloc(BufferSize);
	LoadFileIntoBuffer(Location, (byte*)Buffer, BufferSize);

	auto Tokens = GetMetaDataTokens(Buffer, BufferSize, TempMemory);
	auto res = ProcessTokens(Memory, TempMemory, Tokens, MD_Out);

	return res;
}


/************************************************************************************************/


void PrintMetaDataList(MD_Vector& MD)
{
	std::cout << "FOUND FOLLOWING METADATA:\n";
	for (auto& MetaData : MD)
	{
		std::cout << MetaData->ID << " : Type: ";
		switch (MetaData->type)
		{
		case MetaData::EMETAINFOTYPE::EMI_COLLIDER:				
			std::cout << MetaData->ID << "Collider";			break;
		case MetaData::EMETAINFOTYPE::EMI_MESH:					
			std::cout << MetaData->ID << "Mesh";				break;
		case MetaData::EMETAINFOTYPE::EMI_SCENE:				
			std::cout << MetaData->ID << "Scene";				break;
		case MetaData::EMETAINFOTYPE::EMI_SKELETAL:				
			std::cout << MetaData->ID << "Skeletal";			break;
		case MetaData::EMETAINFOTYPE::EMI_SKELETALANIMATION:	
			std::cout << MetaData->ID << "Skeletal Animation ";	break;
		case MetaData::EMETAINFOTYPE::EMI_ANIMATIONCLIP:		
			std::cout << MetaData->ID << "Animation Clip";		break;
		case MetaData::EMETAINFOTYPE::EMI_ANIMATIONEVENT:		
			std::cout << MetaData->ID << "Animation Event";		break;
		case MetaData::EMETAINFOTYPE::EMI_TEXTURESET:			
			std::cout << MetaData->ID << "TexureSet";			break;
		default:
			break;
		}
	}
}


/************************************************************************************************/


Resource* MetaDataToBlob(MetaData* Meta, iAllocator* Mem)
{
	Resource* Result = nullptr;
	switch (Meta->type)
	{
	case MetaData::EMETAINFOTYPE::EMI_TEXTURESET:
	{
		TextureSet_MetaData* TextureSet = (TextureSet_MetaData*)Meta;
		auto& NewTextureSet = Mem->allocate<TextureSetBlob>();

		NewTextureSet.GUID			= TextureSet->Guid;
		NewTextureSet.ResourceSize	= sizeof(TextureSetBlob);

		for (size_t I = 0; I < 2; ++I) {
			memcpy(NewTextureSet.Textures[I].Directory, TextureSet->Textures.TextureLocation[I].Directory, 64);
			NewTextureSet.Textures[I].guid = TextureSet->Textures.TextureID[I];
		}

		Result = (Resource*)&NewTextureSet;
		break;
	}
	default:
		break;
	}
	return Result;
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


namespace TextUtilities
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


	LoadFontResult LoadFontAsset(char* dir, char* file, RenderSystem* RS, iAllocator* tempMem, iAllocator* outMem)
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
						Fonts[I].Memory  = outMem;
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

						auto res = LoadTextureFromFile(Fonts[I].FontDir, RS, outMem);
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

		return{ FontCount, Fonts };
	}


	/************************************************************************************************/


	void FreeFontAsset(FontAsset* asset)
	{
		FreeTexture(&asset->Text);
		asset->Memory->free(asset->FontDir);
	}


	/************************************************************************************************/
}// namespace TextUtilities