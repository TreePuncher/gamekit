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


#ifndef IMMEDIATERENDERING_H
#define IMMEDIATERENDERING_H

#include "graphics.h"

namespace FlexKit
{

	/************************************************************************************************/


	enum DRAWCALLTYPE : size_t
	{
		DCT_2DRECT,
		DCT_2DRECT_TEXTURED,
		DCT_2DRECT_CLIPPED,
		DCT_LINES2D,
		DCT_LINES3D,
		DCT_TEXT,
		DCT_TEXT2,
		DCT_COUNT,
	};

	struct ClipArea
	{
		float2 CLIPAREA_BLEFT;
		float2 CLIPAREA_TRIGHT;
	};

	struct Draw_RECT
	{
		float2 TRight;
		float2 BLeft;
		float4 Color;
	};

	struct Draw_RECT_CLIPPED
	{
		float2 TRight;
		float2 BLeft;
		float4 Color;
		float2 CLIPAREA_TRIGHT;
		float2 CLIPAREA_BLEFT;
	};

	struct Draw_RECTPoint
	{
		float2 V;
		float2 UV;
		float4 Color;
	};

	struct Draw_TEXTURED_RECT
	{
		float2			TRight;
		float2			BLeft;
		float2			TLeft_UVOffset;
		float2			BRight_UVOffset;
		float4			Color;
		uint32_t		ZOrder;
		Texture2D*		TextureHandle;
	};

	struct Textured_Rect
	{
		Texture2D*	Texture;
		float2		CLIPAREA_BLEFT;
		float2		CLIPAREA_TRIGHT;
		float4		Color;
	};

	struct Draw_LineSet
	{
		size_t Begin, Count;
	};

	struct Draw_TEXT
	{
		float2 CLIPAREA_BLEFT;
		float2 CLIPAREA_TRIGHT;
		float4 Color;

		TextArea*	Text;
		FontAsset*	Font;
	};

	struct Draw_TEXT2
	{
		size_t		Begin, Count;
		FontAsset*	Font;
		size_t		Pad;
	};

	struct DrawCall
	{
		DRAWCALLTYPE	Type;
		size_t			Index;
	};

	struct PrintTextFormatting
	{
		static PrintTextFormatting DefaultParams()
		{
			return 
			{
				{0, 0},
				{1, 1},
				{1, 1},
				{1, 1},
				float4(WHITE, 1),
				false, false
			};
		}

		float2 StartingPOS;
		float2 TextArea;
		float2 Scale;
		float2 PixelSize;
		float4 Color;

		bool CenterX;
		bool CenterY;

	};

	struct PrintState
	{
		float		CurrentX;
		float		CurrentY;
		float		YAdvance;
		Vector<TextEntry> TextBuffer;
	};

	const uint32_t TEXTBUFFERMINSIZE = 1024;

	// TODO: replace and remove with new Frame Graph based functions
	struct ImmediateRender
	{
		operator ImmediateRender* () { return this; }
		RenderSystem*				RS;
		iAllocator*					TempMemory;

		Vector<ClipArea>			ClipAreas;
		Vector<Draw_RECTPoint>		Rects;
		Vector<Textured_Rect>		TexturedRects;
		Vector<Draw_TEXT>			Text;
		Vector<Draw_TEXT2>			Text2;
		Vector<Draw_LineSet>		DrawLines2D;
		Vector<Draw_LineSet>		DrawLines3D;
		Vector<DrawCall>			DrawCalls;

		Vector<TextEntry>		TextBuffer;
		size_t					TextBufferPosition;
		ShaderResourceBuffer	TextBufferGPU;
		uint32_t				TextBufferSizes[3];

		LineSet						Lines2D;
		LineSet						Lines3D;

		FrameBufferedResource		RectBuffer;
		ID3D12PipelineState*		DrawStates[DRAWCALLTYPE::DCT_COUNT];
	};


	/************************************************************************************************/


	FLEXKITAPI void PushCircle2D( ImmediateRender* RG, iAllocator* Memory, float2 POS = {0.0f, 0.0f},		float r = 1.0f, float2 Scale = { 1, 1 },	float3 Color = WHITE );
	FLEXKITAPI void PushCircle3D( ImmediateRender* RG, iAllocator* Memory, float3 POS = {0.0f, 0.0f, 0.0f},	float r = 1.0f, float3 Scale = { 1, 1, 1 }, float3 Color = WHITE );

	FLEXKITAPI void PushCube_Wireframe		( ImmediateRender* RG, iAllocator* Memory, float3 POS = {0.0f, 0.0f, 0.0f}, float r = 1.0f,				 float3 Color = WHITE );
	FLEXKITAPI void PushCapsule_Wireframe	( ImmediateRender* RG, iAllocator* Memory, float3 POS = {0.0f, 0.0f, 0.0f}, float r = 1.0f, float h = 0, float3 Color = WHITE );

	FLEXKITAPI void PushBox_WireFrame( ImmediateRender* RG, iAllocator* Memory, float3 POS = 0, Quaternion Q = Quaternion::Identity(), float3 BoxDim = 1, float3 Color = WHITE);

	FLEXKITAPI void PushRect( ImmediateRender* RG, Draw_RECT Rect );
	FLEXKITAPI void PushRect( ImmediateRender* RG, Draw_RECT_CLIPPED Rect );
	FLEXKITAPI void PushRect( ImmediateRender* RG, Draw_TEXTURED_RECT Rect );

	FLEXKITAPI void PushText( ImmediateRender* RG, Draw_TEXT Text );

	FLEXKITAPI void PushLineSet2D( ImmediateRender* RG, LineSegments );
	FLEXKITAPI void PushLineSet3D( ImmediateRender* RG, LineSegments );
	
	FLEXKITAPI void PrintText					( ImmediateRender* IR, const char* str, FontAsset* Font, PrintTextFormatting& Formatting, iAllocator* TempMemory );
	FLEXKITAPI void PrintText					( ImmediateRender* IR, const char* str, FontAsset* Font, PrintTextFormatting& Formatting, iAllocator* TempMemory, PrintState& State, bool End = false );
	FLEXKITAPI void PrintText					( ImmediateRender* IR, const char* str, FontAsset* Font, float2 StartPOS, float2 TextArea, float4 Color, float2 PixelSize );

	FLEXKITAPI void InitiateImmediateRender		( RenderSystem* RS, ImmediateRender* RG, iAllocator* Memory, iAllocator* TempMemory);
	FLEXKITAPI void ReleaseDrawImmediate		( RenderSystem* RS, ImmediateRender* RG);

	FLEXKITAPI void DrawImmediate	( RenderSystem* RS, ID3D12GraphicsCommandList* CL, ImmediateRender* GUIStack, Texture2D Out, Camera* C );
	FLEXKITAPI void Clear			( ImmediateRender* RG );
	FLEXKITAPI void UploadImmediate	( RenderSystem* RS, ImmediateRender* RG, iAllocator* TempMemory, RenderWindow* TargetWindow);

	FLEXKITAPI void DEBUG_DrawCameraFrustum ( ImmediateRender* Render, Camera* C );


	/************************************************************************************************/

}

#endif