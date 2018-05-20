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

namespace FlexKit
{
	/************************************************************************************************/


	void InitiateSimpleWindow(iAllocator* Memory, SimpleWindow* Out, SimpleWindow_Desc& Desc)
	{
		Out->CellState.Allocator		= Memory;
		Out->Dividers.Allocator			= Memory;
		Out->Elements.Allocator			= Memory;
		Out->Lists.Allocator			= Memory;
		Out->Root.Allocator				= Memory;
		Out->Slider.Allocator			= Memory;
		Out->TextButtons.Allocator		= Memory;
		Out->TextInputs.Allocator		= Memory;
		Out->TexturedButtons.Allocator	= Memory;
		Out->Memory						= Memory;

		Out->ColumnCount	= Desc.ColumnCount;
		Out->RowCount		= Desc.RowCount;
		Out->AspectRatio	= Desc.AspectRatio;

		Out->CellBorder = float2(0.005f * 1/Desc.AspectRatio, 0.005f);

#if 0
		Out->RHeights	= Desc.RowHeights;
		Out->CWidths	= Desc.ColumnWidths;
#endif

		Out->Position	= { 0.0f, 0.0f };
		Out->WH			= { Desc.Width, Desc.Height };


		size_t CellCount = Desc.ColumnCount * Desc.RowCount;
		Out->CellState.reserve(CellCount);

		for (size_t I = 0; I < CellCount; ++I)
			Out->CellState.push_back(false);
	}


	/************************************************************************************************/


	void CleanUpSimpleWindow(SimpleWindow* W, RenderSystem* RS)
	{
		/*
		for (auto TB : W->TextButtons)
			CleanUpTextArea(&TB.Text, W->Memory, RS);

		for (auto TB : W->TextInputs)
			CleanUpTextArea(&TB.TextGUI, W->Memory, RS);
		*/

		W->Elements.Release();
		W->TexturedButtons.Release();
		W->Lists.Release();
		W->Root.Release();
		W->Dividers.Release();
		W->CellState.Release();
		W->TextButtons.Release();
		W->TextInputs.Release();
		W->Slider.Release();
	}


	/************************************************************************************************/


	float2 GetCellPosition(SimpleWindow* Window, uint2 Cell)
	{
		uint32_t ColumnCount = Window->ColumnCount;
		uint32_t RowCount	 = Window->RowCount;
		
		const float WindowWidth  = Window->WH[0];
		const float WindowHeight = Window->WH[1];
		
		const float CellGapWidth  = Window->CellBorder[0];
		const float CellGapHeight = Window->CellBorder[1];
		
		const float CellWidth	= (WindowWidth  / ColumnCount)	- CellGapWidth  - CellGapWidth  / ColumnCount;
		const float CellHeight	= (WindowHeight / RowCount)		- CellGapHeight - CellGapHeight / RowCount;//	

		return Window->Position + float2{ 
				(CellWidth + CellGapWidth)	 * Cell[0] + CellGapWidth, 
				(CellHeight + CellGapHeight) * Cell[1] + CellGapHeight };
	}


	float2 GetCellDimension(SimpleWindow* Window, uint2 WH = {1, 1})
	{
		WH[0] = min(WH[0], Window->ColumnCount);
		WH[1] = min(WH[1], Window->RowCount);

		uint32_t ColumnCount = Window->ColumnCount;
		uint32_t RowCount	 = Window->RowCount;
		
		const float WindowWidth  = Window->WH[0];
		const float WindowHeight = Window->WH[1];
		
		const float CellGapWidth  = Window->CellBorder[0];
		const float CellGapHeight = Window->CellBorder[1];
		
		const float CellWidth	= (WindowWidth  / ColumnCount)	- CellGapWidth  - CellGapWidth  / ColumnCount;
		const float CellHeight	= (WindowHeight / RowCount)		- 2 * CellGapHeight - CellGapHeight / RowCount;	

		return float2(CellWidth, CellHeight) * WH + float2(CellGapWidth *(WH[0] - 1), CellGapHeight);
	}


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


	bool HandleWindowInput(SimpleWindowInput* Input, GUIChildList& Elements, SimpleWindow* Window, ParentInfo& P, FormattingOptions& Options = FormattingOptions())
	{
		FormattingState CurrentFormatting = InitiateFormattingState(Options, P);

		/*
		for (auto& I : Elements)
		{
			const auto& E = Window->Elements[I];
			float2 WH;

			switch (E.Type) {
			case EGE_HLIST:
			case EGE_VLIST: {
				float2 Position = GetCellPosition(Window, E.Position);
					   WH		= GetCellDimension(Window, Window->Lists[E.Index].WH);

				ParentInfo This;
				This.ParentPosition = Position + CurrentFormatting.CurrentOffset;
				This.ClipArea_BL	= This.ParentPosition;
				This.ClipArea_TR	= This.ClipArea_BL + WH;
				This.ClipArea_WH	= WH;

				if (CursorInside(Input->MousePosition, Input->CursorWH, Position, WH)) {
					FormattingOptions Formatting;
					
					if (E.Type == EGE_HLIST)
						Formatting.DrawDirection = EDrawDirection::DD_Rows;
					else if (E.Type == EGE_VLIST)
						Formatting.DrawDirection = EDrawDirection::DD_Columns;

					if (HandleWindowInput(Input, Window->Lists[E.Index].Children, Window, This, Formatting))
						return true;
				}
			}	break;
			case EGE_BUTTON_TEXTURED: {
				auto		CurrentButton	= &Window->TexturedButtons[E.Index];
				float2		Position		= GetFormattedPosition(CurrentFormatting, Options, CurrentButton->WH);
							WH				= CurrentButton->WH;
				Texture2D*	Texture			= CurrentButton->Active ? CurrentButton->Texture_Active : CurrentButton->Texture_InActive;

				if (!WillFit( Position, Position + WH,P.ClipArea_BL, P.ClipArea_TR) && Options.OverflowHandling == OF_Exlude)
					return false;

				if (CursorInside(Input->MousePosition, Input->CursorWH, Position, WH)) {

					if (CurrentButton->OnClicked_CB && Input->LeftMouseButtonPressed)
						if (CurrentButton->OnClicked_CB(I))
							return true;
					if (CurrentButton->OnEntered_CB && !CurrentButton->Entered)
						CurrentButton->OnEntered_CB(I);

					CurrentButton->Active  = true;
					CurrentButton->Entered = true;
				}
				else {
					if (CurrentButton->OnExit_CB && CurrentButton->Entered)
						if (CurrentButton->OnExit_CB(I))
							return true;
					CurrentButton->Entered = false;
				}

				if (!WillFit(Position, Position + WH, P.ClipArea_BL, P.ClipArea_TR) && Options.OverflowHandling == OF_Exlude)
					return false;
			}	break;
			case EGE_BUTTON_TEXT: {
				auto		CurrentButton	= &Window->TextButtons[E.Index];
				float2		Position		= GetFormattedPosition(CurrentFormatting, Options, CurrentButton->WH);
							WH				= CurrentButton->WH;

				if (!WillFit(Position, Position + WH, P.ClipArea_BL, P.ClipArea_TR) && Options.OverflowHandling == OF_Exlude)
					return false;

				if (CursorInside(Input->MousePosition, Input->CursorWH, Position, WH)) {

					if (CurrentButton->OnClicked_CB && Input->LeftMouseButtonPressed)
						if (CurrentButton->OnClicked_CB(I))
							return true;
					if (CurrentButton->OnEntered_CB && !CurrentButton->Entered)
						if (CurrentButton->OnEntered_CB(I))
							return true;

					CurrentButton->Active  = true;
					CurrentButton->Entered = true;
				}
				else {
					if (CurrentButton->OnExit_CB && CurrentButton->Entered)
						if (CurrentButton->OnExit_CB(I))
							return true;
					CurrentButton->Entered = false;
				}
			}	break;
			case EGE_HSLIDER:
			case EGE_VSLIDER: {
				auto& Slider = Window->Slider[E.Index];

				WH				= Slider.WH;
				float2 Position = GetFormattedPosition(CurrentFormatting, Options, Slider.WH);
				
				float  R = Saturate(Slider.BarRatio);

				if (!WillFit(
					Position,
					Position + WH,
					P.ClipArea_BL, P.ClipArea_TR)
					&& Options.OverflowHandling == OF_Exlude)
					return false;

				float2 S  = (E.Type == EGE_HSLIDER) ? float2{R, 1.0f} : float2{ 1.0f, R};
				float2 S2 = (E.Type == EGE_HSLIDER) ? float2{1.0f, 0.0f} : float2{ 0.0f, 1.0f };

				float2 TravelDistance = WH * (1 - R);
				TravelDistance *= S2;

				Draw_RECT_CLIPPED Bar;
				float2 BarPosition	= Position + TravelDistance * Saturate(Slider.SliderPosition);
				float2 BarWH		= Bar.BLeft + WH * S;
				if (CursorInside(Input->MousePosition, Input->CursorWH, Position, WH)) {
					if (!Slider.Entered)
						if (Slider.OnEntered_CB)
							if(Slider.OnEntered_CB(Slider.SliderPosition, Slider.CB_Args, I))
								return true; 

					if (CursorInside(Input->MousePosition, Input->CursorWH, BarPosition, BarWH) && Input->LeftMouseButtonPressed)
						if (Slider.OnClicked_CB)
							if (Slider.OnClicked_CB(Slider.SliderPosition, Slider.CB_Args, I))
								return true;

					Slider.Entered = true;
				}
				else
					Slider.Entered = false;
			}	break;
			case EGE_TEXTINPUT: {
				auto TextInput = Window->TextInputs[E.Index];

				WH = TextInput.WH;
				auto Position = GetFormattedPosition(CurrentFormatting, Options, WH);

				if (!WillFit(Position, Position + WH, P.ClipArea_BL, P.ClipArea_TR) && Options.OverflowHandling == OF_Exlude)
					return false;
			}	break;
			case EGE_DIVIDER: {
				WH = Window->Dividers[E.Index].Size;
			}	break;
			default:
				break;
			}

			if (IncrementFormatingState(CurrentFormatting, Options, WH))
				return true;// Ran out of Space in Row/Column
		}
		*/
		return false;
	}


	/************************************************************************************************/


	bool CursorInsideCell(SimpleWindow* Window, float2 CursorPos, float2 CursorWH, uint2 Cell)
	{
		auto CellPOS = GetCellPosition	(Window, Cell);
		auto CellWH  = GetCellDimension	(Window);

		return CursorInside(CursorPos, CursorWH, CellPOS, CellWH);
	}


	/************************************************************************************************/


	void UpdateSimpleWindow(SimpleWindowInput* Input, SimpleWindow* Window)
	{
		ParentInfo P;
		P.ClipArea_BL = Window->Position;
		P.ClipArea_TR = Window->Position + Window->WH;
		P.ParentPosition = Window->Position;

		HandleWindowInput(Input, Window->Root, Window, P);
	}


	/************************************************************************************************/


	bool GameWindow_PushElement(SimpleWindow* Window, size_t Target, GUIElement E)
	{
		if(Window->Elements.size())
			switch (Window->Elements[Target].Type)
			{
			case EGE_HLIST:
			case EGE_VLIST:
				Window->Lists[Window->Elements[Target].Index].Children.push_back(Window->Elements.size());
				return true;
			default:
				break;
			}
		return false;
	}


	/************************************************************************************************/


	GUIElementHandle GetCellState(uint2 Cell, SimpleWindow* W)
	{
		return W->CellState[Cell[0] * W->RowCount + Cell[1]];
	}

	void SetCellState(uint2 Cell, int32_t S, SimpleWindow* W)
	{
		W->CellState[Cell[0] * W->RowCount + Cell[1]] = S;
	}

	void SetCellState(uint2 Cell, uint2 WH, int32_t S, SimpleWindow* W)
	{
		for (size_t I = 0; I < WH[0]; ++I)
			for (size_t J = 0; J < WH[1]; ++J) 
				SetCellState(Cell + uint2{I, J}, S, W);
	}


	/************************************************************************************************/


	bool RoomAvailable(SimpleWindow* W, uint2 POS, uint2 WH, int32_t ID) {
		for (size_t I = 0; I < WH[0]; ++I) {
			for (size_t J = 0; J < WH[0]; ++J) {
				if (!GetCellState(POS + uint2{ I, J }, W))
					return false;
			}
		}

		return true;
	}

	GUIElementHandle SimpleWindowAddVerticalList(SimpleWindow* Window, GUIList& Desc, GUIElementHandle Parent)
	{
		GUIElement		 E;
		GUIElement_List  P;

		E.Type               = EGE_VLIST;
		E.Position           = Desc.Position;
		E.Index              = Window->Lists.size();

		P.WH                 = Desc.WH;
		P.Color              = Desc.Color;
		P.Children.Allocator = Window->Memory;

		bool Push = false;
		if (Parent == -1) 
		{
			FK_ASSERT(!RoomAvailable(Window, Desc.Position, Desc.WH, Parent));

			Window->Root.push_back(Window->Elements.size());
			Push = true;
		}
		else
			Push = GameWindow_PushElement(Window, Parent, E);

		if (Push) {
			Window->Elements.push_back(E);
			Window->Lists.push_back(P);
			SetCellState(E.Position, P.WH, Window->Elements.size(), Window);
		}

		return Window->Elements.size() - 1;
	}


	/************************************************************************************************/


	GUIElementHandle SimpleWindowAddHorizonalList(SimpleWindow* Window, GUIList& Desc, GUIElementHandle Parent)
	{
		GUIElement		E;
		GUIElement_List P;

		E.Type               = EGE_HLIST;
		E.Position           = Desc.Position;
		E.Index              = Window->Lists.size();

		P.WH                 = Desc.WH;
		P.Color              = Desc.Color;
		P.Children.Allocator = Window->Memory;

		bool Push = false;
		if (Parent == -1) 
		{
			FK_ASSERT(!RoomAvailable(Window, Desc.Position, Desc.WH, Parent));

			Window->Root.push_back(Window->Elements.size());
			Push = true;
		}
		else
			Push = GameWindow_PushElement(Window, Parent, E);

		if (Push) {
			Window->Elements.push_back(E);
			Window->Lists.push_back(P);
			SetCellState(E.Position, P.WH, true, Window);
		}

		return Window->Elements.size() - 1;
	}


	/************************************************************************************************/


	GUIElementHandle SimpleWindowAddTexturedButton(SimpleWindow* Window, GUITexturedButton_Desc&	Desc, GUIElementHandle Parent)
	{
		GUIElement					E;
		GUIElement_TexuredButton	TB;

		E.Position	= 0;// Unused here
		E.Type		= EGE_BUTTON_TEXTURED;
		E.Index		= Window->TexturedButtons.size();

		TB.Texture_Active	= Desc.Texture_Active;
		TB.Texture_InActive = Desc.Texture_InActive;
		TB.WH				= Desc.WH;
		TB.OnClicked_CB		= Desc.OnClicked_CB;
		TB.OnEntered_CB		= Desc.OnEntered_CB;
		TB.OnExit_CB		= Desc.OnExit_CB;

		FK_ASSERT(Parent != -1); // Must be a child of a panel or other Element

		GameWindow_PushElement(Window, Parent, E);

		Window->Elements.push_back(E);
		Window->TexturedButtons.push_back(TB);

		return Window->Elements.size() - 1;
	}


	/************************************************************************************************/


	GUIElementHandle SimpleWindowAddTextButton(SimpleWindow*	Window, GUITextButton_Desc& Desc, RenderSystem* RS, GUIElementHandle Parent)
	{
		FK_ASSERT(Desc.WH.Product() > 0,			"INVALID DIMENSIONS!!");
		FK_ASSERT(Desc.WindowSize.Product() > 0,	"INVALID WINDOW DIMENSIONS!!");

		TextArea_Desc TA_Desc = {
			{ 0, 0 },	// Position Will be set later
			{ Desc.WindowSize * float2{float(Desc.WH[0]), float(Desc.WH[1]) }},
			Desc.CharacterScale * Desc.Font->FontSize
		};
		
		//auto Text = CreateTextArea(RS, Window->Memory, &TA_Desc);
		//PrintText(&Text, Desc.Text);

		GUIElement			  E;
		GUIElement_TextButton TB;
		E.Type			= EGE_BUTTON_TEXT;
		E.Index			= Window->TextButtons.size();;
		E.Position		= 0;// Unused here

		TB.OnClicked_CB     = Desc.OnClicked_CB;
		TB.OnEntered_CB     = Desc.OnEntered_CB;
		TB.OnExit_CB        = Desc.OnExit_CB;
		TB.CB_Args		    = Desc.CB_Args;
		//TB.Text			    = Text;
		TB.WH			    = Desc.WH;
		TB.Font			    = Desc.Font;
		TB.BackGroundColor	= Desc.BackGroundColor;
		TB.TextColor		= Desc.TextColor;

		GameWindow_PushElement(Window, Parent, E);

		Window->Elements.push_back(E);
		Window->TextButtons.push_back(TB);

		return Window->Elements.size() - 1;
	}


	/************************************************************************************************/


	void SimpleWindowAddDivider(SimpleWindow* Window, float2 Size, GUIElementHandle Parent)
	{
		GUIElement	   E;
		GUIElement_Div D;

		E.Position	= 0;// Unused here
		E.Type		= EGE_DIVIDER;
		E.Index		= Window->Dividers.size();
		D.Size		= Size;

		bool Push = false;
		FK_ASSERT(Parent != -1); // Must be a child of a panel or other Element

		GameWindow_PushElement(Window, Parent, E);

		Window->Elements.push_back(E);
		Window->Dividers.push_back(D);
	}


	/************************************************************************************************/


	GUIElementHandle SimpleWindowAddHorizontalSlider(SimpleWindow* Window, GUISlider_Desc Desc, GUIElementHandle Parent)
	{
		/*
		struct GUIElement_Slider
		{
			float2			WH;
			float			SliderPosition;

			SliderEventFN OnClicked_CB = nullptr;
			SliderEventFN OnMoved_CB   = nullptr;
			SliderEventFN OnEntered_CB = nullptr;
			SliderEventFN OnExit_CB	   = nullptr;
			void*		  CB_Args	   = nullptr;
		};
		*/

		GUIElement_Slider	NewSlider;
		GUIElement			E;
		E.Type		= EGE_HSLIDER;
		E.Index		= Window->Slider.size();
		E.Position	= 0;// Unused here

		NewSlider.OnClicked_CB	= Desc.OnClicked_CB;
		NewSlider.OnEntered_CB	= Desc.OnEntered_CB;
		NewSlider.OnExit_CB		= Desc.OnExit_CB;
		NewSlider.OnMoved_CB	= Desc.OnMoved_CB;
		NewSlider.CB_Args		= Desc.CB_Args;

		NewSlider.SliderPosition = Desc.InitialPosition;
		NewSlider.WH			 = Desc.WH;
		NewSlider.BarRatio		 = Desc.BarRatio;

		GameWindow_PushElement(Window, Parent, E);

		Window->Elements.push_back(E);
		Window->Slider.push_back(NewSlider);

		return Window->Elements.size() - 1;
	}


	/************************************************************************************************/


	GUIElementHandle SimpleWindowAddTextInput(SimpleWindow*	Window, GUITextInput_Desc& Desc, RenderSystem* RS, GUIElementHandle Parent)
	{
		/*
		struct GUIElement_TextInput
		{
			float2				WH;
			Texture2D*			Texture;
			char*				Text;

			size_t				MaxTextLength;

			void*				_ptr;
			TextInputEventFN	OnTextEntered;
			TextInputEventFN	OnTextChanged;

			iAllocator*		Memory;
		};
		*/

		GUIElement				E;
		GUIElement_TextInput	TI;

		E.Type		= EGE_TEXTINPUT;
		E.Index		= Window->TextInputs.size();
		E.Position	= 0;

		TextArea_Desc TA_Desc = {
			{ 0, 0 },	// Position Will be set later
			{ Desc.WindowSize * float2{ float(Desc.WH[0]), float(Desc.WH[1]) } },
			{ 16, 16 }
		};

		//TI.TextGUI			= CreateTextArea(RS, Window->Memory, &TA_Desc);
		TI.Memory			= Window->Memory;
		TI.WH				= Desc.WH;
		TI.Font				= Desc.Font;
		TI.MaxTextLength	= TI.TextGUI.BufferDimensions.Product();

		//if(Desc.Text)
		//	PrintText(&TI.TextGUI, Desc.Text);

		GameWindow_PushElement(Window, Parent, E);

		Window->Elements.push_back(E);
		Window->TextInputs.push_back(TI);

		return Window->Elements.size() - 1;
	}


	/************************************************************************************************/


	void SetSliderPosition(float R, GUIElementHandle Element, SimpleWindow* W) {
		W->Slider[W->Elements[Element].Index].SliderPosition = R;
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
		//Grids.Release();
		//Elements.Release();
	}


	/************************************************************************************************/


	void ComplexGUI::Update(double dt, const SimpleWindowInput Input, float2 PixelSize, iAllocator* TempMemory)
	{
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
	}


	/************************************************************************************************/
	/*

	void ComplexGUI::Draw(RenderSystem* RS, ImmediateRender* out, iAllocator* Temp, float2 PixelSize)
	{
		LayoutEngine Layout(Temp, Memory, RS, out, PixelSize);

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
	
	*/
	/************************************************************************************************/


	void ComplexGUI::UpdateElement(GUIElementHandle Element, LayoutEngine* Layout, double dt, const	SimpleWindowInput Input)
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
	/*

	void ComplexGUI::Draw_DEBUG(RenderSystem* RS, ImmediateRender* out, iAllocator* Temp, float2 PixelSize)
	{
		LayoutEngine Layout(Temp, Memory, RS, out, PixelSize);

		for(size_t I = 0; I < Elements.size(); ++I)
		{
			if (Elements[I].UpdatePriority == 0) {
				DrawElement_DEBUG(I, &Layout);
			}
		}
	}

	*/
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


	GUIGridHandle ComplexGUI::CreateGrid(uint2 ID)
	{
		GUIElementHandle BaseIndex = Elements.size();

		Elements.push_back({ 
			Grids.size(), 
			0x00, true, 
			EGUI_ELEMENT_TYPE::EGE_GRID });
		Grids.push_back(GUIGrid(Memory, BaseIndex));

		return{ this, BaseIndex };
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


	LayoutEngine::LayoutEngine(iAllocator* tempmemory, iAllocator* memory, RenderSystem* rs, float2 pixelsize) :
		PositionStack(memory),
		RS(rs),
		Memory(memory),
		PixelSize(pixelsize),
		TempMemory(tempmemory)
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


	void LayoutEngine::PrintLine(const char* Str, float2 WH, FontAsset* Font, float2 Offset, float2 Scale, bool CenterX, bool CenterY)
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

		for (auto& L : Temp) {
			L.A += Offset;
			L.B += Offset;

			L.A = Position2SS(L.A);
			L.B = Position2SS(L.B);
		}

		//PushLineSet2D(GUI, Temp);
	}


	/************************************************************************************************/

	void LayoutEngine::PushRect(Draw_RECT Rect)	{
		auto Offset = GetCurrentPosition();

		FK_ASSERT(0, "Not Implemented!");

		//Rect.BLeft  += Offset;
		//Rect.TRight += Offset;

		// Change Cordinate System From Top Down, to Buttom Up
		Draw_RECT Out = Rect;
		//Out.BLeft	= { Rect.BLeft[0], 1 - Rect.TRight[1] };
		//Out.TRight  = { Rect.TRight[0], 1 - Rect.BLeft[1] };
		//FlexKit::PushRect(GUI, Out);
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


	void GUIGrid::Update(GUIGridHandle Grid, LayoutEngine* LayoutEngine, double dt, const SimpleWindowInput in)
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
		FK_ASSERT(0, "Not Implemented!");

		//Draw_RECT Rect;
		//Rect.BLeft	= {0, 0};
		//Rect.TRight = Grid.GetWH();
		//Rect.Color	= Grid.GetBackgroundColor();

		//LayoutEngine->PushOffset(Grid.GetPosition());
		//LayoutEngine->PushRect(Rect);

		//auto ScanFunction = [&](GUIElementHandle hndl, FlexKit::LayoutEngine& Layout)
		//{
		//	Grid.mWindow->DrawElement(hndl, LayoutEngine);
		//};

		//ScanElements(Grid, *LayoutEngine, ScanFunction);

		//LayoutEngine->PopOffset();
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


			Line.A = float3{ 0, 0, 0 };
			Line.B = float3{ 0, ColumnHeight, 0 };
			Line.AColour = float3(0, 1, 0);
			Line.BColour = float3(1, 0, 0);

			Lines.push_back(Line);
		}

		{	// Draw Horizontal Lines
			float Y = 0;
			auto& Heights = Grid.RowHeights();

			for (auto& RowHeight : Heights) {
				float Width = RowHeight * ColumnHeight;
				Y += Width;

				LineSegment Line;
				Line.A		 = float3{ 0,			Y, 0 };
				Line.B		 = float3{ RowWidth,	Y, 0 };
				Line.AColour = float3(0, 0.5 + Y/2, 0);
				Line.BColour = float3(0, 0.5 + Y/2, 0);

				Lines.push_back(Line);
			}

			LineSegment Line;
			Line.A			= float3{ 0,		0, 0 };
			Line.B			= float3{ RowWidth,	0, 0 };
			Line.AColour	= float3(0, 1, 0);
			Line.BColour	= float3(1, 0, 0);

			Lines.push_back(Line);

			Line.A			= float3{ 0,		ColumnHeight, 0 };
			Line.B			= float3{ RowWidth,	ColumnHeight, 0 };
			Line.AColour	= float3(0, 1, 0);
			Line.BColour	= float3(1, 0, 0);

			Lines.push_back(Line);
		}

		LayoutEngine->PushLineSegments(Lines);

		auto ScanFunction = [&](GUIElementHandle hndl, FlexKit::LayoutEngine& Layout)
		{
			Grid.mWindow->DrawElement_DEBUG(hndl, LayoutEngine);
		};

		ScanElements(Grid, *LayoutEngine, ScanFunction);
		LayoutEngine->PopOffset();
	}


	/************************************************************************************************/


	void GUIButton::Draw(GUIButtonHandle button, LayoutEngine* Layout)
	{
		FK_ASSERT(0, "Not Implemented!");

		//Draw_RECT Rect;
		//Rect.BLeft	= { 0.0f, 0.0f };
		//Rect.TRight = button.WH();
		//Rect.Color  = float4(Grey(0.5f), 1.0f);
	
		//Layout->PushRect(Rect);

		//if(button.Text() && button._IMPL().Font)
		//	Layout->PrintLine(button.Text(), button.WH(), button._IMPL().Font, { 0, 0 }, {1, 1}, true, true);
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


	void GUIButton::Update(GUIButtonHandle Btn, LayoutEngine* LayoutEngine, double dt, const SimpleWindowInput in)
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


	void GUITextBox::Update(GUITextBoxHandle TextBox, LayoutEngine* LayoutEngine, double dT, const SimpleWindowInput Input)
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
		FK_ASSERT(0, "Not Implemented!");

		auto& Element = TextBox._IMPL();
		//Draw_RECT Rect;
		//Rect.BLeft  = { 0, 0 };
		//Rect.TRight = TextBox.WH();
		//Rect.Color  = Element.Highlighted ? Element.HighlightedColor : Element.Color;
		//Layout->PushRect(Rect);

		//if(strlen(Element.Text))
		//	Layout->PrintLine(Element.Text, TextBox.WH(), Element.Font, { 0, 0 }, {0.4f, 0.4f}, true, true);
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


	const GUIBaseElement GUIHandle::Framework() const		{ return mWindow->Elements[mBase];  }
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
		return float2 { _GetGrid().ColumnWidths[ID[0]], _GetGrid().RowHeights[ID[1]] } * _GetGrid().WH;
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
			W = Grid.WH[0]  / float(Columns);

		for (auto& H : Grid.RowHeights)
			H = Grid.WH[1] / float(Rows);
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


	GUIButtonHandle GUIGridHandle::CreateButton(uint2 CellID, const char* Str, FontAsset* font)
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


	GUITextBoxHandle GUIGridHandle::CreateTextBox(uint2 CellID, const char* Str, FontAsset* Font)
	{
		GUIElementHandle out = (uint32_t)-1;
		auto& Grid = _GetGrid();

		if (CellID[0] < Grid.ColumnWidths.size() &&
			CellID[1] < Grid.RowHeights.size())
		{
			auto TextBox = mWindow->CreateTextBox(mBase);

			float2 WH = GetCellWH(CellID);
			TextBox._IMPL().Dimensions		= WH;
			TextBox._IMPL().Color				= float4(Grey(0.5f), 1);

			TextBox._IMPL().Highlighted		= false;
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


	void GUITextBoxHandle::SetTextFont(FontAsset* Font)
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