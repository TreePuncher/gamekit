/**********************************************************************

Copyright (c) 2015 - 2018 Robert May

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

	UIElementID_t GenerateRandomID()
	{
		return -1;
	}

	/************************************************************************************************/


	bool CursorInside(float2 cursorPos, float2 cursorWH, float2 pos, float2 wh)
	{
		auto elementCord = cursorPos - pos;
		auto elementSpan = wh - elementCord;

#if 0
		std::cout << "Checking Cursor Inside: \n CursorPos:";
		printfloat2(cursorPos);
		std::cout << "\n CursorWH:";
		printfloat2(cursorWH);
		std::cout << "\n Element TL POS:";
		printfloat2(pos);
		std::cout << "\n Element TL WH:";
		printfloat2(wh);
		std::cout << "\n ";
#endif

		return 
			(elementCord.x > 0.0f && elementCord.y > 0.0f) && 
			(elementSpan.x > 0.0f && elementSpan.y > 0.0f);
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


	GuiSystem::GuiSystem(GuiSystem_Desc desc, iAllocator* allocator) :
		Buttons		{ allocator, desc.ButtonCount	},
		Grids		{ allocator, desc.GridCount		},
		TextBoxes	{ allocator, desc.TextBoxes		},
		Elements	{ allocator },
		Memory		{ allocator	}
	{}


	/************************************************************************************************/


	void GuiSystem::Release()
	{}


	/************************************************************************************************/


	void GuiSystem::Update(double dt, const WindowInput input, float2 PixelSize, iAllocator* tempMemory)
	{
		LayoutEngine_Desc Desc;
		LayoutEngine layoutEngine(tempMemory, Memory, Desc);

		sort(
			Elements.begin(), Elements.end(), 
			[](auto lhs, auto rhs) -> bool
			{
				return lhs->updatePriority > rhs->updatePriority;
			});

		Grids.Visit(
			[&](auto& Grid)
			{
				Grid.Update(&layoutEngine, dt, input);
			});


		// Update nodes with children first
		for (auto E : Elements)
		{
			switch (E->type)
			{
			case EGUI_ELEMENT_TYPE::EGE_BUTTON_TEXT:
			case EGUI_ELEMENT_TYPE::EGE_GRID:
				continue;
			default:
				E->Update(&layoutEngine, dt, input);
			}
		}
	}


	/************************************************************************************************/


	void GuiSystem::Draw(DrawUI_Desc& Desc, iAllocator* Temp)
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

		Grids.Visit(
			[&](auto& grid) 
			{
				if(grid.updatePriority == 0)
					grid.Draw(&Layout);
			});
	}

	
	/************************************************************************************************/


	void GuiSystem::UpdateElement(GUIElementHandle Element, LayoutEngine* Layout, double dt, const	WindowInput Input)
	{
		FK_ASSERT(0, "NOT IMPLEMENTED");

		/*
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
		*/
	}


	/************************************************************************************************/


	void GuiSystem::DrawElement(GUIElementHandle Element, LayoutEngine* Layout)
	{
		FK_ASSERT(0, "NOT IMPLEMENTED");
		
		/*
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
		*/
	}


	/************************************************************************************************/


	void GuiSystem::DrawElement_DEBUG(GUIElementHandle Element, LayoutEngine* Layout)
	{
		FK_ASSERT(0, "NOT IMPLEMENTED");

		/*
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
		*/
	}


	/************************************************************************************************/
	
	void GuiSystem::Draw_DEBUG(DrawUI_Desc& Desc, iAllocator* Temp)
	{
		/*
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
		*/
	}


	/************************************************************************************************/


	GUIGrid& GuiSystem::CreateGrid(GUIGrid* parent, uint2 ID)
	{
		GUIGrid& newGrid		= Grids.Allocate(GenerateRandomID(), Memory);
		newGrid.updatePriority	= parent ? parent->updatePriority + 1 : 0;
		newGrid.active			= true;
		newGrid.cellID			= ID;

		if (parent)
			parent->AddChild(&newGrid, ID);

		Elements.push_back(&newGrid);

		return newGrid;
	}


	/************************************************************************************************/


	GUIButton& GuiSystem::CreateButton(GUIGrid* parent, uint2 cellID)
	{
		GUIButton& newButton		= Buttons.Allocate(GenerateRandomID());
		newButton.updatePriority	= parent->updatePriority + 1;
		newButton.active			= true;
		newButton.cellID			= cellID;

		parent->AddChild(&newButton, cellID);
		
		Elements.push_back(&newButton);

		return newButton;
	}


	/************************************************************************************************/


	void GuiSystem::CreateTexturedButton()
	{}


	/************************************************************************************************/


	GUITextBox&	GuiSystem::CreateTextBox(GUIElement* Parent)
	{
		FK_ASSERT(Parent != nullptr, "Parent argument cannot be Null!");
		
		GUITextBox& Box = TextBoxes.Allocate(GenerateRandomID());
		Box.CellID             = (0, 0);
		Box.Dimensions         = { 0, 0 };
		Box.Font               = nullptr;
		Box.Memory             = Memory;
		Box.Text               = nullptr;
		Box.TextColor          = float4(WHITE, 1);

		Box.active				= false;
		Box.updatePriority		= Parent->updatePriority + 1;

		return Box;
	}


	/************************************************************************************************/


	void GuiSystem::CreateTextInputBox()
	{}


	/************************************************************************************************/


	void GuiSystem::CreateHorizontalSlider()
	{}


	/************************************************************************************************/


	void GuiSystem::CreateVerticalSlider()
	{}


	/************************************************************************************************/


	void GuiSystem::Release(GUIButton* btn)
	{
		Buttons.Release(*btn);
	}


	/************************************************************************************************/


	LayoutEngine::LayoutEngine(iAllocator* tempmemory, iAllocator* IN_Memory, LayoutEngine_Desc& Desc) :
		AreaStack		{IN_Memory},
		PositionStack	{IN_Memory},
		Memory			{IN_Memory},
		PixelSize		{Desc.PixelSize},
		TempMemory		{tempmemory},
		FrameGraph		{Desc.FrameGraph},
		RenderTarget    {Desc.RenderTarget},
		VertexBuffer    {Desc.VertexBuffer},
		TextBuffer		{Desc.TextBuffer},
		ConstantBuffer  {Desc.ConstantBuffer}
	{}

	
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


	float2 LayoutEngine::GetDrawArea()
	{
		float2 XY = {1.0f, 1.0f};

		for (auto P : AreaStack)
			XY *= P;

		return XY;
	}


	/************************************************************************************************/


	void LayoutEngine::PrintLine(const char* Str, float2 WH, SpriteFontAsset* Font, float2 Offset, float2 Scale, bool CenterX, bool CenterY)
	{
		PrintTextFormatting Formatting = PrintTextFormatting::DefaultParams();
		Formatting.PixelSize           = PixelSize;
		Formatting.StartingPOS         = GetCurrentPosition() + Offset;
		Formatting.TextArea            = GetDrawArea() * WH;
		Formatting.Color               = float4(WHITE, 1);
		Formatting.CenterX			   = true;
		Formatting.CenterY			   = false;

		DrawSprite_Text(
			Str,
			*FrameGraph, 
			*Font, 
			TextBuffer, 
			RenderTarget, 
			TempMemory, 
			Formatting);
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

		float3 CurrentDrawArea;
		{
			float2 Temp2 = GetDrawArea();
			CurrentDrawArea = { Temp2.x, Temp2.y, 0.0f };
		}

		for (auto& L : Temp) 
		{
			L.A = L.A * CurrentDrawArea + Offset;
			L.B = L.B * CurrentDrawArea + Offset;
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
		float3 Offset;
		{
			float2 Temp2 = GetCurrentPosition();
			Offset = { Temp2.x, Temp2.y, 0.0f };
		}

		float3 CurrentDrawArea;  
		{
			float2 Temp2 = GetDrawArea();
			CurrentDrawArea = { Temp2.x, Temp2.y, 0.0f };
		}

		Rect.XY = Rect.XY * CurrentDrawArea + Offset;
		Rect.WH = Rect.WH * CurrentDrawArea;

		
		DrawShapes(
			EPIPELINESTATES::DRAW_PSO,
			*FrameGraph,
			VertexBuffer,
			ConstantBuffer,
			RenderTarget,
			TempMemory,
			RectangleShape(
				Rect.XY,
				Rect.WH,
				Rect.Color));
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


	void LayoutEngine::PushDrawArea(float2 XY)
	{
		AreaStack.push_back(XY);
	}


	/************************************************************************************************/


	void LayoutEngine::PopDrawArea()
	{
		AreaStack.pop_back();
	}


	/************************************************************************************************/


	bool LayoutEngine::IsInside(float2 POS, float2 WH)
	{
		POS.y -= WH.y;

		auto BottomLeft	= this->GetCurrentPosition();
		auto TopRight	= this->GetDrawArea() + BottomLeft;

		return (POS > BottomLeft && POS.y < TopRight.y && POS.x < TopRight.x);
	}


	/************************************************************************************************/


	void GUIButton::Update(LayoutEngine* layoutEngine, double dt, const WindowInput& input)
	{
		if (customUpdate)
		{
			customUpdate(*this, layoutEngine, dt, input);
			return;
		}

		auto CursorInButton = layoutEngine->IsInside(input.MousePosition, input.CursorWH);

		if (CursorInButton)
		{
			HoverDuration += dt;

			if (Entered)
				Entered();
			if (!ClickState && input.LeftMouseButtonPressed)
			{
				if (Clicked)
					Clicked();

				ClickState = true;
			}
			else if (ClickState && !input.LeftMouseButtonPressed) {
				if(Released)
					Released();

				ClickState = false;
			}

			if (HoverDuration > HoverLength && HoverLength > 0 && Hover)
				Hover();
		}
		else
		{
			if (input.LeftMouseButtonPressed)
				ClickState = true;
			else
				ClickState = false;

			HoverDuration = 0;
		}

	}


	/************************************************************************************************/


	void GUIButton::Draw(LayoutEngine* LayoutEngine)
	{
		if (customDraw) {
			customDraw(*this, LayoutEngine);
			return;
		}

		LayoutEngine::Draw_RECT Rect;
		Rect.XY		= { 0.0f, 0.0f };
		Rect.WH		= WH;
		Rect.Color	= Color;

		LayoutEngine->PushRect(Rect);

		if (Font != nullptr &&
			Text != nullptr)
			LayoutEngine->PrintLine(Text, WH, Font, { 0, 0 }, { 1.0f, 1.0f }, true, true);
	}


	/************************************************************************************************/


	/*
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
	*/


	/************************************************************************************************/


	/*
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
	*/


	/************************************************************************************************/

	
	/*
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
	*/


	/************************************************************************************************/


	/*
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
	*/


	/************************************************************************************************/


	/*
	void GUITextBox::Draw_DEBUG(GUITextBoxHandle TextBox, LayoutEngine* Layout)
	{

	}
	*/


	/************************************************************************************************/
}// namespace FlexKit