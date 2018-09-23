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

#include "LobbyGUI.h"


MultiplayerLobbyScreen::MultiplayerLobbyScreen(FlexKit::iAllocator* memory, FlexKit::SpriteFontAsset* IN_font) :
	Rows	{memory},
	gui		{{}, memory},
	font	{IN_font}
{
	PlayerScreen = &gui.CreateGrid(nullptr, {});
	PlayerScreen->SetGridDimensions({ 2, 2 });
	PlayerScreen->XY = { 0.0f, 0.0f };
	PlayerScreen->WH = { 1.0f, 1.0f };

	PlayerList = &gui.CreateGrid(PlayerScreen, {0, 0});
	PlayerList->SetGridDimensions({ 3, 9 });
	PlayerList->XY = { 0.05f, 0.05f };
	PlayerList->WH = { 0.9f, 0.59f };

	auto Temp = &gui.CreateButton(PlayerList, { 0,0 });
	Temp->Text = "Players";
	Temp->Font = font;

	auto Temp2 = &gui.CreateButton(PlayerList, { 1,0 });
	Temp2->Text = "test";
	Temp2->Font = font;

	Temp = &gui.CreateButton(PlayerList, { 2,0 });
	Temp->Text = "Ready";
	Temp->Font = font;
}


void MultiplayerLobbyScreen::Update(double dt, FlexKit::WindowInput& input, float2 pixelSize, FlexKit::iAllocator* allocator)
{
	gui.Update(dt, input, pixelSize, allocator);
}


void MultiplayerLobbyScreen::Draw(
	LobbyScreenDrawDesc&		desc, 
	FlexKit::UpdateDispatcher&	Dispatcher, 
	FlexKit::FrameGraph&		frameGraph)
{
	FlexKit::DrawUI_Desc drawDesc =
	{
		&frameGraph,
		desc.renderTarget,
		desc.vertexBuffer,
		desc.textBuffer,
		desc.constantBuffer
	};

	gui.Draw(drawDesc, desc.allocator);
}


/************************************************************************************************/


void MultiplayerLobbyScreen::CreateRow(LobbyRowID id)
{ 
	Row newRow;
	newRow.ID		= id;
	newRow.Name		= &gui.CreateButton(PlayerList, { 0, 1 + Rows.size() });
	newRow.Ready	= &gui.CreateButton(PlayerList, { 2, 1 + Rows.size() });

	newRow.Name->Text   = "...";
	newRow.Ready->Text  = "...";

	newRow.Name->Font	= font;
	newRow.Ready->Font	= font;

	Rows.push_back(newRow);
}


/************************************************************************************************/


void MultiplayerLobbyScreen::SetPlayerName(LobbyRowID id, char*	name)
{
	for (auto& row : Rows)
	{
		if (row.ID == id)
		{
			row.Name->Text = name;
			return;
		}
	}
}


/************************************************************************************************/


void MultiplayerLobbyScreen::SetPlayerReady(LobbyRowID id, bool	ready)
{
	for (auto& row : Rows)
	{
		if (row.ID == id)
		{
			if (ready)
				row.Ready->Text = "Ready";
			else
				row.Ready->Text = "Not Ready";
			return;
		}
	}
}