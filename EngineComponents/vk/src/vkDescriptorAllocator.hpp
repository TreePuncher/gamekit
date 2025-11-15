#pragma once
#include <Containers.hpp>
#include <ResourceHandles.hpp>
#include <vulkan/vulkan.hpp>
#include <RenderSystemInterface.hpp>

namespace VK_internal
{
	using namespace FlexKit;

	struct HeapAllocatorDescription
	{
		uint64_t CPUBegin;
		uint64_t GPUBegin;
		uint64_t size;
		VkDevice device;
		VkBuffer buffer;
	};

	struct Alloc2Res
	{
		DescriptorRange range;
		uint64_t		offset;
	};

	class vkDescriptorHeapAllocator
	{
	public:
		~vkDescriptorHeapAllocator();

		void							Initialize	(const HeapAllocatorDescription& description, iAllocator* IN_allocator);

		std::optional<DescriptorRange>	Alloc_ST	(const size_t size, uint64_t completedIdx) noexcept;
		std::optional<DescriptorRange>	Alloc		(const size_t size, uint64_t completedIdx) noexcept;

		std::optional<Alloc2Res>		Alloc2_ST	(const size_t size, uint64_t completedIdx) noexcept;
		std::optional<Alloc2Res>		Alloc2		(const size_t size, uint64_t completedIdx) noexcept;

		std::optional<Alloc2Res>		Alloc2Temp_ST	(const size_t size, uint64_t completedIdx, uint64_t lockIdx) noexcept;
		std::optional<Alloc2Res>		Alloc2Temp		(const size_t size, uint64_t completedIdx, uint64_t lockIdx) noexcept;

		void							Release_ST	(const DescriptorRange range, uint64_t lockIdx, uint64_t completed) noexcept;
		void							Release		(const DescriptorRange range, uint64_t lockIdx, uint64_t completed);

		VkBuffer Heap() { return description.buffer; }
		private:

		struct Node
		{
			size_t begin;
			size_t end;
			size_t lockUntil;
			
			bool free		= true;
			Node* left		= nullptr;
			Node* right		= nullptr;
			Node* parent	= nullptr;

			void Split(FlexKit::iAllocator* allocator);
			void Collapse(FlexKit::iAllocator* allocator);
			bool Collapsable(uint64_t completed);

			void Release(iAllocator* allocator);

			std::pair<size_t, size_t> SplitSizes();
			size_t BlockCount() const noexcept { return (end - begin); }
			size_t FreeCount() const noexcept;

		};

		Node* LocateNode(size_t offset);

		Node						root;
		Vector<Node*>				freeList;
		iAllocator*					allocator;
		std::mutex					mutex;
		HeapAllocatorDescription	description;
	};

}
