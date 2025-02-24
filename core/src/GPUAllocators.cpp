#include <GPUAllocators.hpp>


namespace FlexKit
{	/************************************************************************************************/


	void GPUBlockAllocator::Initialize(
			ResourceHandle	IN_resource,
			uint64_t		IN_gpuBegin,
			uint32_t		IN_blockSize,
			iAllocator*		IN_allocator)
	{
		const size_t resourceSize = RenderSystem::_GetInstance().GetResourceSize(IN_resource);
		
		root			= Node{ .begin = 0, .end = resourceSize /  IN_blockSize };
		freeList		= Vector<Node*>{ IN_allocator };
		allocator		= IN_allocator;

		freeList.push_back(&root);

		gpuBegin	= IN_gpuBegin;
		blockSize	= IN_blockSize;
		resource	= IN_resource;
	}


	/************************************************************************************************/


	GPUBlockAllocator::~GPUBlockAllocator()
	{
		root.Release(allocator);
		freeList.clear();

		gpuBegin	= 0;
		allocator	= nullptr;
	}


	/************************************************************************************************/


	std::optional<GPURange> GPUBlockAllocator::Alloc_ST(const size_t size, uint64_t completedIdx) noexcept
	{
		auto cmp_less = [](Node* lhs, Node* rhs) { return lhs->BlockCount() < rhs->BlockCount(); };

		if (freeList.size() > 64) std::ranges::partial_sort(freeList, freeList.begin() + 32, cmp_less );
		else
			std::ranges::sort(freeList, cmp_less);

		for (auto& freeNode : freeList)
		{
			if (freeNode->BlockCount() > size && freeNode->lockUntil <= completedIdx)
			{
				auto node						= freeNode;
				auto potentialSplit				= node->SplitSizes();
				auto& [leftSplit, rightSplit]	= potentialSplit;

				freeList.erase(std::remove(freeList.begin(), freeList.end(), freeNode), freeList.end());

				while((leftSplit > size || rightSplit > size)  && (leftSplit > 0 && rightSplit > 0))
				{
					node->Split(allocator);
					auto lhs = node->left;
					auto rhs = node->right;

					if (rhs->BlockCount() == size)
					{
						node = rhs;
						freeList.push_back(lhs);
						break;
					}
					else if (lhs->BlockCount() == size)
					{
						node = lhs;
						freeList.push_back(rhs);
						break;
					}
					else if (rhs->BlockCount() > size)
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

				const auto offset = blockSize * node->begin;

				if (size == 0)
					DebugBreak();

				return GPURange{
					.devicePtr	= { gpuBegin + offset },
					.offset		= offset,
					.size		= (uint32_t)size,
					.resource	= resource,
				};
			}
		}

#ifdef _DEBUG
		DebugBreak();
#endif

		return {};
	}


	/************************************************************************************************/


	auto GPUBlockAllocator::Alloc(const size_t size, uint64_t completedIdx) noexcept
	{
		std::scoped_lock lock{ mutex };

		return Alloc_ST(size, completedIdx);
	}


	/************************************************************************************************/


	void GPUBlockAllocator::Release_ST(const DevicePointer range, uint64_t lockIdx, uint64_t completedIdx) noexcept
	{
		auto offset1 = (range - gpuBegin) / blockSize;

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


	void GPUBlockAllocator::Release(const DevicePointer range, uint64_t lockIdx, uint64_t completedIdx)
	{
		std::scoped_lock lock{mutex};

		Release_ST(range, lockIdx, completedIdx);
	}


	/************************************************************************************************/


	size_t GPUBlockAllocator::Node::FreeCount() const noexcept
	{
		if (left && right)
			return left->FreeCount() + right->FreeCount();
		if (free)
			return BlockCount();
		else
			return 0;
	}


	/************************************************************************************************/


	std::pair<size_t, size_t> GPUBlockAllocator::Node::SplitSizes()
	{
		const auto numBlocks = BlockCount();
		return { 3 * numBlocks >> 2, numBlocks >> 2 };
	}


	/************************************************************************************************/


	void GPUBlockAllocator::Node::Split(FlexKit::iAllocator* allocator)
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


	void GPUBlockAllocator::Node::Collapse(FlexKit::iAllocator* allocator)
	{
		allocator->release(left);
		allocator->release(right);

		left	= nullptr;
		right	= nullptr;
		free	= true;
	}


	/************************************************************************************************/


	bool GPUBlockAllocator::Node::Collapsable(uint64_t completed)
	{
		return (free && (lockUntil + 2) < completed);
	}


	/************************************************************************************************/


	void GPUBlockAllocator::Node::Release(iAllocator* allocator)
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


	GPUBlockAllocator::Node* GPUBlockAllocator::LocateNode(size_t offset)
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

/**********************************************************************

Copyright (c) 2014-2025 Robert May

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
