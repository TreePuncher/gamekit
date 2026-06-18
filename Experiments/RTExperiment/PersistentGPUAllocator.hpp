#include <RenderSystemInterface.hpp>
#include <Containers.hpp>

#include "dxRenderSystem.hpp"

struct PersistentAllocator
{
	PersistentAllocator(uint32_t IN_blockSize, FlexKit::iAllocator& IN_allocator) :
		freeList	{ &IN_allocator },
        blockSize	{ IN_blockSize	},
	    root	{
			.frameID	= 0,
			.begin		= 0,
			.end		= uint32_t((64 * MEGABYTE) / IN_blockSize),
	    }
	{
		resource	= FlexKit::IRenderSystem::GetInstance().CreateGPUResource(FlexKit::GPUResourceDesc::UAVResource(64 * MEGABYTE));
		buffer		= FlexKit::IRenderSystem::GetInstance().GetDevicePointer(resource);
	}

	struct AllocationNode
	{
		uint64_t frameID;
		uint32_t begin;
		uint32_t end;
		uint32_t freeBlocks;

		bool			allocated = false;
		AllocationNode* parent = nullptr;

		std::unique_ptr<AllocationNode> left;
		std::unique_ptr<AllocationNode> right;

		uint32_t size() const { return end - begin; }

		bool Leaf() const
		{
			return (left == nullptr && right == nullptr);
		}

		bool operator < (const AllocationNode& rhs) noexcept
		{
			return size() < rhs.size();
		}

		void SetAllocated()
		{
			if (Leaf())
			{
				allocated = true;
				if (parent)
					parent->SetAllocated();
			}
			else
		    {
				allocated = left->allocated && right->allocated;
				if (allocated && parent != nullptr)
					parent->SetAllocated();
			}
		}

		void SetFree()
		{
		    
		}


		void Split()
		{
			auto splitSizes = SplitSizes();

			left	= std::make_unique<AllocationNode>();
			right	= std::make_unique<AllocationNode>();

			right->parent = this;
			left->parent = this;

			left->begin = begin;
			left->end	= begin + splitSizes.first;
			
			right->begin	= left->end;
			right->end		= end;
		}

		void Collapse()
		{
			left.reset();
			right.reset();
		}

		uint32_t BlockCount()
		{
			return end - begin;
		}

		std::pair<size_t, size_t> SplitSizes()
		{
			const auto numBlocks = BlockCount();
			return { 3 * numBlocks >> 2, numBlocks >> 2 };
		}
	};

	std::optional<FlexKit::GPURange> AllocBlocks(uint32_t allocationSize, size_t frameID)
	{
		auto allocation = FindFreeNode(&root, allocationSize);
		if (allocation == nullptr)
			return {};

		allocation->SetAllocated();

		FlexKit::GPURange out;
		out.size		= blockSize * allocation->size();
	    out.resource	= resource;
		out.offset		= blockSize * allocation->begin;
		out.devicePtr	= dx_Internal::dxRenderSystem::_GetInstance().GetDevicePointer(resource);
		out.devicePtr  += out.offset;
	    
	    return out;
	}

	static AllocationNode* FindFreeNode(AllocationNode* node, const uint32_t blockCount)
	{
		if (node->Leaf() && !node->allocated)
		{
			auto sizes = node->SplitSizes();

			if (sizes.second > 0 && sizes.first >= blockCount || sizes.second >= blockCount)
			{
				node->Split();

				if (node->right->BlockCount() >= blockCount)
					return FindFreeNode(node->right.get(), blockCount);
				else
				    return FindFreeNode(node->left.get(), blockCount);
			}
			else
			{
				return node;
			}
		}
		else
		{
			if (!node->right->allocated && node->right->BlockCount() >= blockCount)
				return FindFreeNode(node->right.get(), blockCount);
		    
			if (!node->left->allocated && node->left->BlockCount() >= blockCount)
		        return FindFreeNode(node->left.get(), blockCount);

			while(node->parent != nullptr)
			{
				if (auto left = node->parent->left.get(); left != node)
					if (!left->allocated && left->size() >= blockCount)
					    return FindFreeNode(node->parent->left.get(), blockCount);

				node = node->parent;
			}
		}

		return nullptr;
	}

	void Free(FlexKit::DeviceAddressRange addressRange)
	{
	    //freeList.emplace_back(*res);
	}

	uint64_t							buffer;
	uint64_t							blockSize;
	AllocationNode						root;
	FlexKit::Vector<AllocationNode*>	freeList;
	FlexKit::ResourceHandle				resource;
};
