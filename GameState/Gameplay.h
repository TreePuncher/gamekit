#ifndef GAMEPLAY_H
#define GAMEPLAY_H

/**********************************************************************

Copyright (c) 2017 Robert May

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

#include "BaseState.h"
#include "CameraUtilities.h"


struct PlayerController
{
	float3		Pos;
	float3		Velocity;
};

struct PlayerInputState
{
	bool Forward;
	bool Backward;
	bool Left;
	bool Right;
	bool Shield;

	void ClearState()
	{
		Forward = false;
		Backward = false;
		Left = false;
		Right = false;
	}
};


struct Player
{
	EntityHandle				Model;
	PlayerController			PlayerCTR;
	Camera3rdPersonContoller	CameraCTR;
	AnimationStateMachine		PlayerAnimation;
	CapsuleCharacterController	PlayerCollider;

	size_t					ID;

	operator Player* () { return this; }// I'm Getting tired of typeing the &'s everywhere!
};


void InitiatePlayer			( BaseState* Engine, Player* Out );
void ReleasePlayer			( Player* P, BaseState* Engine );
void UpdatePlayer			( BaseState* Engine, Player* P, PlayerInputState Input, float2 MouseMovement, double dT );
void UpdatePlayerAnimations	( BaseState* Engine, Player* P, double dT );


void SetPlayerPosition	  ( Player* P, float3 Position );
void YawPlayer			  ( Player* P, float Degree );
void SetPlayerOrientation ( Player* P, Quaternion Q );


inline Quaternion GetOrientation(Player* P) 
{
	return Quaternion(0, P->CameraCTR.Yaw, 0);
}

inline float3 GetPlayerPosition(Player* P)
{
	return P->PlayerCTR.Pos;
}

template<typename Ty, size_t SIZE = 64>
struct CircularBuffer
{
	CircularBuffer() : _Head(0), _Tail(0), _Size(0)
	{}
	
	size_t size() noexcept
	{
		return _Size;
	}

	Ty pop_front() noexcept
	{
		if (!_Size)
			FK_ASSERT("BUFFER EMPTY!");

		_Size--;

		size_t idx = _Tail++;
		_Tail = _Tail % SIZE;
		return Buffer[idx];
	}

	bool push_back(const Ty Item) noexcept
	{
		if (SIZE > _Size) {
			_Size++;
			size_t idx = _Head++;
			_Head = _Head % SIZE;
			Buffer[idx] = Item;
		}
		return false;
	}

	Ty& front() noexcept
	{
		return Buffer[_Tail];
	}

	Ty& back() noexcept
	{
		return Buffer[_Head];
	}

	size_t _Head, _Tail, _Size;
	Ty Buffer[SIZE];
};

struct Gameplay_Model
{
	struct InputFrame
	{
		PlayerInputState	KeyboardInput;
		float2				MouseInput;
		size_t				FrameID;
	};
	typedef CircularBuffer<InputFrame, 16> InputBuffer;

	FlexKit::static_vector<Player>		Players;
	FlexKit::static_vector<InputBuffer>	BufferedInputs;// Buffered Inputs
	FlexKit::static_vector<InputFrame>	PlayerInputs;
	FlexKit::static_vector<size_t>		LastFrameRecieved;
	double								T;

	void Clear()
	{
		Players.clear();
	}

	void SetPlayerCount(BaseState* Engine, size_t Count)
	{
		BufferedInputs.resize(Count);
		PlayerInputs.resize(Count);
		Players.resize(Count);
		LastFrameRecieved.resize(Count);

		for (auto& Counter : LastFrameRecieved)
			Counter = 0;

		for (auto& P : Players)
			InitiatePlayer(Engine, &P);
	}

	void Update(BaseState* Engine, double dT)
	{
		const double FrameStep = 1.0 / 30.0;
		if (T > FrameStep) {
			for (size_t i = 0; i < Players.size(); ++i)
			{
				if(BufferedInputs[i].size())
					PlayerInputs[i] = BufferedInputs[i].pop_front();

				UpdatePlayer(Engine, &Players[i], 
					PlayerInputs[i].KeyboardInput, 
					PlayerInputs[i].MouseInput, 
					FrameStep);
#if 1
				printf("Player at Position: ");
				printfloat3(Players[i].PlayerCTR.Pos);
				printf("  Orientation:");
				printQuaternion(GetOrientation(&Players[i]));
				printf("\n");
#endif
			}
			T -= FrameStep;
		}
		T += dT;
	}


	void UpdateAnimations(BaseState* Engine, double dT)
	{
		for (auto& P : Players)
			UpdatePlayerAnimations(Engine, &P, dT);
	}

};

#endif