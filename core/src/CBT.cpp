#include <bit>
#include <CBT.hpp>
#include <FrameGraph.hpp>
#include <RenderSystemInterface.hpp>
#include <Type.hpp>


namespace FlexKit
{	/************************************************************************************************/


	uint64_t FindMSB(uint64_t n) noexcept
	{
#ifdef WIN32
		if(std::is_constant_evaluated())
		{
			return std::bit_ceil(n);
		}
		else
		{
			unsigned long index = n;
			_BitScanReverse64(&index, n);
			return index;
		}
#else
		return std::bit_ceil(n);
#endif
	}


	/************************************************************************************************/


	uint64_t FindLSB(uint64_t n) noexcept
	{
#ifdef WIN32
		if(std::is_constant_evaluated())
		{
			return std::bit_floor(n);
		}
		else
		{
			unsigned long index = 0;
			_BitScanForward64(&index, n);
			return index;
		}
#else
		return std::bit_floor(n);
#endif
	}


	/************************************************************************************************/

	uint32_t ipow(uint32_t base, uint32_t exp) noexcept
	{
		uint result = 1;
		for (;;)
		{
			if (exp & 1)
				result *= base;
			exp >>= 1;
			if (!exp)
				break;
			base *= base;
		}

		return result;
	}

	/************************************************************************************************/


	CBTBuffer::CBTBuffer(IRenderSystem& IN_renderSystem, iAllocator& IN_persistent) :
		renderSystem	{ IN_renderSystem	},
		bitField		{ IN_persistent		}
	{
		states.Initialize(renderSystem);
	}


	/************************************************************************************************/


	CBTBuffer::~CBTBuffer()
	{
		renderSystem.ReleaseResource(buffer);
	}


	/************************************************************************************************/


	uint32_t CBTBuffer::Depth(uint32_t Bit) const
	{
		return maxDepth - log2(Bit + 1);
	}


	/************************************************************************************************/

	uint32_t CBTBuffer::HeapToBitIndex(uint32_t k) const noexcept
	{
		return k * ipow(2, maxDepth - FindMSB(k)) - ipow(2, maxDepth);
	}


	/************************************************************************************************/


	uint64_t CBTBuffer::GetBitOffset(uint64_t idx) const noexcept
	{
		const uint64_t d_k		= log2(idx);
		const uint64_t N_d_k	= maxDepth - d_k + 1;
		const uint64_t X_k		= ipow(2, d_k + 1) + idx * N_d_k;

		return X_k;
	}

	/************************************************************************************************/

	uint64_t CBTBuffer::GetHeapValue(uint32_t heapIdx) const noexcept
	{
		const uint32_t bitIdx	= GetBitOffset(heapIdx);
		const uint32_t bitWidth = maxDepth - FindMSB(heapIdx) + 1;
		return ReadValue(bitIdx, bitWidth);
	}


	/************************************************************************************************/


	uint32_t CBTBuffer::DecodeNode(int32_t leafID) const noexcept
	{
		uint32_t heapID = 1;
		uint32_t value	= GetHeapValue(heapID);

		for (int i = 0; i < maxDepth; i++)
		{
			if (value > 1)
			{
				uint32_t temp = GetHeapValue(2 * heapID);
				if (leafID < temp)
				{
					heapID *= 2;
					value = temp;
				}
				else
				{
					leafID = leafID - temp;
					heapID = 2 * heapID + 1;
					value = GetHeapValue(heapID);
				}
			}
			else
				break;
		}

		return heapID;
	}


	/************************************************************************************************/


	uint32_t CBTBuffer::DecodeBitidx(int32_t idx)		const noexcept
	{
		uint32_t heapID = 1;

		for (int i = 0; i < maxDepth; i++)
		{
			uint32_t temp = GetHeapValue(2 * heapID);
			if (idx < temp)
				heapID *= 2;
			else
			{
				idx -= temp;
				heapID = 2 * heapID + 1;
			}
		}

		return HeapIndexToBitIndex(heapID);
	}


	/************************************************************************************************/


	void CBTBuffer::SetBit(uint64_t idx, bool b) noexcept
	{
		const uint64_t bitIdx		= GetBitOffset(ipow(2, maxDepth) + idx);
		const uint64_t wordIdx		= bitIdx / (8 * sizeof(uint64_t));
		const uint64_t bitOffset	= bitIdx % (8 * sizeof(uint64_t)); 

		const uint64_t bits		= bitField[wordIdx];
		const uint64_t mask		= ~(uint64_t{1} << bitOffset);
		const uint64_t newValue = (bits & mask) | ((b ? uint64_t{ 0x01 } : uint64_t{ 0x00 }) << bitOffset);

		bitField[wordIdx] = newValue;
	}


	/************************************************************************************************/


	uint64_t CBTBuffer::GetBit(uint64_t idx, bool b) const noexcept
	{
		auto bitIdx			= (maxDepth) * ipow(2, maxDepth) + idx;
		auto wordIdx		= bitIdx / (8 * sizeof(uint64_t));
		auto bitOffset		= bitIdx % (8 * sizeof(uint64_t));

		const uint64_t bits = bitField[wordIdx];
		return bits & (0x01 << bitOffset);
	}


	/************************************************************************************************/


	uint64_t CBTBuffer::ReadValue(uint64_t start, uint64_t bitWidth) const noexcept
	{
		const uint64_t	mask	= (uint64_t(1) << (bitWidth)) - 1;
		const uint64_t	idx		= start / 64;
		const uint64_t	offset	= start % 64;

		uint64_t value	= (bitField[idx] >> offset) & mask;
		if (((start % 64) + bitWidth) > 64)
		{
			uint32_t bitsRemaining = ((start % 64) + bitWidth) % 64;
			uint64_t remainingBits = bitField[idx + 1] << (bitWidth - bitsRemaining);

			value |= (remainingBits & mask);
		}

		return value;
	}


	/************************************************************************************************/


	void CBTBuffer::WriteValue(uint64_t start, uint64_t bitWidth, uint64_t value) noexcept
	{
		const uint64_t	idx		= start / 64;
		const uint64_t	offset	= start % 64;
		uint64_t		mask	= ~(((uint64_t(1) << (bitWidth)) - 1) << offset);

		auto QWord = (bitField[idx] & mask) | (~mask & (value << offset));
		bitField[idx] = QWord;

		if (((start % 64) + bitWidth) > 64)
		{
			const uint32_t bitsRemaining	= ((start % 64) + bitWidth) % 64;
			const uint64_t remainingBits	= value >> (bitWidth - bitsRemaining);
			const uint64_t mask				= ~(((uint64_t(1) << (bitWidth)) - 1) >> (bitWidth - bitsRemaining));

			bitField[idx + 1] = remainingBits | (bitField[idx + 1] & mask);
		}
	}


	/************************************************************************************************/


	void CBTBuffer::SumReduction() noexcept
	{
		const int	end			= maxDepth;
		uint64_t	stepSize	= 1;
		uint64_t	steps		= ipow(2, maxDepth - 1);
			
		for (uint64_t i = 0; i < end; i++)
		{
			const uint64_t start_IN		= GetBitOffset(ipow(2, maxDepth - 0 - i));
			const uint64_t start_OUT	= GetBitOffset(ipow(2, maxDepth - 1 - i));

			for (int j = 0; j < steps; j++)
			{
				const uint64_t a = ReadValue(start_IN + (2 * j + 0) * stepSize, i + 1);
				const uint64_t b = ReadValue(start_IN + (2 * j + 1) * stepSize, i + 1);
				const uint64_t c = a + b;

				WriteValue(start_OUT + j * (stepSize + 1), i + 2, c);
			}

			stepSize += 1;
			steps /= 2;
		}
	}


	/************************************************************************************************/


	uint64_t CBTBuffer::GetNthPrefixSum(uint64_t idx)
	{
		uint32_t heapID = 1;
		uint64_t sum 	= 0;

		for (int i = 0; i < maxDepth; i++)
		{
			auto shift = maxDepth - 1 - i;
			bool bit = (idx >> (shift)) & 0x01;
			if (bit)
			{
				sum += GetHeapValue(2 * heapID);
				heapID = 2 * heapID + 1;
			}
			else
			{
				heapID *= 2;
			}
		}

		return sum;
	}


	/************************************************************************************************/


	void CBTBuffer::PipelineStates::_InitializeStates(PipelineStates& states, IRenderSystem& renderSystem)
	{
		renderSystem.RegisterPSOLoader(
			SumReductionCBT, [](IRenderSystem& renderSystem, iAllocator& allocator) -> LoadPipelineStateRes
			{
				return PipelineBuilder{ renderSystem, allocator }.
						AddComputeShader("SumReduction", "assets\\shaders\\cbt\\CBT_SumReduction.hlsl", { .hlsl2021 = true }).
						Build(renderSystem);
			});


		renderSystem.QueuePSOLoad(SumReductionCBT);
	}


	/************************************************************************************************/


	void CBTBuffer::PipelineStates::QueueReload(IRenderSystem& renderSystem)
	{
		renderSystem.QueuePSOLoad(SumReductionCBT);
	}


	/************************************************************************************************/


	void CBTBuffer::PipelineStates::Initialize(IRenderSystem& renderSystem)
	{
		init(*this, renderSystem);
	}


	/************************************************************************************************/


	void CBTBuffer::ReloadShaders()
	{
		states.QueueReload(renderSystem);
	}


	/************************************************************************************************/


	void CBTBuffer::Initialize(const CBTBufferDescription& description)
	{
		if (buffer != InvalidHandle)
			renderSystem.ReleaseResource(buffer);

		const size_t bitCount	= ipow(2, description.maxDepth + 2);
		const size_t wordSize	= Max(bitCount / 32 + (bitCount % 32 == 0 ? 0 : 1), 16);
		const size_t byteSize	= Max(bitCount / 8 + (bitCount % 8 == 0 ? 0 : 1), 128);

		buffer		= renderSystem.CreateGPUResource(GPUResourceDesc::UAVResource(byteSize));
		maxDepth	= description.maxDepth;
		bufferSize	= wordSize;

		renderSystem.SetDebugName(buffer, "CBTBuffer");

		bitField.resize(Max(1, bitCount / 64 + (bitCount % 64 == 0 ? 0 : 1)));
		memset(bitField.data(), 0x00, bitField.ByteSize());
	}


	/************************************************************************************************/


	void CBTBuffer::SumReduction_GPU(FrameGraph& frameGraph)
	{
		struct InitializeCBTree
		{
			FrameResourceHandle buffer;
		};

		frameGraph.AddNode<InitializeCBTree>(
			{},
			[&](FrameGraphNodeBuilder& builder, InitializeCBTree& args)
			{
				args.buffer = builder.UnorderedAccess(buffer);
			},
			[this](InitializeCBTree& args, ResourceHandler& handler, IDirectContext& ctx, iAllocator& allocator)
			{
				ctx.BeginEvent_DEBUG("CBTBuffer::SumReduction");

				uint32_t bufferSize = 32;

				struct
				{
					uint32_t maxDepth;
					uint32_t i;
					uint32_t stepSize;
					uint32_t end;
					uint32_t start_IN;
					uint32_t start_OUT;
				} constants;

				ctx.SetComputePipelineState(SumReductionCBT, allocator);
				ctx.SetComputeUnorderedAccessView(0, handler.UAV(args.buffer, ctx));

				uint32_t	stepSize	= 1;
				uint32_t	steps		= ipow(2, maxDepth - 1);

				constants.maxDepth = maxDepth;

				for (uint64_t i = 0; i < maxDepth; i++)
				{
					constants.stepSize	= stepSize;
					constants.i			= i;
					constants.end		= steps;
					constants.start_IN	= GetBitOffset(ipow(2, maxDepth - 0 - i));
					constants.start_OUT	= GetBitOffset(ipow(2, maxDepth - 1 - i));

					ctx.SetComputeConstantValue(1, 6, &constants);
					ctx.Dispatch({ steps / 1024 + (steps % 1024 != 0 ? 1 : 0), 1, 1 });

					ctx.AddUAVBarrier(handler.GetResource(args.buffer), -1, DeviceLayout_Unknown, Sync_Compute, Sync_Compute);

					stepSize	+= 1;
					steps		/= 2;
				}

				ctx.EndEvent_DEBUG();
			});
	}


	/************************************************************************************************/


	void CBTBuffer::Upload(FrameGraph& frameGraph)
	{
		struct InitializeCBTree
		{
			FrameResourceHandle buffer;
		};

		frameGraph.AddNode<InitializeCBTree>(
			{},
			[&](FrameGraphNodeBuilder& builder, InitializeCBTree& args)
			{
				args.buffer = builder.CopyDest(buffer);
			},
			[this](InitializeCBTree& args, ResourceHandler& handler, IDirectContext& ctx, iAllocator& allocator)
			{
				auto upload = ctx.ReserveDirectUploadSpace(bufferSize);
				memcpy(upload.buffer, bitField.data(), bitField.ByteSize());
				ctx.CopyBuffer(upload, handler.CopyDest(args.buffer, ctx));
			}
		);
	}


	/************************************************************************************************/


	void CBTBuffer::Clear(FrameGraph& frameGraph)
	{
		struct InitializeCBTree
		{
			FrameResourceHandle buffer;
		};

		frameGraph.AddNode<InitializeCBTree>(
			{},
			[&](FrameGraphNodeBuilder& builder, InitializeCBTree& args)
			{
				args.buffer = builder.UnorderedAccess(buffer);
			},
			[](InitializeCBTree& args, ResourceHandler& handler, IDirectContext& ctx, iAllocator& allocator)
			{
				uint32_t bufferSize = 32;

				ctx.ClearUAVBufferRange(handler.UAV(args.buffer, ctx), 0, bufferSize / 32);
			}
		);
	}


	/************************************************************************************************/


	void CBTMemoryManager::Update(FrameGraph& frameGraph)
	{
		cbt.SumReduction_GPU(frameGraph);
	}


}	/************************************************************************************************/



/**********************************************************************

Copyright (c) 2024 - 2025 Robert May

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
