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

#include "containers.h"
#include "Components.h"
#include "Events.h"
#include "MathUtils.h"
#include "GameFramework.h"
#include "FrameGraph.h"



/************************************************************************************************/


using FlexKit::Event;
using FlexKit::FrameGraph;
using FlexKit::GameFramework;

using FlexKit::iAllocator;
using FlexKit::CameraHandle;
using FlexKit::ConstantBufferHandle;
using FlexKit::Handle_t;
using FlexKit::VertexBufferHandle;
using FlexKit::ResourceHandle;
using FlexKit::static_vector;

using FlexKit::CameraView;

using FlexKit::int2;
using FlexKit::uint2;
using FlexKit::float2;
using FlexKit::float3;
using FlexKit::float4;
using FlexKit::Quaternion;



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


using CardTypeID_t = uint32_t;

struct CardInterface
{
    const CardTypeID_t  CardID = -1;
    const char* cardName = "!!!!!";
    const char* description = "!!!!!";

    const uint          requiredPowerLevel = 0;
};


struct PowerCard : public CardInterface
{
    PowerCard() : CardInterface{
        GetTypeGUID(PowerCard),
        "PowerCard",
        "Use to increase current level, allows for more powerful spell casting" } {}
};


struct FireBall : public CardInterface
{
    FireBall() : CardInterface{
        GetTypeGUID(FireBall),
        "FireBall",
        "Throws a small ball a fire, burns on contact.\n"
        "Required casting level is 1" } {}

};


/************************************************************************************************/

#endif
