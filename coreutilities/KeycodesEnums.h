/**********************************************************************

Copyright (c) 2014-2018 Robert May

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

// TODOs
// Draw Wireframe Primitive Helper Functions
// Light Sorting for Tiled Rendering
// Shadowing

#ifdef _WIN32
#pragma once
#endif

#ifndef KEYCODESENUMS_H_INCLUDED
#define KEYCODESENUMS_H_INCLUDED

namespace FlexKit
{

	enum KEYCODES
	{
		KC_A = 'A',
		KC_B = 'B',
		KC_C = 'C',
		KC_D = 'D',
		KC_E = 'E',
		KC_F = 'F',
		KC_G = 'G',
		KC_H = 'H',
		KC_I = 'I',
		KC_J = 'J',
		KC_K = 'K',
		KC_L = 'L',
		KC_M = 'M',
		KC_N = 'N',
		KC_O = 'O',
		KC_P = 'P',
		KC_Q = 'Q',
		KC_R = 'R',
		KC_S = 'S',
		KC_T = 'T',
		KC_U = 'U',
		KC_V = 'V',
		KC_W = 'W',
		KC_X = 'X',
		KC_Y = 'Y',
		KC_Z = 'Z',

		KC_0,
		KC_1,
		KC_2,
		KC_3,
		KC_4,
		KC_5,
		KC_6,
		KC_7,
		KC_8,
		KC_9,

		KC_OTHER,

		KC_INPUTCHARACTERCOUNT = 36,

		KC_ENTER,
		KC_ESC,
		KC_LEFTSHIFT,
		KC_LEFTCTRL,
		KC_SPACE,
		KC_BACKSPACE,
		KC_TAB,
		KC_RIGHTSHIFT,
		KC_RIGHTCTRL,

		KC_COMMA,
		KC_ARROWUP,
		KC_ARROWDOWN,
		KC_ARROWLEFT,
		KC_ARROWRIGHT,

		KC_PLUS,
		KC_MINUS,

		KC_UNDERSCORE,
		KC_EQUAL,

		KC_SYMBOL,

		KC_SYMBOLS_BEGIN,

		KC_HASH,
		KC_DOLLER,

		KC_PERCENT,
		KC_CHEVRON,

		KC_EXCLAMATION,
		KC_AT,
		KC_AMPERSAND,
		KC_STAR,

		KC_LEFTPAREN,
		KC_RIGHTPAREN,

		KC_SYMBOLS_END,

		KC_DELETE,
		KC_HOME,
		KC_END,
		KC_PAGEUP,
		KC_PAGEDOWN,

		KC_F1,
		KC_F2,
		KC_F3,
		KC_F4,
		KC_F5,
		KC_F6,
		KC_F7,
		KC_F8,
		KC_F9,
		KC_F10,
		KC_F11,
		KC_F12,

		KC_NUM1,
		KC_NUM2,
		KC_NUM3,
		KC_NUM4,
		KC_NUM5,
		KC_NUM6,
		KC_NUM7,
		KC_NUM8,
		KC_NUM9,
		KC_NUM0,

		KC_MOUSELEFT,
		KC_MOUSERIGHT,
		KC_MOUSEMIDDLE,

		KC_ERROR,

		KC_TILDA = '~',
		KC_COUNT
	};

}

#endif