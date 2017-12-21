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

#include "ImmediateRendering.h"


#include <memory>
#include <Windows.h>
#include <windowsx.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <d3d11sdklayers.h>
#include <d3d11shader.h>

namespace FlexKit
{
	/************************************************************************************************/


	void PushCircle2D(ImmediateRender* RG, iAllocator* Memory, float2 POS, float r, float2 Scale, float3 Color)
	{
		LineSegments Lines(Memory);

		const size_t SegmentCount = 32;
		for (size_t I = 0; I < SegmentCount; ++I) {
			LineSegment	Line;
			Line.AColour = Color;
			Line.BColour = Color;

			float x1 = POS.x + Scale.x * r * std::cos(2 * pi / SegmentCount * I) - .5f;
			float y1 = POS.x + Scale.y * r * std::sin(2 * pi / SegmentCount * I) - .5f;

			float x2 = POS.x + Scale.x * r * std::cos(2 * pi / SegmentCount * (I + 1)) - .5f;
			float y2 = POS.x + Scale.y * r * std::sin(2 * pi / SegmentCount * (I + 1)) - .5f;

			Line.A = float3(x1, y1 , 0);
			Line.B = float3(x2, y2, 0);

			Lines.push_back(Line);
		}

		PushLineSet2D(RG, Lines);
	}


	/************************************************************************************************/


	void PushBox_WireFrame(ImmediateRender* RG, iAllocator* Memory, float3 POS, Quaternion Q, float3 BoxDim, float3 Color)
	{
		const float3 HalfDim = BoxDim / 2;

		float3 Points[8] = {
			Q * (HalfDim * float3(-1,  1,  1)),// NearTopLeft
			Q * (HalfDim * float3( 1,  1,  1)),// Nea1Top1ight
			Q * (HalfDim * float3(-1, -1,  1)),// Nea1BottomLeft
			Q * (HalfDim * float3( 1, -1,  1)),// Nea1Bottom1ight

			Q * (HalfDim * float3(-1,  1, -1)),// Fa1TopLeft
			Q * (HalfDim * float3( 1,  1, -1)),// Fa1Top1ight
			Q * (HalfDim * float3(-1, -1, -1)),// Fa1BottomLeft
			Q * (HalfDim * float3( 1, -1, -1)),// Fa1Bottom1ight
		};

		LineSegments Lines(Memory);

		LineSegment	Line;
		Line.AColour = Color;
		Line.BColour = Color;

		// Add Edges
		{
			// Top Near Edge
			Line.A = Points[0] + POS;
			Line.B = Points[1] + POS;
			Lines.push_back(Line);

			// Top Far Edge
			Line.A = Points[4] + POS;
			Line.B = Points[5] + POS;
			Lines.push_back(Line);

			// Top Left Edge
			Line.A = Points[0] + POS;
			Line.B = Points[4] + POS;
			Lines.push_back(Line);

			// Top Right Edge
			Line.A = Points[1] + POS;
			Line.B = Points[5] + POS;
			Lines.push_back(Line);

			// Bottom Near Edge
			Line.A = Points[2] + POS;
			Line.B = Points[3] + POS;
			Lines.push_back(Line);

			// Bottom Far Edge
			Line.A = Points[6] + POS;
			Line.B = Points[7] + POS;
			Lines.push_back(Line);

			// Bottom Left Edge
			Line.A = Points[2] + POS;
			Line.B = Points[6] + POS;
			Lines.push_back(Line);

			// Bottom Right Edge
			Line.A = Points[3] + POS;
			Line.B = Points[7] + POS;
			Lines.push_back(Line);

			// Middle Near Left Edge
			Line.A = Points[0] + POS;
			Line.B = Points[2] + POS;
			Lines.push_back(Line);

			// Middle Near Right Edge
			Line.A = Points[1] + POS;
			Line.B = Points[3] + POS;
			Lines.push_back(Line);

			// Middle Far Left Edge
			Line.A = Points[4] + POS;
			Line.B = Points[6] + POS;
			Lines.push_back(Line);

			// Middle Far Right Edge
			Line.A = Points[5] + POS;
			Line.B = Points[7] + POS;
			Lines.push_back(Line);
		}
		PushLineSet3D(RG, Lines);
	}


	/************************************************************************************************/


	void PushCircle3D(ImmediateRender* RG, iAllocator* Memory, float3 POS, float r, float3 Scale, float3 Color)
	{
		LineSegments Lines(Memory);

		const size_t SegmentCount = 64;
		for (size_t I = 0; I < SegmentCount; ++I) {
			LineSegment	Line;
			Line.AColour = Color;
			Line.BColour = Color;

			float x1 = Scale.x * r * std::cos(2 * pi / SegmentCount * I);
			float y1 = Scale.y * r * std::sin(2 * pi / SegmentCount * I);

			float x2 = Scale.x * r * std::cos(2 * pi / SegmentCount * (I + 1));
			float y2 = Scale.y * r * std::sin(2 * pi / SegmentCount * (I + 1));

			Line.A = float3(x1, y1, 0) + POS;
			Line.B = float3(x2, y2, 0) + POS;

			Lines.push_back(Line);
		}

		for (size_t I = 0; I < SegmentCount; ++I) {
			LineSegment	Line;
			Line.AColour = Color;
			Line.BColour = Color;

			float x1 = Scale.x * r * std::cos(2 * pi / SegmentCount * I);
			float z1 = Scale.z * r * std::sin(2 * pi / SegmentCount * I);

			float x2 = Scale.x * r * std::cos(2 * pi / SegmentCount * (I + 1));
			float z2 = Scale.z * r * std::sin(2 * pi / SegmentCount * (I + 1));

			Line.A = float3(x1, 0, z1) + POS;
			Line.B = float3(x2, 0, z2) + POS;

			Lines.push_back(Line);
		}

		for (size_t I = 0; I < SegmentCount; ++I) {
			LineSegment	Line;
			Line.AColour = Color;
			Line.BColour = Color;

			float y1 = Scale.x * r * std::cos(2 * pi / SegmentCount * I);
			float z1 = Scale.z * r * std::sin(2 * pi / SegmentCount * I);

			float y2 = Scale.x * r * std::cos(2 * pi / SegmentCount * (I + 1));
			float z2 = Scale.z * r * std::sin(2 * pi / SegmentCount * (I + 1));

			Line.A = float3(0, y1, z1) + POS;
			Line.B = float3(0, y2, z2) + POS;

			Lines.push_back(Line);
		}

		PushLineSet3D(RG, Lines);
	}


	/************************************************************************************************/


	void PushCube_Wireframe(ImmediateRender* RG, iAllocator* Memory, float3 POS, float r, float3 Color)
	{
		float3 Points[8] = {
			float3(-r,  r,  r),// NearTopLeft
			float3( r,  r,  r),// NearTopRight
			float3(-r, -r,  r),// NearBottomLeft
			float3( r, -r,  r),// NearBottomRight

			float3(-r,  r, -r),// FarTopLeft
			float3( r,  r, -r),// FarTopRight
			float3(-r, -r, -r),// FarBottomLeft
			float3( r, -r, -r),// FarBottomRight
		};

		LineSegments Lines(Memory);

		LineSegment	Line;
		Line.AColour = Color;
		Line.BColour = Color;

		// Add Edges
		{
			// Top Near Edge
			Line.A = Points[0] + POS;
			Line.B = Points[1] + POS;
			Lines.push_back(Line);

			// Top Far Edge
			Line.A = Points[4] + POS;
			Line.B = Points[5] + POS;
			Lines.push_back(Line);

			// Top Left Edge
			Line.A = Points[0] + POS;
			Line.B = Points[4] + POS;
			Lines.push_back(Line);

			// Top Right Edge
			Line.A = Points[1] + POS;
			Line.B = Points[5] + POS;
			Lines.push_back(Line);

			// Bottom Near Edge
			Line.A = Points[2] + POS;
			Line.B = Points[3] + POS;
			Lines.push_back(Line);

			// Bottom Far Edge
			Line.A = Points[6] + POS;
			Line.B = Points[7] + POS;
			Lines.push_back(Line);

			// Bottom Left Edge
			Line.A = Points[2] + POS;
			Line.B = Points[6] + POS;
			Lines.push_back(Line);

			// Bottom Right Edge
			Line.A = Points[3] + POS;
			Line.B = Points[7] + POS;
			Lines.push_back(Line);

			// Middle Near Left Edge
			Line.A = Points[0] + POS;
			Line.B = Points[2] + POS;
			Lines.push_back(Line);

			// Middle Near Right Edge
			Line.A = Points[1] + POS;
			Line.B = Points[3] + POS;
			Lines.push_back(Line);

			// Middle Far Left Edge
			Line.A = Points[4] + POS;
			Line.B = Points[6] + POS;
			Lines.push_back(Line);

			// Middle Far Right Edge
			Line.A = Points[5] + POS;
			Line.B = Points[7] + POS;
			Lines.push_back(Line);
		}

		PushLineSet3D(RG, Lines);
	}


	/************************************************************************************************/


	void PushCapsule_Wireframe(ImmediateRender* RG, iAllocator* Memory, float3 POS, float r, float h, float3 Color)
	{
		LineSegments Lines(Memory);

		const size_t SegmentCount = 16;

		float TopHemiSphereOffSet = r + h;

		// Draw HemiSpheres
		for (size_t I = 0; I < SegmentCount; ++I) {
			LineSegment	Line;
			Line.AColour = Color;
			Line.BColour = Color;

			float x1 = r * std::cos(pi / SegmentCount * I);
			float y1 = r * std::sin(pi / SegmentCount * I);

			float x2 = r * std::cos(pi / SegmentCount * (I + 1));
			float y2 = r * std::sin(pi / SegmentCount * (I + 1));

			Line.A = float3(x1, y1 + TopHemiSphereOffSet, 0) + POS;
			Line.B = float3(x2, y2 + TopHemiSphereOffSet, 0) + POS;

			Lines.push_back(Line);
		}

		for (size_t I = 0; I < 2 * SegmentCount; ++I) {
			LineSegment	Line;
			Line.AColour = Color;
			Line.BColour = Color;

			float x1 = r * std::cos(2 * pi / SegmentCount * I);
			float z1 = r * std::sin(2 * pi / SegmentCount * I);

			float x2 = r * std::cos(2 * pi / SegmentCount * (I + 1));
			float z2 = r * std::sin(2 * pi / SegmentCount * (I + 1));

			Line.A = float3(x1, TopHemiSphereOffSet, z1) + POS;
			Line.B = float3(x2, TopHemiSphereOffSet, z2) + POS;

			Lines.push_back(Line);
		}

		for (size_t I = 0; I < SegmentCount; ++I) {
			LineSegment	Line;
			Line.AColour = Color;
			Line.BColour = Color;

			float y1 = r * std::sin(pi / SegmentCount * I);
			float z1 = r * std::cos(pi / SegmentCount * I);

			float y2 = r * std::sin(pi / SegmentCount * (I + 1));
			float z2 = r * std::cos(pi / SegmentCount * (I + 1));

			Line.A = float3(0, y1 + TopHemiSphereOffSet, z1) + POS;
			Line.B = float3(0, y2 + TopHemiSphereOffSet, z2) + POS;

			Lines.push_back(Line);
		}

		// Draw Bottom HemiSphere
		for (size_t I = 0; I < SegmentCount; ++I) {
			LineSegment	Line;
			Line.AColour = Color;
			Line.BColour = Color;

			float x1 = r * std::cos(pi + pi / SegmentCount * I);
			float y1 = r * std::sin(pi + pi / SegmentCount * I);

			float x2 = r * std::cos(pi + pi / SegmentCount * (I + 1));
			float y2 = r * std::sin(pi + pi / SegmentCount * (I + 1));

			Line.A = float3(x1, y1 + r, 0) + POS;
			Line.B = float3(x2, y2 + r, 0) + POS;

			Lines.push_back(Line);
		}

		for (size_t I = 0; I < 2 * SegmentCount; ++I) {
			LineSegment	Line;
			Line.AColour = Color;
			Line.BColour = Color;

			float x1 = r * std::cos(2 * pi / SegmentCount * I);
			float z1 = r * std::sin(2 * pi / SegmentCount * I);

			float x2 = r * std::cos(2 * pi / SegmentCount * (I + 1));
			float z2 = r * std::sin(2 * pi / SegmentCount * (I + 1));

			Line.A = float3(x1, r, z1) + POS;
			Line.B = float3(x2, r, z2) + POS;

			Lines.push_back(Line);
		}

		for (size_t I = 0; I < SegmentCount; ++I) {
			LineSegment	Line;
			Line.AColour = Color;
			Line.BColour = Color;

			float y1 = r * std::sin(pi + pi / SegmentCount * I);
			float z1 = r * std::cos(pi + pi / SegmentCount * I);

			float y2 = r * std::sin(pi + pi / SegmentCount * (I + 1));
			float z2 = r * std::cos(pi + pi / SegmentCount * (I + 1));

			Line.A = float3(0, y1 + r, z1) + POS;
			Line.B = float3(0, y2 + r, z2) + POS;

			Lines.push_back(Line);
		}

		// Draw Edges
		{
			LineSegment	Line;
			Line.AColour = Color;
			Line.BColour = Color;

			// Left Edge
			Line.A = float3(-r, h + r,	0) + POS;
			Line.B = float3(-r, r,		0) + POS;
			Lines.push_back(Line);

			// Right Edge
			Line.A = float3( r, h + r,	0) + POS;
			Line.B = float3( r, r,		0) + POS;
			Lines.push_back(Line);

			// Near Edge
			Line.A = float3( 0, h + r,	r) + POS;
			Line.B = float3( 0, r,		r) + POS;
			Lines.push_back(Line);

			// Far Edge
			Line.A = float3( 0, h + r,	-r) + POS;
			Line.B = float3( 0, r,		-r) + POS;
			Lines.push_back(Line);
		}


		PushLineSet3D(RG, Lines);
	}

	/************************************************************************************************/


	void PushRect(ImmediateRender* RG, Draw_RECT Rect) 
	{
		RG->DrawCalls.push_back({ DRAWCALLTYPE::DCT_2DRECT , 0 });
		Draw_RECTPoint P;
		P.Color = F4MUL(Rect.Color, Rect.Color);

		//
		P.V  = WStoSS({ Rect.TRight.x, Rect.BLeft.y });
		P.UV = { 1, 1 };
		RG->Rects.push_back(P);// Bottom Right
		P.V  = WStoSS(Rect.BLeft);
		P.UV = { 0, 1 };
		RG->Rects.push_back(P);// Bottom Left
		P.V  = WStoSS(Rect.TRight);
		P.UV = { 1, 0 };
		RG->Rects.push_back(P);// Top Right
		//

		//
		P.V  = WStoSS(Rect.TRight);
		P.UV = { 1, 0 };
		RG->Rects.push_back(P);// Top Right
		P.V  = WStoSS(Rect.BLeft);
		P.UV = { 0, 1 };
		RG->Rects.push_back(P);// Bottom Left
		P.V  = WStoSS({ Rect.BLeft.x, Rect.TRight.y });
		P.UV = { 0, 0 };
		RG->Rects.push_back(P);// Top Left
		//
	}


	/************************************************************************************************/


	void PushRect(ImmediateRender* RG, Draw_RECT_CLIPPED Rect)
	{
		RG->DrawCalls.push_back({ DRAWCALLTYPE::DCT_2DRECT_CLIPPED , RG->ClipAreas.size() });
		RG->ClipAreas.push_back({ Rect.CLIPAREA_BLEFT , Rect.CLIPAREA_TRIGHT });

		Draw_RECTPoint P;
		P.Color = F4MUL(Rect.Color, Rect.Color);

		//
		P.V = WStoSS({ Rect.TRight.x, Rect.BLeft.y });
		P.UV = { 1, 1 };
		RG->Rects.push_back(P);// Bottom Right
		P.V = WStoSS(Rect.BLeft);
		P.UV = { 0, 1 };
		RG->Rects.push_back(P);// Bottom Left
		P.V = WStoSS(Rect.TRight);
		P.UV = { 1, 0 };
		RG->Rects.push_back(P);// Top Right
		//

		//
		P.V = WStoSS(Rect.TRight);
		P.UV = { 1, 0 };
		RG->Rects.push_back(P);// Top Right
		P.V = WStoSS(Rect.BLeft);
		P.UV = { 0, 1 };
		RG->Rects.push_back(P);// Bottom Left
		P.V = WStoSS({ Rect.BLeft.x, Rect.TRight.y });
		P.UV = { 0, 0 };
		RG->Rects.push_back(P);// Top Left
		//
	}


	/************************************************************************************************/


	void PushRect(ImmediateRender* RG, Draw_TEXTURED_RECT Rect)
	{
		RG->DrawCalls.push_back		({ DRAWCALLTYPE::DCT_2DRECT_TEXTURED , RG->TexturedRects.size() });
		RG->TexturedRects.push_back	({ Rect.TextureHandle, Rect.BLeft, Rect.TRight });

		Draw_RECTPoint P;
		P.Color = F4MUL(Rect.Color, Rect.Color);

		//
		P.V = WStoSS({ Rect.TRight.x, Rect.BLeft.y });
		P.UV = { 1, 1 };
		RG->Rects.push_back(P);// Bottom Right
		P.V = WStoSS(Rect.BLeft);
		P.UV = { 0, 1 };
		RG->Rects.push_back(P);// Bottom Left
		P.V = WStoSS(Rect.TRight);
		P.UV = { 1, 0 };
		RG->Rects.push_back(P);// Top Right
		//

		//
		P.V = WStoSS(Rect.TRight);
		P.UV = { 1, 0 };
		RG->Rects.push_back(P);// Top Right
		P.V = WStoSS(Rect.BLeft);
		P.UV = { 0, 1 };
		RG->Rects.push_back(P);// Bottom Left
		P.V = WStoSS({ Rect.BLeft.x, Rect.TRight.y });
		P.UV = { 0, 0 };
		RG->Rects.push_back(P);// Top Left
		//
	}


	/************************************************************************************************/


	void PushText(ImmediateRender* RG, Draw_TEXT Text)
	{
		RG->DrawCalls.push_back({ DRAWCALLTYPE::DCT_TEXT, RG->Text.size() });
		RG->Text.push_back(Text);
		/*
		Draw_RECTPoint P;
		P.Color = F4MUL(Text.Color, Text.Color);

		//
		P.V = WStoSS({ Text.CLIPAREA_TRIGHT.x, Text.CLIPAREA_BLEFT.y });
		P.UV = { 1, 1 };
		RG->Rects.push_back(P);// Bottom Right
		P.V = WStoSS(Text.CLIPAREA_BLEFT);
		P.UV = { 0, 1 };
		RG->Rects.push_back(P);// Bottom Left
		P.V = WStoSS(Text.CLIPAREA_TRIGHT);
		P.UV = { 1, 0 };
		RG->Rects.push_back(P);// Top Right
		//

		//
		P.V = WStoSS(Text.CLIPAREA_TRIGHT);
		P.UV = { 1, 0 };
		RG->Rects.push_back(P);// Top Right
		P.V = WStoSS(Text.CLIPAREA_BLEFT);
		P.UV = { 0, 1 };
		RG->Rects.push_back(P);// Bottom Left
		P.V = WStoSS({ Text.CLIPAREA_BLEFT.x, Text.CLIPAREA_TRIGHT.y });
		P.UV = { 0, 0 };
		RG->Rects.push_back(P);// Top Left
		//
		*/
	}


	/************************************************************************************************/


	void PushLineSet3D(ImmediateRender* RG, LineSegments LineSet)
	{
		RG->DrawCalls.push_back({ DCT_LINES3D, RG->DrawLines3D.size() });

		size_t Begin = RG->Lines3D.LineSegments.size();
		size_t Count = LineSet.size();

		for (auto L : LineSet)
			RG->Lines3D.LineSegments.push_back(L);

		RG->DrawLines3D.push_back({ Begin, Count });
	}


	/************************************************************************************************/


	void PushLineSet2D(ImmediateRender* RG, LineSegments LineSet)
	{
		RG->DrawCalls.push_back({ DCT_LINES2D, RG->DrawLines2D.size() });

		size_t Begin = RG->Lines2D.LineSegments.size();
		size_t Count = LineSet.size();

		for (auto L : LineSet)
			RG->Lines2D.LineSegments.push_back(L);

		RG->DrawLines2D.push_back({Begin, Count});
	}


	/************************************************************************************************/

	// Position and Area start at Top Left of Screen, going to Bottom Right; Top Left{0, 0} -> Bottom Right{1.0, 1.0f}
	void PrintText(ImmediateRender* IR, const char* str, FontAsset* Font, PrintTextFormatting& Formatting, iAllocator* TempMemory, PrintState& State, bool End)
	{
		size_t StrLen = strlen(str);

		size_t Idx					= IR->TextBufferGPU.Idx;
		size_t CurrentBufferSize	= IR->TextBufferSizes[Idx];

		if (CurrentBufferSize <  IR->TextBufferPosition)
		{	// Resize Buffer
			IR->TextBufferSizes[Idx] = 2 * CurrentBufferSize;
			CurrentBufferSize		*= 2;
			auto NewResource         = CreateShaderResource(IR->RS, sizeof(TextEntry) * CurrentBufferSize, "TEXT BUFFER");

			AddTempShaderRes(IR->TextBufferGPU, IR->RS);
			IR->TextBufferGPU[0] = NewResource[0];
			IR->TextBufferGPU[1] = NewResource[1];
			IR->TextBufferGPU[2] = NewResource[2];
		}

		size_t BufferSize = StrLen * sizeof(TextEntry);
		auto& Text = State.TextBuffer;
		size_t itr_2 = 0;

		float XBegin = Formatting.StartingPOS.x;
		float YBegin = Formatting.StartingPOS.y;

		float CurrentX = State.CurrentX;
		float CurrentY = State.CurrentY;
		float YAdvance = State.YAdvance;

		size_t BufferOffset = Text.size();
		size_t OutputBegin = itr_2;
		size_t LineBegin = OutputBegin + BufferOffset;

		auto CenterLine = [&]()
		{
			if (Formatting.CenterX)
			{
				const float Offset_X = (Formatting.TextArea.x - CurrentX) / 2.0f;
				const float Offset_Y = (Formatting.TextArea.y - CurrentY - YAdvance / 2) / 2.0f;

				if (abs(Offset_X) > 0.0001f)
				{
					for (size_t ii = LineBegin; ii < itr_2; ++ii)
					{
						Text[ii].POS.x += Offset_X;
						Text[ii].POS.y += Offset_Y;
					}
				}
			}
		};


		for (size_t StrIdx = 0; StrIdx < StrLen; ++StrIdx)
		{
			const auto C           = str[StrIdx];
			const float2 GlyphArea = Font->GlyphTable[C].WH * Formatting.PixelSize;
			const float  XAdvance  = Font->GlyphTable[C].Xadvance * Formatting.PixelSize.x;
			const auto G           = Font->GlyphTable[C];

			const float2 Scale	= float2(1.0f, 1.0f) / Font->TextSheetDimensions;
			const float2 WH		= G.WH * Scale;
			const float2 XY		= G.XY * Scale;
			const float2 UVTL	= XY;
			const float2 UVBR	= XY + WH;

			if (C == '\n' || CurrentX + XAdvance > Formatting.TextArea.x)
			{
				CenterLine();

				LineBegin = itr_2;
				CurrentX = 0;
				CurrentY += YAdvance / 2 * Formatting.Scale.y;
			}

			if (CurrentY > Formatting.StartingPOS.y + Formatting.TextArea.y)
				continue;

			TextEntry Character		= {};
			Character.POS           = float2(CurrentX + XBegin, CurrentY + YBegin);
			Character.TopLeftUV     = UVTL;
			Character.BottomRightUV = UVBR;
			Character.Color         = Formatting.Color;
			Character.Size          = WH * Formatting.Scale;

			Text.push_back(Character);
			YAdvance    = max(YAdvance, GlyphArea.y);
			CurrentX += XAdvance * Formatting.Scale.x;
			itr_2++;
		}

		State.CurrentX		= CurrentX;
		State.CurrentY		= CurrentY;
		State.CurrentY		= YAdvance;

		if (End)
		{
			CenterLine();

			Draw_TEXT2 NewDrawCall;
			NewDrawCall.Font = Font;
			NewDrawCall.Begin = IR->TextBuffer.size();
			NewDrawCall.Count = Text.size();

			IR->TextBufferPosition += StrLen;
			IR->DrawCalls.push_back({ DRAWCALLTYPE::DCT_TEXT2, IR->Text2.size() });
			IR->Text2.push_back(NewDrawCall);

			for (size_t I = 0; I < IR->TextBufferPosition && I < Text.size(); ++I)
				Text[I].POS = Position2SS(Text[I].POS);

			for (uint32_t I = 0; I < StrLen; ++I)
				IR->TextBuffer.push_back(Text[I]);

			State.TextBuffer.Release();
		}
	}


	/************************************************************************************************/


	void PrintText(ImmediateRender* IR, const char* str, FontAsset* Font, PrintTextFormatting& Formatting, iAllocator* TempMemory)
	{
		PrintState State;
		State.CurrentX = 0;
		State.CurrentY = 0;
		State.YAdvance = 0;
		State.TextBuffer.Allocator = TempMemory;

		PrintText(IR, str, Font, Formatting, TempMemory, State, true);
	}


	/************************************************************************************************/


	void PrintText(ImmediateRender* IR, const char* str, FontAsset* Font, float2 StartPOS, float2 TextArea, float4 Color, float2 PixelSize)
	{
		PrintTextFormatting Format;
		Format.PixelSize = PixelSize;
		Format.CenterX   = false;
		Format.CenterY   = false;
		Format.Color     = Color;
		Format.PixelSize = PixelSize;
		Format.Scale     = {1, 1};
		Format.TextArea  = TextArea;

		PrintText(IR, str, Font, Format, IR->TempMemory);
	}


	/************************************************************************************************/


	void InitiateImmediateRender(RenderSystem* RS, ImmediateRender* RG, iAllocator* Memory, iAllocator* TempMemory)
	{
		for (size_t I = 0; I < DRAWCALLTYPE::DCT_COUNT; ++I)
			RG->DrawStates[I] = nullptr;

		Shader DrawRectVShader;
		Shader DrawRectPShader;
		Shader DrawRectPSTexturedShader;
		Shader DrawLineVShader3D;
		Shader DrawLineVShader2D;
		Shader DrawLinePShader;

		DrawRectVShader				= LoadShader("DrawRect_VS", "DrawRect_VS", "vs_5_0", "assets\\vshader.hlsl");
		DrawRectPShader				= LoadShader("DrawRect", "DrawRect", "ps_5_0", "assets\\pshader.hlsl");
		DrawRectPSTexturedShader	= LoadShader("DrawRectTextured", "DrawRectTextured", "ps_5_0", "assets\\pshader.hlsl");
		DrawLineVShader2D			= LoadShader("VSegmentPassthrough_NOPV", "VSegmentPassthrough_NOPV", "vs_5_0", "assets\\vshader.hlsl");
		DrawLineVShader3D			= LoadShader("VSegmentPassthrough", "VSegmentPassthrough", "vs_5_0", "assets\\vshader.hlsl");
		DrawLinePShader				= LoadShader("DrawLine", "DrawLine", "ps_5_0", "assets\\pshader.hlsl");

		FINALLY
			Release(&DrawRectVShader);
			Release(&DrawRectPShader);
			Release(&DrawRectPSTexturedShader);
			Release(&DrawLineVShader2D);
			Release(&DrawLineVShader3D);
			Release(&DrawLinePShader);
		FINALLYOVER

		{
			// Draw Rect State Objects
			auto RootSig = RS->Library.RS4CBVs4SRVs;

			/*
			typedef struct D3D12_INPUT_ELEMENT_DESC
			{
				LPCSTR						SemanticName;
				UINT						SemanticIndex;
				DXGI_FORMAT					Format;
				UINT						InputSlot;
				UINT						AlignedByteOffset;
				D3D12_INPUT_CLASSIFICATION	InputSlotClass;
				UINT						InstanceDataStepRate;
			} 	D3D12_INPUT_ELEMENT_DESC;
			*/

			D3D12_INPUT_ELEMENT_DESC InputElements[] = {
				{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 0, 8,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			D3D12_RASTERIZER_DESC		Rast_Desc  = CD3DX12_RASTERIZER_DESC	(D3D12_DEFAULT);
			D3D12_DEPTH_STENCIL_DESC	Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC	(D3D12_DEFAULT);
			Depth_Desc.DepthFunc		= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
			Depth_Desc.DepthEnable		= false;

			D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
				PSO_Desc.pRootSignature        = RootSig;
				PSO_Desc.VS                    = DrawRectVShader;
				PSO_Desc.RasterizerState       = Rast_Desc;
				PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
				PSO_Desc.SampleMask            = UINT_MAX;
				PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
				PSO_Desc.NumRenderTargets      = 1;
				PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM;
				PSO_Desc.SampleDesc.Count      = 1;
				PSO_Desc.SampleDesc.Quality    = 0;
				PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
				PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
				PSO_Desc.DepthStencilState     = Depth_Desc;
			}

			ID3D12PipelineState* PSO = nullptr;
			HRESULT HR;
				
			PSO_Desc.PS = DrawRectPShader;
			HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
			FK_ASSERT(SUCCEEDED(HR));
			RG->DrawStates[DCT_2DRECT]			= PSO;
			RG->DrawStates[DCT_2DRECT_CLIPPED]	= PSO; PSO->AddRef();

			PSO_Desc.PS = DrawRectPSTexturedShader;
			HR  = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
			FK_ASSERT(SUCCEEDED(HR));
			RG->DrawStates[DCT_2DRECT_TEXTURED] = PSO;
		}

		{
			// Draw Line State Objects
			/*
			typedef struct D3D12_INPUT_ELEMENT_DESC
			{
				LPCSTR						SemanticName;
				UINT						SemanticIndex;
				DXGI_FORMAT					Format;
				UINT						InputSlot;
				UINT						AlignedByteOffset;
				D3D12_INPUT_CLASSIFICATION	InputSlotClass;
				UINT						InstanceDataStepRate;
			} 	D3D12_INPUT_ELEMENT_DESC;
			*/

			D3D12_INPUT_ELEMENT_DESC InputElements[2] = {
				{ "POSITION",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOUR",			0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

			D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
			Depth_Desc.DepthFunc		= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
			Depth_Desc.DepthEnable		= false;

			D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
				PSO_Desc.pRootSignature        = RS->Library.RS4CBVs4SRVs;
				PSO_Desc.VS                    = DrawLineVShader3D;
				PSO_Desc.PS                    = DrawLinePShader;
				PSO_Desc.RasterizerState       = Rast_Desc;
				PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
				PSO_Desc.SampleMask            = UINT_MAX;
				PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
				PSO_Desc.NumRenderTargets      = 1;
				PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM;
				PSO_Desc.SampleDesc.Count      = 1;
				PSO_Desc.SampleDesc.Quality    = 0;
				PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
				PSO_Desc.InputLayout           = { InputElements, 2 };
				PSO_Desc.DepthStencilState     = Depth_Desc;
			}

			ID3D12PipelineState* PSO = nullptr;
			HRESULT HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
			FK_ASSERT(SUCCEEDED(HR));
			RG->DrawStates[DCT_LINES3D] = PSO;

			PSO_Desc.VS = DrawLineVShader2D;

			HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
			FK_ASSERT(SUCCEEDED(HR));
			RG->DrawStates[DCT_LINES2D] = PSO;
		}

		// Create/Load Text Rendering State
		{
			Shader DrawTextVShader = LoadShader("VTextMain", "TextPassThrough", "vs_5_0", "assets\\TextRendering.hlsl");
			Shader DrawTextGShader = LoadShader("GTextMain", "GTextMain",		"gs_5_0", "assets\\TextRendering.hlsl");
			Shader DrawTextPShader = LoadShader("PTextMain", "TextShading",		"ps_5_0", "assets\\TextRendering.hlsl");

			HRESULT HR;
			ID3D12PipelineState* PSO = nullptr;

			/*
			struct TextEntry
			{
				float2 POS;
				float2 Size;
				float2 TopLeftUV;
				float2 BottomRightUV;
				float4 Color;
			};
			*/

			D3D12_INPUT_ELEMENT_DESC InputElements[5] =	{
				{ "POS",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,			0, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{ "WH",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,			0, 8,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{ "UVTL",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,			0, 16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{ "UVBL",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,			0, 24, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{ "COLOR",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,		0, 32, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			};

			D3D12_RASTERIZER_DESC Rast_Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);{
			}

			D3D12_DEPTH_STENCIL_DESC Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);{
				Depth_Desc.DepthEnable	 = false;
				Depth_Desc.StencilEnable = false;
			}

			D3D12_RENDER_TARGET_BLEND_DESC TransparencyBlend_Desc;{
				TransparencyBlend_Desc.BlendEnable           = true;
				
				TransparencyBlend_Desc.SrcBlend              = D3D12_BLEND_SRC_ALPHA;
				TransparencyBlend_Desc.DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
				TransparencyBlend_Desc.BlendOp               = D3D12_BLEND_OP_ADD;
				//TransparencyBlend_Desc.BlendOp               = D3D12_BLEND_OP_;
				TransparencyBlend_Desc.SrcBlendAlpha         = D3D12_BLEND_ZERO;
				TransparencyBlend_Desc.DestBlendAlpha        = D3D12_BLEND_ZERO;
				TransparencyBlend_Desc.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
				TransparencyBlend_Desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

				TransparencyBlend_Desc.LogicOpEnable         = false;
				TransparencyBlend_Desc.LogicOp               = D3D12_LOGIC_OP_NOOP;
			}

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSO_Desc = {};{
				PSO_Desc.pRootSignature             = RS->Library.RS4CBVs4SRVs;
				PSO_Desc.VS                         = DrawTextVShader;
				PSO_Desc.GS                         = DrawTextGShader;
				PSO_Desc.PS                         = DrawTextPShader;
				PSO_Desc.RasterizerState            = Rast_Desc;
				PSO_Desc.BlendState                 = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
				PSO_Desc.SampleMask                 = UINT_MAX;
				PSO_Desc.PrimitiveTopologyType      = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
				PSO_Desc.NumRenderTargets           = 1;
				PSO_Desc.RTVFormats[0]              = DXGI_FORMAT_R8G8B8A8_UNORM;
				PSO_Desc.BlendState.RenderTarget[0] = TransparencyBlend_Desc;
				PSO_Desc.SampleDesc.Count           = 1;
				PSO_Desc.SampleDesc.Quality         = 0;
				PSO_Desc.DSVFormat                  = DXGI_FORMAT_D32_FLOAT;
				PSO_Desc.InputLayout                = { InputElements, 5 };
				PSO_Desc.DepthStencilState          = Depth_Desc;
			}

			HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
			FlexKit::CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE PIPELINE STATE OBJECT"));

			Release(&DrawTextVShader);
			Release(&DrawTextGShader);
			Release(&DrawTextPShader);

			RG->DrawStates[DCT_TEXT]  = PSO;
			RG->DrawStates[DCT_TEXT2] = PSO; PSO->AddRef();
		}

		ConstantBuffer_desc	Desc;
		Desc.InitialSize = KILOBYTE * 32;
		Desc.pInital	 = nullptr;
		Desc.Structured  = false;

		RG->RectBuffer				= CreateConstantBuffer(RS, &Desc);
		RG->TempMemory				= TempMemory;
		RG->Rects.Allocator			= TempMemory;
		RG->DrawCalls.Allocator		= TempMemory;
		RG->Text.Allocator			= TempMemory;
		RG->TexturedRects.Allocator = TempMemory;
		RG->ClipAreas.Allocator		= TempMemory;
		RG->DrawLines3D.Allocator	= TempMemory;
		RG->DrawLines2D.Allocator	= TempMemory;
		RG->TextBuffer.Allocator	= TempMemory;
		RG->Text2.Allocator			= TempMemory;
		RG->TextBufferPosition		= 0;
		RG->RS = RS;

		RG->TextBufferSizes[0] = TEXTBUFFERMINSIZE;
		RG->TextBufferSizes[1] = TEXTBUFFERMINSIZE;
		RG->TextBufferSizes[2] = TEXTBUFFERMINSIZE;

		RG->TextBufferGPU = CreateShaderResource(RS, sizeof(TextEntry) * TEXTBUFFERMINSIZE, "TEXT BUFFER INITIAL");

		InitiateLineSet(RS, TempMemory, &RG->Lines2D);
		InitiateLineSet(RS, TempMemory, &RG->Lines3D);
	}


	/************************************************************************************************/


	void UploadImmediate(RenderSystem* RS, ImmediateRender* IR, iAllocator* TempMemory, RenderWindow* TargetWindow)
	{
		if (IR->Rects.size())
			UpdateResourceByTemp(RS, &IR->RectBuffer, IR->Rects.begin(), IR->Rects.size() * sizeof(Draw_RECT), 1, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		for (auto& I : IR->Text)
			UploadTextArea(I.Font, I.Text, TempMemory, RS, TargetWindow);

		UploadLineSegments(RS, &IR->Lines2D);
		UploadLineSegments(RS, &IR->Lines3D);

		if(IR->TextBuffer.size())
			UpdateResourceByTemp(RS, &IR->TextBufferGPU, IR->TextBuffer.begin(), IR->TextBuffer.size() * sizeof(TextEntry), 1, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	}


	/************************************************************************************************/


	void ReleaseDrawImmediate(RenderSystem* RS, ImmediateRender* RG) {
		RG->Rects.Release();
		RG->TexturedRects.Release();
		RG->ClipAreas.Release();
		RG->DrawLines2D.Release();
		RG->DrawLines3D.Release();
		RG->Lines2D.LineSegments.Release();
		RG->Lines3D.GPUResource.Release();
		RG->TextBuffer.Release();
		RG->Text.Release();
		RG->Text2.Release();

		Push_DelayedRelease(RS, RG->Lines2D.GPUResource[0]);
		Push_DelayedRelease(RS, RG->Lines2D.GPUResource[1]);
		Push_DelayedRelease(RS, RG->Lines2D.GPUResource[2]);

		Push_DelayedRelease(RS, RG->TextBufferGPU[0]);
		Push_DelayedRelease(RS, RG->TextBufferGPU[1]);
		Push_DelayedRelease(RS, RG->TextBufferGPU[2]);

		if (RG->RectBuffer)			RG->RectBuffer.Release();

		for (size_t I = 0; I < DRAWCALLTYPE::DCT_COUNT; ++I)
			if (RG->DrawStates[I])	RG->DrawStates[I]->Release();
	}


	/************************************************************************************************/


	void UploadLineSegments(RenderSystem* RS, LineSet* Set)
	{
		if (Set->LineSegments.size())
		{
			size_t SpaceNeeded = Set->LineSegments.size() * sizeof(LineSegment);
			if (SpaceNeeded  > Set->ResourceSize)
			{
				Push_DelayedRelease(RS, Set->GPUResource[0]);
				Push_DelayedRelease(RS, Set->GPUResource[1]);
				Push_DelayedRelease(RS, Set->GPUResource[2]);

				while (SpaceNeeded > Set->ResourceSize)
					Set->ResourceSize *= 2;

				Set->GPUResource = CreateShaderResource(RS, Set->ResourceSize, "LINE SEGMENTS - Expanded");
			}

			UpdateResourceByTemp(
				RS, &Set->GPUResource,
				Set->LineSegments.begin(),
				Set->LineSegments.size() * sizeof(LineSegment), 1,
				D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		}
	}


	/************************************************************************************************/


	void Clear(ImmediateRender* RG)
	{
		RG->DrawCalls.clear();
		RG->Rects.clear();
		RG->TexturedRects.clear();
	}


	/************************************************************************************************/


	void DrawImmediate(RenderSystem* RS, ID3D12GraphicsCommandList* CL, ImmediateRender* Immediate, Texture2D RenderTarget, Camera* C)
	{
		size_t			RectOffset		= 0;
		size_t			TexturePosition = 0;
		DRAWCALLTYPE	PrevState		= DCT_COUNT;

		FINALLY
		{
			Immediate->ClipAreas.Release();
			Immediate->Rects.Release();
			Immediate->TexturedRects.Release();
			Immediate->Text.Release();
			Immediate->Text2.Release();
			Immediate->TextBuffer.Release();
			Immediate->Lines2D.LineSegments.Release();
			Immediate->Lines3D.LineSegments.Release();
			Immediate->DrawLines2D.Release();
			Immediate->DrawLines3D.Release();
			Immediate->DrawCalls.Release();
			Immediate->TextBufferPosition = 0;
		}
		FINALLYOVER

		if (!Immediate->DrawCalls.size())
			return;


		/*
		typedef struct D3D12_VERTEX_BUFFER_VIEW
		{
			D3D12_GPU_VIRTUAL_ADDRESS	BufferLocation;
			UINT						SizeInBytes;
			UINT						StrideInBytes;
		} 	D3D12_VERTEX_BUFFER_VIEW;
		*/
		
		auto Resources		= RS->_GetCurrentFrameResources();
		auto RTVHeap		= Resources->RTVHeap.DescHeap;
		auto DescriptorHeap = RS->_GetCurrentDescriptorTable();
		auto RTVs			= RS->_GetRTVTableCurrentPosition_CPU();
		auto RTVPOS			= RS->_ReserveRTVHeap(1);
		auto DSVs			= RS->_GetDSVTableCurrentPosition_CPU();
		auto NullTable      = RS->_ReserveDescHeap(9);
		auto NullTableItr   = NullTable;
		auto CamConstants	= C ? C->Buffer.Get()->GetGPUVirtualAddress() : RS->NullConstantBuffer->GetGPUVirtualAddress();

		PushRenderTarget(RS, &RenderTarget, RTVPOS);

		for(auto I = 0; I < 4; ++I)
			NullTableItr = PushTextureToDescHeap(RS, RS->NullSRV, NullTableItr);

		for (auto I = 0; I < 4; ++I)
			NullTableItr = PushCBToDescHeap(RS, RS->NullConstantBuffer.Get(), NullTableItr, 1024);

		
		D3D12_VERTEX_BUFFER_VIEW View[] = {
			{ Immediate->RectBuffer->GetGPUVirtualAddress(), (UINT)(sizeof(Draw_RECT) * Immediate->Rects.size()),	(UINT)sizeof(Draw_RECT) },
		};

		D3D12_VIEWPORT VP;
		VP.Width		= RenderTarget.WH[0];
		VP.Height		= RenderTarget.WH[1];
		VP.MaxDepth		= 1.0f;
		VP.MaxDepth		= 0.0f;
		VP.TopLeftX		= 0;
		VP.TopLeftY		= 0;

		CL->SetGraphicsRootSignature(RS->Library.RS4CBVs4SRVs);
		CL->RSSetViewports(1, &VP);
		CL->OMSetRenderTargets(1, &RTVs, true, nullptr);

		// Nulling Extra Parameters
		CL->SetGraphicsRootDescriptorTable      (0, NullTable);

		for(size_t I = 1; I < 5; ++I)
			CL->SetGraphicsRootConstantBufferView	(I, RS->NullConstantBuffer.Get()->GetGPUVirtualAddress());

		/*
			typedef struct D3D12_RESOURCE_BARRIER
			{
			D3D12_RESOURCE_BARRIER_TYPE Type;
			D3D12_RESOURCE_BARRIER_FLAGS Flags;
			union
			{
			D3D12_RESOURCE_TRANSITION_BARRIER Transition;
			D3D12_RESOURCE_ALIASING_BARRIER Aliasing;
			D3D12_RESOURCE_UAV_BARRIER UAV;
			} 	;
			} 	D3D12_RESOURCE_BARRIER;
		*/

		D3D12_RESOURCE_TRANSITION_BARRIER Transition = {
			Immediate->RectBuffer.Get(), 0,
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
		};


		D3D12_RESOURCE_BARRIER Barrier[] = {
			{ 
				D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE,
				Transition
			}
		};


		//CL->ResourceBarrier(1, Barrier);

		for (auto D : Immediate->DrawCalls)
		{
			bool StateChanged = true;
			if (PrevState != D.Type) {
				CL->SetPipelineState(Immediate->DrawStates[D.Type]);
				StateChanged = true;
			}
			else
				StateChanged = false;

			switch (D.Type)
			{
			case DRAWCALLTYPE::DCT_2DRECT_CLIPPED: {
				/*
				LONG left;
				LONG top;
				LONG right;
				LONG bottom;
				*/

				auto ClipArea	= Immediate->ClipAreas[D.Index];
				auto WH			= RenderTarget.WH;
				
				D3D12_RECT Rects[] = { {
						(LONG)(WH[0] * ClipArea.CLIPAREA_BLEFT[0]),
						(LONG)(WH[1] - WH[1] * ClipArea.CLIPAREA_TRIGHT[1]),
						(LONG)(WH[0] * ClipArea.CLIPAREA_TRIGHT[0]),
						(LONG)(WH[1] - WH[1] * ClipArea.CLIPAREA_BLEFT[1]),
					}
				};

				CL->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				CL->RSSetScissorRects	(1, Rects);
				CL->IASetVertexBuffers	(0, 1, View + 0);
				CL->DrawInstanced		(6, 1, RectOffset, 0);
				RectOffset += 6;
			}	break;
			case DRAWCALLTYPE::DCT_2DRECT:{
				auto WH = RenderTarget.WH;
				D3D12_RECT Rects[] = { {
						0,
						0,
						(LONG)WH[0],
						(LONG)WH[1],
					}
				};

				CL->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				CL->RSSetScissorRects	(1, Rects);
				CL->IASetVertexBuffers	(0, 1, View + 0);
				CL->DrawInstanced		(6, 1, RectOffset, 0);
				RectOffset+= 6;

			}	break;
			case DRAWCALLTYPE::DCT_2DRECT_TEXTURED: {
				auto WH		= RenderTarget.WH;
				auto BLeft	= Immediate->TexturedRects[D.Index].CLIPAREA_BLEFT;
				auto TRight	= Immediate->TexturedRects[D.Index].CLIPAREA_TRIGHT;

				D3D12_RECT Rects[] = { {
						(LONG)(WH[0] * BLeft[0]),
						(LONG)(WH[1] - WH[1] * TRight[1]),
						(LONG)(WH[0] * TRight[0]),
						(LONG)(WH[1] - WH[1] * BLeft[1]),
					}
				};

				auto DescPOS = RS->_ReserveDescHeap(8);
				auto DescItr = DescPOS;
				DescItr = PushTextureToDescHeap(RS, *Immediate->TexturedRects[TexturePosition].Texture, DescItr);

				for (auto I = 0; I < 3; ++I)
					DescItr = PushTextureToDescHeap(RS, RS->NullSRV, DescItr);

				for (auto I = 0; I < 4; ++I)
					DescItr = PushCBToDescHeap(RS, RS->NullConstantBuffer.Get(), DescItr, 1024);

				CL->IASetPrimitiveTopology			(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				CL->SetGraphicsRootDescriptorTable	(0, DescPOS);
				CL->IASetVertexBuffers				(0, 1, View + 0);
				CL->DrawInstanced					(6, 1, RectOffset, 0);
	
				RectOffset+= 6;
				TexturePosition++;
			}	break;
			case DRAWCALLTYPE::DCT_LINES2D: {
				size_t Offset	= Immediate->DrawLines2D[D.Index].Begin;
				size_t Count	= Immediate->DrawLines2D[D.Index].Count;

				D3D12_VERTEX_BUFFER_VIEW Views[] = {
					{ Immediate->Lines2D.GPUResource->GetGPUVirtualAddress(),
					(UINT)Immediate->Lines2D.LineSegments.size() * sizeof(LineSegment),
					(UINT)sizeof(float3) * 2
					},
				};

				CL->IASetVertexBuffers(0, 1, Views);
				CL->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_LINELIST);
				CL->DrawInstanced(2 * Count, 1, 2 * Offset, 0);
			}	break;
			case DRAWCALLTYPE::DCT_LINES3D: {
				size_t Offset	= Immediate->DrawLines3D[D.Index].Begin;
				size_t Count	= Immediate->DrawLines3D[D.Index].Count;

				D3D12_VERTEX_BUFFER_VIEW Views[] = {
					{		Immediate->Lines3D.GPUResource->GetGPUVirtualAddress(),
					(UINT)	Immediate->Lines3D.LineSegments.size() * sizeof(LineSegment),
					(UINT)	sizeof(float3) * 2
					},
				};

				CL->IASetVertexBuffers					(0, 1, Views);
				CL->IASetPrimitiveTopology				(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_LINELIST);
				CL->SetGraphicsRootConstantBufferView	(1, CamConstants);
				CL->DrawInstanced						(2 * Count, 1, 2 * Offset, 0);


			}	break;
			case DRAWCALLTYPE::DCT_TEXT: {
				auto T = Immediate->Text[D.Index];
				auto Font = T.Font;
				auto Text = T.Text;

				if(Text->CharacterCount){
					D3D12_RECT RECT = D3D12_RECT();
					RECT.right	= (UINT)RenderTarget.WH[0];
					RECT.bottom = (UINT)RenderTarget.WH[1];

					/*
					D3D12_SHADER_RESOURCE_VIEW_DESC SRV_DESC = {};
					SRV_DESC.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
					SRV_DESC.Texture2D.MipLevels           = 1;
					SRV_DESC.Texture2D.MostDetailedMip     = 0;
					SRV_DESC.Texture2D.PlaneSlice          = 0;
					SRV_DESC.Texture2D.ResourceMinLODClamp = 0;
					SRV_DESC.Shader4ComponentMapping	   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; 
					*/

					D3D12_VERTEX_BUFFER_VIEW VBuffers[] = {
						{ Text->Buffer.Get()->GetGPUVirtualAddress(), UINT(sizeof(TextEntry) * Text->CharacterCount), sizeof(TextEntry) },
					};
			
					auto DescPOS = RS->_ReserveDescHeap(8);
					auto DescITR = DescPOS;

					DescITR = PushTextureToDescHeap(RS, Font->Texture, DescITR);

					for (auto I = 0; I < 3; ++I)
						DescITR = PushTextureToDescHeap(RS, RS->NullSRV, DescITR);

					for (auto I = 0; I < 4; ++I)
						DescITR = PushCBToDescHeap(RS, RS->NullConstantBuffer.Get(), DescITR, 1024);

					ID3D12DescriptorHeap* Heaps[] = { RS->_GetCurrentDescriptorTable() };

					CL->IASetPrimitiveTopology			(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
					CL->IASetVertexBuffers				(0, 1, VBuffers);
					CL->SetDescriptorHeaps				(1, Heaps);
					CL->SetGraphicsRootDescriptorTable	(0, DescPOS);
					CL->OMSetBlendFactor				(float4(1.0f, 1.0f, 1.0f, 1.0f));
					CL->RSSetScissorRects				(1, &RECT);

					CL->DrawInstanced					(Text->CharacterCount, 1, 0, 0);
				}
			}	break;
			case DRAWCALLTYPE::DCT_TEXT2: {
				auto T	  = Immediate->Text2[D.Index];
				auto Font = T.Font;
				
				
				D3D12_VERTEX_BUFFER_VIEW VBuffers[] = {
					{ Immediate->TextBufferGPU.Get()->GetGPUVirtualAddress(), UINT(sizeof(TextEntry) * Immediate->TextBufferSizes[0]), sizeof(TextEntry) },
				};
			
				auto DescPOS = RS->_ReserveDescHeap(8);
				auto DescITR = DescPOS;

				DescITR = PushTextureToDescHeap(RS, Font->Texture, DescITR);

				for (auto I = 0; I < 3; ++I)
					DescITR = PushTextureToDescHeap(RS, RS->NullSRV, DescITR);

				for (auto I = 0; I < 4; ++I)
					DescITR = PushCBToDescHeap(RS, RS->NullConstantBuffer.Get(), DescITR, 1024);

				ID3D12DescriptorHeap* Heaps[] = { RS->_GetCurrentDescriptorTable() };

				CL->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
				CL->IASetVertexBuffers				(0, 1, VBuffers);
				CL->SetDescriptorHeaps				(1, Heaps);
				CL->SetGraphicsRootDescriptorTable	(0, DescPOS);
				CL->OMSetBlendFactor				(float4(1.0f, 1.0f, 1.0f, 1.0f));
				//CL->RSSetScissorRects				(1, &RECT);

				CL->DrawInstanced					(T.Count, 1, T.Begin, 0);
			}	break;
			//case DRAWCALLTYPE::DCT_3DMesh:
				//CL->IASetVertexBuffers(0, 1, View + 1);
				//CL->DrawInstanced(4, 1, 4 * TexturedRectOffset, 0);
			//break;
			default:
				break;
			}

			PrevState = D.Type;
		}
	}


	/************************************************************************************************/
}