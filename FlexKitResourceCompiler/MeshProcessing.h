#ifndef MESHPROCESSING_H
#define MESHPROCESSING_H


/**********************************************************************

Copyright (c) 2015 - 2017 Robert May

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

namespace FlexKit
{
	/************************************************************************************************/


	float3 TranslateToFloat3(FbxVector4& in);
	float3 TranslateToFloat3(FbxDouble3& in);
	float4 TranslateToFloat4(FbxVector4& in);


	/************************************************************************************************/


	XMMATRIX	FBXMATRIX_2_XMMATRIX(FbxAMatrix& AM);
	FbxAMatrix	XMMATRIX_2_FBXMATRIX(XMMATRIX& M);


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
		bool UV;
		bool Normals;
		bool Weights;

		float3			MinV;
		float3			MaxV;
		float			R;

		size_t			FaceCount;
		FBXSkinDeformer Skin;
	};

	struct CompiledMeshInfo
	{
		bool   Success;
		size_t BuffersFound;
	};


	/************************************************************************************************/


	size_t GetNormalIndex	(size_t pIndex, size_t vIndex, size_t vID, fbxsdk::FbxMesh* Mesh);
	size_t GetTexcordIndex	(size_t pIndex, size_t vIndex, fbxsdk::FbxMesh* Mesh);
	size_t GetVertexIndex	(size_t pIndex, size_t vIndex, size_t vID, fbxsdk::FbxMesh* Mesh);


	/************************************************************************************************/


	FBXSkinDeformer		CreateSkin			(fbxsdk::FbxMesh* Mesh);
	FBXMeshDesc			TranslateToTokens	(fbxsdk::FbxMesh* Mesh, MeshUtilityFunctions::TokenList& TokensOut, Skeleton* S = nullptr, bool SubDiv_Enabled = false);
	CompiledMeshInfo	CompileMeshResource	(TriMeshResource& out, FbxMesh* Mesh, bool EnableSubDiv, const char* ID, const MetaDataList& MD);


	/************************************************************************************************/
}
#endif