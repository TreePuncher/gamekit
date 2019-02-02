/**********************************************************************

Copyright (c) 2018 Robert May

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

/*
TODO's
*/


#ifndef GAMEPLAY_H_INCLUDED
#define GAMEPLAY_H_INCLUDED

#include "..\coreutilities\containers.h"
#include "..\coreutilities\Components.h"
#include "..\coreutilities\Events.h"
#include "..\coreutilities\MathUtils.h"
#include "..\graphicsutilities\FrameGraph.h"


/************************************************************************************************/


using FlexKit::Event;
using FlexKit::FrameGraph;
using FlexKit::GameFramework;

using FlexKit::iAllocator;
using FlexKit::CameraHandle;
using FlexKit::ConstantBufferHandle;
using FlexKit::VertexBufferHandle;
using FlexKit::TextureHandle;
using FlexKit::static_vector;

using FlexKit::CameraBehavior;

using FlexKit::int2;
using FlexKit::uint2;
using FlexKit::float2;
using FlexKit::float3;
using FlexKit::float4;
using FlexKit::Quaternion;


/************************************************************************************************/


typedef size_t Player_Handle;
typedef size_t GridObject_Handle;
typedef size_t GridSpace_Handle;


/************************************************************************************************/


class GameplayModel
{
public:
	GameplayModel(FlexKit::iAllocator* memory)	{}


	GameplayModel& operator = (GameplayModel& rhs) = delete;// No Copy!


	GameplayModel& operator = (GameplayModel&& rhs)
	{
		return *this;
	}


	GameplayModel(GameplayModel&& rhs)
	{
	}


	void Release()
	{
	}
};


/************************************************************************************************/


enum PLAYER_EVENTS : int64_t
{
	PLAYER_UP				= GetCRCGUID(PLAYER_UP),
	PLAYER_LEFT				= GetCRCGUID(PLAYER_LEFT),
	PLAYER_DOWN				= GetCRCGUID(PLAYER_DOWN),
	PLAYER_RIGHT			= GetCRCGUID(PLAYER_RIGHT),
	PLAYER_ACTION1			= GetCRCGUID(PLAYER_ACTION1),
	PLAYER_HOLD				= GetCRCGUID(PLAYER_HOLD),

	PLAYER_ROTATE_LEFT		= GetCRCGUID(PLAYER_ROTATE_LEFT),
	PLAYER_ROTATE_RIGHT		= GetCRCGUID(PLAYER_ROTATE_RIGHT),

	DEBUG_PLAYER_UP			= GetCRCGUID(DEBUG_PLAYER_UP),
	DEBUG_PLAYER_LEFT		= GetCRCGUID(DEBUG_PLAYER_LEFT),
	DEBUG_PLAYER_DOWN		= GetCRCGUID(DEBUG_PLAYER_DOWN),
	DEBUG_PLAYER_RIGHT		= GetCRCGUID(DEBUG_PLAYER_RIGHT),
	DEBUG_PLAYER_ACTION1	= GetCRCGUID(DEBUG_PLAYER_ACTION1),
	DEBUG_PLAYER_HOLD		= GetCRCGUID(DEBUG_PLAYER_HOLD),

	PLAYER_UNKNOWN,
};


enum DEBUG_EVENTS : int64_t
{
	TOGGLE_DEBUG_CAMERA  = GetCRCGUID(TOGGLE_DEBUG_CAMERA),
	TOGGLE_DEBUG_OVERLAY = GetCRCGUID(TOGGLE_DEBUG_OVERLAY)
};


/************************************************************************************************/


class LocalPlayerHandler
{
public:
};


/************************************************************************************************/

#endif