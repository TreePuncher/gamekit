#pragma once
#include <vector>
#include "Components.h"
#include "ModifiableShape.h"
#include "SceneResource.h"
#include "type.h"


/************************************************************************************************/


constexpr uint32_t CSGComponentID = GetTypeGUID(CSGComponentID);


using FlexKit::ModifiableShape;
using FlexKit::Ray;
using FlexKit::Triangle;


enum class CSG_OP
{
    CSG_ADD,
    CSG_SUB,
    CSG_INTERSECTION,
};


struct CSGBrush
{
    ModifiableShape             shape;
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
        ModifiableShape*    shape;
        uint32_t            faceIdx;
        uint32_t            faceSubIdx;
        float               distance;
        FlexKit::float3     BaryCentricResult;
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
    EditorComponentCSG() :
        Serializable{ CSGComponentID } {}

    void Serialize(auto& ar)
    {
        EntityComponent::Serialize(ar);

        ar& brushes;
    }

    FlexKit::Blob GetBlob() override;

    std::vector<CSGBrush>   brushes;

    inline static RegisterConstructorHelper<EditorComponentCSG, CSGComponentID> registered{};
};


/************************************************************************************************/


struct CSGComponentData
{
    std::vector<CSGBrush>   brushes;
    int32_t                 selectedBrush = -1;
    int                     debugVal1 = 0;
};

struct CSGComponentEventHandler
{
    static void OnCreateView(FlexKit::GameObject& gameObject, void* user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator);
};

using CSGHandle     = FlexKit::Handle_t<32, CSGComponentID>;
using CSGComponent  = FlexKit::BasicComponent_t<CSGComponentData, CSGHandle, CSGComponentID, CSGComponentEventHandler>;
using CSGView       = CSGComponent::View;


/************************************************************************************************/


class EditorViewport;

struct Vertex
{
    FlexKit::float4 Position;
    FlexKit::float4 Color;
    FlexKit::float2 UV;
};


void                        RegisterCSGInspector(EditorViewport& viewport);
static_vector<Vertex, 24>   CreateWireframeCube(const float halfW);


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
