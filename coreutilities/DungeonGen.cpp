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

#include "..\coreutilities\DungeonGen.H"

#pragma warning( disable : 4267 )

namespace FlexKit
{
	const int2 North	= int2{ 0, -1 };
	const int2 East		= int2{ 1,  0 };
	const int2 South	= int2{ 0,  1 };
	const int2 West		= int2{ -1, 0 };

	const static_vector<int2, 4> Directions = static_vector < int2, 4 > {{0, -1}, { 1, 0 }, { 0, 1 }, { -1, 0 }};

	/************************************************************************************************/

	bool DungeonGenerator::FillRegion(size_t x_1, size_t y_1, size_t x_2, size_t y_2, Tile t)
	{
		int step = 0;
		if (y_1 < y_2)
			step = -1;
		else if ( y_1 > y_2)
			step = 1;
		else// Region too Small
			return false;

		for (size_t y = y_2; y != y_1; y+= step)
			for (size_t x = x_2; x<x_1; ++x)
				Tiles[y][x]=FLOOR_Dungeon;
			
		return true;
	}

	/************************************************************************************************/

	size_t DungeonGenerator::ScanRegionB(size_t x_1, size_t y_1, size_t x_2, size_t y_2)
	{
		int step = 0;
		if (y_1 < y_2)
			step = -1;
		else if (y_1 > y_2)
			step = 1;
		else// Region too Small
			return false;

		size_t Count = 0;
		for (size_t y = y_2; y != y_1; y += step){
			for (size_t x = x_2; x < x_1; ++x)
				if (Tiles[y][x] != Empty)
					++Count;
		}

		return Count;
	}
	
	/************************************************************************************************/

	bool DungeonGenerator::ScanRegion(size_t x_1, size_t y_1, size_t x_2, size_t y_2)
	{
		int step = 0;
		if (y_1 < y_2)
			step = -1;
		else if (y_1 > y_2)
			step = 1;
		else// Region too Small
			return false;

		for (size_t y = y_2; y != y_1; y += step){
			for (size_t x = x_2; x < x_1; ++x)
				if (Tiles[y][x] != Empty)
					return false;
		}

		return true;
	}

	/************************************************************************************************/

	void DungeonGenerator::AddCorridorNF(size_t x, size_t y, int length, size_t width)
	{	// TODO Finish This
		size_t x_1 = x + width / 2;
		size_t x_2 = x - width / 2;
		size_t EY = y + length;

		if (ScanRegion(x_1, y, x_2, EY))
		{
			float EY = x + length;
			float EX = x;
			size_t step = length / abs(length);

			for (auto I = 0; I < length; ++I)
			{
			}
		}
	}

	/************************************************************************************************/

	void DungeonGenerator::RemoveRoom(size_t I)
	{
		auto temp = Rooms[RoomCount - 1];
		Rooms[I] = temp;
		Rooms[RoomCount - 1] = Room();
		RoomCount--;
	}

	/************************************************************************************************/

	bool DungeonGenerator::AddRoom(size_t x, size_t y, size_t height, size_t width, size_t sparseness)
	{
		int x_1 = x + width / 2;
		int x_2 = x - width / 2;
		int y_1 = y + height / 2;
		int y_2 = y - height / 2;

		if (y_2 < sparseness)
			return false;

		if (y_1 + sparseness >= DUNGEONGRIDHEIGHT)
			return false;

		if (x_2 < sparseness && x_2 - sparseness > 0)
			return false;

		if (x_1 + sparseness >= DUNGEONGRIDWIDTH)
			return false;

		if (ScanRegion(x_1 + sparseness, y_1 + sparseness, x_2 - sparseness, y_2 - sparseness))
		{
			FillRegion(x_1, y_1, x_2, y_2, FLOOR_Dungeon);

			Tiles[y_1][x_1] = Corner;// Top Left
			Tiles[y_2][x_1] = Corner;// Bottom Left
			Tiles[y_1][x_2] = Corner;// Top Right
			Tiles[y_2][x_2] = Corner;// Bottom Right


			{// Bottom Wall
				size_t x = x_2 + 1;
				while (x < x_1){
					Tiles[y_2][x] = WALL_TOP;
					++x;
				}
			}

			{// Bottom Wall
				size_t x = x_2 + 1;
				while (x < x_1){
					Tiles[y_1][x] = WALL_BOTTOM;
					++x;
				}
			}

			{// Left Wall
				size_t y = y_2 + 1;
				while (y < y_1){
					Tiles[y][x_1] = WALL_Right;
					++y;
				}
			}

			{// Right Wall
				size_t y = y_2 + 1;
				while (y < y_1){
					Tiles[y][x_2] = WALL_Left;
					++y;
				}
			}
		}
		else
		{
			return false;
		}
		return true;
	}

	/************************************************************************************************/

	void DungeonGenerator::GrowMaze(size_t x, size_t y)
	{
		TileID c = {x, y};

		PushBackTileID(c);
		char LastDir = 1;
		char DirTravel = 20;
		char Dir = 1;

		while (TileStackSize)
		{
			while (CanCarveInD(c, Dir) && isTileIDInGrid(c))
			{
				if (LastDir == Dir)
					DirTravel--;
				else
					do{
						DirTravel = rand() % 15;
					} while (DirTravel < 5);


					switch (Dir)
					{
					case 0: // North
					{
						SetTile(c, Corridor );
						c[1]--;

						break;
					}
					case 1: // East
					{
						SetTile(c, Corridor );
						c[0]++;
						break;
					}
					case 2: // South
					{
						SetTile(c, Corridor );
						c[1]++;
						break;
					}
					case 3: // West
					{
						SetTile(c, Corridor );
						c[0]--;
						break;
					}
					default:
						Dir = 0;

						break;
					}

					LastDir = Dir;

					if (!DirTravel)
					{
						do{
							DirTravel = rand() % 15;
						} while (DirTravel < 5);

						while (Dir == LastDir)
						{
							Dir = (rand()) % 5;
						}
					}

					PushBackTileID(c);
			}

			bool x = true;
			for (auto I = 0; I < 4; I++)
			{
				do
				{
					Dir = (rand()) % 4;
				} while (Dir == LastDir);
				if (CanCarveInD(c, Dir))
				{
					x = false;
					break;
				}
			}
			if (x)
			{
				PopBack();
				c = GetTile();
			}
		}
	}

	/************************************************************************************************/

	int DungeonGenerator::DiagnalNeighborCount(TileID C)
	{
		auto tile = GetTile(C);

		size_t x_1 = C[0] + 1;
		size_t x_2 = C[0] - 1;
		size_t y_1 = C[1] + 1;
		size_t y_2 = C[1] - 1;

		uint2 TL = C;
		uint2 BR = C;


		if (y_2 < 0)
			return false;

		if (y_1 >= DUNGEONGRIDHEIGHT)
			return false;

		if (x_2 < 0)
			return false;

		if (x_1 >= DUNGEONGRIDWIDTH)
			return false;

		size_t c = 0;
		if (GetTile({C[0] + 1,C[1] + 1}) == Empty) c++; // TopRight
		if (GetTile({C[0] + 1,C[1] - 1}) == Empty) c++; // BottomRight
		if (GetTile({C[0] - 1,C[1] + 1}) == Empty) c++; // Topleft
		if (GetTile({C[0] - 1,C[1] - 1}) == Empty) c++; // BottomLeft

		return c;
	}

	/************************************************************************************************/

	int DungeonGenerator::NeighborCount(TileID C)
	{
		auto tile = GetTile(C);

		size_t x_1 = C[0] + 1;
		size_t x_2 = C[0] - 1;
		size_t y_1 = C[1] + 1;
		size_t y_2 = C[1] - 1;

		if (y_2 < 0)
			return false;

		if (y_1 >= DUNGEONGRIDHEIGHT)
			return false;

		if (x_2 < 0)
			return false;

		if (x_1 >= DUNGEONGRIDWIDTH)
			return false;

		int c = 0;
		if (GetTile({C[0],		C[1] - 1	}) == Empty) c++; // Top
		if (GetTile({C[0] + 1,	C[1]		}) == Empty) c++; // Right
		if (GetTile({C[0],		C[1] + 1	}) == Empty) c++; // Bottom
		if (GetTile({C[0] - 1,	C[1]		}) == Empty) c++; // Left

		return c;
	}

	/************************************************************************************************/

	bool DungeonGenerator::CanCarveInD(TileID C, int D)
	{
		// Redo this Function to Use Step Values instead
		int m = 3;

		if (!isTileIDInGrid(C))
			return false;

		switch (D)
		{
		case 0: // North
		{
			C[1]--;
			if (!isTileIDInGrid(C))
				return false;

			auto X = C[0];
			auto Y = C[1];

			return (Tiles[Y]	[X - 1] == Empty	&&
					Tiles[Y]	[X]		== Empty	&&
					Tiles[Y]	[X + 1] == Empty	&&
					Tiles[Y - 1][X - 1] == Empty	&&
					Tiles[Y - 1][X]		== Empty	&&
					Tiles[Y - 1][X + 1] == Empty);
			break;
		}
		case 1: // East
		{
			C[0]++;

			if (!isTileIDInGrid(C))
				return false;

			auto X = C[0];
			auto Y = C[1];

			return (Tiles[Y + 1][X]		== Empty	&&
					Tiles[Y]	[X]		== Empty	&&
					Tiles[Y - 1][X]		== Empty	&&
					Tiles[Y + 1][X + 1] == Empty	&&
					Tiles[Y]	[X + 1] == Empty	&&
					Tiles[Y - 1][X + 1] == Empty);
			break;
		}
		case 2: // South
		{
			C[1]--;

			if (!isTileIDInGrid(C))
				return false;

			auto X = C[0];
			auto Y = C[1];

			return (Tiles[Y]	[X - 1] == Empty	&&
					Tiles[Y]	[X]		== Empty	&&
					Tiles[Y]	[X + 1] == Empty	&&
					Tiles[Y + 1][X - 1] == Empty	&&
					Tiles[Y + 1][X]		== Empty	&&
					Tiles[Y + 1][X + 1] == Empty);
			break;
		}
		case 3: // West
		{
			C[0]--;

			if (!isTileIDInGrid(C))
				return false;

			auto Y = C[1];
			auto X = C[0];

			return (Tiles[Y + 1][X]		== Empty	&&
					Tiles[Y]	[X]		== Empty	&&
					Tiles[Y - 1][X]		== Empty	&&
					Tiles[Y + 1][X - 1]	== Empty	&&
					Tiles[Y]	[X - 1]	== Empty	&&
					Tiles[Y - 1][X - 1]	== Empty);
			break;
		}
		}
		return false;
	}

	/************************************************************************************************/

	bool DungeonGenerator::CanCarve(TileID C)
	{
		auto Y = C[1];
		auto X = C[0];

		auto tile = GetTile(C);

		if (!isTileIDInGrid(C))
			return false;

		int c = 0;
		if (Tiles[Y + 1][X]		== Empty) c++; // Top
		if (Tiles[Y - 1][X]		== Empty) c++; // Bottom
		if (Tiles[Y]	[X + 1] == Empty) c++; // Right 
		if (Tiles[Y]	[X - 1] == Empty) c++; // Left

		return (c > 1);
	}

	/************************************************************************************************/

	bool DungeonGenerator::isTileIDInGrid(TileID C)
	{
		auto Y = C[1];
		auto X = C[0];

		if ((1 >= Y) || (Y >= DUNGEONGRIDHEIGHT - 1) || (1 >= X) || (X >= DUNGEONGRIDWIDTH - 1))
			return false;

		return true;
	}
	
	/************************************************************************************************/

	void DungeonGenerator::GenerateHallways()
	{
		for (size_t y = 2; y < DUNGEONGRIDHEIGHT - 2; ++y)
		{
			for (size_t x = 2; x < DUNGEONGRIDWIDTH - 10; ++x)
			{
				if (Tiles[y][x] == Empty)
					GrowMaze(x, y);
				break;
			}
		}
	}

	/************************************************************************************************/

	void DungeonGenerator::ConnectRoom(size_t I)
	{
		int Dir = 0;
		Room r = Rooms[I];
		uint2 RoomPOS = { r.x, r.y };
		int2 Offsets[] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
		int2 A2[] = { { 0, -int(r.Height / 2) }, { int(r.Width / 2), 0 }, { 0, int(r.Height / 2) }, { -int(r.Width / 2), 0 } };

		if (GetTile(RoomPOS) != FLOOR_Dungeon) // Room not in Correct Location
			RemoveRoom(I);

		size_t side = rand() % 4;
		for (auto c = 0; c < 4; c++)
		{
			int2 Dir;

			Dir = Offsets[(side + 1) % 4];
			uint2 ScanStart = RoomPOS + A2[side];
			uint2 ScanPosition = ScanStart;

#ifdef _DEBUG
			SetTile(RoomPOS, DEBUG);
			SetTile(ScanStart, DEBUG);
#endif	

			int2 debug = int2{ int(r.Width / 2), int(r.Height / 2) };

			for (int2 i = Dir * int2{ int(r.Width / 2), int(r.Height / 2) }; i.Sum() != 0; i -= Dir)
			{
				if (GetTile(ScanPosition + Offsets[side]) == Connector)
				{
#ifdef _DEBUG
					SetTile(RoomPOS, FLOOR_Dungeon);
					SetTile(ScanStart, FLOOR_Dungeon);
#endif				
					SetTile(ScanPosition + Offsets[side], DoorWay);
					Rooms[I].Connected = true;
					return;
				}
				ScanPosition += Dir;
			}

			ScanPosition = ScanStart;

			for (int2 i = Dir * int2{ int(r.Width / 2), int(r.Height / 2) }; i.Sum() != 0; i -= Dir)
			{
				if (GetTile(ScanPosition + Offsets[side]) == Connector)
				{
#ifdef _DEBUG
					SetTile(RoomPOS, FLOOR_Dungeon);
					SetTile(ScanStart, FLOOR_Dungeon);
#endif				
					SetTile(ScanPosition + Offsets[side], DoorWay);
					Rooms[I].Connected = true;
					return;
				}
				ScanPosition -= Dir;
			}

#ifdef _DEBUG

			SetTile(RoomPOS, FLOOR_Dungeon);
			SetTile(ScanStart, FLOOR_Dungeon);
#endif				
			side = (side + 1) % 4;
		}
		RemoveRoom(I);
	}

	/************************************************************************************************/

	void DungeonGenerator::ConnectRooms()
	{
		FindConnectors();
		while (!areRoomsConnected())
		{
			auto I = FindUnconnectedRoom();
			ConnectRoom(I);
		}

		for (size_t y = 2; y < DUNGEONGRIDHEIGHT - 2; ++y)
		{// Find Connectors
			for (size_t x = 2; x < DUNGEONGRIDHEIGHT - 2; ++x)
			{
				if (Tiles[y][x] == Connector)
					Tiles[y][x] = Empty;
			}
		}
	}

	/************************************************************************************************/

	size_t DungeonGenerator::FindUnconnectedRoom()
	{
		size_t index = 0;
		for (; index < RoomCount && Rooms[index].Connected; ++index);

		return index;
	}

	/************************************************************************************************/

	bool DungeonGenerator::areRoomsConnected()
	{
		for (size_t i = 0; i < RoomCount; ++i)
			if (!Rooms[i].Connected)
				return false;
		return true;
	}

	/************************************************************************************************/

	void DungeonGenerator::FindConnectors()
	{
		ClearStack();
		for (size_t y = 2; y < DUNGEONGRIDHEIGHT - 2; ++y)
		{// Find Connectors
			for (size_t x = 2; x < DUNGEONGRIDHEIGHT - 2; ++x)
			{
				for (size_t dir = 0; dir < 4; ++dir)
				{
					uint2 POS = { x, y };
					// Connection Between Rooms and Hallways
					if ((GetTile(POS) == Empty) && (GetTile(POS + Directions[dir]) == FLOOR_Dungeon) && (GetTile(POS - Directions[dir]) == Corridor))
					{
						PushBackTileID(TileID(POS));
						SetTile(POS, Connector);
					}
					// Connection Between Rooms
					if ((GetTile(POS) == Empty) && (GetTile(POS + Directions[dir]) == FLOOR_Dungeon) && (GetTile(POS - Directions[dir]) == FLOOR_Dungeon))
					{
						PushBackTileID(TileID(POS));
						SetTile(POS, Connector);
					}
				}
			}
		}
	}

	/************************************************************************************************/

	bool DungeonGenerator::isWall(uint2 XY)
	{
		return	(GetTile(XY) == WALL_TOP)	||
				(GetTile(XY) == WALL_Right) ||
				(GetTile(XY) == WALL_BOTTOM)||
				(GetTile(XY) == WALL_Left);
	}

	/************************************************************************************************/

	bool DungeonGenerator::isWall(size_t x, size_t y)
	{
		return (
			(Tiles[y][x] == WALL_TOP)	||
			(Tiles[y][x] == WALL_Right) ||
			(Tiles[y][x] == WALL_BOTTOM)||
			(Tiles[y][x] == WALL_Left));
	}

	/************************************************************************************************/

	void DungeonGenerator::PrintDungeon()
	{	
		// Print Generation
		for (auto I = 0; I < DUNGEONGRIDHEIGHT; I++)
		{
		char temp[DUNGEONGRIDHEIGHT + 1];
		temp[DUNGEONGRIDHEIGHT] = '\0';

		for (auto I2 = 0; I2 < DUNGEONGRIDHEIGHT; I2++)
		temp[I2] = Tiles[I][I2];

		printf(temp);
		printf("\n");
		}
	}

	/************************************************************************************************/

	void DungeonGenerator::Generate()
	{
		CleanTiles();
		for (size_t I = 0; I < DUNGEONMAXRMCOUNT; ++I)
		{
			if (I > 50)
			{
				int x = 0;
			}
			auto temp = rand() % 20;
			size_t RoomWidth = temp > 8 ? temp : 8;
			temp = rand() % 20;
			size_t RoomHeight = temp > 5 ? temp : 5;

			size_t X = rand() % DUNGEONGRIDWIDTH;
			size_t Y = rand() % DUNGEONGRIDHEIGHT;
			if (AddRoom(X, Y, RoomHeight, RoomWidth, 3)) // Add Staring Room
			{
				Room R;
				R.Height = RoomHeight;
				R.Width = RoomWidth;
				R.x = X;
				R.y = Y;
				AddRoom(R);
			}
		}

		GenerateHallways();
		ConnectRooms();
	}

	/************************************************************************************************/

	void DungeonGenerator::CleanTiles()
	{
		for (size_t x = 0; x < DUNGEONGRIDHEIGHT; ++x)
			for (size_t y = 0; y < DUNGEONGRIDHEIGHT; ++y)
				Tiles[x][y] = Empty;
	}

	/************************************************************************************************/

	void DungeonGenerator::AddRoom(Room r)
	{
		r.Connected = false;
		Rooms[RoomCount++] = r;
	}

	/************************************************************************************************/

	void DungeonGenerator::BlockScanCallBack(ScanCallBack out, byte* _ptr, uint2 XY, uint2 HW)
	{
		size_t HOffset = HW[1] / 2;
		size_t WOffset = HW[0] / 2;
		size_t XStart = XY[0] - HOffset;
		size_t YStart = XY[1] - WOffset;

		Tile ScanWindow[3][3];
		for (size_t i = 0; i < WOffset; ++i)
			for (size_t i2 = 0; i2 < WOffset; ++i2)
			{
				if (isTileIDInGrid(TileID{ size_t(XStart - 1), size_t(YStart - 1) }) &&
					isTileIDInGrid(TileID{ size_t(XStart + 1), size_t(YStart + 1) }))
				{
					// Gather Tiles
					for (size_t i3 = 0; i3 < 3; ++i3)
					{
						for (size_t i4 = 0; i4 < 3; ++i4)
						{
							size_t x = XStart + i2 + i4 - 1;
							size_t y = YStart + i + i3 - 1;
							ScanWindow[i3][i4] = Tiles[y][x];
						}
					}
					out(_ptr, ScanWindow, { XStart + i2, YStart + i }, { i2, i }, this);
				}
			}
	}

	void DungeonGenerator::DungeonScanCallBack(ScanCallBack out, byte* _ptr)
	{
		size_t XStart = 1;
		size_t YStart = 1;

		Tile ScanWindow[3][3];
		for (size_t i = 1; i < DUNGEONGRIDWIDTH - 2; ++i)
			for (size_t i2 = 1; i2 < DUNGEONGRIDHEIGHT - 2; ++i2)
			{
				// Gather Tiles
				for (size_t i3 = 0; i3 < 3; ++i3)
				{
					for (size_t i4 = 0; i4 < 3; ++i4)
					{
						size_t x = XStart + i2 + i4 - 1;
						size_t y = YStart + i + i3 - 1;
						ScanWindow[i3][i4] = Tiles[y][x];
					}
				}
				out(_ptr, ScanWindow, { XStart + i2, YStart + i }, { i2, i }, this);
			}
	}

	/************************************************************************************************/

	TileID DungeonGenerator::PopBack()
	{
		return TileStack[--TileStackSize];
	}

	/************************************************************************************************/

	void DungeonGenerator::RemoveTileID(size_t i)
	{
		TileID t = TileStack[i];
		TileStack[i] = TileStack[TileStackSize - 1];
		TileStackSize--; 
	}

	/************************************************************************************************/

}