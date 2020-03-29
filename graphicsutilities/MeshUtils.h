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

#pragma once

#include "..\buildsettings.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\MathUtils.h"
#include "..\coreutilities\memoryutilities.h"

#include <algorithm>
#include <functional>
#include <variant>
#include <limits>

namespace FlexKit
{
	struct JointWeightToken
	{
        float4 weights;
	};


	struct JointIndexToken
	{
		uint4_16 joints;
	};


    struct PointToken
    {
        float3 xyz;
    };


    struct TextureCoordinateToken
    {
        float2   UV;
        uint32_t index;

        bool operator == (const TextureCoordinateToken& rhs) const
        {
            return UV == rhs.UV && index == rhs.index;
        }
    };


    struct NormalToken
    {
        float3 normal;
    };


    struct TangentToken
    {
        float3 tangent;
    };


    struct MaterialToken
    {
        uint32_t materialID;
    };


    struct VertexField
    {
        uint32_t idx;

        enum FieldType
        {
            Point,
            TextureCoordinate,
            Normal,
            Tangent,
            Material,
            JointWeight,
            JointIndex
        } type;
    };


    using VertexIndexList = static_vector<VertexField>;

    struct VertexToken
    {
        VertexIndexList vertex;
    };


    using MeshToken = std::variant<
        PointToken,
        TextureCoordinateToken,
        NormalToken,
        TangentToken,
        MaterialToken,
        JointWeightToken,
        JointIndexToken,
        VertexToken>;


	struct PackedVertexLayout
	{
		float3 Pos;
		float3 Normal;
		float3 Tangent;
		float2 UV;
	};

	struct PackedAnimationLayout
	{
		float3		Weights;
		uint4_16	Joints;
	};

	template <class T>
	inline void hash_combine(std::size_t& seed, const T& v)
	{
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	struct CombinedVertex
	{
        CombinedVertex() : NORMAL{ 0 }, POS{ 0 }, TEXCOORD{ 0 }, WEIGHTS{ 0 } {}
		float3	 POS;
        float3	 NORMAL;
        float3	 TANGENT;
		float3	 TEXCOORD;
		float3	 WEIGHTS;
		uint4_16 WIndices;

		struct IndexBitField
		{
			size_t p_Index = 0;
			size_t n_Index = 0;
			size_t t_Index = 0;

			size_t Hash() const 
			{ 
				size_t hash = 0;
				hash_combine(hash, p_Index);
				hash_combine(hash, t_Index);
				hash_combine(hash, n_Index);

				return hash;
			};

			bool operator == (const IndexBitField in) const { return (in.Hash() == Hash()); }
		} index;

		bool operator == ( const CombinedVertex& rhs )
		{
			return eqlcompare( this, &rhs );
		}

		bool eqlcompare( const CombinedVertex * __restrict lhs, const CombinedVertex * __restrict rhs )
		{
			IndexBitField rhsindex = rhs->index;
			if( rhsindex.p_Index == index.p_Index )
				if( rhsindex.n_Index == index.n_Index )
					if( rhsindex.t_Index == index.t_Index )
						return true;
			return false;
		}
	};

	FLEXKITAPI bool operator < ( const CombinedVertex::IndexBitField lhs, const CombinedVertex::IndexBitField rhs );

	namespace MeshUtilityFunctions
	{
		typedef Vector<MeshToken>		TokenList;
		typedef Vector<uint32_t>		IndexList;
		typedef Vector<CombinedVertex>	CombinedVertexBuffer;

		struct MeshBuildInfo
		{
			size_t IndexCount;
			size_t VertexCount;
		};

        class OptimizedMesh;

        class OptimizedBuffer
        {
        public:
            OptimizedBuffer(const OptimizedMesh& buffer);

            Vector<char> points     { SystemAllocator };
            Vector<char> normals    { SystemAllocator };
            Vector<char> tangents   { SystemAllocator };

            Vector<char> textureCoordinates { SystemAllocator };
            Vector<char> jointWeights       { SystemAllocator };
            Vector<char> jointIndexes       { SystemAllocator };

            Vector<char> indexes    { SystemAllocator };

            size_t IndexCount() const
            {
                return indexes.size() / sizeof(uint32_t);
            }
        };

		FLEXKITAPI OptimizedBuffer	BuildVertexBuffer(const TokenList& in);
		FLEXKITAPI char*		    ScrubLine( char* inLine, size_t RINOUT lineLength );

		namespace OBJ_Tools
		{
			struct LoaderState
			{
				LoaderState(){
					Color_1	= false;
					Normals	= false;
					UV_1	= false;
				}

				bool Color_1;
				bool Normals;
				bool UV_1;
			};

			typedef Pair<float3, uint4_16> WeightIndexPair;
			FLEXKITAPI void			AddVertexToken		(float3 in, TokenList& out);
			FLEXKITAPI void			AddWeightToken		(WeightIndexPair in, TokenList& out );
            FLEXKITAPI void			AddNormalToken      (float3 in, TokenList& out);
            FLEXKITAPI void			AddNormalToken      (const float3 N, const float3 T, TokenList& out);
            FLEXKITAPI void			AddTexCordToken		(float3 in, TokenList& out);
			FLEXKITAPI void			AddIndexToken		(const uint32_t V, const uint32_t N, const uint32_t T, TokenList& out);
			FLEXKITAPI void			AddIndexToken		(const uint32_t V, const uint32_t N, const uint32_t T, const uint32_t texCoord, TokenList& out);
			//FLEXKITAPI void			AddPatchBeginToken	(TokenList& out);
			//FLEXKITAPI void			AddPatchEndToken	(TokenList& out);
			FLEXKITAPI void			CStrToToken			(const char* in_Line, size_t lineLength, TokenList& out, LoaderState& State_in_out );
			FLEXKITAPI std::string	GetNextLine			(std::string::iterator& File_Position, std::string& File_Buffer );
			FLEXKITAPI void			SkipToFloat			(std::string::const_iterator& in, const std::string& in_str );
		}
	}
}
