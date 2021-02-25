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

