#include "Player.h"


namespace FlexKit
{	/************************************************************************************************/


	bool PlayerState::actionPossible() const
	{
		if (playerState != MainState::Moving || playerState != MainState::Idle)
			return false;

		bool possible = true;
		for (const auto state : actionStates)
			possible &= state != ActionState::playing;

		return possible;
	}


	/************************************************************************************************/


	void PlayerState::Action1()
	{   // Basic Slash
		if (actionPossible() && actionStates[0] == FlexKit::PlayerState::ActionState::Available)
		{
			playerState = MainState::Action;

			FlexKit::Event evt;
			evt.InputSource = Event::InputType::Local;
			evt.mType = Event::EventType::iObject;
			evt.mData1.mINT[0] = (int)PlayerEvents::Action1;

			playerEvents.push_back(evt);

			onAction1Play();
		}
	}


	/************************************************************************************************/


	void PlayerState::Action2()
	{   // Hadoken
		if (actionPossible() && actionStates[1] == FlexKit::PlayerState::ActionState::Available)
		{
			playerState = MainState::Action;

			FlexKit::Event evt;
			evt.InputSource = Event::InputType::Local;
			evt.mType = Event::EventType::iObject;
			evt.mData1.mINT[0] = (int)PlayerEvents::Action2;

			playerEvents.push_back(evt);

			onAction2Play();
		}
	}


	/************************************************************************************************/


	void PlayerState::Action3()
	{   // Forward Dash
		if (actionPossible() && actionStates[2] == FlexKit::PlayerState::ActionState::Available)
		{
			playerState = MainState::Action;

			FlexKit::Event evt;
			evt.InputSource = Event::InputType::Local;
			evt.mType = Event::EventType::iObject;
			evt.mData1.mINT[0] = (int)PlayerEvents::Action3;

			playerEvents.push_back(evt);

			onAction3Play();
		}
	}


	/************************************************************************************************/


	void PlayerState::Action4()
	{   // Strong Attack
		if (actionPossible() && actionStates[3] == FlexKit::PlayerState::ActionState::Available)
		{
			playerState = MainState::Action;

			FlexKit::Event evt;
			evt.InputSource = Event::InputType::Local;
			evt.mType = Event::EventType::iObject;
			evt.mData1.mINT[0] = (int)PlayerEvents::Action4;

			playerEvents.push_back(evt);

			onAction4Play();
		}
	}


	/************************************************************************************************/


	void PlayerState::Jump()
	{
		switch (playerState)
		{
		case MainState::Moving:
		case MainState::Idle:
			onJumpStart();
		}
	}


	/************************************************************************************************/


	void PlayerState::ApplyDamage(float damage)
	{
		playerHealth -= damage;

		if (playerHealth > 0.0f)
		{
			FlexKit::Event evt;
			evt.InputSource = Event::InputType::Local;
			evt.mType = Event::EventType::iObject;
			evt.mData1.mINT[0] = (int)PlayerEvents::PlayerHit;

			playerEvents.push_back(evt);
		}
		else if (playerHealth <= 0.0f)
		{
			FlexKit::Event evt;
			evt.InputSource = Event::InputType::Local;
			evt.mType = Event::EventType::iObject;
			evt.mData1.mINT[0] = (int)PlayerEvents::PlayerDeath;

			playerEvents.push_back(evt);

			onDeath(*this);
		}
	}

}	/************************************************************************************************/


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
