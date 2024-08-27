#include "FrameGraph.h"
#include "Components.h"

namespace FlexKit
{
	static const PSOHandle INSTANCEPARTICLEDPASS		= PSOHandle(GetTypeGUID(INSTANCEPARTICLEDPASS));
	static const PSOHandle INSTANCEPARTICLEDEPTHDPASS	= PSOHandle(GetTypeGUID(INSTANCEPARTICLEDEPTHDPASS));

	LoadPipelineStateRes CreateParticleMeshInstancedDepthPSO(RenderSystem* RS);
	LoadPipelineStateRes CreateParticleMeshInstancedPSO(RenderSystem* RS);

	struct DrawInstancedParticles
	{
		ReserveConstantBufferFunction& reserveCB;
		ReserveVertexBufferFunction& reserveVB;
		TriMeshHandle					mesh;

		FrameResourceHandle				albedoTarget;
		FrameResourceHandle				materialTarget;
		FrameResourceHandle				normalTarget;
		FrameResourceHandle				depthTarget;
	};

	void DrawInstanceMesh(
		ParticleSystemInterface&		particleSystem,
		UpdateTask&						particleSystemUpdate,
		UpdateDispatcher&				dispatcher,
		FrameGraph&						frameGraph,
		TriMeshHandle					mesh,
		ReserveConstantBufferFunction	reserveCB,
		ReserveVertexBufferFunction		reserveVB,
		CameraHandle					camera,
		GBuffer&						gbuffer,
		ResourceHandle					depthTarget){}

	void DrawInstanceMeshShadowPass(
		TriMeshHandle					mesh,
		ReserveConstantBufferFunction	reserveCB,
		ReserveVertexBufferFunction		reserveVB,
		ConstantBufferDataSet*			passConstants,
		ResourceHandle*					depthTarget,
		size_t							depthTargetCount,
		const ResourceHandler&			resources,
		Context&						ctx,
		iAllocator&						allocator) {}
}
