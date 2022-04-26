#pragma once


#include "Components.h"
#include "ResourceHandles.h"


namespace FlexKit
{
    struct Enemy_1_Desc
    {
        float3      initialPos;
        LayerHandle layer;
    };

    
    struct Enemy1_state
    {
        GameObject* gameObject = nullptr;
        LayerHandle layer;

        enum class State
        {
            Wander,
        }state = State::Wander;


        float3  direction       = float3{ 0.0f, 0.0f, 1.0f };
        float3  pendingMoves    = float3{ 0.0f };
        double  t               = 0.0f;
        double  t2              = 0.0f;
        float   Health          = 100.0f;
    };

    struct Enemy1EventHandler
    {
        static void OnCreateView(GameObject& gameObject, void* user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator);
    };

    constexpr size_t Enemy1ID   = GetTypeGUID(Enemy1Component);
    using Enemy1Handle          = Handle_t<32, Enemy1ID>;
    using Enemy_1_Component     = BasicComponent_t<Enemy1_state, Enemy1Handle, Enemy1ID, Enemy1EventHandler>;
    using Enemey_1_View         = Enemy_1_Component::View;

    void CreateEnemy1(GameObject& gameObject, const Enemy_1_Desc& desc);

    UpdateTask& UpdateEnemies   (UpdateDispatcher& dispatcher, double dt);
    void        CommitEnemyMoves(double dt);
}


/**********************************************************************

Copyright (c) 2022 Robert May

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
