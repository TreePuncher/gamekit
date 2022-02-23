#include "PCH.h"
#include "CSGRendering.h"
#include "defaultpipelinestates.h"


auto CreateWireframeCube(const float halfW)
{
    using FlexKit::float2;
    using FlexKit::float3;
    using FlexKit::float4;

    struct Vertex
    {
        float4 Position;
        float4 Color;
        float2 UV;
    };

    const static_vector<Vertex> vertices = {
        // Top
        { float4{ -halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        { float4{ -halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        { float4{ -halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{ -halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },


        { float4{  halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        // Bottom
        { float4{ -halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        { float4{ -halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        { float4{ -halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{ -halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        { float4{  halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        // Sides
        { float4{  halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

        { float4{  halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{  halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },


        { float4{ -halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{ -halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },


        { float4{ -halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
        { float4{ -halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
    };

    return vertices;
}

CSGRender::CSGRenderData& CSGRender::Render(
    FlexKit::UpdateDispatcher&              dispatcher,
    FlexKit::FrameGraph&                    frameGraph,
    FlexKit::ReserveConstantBufferFunction& cb,
    FlexKit::ReserveVertexBufferFunction&   vb,
    FlexKit::FrameResourceHandle            renderTarget,
    const double                            dt)
{
    struct UpdateGeometry_task
    {
    };

    auto& updateGeometryTask = dispatcher.Add<UpdateGeometry_task>(
        [&](auto& builder, UpdateGeometry_task)
        { },
        [](UpdateGeometry_task& data, iAllocator& threadLocalAllocator)
        {
            auto& csg = CSGComponent::GetComponent();

            for (auto& csgObject : csg)
            {   // Do stuff
                csgObject.componentData.nodes;
            }
        });

    return frameGraph.AddNode<CSGRenderData>(
        CSGRenderData{ cb, vb },
        [&](FlexKit::FrameGraphNodeBuilder& builder, CSGRenderData& data)
        {
            builder.AddDataDependency(updateGeometryTask);
            builder.WriteTransition(renderTarget, FlexKit::DRS_RenderTarget);
        },
        [&](CSGRenderData& data, FlexKit::ResourceHandler& resourceStates, FlexKit::Context& ctx, FlexKit::iAllocator& threadLocal)
        {
            using FlexKit::float3;
            using FlexKit::float4;

            auto& csg = CSGComponent::GetComponent();

            struct Vertex
            {
                float4 Position;
                float4 Color;
                float2 UV;
            };

            std::vector<Vertex> v;

            ctx.SetPipelineState(resourceStates.GetPipelineState(FlexKit::DRAW_LINE3D_PSO));

            for (auto& csgObject : csg)
            {
                for (auto& csgNode : csgObject.componentData.nodes)
                {
                    const auto volume   = csgNode.shape.GetBoundingVolume();
                    const float radius  = volume.w;

                    const auto vertices = CreateWireframeCube(radius);

                    FlexKit::VBPushBuffer VBBuffer = data.ReserveVertexBuffer(sizeof(Vertex) * 24);

                    const FlexKit::VertexBufferDataSet vbDataSet{
                        vertices.data(),
                        sizeof(vertices[0]) * vertices.size(),
                        VBBuffer
                    };

                    ctx.SetVertexBuffers({ vbDataSet });
                    ctx.Draw(24);
                }
            }
        });
}
