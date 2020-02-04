/**********************************************************************

Copyright (c) 2015 - 2019 Robert May

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

	typedef Vector<size_t>	GUIChildList;
	typedef size_t			UIElementID_t;
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
	typedef std::function<bool (char*, size_t)>	TextInputEventFN;
	typedef std::function<bool ()>				EnteredEventFN;
	typedef std::function<bool ()>				GenericGUIEventFN;
	typedef std::function<bool (float)>			SliderEventFN;

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


	class GuiSystem;

	struct GUIGrid;
	struct GUIGridCell;


	/************************************************************************************************/


	struct LayoutEngine_Desc
	{
		FrameGraph*				FrameGraph;
		ConstantBufferHandle	ConstantBuffer;
		ResourceHandle			RenderTarget;
		VertexBufferHandle		VertexBuffer;
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
		ResourceHandle			RenderTarget;
		VertexBufferHandle		VertexBuffer;
		ConstantBufferHandle	ConstantBuffer;

		iAllocator*			Memory;
		iAllocator*			TempMemory;

		Vector<float2>	PositionStack;
		Vector<float2>	AreaStack;
		
		float2 PixelSize;
		float2 GetCurrentPosition();
		float2 GetDrawArea();

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

		void PushDrawArea(float2 XY);
		void PopDrawArea();

		bool IsInside(float2 POS, float2 WH);

		void Begin		();
		void End		();
	};


	/************************************************************************************************/


	class GUIElement
	{
	public:
		GUIElement(const UIElementID_t id, EGUI_ELEMENT_TYPE in_type) :
			type			{ in_type	},
			elementID		{ id		},	
			active			{ false		}, 
			cellID			{ 0u, 0u	},
			updatePriority	{ 0			}{}

		virtual ~GUIElement() {};

		virtual void Update	(LayoutEngine* layoutEngine, double dt, const WindowInput& input) = 0;
		virtual void Draw	(LayoutEngine* LayoutEngine) = 0;

		// No slicing
					GUIElement	(const GUIElement&)		= delete;
		GUIElement& operator =	(const GUIElement&)		= delete;

		// No Moving
					GUIElement	(GUIElement&&)			= delete;
		GUIElement& operator =	(const GUIElement&&)	= delete;

		uint2					cellID;
		const UIElementID_t		elementID;
		size_t					updatePriority;
		bool					active;
		const EGUI_ELEMENT_TYPE	type;
	};


	/************************************************************************************************/


	struct GUIDimension
	{
		GUIDimension(float f = 0.0f) : D{ f } {}

		GUIDimension& operator = (float f) { D = f; return *this; }

		operator float () const {
			return D;
		}

		float D;
	};
	
	
	/************************************************************************************************/


	class GUIButton : public GUIElement
	{
	public:
		GUIButton(UIElementID_t in_ID) :
			GUIElement		{ in_ID, EGUI_ELEMENT_TYPE::EGE_BUTTON_TEXT},

			Color			{ Grey(0.1f), 1 },
			WH				{ 1.0f, 1.0f	},
			Entered			{ nullptr		},
			Clicked			{ nullptr		},
			Released		{ nullptr		},
			Hover			{ nullptr		},
			Text			{ nullptr		},
			Memory			{ nullptr		},
			USR				{ nullptr		},
			HoverDuration	{ 0.0			},
			HoverLength		{ 1.0f			},
			ClickState		{ false			}{}


		~GUIButton() override {}
		

		void Update	(LayoutEngine* layoutEngine, double dt, const WindowInput& input) override;
		void Draw	(LayoutEngine* LayoutEngine) override;

		EnteredEventFN		Entered;
		GenericGUIEventFN	Clicked;
		GenericGUIEventFN	Released;
		GenericGUIEventFN	Hover;	

		typedef std::function<
			void(GUIButton& button, LayoutEngine* LayoutEngine)>										FN_CustomDraw;
		typedef std::function<
			void(GUIButton& button, LayoutEngine* layoutEngine, double dt, const WindowInput& input)>	FN_CustomUpdate;

		FN_CustomDraw	customDraw;
		FN_CustomUpdate customUpdate;

		float2				WH;
		float4				Color;
		const char*			Text;
		SpriteFontAsset*	Font;
		iAllocator*			Memory;

		void*	USR;
		double	HoverDuration;
		double	HoverLength;
		bool	ClickState;
	};


	/************************************************************************************************/


	class GUITextBox : public GUIElement
	{
	public:
		GUITextBox(UIElementID_t in_ID) : 
			GUIElement	{ in_ID, EGUI_ELEMENT_TYPE::EGE_TEXTBOX },
			Dimensions	{ 0.0f, 0.0f }
		{}

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

		void Update(LayoutEngine* layoutEngine, double dt, const WindowInput& input) override
		{

		}


		void Draw(LayoutEngine* LayoutEngine) override
		{
			FK_ASSERT(0, "Not Implemented!");
		}

		//static void Update		(LayoutEngine* Layout, double dT, const WindowInput Input);
		//static void Draw		(LayoutEngine* Layout);
		//static void Draw_DEBUG	(LayoutEngine* Layout);
	};


	/************************************************************************************************/


	struct GUIGridCell
	{
		uint2			ID;
		uint32_t		Children;
		EDrawDirection	StackFormatting;
	};

	struct GUIGrid : public GUIElement
	{
		GUIGrid(UIElementID_t in_id, iAllocator* allocator ) :
			GUIElement			{ in_id, EGUI_ELEMENT_TYPE::EGE_GRID	},
			Children			{ allocator								},
			RowHeights			{ allocator								},
			ColumnWidths		{ allocator								},
			Cells				{ allocator								},
			XY					{ 0.0f, 0.0f							},
			framework			{ (uint32_t) -1							},
			BackgroundColor		{ Grey(0.5f), 1							},
			WH					{ 1, 1									}
		{}

		GUIGrid(const GUIGrid& rhs) = delete;

		template<typename FN>
		void Visit(LayoutEngine& layoutEngine, FN Visitor)
		{
			float Y			= 0;
			uint32_t Y_ID	= 0;

			float2 Area = layoutEngine.GetDrawArea();

			for (const auto& h : RowHeights)
			{
				uint32_t  X_ID = 0;

				for (const auto& w : ColumnWidths) {
					GUIGridCell* Cell = nullptr;

					if (GetCell({ X_ID, Y_ID }, Cell))
					{
						auto const CurrentID = uint2{ X_ID, Y_ID };

						layoutEngine.PushDrawArea(GetCellDimension(CurrentID));
						layoutEngine.PushOffset(GetCellPosition(CurrentID) * Area);

						for (auto& C : Children)
						{
							if (C->cellID == CurrentID)
								Visitor(C, CurrentID);
						}

						layoutEngine.PopOffset();
						layoutEngine.PopDrawArea();
					}
					X_ID++;
				}

				Y_ID++;
			}
		}


		void SetGridDimensions(uint2 CR)// CR = {Columns, Rows}
		{
			ColumnWidths.resize(CR[0]);
			RowHeights.resize(CR[1]);

			for (auto& W : ColumnWidths)
				W = float(WH[0]) / float(CR[0]);

			for (auto& H : RowHeights)
				H = float(WH[1]) / float(CR[1]);

			Cells.resize(CR.Product());

			for (auto Column = 0u; Column < CR[0]; ++Column)
			{
				for (auto Row = 0u; Row < CR[1]; ++Row)
				{
					GUIGridCell Cell = 
					{
						{ Column, Row},
						0u,
						DD_Grid
					};

					Cells[Column * CR[1] + Row] = Cell;
				}
			}
		}

		void Update(LayoutEngine* layoutEngine, double dt, const WindowInput& windowInput) override
		{
			layoutEngine->PushOffset(XY);
			layoutEngine->PushDrawArea(WH);

			Visit(*layoutEngine, [&](auto Child, auto ID)
				{
					Child->Update(layoutEngine, dt, windowInput);
				});

			layoutEngine->PopDrawArea();
			layoutEngine->PopOffset();
		}

		void Draw(LayoutEngine* LayoutEngine) override
		{
			Debug_Draw(LayoutEngine);
		}

		void Debug_Draw(LayoutEngine* LayoutEngine)
		{
			float3 Area = { LayoutEngine->GetDrawArea(), 0 };
			LayoutEngine->PushOffset(XY * Area);
			LayoutEngine->PushDrawArea(WH);

			Visit(
				*LayoutEngine,
				[&](auto* Element, const auto cellID)
				{
					Element->Draw(LayoutEngine);

				});


			float RowWidth		= WH[0];
			float ColumnHeight	= WH[1];
			Vector<LineSegment> Lines(LayoutEngine->Memory);

			{	// Draw Vertical Lines
				float X = 0;

				for (const auto& ColumnWidth : ColumnWidths) {
					float Width = ColumnWidth;
					X += Width;

					LineSegment Line;
					Line.A       = float3{ X, 0, 0};
					Line.B       = float3{ X, 1, 0};
					Line.AColour = float3{ 1, 1, 1};
					Line.BColour = float3{ 1, 1, 1};

					Lines.push_back(Line);
				}

				LineSegment Line;
				Line.A       = float3{ 0, 0, 0};
				Line.B       = float3{ 0, 1, 0};
				Line.AColour = float3{ 1, 0, 0};
				Line.BColour = float3{ 0, 1, 0};

				Lines.push_back(Line);


				Line.A			= float3{0, 0, 0};
				Line.B			= float3{0, 1, 0};
				Line.AColour	= float3{1, 1, 1};
				Line.BColour	= float3{1, 1, 1};

				Lines.push_back(Line);
			}

			{	// Draw Horizontal Lines
				float Y = 0;
				const auto& Heights = RowHeights;

				for (const auto& RowHeight : Heights) {
					float Width = RowHeight;
					Y += Width;

					LineSegment Line;
					Line.A		 = float3{ 0,	Y,		0};
					Line.B		 = float3{ 1,	Y,		0};
					Line.AColour = float3{ 1,	1.0f,	1};
					Line.BColour = float3{ 1,	1.0f,	1};

					Lines.push_back(Line);
				}

				LineSegment Line;
				Line.A			= float3{0, 0, 0};
				Line.B			= float3{1, 0, 0};
				Line.AColour	= float3{1, 1, 1};
				Line.BColour	= float3{1, 1, 1};

				Lines.push_back(Line);

				Line.A			= float3{0, 1, 0};
				Line.B			= float3{1, 1, 0};
				Line.AColour	= float3{1, 1, 1};
				Line.BColour	= float3{1, 1, 1};

				Lines.push_back(Line);
			}

			LayoutEngine->PushLineSegments(Lines);

			LayoutEngine->PopDrawArea();
			LayoutEngine->PopOffset();
		}


		void AddChild(GUIElement* element, uint2 ID)
		{
			GUIGridCell* Cell = nullptr;
			if (GetCell(ID, Cell))
				Cell->Children++;

			Children.push_back(element);
		}


		void RemoveChild(GUIElement* element)
		{
			Children.erase(std::remove(Children.begin(), Children.end(), element), Children.end());
		}


		bool GetCell(uint2 ID, GUIGridCell*& Cell)
		{
			auto res = find(Cells, [&](auto& I) { return I.ID == ID; });
			bool Found = res != Cells.end();

			if(res)
				Cell = res;

			return Found;
		}


		float2 GetCellDimension(const uint2 cellID)
		{
			return { ColumnWidths[cellID[0]], RowHeights[cellID[1]] };
		}


		float2 GetCellPosition(const uint2 cellID)
		{
			float CellX = 0;
			float CellY = 0;

			// Get X Position
			for (size_t idx = 0; idx < cellID[0]; ++idx)
				CellX += ColumnWidths[idx];

			// Get Y Position
			for (size_t idx = 0; idx < cellID[1]; ++idx)
				CellY += RowHeights[idx];

			return {CellX, CellY};
		}


		Vector<GUIDimension>	RowHeights;
		Vector<GUIDimension>	ColumnWidths;
		Vector<GUIGridCell>		Cells;
		Vector<GUIElement*>		Children;
		float2					WH;
		float2					XY; // Relative to parent
		float4					BackgroundColor;
		uint32_t				framework;
	};


	/************************************************************************************************/


	struct DrawUI_Desc
	{
		FrameGraph*				FrameGraph;
		ResourceHandle			RenderTarget;
		VertexBufferHandle		VertexBuffer;
		ConstantBufferHandle	ConstantBuffer;
	};


	struct GuiSystem_Desc
	{
		size_t GridCount	= 10;
		size_t ButtonCount	= 25;
		size_t TextBoxes	= 25;
	};


	/************************************************************************************************/


	class GuiSystem
	{
	public:
		GuiSystem ( GuiSystem_Desc desc, iAllocator* memory );
		GuiSystem ( GuiSystem&& ); // Moves are allowed

		~GuiSystem() { Release(); }

		void Release();

		void Update		( double dt, const WindowInput in, float2 PixelSize, iAllocator* TempMemory );

		void Draw			(DrawUI_Desc& Desc, iAllocator* Temp);
		void Draw_DEBUG		(DrawUI_Desc& Desc,	iAllocator* Temp);

		void DrawElement		( GUIElementHandle Element, LayoutEngine* Layout );
		void DrawElement_DEBUG	( GUIElementHandle Element, LayoutEngine* Layout );

		void UpdateElement		( GUIElementHandle Element, LayoutEngine* Layout, double dt, const	WindowInput Input );

		GUIGrid&			CreateGrid		( GUIGrid* Parent = nullptr, uint2 ID = {0, 0} );
		GUIButton&			CreateButton	( GUIGrid* Parent, uint2 CellID = { 0, 0 });
		GUITextBox&			CreateTextBox	( GUIElement* Parent );
		void				CreateTextInputBox();

		void Release(GUIButton*);

		// to be implemented
		void CreateTexturedButton();
		void CreateHorizontalSlider();
		void CreateVerticalSlider();

		ObjectPool<GUIGrid>					Grids;
		ObjectPool<GUITextBox>				TextBoxes;
		ObjectPool<GUIButton>				Buttons;

		Vector<GUIElement*>					Elements;
		iAllocator*							Memory;
	};


}	/************************************************************************************************/

#endif
