#pragma once
#include <RenderSystemInterface.hpp>
#include <vulkan/vulkan.hpp>
#include <Containers.hpp>
#include <mutex>

namespace VK_internal
{
	using namespace FlexKit;

	struct vkPipelineInterface final : IPipelineInterface, NoCopy
	{
		vkPipelineInterface(iAllocator& allocator);
		virtual ~vkPipelineInterface() final;

		const DescriptorHeapLayout&	GetDescHeap(uint32_t idx) const noexcept final;
		DeviceRootSignature_ptr		GetAPIObject() const noexcept final;

		void Release() final;

		VkPipelineLayout				layout;
		VkDescriptorSetLayout			pushLayout;
		uint32_t						pushSet;
		uint32_t						pushConstantFlags;
		uint32_t						pushCount;
		Vector<VkDescriptorSetLayout>	vkLayouts;
		Vector<DescriptorHeapLayout>		heapLayouts;
		Vector<DescriptorHeapLayout>		pushLayouts;
	};

	struct vkPipelineState : IPipelineState, NoCopy
	{
		vkPipelineState() = default;

	    vkPipelineState(vkPipelineState&& rhs);
		vkPipelineState& operator= (vkPipelineState&& rhs);

		const IPipelineInterface*	GetInterface() const noexcept	final { return interface; }
		DevicePipelineState_ptr		GetDevicePipeState() const		final { return pipelineState; }

		const vkPipelineInterface*	interface		= nullptr;
		VkPipeline					pipelineState	= nullptr;
	};

	struct vkStateTable
	{
		vkStateTable(ThreadManager* IN_threads, iAllocator& IN_allocator) :
			allocator		{ IN_allocator },
			pipelineStates	{ IN_allocator },
			threads			{ IN_threads } {}

		enum class PipelineState
		{
			Unloaded,
		    Loaded,
			Loading,
			Error
		};

		struct PipelineObject
		{
			vkPipelineState pipeline;
			LOADSTATE_FN	loader;
			PipelineState	state;

			vkPipelineState* Wait()
			{
				std::atomic_ref<PipelineState> atomic_state{ state };

				while (atomic_state == PipelineState::Loading);

				return &pipeline;
			}
		};

		void RegisterLoader(PSOHandle state, LOADSTATE_FN FN);
		void QueueLoad(PSOHandle state);

		vkPipelineState* GetPSO(PSOHandle state, iAllocator&);


		std::shared_mutex							m;
		ThreadManager*								threads;
		HashTable<std::unique_ptr<PipelineObject>>	pipelineStates;
		iAllocator*									allocator;
	};
}
