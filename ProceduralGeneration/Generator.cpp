#include "pch.h"
#include "Generator.h"

#include <type_traits>

using namespace FlexKit;

/************************************************************************************************/


int64_t GetChunkIndex(ChunkCoord coord)
{
	return coord[2] * 2048 * 2048 + coord[1] * 2048 + coord[0];
}

struct SparseMap;

struct MapCordHasher
{
	uint64_t operator ()(const CellCoord& in) noexcept
	{
		return -1;
	}
};


static_assert(CellStates::Count < sizeof(CellState_t) * 8, "Increase CellState_t ByteSize");

constexpr uint8_t GetIdBit(const CellState_t id)
{
	return 1 << id;
}

constexpr float CellWeights[] = { 1, 1, 1, 1, 1, 1 };

template<size_t ... ints>
consteval auto _GetCellState_tsCollectionHelper(std::integer_sequence<size_t, ints...> int_seq)
{
	return std::array<CellState_t, (size_t)CellStates::Count>{ ints... };
}

template<size_t ... ints>
consteval auto _GetCellState_tsProbabilitiesHelper(std::integer_sequence<size_t, ints...> int_seq, const float total)
{
	return std::array<float, (size_t)CellStates::Count>{ ((CellWeights[ints] / total), ...) };
}

constinit const auto SuperSet = []() { return _GetCellState_tsCollectionHelper(std::make_index_sequence<(size_t)CellStates::Count>{});  }();

constinit std::array<float, (size_t)CellStates::Count> CellProbabilies =
														[] () -> std::array<float, (size_t)CellStates::Count>
														{
															const float sum = std::accumulate(std::begin(CellWeights), std::end(CellWeights), 0.0f);

															return _GetCellState_tsProbabilitiesHelper(std::make_index_sequence<(size_t)CellStates::Count>{}, sum);
														}();

consteval CellState_t FullSet()
{
	uint8_t set = 0;
	for (uint8_t I = 0; I < CellStates::Count; I++)
		set |= GetIdBit(I);

	return set;
}




int64_t GetTileID(const ChunkCoord cord) noexcept
{
	constexpr auto r_max = std::numeric_limits<int16_t>::max();

	return cord[0] + cord[1] * r_max + cord[2] * r_max * r_max;
}


CellState_t& MapChunk::operator[] (const int3 localCoord) noexcept
{
	const auto idx = abs(localCoord[0]) % 8 + abs(localCoord[1]) % 8 * 8 + abs(localCoord[2]) % 8 * 64;

	return cells[idx];
}

CellState_t MapChunk::operator[] (const int3 localCoord) const noexcept
{
	const auto idx = abs(localCoord[0]) % 8 + abs(localCoord[1]) % 8 * 8 + abs(localCoord[2]) % 8 * 64;
	return cells[idx];
}


/************************************************************************************************/


MapChunk::iterator::_Pair MapChunk::iterator::operator * ()
{
	return { cells[flatIdx], coord * 8 + int3{ flatIdx % 8, (flatIdx / 8) % 8,  (flatIdx / 64) } };
}

void MapChunk::iterator::operator ++ ()
{
	flatIdx++;
}

bool MapChunk::iterator::operator != (const iterator& rhs) const
{
	return !(cells == rhs.cells && flatIdx == rhs.flatIdx);
}

MapChunk::iterator MapChunk::begin()
{
	if (active)
		return iterator{ cells, 0, coord };
	else return end();
}

MapChunk::iterator MapChunk::end()
{
	return iterator{ cells, 8 * 8 * 8, coord };
}

MapChunk::operator int64_t () const { return GetTileID(coord); }


/************************************************************************************************/


void ChunkedChunkMap::InsertBlock(const int3 xyz, const int3 begin)
{
	const auto offset = xyz / 2;

	for (int x = 0; x < xyz[0]; x++)
		for (int y = 0; y < xyz[1]; y++)
			for (int z = 0; z < xyz[2]; z++)
			{
				const auto chunkIdx = chunks.emplace_back(begin + int3{ x, y, z } - offset);

				for (auto&& [cell, cellIdx] : chunks[chunkIdx])
					cell = FullSet();

				chunks[chunkIdx].active = false;
			}

	std::ranges::sort(chunks);
	chunkIDs.resize(chunks.size());
	for (size_t I = 0; I < chunks.size(); I++)
		chunkIDs[I] = chunks[I];
}


/************************************************************************************************/


std::optional<std::reference_wrapper<MapChunk>> ChunkedChunkMap::operator [](ChunkCoord coord)
{
	const auto idx = GetChunkIndex(coord);

	auto res = std::ranges::lower_bound(chunkIDs, idx);

	if (res != chunkIDs.end())
	{
		auto idx = std::distance(chunkIDs.begin(), res);
		if (chunks[idx].coord == coord)
			return { chunks[idx] };
	}

	return {};
}


/************************************************************************************************/


std::optional<std::reference_wrapper<const MapChunk>> ChunkedChunkMap::operator [](ChunkCoord coord) const
{
	const auto idx = GetChunkIndex(coord);

	auto res = std::ranges::lower_bound(chunks, idx);

	if (res != chunks.end() && res->coord == coord)
	{
		//if (idx != 0)
		//	DebugBreak();

		return { *res };
	}
	else
		return {};
}


/************************************************************************************************/



CircularBuffer<SparseMap::Neighbor, 8> SparseMap::SparseMapGetNeighborRing(const SparseMap& map, CellCoord coord)
{
	CircularBuffer<SparseMap::Neighbor, 8> buffer;
	for (auto&& [c, xyz] : SparseMap::NeighborIterator{map, coord})
		buffer.push_back({ c, xyz });

	return buffer;
}


TileSet SparseMap::GetSuperState(this const SparseMap& map, const CellCoord xyz)
{
	auto bits = map[xyz];
	static_vector<CellState_t> set;
	for (size_t I = 0; I < CellStates::Count; I++)
	{
		if (GetIdBit((CellState_t)I) & bits)
			set.push_back((CellState_t)I);
	}

	return set;
}

TileSet SparseMap::CalculateSuperState(this const SparseMap& map, const ConstraintSpan& constraints, const CellCoord xyz)
{
	TileSet out;

	for(auto& constraint : constraints)
	{
		if (!constraint->Constraint(map, xyz))
		{
			auto invalidTileSet = constraint->GetInvalidTiles(map, xyz);
		}
	}

	return out;
}

void SparseMap::UpdateNeighboringSuperStates(this auto& map, const ConstraintSpan& constraints, const CellCoord cellCoord)
{
	for (auto [neighborCell, neighborCellState_tx] : NeighborIterator{ map, cellCoord })
	{
		if (__popcnt(neighborCell) == 1)
			continue;

		auto set = map.CalculateSuperState(constraints, neighborCellState_tx);

		if (set.size() == 1)
		{
			map.SetCell(neighborCellState_tx, GetIdBit(set.front()));
			//map.UpdateNeighboringSuperStates(constraints, neighborCellState_tx);
		}
		else
		{
			CellState_t bitSet = 0;

			for (auto&& it : set)
				bitSet |= GetIdBit(it);

			map.SetCell(neighborCellState_tx, bitSet);
		}
	}
}

void SparseMap::SetCell(const CellCoord XYZ, const CellState_t ID)
{
	auto chunkID = XYZ / 8;

	if (auto res = chunks[chunkID]; res)
	{
		auto& chunk = res.value().get();
		chunk[XYZ % 8] = ID;
		chunk.active = true;
	}
}

	
void SparseMap::RemoveBits(const CellCoord XYZ, const CellState_t ID)
{
	auto chunkID = (XYZ & (-1 + 15)) / 8;

	if (auto res = chunks[chunkID]; res)
	{
		auto& chunk			= res.value().get();
		auto currentState	= chunk[XYZ % 8];
		auto newState		= currentState & ~ID;
		chunk[XYZ % 8]		= newState;

		chunk.active = true;
	}
}


float SparseMap::CalculateCellEntropy(this const auto& map, const CellCoord coord, const static_vector<CellState_t>& superSet)
{
	const auto sum = [&]()
		{
			float s = 0;
			for(auto e : superSet)
				s += CellWeights[e];

			return s;
		}();

	float entropy = 0.0f;
	for (auto& cell : superSet)
		entropy += -log(CellWeights[cell] / sum) * CellWeights[cell];

	return entropy;
}

void SparseMap::Generate(this SparseMap& map, const ConstraintSpan& constraints, const uint3 WHD, iAllocator& tempMemory)
{
	const auto XYZ = Max(WHD, uint3{ 8, 8, 8 }) / 8;

	map.chunks.InsertBlock(XYZ, XYZ / -2);
	map.UpdateNeighboringSuperStates(constraints, { 0, 0, 0 });


	struct EntropyPair
	{
		float       entropy;
		CellCoord   cellIdx;

		operator float() const { return entropy; }
	};

	Vector<EntropyPair> entropySet{ tempMemory };
	entropySet.reserve(4096);

	bool continueGeneration;
		
	do
	{
		continueGeneration = false;

		for (auto& chunk : map.chunks)
		{
			if (chunk.active)
			{
				for (auto [cell, cellCoord] : chunk)
				{
					auto res = __popcnt(cell);
					if (__popcnt(cell) > 1)
					{
						continueGeneration |= true;

						const auto super = map.GetSuperState(cellCoord);

						CellState_t bitSet = 0;

						for (auto&& it : super)
							bitSet |= GetIdBit(it);

						map.SetCell(cellCoord, bitSet);

						entropySet.emplace_back(
							map.CalculateCellEntropy(cellCoord, super),
							cellCoord);

						if (!super.size())
							DebugBreak();
					}
				}
			}
		}


		if (entropySet.size())
		{
			std::ranges::partial_sort(entropySet, entropySet.begin() + 1);

			auto entity = entropySet.front();
			const auto superSet = map.GetSuperState(entity.cellIdx);
				
			if (superSet.size() == 1)
			{
				for (auto& constraint : constraints)
					constraint->Apply(map, entity.cellIdx);

				map.UpdateNeighboringSuperStates(constraints, entity.cellIdx);
			}
			else if (superSet.size() > 1)
			{
				auto cellId = superSet[rand() % superSet.size()];

				for (auto& constraint : constraints)
					constraint->Apply(map, entity.cellIdx);

				map.UpdateNeighboringSuperStates(constraints, entity.cellIdx);
			}
			else
				DebugBreak();
		}

		entropySet.clear();

		if (0)
		{
			std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";

			if (auto chunk = map.GetChunk({ 0, 0, 0 }); chunk)
			{
				auto& chunk_ref = chunk.value().get();

				int x = 0;
				int y = 0;
				for (auto [c, _] : chunk_ref)
				{
					/*
					if(c & GetIdBit(CellStates::Floor))
						std::cout << "_";
					if(c & GetIdBit(CellStates::Ramp))
						std::cout << "R";
					if(c & GetIdBit(CellStates::Wall))
						std::cout << "X";
					if(c & GetIdBit(CellStates::Corner))
						std::cout << "C";
					*/

					auto res = __popcnt(c);
					for(size_t i = 0; i < CellStates::Count - res; i++)
						std::cout << " ";

					if (++x >= 8)
					{
						std::cout << "\n";

						y++;
						x = 0;
						if (y > 8)
							break;
					}
				}
			}
		}
	} while (continueGeneration);
}


/************************************************************************************************/


void GenerateWorld(ChunkTranslatorFN translator, ConstraintTable& constraints, iAllocator& temp)
{
	// Step 1. Generate world
	SparseMap map{ temp };

	map.Generate(constraints, int3{ 256, 256, 1 }, temp);

	// Step 2. Translate map cells -> game world
	for (auto& chunk : map)
	{
		if (chunk.active)
			translator(chunk);// TranslateChunk(chunk, map, world, assets, temp);
	}

	// Print Chunk (0, 0, 0)
	std::cout << "Legend:\n";
	std::cout << "_ = Space\n";
	std::cout << "R = Ramp\n";
	std::cout << "X = Wall\n";
	std::cout << "C = Corner\n";

	auto chunk = map.GetChunk({ 0, 0, 0 });
	if (chunk)
	{
		auto& chunk_ref = chunk.value().get();

		int x = 0;
		int y = 0;
		for (auto [c, _] : chunk_ref)
		{
			/*
			if (c & GetIdBit(CellStates::Floor))
				std::cout << "_ ";
			else if (c & GetIdBit(CellStates::Ramp))
				std::cout << "R ";
			else if (c & GetIdBit(CellStates::Wall))
				std::cout << "X ";
			if (c & GetIdBit(CellStates::Corner))
				std::cout << "C ";
			*/

			if (++x >= 8)
			{
				std::cout << "\n";

				y++;
				x = 0;
				if (y > 8)
					break;
			}
		}
	}

	//return map;
}


/**********************************************************************

Copyright (c) 2015 - 2023 Robert May

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
