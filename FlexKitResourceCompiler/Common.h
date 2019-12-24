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

#ifndef COMMON_H
#define COMMON_H

#include "fbxsdk/include/fbxsdk.h"

#include "../coreutilities/MathUtils.h"
#include "../coreutilities/MemoryUtilities.h"
#include "../coreutilities/Assets.h"
#include "../coreutilities/XMMathConversion.h"
#include "../graphicsutilities/MeshUtils.h"
#include "../graphicsutilities/AnimationUtilities.h"


#include <DirectXMath.h>


#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "Shell32.lib")

// FBX Loading
#include				"fbxsdk/include/fbxsdk.h"
#pragma comment	(lib,	"libfbxsdk.lib")

// PhysX Cooking Deps
#ifdef _DEBUG
#pragma comment(lib,	"PhysXFoundation_64.lib"	)
#pragma comment(lib,	"PhysXCommon_64.lib"		)
#pragma comment(lib,	"PhysXCooking_64.lib"		)

#else
#pragma comment(lib,	"PhysXFoundation_64.lib"	)
#pragma comment(lib,	"PhysXCommon_64.lib"		)
#pragma comment(lib,	"PhysXCooking_64.lib"		)
#endif


using FlexKit::float2;
using FlexKit::float3;
using FlexKit::float4;
using FlexKit::float4x4;
using FlexKit::uint4_16;
using FlexKit::uint4_32;
using FlexKit::iAllocator;

using FlexKit::Skeleton;
using DirectX::XMMATRIX;

using FlexKit::MeshUtilityFunctions::IndexList;
using FlexKit::MeshUtilityFunctions::CombinedVertexBuffer;


/************************************************************************************************/


inline uint32_t		FetchIndex			(uint32_t itr, fbxsdk::FbxMesh* Mesh)			{ return Mesh->GetPolygonVertex(itr / 3, itr % 3);	}
inline uint32_t		FetchIndex2			(uint32_t itr, const IndexList* IL)				{ return IL->at(itr);								}
inline float3		FetchVertexPOS		(uint32_t itr, const CombinedVertexBuffer* Buff){ return Buff->at(itr).POS;							}
inline float3		FetchWeights		(uint32_t itr, const CombinedVertexBuffer* Buff){ return Buff->at(itr).WEIGHTS;						}
inline float3		FetchVertexNormal	(uint32_t itr, const CombinedVertexBuffer* Buff){ return Buff->at(itr).NORMAL;						}
inline float3		FetchVertexTangent  (uint32_t itr, const CombinedVertexBuffer* Buff){ return Buff->at(itr).TANGENT;						}
inline float3		FetchFloat3ZERO		(uint32_t itr, const CombinedVertexBuffer* Buff){ return{ 0.0f, 0.0f, 0.0f };						}
inline float2		FetchVertexUV		(uint32_t itr, const CombinedVertexBuffer* Buff){ auto temp = Buff->at(itr).TEXCOORD.xy(); 
																						    return {temp.x, temp.y};						}
inline uint4_16		FetchWeightIndices	(size_t itr, const CombinedVertexBuffer* Buff)	{ return Buff->at(itr).WIndices;					}
inline uint32_t		WriteIndex			(uint32_t in)								    { return in;										}
inline float3		WriteVertex			(float3 in)									    { return float3(in);								}
inline float2		WriteUV				(float2 in)									    { return in;										}
inline uint4_16		Writeuint4			(uint4_16 in);


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

class iResource;

struct ResourceBlob
{
	ResourceBlob() = default;

	~ResourceBlob()
	{
		if(buffer)
			free(buffer);
	}

	// No Copy
	ResourceBlob			 (ResourceBlob& rhs)		= delete;
	ResourceBlob& operator = (const ResourceBlob& rhs)	= delete;

	// Allow Moves
	ResourceBlob(ResourceBlob&& rhs)
	{
		buffer			= rhs.buffer;
		bufferSize		= rhs.bufferSize;

		GUID			= rhs.GUID;
		resourceType	= rhs.resourceType;
		ID				= std::move(rhs.ID);

		rhs.buffer		= nullptr;
		rhs.bufferSize	= 0;
	}

	ResourceBlob& operator =(ResourceBlob&& rhs)
	{
		buffer			= rhs.buffer;
		bufferSize		= rhs.bufferSize;

		GUID			= rhs.GUID;
		resourceType	= rhs.resourceType;
		ID				= std::move(rhs.ID);

		rhs.buffer		= nullptr;
		rhs.bufferSize	= 0;

		return *this;
	}

	size_t						GUID		= INVALIDHANDLE;
	std::string					ID;
	FlexKit::EResourceType		resourceType;

	char*			buffer		= nullptr;
	size_t			bufferSize	= 0;
};


/************************************************************************************************/


class iResource
{
public:
	virtual ResourceBlob CreateBlob() = 0;
};


using ResourceList = std::vector<std::shared_ptr<iResource>>;


/************************************************************************************************/

#endif
