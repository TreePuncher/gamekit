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

#include "..\pch.h"
#include "..\graphicsutilities\MeshUtils.h"
#include "..\coreutilities\memoryutilities.h"


namespace FlexKit
{
	bool operator < ( const CombinedVertex::IndexBitField lhs, const CombinedVertex::IndexBitField rhs )
	{
		return lhs.Hash() > rhs.Hash();
	}


	namespace MeshUtilityFunctions
	{
		/************************************************************************************************/


		Pair<bool, MeshBuildInfo>
		BuildVertexBuffer(TokenList RIN in, CombinedVertexBuffer ROUT out_buffer, IndexList ROUT out_indexes, iAllocator* LevelSpace, iAllocator* ScratchSpace, bool DoWeights, bool tangentsIncluded )
		{
			Vector<float3>		position	{ ScratchSpace };
			Vector<float3>		normal		{ ScratchSpace };
			Vector<float3>		tangent		{ ScratchSpace };
			Vector<float2>		UV			{ ScratchSpace };
			Vector<float3>		Weights		{ ScratchSpace };
			Vector<uint4_16>	WIndexes	{ ScratchSpace };
            Vector<size_t>	    NormalTable { ScratchSpace };
            Vector<size_t>	    TangentTable{ ScratchSpace };

			IndexList&	Indexes	= out_indexes;
			MeshBuildInfo MBI	= {0};

			map_t<CombinedVertex::IndexBitField, unsigned int>	IndexMap;
			auto& FinalVerts = out_buffer;

			auto count  = 0;
			auto count2 = 0;

			for( auto itr : in )
			{
				switch( itr.token )
				{
				case BEGINOBJECT:
					break;
				case COMMENT:
					break;
				case END:
					return{ true, MBI };
					break;
				case ERRORTOKEN:
					FK_ASSERT( 0 );
					break;
				case INDEX:
                {
                    const s_TokenValue	token = itr;
                    CombinedVertex	    vertex;

                    CombinedVertex::IndexBitField bitLayout;
                    memcpy(&bitLayout, token.buffer, sizeof(bitLayout));

                    vertex.index		        = bitLayout;
                    vertex.index.n_Index        = NormalTable[vertex.index.n_Index];
					memcpy(&vertex.POS, &position[vertex.index.p_Index], sizeof(vertex.POS));

					if (DoWeights) {
						memcpy(&vertex.WEIGHTS,  &Weights[vertex.index.p_Index],  sizeof(vertex.POS));
						memcpy(&vertex.WIndices, &WIndexes[vertex.index.p_Index], sizeof(uint4_16));
					}
					if (normal.size() != 0)
						memcpy(&vertex.NORMAL, &normal[vertex.index.n_Index], sizeof(vertex.NORMAL));
					else {
                        memcpy(&vertex.NORMAL, &float3{ 0, 0, 0 }, sizeof(vertex.NORMAL));
					}
					if (UV.size() != 0)
						memcpy(&vertex.TEXCOORD, &UV[vertex.index.t_Index], sizeof(vertex.NORMAL));
					else
					{
						float2 Temp;
						memcpy(&vertex.TEXCOORD, &Temp, sizeof(vertex.TEXCOORD));
					}
					if (IndexMap.find(vertex.index) == IndexMap.end())
					{
						Indexes.push_back(static_cast<uint32_t>(FinalVerts.size()));
						IndexMap[vertex.index] = static_cast<unsigned int>(FinalVerts.size());
						FinalVerts.push_back(vertex);
						count++;
					}
					else
					{
						Indexes.push_back(IndexMap[vertex.index]);
						count2++;
					}

					MBI.IndexCount++;
				}	break;
				case LOADMATERIAL:
					break;
				case Normal_COORD:
				{
					bool unqiue = true;
                    float3 newNormal;
                    float3 newTangent;

                    memcpy(&newNormal, itr.buffer,                      sizeof(float[3]));
                    memcpy(&newTangent, itr.buffer + sizeof(float[3]),  sizeof(float[3]));

					newNormal[3]  = 0.0f;
                    newTangent[3] = 0.0f;

					newNormal.normalize();

                    if(tangentsIncluded)
					    newTangent.normalize();
					
					for (size_t I = 0; I < normal.size(); ++I)
					{
						if ( 1 - normal[I].dot( newNormal ) < 0.00001f)
						{
							unqiue = false;
							NormalTable.push_back(I);
							break;
						}
					}

					if (unqiue) {
						NormalTable.push_back(normal.size());
						normal.push_back(newNormal);

                        if (tangentsIncluded)
                            tangent.push_back(newTangent);
					}
				}break;
				case POSITION_COORD:
				{
					position.push_back(*(float3*)itr.buffer);
				}   break;
				case WEIGHT:
				{
					if(DoWeights)
						Weights.push_back(*(float3*)itr.buffer);
				}	break;
				case WEIGHTINDEX:
				{
					if (DoWeights)
					{
						auto Indexes = *(uint4_32*)itr.buffer;
						for(size_t itr = 0; itr < 4; ++itr)
							Indexes[itr] = (Indexes[itr] == -1) ? 0 : Indexes[itr];

						WIndexes.push_back(Indexes);
					}
				}	break;
				case UV_COORD:
					UV.push_back( *(float2*)itr.buffer );
					break;
				case SMOOTHING:
					break;
				default:
					return{ false, {0} };
				}
			}

			MBI.VertexCount = FinalVerts.size();
			return{ true, MBI };
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
			void SkipToFloat(  size_t RINOUT itr, const char* in_str, size_t lineLength )
			{
				bool continueloop = true;
				while( itr < lineLength && continueloop )
				{
					switch ( in_str[itr] )
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


			void ExtractFloat( size_t RINOUT itr, const char* in_str, const size_t LineLength, float* __restrict out )
			{
				char		c_str[16];
				uint32_t	itr2 = 0;
				bool continueloop = true;
				while( itr < LineLength && continueloop )
				{
					switch ( in_str[itr] )
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
				((float*)out)[0] = (float)atof( c_str );
			}


			/************************************************************************************************/


			void ExtractFloats_3( const char* in, size_t LineLength, byte* out )
			{
				// Skip token plus spaces
				size_t itr = 0;

				itr++;
				itr++;

				//Get Float 1
				SkipToFloat	( itr, in, LineLength );
				ExtractFloat( itr, in, LineLength, &((float*)out)[0] );
				//Get Float 2
				SkipToFloat	( itr, in, LineLength );
				ExtractFloat( itr, in, LineLength, &((float*)out)[1] );
				//Get Float 3
				SkipToFloat	( itr, in, LineLength );
				ExtractFloat( itr, in, LineLength, &((float*)out)[2] );
				//Done!
			}


			/************************************************************************************************/


			void ExtractFloats_2( const char* in, size_t LineLength, FlexKit::byte* out )
			{
				// Skip token plus spaces
				size_t itr = 0;

				itr++;
				itr++;

				//Get Float 1
				SkipToFloat( itr, in, LineLength );
				ExtractFloat( itr, in, LineLength, &((float*)out)[0] );
				//Get Float 2
				SkipToFloat( itr, in, LineLength );
				ExtractFloat( itr, in, LineLength, &((float*)out)[1] );
				//Done!
			}


			/************************************************************************************************/


			int GetSlashCount( const char* in_str, const size_t lineLength )
			{
				int slashcount = 0;
				auto itr = 0;
				itr++;
				itr++;
				if( in_str[itr]== ' ' )
					itr++;

				while( itr < lineLength && in_str[itr] != ' ' )
				{
					if( in_str[itr] == '/' )
						slashcount++;
					itr++;
				}
				return slashcount;
			}


			/************************************************************************************************/


			int GetIndiceCount( const char* in_str, const size_t lineLength )
			{
				int spacecount = 0;
				auto itr = 0;
				itr++;
				itr++;
				if( in_str[itr] == ' ' )
					itr++;
				while( itr < lineLength )
				{
					if( in_str[itr] == ' ' )
						spacecount++;
					itr++;
				}
				return spacecount;
			}


			/************************************************************************************************/


			void ExtractIndice( size_t RINOUT itr, const char* in_str, size_t LineLength, CombinedVertex::IndexBitField* __restrict out, int ITYPE )
			{
				char			c_str[ 16 ];
				unsigned int	POS = 0;
				bool continueloop = true;

				while( itr < LineLength && continueloop  )
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

				for( int32_t itr2 = POS - 1; itr2 >= 0; itr2-- )
				{
					indice += ( c_str[itr2] - 48 ) * size;
					size *= 10;
				}

				if( indice != 0 )
				{
					switch( ITYPE )
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


			void ExtractFace( const char* str, size_t LineLength, CombinedVertex::IndexBitField* __restrict out, int index, int slashcount )
			{
				// Skip token plus spaces
				size_t itr = 0;

				itr++;
				itr++;
				if( str[itr]== ' ' )
					itr++;

				for( ; index; index-- )
				{
					for( ; str[itr] != ' ' && itr < LineLength; itr++ );
					for( ; str[itr] == ' ' && itr < LineLength; itr++ );
				}

				switch ( slashcount )
				{
				case 0:

					ExtractIndice( itr, str, LineLength, out, 0 );
					out[0].n_Index = 0xfffff;
					out[0].t_Index = 0xfffff;
					
					break;
				case 1:

					ExtractIndice( itr, str, LineLength, out, 0 );
					ExtractIndice( itr, str, LineLength, out, 2 );
					out[0].n_Index = 0xfffff;

					break;
				case 2:

					ExtractIndice( itr, str, LineLength, out, 0 );
					ExtractIndice( itr, str, LineLength, out, 1 );
					ExtractIndice( itr, str, LineLength, out, 2 );

					break;
				default:
#if USING(FATALERROR)
					FK_ASSERT( 0 ); // Unsupported Face Combination, CRASHING!!!
#endif
					break;
				}
				//Done!
			}


			/************************************************************************************************/


			std::string GetNextLine( const char* File_Position, const char* File_Buffer )
			{
				char CurrentLine[256];

				do
				{
					//strcat( Line_Str, File_Position );
					File_Position++;
				} while( *File_Position != '\0' );
				//strcpy( CurrentLine, ScrubgetLineOBJ( Line_Str ) );

				return CurrentLine;
			}
			

			/************************************************************************************************/


			void AddVertexToken(float3 in, TokenList& out)
			{
				s_TokenValue T;
				T.token = FlexKit::Token::POSITION_COORD;
				s_TokenVertexLayout* V =  (s_TokenVertexLayout*)T.buffer;
				V->f[0] = in.x;
				V->f[1] = in.y;
				V->f[2] = in.z;
				out.push_back(T);
			}
			

			/************************************************************************************************/


			void AddWeightToken(WeightIndexPair in, TokenList& out)
			{
				s_TokenValue T;
				T.token = FlexKit::Token::WEIGHT;
				s_TokenVertexLayout* V =  (s_TokenVertexLayout*)T.buffer;
				V->f[0] = ((float3)in).x;
				V->f[1] = ((float3)in).y;
				V->f[2] = ((float3)in).z;
				out.push_back(T);

				T.token = FlexKit::Token::WEIGHTINDEX;
				s_TokenWIndexLayout* W = (s_TokenWIndexLayout*)T.buffer;
				W->i[0] = in.V2[0];
				W->i[1] = in.V2[1];
				W->i[2] = in.V2[2];
				W->i[3] = in.V2[3];
				out.push_back(T);
			}


			/************************************************************************************************/


			void AddNormalToken(float3 in, TokenList& out)
			{
				s_TokenValue T;
				T.token = FlexKit::Token::Normal_COORD;
				s_TokenVertexLayout* V =  (s_TokenVertexLayout*)T.buffer;
				V->f[0] = in.x;
				V->f[1] = in.y;
				V->f[2] = in.z;
				out.push_back(T);
			}


            /************************************************************************************************/


			void AddNormalToken(const float3 N, const float3 T, TokenList& out)
			{
				s_TokenValue token;
				token.token = FlexKit::Token::Normal_COORD;
                memcpy(token.buffer, &N, sizeof(float[3]));
                memcpy(token.buffer + sizeof(float[3]), &N, sizeof(float3));

				out.push_back(token);
			}


			/************************************************************************************************/


			void AddTexCordToken(float3 in, TokenList& out)
			{
				s_TokenValue T;
				T.token = FlexKit::Token::UV_COORD;
				s_TokenVertexLayout* V =  (s_TokenVertexLayout*)T.buffer;
				V->f[0] = in.x;
				V->f[1] = in.y;
				V->f[2] = 0.0;

				out.push_back(T);
			}


			/************************************************************************************************/


			void AddIndexToken(size_t V, size_t N, size_t T, TokenList& out)
			{
				s_TokenValue					Token;
				CombinedVertex::IndexBitField*	BitLayout = (CombinedVertex::IndexBitField*)Token.buffer;
				
				Token.token = Token::INDEX;

				CombinedVertex::IndexBitField Temp;
				Temp.p_Index = uint64_t(V);
				Temp.n_Index = uint64_t(N);
				Temp.t_Index = uint64_t(T);

				memcpy(Token.buffer, &Temp, sizeof(Temp));
				out.push_back(Token);
			}


			/************************************************************************************************/


			void AddPatchBeginToken(TokenList& out)
			{
				s_TokenValue Token;
				Token.token = Token::PATCHBEGIN;
				out.push_back(Token);
			}


			/************************************************************************************************/


			void AddPatchEndToken(TokenList& out)
			{
				s_TokenValue Token;
				Token.token = Token::PATCHEND;
				out.push_back(Token);
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
				s_TokenValue newToken;

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
		}

			/************************************************************************************************/
	}
	}
}
