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

#include <functional>

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
		EGE_TEXTBOX,
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
	typedef std::function<bool (char*, size_t,	size_t GUIElement)>	TextInputEventFN;
	typedef std::function<bool (				size_t GUIElement)> EnteredEventFN;
	typedef std::function<bool (				size_t GUIElement)> GenericGUIEventFN;
	typedef std::function<bool (float,			size_t GUIElement)> SliderEventFN;

	typedef uint32_t GUIElementHandle;

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
		GenericGUIEventFN	OnEntered_CB		= nullptr;
		GenericGUIEventFN	OnExit_CB			= nullptr;
	};


	struct GUIElement_TextButton
	{
		bool				Active	= false;
		bool				Entered = false;

		float2				WH;
		float4				BackGroundColor;
		float4				TextColor;
		TextArea			Text;
		SpriteFontAsset*	Font;
		Texture2D*			BackgroundTexture	= nullptr;

		void*				CB_Args				= nullptr;
		EnteredEventFN		OnClicked_CB		= nullptr;
		GenericGUIEventFN		OnEntered_CB		= nullptr;
		GenericGUIEventFN		OnExit_CB			= nullptr;

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
		SpriteFontAsset*	Font;
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
		GenericGUIEventFN	OnChanged;
	
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
	

	struct WindowInput
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
		GenericGUIEventFN OnClicked_CB = nullptr;
		GenericGUIEventFN OnEntered_CB = nullptr;
		GenericGUIEventFN OnExit_CB	   = nullptr;

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
		GenericGUIEventFN OnClicked_CB = nullptr;
		GenericGUIEventFN OnEntered_CB = nullptr;
		GenericGUIEventFN OnExit_CB	   = nullptr;
		void*		  CB_Args;

		float2				WH;				// Width Heigth of Button in Pixels
		float2				WindowSize;		// RenderTarget Size
		float4				BackGroundColor = float4(Grey(.5), 1);
		float4				TextColor	    = float4(WHITE, 1);
		char*				Text;			// Text To Be Displayed
		SpriteFontAsset*	Font;			// Font to be used during Rendering
		float2				CharacterScale	= {1.0f, 1.0f}; // Increase Size of Characters by this Factor	

		EGUI_FORMATTING Placement = EGUI_FORMATTING::EGF_FORMAT_DEFAULT;

		GUITextButton_Desc& SetButtonSizeByPixelSize(float2 PixelSize, uint2 PixelWH)
		{
			WH = PixelSize * PixelWH;
			return *this;
		}
	};


	struct GUITextInput_Desc
	{
		float2				WH;

		float2				WindowSize;
		char*				Text;
		SpriteFontAsset*	Font;

		void*				CB_Args			= nullptr;
		EnteredEventFN		OnClicked_CB	= nullptr;
		GenericGUIEventFN	OnEntered_CB	= nullptr;
		GenericGUIEventFN	OnExit_CB		= nullptr;
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


	class ComplexGUI;

	struct GUIGrid;
	struct GUIGridCell;

	struct GUIButton;
	class GUIButtonHandle;


	/************************************************************************************************/


	struct LayoutEngine_Desc
	{
		FrameGraph*				FrameGraph;
		ConstantBufferHandle	ConstantBuffer;
		TextureHandle			RenderTarget;
		VertexBufferHandle		VertexBuffer;
		VertexBufferHandle		TextBuffer;
		float2					PixelSize;
	};


	struct LayoutEngine
	{
		struct Draw_RECT
		{
			float2 XY;
			float2 WH;
			float4 Color;
		};

		LayoutEngine(iAllocator* tempmemory, iAllocator* Memory, LayoutEngine_Desc& RS);

		FrameGraph*				FrameGraph;
		TextureHandle			RenderTarget;
		VertexBufferHandle		VertexBuffer;
		VertexBufferHandle		TextBuffer;
		ConstantBufferHandle	ConstantBuffer;

		iAllocator*			Memory;
		iAllocator*			TempMemory;

		Vector<float2, 128> PositionStack;
		
		float2 PixelSize;
		float2 GetCurrentPosition();

		static float2 Position2SS(float2);
		static float3 Position2SS(float3);

		void PrintLine(
			const char* Str, 
			float2 WH, 
			SpriteFontAsset* Font, 
			float2 Offset	= { 0.0f, 0.0f }, 
			float2 Scale	= { 1.0f, 1.0f },
			bool CenterX	= false, 
			bool CenterY	= false);

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
		GUIDimension(float f = 0.0f) : D{ f } {}

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

		EnteredEventFN		Entered;
		GenericGUIEventFN	Clicked;
		GenericGUIEventFN	Released;
		GenericGUIEventFN	Hover;	

		const char*			Text; 
		SpriteFontAsset*	Font;
		iAllocator*			Memory;

		void*	USR;
		double	HoverDuration;
		double	HoverLength;
		bool	ClickState;

		static void Draw		( GUIButtonHandle button, LayoutEngine* Layout );
		static void Draw_DEBUG	( GUIButtonHandle button, LayoutEngine* Layout );

		static void Update		( GUIButtonHandle Grid, LayoutEngine* LayoutEngine, double dt, const WindowInput in );
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

	struct GUITextBox;

	class GUITextBoxHandle : public GUIHandle
	{
	public:
		GUITextBoxHandle	( GUIHandle );
		GUITextBoxHandle	( ComplexGUI* Window, GUIElementHandle In );
		
		void SetText	( const char* Text );
		void SetTextFont( SpriteFontAsset* Font );
		void SetCellID	( uint2 CellID );

		float2 WH();

		GUITextBox& _IMPL();
	};


	/************************************************************************************************/


	struct GUITextBox
	{
		uint2	CellID;
		float2	Dimensions;
		float4	Color;
		float4  HighlightedColor;

		EnteredEventFN		Entered;
		GenericGUIEventFN	Clicked;
		GenericGUIEventFN	Released;
		GenericGUIEventFN	Hover;

		float	HoverDuration	= 0;
		bool	Highlighted		= false;
		bool	ClickState		= false;

		const char*			Text;
		float4				TextColor;
		SpriteFontAsset*	Font;
		iAllocator*		Memory;

		static void Update		(GUITextBoxHandle  TextBox, LayoutEngine* Layout, double dT, const WindowInput Input);
		static void Draw		(GUITextBoxHandle  button, LayoutEngine* Layout);
		static void Draw_DEBUG	(GUITextBoxHandle  button, LayoutEngine* Layout);
	};


	/************************************************************************************************/


	class GUIGridHandle : public GUIHandle
	{
	public:
		GUIGridHandle(GUIHandle);
		GUIGridHandle(ComplexGUI* Window, GUIElementHandle In);

		Vector<GUIDimension>&		RowHeights		();
		Vector<GUIDimension>&		ColumnWidths	();
		GUIGridCell*				CreateCell		(uint2);
		GUIGridCell&				GetCell			(uint2 ID, bool& Found);
		float2						GetCellWH		(uint2 ID);


		float2						GetWH();
		float2						GetPosition();
		float2						GetChildPosition(GUIElementHandle Element);
		float4						GetBackgroundColor();

		void						SetBackgroundColor	(float4 K);
		void						SetPosition			(float2 XY);

		GUIGrid&							_GetGrid();
		Vector<Vector<GUIElementHandle>>	_GetChildren();

		operator GUIElementHandle () { return mBase; }

		void resize				( float Width_percent, float Height_percent );
		void SetGridDimensions	( size_t Width, size_t Height );
		void SetCellFormatting	( uint2, EDrawDirection );
		

		GUIButtonHandle		CreateButton	(uint2 CellID, const char* Str = nullptr, SpriteFontAsset* font = nullptr);
		GUITextBoxHandle	CreateTextBox	(uint2 CellID, const char* Str = nullptr, SpriteFontAsset* font = nullptr);
	};


	/************************************************************************************************/


	class GUIButtonHandle : public GUIHandle
	{
	public:
		GUIButtonHandle(ComplexGUI* Window, GUIElementHandle In);

		operator GUIElementHandle () { return mBase; }

		void resize(float Width_percent, float Height_percent);
		void SetCellID(uint2 CellID);
		void SetUSR(void*);

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
			RowHeights			(nullptr), 
			ColumnWidths		(nullptr),
			Cells				(nullptr),
			XY					(0.0f, 0.0f),
			Framework			((uint32_t)-1),
			BackgroundColor		(Grey(0.5f), 1),
			WH					{1, 1}
		{}


		GUIGrid( iAllocator* memory, uint32_t base ) : 
			RowHeights		(memory), 
			ColumnWidths	(memory),
			Cells			(memory),
			XY				(0.0f, 0.0f),
			Framework		(base),
			BackgroundColor	(Grey(0.5f), 1),
			WH				{1, 1}
		{}


		GUIGrid(const GUIGrid& rhs)
		{
			RowHeights		= rhs.RowHeights;
			ColumnWidths	= rhs.ColumnWidths;
			Cells			= rhs.Cells;
			Framework		= rhs.Framework;
			XY				= rhs.XY;
			BackgroundColor	= rhs.BackgroundColor;
			WH				= rhs.WH;
		}

		template<typename FN>
		static void ScanElements(GUIGridHandle Grid, LayoutEngine& LayoutEngine, FN ScanFunction)
		{
			float Y = 0;
			uint32_t Y_ID = 0;

			float RowWidth		= Grid._GetGrid().WH[0];
			float ColumnHeight	= Grid._GetGrid().WH[1];

			for (auto& h : Grid.RowHeights())
			{
				uint32_t  X_ID = 0;

				for (auto& w : Grid.ColumnWidths()) {
					bool Result = false;
					auto& Cell	= Grid.GetCell({ X_ID, Y_ID }, Result);
					if (Result)
					{
						auto& Children = Grid.mWindow->Children[Cell.Children];
						if (&Children && Children.size())
							for (auto& C : Children)
								ScanFunction(C, LayoutEngine);
					}

					LayoutEngine.PushOffset({ w * RowWidth, 0 });
					X_ID++;
				}

				for (auto& w : Grid.ColumnWidths())
					LayoutEngine.PopOffset();

				LayoutEngine.PushOffset({ 0, h * ColumnHeight });
				Y_ID++;
			}

			for (auto& h : Grid.RowHeights())
				LayoutEngine.PopOffset();
		}

		static void Update		( GUIGridHandle Grid, LayoutEngine* LayoutEngine, double dt, const WindowInput in );
		static void Draw		( GUIGridHandle Grid, LayoutEngine* LayoutEngine );
		static void Draw_DEBUG	( GUIGridHandle Grid, LayoutEngine* LayoutEngine );

		Vector<GUIDimension>	RowHeights;
		Vector<GUIDimension>	ColumnWidths;
		Vector<GUIGridCell>		Cells;
		float2					WH;
		float2					XY;
		float4					BackgroundColor;
		uint32_t				Framework;
	};


	/************************************************************************************************/


	struct DrawUI_Desc
	{
		FrameGraph*				FrameGraph;
		TextureHandle			RenderTarget;
		VertexBufferHandle		VertexBuffer;
		VertexBufferHandle		TextBuffer;
		ConstantBufferHandle	ConstantBuffer;
	};


	class ComplexGUI
	{
	public:
		ComplexGUI ( iAllocator* memory );
		ComplexGUI ( const ComplexGUI& );

		~ComplexGUI() { Release(); }


		void Release();

		void Update		( double dt, const WindowInput in, float2 PixelSize, iAllocator* TempMemory );

		void Draw			(DrawUI_Desc& Desc, iAllocator* Temp);
		void Draw_DEBUG		(DrawUI_Desc& Desc,	iAllocator* Temp);

		void DrawElement		( GUIElementHandle Element, LayoutEngine* Layout );
		void DrawElement_DEBUG	( GUIElementHandle Element, LayoutEngine* Layout );

		void UpdateElement		( GUIElementHandle Element, LayoutEngine* Layout, double dt, const	WindowInput Input );

		GUIGridHandle		CreateGrid		( uint2 ID = {0, 0}, uint2 CellCounts = {1, 1} );
		GUIGridHandle		CreateGrid		( GUIElementHandle	Parent, uint2 ID = {0, 0} );
		GUIButtonHandle		CreateButton	( GUIElementHandle Parent );
		GUITextBoxHandle	CreateTextBox	( GUIElementHandle Parent );
		void				CreateTextInputBox();

		// to be implemented
		void CreateTexturedButton();
		void CreateHorizontalSlider();
		void CreateVerticalSlider();

		Vector<GUIBaseElement>				Elements;
		Vector<GUIGrid>						Grids;
		Vector<GUIButton>					Buttons;
		Vector<GUITextBox>					TextBoxes;
		Vector<Vector<GUIElementHandle>>	Children;

		iAllocator*							Memory;
	};


}	/************************************************************************************************/

#endif