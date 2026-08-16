#include <RenderSystemInterface.hpp>
#include <Containers.hpp>

namespace FlexKit
{
	struct PersistentAllocator
	{
		PersistentAllocator(uint32_t IN_blockCount, uint32_t IN_blockSize, iAllocator& IN_allocator);
		~PersistentAllocator();

		struct AllocationNode
		{
			uint64_t frameID;
			uint32_t begin;
			uint32_t end;
			uint32_t freeBlocks;

			bool			allocated	= false;
			AllocationNode* parent		= nullptr;
			AllocationNode* children	= nullptr;

			AllocationNode* left()	const { return children + 0; }
			AllocationNode* right()	const { return children + 1; }

			AllocationNode* GetSibling(AllocationNode*);

			uint32_t	size() const;
			bool		Leaf() const;
			bool		operator < (const AllocationNode& rhs) const noexcept;

			void		SetAllocated();

			void		SetFree(iAllocator*, uint64_t);
			void		Split(iAllocator*);

			void		Collapse(iAllocator*);

			uint32_t	BlockCount() const noexcept;

			struct SplitSizesRes
			{
				uint32_t blockASize;
				uint32_t blockBSize;
			};

			SplitSizesRes SplitSizes() const noexcept;

			void Release(iAllocator*);
		};

		std::optional<GPURange> AllocBlocks(uint32_t allocationSize, uint64_t frameID);
		uint32_t				AddressToBlockOffset(DevicePointer) const;
		
	    static AllocationNode* FindFreeNode(AllocationNode* node, const uint32_t blockCount, iAllocator* allocator);
		static AllocationNode* FindSubNode(AllocationNode* node, uint32_t offset);

		void Free(DeviceAddressRange	addressRange);
		void Free(DevicePointer			address);

		uint64_t					buffer;
		uint64_t					blockSize;
		AllocationNode				root;
		ResourceHandle				resource;
		iAllocator*					allocator = nullptr;
	};

}
