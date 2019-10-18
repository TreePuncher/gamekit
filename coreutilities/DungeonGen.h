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

#include "..\buildsettings.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\MathUtils.h"
#include <cstdlib>

//#pragma warning( disable : 4251 )

#ifndef DUNGEONGEN_H
#define DUNGEONGEN_H

// TODO: Change Out TileID struct to Tile ID, and update methods that use TileID to Tile ID
// TODO: Change Methods to Use Tile ID, and GetTile instead
// TODO: After Change to Tile ID's lift out methods to Functions
// Usings

#define	DUNGEONGRIDHEIGHT 128
#define DUNGEONGRIDWIDTH  128
#define DUNGEONMAXRMCOUNT 100

namespace FlexKit
{
	typedef uint2 TileID;

	struct FLEXKITAPI DungeonGenerator
	{


		DungeonGenerator()
		{
			Seed = 0;
			RoomCount = 0;
			TileStackSize = 0;
		}

		enum Tile : char
		{
			Empty         = 'S',
			FLOOR_Dungeon = '-',
			/*
			WALL_TOP	  = 'T',
			WALL_Left	  = 'L',
			WALL_Right	  = 'R',
			WALL_BOTTOM	  = 'B',
			*/
			WALL		  = '-',
			WALL_TOP      = '-',
			WALL_Left     = '-',
			WALL_Right    = '-',
			WALL_BOTTOM   = '-',
			Corner        = '-',
			DoorWay       = 'A',
			Corridor      = '_',
			Connector     = 'X',
			DEBUG         = 'D'
		};

		typedef void(*ScanCallBack)(byte* _ptr, Tile in[3][3], uint2 POS, uint2 BPOS, DungeonGenerator* D);

		void BlockScanCallBack	(ScanCallBack out, byte* _ptr, uint2 XY, uint2 HW);
		void DungeonScanCallBack(ScanCallBack out, byte* _ptr);

		TileID TileStack[DUNGEONGRIDHEIGHT * DUNGEONGRIDWIDTH];
		size_t TileStackSize;


		TileID PopBack();
		void RemoveTileID(size_t i);

		inline void PushBackTileID(TileID c) { TileStack[TileStackSize++] = c; }
		inline void ClearStack(){ TileStackSize = 0; }

		inline TileID	GetTile()					{ return TileStack[TileStackSize - 1]; } // Gets Last Node }
		inline Tile		GetTile(TileID POS)			{ return Tiles[POS[1]][POS[0]]; }
		inline void		SetTile(TileID POS, Tile t)	{ Tiles[POS[1]][POS[0]] = t; }

		Tile	Tiles[DUNGEONGRIDHEIGHT][DUNGEONGRIDWIDTH];
		struct  Room
		{
			size_t x, y, Height, Width; bool Connected;
		}Rooms[DUNGEONMAXRMCOUNT];

		size_t	RoomCount;
		size_t	Seed;

		void AddRoom(Room r);
		void CleanTiles();
		void Generate();
		void PrintDungeon();

		bool isWall(size_t x, size_t y); // DEPRECIATED NEED TO BE REMOVED
		bool isWall(uint2 XY);

		void	FindConnectors();
		bool	areRoomsConnected();
		size_t	FindUnconnectedRoom();

		void	ConnectRoom(size_t I);
		void	ConnectRooms();
		void	GenerateHallways();

		bool	isTileIDInGrid	(TileID C);
		bool	CanCarve		(TileID C);
		bool	CanCarveInD		(TileID C, int D);

		int		NeighborCount		(TileID C);
		int		DiagnalNeighborCount(TileID C);

		void	GrowMaze	(size_t x, size_t y);
		void	RemoveRoom	(size_t I);

		bool	AddRoom			(size_t x, size_t y, size_t height, size_t width, size_t sparseness);
		void	AddCorridorNF	(size_t x, size_t y, int length, size_t width);

		// Region Functions
		bool	ScanRegion	(size_t x_1, size_t y_1, size_t x_2, size_t y_2);
		size_t	ScanRegionB	(size_t x_1, size_t y_1, size_t x_2, size_t y_2);
		bool	FillRegion	(size_t x_1, size_t y_1, size_t x_2, size_t y_2, Tile t);
	};

	FLEXKITAPI uint2	TranslatePositionToDungeonCord(DungeonGenerator* D, float3, float S);
	FLEXKITAPI float3	DungeonCordToTranslatePosition(DungeonGenerator* D, uint2, float S);
}

#endif
