#pragma once

#include "ClusteredRendering.hpp"
#include "FrameGraph.hpp"
#include "Graphics.hpp"

namespace FlexKit
{   /************************************************************************************************/


	struct BuildSceneRes
	{
		void* _ptr;
	};


	class GITechniqueInterface
	{
	public:
		virtual ~GITechniqueInterface() {}

		virtual void Init(FrameGraph& frameGraph) = 0;

		virtual BuildSceneRes BuildScene(
			FrameGraph&						frameGraph,
			Scene&							scene,
			GatherPassesTask&				passes,
			iAllocator&						allocator) = 0;

		virtual void RayTrace(
			UpdateDispatcher&				dispatcher,
			FrameGraph&						frameGraph,
			const CameraHandle				camera,
			GatherPassesTask&				passes,
			BuildSceneRes					bvh,
			ResourceHandle					depthTarget,
			FrameResourceHandle				renderTarget,
			GBuffer&						gbuffer,
			LightBufferUpdate&				lightBuffers,
			iAllocator*						allocator) = 0;
	};


	/************************************************************************************************/


	enum class EGITECHNIQUE
	{
		VXGL,
		RTGI,
		DISABLE,
		AUTOMATIC
	};


	class GlobalIlluminationEngine
	{
	public:
		GlobalIlluminationEngine(RenderSystem& renderSystem, iAllocator& allocator, EGITECHNIQUE technique = EGITECHNIQUE::AUTOMATIC);
		~GlobalIlluminationEngine();


		void Init(FrameGraph& frameGraph);

		BuildSceneRes BuildScene(
			FrameGraph&						frameGraph,
			Scene&							scene,
			GatherPassesTask&				passes,
			iAllocator&						allocator);

		void RayTrace(
			UpdateDispatcher&				dispatcher,
			FrameGraph&						frameGraph,
			const CameraHandle				camera,
			GatherPassesTask&				passes,
			BuildSceneRes					bvh,
			ResourceHandle					depthTarget,
			FrameResourceHandle				renderTarget,
			GBuffer&						gbuffer,
			LightBufferUpdate&				lightBuffers,
			iAllocator*						allocator);

	private:
		GITechniqueInterface*	technique = nullptr;
		iAllocator&				allocator;
	};


}   /************************************************************************************************/
