#include "MathUtils.h"
#include "Components.h"
#include "FrameGraph.h"
#include <random>

namespace FlexKit
{   /************************************************************************************************/


	inline ID3D12PipelineState* CreateParticleMeshInstancedPSO(RenderSystem* RS)
	{
		auto VShader = RS->LoadShader("ParticleMeshInstanceVS", "vs_6_0", "assets\\shaders\\ParticleRendering.hlsl");
		auto PShader = RS->LoadShader("ParticleMeshInstancePS", "ps_6_0", "assets\\shaders\\ParticleRendering.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			{ "INSTANCEPOS",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 3, 0,                  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEARGS",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 3, sizeof(float4),     D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RSDefault;
			PSO_Desc.VS                    = VShader;
			PSO_Desc.PS                    = PShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets      = 3;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM;        // Albedo
			PSO_Desc.RTVFormats[1]         = DXGI_FORMAT_R16G16B16A16_FLOAT;    // Specular
			PSO_Desc.RTVFormats[2]         = DXGI_FORMAT_R16G16B16A16_FLOAT;    // Normal
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
			PSO_Desc.DepthStencilState     = Depth_Desc;
			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


    /************************************************************************************************/


	inline ID3D12PipelineState* CreateParticleMeshInstancedDepthPSO(RenderSystem* RS)
	{
        auto VShader = RS->LoadShader("ParticleMeshInstanceDepthVS", "vs_6_0", "assets\\shaders\\ParticleRendering.hlsl");
        auto GShader = RS->LoadShader("ParticleMeshInstanceDepthGS", "gs_6_0", "assets\\shaders\\ParticleRendering.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "POSITION",	    0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	                D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "INSTANCEPOS",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0,                  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEARGS",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, sizeof(float4),     D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RSDefault;
			PSO_Desc.VS                    = VShader;
			PSO_Desc.GS                    = GShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets      = 0;
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
			PSO_Desc.DepthStencilState     = Depth_Desc;
			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


    static const PSOHandle INSTANCEPARTICLEDPASS        = PSOHandle(GetTypeGUID(INSTANCEPARTICLEDPASS));
    static const PSOHandle INSTANCEPARTICLEDEPTHDPASS   = PSOHandle(GetTypeGUID(INSTANCEPARTICLEDEPTHDPASS));


    /************************************************************************************************/


    struct EmitterProperties
    {
        float   maxLife             = 10.0f;
        float   minEmissionRate     = 100;
        float   maxEmissionRate     = 200;
        float   emissionVariance    = 0.5f;
        float   initialVelocity     = 1.0f;
        float   initialVariance     = 1.0f;

        float   emissionSpread              = pi / 4;
        float   emissionDirectionVariance   = 0.3f;
    };


    struct InitialParticleProperties
    {
        float3 position;
        float3 velocity;
        float  lifespan;
    };


    class ParticleSystemInterface
    {
    public:
        virtual void Emit(double dT, EmitterProperties& properties, NodeHandle node) = 0;
    };


    /************************************************************************************************/


    template<typename TY_ParticleData>
    class ParticleSystem : public ParticleSystemInterface
    {
    public:
        ParticleSystem(iAllocator& allocator) :
            particles{ &allocator } {}


        UpdateTask& Update(double dT, ThreadManager& threads, UpdateDispatcher& dispatcher)
        {
            struct _ {  };

            return dispatcher.Add<_>(
                [this](auto& builder, auto& data){},
                [&, &threads = threads, dT = dT](auto& data, auto& allocator)
                {
                    Vector<TY_ParticleData> prev{ &allocator };
                    prev = particles;
                    particles.clear();


                    for (auto& particle : prev)
                        if(auto particleState = TY_ParticleData::Update(particle, dT); particleState)
                            particles.push_back(particleState.value());
                });
        }


        void Emit(double dT, EmitterProperties& properties, NodeHandle node)
        {
            const auto minRate  = properties.minEmissionRate;
            const auto maxRate  = properties.maxEmissionRate;
            const auto variance = properties.emissionVariance;
            const float spread  = properties.emissionSpread;

            std::random_device                      generator;
            std::uniform_real_distribution<float>   thetaspreadDistro(0.0f, 1.0f);
            std::normal_distribution<float>         gammaspreadDistro(-1.0f, 1.0f);
            std::normal_distribution<float>         realDistro(minRate, maxRate);
            std::uniform_real_distribution<float>   velocityDistro( properties.initialVelocity * (1.0f - properties.initialVariance),
                                                                    properties.initialVelocity * (1.0f + properties.initialVariance));
            const auto emitterPOS           = GetPositionW(node);
            const auto emitterOrientation   = GetOrientation(node);
            const auto emissionCount        = realDistro(generator) * dT;

            for (size_t I = 0; I < emissionCount; I++)
            {
                const float theta       = 2 * float(pi) * thetaspreadDistro(generator);
                const float gamma       = pi / 2.0f - (float(pi) * abs(gammaspreadDistro(generator)));

                const float sinTheta = sin(theta);
                const float cosTheta = cos(theta);
                const float cosGamma = cos(gamma);


                const float y_velocity  = sin(gamma);
                const float x_velocity  = sinTheta * cosGamma;
                const float z_velocity  = cosTheta * cosGamma;


                particles.push_back(InitialParticleProperties{
                        .position   = emitterPOS,
                        .velocity   = (emitterOrientation * float3{ x_velocity, y_velocity, z_velocity }) * 10,
                        .lifespan   = properties.maxLife,
                    });
            }
        }


        auto& DrawInstanceMesh(
            ParticleSystemInterface&        particleSystem,
            UpdateTask&                     particleSystemUpdate,
            UpdateDispatcher&               dispatcher,
            FrameGraph&                     frameGraph,
            TriMeshHandle                   mesh,
            ReserveConstantBufferFunction   reserveCB,
            ReserveVertexBufferFunction     reserveVB,
            CameraHandle                    camera,
            GBuffer&                        gbuffer,
            ResourceHandle                  depthTarget)
        {
            struct Resources
            {
                ReserveConstantBufferFunction   reserveCB;
                ReserveVertexBufferFunction     reserveVB;
                TriMeshHandle                   mesh;

                FrameResourceHandle             albedoTarget;
                FrameResourceHandle             materialTarget;
                FrameResourceHandle             normalTarget;
                FrameResourceHandle             depthTarget;
            };

            return frameGraph.AddNode<Resources>(
                Resources{
                    reserveCB,
                    reserveVB,
                    mesh
                },
                [&](FrameGraphNodeBuilder& builder, Resources& local)
                {
                    builder.AddDataDependency(particleSystemUpdate);

                    local.albedoTarget      = builder.RenderTarget(gbuffer.albedo);
                    local.materialTarget    = builder.RenderTarget(gbuffer.MRIA);
                    local.normalTarget      = builder.RenderTarget(gbuffer.normal);
                    local.depthTarget       = builder.DepthTarget(depthTarget);
                },
                [this, camera = camera](Resources& local, const ResourceHandler& resources, Context& ctx, iAllocator& allocator)
                {
                    if (!particles.size())
                        return;

                    struct InstanceData {
                        float4 Position;
                        float4 params;
                    };

                    VBPushBuffer instanceBuffer{ local.reserveVB(particles.size() * sizeof(InstanceData)) };
                    CBPushBuffer constantBuffer{ local.reserveCB(sizeof(Camera::ConstantBuffer)) };

                    TriMesh* mesh = GetMeshResource(local.mesh);
                    VertexBufferList instancedBuffer;

                    VertexBufferDataSet dataSet{ SET_MAP_OP, particles,
                        [](const TY_ParticleData& particle) -> InstanceData
                        {
                            return { particle.position, float4{ particle.age, 0, 0, 0 } };
                        },
                        instanceBuffer };

                    ConstantBufferDataSet constants{ GetCameraConstants(camera), constantBuffer };

                    instancedBuffer.push_back(dataSet);

                    DescriptorHeap descHeap1{};
                    descHeap1.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(0), 1, &allocator);

                    DescriptorHeap descHeap2{};
                    descHeap2.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(1), 1, &allocator);

                    ctx.SetRootSignature(resources.renderSystem().Library.RSDefault);
                    ctx.SetPipelineState(resources.GetPipelineState(INSTANCEPARTICLEDPASS));

                    ctx.SetGraphicsConstantBufferView(0, constants);
                    ctx.SetGraphicsDescriptorTable(3, descHeap1);
                    ctx.SetGraphicsDescriptorTable(4, descHeap2);

                    ctx.SetScissorAndViewports({
                        resources.GetResource(local.albedoTarget),
                        resources.GetResource(local.materialTarget),
                        resources.GetResource(local.normalTarget), });

                    ctx.SetRenderTargets(
                        {   resources.GetResource(local.albedoTarget),
                            resources.GetResource(local.materialTarget),
                            resources.GetResource(local.normalTarget), },
                        true,
                        resources.GetResource(local.depthTarget));

                    ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);

                    ctx.AddIndexBuffer(mesh);
                    ctx.AddVertexBuffers(mesh,
                        mesh->GetHighestLoadedLodIdx(),
                        {   VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
                            VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
                            VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT },
                        &instancedBuffer);

                    ctx.DrawIndexedInstanced(mesh->lods[0].GetIndexCount(), 0, 0, particles.size(), 0);
                });
        }

        void DrawInstanceMeshShadowPass(
            TriMeshHandle                   mesh,
            ReserveConstantBufferFunction   reserveCB,
            ReserveVertexBufferFunction     reserveVB,
            ConstantBufferDataSet*          passConstants,
            ResourceHandle*                 depthTarget,
            size_t                          depthTargetCount,
            const ResourceHandler&          resources,
            Context&                        ctx,
            iAllocator&                     allocator)
        {
            if (!particles.size())
                return;

            struct InstanceData {
                float4 Position;
                float4 params;
            };

            VBPushBuffer instanceBuffer{ reserveVB(particles.size() * sizeof(InstanceData)) };

            TriMesh* meshResource = GetMeshResource(mesh);
            VertexBufferList instancedBuffer;

            VertexBufferDataSet dataSet{ SET_MAP_OP, particles,
                [](const TY_ParticleData& particle) -> InstanceData
                {
                    return { particle.position, float4{ particle.age, 0, 0, 0 } };
                },
                instanceBuffer };


            instancedBuffer.push_back(dataSet);

            DescriptorHeap descHeap1{};
            descHeap1.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(0), 1, &allocator);

            DescriptorHeap descHeap2{};
            descHeap2.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(1), 1, &allocator);

            ctx.SetRootSignature(resources.renderSystem().Library.RSDefault);
            ctx.SetPipelineState(resources.GetPipelineState(INSTANCEPARTICLEDEPTHDPASS));

            ctx.SetGraphicsDescriptorTable(3, descHeap1);
            ctx.SetGraphicsDescriptorTable(4, descHeap2);

            ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);

            ctx.AddIndexBuffer(meshResource);
            ctx.AddVertexBuffers(meshResource,
                meshResource->GetHighestLoadedLodIdx(),
                {   VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION   },
                &instancedBuffer);

            struct PassConstants
            {
                struct PlaneMatrices
                {
                    float4x4 ViewI;
                    float4x4 PV;
                }matrices[6];
            };

            for (size_t I = 0; I < depthTargetCount; I++)
            {
                ctx.SetGraphicsConstantBufferView(0, passConstants[I]);
                ctx.SetGraphicsConstantBufferView(1, passConstants[I]);

                ctx.SetScissorAndViewports({ depthTarget[I] });
                ctx.SetRenderTargets({}, true, depthTarget[I]);
                ctx.DrawIndexedInstanced(meshResource->lods[0].GetIndexCount(), 0, 0, particles.size(), 0);
            }
        }


        Vector<TY_ParticleData> particles;
    };


    inline static const ComponentID ParticleEmitterID = GetTypeGUID(ParticleEmitterComponent);

    struct ParticleEmitterData
    {
        ParticleSystemInterface*    parentSystem = nullptr;
        NodeHandle                  node;

        EmitterProperties           properties;

        void Emit(double dT)
        {
            parentSystem->Emit(dT, properties, node);
        }
    };


    using ParticleEmitterHandle     = Handle_t<32, ParticleEmitterID>;
    using ParticleEmitterComponent  = BasicComponent_t<ParticleEmitterData, ParticleEmitterHandle, ParticleEmitterID>;
    using ParticleEmitterView       = ParticleEmitterComponent::View;


    UpdateTask& UpdateParticleEmitters(UpdateDispatcher& dispatcher, double dT)
    {
        struct _ {};

        return dispatcher.Add<_>(
            [](auto&, auto& data){},
            [=](auto&, auto& threadMemory)
            {
                auto& particleEmitters = ParticleEmitterComponent::GetComponent();

                for (auto& emitter : particleEmitters)
                    emitter.componentData.Emit(dT);
            });
    }
}


/**********************************************************************

Copyright (c) 2021 Robert May

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
