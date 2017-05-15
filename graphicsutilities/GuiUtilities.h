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
		EGE_GRID,
		EGE_VLIST,
		EGE_HLIST,
		EGE_HSLIDER,
		EGE_VSLIDER,
		EGE_DIVIDER,
		EGE_BUTTON_TEXTURED,
		EGE_BUTTON_TEXT,
		EGE_TEXTINPUT,
	};

	enum EGUI_FORMATTING
	{
		EGF_FORMAT_DEFAULT = 0,
		EGF_FORMAT_CENTER  = 0x01,
		EGF_FORMAT_LEFT	   = 0x02,
		EGF_FORMAT_RIGHT   = 0x04,
		EGF_FORMAT_TOP	   = 0x08,
		EGF_FORMAT_BOTTOM  = 0x0F,
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

	typedef Vector<size_t> GUIChildList;

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
	typedef bool (*EnteredEventFN)	( 				 void* _ptr, size_t GUIElement ); // If True Begins Immediate Return
	typedef bool (*ButtonEventFN)	( 				 void* _ptr, size_t GUIElement ); // If True Begins Immediate Return
	typedef bool (*TextInputEventFN)( char*, size_t, void* _ptr, size_t GUIElement ); // If True Begins Immediate Return
	typedef bool (*SliderEventFN)	( float,		 void* _ptr, size_t GUIElement ); // If True Begins Immediate Return

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
		float4				BackGroundColor;
		float4				TextColor;
		TextArea			Text;
		FontAsset*			Font;
		Texture2D*			BackgroundTexture	= nullptr;

		void*				CB_Args				= nullptr;
		EnteredEventFN		OnClicked_CB		= nullptr;
		ButtonEventFN		OnEntered_CB		= nullptr;
		ButtonEventFN		OnExit_CB			= nullptr;

		EGUI_FORMATTING		Formatting = EGF_FORMAT_DEFAULT;
	};

	struct GUIElement_TextInput
	{
		bool				Active			= false;
		float4				ActiveColor		= { Grey(0.35f), 1 };
		float4				InActiveColor	= { Grey(0.5f), 1 };
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
		Vector<GUIElement>				Elements;
		Vector<GUIElement_List>			Lists;
		Vector<GUIElement_TexuredButton>	TexturedButtons;
		Vector<GUIElement_TextButton>		TextButtons;
		Vector<GUIElement_TextInput>		TextInputs;
		Vector<GUIElement_TextBox>		TextBoxes;
		Vector<GUIElement_Slider>			Slider;
		Vector<GUIElement_RadioButton>	RadioButton;
		Vector<GUIElement_Graph>			Graphs;
		Vector<GUIElement_Div>			Dividers;

		Vector<int32_t> CellState;// Marks Owner, -1 if Unused

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
		float2	CursorWH;
		bool	LeftMouseButtonPressed;
	};

	struct GUIList
	{
		uint2  Position = { 0, 0 };			// Cell Coordinate
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

		float2			WH;				// Width Heigth of Button in Pixels
		float2			WindowSize;		// RenderTarget Size
		float4			BackGroundColor = float4(Grey(.5), 1);
		float4			TextColor	    = float4(WHITE, 1);
		char*			Text;			// Text To Be Displayed
		FontAsset*		Font;			// Font to be used during Rendering
		float2			CharacterScale   = {1.0f, 1.0f}; // Increase Size of Characters by this Factor	

		EGUI_FORMATTING Placement = EGUI_FORMATTING::EGF_FORMAT_DEFAULT;

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
		SimpleWindow_Desc(float width, float height, size_t ColumnCount, size_t RowCount = 1, float aspectratio = 1.0f) :
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
	FLEXKITAPI void		CleanUpSimpleWindow		( SimpleWindow* Out, RenderSystem* RS = nullptr);
	FLEXKITAPI void		DrawChildren			( GUIChildList&		Elements,	SimpleWindow* Window,	ImmediateRender* Out, ParentInfo& P, FormattingOptions& Formatting = FormattingOptions());
	FLEXKITAPI void		DrawSimpleWindow		( SimpleWindowInput	Input,		SimpleWindow* Window,	ImmediateRender* Out );

	FLEXKITAPI void		UpdateSimpleWindow		( SimpleWindowInput* Input, SimpleWindow* Window);

	FLEXKITAPI GUIElementHandle	SimpleWindowAddVerticalList		( SimpleWindow*	Window, GUIList& Desc, GUIElementHandle Parent = -1 );
	FLEXKITAPI GUIElementHandle	SimpleWindowAddHorizonalList	( SimpleWindow*	Window, GUIList& Desc, GUIElementHandle Parent = -1 );

	FLEXKITAPI GUIElementHandle	SimpleWindowAddTexturedButton	( SimpleWindow*	Window, GUITexturedButton_Desc&	Desc,					GUIElementHandle Parent = -1);
	FLEXKITAPI GUIElementHandle	SimpleWindowAddTextButton		( SimpleWindow*	Window, GUITextButton_Desc&		Desc, RenderSystem* RS, GUIElementHandle Parent = -1);

	FLEXKITAPI GUIElementHandle	SimpleWindowAddTextInput		( SimpleWindow*	Window, GUITextInput_Desc&		Desc, RenderSystem* RS, GUIElementHandle Parent = -1);


	FLEXKITAPI void				SimpleWindowAddDivider			( SimpleWindow*	Window, float2 Size,				  GUIElementHandle Parent );

	FLEXKITAPI GUIElementHandle	SimpleWindowAddHorizontalSlider ( SimpleWindow*	Window, GUISlider_Desc			Desc, GUIElementHandle Parent);


	FLEXKITAPI bool	CursorInside	 ( float2 CursorPos, float2 CursorWH, float2 POS, float2 WH );
	FLEXKITAPI bool	CursorInsideCell ( SimpleWindow* Window, float2 CursorPos, float2 CursorWH, uint2 Cell);

	FLEXKITAPI void				SetCellState ( uint2 Cell, int32_t S, SimpleWindow* W);
	FLEXKITAPI void				SetCellState ( uint2 Cell, uint2 WH, int32_t S, SimpleWindow* W);
	FLEXKITAPI GUIElementHandle	GetCellState ( uint2 Cell, SimpleWindow* W);

	FLEXKITAPI void SetSliderPosition(float R, GUIElementHandle, SimpleWindow* W);


	/************************************************************************************************/


	class ComplexGUI;

	struct GUIGrid;
	struct GUIGridCell;

	struct GUIButton;
	class GUIButtonHandle;


	/************************************************************************************************/


	struct LayoutEngine
	{
		LayoutEngine(iAllocator* Memory, RenderSystem* RS, ImmediateRender* GUI);

		RenderSystem*		RS;
		ImmediateRender*	GUI;
		iAllocator*			Memory;

		Vector<float2, 128> PositionStack;
		
		float2 GetCurrentPosition();

		static float2 Position2SS(float2);
		static float3 Position2SS(float3);

		void PrintLine(const char* Str, float2 WH, FontAsset* Font, float2 Offset = {0.0f, 0.0f});

		void PushLineSegments	( FlexKit::LineSegments& );
		void PushRect			( Draw_RECT Rect );
		void PushOffset			( float2 XY );
		void PopOffset			( );

		void Begin		();
		void End		();
	};


	/************************************************************************************************/


	struct GUIBaseElement
	{
		size_t Index;
		size_t UpdatePriority;

		bool				Active;
		EGUI_ELEMENT_TYPE	Type;
	};


	/************************************************************************************************/


	struct GUIDimension
	{
		float D;
		GUIDimension& operator = (float f) { D = f; return *this; }
		operator float () {
			return D;
		}
	};
	
	
	/************************************************************************************************/


	struct GUIButton
	{
		GUIButton() :
			CellID			({}),
			Dimensions		({}),
			Entered			(nullptr),
			Clicked			(nullptr),
			Released		(nullptr),
			Hover			(nullptr),
			Text			(nullptr),
			Memory			(nullptr),
			USR				(nullptr),
			HoverDuration	(0.0),
			HoverLength		(1.0f),
			ClickState		(false)
		{}


		uint2	CellID;
		float2	Dimensions;

		EnteredEventFN	Entered;
		ButtonEventFN	Clicked;
		ButtonEventFN	Released;
		ButtonEventFN	Hover;	

		const char* Text; 
		FontAsset*	Font;
		iAllocator*	Memory;

		void*	USR;
		double	HoverDuration;
		double	HoverLength;
		bool	ClickState;

		static void Draw		(GUIButtonHandle button, LayoutEngine* Layout);
		static void Draw_DEBUG	(GUIButtonHandle button, LayoutEngine* Layout);

		static void Update(GUIButtonHandle Grid, LayoutEngine* LayoutEngine, double dt, const SimpleWindowInput in);
	};


	/************************************************************************************************/


	class GUIHandle
	{
	public:
		GUIHandle() {}
		GUIHandle(ComplexGUI* window, GUIElementHandle hndl) : mWindow(window), mBase(hndl) {}

		GUIElementHandle	mBase;
		ComplexGUI*			mWindow;

		void SetActive(bool);

		const EGUI_ELEMENT_TYPE	Type();
		const EGUI_ELEMENT_TYPE	Type() const;

		const GUIBaseElement	Framework() const;
		GUIBaseElement&			Framework();
	};


	/************************************************************************************************/


	class GUIGridHandle : public GUIHandle
	{
	public:
		GUIGridHandle(GUIHandle);
		GUIGridHandle(ComplexGUI* Window, GUIElementHandle In);

		Vector<GUIDimension>&		RowHeights();
		Vector<GUIDimension>&		ColumnWidths();
		GUIGridCell*				CreateCell(uint2);
		GUIGridCell&				GetCell(uint2 ID, bool& Found);
		float2						GetCellWH(uint2 ID);

		float2						GetPosition();
		float2						GetChildPosition(GUIElementHandle Element);

		void						SetPosition(float2 XY);

		GUIGrid&					_GetGrid();

		operator GUIElementHandle () { return mBase; }

		void resize(float Width_percent, float Height_percent);
		void SetGridDimensions(size_t Width, size_t Height);
		void SetCellFormatting(uint2, EDrawDirection);
		

		GUIButtonHandle CreateButton(uint2 CellID, const char* Str = nullptr, FontAsset* font = nullptr);
	};


	/************************************************************************************************/


	class GUIButtonHandle : public GUIHandle
	{
	public:
		GUIButtonHandle(ComplexGUI* Window, GUIElementHandle In);

		operator GUIElementHandle () { return mBase; }

		void resize(float Width_percent, float Height_percent);
		void SetCellID(uint2 CellID);

		float2 WH();

		const char* Text();
		GUIButton& _IMPL();

	private:
	};


	/************************************************************************************************/


	struct GUIGridCell
	{
		uint2			ID;
		uint32_t		Children;
		EDrawDirection	StackFormatting;
	};

	struct GUIGrid
	{
		GUIGrid() : 
			RowHeights	(nullptr), 
			ColumnWidths(nullptr),
			Cells		(nullptr),
			XY			(0.0f, 0.0f),
			Framework	((uint32_t)-1) {}


		GUIGrid( iAllocator* memory, uint32_t base ) : 
			RowHeights		(memory), 
			ColumnWidths	(memory),
			Cells			(memory),
			XY				(0.0f, 0.0f),
			Framework		(base) {}


		GUIGrid(const GUIGrid& rhs)
		{
			RowHeights		= rhs.RowHeights;
			ColumnWidths	= rhs.ColumnWidths;
			Cells			= rhs.Cells;
			Framework		= rhs.Framework;
			XY				= rhs.XY;
		}


		static void Update		( GUIGridHandle Grid, LayoutEngine* LayoutEngine, double dt, const SimpleWindowInput in );
		static void Draw		( GUIGridHandle Grid, LayoutEngine* LayoutEngine );
		static void Draw_DEBUG	( GUIGridHandle Grid, LayoutEngine* LayoutEngine );

		Vector<GUIDimension>	RowHeights;
		Vector<GUIDimension>	ColumnWidths;
		Vector<GUIGridCell>	Cells;
		float2					WH;
		float2					XY;
		uint32_t				Framework;
	};


	/************************************************************************************************/


	class ComplexGUI
	{
	public:
		ComplexGUI ( iAllocator* memory );
		ComplexGUI ( const ComplexGUI& );

		void Release();

		void Update		( double dt, const SimpleWindowInput in );
		void Upload		( RenderSystem* RS, ImmediateRender* out );

		void Draw		( RenderSystem* RS, ImmediateRender* out );
		void Draw_DEBUG	( RenderSystem* RS, ImmediateRender* out );

		void DrawElement		( GUIElementHandle Element, LayoutEngine* Layout );
		void DrawElement_DEBUG	( GUIElementHandle Element, LayoutEngine* Layout );

		void UpdateElement		( GUIElementHandle Element, LayoutEngine* Layout, double dt, const	SimpleWindowInput Input );

		GUIGridHandle	CreateGrid	( uint2 ID = {0, 0} );
		GUIGridHandle	CreateGrid	( GUIElementHandle	Parent, uint2 ID = {0, 0} );
		GUIButtonHandle CreateButton( GUIElementHandle Parent );

		void CreateTexturedButton();

		void CreateTextBox();
		void CreateTextInputBox();

		void CreateHorizontalSlider();
		void CreateVerticalSlider();

		Vector<GUIBaseElement>		Elements;
		Vector<GUIGrid>				Grids;
		Vector<GUIButton>				Buttons;

		Vector<Vector<GUIElementHandle>>	Children;

		iAllocator* Memory;
	};


}	/************************************************************************************************/

#endif