#pragma once
#include "Components.h"
#include "Events.h"

namespace FlexKit
{
    enum class PlayerEvents
    {
        PlayerDeath,
        PlayerHit,
    };

    struct PlayerState
    {
        GameObject* gameObject;
        uint64_t    playerID;
        float       playerHealth    = 100.0f;

        float3      forward;
        float3      position;

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
            }
        }

        static_vector<FlexKit::Event> playerEvents;
    };

    inline static const ComponentID PlayerComponentID = GetTypeGUID(PlayerGameComponentID);

    using PlayerHandle      = Handle_t<32, PlayerComponentID>;
    using PlayerComponent   = BasicComponent_t<PlayerState, PlayerHandle, PlayerComponentID>;
    using PlayerView        = PlayerComponent::View;
}
