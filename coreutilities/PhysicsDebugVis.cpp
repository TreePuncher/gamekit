#include "PhysicsDebugVis.h"
#include "FrameGraph.h"
#include "physicsutilities.h"
#include "defaultpipelinestates.h"


namespace FlexKit
{


    PhysicsDebugOverlayPass& RenderPhysicsOverlay(
        UpdateDispatcher&               dispatcher,
        FrameGraph&                     frameGraph,
        ResourceHandle                  renderTarget,
        LayerHandle                     layer,
        CameraHandle                    camera,
        ReserveVertexBufferFunction&    reserveVB,
        ReserveConstantBufferFunction&  reserveCB)
    {


        return frameGraph.AddNode<PhysicsDebugOverlayPass>(
            PhysicsDebugOverlayPass{
                reserveVB,
                reserveCB
            },
            [&](FrameGraphNodeBuilder& builder, PhysicsDebugOverlayPass& data)
            {
                data.renderTarget = builder.RenderTarget(renderTarget);
            },
            [=](PhysicsDebugOverlayPass& pass, FlexKit::ResourceHandler& frameResources, FlexKit::Context& ctx, auto& allocator)
            {
                auto& layer     = FlexKit::PhysXComponent::GetComponent().GetLayer_ref(LayerHandle(0));
                auto& geometry  = layer.debugGeometry;

                if (geometry.size())
                {
                    ctx.BeginEvent_DEBUG("Physics Debug Overlay");

                    auto VBuffer = pass.reserveVB(geometry.size() * sizeof(geometry[0]));
                    auto CBuffer = pass.reserveCB(sizeof(GetCameraConstants(camera)) + sizeof(Brush::VConstantsLayout));

                    struct Constants
                    {
                        float4 albedo;
                        float4 specular;
                        float4x4 WT;
                    } passConsants{
                        .WT = float4x4::Identity()
                    };


                    VertexBufferDataSet     vertices        { geometry, VBuffer };
                    ConstantBufferDataSet   cameraConstants { GetCameraConstants(camera), CBuffer };
                    ConstantBufferDataSet   entityConstants { passConsants, CBuffer };

                    auto renderTarget = frameResources.GetRenderTarget(pass.renderTarget);

                    ctx.SetRootSignature(frameResources.renderSystem().Library.RS6CBVs4SRVs);
                    ctx.SetPipelineState(frameResources.GetPipelineState(DRAW_LINE3D_PSO));
                    ctx.SetScissorAndViewports({ renderTarget });
                    ctx.SetRenderTargets({ renderTarget }, false);
                    ctx.SetPrimitiveTopology(EIT_LINE);
                    ctx.SetVertexBuffers({ vertices });
                    ctx.SetGraphicsConstantBufferView(1, cameraConstants);
                    ctx.SetGraphicsConstantBufferView(2, entityConstants);

                    ctx.Draw(geometry.size());

                    ctx.EndEvent_DEBUG();
                }
            });
    }


}
