#pragma once
#include "Components.h"
#include "SceneResource.h"
#include "Serialization.hpp"
#include "RuntimeComponentIDs.h"


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
    char        defaultValue[16];

    void Serialize(auto& ar)
    {
        ar& type;
        ar& IDHash;
        ar& stringID;
        ar& defaultValue;
    }
};


class AnimatorComponent :
    public FlexKit::Serializable<AnimatorComponent, FlexKit::EntityComponent, FlexKit::AnimatorComponentID>
{
public:
    AnimatorComponent() :
        Serializable{ FlexKit::AnimatorComponentID } {}

    FlexKit::Blob GetBlob() override;

    void Serialize(auto& ar)
    {
        EntityComponent::Serialize(ar);

        ar& scriptResource;
        ar& inputs;
    }

    uint64_t                    scriptResource;
    std::vector<AnimationInput> inputs;


    inline static RegisterConstructorHelper<AnimatorComponent, FlexKit::AnimatorComponentID> registered{};
};


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
