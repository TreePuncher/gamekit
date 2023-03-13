#include "Player.hpp"
#include "level.hpp"

#include <memoryutilities.h>
#include <physicsutilities.h>
#include <TriggerComponent.h>
#include <TriggerSlotIDs.hpp>

using namespace FlexKit;


/************************************************************************************************/


Player PlayerFactory::OnCreate(FlexKit::GameObject& gameObject)
{
	Player player{ gameObject, allocator };

	auto& triggerView	= gameObject.AddView<TriggerView>();
	auto& triggers		= triggerView.GetData();

	triggers.CreateTrigger(OnJumpTriggerID);
	triggers.CreateTrigger(OnJumpReleaseTriggerID);
	triggers.CreateTrigger(OnCrouchTriggerID);
	triggers.CreateTrigger(OnCrouchReleaseTriggerID);
	triggers.CreateTrigger(OnFloorContact);

	triggers.Connect(OnJumpTriggerID, player.playerSlot,
		[&gameObject](auto...)
		{
			auto& player = gameObject.AddView<PlayerView>().GetData();
			player.OnJump();
		});

	triggers.Connect(OnJumpReleaseTriggerID, player.playerSlot,
		[&gameObject](auto...)
		{
			auto& player = gameObject.AddView<PlayerView>().GetData();
			player.OnJumpRelease();
		});

	triggers.Connect(OnCrouchTriggerID, player.playerSlot,
		[&gameObject](auto...)
		{
			auto& player = gameObject.AddView<PlayerView>().GetData();
			player.OnCrouch();
		});

	triggers.Connect(OnCrouchReleaseTriggerID, player.playerSlot,
		[&gameObject](auto...)
		{
			auto& player = gameObject.AddView<PlayerView>().GetData();
			player.OnCrouchRelease();
		});

	triggers.Connect(OnFloorContact, player.playerSlot,
		[&gameObject](auto...)
		{
			auto& player = gameObject.AddView<PlayerView>().GetData();
			player.OnFloorContact();
		});

	return player;
}


/************************************************************************************************/


bool PlayerHandleEvents(FlexKit::GameObject& gameObject, FlexKit::Event& evt)
{
	return Apply(gameObject,
		[&](PlayerView& playerView) -> bool
		{
			return playerView->HandleEvents(evt);
		},
		[] { return false; });
}


/************************************************************************************************/


FlexKit::UpdateTask& QueuePlayerUpdate(GameObject& playerObject, FlexKit::UpdateDispatcher& dispatcher, double dT)
{
	struct _{};

	auto& task = dispatcher.Add<_>(
		[&](auto& builder, auto& data) {},
		[&, dT](_&, iAllocator& threadAllocator)
		{
			auto& controllerView = *playerObject.GetView<CameraControllerView>();
			auto& playerView = *playerObject.GetView<PlayerView>();
			auto& controller = controllerView.GetData();
			auto& player = playerView.GetData();

			switch (player.state)
			{
			case Player::EPlayerState::Walking:
			{
				controller.active = true;

				const auto right	= controllerView->GetRightVector();
				const auto down		= controllerView->gravity.normal();
				const auto body		= controllerView->GetPosition();
				const auto forward	= right.cross(down);
				const auto head		= body + forward + -down * 1.5f;

				auto	level	= GetActiveLevel();
				auto& layer_ref = FlexKit::PhysXComponent::GetComponent().GetLayer_ref(level->layer);

				const FlexKit::Ray ray1{ .D = down,		.O = head + forward * 1.1f + -down * 0.5f };
				const FlexKit::Ray ray2{ .D = forward,	.O = body + forward };

				GameObject* obj1 = nullptr;
				GameObject* obj2 = nullptr;

				float3 normal;

				auto hit1 = layer_ref.RayCast(ray1, 1.5f,
					[&](FlexKit::PhysicsLayer::RayCastHit res)
					{
						if (res.distance <= 0)
							return false;

						obj1	= res.gameObject;
						normal	= res.normal;

						return false;
					});

				auto hit2 = layer_ref.RayCast(ray2, 1.5f,
					[&](FlexKit::PhysicsLayer::RayCastHit res)
					{
						if (res.distance <= 0)
							return false;

						obj2 = res.gameObject;

						return false;
					});

				if (hit1 && hit2 && (obj1 == obj2))
				{
					player.hangPossible = true;
				}
				else player.hangPossible = false;

				if (player.grabLedge && player.hangPossible)
					player.state = Player::EPlayerState::Hanging;
			}	break;
			case Player::EPlayerState::Hanging:
			{
				controller.active	= false;
				controller.velocity = float3::Zero();

				if (player.shimmyDirection != 0.0f)
				{
					const auto right	= controllerView->GetRightVector();
					const auto down		= controllerView->gravity.normal();
					const auto body		= controllerView->GetPosition();
					const auto forward	= right.cross(down);
					const auto head		= body + forward + -down * 1.5f;

					const FlexKit::Ray ray1{ .D = forward,	.O = body };

					auto	level	= GetActiveLevel();
					auto& layer_ref	= FlexKit::PhysXComponent::GetComponent().GetLayer_ref(level->layer);

					const physx::PxControllerFilters	filters;
					const physx::PxQueryFilterData		filterData{ { 0, 0, 0, 0 }, physx::PxQueryFlag::eANY_HIT | physx::PxQueryFlag::eSTATIC };

					float3 hitNormal = float3::Zero();
					auto hit1 = layer_ref.RayCast(ray1, 5.5f, filterData,
						[&](FlexKit::PhysicsLayer::RayCastHit res)
						{
							hitNormal = res.normal;

							return false;
						});

					if (!hit1)
						return;

					const float3 bodyLeft	= hitNormal.cross(down);
					const float3 bodyRight	= -bodyLeft;

					const FlexKit::Ray rayLeft	{ .D = bodyLeft,	.O = body };
					const FlexKit::Ray rayRight	{ .D = bodyRight,	.O = body };

					if (player.shimmyDirection > 0.0f)
					{
						auto moveClear = !layer_ref.RayCast(rayRight, 4.5f, filterData, [&](FlexKit::PhysicsLayer::RayCastHit res) { return false; });

						if(moveClear)
							MoveController(*player.gameObject, bodyLeft * player.climbRate * (float)dT * player.shimmyDirection * 10.0f, filters, dT);
					}
					else
					{
						auto moveClear = !layer_ref.RayCast(rayLeft, 4.5f, filterData, [&](FlexKit::PhysicsLayer::RayCastHit res) { return false; });

						if (moveClear)
							MoveController(*player.gameObject, bodyLeft * player.climbRate * (float)dT * player.shimmyDirection * 10.0f, filters, dT);
					}
				}

				player.shimmyDirection = 0.0f;
			}	break;
			case Player::EPlayerState::Climbing:
			{
				const auto right	= controllerView->GetRightVector();
				const auto down		= controllerView->gravity.normal();
				const auto up		= -down;
				const auto body		= controllerView->GetPosition();
				const auto foot		= controllerView->GetFootPosition();
				const auto forward	= right.cross(down);
				const auto head		= body + -down * 1.5f;

				auto	level = GetActiveLevel();
				auto& layer_ref = FlexKit::PhysXComponent::GetComponent().GetLayer_ref(level->layer);

				const FlexKit::Ray ray1{ .D = down,		.O = head + forward * 3.5f + -down * 0.5f };
				const FlexKit::Ray ray2{ .D = down,		.O = foot };

				float distance = 0.0f;
				const float height = 4.5f;

				auto hit1 = layer_ref.RayCast(ray1, height,
					[&](FlexKit::PhysicsLayer::RayCastHit res)
					{
						if (res.distance <= 0)
							return true;

						distance = res.distance;

						return false;
					});

				auto hit2 = layer_ref.RayCast(ray2, 0.1f,
					[&](FlexKit::PhysicsLayer::RayCastHit res)
					{
						if (res.distance <= 0)
							return true;

						distance = res.distance;

						return false;
					});

				if (hit1 && distance < height)
				{
					physx::PxControllerFilters filters;
					MoveController(*player.gameObject, up * player.climbRate * (float)dT * 10.0 + forward * dT, filters, dT);
				}
				else if (!hit2)
				{
					physx::PxControllerFilters filters;
					MoveController(*player.gameObject, forward * dT * 2.0f, filters, dT);
				}
				else
					player.state = Player::EPlayerState::Walking;
			}	break;
			default:
				break;
			}
		});

	return task;
}


/************************************************************************************************/


void Player::Action()
{
	switch (state)
	{
	case EPlayerState::Walking:
	{
		auto camera		= GetCameraControllerCamera(*gameObject);
		const auto r	= FlexKit::ViewRay(camera, { 0.0f, 0.5f });
		auto layer		= GetActiveLevel()->layer;
		auto& layer_ref = PhysXComponent::GetComponent().GetLayer_ref(layer);

		layer_ref.RayCast({ r.D, r.O }, 100,
			[&](PhysicsLayer::RayCastHit hit)
			{
				Trigger(*hit.gameObject, ActivateTrigger, nullptr);
				return false;
			});
	}	break;
	}
}


/************************************************************************************************/


bool Player::HandleEvents(FlexKit::Event& evt)
{
	switch (state)
	{
	case EPlayerState::Walking:
		return HandleTPCEvents(*gameObject, evt);
	case EPlayerState::Hanging:
		switch (evt.mData1.mINT[0])
		{
		case TPC_MoveLeft:
			shimmyDirection = -1.0f;
			return true;
		case TPC_MoveRight:
			shimmyDirection = 1.0f;
			return true;
		default:
			return false;
		}
	default:
		return false;
	}

	std::unreachable();
}


/************************************************************************************************/


void Player::OnJump()
{
	switch (state)
	{
	case EPlayerState::Walking:
	{
		if (hangPossible == false)
		{
			Apply(*gameObject, [&](CameraControllerView& view)
				{
					if (view->floorContact && jumpEnable)
					{
						CharacterControllerApplyForce(*gameObject, float3{ 0.0f, jumpSpeed, 0.0f });
						jumpEnable	= false;
						grabLedge	= true;
					}
				});
		}
		else
		{	// Begin Hang
			state = EPlayerState::Hanging;
		}
	}	break;
	case EPlayerState::Hanging:
	{
		state = EPlayerState::Climbing;
	}
	};
}


/************************************************************************************************/


void Player::OnJumpRelease()
{
	Apply(*gameObject, [&](CameraControllerView& view)
		{
			switch (state)
			{
			case EPlayerState::Jumping:
				if (!view->floorContact)
					view->gravity *= fallGravityRatio;
				break;
			}	
		});

	grabLedge	= false;
	jumpEnable	= true;
}


/************************************************************************************************/


void Player::OnFloorContact()
{
	Apply(*gameObject, [&](CameraControllerView& view)
		{
			view->gravity = gravity;
		});

	state = EPlayerState::Walking;
}


/************************************************************************************************/


void Player::OnCrouch()
{

}


/************************************************************************************************/


void Player::OnCrouchRelease()
{

}


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
