#pragma once
#include <RenderSystemInterface.hpp>


namespace FlexKit
{	/************************************************************************************************/


	class CBPushBuffer
	{
	public:
		CBPushBuffer(ConstantBufferHandle cBuffer = InvalidHandle, char* IN_buffer = nullptr, size_t offsetBegin = 0, size_t reservedSize = 0) :
			CB				{ cBuffer		},
			buffer			{ IN_buffer		},
			pushBufferSize	{ reservedSize	},
			pushBufferBegin	{ offsetBegin	} {}


		CBPushBuffer(ConstantBufferHandle IN_CB, size_t reserveSize, IRenderSystem& renderSystem)
		{
			auto reservedBuffer = renderSystem.ReserveConstantBuffer(IN_CB, reserveSize);

			CB				= IN_CB;
			buffer			= reservedBuffer.data;
			pushBufferSize	= reserveSize;
			pushBufferBegin	= reservedBuffer.offsetBegin;
		}


		CBPushBuffer(const CBPushBuffer&)							= delete;
		CBPushBuffer& operator = (const CBPushBuffer& pushBuffer)	= delete;


		CBPushBuffer(CBPushBuffer&& pushBuffer) 
		{
			CB				= pushBuffer.CB;
			buffer			= pushBuffer.buffer;
			pushBufferBegin = pushBuffer.pushBufferBegin;
			pushBufferSize	= pushBuffer.pushBufferSize;
			pushBufferUsed	= pushBuffer.pushBufferUsed;

			pushBuffer.CB				= InvalidHandle;
			pushBuffer.buffer			= nullptr;
			pushBuffer.pushBufferBegin	= 0;
			pushBuffer.pushBufferSize	= 0;
			pushBuffer.pushBufferUsed	= 0;
		}


		CBPushBuffer& operator = (CBPushBuffer&& pushBuffer)
		{
			CB				= pushBuffer.CB;
			buffer			= pushBuffer.buffer;
			pushBufferBegin = pushBuffer.pushBufferBegin;
			pushBufferSize	= pushBuffer.pushBufferSize;
			pushBufferUsed	= pushBuffer.pushBufferUsed;

			pushBuffer.CB				= InvalidHandle;
			pushBuffer.buffer			= nullptr;
			pushBuffer.pushBufferBegin	= 0;
			pushBuffer.pushBufferSize	= 0;
			pushBuffer.pushBufferUsed	= 0;

			return *this; 
		}

		operator ConstantBufferHandle () const	{ return CB; }
		operator ConstantBufferHandle ()		{ return CB; }


		ConstantBufferHandle Handle() const { return CB; }

		template<typename TY>
		static constexpr size_t CalculateOffset()
		{
			return Max((sizeof(TY) / 256) * 256, 256);
		}

		static constexpr size_t CalculateOffset(const size_t size)
		{
			return Max((size / 256) * 256, 256);
		}

		template<typename _TY>
		size_t Push(const _TY& data)
		{
			constexpr size_t alignedSize = CalculateOffset<_TY>();
			if (pushBufferUsed + alignedSize > pushBufferSize)
			{
				FK_LOG_ERROR("Failed To Push Constants");
				return -1;
			}

			const size_t offset = pushBufferUsed;
			memcpy(buffer + pushBufferBegin + offset, (char*)&data, sizeof(data));
			pushBufferUsed += CalculateOffset<_TY>();

			return offset + pushBufferBegin;
		}

		size_t Push(char* _ptr, const size_t size)
		{
			if (pushBufferUsed + AlignedSize(size) > pushBufferSize)
				return -1;

			const size_t offset = pushBufferUsed;
			memcpy(buffer + pushBufferBegin + offset, _ptr, size);
			pushBufferUsed += AlignedSize(size);

			return offset + pushBufferBegin;
		}

		size_t begin() const { return pushBufferBegin; }


		std::byte* data() { return (std::byte*)buffer; }
	private:
		ConstantBufferHandle	CB				= InvalidHandle;
		char*					buffer			= nullptr;
		size_t					pushBufferBegin = 0;
		size_t					pushBufferSize	= 0;
		size_t					pushBufferUsed	= 0;
	};


	/************************************************************************************************/


	class VBPushBuffer
	{
	public:
		VBPushBuffer(VertexBufferHandle vBuffer = InvalidHandle, char* buffer_ptr = nullptr, size_t offsetBegin = 0, size_t reservedSize = 0) noexcept :
			VB					{ vBuffer		},
			buffer				{ buffer_ptr	},
			pushBufferSize		{ reservedSize	},
			pushBufferOffset	{ offsetBegin	} {}


		VBPushBuffer(VertexBufferHandle IN_VB, size_t IN_reserveSize, IRenderSystem& renderSystem)
		{
			auto reservedBuffer = renderSystem.ReserveVertexBuffer(IN_VB, IN_reserveSize);
			VB					= IN_VB;
			pushBufferSize		= IN_reserveSize;
			buffer				= reservedBuffer.data;
			pushBufferOffset	= reservedBuffer.offsetBegin;
		}


		VBPushBuffer(const VBPushBuffer&)						= delete;
		VBPushBuffer& operator = (VBPushBuffer& pushBuffer)		= delete;


		VBPushBuffer(VBPushBuffer&& pushBuffer)
		{
			VB				 = pushBuffer.VB;
			buffer			 = pushBuffer.buffer;
			pushBufferOffset = pushBuffer.pushBufferOffset;
			pushBufferSize	 = pushBuffer.pushBufferSize;
			pushBufferUsed	 = pushBuffer.pushBufferUsed;

			pushBuffer.VB				= InvalidHandle;
			pushBuffer.buffer			= nullptr;
			pushBuffer.pushBufferOffset	= 0;
			pushBuffer.pushBufferSize	= 0;
			pushBuffer.pushBufferUsed	= 0;
		}


		VBPushBuffer& operator = (VBPushBuffer&& pushBuffer) 
		{ 
			VB				 = pushBuffer.VB;
			buffer			 = pushBuffer.buffer;
			pushBufferOffset = pushBuffer.pushBufferOffset;
			pushBufferSize	 = pushBuffer.pushBufferSize;
			pushBufferUsed	 = pushBuffer.pushBufferUsed;

			pushBuffer.VB				= InvalidHandle;
			pushBuffer.buffer			= nullptr;
			pushBuffer.pushBufferOffset	= 0;
			pushBuffer.pushBufferSize	= 0;
			pushBuffer.pushBufferUsed	= 0;

			return *this; 
		}

		operator VertexBufferHandle ()			{ return VB; }
		operator VertexBufferHandle () const	{ return VB; }

		VertexBufferHandle Handle() const { return VB;  }

		template<typename _TY>
		size_t Push(const _TY& data) noexcept
		{
			if (pushBufferUsed + sizeof(data) > pushBufferSize)
				return -1;

			size_t offset = pushBufferUsed;
			memcpy(pushBufferOffset + buffer + pushBufferUsed, (char*)&data, sizeof(data));
			pushBufferUsed += sizeof(data);

			return offset;
		}


		size_t Push(char* _ptr, size_t size) noexcept
		{
			if (pushBufferUsed + size > pushBufferSize)
				return -1;

			size_t offset = pushBufferUsed;
			memcpy(pushBufferOffset + buffer + pushBufferUsed, _ptr, size);
			pushBufferUsed += (size + 255) & ~255;

			return offset;
		}


		size_t begin() { return pushBufferOffset; }


		size_t GetOffset()const noexcept
		{
			return pushBufferUsed + pushBufferOffset;
		}

		VertexBufferHandle	VB					= InvalidHandle;
		char*				buffer				= 0;
		size_t				pushBufferOffset	= 0;
		size_t				pushBufferSize		= 0;
		size_t				pushBufferUsed		= 0;
	};

	
	/************************************************************************************************/


	struct SET_MAP_t{
	}const SET_MAP_OP;

	struct SET_TRANSFORM_t {
	}const SET_TRANSFORM_OP;

	class VertexBufferDataSet
	{
	public:
		VertexBufferDataSet() {}

		template<typename TY>
		VertexBufferDataSet(const TY& initialData, VBPushBuffer& buffer) :
			vertexBuffer	{ buffer },
			offsetBegin		{ buffer.GetOffset() },
			vertexStride    { sizeof(initialData[0]) }
		{

			for (auto& vertex : initialData) 
				buffer.Push(vertex);
		}

		template<typename TY>
		VertexBufferDataSet(const TY* buffer, size_t bufferSize, VBPushBuffer& pushBuffer) :
			vertexBuffer	{ pushBuffer },
			offsetBegin		{ pushBuffer.GetOffset() }
		{
			vertexStride = sizeof(TY);

			if (pushBuffer.Push((char*)buffer, bufferSize) == -1)
				throw std::runtime_error("buffer too short to accomdate push!");
		}

		template<typename TY, typename FN_TransformVertex>
		VertexBufferDataSet(const SET_MAP_t, const TY& initialData, const FN_TransformVertex& TransformVertex, VBPushBuffer& buffer) :
			vertexBuffer	{ buffer },
			offsetBegin		{ buffer.GetOffset() }
		{
			vertexStride = sizeof(decltype(TransformVertex(initialData[0])));

			for (const auto& vertex : initialData) {
				auto transformedVertex = TransformVertex(vertex);
				if (buffer.Push(transformedVertex) == -1)
					throw std::runtime_error("buffer too short to accommodate push!");
			}
		}


		template<typename TY_CONTAINER, typename FN_TransformVertex>
		VertexBufferDataSet(const SET_TRANSFORM_t, const TY_CONTAINER& initialData, const FN_TransformVertex& TransformVertex, VBPushBuffer& buffer) :
			vertexBuffer	{ buffer.Handle()		},
			offsetBegin		{ buffer.GetOffset()	}
		{
			using TY = decltype(TransformVertex(initialData.front(), buffer));
			vertexStride = sizeof(decltype(TransformVertex(initialData.front(), buffer)));

			for (const auto& vertex : initialData)
				TransformVertex(vertex, buffer);
		}


		~VertexBufferDataSet() = default;

		VertexBufferDataSet(const VertexBufferDataSet& rhs)					= default;
		VertexBufferDataSet& operator = (const VertexBufferDataSet& rhs)	= default;

		operator VertexBufferEntry () const
		{
			return { vertexBuffer, (uint32_t)vertexStride, (uint32_t)offsetBegin };
		}


		operator VertexBufferHandle const ()	{ return vertexBuffer;	}
		operator size_t const ()				{ return offsetBegin;	}

	private:
		uint64_t			offsetBegin		= 0;
		uint64_t			vertexStride	= 0;
		VertexBufferHandle	vertexBuffer	= InvalidHandle;
	};


	/************************************************************************************************/


	class ConstantBufferDataSet
	{
	public:
		ConstantBufferDataSet() {}

		template<typename TY>
		ConstantBufferDataSet(const TY& initialData, CBPushBuffer& buffer) :
			constantBuffer	{ buffer.Handle()			},
			constantsOffset	{ buffer.Push(initialData)	},
			size            { AlignedSize<TY>()			} {}

		ConstantBufferDataSet(const size_t& IN_offset, ConstantBufferHandle IN_buffer, size_t IN_size = 0) :
			constantBuffer  { IN_buffer },
			constantsOffset { IN_offset },
			size            { IN_size   } {} 

		explicit ConstantBufferDataSet(char* initialData, const size_t bufferSize, CBPushBuffer& buffer) :
			constantBuffer  { buffer.Handle()						},
			constantsOffset { buffer.Push(initialData, bufferSize)	},
			size            { AlignedSize(bufferSize)				} {}

		~ConstantBufferDataSet() = default;

		ConstantBufferDataSet(const ConstantBufferDataSet& rhs)					= default;
		ConstantBufferDataSet& operator = (const ConstantBufferDataSet& rhs)	= default;

		ConstantBufferHandle	Handle() const { return constantBuffer;	}
		size_t					Offset() const { return constantsOffset; }
		size_t					Size() const { return size; }

	private:
		ConstantBufferHandle	constantBuffer		= InvalidHandle;
		size_t					constantsOffset		= 0;
		size_t					size                = 0;
	};


	/************************************************************************************************/


	template<typename TY>
	auto CreateCBIterator(const CBPushBuffer& source)
	{
		struct CB_Proxy
		{
			CB_Proxy operator[](const size_t idx) const noexcept
			{
				return { offset, idx, CB };
			}

			operator ConstantBufferDataSet () const		{ return { offset + idx * CBPushBuffer::CalculateOffset<TY>(), CB, 0u }; }
			operator ConstantBufferDataSet ()			{ return { offset + idx * CBPushBuffer::CalculateOffset<TY>(), CB, 0u }; }

			const size_t				offset;
			const size_t				idx;
			const ConstantBufferHandle	CB = InvalidHandle;
		};

		return CB_Proxy{ source.begin(), 0, source };
	}


	/************************************************************************************************/


	inline CBPushBuffer Reserve(ConstantBufferHandle CB, size_t pushSize, size_t count, IRenderSystem& renderSystem)
	{
		size_t reserveSize = (pushSize / 256 + 1) * 256 * count;
		auto buffer = renderSystem.ReserveConstantBuffer(CB, reserveSize);

		return { CB, buffer.data, buffer.offsetBegin, reserveSize };
	}


	inline VBPushBuffer Reserve(VertexBufferHandle VB, size_t reserveSize, IDirectContext& ctx)
	{
		auto buffer = ctx.GetRenderSystem().ReserveVertexBuffer(VB, reserveSize);
		return { VB, buffer.data, buffer.offsetBegin, reserveSize };
	}

	inline VBPushBuffer Reserve(VertexBufferHandle VB, size_t reserveSize, IRenderSystem& rs)
	{
		auto buffer = rs.ReserveVertexBuffer(VB, reserveSize);
		return { VB, buffer.data, buffer.offsetBegin, reserveSize };
	}


	/************************************************************************************************/


	[[nodiscard]]
	inline auto CreateVertexBufferReserveObject(
		VertexBufferHandle	vertexBuffer,
		IRenderSystem&		renderSystem,
		iAllocator*			allocator)
	{
		return MakeSynchonized(
			[=, &renderSystem](size_t reserveSize) -> VBPushBuffer
			{
				return VBPushBuffer(vertexBuffer, reserveSize, renderSystem);
			},
			allocator);
	}

	[[nodiscard]]
	inline auto CreateConstantBufferReserveObject(
		ConstantBufferHandle	constantBuffer,
		IRenderSystem&			renderSystem,
		iAllocator*				allocator)
	{
		return MakeSynchonized(
			[=, &renderSystem](const size_t reserveSize) -> CBPushBuffer
			{
				return CBPushBuffer(constantBuffer, reserveSize, renderSystem);
			},
			allocator);
	}

	using ReserveVertexBufferFunction	= decltype(CreateVertexBufferReserveObject(InvalidHandle,	std::declval<IRenderSystem&>(), nullptr));
	using ReserveConstantBufferFunction	= decltype(CreateConstantBufferReserveObject(InvalidHandle, std::declval<IRenderSystem&>(), nullptr));


	[[nodiscard]]
	inline auto CreateOnceReserveBuffer(ReserveConstantBufferFunction& reserveConstants, iAllocator* allocator)
	{
		return MakeLazyObject<CBPushBuffer>(
			allocator,
			[reserveConstants = reserveConstants](size_t reserveSize) mutable
			{
				return reserveConstants(reserveSize);
			});
	}

	using CreateOnceReserveBufferFunction = decltype(CreateOnceReserveBuffer(*((ReserveConstantBufferFunction*)nullptr), nullptr));


}	/************************************************************************************************/


/**********************************************************************

Copyright (c) 2025 Robert May

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

