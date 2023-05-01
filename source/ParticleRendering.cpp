#include "Particles.h"
#include "FrameGraph.h"
#include "ClusteredRendering.h"

namespace FlexKit
{
	ID3D12PipelineState* CreateParticleMeshInstancedPSO(RenderSystem* RS)
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


		D3D12_RASTERIZER_DESC		Rast_Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable = true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature = RS->Library.RSDefault;
			PSO_Desc.VS = VShader;
			PSO_Desc.PS = PShader;
			PSO_Desc.RasterizerState = Rast_Desc;
			PSO_Desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets = 3;
			PSO_Desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;        // Albedo
			PSO_Desc.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;    // Specular
			PSO_Desc.RTVFormats[2] = DXGI_FORMAT_R16G16B16A16_FLOAT;    // Normal
			PSO_Desc.SampleDesc.Count = 1;
			PSO_Desc.SampleDesc.Quality = 0;
			PSO_Desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout = { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
			PSO_Desc.DepthStencilState = Depth_Desc;
			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		SETDEBUGNAME(PSO, "DrawParticleMeshInstances");

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateParticleMeshInstancedDepthPSO(RenderSystem* RS)
	{
		auto VShader = RS->LoadShader("ParticleMeshInstanceDepthVS", "vs_6_0", "assets\\shaders\\ParticleShadowMapping.hlsl");
		auto GShader = RS->LoadShader("ParticleMeshInstanceDepthGS", "gs_6_0", "assets\\shaders\\ParticleShadowMapping.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "POSITION",	    0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	                D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "INSTANCEPOS",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0,                  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEARGS",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, sizeof(float4),     D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable = true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature = RS->Library.RSDefault;
			PSO_Desc.VS = VShader;
			PSO_Desc.GS = GShader;
			PSO_Desc.RasterizerState = Rast_Desc;
			PSO_Desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets = 0;
			PSO_Desc.SampleDesc.Count = 1;
			PSO_Desc.SampleDesc.Quality = 0;
			PSO_Desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout = { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
			PSO_Desc.DepthStencilState = Depth_Desc;
			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		SETDEBUGNAME(PSO, "DrawParticleMeshInstancesDepth");

		return PSO;
	}


#if 0
	DrawInstancedParticles& DrawInstanceMesh(
		ParticleSystemInterface&		particleSystem,
		UpdateTask&						particleSystemUpdate,
		UpdateDispatcher&				dispatcher,
		FrameGraph&						frameGraph,
		TriMeshHandle					mesh,
		ReserveConstantBufferFunction	reserveCB,
		ReserveVertexBufferFunction		reserveVB,
		CameraHandle					camera,
		GBuffer&						gbuffer,
		ResourceHandle					depthTarget)
	{
		return frameGraph.AddNode<DrawInstancedParticles>(
			DrawInstancedParticles{
				reserveCB,
				reserveVB,
				mesh
			},
			[&](FrameGraphNodeBuilder& builder, Resources& local)
			{
				builder.AddDataDependency(particleSystemUpdate);

				local.albedoTarget = builder.RenderTarget(gbuffer.albedo);
				local.materialTarget = builder.RenderTarget(gbuffer.MRIA);
				local.normalTarget = builder.RenderTarget(gbuffer.normal);
				local.depthTarget = builder.DepthTarget(depthTarget);
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
				ctx.SetGraphicsDescriptorTable(4, descHeap1);
				ctx.SetGraphicsDescriptorTable(5, descHeap2);

				ctx.SetScissorAndViewports({
					resources.GetResource(local.albedoTarget),
					resources.GetResource(local.materialTarget),
					resources.GetResource(local.normalTarget), });

				ctx.SetRenderTargets(
					{ resources.GetResource(local.albedoTarget),
						resources.GetResource(local.materialTarget),
						resources.GetResource(local.normalTarget), },
					true,
					resources.GetResource(local.depthTarget));

				ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);

				ctx.AddIndexBuffer(mesh);
				ctx.AddVertexBuffers(mesh,
					mesh->GetHighestLoadedLodIdx(),
					{ VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
						VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
						VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT },
					&instancedBuffer);

				ctx.DrawIndexedInstanced(mesh->lods[0].GetIndexCount(), 0, 0, particles.size(), 0);
			});
	}

	void DrawInstanceMeshShadowPass(
		TriMeshHandle					mesh,
		ReserveConstantBufferFunction	reserveCB,
		ReserveVertexBufferFunction		reserveVB,
		ConstantBufferDataSet*			passConstants,
		ResourceHandle*					depthTarget,
		size_t							depthTargetCount,
		const ResourceHandler&			resources,
		Context&						ctx,
		iAllocator&						allocator)
	{
		ctx.BeginEvent_DEBUG("Particle Shadow Map Pass");

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

		ctx.SetGraphicsDescriptorTable(4, descHeap1);
		ctx.SetGraphicsDescriptorTable(5, descHeap2);

		ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);

		ctx.AddIndexBuffer(meshResource);
		ctx.AddVertexBuffers(meshResource,
			meshResource->GetHighestLoadedLodIdx(),
			{ VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION },
			&instancedBuffer);

		for (size_t I = 0; I < depthTargetCount; I++)
		{
			ctx.SetGraphicsConstantBufferView(0, passConstants[I]);
			ctx.SetGraphicsConstantBufferView(1, passConstants[I]);

			const DepthStencilView_Options DSV_desc = {
							0, 0,
							depthTarget[I] };

			ctx.SetScissorAndViewports({ depthTarget[I] });
			ctx.SetRenderTargets2({}, 0, DSV_desc);
			ctx.DrawIndexedInstanced(meshResource->lods[0].GetIndexCount(), 0, 0, particles.size(), 0);
		}

		ctx.EndEvent_DEBUG();
	}
#endif
}
