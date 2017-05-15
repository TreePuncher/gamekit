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

#ifndef COMMON_H
#define COMMON_H

#include "fbxsdk/include/fbxsdk.h"

#include "../coreutilities/MathUtils.h"
#include "../coreutilities/MemoryUtilities.h"
#include "../graphicsutilities/graphics.h"
#include "../graphicsutilities/MeshUtils.h"

#include <DirectXMath.h>


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


using FlexKit::float2;
using FlexKit::float3;
using FlexKit::float4;
using FlexKit::uint4_16;
using FlexKit::uint4_32;
using FlexKit::iAllocator;

using FlexKit::Skeleton;

using DirectX::XMMATRIX;

using FlexKit::MeshUtilityFunctions::IndexList;
using FlexKit::MeshUtilityFunctions::CombinedVertexBuffer;

inline uint32_t		FetchIndex			(size_t itr, fbxsdk::FbxMesh* Mesh)			{return Mesh->GetPolygonVertex(itr/3, itr % 3);}
inline uint32_t		FetchIndex2			(size_t itr, IndexList* IL)					{return IL->at(itr);			}
inline float3		FetchVertexPOS		(size_t itr, CombinedVertexBuffer* Buff)	{return Buff->at(itr).POS;		}
inline float3		FetchWeights		(size_t itr, CombinedVertexBuffer* Buff)	{return Buff->at(itr).WEIGHTS;	}
inline float3		FetchVertexNormal	(size_t itr, CombinedVertexBuffer* Buff)	{return Buff->at(itr).NORMAL;	}
inline float3		FetchFloat3ZERO		(size_t itr, CombinedVertexBuffer* Buff)	{return{ 0.0f, 0.0f, 0.0f };	}
inline float2		FetchVertexUV		(size_t itr, CombinedVertexBuffer* Buff)	{auto temp = Buff->at(itr).TEXCOORD.xy();return {temp.x, temp.y};}
inline uint4_16		FetchWeightIndices	(size_t itr, CombinedVertexBuffer* Buff)	{return Buff->at(itr).WIndices;}
inline uint32_t		WriteIndex			(uint32_t in)								{return in;}
inline float3		WriteVertex			(float3 in)									{return float3(in);}
inline float2		WriteUV				(float2 in)									{return in;}
inline uint4_16		Writeuint4			(uint4_16 in);

#endif