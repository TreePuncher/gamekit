/**********************************************************************

Copyright (c) 2018 Robert May

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

#include "Gameplay.h"
#include "..\coreutilities\containers.h"
#include <algorithm>


using namespace FlexKit;

/************************************************************************************************/


Player_Handle Game::CreatePlayer(GridID_t CellID)
{
	Players.push_back(GridPlayer());
	Players.back().XY = CellID;

	MarkCell(CellID, EState::Player);

	return Players.size() - 1;
}


/************************************************************************************************/


GridObject_Handle Game::CreateGridObject(GridID_t CellID)
{
	Objects.push_back(GridObject());

	Objects.back().XY = CellID;
	MarkCell(CellID, EState::Object);

	return Objects.size() - 1;
}


/************************************************************************************************/


GridSpace_Handle Game::CreateGridSpace(GridID_t CellID)
{
	Spaces.push_back(GridSpace());

	Spaces.back().XY = CellID;
	MarkCell(CellID, EState::Destroyed);

	return Spaces.size() - 1;

}


/************************************************************************************************/


void Game::CreateBomb(
	EBombType Type, 
	GridID_t CellID, 
	BombID_t ID, 
	Player_Handle PlayerID)
{
	if (!Players[PlayerID].DecrementBombSlot(Type))
		return;

	Bombs.push_back({ 
		CellID, 
		EBombType::Regular, 
		0, 
		ID,
		{    0,   0		},
		{ 0.0f, 0.0f	}});

	auto* Task = &Memory->allocate_aligned<RegularBombTask>(
		ID, 
		this);

	Tasks.push_back(Task);
}


/************************************************************************************************/


bool Game::MovePlayer(Player_Handle Player, GridID_t GridID)
{
	if (Players[Player].State != GridPlayer::PS_Idle)
		return false;

	if (!IsCellClear(GridID))
		return false;

	auto OldPOS = Players[Player].XY;

	auto* Task = &Memory->allocate_aligned<MovePlayerTask>(
		OldPOS,
		GridID,
		Player,
		0.25f,
		this);

	Tasks.push_back(Task);

	return true;
}


/************************************************************************************************/


bool Game::MoveBomb(GridID_t GridID, int2 Direction)
{
	if (GetCellState(GridID) != EState::Bomb)
		return false;

	auto res = FlexKit::find(Bombs, [GridID](auto& entry) {
		return entry.XY == GridID; });

	if (!res)
		return false;

	res->Direction = Direction;

	return true;
}


/************************************************************************************************/


bool Game::IsCellClear(GridID_t GridID)
{
	if (!IDInGrid(GridID))
		return false;

	size_t Idx = GridID2Index(GridID);

	return (Grid[Idx] == EState::Empty);
}


/************************************************************************************************/


bool Game::IsCellDestroyed(GridID_t GridID)
{
	if (!IDInGrid(GridID))
		return false;

	size_t Idx = GridID2Index(GridID);

	return (Grid[Idx] == EState::Object);
}


/************************************************************************************************/


void Game::Update(const double dt, iAllocator* TempMemory)
{
	std::stable_sort(
		Tasks.begin(), 
		Tasks.end(), 
		[](auto& lhs, auto& rhs) -> bool
		{
			return lhs->UpdatePriority > rhs->UpdatePriority;
		});


	for (auto& Task : Tasks)
	{
		Task->Update(dt);

		if (Task->Complete())
		{
			(Task)->~iGameTask();
			Memory->free(Task);
			Task = nullptr;
		}
	}

	Tasks.erase(
		std::remove(Tasks.begin(), Tasks.end(), nullptr), 
		Tasks.end());
}


/************************************************************************************************/


bool Game::MarkCell(GridID_t CellID, EState State)
{
	size_t Idx = GridID2Index(CellID);

	//if (!IsCellClear(CellID))
	//	return false;

	Grid[Idx] = State;

	return true;
}


/************************************************************************************************/

// Certain State will be lost
void Game::Resize(uint2 wh)
{
	WH = wh;
	Grid.resize(WH.Product());

	for (auto& Cell : Grid)
		Cell = EState::Empty;

	for (auto Obj : Objects)
		MarkCell(Obj.XY, EState::Object);

	for (auto Player : Players)
		MarkCell(Player.XY, EState::Player);
}


/************************************************************************************************/


bool Game::GetBomb(BombID_t ID, GridBomb& Out)
{
	auto Res = find(Bombs, 
		[this, ID](auto res) 
		{ 
			return (res.ID == ID);
		});

	Out = *(GridBomb*)Res;

	return (bool)Res;
}


/************************************************************************************************/


bool Game::SetBomb(BombID_t ID, const GridBomb& In)
{
	auto Res = find(Bombs,
		[this, ID](auto res)
	{
		return (res.ID == ID);
	});

	*(GridBomb*)Res = In;

	return (bool)Res;
}


/************************************************************************************************/


EState Game::GetCellState(GridID_t CellID)
{
	size_t Idx = WH[0] * CellID[1] + CellID[0];

	return Grid[Idx];
}


/************************************************************************************************/


bool Game::RemoveBomb(BombID_t ID) 
{
	auto Res = find(Bombs,
		[this, ID](auto res)
	{
		return (res.ID == ID);
	});

	if (Res)
		Bombs.remove_unstable(Res);

	return Res;
}



/************************************************************************************************/


void MovePlayerTask::Update(const double dt)
{
	T += dt / D;

	if (T < (1.0f - 1.0f/60.f))
	{
		int2 temp	= B - A;
		float2 C	= { 
			(float)temp[0], 
			(float)temp[1] };

		auto State = Grid->GetCellState(B);
		if (State != EState::InUse)
		{
			Grid->MarkCell(A, EState::Player);

			Grid->Players[Player].State		= GridPlayer::PS_Idle;
			Grid->Players[Player].XY		= A;
			Grid->Players[Player].Offset	= float2{ 0.f, 0.f };
			complete = true;
		}
		else
			Grid->Players[Player].Offset = float2{ C * T };
	}
	else
	{
		Grid->MarkCell(A, EState::Empty);
		Grid->MarkCell(B, EState::Player);

		Grid->Players[Player].Offset = float2{ 0.f, 0.f };
		Grid->Players[Player].XY	 = B;
		Grid->Players[Player].State	 = GridPlayer::PS_Idle;
		complete = true;
	}
}


/************************************************************************************************/


void RegularBombTask::Update(const double dt)
{
	T += dt;

	GridBomb BombEntry;
	if (!Grid->GetBomb(Bomb, BombEntry))
		return;

	auto Direction = BombEntry.Direction;

 	if ((Direction * Direction).Sum() > 0)
	{
		T2 += dt * 4;
		auto A = BombEntry.XY;
		auto B = BombEntry.XY + BombEntry.Direction;

		if (Grid->GetCellState(B) != EState::Empty)
		{
			T = 2.0f;
		}
		else if (T2 < (1.0f - 1.0f / 60.f))
		{

			int2 temp = B - A;
			float2 C{
				(float)temp[0],
				(float)temp[1] };

			BombEntry.Offset = { C * T2 };
		}
		else
		{
   			Grid->MarkCell(A, EState::Empty);
			Grid->MarkCell(B, EState::Bomb);

			T2					= 0.0f;
			BombEntry.Offset	= { 0.f, 0.f };
			BombEntry.XY		= B;
		}

		Grid->SetBomb(Bomb, BombEntry);
	}

	if (T >= 2.0f)
	{
		Completed = true;

		Grid->RemoveBomb(Bomb);

		Grid->MarkCell(BombEntry.XY + int2{ -1, -1 }, EState::Destroyed);
		Grid->MarkCell(BombEntry.XY + int2{  0, -1 }, EState::Destroyed);
		Grid->MarkCell(BombEntry.XY + int2{  1, -1 }, EState::Destroyed);

		Grid->MarkCell(BombEntry.XY + int2{ -1,  0 }, EState::Destroyed);
		Grid->MarkCell(BombEntry.XY + int2{  0,  0 }, EState::Destroyed);
		Grid->MarkCell(BombEntry.XY + int2{  1,  0 }, EState::Destroyed);

  		Grid->MarkCell(BombEntry.XY + int2{ -1,  1 }, EState::Destroyed);
		Grid->MarkCell(BombEntry.XY + int2{  0,  1 }, EState::Destroyed);
		Grid->MarkCell(BombEntry.XY + int2{  1,  1 }, EState::Destroyed);

		Grid->CreateGridSpace(BombEntry.XY + int2{ -1, -1 });
		Grid->CreateGridSpace(BombEntry.XY + int2{ -0, -1 });
		Grid->CreateGridSpace(BombEntry.XY + int2{  1, -1 });

		Grid->CreateGridSpace(BombEntry.XY + int2{ -1, -0 });
		Grid->CreateGridSpace(BombEntry.XY + int2{ -0, -0 });
		Grid->CreateGridSpace(BombEntry.XY + int2{  1, -0 });

		Grid->CreateGridSpace(BombEntry.XY + int2{ -1,  1 });
		Grid->CreateGridSpace(BombEntry.XY + int2{  0,  1 });
		Grid->CreateGridSpace(BombEntry.XY + int2{  1,  1 });
	}
}


/************************************************************************************************/


void DrawGameGrid_Debug(
	double					dt,
	float					AspectRatio,
	Game&				Grid,
	FrameGraph&				FrameGraph,
	ConstantBufferHandle	ConstantBuffer,
	VertexBufferHandle		VertexBuffer,
	TextureHandle			RenderTarget,
	iAllocator*				TempMem
	)
{
	const size_t ColumnCount	= Grid.WH[1];
	const size_t RowCount		= Grid.WH[0];

	FlexKit::LineSegments Lines(TempMem);
	Lines.reserve(ColumnCount + RowCount);

	const auto RStep = 1.0f / RowCount;

	for (size_t I = 1; I < RowCount; ++I)
		Lines.push_back(
			{	{0, RStep  * I,1}, 
				{1.0f, 1.0f, 1.0f}, 
				{ 1, RStep  * I, 1, 1 }, 
				{1, 1, 1, 1} });


	const auto CStep = 1.0f / ColumnCount;
	for (size_t I = 1; I < ColumnCount; ++I)
		Lines.push_back(
			{	{ CStep  * I, 0, 0 },
				{ 1.0f, 1.0f, 1.0f },
				{ CStep  * I, 1, 0 },
				{ 1, 1, 1, 1 } });


	DrawShapes(FlexKit::DRAW_LINE_PSO, 
		FrameGraph, 
		VertexBuffer, 
		ConstantBuffer, 
		RenderTarget, 
		TempMem,
		FlexKit::LineShape(Lines));


	for (auto Player : Grid.Players)
		DrawShapes(FlexKit::DRAW_PSO, 
			FrameGraph, 
			VertexBuffer, 
			ConstantBuffer, 
			RenderTarget, 
			TempMem,
			FlexKit::CircleShape(
				float2{	
					CStep / 2 + Player.XY[0] * CStep + Player.Offset.x * CStep,
					RStep / 2 + Player.XY[1] * RStep + Player.Offset.y * RStep },
				min(
					(CStep / 2.0f) / AspectRatio,
					(RStep / 2.0f)),
				float4{1.0f, 1.0f, 1.0f, 1.0f}, AspectRatio));


	for (auto Bombs : Grid.Bombs)
		DrawShapes(FlexKit::DRAW_PSO, 
			FrameGraph, 
			VertexBuffer, 
			ConstantBuffer, 
			RenderTarget, 
			TempMem,
			FlexKit::CircleShape(
				float2{
					CStep / 2 + Bombs.XY[0] * CStep + Bombs.Offset.x * CStep,
					RStep / 2 + Bombs.XY[1] * RStep + Bombs.Offset.y * RStep },
					min(
						(CStep / 2.0f) / AspectRatio,
						(RStep / 2.0f)),
				float4{ 0.0f, 1.0f, 0.0f, 0.0f }, AspectRatio, 4));


	for (auto Object : Grid.Objects)
		DrawShapes(FlexKit::DRAW_PSO, 
			FrameGraph, 
			VertexBuffer, 
			ConstantBuffer, 
			RenderTarget, 
			TempMem,
			FlexKit::RectangleShape(float2{
				Object.XY[0] * CStep, 
				Object.XY[1] * RStep }, 
				{ CStep , RStep },
				{0.5f, 0.5f, 0.5f, 1.0f}));


	for (auto Object : Grid.Spaces)
		DrawShapes(FlexKit::DRAW_PSO, 
			FrameGraph, 
			VertexBuffer, 
			ConstantBuffer, 
			RenderTarget, 
			TempMem,
			FlexKit::RectangleShape(float2{
				Object.XY[0] * CStep,
				Object.XY[1] * RStep },
				{ CStep , RStep },
				{ 0.5f, 0.0f, 0.0f, 1.0f }));
}


/************************************************************************************************/


void Draw3DGrid(
	FrameGraph&				FrameGraph,
	const size_t			ColumnCount,
	const size_t			RowCount,
	const float2			GridWH,
	const float4			GridColor,
	TextureHandle			RenderTarget,
	TextureHandle			DepthBuffer,
	VertexBufferHandle		VertexBuffer,
	ConstantBufferHandle	Constants,
	CameraHandle			Camera,
	iAllocator*				TempMem)
{
	FlexKit::LineSegments Lines(TempMem);
	Lines.reserve(ColumnCount + RowCount);

	const auto RStep = 1.0f / RowCount;

	//  Vertical Lines on ground
	for (size_t I = 1; I < RowCount; ++I)
		Lines.push_back(
			{ { RStep  * I * GridWH.x, 0, 0 },
			GridColor,
			{ RStep  * I * GridWH.x, 0, GridWH.y },
			GridColor });


	// Horizontal lines on ground
	const auto CStep = 1.0f / ColumnCount;
	for (size_t I = 1; I < ColumnCount; ++I)
		Lines.push_back(
			{ { 0,			0, CStep  * I * GridWH.y },
			GridColor,
			{ GridWH.x,		0, CStep  * I * GridWH.y },
			GridColor });


	struct DrawGrid
	{
		FlexKit::FrameResourceHandle		RenderTarget;
		FlexKit::FrameResourceHandle		DepthBuffer;

		size_t							VertexBufferOffset;
		size_t							VertexCount;
		FlexKit::VertexBufferHandle		VertexBuffer;
		FlexKit::ConstantBufferHandle	CB;

		size_t CameraConstantsOffset;
		size_t LocalConstantsOffset;

		FlexKit::DesciptorHeap	Heap; // Null Filled
	};


	struct VertexLayout
	{
		float4 POS;
		float4 Color;
		float2 UV;
	};

	FrameGraph.AddNode<DrawGrid>(0,
		[&](FlexKit::FrameGraphNodeBuilder& Builder, auto& Data)
	{
		Data.RenderTarget	= Builder.WriteRenderTarget(FrameGraph.Resources.RenderSystem->GetTag(RenderTarget));
		Data.DepthBuffer	= Builder.WriteDepthBuffer(FrameGraph.Resources.RenderSystem->GetTag(DepthBuffer));
		Data.CB				= Constants;

		Data.CameraConstantsOffset = BeginNewConstantBuffer(Constants, FrameGraph.Resources);
		PushConstantBufferData(
			GetCameraConstantBuffer(Camera),
			Constants,
			FrameGraph.Resources);

		Data.Heap.Init(
			FrameGraph.Resources.RenderSystem,
			FrameGraph.Resources.RenderSystem->Library.RS4CBVs4SRVs.GetDescHeap(0),
			TempMem);
		Data.Heap.NullFill(FrameGraph.Resources.RenderSystem);

		Drawable::VConsantsLayout DrawableConstants;
		DrawableConstants.Transform = DirectX::XMMatrixIdentity();

		Data.LocalConstantsOffset = BeginNewConstantBuffer(Constants, FrameGraph.Resources);
		PushConstantBufferData(
			DrawableConstants,
			Constants,
			FrameGraph.Resources);

		Data.VertexBuffer = VertexBuffer;
		Data.VertexBufferOffset = FrameGraph.Resources.GetVertexBufferOffset(VertexBuffer);

		// Fill Vertex Buffer Section
		for (auto& LineSegment : Lines)
		{
			VertexLayout Vertex;
			Vertex.POS = float4(LineSegment.A, 1);
			Vertex.Color = float4(LineSegment.AColour, 1) * float4 { 1.0f, 0.0f, 0.0f, 1.0f };
			Vertex.UV = { 0.0f, 0.0f };

			PushVertex(Vertex, VertexBuffer, FrameGraph.Resources);

			Vertex.POS = float4(LineSegment.B, 1);
			Vertex.Color = float4(LineSegment.BColour, 1) * float4 { 0.0f, 1.0f, 0.0f, 1.0f };
			Vertex.UV = { 1.0f, 1.0f };

			PushVertex(Vertex, VertexBuffer, FrameGraph.Resources);
		}

		Data.VertexCount = Lines.size() * 2;

	},
		[](auto& Data, const FlexKit::FrameResources& Resources, FlexKit::Context* Ctx)
	{
		Ctx->SetRootSignature(Resources.RenderSystem->Library.RS4CBVs4SRVs);
		Ctx->SetPipelineState(Resources.GetPipelineState(DRAW_LINE3D_PSO));

		Ctx->SetScissorAndViewports({ Resources.GetRenderTarget(Data.RenderTarget) });
		Ctx->SetRenderTargets(
			{	(DescHeapPOS)Resources.GetRenderTargetObject(Data.RenderTarget) }, false,
				(DescHeapPOS)Resources.GetRenderTargetObject(Data.DepthBuffer));

		Ctx->SetPrimitiveTopology(EInputTopology::EIT_LINE);
		Ctx->SetVertexBuffers(VertexBufferList{ { Data.VertexBuffer, sizeof(VertexLayout), (UINT)Data.VertexBufferOffset } });

		Ctx->SetGraphicsDescriptorTable(0, Data.Heap);
		Ctx->SetGraphicsConstantBufferView(1, Data.CB, Data.CameraConstantsOffset);
		Ctx->SetGraphicsConstantBufferView(2, Data.CB, Data.LocalConstantsOffset);

		Ctx->Draw(Data.VertexCount, 0);
	});
}


/************************************************************************************************/


void DrawGame(
	double							dt,
	float							AspectRatio,
	Game&							Grid,
	FlexKit::FrameGraph&			FrameGraph,
	FlexKit::ConstantBufferHandle	ConstantBuffer,
	FlexKit::VertexBufferHandle		VertexBuffer,
	FlexKit::TextureHandle			RenderTarget,
	FlexKit::TextureHandle			DepthBuffer,
	FlexKit::CameraHandle			Camera,
	FlexKit::TriMeshHandle			PlayerModel,
	FlexKit::iAllocator*			TempMem)
{
	const size_t ColumnCount = Grid.WH[1];
	const size_t RowCount 	 = Grid.WH[0];
	const float2 GridWH		 = { 50.0, 50.0 };
	const float4 GridColor	 = { 1, 1, 1, 1 };


	struct InstanceConstantLayout
	{
		FlexKit::float4x4 WorldTransform;
	};

	FlexKit::Drawable::VConsantsLayout Constants_1 =
	{
		FlexKit::Drawable::MaterialProperties(),
		DirectX::XMMatrixIdentity()
	};

	auto CameraConstants = GetCameraConstantBuffer(Camera);

	FlexKit::DrawCollection_Desc DrawDesc =
	{
		PlayerModel,
		RenderTarget,
		DepthBuffer,
		VertexBuffer,
		ConstantBuffer,

		FORWARDDRAWINSTANCED,

		sizeof(InstanceConstantLayout),

		{
			sizeof(CameraConstants),
			sizeof(Drawable::VConsantsLayout)
		},

		{
			(char*)&CameraConstants,
			(char*)&Constants_1
		}
	};


	Draw3DGrid(
		FrameGraph,
		ColumnCount,
		RowCount,
		GridWH,
		GridColor,
		RenderTarget,
		DepthBuffer,
		VertexBuffer,
		ConstantBuffer,
		Camera,
		TempMem);

	DrawCollection(
		FrameGraph,
		Grid.Players,
		[&](auto& Player) -> InstanceConstantLayout
		{
			float3 Position = {
				(Player.XY[0] + Player.Offset.x) * GridWH.x / ColumnCount,
				0.0,
				(Player.XY[1] + Player.Offset.y) * GridWH.y / RowCount };

			auto Transform = TranslationMatrix(Position);

			return  { Transform };
		},
		DrawDesc,
		TempMem
		);

	DrawCollection(
		FrameGraph,
		Grid.Objects,
		[&](auto& Entity) -> InstanceConstantLayout
		{
			float3 Position = {
				(Entity.XY[0]) * GridWH.x / ColumnCount,
				0.0,
				(Entity.XY[1]) * GridWH.y / RowCount };

			auto Transform = TranslationMatrix(Position);

			return  { Transform };
		},
		DrawDesc,
		TempMem
	);

	DrawCollection(
		FrameGraph,
		Grid.Spaces,
		[&](auto& Entity) -> InstanceConstantLayout
		{
			float3 Position = {
				(Entity.XY[0]) * GridWH.x / ColumnCount,
				0.0,
				(Entity.XY[1]) * GridWH.y / RowCount };

			return  { TranslationMatrix(Position) };
		},
		DrawDesc,
		TempMem
	);

	DrawCollection(
		FrameGraph,
		Grid.Bombs,
		[&](auto& Entity) -> InstanceConstantLayout
		{
			float3 Position = {
				(Entity.XY[0] + Entity.Offset.x) * GridWH.x / ColumnCount,
				0.0,
				(Entity.XY[1] + Entity.Offset.y) * GridWH.y / RowCount };

			return  { TranslationMatrix(Position) };
		},
		DrawDesc,
		TempMem
	);
}


/************************************************************************************************/


FrameSnapshot::FrameSnapshot(Game* IN_Game, EventList* IN_FrameEvents, size_t IN_FrameID, iAllocator* IN_Memory) :
	FrameID		{ IN_FrameID },
	FrameCopy	{ IN_Memory  },
	Memory		{ IN_Memory  },
	FrameEvents	{ IN_Memory  }
{
	if (!IN_Memory)
		return;

	FrameEvents = *IN_FrameEvents;
	FrameCopy	= *IN_Game;
}


/************************************************************************************************/


FrameSnapshot::~FrameSnapshot()
{
	FrameCopy.Release();
}


/************************************************************************************************/


void FrameSnapshot::Restore(Game* out)
{
}


/************************************************************************************************/