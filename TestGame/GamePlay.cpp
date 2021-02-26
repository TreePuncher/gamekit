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

    auto node = GetCameraControllerNode(player);

    player.AddView<LocalPlayerView>();
    player.AddView<SceneNodeView<>>(node);
    player.AddView<DrawableView>(triMesh, node);

    desc.gscene.AddGameObject(player, node);

    return player;
}


/************************************************************************************************/


void CreateMultiplayerScene(EngineCore& core, GraphicScene& gscene, PhysXSceneHandle pscene, ObjectPool<GameObject>& objectPool)
{
    static const GUID_t sceneID = 1234;
    FK_ASSERT(LoadScene(core, gscene, sceneID));

    auto& physics       = PhysXComponent::GetComponent();
    auto& visibility    = SceneVisibilityComponent::GetComponent();
    auto floorShape     = physics.CreateCubeShape({ 200, 1, 200 });

    auto& floorCollider = objectPool.Allocate();
    floorCollider.AddView<StaticBodyView>(pscene, floorShape, float3{ 0, -1.0f, 0 });

    static std::regex pattern{"Cube"};
    for (auto& entity : gscene.sceneEntities)
    {
        auto& go    = *visibility[entity].entity;
        auto id     = GetStringID(go);

        if (id && std::regex_search(id, pattern))
        {
            auto meshHandle = GetTriMesh(go);
            auto mesh       = GetMeshResource(meshHandle);

            AABB aabb{
                .Min = mesh->Info.Min,
                .Max = mesh->Info.Max };

            const auto midPoint = aabb.MidPoint();
            const auto dim      = aabb.Dim() / 2.0f;
            const auto pos      = GetWorldPosition(go);
            auto q              = GetOrientation(go);


            PxShapeHandle shape = physics.CreateCubeShape(dim);

            go.AddView<StaticBodyView>(pscene, shape, pos, q);
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

