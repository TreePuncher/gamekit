#pragma once
#include "Components.h"
#include "Handle.h"
#include "Events.h"

namespace FlexKit
{
	enum class PlayerEvents
	{
		PlayerDeath,
		PlayerHit,
		Action1, // Slash
		Action2, // Hadouken
		Action3, // Forward Dash
		Action4, // Strong Attack
	};

	struct PlayerItem
	{

	};

	struct CoolDownTimer
	{
		double cooldownTime	= 3.0;
		double timer		= -1.0;

		void Reset() { timer = 3.0; };
		void Begin() { timer = 0.0; };

		bool Update(double dt)
		{
			if (timer > 0.0)
			{
				timer += dt;
				if (timer > cooldownTime)
				{
					timer = -1.0;
					return true;
				}
			}

			return false;
		}
	};

	struct PlayerState
	{
		GameObject* gameObject;
		uint64_t    playerID;
		float       playerHealth    = 100.0f;

		float3      forward         = {};
		float3      position        = {};

		Vector<PlayerItem> Items;

		CoolDownTimer action1Cooldown{ 1.0 };
		CoolDownTimer action2Cooldown{ 1.0 };
		CoolDownTimer action3Cooldown{ 1.0 };
		CoolDownTimer action4Cooldown{ 1.0 };


		enum class ActionState
		{
			Available,
			Cooldown,
			playing
		};


		enum class MainState
		{
			Moving,
			Idle,
			Jumping,
			Action,
			Dashing
		};


		MainState   playerState;
		ActionState actionStates[4];

		FlexKit::TypeErasedCallable<void()> onJumpStart = [] {};
		FlexKit::TypeErasedCallable<void()> onJumpEnd   = [] {};

		FlexKit::TypeErasedCallable<void()> onAction1Play = []{};
		FlexKit::TypeErasedCallable<void()> onAction2Play = []{};
		FlexKit::TypeErasedCallable<void()> onAction3Play = []{};
		FlexKit::TypeErasedCallable<void()> onAction4Play = []{};

		FlexKit::TypeErasedCallable<void()> onAction1CooldownEnd = [] {};
		FlexKit::TypeErasedCallable<void()> onAction2CooldownEnd = [] {};
		FlexKit::TypeErasedCallable<void()> onAction3CooldownEnd = [] {};
		FlexKit::TypeErasedCallable<void()> onAction4CooldownEnd = [] {};

		FlexKit::TypeErasedCallable<void()> onAction1End = [] {};
		FlexKit::TypeErasedCallable<void()> onAction2End = [] {};
		FlexKit::TypeErasedCallable<void()> onAction3End = [] {};
		FlexKit::TypeErasedCallable<void()> onAction4End = [] {};

		FlexKit::TypeErasedCallable<void(PlayerState&)> onDeath = [](PlayerState& player) noexcept {};

		static_vector<FlexKit::Event> playerEvents;

		bool actionPossible() const;

		void Action1();
		void Action2();
		void Action3();
		void Action4();

		void Jump();
		void ApplyDamage(float damage);
	};

	inline static const ComponentID PlayerComponentID = GetTypeGUID(PlayerGameComponentID);

	using PlayerHandle      = FlexKit::Handle_t<32, PlayerComponentID>;
	using PlayerComponent   = FlexKit::BasicComponent_t<PlayerState, PlayerHandle, PlayerComponentID>;
	using PlayerView        = PlayerComponent::View;
}

/**********************************************************************

Copyright (c) 2015 - 2022 Robert May

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
