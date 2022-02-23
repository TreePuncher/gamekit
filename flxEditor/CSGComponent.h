#pragma once


#include <vector>
#include "type.h"
#include "..\FlexKitResourceCompiler\SceneResource.h"
#include "..\coreutilities\Components.h"


/************************************************************************************************/


constexpr uint32_t CSGComponentID = GetTypeGUID(CSGComponentID);


struct CSGPlane
{
    FlexKit::float3     position;
    FlexKit::float3     size;
    FlexKit::Quaternion orientation;
};


struct CSGShape
{
    std::vector<CSGPlane>   planes;
    std::vector<float3>     GetTris() const;
    FlexKit::BoundingSphere GetBoundingVolume() const;
};


/************************************************************************************************/


enum class CSG_OP
{
    CSG_ADD,
    CSG_SUB
};


struct CSGNode
{
    CSGShape                    shape;
    CSG_OP                      op;

    std::shared_ptr<CSGNode>    left;
    std::shared_ptr<CSGNode>    right;

    FlexKit::float3             position;
    FlexKit::Quaternion         orientation;
    FlexKit::float3             scale;
};


/************************************************************************************************/


class EditorComponentCSG :
    public FlexKit::Serializable<EditorComponentCSG, FlexKit::EntityComponent, CSGComponentID>
{
public:
    void Serialize(auto& ar)
    {
        EntityComponent::Serialize(ar);
    }

    FlexKit::Blob GetBlob() override
    {
        return {};
    }
};


/************************************************************************************************/


struct CSGComponentData
{
    std::vector<CSGNode> nodes;
};


using CSGHandle     = FlexKit::Handle_t<32, CSGComponentID>;
using CSGComponent  = FlexKit::BasicComponent_t<CSGComponentData, CSGHandle, CSGComponentID>;


using CSGView = CSGComponent::View;


/************************************************************************************************/
