#include "PersistentGPUAllocator.hpp"

namespace FlexKit
{

	uint32_t PersistentAllocator::AllocationNode::size() const
	{
	    return end - begin;
	}

	bool PersistentAllocator::AllocationNode::Leaf() const
	{
		return (children == nullptr);
	}

	bool PersistentAllocator::AllocationNode::operator < (const AllocationNode& rhs) const noexcept
	{
		return size() < rhs.size();
	}

	void PersistentAllocator::AllocationNode::SetAllocated()
	{
		if (Leaf())
		{
			allocated = true;
			if (parent)
				parent->SetAllocated();
		}
		else
		{
			allocated = left()->allocated && right()->allocated;
			if (allocated && parent != nullptr)
				parent->SetAllocated();
		}
	}

	PersistentAllocator::AllocationNode* PersistentAllocator::AllocationNode::GetSibling(AllocationNode* _ptr)
	{
		if (left() == _ptr)
			return right();
		else if (right() == _ptr)
			return left();
		else
			return nullptr;
	}

	void PersistentAllocator::AllocationNode::SetFree(iAllocator* allocator, uint64_t lockUntil)
	{
		allocated	= false;
		frameID		= lockUntil;

		auto sibling = parent->GetSibling(this);

		if (!sibling->allocated)
		{
			parent->Collapse(allocator);
		    parent->SetFree(allocator, lockUntil);
		}
	}


	void PersistentAllocator::AllocationNode::Split(iAllocator* allocator)
	{
		const auto splitSizes = SplitSizes();

		children = allocator->allocate<AllocationNode[2]>();

		right()->parent	= this;
		left()->parent	= this;

		left()->begin	= begin;
		left()->end		= begin + splitSizes.blockASize;

		right()->begin	= begin + splitSizes.blockASize;
		right()->end	= end;
	}

	void PersistentAllocator::AllocationNode::Collapse(iAllocator* allocator)
	{
		if (!Leaf())
		{
			allocator->release(children);
			children = nullptr;
		}
	}

	uint32_t PersistentAllocator::AllocationNode::BlockCount() const noexcept
	{
		return end - begin;
	}

	PersistentAllocator::AllocationNode::SplitSizesRes PersistentAllocator::AllocationNode::SplitSizes() const noexcept
	{
		const auto numBlocks = BlockCount();
		return { 3 * numBlocks >> 2, numBlocks >> 2 };
	}

	void PersistentAllocator::AllocationNode::Release(iAllocator* allocator)
	{
		Collapse(allocator);
	}


	PersistentAllocator::PersistentAllocator(uint32_t IN_blockCount, uint32_t IN_blockSize, iAllocator& IN_allocator) :
		allocator	{ &IN_allocator },
		blockSize	{ IN_blockSize },
		root {
			.frameID	= 0,
			.begin		= 0,
			.end		= IN_blockCount,
		}
	{
		resource	= IRenderSystem::GetInstance().CreateGPUResource(GPUResourceDesc::UAVResource(IN_blockCount * IN_blockSize));
		buffer		= IRenderSystem::GetInstance().GetDevicePointer(resource);
	}

	PersistentAllocator::~PersistentAllocator()
	{
		root.Release(allocator);
	}

	std::optional<GPURange> PersistentAllocator::AllocBlocks(uint32_t allocationSize, uint64_t frameID)
	{
		auto allocation = FindFreeNode(&root, allocationSize, allocator);
		if (allocation == nullptr)
			return {};

		allocation->SetAllocated();

		const uint32_t byteOffset = blockSize * allocation->begin;

		return GPURange{
			.devicePtr	= IRenderSystem::GetInstance().GetDevicePointer(resource) + byteOffset,
			.offset		= byteOffset,
			.size		= (uint32_t)blockSize * allocation->size(),
			.resource	= resource,
		};
	}

	PersistentAllocator::AllocationNode* PersistentAllocator::FindFreeNode(AllocationNode* node, const uint32_t blockCount, iAllocator* allocator)
	{
		if (node->Leaf() && !node->allocated)
		{
			auto sizes = node->SplitSizes();

			if (sizes.blockBSize > 0 && sizes.blockASize >= blockCount || sizes.blockBSize >= blockCount)
			{
				node->Split(allocator);

				if (node->right()->BlockCount() >= blockCount)
					return FindFreeNode(node->right(), blockCount, allocator);
				else
					return FindFreeNode(node->left(), blockCount, allocator);
			}
			else
				return node;
		}
		else
		{
			if (!node->right()->allocated && node->right()->BlockCount() >= blockCount)
				return FindFreeNode(node->right(), blockCount, allocator);

			if (!node->left()->allocated && node->left()->BlockCount() >= blockCount)
				return FindFreeNode(node->left(), blockCount, allocator);

			while (node->parent != nullptr)
			{
				if (auto left = node->parent->left(); left != node)
					if (!left->allocated && left->size() >= blockCount)
						return FindFreeNode(node->parent->left(), blockCount, allocator);

				node = node->parent;
			}
		}

		return nullptr;
	}

	uint32_t PersistentAllocator::AddressToBlockOffset(DeviceAddressRange range) const
	{
		return (range.address - IRenderSystem::GetInstance().GetDevicePointer(resource)) / blockSize;
	}

	PersistentAllocator::AllocationNode* PersistentAllocator::FindSubNode(AllocationNode* node, uint32_t offset)
	{
		while(node->begin <= offset && node->end > offset)
		{
			if (node->Leaf())
				return node;

			auto splitSizes = node->SplitSizes();
			AllocationNode* nextNode = nullptr;
			
			if (node->begin + splitSizes.blockASize > offset)
				node = node->left();
			else
				node = node->right();
		}
		
		return nullptr;
	}

	void PersistentAllocator::Free(DeviceAddressRange addressRange)
	{
		auto node_ptr = FindSubNode(&root, AddressToBlockOffset(addressRange));
		const uint64_t submissionID = IRenderSystem::GetInstance().GetCurrentCounter();

		if (node_ptr)
		    node_ptr->SetFree(allocator, submissionID);
	}
}
