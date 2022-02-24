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

    void Serialize(auto& ar)
    {
        ar& position;
        ar& size;
        ar& orientation;
    }
};

struct Triangle
{
    FlexKit::float3 position[3];
};

struct CSGShape
{
    std::vector<CSGPlane>   planes;

    FlexKit::AABB           GetAABB() const noexcept;
    std::vector<Triangle>   GetTris() const noexcept;
    FlexKit::BoundingSphere GetBoundingVolume() const noexcept;

    void Serialize(auto& ar)
    {
        ar& planes;
    }
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

    FlexKit::float3             position;
    FlexKit::Quaternion         orientation;
    FlexKit::float3             scale;

    std::shared_ptr<CSGNode>    left;
    std::shared_ptr<CSGNode>    right;

    FlexKit::AABB               GetAABB() const noexcept;

    void Serialize(auto& ar)
    {
        ar& shape;
        ar& op;

        ar& position;
        ar& orientation;
        ar& scale;

        ar& left;
        ar& right;
    }
};


/************************************************************************************************/


class EditorComponentCSG :
    public FlexKit::Serializable<EditorComponentCSG, FlexKit::EntityComponent, CSGComponentID>
{
public:
    void Serialize(auto& ar)
    {
        EntityComponent::Serialize(ar);

        ar& nodes;
    }

    FlexKit::Blob GetBlob() override
    {
        return {};
    }

    std::vector<CSGNode>    nodes;
};


/************************************************************************************************/


struct CSGComponentData
{
    std::vector<CSGNode>    nodes;
    int32_t                 selectedNode = -1;
};


using CSGHandle     = FlexKit::Handle_t<32, CSGComponentID>;
using CSGComponent  = FlexKit::BasicComponent_t<CSGComponentData, CSGHandle, CSGComponentID>;
using CSGView       = CSGComponent::View;


/************************************************************************************************/


class EditorViewport;

struct Vertex
{
    FlexKit::float4 Position;
    FlexKit::float4 Color;
    FlexKit::float2 UV;
};


void                        RegisterComponentInspector(EditorViewport& viewport);
static_vector<Vertex, 24>   CreateWireframeCube(const float halfW);


/************************************************************************************************/
