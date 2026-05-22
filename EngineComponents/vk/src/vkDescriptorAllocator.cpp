#include "vkDescriptorAllocator.hpp"

namespace VK_internal
{

	vkDescriptorHeapAllocator::~vkDescriptorHeapAllocator()
	{
	}


	/************************************************************************************************/


	std::optional<DescriptorRange> vkDescriptorHeapAllocator::Alloc_ST(const size_t size, uint64_t completedIdx, uint32_t alignment) noexcept
	{
		return Alloc2_ST(size, completedIdx, alignment).and_then([](auto res) { return std::optional{ res.range }; });
	}


	/************************************************************************************************/


	std::optional<Alloc2Res> vkDescriptorHeapAllocator::Alloc2_ST(const size_t size, uint64_t completedIdx, uint32_t alignment) noexcept
	{
		auto cmp_less = [](Node* lhs, Node* rhs) { return lhs->BlockCount() < rhs->BlockCount(); };
		const size_t blockCount = AlignedSize(size, alignment) / alignment;

		if (freeList.size() > 64) std::ranges::partial_sort(freeList, freeList.begin() + 32, cmp_less);
		else
			std::ranges::sort(freeList, cmp_less);

		for (auto& freeNode : freeList)
		{
			if (freeNode->BlockCount() > blockCount && freeNode->lockUntil <= completedIdx)
			{
				auto node = freeNode;
				auto potentialSplit = node->SplitSizes();
				auto& [leftSplit, rightSplit] = potentialSplit;

				freeList.erase(std::remove(freeList.begin(), freeList.end(), freeNode), freeList.end());

				while ((leftSplit > blockCount || rightSplit > blockCount) && (leftSplit > 0 && rightSplit > 0))
				{
					node->Split(allocator);
					auto lhs = node->left;
					auto rhs = node->right;

					if (rhs->BlockCount() == blockCount)
					{
						node = rhs;
						freeList.push_back(lhs);
						break;
					}
					else if (lhs->BlockCount() == blockCount)
					{
						node = lhs;
						freeList.push_back(rhs);
						break;
					}
					else if (rhs->BlockCount() > blockCount)
					{
						node = rhs;
						freeList.push_back(lhs);
					}
					else
					{
						node = lhs;
						freeList.push_back(rhs);
					}

					potentialSplit = node->SplitSizes();
				}

				node->free = false;

				const auto offset = node->begin * 64;

				if (blockCount == 0)
					DebugBreak();

				return Alloc2Res{
					.range{
					.begin	= { { (uint64_t)description.CPUBegin + offset },
								{ (uint64_t)description.GPUBegin + offset } },
					.size	= (uint32_t)size,
					.stride = (uint32_t)8,
				    },

				    .offset = offset };
			}
		}

		DebugBreak();
		return {};
	}


	/************************************************************************************************/


	void vkDescriptorHeapAllocator::Initialize(const HeapAllocatorDescription& IN_Description, FlexKit::iAllocator* IN_allocator)
	{
		root = Node{ .begin = 0, .end = IN_Description.size / 64 };
		freeList = Vector<Node*>{ IN_allocator };
		allocator = IN_allocator;

		freeList.push_back(&root);
		description = IN_Description;
	}


	/************************************************************************************************/


	std::optional<DescriptorRange> vkDescriptorHeapAllocator::Alloc(const size_t size, uint64_t completedIdx, uint32_t alignment) noexcept
	{
		std::scoped_lock lock{ mutex };

		return Alloc_ST(size, completedIdx, alignment);
	}

	std::optional<Alloc2Res> vkDescriptorHeapAllocator::Alloc2(const size_t size, uint64_t completedIdx, uint32_t alignment) noexcept
	{
		std::scoped_lock lock{ mutex };
		return Alloc2_ST(size, completedIdx, alignment);
	}


	/************************************************************************************************/


	std::optional<Alloc2Res> vkDescriptorHeapAllocator::Alloc2Temp_ST(const size_t size, uint64_t completedIdx, uint64_t lockIdx, uint32_t alignment) noexcept
	{
	    auto cmp_less = [](Node* lhs, Node* rhs) { return lhs->BlockCount() < rhs->BlockCount(); };
		const size_t blockCount = AlignedSize(size, 64) / 64;

		if (freeList.size() > 64) std::ranges::partial_sort(freeList, freeList.begin() + 32, cmp_less);
		else
			std::ranges::sort(freeList, cmp_less);

		for (auto& freeNode : freeList)
		{
			if (freeNode->BlockCount() == blockCount && freeNode->lockUntil <= completedIdx)
			{
				auto node = freeNode;
				node->lockUntil = lockIdx;
				const auto offset = node->begin * 64;

				if (blockCount == 0)
					DebugBreak();

				return Alloc2Res{
						.range{
						.begin	= { { (uint64_t)description.CPUBegin + offset },
									{ (uint64_t)description.GPUBegin + offset } },
						.size	= (uint32_t)size,
						.stride = (uint32_t)8,
				    },

				    .offset = offset };
			}
			else if (freeNode->BlockCount() > blockCount && freeNode->lockUntil <= completedIdx)
			{
				auto node = freeNode;
				auto potentialSplit = node->SplitSizes();
				auto& [leftSplit, rightSplit] = potentialSplit;

				freeList.erase(std::remove(freeList.begin(), freeList.end(), freeNode), freeList.end());

				while ((leftSplit > blockCount || rightSplit > blockCount) && (leftSplit > 0 && rightSplit > 0))
				{
					node->Split(allocator);
					auto lhs = node->left;
					auto rhs = node->right;

					if (rhs->BlockCount() == blockCount)
					{
						node = rhs;
						freeList.push_back(lhs);
						break;
					}
					else if (lhs->BlockCount() == blockCount)
					{
						node = lhs;
						freeList.push_back(rhs);
						break;
					}
					else if (rhs->BlockCount() > blockCount)
					{
						node = rhs;
						freeList.push_back(lhs);
					}
					else
					{
						node = lhs;
						freeList.push_back(rhs);
					}

					potentialSplit = node->SplitSizes();
				}

				node->free = true;
				node->lockUntil = lockIdx;
				freeList.push_back(node);

				const auto offset = node->begin * 64;

				if (blockCount == 0)
					DebugBreak();

				return Alloc2Res{
					.range{
					.begin	= { { (uint64_t)description.CPUBegin + offset },
								{ (uint64_t)description.GPUBegin + offset } },
					.size	= (uint32_t)size,
					.stride = (uint32_t)8,
				    },

				    .offset = offset };
			}
		}

		DebugBreak();
		return {};
	}


	std::optional<Alloc2Res> vkDescriptorHeapAllocator::Alloc2Temp(const size_t size, uint64_t completedIdx, uint64_t lockIdx, uint32_t alignment) noexcept
	{
		std::scoped_lock lock{ mutex };
		return Alloc2Temp_ST(size, completedIdx, lockIdx, alignment);
	}


	/************************************************************************************************/


	void vkDescriptorHeapAllocator::Release_ST(const DescriptorRange range, uint64_t lockIdx, uint64_t completedIdx) noexcept
	{
		auto& [cpu_ptr, gpu_ptr] = range.begin;
		auto offset1 = (gpu_ptr - description.GPUBegin);
		auto offset2 = (cpu_ptr - description.CPUBegin);

		FK_ASSERT(offset1 == offset2); // quick sanity check

		auto node = LocateNode(offset1);
		node->free		= true;
		node->lockUntil = lockIdx;

		while (node->parent)
		{
			if (node->parent->left->Collapsable(completedIdx) && node->parent->right->Collapsable(completedIdx))
			{
				auto n = node->parent->left != node ? node->parent->left : node->parent->right;

				freeList.erase(std::remove(freeList.begin(), freeList.end(), n), freeList.end());

				node = node->parent;

				node->Collapse(allocator);
			}
			else
				break;
		}

		freeList.push_back(node);
	}


	/************************************************************************************************/


	void vkDescriptorHeapAllocator::Release(const DescriptorRange range, uint64_t lockIdx, uint64_t completedIdx)
	{
		std::scoped_lock lock{mutex};

		Release_ST(range, lockIdx, completedIdx);
	}


	/************************************************************************************************/


	size_t vkDescriptorHeapAllocator::Node::FreeCount() const noexcept
	{
		if (left && right)
			return left->FreeCount() + right->FreeCount();
		if (free)
			return BlockCount();
		else
			return 0;
	}


	/************************************************************************************************/


	std::pair<size_t, size_t> vkDescriptorHeapAllocator::Node::SplitSizes()
	{
		const auto numBlocks = BlockCount();
		return { 3 * numBlocks >> 2, numBlocks >> 2 };
	}


	/************************************************************************************************/


	void vkDescriptorHeapAllocator::Node::Split(FlexKit::iAllocator* allocator)
	{
		auto lhs = &allocator->allocate<Node>();
		auto rhs = &allocator->allocate<Node>();
		const auto numBlocks = BlockCount();

		lhs->begin	= begin;
		lhs->end	= begin + 3 * (numBlocks >> 2);
		lhs->parent = this;

		rhs->begin	= begin + 3 * (numBlocks >> 2);
		rhs->end	= end;
		rhs->parent = this;

		left = lhs;
		right = rhs;
	}


	/************************************************************************************************/


	void vkDescriptorHeapAllocator::Node::Collapse(FlexKit::iAllocator* allocator)
	{
		allocator->release(left);
		allocator->release(right);

		left	= nullptr;
		right	= nullptr;
		free	= true;
	}


	/************************************************************************************************/


	bool vkDescriptorHeapAllocator::Node::Collapsable(uint64_t completed)
	{
		return (free && (lockUntil + 2) < completed);
	}


	/************************************************************************************************/


	void vkDescriptorHeapAllocator::Node::Release(iAllocator* allocator)
	{
		if (left)
		{
			left->Release(allocator);
			allocator->free(left);
		}

		if (right)
		{
			right->Release(allocator);
			allocator->free(right);
		}

		left	= nullptr;
		right	= nullptr;
	}

	/************************************************************************************************/


	vkDescriptorHeapAllocator::Node* vkDescriptorHeapAllocator::LocateNode(size_t offset)
	{
		auto node = &root;

		while (node->left && node->right)
		{
			if (node->left->begin <= offset && offset < node->left->end)
				node = node->left;
			else if (node->right->begin <= offset && offset < node->right->end)
				node = node->right;
		}

		if (!node->left && !node->right)
			return node;
		else
			return nullptr;
	}

}
