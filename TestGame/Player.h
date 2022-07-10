#pragma once
#include "Components.h"
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
        double cooldownTime = 3.0;
        double timer        = -1.0;

        void Reset()            { timer = 3.0; };
        void Begin()            { timer = 0.0; };

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

        bool actionPossible() const
        {
            if (playerState != MainState::Moving || playerState != MainState::Idle)
                return false;
            
            bool possible = true;
            for (const auto state : actionStates)
                possible &= state != ActionState::playing;

            return possible;
        }

        void Action1()
        {   // Basic Slash
            if (actionPossible() && actionStates[0] == FlexKit::PlayerState::ActionState::Available)
            {
                playerState = MainState::Action;

                FlexKit::Event evt;
                evt.InputSource     = Event::InputType::Local;
                evt.mType           = Event::EventType::iObject;
                evt.mData1.mINT[0]  = (int)PlayerEvents::Action1;

                playerEvents.push_back(evt);

                onAction1Play();
            }
        }

        void Action2()
        {   // Hadoken
            if (actionPossible() && actionStates[1] == FlexKit::PlayerState::ActionState::Available)
            {
                playerState = MainState::Action;

                FlexKit::Event evt;
                evt.InputSource     = Event::InputType::Local;
                evt.mType           = Event::EventType::iObject;
                evt.mData1.mINT[0]  = (int)PlayerEvents::Action2;

                playerEvents.push_back(evt);

                onAction2Play();
            }
        }

        void Action3()
        {   // Forward Dash
            if (actionPossible() && actionStates[2] == FlexKit::PlayerState::ActionState::Available)
            {
                playerState = MainState::Action;

                FlexKit::Event evt;
                evt.InputSource     = Event::InputType::Local;
                evt.mType           = Event::EventType::iObject;
                evt.mData1.mINT[0]  = (int)PlayerEvents::Action3;

                playerEvents.push_back(evt);
                
                onAction3Play();
            }
        }

        void Action4()
        {   // Strong Attack
            if (actionPossible() && actionStates[3] == FlexKit::PlayerState::ActionState::Available)
            {
                playerState = MainState::Action;

                FlexKit::Event evt;
                evt.InputSource     = Event::InputType::Local;
                evt.mType           = Event::EventType::iObject;
                evt.mData1.mINT[0]  = (int)PlayerEvents::Action4;

                playerEvents.push_back(evt);

                onAction4Play();
            }
        }

        void Jump()
        {
            switch (playerState)
            {
            case MainState::Moving:
            case MainState::Idle:
                onJumpStart();
            }
        }

        void ApplyDamage(float damage)
        {
            playerHealth -= damage;

            if (playerHealth > 0.0f)
            {
                FlexKit::Event evt;
                evt.InputSource       = Event::InputType::Local;
                evt.mType             = Event::EventType::iObject;
                evt.mData1.mINT[0]    = (int)PlayerEvents::PlayerHit;

                playerEvents.push_back(evt);
            }
            else if(playerHealth <= 0.0f)
            {
                FlexKit::Event evt;
                evt.InputSource       = Event::InputType::Local;
                evt.mType             = Event::EventType::iObject;
                evt.mData1.mINT[0]    = (int)PlayerEvents::PlayerDeath;

                playerEvents.push_back(evt);

                onDeath(*this);
            }
        }
    };

    inline static const ComponentID PlayerComponentID = GetTypeGUID(PlayerGameComponentID);

    using PlayerHandle      = Handle_t<32, PlayerComponentID>;
    using PlayerComponent   = BasicComponent_t<PlayerState, PlayerHandle, PlayerComponentID>;
    using PlayerView        = PlayerComponent::View;
}
