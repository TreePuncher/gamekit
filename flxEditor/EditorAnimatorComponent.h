#pragma once
#include "Components.h"
#include "..\FlexKitResourceCompiler\SceneResource.h"
#include "Serialization.hpp"

constexpr FlexKit::ComponentID EditorAnimatorComponentID = GetTypeGUID(EditorAnimatorComponentID);

class EditorAnimatorComponent :
    public FlexKit::Serializable<EditorAnimatorComponent, FlexKit::EntityComponent, EditorAnimatorComponentID>
{
public:
    EditorAnimatorComponent() :
        Serializable{ EditorAnimatorComponentID } {}

    FlexKit::Blob GetBlob() override;

    void Serialize(auto& ar)
    {
        EntityComponent::Serialize(ar);
    }

    inline static RegisterConstructorHelper<EditorAnimatorComponent, EditorAnimatorComponentID> registered{};
};
