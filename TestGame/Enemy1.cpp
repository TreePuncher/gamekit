#include "Enemy1.h"
#include "physicsutilities.h"
#include "Player.h"

namespace FlexKit
{   /************************************************************************************************/


    void Enemy1EventHandler::OnCreateView(GameObject& gameObject, ValueMap values, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
    {

    }


    /************************************************************************************************/


    UpdateTask& UpdateEnemies(UpdateDispatcher& dispatcher, double dt)
    {
        ProfileFunction();

        struct EnemyUpdate
        {

        };

        return dispatcher.Add<EnemyUpdate>(
            [&](auto& ... args)
            {
            },
            [=, &dispatcher](auto& args, iAllocator& allocator)
            {
                ProfileFunction();

                auto& enemies = Enemy_1_Component::GetComponent();
                auto& players = PlayerComponent::GetComponent();

                Vector<float3> playerPositions{ &allocator };
                for (auto& p : players)
                    playerPositions.push_back(p.componentData.position);

                Parallel_For(*dispatcher.threads, allocator, enemies.begin(), enemies.end(), 10,
                    [&](auto& enemyView, iAllocator& allocator)
                    {
                        ProfileFunction();

                        auto& enemyData = enemyView.componentData;
                        enemyData.Health;

                        const auto enemyPosition = GetWorldPosition(*enemyView.componentData.gameObject);

                        auto d          = playerPositions.front() - enemyPosition;
                        const auto d_n  = d.normal();

                        auto& component     = PhysXComponent::GetComponent();
                        auto hitDetected    = false;

                        component.GetLayer_ref(enemyData.layer).RayCast(
                            enemyPosition + enemyData.pendingMoves * (float)enemyData.t + enemyData.direction + float3(0.0f, 10.0f, 0.0f), enemyData.direction, 2.0f,
                            [&](PhysicsLayer::RayCastHit&& hit)
                            {
                                if (hit.distance < 5)
                                {
                                    enemyData.direction = (enemyData.direction + 2 * hit.normal).normal();
                                    enemyData.t2        = 0.0f;
                                    hitDetected         = true;
                                }
                                return false;
                            });

                        if (!hitDetected)
                        {
                            if (d.magnitude() < 20)
                            {
                                enemyView.componentData.pendingMoves += -d_n * float3{ 1, 0, 1 } * (float)dt * 5.0f;
                                enemyView.componentData.direction     = -d_n.normal();
                            }
                            else
                            {
                                if (enemyData.t2 > 5)
                                {
                                    enemyData.direction = float3((float)(rand() % 20 - 10), 0.0f, (float)(rand() % 20 - 10)).normal();
                                    enemyData.t2        = 0.0f;
                                }
                                else
                                    enemyData.t2 += dt;

                                enemyView.componentData.pendingMoves += enemyData.direction * (float)dt * 5.0f;
                            }
                        }

                        const auto orientation = Vector2Quaternion(enemyData.direction, float3{ 0, 1, 0 }, enemyData.direction.cross(float3{ 0, 1, 0 }));
                        SetOrientation(*enemyData.gameObject, orientation);

                        enemyView.componentData.t += dt;
                    });
            });
    }


    /************************************************************************************************/


    void CommitEnemyMoves(double dt)
    {
        ProfileFunction();

        auto& enemies = Enemy_1_Component::GetComponent();


        for (auto& enemyView : enemies)
        {
            auto& enemyData = enemyView.componentData;
            enemyData.Health;

            const auto enemyPosition = GetWorldPosition(*enemyView.componentData.gameObject);

            auto& gameObject        = *enemyView.componentData.gameObject;
            const auto pendingMove  =  enemyView.componentData.pendingMoves;
            const auto t            =  enemyView.componentData.t;

            physx::PxControllerFilters filters;

            if (pendingMove.magnitude() > 0.001f)
            {
                MoveController(gameObject, pendingMove, filters, dt);
                SetWorldPosition(gameObject, GetControllerPosition(gameObject));
            }

            enemyView.componentData.pendingMoves    = 0.0f;
            enemyView.componentData.t               = 0.0;
        }
    }


    /************************************************************************************************/


    void CreateEnemy1(GameObject& gameObject, const Enemy_1_Desc& desc)
    {
        static auto guraModel = []
        {
            auto [gura, _] = FindMesh("1x1x1Cube");
            return gura;
        }();

        const auto node           = FlexKit::GetZeroedNode();
        const auto controllerNode = FlexKit::GetZeroedNode();

        SetParentNode(controllerNode, node);
        SetPositionL(node, { 0.0f, -1.5f, 0.0f });
        Yaw(node, (float)pi / 1.5f );
        auto& brushView             = gameObject.AddView<BrushView>(guraModel);
        brushView.GetBrush().Node   = node;

        auto& nodeView = gameObject.AddView<SceneNodeView<>>(controllerNode);

        gameObject.AddView<CharacterControllerView>(desc.layer, desc.initialPos, controllerNode);
        gameObject.AddView<Enemey_1_View>(Enemy1_state{ &gameObject, desc.layer });
    }


}   /************************************************************************************************/


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

