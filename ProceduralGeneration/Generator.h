#pragma once
#include <MathUtils.h>
#include <static_vector.h>
#include <containers.h>
#include <span>

using FlexKit::int3;
using FlexKit::iAllocator;
using FlexKit::Vector;
using FlexKit::static_vector;


using CellCoord		= FlexKit::int3;
using ChunkCoord	= FlexKit::int3;
using CellState_t	= uint8_t;


int64_t GetChunkIndex(ChunkCoord coord);
int64_t GetTileID(const ChunkCoord cord) noexcept;

enum CellStates : CellState_t
{
	TileE,
	TileEW,
	TileN,
	TileNS,
	TileNSE,
	TileNSEW,
	Count,
	Super,
	Error
};


using TileSet = FlexKit::static_vector<CellState_t, CellStates::Count>;


struct MapChunk
{
	ChunkCoord		coord;
	bool			active = true;
	CellState_t		cells[8*8*8]; //8x8

	CellState_t&	operator[] (const int3 localCoord) noexcept;
	CellState_t		operator[] (const int3 localCoord) const noexcept;

	struct iterator
	{
		CellState_t*	cells;
		int				flatIdx;
		ChunkCoord		coord;

		struct _Pair
		{
			CellState_t& cell;
			int3    flatIdx;
		};

		_Pair operator * ();

		void operator ++ ();
		bool operator != (const iterator& rhs) const;
	};

	iterator begin();
	iterator end();

	operator int64_t () const;
};


struct ChunkedChunkMap
{
	ChunkedChunkMap(iAllocator& allocator) :
		chunkIDs	{ allocator },
		chunks		{ allocator } {}

	auto begin()	{ return chunks.begin(); }
	auto end()		{ return chunks.end(); }

	void InsertBlock(const int3 xyz, const int3 begin = { 0, 0, 0 });

	std::optional<std::reference_wrapper<MapChunk>>			operator [](ChunkCoord coord);
	std::optional<std::reference_wrapper<const MapChunk>>	operator [](ChunkCoord coord) const;

	Vector<int64_t>		chunkIDs;
	Vector<MapChunk>	chunks;
};

struct SparseMap;

struct iConstraint
{
	virtual bool		Constraint	(const	SparseMap& map, const CellCoord coord)	const noexcept = 0;
	virtual bool		Apply		(SparseMap& map, const CellCoord coord)	const noexcept = 0;
	virtual				GetInvalidTiles(const	SparseMap& map, const CellCoord coord)	const noexcept = 0;
};


using ConstraintTable = Vector<std::unique_ptr<iConstraint>, 16>;
using ConstraintSpan = std::span<std::unique_ptr<iConstraint>>;

struct SparseMap
{
	ChunkedChunkMap chunks;

	auto operator [](CellCoord cord) const
	{
		auto chunkID = cord / (8);
		if (auto res = chunks[chunkID]; res)
			return res.value().get()[cord - (chunkID * 8)];

		return (CellState_t)CellStates::Error;
	}

	struct NeighborIterator
	{
		NeighborIterator(const SparseMap& IN_map, CellCoord coord, int IN_idx = 0) :
			centralCord	{ coord  },
			map			{ IN_map },
			idx			{ IN_idx } {}

		auto operator * ()
		{
			static const int3 offsets[] =
			{
				{  0, -1,  0 },
				{ -1,  0,  0 },
				{  1,  0,  0 },
				{  0, -1,  0 },
				{  0,  0,  1 },
				{  0,  0, -1 },
			};

			const auto neighborCoord = offsets[idx] + centralCord;
			return std::make_pair(map[neighborCoord], neighborCoord);
		}

		NeighborIterator begin()    { return { map, centralCord, 0 }; }
		NeighborIterator end()      { return { map, centralCord, 6 }; }

		void operator ++ () noexcept { idx++; }

		bool operator != (const NeighborIterator& rhs) const
		{
			return !(rhs.centralCord == centralCord && rhs.idx == idx);
		}

		const SparseMap&	map;
		CellCoord			centralCord;
		int					idx = 0;
	};

	struct Neighbor
	{
		uint8_t c;
		int3    xyz;

		operator uint8_t& () { return c; }
	};

	auto GetChunk(FlexKit::uint3 xyz)
	{
		auto chunkID = (xyz & (-1 + 15)) / 8;

		return chunks[chunkID];
	}

	auto begin() { return chunks.begin(); }
	auto end() { return chunks.end(); }

	static FlexKit::CircularBuffer<Neighbor, 8> SparseMapGetNeighborRing(const SparseMap& map, CellCoord coord);
	TileSet										GetSuperState(this const SparseMap& map, const CellCoord xyz);
	TileSet										CalculateSuperState(this const SparseMap& map, const ConstraintSpan& constraints, const CellCoord xyz);

	void	UpdateNeighboringSuperStates(this auto& map, const ConstraintSpan& constraints, const CellCoord cellCoord);
	void	SetCell(const CellCoord XYZ, const CellState_t ID);

	void	RemoveBits(const CellCoord XYZ, const CellState_t ID);

	float	CalculateCellEntropy(this const auto& map, const CellCoord coord, const static_vector<CellState_t>& superSet);

	void	Generate(this SparseMap& map, const ConstraintSpan& constraints, const FlexKit::uint3 WHD, iAllocator& tempMemory);
};


using ChunkTranslatorFN = std::function<void(const MapChunk&)>;

void GenerateWorld(ChunkTranslatorFN translator, ConstraintTable& constraints, iAllocator& temp);


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
