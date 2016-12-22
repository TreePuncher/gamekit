/**********************************************************************

Copyright (c) 2015 - 2016 Robert May

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
#include "..\coreutilities\MathUtils.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\graphicsutilities\graphics.h"

#ifndef GUIUTILITIES_H
#define GUIUTILITIES_H

namespace FlexKit
{

	/************************************************************************************************/

	enum EGUI_ELEMENT_TYPE
	{
		EGE_VLIST,
		EGE_HLIST,
		EGE_HSLIDER,
		EGE_VSLIDER,
		EGE_DIVIDER,
		EGE_BUTTON_TEXTURED,
		EGE_BUTTON_TEXT,
		EGE_TEXTINPUT,
	};

	struct Row
	{
		size_t	MaxColumns;
		float	Columns[10];
	};

	struct GUIElement
	{
		EGUI_ELEMENT_TYPE	Type;
		size_t				Index;
		uint2				Position;
		uint2				RC;// ROW - COLUMN
	};

	typedef DynArray<size_t> GUIChildList;

	struct GUIElement_List{
		GUIElement_List(GUIElement_List&& in) {
			WH			= in.WH;
			Color		= in.Color;
			Children	= std::move(in.Children);
		}

		GUIElement_List& operator = (const GUIElement_List& in)
		{
			WH		 = in.WH;
			Color	 = in.Color;
			Children = in.Children;
			return *this;
		}

		GUIElement_List(const GUIElement_List& in)	{
			WH			= in.WH;
			Color		= in.Color;
			Children	= in.Children;
		}

		GUIElement_List() {}

		uint2			WH;
		float4			Color;
		GUIChildList	Children;
	};

	// CallBacks Definitions
	typedef void(*EnteredEventFN)	( 				 void* _ptr, size_t GUIElement );
	typedef void(*ButtonEventFN)	( 				 void* _ptr, size_t GUIElement );
	typedef void(*TextInputEventFN)	( char*, size_t, void* _ptr, size_t GUIElement );
	typedef void(*SliderEventFN)	( float,		 void* _ptr, size_t GUIElement );

	struct TextInputState{
		bool RemainActive = true;
		bool TextChanged  = false;
	};

	typedef TextInputState (*TextInputUpdateFN) ( TextArea* TextArea, char* str, size_t strBufferSize, void* _ptr );

	struct GUIElement_TexuredButton
	{
		bool				Active				= false;
		bool				Entered				= false;

		float2				WH					= {};
		float4				ActiveColor			= {WHITE, 1.0f};
		float4				InActiveColor		= {WHITE, 1.0f};
		Texture2D*			Texture_InActive	= nullptr;
		Texture2D*			Texture_Active		= nullptr;

		void*				_ptr				= nullptr;
		EnteredEventFN		OnClicked_CB		= nullptr;
		ButtonEventFN		OnEntered_CB		= nullptr;
		ButtonEventFN		OnExit_CB			= nullptr;
	};

	struct GUIElement_TextButton
	{
		bool				Active	= false;
		bool				Entered = false;

		float2				WH;
		TextArea			Text;
		FontAsset*			Font;
		Texture2D*			BackgroundTexture	= nullptr;

		void*				CB_Args				= nullptr;
		EnteredEventFN		OnClicked_CB		= nullptr;
		ButtonEventFN		OnEntered_CB		= nullptr;
		ButtonEventFN		OnExit_CB			= nullptr;
	};

	struct GUIElement_TextInput
	{
		bool				Active			= false;
		float4				ActiveColor		= { Gray(0.35f), 1 };
		float4				InActiveColor	= { Gray(0.5f), 1 };
		float2				WH;

		Texture2D*			Texture;
		char*				TextSTR;

		TextArea			TextGUI;
		FontAsset*			Font;
		size_t				MaxTextLength;

		void*				CB_Args;
		TextInputUpdateFN	OnTextUpdate;
		TextInputEventFN	OnTextEntered;
		TextInputEventFN	OnTextChanged;

		iAllocator*		Memory;
	};

	struct GUIElement_TextBox
	{
		float2		WH;
		Texture2D*	Texture;

		char*		Text;
		size_t		TextLength;

		iAllocator*	Memory;
	};

	struct GUIElement_Slider
	{
		float2			WH;
		float			BarRatio;
		float			SliderPosition;
		bool			Active		= false;
		bool			Entered		= false;

		SliderEventFN OnClicked_CB = nullptr;
		SliderEventFN OnMoved_CB   = nullptr;
		SliderEventFN OnEntered_CB = nullptr;
		SliderEventFN OnExit_CB	   = nullptr;
		void*		  CB_Args	   = nullptr;

	};

	struct GUIElement_RadioButton
	{
		float2			WH;

		void*			_ptr;
		ButtonEventFN	OnChanged;
	
		bool Toggled;
	};

	struct GUIElement_Graph
	{
	};

	struct GUIElement_Div
	{
		float2 Size;
	};


	/************************************************************************************************/


	typedef uint32_t GUIElementHandle;

	struct SimpleWindow
	{
		GUIChildList						Root;
		DynArray<GUIElement>				Elements;
		DynArray<GUIElement_List>			Lists;
		DynArray<GUIElement_TexuredButton>	TexturedButtons;
		DynArray<GUIElement_TextButton>		TextButtons;
		DynArray<GUIElement_TextInput>		TextInputs;
		DynArray<GUIElement_TextBox>		TextBoxes;
		DynArray<GUIElement_Slider>			Slider;
		DynArray<GUIElement_RadioButton>	RadioButton;
		DynArray<GUIElement_Graph>			Graphs;
		DynArray<GUIElement_Div>			Dividers;

		DynArray<int32_t> CellState;// Marks Owner, -1 if Unused

		size_t RowCount;
		size_t ColumnCount;

		float2 TitleBarWH;
		float2 CellBorder;

		float4 PanelColor;
		float4 CellColor;

		float2 WH;
		float2 Position;
		float  AspectRatio;

#if 0
		static_vector<float, 16> RHeights;
		static_vector<float, 16> CWidths;
#endif
		iAllocator*	Memory;
	};


	/************************************************************************************************/
	

	struct SimpleWindowInput
	{
		float2	MousePosition;
		bool	LeftMouseButtonPressed;
	};

	struct GUIList
	{
		uint2  Position = { 0, 0 };			// Will Use Cell
		uint2  WH		= { 1, 1 };			// Number of cells will occupy
		float4 Color	= float4(WHITE, 1);	// W Value will be alpha
	};


	struct GUITexturedButton_Desc
	{
		ButtonEventFN OnClicked_CB = nullptr;
		ButtonEventFN OnEntered_CB = nullptr;
		ButtonEventFN OnExit_CB	   = nullptr;

		void* CB_Args;

		Texture2D*	Texture_Active		= nullptr;
		Texture2D*	Texture_InActive	= nullptr;
		float2		WH;

		GUITexturedButton_Desc& SetButtonSizeByPixelSize(float2 PixelSize, uint2 PixelWH)
		{
			WH = PixelSize * PixelWH;
			return *this;
		}
	};


	struct GUITextButton_Desc
	{
		ButtonEventFN OnClicked_CB = nullptr;
		ButtonEventFN OnEntered_CB = nullptr;
		ButtonEventFN OnExit_CB	   = nullptr;
		void*		  CB_Args;

		float2		WH;
		float2		WindowSize;
		char*		Text;
		FontAsset*	Font;


		GUITextButton_Desc& SetButtonSizeByPixelSize(float2 PixelSize, uint2 PixelWH)
		{
			WH = PixelSize * PixelWH;
			return *this;
		}
	};


	struct GUITextInput_Desc
	{
		float2		WH;

		float2		WindowSize;
		char*		Text;
		FontAsset*	Font;

		void*				CB_Args			= nullptr;
		EnteredEventFN		OnClicked_CB	= nullptr;
		ButtonEventFN		OnEntered_CB	= nullptr;
		ButtonEventFN		OnExit_CB		= nullptr;
		TextInputUpdateFN	OnTextUpdate	= nullptr;

		GUITextInput_Desc& SetTextBoxSizeByPixelSize(float2 PixelSize, uint2 PixelWH) {
			WH = PixelSize * PixelWH;
			return *this;
		}
	};


	struct GUISlider_Desc
	{
		SliderEventFN OnClicked_CB = nullptr;
		SliderEventFN OnMoved_CB   = nullptr;
		SliderEventFN OnEntered_CB = nullptr;
		SliderEventFN OnExit_CB	   = nullptr;
		void*		  CB_Args	   = nullptr;

		float		InitialPosition	= 0.5f;
		float		BarRatio;
		float2		WH;
	};


	struct SimpleWindow_Desc
	{
		SimpleWindow_Desc(float width, float height, size_t ColumnCount, size_t RowCount = 1, size_t  = 1, float aspectratio = 1.0f) :
			RowCount		(RowCount),
			ColumnCount		(ColumnCount),
			Width			(width),
			Height			(height),
			AspectRatio		(aspectratio)
		{}

		float	Width;
		float	Height;
		float	AspectRatio;

		size_t	RowCount;
		size_t	ColumnCount;

		float2  CellDividerSize;
	};

	struct ParentInfo 
	{
		float2 ClipArea_BL;
		float2 ClipArea_TR;
		float2 ClipArea_WH;
		float2 ParentPosition;
	};

	enum EDrawDirection
	{
		DD_Rows,
		DD_Columns,
		DD_Grid,
	};

	enum EOverFlowOption
	{
		OF_DrawOutsize,
		OF_DrawSlider,
		OF_DrawClip,
		OF_Exlude,
	};

	struct FormattingOptions
	{
		EDrawDirection	DrawDirection	 = DD_Columns;
		EOverFlowOption	OverflowHandling = OF_Exlude;

		float2			CellDividerSize	= float2(0.01f, 0.01f);
	};


	/************************************************************************************************/


	FLEXKITAPI void		InitiateSimpleWindow	( iAllocator*		Memory,		SimpleWindow* Out,		SimpleWindow_Desc& Desc );
	FLEXKITAPI void		CleanUpSimpleWindow		( SimpleWindow* Out );
	FLEXKITAPI void		DrawChildren			( GUIChildList&		Elements,	SimpleWindow* Window,	GUIRender* Out, ParentInfo& P, FormattingOptions& Formatting = FormattingOptions());
	FLEXKITAPI void		DrawSimpleWindow		( SimpleWindowInput	Input,		SimpleWindow* Window,	GUIRender* Out );

	FLEXKITAPI void		UpdateSimpleWindow		( SimpleWindowInput* Input, SimpleWindow* Window);

	FLEXKITAPI GUIElementHandle	SimpleWindowAddVerticalList		( SimpleWindow*	Window, GUIList& Desc, GUIElementHandle Parent = -1 );
	FLEXKITAPI GUIElementHandle	SimpleWindowAddHorizonalList	( SimpleWindow*	Window, GUIList& Desc, GUIElementHandle Parent = -1 );

	FLEXKITAPI GUIElementHandle	SimpleWindowAddTexturedButton	( SimpleWindow*	Window, GUITexturedButton_Desc&	Desc,					GUIElementHandle Parent = -1);
	FLEXKITAPI GUIElementHandle	SimpleWindowAddTextButton		( SimpleWindow*	Window, GUITextButton_Desc&		Desc, RenderSystem* RS, GUIElementHandle Parent = -1);

	FLEXKITAPI GUIElementHandle	SimpleWindowAddTextInput		( SimpleWindow*	Window, GUITextInput_Desc&		Desc, RenderSystem* RS, GUIElementHandle Parent = -1);


	FLEXKITAPI void				SimpleWindowAddDivider			( SimpleWindow*	Window, float2 Size,				  GUIElementHandle Parent );

	FLEXKITAPI GUIElementHandle	SimpleWindowAddHorizontalSlider ( SimpleWindow*	Window, GUISlider_Desc			Desc, GUIElementHandle Parent);


	FLEXKITAPI bool	CursorInside	 ( float2 CursorPos, float2 POS, float2 WH );
	FLEXKITAPI bool	CursorInsideCell ( SimpleWindow* Window, float2 CursorPos, uint2 Cell);

	FLEXKITAPI void				SetCellState ( uint2 Cell, int32_t S, SimpleWindow* W);
	FLEXKITAPI void				SetCellState ( uint2 Cell, uint2 WH, int32_t S, SimpleWindow* W);
	FLEXKITAPI GUIElementHandle	GetCellState ( uint2 Cell, SimpleWindow* W);

	FLEXKITAPI void SetSliderPosition(float R, GUIElementHandle, SimpleWindow* W);


	/************************************************************************************************/
}

#endif