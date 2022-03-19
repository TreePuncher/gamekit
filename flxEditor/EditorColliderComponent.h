#pragma once

#include <vector>
#include "type.h"
#include "..\FlexKitResourceCompiler\SceneResource.h"
#include "..\coreutilities\Components.h"
#include "physicsutilities.h"

class EditorColliderComponent :
    public FlexKit::Serializable<EditorColliderComponent, FlexKit::EntityComponent, FlexKit::StaticBodyComponentID>
{
public:
    EditorColliderComponent() :
        Serializable{ FlexKit::StaticBodyComponentID } {}

    void Serialize(auto& ar)
    {
        EntityComponent::Serialize(ar);
    }

    FlexKit::Blob GetBlob() override;

    FlexKit::ResourceHandle ColliderResourceID;

    inline static RegisterConstructorHelper<EditorColliderComponent, FlexKit::StaticBodyComponentID> registered{};
};

class EditorRigidBodyComponent :
    public FlexKit::Serializable<EditorRigidBodyComponent, FlexKit::EntityComponent, FlexKit::RigidBodyComponentID>
{
public:
    EditorRigidBodyComponent() :
        Serializable{ FlexKit::RigidBodyComponentID } {}

    void Serialize(auto& ar)
    {
        EntityComponent::Serialize(ar);
    }

    FlexKit::Blob GetBlob() override;

    FlexKit::ResourceHandle ColliderResourceID;

    inline static RegisterConstructorHelper<EditorColliderComponent, FlexKit::RigidBodyComponentID> registered{};
};

class EditorViewport;
class EditorProject;

void RegisterColliderInspector  (EditorViewport& viewport, EditorProject& project);
void RegisterRigidBodyInspector (EditorViewport& viewport, EditorProject& project);



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
