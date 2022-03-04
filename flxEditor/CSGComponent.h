#pragma once


#include <vector>
#include "type.h"
#include "..\FlexKitResourceCompiler\SceneResource.h"
#include "..\coreutilities\Components.h"


/************************************************************************************************/


constexpr uint32_t CSGComponentID = GetTypeGUID(CSGComponentID);


struct Triangle
{
    FlexKit::float3 position[3];



    void Serialize(auto& ar)
    {
        ar& position[0];
        ar& position[1];
        ar& position[2];
    }

            FlexKit::float3& operator [] (size_t idx) noexcept;
    const   FlexKit::float3& operator [] (size_t idx) const noexcept;
    Triangle        Offset(FlexKit::float3) const noexcept;
    FlexKit::AABB   GetAABB() const noexcept;
    const float3    Normal() const noexcept;
    const float3    TriPoint() const noexcept;
};

struct CSGShape
{
    std::vector<Triangle> tris;

    FlexKit::AABB                   GetAABB(const float3 pos) const noexcept;
    const std::vector<Triangle>&    GetTris() const noexcept;
    FlexKit::BoundingSphere         GetBoundingVolume() const noexcept;

    struct RayCast_result
    {
        float3 hitLocation; // barycentric intersection cordinate
        float  distance;
        size_t triangleIdx;
    };

    std::optional<RayCast_result>   RayCast(const FlexKit::Ray& r) const noexcept;


    bool dirty = false;


    void Serialize(auto& ar)
    {
        ar& tris;
    }
};


/************************************************************************************************/


enum class CSG_OP
{
    CSG_ADD,
    CSG_SUB,
    CSG_INTERSECTION,
};


struct CSGBrush
{
    CSGShape                    shape;
    CSG_OP                      op;
    bool                        dirty = false;

    FlexKit::float3             position;
    FlexKit::Quaternion         orientation;
    FlexKit::float3             scale;

    std::shared_ptr<CSGBrush>   left;
    std::shared_ptr<CSGBrush>   right;

    struct TrianglePair
    {
        Triangle A;
        Triangle B;
    };

    struct RayCast_result
    {
        CSGShape*       shape;
        size_t          triIdx;
        FlexKit::float3 BaryCentricResult;
    };

    std::optional<RayCast_result> RayCast(const FlexKit::Ray& r) const noexcept;

    std::vector<TrianglePair> intersections;

    bool                        IsLeaf() const noexcept;
    FlexKit::AABB               GetAABB() const noexcept;
    void                        Rebuild() noexcept;

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

        ar& brushes;
    }

    FlexKit::Blob GetBlob() override
    {
        return {};
    }

    std::vector<CSGBrush>   brushes;
};


/************************************************************************************************/


struct CSGComponentData
{
    std::vector<CSGBrush>   brushes;
    int32_t                 selectedBrush = -1;
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
