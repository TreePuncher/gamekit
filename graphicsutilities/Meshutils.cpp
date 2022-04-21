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

#include "..\pch.h"
#include "MeshUtils.h"
#include "memoryutilities.h"

namespace FlexKit
{   /************************************************************************************************/


	bool operator < ( const CombinedVertex::IndexBitField lhs, const CombinedVertex::IndexBitField rhs )
	{
		return lhs.Hash() > rhs.Hash();
	}


	namespace MeshUtilityFunctions
	{   /************************************************************************************************/


        UnoptimizedMesh::UnoptimizedMesh(const TokenList& tokens)
        {
            for (auto& token : tokens)
            {
                std::visit(
                    overloaded{
                        [](auto&)
                        {
                            FK_ASSERT(0, "NO OVERLOAD! " + __LINE__);
                        },
                        [&](const JointIndexToken token)
                        {
                            jointIndexes.push_back(token.joints);
                        },
                        [&](const JointWeightToken& token)
                        {
                            jointWeights.push_back(token.weights);
                        },
                        [&](const PointToken& token)
                        {
                            points.push_back(token.xyz);
                        },
                        [&](const TextureCoordinateToken& texCoord)
                        {
                            textureCoordinates.push_back(texCoord);
                        },
                        [&](const NormalToken& token)
                        {
                            normals.push_back(token.normal);
                        },
                        [&](const TangentToken token)
                        {
                            tangents.push_back(token.tangent);
                        },
                        [&](const MaterialToken& material)
                        {
                        },
                        [&](const VertexToken& token)
                        {
                            indexes.push_back(token.vertex);
                        },
                        [&](const MorphTargetVertexToken& token)
                        {
                            morphTargets.push_back(token);
                        }
                    },
                    token);
            };

            for (uint32_t I = 0; I < indexes.size(); I += 3)
                tris.push_back({ I + 0, I + 1, I + 2 });
        }


        /************************************************************************************************/


        float3 UnoptimizedMesh::GetPoint(uint32_t vertexIndex) const noexcept
        {
            auto fields = indexes[vertexIndex];
            auto res = std::find_if(
                fields.begin(), fields.end(),
                [&](const VertexField& field)
                {
                    return field.type == VertexField::Point;
                });

            return points[res->idx];
        }


        /************************************************************************************************/


        float3 UnoptimizedMesh::GetTrianglePosition(const Triangle tri) const
        {
            float3 point = { 0, 0, 0 };

            for (auto v : tri.vertices)
                point += GetPoint(v);

            return point / 3;
        }


        /************************************************************************************************/


        AABB UnoptimizedMesh::GetTriangleAABB(const Triangle tri) const
        {
            AABB aabb;

            for (auto V : tri.vertices) {
                const float3 Pos = GetPoint(V);
                aabb += AABB{ Pos, Pos };
            }

            return { aabb };
        }


        /************************************************************************************************/


        bool UnoptimizedMesh::VertexCompare(const uint32_t lhs, const uint32_t rhs) const
        {
            const auto lhsVertexField = indexes[lhs];
            const auto rhsVertexField = indexes[rhs];

            bool res = true;
            for (size_t I = 0; I < lhsVertexField.size(); ++I)
            {
                switch (lhsVertexField[I].type)
                {
                case VertexField::FieldType::JointIndex:
                {
                    res &= jointIndexes[lhsVertexField[I].idx] == jointIndexes[rhsVertexField[I].idx];
                }   break;
                case VertexField::FieldType::JointWeight:
                {
                    res &= jointWeights[lhsVertexField[I].idx] == jointWeights[rhsVertexField[I].idx];
                }   break;
                case VertexField::FieldType::Material:
                    break;
                case VertexField::FieldType::Normal:
                {
                    res &= (1.0f - normals[lhsVertexField[I].idx].dot(normals[rhsVertexField[I].idx])) < 0.001f;
                }   break;
                case VertexField::FieldType::Tangent:
                {
                    res &= (1.0f - tangents[lhsVertexField[I].idx].dot(tangents[rhsVertexField[I].idx])) < 0.001f;
                }   break;
                case VertexField::FieldType::Point:
                {
                    res &= VectorCompare(points[lhsVertexField[I].idx], points[rhsVertexField[I].idx], 0.000001f);
                }   break;
                case VertexField::FieldType::TextureCoordinate:
                {
                    const auto lhs    = textureCoordinates[lhsVertexField[I].idx];
                    const auto rhs    = textureCoordinates[rhsVertexField[I].idx];

                    res &= (lhs.index == rhs.index && VectorCompare(lhs.UV, rhs.UV, 0.000001f));
                }   break;
                default:    break;
                };
            }

            return res;
        }


        /************************************************************************************************/


        uint64_t UnoptimizedMesh::VertexHash(const uint32_t v ) const
        {
            const VertexIndexList fields = indexes[v];
            uint64_t hash = 0;

            for (auto field : fields) {
                switch (field.type)
                {
                case VertexField::FieldType::JointIndex:
                {
                    std::bitset<sizeof(uint4_16) * 8> bitset;
                    memcpy(&bitset, &jointIndexes[field.idx], sizeof(bitset));
                    hash_combine(hash, bitset);
                }   break;
                case VertexField::FieldType::JointWeight:
                {
                    std::bitset<sizeof(float4) * 8> bitset;
                    memcpy(&bitset, &jointWeights[field.idx], sizeof(bitset));
                    hash_combine(hash, bitset);
                }   break;
                case VertexField::FieldType::Material:
                    break;
                case VertexField::FieldType::Normal:
                {
                    std::bitset<sizeof(float3) * 8> bitset;
                    memcpy(&bitset, &normals[field.idx], sizeof(bitset));
                    hash_combine(hash, bitset);
                }   break;
                case VertexField::FieldType::Tangent:
                {
                    std::bitset<sizeof(float3) * 8> bitset;
                    memcpy(&bitset, &tangents[field.idx], sizeof(bitset));
                    hash_combine(hash, bitset);
                }   break;
                case VertexField::FieldType::Point:
                {
                    std::bitset<sizeof(float3) * 8> bitset;
                    memcpy(&bitset, &points[field.idx], sizeof(bitset));
                    hash_combine(hash, bitset);
                }   break;
                default:    break;
                };
            }

            return hash;
        }


        /************************************************************************************************/


        uint32_t LocalBlockContext::LocallyUnique(const uint32_t vertex, const uint32_t localIdx)
        {
            for (const auto recentVertex : vertexHistory)
                if (mesh.VertexCompare(recentVertex.vertexIndex, vertex))
                    return false;

            vertexHistory.push_back({ localIdx, vertex });

            return true;
        }


        /************************************************************************************************/


        uint32_t LocalBlockContext::Map(const uint32_t vertex) const
        {
            uint32_t remapped = vertex;

            for (const auto recentVertex : vertexHistory)
                if (mesh.VertexCompare(recentVertex.vertexIndex, vertex))
                    remapped = recentVertex.localIndex;

            return remapped;
        }


        /************************************************************************************************/


        BoundingSphere MeshKDBTree::KDBNode::GetBoundingSphere(const UnoptimizedMesh& IN_mesh)
        {
            const float3 midPoint = { 0, 0, 0 };
            float d = 0.0f;

            
            std::for_each(
                IN_mesh.tris.data() + begin, IN_mesh.tris.data() + end,
                [&](const auto tri)
                {
                    const float3 pos = IN_mesh.GetTrianglePosition(tri);
                    const float distance = (pos - midPoint).magnitude();

                    if (distance > d)
                        d = distance;
                });

            return { midPoint, d };
        }


        MeshKDBTree::MeshKDBTree(const UnoptimizedMesh& IN_mesh) :
            mesh{ IN_mesh },
            root{ BuildNode(0, mesh.tris.size()) } {}



        auto MeshKDBTree::begin() const
        {
            return leafNodes.begin();
        }


        auto MeshKDBTree::end() const
        {
            return leafNodes.end();
        }


        auto MeshKDBTree::GetMedianSplitPlaneAABB(const Triangle* begin, const Triangle* end) const
        {
            float3  midPoint{ 0, 0, 0 };
            AABB    aabb;
            std::for_each(
                begin, end,
                [&](const auto tri)
                {
                    midPoint += mesh.GetTrianglePosition(tri);
                    aabb += mesh.GetTriangleAABB(tri);
                });

            return std::make_pair(midPoint / (float)std::distance(begin, end), aabb);
        }


        std::shared_ptr<MeshKDBTree::KDBNode> MeshKDBTree::BuildNode(size_t begin, size_t end)
        {
            const auto [midPoint, aabb] = GetMedianSplitPlaneAABB(mesh.tris.data() + begin, mesh.tris.data() + end);

            if (end - begin < 512 || aabb.AABBArea() == 0.0f)
            {
                auto node = std::make_shared<KDBNode>(KDBNode{ 0, 0, begin, end, aabb });
                leafNodes.push_back(node);

                return node;
            }
            else
            {
                auto plane = aabb.LongestAxis();
                while (true)
                {
                    auto splitPlane = SplitPlanes[plane];

                    const auto mid =
                        std::partition(
                            mesh.tris.begin() + begin,
                            mesh.tris.begin() + end,
                            [&](const auto& tri) -> bool
                            {
                                const float3 pos    = mesh.GetTrianglePosition(tri);
                                const float d       = (midPoint - pos).dot(splitPlane);

                                return d >= 0;
                            });

                    const size_t midIdx = std::distance(mesh.tris.begin(), mid);

                    if (begin == midIdx || midIdx == end)
                    {
                        DebugBreak();
                        //plane = (AABB::Axis)((plane + 1) % AABB::Axis::Axis_count);
                        continue;
                    }

                    return std::make_unique<KDBNode>(
                        KDBNode{
                            BuildNode(begin, midIdx),
                            BuildNode(midIdx, end),
                            begin, end, aabb });
                }
            }
        }


        /************************************************************************************************/


        OptimizedMesh& OptimizedMesh::operator += (OptimizedMesh& rhs)
        {
            auto appendBuffers = [](auto& target, auto& source)
            {
                if (!target.size()) {
                    target = source;
                    return;
                }

                const auto size = target.size();
                target.resize(target.size() + source.size());

                memcpy(target.data() + size, source.data(), sizeof(decltype(source[0])) * source.size());
            };


            if (indexes.size())
            {
                const size_t indexOffset = points.size();

                for (auto index : rhs.indexes)
                    indexes.push_back(index_t(index + indexOffset));
            }
            else
                indexes = rhs.indexes;

            appendBuffers(points, rhs.points);
            appendBuffers(normals, rhs.normals);
            appendBuffers(tangents, rhs.tangents);
            appendBuffers(textureCoordinates, rhs.textureCoordinates);
            appendBuffers(jointIndexes, rhs.jointIndexes);
            appendBuffers(jointWeights, rhs.jointWeights);

            if (morphTargets.size() < rhs.morphTargets.size())
                morphTargets.resize(rhs.morphTargets.size());

            for(size_t I = 0; I < morphTargets.size(); I++)
                appendBuffers(morphTargets[I].vertices, rhs.morphTargets[I].vertices);

            return *this;
        }


        /************************************************************************************************/


        void OptimizedMesh::PushVertex(uint32_t globalIdx, LocalBlockContext& ctx)
            {
                auto& vertexField = ctx.mesh.indexes[globalIdx];

                for (auto field : vertexField)
                {
                    switch (field.type)
                    {
                    case VertexField::Point:
                        points.push_back(ctx.mesh.points[field.idx]);
                        break;
                    case VertexField::TextureCoordinate:
                        textureCoordinates.push_back(ctx.mesh.textureCoordinates[field.idx].UV);
                        break;
                    case VertexField::Normal:
                        normals.push_back(ctx.mesh.normals[field.idx]);
                        break;
                    case VertexField::Tangent:
                        tangents.push_back(ctx.mesh.tangents[field.idx]);
                        break;
                    case VertexField::Material:
                        break;
                    case VertexField::JointWeight:
                        jointWeights.push_back({ ctx.mesh.jointWeights[field.idx][0], ctx.mesh.jointWeights[field.idx][1], ctx.mesh.jointWeights[field.idx][2] });
                        break;
                    case VertexField::JointIndex:
                        jointIndexes.push_back(ctx.mesh.jointIndexes[field.idx]);
                        break;
                    case VertexField::MorphTarget:
                    {
                        auto& vertex = ctx.mesh.morphTargets[field.idx];
                        MorphTargetVertex mtv;
                        mtv.normal      = vertex.normal;
                        mtv.position    = vertex.position;
                        mtv.tangent     = vertex.tangent;

                        if (vertex.morphIdx >= morphTargets.size())
                            morphTargets.emplace_back();

                        morphTargets[vertex.morphIdx].vertices.push_back(mtv);
                    }   break;
                    default:
                        break;
                    }
                }
            }


        /************************************************************************************************/


        void OptimizedMesh::PushTri(const Triangle& tri, LocalBlockContext& ctx)
        {
            for (auto& v : tri.vertices)
            {
                if (ctx.LocallyUnique(v, (uint32_t)points.size()))
                {
                    indexes.push_back((index_t)points.size());
                    PushVertex(v, ctx);
                }
                else
                {
                    const uint32_t idx = ctx.Map(v);
                    indexes.push_back(idx);
                }
            }
        }


        /************************************************************************************************/


        OptimizedBuffer::OptimizedBuffer(const OptimizedMesh& mesh) :
            aabb    { mesh.aabb           },
            bs      { mesh.boundingSphere }
        {
            // Not quick I know, leave me alone
            auto moveFloat3s =
                [&](auto& target, auto& source)
            {
                for (auto temp : source)
                {
                    char byteBuffer[sizeof(float[3])];
                    memcpy(byteBuffer, &temp, sizeof(byteBuffer));
                    for (auto byte : byteBuffer)
                        target.push_back(byte);
                }
            };

            auto moveBuffer =
                [&](auto& target, auto& source)
                {
                    target.reserve(target.size() + sizeof(source[0]) * source.size());

                    for(auto temp : source)
                    {
                        char byteBuffer[sizeof(temp)];
                        memcpy(byteBuffer, &temp, sizeof(temp));
                        for (auto byte : byteBuffer)
                            target.push_back(byte);
                    }
                };

            moveFloat3s(points,     mesh.points);
            moveFloat3s(normals,    mesh.normals);
            moveFloat3s(tangents,   mesh.tangents);

            moveBuffer(textureCoordinates,  mesh.textureCoordinates);
            moveBuffer(jointWeights,        mesh.jointWeights);
            moveBuffer(jointIndexes,        mesh.jointIndexes);
            moveBuffer(indexes,             mesh.indexes);

            for (auto& morphTarget : mesh.morphTargets)
            {
                const size_t bufferSize = morphTarget.vertices.size() * sizeof(float3) * 3;
                MorphBuffer buffer{ SystemAllocator };
                buffer.resize(bufferSize);

                memcpy(buffer.data(), (void*)&morphTarget.vertices[0], bufferSize);

                morphBuffers.push_back(buffer);
            }
        }


        /************************************************************************************************/


        OptimizedMesh CreateOptimizedMesh(const MeshKDBTree& tree)
        {
            OptimizedMesh       optimized;
            LocalBlockContext   context{ tree.mesh };

            for (const auto& leaf : tree)
                for (auto I = leaf->begin; I < leaf->end; I++)
                    optimized.PushTri(tree.mesh.tris[I], context);


            /*
            const float3 meshMidPoint = tree.root->aabb.MidPoint();
            KDBTree::KDBNode* node = nullptr;

            float distance = 0;
            for (const auto& leaf : tree)
            {
                auto midPoint = leaf->aabb.MidPoint();
                auto d = (midPoint - meshMidPoint).magnitudeSq();

                if (d >= distance)
                {
                    d = distance;
                    node = leaf;
                }
            }

            float radius = 0.0f;

            std::for_each(node->begin, node->end,
                [&](Triangle& tri)
                {
                    for (auto& vertex : tri.vertices) {
                        const float3 point = tree.mesh.GetPoint(vertex);
                        auto d = (meshMidPoint - point).magnitude();

                        if (d > radius)
                            radius = fabs(d);
                    }
                });
            */

            optimized.aabb              = tree.root->aabb;
            optimized.boundingSphere    = tree.root->GetBoundingSphere(tree.mesh);

            return optimized;
        }


		/************************************************************************************************/


		void SkipSpaces( std::string::iterator& POSITION, std::string& line )
		{
			while( POSITION != line.end() && *POSITION == ' '  )
				POSITION++;
		}

		
		/************************************************************************************************/


		void RemoveSpaces(char* inLine, size_t RINOUT linelength) // moves all characters found after it finds first character, Sets line Length to exclude Spaces off of end
		{
			size_t spaceCount = 0;

			while (inLine[spaceCount] == ' ' && linelength > spaceCount)
				++spaceCount;
			size_t position = 0;
			for (; position < linelength - spaceCount; ++position)
			{
				inLine[position] = inLine[spaceCount + position];
			}
			inLine[position] = '\0';
			linelength -= spaceCount;
		}

		char* ScrubLine( char* inLine, size_t RINOUT linelength )
		{
			// Take Spaces and End Lines off beginning
			bool SkipSpaces = true;
			RemoveSpaces(inLine, linelength);

					size_t StrPos = 0;
			const	size_t tempSize = 512;
			char	temp[tempSize];

			while( StrPos != linelength )
			{
				switch( inLine[StrPos] )
				{
				case 0x0a:
				case 0x00:
					break;
				case 0x20:
					if( !SkipSpaces )
						temp[StrPos] = inLine[StrPos];
					break;
				default:
					SkipSpaces = false;
					temp[StrPos] = inLine[StrPos];
					break;
				}
				++StrPos;
			}
			
			if( !StrPos )
			{
				memcpy(inLine, temp, linelength);
				return inLine;
			}

			return inLine;
		}


		/************************************************************************************************/


        namespace OBJ_Tools
        {
            void SkipToFloat(size_t RINOUT itr, const char* in_str, size_t lineLength)
            {
                bool continueloop = true;
                while (itr < lineLength && continueloop)
                {
                    switch (in_str[itr])
                    {
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                    case '0':
                    case '-':
                        continueloop = false;
                        break;
                    default:
                        itr++;
                    }
                }
            }


            /************************************************************************************************/


            void ExtractFloat(size_t RINOUT itr, const char* in_str, const size_t LineLength, float* __restrict out)
            {
                char		c_str[16];
                uint32_t	itr2 = 0;
                bool continueloop = true;
                while (itr < LineLength && continueloop)
                {
                    switch (in_str[itr])
                    {
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                    case '0':
                    case '.':
                    case '-':
                        c_str[itr2] = in_str[itr];
                        itr2++;
                        break;
                    default:
                        continueloop = false;
                    }
                    itr++;
                }
                ((float*)out)[0] = (float)atof(c_str);
            }


            /************************************************************************************************/


            void ExtractFloats_3(const char* in, size_t LineLength, byte* out)
            {
                // Skip token plus spaces
                size_t itr = 0;

                itr++;
                itr++;

                //Get Float 1
                SkipToFloat(itr, in, LineLength);
                ExtractFloat(itr, in, LineLength, &((float*)out)[0]);
                //Get Float 2
                SkipToFloat(itr, in, LineLength);
                ExtractFloat(itr, in, LineLength, &((float*)out)[1]);
                //Get Float 3
                SkipToFloat(itr, in, LineLength);
                ExtractFloat(itr, in, LineLength, &((float*)out)[2]);
                //Done!
            }


            /************************************************************************************************/


            void ExtractFloats_2(const char* in, size_t LineLength, FlexKit::byte* out)
            {
                // Skip token plus spaces
                size_t itr = 0;

                itr++;
                itr++;

                //Get Float 1
                SkipToFloat(itr, in, LineLength);
                ExtractFloat(itr, in, LineLength, &((float*)out)[0]);
                //Get Float 2
                SkipToFloat(itr, in, LineLength);
                ExtractFloat(itr, in, LineLength, &((float*)out)[1]);
                //Done!
            }


            /************************************************************************************************/


            int GetSlashCount(const char* in_str, const size_t lineLength)
            {
                int slashcount = 0;
                auto itr = 0;
                itr++;
                itr++;
                if (in_str[itr] == ' ')
                    itr++;

                while (itr < lineLength && in_str[itr] != ' ')
                {
                    if (in_str[itr] == '/')
                        slashcount++;
                    itr++;
                }
                return slashcount;
            }


            /************************************************************************************************/


            int GetIndiceCount(const char* in_str, const size_t lineLength)
            {
                int spacecount = 0;
                auto itr = 0;
                itr++;
                itr++;
                if (in_str[itr] == ' ')
                    itr++;
                while (itr < lineLength)
                {
                    if (in_str[itr] == ' ')
                        spacecount++;
                    itr++;
                }
                return spacecount;
            }


            /************************************************************************************************/


            void ExtractIndice(size_t RINOUT itr, const char* in_str, size_t LineLength, CombinedVertex::IndexBitField* __restrict out, int ITYPE)
            {
                char			c_str[16];
                unsigned int	POS = 0;
                bool continueloop = true;

                while (itr < LineLength && continueloop)
                {
                    switch (in_str[itr])
                    {
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                    case '0':
                        c_str[POS] = in_str[itr];
                        POS++;
                        //ss << *in;
                        break;
                    default:
                        continueloop = false;
                    }
                    itr++;
                }

                uint32_t indice = 0;
                uint32_t size = 1;

                for (int32_t itr2 = POS - 1; itr2 >= 0; itr2--)
                {
                    indice += (c_str[itr2] - 48) * size;
                    size *= 10;
                }

                if (indice != 0)
                {
                    switch (ITYPE)
                    {
                    case 0:
                        (*out).p_Index = indice - 1; // Vertex
                        break;
                    case 1:
                        (*out).t_Index = indice - 1; // Texcoord
                        break;
                    case 2:
                        (*out).n_Index = indice - 1; // Normal
                        break;
                    }
                }
            }


            /************************************************************************************************/


            void ExtractFace(const char* str, size_t LineLength, CombinedVertex::IndexBitField* __restrict out, int index, int slashcount)
            {
                // Skip token plus spaces
                size_t itr = 0;

                itr++;
                itr++;
                if (str[itr] == ' ')
                    itr++;

                for (; index; index--)
                {
                    for (; str[itr] != ' ' && itr < LineLength; itr++);
                    for (; str[itr] == ' ' && itr < LineLength; itr++);
                }

                switch (slashcount)
                {
                case 0:

                    ExtractIndice(itr, str, LineLength, out, 0);
                    out[0].n_Index = 0xfffff;
                    out[0].t_Index = 0xfffff;

                    break;
                case 1:

                    ExtractIndice(itr, str, LineLength, out, 0);
                    ExtractIndice(itr, str, LineLength, out, 2);
                    out[0].n_Index = 0xfffff;

                    break;
                case 2:

                    ExtractIndice(itr, str, LineLength, out, 0);
                    ExtractIndice(itr, str, LineLength, out, 1);
                    ExtractIndice(itr, str, LineLength, out, 2);

                    break;
                default:
#if USING(FATALERROR)
                    FK_ASSERT(0); // Unsupported Face Combination, CRASHING!!!
#endif
                    break;
                }
                //Done!
            }


            /************************************************************************************************/


            std::string GetNextLine(const char* File_Position, const char* File_Buffer)
            {
                char CurrentLine[256];

                do
                {
                    //strcat( Line_Str, File_Position );
                    File_Position++;
                } while (*File_Position != '\0');
                //strcpy( CurrentLine, ScrubgetLineOBJ( Line_Str ) );

                return CurrentLine;
            }


            /************************************************************************************************/


            void AddVertexToken(float3 point, TokenList& out)
            {
                out.push_back(PointToken{ point });
            }


            /************************************************************************************************/


            void AddWeightToken(WeightIndexPair in, TokenList& out)
            {
                out.push_back(JointWeightToken{ float4{ (float3)in, 0 } });
                out.push_back(JointIndexToken{ in.Get<1>() });
			}


			/************************************************************************************************/


			void AddNormalToken(float3 in, TokenList& out)
			{
                out.push_back(NormalToken{ in });
			}


            /************************************************************************************************/


			void AddNormalToken(const float3 N, const float3 T, TokenList& out)
			{
                out.push_back(NormalToken{ N });
                out.push_back(TangentToken{ T });
			}


			/************************************************************************************************/


			void AddTexCordToken(const float3 in, TokenList& out)
			{
                out.push_back(TextureCoordinateToken{ in, 0 });
			}


            /************************************************************************************************/


            void AddTexCordToken(const float3 in, uint32_t coordinateIdx, TokenList& out)
            {
                out.push_back(TextureCoordinateToken{ in, coordinateIdx });
            }


			/************************************************************************************************/


			void AddIndexToken(const uint32_t V, const uint32_t N, const uint32_t T, TokenList& out)
			{
                VertexToken token;
                token.vertex.push_back(VertexField{ V, VertexField::Point });
                token.vertex.push_back(VertexField{ N, VertexField::Normal });
                token.vertex.push_back(VertexField{ T, VertexField::TextureCoordinate });

                std::sort(
                    token.vertex.begin(), token.vertex.end(),
                    [&](VertexField& lhs, VertexField& rhs)
                    {
                        return lhs.type < rhs.type;
                    });

                out.push_back(token);
			}


            /************************************************************************************************/


			void AddIndexToken(const uint32_t V, const uint32_t N, const uint32_t T, const uint32_t texCoord, TokenList& out)
			{
                VertexToken token;
                token.vertex.push_back(VertexField{ V, VertexField::Point });
                token.vertex.push_back(VertexField{ N, VertexField::Normal });
                token.vertex.push_back(VertexField{ T, VertexField::Tangent });
                token.vertex.push_back(VertexField{ texCoord, VertexField::TextureCoordinate });

                std::sort(
                    token.vertex.begin(), token.vertex.end(),
                    [&](VertexField& lhs, VertexField& rhs)
                    {
                        return lhs.type < rhs.type;
                    });

                out.push_back(token);
			}


			/************************************************************************************************/


			void CStrToToken( const char* in_Line, size_t LineLength, TokenList ROUT TL, LoaderState ROUT S)
			{
				union HigherLower
				{
					unsigned int BOTHPARTS : 16;

					struct PARTS
					{
						unsigned char HIGH	: 8;
						unsigned char LOW	: 8;
					} HIGH_LOW;
				}hl;

				hl.HIGH_LOW.LOW  = in_Line[0];
				hl.HIGH_LOW.HIGH = in_Line[1];

				int BOTH = hl.BOTHPARTS;

                /*
				switch (hl.BOTHPARTS)
				{
				case 0x00002320: // Comment
#ifdef _DEBUG
					// TODO: Decide on storage of string and Disposal
					newToken = s_TokenValue::Empty();
					TL.push_back( newToken );
#endif
					break;
				case 0x00006d74: // mtl MTLLIB
					newToken.token = COMMENT;
					TL.push_back( newToken );
					break;
				case 0x00006f20: // o iObject
					newToken.token = BEGINOBJECT;
					TL.push_back( newToken );
					break;
				case 0x00007620: // v Vertex
					newToken.token = POSITION_COORD;
					ExtractFloats_3( in_Line, LineLength, newToken.buffer );
					TL.push_back( newToken );
					break;
				case 0x00007674: // vt Vertex Texture Coord
					newToken.token = UV_COORD;
					ExtractFloats_2( in_Line, LineLength, newToken.buffer );
					TL.push_back( newToken );
					S.UV_1 = true;
					break;
				case 0x0000766e: // vn Vertex Normal
					newToken.token = Normal_COORD;
					ExtractFloats_3( in_Line, LineLength, newToken.buffer );
					TL.push_back( newToken );
					S.Normals = true;
					break;
				case 0x00007573: // us usemtl
					newToken.token = LOADMATERIAL;
					TL.push_back( newToken );
					break;
				case 0x00007320: // s_ set Smoothing
					newToken.token = SMOOTHING;
					TL.push_back( newToken );
					break;
				case 0x00006620: // f Face
					{
						int spacecount = GetIndiceCount	( in_Line, LineLength );
						int slashcount = GetSlashCount	( in_Line, LineLength );
						newToken.token = INDEX;
						for( auto itr = 0; itr <= spacecount; itr++ )
						{
							new( &newToken.buffer ) CombinedVertex::IndexBitField();
							ExtractFace( in_Line, LineLength, ( CombinedVertex::IndexBitField*)&newToken.buffer, itr, slashcount );
							TL.push_back( newToken );
						}
					}
					break;
				default:
					break;
				}
                */

		}

			/************************************************************************************************/
	}
	}
}
