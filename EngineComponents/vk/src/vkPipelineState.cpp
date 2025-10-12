#include "vkPipelineState.hpp"
#include "vkRenderSystem.hpp"
#include <ThreadUtilities.hpp>

namespace VK_internal
{
	vkPipelineInterface::vkPipelineInterface(iAllocator& allocator) :
		vkLayouts	{ allocator },
		heapLayouts	{ allocator } {}

	vkPipelineInterface::~vkPipelineInterface()
	{
	}

	const DesciptorHeapLayout&	vkPipelineInterface::GetDescHeap(uint32_t idx) const noexcept
	{
		return heapLayouts[idx];
	}

	DeviceRootSignature_ptr	vkPipelineInterface::GetAPIObject() const noexcept
	{
		return layout;
	}

	void vkPipelineInterface::Release() 
	{
	    
	}

	vkPipelineState::vkPipelineState(vkPipelineState&& rhs) :
		interface		{ std::exchange(rhs.interface,		nullptr) },
	    pipelineState	{ std::exchange(rhs.pipelineState,	nullptr) } {}


	vkPipelineState& vkPipelineState::operator=(vkPipelineState&& rhs)
	{
		interface		= std::exchange(rhs.interface,		nullptr);
	    pipelineState	= std::exchange(rhs.pipelineState,	nullptr);

		return *this;
	}


	void vkStateTable::RegisterLoader(PSOHandle state, LOADSTATE_FN FN)
	{
		std::unique_lock ul{ m };

		auto ptr = std::unique_ptr<PipelineObject>(&allocator->allocate<PipelineObject>(
			PipelineObject{
			.loader = FN,
			.state = PipelineState::Unloaded,
			}));

		pipelineStates.insert((uint64_t)state.to_uint(), std::move(ptr));
	}


	void vkStateTable::QueueLoad(PSOHandle state)
	{
		PipelineObject* object = nullptr;

	    {
			std::shared_lock sl{ m };
			object = pipelineStates.find(state)->get();
		}

		if (object)
		{
			auto state = std::atomic_ref{ object->state };
			if (state != PipelineState::Loading)
			{
				auto expected = state.load(std::memory_order_relaxed);
				while (!state.compare_exchange_strong(expected, PipelineState::Loading))
				{
				    if (expected == PipelineState::Loading)
						return;
				}

				auto& vkRS = static_cast<vkRenderSystem&>(IRenderSystem::GetInstance());
				auto& workItem = CreateWorkItem(
					[object](iAllocator& tempAllocator)
					{
						auto res = object->loader(IRenderSystem::GetInstance(), tempAllocator);
						if (res.pipelineState != nullptr)
						{
							object->pipeline.pipelineState	= res.pipelineState.As<VkPipeline_T>();
							object->pipeline.interface		= static_cast<const vkPipelineInterface*>(res.pipelineInterface);
							object->state					= PipelineState::Loaded;

							//TODO: Release old pipeline objects
						}
					}, vkRS.allocator, vkRS.allocator);

				threads->AddWork(&workItem);
				return;
			}
		}
	}


	vkPipelineState* vkStateTable::GetPSO(PSOHandle state, iAllocator& tempAllocator)
	{
		PipelineObject* object = nullptr;

		{
			std::shared_lock sl{ m };
			object = pipelineStates.find(state)->get();
		}

		if (object)
		{
			auto state = std::atomic_ref{ object->state };
			if (state != PipelineState::Loading)
			{
				auto expected = state.load(std::memory_order_relaxed);
				while (!state.compare_exchange_strong(expected, PipelineState::Loading))
				{
					if (expected == PipelineState::Loading)
						return object->Wait();

					if (expected == PipelineState::Loaded)
						return &object->pipeline;
				}

				auto& vkRS = static_cast<vkRenderSystem&>(IRenderSystem::GetInstance());
				auto [pipelineState, pipelineInterface]	= object->loader(IRenderSystem::GetInstance(), tempAllocator);
				object->pipeline.pipelineState			= pipelineState.As<VkPipeline_T>();
				object->pipeline.interface				= static_cast<const vkPipelineInterface*>(pipelineInterface);
				object->state							= PipelineState::Loaded;

				return &object->pipeline;
			}
			else
				return object->Wait();
		}
		else
			return nullptr;
	}
}
