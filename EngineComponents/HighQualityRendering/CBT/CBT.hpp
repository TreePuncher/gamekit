#pragma once
#include <atomic>
#include <ResourceHandles.hpp>

#include "Containers.hpp"
#include "DoOnce.hpp"


namespace FlexKit
{
	class IRenderSystem;

	uint64_t FindMSB(uint64_t) noexcept;
	uint64_t FindLSB(uint64_t) noexcept;

	uint ipow(uint32_t base, uint32_t exp) noexcept;

	inline uint32_t GetCBTSizeBitSize(const uint32_t maxDepth)
	{
		const uint32_t bitsRequired = ((ipow(2, maxDepth + 2)) - (maxDepth + 3));
		return bitsRequired;
	}

	inline uint32_t GetCBTSizeBytes(const uint32_t maxDepth, const uint32_t numTrees) noexcept
	{
		auto bitsRequired = GetCBTSizeBitSize(maxDepth) * numTrees;
		return (bitsRequired / 8) + ((bitsRequired % 8) != 0 ? 1 : 0);
	}

	struct CBTBufferDescription
	{
		uint32_t cbtTreeCount	= 1;
		uint32_t maxDepth		= 4;
	};


	constexpr PSOHandle SumReductionCBT		= PSOHandle(GetTypeGUID(UpdateCBTTree));


	struct CBTBuffer
	{
		CBTBuffer(IRenderSystem& IN_renderSystem, iAllocator& persistent);
		~CBTBuffer();

		void Initialize(const CBTBufferDescription & = {});
		void SumReduction_GPU(class FrameGraph&);
		void Upload(class FrameGraph&);
		void Clear(class FrameGraph&);

		void ReloadShaders();

		ResourceHandle	GetBuffer()		const { return buffer; }
		uint32_t		GetMaxDepth()	const { return maxDepth; }

		struct PipelineStates
		{
			void Initialize(IRenderSystem& renderSystem);
			void QueueReload(IRenderSystem& renderSystem);

			static void _InitializeStates(PipelineStates& states, IRenderSystem& renderSystem);

			using TY_helper = decltype(FlexKit::MakeDoOnce(&_InitializeStates));

			TY_helper init = FlexKit::MakeDoOnce(&_InitializeStates);
			
			PSOHandle sumReduce;
		};

		inline static PipelineStates states;

		uint32_t Depth(uint32_t Bit) const;
		uint32_t HeapToBitIndex(uint32_t x) const noexcept;
		uint64_t GetBitOffset(uint64_t idx) const noexcept;

		uint64_t GetHeapValue(uint32_t heapIdx) const noexcept;
		uint32_t DecodeNode(int32_t leafID)		const noexcept;
		uint32_t DecodeBitidx(int32_t idx)		const noexcept;

		void		SetBit(uint64_t idx, bool b) noexcept;
		uint64_t	GetBit(uint64_t idx, bool b) const noexcept;

		uint64_t	ReadValue(uint64_t start, uint64_t bitWidth) const noexcept;
		void		WriteValue(uint64_t start, uint64_t bitWidth, uint64_t value) noexcept;

		void		SumReduction() noexcept;

		uint64_t	GetNthPrefixSum(uint64_t idx);

		void*		data()		noexcept { return bitField.data(); }
		size_t		ByteSize()	noexcept { return bitField.ByteSize(); }

		uint HeapIndexToBitIndex(const uint k) const noexcept { return k * ipow(2, maxDepth - FindMSB(k)) - ipow(2, maxDepth); }

	private:
		uint32_t			maxDepth	= 0;
		uint32_t			bufferSize	= 0;
		ResourceHandle		buffer		= InvalidHandle;
		IRenderSystem&		renderSystem;
		Vector<uint64_t>	bitField;
	};


	struct CBTMemoryManager
	{
	public:
		CBTMemoryManager(IRenderSystem& IN_renderSystem, iAllocator& IN_persistent) :
			cbt{ IN_renderSystem, IN_persistent } {}


		void Update(class FrameGraph&);
		void DrawDebugVIS(class FrameGraph&, ResourceHandle renderTarget);

		ResourceHandle		buffer;
		CBTBuffer			cbt;
	};

}


/**********************************************************************

Copyright (c) 2024 Robert May

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
