#pragma once
#include <Components.h>
#include <Events.h>
#include <type.h>
#include <TriggerComponent.h>

struct Player
{
	Player(FlexKit::GameObject& IN_gameObject, FlexKit::iAllocator& allocator) :
		gameObject	{ &IN_gameObject },
		playerSlot	{ allocator } {}

	enum class EPlayerState
	{
		Walking,
		Jumping,
		Hanging,
		Climbing,
	}	state = EPlayerState::Walking;

	FlexKit::GameObject* gameObject = nullptr;

	float jumpSpeed			= 15.0f;
	float gravity			= 19.0f;
	float fallGravityRatio	= 2.0f;
	float airMovementRatio	= 0.5f;
	float moveRate			= 50.0f;
	float climbRate			= 1.0f;

	float shimmyDirection	= 0.0f;

	bool  jumpEnable		= true;
	bool  grabLedge			= false;
	bool  hangPossible		= false;
	bool  jumpInProgress	= false;

	FlexKit::Slot_t		playerSlot;

	void Action();

	void OnJump();
	void OnJumpRelease();

	void OnFloorContact();

	void OnCrouch();
	void OnCrouchRelease();

	bool HandleEvents	(FlexKit::Event& evt);
};

struct PlayerFactory
{
	FlexKit::iAllocator& allocator;

	Player	OnCreate(FlexKit::GameObject& gameObject);
	void	OnCreateView(FlexKit::GameObject& gameObject, FlexKit::ValueMap user_ptr, const std::byte* buffer, const size_t bufferSize, FlexKit::iAllocator* allocator) {}
};

using PlayerHandle		= FlexKit::Handle_t<32, GetTypeGUID(Player)>;
using PlayerComponent	= FlexKit::BasicComponent_t<Player, PlayerHandle, GetTypeGUID(Player), PlayerFactory>;
using PlayerView		= PlayerComponent::View;

bool PlayerHandleEvents(FlexKit::GameObject& gameObject, FlexKit::Event&);

FlexKit::UpdateTask& QueuePlayerUpdate(FlexKit::GameObject& playerObject, FlexKit::UpdateDispatcher& dispatcher, double dT);


/**********************************************************************

Copyright (c) 2019-2023 Robert May

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
