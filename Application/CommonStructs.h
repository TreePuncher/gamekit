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

#pragma once

#include "..\buildsettings.h"
#include "..\coreutilities\Handle.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\MathUtils.h"
#include "..\coreutilities\MeshUtils.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\coreutilities\timeutilities.h"
#include "..\graphicsutilities\graphics.h"

/************************************************************************************************/

using FlexKit::float2;
using FlexKit::float3;
using FlexKit::float4;

using FlexKit::Pair;
using FlexKit::ConstantBuffer;
using FlexKit::NodeHandle;
using FlexKit::RenderSystem;
using FlexKit::SceneNodes;
using FlexKit::Shader;
using FlexKit::StackAllocator;
using FlexKit::VertexBuffer;
using FlexKit::VertexBufferView;

/************************************************************************************************/

struct sKeyState
{
	sKeyState()
	{
		W = false;
		A = false;
		S = false;
		D = false;
		Q = false;
		E = false;
		P = false;
		R = false;
		F = false;

		FrameID = 0;
	}

	char B[4];
	bool W;
	bool A;
	bool S;
	bool D;
	bool Q;
	bool E;
	bool P;
	bool R;
	bool F;
	size_t FrameID;
};

/************************************************************************************************/

struct Player
{
	NodeHandle								PitchNode;
	NodeHandle								Node;
	float3									dV;
	float									r;// Rotation Rate;
	float									m;// Movement Rate;
};

/************************************************************************************************/
