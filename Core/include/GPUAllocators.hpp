#include <BuildSettings.hpp>
#include <Containers.hpp>
#include <MemoryUtilities.hpp>
#include <ResourceHandles.hpp>

namespace FlexKit
{
	class GPUBlockAllocator
	{
	public:
		~GPUBlockAllocator();

		void							Initialize	(ResourceHandle resource, uint64_t in_gpuBegin, uint32_t blockSize, iAllocator* IN_allocator);

		std::optional<GPURange>			Alloc_ST	(const size_t size, uint64_t completedIdx) noexcept;
		auto							Alloc		(const size_t size, uint64_t completedIdx) noexcept;

		void							Release_ST	(const DevicePointer range, uint64_t lockIdx, uint64_t completed) noexcept;
		void							Release		(const DevicePointer range, uint64_t lockIdx, uint64_t completed);

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

			void Split			(iAllocator* allocator);
			void Collapse		(iAllocator* allocator);
			bool Collapsable	(uint64_t completed);

			void Release(iAllocator* allocator);

			std::pair<size_t, size_t> SplitSizes();
			size_t BlockCount() const noexcept { return end - begin; }
			size_t FreeCount() const noexcept;

		};

		Node* LocateNode(size_t offset);

		uint64_t					gpuBegin	= 0;
		uint32_t					blockSize	= 0;
		ResourceHandle				resource	= InvalidHandle;

		Node						root;
		Vector<Node*>				freeList;
		iAllocator*					allocator;
		std::mutex					mutex;
	};

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
