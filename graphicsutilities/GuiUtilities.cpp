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

#include "GuiUtilities.h"
#include "graphics.h"
#include "FrameGraph.h"

namespace FlexKit
{
	/************************************************************************************************/


	bool CursorInside(float2 CursorPos, float2 CursorWH, float2 POS, float2 WH)
	{
		auto ElementCord = CursorPos - POS;
		auto T = WH - ElementCord;

#if 1
		std::cout << "Checking Cursor Inside: \n CursorPos:";
		printfloat2(CursorPos);
		std::cout << "\n CursorWH:";
		printfloat2(CursorWH);
		std::cout << "\n Element TL POS:";
		printfloat2(POS);
		std::cout << "\n Element TL WH:";
		printfloat2(WH);
		std::cout << "\n ";
#endif

		return (ElementCord.x > 0.0f && ElementCord.y > 0.0f) && (T.x > 0.0f && T.y > 0.0f);
	}


	/************************************************************************************************/
	

	struct FormattingState
	{
		float2	CurrentPosition;
		float2	CurrentOffset;
		float2	End;

		float RowEnd;
		float ColumnBegin;
	};


	FormattingState InitiateFormattingState(FormattingOptions& Formatting, ParentInfo& P)
	{
		FormattingState	Out;
		Out.CurrentPosition = P.ClipArea_BL + float2{ 0.0f, P.ClipArea_WH[1] } -Formatting.CellDividerSize * float2(0.0f, 1.0f);
		Out.CurrentOffset	= { 0.0f, 0.0f };
		Out.RowEnd			= P.ClipArea_TR[0];
		Out.ColumnBegin		= P.ClipArea_TR[1];
		Out.End				= { Out.RowEnd, P.ClipArea_BL[1] };

		return Out;
	}

	bool WillFit(float2 ElementBLeft, float2 ElementTRight, float2 AreaBLeft, float2 AreaTRight)
	{
		return (
			ElementBLeft.x >=  AreaBLeft.x  &&
			ElementBLeft.y >=  AreaBLeft.y  &&
			ElementTRight.x <=  AreaTRight.x &&
			ElementTRight.y <=  AreaTRight.y);
	}

	bool IncrementFormatingState(FormattingState& State, FormattingOptions& Options, float2 WH)
	{
		switch (Options.DrawDirection)
		{
		case DD_Rows:
			State.CurrentOffset += (WH * float2(1.0f, 0.0f));
			break;
		case DD_Columns:
			State.CurrentOffset += (WH * float2(0.0f,-1.0f));
			break;
		case DD_Grid: {
			auto temp = State.CurrentPosition += (WH * float2(0.0f, 1.0f));

			if (temp.x > State.End.x)
			{
				State.CurrentOffset.x = State.ColumnBegin;
				State.CurrentOffset.y += WH[1];
			}
			if (temp.y > State.End.y)
				return true;

		}	break;
		default:
			break;
		}
		return false;
	}

	float2 GetFormattedPosition(FormattingState& State, FormattingOptions& Options, float2 WH)
	{
		float2 Position = State.CurrentPosition + State.CurrentOffset;
		Position[1] -= WH[1];

		return Position;
	}


	/************************************************************************************************/


	ComplexGUI::ComplexGUI(iAllocator* memory) : 
		Buttons		(memory),
		Children	(memory), 
		Grids		(memory), 
		Elements	(memory),
		TextBoxes	(memory),
		Memory		(memory)
	{
		int x = 0;
	}

	ComplexGUI::ComplexGUI(const ComplexGUI& RHS)
	{
		Children = RHS.Children;
		Elements = RHS.Elements;
		Grids    = RHS.Grids;
		Memory   = RHS.Memory;
	}



	/************************************************************************************************/


	void ComplexGUI::Release()
	{
		//Buttons.Release();
		//Children.Release();
		Grids.Release();
		//Elements.Release();
	}


	/************************************************************************************************/


	void ComplexGUI::Update(double dt, const WindowInput Input, float2 PixelSize, iAllocator* TempMemory)
	{
		/*
		LayoutEngine Layout(TempMemory, Memory, nullptr, PixelSize);

		for (size_t I = 0; I < Elements.size(); ++I) {
			switch (Elements[I].Type)
			{
			case EGUI_ELEMENT_TYPE::EGE_GRID: {
				GUIGrid::Update(GUIGridHandle(this, I), &Layout, dt, Input);
			}	break;
			default:
				break;
			}
		}
		*/
	}


	/************************************************************************************************/


	void ComplexGUI::Draw(DrawUI_Desc& Desc, iAllocator* Temp)
	{
		/*
			struct LayoutEngine_Desc
			{
			FrameGraph*			FrameGraph;
			TextureHandle		RenderTarget;
			VertexBufferHandle	VertexBuffer;
			VertexBufferHandle	TextBuffer;
			float2				PixelSize;
			};
		*/

		auto WH			= Desc.FrameGraph->Resources.RenderSystem->GetRenderTargetWH(Desc.RenderTarget);
		auto PixelSize	= float2{ 1.0f / WH[0], 1.0f / WH[1] };

		LayoutEngine_Desc LE_Desc = 
		{
			Desc.FrameGraph, 
			Desc.ConstantBuffer,
			Desc.RenderTarget,
			Desc.VertexBuffer,
			Desc.TextBuffer,
			PixelSize
		};

		
		LayoutEngine Layout(Temp, Memory, LE_Desc);

		for (size_t I = 0; I < Elements.size(); ++I) {
			switch (Elements[I].Type)
			{
				case EGUI_ELEMENT_TYPE::EGE_GRID:{
					GUIGrid::Draw(GUIGridHandle(this, I), &Layout);
				}	break;
			default:
				break;
			}
		}
		
	}

	
	/************************************************************************************************/


	void ComplexGUI::UpdateElement(GUIElementHandle Element, LayoutEngine* Layout, double dt, const	WindowInput Input)
	{
		if (Elements[Element].Active) {
			switch (Elements[Element].Type)
			{
			case EGUI_ELEMENT_TYPE::EGE_GRID: {
				GUIGrid::Update(GUIGridHandle(this, Element), Layout, dt, Input);
			}	break;
			case EGUI_ELEMENT_TYPE::EGE_BUTTON_TEXT: {
				GUIButton::Update(GUIButtonHandle(this, Element), Layout, dt, Input);
			}	break;
			case EGUI_ELEMENT_TYPE::EGE_TEXTBOX: {
				GUITextBox::Update(GUITextBoxHandle(this, Element), Layout, dt, Input);
			}	break;
			default:
				break;
			}
		}
	}


	/************************************************************************************************/


	void ComplexGUI::DrawElement(GUIElementHandle Element, LayoutEngine* Layout)
	{
		if (Elements[Element].Active) {
			switch (Elements[Element].Type)
			{
			case EGUI_ELEMENT_TYPE::EGE_GRID: {
				GUIGrid::Draw(GUIGridHandle(this, Element), Layout);
			}	break;
			case EGUI_ELEMENT_TYPE::EGE_BUTTON_TEXT: {
				GUIButton::Draw(GUIButtonHandle(this, Element), Layout);
			}	break;
			case EGUI_ELEMENT_TYPE::EGE_TEXTBOX: {
				GUITextBox::Draw(GUITextBoxHandle(this, Element), Layout);
			}	break;
			default:
				break;
			}
		}
	}


	/************************************************************************************************/


	void ComplexGUI::DrawElement_DEBUG(GUIElementHandle Element, LayoutEngine* Layout)
	{
		if (Elements[Element].Active) {
			switch (Elements[Element].Type)
			{
			case EGUI_ELEMENT_TYPE::EGE_GRID: {
				GUIGrid::Draw_DEBUG(GUIGridHandle(this, Element), Layout);
			}	break;
			case EGUI_ELEMENT_TYPE::EGE_BUTTON_TEXT: {
				GUIButton::Draw_DEBUG(GUIButtonHandle(this, Element), Layout);
			}	break;
			case EGUI_ELEMENT_TYPE::EGE_TEXTBOX: {
				GUITextBox::Draw_DEBUG(GUITextBoxHandle(this, Element), Layout);
			}	break;
			default:
				break;
			}
		}
	}


	/************************************************************************************************/
	
	void ComplexGUI::Draw_DEBUG(DrawUI_Desc& Desc, iAllocator* Temp)
	{
		auto WH			= Desc.FrameGraph->Resources.RenderSystem->GetRenderTargetWH(Desc.RenderTarget);
		auto PixelSize	= float2{ 1.0f / WH[0], 1.0f / WH[1] };

		LayoutEngine_Desc LE_Desc =
		{
			Desc.FrameGraph,
			Desc.ConstantBuffer,
			Desc.RenderTarget,
			Desc.VertexBuffer,
			Desc.TextBuffer,
			PixelSize
		};

		LayoutEngine Layout(Temp, Memory, LE_Desc);

		for(size_t I = 0; I < Elements.size(); ++I)
		{
			if (Elements[I].UpdatePriority == 0) 
				DrawElement_DEBUG(I, &Layout);
		}
	}


	/************************************************************************************************/


	GUIGridHandle ComplexGUI::CreateGrid(GUIElementHandle Parent, uint2 ID)
	{
		GUIElementHandle BaseIndex = Elements.size();

		Elements.push_back({
			Grids.size(), 
			Parent + 1, true, 
			EGUI_ELEMENT_TYPE::EGE_GRID });

		Grids.push_back(GUIGrid(Memory, BaseIndex));

		return{ this, BaseIndex };
	}


	/************************************************************************************************/


	GUIGridHandle ComplexGUI::CreateGrid(uint2 ID, uint2 CellCounts)
	{
		GUIElementHandle BaseIndex = Elements.size();

		Elements.push_back({ 
			Grids.size(), 
			0x00, true, 
			EGUI_ELEMENT_TYPE::EGE_GRID });

		Grids.push_back(GUIGrid(Memory, BaseIndex));

		GUIGridHandle Grid = { this, BaseIndex };
		Grid.SetGridDimensions(CellCounts[0], CellCounts[1]);

		return Grid;
	}


	/************************************************************************************************/


	GUIButtonHandle ComplexGUI::CreateButton(GUIElementHandle Parent)
	{
		GUIElementHandle out = this->Elements.size();

		Elements.push_back({ 
			Buttons.size(), 
			Elements[Parent].UpdatePriority + 1, 
			false, EGUI_ELEMENT_TYPE::EGE_BUTTON_TEXT});

		Buttons.push_back(GUIButton());

		return { this, out };
	}


	/************************************************************************************************/


	void ComplexGUI::CreateTexturedButton()
	{

	}


	/************************************************************************************************/


	GUITextBoxHandle ComplexGUI::CreateTextBox(GUIElementHandle Parent)
	{
		GUITextBox Box;
		Box.CellID             = (0, 0);
		Box.Dimensions         = { 0, 0 };
		Box.Font               = nullptr;
		Box.Memory             = Memory;
		Box.Text               = nullptr;
		Box.TextColor          = float4(WHITE, 1);
		TextBoxes.push_back(Box);

		GUIBaseElement Element;
		Element.Active         = false;
		Element.Index          = TextBoxes.size() - 1;
		Element.Type           = EGUI_ELEMENT_TYPE::EGE_TEXTBOX;
		Element.UpdatePriority = Elements[Parent].UpdatePriority + 1;
		Elements.push_back(Element);

		GUIElementHandle Handle = Elements.size() - 1;
		return { this, Handle };
	}


	/************************************************************************************************/


	void ComplexGUI::CreateTextInputBox()
	{

	}


	/************************************************************************************************/


	void ComplexGUI::CreateHorizontalSlider()
	{

	}


	/************************************************************************************************/


	void ComplexGUI::CreateVerticalSlider()
	{

	}
	/************************************************************************************************/


	LayoutEngine::LayoutEngine(iAllocator* tempmemory, iAllocator* IN_Memory, LayoutEngine_Desc& Desc) :
		PositionStack	{IN_Memory},
		Memory			{IN_Memory},
		PixelSize		{Desc.PixelSize},
		TempMemory		{tempmemory},
		FrameGraph		{Desc.FrameGraph},
		RenderTarget    {Desc.RenderTarget},
		VertexBuffer    {Desc.VertexBuffer},
		ConstantBuffer  {Desc.ConstantBuffer}
	{
	}

	
	/************************************************************************************************/


	float2 LayoutEngine::Position2SS( float2 in ) { return {in.x * 2 - 1, in.y * -2 + 1}; }
	float3 LayoutEngine::Position2SS( float3 in ) { return{ in.x * 2 - 1, in.y * -2 + 1, .5 }; }


	/************************************************************************************************/


	float2 LayoutEngine::GetCurrentPosition()
	{
		float2 XY = {};

		for (auto P : PositionStack)
			XY += P;

		return XY;
	}


	/************************************************************************************************/


	void LayoutEngine::PrintLine(const char* Str, float2 WH, SpriteFontAsset* Font, float2 Offset, float2 Scale, bool CenterX, bool CenterY)
	{
		PrintTextFormatting Formatting = PrintTextFormatting::DefaultParams();
		Formatting.PixelSize           = PixelSize;
		Formatting.StartingPOS         = GetCurrentPosition() + Offset;
		Formatting.TextArea            = WH;
		Formatting.Color               = float4(WHITE, 1);
		Formatting.CenterX			   = CenterX;
		Formatting.CenterY			   = CenterY;
		
		//PrintText(GUI, Str, Font, Formatting, TempMemory);
	}


	/************************************************************************************************/


	void LayoutEngine::PushLineSegments(LineSegments& Lines)
	{
		auto Temp = Lines;
		float3 Offset;
		{
			float2 Temp2 = GetCurrentPosition();
			Offset = { Temp2.x, Temp2.y, 0.0f };
		}

		for (auto& L : Temp) 
		{
			L.A += Offset;
			L.B += Offset;

			//L.A = Position2SS(L.A);
			//L.B = Position2SS(L.B);
		}

		DrawShapes(
			EPIPELINESTATES::DRAW_LINE_PSO,
			*FrameGraph, 
			VertexBuffer, 
			ConstantBuffer, 
			RenderTarget, 
			TempMemory, 
			LineShape(Temp));
	}


	/************************************************************************************************/

	void LayoutEngine::PushRect(Draw_RECT Rect)	{
		auto Offset = GetCurrentPosition();

		FK_ASSERT(0, "Not Implemented!");

		Rect.XY+= Offset;

		// Change Cordinate System From Top Down, to Buttom Up
		Draw_RECT Out = Rect;
		//Out.BLeft	= { Rect.BLeft[0], 1 - Rect.TRight[1] };
		//Out.TRight  = { Rect.TRight[0], 1 - Rect.BLeft[1] };
		//FlexKit::PushRect(GUI, Out);

		
		/*
		DrawShapes(
			EPIPELINESTATES::DRAW_PSO,
			*FrameGraph,
			VertexBuffer,
			ConstantBuffer,
			RenderTarget,
			RectangleShape(
				Rect.XY,
				Rect.WH,
				Rect.Color));
				*/
	}


	/************************************************************************************************/


	void LayoutEngine::PushOffset(float2 XY) {
		PositionStack.push_back(XY);
	}


	/************************************************************************************************/


	void LayoutEngine::PopOffset() {
		PositionStack.pop_back();
	}


	/************************************************************************************************/


	void GUIGrid::Update(GUIGridHandle Grid, LayoutEngine* LayoutEngine, double dt, const WindowInput in)
	{
		LayoutEngine->PushOffset(Grid.GetPosition());

		auto ScanFunction = [&](GUIElementHandle hndl, FlexKit::LayoutEngine& Layout)
		{
			Grid.mWindow->UpdateElement(hndl, LayoutEngine, dt, in);
		};

		ScanElements(Grid, *LayoutEngine, ScanFunction);
		LayoutEngine->PopOffset();
	}


	/************************************************************************************************/


	void GUIGrid::Draw(GUIGridHandle Grid, LayoutEngine* LayoutEngine)
	{
		LayoutEngine::Draw_RECT Rect;
		Rect.XY		= {0, 0};
		Rect.WH		= Grid.GetWH();
		Rect.Color	= Grid.GetBackgroundColor();

		LayoutEngine->PushOffset(Grid.GetPosition());
		LayoutEngine->PushRect(Rect);

		auto ScanFunction = [&](GUIElementHandle hndl, FlexKit::LayoutEngine& Layout)
		{
			Grid.mWindow->DrawElement(hndl, LayoutEngine);
		};

		ScanElements(Grid, *LayoutEngine, ScanFunction);
		LayoutEngine->PopOffset();
	}


	/************************************************************************************************/


	void GUIGrid::Draw_DEBUG(GUIGridHandle Grid, LayoutEngine* LayoutEngine)
	{	// Draws Grid Lines
		LayoutEngine->PushOffset(Grid.GetPosition());

		float RowWidth		= Grid._GetGrid().WH[0];
		float ColumnHeight	= Grid._GetGrid().WH[1];

		Vector<LineSegment> Lines(LayoutEngine->Memory);

		{	// Draw Vertical Lines
			float X = 0;
			auto& ColumnWidths = Grid.ColumnWidths();

			for (auto& ColumnWidth : ColumnWidths) {
				float Width = ColumnWidth * RowWidth;
				X += Width;

				LineSegment Line;
				Line.A       = float3{ X, 0, 0 };
				Line.B       = float3{ X, ColumnHeight, 0 };
				Line.AColour = float3(0, 0, 0.5 +  X/2);
				Line.BColour = float3(0, 0, 0.5 +  X/2);

				Lines.push_back(Line);
			}

			LineSegment Line;
			Line.A       = float3{ RowWidth, 0, 0 };
			Line.B       = float3{ RowWidth, ColumnHeight, 0 };
			Line.AColour = float3(1, 0, 0);
			Line.BColour = float3(0, 1, 0);

			Lines.push_back(Line);


			Line.A			= float3{ 0, 0, 0 };
			Line.B			= float3{ 0, ColumnHeight, 0 };
			Line.AColour	= float3(0, 1, 0);
			Line.BColour	= float3(1, 0, 0);

			Lines.push_back(Line);
		}

		{	// Draw Horizontal Lines
			float Y = 0;
			auto& Heights = Grid.RowHeights();

			for (auto& RowHeight : Heights) {
				float Width = RowHeight * ColumnHeight;
				Y += Width;

				LineSegment Line;
				Line.A		 = float3{ 0,			Y,			0 };
				Line.B		 = float3{ RowWidth,	Y,			0 };
				Line.AColour = float3( 0,			0.5 + Y/2,	0 );
				Line.BColour = float3( 0,			0.5 + Y/2,	0 );

				Lines.push_back(Line);
			}

			LineSegment Line;
			Line.A			= float3{ 0,		0, 0 };
			Line.B			= float3{ RowWidth,	0, 0 };
			Line.AColour	= float3( 0,		1, 0 );
			Line.BColour	= float3( 1,		0, 0 );

			Lines.push_back(Line);

			Line.A			= float3{ 0,		ColumnHeight,	0 };
			Line.B			= float3{ RowWidth,	ColumnHeight,	0 };
			Line.AColour	= float3( 0,		1,				0 );
			Line.BColour	= float3( 1,		0,				0 );

			Lines.push_back(Line);
		}

		LayoutEngine->PushLineSegments(Lines);

		ScanElements(
			Grid, 
			*LayoutEngine,
			[&](GUIElementHandle hndl, FlexKit::LayoutEngine& Layout)
			{
				Grid.mWindow->DrawElement_DEBUG(hndl, LayoutEngine);
			});

		LayoutEngine->PopOffset();
	}


	/************************************************************************************************/


	void GUIButton::Draw(GUIButtonHandle button, LayoutEngine* Layout)
	{
		LayoutEngine::Draw_RECT Rect;
		Rect.XY		= { 0.0f, 0.0f };
		Rect.WH		= button.WH();
		Rect.Color  = float4(Grey(0.5f), 1.0f);
	
		Layout->PushRect(Rect);

		if(button.Text() && button._IMPL().Font)
			Layout->PrintLine(
				button.Text(), 
				button.WH(), button._IMPL().Font, 
				{ 0, 0 }, 
				{1, 1}, 
				true, 
				true);
	}


	/************************************************************************************************/


	void GUIButton::Draw_DEBUG(GUIButtonHandle button, LayoutEngine* Layout)
	{
		LineSegments Lines(Layout->Memory);
		LineSegment Line;
		float2	WH = button.WH();

		Line.A       = { 0, 0, 0 };
		Line.AColour = RED;
		Line.B       = { WH.x, WH.y, 0 };
		Line.BColour = RED;

		Lines.push_back(Line);

		Line.A       = { WH.x, 0, 0 };
		Line.AColour = RED;
		Line.B       = { 0, WH.y, 0 };
		Line.BColour = RED;

		Lines.push_back(Line);
		Layout->PushLineSegments(Lines);
	}


	/************************************************************************************************/


	void GUIButton::Update(GUIButtonHandle Btn, LayoutEngine* LayoutEngine, double dt, const WindowInput in)
	{
		auto TL       = LayoutEngine->GetCurrentPosition();
		auto BR       = TL + Btn.WH();
		auto MousePOS = in.MousePosition;
		MousePOS.y    = 1.0f - MousePOS.y;

		//for(size_t I = 0; I < 50; ++I)
		//	std::cout << "\n";

		//std::cout << "TL{ " << TL.x << ":" << TL.y << "}\n";
		//std::cout << "MP{ " <<MousePOS.x << ":" << MousePOS.y << "}\n";
		//std::cout << "BR{ " << BR.x << ":" << BR.y << "}\n";

		if(	MousePOS.x >= TL.x && 
			MousePOS.y >= TL.y && 
			MousePOS.y <= BR.y &&
			MousePOS.x <= BR.x )
		{
			if (Btn._IMPL().Entered && Btn._IMPL().HoverDuration == 0.0)
				Btn._IMPL().Entered(Btn);

			Btn._IMPL().HoverDuration += dt;

			if (Btn._IMPL().Hover && Btn._IMPL().HoverDuration > Btn._IMPL().HoverLength)
				Btn._IMPL().Hover(Btn);

			if (Btn._IMPL().Clicked && in.LeftMouseButtonPressed && !Btn._IMPL().ClickState) {
				Btn._IMPL().Clicked(Btn);
				Btn._IMPL().ClickState = true;
			}
		}
		else
		{
			Btn._IMPL().ClickState		= false;
			Btn._IMPL().HoverDuration	= 0.0;
		}
	}


	/************************************************************************************************/


	void GUITextBox::Update(GUITextBoxHandle TextBox, LayoutEngine* LayoutEngine, double dT, const WindowInput Input)
	{
		auto& Element = TextBox._IMPL();
		auto TL       = LayoutEngine->GetCurrentPosition();
		auto BR       = TL + TextBox.WH();
		auto MousePOS = Input.MousePosition;
		MousePOS.y    = 1.0f - MousePOS.y;

		if(	MousePOS.x >= TL.x && 
			MousePOS.y >= TL.y && 
			MousePOS.y <= BR.y &&
			MousePOS.x <= BR.x )
		{
			if (Element.Entered && Element.HoverDuration == 0.0)
				Element.Entered(TextBox.mBase);


			Element.HoverDuration += dT;

			if (Input.LeftMouseButtonPressed)
			{
				if (Element.Clicked && !Element.ClickState)
				{
					Element.Clicked(TextBox.mBase);
					Element.ClickState = true;
				}
			}
			else
			{
				Element.ClickState = false;
			}
		}
		else
		{
			Element.ClickState		= false;
			Element.HoverDuration	= 0.0;
		}
	}


	/************************************************************************************************/


	void GUITextBox::Draw(GUITextBoxHandle TextBox, LayoutEngine* Layout)
	{
		auto& Element = TextBox._IMPL();
		LayoutEngine::Draw_RECT Rect;
		Rect.XY		= { 0, 0 };
		Rect.WH		= TextBox.WH();
		Rect.Color  = Element.Highlighted ? Element.HighlightedColor : Element.Color;
		Layout->PushRect(Rect);

		if(strlen(Element.Text))
			Layout->PrintLine(Element.Text, TextBox.WH(), Element.Font, { 0, 0 }, {0.4f, 0.4f}, true, true);
	}


	/************************************************************************************************/


	void GUITextBox::Draw_DEBUG(GUITextBoxHandle TextBox, LayoutEngine* Layout)
	{

	}


	/************************************************************************************************/


	GUIBaseElement&	GUIHandle::Framework()	{
		return mWindow->Elements[mBase];
	}

	GUIGrid& GUIGridHandle::_GetGrid()	{
		return mWindow->Grids[mWindow->Elements[mBase].Index];
	}


	/************************************************************************************************/


	GUIGridHandle::GUIGridHandle(GUIHandle hndl) : GUIHandle(hndl.mWindow, hndl.mBase) { FK_ASSERT(hndl.Type() == EGUI_ELEMENT_TYPE::EGE_GRID); }

	GUIGridHandle::GUIGridHandle(ComplexGUI* Window, GUIElementHandle In) : GUIHandle(Window, In) {}


	void GUIHandle::SetActive(bool A) {	Framework().Active = A; }

	const EGUI_ELEMENT_TYPE GUIHandle::Type()		{ return Framework().Type; }
	const EGUI_ELEMENT_TYPE GUIHandle::Type() const { return Framework().Type; }


	/************************************************************************************************/


	const GUIBaseElement	GUIHandle::Framework() const	{ return mWindow->Elements[mBase];  }
	Vector<GUIDimension>&	GUIGridHandle::RowHeights()		{ return _GetGrid().RowHeights;		}
	Vector<GUIDimension>&	GUIGridHandle::ColumnWidths()	{ return _GetGrid().ColumnWidths;	}


	/************************************************************************************************/


	GUIGridCell& GUIGridHandle::GetCell(uint2 ID, bool& Found)
	{
		auto res = find(_GetGrid().Cells, [&](auto& I) { return I.ID == ID; });

		Found = res != _GetGrid().Cells.end();
		return *res;
	}


	/************************************************************************************************/


	float2 GUIGridHandle::GetCellWH(uint2 ID){
		auto temp = _GetGrid().ColumnWidths[ID[1]];
		return float2 { 
			_GetGrid().ColumnWidths[ID[0]], 
			_GetGrid().RowHeights[ID[1]] } * _GetGrid().WH;
	}


	/************************************************************************************************/


	float2 GUIGridHandle::GetWH()
	{
		return _GetGrid().WH;
	}


	/************************************************************************************************/


	float2 GUIGridHandle::GetPosition(){
		return _GetGrid().XY;
	}


	/************************************************************************************************/


	void GUIGridHandle::SetPosition(float2 XY)
	{
		_GetGrid().XY = XY;
	}


	/************************************************************************************************/


	float2 GUIGridHandle::GetChildPosition(GUIElementHandle Element)
	{
		return {};
	}


	/************************************************************************************************/


	float4 GUIGridHandle::GetBackgroundColor()
	{
		return _GetGrid().BackgroundColor;
	}


	/************************************************************************************************/


	void GUIGridHandle::SetBackgroundColor(float4 K)
	{
		_GetGrid().BackgroundColor = K;
	}


	/************************************************************************************************/


	void GUIGridHandle::resize(float Width_percent, float Height_percent)
	{
		_GetGrid().WH = {Width_percent, Height_percent};
	}


	/************************************************************************************************/


	void GUIGridHandle::SetGridDimensions(size_t Columns, size_t Rows)
	{
		auto& Grid = _GetGrid();
		Grid.ColumnWidths.resize(Columns);
		Grid.RowHeights.resize(Rows);

		for (auto& W : Grid.ColumnWidths)
			W = float(Grid.WH[0])  / float(Columns);

		for (auto& H : Grid.RowHeights)
			H = float(Grid.WH[1]) / float(Rows);
	}


	/************************************************************************************************/


	void GUIGridHandle::SetCellFormatting(uint2 CellID, EDrawDirection Formatting)
	{
		bool Result = false;
		auto* Cell = &GetCell(CellID, Result);

		if (!Result) {
			_GetGrid().Cells.push_back({ CellID, (uint32_t)-1 });
			Cell = &GetCell(CellID, Result);
		}

		Cell->StackFormatting = Formatting;
	}


	/************************************************************************************************/


	GUIGridCell* GUIGridHandle::CreateCell(uint2 CellID)
	{
		_GetGrid().Cells.push_back({ CellID, (uint32_t)-1, EDrawDirection::DD_Columns });
		return &_GetGrid().Cells.back();
	}


	/************************************************************************************************/


	GUIButtonHandle GUIGridHandle::CreateButton(uint2 CellID, const char* Str, SpriteFontAsset* font)
	{
		GUIElementHandle out = (uint32_t)-1;
		auto& Grid = _GetGrid();

		if (CellID[0] < Grid.ColumnWidths.size() && 
			CellID[1] < Grid.RowHeights.size())
		{
			out = mWindow->CreateButton(mBase);

			float2 WH	= GetCellWH(CellID);
			mWindow->Buttons[mWindow->Elements[out].Index].Dimensions	= WH;
			mWindow->Buttons[mWindow->Elements[out].Index].Font			= font;
			mWindow->Buttons[mWindow->Elements[out].Index].Text			= Str;

			bool Result = false;
			auto* Cell = &GetCell(CellID, Result);

			if (!Result)
				Cell = CreateCell(CellID);

			if (Cell->Children == 0xFFFFFFFF) {
				auto idx = mWindow->Children.size();
				mWindow->Children.push_back(Vector<GUIElementHandle>(mWindow->Memory));
				Cell->Children = idx;
			}

			mWindow->Children[Cell->Children].push_back(out);
		}

		return{ mWindow, out };
	}


	/************************************************************************************************/


	GUITextBoxHandle GUIGridHandle::CreateTextBox(uint2 CellID, const char* Str, SpriteFontAsset* Font)
	{
		GUIElementHandle out = (uint32_t)-1;
		auto& Grid = _GetGrid();

		if (CellID[0] < Grid.ColumnWidths.size() &&
			CellID[1] < Grid.RowHeights.size())
		{
			auto TextBox = mWindow->CreateTextBox(mBase);

			float2 WH = GetCellWH(CellID);
			TextBox._IMPL().Dimensions			= WH;
			TextBox._IMPL().Color				= float4(Grey(0.5f), 1);

			TextBox._IMPL().Highlighted			= false;
			TextBox._IMPL().HighlightedColor	= float4(Grey(0.8f), 1);

			TextBox.SetCellID(CellID);
			TextBox.SetText(Str);
			TextBox.SetTextFont(Font);

			bool Result = false;
			auto* Cell = &GetCell(CellID, Result);

			if (!Result)
				Cell = CreateCell(CellID);

			if (Cell->Children == 0xFFFFFFFF) {
				auto idx = mWindow->Children.size();
				mWindow->Children.push_back(Vector<GUIElementHandle>(mWindow->Memory));
				Cell->Children = idx;
			}

			mWindow->Children[Cell->Children].push_back(TextBox.mBase);
			out = TextBox.mBase;
		}

		return{ mWindow, out };
	}


	/************************************************************************************************/


	GUIButtonHandle::GUIButtonHandle(ComplexGUI* Window, GUIElementHandle In) : GUIHandle(Window, In){}


	/************************************************************************************************/


	GUIButton& GUIButtonHandle::_IMPL()
	{
		auto Impl = Framework().Index;
		return mWindow->Buttons[Impl];
	}


	/************************************************************************************************/


	void GUIButtonHandle::resize(float Width_percent, float Height_percent) {
		_IMPL().Dimensions = {Width_percent, Height_percent};
	}


	/************************************************************************************************/


	void GUIButtonHandle::SetCellID(uint2 CellID) {	_IMPL().CellID = CellID; }


	/************************************************************************************************/


	void GUIButtonHandle::SetUSR(void* usr)
	{
		_IMPL().USR = usr;
	}


	/************************************************************************************************/


	const char* GUIButtonHandle::Text() { return _IMPL().Text; }


	/************************************************************************************************/


	float2 GUIButtonHandle::WH() { return _IMPL().Dimensions; }


	/************************************************************************************************/


	GUITextBoxHandle::GUITextBoxHandle(GUIHandle Handle)
	{
		mBase = Handle.mBase;
		mWindow = Handle.mWindow;
	}


	GUITextBoxHandle::GUITextBoxHandle(ComplexGUI* Window, GUIElementHandle In)
	{
		mBase = In;
		mWindow = Window;
	}


	void GUITextBoxHandle::SetText(const char* Text)
	{
		auto& TextBox = _IMPL();
		TextBox.Text = Text;
	}


	void GUITextBoxHandle::SetTextFont(SpriteFontAsset* Font)
	{
		auto& TextBox = _IMPL();
		TextBox.Font = Font;
	}


	void GUITextBoxHandle::SetCellID(uint2 CellID)
	{
		auto& TextBox = _IMPL();
		TextBox.CellID = CellID;
	}


	float2 GUITextBoxHandle::WH()
	{
		return _IMPL().Dimensions;
	}


	GUITextBox& GUITextBoxHandle::_IMPL()
	{
		return mWindow->TextBoxes[mWindow->Elements[mBase].Index];
	}

	/************************************************************************************************/
}// namespace FlexKit