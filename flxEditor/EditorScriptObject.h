#pragma once

#include "Common.h"
#include "SceneResource.h"
#include "Serialization.hpp"


/************************************************************************************************/


constexpr FlexKit::ComponentID EditorScriptComponentID = GetTypeGUID(EditorScriptComponentID);


struct AnimationInput
{
    enum class InputType : uint32_t
    {
        Float,
        Float2,
        Float3,
        Float4,
        Uint,
        Uint2,
        Uint3,
        Uint4
    } type;

    uint32_t    IDHash;
    std::string stringID;

    void Serialize(auto& ar)
    {
        ar& type;
        ar& IDHash;
        ar& stringID;
    }
};


class ScriptResource :
    public FlexKit::Serializable<ScriptResource, FlexKit::iResource, GetTypeGUID(ScriptResource)>
{
public:
    FlexKit::ResourceBlob   CreateBlob()        const           override;
    const std::string&      GetResourceID()     const noexcept  override;
    const uint64_t          GetResourceGUID()   const noexcept  override;
    const ResourceID_t      GetResourceTypeID() const noexcept  override;

    void                    SetResourceID(std::string& id) override;

    void Serialize(auto& ar)
    {
        ar& resourceId;
        ar& source;
        ar& ID;
        ar& inputs;
    }

    uint64_t    resourceId      = rand();
    std::vector<AnimationInput> inputs;
    std::string ID              = "ScriptObject";
    std::string source;
};


using ScriptResource_ptr = std::shared_ptr<ScriptResource>;


/************************************************************************************************/


class ScriptedGameObjectResource :
    public FlexKit::Serializable<ScriptedGameObjectResource, FlexKit::iResource, GetTypeGUID(ScriptedGameObjectResource)>
{
public:
    FlexKit::ResourceBlob   CreateBlob()        const           override;
    const std::string&      GetResourceID()     const noexcept  override;
    const uint64_t          GetResourceGUID()   const noexcept  override;
    const ResourceID_t      GetResourceTypeID() const noexcept  override;

    void                    SetResourceID(std::string& id) override;

    void Serialize(auto& ar)
    {
        ar& resourceId;
        ar& ID;

        ar& scriptId;
        ar& skeletonId;
        ar& triMeshId;
    }

    uint64_t    resourceId  = rand();
    std::string ID          = "ScriptedObject";

    uint64_t    scriptId;
    uint64_t    skeletonId;
    uint64_t    triMeshId;
};


using ScriptGameObjectResource_ptr = std::shared_ptr<ScriptedGameObjectResource>;


/************************************************************************************************/


/**********************************************************************

Copyright (c) 2021 - 2022 Robert May

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
