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


/************************************************************************************************/


MultiplayerLobbyScreen::MultiplayerLobbyScreen(FlexKit::iAllocator* memory, FlexKit::SpriteFontAsset* IN_font) :
	rows	{memory},
	gui		{{}, memory},
	font	{IN_font}
{
	playerScreen = &gui.CreateGrid(nullptr, {});
	playerScreen->SetGridDimensions({ 2, 2 });
	playerScreen->XY = { 0.0f, 0.0f };
	playerScreen->WH = { 1.0f, 1.0f };

	playerList = &gui.CreateGrid(playerScreen, {0, 0});
	playerList->SetGridDimensions({ 3, 9 });
	playerList->XY = { 0.05f, 0.05f };
	playerList->WH = { 0.9f, 0.59f };

	auto Temp = &gui.CreateButton(playerList, { 0,0 });
	Temp->Text = "Players";
	Temp->Font = font;

	auto Temp2 = &gui.CreateButton(playerList, { 1,0 });
	Temp2->Text = "ping";
	Temp2->Font = font;

	Temp = &gui.CreateButton(playerList, { 2,0 });
	Temp->Text = "Ready";
	Temp->Font = font;
}


/************************************************************************************************/


void MultiplayerLobbyScreen::Update(double dt, FlexKit::WindowInput& input, float2 pixelSize, FlexKit::iAllocator* allocator)
{
	gui.Update(dt, input, pixelSize, allocator);
}


/************************************************************************************************/


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
		desc.constantBuffer
	};

	gui.Draw(drawDesc, desc.allocator);
}


/************************************************************************************************/



void MultiplayerLobbyScreen::ClearRows()
{
	for (auto& row : rows)
	{
		playerList->RemoveChild(row.name);
		playerList->RemoveChild(row.ready);

		gui.Release(row.name);
		gui.Release(row.ready);
	}

	rows.clear();
}


/************************************************************************************************/



void MultiplayerLobbyScreen::CreateRow(LobbyRowID id)
{ 
	Row newRow;
	newRow.id		= id;
	newRow.name		= &gui.CreateButton(playerList, { 0, 1 + (uint32_t)rows.size() });
    newRow.ping     = &gui.CreateButton(playerList, { 1, 1 + (uint32_t)rows.size() });
	newRow.ready	= &gui.CreateButton(playerList, { 2, 1 + (uint32_t)rows.size() });

	newRow.name->Text   = "Joining...";
    newRow.ready->Text  = "...";
    newRow.ping->Text   = nullptr;

	newRow.name->Font	= font;
	newRow.ping->Font   = font;
	newRow.ready->Font	= font;

	rows.push_back(newRow);
}


/************************************************************************************************/


void MultiplayerLobbyScreen::SetPlayerName(LobbyRowID id, const char*	name)
{
	for (auto& row : rows)
	{
		if (row.id == id)
		{
			row.name->Text = name;
			return;
		}
	}
}


/************************************************************************************************/


void MultiplayerLobbyScreen::SetPlayerReady(LobbyRowID id, bool	ready)
{
	for (auto& row : rows)
	{
		if (row.id == id)
		{
			if (ready)
				row.ready->Text = "Ready";
			else
				row.ready->Text = "Not Ready";
			return;
		}
	}
}


/************************************************************************************************/


void MultiplayerLobbyScreen::SetPlayerPing(LobbyRowID id, int ping, iAllocator* allocator)
{
    for (auto& row : rows)
    {
        if (row.id == id)
        {
            if (row.ping->Text != zero)
                allocator->free((void*)row.ping->Text);

            char* text = (char*)allocator->malloc(32);
            row.ping->Text = _itoa(ping, text, 10);

            return;
        }
    }
}


/************************************************************************************************/
