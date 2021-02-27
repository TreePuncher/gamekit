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

#include "Gameplay.h"
#include "containers.h"
#include <algorithm>


using namespace FlexKit;



/************************************************************************************************/


GameObject& CreatePlayer(const PlayerDesc& desc, RenderSystem& renderSystem, iAllocator& allocator)
{
    GameObject& player = CreateThirdPersonCameraController(desc.pscene, allocator, desc.r, desc.h);

    //SetCameraControllerCameraHeightOffset(player, 7.5);
    //SetCameraControllerCameraBackOffset(player, 15);

    static const GUID_t meshID = 7896;

    auto [triMesh, loaded] = FindMesh(meshID);

    if (!loaded)
        triMesh = LoadTriMeshIntoTable(renderSystem, renderSystem.GetImmediateUploadQueue(), meshID);


    auto& localplayer               = player.AddView<LocalPlayerView>();
    localplayer.GetData().player    = desc.player;

    auto node = GetCameraControllerNode(player);

    player.AddView<SceneNodeView<>>(node);
    player.AddView<DrawableView>(triMesh, node);

    desc.gscene.AddGameObject(player, node);

    return player;
}


/************************************************************************************************/


void CreateMultiplayerScene(GameWorld& world)
{
    static const GUID_t sceneID = 1234;
    world.LoadScene(sceneID);

    auto& physics       = PhysXComponent::GetComponent();
    auto& floorCollider = world.objectPool.Allocate();
    auto floorShape     = physics.CreateCubeShape({ 200, 1, 200 });

    floorCollider.AddView<StaticBodyView>(world.pscene, floorShape, float3{ 0, -1.0f, 0 });
}


/************************************************************************************************/


void GameWorld::UpdatePlayer(const PlayerFrameState& playerState)
{
    auto playerView = FindPlayer(playerState.player);

    if (playerView) {
        playerView->Update(playerState);

        std::cout << playerState.orientation << "\n";

        for (const auto event_ : playerState.inputState.events)
        {
            switch (event_)
            {
            case PlayerInputState::Event::Action1:
            case PlayerInputState::Event::Action2:
            case PlayerInputState::Event::Action3:
            case PlayerInputState::Event::Action4:
            {
                SpellData initial;
                initial.card     = FireBall();
                initial.caster   = playerState.player;
                initial.duration = 3.0f;
                initial.life     = 0.0f;

                auto& player     = *playerView->gameObject;

                auto velocity    = playerState.forwardVector * 50.0f;//(float3{1.0f, 0.0f, 1.0f} * GetCameraControllerForwardVector(player)).normal() * 50.0f;
                auto position    = playerState.pos;

                CreateSpell(initial, position, velocity);
            }   break;
            default:
                break;
            }
        }
    }
}

/************************************************************************************************/


UpdateTask& UpdateSpells(FlexKit::UpdateDispatcher& dispathcer, ObjectPool<GameObject>& objectPool, const double dt)
{

    struct _ {};
    return dispathcer.Add<_>(
        [&](auto& builder, auto& _)
        {
        },
        [=, &objectPool = objectPool](auto& _, auto& allocator)
        {
            auto& spellComponent = SpellComponent::GetComponent();

            Vector<GameObject*> freeList{ &allocator };

            for (auto& spellInstance : spellComponent)
            {
                auto& spell = spellInstance.componentData;
                spell.life      += dt;
                spell.position  += spell.velocity * dt;

                if (spell.life > spell.duration)
                    freeList.push_back(spell.gameObject);
                else
                    SetWorldPosition(*spell.gameObject, spell.position);
            }

            for (auto object : freeList)
                objectPool.Release(*object);
        });

}
/************************************************************************************************/


PlayerFrameState GetPlayerFrameState(GameObject& gameObject)
{
    PlayerFrameState out;

    Apply(
        gameObject,
        [&](LocalPlayerView& view)
        {
            const float3        pos = GetCameraControllerModelPosition(gameObject);
            const Quaternion    q = GetCameraControllerModelOrientation(gameObject);

            out.player          = view.GetData().player;
            out.pos             = pos;
            out.orientation     = q;
            out.forwardVector   = (float3{ 1.0f, 0.0f, 1.0f } * GetCameraControllerForwardVector(gameObject)).normal();
        });

    return out;
}

/************************************************************************************************/


RemotePlayerData* FindPlayer(MultiplayerPlayerID_t ID)
{
    auto& players = RemotePlayerComponent::GetComponent();

    for (auto& player : players)
    {
        if (player.componentData.ID == ID)
            return &players[player.handle];
    }

    return nullptr;
}



/************************************************************************************************/

