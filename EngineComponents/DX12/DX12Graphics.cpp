#include "..\pch.h"
#include "BuildSettings.hpp"

#include "AnimationUtilities.hpp"
#include "Containers.hpp"
#include "DDSUtilities.hpp"
#include "DX12Graphics.hpp"
#include "Logging.hpp"
#include "MemoryUtilities.hpp"
#include "MeshUtilities.hpp"
#include "PushBuffers.hpp"
#include "ThreadUtilities.hpp"
#include "TriMeshResource.hpp"

#include <algorithm>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <filesystem>
#include <fmt\format.h>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <iostream>
#include <ranges>
#include <Windows.h>
#include <stacktrace>

#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <directx/d3d12sdklayers.h>
#include <DirectXMath.h>


extern "C" __declspec(dllexport) DWORD  NvOptimusEnablement = 1;
extern "C" __declspec(dllexport) int    AmdPowerXpressRequestHighPerformance = 1;

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion    = 615; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath      = ".\\"; }

namespace FlexKit
{
	using std::ranges::sort;
	using std::views::iota;
	using std::views::zip;


	/************************************************************************************************/


	void SetDebugName(ID3D12Object* Obj, const char* cstr, size_t size)
	{
#if USING(DEBUGGRAPHICS)
		if (!Obj)
			return;

		const size_t StringSize = 128;
		size_t ConvertedCount = 0;
		wchar_t WString[StringSize];
		mbstowcs_s(&ConvertedCount, WString, cstr, StringSize);
		Obj->SetName(WString);
#endif
	}


	/************************************************************************************************/

	
	UAVBuffer::UAVBuffer(const RenderSystem& rs, const ResourceHandle handle, const size_t IN_stride, const size_t IN_offset)
	{
		FK_ASSERT(IN_offset < std::numeric_limits<uint32_t>::max());

		auto uavLayout	= rs.GetUAVBufferLayout(handle);
		auto bufferSize = rs.GetUAVBufferSize(handle);

		resource		= rs.GetDeviceResource(handle);
		stride			= (uint32_t)(IN_stride == -1 ? uavLayout.stride : IN_stride);
		elementCount	= (uint32_t)bufferSize / stride;
		counterOffset	= 0;
		offset			= (uint32_t)IN_offset;
		format			= uavLayout.format;
	}


	/************************************************************************************************/


	DescriptorHeapAllocator::~DescriptorHeapAllocator()
	{
		if (descHeap)
		{
			root.Release(allocator);
			freeList.clear();

			descHeap->Release();

			descHeap		= nullptr;
			allocator		= nullptr;
			renderSystem	= nullptr;
		}
	}


	/************************************************************************************************/


	std::optional<DescriptorRange> DescriptorHeapAllocator::Alloc_ST(const size_t size, uint64_t completedIdx) noexcept
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

				const auto offset = descriptorSize * node->begin;

				if (size == 0)
					DebugBreak();

				return DescriptorRange{
					.begin	= { { (uint64_t)cpuHeap.ptr + offset },
								{ (uint64_t)gpuHeap.ptr + offset } },
					.size	= (uint32_t)size,
					.stride = (uint32_t)descriptorSize,
				};
			}
		}

		DebugBreak();
		return {};
	}


	/************************************************************************************************/


	void DescriptorHeapAllocator::Initialize(FlexKit::RenderSystem& IN_renderSystem, const size_t numDescCount, FlexKit::iAllocator* IN_allocator)
	{
		descHeap		= IN_renderSystem._CreateShaderVisibleHeap(numDescCount);
		renderSystem	= &IN_renderSystem;
		root			= Node{ .begin = 0, .end = numDescCount };
		freeList		= FlexKit::Vector<Node*>{ IN_allocator };
		allocator		= IN_allocator;

		freeList.push_back(&root);

		gpuHeap = descHeap->GetGPUDescriptorHandleForHeapStart();
		cpuHeap = descHeap->GetCPUDescriptorHandleForHeapStart();

		descriptorSize = renderSystem->DescriptorCBVSRVUAVSize;
	}


	/************************************************************************************************/


	auto DescriptorHeapAllocator::Alloc(const size_t size, uint64_t completedIdx) noexcept
	{
		std::scoped_lock lock{ mutex };

		return Alloc_ST(size, completedIdx);
	}


	/************************************************************************************************/


	void DescriptorHeapAllocator::Release_ST(const DescriptorRange range, uint64_t lockIdx, uint64_t completedIdx) noexcept
	{
		auto& [cpu_ptr, gpu_ptr] = range.begin;
		auto offset1 = (gpu_ptr - gpuHeap.ptr) / descriptorSize;
		auto offset2 = (cpu_ptr - cpuHeap.ptr) / descriptorSize;

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


	void DescriptorHeapAllocator::Release(const DescriptorRange range, uint64_t lockIdx, uint64_t completedIdx)
	{
		std::scoped_lock lock{mutex};

		Release_ST(range, lockIdx, completedIdx);
	}


	/************************************************************************************************/


	size_t DescriptorHeapAllocator::Node::FreeCount() const noexcept
	{
		if (left && right)
			return left->FreeCount() + right->FreeCount();
		if (free)
			return BlockCount();
		else
			return 0;
	}


	/************************************************************************************************/


	std::pair<size_t, size_t> DescriptorHeapAllocator::Node::SplitSizes()
	{
		const auto numBlocks = BlockCount();
		return { 3 * numBlocks >> 2, numBlocks >> 2 };
	}


	/************************************************************************************************/


	void DescriptorHeapAllocator::Node::Split(FlexKit::iAllocator* allocator)
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


	void DescriptorHeapAllocator::Node::Collapse(FlexKit::iAllocator* allocator)
	{
		allocator->release(left);
		allocator->release(right);

		left	= nullptr;
		right	= nullptr;
		free	= true;
	}


	/************************************************************************************************/


	bool DescriptorHeapAllocator::Node::Collapsable(uint64_t completed)
	{
		return (free && (lockUntil + 2) < completed);
	}


	/************************************************************************************************/


	void DescriptorHeapAllocator::Node::Release(iAllocator* allocator)
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


	DescriptorHeapAllocator::Node* DescriptorHeapAllocator::LocateNode(size_t offset)
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


	/************************************************************************************************/


	ConstantBufferHandle	ConstantBufferTable::CreateConstantBuffer(uint32_t BufferSize, bool GPUResident)
	{
		D3D12_RESOURCE_DESC   Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(BufferSize);
		Resource_DESC.Alignment				= 0;
		Resource_DESC.DepthOrArraySize		= 1;
		Resource_DESC.Dimension				= D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		Resource_DESC.Layout				= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Resource_DESC.Height				= 1;
		Resource_DESC.Format				= DXGI_FORMAT_UNKNOWN;
		Resource_DESC.SampleDesc.Count		= 1;
		Resource_DESC.SampleDesc.Quality	= 0;
		Resource_DESC.Flags					= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;//D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE; // Causes Graphics Debugger to crash

		D3D12_HEAP_PROPERTIES HEAP_Props	={};
		HEAP_Props.CPUPageProperty			= D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type						= GPUResident ? D3D12_HEAP_TYPE_DEFAULT : D3D12_HEAP_TYPE_UPLOAD;
		HEAP_Props.MemoryPoolPreference		= D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask			= 0;
		HEAP_Props.VisibleNodeMask			= 0;

		constexpr size_t bufferCount = 3;
		ID3D12Resource*  resources[3];

		D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;

		for(size_t I = 0; I < bufferCount; ++I)
		{
			ID3D12Resource* Resource = nullptr;
			HRESULT HR = renderSystem->pDevice->CreateCommittedResource(
							&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, 
							&Resource_DESC, InitialState, nullptr,
							IID_PPV_ARGS(&Resource));

			CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE CONSTANT BUFFER"));
			resources[I] = Resource;

			SETDEBUGNAME(Resource, __func__);
		}

		std::scoped_lock lock{ criticalSection };
		const uint32_t BufferIdx = uint32_t(buffers.size());

		void* Mapped_ptr = nullptr;

		if(!GPUResident)
			resources[0]->Map(0, nullptr, &Mapped_ptr);

		ConstantBufferHandle handle = handles.GetNewHandle();

		UserConstantBuffer buffer = {
			BufferSize,
			0,
			Mapped_ptr,

			GPUResident,
			false,

			{ 0, 0, 0 },
			0,
			handle,

			{ resources[0], resources[1], resources[2], },
		};


		handles[handle] = (uint32_t)buffers.push_back(buffer);

		return handle;
	}


	/************************************************************************************************/


	ID3D12Resource* ConstantBufferTable::GetDeviceResource(const ConstantBufferHandle handle) const
	{
		auto& buffer = buffers[handles[handle]];

		return buffer.resources[buffer.currentRes];
	}


	/************************************************************************************************/


	size_t ConstantBufferTable::GetBufferOffset(const ConstantBufferHandle handle) const
	{
		return buffers[handles[handle]].offset;
	}


	/************************************************************************************************/


	size_t ConstantBufferTable::GetBufferSize(const ConstantBufferHandle Handle) const
	{
		auto& buffer = buffers[handles[Handle]];

		return buffer.size;
	}


	/************************************************************************************************/


	size_t ConstantBufferTable::AlignNext(ConstantBufferHandle Handle)
	{
		auto& buffer = buffers[handles[Handle]];

		if (buffer.GPUResident)
			return -1; // Cannot directly push to GPU Resident Memory

		const uint32_t size		= buffer.size;
		const uint32_t offset	= buffer.offset;

		const size_t alignOffset	= 256 - offset % 256;
		const size_t adjustedOffset = (alignOffset == 256) ? 0 : offset;
		const size_t alignedOffset  = offset + adjustedOffset;

		buffer.offset = (uint32_t)alignedOffset;

		return alignedOffset;
	}


	/************************************************************************************************/


	std::optional<size_t> ConstantBufferTable::Push(ConstantBufferHandle handle, void* _Ptr, size_t pushSize)
	{
		auto& buffer = buffers[handles[handle]];

		if (buffer.GPUResident)
			return false; // Cannot directly push to GPU Resident Memory

		const char* Debug_mapped_Ptr = (char*)buffer.mapped_ptr;

		const uint32_t size		= (uint32_t)buffer.size;
		const uint32_t offset	= (uint32_t)buffer.offset;
		const char*  mapped_Ptr	= (char*)buffer.mapped_ptr;

		if (!mapped_Ptr)
			return {};

		if (size < offset + pushSize)
			return {}; // Buffer To small to accommodate Push

		buffer.offset += (uint32_t)pushSize;

		if(!buffer.writeFlag)
			buffer.writeFlag = true;

		FK_ASSERT(pushSize % 256 == 0); // size requests must be blocks of 256

		if(_Ptr)
			memcpy((void*)(mapped_Ptr + offset), _Ptr, pushSize);

		return { offset };
	}


	/************************************************************************************************/


	SubAllocation ConstantBufferTable::Reserve(ConstantBufferHandle CB, size_t reserveSize)
	{
		const auto res = Push(CB, nullptr, reserveSize);

		if (!res.has_value())
			DebugBreak();

		FK_ASSERT(res.has_value());

		const size_t UserIdx	= handles[CB];
		void*		 buffer		= buffers[UserIdx].mapped_ptr;

		return { static_cast<char*>(buffer), res.value_or(0), reserveSize };
	}


	/************************************************************************************************/


	void ConstantBufferTable::ReleaseBuffer(ConstantBufferHandle handle)
	{
		std::scoped_lock lock(criticalSection);

		const size_t UserIdx	= handles[handle];
		auto&  buffer			= buffers[UserIdx];

		for (auto& res : buffer.resources)
		{
			if (res)
				res->Release();

			res = nullptr;
		}

		buffer = buffers.back();

		handles[buffer.handle] = handles[handle];

		buffers.pop_back();
	}


	/************************************************************************************************/


	void ConstantBufferTable::Reset(ConstantBufferHandle Handle)
	{
		ProfileFunctionTextName(ConstantBufferTableReset);

		const size_t UserIdx	= handles[Handle];
		auto& buffer			= buffers[UserIdx];

		if (buffer.offset == 0)
			return; // unused

		buffer.locks[buffer.currentRes] = renderSystem->GetCurrentCounter();
		buffer.currentRes = (buffer.currentRes + 1) % 3;

		renderSystem->WaitFor(buffer.locks[buffer.currentRes]);

		char* mapped_Ptr	= nullptr;
		auto HR				= buffer.resources[buffer.currentRes]->Map(0, nullptr, (void**)&mapped_Ptr);
		FK_ASSERT(FAILED(HR), "Failed to map Constant Buffer");

		buffer.mapped_ptr	= mapped_Ptr;
		buffer.offset		= 0;
	}


	/************************************************************************************************/


	DescriptorHeapImpl::DescriptorHeapImpl(IContext& ictx, const DesciptorHeapLayout<16>& Layout_IN, iAllocator* TempMemory) :
		FillState(TempMemory)
	{
		auto& ctx = static_cast<Context&>(ictx);

		FK_ASSERT(TempMemory);

		const size_t EntryCount = Layout_IN.size();
		descriptorHeap	= ctx._ReserveSRV(EntryCount).value();
		Layout			= &Layout_IN;

		for (size_t I = 0; I < EntryCount; I++)
			FillState.push_back(false);
	}


	/************************************************************************************************/


	DescriptorHeapImpl::DescriptorHeapImpl(DescriptorHeapImpl&& rhs)
	{
		descriptorHeap	= rhs.descriptorHeap;
		FillState		= std::move(rhs.FillState);
		Layout			= rhs.Layout;

		rhs.descriptorHeap	= DescHeapPOS{ InvalidHandle, InvalidHandle };
		rhs.Layout			= nullptr;
	}


	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::operator = (DescriptorHeapImpl&& rhs)
	{
		descriptorHeap	= rhs.descriptorHeap;
		FillState		= std::move(rhs.FillState);
		Layout			= rhs.Layout;

		rhs.descriptorHeap = DescHeapPOS{ InvalidHandle, InvalidHandle };
		rhs.Layout = nullptr;

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::Init(IContext& ictx, const DesciptorHeapLayout<16>& Layout_IN, iAllocator* TempMemory)
	{
		auto& ctx = static_cast<Context&>(ictx);

		FK_ASSERT(TempMemory);
		FillState = Vector<bool>(TempMemory);

		const size_t EntryCount	= Layout_IN.size();
		descriptorHeap			= ctx._ReserveSRV(EntryCount).value();
		Layout					= &Layout_IN;

		for (size_t I = 0; I < EntryCount; I++)
			FillState.push_back(false);

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::Init(IContext& ictx, const DesciptorHeapLayout<16>& Layout_IN, const size_t reserveCount, iAllocator* TempMemory)
	{
		auto& ctx = static_cast<Context&>(ictx);

		FK_ASSERT(TempMemory);
		FillState = Vector<bool>(TempMemory);

		const size_t EntryCount = Layout_IN.size() * reserveCount;
		descriptorHeap = ctx._ReserveSRV(EntryCount).value();
		Layout = &Layout_IN;

		for (size_t I = 0; I < EntryCount; I++)
			FillState.push_back(false);

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::Init2(IContext& ictx, const DesciptorHeapLayout<16>& Layout_IN, const size_t reserveCount, iAllocator* TempMemory)
	{
		auto& ctx = static_cast<Context&>(ictx);

		FK_ASSERT(TempMemory);
		FillState = Vector<bool>(TempMemory, reserveCount);

		descriptorHeap = ctx._ReserveSRV(reserveCount).value();
		Layout = &Layout_IN;

		for (size_t I = 0; I < reserveCount; I++)
			FillState.push_back(false);

		return *this;
	}



	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::NullFill(IContext& ictx, const size_t end)
	{
		auto& ctx = static_cast<Context&>(ictx);

		auto& Entries = Layout->Entries;
		for (size_t I = 0, Idx = 0; I < Entries.size(); I++)
		{
			auto& e = Entries[I];
			//
			for (size_t II = 0; II < e.Count + e.Space; II++)
			{
				if (I + II > end)
					return *this;
				if (!FillState[Idx])
				{
					switch (e.Type)
					{
					case DescHeapEntryType::ConstantBuffer:
					{
						auto POS = IncrementHeapPOS(
							descriptorHeap,
							ctx.renderSystem->DescriptorCBVSRVUAVSize,
							Idx);

						PushCBToDescHeap(
							ctx.renderSystem, 0,
							POS, 1024);
					}	break;
					case DescHeapEntryType::ShaderResource:
					{
						auto POS = IncrementHeapPOS(
							descriptorHeap,
							ctx.renderSystem->DescriptorCBVSRVUAVSize,
							Idx);

						PushSRVToDescHeap(
							ctx.renderSystem,
							nullptr,
							POS, 16, 16);
					}	break;
					case DescHeapEntryType::UAVBuffer:
					{
						auto POS = IncrementHeapPOS(
							descriptorHeap,
							ctx.renderSystem->DescriptorCBVSRVUAVSize,
							Idx);

						Texture2D nullTexture{ nullptr };
						nullTexture.Format = DXGI_FORMAT_R8G8B8A8_UINT;

						PushUAV2DToDescHeap(
							ctx.renderSystem,
							nullTexture,
							POS);
					}	break;
					case DescHeapEntryType::HeapError:
					{
						FK_ASSERT(false, "ERROR IN HEAP LAYOUT!");
					}	break;
					default:
						break;
					}
				}
				Idx++;
			}
		}

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::SetSRV(IContext& ictx, size_t idx, ResourceHandle handle)
	{
		auto& ctx = static_cast<Context&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetSRV(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return *this;
		}
#endif

		FillState[idx] = true;

		PushTextureToDescHeap(
			ctx.renderSystem,
			ctx.renderSystem->GetTextureDeviceFormat(handle),
			handle,
			IncrementHeapPOS(
					descriptorHeap, 
					ctx.renderSystem->DescriptorCBVSRVUAVSize,
					idx));

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::SetSRVCubemap(IContext& ictx, size_t idx, ResourceHandle	handle)
	{
		Context& ctx = static_cast<Context&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetSRVCubemap(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return *this;
		}
#endif

		FillState[idx] = true;

		PushCubeMapTextureToDescHeap(
			ctx.renderSystem,
			handle,
			IncrementHeapPOS(
					descriptorHeap, 
					ctx.renderSystem->DescriptorCBVSRVUAVSize, 
					idx),
			ctx.renderSystem->GetTextureFormat(handle));

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::SetSRVCubemap(IContext& ictx, size_t idx, ResourceHandle	handle, DeviceFormat format)
	{
		auto& ctx = static_cast<Context&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetSRVCubemap(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return *this;
		}
#endif

		FillState[idx] = true;

		PushCubeMapTextureToDescHeap(
			ctx.renderSystem,
			handle,
			IncrementHeapPOS(
					descriptorHeap, 
					ctx.renderSystem->DescriptorCBVSRVUAVSize, 
					idx),
			format);

		return *this;
	}



	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::SetSRV(IContext& ictx, size_t idx, ResourceHandle handle, DeviceFormat format)
	{
		Context& ctx = static_cast<Context&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetSRV(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return *this;
		}
#endif

		FillState[idx] = true;

		auto dxFormat = TextureFormat2DXGIFormat(format);

		PushTextureToDescHeap(
			ctx.renderSystem,
			dxFormat,
			handle,
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx));

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::SetSRV(IContext& ictx, size_t idx, ResourceHandle handle, uint MipOffset, DeviceFormat format)
	{
		auto& ctx = static_cast<Context&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetSRV(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return *this;
		}
#endif

		FillState[idx] = true;

		auto dxFormat = TextureFormat2DXGIFormat(format);

		PushTextureToDescHeap(
			ctx.renderSystem,
			dxFormat,
			MipOffset,
			handle,
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx));

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::SetSRVArray(IContext& ictx, size_t idx, ResourceHandle handle, DeviceFormat format)
	{
		Context& ctx = static_cast<Context&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetSRV(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return *this;
		}
#endif

		FillState[idx] = true;

		auto dxFormat = TextureFormat2DXGIFormat(format);

		PushTextureToDescHeap(
			ctx.renderSystem,
			dxFormat,
			handle,
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx));

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::SetSRV3D(IContext& ictx, size_t idx, ResourceHandle handle)
	{
		auto& ctx = static_cast<Context&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetSRV3D(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return *this;
		}
#endif

		FillState[idx] = true;

		const uint32_t	mipCount	= ctx.renderSystem->GetTextureMipCount(handle);
		const auto		format		= ctx.renderSystem->GetTextureFormat(handle);
		const auto		dxFormat	= TextureFormat2DXGIFormat(format);

		PushTexture3DToDescHeap(
			ctx.renderSystem,
			dxFormat,
			mipCount,
			0,
			0,
			handle,
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx));

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::SetCBV(IContext& ictx, size_t idx, const ConstantBufferDataSet& constants)
	{
		Context& ctx = static_cast<Context&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::ConstantBuffer, idx))
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetCBV(%u, %u, %u): Failed to set descriptor!", idx, constants.Handle().to_uint(), constants.Offset());
			return *this;
		}
#endif

		FillState[idx] = true;

		auto resource = ctx.renderSystem->GetDeviceResource(constants.Handle());

		PushCBToDescHeap(
			ctx.renderSystem,
			resource,
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx),
			constants.Size(),
			constants.Offset());

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::SetCBV(IContext& ictx, size_t idx, ConstantBufferHandle handle, size_t offset, size_t bufferSize)
	{
		auto& ctx = static_cast<Context&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::ConstantBuffer, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetCBV(%u, %u, %u): Failed to set descriptor!", idx, handle, offset);
			return *this;
		}
#endif

		FillState[idx] = true;

		auto resource = ctx.renderSystem->GetDeviceResource(handle);
		PushCBToDescHeap(
			ctx.renderSystem,
			resource,
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx),
			(bufferSize / 256) * 256 + 256,
			offset);

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::SetCBV(IContext& ictx, size_t idx, ResourceHandle	handle, size_t offset, size_t bufferSize)
	{
		Context& ctx = static_cast<Context&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::ConstantBuffer, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetCBV(%u, %u, %u): Failed to set descriptor!", idx, handle.to_uint(), offset);
			return *this;
		}
#endif

		FillState[idx] = true;

		auto resource = ctx.renderSystem->GetDeviceResource(handle);
		PushCBToDescHeap(
			ctx.renderSystem,
			resource,
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx),
			(bufferSize / 256) * 256 + 256,
			offset);

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::SetUAVBuffer(IContext& ictx, size_t idx, ResourceHandle handle, size_t offset)
	{
		auto& ctx = static_cast<Context&>(ictx);

		FK_ASSERT(idx < std::numeric_limits<uint32_t>::max());
		FK_ASSERT(offset < std::numeric_limits<uint32_t>::max());

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetUAVBuffer(%u, %u, %u): Failed to set descriptor!", idx, handle.to_uint(), offset);
			return *this;
		}
#endif

		FillState[idx] = true;

		UAVBuffer UAV{ *ctx.renderSystem, handle };
		UAV.offset = (uint32_t)offset;

		PushUAVBufferToDescHeap(
			ctx.renderSystem,
			UAV,
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx));

		return *this;
	}
	

	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::SetUAVTexture(IContext& ictx, size_t idx, ResourceHandle handle)
	{
		Context& ctx = static_cast<Context&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetUAVTexture(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return *this;
		}
#endif

		FillState[idx] = true;

		Texture2D tex;
		tex.WH			= ctx.renderSystem->GetTextureWH(handle);
		tex.Texture		= ctx.renderSystem->GetDeviceResource(handle);
		tex.Format		= ctx.renderSystem->GetTextureDeviceFormat(handle);

		PushUAV2DToDescHeap(
			ctx.renderSystem,
			tex, 
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx));

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::SetUAVTexture(IContext& ictx, size_t idx, ResourceHandle handle, DeviceFormat format)
	{
		auto& ctx = static_cast<Context&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetUAVTexture(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return *this;
		}
#endif

		FillState[idx] = true;

		Texture2D tex;
		tex.WH		= ctx.renderSystem->GetTextureWH(handle);
		tex.Texture	= ctx.renderSystem->GetDeviceResource(handle);
		tex.Format	= TextureFormat2DXGIFormat(format);

		PushUAV2DToDescHeap(
			ctx.renderSystem,
			tex, 
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx));

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::SetUAVTexture(IContext& ictx, size_t idx, size_t mipLevel, ResourceHandle handle, DeviceFormat format)
	{
		Context& ctx = static_cast<Context&>(ictx);

		FK_ASSERT(idx < std::numeric_limits<uint32_t>::max());
		FK_ASSERT(mipLevel < std::numeric_limits<uint32_t>::max());

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetUAVTexture(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return *this;
		}
#endif

		FillState[idx] = true;

		Texture2D tex;
		tex.WH		= ctx.renderSystem->GetTextureWH(handle);
		tex.Texture	= ctx.renderSystem->GetDeviceResource(handle);
		tex.Format	= TextureFormat2DXGIFormat(format);

		PushUAV2DToDescHeap(
			ctx.renderSystem,
			tex,
			(uint32_t)mipLevel,
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx));

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::SetUAVCubemap(IContext& ictx, size_t idx, ResourceHandle	handle)
	{
		auto& ctx = static_cast<Context&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetUAVCubemap(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return *this;
		}
#endif

		FillState[idx] = true;

		PushUAVCubeMapToDescHeap(
			ctx.renderSystem,
			ctx.renderSystem->GetTextureDeviceFormat(handle),
			ctx.renderSystem->GetDeviceResource(handle),
			IncrementHeapPOS(
					descriptorHeap, 
					ctx.renderSystem->DescriptorCBVSRVUAVSize, 
					idx));

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::SetUAVTexture3D(IContext& ictx, size_t idx, ResourceHandle handle, DeviceFormat format)
	{
		Context& ctx = static_cast<Context&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetUAVTexture3D(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return *this;
		}
#endif

		FillState[idx] = true;

		Texture2D tex;
		tex.WH		= ctx.renderSystem->GetTextureWH(handle);
		tex.Texture	= ctx.renderSystem->GetDeviceResource(handle);
		tex.Format	= TextureFormat2DXGIFormat(format);

		PushUAV3DToDescHeap(
			ctx.renderSystem,
			tex,
			tex.WH[0],
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx));

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::SetUAVStructured(IContext& ictx, size_t idx, ResourceHandle handle, size_t stride, size_t offset)
	{
		auto& ctx = static_cast<Context&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetUAVStructured(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return *this;
		}
#endif

		FillState[idx] = true;

		UAVBuffer uavDesc{ *ctx.renderSystem, handle, stride, offset };
		
		PushUAVBufferToDescHeap(
			ctx.renderSystem,
			uavDesc,
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx));

		return *this;
	}


	
	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::SetUAVStructured(
		IContext&		ictx,
		size_t			idx,
		ResourceHandle	resource,
		ResourceHandle	counter,
		size_t			stride,
		size_t			counterOffset)
	{
		auto& ctx = static_cast<Context&>(ictx);

		FK_ASSERT(idx < std::numeric_limits<uint32_t>::max());
		FK_ASSERT(stride < std::numeric_limits<uint32_t>::max());
		FK_ASSERT(counterOffset < std::numeric_limits<uint32_t>::max());

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx) || resource == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeap::SetUAVStructured(%u, %u): Failed to set descriptor!", idx, resource.to_uint());
			return *this;
		}
#endif

		FillState[idx] = true;

		UAVBuffer uavDesc{ *ctx.renderSystem, resource, stride, 0 };
		uavDesc.counterOffset   = (uint32_t)counterOffset;
		uavDesc.offset          = (uint32_t)(resource == counter ? Max(4096 / stride, 1) : 0);

		PushUAVBufferToDescHeap2(
			ctx.renderSystem,
			uavDesc,
			ctx.renderSystem->GetDeviceResource(counter),
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx));

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeapImpl& DescriptorHeapImpl::SetStructuredResource(IContext& ictx, size_t idx, ResourceHandle handle, size_t stride, size_t offset)
	{
		auto& ctx = static_cast<Context&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetStructuredResource(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return *this;
		}
#endif

		if (handle == InvalidHandle)
		{
			PushSRVNULLDescHeap(
				ctx.renderSystem,
				IncrementHeapPOS(
					descriptorHeap,
					ctx.renderSystem->DescriptorCBVSRVUAVSize,
					idx));

			return *this;
		}
		else
		{
			FillState[idx] = true;

			const auto byteSize = ctx.renderSystem->GetResourceSize(handle);

			PushSRVToDescHeap(
				ctx.renderSystem,
				ctx.renderSystem->Textures[handle],
				IncrementHeapPOS(descriptorHeap,
					ctx.renderSystem->DescriptorCBVSRVUAVSize,
					idx),
				byteSize / stride,
				stride,
				D3D12_BUFFER_SRV_FLAG_NONE,
				offset);

			return *this;
		}
	}


	/************************************************************************************************/


	DescriptorHeapImpl DescriptorHeapImpl::GetHeapOffsetted(size_t offset, IContext& ictx) const
	{
		auto& ctx = static_cast<Context&>(ictx);

		DescriptorHeap subHeap = Clone();
		subHeap.descriptorHeap = IncrementHeapPOS(
										descriptorHeap,
										ctx.renderSystem->DescriptorCBVSRVUAVSize,
										offset);

		return subHeap;
	}


	//void DescriptorHeap::Mirror(const DescriptorHeap& rhs);
	//DescriptorHeap DescriptorHeap::Clone() const;

	DescriptorHeap::operator DescriptorRange() const noexcept
	{
		return {
			.begin		= descriptorHeap,
			.size		= static_cast<uint32_t>(FillState.size()),
			.stride		= static_cast<uint32_t>(RenderSystem::_GetInstance().DescriptorCBVSRVUAVSize)
		};
	}


	bool DescriptorHeapImpl::CheckType(const DesciptorHeapLayout<>& layout, DescHeapEntryType type, size_t idx)
	{
		size_t entryIdx = 0;
		for (HeapDescriptor entry : layout.Entries)
		{
			if ((entry.Type == type)	&& 
				(entryIdx <= idx)		&&
				(entryIdx + entry.Space + entry.Count > idx))
				return true;

			entryIdx += entry.Count + entry.Space;
		}

		return false;
	}


	/************************************************************************************************/


	bool RootSignatureBuilder::SetParameterAsUINT(size_t Index, uint32_t size, uint32_t cbRegister, uint32_t registerSpace, PIPELINE_DESTINATION AccessableStages)
	{
		RootEntry Desc;
		Desc.Type							= RootSignatureEntryType::UINT;
		Desc.UINTConstant.size              = size;
		Desc.UINTConstant.Register		    = cbRegister;
		Desc.UINTConstant.RegisterSpace     = registerSpace;
		Desc.UINTConstant.Accessibility	    = AccessableStages;

		if (RootEntries.size() <= Index)
		{
			if (!RootEntries.full())
				RootEntries.resize(Index + 1);
			else
				return false;
		}

		RootEntries[Index]  = Desc;

		return true;
	}


	/************************************************************************************************/


	bool RootSignatureBuilder::SetParameterAsCBV(
		size_t Index, size_t Register, size_t RegisterSpace, 
		PIPELINE_DESTINATION AccessableStages)
	{
		RootEntry Desc;
		Desc.Type					= RootSignatureEntryType::ConstantBuffer;
		Desc.Direct.Register		= (uint32_t)Register;
		Desc.Direct.RegisterSpace	= (uint32_t)RegisterSpace;
		Desc.Direct.Accessibility	= AccessableStages;


		if (RootEntries.size() <= Index)
		{
			if (!RootEntries.full())
				RootEntries.resize(Index + 1);
			else
				return false;
		}

		RootEntries[Index] = Desc;

		return false;
	}


	/************************************************************************************************/


	bool RootSignatureBuilder::SetParameterAsUAV(
		size_t Index, size_t Register, size_t RegisterSpace,
		PIPELINE_DESTINATION AccessableStages)
	{
		RootEntry Desc;
		Desc.Type					= RootSignatureEntryType::UnorderedAcess;
		Desc.Direct.Register		= (uint32_t)Register;
		Desc.Direct.RegisterSpace	= (uint32_t)RegisterSpace;
		Desc.Direct.Accessibility	= AccessableStages;


		if (RootEntries.size() <= Index)
		{
			if (!RootEntries.full())
				RootEntries.resize(Index + 1);
			else
				return false;
		}

		RootEntries[Index] = Desc;

		return false;
	}


	/************************************************************************************************/


	bool RootSignatureBuilder::SetParameterAsSRV(
		size_t Index, size_t Register, size_t RegisterSpace,
		PIPELINE_DESTINATION AccessableStages)
	{
		RootEntry Desc;
		Desc.Type					= RootSignatureEntryType::StructuredBuffer;
		Desc.Direct.Register		= (uint32_t)Register;
		Desc.Direct.RegisterSpace	= (uint32_t)RegisterSpace;
		Desc.Direct.Accessibility	= AccessableStages;

		if (RootEntries.size() <= Index)
		{
			if (!RootEntries.full())
				RootEntries.resize(Index + 1);
			else
				return false;
		}

		RootEntries[Index] = Desc;

		return false;
	}


	/************************************************************************************************/


	void RootSignatureBuilder::Clear()
	{
		Heaps.clear();
		RootEntries.clear();

		AllowIA = true;
		AllowSO = false;
	}


	/************************************************************************************************/


	RootSignature* RootSignatureBuilder::Build(RenderSystem* RS, iAllocator& temp)
	{
		auto result = RenderSystem::_GetInstance()._CreateRootSignature(*this, temp);

		if (result)
		{
			Clear();
		}
		else
			FK_LOG_ERROR("Failed to build root signature!");

		return result;
	}


	/************************************************************************************************/


	RootSignature* RootSignatureBuilder::LoadSignatureFromFile(const char* dir, const char* entry, RenderSystem& renderSystem, iAllocator& temp)
	{
		auto result = renderSystem.LoadRootSignature(dir, entry);
		
		if(result)
		{
			auto&& rootSignature = result.value();
			
			ID3D12VersionedRootSignatureDeserializer* deserializer;
			auto res  = D3D12CreateVersionedRootSignatureDeserializer(rootSignature.buffer, rootSignature.bufferSize, IID_PPV_ARGS(&deserializer));

			if (!deserializer)
			{
				std::string trace;

				for (const auto& frame : std::stacktrace::current())
					trace += "\t" + frame.description() + "\n";

				FK_LOG_ERROR("Failed to deserialize root signature!\nStack trace:\n%s", trace.c_str());

				return nullptr;
			}

			const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* versioned_desc;
			deserializer->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_0, &versioned_desc);
			auto desc = &versioned_desc->Desc_1_1;

			size_t parametersEnd = desc->NumParameters;
			for(size_t itr = 0; itr < parametersEnd; itr++)
			{
				switch(desc->pParameters[itr].ParameterType)
				{
					case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
					{
						auto& parameter = desc->pParameters[itr].DescriptorTable;
						
						for(auto&& [idx, range] : zip(iota(0), std::span{ parameter.pDescriptorRanges, parameter.NumDescriptorRanges}))
						{
							DesciptorHeapLayout<16> layout;
							switch(range.RangeType)
							{
							case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
							{
								layout.SetParameterAsSRV(
									idx, 
									range.BaseShaderRegister, 
									range.NumDescriptors,
									range.RegisterSpace);
							}	break;
							case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
							{
								layout.SetParameterAsShaderUAV(
									idx, 
									range.BaseShaderRegister, 
									range.NumDescriptors,
									range.RegisterSpace);
							}	break;
							case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
							{
								layout.SetParameterAsCBV(
									idx, 
									range.BaseShaderRegister, 
									range.NumDescriptors,
									range.RegisterSpace);
							}	break;
							case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER: 
							{
								FK_ASSERT(0, "Unimplemented funcionality!");
							}	break;
							}

							Heaps.emplace_back(Heaps.size(), layout);
						}
					
						SetParameterAsDescriptorTable(itr, Heaps.back().Heap, ShaderVis2PipelineDest(desc->pParameters[itr].ShaderVisibility));
					}	break;
					case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
					{
						auto& parameter = desc->pParameters[itr].Constants;
						SetParameterAsUINT(itr, parameter.Num32BitValues, parameter.ShaderRegister, parameter.RegisterSpace, ShaderVis2PipelineDest(desc->pParameters[itr].ShaderVisibility));
					}	break;
					case D3D12_ROOT_PARAMETER_TYPE_CBV:
					{
						auto& parameter = desc->pParameters[itr].Descriptor;
						SetParameterAsCBV(itr, parameter.ShaderRegister, parameter.ShaderRegister, ShaderVis2PipelineDest(desc->pParameters[itr].ShaderVisibility));
					}	break;
					case D3D12_ROOT_PARAMETER_TYPE_SRV:
					{
						auto& parameter = desc->pParameters[itr].Descriptor;
						SetParameterAsSRV(itr, parameter.ShaderRegister, parameter.ShaderRegister, ShaderVis2PipelineDest(desc->pParameters[itr].ShaderVisibility));
					}	break;
					case D3D12_ROOT_PARAMETER_TYPE_UAV:
					{
						auto& parameter = desc->pParameters[itr].Descriptor;
						SetParameterAsUAV(itr, parameter.ShaderRegister, parameter.ShaderRegister, ShaderVis2PipelineDest(desc->pParameters[itr].ShaderVisibility));
					}	break;
				}
			}

			deserializer->Release();

			auto signature = renderSystem._CreateRootSignature(*this, temp);

			if (!signature)
				return nullptr;

			Clear();

			return signature;
		}
		else
			FK_LOG_ERROR("LoadSignatureFromFile failed to load signature. Reason: %s", result.error().c_str());

			return nullptr;
	}


	/************************************************************************************************/


	RootSignature* RootSignatureBuilder::LoadSignatureFromBlob(void* buffer, const size_t bufferSize, RenderSystem& renderSystem, iAllocator& temp)
	{
		ID3D12VersionedRootSignatureDeserializer* deserializer;
		auto HR  = D3D12CreateVersionedRootSignatureDeserializer(buffer, bufferSize, IID_PPV_ARGS(&deserializer));
		if (FAILED(HR))
			return nullptr;

		const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* versioned_desc;
		HR	= deserializer->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_1, &versioned_desc);

		if (FAILED(HR))
			return nullptr;

		ID3D12RootSignature* dxRootSig = nullptr;
		HR = renderSystem.pDevice14->CreateRootSignature(0, buffer, bufferSize, IID_PPV_ARGS(&dxRootSig));

		if (FAILED(HR))
			return nullptr;

		auto desc = &versioned_desc->Desc_1_1;

		size_t parametersEnd = desc->NumParameters;
		for(size_t itr = 0; itr < parametersEnd; itr++)
		{
			switch(desc->pParameters[itr].ParameterType)
			{
				case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
				{
					auto& parameter = desc->pParameters[itr].DescriptorTable;
					DesciptorHeapLayout<16> layout;

					for(auto&& [idx, range] : zip(iota(0), std::span{ parameter.pDescriptorRanges, parameter.NumDescriptorRanges}))
					{
						switch(range.RangeType)
						{
						case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
						{
							layout.SetParameterAsSRV(
								idx, 
								range.BaseShaderRegister, 
								range.NumDescriptors,
								range.RegisterSpace);
						}	break;
						case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
						{
							layout.SetParameterAsShaderUAV(
								idx, 
								range.BaseShaderRegister, 
								range.NumDescriptors,
								range.RegisterSpace);
						}	break;
						case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
						{
							layout.SetParameterAsCBV(
								idx, 
								range.BaseShaderRegister, 
								range.NumDescriptors,
								range.RegisterSpace);
						}	break;
						case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER: 
						{
							FK_ASSERT(0, "Unimplemented funcionality!");
						}	break;
						}
					}
					
					SetParameterAsDescriptorTable(itr, layout, ShaderVis2PipelineDest(desc->pParameters[itr].ShaderVisibility));
				}	break;
				case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
				{
					auto& parameter = desc->pParameters[itr].Constants;
					SetParameterAsUINT(itr, parameter.Num32BitValues, parameter.ShaderRegister, parameter.RegisterSpace, ShaderVis2PipelineDest(desc->pParameters[itr].ShaderVisibility));
				}	break;
				case D3D12_ROOT_PARAMETER_TYPE_CBV:
				{
					auto& parameter = desc->pParameters[itr].Descriptor;
					SetParameterAsCBV(itr, parameter.ShaderRegister, parameter.ShaderRegister, ShaderVis2PipelineDest(desc->pParameters[itr].ShaderVisibility));
				}	break;
				case D3D12_ROOT_PARAMETER_TYPE_SRV:
				{
					auto& parameter = desc->pParameters[itr].Descriptor;
					SetParameterAsSRV(itr, parameter.ShaderRegister, parameter.ShaderRegister, ShaderVis2PipelineDest(desc->pParameters[itr].ShaderVisibility));
				}	break;
				case D3D12_ROOT_PARAMETER_TYPE_UAV:
				{
					auto& parameter = desc->pParameters[itr].Descriptor;
					SetParameterAsUAV(itr, parameter.ShaderRegister, parameter.ShaderRegister, ShaderVis2PipelineDest(desc->pParameters[itr].ShaderVisibility));
				}	break;
			}
		}

		auto signature = renderSystem._CreateRootSignature(dxRootSig, *this);

		deserializer->Release();

		Clear();

		return signature;
	}


	/************************************************************************************************/


	void RootSignature::Release() const
	{
		if(Signature && !Signature->Release())
		{
			auto* mutable_this = const_cast<RootSignature*>(this);

			mutable_this->Heaps.Release();

			auto t = (uint64_t)Signature;
			mutable_this->Signature = nullptr;
			RenderSystem::_GetInstance()._ReleaseRootSignature(t);
			allocator->free(mutable_this);
		}
	}


	/************************************************************************************************/


	size_t RootSignature::GetDesciptorTableSize(size_t idx) const
	{
		FK_ASSERT(idx < Heaps.size());
		return Heaps[idx].Heap.size();
	}


	/************************************************************************************************/


	PipelineBuilderImpl::PipelineBuilderImpl(iAllocator& IN_allocator) :
		allocator	{ IN_allocator },
		blob		{ IN_allocator },
		shaders		{ IN_allocator }
	{
		blob.buffer.reserve(1024);
	}


	/************************************************************************************************/


	PipelineBuilderImpl::~PipelineBuilderImpl()
	{
		allocator->free(inputElements);
	}


	/************************************************************************************************/


	PipelineBuilderImpl& PipelineBuilderImpl::AddRootSignature(const RootSignature* IN_rootSig)
	{
		rootSig = IN_rootSig;

		blob += CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE{ rootSig->Get_ptr() };

		return *this;
	}


	/************************************************************************************************/


	PipelineBuilderImpl& PipelineBuilderImpl::AddShaderLibrary(const char* file, const ShaderOptions& options)
	{
		/*
		shaders.emplace_back(RenderSystem::_GetInstance().LoadShader(nullptr, "lib_6_8", file, options));
		hash = FNVa62(shaders.back().buffer, shaders.back().bufferSize, hash);

		struct
		{
			D3D12_STATE_SUBOBJECT_TYPE type = D3D12_STATE_SUBOBJECT_TYPE::D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;

		} DXILObject;
		blob += DXILObject;

		//D3D12_STATE_SUBOBJECT_TYPE_WORK_GRAPH
		*/

		return *this;
	}


	/************************************************************************************************/


	PipelineBuilderImpl& PipelineBuilderImpl::AddComputeShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		shaders.emplace_back(RenderSystem::_GetInstance().LoadShader(entryPoint, "cs_6_7", file, options));
		hash = FNVa62(shaders.back().buffer, shaders.back().bufferSize, hash);

		CD3DX12_PIPELINE_STATE_STREAM_CS streamObject = D3D12_SHADER_BYTECODE{ (D3D12_SHADER_BYTECODE)Shader2ByteCode(shaders.back()) };
		blob += streamObject;

		return *this;
	}


	/************************************************************************************************/


	PipelineBuilderImpl& PipelineBuilderImpl::AddWorkGraph(const WorkGraph_Desc& work_desc)
	{
		struct {
			D3D12_STATE_SUBOBJECT_TYPE type = D3D12_STATE_SUBOBJECT_TYPE::D3D12_STATE_SUBOBJECT_TYPE_WORK_GRAPH;
			D3D12_WORK_GRAPH_DESC workGraph;
		} subObject = {
			.workGraph {
				.ProgramName				= nullptr,
				.Flags						= (D3D12_WORK_GRAPH_FLAGS)work_desc.flags,
				.NumEntrypoints				= 0,
				.pEntrypoints				= nullptr,
				.NumExplicitlyDefinedNodes	= work_desc.nodeCount,
				.pExplicitlyDefinedNodes	= nullptr,
			}
		};

		blob += subObject;

		return *this;
	}


	/************************************************************************************************/


	PipelineBuilderImpl& PipelineBuilderImpl::AddVertexShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		shaders.emplace_back(RenderSystem::_GetInstance().LoadShader(entryPoint, "vs_6_7", file, options));
		hash = FNVa62(shaders.back().buffer, shaders.back().bufferSize, hash);

		CD3DX12_PIPELINE_STATE_STREAM_VS streamObject = D3D12_SHADER_BYTECODE{ (D3D12_SHADER_BYTECODE)Shader2ByteCode(shaders.back()) };
		blob += streamObject;

		return *this;
	}


	/************************************************************************************************/


	PipelineBuilderImpl& PipelineBuilderImpl::AddDomainShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		shaders.emplace_back(RenderSystem::_GetInstance().LoadShader(entryPoint, "ds_6_7", file, options));
		hash = FNVa62(shaders.back().buffer, shaders.back().bufferSize, hash);

		CD3DX12_PIPELINE_STATE_STREAM_DS streamObject = D3D12_SHADER_BYTECODE{ (D3D12_SHADER_BYTECODE)Shader2ByteCode(shaders.back()) };
		blob += streamObject;

		return *this;
	}


	/************************************************************************************************/


	PipelineBuilderImpl& PipelineBuilderImpl::AddHullShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		shaders.emplace_back(RenderSystem::_GetInstance().LoadShader(entryPoint, "hs_6_7", file, options));
		hash = FNVa62(shaders.back().buffer, shaders.back().bufferSize, hash);

		CD3DX12_PIPELINE_STATE_STREAM_HS streamObject = D3D12_SHADER_BYTECODE{ (D3D12_SHADER_BYTECODE)Shader2ByteCode(shaders.back()) };
		blob += streamObject;

		return *this;
	}


	/************************************************************************************************/


	PipelineBuilderImpl& PipelineBuilderImpl::AddGeometryShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		shaders.emplace_back(RenderSystem::_GetInstance().LoadShader(entryPoint, "gs_6_7", file, options));
		hash = FNVa62(shaders.back().buffer, shaders.back().bufferSize, hash);

		CD3DX12_PIPELINE_STATE_STREAM_GS streamObject = D3D12_SHADER_BYTECODE{ (D3D12_SHADER_BYTECODE)Shader2ByteCode(shaders.back()) };
		blob += streamObject;

		return *this;
	}


	/************************************************************************************************/


	PipelineBuilderImpl& PipelineBuilderImpl::AddAmplificationShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		shaders.emplace_back(RenderSystem::_GetInstance().LoadShader(entryPoint, "as_6_7", file, options));
		hash = FNVa62(shaders.back().buffer, shaders.back().bufferSize, hash);

		CD3DX12_PIPELINE_STATE_STREAM_AS streamObject = D3D12_SHADER_BYTECODE{ (D3D12_SHADER_BYTECODE)Shader2ByteCode(shaders.back()) };
		blob += streamObject;

		return *this;
	}


	/************************************************************************************************/


	PipelineBuilderImpl& PipelineBuilderImpl::AddMeshShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		shaders.emplace_back(RenderSystem::_GetInstance().LoadShader(entryPoint, "ms_6_7", file, options));
		hash = FNVa62(shaders.back().buffer, shaders.back().bufferSize, hash);

		CD3DX12_PIPELINE_STATE_STREAM_MS streamObject = D3D12_SHADER_BYTECODE{ (D3D12_SHADER_BYTECODE)Shader2ByteCode(shaders.back()) };
		blob += streamObject;

		return *this;
	}


	/************************************************************************************************/


	PipelineBuilderImpl& PipelineBuilderImpl::AddPixelShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		shaders.emplace_back(RenderSystem::_GetInstance().LoadShader(entryPoint, "ps_6_7", file, options));
		hash = FNVa62(shaders.back().buffer, shaders.back().bufferSize, hash);

		CD3DX12_PIPELINE_STATE_STREAM_PS streamObject = D3D12_SHADER_BYTECODE{ (D3D12_SHADER_BYTECODE)Shader2ByteCode(shaders.back()) };
		blob += streamObject;

		return *this;
	}


	/************************************************************************************************/


	PipelineBuilderImpl& PipelineBuilderImpl::AddInputLayout(const InputLayoutState& state)
	{
		if (inputElements)
			return *this;

		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT layout;
		memset(std::addressof(layout), 0, sizeof(layout));

		inputElements = (D3D12_INPUT_ELEMENT_DESC*)allocator->malloc(state.count * sizeof(D3D12_INPUT_ELEMENT_DESC));

		for (auto&& [idx, input] : zip(iota(0u, state.count), state.inputs))
		{
			inputElements[idx] =
				D3D12_INPUT_ELEMENT_DESC{
					.SemanticName			= input.name,
					.SemanticIndex			= input.index,
					.Format					= TextureFormat2DXGIFormat(input.format),
					.InputSlot				= input.slot,
					.AlignedByteOffset		= input.alignedByteOffset,
					.InputSlotClass			= ToDX(input.inputSlotClass),
					.InstanceDataStepRate	= input.instanceStepRate
				};
		}

		layout = D3D12_INPUT_LAYOUT_DESC{
			.pInputElementDescs = inputElements,
			.NumElements		= state.count,
		};

		hash = FNVa62((const char*)inputElements, state.count * sizeof(D3D12_INPUT_ELEMENT_DESC), hash);
		blob += layout;

		return *this;
	}


	/************************************************************************************************/


	PipelineBuilderImpl& PipelineBuilderImpl::AddInputTopology(const ETopology topology)
	{
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY dxTopolgy{ (D3D12_PRIMITIVE_TOPOLOGY_TYPE)topology };
		hash = FNVa62((const char*)&dxTopolgy, sizeof(dxTopolgy), hash);
		blob += dxTopolgy;

		return *this;
	}


	/************************************************************************************************/


	PipelineBuilderImpl& PipelineBuilderImpl::AddRasterizerState(const RasterizerState& state)
	{
		CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER2 rasterizerState;
		memset(&rasterizerState, 0, sizeof(CD3DX12_RASTERIZER_DESC2));

		CD3DX12_RASTERIZER_DESC2& desc = rasterizerState;
		desc = CD3DX12_RASTERIZER_DESC2{ D3D12_DEFAULT };

		desc.ConservativeRaster		= state.conservativeRasterEnable ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		desc.CullMode				= (D3D12_CULL_MODE)state.CullMode;
		desc.DepthBias				= state.depthBias;
		desc.DepthBiasClamp			= state.depthBiasClamp;
		desc.FillMode				= (D3D12_FILL_MODE)state.fill;
		desc.ForcedSampleCount		= state.forcedSampleCount;
		desc.FrontCounterClockwise	= state.frontCounterClockWise;
		desc.LineRasterizationMode	= (D3D12_LINE_RASTERIZATION_MODE)state.antialiasedLineMode;

		hash = FNVa62((const char*)&rasterizerState, sizeof(rasterizerState), hash);
		blob += rasterizerState;

		return *this;
	}


	/************************************************************************************************/


	PipelineBuilderImpl& PipelineBuilderImpl::AddDepthStencilState(const DepthStencilState& inputState)
	{
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL depthStencil{};
		CD3DX12_DEPTH_STENCIL_DESC& state = depthStencil;

		state.BackFace.StencilDepthFailOp	= (D3D12_STENCIL_OP)inputState.backFace.stencilDepthFailOp;
		state.BackFace.StencilFailOp		= (D3D12_STENCIL_OP)inputState.backFace.stencilFailOp;
		state.BackFace.StencilPassOp		= (D3D12_STENCIL_OP)inputState.backFace.stencilPassOp;
		state.BackFace.StencilFunc			= (D3D12_COMPARISON_FUNC)inputState.backFace.stencilFunc;

		state.FrontFace.StencilDepthFailOp	= (D3D12_STENCIL_OP)inputState.frontFace.stencilDepthFailOp;
		state.FrontFace.StencilFailOp		= (D3D12_STENCIL_OP)inputState.frontFace.stencilFailOp;
		state.FrontFace.StencilPassOp		= (D3D12_STENCIL_OP)inputState.frontFace.stencilPassOp;
		state.FrontFace.StencilFunc			= (D3D12_COMPARISON_FUNC)inputState.frontFace.stencilFunc;

		state.DepthEnable		= (uint32_t)inputState.depthEnable;
		state.DepthFunc			= (D3D12_COMPARISON_FUNC)inputState.depthFunc;
		state.DepthWriteMask	= (D3D12_DEPTH_WRITE_MASK)inputState.depthWriteMask;
		state.StencilEnable		= inputState.stencilEnable;
		state.StencilReadMask	= inputState.stencilReadMask;
		state.StencilWriteMask	= inputState.stencilWriteMask;

		hash = FNVa62((const char*)&depthStencil, sizeof(depthStencil), hash);
		blob += depthStencil;

		return *this;
	}


	/************************************************************************************************/


	PipelineBuilderImpl& PipelineBuilderImpl::AddBlendState(const BlendState& state)
	{
		CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC blendState{};
		CD3DX12_BLEND_DESC& desc = blendState;

		desc.AlphaToCoverageEnable	= state.alphaToCoverageEnable;
		desc.IndependentBlendEnable	= state.independentBlendEnable;
		memcpy(&desc.RenderTarget, &state.renderTarget, sizeof(state));

		blob += blendState;

		return *this;
	}


	/************************************************************************************************/


	PipelineBuilderImpl& PipelineBuilderImpl::AddRenderTargetState(const RenderTargetState& state)
	{
		CD3DX12_RT_FORMAT_ARRAY formats{};
		memset(&formats, 0, sizeof(formats));
		formats.NumRenderTargets = state.targetCount;

		for (auto [idx, format] : zip(iota(0u, state.targetCount), state.targetFormats))
			formats.RTFormats[idx] = TextureFormat2DXGIFormat(format);

		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS dxFormats{ formats };
		hash = FNVa62((const char*)&dxFormats, sizeof(dxFormats), hash);
		blob += dxFormats;

		return *this;
	}


	/************************************************************************************************/


	PipelineBuilderImpl& PipelineBuilderImpl::AddDepthStencilFormat(const DeviceFormat format)
	{
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT dxDepthFormat{ TextureFormat2DXGIFormat(format) };
		hash = FNVa62((const char*)&dxDepthFormat, sizeof(dxDepthFormat), hash);
		blob += dxDepthFormat;

		return *this;
	}


	/************************************************************************************************/


	FlexKit::LoadPipelineStateRes PipelineBuilderImpl::Build(RenderSystem& renderSystem)
	{
		D3D12_PIPELINE_STATE_STREAM_DESC streamDesc{
			.SizeInBytes					= blob.size(),
			.pPipelineStateSubobjectStream	= blob.data()
		};

		ID3D12PipelineState* pso = nullptr;
		auto HR = renderSystem.pDevice14->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pso));

		if (SUCCEEDED(HR))
		{
			if (!rootSig)
			{
				for (auto& shader : shaders)
				{
					ID3D12RootSignature* dxRootSig = nullptr;
					HR = renderSystem.pDevice14->CreateRootSignature(0,shader.buffer, shader.bufferSize, IID_PPV_ARGS(&dxRootSig));

					if (SUCCEEDED(HR))
					{
						rootSig = renderSystem._GetRootSignature((uint64_t)dxRootSig);

						if (!rootSig)
						{
							RootSignatureBuilder builder{ *renderSystem.Memory };
							rootSig = builder.LoadSignatureFromBlob(shader.buffer, shader.bufferSize, renderSystem, *renderSystem.Memory);
						}
						break;
					}
				}
			}

			if (!rootSig)
			{
				// TODO: use shader reflection!
				FK_LOG_ERROR("Failed to acquire root signature!");
				return { nullptr, nullptr };
			}

			if (pso && debugName)
			{
				SETDEBUGNAME(pso, debugName);
				rootSig->SetDebugStr(debugName);
			}

			allocator->free(inputElements);
			inputElements = nullptr;
			blob.Clear();

			return { pso, rootSig };
		}
		else
			return { nullptr, nullptr };
	}


	/************************************************************************************************/


	FlexKit::LoadPipelineStateRes PipelineBuilderImpl::BuildStream(RenderSystem& renderSystem, void* buffer, const size_t size)
	{
		const D3D12_PIPELINE_STATE_STREAM_DESC streamDesc{
			.SizeInBytes					= size,
			.pPipelineStateSubobjectStream	= buffer
		};

		ID3D12PipelineState* pso = nullptr;
		auto HR = renderSystem.pDevice14->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pso));

		return { pso, nullptr };
	}


	/************************************************************************************************/


	Context::Context(
				RenderSystem*	renderSystem_IN,
				iAllocator*		allocator) :
			CurrentRootSignature	{ nullptr			},
			pendingBarriers			{ },
			renderSystem			{ renderSystem_IN	},
			Memory					{ allocator			},
			RenderTargetCount		{ 0					},
			DepthStencilEnabled		{ false				},
			TrackedSOBuffers		{ }
	{
		HRESULT HR;

		D3D12_DESCRIPTOR_HEAP_DESC cpuDescriptorHeapdesc;
		cpuDescriptorHeapdesc.Flags				= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		cpuDescriptorHeapdesc.NumDescriptors	= 1024;
		cpuDescriptorHeapdesc.NodeMask			= 0;
		cpuDescriptorHeapdesc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		HR = renderSystem->pDevice->CreateDescriptorHeap(&cpuDescriptorHeapdesc, IID_PPV_ARGS(&descHeapSRVLocal));
		FK_ASSERT(HR, "FAILED TO CREATE DESCRIPTOR HEAP");

		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapdesc;
		descriptorHeapdesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		descriptorHeapdesc.NumDescriptors	= 128;
		descriptorHeapdesc.NodeMask			= 0;
		descriptorHeapdesc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		HR = renderSystem->pDevice->CreateDescriptorHeap(&descriptorHeapdesc, IID_PPV_ARGS(&descHeapRTV));
		FK_ASSERT(HR, "FAILED TO CREATE DESCRIPTOR HEAP");

		descriptorHeapdesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		descriptorHeapdesc.NumDescriptors	= 128;
		descriptorHeapdesc.NodeMask			= 0;
		descriptorHeapdesc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		HR = renderSystem->pDevice->CreateDescriptorHeap(&descriptorHeapdesc, IID_PPV_ARGS(&descHeapDSV));
		FK_ASSERT(HR, "FAILED TO CREATE DESCRIPTOR HEAP");

		FK_LOG_9("GRAPHICS SRV DESCRIPTOR HEAP CREATED: %u", descHeapSRVLocal);
		FK_LOG_9("GRAPHICS RTV DESCRIPTOR HEAP CREATED: %u", descHeapRTV);
		FK_LOG_9("GRAPHICS DSV DESCRIPTOR HEAP CREATED: %u", descHeapDSV);

		SETDEBUGNAME(descHeapRTV, "GPURESOURCEHEAP");
		SETDEBUGNAME(descHeapDSV, "RENDERTARGETHEAP");

		HR = renderSystem->pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));												FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND ALLOCATOR!");
		HR = renderSystem->pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&DeviceContext));	FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND LIST!");

#if USING(DEBUGGRAPHICS)
		DeviceContext->QueryInterface(IID_PPV_ARGS(&debugCommandList));
#endif

		SETDEBUGNAME(DeviceContext, "GraphicsContext");
		SETDEBUGNAME(DeviceContext, "GraphicsContextAllocator");

#if USING(AFTERMATH)
		auto res = GFSDK_Aftermath_DX12_CreateContextHandle(DeviceContext, &AFTERMATH_context);
#endif

		DeviceContext->Close();
	}


	/************************************************************************************************/

	Context::Context(Context&& RHS)
	{
		DeviceContext			= RHS.DeviceContext;
		CurrentRootSignature	= RHS.CurrentRootSignature;
		CurrentPipelineState	= RHS.CurrentPipelineState;
		renderSystem			= RHS.renderSystem;

#if USING(DEBUGGRAPHICS)
		debugCommandList		= RHS.debugCommandList;
#endif

		RTV_CPU = RHS.RTV_CPU;

		shaderResources = RHS.shaderResources;
		heapUsed		= RHS.heapUsed;

		DSV_CPU = RHS.DSV_CPU;

		descHeapRTV = RHS.descHeapRTV;
		descHeapDSV = RHS.descHeapDSV;

		RenderTargetCount		= RHS.RenderTargetCount;
		DepthStencilEnabled		= RHS.DepthStencilEnabled;

		DesciptorHeaps			= RHS.DesciptorHeaps;
		VBViews					= RHS.VBViews;
		pendingBarriers			= RHS.pendingBarriers;
		Memory					= RHS.Memory;
		commandAllocator		= RHS.commandAllocator;

		// Null out old Context
		RHS.commandAllocator		= nullptr;
		RHS.DeviceContext			= nullptr;
		RHS.CurrentRootSignature	= nullptr;

#if USING(DEBUGGRAPHICS)
		RHS.debugCommandList		= nullptr;
#endif

		RHS.RenderTargetCount		= 0;
		RHS.DepthStencilEnabled		= false;
		RHS.Memory					= nullptr;

		RHS.DesciptorHeaps.clear();
		RHS.VBViews.clear();
		RHS.pendingBarriers.clear();

		RHS.RTV_CPU = { 0 };

		RHS.shaderResources = {};
		RHS.heapUsed		= 0;

		RHS.DSV_CPU = { 0 };
		RHS.descHeapRTV = nullptr;
		RHS.descHeapDSV = nullptr;

#if USING(AFTERMATH)
		AFTERMATH_context       = RHS.AFTERMATH_context;
		RHS.AFTERMATH_context   = nullptr;
#endif
	}


	/************************************************************************************************/


	Context& Context::operator = (Context&& RHS)// Moves only
	{
		DeviceContext			= RHS.DeviceContext;
		CurrentRootSignature	= RHS.CurrentRootSignature;
		CurrentPipelineState	= RHS.CurrentPipelineState;
		renderSystem			= RHS.renderSystem;

#if USING(DEBUGGRAPHICS)
		debugCommandList		= RHS.debugCommandList;
#endif

		
		shaderResources = RHS.shaderResources;
		heapUsed		= RHS.heapUsed;


		RTV_CPU = RHS.RTV_CPU;
		DSV_CPU = RHS.DSV_CPU;

		descHeapRTV = RHS.descHeapRTV;
		descHeapDSV = RHS.descHeapDSV;

		RenderTargetCount		= RHS.RenderTargetCount;
		DepthStencilEnabled		= RHS.DepthStencilEnabled;

		DesciptorHeaps			= RHS.DesciptorHeaps;
		VBViews					= RHS.VBViews;
		pendingBarriers			= RHS.pendingBarriers;
		Memory					= RHS.Memory;
		commandAllocator		= RHS.commandAllocator;

		// Null out old Context
		RHS.commandAllocator		= nullptr;
		RHS.DeviceContext			= nullptr;
		RHS.CurrentRootSignature	= nullptr;

#if USING(DEBUGGRAPHICS)
		RHS.debugCommandList		= nullptr;
#endif

		RHS.RenderTargetCount		= 0;
		RHS.DepthStencilEnabled		= false;
		RHS.Memory					= nullptr;

		RHS.DesciptorHeaps.clear();
		RHS.VBViews.clear();
		RHS.pendingBarriers.clear();

		RHS.DSV_CPU			= { 0 };
		RHS.RTV_CPU			= { 0 };
		RHS.shaderResources = {};
		RHS.heapUsed		= 0;

		RHS.descHeapRTV = nullptr;
		RHS.descHeapDSV = nullptr;


#if USING(AFTERMATH)
		AFTERMATH_context       = RHS.AFTERMATH_context;
		RHS.AFTERMATH_context   = nullptr;
#endif

		return *this;
	}


	/************************************************************************************************/


	void Context::Release()
	{
#if USING(AFTERMATH)
		GFSDK_Aftermath_ReleaseContextHandle(AFTERMATH_context);
#endif

		if(descHeapRTV)
			descHeapRTV->Release();

		if (descHeapSRVLocal)
			descHeapSRVLocal->Release();

		if (descHeapDSV)
			descHeapDSV->Release();

		if(commandAllocator)
			commandAllocator->Release();

#if USING(DEBUGGRAPHICS)
		if (debugCommandList)
			debugCommandList->Release();
#endif

		if(DeviceContext)
			DeviceContext->Release();

		shaderResources			= {};
		heapUsed				= 0;
		descHeapDSV				= nullptr;
		descHeapRTV				= nullptr;
		descHeapSRVLocal		= nullptr;
		commandAllocator		= nullptr;
		DeviceContext			= nullptr;
		CurrentPipelineState	= nullptr;

#if USING(DEBUGGRAPHICS)
		debugCommandList	 = nullptr;
#endif
	}


	/************************************************************************************************/


	void Context::CreateAS(const AccelerationStructureDesc& asDesc, const TriMesh&)
	{
		FK_ASSERT(0);
	}


	void Context::BuildBLAS(IVertexBufferSet& bufferSet, ResourceHandle destination, ResourceHandle scratchSpace)
	{
		auto indexBuffer    = bufferSet[bufferSet.GetIndexBufferIndex()];
		auto positionBuffer = bufferSet.Find(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION);

		D3D12_RAYTRACING_GEOMETRY_DESC desc;
		desc.Type   = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		desc.Flags =
			D3D12_RAYTRACING_GEOMETRY_FLAGS::D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

		desc.Triangles.Transform3x4 = 0;
		desc.Triangles.IndexFormat  = DXGI_FORMAT_R32_UINT;
		desc.Triangles.IndexBuffer  = GetDevicePointer(indexBuffer);
		desc.Triangles.IndexCount   = (UINT)indexBuffer.Size();

		desc.Triangles.VertexFormat                 = DXGI_FORMAT_R32G32B32_FLOAT;
		desc.Triangles.VertexBuffer.StartAddress    = GetDevicePointer(positionBuffer.value());
		desc.Triangles.VertexBuffer.StrideInBytes   = positionBuffer.value().byteStride;
		desc.Triangles.VertexCount                  = (UINT)positionBuffer->Size();

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build_desc {
							.DestAccelerationStructureData = renderSystem->GetDeviceResource(destination)->GetGPUVirtualAddress(),
							.Inputs = {
								.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
								.Flags          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
								.NumDescs       = 1,
								.DescsLayout    = D3D12_ELEMENTS_LAYOUT::D3D12_ELEMENTS_LAYOUT_ARRAY,
								.pGeometryDescs = &desc,
							},
							.ScratchAccelerationStructureData = renderSystem->GetDeviceResource(scratchSpace)->GetGPUVirtualAddress(),
		};

		UpdateResourceStates();
		DeviceContext->BuildRaytracingAccelerationStructure(&build_desc, 0, nullptr);
	}


	/************************************************************************************************/


	void Context::DiscardResource(ResourceHandle resource)
	{
		UpdateResourceStates();

		DeviceContext->DiscardResource(renderSystem->GetDeviceResource(resource), nullptr);
	}


	/************************************************************************************************/


	void Context::AddAliasingBarrier(ResourceHandle before, ResourceHandle after)
	{
		DebugBreak();

		/*
		auto res = find(PendingBarriers,
			[&](Barrier& rhs) -> bool
			{
				return
					(rhs.Type            == Barrier::Type::Aliasing) &&
					((before != InvalidHandle &&  rhs.aliasedResources[0] == before) ||
					 (after != InvalidHandle &&   rhs.aliasedResources[1] == after));
			});

		if (std::end(PendingBarriers) == res)
		{
			Barrier barrier;
			barrier.Type                = Barrier::Type::Aliasing;
			barrier.aliasedResources[0] = before;
			barrier.aliasedResources[1] = after;

			PendingBarriers.push_back(barrier);
		}
		else
		{
			Barrier barrier;
			barrier.Type                = Barrier::Type::Aliasing;
			barrier.aliasedResources[0] = res->aliasedResources[0] == InvalidHandle ? before : res->aliasedResources[0];
			barrier.aliasedResources[1] = res->aliasedResources[1] == InvalidHandle ? after  : res->aliasedResources[1];

			PendingBarriers.push_back(barrier);
		}
		*/
	}


	/************************************************************************************************/


	void Context::AddUAVBarrier(ResourceHandle resource, uint32_t subresource, DeviceLayout layout, DeviceSyncPoint src, DeviceSyncPoint dst)
	{
		if(resource != FlexKit::InvalidHandle)
		{
			const auto dimension = renderSystem->GetTextureDimension(resource);
			Barrier barrier;
			barrier.resource		= resource;
			barrier.accessBefore	= DASUAV;
			barrier.accessAfter		= DASUAV;
			barrier.src				= src;
			barrier.dst				= dst;

			switch (dimension)
			{
			case TextureDimension::Buffer:
				barrier.type = BarrierType::Buffer;
				break;
			case TextureDimension::Texture2D:
			case TextureDimension::Texture2DArray:
			{
				barrier.type					= BarrierType::Texture;
				barrier.texture.layoutAfter		= layout;
				barrier.texture.layoutBefore	= layout;
			}	break;
			case TextureDimension::Texture1D:
			case TextureDimension::Texture3D:
			case TextureDimension::TextureCubeMap:
				DebugBreak();
			}

			pendingBarriers.push_back(barrier);
		}
		else
		{
			Barrier barrier;
			barrier.accessBefore	= DASUAV;
			barrier.accessAfter		= DASUAV;
			barrier.src				= src;
			barrier.dst				= dst;
			barrier.type			= BarrierType::Global;

			pendingBarriers.push_back(barrier);
		}
	}


	/************************************************************************************************/


	void Context::AddPresentBarrier(ResourceHandle Handle, DeviceAccessState Before)
	{
		DebugBreak();

		/*
		Barrier NewBarrier;
		NewBarrier.OldState		    = Before;
		NewBarrier.NewState		    = DeviceAccessState::DASPresent;
		NewBarrier.Type			    = Barrier::Type::Resource;
		NewBarrier.resourceHandle	= Handle;

		PendingBarriers.push_back(NewBarrier);
		*/
	}


	/************************************************************************************************/


	void Context::AddStreamOutBarrier(SOResourceHandle streamOut, DeviceAccessState Before, DeviceAccessState State)
	{
		DebugBreak();

		/*
		auto res = find(PendingBarriers, 
			[&](Barrier& rhs) -> bool
			{
				return
					rhs.Type		== Barrier::Type::StreamOut &&
					rhs.streamOut	== streamOut;
			});

		if (res != PendingBarriers.end()) {
			res->NewState = State;
		}
		else
		{
			Barrier NewBarrier;
			NewBarrier.OldState		= Before;
			NewBarrier.NewState		= State;
			NewBarrier.Type			= Barrier::Type::StreamOut;
			NewBarrier.streamOut	= streamOut;

			PendingBarriers.push_back(NewBarrier);
		}
		*/
	}


	/************************************************************************************************/

	void Context::AddGlobalBarrier(ResourceHandle resource, DeviceAccessState accessBefore, DeviceAccessState accessAfter, DeviceSyncPoint syncBefore, DeviceSyncPoint syncAfter)
	{
		Barrier barrier;
		barrier.accessBefore	= accessBefore;
		barrier.accessAfter		= accessAfter;
		barrier.src				= syncBefore;
		barrier.dst				= syncAfter;
		barrier.resource		= resource;
		barrier.type			= BarrierType::Global;

		auto res = find(pendingBarriers, [&](const auto& i) { return (i.resource == resource); });

		if (res == std::end(pendingBarriers))
			pendingBarriers.push_back(barrier);
		else
			(*res) = barrier;
	}


	/************************************************************************************************/


	void Context::AddTextureBarrier(ResourceHandle resource, DeviceAccessState accessBefore, DeviceAccessState accessAfter, DeviceLayout layoutBefore, DeviceLayout layoutAfter, DeviceSyncPoint syncBefore, DeviceSyncPoint syncAfter, BarrierSubResourceRange range)
	{
		auto res = find(pendingBarriers,
			[&](Barrier& rhs) -> bool { return (rhs.resource == resource); });

		if (res != pendingBarriers.end())
		{
			res->accessAfter			= accessAfter;
			res->texture.layoutAfter	= layoutAfter;
			res->dst					= syncAfter;
		}
		else
		{
			Barrier barrier;
			barrier.accessBefore	= accessBefore;
			barrier.accessAfter		= accessAfter;
			barrier.src				= syncBefore;
			barrier.dst				= syncAfter;
			barrier.resource		= resource;
			barrier.type			= BarrierType::Texture;

			barrier.texture.layoutBefore	= layoutBefore;
			barrier.texture.layoutAfter		= layoutAfter;
			barrier.texture.flags			= 0;

			pendingBarriers.push_back(barrier);
		}
	}


	/************************************************************************************************/


	void Context::AddBufferBarrier(ResourceHandle resource, DeviceAccessState accessBefore, DeviceAccessState accessAfter, DeviceSyncPoint syncBefore, DeviceSyncPoint syncAfter)
	{
		auto res = find(pendingBarriers,
			[&](Barrier& rhs) -> bool { return (rhs.resource == resource); });

		if (res != pendingBarriers.end())
		{
			res->accessAfter		= accessAfter;
		}
		else
		{
			Barrier barrier;
			barrier.accessBefore	= accessBefore;
			barrier.accessAfter		= accessAfter;
			barrier.src				= syncBefore;
			barrier.dst				= syncAfter;
			barrier.resource		= resource;
			barrier.type			= BarrierType::Buffer;

			pendingBarriers.push_back(barrier);
		}
	}


	/************************************************************************************************/


	void Context::AddBarriers(std::span<const Barrier> barriers)
	{
		for (auto& barrier : barriers)
		{
			if (barrier.resource != InvalidHandle)
				pendingBarriers.push_back(barrier);
		}
	}


	/************************************************************************************************/


	void Context::AddCopyResourceBarrier(ResourceHandle resource, DeviceAccessState Before, DeviceAccessState State)
	{
		DebugBreak();

		/*
		auto res = find(PendingBarriers,
			[&](Barrier& rhs) -> bool
			{
				return
					rhs.Type            == Barrier::Type::Resource &&
					rhs.resourceHandle  == resource;
			});

		if (res != PendingBarriers.end()) {
			res->NewState = State;
		}
		else
		{
			Barrier NewBarrier;
			NewBarrier.OldState         = Before;
			NewBarrier.NewState         = State;
			NewBarrier.Type             = Barrier::Type::Resource;
			NewBarrier.resourceHandle   = resource;

			PendingBarriers.push_back(NewBarrier);
		}
		*/
	}


	/************************************************************************************************/


	void Context::SetRootSignature(RootSigHandle rootSigHandle)
	{
		auto rootSig			= renderSystem->_GetRootSignature(rootSigHandle);
		CurrentRootSignature	= rootSig;
		DeviceContext->SetGraphicsRootSignature(*rootSig);
	}

	void Context::SetRootSignature(const IRootSignature* rootSig)
	{
		auto dxRootSig = static_cast<const RootSignature*>(rootSig);
		CurrentRootSignature	= dxRootSig;
		DeviceContext->SetGraphicsRootSignature(*dxRootSig);
	}


	/************************************************************************************************/


	void Context::SetComputeRootSignature(RootSigHandle rootSigHandle)
	{
		auto rootSig = renderSystem->_GetRootSignature(rootSigHandle);

		CurrentComputeRootSignature = rootSig;
		DeviceContext->SetComputeRootSignature(*rootSig);
	}


	void Context::SetComputeRootSignature(const IRootSignature* rootSig)
	{
		auto dxRootSig = static_cast<const RootSignature*>(rootSig);

		CurrentComputeRootSignature = dxRootSig;
		DeviceContext->SetComputeRootSignature(*dxRootSig);
	}


	/************************************************************************************************/


	/*
	void Context::SetPipelineState(ID3D12PipelineState* PSO)
	{
		FK_ASSERT(PSO);

		if (PSO == nullptr)
			__debugbreak();

		if (CurrentPipelineState == PSO)
			return;

		CurrentPipelineState = PSO;
		DeviceContext->SetPipelineState(PSO);
	}
	*/

	/************************************************************************************************/


	void Context::SetPipelineState(const IPipelineState* const PSO)
	{
		FK_ASSERT(PSO);

		const DXPipelineState* pso = static_cast<const DXPipelineState*>(PSO);

		if (PSO == nullptr)
			__debugbreak();

		if (CurrentPipelineState == pso->state)
			return;

		CurrentPipelineState = pso->state;
		DeviceContext->SetPipelineState(pso->state);
	}


	/************************************************************************************************/


	void Context::SetComputePipelineState(const PSOHandle stateHandle, iAllocator& temp)
	{
		auto [PSO, rootSignature]	= renderSystem->GetPSOAndRootSignature(stateHandle, temp);
		auto implRootSignature		= static_cast<const RootSignature*>(rootSignature);

		if (PSO == nullptr)
			__debugbreak();

		if (CurrentComputeRootSignature != rootSignature)
		{
			DeviceContext->SetComputeRootSignature(*implRootSignature);
			CurrentComputeRootSignature = implRootSignature;
		}

		if (auto implPSO = PSO->GetDevicePipeState(); CurrentPipelineState != implPSO)
		{
			CurrentPipelineState = implPSO;
			DeviceContext->SetPipelineState(implPSO);
		}
	}


	/************************************************************************************************/


	void Context::SetGraphicsPipelineState(const PSOHandle stateHandle, iAllocator& temp)
	{
		auto [PSO, rootSignature] = renderSystem->GetPSOAndRootSignature(stateHandle, temp);
		auto implRootSignature = static_cast<const RootSignature*>(rootSignature);

		if (PSO == nullptr)
		{
			FK_LOG_ERROR("Failed to load pipeline state! %u", stateHandle.to_uint());

#if _DEBUG
			__debugbreak();
#endif
		}

		if (CurrentRootSignature != implRootSignature)
		{
			DeviceContext->SetGraphicsRootSignature(*implRootSignature);
			CurrentRootSignature = implRootSignature;
		}

		if (auto implPSO = PSO->GetDevicePipeState(); CurrentPipelineState != implPSO)
		{
			CurrentPipelineState = implPSO;
			DeviceContext->SetPipelineState(implPSO);
		}
	}


	/************************************************************************************************/


	void Context::SetRenderTargets(const static_vector<ResourceHandle> RTs, bool enableDepthStencil, ResourceHandle depthStencil, const size_t MIPMapOffset)
	{
		static_vector<D3D12_CPU_DESCRIPTOR_HANDLE> RTV_CPU_HANDLES;


		bool WHsAllEqual = true;

		uint2 depthWH;
		uint2 textureWH;

		if (RTs.size() && enableDepthStencil)
		{
			depthWH = renderSystem->GetTextureWH(depthStencil);
			textureWH = renderSystem->GetTextureWH(RTs.front());

			WHsAllEqual = depthWH == textureWH;;
		}


		if (!MIPMapOffset && WHsAllEqual)
		{
			for (auto renderTarget : RTs) {
				auto res = std::find_if(
					renderTargetViews.begin(),
					renderTargetViews.end(),
					[&](RTV_View& view)
					{
						return view.resource == renderTarget;
					});

				if (res != renderTargetViews.end())
				{
					RTV_CPU_HANDLES.push_back(D3D12_CPU_DESCRIPTOR_HANDLE{ res->descriptor.V1 });
				}
				else
				{
					auto view = _ReserveRTV(1);
					PushRenderTarget(renderSystem, renderTarget, view);
					RTV_CPU_HANDLES.push_back(D3D12_CPU_DESCRIPTOR_HANDLE{ view.V1 });
					renderTargetViews.push_back({ renderTarget, view });
				}
			}
		}
		else
		{
			auto view = _ReserveRTV(RTs.size());
			
			for (auto& renderTarget : RTs)
			{
				auto WH = Min(depthWH, textureWH);

				RTV_CPU_HANDLES.push_back(D3D12_CPU_DESCRIPTOR_HANDLE{ view.V1 });
				view = PushRenderTarget(renderSystem, renderTarget, view, MIPMapOffset);
			}
		}

		auto DSV_CPU_HANDLE = D3D12_CPU_DESCRIPTOR_HANDLE{};

		if(enableDepthStencil)
		{
			if (auto res = std::find_if(
					depthStencilViews.begin(),
					depthStencilViews.end(),
					[&](RTV_View& view)
					{
						return view.resource == depthStencil;
					});
					res == depthStencilViews.end())
			{
				auto DSV = _ReserveDSV(1);
				PushDepthStencil(renderSystem, depthStencil, DSV);

				DSV_CPU_HANDLE = D3D12_CPU_DESCRIPTOR_HANDLE{ DSV.V1 };
			}
			else
				DSV_CPU_HANDLE = D3D12_CPU_DESCRIPTOR_HANDLE{ res->descriptor.V1 };
		}


		DeviceContext->OMSetRenderTargets(
			(UINT)RTV_CPU_HANDLES.size(),
			RTV_CPU_HANDLES.begin(),
			enableDepthStencil,
			enableDepthStencil ? &DSV_CPU_HANDLE : nullptr);
	}


	/************************************************************************************************/


	void Context::SetRenderTargets2(const static_vector<ResourceHandle> RTs, const size_t MIPMapOffset, const DepthStencilView_Options DSV)
	{
		static_vector<D3D12_CPU_DESCRIPTOR_HANDLE> RTV_CPU_HANDLES;

		if (!MIPMapOffset)
		{
			for (auto renderTarget : RTs) {
				auto res = std::find_if(
					renderTargetViews.begin(),
					renderTargetViews.end(),
					[&](RTV_View& view)
					{
						return view.resource == renderTarget;
					});

				if (res != renderTargetViews.end())
				{
					RTV_CPU_HANDLES.push_back(D3D12_CPU_DESCRIPTOR_HANDLE{ res->descriptor.V1 });
				}
				else
				{
					auto view = _ReserveRTV(1);
					PushRenderTarget(renderSystem, renderTarget, view);
					RTV_CPU_HANDLES.push_back(D3D12_CPU_DESCRIPTOR_HANDLE{ view.V1 });
					renderTargetViews.push_back({ renderTarget, view });
				}
			}
		}
		else
		{
			auto view = _ReserveRTV(RTs.size());
			
			for (auto& renderTarget : RTs)
			{
				RTV_CPU_HANDLES.push_back(D3D12_CPU_DESCRIPTOR_HANDLE{ view.V1 });
				view = PushRenderTarget(renderSystem, renderTarget, view, MIPMapOffset);
			}
		}
		
		auto DSV_CPU_HANDLE = D3D12_CPU_DESCRIPTOR_HANDLE{};

		const bool depthEnabled = DSV.depthStencil != InvalidHandle;

		if(depthEnabled)
		{
			auto descriptor = _GetDepthDesciptor(DSV.depthStencil);

			PushDepthStencilArray(renderSystem, DSV.depthStencil, DSV.ArraySliceOffset, DSV.MipOffset, descriptor, DSV.arraySize);

			DSV_CPU_HANDLE = D3D12_CPU_DESCRIPTOR_HANDLE{ descriptor.V1 };
		}

		DeviceContext->OMSetRenderTargets(
			(UINT)RTV_CPU_HANDLES.size(),
			RTV_CPU_HANDLES.begin(),
			depthEnabled,
			depthEnabled ? &DSV_CPU_HANDLE : nullptr);
	}


	/************************************************************************************************/

	/*
	void Context::SetViewports(static_vector<D3D12_VIEWPORT, 16> VPs)
	{
		DeviceContext->RSSetViewports((UINT)VPs.size(), VPs.begin());
	}


	void Context::SetViewports(std::span<const D3D12_VIEWPORT>	VPs)
	{
		DeviceContext->RSSetViewports((UINT)VPs.size(), VPs.data());
	}

	void Context::SetScissorRects(static_vector<D3D12_RECT, 16>	Rects)
	{
		DeviceContext->RSSetScissorRects((UINT)Rects.size(), Rects.begin());
	}


	void Context::SetScissorRects(std::span<const D3D12_RECT> rects)
	{
		DeviceContext->RSSetScissorRects((UINT)rects.size(), rects.data());
	}

	*/

	/************************************************************************************************/


	void Context::SetViewports(std::span<const Viewport> rects)
	{
		DeviceContext->RSSetViewports((UINT)rects.size(), (const D3D12_VIEWPORT*)rects.data());
	}


	/************************************************************************************************/


	void Context::SetScissorRects(std::span<const Rect>	rects)
	{
		DeviceContext->RSSetScissorRects((UINT)rects.size(), (const D3D12_RECT*)rects.data());
	}


	/************************************************************************************************/

	// Assumes setting each to fullscreen
	void Context::SetScissorAndViewports(static_vector<ResourceHandle, 16>	RenderTargets)
	{
		static_vector<Viewport, 16>	VPs;
		static_vector<Rect, 16>		Rects;

		for (auto RT : RenderTargets)
		{
			auto WH = renderSystem->GetTextureWH(RT);
			VPs.emplace_back	(0.0f, 0.0f, (float)WH[0], (float)WH[1], 0.0f, 1.0f);
			Rects.emplace_back	(0u, 0u, WH[0], WH[1]);
		}

		SetViewports(VPs);
		SetScissorRects(Rects);
	}

	/************************************************************************************************/


	void Context::SetScissorAndViewports2(static_vector<ResourceHandle, 16>	RenderTargets, const size_t MIPMapOffset)
	{
		static_vector<Viewport, 16>	VPs;
		static_vector<Rect, 16>		Rects;

		for (auto RT : RenderTargets)
		{
			auto WH = float2{ renderSystem->GetTextureWH(RT) } / std::pow(2.0f, (float)MIPMapOffset);
			VPs.emplace_back	(0.0f, 0.0f,	WH[0], WH[1], 0.0f, 1.0f);
			Rects.emplace_back	(0u, 0u, (uint32_t)WH[0], (uint32_t)WH[1]);
		}

		SetViewports(VPs);
		SetScissorRects(Rects);
	}


	/************************************************************************************************/


	void Context::QueueReadBack(ReadBackResourceHandle readBack)
	{
		queuedReadBacks.push_back(readBack);
	}

	void Context::QueueReadBack(ReadBackResourceHandle readBack, ReadBackEventHandler callback)
	{
		renderSystem->SetReadBackEvent(readBack, std::move(callback));
		QueueReadBack(readBack);
	}


	/************************************************************************************************/


	void Context::SetDepthStencil(ResourceHandle DS)
	{
		if (DS != InvalidHandle)
		{
			auto DSV = _ReserveDSV(1);
			PushDepthStencil(renderSystem, DS, DSV);
			DeviceContext->OMSetRenderTargets(
				(UINT)RenderTargetCount,
				RenderTargetCount ? &RTVPOSCPU : nullptr, true,
				DepthStencilEnabled ? &DSVPOSCPU : nullptr);
		}
		else 
			DepthStencilEnabled = false;
	}


	/************************************************************************************************/


	void Context::SetInputPrimitive(EInputPrimitive topology)
	{
		DeviceContext->IASetPrimitiveTopology((D3D12_PRIMITIVE_TOPOLOGY)topology);
	}


	/************************************************************************************************/


	void Context::SetGraphicsConstantValue(size_t idx, size_t valueCount, const void* data_ptr, size_t offset)
	{
		DeviceContext->SetGraphicsRoot32BitConstants((UINT)idx, (UINT)valueCount, data_ptr, (UINT)offset);
	}


	/************************************************************************************************/


	void Context::NullGraphicsConstantBufferView(size_t idx)
	{
		DeviceContext->SetGraphicsRootConstantBufferView((UINT)idx, 0);
	}


	/************************************************************************************************/


	void Context::SetGraphicsConstantBufferView(size_t idx, const ConstantBufferHandle CB, size_t Offset)
	{
		FK_ASSERT(!(Offset % 256), "Incorrect CB Offset!");

		DeviceContext->SetGraphicsRootConstantBufferView((UINT)idx, renderSystem->GetConstantBufferAddress(CB) + (UINT)Offset);
	}


	/************************************************************************************************/


	void Context::SetGraphicsConstantBufferView(size_t idx, const ConstantBuffer& CB)
	{
		DeviceContext->SetGraphicsRootConstantBufferView((UINT)idx, CB.Get()->GetGPUVirtualAddress());
	}


	/************************************************************************************************/


	void Context::SetGraphicsConstantBufferView(size_t idx, const ConstantBufferDataSet& CB)
	{
		DeviceContext->SetGraphicsRootConstantBufferView((UINT)idx, renderSystem->GetConstantBufferAddress(CB.Handle()) + CB.Offset());
	}


	/************************************************************************************************/


	void Context::SetGraphicsDescriptorTable(size_t idx, const IDescriptorHeap& IDH)
	{
		auto& DH = static_cast<const DescriptorHeap&>(IDH);
		DeviceContext->SetGraphicsRootDescriptorTable((UINT)idx, DH);
	}


	/************************************************************************************************/


	void Context::SetGraphicsConstantBufferView(size_t idx, DevicePointer devicePointer)
	{
		DeviceContext->SetGraphicsRootConstantBufferView((UINT)idx, devicePointer);
	}


	/************************************************************************************************/


	void Context::SetGraphicsDescriptorTable(size_t idx, const DescriptorRange& range)
	{
		DeviceContext->SetGraphicsRootDescriptorTable(
			(UINT)idx,
			D3D12_GPU_DESCRIPTOR_HANDLE{ range.begin.V2 });
	}


	/************************************************************************************************/


	void Context::SetGraphicsShaderResourceView(size_t idx, FrameBufferedResource* Resource, size_t Count, size_t ElementSize)
	{
#if USING(DEBUGGRAPHICS)
		if(debugCommandList)
			debugCommandList->AssertResourceState(Resource->Get(), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_GENERIC_READ);
#endif

		DeviceContext->SetGraphicsRootShaderResourceView((UINT)idx, Resource->Get()->GetGPUVirtualAddress());
	}


	/************************************************************************************************/

	void Context::SetGraphicsShaderResourceView(size_t idx, Texture2D& Texture)
	{
#if USING(DEBUGGRAPHICS)
		if (debugCommandList)
			debugCommandList->AssertResourceState(Texture, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_GENERIC_READ);
#endif

		DeviceContext->SetGraphicsRootShaderResourceView((UINT)idx, Texture->GetGPUVirtualAddress());
	}


	/************************************************************************************************/


	void Context::SetGraphicsShaderResourceView(size_t idx, ResourceHandle resource, size_t offset)
	{
#if USING(DEBUGGRAPHICS)
		if (resource != InvalidHandle && debugCommandList)
			debugCommandList->AssertResourceState(renderSystem->GetDeviceResource(resource), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_GENERIC_READ);
#endif

		if(resource != InvalidHandle)
			DeviceContext->SetGraphicsRootShaderResourceView((UINT)idx, renderSystem->GetDeviceResource(resource)->GetGPUVirtualAddress());
		else
			DeviceContext->SetGraphicsRootShaderResourceView((UINT)idx, { 0 });
	}


	/************************************************************************************************/


	void Context::SetGraphicsUnorderedAccessView(size_t idx, ResourceHandle UAVresource, size_t offset)
	{
		auto resource = renderSystem->GetDeviceResource(UAVresource);

#if USING(DEBUGGRAPHICS)
		if(debugCommandList)
			debugCommandList->AssertResourceState(resource, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
#endif

		DeviceContext->SetGraphicsRootUnorderedAccessView((UINT)idx, resource->GetGPUVirtualAddress() + offset);
	}


	/************************************************************************************************/


	void Context::SetComputeDescriptorTable(size_t idx)
	{
		DeviceContext->SetComputeRootDescriptorTable((UINT)idx, D3D12_GPU_DESCRIPTOR_HANDLE{ 0 });
	}


	void Context::SetComputeDescriptorTable(size_t idx, const IDescriptorHeap& IDH)
	{
		auto& DH = static_cast<const DescriptorHeap&>(IDH);
		DeviceContext->SetComputeRootDescriptorTable((UINT)idx, DH);
	}


	void Context::SetComputeDescriptorTable(size_t idx, const DescriptorRange& range)
	{
		DeviceContext->SetComputeRootDescriptorTable((UINT)idx, D3D12_GPU_DESCRIPTOR_HANDLE{ range.begin.V2 });
	}


	/************************************************************************************************/


	void Context::SetComputeConstantBufferView(size_t idx, const ConstantBufferHandle CB, size_t offset)
	{
#if USING(DEBUGGRAPHICS)
		auto resource = renderSystem->GetDeviceResource(CB);

		if (debugCommandList)
			debugCommandList->AssertResourceState(resource, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_GENERIC_READ);
#endif

		DeviceContext->SetGraphicsRootConstantBufferView((UINT)idx, renderSystem->GetConstantBufferAddress(CB) + offset);
	}


	/************************************************************************************************/


	void Context::SetComputeConstantBufferView(size_t idx, const ConstantBufferDataSet& CB)
	{
#if USING(DEBUGGRAPHICS)
		auto resource = renderSystem->GetDeviceResource(CB.Handle());

		if(debugCommandList)
			debugCommandList->AssertResourceState(resource, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_GENERIC_READ);
#endif

		DeviceContext->SetComputeRootConstantBufferView((UINT)idx, renderSystem->GetConstantBufferAddress(CB.Handle()) + CB.Offset());
	}


	/************************************************************************************************/


	void Context::SetComputeConstantBufferView(size_t idx, ResourceHandle resource, size_t offset, size_t bufferSize)
	{
		auto deviceResource     = renderSystem->GetDeviceResource(resource);
		auto gpuAddress         = deviceResource->GetGPUVirtualAddress();

#if USING(DEBUGGRAPHICS)
		if (debugCommandList)
			debugCommandList->AssertResourceState(deviceResource, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_COMMON);
#endif

		DeviceContext->SetComputeRootConstantBufferView((UINT)idx, gpuAddress + offset);
	}


	/************************************************************************************************/


	void Context::SetComputeConstantBufferView(size_t idx, DevicePointer pointer)
	{
		DeviceContext->SetComputeRootConstantBufferView((UINT)idx, pointer);
	}


	/************************************************************************************************/


	void Context::SetComputeShaderResourceView(size_t idx, Texture2D& Texture)
	{
#if USING(DEBUGGRAPHICS)
		if (debugCommandList)
			debugCommandList->AssertResourceState(Texture, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
#endif

		DeviceContext->SetComputeRootShaderResourceView((UINT)idx, Texture->GetGPUVirtualAddress());
	}


	/************************************************************************************************/


	void Context::SetComputeShaderResourceView(size_t idx, ResourceHandle resource, const size_t offset)
	{
#if USING(DEBUGGRAPHICS)
		if (debugCommandList)
			debugCommandList->AssertResourceState(renderSystem->GetDeviceResource(resource), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
#endif
		if(resource != InvalidHandle)
			DeviceContext->SetComputeRootShaderResourceView((UINT)idx, renderSystem->GetDeviceResource(resource)->GetGPUVirtualAddress() + offset);
		else
			DeviceContext->SetComputeRootShaderResourceView((UINT)idx, 0);
	}


	/************************************************************************************************/


	void Context::SetComputeUnorderedAccessView(size_t idx, ResourceHandle UAVresource, size_t offset)
	{
		auto resource = renderSystem->GetDeviceResource(UAVresource);

#if USING(DEBUGGRAPHICS)
		if(debugCommandList)
			debugCommandList->AssertResourceState(resource, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
#endif
		if (UAVresource != InvalidHandle)
			DeviceContext->SetComputeRootUnorderedAccessView((UINT)idx, resource->GetGPUVirtualAddress() + offset);
		else
			DeviceContext->SetComputeRootUnorderedAccessView((UINT)idx, 0);
	}


	/************************************************************************************************/


	void Context::SetComputeConstantValue(size_t idx, size_t valueCount, const void* data_ptr, size_t offset)
	{
		DeviceContext->SetComputeRoot32BitConstants((UINT)idx, (UINT)valueCount, data_ptr, (UINT)offset);
	}


	/************************************************************************************************/


	void Context::BeginQuery(QueryHandle query, size_t idx)
	{
		if (query == InvalidHandle)
			return;

		auto resource	= renderSystem->Queries.GetDeviceObject(query);
		auto queryType	= renderSystem->Queries.GetType(query);

		DeviceContext->BeginQuery(resource, queryType, (UINT)idx);
	}


	void Context::EndQuery(QueryHandle query, size_t idx)
	{
		if (query == InvalidHandle)
			return;

		auto resource	= renderSystem->Queries.GetDeviceObject(query);
		auto queryType	= renderSystem->Queries.GetType(query);

		DeviceContext->EndQuery(resource, queryType, (UINT)idx);
	}


	void Context::TimeStamp(QueryHandle query, size_t idx)
	{
		if (query == InvalidHandle)
			return;

		auto resource   = renderSystem->Queries.GetDeviceObject(query);
		auto queryType  = renderSystem->Queries.GetType(query);

		DeviceContext->EndQuery(resource, queryType, (UINT)idx);
	}


	/************************************************************************************************/


	void Context::SetMarker_DEBUG(const char* str)
	{
#if USING(AFTERMATH)
		GFSDK_Aftermath_GetShaderHash;
		AFTERMATH_context;
#endif
	}


	/************************************************************************************************/


	void Context::BeginEvent_DEBUG(const char* str)
	{
#if USING(PIX)
		wchar_t temp[64];
		mbstowcs(temp, str, 64);

		PIXBeginEvent(DeviceContext, PIX_COLOR_INDEX(rand() % 255), temp);
#endif
	}


	/************************************************************************************************/


	void Context::EndEvent_DEBUG()
	{
#if USING(PIX)
		PIXEndEvent(DeviceContext);
#endif
	}


	/************************************************************************************************/


	void Context::CopyResource(ResourceHandle dest, ResourceHandle src)
	{
		FlushBarriers();

		DeviceContext->CopyResource(
			renderSystem->GetDeviceResource(dest),
			renderSystem->GetDeviceResource(src));
	}


	/************************************************************************************************/


	void Context::CopyTextureRegion(
		ID3D12Resource*		destination,
		size_t				subResourceIdx,
		uint3				XYZ,
		UploadReservation	source,
		uint2				WH,
		DeviceFormat		format)
	{
		FlushBarriers();

		const auto		deviceFormat	= TextureFormat2DXGIFormat(format);
		const size_t	formatSize		= GetFormatElementSize(deviceFormat);
		const bool		BCformat		= IsDDS(format);
		const size_t	rowPitch		= AlignedSize(BCformat ? formatSize * WH[0] / 4 : formatSize * WH[0]);

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT SubRegion;
		SubRegion.Footprint.Depth		= 1;
		SubRegion.Footprint.Format		= deviceFormat;
		SubRegion.Footprint.RowPitch	= (UINT)rowPitch;
		SubRegion.Footprint.Width		= WH[0];
		SubRegion.Footprint.Height		= WH[1];
		SubRegion.Offset				= source.offset;

		auto destinationLocation	= CD3DX12_TEXTURE_COPY_LOCATION(destination, (UINT)subResourceIdx);
		auto sourceLocation			= CD3DX12_TEXTURE_COPY_LOCATION(source.resource, SubRegion);

		DeviceContext->CopyTextureRegion(
			&destinationLocation,
			XYZ[0], XYZ[1], XYZ[2],
			&sourceLocation,
			nullptr);
	}

	/************************************************************************************************/

	void Context::CopyTextureRegion(
		ResourceHandle		dest,
		size_t				subResourceIdx,
		uint3				XYZ,
		UploadReservation	source)
	{
		FlushBarriers();

		auto destination				= renderSystem->GetDeviceResource(dest);
		const auto		WH				= renderSystem->GetTextureWH(dest);
		const auto		deviceFormat	= renderSystem->GetTextureDeviceFormat(dest);
		const size_t	formatSize		= GetFormatElementSize(deviceFormat);
		const bool		BCformat		= IsDDS(renderSystem->GetTextureFormat(dest));
		const size_t	rowPitch		= AlignedSize(BCformat ? formatSize * WH[0] / 4 : formatSize * WH[0]);

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT SubRegion;
		SubRegion.Footprint.Depth		= 1;
		SubRegion.Footprint.Format		= deviceFormat;
		SubRegion.Footprint.RowPitch	= (UINT)rowPitch;
		SubRegion.Footprint.Width		= WH[0];
		SubRegion.Footprint.Height		= WH[1];
		SubRegion.Offset				= source.offset;

		auto destinationLocation	= CD3DX12_TEXTURE_COPY_LOCATION(destination, (UINT)subResourceIdx);
		auto sourceLocation			= CD3DX12_TEXTURE_COPY_LOCATION(source.resource, SubRegion);

		DeviceContext->CopyTextureRegion(
			&destinationLocation,
			XYZ[0], XYZ[1], XYZ[2],
			&sourceLocation,
			nullptr);
	}


	/************************************************************************************************/


	void Context::CopyTile(ID3D12Resource* dest, const uint3 destTile, const size_t tileOffset, const UploadReservation src)
	{
		FlushBarriers();

		D3D12_TILED_RESOURCE_COORDINATE coordinate;
		coordinate.X			= (UINT)destTile[0];
		coordinate.Y			= (UINT)destTile[1];
		coordinate.Z			= (UINT)0;
		coordinate.Subresource	= (UINT)destTile[2];
		
		D3D12_TILE_REGION_SIZE regionSize;
		regionSize.NumTiles		= 1;
		regionSize.UseBox		= false;
		regionSize.Width		= 1;
		regionSize.Height		= 1;
		regionSize.Depth		= 1;

		DeviceContext->CopyTiles(
			dest,
			&coordinate,
			&regionSize,
			src.resource,
			src.offset,
			D3D12_TILE_COPY_FLAG_LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE);
	}


	/************************************************************************************************/


	void Context::CopyTile(
		ResourceHandle			dest,
		const uint3				destTile,
		const size_t			tileOffset,
		const UploadReservation src)
	{
		FlushBarriers();

		auto resource_ptr	= renderSystem->GetDeviceResource(dest);

		D3D12_TILED_RESOURCE_COORDINATE coordinate;
		coordinate.X			= (UINT)destTile[0];
		coordinate.Y			= (UINT)destTile[1];
		coordinate.Z			= (UINT)0;
		coordinate.Subresource	= (UINT)destTile[2];
		
		D3D12_TILE_REGION_SIZE regionSize;
		regionSize.NumTiles		= 1;
		regionSize.UseBox		= false;
		regionSize.Width		= 1;
		regionSize.Height		= 1;
		regionSize.Depth		= 1;

		DeviceContext->CopyTiles(
			resource_ptr,
			&coordinate,
			&regionSize,
			src.resource,
			src.offset,
			D3D12_TILE_COPY_FLAG_LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE);
	}


	/************************************************************************************************/


	void Context::CopyBufferRegion(
		ResourceHandle	destination,
		ResourceHandle	source,
		size_t			size,
		size_t			destinationOffset,
		size_t			sourceOffset)
	{
		FlushBarriers();

		DeviceContext->CopyBufferRegion(
			renderSystem->GetDeviceResource(destination),
			destinationOffset,
			renderSystem->GetDeviceResource(source),
			sourceOffset,
			size);
	}

	void Context::CopyBufferRegion(
		ResourceHandle	destination,
		ID3D12Resource* source,
		size_t			size,
		size_t			destinationOffset,
		size_t			sourceOffset
	)
	{
		FlushBarriers();

		DeviceContext->CopyBufferRegion(
			renderSystem->GetDeviceResource(destination),
			destinationOffset,
			source,
			sourceOffset,
			size);
	}

	void Context::CopyBufferRegion(
		ID3D12Resource* destination,
		ResourceHandle	source,
		size_t			size,
		size_t			destinationOffset,
		size_t			sourceOffset)
	{
		FlushBarriers();

		DeviceContext->CopyBufferRegion(
			destination,
			destinationOffset,
			renderSystem->GetDeviceResource(source),
			sourceOffset,
			size);
	}

	void Context::CopyBufferRegion(
		ID3D12Resource*	destination,
		ID3D12Resource* source,
		size_t			size,
		size_t			destinationOffset,
		size_t			sourceOffset)
	{
		FlushBarriers();

		DeviceContext->CopyBufferRegion(
			destination,
			destinationOffset,
			source,
			sourceOffset,
			size);
	}


	/************************************************************************************************/


	void Context::ImmediateWrite(
		static_vector<ResourceHandle>		    handles,
		static_vector<size_t>					value,
		static_vector<DeviceAccessState>		currentStates,
		static_vector<DeviceAccessState>		finalStates)
	{
		DebugBreak();

		FK_ASSERT(handles.size() == currentStates.size(), "Invalid argument!");

		/*
		typedef struct D3D12_WRITEBUFFERIMMEDIATE_PARAMETER
		{
		D3D12_GPU_VIRTUAL_ADDRESS Dest;
		UINT32 Value;
		} 	D3D12_WRITEBUFFERIMMEDIATE_PARAMETER;
		*/
		/*
		DeviceAccessState prevState		= DeviceAccessState::DASERROR;
		ID3D12Resource*		prevResource	= nullptr;

		for (size_t itr = 0; itr < handles.size(); ++itr)
		{
			auto resource	= renderSystem->GetDeviceResource(handles[itr]);
			auto state		= currentStates[itr];

			if(prevResource != resource && prevState != state)
				_AddBarrier(resource, state, DeviceAccessState::DASCopyDest);

			prevResource	= resource;
			prevState		= state;
		}

		FlushBarriers();

		for (size_t itr = 0; itr < handles.size(); ++itr)
		{
			auto resource = renderSystem->GetDeviceResource(handles[itr]);

			D3D12_WRITEBUFFERIMMEDIATE_PARAMETER params[] = {
				{resource->GetGPUVirtualAddress() + 0, 0u },
			};

			D3D12_WRITEBUFFERIMMEDIATE_MODE modes[] = {
				D3D12_WRITEBUFFERIMMEDIATE_MODE_MARKER_OUT
			};

			DeviceContext->WriteBufferImmediate(1, params, nullptr);
		}

		prevState		= DeviceAccessState::DASERROR;
		prevResource	= nullptr;

		for (size_t itr = 0; itr < handles.size(); ++itr)
		{
			auto resource	= renderSystem->GetDeviceResource(handles[itr]);
			auto state		= currentStates[itr];

			if (prevResource != resource && prevState != state)
				_AddBarrier(resource, DeviceAccessState::DASCopyDest, state);

			prevResource	= resource;
			prevState		= prevState;
		}
		*/
	}


	/************************************************************************************************/


	// Requires SO resources to be in DeviceAccessState::DRS::STREAMOUTCLEAR!
	/*
	void Context::ClearSOCounters(static_vector<SOResourceHandle> handles)
	{
		
		//typedef struct D3D12_WRITEBUFFERIMMEDIATE_PARAMETER
		//{
		//D3D12_GPU_VIRTUAL_ADDRESS Dest;
		//UINT32 Value;
		//} 	D3D12_WRITEBUFFERIMMEDIATE_PARAMETER;
		

		static_vector<ID3D12Resource*>		sources;
		static_vector<size_t>				sourceOffset;
		static_vector<ID3D12Resource*>		destinations;
		static_vector<size_t>				destinationOffset;
		static_vector<size_t>				copySize;
		static_vector<DeviceAccessState>	currentSOStates;
		static_vector<DeviceAccessState>	finalStates;

		for (auto& s : handles)
			sources.push_back(nullptr);

		for (auto& s : handles)
			sourceOffset.push_back(0);

		for (auto& s : handles)
			destinations.push_back(renderSystem->GetSOCounterResource(s));

		for (auto& s : handles)
			destinationOffset.push_back(0);

		for (auto& s : destinations)
			copySize.push_back(16);

		for (auto& s : handles)
			currentSOStates.push_back(DeviceAccessState::DASCopyDest);

		for (auto& s : handles)
			finalStates.push_back(DeviceAccessState::DASCopyDest);


		CopyBufferRegion(
			sources,			// sources
			sourceOffset,		// source offsets
			destinations,		// destinations
			destinationOffset,  // destination offsets
			copySize,			// copy sizes
			currentSOStates,	// source initial state
			finalStates);		// source final	state
	}
	*/


	/************************************************************************************************/


	void Context::CopyUInt64(
		static_vector<ID3D12Resource*>			sources,
		static_vector<DeviceAccessState>		sourceState,
		static_vector<size_t>					sourceOffsets,
		static_vector<ID3D12Resource*>			destinations,
		static_vector<DeviceAccessState>		destinationState,
		static_vector<size_t>					destinationOffset)
	{
		DebugBreak();

		FK_ASSERT(sources.size()		== sourceState.size(),			"Invalid argument!");
		FK_ASSERT(sources.size()		== sourceOffsets.size(),		"Invalid argument!");
		FK_ASSERT(destinations.size()	== destinationState.size(),		"Invalid argument!");
		FK_ASSERT(destinations.size()	== destinationOffset.size(),	"Invalid argument!");
		FK_ASSERT(sources.size()		== destinations.size(),			"Invalid argument!");

		/*
		typedef struct D3D12_WRITEBUFFERIMMEDIATE_PARAMETER
		{
		D3D12_GPU_VIRTUAL_ADDRESS Dest;
		UINT32 Value;
		} 	D3D12_WRITEBUFFERIMMEDIATE_PARAMETER;
		*/
		/*

		// transition source resources
		for (size_t itr = 0; itr < sources.size(); ++itr) 
		{
			auto resource	= sources[itr];
			auto state		= sourceState[itr];
			_AddBarrier(resource, state, DeviceAccessState::DASCopySrc);
		}

		for (size_t itr = 0; itr < sources.size(); ++itr)
		{
			auto resource	= destinations[itr];
			auto state		= destinationState[itr];
			_AddBarrier(resource, state, DeviceAccessState::DASCopyDest);
		}

		FlushBarriers();

		for (size_t itr = 0; itr < sources.size(); ++itr)
		{
			DeviceContext->AtomicCopyBufferUINT64(
				destinations[itr],
				destinationOffset[itr], 
				sources[itr], 
				sourceOffsets[itr], 
				0, 
				nullptr, 
				nullptr);
		}

		for (size_t itr = 0; itr < sources.size(); ++itr) 
		{
			auto resource	= sources[itr];
			auto state		= sourceState[itr];
			_AddBarrier(resource, DeviceAccessState::DASCopySrc, state);
		}

		for (size_t itr = 0; itr < sources.size(); ++itr)
		{
			auto resource	= destinations[itr];
			auto state		= destinationState[itr];
			_AddBarrier(resource, DeviceAccessState::DASCopyDest, state);
		}
		*/
	}


	/************************************************************************************************/


	void Context::AddIndexBuffer(TriMesh* Mesh, uint32_t lod)
	{
		const size_t	IBIndex		= Mesh->lods[lod].GetIndexBufferIndex();
		const size_t	IndexCount	= Mesh->lods[lod].GetIndexCount();

		D3D12_INDEX_BUFFER_VIEW		IndexView;
		IndexView.BufferLocation	= GetBuffer(Mesh, lod, IBIndex)->GetGPUVirtualAddress();
		IndexView.Format			= DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
		IndexView.SizeInBytes		= (UINT)IndexCount * 4;

		DeviceContext->IASetIndexBuffer(&IndexView);
	}


	/************************************************************************************************/


	void Context::SetIndexBuffer(VertexBufferEntry buffer, DeviceFormat format)
	{
		D3D12_INDEX_BUFFER_VIEW		IndexView;
		IndexView.BufferLocation    = renderSystem->GetVertexBufferAddress(buffer.VertexBuffer) + buffer.Offset;
		IndexView.Format            = TextureFormat2DXGIFormat(format);
		IndexView.SizeInBytes       = (UINT)(renderSystem->GetVertexBufferSize(buffer.VertexBuffer) - buffer.Offset);

		DeviceContext->IASetIndexBuffer(&IndexView);
	}


	/************************************************************************************************/


	void Context::SetIndexBuffer(ResourceHandle resource, DeviceFormat format)
	{
		D3D12_INDEX_BUFFER_VIEW		IndexView;
		IndexView.BufferLocation    = renderSystem->GetDeviceResource(resource)->GetGPUVirtualAddress();
		IndexView.Format            = TextureFormat2DXGIFormat(format);
		IndexView.SizeInBytes       = (UINT)(renderSystem->GetResourceSize(resource));

		DeviceContext->IASetIndexBuffer(&IndexView);
	}


	/************************************************************************************************/


	void Context::AddVertexBuffers(TriMesh* mesh, uint32_t lod, const std::initializer_list<VERTEXBUFFER_TYPE>& buffers, VertexBufferList* instanceBuffers)
	{
		AddVertexBuffers(mesh, lod, std::span{ buffers.begin(), buffers.size() }, instanceBuffers);
	}


	/************************************************************************************************/


	void Context::AddVertexBuffers(TriMesh* mesh, uint32_t lod, const std::span<const VERTEXBUFFER_TYPE> buffers, VertexBufferList* instanceBuffers)
	{
		static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;

		for (auto& I : buffers)
			FK_ASSERT(AddVertexBuffer(I, mesh, lod, VBViews));

		if (instanceBuffers)
		{
			for (auto& IB : *instanceBuffers)
			{
				VBViews.push_back({
					renderSystem->GetVertexBufferAddress(IB.VertexBuffer) + IB.Offset,
					(UINT)renderSystem->GetVertexBufferSize(IB.VertexBuffer) - IB.Offset,
					IB.Stride });
			}
		}

		DeviceContext->IASetVertexBuffers(0, (UINT)VBViews.size(), VBViews.begin());
	}


	/************************************************************************************************/


	void Context::SetVertexBuffers(const std::initializer_list<VertexBufferEntry>& list)
	{
		SetVertexBuffers(std::span{ list.begin(), list.end() });
	}


	/************************************************************************************************/


	void Context::SetVertexBuffers(const std::span<const VertexBufferEntry> list)
	{
		static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;
		for (auto& VB : list)
		{
			/*
			typedef struct D3D12_VERTEX_BUFFER_VIEW
			{
			D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
			UINT SizeInBytes;
			UINT StrideInBytes;
			} 	D3D12_VERTEX_BUFFER_VIEW;
			*/

			VBViews.push_back({
				renderSystem->GetVertexBufferAddress(VB.VertexBuffer) + VB.Offset,
				(UINT)renderSystem->GetVertexBufferSize(VB.VertexBuffer) - +VB.Offset,
				VB.Stride});
		}

		DeviceContext->IASetVertexBuffers(0, (UINT)VBViews.size(), VBViews.begin());
	}


	/************************************************************************************************/


	void Context::SetVertexBuffers(const std::initializer_list<VertexBufferResource>& span)
	{
		SetVertexBuffers(std::span{ span.begin(), span.end() });
	}


	/************************************************************************************************/


	void Context::SetVertexBuffers(const std::span<const VertexBufferResource> list)
	{
		static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;
		for (auto& VB : list)
		{
			/*
			typedef struct D3D12_VERTEX_BUFFER_VIEW
			{
			D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
			UINT SizeInBytes;
			UINT StrideInBytes;
			} 	D3D12_VERTEX_BUFFER_VIEW;
			*/

			VBViews.push_back({
				renderSystem->GetDeviceResource(VB.resource)->GetGPUVirtualAddress() + VB.offset,
				(UINT)renderSystem->GetResourceSize(VB.resource) - VB.offset,
				VB.stride});
		}

		DeviceContext->IASetVertexBuffers(0, (UINT)VBViews.size(), VBViews.begin());
	}


	/************************************************************************************************/


	void Context::SetVertexBuffers2(const std::initializer_list<D3D12_VERTEX_BUFFER_VIEW>& list)
	{
		SetVertexBuffers2(std::span(list.begin(), list.end()));
	}


	/************************************************************************************************/


	void Context::SetVertexBuffers2(const std::span<const D3D12_VERTEX_BUFFER_VIEW> list)
	{
		DeviceContext->IASetVertexBuffers(0, (UINT)list.size(), list.data());
	}


	/************************************************************************************************/


	void Context::SetVertexBuffers2(const std::span<const VBView>	views, uint32_t offset)
	{
		D3D12_VERTEX_BUFFER_VIEW apiViews[16];
		for (auto&& [idx, view] : enumerate(views))
		{
			apiViews[idx] = D3D12_VERTEX_BUFFER_VIEW{
				.BufferLocation	= view.buffer,
				.SizeInBytes	= view.size,
				.StrideInBytes	= view.stride,
			};
		}

		DeviceContext->IASetVertexBuffers(0, (UINT)views.size(), apiViews);
	}


	/************************************************************************************************/


	void Context::SetSOTargets(static_vector<D3D12_STREAM_OUTPUT_BUFFER_VIEW, 4> SOViews)
	{
		DeviceContext->SOSetTargets(0, (UINT)SOViews.size(), SOViews.begin());
	}


	/************************************************************************************************/


	void Context::ClearDepthBuffer(ResourceHandle depthBuffer, float clearDepth)
	{
		UpdateResourceStates();

		auto descriptor = _GetDepthDesciptor(depthBuffer);
		PushDepthStencilArray(renderSystem, depthBuffer, 0, 0, descriptor);

		DeviceContext->ClearDepthStencilView(
			D3D12_CPU_DESCRIPTOR_HANDLE{ descriptor.V1 },
			D3D12_CLEAR_FLAG_DEPTH, clearDepth, 0, 0, nullptr);

		renderSystem->Textures.MarkRTUsed(depthBuffer);
	}


	/************************************************************************************************/


	void Context::ClearRenderTarget(ResourceHandle renderTarget, float4 clearColor)
	{
		UpdateResourceStates();

		D3D12_CPU_DESCRIPTOR_HANDLE RTV_CPU_HANDLES{ 0 };

		auto res = std::find_if(
			renderTargetViews.begin(),
			renderTargetViews.end(),
			[&](RTV_View& view)
			{
				return view.resource == renderTarget;
			});

		if (res != renderTargetViews.end())
		{
			RTV_CPU_HANDLES = D3D12_CPU_DESCRIPTOR_HANDLE{ res->descriptor.GetByType<CPUDescriptorHandle>() };
		}
		else
		{
			auto view = _ReserveRTV(1);
			PushRenderTarget(renderSystem, renderTarget, view);
			RTV_CPU_HANDLES = D3D12_CPU_DESCRIPTOR_HANDLE{ view.V1 };
			renderTargetViews.push_back({ renderTarget, view });
		}

		DeviceContext->ClearRenderTargetView(RTV_CPU_HANDLES, clearColor, 0, nullptr);

		renderSystem->Textures.MarkRTUsed(renderTarget);
	}


	/************************************************************************************************/


	void Context::ClearUAVTextureFloat(ResourceHandle UAV, float4 clearColor)
	{
		auto viewCPU    = _ReserveSRVLocal(1);
		auto viewGPU    = _ReserveSRV(1).value();
		auto resource   = renderSystem->GetDeviceResource(UAV);

		Texture2D tex{
			renderSystem->GetDeviceResource(UAV),
			renderSystem->GetTextureWH(UAV),
			renderSystem->GetTextureMipCount(UAV),
			renderSystem->GetTextureDeviceFormat(UAV),
		};

		PushUAV2DToDescHeap(
			renderSystem,
			tex,
			viewCPU);

		PushUAV2DToDescHeap(
			renderSystem,
			tex,
			viewGPU);

		FlushBarriers();

		DeviceContext->ClearUnorderedAccessViewFloat(
			D3D12_GPU_DESCRIPTOR_HANDLE{ viewGPU.V2 },
			D3D12_CPU_DESCRIPTOR_HANDLE{ viewCPU.V1 },
			resource, clearColor, 0, nullptr);
	}

	/************************************************************************************************/


	void Context::ClearUAVTextureUint(ResourceHandle UAV, uint4 clearColor)
	{
		auto CPUview	= _ReserveSRVLocal(1);
		auto GPUview	= _ReserveSRV(1);
		auto resource	= renderSystem->GetDeviceResource(UAV);

		FK_ASSERT(GPUview.has_value() != false, "Failed to allocated descriptor");

		Texture2D tex{
			renderSystem->GetDeviceResource(UAV),
			renderSystem->GetTextureWH(UAV),
			renderSystem->GetTextureMipCount(UAV),
			renderSystem->GetTextureDeviceFormat(UAV),
		};

		PushUAV2DToDescHeap(
			renderSystem,
			tex,
			CPUview);

		PushUAV2DToDescHeap(
			renderSystem,
			tex,
			GPUview.value());

		const auto CPUHandle = D3D12_CPU_DESCRIPTOR_HANDLE{ CPUview.Get<0>() };
		const auto GPUHandle = D3D12_GPU_DESCRIPTOR_HANDLE{ GPUview.value().Get<1>() };

		FlushBarriers();

		DeviceContext->ClearUnorderedAccessViewUint(GPUHandle, CPUHandle, resource, (UINT*)&clearColor, 0, nullptr);
	}


	/************************************************************************************************/


	void Context::ClearUAV(ResourceHandle resource, uint4 clearColor)
	{
		const auto view				= _ReserveSRVLocal(1);
		const auto deviceResource	= renderSystem->GetDeviceResource(resource);
		const auto deviceFormat		= renderSystem->GetTextureDeviceFormat(resource);

		PushUAV1DToDescHeap(renderSystem, deviceResource, deviceFormat, 0, view);

		const auto CPUHandle = D3D12_CPU_DESCRIPTOR_HANDLE{ view.Get<0>() };
		const auto GPUHandle = D3D12_GPU_DESCRIPTOR_HANDLE{ view.Get<1>() };

		FlushBarriers();

		DeviceContext->ClearUnorderedAccessViewUint(GPUHandle, CPUHandle, deviceResource, clearColor, 0, 0);
	}


	/************************************************************************************************/


	void Context::ClearUAVBuffer(ResourceHandle UAV, uint4 clearColor)
	{
		BeginEvent_DEBUG("ClearUAVBuffer");

		UpdateResourceStates();

		static auto PSO = static_cast<const DXPipelineState*>(renderSystem->GetPSO(CLEARBUFFERPSO, *renderSystem->Memory));
		DeviceContext->SetComputeRootSignature(renderSystem->Library(ROOTLIBRARYSIG::ClearBuffer)->GetAPIObject());
		DeviceContext->SetPipelineState(PSO->state);
		DeviceContext->SetComputeRoot32BitConstants(0, 4, &clearColor, 0);
		DeviceContext->SetComputeRootUnorderedAccessView(1, renderSystem->GetDeviceResource(UAV)->GetGPUVirtualAddress());

		auto resourceSize = renderSystem->GetResourceSize(UAV);

		uint2 range{ 0, resourceSize / 16 };
		DeviceContext->SetComputeRoot32BitConstants(0, 2, &range, 4);

		DeviceContext->Dispatch(UINT(ceil(resourceSize / 1024.0f)), 1, 1);

		if(CurrentComputeRootSignature)
			DeviceContext->SetComputeRootSignature(*CurrentComputeRootSignature);

		if(CurrentPipelineState)
			DeviceContext->SetPipelineState(CurrentPipelineState);

		EndEvent_DEBUG();
	}


	/************************************************************************************************/


	void Context::ClearUAVBufferRange(ResourceHandle UAV, uint begin, uint end, uint4 clearColor)
	{
		FK_ASSERT(begin % 16 == 0, "Begin must be 16-byte aligned");
		FK_ASSERT(end % 16 == 0, "End must be 16-byte aligned");

		BeginEvent_DEBUG("ClearUAVBuffer");

		UpdateResourceStates();

		end = Min((uint32_t)renderSystem->GetResourceSize(UAV), end);
		uint2 range{ begin / 16, end / 16};

		auto PSO = static_cast<const DXPipelineState*>(renderSystem->GetPSO(CLEARBUFFERPSO, *renderSystem->Memory));
		DeviceContext->SetComputeRootSignature(renderSystem->Library(ROOTLIBRARYSIG::ClearBuffer)->GetAPIObject());
		DeviceContext->SetPipelineState(PSO->state);
		DeviceContext->SetComputeRoot32BitConstants(0, 4, &clearColor, 0);
		DeviceContext->SetComputeRoot32BitConstants(0, 2, &range, 4);
		DeviceContext->SetComputeRootUnorderedAccessView(1, renderSystem->GetDeviceResource(UAV)->GetGPUVirtualAddress());

		auto resourceSize = renderSystem->GetResourceSize(UAV);
		DeviceContext->Dispatch(UINT(ceil(Min(resourceSize, end - begin) / 1024.0f)), 1, 1);

		if(CurrentComputeRootSignature)
			DeviceContext->SetComputeRootSignature(*CurrentComputeRootSignature);

		if(CurrentPipelineState)
			DeviceContext->SetPipelineState(CurrentPipelineState);

		EndEvent_DEBUG();
	}


	/************************************************************************************************/


	void Context::ResolveQuery(QueryHandle query, size_t begin, size_t end, ResourceHandle destination, size_t destOffset)
	{
		if (query == InvalidHandle)
			return;

		auto res			= renderSystem->GetDeviceResource(destination);
		auto type			= renderSystem->Queries.GetType(query);
		auto queryResource	= renderSystem->Queries.GetDeviceObject(query);

		UpdateResourceStates();

		DeviceContext->ResolveQueryData(queryResource, type, (UINT)begin, (UINT)(end - begin), res, (UINT)destOffset);
	}


	/************************************************************************************************/


	void Context::ResolveQuery(QueryHandle query, size_t begin, size_t end, ID3D12Resource* destination, size_t destOffset)
	{
		if (query == InvalidHandle)
			return;

		auto type			= renderSystem->Queries.GetType(query);
		auto queryResource	= renderSystem->Queries.GetDeviceObject(query);

		UpdateResourceStates();

		DeviceContext->ResolveQueryData(queryResource, type, (UINT)begin, (UINT)(end - begin), destination, (UINT)destOffset);
	}


	/************************************************************************************************/


	void Context::ExecuteIndirect(ResourceHandle args, const IndirectLayout& layout, size_t argumentBufferOffset, size_t executionCount)
	{
		UpdateResourceStates();

		DeviceContext->ExecuteIndirect(
			layout.signature, 
			(UINT)Min(layout.entries.size(), executionCount),
			renderSystem->GetDeviceResource(args),
			(UINT)argumentBufferOffset,
			nullptr, 
			0);
	}


	void Context::Draw(const size_t VertexCount, const size_t BaseVertex, const size_t baseIndex)
	{
		UpdateResourceStates();
		DeviceContext->DrawInstanced((UINT)VertexCount, 1, (UINT)BaseVertex, (UINT)baseIndex);
	}


	void Context::DrawInstanced(const size_t vertexCount, const size_t baseVertex, const size_t instanceCount, const size_t instanceOffset )
	{
		UpdateResourceStates();
		DeviceContext->DrawInstanced((UINT)vertexCount, (UINT)instanceCount, (UINT)baseVertex, (UINT)instanceOffset);
	}


	void Context::DrawIndexed(const size_t IndexCount, const size_t IndexOffet, const size_t BaseVertex)
	{
		UpdateResourceStates();
		DeviceContext->DrawIndexedInstanced((UINT)IndexCount, 1, (UINT)IndexOffet, (UINT)BaseVertex, 0);
	}


	void Context::DrawIndexedInstanced(
		const size_t IndexCount, const size_t IndexOffet, 
		const size_t BaseVertex, const size_t InstanceCount, 
		const size_t InstanceOffset)
	{
		UpdateResourceStates();
		DeviceContext->DrawIndexedInstanced(
			(UINT)IndexCount, (UINT)InstanceCount,
			(UINT)IndexOffet, (UINT)BaseVertex,
			(UINT)InstanceOffset);
	}


	/************************************************************************************************/


	void Context::Dispatch(const uint3 xyz)
	{
		UpdateResourceStates();
		DeviceContext->Dispatch((UINT)xyz[0], (UINT)xyz[1], (UINT)xyz[2]);
	}


	/************************************************************************************************/


	void Context::DispatchRays(const uint3 WHD, const DispatchDesc desc)
	{
		UpdateResourceStates();

		D3D12_DISPATCH_RAYS_DESC dispatchDesc{};
		dispatchDesc.Width  = WHD[0];
		dispatchDesc.Height = WHD[1];
		dispatchDesc.Depth  = WHD[2];

		dispatchDesc.CallableShaderTable		= DeviceAddressRangeStrideToDX(desc.callableShaderTable);
		dispatchDesc.HitGroupTable				= DeviceAddressRangeStrideToDX(desc.hitGroupTable);
		dispatchDesc.MissShaderTable			= DeviceAddressRangeStrideToDX(desc.missTable);
		dispatchDesc.RayGenerationShaderRecord	= DeviceAddressRangeToDX(desc.rayGenerationRecord);

		DeviceContext->DispatchRays(&dispatchDesc);
	}


	/************************************************************************************************/


	void Context::DispatchMesh(const uint3 xyz)
	{
		UpdateResourceStates();
		DeviceContext->DispatchMesh(xyz[0], xyz[1], xyz[2]);
	}


	/************************************************************************************************/


	void Context::FlushBarriers() noexcept
	{
		UpdateResourceStates();
	}

	/************************************************************************************************/


	void Context::SetPredicate(bool Enabled, ResourceHandle handle, size_t Offset, PredicateOp op)
	{
		if (Enabled)
			DeviceContext->SetPredication(
				renderSystem->GetDeviceResource(handle),
				Offset * 8, 
				op == PredicateOp::NotEqualZero ? D3D12_PREDICATION_OP::D3D12_PREDICATION_OP_NOT_EQUAL_ZERO : D3D12_PREDICATION_OP::D3D12_PREDICATION_OP_EQUAL_ZERO);
		else
			DeviceContext->SetPredication(nullptr, 0, D3D12_PREDICATION_OP::D3D12_PREDICATION_OP_EQUAL_ZERO);
	}


	/************************************************************************************************/


	void Context::CopyBuffer(const UploadReservation src, const ResourceHandle destination, const size_t destOffset)
	{
		const auto destinationResource	= renderSystem->GetDeviceResource(destination);
		const auto sourceResource       = src.resource;

		UpdateResourceStates();

		DeviceContext->CopyBufferRegion(destinationResource, destOffset, sourceResource, src.offset, src.size);
	}


	/************************************************************************************************/


	void Context::CopyTexture2D(const UploadReservation src, const ResourceHandle destination, const uint2 BufferSize)
	{
		const auto destinationResource		= renderSystem->GetDeviceResource(destination);
		const auto WH						= renderSystem->GetTextureWH(destination);
		const auto format					= renderSystem->GetTextureDeviceFormat(destination);
		const auto texelSize				= renderSystem->GetTextureElementSize(destination);

		D3D12_TEXTURE_COPY_LOCATION destLocation{};
		destLocation.pResource			= destinationResource;
		destLocation.SubresourceIndex	= 0;
		destLocation.Type				= D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;


		D3D12_TEXTURE_COPY_LOCATION srcLocation{};
		srcLocation.pResource							= src.resource;
		srcLocation.Type								= D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcLocation.PlacedFootprint.Offset				= src.offset;
		srcLocation.PlacedFootprint.Footprint.Depth		= 1;
		srcLocation.PlacedFootprint.Footprint.Format	= format;
		srcLocation.PlacedFootprint.Footprint.Height	= WH[1];
		srcLocation.PlacedFootprint.Footprint.Width		= WH[0];
		srcLocation.PlacedFootprint.Footprint.RowPitch	= (UINT)(BufferSize[0] * texelSize);

		UpdateResourceStates();
		DeviceContext->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, nullptr);
	}


	/************************************************************************************************/


	void Context::Clear()
	{
		pendingBarriers.clear();
		RenderTargets.clear();
		DesciptorHeaps.clear();
		VBViews.clear();

		TrackedSOBuffers.clear();

		CurrentPipelineState = nullptr;

		DeviceContext->ClearState(nullptr);

		depthStencilViews.clear();
		renderTargetViews.clear();
	}


	/************************************************************************************************/


	void Context::SetRTRead(ResourceHandle Handle)
	{
		FK_ASSERT(0);
	}


	/************************************************************************************************/


	void Context::SetRTWrite(ResourceHandle Handle)
	{
		FK_ASSERT(0);
	}


	/************************************************************************************************/


	void Context::SetRTFree(ResourceHandle Handle)
	{
		FK_ASSERT(0);
	}


	/************************************************************************************************/


	void Context::Close()
	{
		renderSystem->_ReleaseDescriptorRange(shaderResources, dispatchIdx);
		shaderResources = {};

		if (auto HR = DeviceContext->Close(); FAILED(HR)) {
			FK_LOG_ERROR("Failed to close graphics context!");
			renderSystem->_OnCrash();
		}
	}


	/************************************************************************************************/


	Context& Context::Reset(DescriptorRange range, const size_t newDispatchIdx, ID3D12DescriptorHeap* heap)
	{
		shaderResources = range;

		CurrentPipelineState		= nullptr;
		CurrentRootSignature		= nullptr;
		CurrentComputeRootSignature = nullptr;

		if (FAILED(commandAllocator->Reset()))
		{
			DebugBreak();
			FK_LOG_ERROR("Failed to reset command allocator");
		}

		if (FAILED(DeviceContext->Reset(commandAllocator, nullptr)))
		{
			DebugBreak();
			FK_LOG_ERROR("Failed to reset device context");
		}

		_ResetDSV();
		_ResetRTV();
		_ResetSRV();

		TrackedSOBuffers.clear();
		pendingBarriers.clear();
		queuedBarriers.clear();
		renderTargetViews.clear();
		depthStencilViews.clear();
		queuedReadBacks.clear();

		heapUsed	= 0;
		dispatchIdx	= newDispatchIdx;

		DeviceContext->SetDescriptorHeaps(1, &heap);

		return *this;
	}


	/************************************************************************************************/


	UploadReservation Context::ReserveDirectUploadSpace(size_t size, size_t alignment) noexcept
	{
		return renderSystem->ReserveDirectUploadSpace(size, alignment);
	}


	/************************************************************************************************/


	void Context::SetUAVRead() 
	{
		FK_ASSERT(0);
	}


	/************************************************************************************************/


	void Context::SetUAVWrite() 
	{
		FK_ASSERT(0);
	}


	/************************************************************************************************/


	void Context::SetUAVFree() 
	{
		FK_ASSERT(0);
	}


	/************************************************************************************************/


	IRenderSystem& Context::GetRenderSystem() noexcept
	{
		return RenderSystem::_GetInstance();
	}


	/************************************************************************************************/


	void Context::BeginMarker(const char* str)
	{
	}


	/************************************************************************************************/


	void Context::EndMarker(const char* str)
	{

	}


	/************************************************************************************************/


	DescHeapPOS Context::_GetDepthDesciptor(ResourceHandle depthBuffer)
	{
		auto DSV_CPU_HANDLE = DescHeapPOS{};

		if (auto res = std::find_if(
			depthStencilViews.begin(),
			depthStencilViews.end(),
			[&](RTV_View& view)
			{
				return view.resource == depthBuffer;
			});
			res == depthStencilViews.end())
		{
			if (!depthStencilViews.full()) {
				auto DSV        = _ReserveDSV(1);
				DSV_CPU_HANDLE  = DSV;
				depthStencilViews.push_back({ depthBuffer, DSV });
			}
			else
				DSV_CPU_HANDLE = depthStencilViews[rand() % depthStencilViews.size()].descriptor;
		}
		else
			DSV_CPU_HANDLE = res->descriptor;

		return DSV_CPU_HANDLE;
	}


	/************************************************************************************************/


	void Context::UpdateResourceStates()
	{
		if (!pendingBarriers.size())
			return;

		static_vector<D3D12_GLOBAL_BARRIER, 32>		globalBarriers;
		static_vector<D3D12_TEXTURE_BARRIER, 32>	textureBarriers;
		static_vector<D3D12_BUFFER_BARRIER, 32>		bufferBarriers;

		for (const Barrier& barrier : pendingBarriers)
		{
			switch(barrier.type)
			{
			case BarrierType::Global:
			{
				D3D12_GLOBAL_BARRIER globalBarrier;
				globalBarrier.AccessBefore	= DAS2AccessState(barrier.accessBefore);
				globalBarrier.AccessAfter	= DAS2AccessState(barrier.accessAfter);
				globalBarrier.SyncBefore	= SyncPoint2DX(barrier.src);
				globalBarrier.SyncAfter		= SyncPoint2DX(barrier.dst);

				globalBarriers.push_back(globalBarrier);
			}	break;
			case BarrierType::Buffer:
			{
#ifdef USING(DEBUGGRAPHICS)
				FK_ASSERT(renderSystem->GetTextureDimension(barrier.resource) == TextureDimension::Buffer);
#endif

				D3D12_BUFFER_BARRIER bufferBarrier;
				bufferBarrier.pResource		= renderSystem->GetDeviceResource(barrier.resource);
				bufferBarrier.AccessBefore	= DAS2AccessState(barrier.accessBefore);
				bufferBarrier.AccessAfter	= DAS2AccessState(barrier.accessAfter);
				bufferBarrier.SyncBefore	= SyncPoint2DX(barrier.src);
				bufferBarrier.SyncAfter		= SyncPoint2DX(barrier.dst);
				bufferBarrier.Offset		= 0;// barrier.buffer.rangeBegin;
				bufferBarrier.Size			= UINT64_MAX;//barrier.buffer.rangeEnd - barrier.buffer.rangeBegin;

				bufferBarriers.push_back(bufferBarrier);
			}	break;
			case BarrierType::Texture:
			{
#ifdef USING(DEBUGGRAPHICS)
				auto dimension = renderSystem->GetTextureDimension(barrier.resource);
				FK_ASSERT(
					dimension == TextureDimension::Texture1D ||
					dimension == TextureDimension::Texture2D ||
					dimension == TextureDimension::Texture2DArray ||
					dimension == TextureDimension::Texture3D ||
					dimension == TextureDimension::TextureCubeMap);
#endif

				D3D12_TEXTURE_BARRIER textureBarrier;
				textureBarrier.AccessBefore		= DAS2AccessState(barrier.accessBefore);
				textureBarrier.AccessAfter		= DAS2AccessState(barrier.accessAfter);
				textureBarrier.LayoutBefore		= DeviceLayout2DX(barrier.texture.layoutBefore);
				textureBarrier.LayoutAfter		= DeviceLayout2DX(barrier.texture.layoutAfter);
				textureBarrier.Flags			= D3D12_TEXTURE_BARRIER_FLAG_NONE;
				textureBarrier.pResource		= renderSystem->GetDeviceResource(barrier.resource);
				textureBarrier.SyncAfter		= SyncPoint2DX(barrier.dst);
				textureBarrier.SyncBefore		= SyncPoint2DX(barrier.src);

				textureBarrier.Subresources		= D3D12_BARRIER_SUBRESOURCE_RANGE{
					.IndexOrFirstMipLevel	= 0,
					.NumMipLevels			= renderSystem->GetTextureMipCount(barrier.resource),
					.FirstArraySlice		= 0,
					.NumArraySlices			= (uint32_t)renderSystem->GetTextureArraySize(barrier.resource),
					.FirstPlane				= 0,
					.NumPlanes				= 1,
				};

				textureBarriers.push_back(textureBarrier);
			}	break;
			default:
				FK_ASSERT(0);
			};
		}

		static_vector<D3D12_BARRIER_GROUP>		groups;
		if (globalBarriers.size())	groups.emplace_back(D3D12_BARRIER_GROUP{ .Type = D3D12_BARRIER_TYPE::D3D12_BARRIER_TYPE_GLOBAL, .NumBarriers = (uint32_t)globalBarriers.size(), .pGlobalBarriers = globalBarriers.data()});
		if (textureBarriers.size())	groups.emplace_back(D3D12_BARRIER_GROUP{ .Type = D3D12_BARRIER_TYPE::D3D12_BARRIER_TYPE_TEXTURE, .NumBarriers = (uint32_t)textureBarriers.size(), .pTextureBarriers = textureBarriers.data() });
		if (bufferBarriers.size())	groups.emplace_back(D3D12_BARRIER_GROUP{ .Type = D3D12_BARRIER_TYPE::D3D12_BARRIER_TYPE_BUFFER, .NumBarriers =  (uint32_t)bufferBarriers.size(), .pBufferBarriers = bufferBarriers.data() });

		if(groups.size())
			DeviceContext->Barrier((uint32_t)groups.size(), groups);

		pendingBarriers.clear();
	}


	/************************************************************************************************/


	void Context::_QueueReadBacks()
	{
		for (const auto readBackHandle : queuedReadBacks)
		{
			auto& readBack	= renderSystem->ReadBackTable[readBackHandle];
			auto fence		= renderSystem->directFence;

			auto HR	= fence->SetEventOnCompletion(dispatchIdx, readBack.event); FK_ASSERT(SUCCEEDED(HR));
			renderSystem->GraphicsQueue->Signal(fence, dispatchIdx);

			readBack.queueUntil	= dispatchIdx;
			readBack.queued		= true;
		}

		queuedReadBacks.clear();
	}


	/************************************************************************************************/


	DescHeapPOS Context::_ReserveDSV(size_t count)
	{
		auto currentCPU = CPUDescriptorHandle{ DSV_CPU.ptr };
		DSV_CPU.ptr = DSV_CPU.ptr + renderSystem->DescriptorDSVSize * count;

		return { currentCPU, InvalidHandle };
	}


	/************************************************************************************************/


	std::optional<DescHeapPOS> Context::_ReserveSRV(size_t count)
	{
		if (shaderResources.size > heapUsed + count)
		{
			auto out = shaderResources[heapUsed];

			heapUsed += count;
			return { out };
		}
		else
		{
			DebugBreak();
			return {};
		}
	}



	/************************************************************************************************/


	DescHeapPOS Context::_ReserveSRVLocal(size_t count)
	{
		auto currentCPU = SRV_LOCAL_CPU;
		SRV_LOCAL_CPU.ptr = SRV_LOCAL_CPU.ptr + renderSystem->DescriptorCBVSRVUAVSize * count;

		return { CPUDescriptorHandle{ currentCPU.ptr }, InvalidHandle };
	}


	/************************************************************************************************/


	DescHeapPOS Context::_ReserveRTV(size_t count)
	{
		auto currentCPU = RTV_CPU;
		RTV_CPU.ptr = RTV_CPU.ptr + renderSystem->DescriptorRTVSize * count;

		return { CPUDescriptorHandle{ currentCPU.ptr }, InvalidHandle };
	}


	/************************************************************************************************/


	void Context::_ResetRTV()
	{
		RTV_CPU = descHeapRTV->GetCPUDescriptorHandleForHeapStart();

		renderTargetViews.clear();
	}


	/************************************************************************************************/


	void Context::_ResetDSV()
	{
		DSV_CPU = descHeapDSV->GetCPUDescriptorHandleForHeapStart();

		depthStencilViews.clear();
	}


	/************************************************************************************************/


	void Context::_ResetSRV()
	{
		heapUsed = 0;

		SRV_LOCAL_CPU = descHeapSRVLocal->GetCPUDescriptorHandleForHeapStart();
	}


	/************************************************************************************************/


	void RenderSystem::RootSigLibrary::Initiate(RenderSystem* RS, iAllocator& allocator, iAllocator& temp)
	{
		ID3D12Device* Device = RS->pDevice;

		/*	CD3DX12_STATIC_SAMPLER_DESC(
			UINT shaderRegister,
			D3D12_FILTER filter                      = D3D12_FILTER_ANISOTROPIC,
			D3D12_TEXTURE_ADDRESS_MODE addressU      = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE addressV      = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE addressW      = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			FLOAT mipLODBias                         = 0,
			UINT maxAnisotropy                       = 16,
			D3D12_COMPARISON_FUNC comparisonFunc     = D3D12_COMPARISON_FUNC_LESS_EQUAL,
			D3D12_STATIC_BORDER_COLOR borderColor    = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
			FLOAT minLOD                             = 0.f,
			FLOAT maxLOD                             = D3D12_FLOAT32_MAX,
			D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
			UINT registerSpace                       = 0)
		{
		*/

		RootSignatureBuilder builder{ allocator };

		CD3DX12_STATIC_SAMPLER_DESC	 Samplers[] = {
			CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT, 
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP),

			CD3DX12_STATIC_SAMPLER_DESC{1, D3D12_FILTER::D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR },
			CD3DX12_STATIC_SAMPLER_DESC{2, D3D12_FILTER::D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT},
		};

		{
			builder.AllowIA = true;
			DesciptorHeapLayout<2> DescriptorHeap;
			DescriptorHeap.SetParameterAsSRV(0, 0, 6);
			DescriptorHeap.SetParameterAsCBV(1, 6, 4);
			FK_ASSERT(DescriptorHeap.Check());

			builder.SetParameterAsDescriptorTable(0, DescriptorHeap, -1);
			builder.SetParameterAsCBV(1, 0, 0, PIPELINE_DEST_ALL);
			builder.SetParameterAsCBV(2, 1, 0, PIPELINE_DEST_ALL);
			builder.SetParameterAsCBV(3, 2, 0, PIPELINE_DEST_ALL);
			builder.SetParameterAsCBV(4, 3, 0, PIPELINE_DEST_ALL);
			builder.SetParameterAsCBV(5, 4, 0, PIPELINE_DEST_ALL);
			builder.SetParameterAsCBV(6, 5, 0, PIPELINE_DEST_ALL);
			builder.SetParameterAsSRV(7, 7, 0, PIPELINE_DEST_VS);
			RS6CBVs4SRVs = builder.Build(RS, temp);
			SETDEBUGNAME(*RS6CBVs4SRVs, "RS4CBVs4SRVs");
		}
		{
			builder.AllowIA	= true;
			builder.AllowSO	= true;
			DesciptorHeapLayout<1> DescriptorHeap;
			DescriptorHeap.SetParameterAsSRV(0, 0, 8);

			builder.SetParameterAsCBV				(0, 0, 0, PIPELINE_DEST_ALL);
			builder.SetParameterAsCBV				(1, 1, 0, PIPELINE_DEST_ALL);
			builder.SetParameterAsCBV				(2, 2, 0, PIPELINE_DEST_ALL);
			builder.SetParameterAsDescriptorTable	(3, DescriptorHeap, -1);
			builder.SetParameterAsUAV				(4, 0, 0, PIPELINE_DEST_ALL);
			RS4CBVs_SO = builder.Build(RS, temp);

			SETDEBUGNAME(*RS4CBVs_SO, "RS4CBVs_SO");
		}
		{
			builder.AllowIA = true;
			DesciptorHeapLayout<16> DescriptorHeap;
			DescriptorHeap.SetParameterAsShaderUAV	(0, 0, 4);
			DescriptorHeap.SetParameterAsSRV		(1, 0, 4);
			DescriptorHeap.SetParameterAsCBV		(2, 4, 4);
			FK_ASSERT(DescriptorHeap.Check());

			builder.SetParameterAsDescriptorTable(0, DescriptorHeap, -1);
			builder.SetParameterAsCBV(1, 0, 3, PIPELINE_DESTINATION::PIPELINE_DEST_ALL);
			RS2UAVs4SRVs4CBs = builder.Build(RS, temp);

			SETDEBUGNAME(*RS2UAVs4SRVs4CBs, "RS2UAVs4SRVs4CBs");
		}
		{
			DesciptorHeapLayout<16> DescriptorHeap;
			DescriptorHeap.SetParameterAsSRV		(0, 0, 8);
			DescriptorHeap.SetParameterAsShaderUAV	(1, 0, 1);
			DescriptorHeap.SetParameterAsCBV		(2, 0, 2);
			FK_ASSERT(DescriptorHeap.Check());

			builder.AllowIA = false;
			builder.SetParameterAsDescriptorTable(0, DescriptorHeap, -1);
			ShadingRTSig = builder.Build(RS, temp);

			SETDEBUGNAME(*ShadingRTSig, "ShadingRTSig");
		}
		{

			DesciptorHeapLayout<16> DescriptorHeapSRV;
			DescriptorHeapSRV.SetParameterAsSRV(0, 0, -1, 0);
			FK_ASSERT(DescriptorHeapSRV.Check());

			DesciptorHeapLayout<16> DescriptorHeapUAV;
			DescriptorHeapUAV.SetParameterAsShaderUAV(0, 0, -1);
			FK_ASSERT(DescriptorHeapUAV.Check());

			builder.AllowIA = true;
			builder.SetParameterAsCBV				(0, 0, 0, PIPELINE_DESTINATION::PIPELINE_DEST_ALL);
			builder.SetParameterAsCBV				(1, 1, 0, PIPELINE_DESTINATION::PIPELINE_DEST_ALL);
			builder.SetParameterAsCBV				(2, 2, 0, PIPELINE_DESTINATION::PIPELINE_DEST_ALL);
			builder.SetParameterAsCBV				(3, 3, 0, PIPELINE_DESTINATION::PIPELINE_DEST_ALL);
			builder.SetParameterAsDescriptorTable	(4, DescriptorHeapSRV, -1, PIPELINE_DESTINATION::PIPELINE_DEST_ALL);
			builder.SetParameterAsDescriptorTable	(5, DescriptorHeapUAV, -1, PIPELINE_DESTINATION::PIPELINE_DEST_ALL);
			RSDefault = builder.Build(RS, temp);

			SETDEBUGNAME(*RSDefault, "RSDefault");
		}
		{
			builder.AllowIA = false;
			DesciptorHeapLayout<16> DescriptorHeap;
			DescriptorHeap.SetParameterAsShaderUAV(0, 0, 4, 0);
			DescriptorHeap.SetParameterAsSRV(1, 0, 4, 0);
			DescriptorHeap.SetParameterAsCBV(2, 0, 2, 0);
			FK_ASSERT(DescriptorHeap.Check());

			builder.SetParameterAsDescriptorTable(0, DescriptorHeap, -1, PIPELINE_DEST_CS);
			ComputeSignature = builder.Build(RS, temp);

			SETDEBUGNAME(*ComputeSignature, "ComputeSignature");

			builder.SetParameterAsUINT(0, 6, 0, 0, PIPELINE_DEST_CS);
			builder.SetParameterAsUAV(1, 0, 0, PIPELINE_DEST_CS);
			ClearBuffer = builder.Build(RS, temp);

			SETDEBUGNAME(*ClearBuffer, "ClearBuffer");
		}
	}


	/************************************************************************************************/


	UploadBuffer::UploadBuffer(ID3D12Device* pDevice)  :
		parentDevice	{ pDevice		},
		Size			{ MEGABYTE * 64	}
	{
		D3D12_RESOURCE_DESC   Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(Size);
		D3D12_HEAP_PROPERTIES HEAP_Props    = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		HRESULT HR = pDevice->CreateCommittedResource(
			&HEAP_Props,
			D3D12_HEAP_FLAG_NONE,
			&Resource_DESC,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&deviceBuffer));

		SETDEBUGNAME(deviceBuffer, __func__);

		CD3DX12_RANGE Range(0, 0);
		HR = deviceBuffer->Map(0, &Range, (void**)&Buffer); CheckHR(HR, ASSERTONFAIL("FAILED TO MAP TEMP BUFFER"));
	}


	/************************************************************************************************/


	void UploadBuffer::Release()
	{
		if (!deviceBuffer)
			return;

		deviceBuffer->Unmap(0, nullptr);
		deviceBuffer->Release();

		Position		= 0;
		Size			= 0;
		deviceBuffer	= nullptr;
		Buffer			= nullptr;
	}


	/************************************************************************************************/


	UploadBuffer::UploadBuffer(UploadBuffer&& rhs)
	{
		Release();

		Position		= rhs.Position;
		Size			= rhs.Size;
		deviceBuffer	= rhs.deviceBuffer;
		Buffer			= rhs.Buffer;
		parentDevice	= rhs.parentDevice;

		rhs.Position		= 0;
		rhs.Size			= 0;
		rhs.deviceBuffer	= nullptr;
		rhs.Buffer			= nullptr;
		rhs.parentDevice	= nullptr;
	}


	UploadBuffer& UploadBuffer::operator = (UploadBuffer&& rhs) noexcept
	{
		Release();

		Position		= rhs.Position;
		Size			= rhs.Size;
		deviceBuffer	= rhs.deviceBuffer;
		Buffer			= rhs.Buffer;
		parentDevice	= rhs.parentDevice;

		rhs.Position		= 0;
		rhs.Size			= 0;
		rhs.deviceBuffer	= nullptr;
		rhs.Buffer			= nullptr;
		rhs.parentDevice	= nullptr;

		return *this;
	}


	UploadBuffer::~UploadBuffer()
	{
		Release();
	}


	/************************************************************************************************/


	std::expected<UploadReservation, ReserveErrors> UploadBuffer::Reserve(const size_t reserveSize, const size_t alignment)
	{
		// Not enough remaining Space in Buffer GOTO Beginning if space in front of upload buffer is available
		if	(Position + reserveSize > Size && Last != 0)
			Position = 0;

		auto GetOffset = [&]() {
			auto offset = alignment - (Position & (alignment - 1));
			return (offset == alignment) ? 0 : offset;
		};

		// Buffer too Small
		if (Position + reserveSize + GetOffset() > Size)
			return std::unexpected{ ReserveErrors::OutOfSpace };

		if (Last > Position)
		{	// Potential Overlap condition
			if (Position + reserveSize + GetOffset() >= Last)
				return std::unexpected{ ReserveErrors::OutOfSpace };  // Resize Buffer and then upload

			const auto alignmentOffset  = GetOffset();
			char*           buffer      = Buffer + Position + alignmentOffset;
			const size_t    offset      = Position + alignmentOffset;

			Position += reserveSize + alignmentOffset;

			return UploadReservation{
				.resource	= deviceBuffer,
				.size		= reserveSize,
				.offset		= offset,
				.buffer		= buffer,
			};
		}

		if(Last <= Position)
		{	// Safe, Do Upload
			const auto alignmentOffset = GetOffset();

			char* buffer		= Buffer + Position + alignmentOffset;
			size_t offset		= Position + alignmentOffset;
			Position			+= reserveSize + alignmentOffset;

			return UploadReservation{
				.resource	= deviceBuffer,
				.size		= reserveSize,
				.offset		= offset,
				.buffer		= buffer,
			};
		}

		return std::unexpected{ ReserveErrors::Unknown };
	}


	/************************************************************************************************/


	ID3D12Resource* UploadBuffer::Resize(const size_t size)
	{
		if(deviceBuffer)
			deviceBuffer->Unmap(0, 0);

		auto previousBuffer = deviceBuffer;

		D3D12_RESOURCE_DESC   Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(size);
		D3D12_HEAP_PROPERTIES HEAP_Props	= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		ID3D12Resource* newDeviceBuffer = nullptr;

		HRESULT HR = parentDevice->CreateCommittedResource(
			&HEAP_Props,
			D3D12_HEAP_FLAG_NONE,
			&Resource_DESC,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&newDeviceBuffer));

		Position		= 0;
		Last			= 0;
		Size			= size;
		deviceBuffer	= newDeviceBuffer;
		SETDEBUGNAME(newDeviceBuffer, "TEMPORARY");

		CD3DX12_RANGE Range(0, 0);
		HR = newDeviceBuffer->Map(0, &Range, (void**)&Buffer);   CheckHR(HR, ASSERTONFAIL("FAILED TO MAP TEMP BUFFER"));

		return previousBuffer;
	}


	/************************************************************************************************/


	void CopyContext::Barrier(ID3D12Resource* resource, const DeviceAccessState before, const DeviceAccessState after)
	{
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			resource,
			DRS2D3DState(before),
			DRS2D3DState(after));


		if (pendingBarriers.full())
			flushPendingBarriers();

		pendingBarriers.push_back(barrier);
	}


	/************************************************************************************************/


	void CopyContext::flushPendingBarriers()
	{
		if (pendingBarriers.empty())
			return;

		commandList->ResourceBarrier((UINT)pendingBarriers.size(), pendingBarriers.data());
		pendingBarriers.clear();
	}


	/************************************************************************************************/


	IRenderSystem& CopyContext::GetRenderSystem() noexcept
	{
		return RenderSystem::_GetInstance();
	}



	/************************************************************************************************/


	UploadReservation CopyContext::Reserve(const size_t reserveSize, const size_t reserveAlignement)
	{
		// Not enough remaining Space in Buffer GOTO Beginning if space in front of upload buffer is available
		if	(uploadBuffer.Position + reserveSize > uploadBuffer.Size && uploadBuffer.Last != 0)
			uploadBuffer.Position = 0;

		auto GetOffset = [&]() {
			auto offset = reserveAlignement - (uploadBuffer.Position & (reserveAlignement - 1));
			return (offset == reserveAlignement) ? 0 : offset;
		};

		auto ResizeBuffer = [&] {
			const auto newSize = (size_t )std::pow(2, std::floor(std::log2(reserveSize)) + 1);
			freeResources.push_back(uploadBuffer.Resize(newSize));
		};

		// Buffer too Small
		auto temp = reserveSize + GetOffset();
		if (uploadBuffer.Position + reserveSize + GetOffset() > uploadBuffer.Size)
			ResizeBuffer();

		if (uploadBuffer.Last > uploadBuffer.Position)
		{	// Potential Overlap condition
			if (uploadBuffer.Position + reserveSize + GetOffset() >= uploadBuffer.Last)
				ResizeBuffer();  // Resize Buffer and then upload

			const auto alignmentOffset  = GetOffset();
			char*           buffer      = uploadBuffer.Buffer + uploadBuffer.Position + alignmentOffset;
			const size_t    offset      = uploadBuffer.Position + alignmentOffset;

			uploadBuffer.Position += reserveSize + alignmentOffset;

			return UploadReservation{
				.resource	= uploadBuffer.deviceBuffer,
				.size		= reserveSize,
				.offset		= offset,
				.buffer		= buffer
			};
		}

		if(uploadBuffer.Last <= uploadBuffer.Position)
		{	// Safe, Do Upload
			const auto alignmentOffset = GetOffset();

			char* buffer            = uploadBuffer.Buffer + uploadBuffer.Position + alignmentOffset;
			size_t offset           = uploadBuffer.Position + alignmentOffset;
			uploadBuffer.Position  += reserveSize + alignmentOffset;

			return UploadReservation{
				.resource	= uploadBuffer.deviceBuffer,
				.size		= reserveSize,
				.offset		= offset,
				.buffer		= buffer
			};
		}

		return {};
	}


	/************************************************************************************************/


	void CopyContext::CopyBuffer(GPURange destRange, void* source_ptr, uint64_t size)
	{
		auto uploadSize		= Min(destRange.size, size);
		auto uploadSpace	= Reserve(uploadSize);
		auto dest			= RenderSystem::globalInstance->GetDeviceResource(destRange.resource);

		commandList->CopyBufferRegion(
			dest,
			destRange.offset,
			uploadSpace.resource,
			uploadSpace.offset,
			uploadSize);
	}


	/************************************************************************************************/


	void CopyContext::CopyBuffer(ResourceHandle handle, const size_t destOffset, UploadReservation source)
	{
		flushPendingBarriers();

		auto dest = RenderSystem::globalInstance->GetDeviceResource(handle);

		commandList->CopyBufferRegion(
			dest,
			destOffset,
			source.resource,
			source.offset,
			source.size);
	}


	/************************************************************************************************/


	void CopyContext::CopyBuffer(ID3D12Resource* destination, const size_t destinationOffset, UploadReservation source)
	{
		flushPendingBarriers();

		commandList->CopyBufferRegion(
			destination,
			destinationOffset,
			source.resource,
			source.offset,
			source.size);
	}


	/************************************************************************************************/


	void CopyContext::CopyBuffer(ID3D12Resource* destination, const size_t destinationOffset, ID3D12Resource* source, const size_t sourceOffset, const size_t sourceSize)
	{
		flushPendingBarriers();

		commandList->CopyBufferRegion(
			destination,
			destinationOffset,
			source,
			sourceOffset,
			sourceSize);
	}


	/************************************************************************************************/


	void CopyContext::CopyTextureRegion(
		ID3D12Resource*		destination,
		size_t				subResourceIdx,
		uint3				XYZ,
		UploadReservation	source,
		uint2				WH,
		DeviceFormat		format)
	{
		flushPendingBarriers();

		const auto		deviceFormat	= TextureFormat2DXGIFormat(format);
		const size_t	formatSize		= GetFormatElementSize(deviceFormat);
		const bool		BCformat		= IsDDS(format);
		const size_t	rowPitch		= AlignedSize(BCformat ? formatSize * WH[0] / 4 : formatSize * WH[0]);
		//size_t alignmentOffset    = rowPitch & 0x01ff;

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT SubRegion;
		SubRegion.Footprint.Depth		= 1;
		SubRegion.Footprint.Format		= deviceFormat;
		SubRegion.Footprint.RowPitch	= (UINT)rowPitch;
		SubRegion.Footprint.Width		= WH[0];
		SubRegion.Footprint.Height		= WH[1];
		SubRegion.Offset				= source.offset;

		auto destinationLocation	= CD3DX12_TEXTURE_COPY_LOCATION(destination, (UINT)subResourceIdx);
		auto sourceLocation			= CD3DX12_TEXTURE_COPY_LOCATION(source.resource, SubRegion);

		commandList->CopyTextureRegion(
			&destinationLocation,
			XYZ[0], XYZ[1], XYZ[2],
			&sourceLocation,
			nullptr);
	}


	/************************************************************************************************/


	void CopyContext::CopyTile(ID3D12Resource* dest, const uint3 destTile, const size_t tileOffset, const UploadReservation src)
	{
		flushPendingBarriers();

		auto desc = dest->GetDesc();

		D3D12_TILED_RESOURCE_COORDINATE coordinate;
		coordinate.X			= (UINT)destTile[0];
		coordinate.Y			= (UINT)destTile[1];
		coordinate.Z			= (UINT)0;
		coordinate.Subresource	= (UINT)destTile[2];
		
		D3D12_TILE_REGION_SIZE regionSize;
		regionSize.NumTiles		= 1;
		regionSize.UseBox		= false;
		regionSize.Width		= 1;
		regionSize.Height		= 1;
		regionSize.Depth		= 1;

		commandList->CopyTiles(
			dest,
			&coordinate,
			&regionSize,
			src.resource,
			src.offset,
			D3D12_TILE_COPY_FLAG_LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE);
	}


	/************************************************************************************************/


	bool CopyContext::IsSubResourceTiled(ID3D12Resource* resource, const size_t level) const
	{
		ID3D12Device* device = nullptr;
		commandList->GetDevice(IID_PPV_ARGS(&device));

		UINT						TileCount = 0;
		D3D12_PACKED_MIP_INFO		packedMipInfo;
		D3D12_TILE_SHAPE			TileShape;
		UINT						subResourceTilingCount = 1;
		D3D12_SUBRESOURCE_TILING	subResourceTiling_Packed;

		device->GetResourceTiling(resource, &TileCount, &packedMipInfo, &TileShape, &subResourceTilingCount, (UINT)level, &subResourceTiling_Packed);

		return (subResourceTiling_Packed.HeightInTiles * subResourceTiling_Packed.WidthInTiles) != 0;
	}


	/************************************************************************************************/
	

	CopyContext& CopyEngine::operator [](CopyContextHandle handle)
	{
		return copyContexts[handle];
	}


	/************************************************************************************************/


	CopyContextHandle CopyEngine::Open()
	{
		const size_t currentIdx	= idx++ % copyContexts.size();
		auto& ctx				= copyContexts[currentIdx];

		Wait(CopyContextHandle{ currentIdx });

		if (FAILED(ctx.commandAllocator->Reset()))
			__debugbreak();

		if (FAILED(ctx.commandList->Reset(ctx.commandAllocator, nullptr)))
			__debugbreak();

		return CopyContextHandle{ currentIdx };
	}


	/************************************************************************************************/


	void CopyEngine::Wait(SyncPoint syncTo)
	{
		copyQueue->Wait(syncTo.fence, syncTo.syncCounter);
	}


	/************************************************************************************************/


	void CopyEngine::Wait(CopyContextHandle handle)
	{
		auto& ctx = copyContexts[handle];

		if (fence->GetCompletedValue() < ctx.counter)
		{
			fence->SetEventOnCompletion(ctx.counter, ctx.eventHandle);
			WaitForSingleObject(ctx.eventHandle, 0xffffffff);
		}
	}


	/************************************************************************************************/


	void CopyEngine::Close(CopyContextHandle handle)
	{
		auto& ctx = copyContexts[handle];
		ctx.flushPendingBarriers();

		if (FAILED(ctx.commandList->Close()))
		{
			FK_LOG_ERROR("Failed to close Copy Command list!");
			__debugbreak();
		}
	}


	/************************************************************************************************/


	void CopyEngine::Submit(CopyContextHandle* begin, CopyContextHandle* end, std::optional<SyncPoint> syncOpt)
	{
		const size_t localCounter = ++counter;
		static_vector<ID3D12CommandList*, 64> cmdLists;

		for (auto itr = begin; itr < end; itr++)
		{
			auto& context = copyContexts[*itr];
			cmdLists.push_back(context.commandList);

			context.counter				= localCounter;
			context.uploadBuffer.Last	= context.uploadBuffer.Position;
			
			Close(*itr);
		}

		if (auto syncPoint = syncOpt.value_or(SyncPoint{}); syncOpt.has_value())
			copyQueue->Wait(syncPoint.fence, syncPoint.syncCounter);

		copyQueue->ExecuteCommandLists((UINT)cmdLists.size(), cmdLists);

		if (const auto HR = copyQueue->Signal(fence, localCounter); FAILED(HR))
			FK_LOG_ERROR("FAILED TO SUBMIT TO COPY ENGINE!");
	}


	/************************************************************************************************/


	void CopyEngine::Signal(ID3D12Fence* fence, const size_t counter)
	{
		if (auto HR = copyQueue->Signal(fence, counter); FAILED(HR))
			FK_LOG_ERROR("Failed to Signal");
	}

	void CopyEngine::Signal(SyncPoint sync)
	{
		if (auto HR = copyQueue->Signal(sync.fence, sync.syncCounter); FAILED(HR))
			FK_LOG_ERROR("Failed to Signal");
	}


	/************************************************************************************************/


	void CopyEngine::Push_Temporary(ID3D12Resource* resource, CopyContextHandle handle)
	{
		copyContexts[handle].freeResources.push_back(resource);
	}


	/************************************************************************************************/


	void CopyEngine::Release()
	{
		if (!fence)
			return;

		for (auto& ctx : copyContexts)
		{
			if (fence->GetCompletedValue() < ctx.counter)
			{
				fence->SetEventOnCompletion(ctx.counter, ctx.eventHandle);
				WaitForSingleObject(ctx.eventHandle, 0xffffffff);
			}

			for (auto resource : ctx.freeResources)
				resource->Release();

			ctx.uploadBuffer.Release();
		}

		fence->Release();
		fence = nullptr;

		for (auto& ctx : copyContexts)
		{
			ctx.commandAllocator->Release();
			ctx.commandList->Release();
		}

		copyContexts.clear();
		copyQueue->Release();
	}


	/************************************************************************************************/


	bool CopyEngine::Initiate(ID3D12Device* Device, const size_t threadCount, Vector<ID3D12DeviceChild*>& ObjectsCreated, iAllocator* allocator)
	{
		bool Success = true;


		D3D12_COMMAND_QUEUE_DESC copyQueue_Desc = {};
		copyQueue_Desc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
		copyQueue_Desc.Type  = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY;

		auto HR = Device->CreateCommandQueue(&copyQueue_Desc, IID_PPV_ARGS(&copyQueue));	FK_ASSERT(FAILED(HR), "FAILED TO CREATE COPY QUEUE!");

		SETDEBUGNAME(copyQueue, "UPLOAD QUEUE");

		for (size_t I = 0; I < threadCount && Success; ++I)
		{
			ID3D12CommandAllocator*		commandAllocator	= nullptr;
			ID3D12GraphicsCommandList2*	copyCommandList		= nullptr;

			auto HR = Device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY,
				IID_PPV_ARGS(&commandAllocator));

			ObjectsCreated.push_back(commandAllocator);
				

#ifdef _DEBUG
			FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND ALLOCATOR!");
			Success &= !FAILED(HR);
#endif
			if (!Success)
				break;

			ObjectsCreated.push_back(commandAllocator);


			HR = Device->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY,
				commandAllocator,
				nullptr,
				__uuidof(ID3D12CommandList),
				(void**)&copyCommandList);


			FK_ASSERT	(FAILED(HR),        "FAILED TO CREATE COMMAND LIST!");
			FK_VLOG		(10,                "COPY COMMANDLIST CREATED: %u", copyCommandList);
			SETDEBUGNAME(commandAllocator,  "COPY ALLOCATOR");
			SETDEBUGNAME(copyCommandList,   "COPY COMMAND LIST");

			ObjectsCreated.push_back(copyCommandList);

			Success &= !FAILED(HR);

			copyCommandList->Close();
			commandAllocator->Reset();

			CopyContext copyCtx;
			copyCtx.commandAllocator	= commandAllocator;
			copyCtx.commandList			= copyCommandList;
			copyCtx.eventHandle			= CreateEvent(nullptr, FALSE, FALSE, nullptr);
			copyCtx.uploadBuffer		= UploadBuffer{ Device };
			copyCtx.freeResources		= Vector<ID3D12Resource*>{ allocator };

			copyContexts.emplace_back(std::move(copyCtx));
			/*
			copyContexts.push_back(
				CopyContext{
					commandAllocator,
					copyCommandList,
					0,
					CreateEvent(nullptr, FALSE, FALSE, nullptr),
					UploadBuffer{ Device },
					Vector<ID3D12Resource*>{ allocator }});
			*/
			}

		Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

		return Success;
	}


	/************************************************************************************************/


	LoadPipelineStateRes CreateClearBufferPSO(IRenderSystem& irs, iAllocator&)
	{
		auto& RS = static_cast<RenderSystem&>(irs);
		Shader computeShader = RS.LoadShader("Clear", "cs_6_0", R"(assets\shaders\ClearBuffer.hlsl)");

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			RS.Library(ROOTLIBRARYSIG::ClearBuffer)->GetAPIObject(),
			Shader2ByteCode(computeShader)
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS.pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

		FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

		return { PSO, RS.Library(ROOTLIBRARYSIG::ClearBuffer) };
	}


	/************************************************************************************************/


	HeapTable::HeapTable(ID3D12Device* IN_device, iAllocator* allocator) :
		pDevice	{ IN_device },
		handles	{ allocator },
		heaps	{ allocator } {}


	/************************************************************************************************/


	HeapTable::~HeapTable()
	{
		Release();
	}


	/************************************************************************************************/


	void HeapTable::Release()
	{
		for (auto& heap : heaps)
			heap.Release();

		heaps.clear();
		handles.Clear();
	}


	/************************************************************************************************/


	void HeapTable::Init(ResourceHeapTier IN_tier, ID3D12Device* IN_device)
	{
		tier	= IN_tier;
		pDevice = IN_device;
	}


	/************************************************************************************************/


	DeviceHeapHandle HeapTable::CreateHeap(const size_t size, const uint32_t flags)
	{
		D3D12_HEAP_PROPERTIES HEAP_Props	={};
		HEAP_Props.CPUPageProperty			= D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type						= D3D12_HEAP_TYPE_DEFAULT;
		HEAP_Props.MemoryPoolPreference		= D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask			= 0;
		HEAP_Props.VisibleNodeMask			= 0;

		D3D12_HEAP_DESC heapDesc;

		switch (tier)
		{
		case ResourceHeapTier::HeapTier1:
		{
			heapDesc = D3D12_HEAP_DESC
			{
				size,
				HEAP_Props,
				D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
				((flags == DeviceHeapFlags::NONE)			? D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES : D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE) |
				((flags &  DeviceHeapFlags::RenderTarget)	? D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES : D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE) |
				((flags &  DeviceHeapFlags::UAVBuffer)		? D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS | D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS: D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE) | 
				((flags &  DeviceHeapFlags::UAVTextures)	? D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS | D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES: D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE)
			};
		}	break;
		case ResourceHeapTier::HeapTier2:
		{
			heapDesc = D3D12_HEAP_DESC
			{
				size,
				HEAP_Props,
				D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
				D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE
			};
		}	break;
		default:
			break;
		}


		ID3D12Heap1* heap_ptr = nullptr;

		const auto HR = pDevice->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap_ptr));

#if USING(DEBUGGRAPHICS)
		auto temp = fmt::format("Heap {}", heaps.size());
		SETDEBUGNAME(heap_ptr, temp.c_str());
#endif

		if (SUCCEEDED(HR))
		{
			auto localLock = std::scoped_lock{ m };

			auto handle = handles.GetNewHandle();
			handles[handle] = (index_t)heaps.push_back({ heap_ptr, handle });

			return handle;
		}
		else
		{
			FK_LOG_ERROR("Failed to create Heap. Flags: %u", flags);
			return InvalidHandle;
		}
	}


	/************************************************************************************************/


	void HeapTable::ReleaseHeap(DeviceHeapHandle heap)
	{
		const auto idx = handles[heap];
		heaps[idx].Release();

		auto localLock = std::scoped_lock{ m };
		handles.RemoveHandle(heap);

		if (heaps.size() > 1)
		{
			heaps[idx] = heaps.back();
			handles[heaps[idx].handle] = idx;
		}

		heaps.pop_back();
	}


	/************************************************************************************************/


	size_t HeapTable::GetHeapSize(DeviceHeapHandle heap) const
	{
		auto desc = heaps[handles[heap]].heap->GetDesc();
		return desc.SizeInBytes;
	}


	/************************************************************************************************/


	ID3D12Heap* HeapTable::GetDeviceResource(DeviceHeapHandle handle) const
	{
		return heaps[handles[handle]].heap;
	}

	/************************************************************************************************/


	RenderSystem::RenderSystem(iAllocator* IN_allocator, ThreadManager* IN_Threads) :
			Memory			{ IN_allocator },
			Queries			{ IN_allocator, this },
			Textures		{ IN_allocator },
			VertexBuffers	{ IN_allocator },
			ConstantBuffers	{ IN_allocator, this },
			PipelineStates	{ IN_allocator, this, IN_Threads },
			StreamOutTable	{ IN_allocator },
			ReadBackTable	{ IN_allocator },
			rootSignatures	{ IN_allocator },
			threads			{ *IN_Threads },
			Syncs			{ IN_allocator, 64 },
			Contexts		{ IN_allocator, 3 * (1 + IN_Threads->GetThreadCount()) },
			heaps			{ pDevice, IN_allocator }
	{
		if (globalInstance)
			throw std::runtime_error{"Two Render Systems created!"};

		globalInstance = this;
	}


	RenderSystem::~RenderSystem() { Release(); }


	/************************************************************************************************/


	bool RenderSystem::Initiate(Graphics_Desc* in)
	{
		Vector<ID3D12DeviceChild*> ObjectsCreated(in->Memory);

		Memory				= in->Memory;
		Settings.AAQuality	= 0;
		Settings.AASamples	= 1;
		UINT DeviceFlags	= 0;

		ID3D12Device10* Device = nullptr;
		ID3D12Debug1* Debug = nullptr;
		ID3D12DebugDevice* DebugDevice = nullptr;


#if USING(ENABLEDRED)
		ID3D12DeviceRemovedExtendedDataSettings1* dredSettings;
		if (auto HR = D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings)); FAILED(HR))
			FK_LOG_ERROR("Failed to enable Dred!");
		else
		{
			dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
			dredSettings->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
			dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);

			FK_LOG_INFO("DRED enabled");
		}
#endif


		if (in->DX_DebugMode && !FAILED(D3D12GetDebugInterface(__uuidof(ID3D12Debug1), (void**)&Debug)))
		{
			Debug->EnableDebugLayer();

			if (!FAILED(D3D12GetDebugInterface(__uuidof(ID3D12Debug5), (void**)&pDebug5)))
			{
				pDebug5->SetEnableAutoName(true);
				pDebug5->SetEnableGPUBasedValidation(in->DX_GPUvalidation);
				Debug->SetEnableSynchronizedCommandQueueValidation(in->DX_GPUvalidation);
			}
		}
		else
		{
			Debug		= nullptr;
			DebugDevice = nullptr;
		}

		bool InitiateComplete = false;

		if(FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&Device))))
		{
			FK_LOG_ERROR("Failed to create A DX12 Device!");

			if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&Device))))
			{
				FK_LOG_ERROR("Failed to create A DX12 Device!");
				return false;
			}
		}

#if USING(ENABLEDRED)
		if (auto HR = Device->QueryInterface(IID_PPV_ARGS(&dred)); FAILED(HR))
			FK_LOG_ERROR("Failed to enable Dred!");
#endif

		if(auto HR = Device->QueryInterface(IID_PPV_ARGS(&pDevice14)); FAILED(HR))
			FK_LOG_ERROR("Device fails feature request!");

		if(Debug)
			Device->QueryInterface(IID_PPV_ARGS(&DebugDevice));

		D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
		pDevice14->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));

		D3D12_FEATURE_DATA_D3D12_OPTIONS2 options2 = {};
		pDevice14->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS2, &options2, sizeof(options2));

		D3D12_FEATURE_DATA_D3D12_OPTIONS3 options3 = {};
		pDevice14->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS3, &options3, sizeof(options3));

		D3D12_FEATURE_DATA_D3D12_OPTIONS4 options4 = {};
		pDevice14->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS4, &options4, sizeof(options4));

		D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
		pDevice14->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));

		D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
		pDevice14->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6));

		D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
		pDevice14->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7));

		D3D12_FEATURE_DATA_D3D12_OPTIONS8 options8 = {};
		pDevice14->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS8, &options8, sizeof(options8));

		D3D12_FEATURE_DATA_D3D12_OPTIONS9 options9 = {};
		pDevice14->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS9, &options9, sizeof(options9));

		D3D12_FEATURE_DATA_D3D12_OPTIONS10 options10 = {};
		pDevice14->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS10, &options10, sizeof(options10));

		D3D12_FEATURE_DATA_D3D12_OPTIONS11 options11 = {};
		pDevice14->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS11, &options11, sizeof(options11));

		D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12;
		pDevice14->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS12, &options12, sizeof(options12));

		D3D12_FEATURE_DATA_D3D12_OPTIONS13 options13;
		pDevice14->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS13, &options13, sizeof(options13));

		D3D12_FEATURE_DATA_D3D12_OPTIONS14 options14;
		pDevice14->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS14, &options14, sizeof(options14));

		D3D12_FEATURE_DATA_D3D12_OPTIONS15 options15;
		pDevice14->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS15, &options15, sizeof(options15));

		D3D12_FEATURE_DATA_D3D12_OPTIONS16 options16;
		pDevice14->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS16, &options16, sizeof(options16));

		D3D12_FEATURE_DATA_D3D12_OPTIONS17 options17;
		pDevice14->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS17, &options17, sizeof(options17));

		D3D12_FEATURE_DATA_D3D12_OPTIONS18 options18;
		pDevice14->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS18, &options18, sizeof(options18));

		D3D12_FEATURE_DATA_D3D12_OPTIONS19 options19;
		pDevice14->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS19, &options19, sizeof(options19));

		D3D12_FEATURE_DATA_D3D12_OPTIONS20 options20;
		pDevice14->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS20, &options20, sizeof(options20));

		D3D12_FEATURE_DATA_D3D12_OPTIONS21 options21;
		pDevice14->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS21, &options21, sizeof(options21));

		if (!options12.EnhancedBarriersSupported)
			FK_LOG_ERROR("Required Feature: 'Enhanced Barriers' not available.");

		switch (options5.RaytracingTier)
		{
		case D3D12_RAYTRACING_TIER_1_0:
			features.RT_Level = AvailableFeatures::Raytracing::RT_FeatureLevel_1;
			break;
		case D3D12_RAYTRACING_TIER_1_1:
			features.RT_Level = AvailableFeatures::Raytracing::RT_FeatureLevel_1_1;
			break;
		default:
			features.RT_Level = AvailableFeatures::Raytracing::RT_FeatureLevel_NOTAVAILABLE;
		}

		switch (options.ConservativeRasterizationTier)
		{
		case D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED:
			features.conservativeRast = AvailableFeatures::ConservativeRast_NOTAVAILABLE;
			break;
		default:
			features.conservativeRast = AvailableFeatures::ConservativeRast_AVAILABLE;
			break;
		};

		switch (options.ResourceHeapTier)
		{
		case D3D12_RESOURCE_HEAP_TIER_1:
			features.resourceHeapTier = ResourceHeapTier::HeapTier1;
			break;
		case D3D12_RESOURCE_HEAP_TIER_2:
			features.resourceHeapTier = ResourceHeapTier::HeapTier2;
			break;
		default:
			break;
		}

		switch (options21.ExecuteIndirectTier)
		{
			case D3D12_EXECUTE_INDIRECT_TIER_1_0:
				break;
			case D3D12_EXECUTE_INDIRECT_TIER_1_1:
				features.indirectLevel = AvailableFeatures::IndirectLevel_1_1;
				break;
		}

		switch (options21.WorkGraphsTier)
		{
			case D3D12_WORK_GRAPHS_TIER_NOT_SUPPORTED:
				break;
			case D3D12_WORK_GRAPHS_TIER_1_0:
				features.workGraph = AvailableFeatures::WorkGraphs_AVAILABLE;
				break;
		}


#if USING(AFTERMATH)
		auto res2 = GFSDK_Aftermath_EnableGpuCrashDumps(
			GFSDK_Aftermath_Version_API,
			GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_DX,
			GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks,   // Let the Nsight Aftermath library cache shader debug information.
			GpuCrashDumpCallback,                                               // Register callback for GPU crash dumps.
			nullptr,                                                            // Register callback for shader debug information.
			nullptr,                                                            // Register callback for GPU crash dump description.
			this);                                                              // Set the GpuCrashTracker object as user data for the above callbacks.

		auto res = GFSDK_Aftermath_DX12_Initialize(GFSDK_Aftermath_Version_API, GFSDK_Aftermath_FeatureFlags_Maximum, Device);

		if (res2)
			FK_LOG_INFO("Aftermath enabled");
		else
			FK_LOG_INFO("Aftermath disabled");
#endif

		ID3D12Fence* NewFence = nullptr;
		FK_ASSERT(FAILED(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&NewFence))), "FAILED TO CREATE FENCE!");
		SETDEBUGNAME(NewFence, "GRAPHICS FENCE");

		FK_LOG_9("GRAPHICS FENCE CREATED: %u", NewFence);

		directFence = NewFence;
		ObjectsCreated.push_back(NewFence);
		
		D3D12_COMMAND_QUEUE_DESC CQD		= {};
		CQD.Flags										= D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
		CQD.Type										= D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;

		D3D12_COMMAND_QUEUE_DESC ComputeCQD = {};
		ComputeCQD.Flags								= D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
		ComputeCQD.Type									= D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE;

		IDXGIFactory5*				DXGIFactory			= nullptr;
		IDXGIAdapter4*				DXGIAdapter			= nullptr;
		

		FK_ASSERT(FAILED(Device->CreateCommandQueue(&CQD,			IID_PPV_ARGS(&GraphicsQueue))), "FAILED TO CREATE COMMAND QUEUE!");
		FK_ASSERT(FAILED(Device->CreateCommandQueue(&ComputeCQD,	IID_PPV_ARGS(&ComputeQueue))), "FAILED TO CREATE COMMAND QUEUE!");

		ObjectsCreated.push_back(GraphicsQueue);
		ObjectsCreated.push_back(ComputeQueue);

		FK_LOG_9("GRAPHICS COMMAND QUEUE CREATED: %u", CQD);
		FK_LOG_9("GRAPHICS COMPUTE QUEUE CREATED: %u", ComputeCQD);

		SETDEBUGNAME(GraphicsQueue, "GRAPHICS QUEUE");
		SETDEBUGNAME(ComputeQueue,	"COMPUTE QUEUE");


		FINALLY
			if (!InitiateComplete)
			{
				for (auto O : ObjectsCreated)
					O->Release();
			}
		FINALLYOVER;

		if (FAILED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&hlslLibrary))))
			throw(std::runtime_error{ "Unable to create HLSL 6.x Library!" });

		if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&hlslCompiler))))
			throw(std::runtime_error{ "Unable to create HLSL 6.x Compiler!" });

		hlslLibrary->CreateIncludeHandler(&hlslIncludeHandler);

		// Create Resources
		const UINT DXGIFLAGS =
#if USING( DEBUGGRAPHICS )
			0;// DXGI_CREATE_FACTORY_DEBUG;
#else
			0;
#endif

		if (FAILED(CreateDXGIFactory2(DXGIFLAGS, IID_PPV_ARGS(&DXGIFactory))))
		{
			FK_LOG_ERROR("FAILED TO CREATE DXGIFactory!");
			return false;
		}

		DXGIFactory->EnumAdapterByLuid(Device->GetAdapterLuid(), IID_PPV_ARGS(&DXGIAdapter));

		DXGI_ADAPTER_DESC1 desc;
		DXGIAdapter->GetDesc1(&desc);

		switch (desc.VendorId)
		{
		case 0x1002:
			vendorID = DeviceVendor::AMD;
			break;
		case 0x10de:
			vendorID = DeviceVendor::NVIDIA;
			break;
		case 0x8086:
			vendorID = DeviceVendor::INTEL;
			break;
		default:
			vendorID = DeviceVendor::UNKNOWN;
		}	


		FINALLY
			if (!InitiateComplete)
				DXGIFactory->Release();
		FINALLYOVER

		// Copy temp resources over
		pDevice						= Device;
		pGIFactory					= DXGIFactory;
		pDXGIAdapter				= DXGIAdapter;
		pDebugDevice				= DebugDevice;
		pDebug						= Debug;
		BufferCount					= 3;
		directSubmissionCounter		= 0;
		DescriptorRTVSize			= Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		DescriptorDSVSize			= Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		DescriptorCBVSRVUAVSize		= Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		descriptorHeapAllocator.Initialize(*this, 1'000'000, in->Memory);
		heaps.Init(features.resourceHeapTier, pDevice);
		copyEngine.Initiate(Device, uint32_t((threads.GetThreadCount() + 1) * 1.5), ObjectsCreated, in->Memory);

		for (size_t I = 0; I < 3 * (1 + threads.GetThreadCount()); ++I)
			Contexts.emplace_back(this, Memory);

		InitiateComplete = true;
		
		rootLibrary.Initiate(this, *in->Memory, *in->TempMemory);
		ReadBackTable.Initiate(Device);

		FreeList_GraphicsQueue.Allocator	= in->Memory;
		FreeList_CopyQueue.Allocator		= in->Memory;
		DefaultTexture						= _CreateDefaultTexture();

		RegisterPSOLoader(CLEARBUFFERPSO, CreateClearBufferPSO);
		QueuePSOLoad(CLEARBUFFERPSO);

		directUploadBuffer = UploadBuffer(pDevice);

		SetDebugName(DefaultTexture, "Default Texture");

		return InitiateComplete;
	}


	/************************************************************************************************/


	void RenderSystem::Release()
	{
		if (!Memory)
			return;

		const size_t completedValue = directFence->GetCompletedValue();

		SyncDirectTo(SyncUploadTicket());
		WaitFor(SyncDirectTicket());

		FK_LOG_9("Releasing RenderSystem");

		for (auto& ctx : Contexts)
			ctx.Release();


		while (FreeList_GraphicsQueue.size() || FreeList_CopyQueue.size())
			Free_DelayedReleaseResources(this);

		FreeList_GraphicsQueue.Release();
		FreeList_CopyQueue.Release();
		copyEngine.Release();

		for (auto& FR : Contexts)
			FR.Release();

		Contexts.Release();
		ConstantBuffers.Release();
		VertexBuffers.Release();
		Textures.Release();
		PipelineStates.ReleasePSOs();
		ReadBackTable.Release();
		Queries.Release();
		directUploadBuffer.Release();
		heaps.Release();
		rootSignatures.Release();

		if(GraphicsQueue)	GraphicsQueue->Release();
		if(ComputeQueue)	ComputeQueue->Release();
		if(pGIFactory)		pGIFactory->Release();
		if(pDXGIAdapter)	pDXGIAdapter->Release();
		if(pDevice)			pDevice->Release();
		if(directFence)		directFence->Release();

#if USING(DEBUGGRAPHICS)
		// Prints Detailed Report
		if (pDebugDevice && pDebug)
		{
			pDebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
			pDebugDevice->Release();
			pDebug->Release();
		}
#endif

		Memory = nullptr;
	}


	/************************************************************************************************/


	const IPipelineState* RenderSystem::GetPSO(PSOHandle StateID, iAllocator& temp)
	{
		return PipelineStates.GetPSO(StateID, temp);
	}


	/************************************************************************************************/


	const IRootSignature * const RenderSystem::GetPSORootSignature(PSOHandle handle) const
	{
		return PipelineStates.GetPSORootSig(handle);
	}


	/************************************************************************************************/

	std::tuple<IPipelineState*, const IRootSignature*> RenderSystem::GetPSOAndRootSignature(PSOHandle handle, iAllocator& temp) const
	{
		auto object_ptr = PipelineStates.GetPSOObject(handle);
		object_ptr->WaitForLoad(temp);

		return { &object_ptr->PSO, object_ptr->rootSignature };
	}


	void RenderSystem::BuildLibrary(PSOHandle State, const PipelineStateLibraryDesc desc)
	{

		FK_ASSERT(0);
		//CompileShader();
	}


	/************************************************************************************************/


	void RenderSystem::RegisterPSOLoader(PSOHandle State, LOADSTATE_FN fn)
	{
		PipelineStates.RegisterPSOLoader(State, std::move(fn));
	}


	/************************************************************************************************/


	void RenderSystem::LoadPSOIfRequired(PSOHandle state)
	{
		auto obj = PipelineStates.GetPSOObject(state);

		if (obj && obj->state != PipelineStateObject::PSO_States::Loaded)
			QueuePSOLoad(state);
	}


	/************************************************************************************************/


	void RenderSystem::QueuePSOLoad(PSOHandle State)
	{
		FK_LOG_2("Reloading PSO!");

		PipelineStates.QueuePSOLoad(State, Memory);
	}


	/************************************************************************************************/


	size_t RenderSystem::GetCurrentCounter()
	{
		return directSubmissionCounter.load(std::memory_order_relaxed);
	}


	/************************************************************************************************/


	void RenderSystem::_UpdateSubResources(ResourceHandle handle, ID3D12Resource** resources, const size_t size)
	{
		Textures.ReplaceResources(handle, resources, size);
	}


	/************************************************************************************************/


	void RenderSystem::WaitForGPU()
	{
		const size_t completedValue	= directFence->GetCompletedValue();

		if (completedValue < directSubmittedCounter)
		{
			const size_t currentCounter = directSubmittedCounter;

			const HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS); FK_ASSERT(eventHandle != 0);
			directFence->SetEventOnCompletion(currentCounter, eventHandle);

			while (Textures.FreeDelayedResourcesIncrementally(directFence->GetCompletedValue()))
			{
				if (WaitForSingleObject(eventHandle, 0) == WAIT_OBJECT_0)
				{
					CloseHandle(eventHandle);
					return;
				}
			}

			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}


	/************************************************************************************************/


	void RenderSystem::WaitFor(const SyncPoint& sp)
	{
		WaitFor(sp.syncCounter);
	}


	/************************************************************************************************/


	void RenderSystem::WaitFor(const uint64_t counter)
	{
		uint32_t stallCounter = 0;
		while (true)
		{
			ProfileFunctionTextName(GPU_WAIT);
#ifdef _DEBUG
			if (stallCounter == 100)
				FK_LOG_ERROR("Stuck waiting for: %z\n", counter);
#endif

			const size_t completedValue = directFence->GetCompletedValue();
			if (completedValue < counter)
			{
				GraphicsQueue->Signal(directFence, counter);

				const HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS); FK_ASSERT(eventHandle != 0);
				directFence->SetEventOnCompletion(counter, eventHandle);

				while (Textures.FreeDelayedResourcesIncrementally(directFence->GetCompletedValue()))
				{
					if (WaitForSingleObject(eventHandle, 0) == WAIT_OBJECT_0)
					{
						CloseHandle(eventHandle);
						return;
					}
				}

				WaitForSingleObject(eventHandle, 33);
				CloseHandle(eventHandle);
			}
			else
				return;

			stallCounter++;
		}
	}


	/************************************************************************************************/


	void RenderSystem::SetDebugName(ResourceHandle handle, const char* str)
	{
		Textures.SetDebugName(handle, str);
	}


	/************************************************************************************************/


	void RenderSystem::SetDebugName(DeviceHeapHandle heap, const char* str)
	{
		SETDEBUGNAME(heaps.GetDeviceResource(heap), str);
	}


	/************************************************************************************************/


	D3D12_GPU_VIRTUAL_ADDRESS RenderSystem::GetVertexBufferAddress(const VertexBufferHandle VB)
	{
		return VertexBuffers.GetAsset(VB)->GetGPUVirtualAddress();
	}


	/************************************************************************************************/


	size_t RenderSystem::GetVertexBufferSize(const VertexBufferHandle VB) const noexcept
	{
		return VertexBuffers.GetBufferSize(VB);
	}


	/************************************************************************************************/


	void RenderSystem::MarkTextureUsed(ResourceHandle Handle)
	{
		Textures.MarkRTUsed(Handle);
	}


	/************************************************************************************************/

	DevicePointer RenderSystem::GetDevicePointer(const ResourceHandle resourceHandle) const noexcept
	{
		auto deviceResource = GetDeviceResource(resourceHandle);
		return deviceResource->GetGPUVirtualAddress();
	}


	DeviceAddressRange	RenderSystem::GetDeviceRange(const ResourceHandle resource) const noexcept
	{
		auto deviceResource = GetDeviceResource(resource);
		auto resourceSize	= GetResourceSize(resource);
		auto gpuAddress		= deviceResource->GetGPUVirtualAddress();

		return { gpuAddress, resourceSize };
	}


	/************************************************************************************************/


	DeviceAddressRange	RenderSystem::GetDeviceRange(const ConstantBufferHandle handle) const noexcept
	{
		DeviceResource_ptr	resource		= GetDeviceResource(handle);
		const size_t		resourceSize	= ConstantBuffers.GetBufferSize(handle);
		D3D12_GPU_VIRTUAL_ADDRESS ptr		= resource->GetGPUVirtualAddress();

		return DeviceAddressRange{
			.address	= ptr,
			.size		= resourceSize
		};
	}


	/************************************************************************************************/


	size_t RenderSystem::GetResourceSize(ConstantBufferHandle handle) const noexcept
	{
		return ConstantBuffers.GetBufferOffset(handle);
	}


	size_t RenderSystem::GetResourceSize(ResourceHandle handle) const noexcept
	{
		return Textures.GetResourceSize(handle);
	}


	/************************************************************************************************/


	size_t    RenderSystem::GetAllocationSize(ResourceHandle handle) const noexcept
	{
		auto resource				= GetDeviceResource(handle);
		D3D12_RESOURCE_DESC desc	= resource->GetDesc();
		auto resourceInfo			= pDevice->GetResourceAllocationInfo(0, 1, &desc);

		return resourceInfo.SizeInBytes;
	}


	size_t    RenderSystem::GetAllocationSize(GPUResourceDesc desc) const noexcept
	{
		const D3D12_RESOURCE_DESC Resource_DESC = GetD3D12ResourceDesc(desc);
		auto res = pDevice->GetResourceAllocationInfo(0, 1, &Resource_DESC);

		return res.SizeInBytes;
	}


	/************************************************************************************************/


	size_t	RenderSystem::GetTextureElementSize(ResourceHandle handle) const
	{
		auto Format = Textures.GetFormat(handle);
		return GetFormatElementSize(Format);
	}


	/************************************************************************************************/


	uint2	RenderSystem::GetTextureWH(ResourceHandle handle) const
	{
		if (handle == InvalidHandle)
			return { 0, 0 };
		else
			return Textures.GetWH(handle);
	}


	/************************************************************************************************/


	/*
	const uint2	RenderSystem::GetTextureWH(ResourceHandle Handle) const
	{
		return Texture2DUAVs.GetExtra(Handle).WH;
	}
	*/

	/************************************************************************************************/


	DeviceFormat RenderSystem::GetTextureFormat(ResourceHandle handle) const
	{
		return DXGIFormat2TextureFormat(Textures.GetFormat(handle));
	}


	/************************************************************************************************/


	DXGI_FORMAT RenderSystem::GetTextureDeviceFormat(ResourceHandle handle) const
	{
		return Textures.GetFormat(handle);
	}


	/************************************************************************************************/


	TextureDimension RenderSystem::GetTextureDimension(ResourceHandle handle) const
	{
		return Textures.GetDimension(handle);
	}


	/************************************************************************************************/


	size_t RenderSystem::GetTextureArraySize(ResourceHandle handle) const
	{
		return Textures.GetArraySize(handle);
	}


	/************************************************************************************************/


	uint8_t RenderSystem::GetTextureMipCount(ResourceHandle handle) const
	{
		return Textures.GetMIPCount(handle);
	}


	/************************************************************************************************/


	uint2 RenderSystem::GetTextureTilingWH(ResourceHandle handle, const uint mipLevel) const
	{
		auto WH = Textures.GetWH(handle);
		WH[0]   = WH[0] >> mipLevel;
		WH[1]   = WH[1] >> mipLevel;

		const auto tileSize = GetFormatTileSize(GetTextureFormat(handle));

		return WH / tileSize;
	}


	/************************************************************************************************/


	uint2 RenderSystem::GetHeapOffset(ResourceHandle handle, uint subResourceID) const
	{
		return Textures.GetHeapOffset(handle, subResourceID);
	}


	/************************************************************************************************/


	void RenderSystem::UploadTexture(ResourceHandle handle, CopyContextHandle queue, std::byte* buffer, size_t bufferSize)
	{
		auto resource	= GetDeviceResource(handle);
		auto wh			= GetTextureWH(handle);
		auto formatSize = GetTextureElementSize(handle); FK_ASSERT(formatSize != -1);
		auto format     = GetTextureFormat(handle);

		size_t resourceSize = bufferSize;
		size_t offset       = 0;

		TextureBuffer textureBuffer{ wh, buffer, bufferSize, formatSize, nullptr };

		SubResourceUpload_Desc desc;
		desc.buffers            = &textureBuffer;
		desc.subResourceCount   = 1;
		desc.subResourceStart   = 0;
		desc.format             = format;

		_UpdateSubResourceByUploadQueue(this, queue, resource, &desc);

		Textures.SetLayout(handle, DeviceLayout::Common);
	}


	/************************************************************************************************/


	void RenderSystem::UploadTexture(ResourceHandle handle, CopyContextHandle queue, TextureBuffer* buffers, size_t resourceCount) // Uses Upload Queue
	{
		auto resource	= GetDeviceResource(handle);
		auto format		= GetTextureFormat(handle);

		SubResourceUpload_Desc desc;
		desc.buffers			= buffers;
		desc.subResourceCount	= resourceCount;
		desc.format				= format;

		_UpdateSubResourceByUploadQueue(this, queue, resource, &desc);
	}


	/************************************************************************************************/


	D3D12_GPU_VIRTUAL_ADDRESS RenderSystem::GetConstantBufferAddress(const ConstantBufferHandle CB)
	{
		// TODO: deal with Push Buffer Offsets
		return ConstantBuffers.GetDeviceResource(CB)->GetGPUVirtualAddress();
	}


	/************************************************************************************************/


	BLAS_PreBuildInfo RenderSystem::GetBLASPreBuildInfo(const IVertexBufferSet& vertexBufferSet) const noexcept
	{
		uint8_t	indexBufferIdx	= vertexBufferSet.GetIndexBufferIndex();
		auto indexBuffer		= vertexBufferSet[indexBufferIdx];
		auto positionRes		= vertexBufferSet.Find(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION);

		if (!positionRes.has_value())
			return {};

		auto& positionBuffer = positionRes.value();

		D3D12_RAYTRACING_GEOMETRY_DESC desc;
		desc.Type   = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		desc.Flags  = D3D12_RAYTRACING_GEOMETRY_FLAGS::D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		desc.Triangles.IndexFormat  = DXGI_FORMAT_R32_UINT;
		desc.Triangles.IndexBuffer  = indexBuffer.resource->GetGPUVirtualAddress();
		desc.Triangles.IndexCount   = (UINT)indexBuffer.Size();

		desc.Triangles.VertexFormat					= DXGI_FORMAT_R32G32B32_FLOAT;
		desc.Triangles.VertexBuffer.StartAddress	= positionBuffer.resource->GetGPUVirtualAddress();
		desc.Triangles.VertexBuffer.StrideInBytes	= positionBuffer.byteStride;
		desc.Triangles.VertexCount					= (UINT)positionBuffer.byteSize;


		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs;
		inputs.DescsLayout      = D3D12_ELEMENTS_LAYOUT::D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.Type             = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		inputs.pGeometryDescs   = &desc;
		inputs.NumDescs         = 1u;
		inputs.Flags            = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
		pDevice14->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

		return {
			.BLAS_byteSize          = info.ResultDataMaxSizeInBytes,
			.scratchPad_byteSize    = info.ScratchDataSizeInBytes,
			.update_byteSize        = info.UpdateScratchDataSizeInBytes
		};
	}


	/************************************************************************************************/


	size_t RenderSystem::GetTextureFrameGraphIndex(ResourceHandle Texture) noexcept
	{
		return Textures.GetFrameGraphIndex(Texture, directSubmissionCounter);
	}


	/************************************************************************************************/


	void RenderSystem::SetTextureFrameGraphIndex(ResourceHandle Texture, size_t Index) noexcept
	{
		Textures.SetFrameGraphIndex(Texture, directSubmissionCounter, Index);
	}


	/************************************************************************************************/


	DeviceHeapHandle  RenderSystem::CreateHeap(const size_t heapSize, const uint32_t flags)
	{
		return heaps.CreateHeap(heapSize, flags);
	}


	/************************************************************************************************/


	ConstantBufferHandle RenderSystem::CreateConstantBuffer(size_t BufferSize, bool GPUResident)
	{
		return ConstantBuffers.CreateConstantBuffer((uint32_t)BufferSize, GPUResident);
	}


	/************************************************************************************************/


	VertexBufferHandle RenderSystem::CreateVertexBuffer(size_t BufferSize, bool GPUResident)
	{
		return VertexBuffers.CreateVertexBuffer(BufferSize, GPUResident, this);
	}


	/************************************************************************************************/


	ResourceHandle RenderSystem::CreateDepthBuffer( const uint2 WH, const bool UseFloat, const size_t bufferCount)
	{
		auto resourceDesc			= GPUResourceDesc::DepthTarget(WH, UseFloat ? DeviceFormat::D32_FLOAT : DeviceFormat::D24_UNORM_S8_UINT);
		resourceDesc.bufferCount	= (uint8_t)bufferCount;

		auto resource = CreateGPUResource(resourceDesc);
		SetDebugName(resource, "DepthBuffer");

		return resource;
	}


	/************************************************************************************************/


	ResourceHandle RenderSystem::CreateDepthBufferArray(
		const uint2						WH,
		const bool						UseFloat,
		const size_t					arraySize,
		const bool						buffered,
		const ResourceAllocationType	allocationType)
	{
		auto desc			= GPUResourceDesc::DepthTarget(WH, UseFloat ? DeviceFormat::D32_FLOAT : DeviceFormat::D24_UNORM_S8_UINT);
		desc.arraySize		= (uint8_t)arraySize;
		desc.allocationType	= allocationType;

		auto resource	= CreateGPUResource(desc);
		SetDebugName(resource, "DepthBufferArray");

		return resource;
	}


	/************************************************************************************************/


	ResourceHandle RenderSystem::CreateGPUResource(const GPUResourceDesc& desc)
	{
		ProfileFunction();

		if (desc.PreCreated)
		{
			return Textures.AddResource(desc, desc.initialLayout);
		}
		else
		{
			size_t byteSize							= CalculateByteSize(desc);

			D3D12_HEAP_PROPERTIES heapProperties	={};
			heapProperties.CPUPageProperty			= D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProperties.Type						= D3D12_HEAP_TYPE_DEFAULT;
			heapProperties.MemoryPoolPreference		= D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
			heapProperties.CreationNodeMask			= 0;
			heapProperties.VisibleNodeMask			= 0;

			const auto flags	=
				D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES |
				(desc.type == ResourceType::UnorderedAccess ? D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS : D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE) |
				(desc.type == ResourceType::UnorderedAccessRenderTarget ? D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS : D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE);

			const D3D12_CLEAR_VALUE clearValue = desc.clearValue.has_value() ? ClearValue2DXClearValue(desc.clearValue.value()) : D3D12_CLEAR_VALUE{};
			
			D3D12_BARRIER_LAYOUT initialLayout = DeviceLayout2DX(desc.initialLayout);
			ID3D12Resource* NewResource[3]		= { nullptr, nullptr, nullptr };

			FK_ASSERT(desc.bufferCount <= 3);

			FK_LOG_9("Creating Texture!");

			const size_t end = desc.bufferCount;
			for (size_t itr = 0; itr < end; ++itr)
			{
				switch(desc.allocationType)
				{
				case ResourceAllocationType::Tiled:
				{
					ProfileFunctionLabeled(Tiled);
					D3D12_RESOURCE_DESC Resource_DESC = GetD3D12ResourceDesc(desc);

					Resource_DESC.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
					HRESULT HR = pDevice14->CreateReservedResource2(
									&Resource_DESC,
									initialLayout,
									desc.clearValue.has_value() ? &clearValue : nullptr,
									nullptr,
									0,
									nullptr,
									IID_PPV_ARGS(&NewResource[itr]));

					if (FAILED(HR))
					{
						FK_LOG_ERROR("FAILED TO CREATE VIRTUAL MEMORY FOR TEXTURE");
						_OnCrash();
					}
				}	break;
				case ResourceAllocationType::Committed:
				{
					ProfileFunctionLabeled(Committed);
					D3D12_RESOURCE_DESC1 Resource_DESC = GetD3D12ResourceDesc1(desc);

					auto HR = pDevice14->CreateCommittedResource3(
						&heapProperties,
						flags,
						&Resource_DESC,
						initialLayout,
						desc.clearValue.has_value() ? &clearValue : nullptr,
						nullptr, // protected sessction,
						0, // castable formats,
						nullptr,
						IID_PPV_ARGS(&NewResource[itr]));

					if (FAILED(HR))
					{
						FK_LOG_ERROR("FAILED TO COMMIT MEMORY FOR TEXTURE");
						_OnCrash();
					}
				}   break;
				case ResourceAllocationType::Placed:
				{
					//static std::mutex m;
					//std::unique_lock lock{ m };

					ProfileFunctionLabeled(Placed);
					D3D12_RESOURCE_DESC1 Resource_DESC = GetD3D12ResourceDesc1(desc);

					HRESULT HR = pDevice14->CreatePlacedResource2(
						desc.placed.heap != InvalidHandle ? GetDeviceResource(desc.placed.heap) : desc.placed.customHeap,
						desc.placed.offset,
						&Resource_DESC,
						initialLayout,
						desc.clearValue.has_value() ? &clearValue : nullptr,
						0,
						nullptr,
						IID_PPV_ARGS(&NewResource[itr]));

					if (FAILED(HR))
					{
						FK_LOG_ERROR("FAILED TO CREATE PLACED RESOURCE");
						_OnCrash();
					}
				}   break;
				}
				FK_ASSERT(NewResource[itr], "Failed to Create Texture!");
				SETDEBUGNAME(NewResource[itr], __func__);
			}

			auto filledDesc = desc;

			filledDesc.resources	= NewResource;
			filledDesc.byteSize		= byteSize;

			return Textures.AddResource(filledDesc, filledDesc.initialLayout);
		}

		return InvalidHandle;
	}


	/************************************************************************************************/


	void RenderSystem::BackResource(ResourceHandle handle, const GPUResourceDesc& desc) noexcept
	{
		ProfileFunction();

		size_t byteSize						= CalculateByteSize(desc);
		D3D12_RESOURCE_DESC1 Resource_DESC	= GetD3D12ResourceDesc1(desc);

		D3D12_HEAP_PROPERTIES HEAP_Props	= {};
		HEAP_Props.CPUPageProperty			= D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type						= D3D12_HEAP_TYPE_DEFAULT;
		HEAP_Props.MemoryPoolPreference		= D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask			= 0;
		HEAP_Props.VisibleNodeMask			= 0;

		const auto flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES | (desc.type == ResourceType::UnorderedAccess ? D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS : D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE);

		const D3D12_CLEAR_VALUE clearValue = desc.clearValue.has_value() ? ClearValue2DXClearValue(desc.clearValue.value()) : D3D12_CLEAR_VALUE{};

		D3D12_BARRIER_LAYOUT initialLayout = DeviceLayout2DX(desc.initialLayout);

		ID3D12Resource* NewResource[3] = { nullptr, nullptr, nullptr };

		FK_ASSERT(desc.bufferCount <= 3);

		FK_LOG_9("Creating Texture!");


		const size_t end = desc.bufferCount;
		for (size_t itr = 0; itr < end; ++itr)
		{
			switch(desc.allocationType)
			{
			case ResourceAllocationType::Tiled:
			{
				DebugBreak();

				/*
				ProfileFunctionLabeled(Tiled);

				Resource_DESC.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
					
				HRESULT HR = pDevice->CreateReservedResource(
								&Resource_DESC,
								InitialState,
								pCV,
								IID_PPV_ARGS(&NewResource[itr]));

				CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE VIRTUAL MEMORY FOR TEXTURE"));
				*/
			}   break;
			case ResourceAllocationType::Committed:
			{
				DebugBreak();

				/*
				ProfileFunctionLabeled(Committed);

				HRESULT HR = pDevice->CreateCommittedResource(
								&HEAP_Props, 
								flags,
								&Resource_DESC, InitialState, pCV, IID_PPV_ARGS(&NewResource[itr]));

				CheckHR(HR, ASSERTONFAIL("FAILED TO COMMIT MEMORY FOR TEXTURE"));
				*/
			}   break;
			case ResourceAllocationType::Placed:
			{
				ProfileFunctionLabeled(Placed);

				HRESULT HR = pDevice14->CreatePlacedResource2(
					desc.placed.heap != InvalidHandle ? GetDeviceResource(desc.placed.heap) : desc.placed.customHeap,
					desc.placed.offset,
					&Resource_DESC,
					initialLayout,
					desc.clearValue.has_value() ? &clearValue : nullptr,
					0,
					nullptr,
					IID_PPV_ARGS(&NewResource[itr]));

				CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE PLACED RESOURCE"));

				if (FAILED(HR))
					_OnCrash();
			}   break;
			}
			FK_ASSERT(NewResource[itr], "Failed to Create Texture!");
			SETDEBUGNAME(NewResource[itr], __func__);
		}

		auto filledDesc     = desc;

		filledDesc.resources    = NewResource;
		filledDesc.byteSize     = byteSize;

		Textures.SetResource(handle, filledDesc, filledDesc.initialLayout);
	}


	/************************************************************************************************/


	ResourceHandle RenderSystem::CreateGPUResourceHandle()
	{
		return Textures.GetFreeHandle();
	}


	/************************************************************************************************/


	QueryHandle	RenderSystem::CreateOcclusionBuffer(size_t Counts)
	{
		return Queries.CreateQueryBuffer(Counts, QueryType::BinaryOcclusionQuery);
	}


	/************************************************************************************************/


	ResourceHandle RenderSystem::CreateUAVBufferResource(size_t resourceSize, bool tripleBuffer)
	{
		auto desc = GPUResourceDesc::UAVResource(resourceSize);
		desc.bufferCount = tripleBuffer ? 3 : 1;

		UAVResourceLayout layout;
		layout.elementCount	= (UINT)(resourceSize / sizeof(uint32_t));// initial layout assume a uint buffer
		layout.format		= DXGI_FORMAT_UNKNOWN;
		layout.stride		= sizeof(uint32_t);

		auto handle = CreateGPUResource(desc);
		Textures.SetExtra(handle, layout);
		SetDebugName(handle, "CreateUAVBuffer");

		return handle;
	}


	/************************************************************************************************/


	ResourceHandle RenderSystem::CreateUAVTextureResource(const uint2 WH, const DeviceFormat format, const bool renderTarget)
	{
		auto desc			= GPUResourceDesc::UAVTexture(WH, format, renderTarget);
		desc.bufferCount	= 3;

		auto UAVresource = CreateGPUResource(desc);
		SetDebugName(UAVresource, "CreateUAVBuffer");

		return UAVresource;
	}


	/************************************************************************************************/


	SOResourceHandle RenderSystem::CreateStreamOutResource(size_t resourceSize, bool tripleBuffered)
	{
		D3D12_RESOURCE_DESC Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(resourceSize);
		Resource_DESC.Width		= resourceSize;
		Resource_DESC.Format	= DXGI_FORMAT_UNKNOWN;
		Resource_DESC.Flags		= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		D3D12_RESOURCE_DESC Counter_DESC = CD3DX12_RESOURCE_DESC::Buffer(resourceSize);
		Counter_DESC.Width		= 512;
		Counter_DESC.Format		= DXGI_FORMAT_UNKNOWN;
		Counter_DESC.Flags		= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		D3D12_HEAP_PROPERTIES HEAP_Props	= {};
		HEAP_Props.CPUPageProperty			= D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type						= D3D12_HEAP_TYPE_DEFAULT;
		HEAP_Props.MemoryPoolPreference		= D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;

		static_vector<ID3D12Resource*>	resources;
		static_vector<ID3D12Resource*>	counters;
		for (size_t I = 0; I < (tripleBuffered ? BufferCount : 1); ++I)
		{
			ID3D12Resource* Resource	= nullptr;
			ID3D12Resource* Counter		= nullptr;

			HRESULT HR;

			HR = pDevice->CreateCommittedResource(
								&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
								&Resource_DESC, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr,
								IID_PPV_ARGS(&Resource));
			CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE STREAMOUT RESOURCE!"));

			HR = pDevice->CreateCommittedResource(
								&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
								&Resource_DESC, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr,
								IID_PPV_ARGS(&Counter));

			CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE STREAMOUT RESOURCE!"));
			resources.push_back(Resource);
			counters.push_back(Counter);

			SETDEBUGNAME(Resource, "StreamOutResource" );
			SETDEBUGNAME(Counter, "StreamOutCounter" );
		}

		return StreamOutTable.AddResource(resources, counters, resourceSize, DeviceLayout::Common);
	}


	/************************************************************************************************/



	QueryHandle RenderSystem::CreateSOQuery(size_t SOIndex, size_t count)
	{
		return Queries.CreateSOQueryBuffer(count, SOIndex);
	}


	/************************************************************************************************/


	QueryHandle	RenderSystem::CreateTimeStampQuery(size_t count)
	{
		return Queries.CreateQueryBuffer(count, QueryType::TimeStats);
	}


	/************************************************************************************************/


	struct D3D12_DISPATCH_RAYS_DESC
	{
		D3D12_GPU_VIRTUAL_ADDRESS_RANGE				RayGenerationShaderRecord;
		D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE	MissShaderTable;
		D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE	HitGroupTable;
		D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE	CallableShaderTable;
		UINT Width;
		UINT Height;
		UINT Depth;
	} D3D12_DISPATCH_RAYS_DESC;


	IndirectLayout RenderSystem::CreateIndirectLayout(static_vector<IndirectDrawDescription> entries, iAllocator* allocator, const IRootSignature* irootSignatureID)
	{
		ID3D12CommandSignature* signature = nullptr;
		
		Vector<IndirectDrawDescription>				layout{allocator};
		static_vector<D3D12_INDIRECT_ARGUMENT_DESC> signatureEntries;

		size_t entryStride = 0;

		for (size_t itr = 0; itr < entries.size(); ++itr)
		{
			switch (entries[itr].type)
			{
				case ILE_DrawCall:
				{
					D3D12_INDIRECT_ARGUMENT_DESC desc = {};
					desc.Type   = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

					signatureEntries.push_back(desc);
					layout.push_back(ILE_DrawCall);
					entryStride += sizeof(uint32_t) * 4; // uses 4 x 4 byte values
				}	break;
				case ILE_DrawIndexedCall:
				{
					D3D12_INDIRECT_ARGUMENT_DESC desc = {};
					desc.Type	= D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

					signatureEntries.push_back(desc);
					layout.push_back(ILE_DrawCall);
					entryStride += sizeof(uint32_t) * 5; // uses 5 x 4 byte values
				}	break;
				case ILE_DispatchCall:
				{
					D3D12_INDIRECT_ARGUMENT_DESC desc = {};
					desc.Type   = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

					signatureEntries.push_back(desc);
					layout.push_back(ILE_DispatchCall);
					entryStride += sizeof(uint4); // uses 4 x 4 byte values
				}   break;
				case ILE_DispatchMesh:
				{
					D3D12_INDIRECT_ARGUMENT_DESC desc = {};
					desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;
					signatureEntries.push_back(desc);
					layout.push_back(ILE_DispatchCall);
					entryStride += sizeof(uint3); // uses 4 x 4byte values
				}   break;
				case ILE_DispatchRays:
				{
					D3D12_INDIRECT_ARGUMENT_DESC desc = {};
					desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS;
					signatureEntries.push_back(desc);
					layout.push_back(ILE_DispatchCall);
					entryStride += sizeof(D3D12_DISPATCH_RAYS_DESC);
				}   break;
				case ILE_RootDescriptorUINT:
				{
					D3D12_INDIRECT_ARGUMENT_DESC desc = {};
					desc.Type                               = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
					desc.Constant.DestOffsetIn32BitValues   = entries[itr].description.constantValue.destinationOffset;
					desc.Constant.Num32BitValuesToSet       = entries[itr].description.constantValue.numValues;
					desc.Constant.RootParameterIndex        = entries[itr].description.constantValue.rootParameterIdx;

					signatureEntries.push_back(desc);
					layout.push_back(ILE_RootDescriptorUINT);

					entryStride += desc.Constant.Num32BitValuesToSet * sizeof(uint32_t);
				}   break;
			}
		}

		ID3D12RootSignature* dxRootSig = nullptr;

		D3D12_COMMAND_SIGNATURE_DESC desc;
		desc.ByteStride			= (UINT)entryStride;
		desc.NumArgumentDescs	= (UINT)entries.size();
		desc.pArgumentDescs		= signatureEntries.begin();
		desc.NodeMask			= 0;

		auto rootSignatureID = static_cast<const RootSignature*>(irootSignatureID);

		auto HR = pDevice->CreateCommandSignature(
			&desc,
			rootSignatureID ? *rootSignatureID : nullptr,
			IID_PPV_ARGS(&signature));

		CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE CONSTANT BUFFER"));

		return { signature, entryStride, std::move(layout) };
	}


	/************************************************************************************************/


	void RenderSystem::SubmitTileMappings(std::span<ResourceHandle> resources, iAllocator* allocator)
	{
		FK_LOG_9("Updating tile mappings for frame: %u", directSubmissionCounter.load(std::memory_order_relaxed));

		for(const auto resource : resources)
		{
			FK_ASSERT(resource >= Textures.Handles.size(), "Invalid Handle Detected");

			auto& mappings		= Textures._GetTileMappings(resource);
			auto deviceResource	= GetDeviceResource(resource);
			const auto mipCount	= Textures.GetMIPCount(resource);

			Vector<D3D12_TILED_RESOURCE_COORDINATE>	coordinates	{ allocator };
			Vector<D3D12_TILE_REGION_SIZE>			regionSizes	{ allocator };
			Vector<D3D12_TILE_RANGE_FLAGS>			flags		{ allocator };
			Vector<UINT>							offsets		{ allocator };
			Vector<UINT>							tileRanges	{ allocator };
			DeviceHeapHandle						heap = InvalidHandle;


			auto nullEnd = std::partition(mappings.begin(), mappings.end(),
				[](auto& mapping)
				{
					return mapping.state == TileMapState::Null;
				});

			std::for_each(mappings.begin(), nullEnd,
				[&](auto& mapping)
				{
					const auto flag = D3D12_TILE_RANGE_FLAG_NULL;

					D3D12_TILED_RESOURCE_COORDINATE coordinate;
					coordinate.Subresource	= mapping.tileID.GetMipLevel();
					coordinate.X			= mapping.tileID.GetTileX();
					coordinate.Y			= mapping.tileID.GetTileY();
					coordinate.Z			= 0;

					if (mapping.tileID.bytes != -1)
					{
						coordinates.push_back(coordinate);
						offsets.push_back(mapping.heapOffset);
						flags.push_back(flag);
						tileRanges.push_back(1);
					}
					else
						FK_LOG_ERROR("Invalid virtual texture tileID received");
				});

			if (coordinates.size())
			{
				copyEngine.copyQueue->UpdateTileMappings(
					deviceResource,
					(UINT)coordinates.size(),
					coordinates.data(),
					nullptr,
					nullptr,
					(UINT)coordinates.size(),
					flags.data(),
					offsets.data(),
					tileRanges.data(),
					D3D12_TILE_MAPPING_FLAG_NONE);
			}

			coordinates.clear();
			regionSizes.clear();
			flags.clear();
			offsets.clear();
			tileRanges.clear();

			uint32_t I = (uint32_t)std::distance(mappings.begin(), nullEnd);
			while(I < mappings.size())
			{
				heap = mappings[I].heap;

				for (;mappings[I].heap == heap && I < mappings.size(); I++)
				{
					auto& mapping = mappings[I];

					mapping.state = TileMapState::InUse;

					const auto flag = D3D12_TILE_RANGE_FLAG_NONE;

					D3D12_TILED_RESOURCE_COORDINATE coordinate;
					coordinate.Subresource	= mapping.tileID.GetMipLevel();
					coordinate.X			= mapping.tileID.GetTileX();
					coordinate.Y			= mapping.tileID.GetTileY();
					coordinate.Z			= 0;

#if _DEBUG
					if(flag == D3D12_TILE_RANGE_FLAG_NONE)
						FK_LOG_9("Mapping Tile { %u, %u, %u MIP } to Offset: { %u }", coordinate.X, coordinate.Y, coordinate.Subresource, mapping.heapOffset );
					else
						FK_LOG_INFO("Unmapping Tile { %u, %u, %u MIP } from Offset: { %u }", coordinate.X, coordinate.Y, coordinate.Subresource, mapping.heapOffset);
#endif

					if (mapping.tileID.bytes != -1)
					{
						coordinates.push_back(coordinate);

						D3D12_TILE_REGION_SIZE regionSize;
						regionSize.Depth	= 1;
						regionSize.Height	= 1;
						regionSize.Width	= 1;

						regionSize.NumTiles	= 1;
						regionSize.UseBox	= false;

						regionSizes.push_back(regionSize);

						offsets.push_back(mapping.heapOffset);
						flags.push_back(flag);
						tileRanges.push_back(1);
					}
					else
						FK_LOG_ERROR("Invalid virtual texture tile received");
				}

				I++;

				if (heap != InvalidHandle)
				{
					D3D12_TILE_RANGE_FLAGS nullRangeFlag = D3D12_TILE_RANGE_FLAG_NULL;

					if (!coordinates.size())
					{
						//D3D12_TILE_RANGE_FLAGS rangeFlags = D3D12_TILE_RANGE_FLAG_NULL;
						//copyEngine.copyQueue->UpdateTileMappings(deviceResource, 1, NULL, NULL, NULL, 1, &rangeFlags, NULL, NULL, D3D12_TILE_MAPPING_FLAG_NONE);
					}
					else
					{
						copyEngine.copyQueue->UpdateTileMappings(
							deviceResource,
							(UINT)coordinates.size(),
							coordinates.data(),
							regionSizes.data(),
							GetDeviceResource(heap),
							(UINT)coordinates.size(),
							flags.data(),
							offsets.data(),
							tileRanges.data(),
							D3D12_TILE_MAPPING_FLAG_NONE);
					}
				}

				mappings.erase(
					std::remove_if(
						std::begin(mappings),
						std::end(mappings),
						[](TileMapping& mapping)
						{
							return mapping.state == TileMapState::Null;
						}),
					std::end(mappings));

				coordinates.clear();
				regionSizes.clear();
				flags.clear();
				offsets.clear();
				tileRanges.clear();
			}
		}

		FK_LOG_9("Completed file mapping update");
	}


	/************************************************************************************************/


	void RenderSystem::UpdateTextureTileMappings(const ResourceHandle handle, std::span<const TileMapping> tileMaps, iAllocator& temp)
	{
		Textures.UpdateTileMappings(handle, tileMaps.data(), tileMaps.data() + tileMaps.size(), temp);
	}


	/************************************************************************************************/


	const TileMapList& RenderSystem::GetTileMappings(const ResourceHandle handle)
	{
		return Textures.GetTileMappings(handle);
	}


	/************************************************************************************************/


	ReadBackResourceHandle  RenderSystem::CreateReadBackBuffer(const size_t bufferSize)
	{
		D3D12_RESOURCE_DESC Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
		Resource_DESC.Width     = bufferSize;
		Resource_DESC.Format    = DXGI_FORMAT_UNKNOWN;
		Resource_DESC.Flags     = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;

		D3D12_HEAP_PROPERTIES HEAP_Props = {};
		HEAP_Props.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type                 = D3D12_HEAP_TYPE_READBACK;
		HEAP_Props.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;

		ID3D12Resource* resource = nullptr;

		auto HR = pDevice->CreateCommittedResource(
			&HEAP_Props,
			D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
			&Resource_DESC,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&resource));

		SETDEBUGNAME(resource, "ReadBackHeap");

		return ReadBackTable.AddReadBack(bufferSize, resource);
	}


	/************************************************************************************************/


	SubAllocation RenderSystem::ReserveConstantBuffer(ConstantBufferHandle CB, size_t reserveSize)	noexcept
	{
		return ConstantBuffers.Reserve(CB, reserveSize);
	}


	SubAllocation RenderSystem::ReserveVertexBuffer(VertexBufferHandle VB, size_t reserveSize)		noexcept
	{
		return VertexBuffers.Reserve(VB, reserveSize);
	}


	/************************************************************************************************/


	void RenderSystem::SetReadBackEvent(ReadBackResourceHandle readbackBuffer, ReadBackEventHandler&& handler)
	{
		ReadBackTable.SetCallback(readbackBuffer, std::move(handler));
	}


	/************************************************************************************************/


	std::pair<void*, size_t> RenderSystem::OpenReadBackBuffer(ReadBackResourceHandle readbackBuffer, const size_t readSize)
	{
		auto temp = ReadBackTable.OpenBufferData(readbackBuffer, readSize);
		return { temp.buffer, temp.bufferSize };
	}


	/************************************************************************************************/


	void RenderSystem::CloseReadBackBuffer(ReadBackResourceHandle readbackBuffer)
	{
		ReadBackTable.CloseBufferData(readbackBuffer);
	}


	/************************************************************************************************/


	void RenderSystem::FlushPendingReadBacks()
	{
		ReadBackTable.Update();
	}


	/************************************************************************************************/


	void RenderSystem::SetObjectLayout(SOResourceHandle handle, DeviceLayout layout) noexcept
	{
		StreamOutTable.SetLayout(handle, layout);
	}


	/************************************************************************************************/


	void RenderSystem::SetObjectLayout(ResourceHandle handle, DeviceLayout layout) noexcept
	{
		Textures.SetLayout(handle, layout);
	}


	/************************************************************************************************/


	DeviceLayout RenderSystem::GetObjectLayout(const QueryHandle handle) const noexcept
	{
		return Queries.GetLayout(handle);
	}


	/************************************************************************************************/


	DeviceLayout RenderSystem::GetObjectLayout(const SOResourceHandle handle) const noexcept
	{
		return StreamOutTable.GetLayout(handle);
	}


	/************************************************************************************************/


	DeviceLayout RenderSystem::GetObjectLayout(const ResourceHandle handle) const noexcept
	{
		return Textures.GetLayout(handle);
	}


	/************************************************************************************************/


	PackedResourceTileInfo RenderSystem::GetPackedTileInfo(ID3D12Resource* resource) const
	{
		UINT						TileCount = 0;
		D3D12_PACKED_MIP_INFO		packedMipInfo;
		D3D12_TILE_SHAPE			TileShape;
		UINT						subResourceTilingCount = 1;
		D3D12_SUBRESOURCE_TILING	subResourceTiling_Packed;

		pDevice->GetResourceTiling(resource, &TileCount, &packedMipInfo, &TileShape, &subResourceTilingCount, 0, &subResourceTiling_Packed);

		return {
			size_t(packedMipInfo.NumStandardMips),
			size_t(packedMipInfo.NumStandardMips + packedMipInfo.NumPackedMips),
			packedMipInfo.StartTileIndexInOverallResource };
	}


	/************************************************************************************************/


	PackedResourceTileInfo RenderSystem::GetPackedTileInfo(ResourceHandle resource)	const
	{
		UINT						tileCount = 0;
		D3D12_PACKED_MIP_INFO		packedMipInfo;
		D3D12_TILE_SHAPE			tileShape;
		UINT						subResourceTilingCount = 1;
		D3D12_SUBRESOURCE_TILING	subResourceTiling_Packed;

		pDevice->GetResourceTiling(GetDeviceResource(resource), &tileCount, &packedMipInfo, &tileShape, &subResourceTilingCount, 0, &subResourceTiling_Packed);

		return {
			size_t(packedMipInfo.NumStandardMips),
			size_t(packedMipInfo.NumStandardMips + packedMipInfo.NumPackedMips),
			packedMipInfo.StartTileIndexInOverallResource };
	}


	/************************************************************************************************/


	ID3D12Heap* RenderSystem::GetDeviceResource(const DeviceHeapHandle handle) const
	{
		return heaps.GetDeviceResource(handle);
	}


	ID3D12Resource* RenderSystem::GetDeviceResource(const ReadBackResourceHandle handle) const
	{
		return ReadBackTable.GetDeviceResource(handle);
	}


	/************************************************************************************************/


	ID3D12Resource* RenderSystem::GetDeviceResource(const ConstantBufferHandle handle) const
	{
		return ConstantBuffers.GetDeviceResource(handle);
	}


	/************************************************************************************************/


	ID3D12Resource* RenderSystem::GetDeviceResource(const ResourceHandle handle) const
	{
		if (handle != InvalidHandle)
			return Textures.GetResource(handle, pDevice);
		else
			return nullptr;
	}


	/************************************************************************************************/


	ID3D12Resource*	RenderSystem::GetDeviceResource(const SOResourceHandle handle) const
	{
		return StreamOutTable.GetAsset(handle);
	}


	/************************************************************************************************/


	ID3D12Resource*	RenderSystem::GetSOCounterResource(const SOResourceHandle handle) const
	{
		return StreamOutTable.GetAssetCounter(handle);
	}



	/************************************************************************************************/


	size_t RenderSystem::GetStreamOutBufferSize(const SOResourceHandle handle) const
	{
		return StreamOutTable.GetAssetSize(handle);
	}


	/************************************************************************************************/


	size_t	RenderSystem::GetVertexBufferOffset(const VertexBufferHandle handle) const
	{
		return VertexBuffers.GetCurrentVertexBufferOffset(handle);
	}


	/************************************************************************************************/


	bool RenderSystem::VertexBufferPush(VertexBufferHandle buffer, void* _ptr, size_t elementSize)
	{
		return VertexBuffers.PushVertex(buffer, _ptr, elementSize);
	}


	/************************************************************************************************/


	size_t RenderSystem::ConstantBufferAlign(ConstantBufferHandle cb) noexcept
	{
		return ConstantBuffers.AlignNext(cb);
	}


	/************************************************************************************************/


	UAVResourceLayout RenderSystem::GetUAVBufferLayout(const ResourceHandle handle) const noexcept
	{
		auto extra  = Textures.GetExtra(handle);
		auto temp   = std::get_if<UAVResourceLayout>(&extra);

		return temp ? *temp : UAVResourceLayout{};
	}


	/************************************************************************************************/


	void RenderSystem::SetUAVBufferLayout(const ResourceHandle handle, const UAVResourceLayout newConfig) noexcept
	{
		Textures.SetExtra(handle, newConfig);
	}


	/************************************************************************************************/


	size_t RenderSystem::GetUAVBufferSize(const ResourceHandle handle) const noexcept
	{
		return Textures.GetResourceSize(handle);
	}


	/************************************************************************************************/


	size_t RenderSystem::GetHeapSize(const DeviceHeapHandle heap) const
	{
		return heaps.GetHeapSize(heap);
	}


	/************************************************************************************************/


	AvailableFeatures::Raytracing RenderSystem::GetRTFeatureLevel() const noexcept
	{
		return features.RT_Level;
	}


	/************************************************************************************************/


	bool RenderSystem::RTAvailable() const noexcept
	{
		return (features.RT_Level != AvailableFeatures::Raytracing::RT_FeatureLevel_NOTAVAILABLE);
	}


	/************************************************************************************************/


	void RenderSystem::ResetConstantBuffer(ConstantBufferHandle constantBuffer)
	{
		ConstantBuffers.Reset(constantBuffer);
	}


	/************************************************************************************************/


	void RenderSystem::ResetVertexBuffer(VertexBufferHandle constant)
	{
		VertexBuffers.Reset(constant);
	}


	/************************************************************************************************/


	void RenderSystem::ResetQuery(QueryHandle handle)
	{
		Queries.LockUntil(handle, directSubmissionCounter);
	}


	/************************************************************************************************/


	void RenderSystem::ReleaseCB(ConstantBufferHandle Handle)
	{
		ConstantBuffers.ReleaseBuffer(Handle);
	}


	/************************************************************************************************/


	void RenderSystem::ReleaseVB(VertexBufferHandle Handle)
	{
		VertexBuffers.ReleaseVertexBuffer(Handle, directSubmissionCounter);
	}


	/************************************************************************************************/


	void RenderSystem::ReleaseResource(ResourceHandle Handle)
	{
		Textures.ReleaseTexture(Handle, directSubmissionCounter);
	}


	/************************************************************************************************/


	void RenderSystem::ReleaseReadBack(ReadBackResourceHandle handle)
	{
		ReadBackTable.ReleaseResource(handle);
	}


	/************************************************************************************************/


	void RenderSystem::ReleaseHeap(DeviceHeapHandle heap)
	{
		heaps.ReleaseHeap(heap);
	}


	/************************************************************************************************/


	void RenderSystem::ReleaseQuery(QueryHandle q)
	{
		Queries.Release(q, directSubmissionCounter.load(std::memory_order_acquire));
	}


	/************************************************************************************************/


	void Push_DelayedRelease(RenderSystem* RS, ID3D12Resource* Res)
	{
		RS->FreeList_GraphicsQueue.push_back({ Res, RS->directSubmissionCounter });
	}


	/************************************************************************************************/


	void Push_DelayedReleaseCopy(RenderSystem* RS, ID3D12Resource* Res)
	{
		RS->FreeList_CopyQueue.push_back({ Res, RS->copyEngine.counter });
	}


	/************************************************************************************************/


	void Free_DelayedReleaseResources(RenderSystem* RS)
	{
		{
			auto completed = RS->directFence->GetCompletedValue();
			for (auto& R : RS->FreeList_GraphicsQueue)
				if (completed > R.Counter)
					SAFERELEASE(R.Resource);

			RS->FreeList_GraphicsQueue.erase(
				std::partition(
					RS->FreeList_GraphicsQueue.begin(),
					RS->FreeList_GraphicsQueue.end(),
					[](const auto& R) -> bool {return (R.Resource); }),
				RS->FreeList_GraphicsQueue.end());
		}

		{
			auto completed = RS->copyEngine.fence->GetCompletedValue();
			for (auto& R : RS->FreeList_CopyQueue)
				if (completed > R.Counter)
					SAFERELEASE(R.Resource);

			RS->FreeList_CopyQueue.erase(
				std::partition(
					RS->FreeList_CopyQueue.begin(),
					RS->FreeList_CopyQueue.end(),
					[](const auto& R) -> bool {return (R.Resource); }),
				RS->FreeList_CopyQueue.end());
		}
	}


	/************************************************************************************************/


	constexpr DXGI_FORMAT TextureFormat2DXGIFormat(DeviceFormat F) noexcept
	{
		;
		switch (F)
		{
		case FlexKit::DeviceFormat::R16_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT;
		case FlexKit::DeviceFormat::R16_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
		case FlexKit::DeviceFormat::R16G16_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R16G16_UINT;
		case DeviceFormat::R16G16B16A16_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT;
		case FlexKit::DeviceFormat::R32_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
		case FlexKit::DeviceFormat::R32G32_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R32G32_UINT;
		case FlexKit::DeviceFormat::R32G32_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
		case FlexKit::DeviceFormat::R32G32B32_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R32G32B32_UINT;
		case FlexKit::DeviceFormat::R32G32B32A32_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_UINT;
		case FlexKit::DeviceFormat::R8G8B8A8_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
		case FlexKit::DeviceFormat::R8G8B8A8_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case FlexKit::DeviceFormat::R8G8_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_R8G8_UNORM;
		case FlexKit::DeviceFormat::R16G16_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_R16G16_FLOAT;
		case FlexKit::DeviceFormat::R32G32B32_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
		case FlexKit::DeviceFormat::R16G16B16A16_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		case FlexKit::DeviceFormat::R32G32B32A32_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
		case FlexKit::DeviceFormat::R32_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		case FlexKit::DeviceFormat::D32_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
		case FlexKit::DeviceFormat::BC1_TYPELESS:
			return DXGI_FORMAT::DXGI_FORMAT_BC1_TYPELESS;
		case FlexKit::DeviceFormat::BC1_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM;
		case FlexKit::DeviceFormat::BC1_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM_SRGB;
		case FlexKit::DeviceFormat::BC2_TYPELESS:
			return DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM_SRGB;
		case FlexKit::DeviceFormat::BC2_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM;
		case FlexKit::DeviceFormat::BC2_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM_SRGB;
		case FlexKit::DeviceFormat::BC3_TYPELESS:
			return DXGI_FORMAT::DXGI_FORMAT_BC3_TYPELESS;
		case FlexKit::DeviceFormat::BC3_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM;
		case FlexKit::DeviceFormat::BC3_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM_SRGB;
		case FlexKit::DeviceFormat::BC4_TYPELESS:
			return DXGI_FORMAT::DXGI_FORMAT_BC4_TYPELESS;
		case FlexKit::DeviceFormat::BC4_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC4_UNORM;
		case FlexKit::DeviceFormat::BC4_SNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC4_SNORM;
		case FlexKit::DeviceFormat::BC5_TYPELESS:
			return DXGI_FORMAT::DXGI_FORMAT_BC5_TYPELESS;
		case FlexKit::DeviceFormat::BC5_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC5_UNORM;
		case FlexKit::DeviceFormat::BC5_SNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC5_SNORM;
		case FlexKit::DeviceFormat::BC7_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM;
		case FlexKit::DeviceFormat::UNKNOWN:
			return DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		default:
			return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UINT;
			break;
		}
	}

	
	/************************************************************************************************/


	DeviceFormat	DXGIFormat2TextureFormat(DXGI_FORMAT F) noexcept
	{
		switch (F)
		{
		case DXGI_FORMAT::DXGI_FORMAT_R16G16_UINT:
			return FlexKit::DeviceFormat::R16G16_UINT;
		case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT:
			return  DeviceFormat::R16G16B16A16_UINT;
		case DXGI_FORMAT::DXGI_FORMAT_R32G32_UINT:
			return FlexKit::DeviceFormat::R32G32_UINT;
		case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM:
			return FlexKit::DeviceFormat::R8G8B8A8_UNORM;
		case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return FlexKit::DeviceFormat::R8G8B8A8_UNORM_SRGB;
		case DXGI_FORMAT::DXGI_FORMAT_R8G8_UNORM:
			return FlexKit::DeviceFormat::R8G8_UNORM;
		case DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT:
			return FlexKit::DeviceFormat::R32G32B32_FLOAT;
		case DXGI_FORMAT::DXGI_FORMAT_R16G16_FLOAT:
			return FlexKit::DeviceFormat::R16G16_FLOAT;
		case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT:
			return FlexKit::DeviceFormat::R16G16B16A16_FLOAT;
		case DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT:
			return FlexKit::DeviceFormat::R32G32B32A32_FLOAT;
		case DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT:
			return FlexKit::DeviceFormat::R32_FLOAT;
		case DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT:
			return FlexKit::DeviceFormat::D32_FLOAT;
		case DXGI_FORMAT::DXGI_FORMAT_BC1_TYPELESS:
			return FlexKit::DeviceFormat::BC1_TYPELESS;
		case DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM:
			return FlexKit::DeviceFormat::BC1_UNORM;
		case DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM_SRGB:
			return FlexKit::DeviceFormat::BC1_UNORM_SRGB;
		case DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM:
			return FlexKit::DeviceFormat::BC2_UNORM;
		case DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM_SRGB:
			return FlexKit::DeviceFormat::BC2_UNORM_SRGB;
		case DXGI_FORMAT::DXGI_FORMAT_BC3_TYPELESS:
			return FlexKit::DeviceFormat::BC3_TYPELESS;
		case DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM:
			return FlexKit::DeviceFormat::BC3_UNORM;
		case DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM_SRGB:
			return FlexKit::DeviceFormat::BC3_UNORM_SRGB;
		case DXGI_FORMAT::DXGI_FORMAT_BC4_TYPELESS:
			return FlexKit::DeviceFormat::BC4_TYPELESS;
		case DXGI_FORMAT::DXGI_FORMAT_BC4_UNORM:
			return FlexKit::DeviceFormat::BC4_UNORM;
		case DXGI_FORMAT::DXGI_FORMAT_BC4_SNORM:
			return FlexKit::DeviceFormat::BC4_SNORM;
		case DXGI_FORMAT::DXGI_FORMAT_BC5_TYPELESS:
			return FlexKit::DeviceFormat::BC5_TYPELESS;
		case DXGI_FORMAT::DXGI_FORMAT_BC5_UNORM:
			return FlexKit::DeviceFormat::BC5_UNORM;
		case DXGI_FORMAT::DXGI_FORMAT_BC5_SNORM:
			return FlexKit::DeviceFormat::BC5_SNORM;
		case DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM:
			return FlexKit::DeviceFormat::BC7_UNORM;
		case DXGI_FORMAT::DXGI_FORMAT_R32_UINT:
			return FlexKit::DeviceFormat::R32_UINT;
		case DXGI_FORMAT::DXGI_FORMAT_R32G32B32_UINT:
			return FlexKit::DeviceFormat::R32G32B32_UINT;
		case DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_UINT:
			return FlexKit::DeviceFormat::R32G32B32A32_UINT;

		default:
			return FlexKit::DeviceFormat::UNKNOWN;
			break;
		}
	}


	/************************************************************************************************/


	FLEXKITAPI void _UpdateSubResourceByUploadQueue(RenderSystem* RS, CopyContextHandle uploadHandle, ID3D12Resource* destinationResource, SubResourceUpload_Desc* desc)
	{
		auto& copyCtx = RS->_GetCopyContext(uploadHandle);

		for (size_t I = 0; I < desc->subResourceCount; ++I)
		{
			const auto region	= copyCtx.Reserve(desc->buffers[I].Size, 512);

			memcpy(
				(char*)region.buffer,
				(char*)desc->buffers[I].Buffer,
				desc->buffers[I].Size);

			copyCtx.CopyTextureRegion(
				destinationResource,
				I,
				{ 0, 0, 0 },
				region,
				desc->buffers[I].WH,
				desc->format);
		}
	}


	/************************************************************************************************/


	void RenderSystem::UpdateResourceByUploadQueue(ID3D12Resource* dest, CopyContextHandle uploadQueue, const void* data, size_t Size, size_t byteSize, DeviceAccessState endState)
	{
		if (nullptr == data || nullptr == dest) [[unlikely]]
			return;

		auto& copyCtx = _GetCopyContext(uploadQueue);

		const auto reservedSpace = copyCtx.Reserve(Size);

		memcpy(reservedSpace.buffer, data, Size);

		copyCtx.CopyBuffer(dest, 0, reservedSpace);
	}


	/************************************************************************************************/

	struct IncludeHandler : public IDxcIncludeHandler
	{
		HRESULT STDMETHODCALLTYPE LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource) override
		{
			char fileStr[256];
			auto fileLength = wcstombs(fileStr, pFilename, 256);

			std::filesystem::path file{ fileStr };
			auto newFilePath = includePath.string() + R"(\)" + file.string();

			wchar_t fileW[256];
			mbstowcs(fileW, newFilePath.c_str(), 256);


			return handler->LoadSource(fileW, ppIncludeSource);
		}

		HRESULT IUnknown::QueryInterface(const IID&, void**)
		{
			return 0;
		}

		std::filesystem::path   includePath;
		IDxcIncludeHandler*     handler;

		ULONG AddRef() { return 0; }
		ULONG Release() { return 0; }
	};


	/************************************************************************************************/


	Shader  RenderSystem::LoadShader(const char* entryPoint, const char* profile, const char* file, const ShaderOptions& options)
	{
		std::filesystem::path filePath{ file };
		auto parentPath = filePath.parent_path();

		wchar_t entryPointW[64];
		wchar_t fileW[256];
		wchar_t filenameW[256];
		wchar_t profileW[64];

		size_t fileWLength = 0;
		if (entryPoint != nullptr)
			mbstowcs(entryPointW, entryPoint, 64);

		mbstowcs(profileW, profile, 64);
		mbstowcs(fileW, file, 256);
		mbstowcs(filenameW, filePath.filename().string().c_str(), 256);


		IDxcBlobEncoding* blob;
		auto HR1 = hlslLibrary->CreateBlobFromFile(fileW, nullptr, &blob);

		if (FAILED(HR1))
		{
			LPSTR string = nullptr;

			const auto msgLen = FormatMessageA(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr,
				HR1,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPSTR)&string,
				0,
				nullptr);

			auto converted = fmt::format("Shader failed to load: {}", string);

			FK_LOG_ERROR(converted.c_str());

			LocalFree(string);

			return {};
		}

		IncludeHandler includeHandler;
		includeHandler.includePath      = parentPath;
		includeHandler.handler          = hlslIncludeHandler;


		IDxcCompiler2* debugCompiler = nullptr;
		hlslCompiler->QueryInterface<IDxcCompiler2>(&debugCompiler);

		static_vector<LPCWSTR> arguments;

#if USING(DEBUGSHADERS)
		arguments.push_back(L"-Od");
		arguments.push_back(L"/Zi");
		arguments.push_back(L"-Qembed_debug");
#else
		arguments.push_back(L"-O2");
#endif

		if(options.enable16BitTypes)
			arguments.push_back(L"-enable-16bit-types");

		if (options.hlsl2021)
			arguments.push_back(L"-HV 2021");

		IDxcOperationResult* result = nullptr;

		HRESULT HR2;
		try
		{
			HR2 = hlslCompiler->Compile(blob, filenameW, entryPoint != nullptr ? entryPointW : nullptr, profileW, arguments.data(), (UINT)arguments.size(), nullptr, 0, &includeHandler, &result);
		}
		catch (...)
		{
			std::cout << "t";
		}

		if (FAILED(HR2))
		{
			if(result)
				result->Release();

			return {};
		}
		else
		{
			IDxcBlob* byteCodeBlob;
			HRESULT status;
			result->GetStatus(&status);

			while(FAILED(status))
			{
				IDxcBlobEncoding* errors;
				result->GetErrorBuffer(&errors);

				auto errorString = (const char*)errors->GetBufferPointer();

				std::string traceMessage = GetCallStackString();
				std::string formattedMessage =
					std::format("{}\nFailed to Compile Shader\nEntryPoint: {}\nFile : {}\nStack Trace : \n {}\nPress Enter to try again\n",
						errorString, entryPoint ? entryPoint : "No Entry Point", file, traceMessage);

				FK_LOG_ERROR(formattedMessage.c_str());

				errors->Release();

				char str[100];
				std::cin >> str;

				HR1 = hlslLibrary->CreateBlobFromFile(fileW, nullptr, &blob);
				HR2 = hlslCompiler->Compile(blob, filenameW, entryPointW, profileW, arguments.data(), (UINT)arguments.size(), nullptr, 0, &includeHandler, &result);

				result->GetStatus(&status);
			}


			auto HR = result->GetResult(&byteCodeBlob);

			wchar_t* text = (wchar_t*)byteCodeBlob->GetBufferPointer();

			Shader out = CreateShader( byteCodeBlob, Memory );
			byteCodeBlob->Release();
			result->Release();

			return out;
		}
	}


	/************************************************************************************************/


	std::expected<Shader, std::string> RenderSystem::LoadRootSignature(const char* file, const char* entry)
	{
		std::filesystem::path filePath{ file };
		auto parentPath = filePath.parent_path();

		wchar_t entryW[64];
		wchar_t profileW[] = L"rootsig_1_1";

		if(entry != nullptr)
			mbstowcs(entryW, entry, 64);

		IDxcBlobEncoding* blob;
		auto HR1 = hlslLibrary->CreateBlobFromFile(filePath.c_str(), nullptr, &blob);

		if (FAILED(HR1))
		{
			LPSTR string = nullptr;

			const auto msgLen = FormatMessageA(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr,
				HR1,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPSTR)&string,
				0,
				nullptr);

			std::string formattedMessage =
				std::format("Signature failed to load : {}", string);

			FK_LOG_ERROR(formattedMessage.c_str());

			LocalFree(string);

			return std::unexpected{ "File Not Found!" };
		}

		IncludeHandler includeHandler;
		includeHandler.includePath = parentPath;
		includeHandler.handler = hlslIncludeHandler;


		IDxcCompiler2* debugCompiler = nullptr;
		hlslCompiler->QueryInterface<IDxcCompiler2>(&debugCompiler);

		static_vector<LPCWSTR> arguments;

		arguments.push_back(L"/extractrootsignature");

		IDxcOperationResult* result = nullptr;
		auto HR2 = hlslCompiler->Compile(
			blob,
			filePath.c_str(),
			entry != nullptr ? entryW : nullptr,
			profileW,
			arguments.data(), (UINT)arguments.size(),
			nullptr, 0,
			&includeHandler,
			&result);

		if (FAILED(HR2))
		{
			if (result)
				result->Release();

			return std::unexpected{ std::string{ "Failed to load file: " } + file };
		}
		else
		{
			IDxcBlob* byteCodeBlob;
			HRESULT status;
			result->GetStatus(&status);

			if (FAILED(status))
			{
				IDxcBlobEncoding* errors;
				result->GetErrorBuffer(&errors);

				auto errorString = (const char*)errors->GetBufferPointer();
				FK_LOG_ERROR("%s\nFailed to compile root signature \nFile: %s\nPress Enter to try again\n", filePath.string().c_str());

				errors->Release();

				return std::unexpected{ std::string{ errorString } };
			}

			auto HR = result->GetResult(&byteCodeBlob);

			wchar_t* text = (wchar_t*)byteCodeBlob->GetBufferPointer();

			Shader out = CreateShader(byteCodeBlob, Memory);
			byteCodeBlob->Release();
			result->Release();

			return out;
		}

		return std::unexpected{ std::string{ "Unexpected error!" } };
	}


	/************************************************************************************************/


	void CreateVertexBuffer(RenderSystem* RS, CopyContextHandle handle, VertexBufferView** Buffers, size_t BufferCount, VertexBufferSet& DVB_Out)
	{
		// TODO: Add Buffer Layout Structure for more complex Buffer Layouts
		// TODO: ATM only is able to make Buffers of a single Value
		InputDescription Input_Desc;

		DVB_Out.buffers.SetFull();

		// Generate Input Layout
		HRESULT	HR = ERROR;
		D3D12_RESOURCE_DESC Resource_DESC	= CD3DX12_RESOURCE_DESC::Buffer(0);
		Resource_DESC.Alignment				= 0;
		Resource_DESC.DepthOrArraySize		= 1;
		Resource_DESC.Dimension				= D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		Resource_DESC.Layout				= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Resource_DESC.Width					= 0;
		Resource_DESC.Height				= 1;
		Resource_DESC.Format				= DXGI_FORMAT_UNKNOWN;
		Resource_DESC.SampleDesc.Count		= 1;
		Resource_DESC.SampleDesc.Quality	= 0;
		Resource_DESC.Flags					= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;

		D3D12_HEAP_PROPERTIES HEAP_Props ={};
		HEAP_Props.CPUPageProperty	    = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type				    = D3D12_HEAP_TYPE_DEFAULT;
		HEAP_Props.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask	    = 0;
		HEAP_Props.VisibleNodeMask		= 0;

		auto& cctx = RS->_GetCopyContext(handle);

		for (uint32_t itr = 0; itr < BufferCount; ++itr)
		{
			auto& buffer = Buffers[itr];
			if (nullptr != Buffers[itr] && Buffers[itr]->GetBufferType() == VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX)
			{
				// Create the Vertex Buffer
				FK_ASSERT(Buffers[itr]->GetBufferSizeRaw());// ERROR BUFFER EMPTY;
				Resource_DESC.Width	= Buffers[itr]->GetBufferSizeRaw();

				ID3D12Resource* NewBuffer = nullptr;

				HRESULT HR = RS->pDevice->CreateCommittedResource(
									&HEAP_Props, 
									D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, 
									&Resource_DESC, 
									D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, 
									nullptr, 
									IID_PPV_ARGS(&NewBuffer));
			
				if (FAILED(HR))
				{// TODO!
					FK_ASSERT(0);
				}

				RS->UpdateResourceByUploadQueue(
					NewBuffer, 
					handle,
					Buffers[itr]->GetBuffer(),
					Buffers[itr]->GetBufferSizeRaw(), 1,
					DASCommon);

				SETDEBUGNAME(NewBuffer, "INDEXBUFFER");

				DVB_Out.buffers[itr].apiResource		= NewBuffer;
				DVB_Out.buffers[itr].bufferSizeInBytes	= (uint32_t)Buffers[itr]->GetBufferSizeRaw();
				DVB_Out.buffers[itr].bufferStride		= (uint32_t)Buffers[itr]->GetElementSize();
				DVB_Out.buffers[itr].type				= Buffers[itr]->GetBufferType();
				DVB_Out.MD.IndexBuffer_Index			= itr;
				DVB_Out.MD.InputElementCount			= Buffers[itr]->GetBufferSize();
			}
			else if (Buffers[itr] && Buffers[itr]->GetBufferSize())
			{
				ID3D12Resource* apiResource = nullptr;
				// Create the Vertex Buffer
				FK_ASSERT(Buffers[itr]->GetBufferSizeRaw());// ERROR BUFFER EMPTY;
				Resource_DESC.Width	= Buffers[itr]->GetBufferSizeRaw();

				HRESULT HR = RS->pDevice->CreateCommittedResource(&HEAP_Props, 
									D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &Resource_DESC, 
									D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON,
									nullptr, IID_PPV_ARGS(&apiResource));

				if (FAILED(HR))
				{// TODO!
					FK_ASSERT(0);
				}

				switch (Buffers[itr]->GetBufferType())
				{
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION:
					{SETDEBUGNAME(apiResource, "VERTEXBUFFER");				break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL:
					{SETDEBUGNAME(apiResource, "NORMAL BUFFER");			break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT:
					{SETDEBUGNAME(apiResource, "TANGET BUFFER");			break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_COLOR:
					{SETDEBUGNAME(apiResource, "COLOUR BUFFER");			break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV:
					{SETDEBUGNAME(apiResource, "TEXCOORD BUFFER");			break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1:
					{SETDEBUGNAME(apiResource, "AnimationWeights");			break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2:
					{SETDEBUGNAME(apiResource, "AnimationIndices");			break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_PACKED:
					{SETDEBUGNAME(apiResource, "PACKED_BUFFER");			break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ERROR:
				default:
					{SETDEBUGNAME(apiResource, "VERTEXBUFFER_TYPE_ERROR");	break; }
					break;
				}

				RS->UpdateResourceByUploadQueue(
					apiResource,
					handle,
					Buffers[itr]->GetBuffer(),
					Buffers[itr]->GetBufferSizeRaw(), 1, 
					DASCommon);

				DVB_Out.buffers[itr].apiResource		= apiResource;
				DVB_Out.buffers[itr].bufferStride		= (uint32_t)Buffers[itr]->GetElementSize();
				DVB_Out.buffers[itr].bufferSizeInBytes	= (uint32_t)Buffers[itr]->GetBufferSizeRaw();
				DVB_Out.buffers[itr].type				= Buffers[itr]->GetBufferType();
			}
		}
	}
	

	/************************************************************************************************/

	VertexBufferHandle VertexBufferStateTable::CreateVertexBuffer(size_t BufferSize, bool GPUResident, RenderSystem* RS) // Creates Using Placed Resource
	{
		VBufferHandle Buffer[3];

		for(size_t I = 0; I < 3; ++I)
			Buffer[I] = CreateVertexBufferResource(BufferSize, GPUResident, RS);

		const auto handle   = Handles.GetNewHandle();
		Handles[handle]     = (index_t)UserBuffers.size();

		UserBuffers.push_back(UserVertexBuffer{
			0,
			{ Buffer[0], Buffer[1], Buffer[2] },
			BufferSize,
			0, 
			GPUResident ? nullptr : _Map(Buffer[0]),
			Buffers[Buffer[0]].resource,
			false,
			handle});

		return handle;
	}


	/************************************************************************************************/


	VertexBufferStateTable::VBufferHandle VertexBufferStateTable::CreateVertexBufferResource(size_t BufferSize, bool GPUResident, RenderSystem* RS)
	{
		ID3D12Resource*	NewResource = RS->_CreateVertexBufferDeviceResource(BufferSize, GPUResident);
		void* _ptr = nullptr;

		FK_ASSERT(NewResource, "Failed To Create VertexBuffer Resource!");

		CD3DX12_RANGE readRange(0, 0);

		Buffers.push_back(
			VertexBuffer{
				NewResource,
				BufferSize,
				0 });

		return Buffers.size() - 1;
	}


	/************************************************************************************************/


	bool VertexBufferStateTable::PushVertex(VertexBufferHandle Handle, void* _ptr, size_t ElementSize)
	{
		auto	Idx			= Handles[Handle];
		auto&	UserBuffer  = UserBuffers[Idx];
		auto	Offset		= UserBuffers[Idx].Offset;
		auto	Mapped_Ptr	= UserBuffers[Idx].MappedPtr;

		memcpy(Mapped_Ptr + Offset, _ptr, ElementSize);
		UserBuffers[Idx].Offset += ElementSize;
		UserBuffers[Idx].WrittenTo = true;

		return true;
	}
	

	/************************************************************************************************/


	void VertexBufferStateTable::LockUntil(size_t Frame)
	{
		for (auto& userBuffer : UserBuffers)
		{
			if (!userBuffer.WrittenTo)
				continue;

			Buffers[userBuffer.GetCurrentBuffer()].lockCounter = Frame;

			_UnMap(userBuffer.GetCurrentBuffer(), userBuffer.Offset);

			userBuffer.IncrementCurrentBuffer();
			auto bufferIdx					= userBuffer.GetCurrentBuffer();
			userBuffer.MappedPtr			= _Map(bufferIdx);
			userBuffer.WrittenTo			= false;
			userBuffer.Offset				= 0;
		}
	}


	/************************************************************************************************/


	void VertexBufferStateTable::Reset(VertexBufferHandle Handle)
	{
		auto& UserBuffer = UserBuffers[Handles[Handle]];

		if (UserBuffer.Offset == 0)
			return; // Unused

		if(UserBuffer.WrittenTo)
		{	// UnMap Current Buffer
			auto  ResourceIdx = UserBuffer.GetCurrentBuffer();
			D3D12_RANGE Range = { 0, UserBuffer.Offset };
			Buffers[ResourceIdx].resource->Unmap(0, &Range);
		}
		
		UserBuffer.WrittenTo = false;
		UserBuffer.IncrementCurrentBuffer();

		{	// Map Current Buffer
			auto	ResourceIdx		= UserBuffer.GetCurrentBuffer();
			void*	MappedPtr		= nullptr;

			D3D12_RANGE Range{ 0, 0 };
			Buffers[ResourceIdx].resource->Map(0, &Range, &MappedPtr);

			UserBuffer.Offset		= 0;
			UserBuffer.MappedPtr	= (char*)MappedPtr;
			UserBuffer.Resource		= Buffers[ResourceIdx].resource;
		}
	}


	/************************************************************************************************/


	ID3D12Resource* VertexBufferStateTable::GetAsset(VertexBufferHandle Handle)
	{
		return Buffers[UserBuffers[Handles[Handle]].GetCurrentBuffer()].resource;
	}


	/************************************************************************************************/


	size_t VertexBufferStateTable::GetCurrentVertexBufferOffset(VertexBufferHandle Handle) const
	{
		return UserBuffers[Handles[Handle]].Offset;
	}


	/************************************************************************************************/


	size_t VertexBufferStateTable::GetBufferSize(VertexBufferHandle Handle) const
	{
		return UserBuffers[Handles[Handle]].ResourceSize;
	}


	/************************************************************************************************/


	SubAllocation VertexBufferStateTable::Reserve(VertexBufferHandle Handle, size_t size) noexcept
	{
		auto	idx			= Handles[Handle];
		auto&	userBuffer  = UserBuffers[idx];
		auto	offset		= UserBuffers[idx].Offset;
		auto	mapped_Ptr	= UserBuffers[idx].MappedPtr;

		const size_t mask = 0XFF;

		if( offset & mask)
		{
			size_t alignementOffset = mask - (offset & mask) + 1;
			offset  += alignementOffset;
			size    += alignementOffset;
		}

		UserBuffers[idx].Offset		+= size;
		UserBuffers[idx].WrittenTo	 = true;

		return { mapped_Ptr, offset, size };
	}



	/************************************************************************************************/


	bool VertexBufferStateTable::CurrentlyAvailable(VertexBufferHandle Handle, size_t completed) const
	{
		auto UserIdx	= Handles[Handle];
		auto BufferIdx	= UserBuffers[UserIdx].GetCurrentBuffer();

		return Buffers[Handle].lockCounter <= completed;
	}


	/************************************************************************************************/


	char* VertexBufferStateTable::_Map(VBufferHandle handle)
	{
		void* ptr;
		Buffers[handle].resource->Map(0, nullptr, &ptr);

		return (char*)ptr;
	}


	void VertexBufferStateTable::_UnMap(VBufferHandle handle, size_t range)
	{
		D3D12_RANGE writtenRange;
		writtenRange.Begin  = 0;
		writtenRange.End    = range;

		Buffers[handle].resource->Unmap(0, &writtenRange);
	}


	/************************************************************************************************/


	void VertexBufferStateTable::Release()
	{
		for (auto& B : Buffers) {
			if(B.resource)
				B.resource->Release();
			B.resource = nullptr;
		}


		Buffers.Release();
		UserBuffers.Release();
		FreeBuffers.Release();
		Handles.Release();
	}


	/************************************************************************************************/


	void VertexBufferStateTable::ReleaseVertexBuffer(VertexBufferHandle handle, uint64_t current)
	{
		std::scoped_lock lock{ criticalSection };

		auto userIdx		= Handles[handle];
		auto& userEntry     = UserBuffers[userIdx];

		for (const auto ResourceIdx : userEntry.Buffers)
			if(ResourceIdx != INVALIDHANDLE)
				FreeBuffers.push_back({ current, ResourceIdx});
			else
				break;

		if (UserBuffers.size() > 1)
		{
			auto& back = UserBuffers.back();
			userEntry = back;

			Handles[back.handle] = userIdx;
		}

		UserBuffers.pop_back();
	}


	/************************************************************************************************/


	void VertexBufferStateTable::ReleaseFree(uint64_t current)
	{
		std::scoped_lock lock{ criticalSection };

		for (auto freeBuffer : FreeBuffers)
		{
			if (Buffers[freeBuffer.BufferIdx].lockCounter <= current)
				Buffers[freeBuffer.BufferIdx].resource->Release();
		}

		std::sort(FreeBuffers.begin(), FreeBuffers.end());

		FreeBuffers.erase(
			std::remove_if(
				FreeBuffers.begin(),
				FreeBuffers.end(),
				[&](auto buffer)-> bool
				{
					return Buffers[buffer].lockCounter <= current;
				}),
			FreeBuffers.end());
	}


	/************************************************************************************************/


	QueryHandle QueryTable::CreateQueryBuffer(size_t count, QueryType type)
	{
		D3D12_QUERY_HEAP_DESC desc;
		desc.Count			= (UINT)count;
		desc.NodeMask		= 0;

		ResourceEntry newResEntry = { 0 };

		switch (type)
		{
		case QueryType::OcclusionQuery:
			desc.Type			= D3D12_QUERY_HEAP_TYPE_OCCLUSION;
			newResEntry.type	= D3D12_QUERY_TYPE_OCCLUSION;
			break;
		case QueryType::BinaryOcclusionQuery:
			desc.Type			= D3D12_QUERY_HEAP_TYPE_OCCLUSION;
			newResEntry.type	= D3D12_QUERY_TYPE_BINARY_OCCLUSION;
			break;
		case QueryType::PipelineStats:
			desc.Type			= D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS;
			newResEntry.type	= D3D12_QUERY_TYPE_PIPELINE_STATISTICS;
			break;
		case QueryType::TimeStats:
			desc.Type           = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
			newResEntry.type    = D3D12_QUERY_TYPE_TIMESTAMP;
			break;
		default:
			break;
		}


		for (auto& Res : newResEntry.resources)
			RS->pDevice->CreateQueryHeap(&desc, IID_PPV_ARGS(&Res));

		size_t		userIdx = users.size();
		UserEntry	newUserEntry = { 0, 0, 0, false };

		newUserEntry.resourceIdx	= resources.size();
		users.push_back(newUserEntry);
		resources.push_back(newResEntry);

		return QueryHandle(userIdx);
	}


	/************************************************************************************************/


	QueryHandle	QueryTable::CreateSOQueryBuffer(size_t count, size_t SOIndex)
	{
		FK_ASSERT(SOIndex < D3D12_QUERY_TYPE_SO_STATISTICS_STREAM3 - D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0, "invalid argument");

		D3D12_QUERY_HEAP_DESC	heapDesc = {};
		
		heapDesc.Count	= (UINT)count;
		heapDesc.Type	= static_cast<D3D12_QUERY_HEAP_TYPE>(D3D12_QUERY_HEAP_TYPE_SO_STATISTICS);

		ResourceEntry newResEntry	= { 0 };
		newResEntry.currentResource = 0;
		newResEntry.type			= static_cast<D3D12_QUERY_TYPE>(D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0 + SOIndex);

		for (size_t itr = 0; itr < 3; ++itr)
		{
			newResEntry.layouts[itr] = DeviceLayout::Common;
			newResEntry.resourceLocks[itr] = 0;
			RS->pDevice->CreateQueryHeap(
				&heapDesc,
				IID_PPV_ARGS(&newResEntry.resources[itr]));
		}

		size_t resourceIdx = resources.push_back(newResEntry);

		return QueryHandle(users.push_back(
								{	resourceIdx,
									count,
									0,
									false}));
	}


	/************************************************************************************************/


	void QueryTable::LockUntil(QueryHandle handle, size_t FrameID)
	{
		auto user = users[handle];

		if (user.used)
		{
			size_t resourceIdx			= user.resourceIdx;
			size_t currentResourceIdx	= resources[resourceIdx].currentResource;

			resources[resourceIdx].currentResource = (currentResourceIdx + 2 % 3);
		}
	}


	/************************************************************************************************/


	void ResourceStateTable::Release()
	{
		for (size_t I = 0; I < UserEntries.size(); ++I)
		{
			auto& T = UserEntries[I];
			auto F  = T.Flags;

			Resources[T.ResourceIdx].Release();
		}

		for (auto& resource : delayRelease)
			if(resource.resource)
				resource.resource->Release();

		delayRelease.Release();
		UserEntries.Release();
		Resources.Release();
		Handles.Clear();
	}


	/************************************************************************************************/


	ResourceHandle ResourceStateTable::GetFreeHandle()
	{
		std::scoped_lock lock{ m };

		auto newHandle = Handles.GetNewHandle();
		Handles[newHandle] = -1;

		return newHandle;
	}


	/************************************************************************************************/


	ResourceHandle ResourceStateTable::AddResource(const GPUResourceDesc& desc, const DeviceLayout layout)
	{
		std::scoped_lock lock{m};

		auto Handle		 = Handles.GetNewHandle();

		ResourceEntry newEntry = 
		{	
			desc.bufferCount,
			0,
			{ nullptr, nullptr, nullptr },
			{ 0, 0, 0 },
			{ layout, layout, layout },
			TextureFormat2DXGIFormat(desc.format),
			desc.MipLevels,
			desc.WH,
			Handle,
			{ { desc.placed.offset, 0 }, {}, {} }
		};

		for (size_t I = 0; I < desc.bufferCount; ++I) {
			newEntry.Resources[I] = desc.resources[I];

#if USING(AFTERMATH)
			GFSDK_Aftermath_ResourceHandle  aftermathResource;
			GFSDK_Aftermath_DX12_RegisterResource(desc.resources[I], &aftermathResource);
			NewEntry.aftermathResource[I]   = aftermathResource;
#endif
		}

		UserEntry entry;
		entry.ResourceIdx		= Resources.push_back(newEntry);
		entry.resourceSize		= desc.byteSize;
		entry.FrameGraphIndex	= -1;
		entry.FGI_FrameStamp	= -1;
		entry.Handle			= Handle;
		entry.Format			= newEntry.Format;
		entry.Flags				= desc.backBuffer ? TF_BackBuffer : 0;
		entry.dimension			= desc.Dimensions;
		entry.arraySize			= desc.arraySize;
		entry.tileMappings		= { allocator };

		if (desc.Dimensions == TextureDimension::Buffer)
		{
			UAVResourceLayout layout;
			layout.format		= newEntry.Format;
			layout.elementCount	= (UINT)(desc.byteSize / 4);
			layout.stride		= 4;
			entry.extra			= layout;
		}

		Handles[Handle]	= (UINT)UserEntries.push_back(entry);

		if (desc.bufferCount > 1)
			BufferedResources.push_back(Handle);

		return Handle;
	}


	/************************************************************************************************/


	void ResourceStateTable::SetResource(ResourceHandle handle, const GPUResourceDesc& desc, const DeviceLayout layout)
	{
		std::scoped_lock lock{ m };

		ResourceEntry newEntry = 
		{	
			desc.bufferCount,
			0,
			{ nullptr, nullptr, nullptr },
			{ 0, 0, 0 },
			{ layout, layout , layout },
			TextureFormat2DXGIFormat(desc.format),
			desc.MipLevels,
			desc.WH,
			handle,
			{ { desc.placed.offset, 0 }, {}, {} }
		};

		for (size_t I = 0; I < desc.bufferCount; ++I) {
			newEntry.Resources[I] = desc.resources[I];

#if USING(AFTERMATH)
			GFSDK_Aftermath_ResourceHandle  aftermathResource;
			GFSDK_Aftermath_DX12_RegisterResource(desc.resources[I], &aftermathResource);
			NewEntry.aftermathResource[I]   = aftermathResource;
#endif
		}

		UserEntry entry;
		entry.ResourceIdx		= Resources.push_back(newEntry);
		entry.resourceSize		= desc.byteSize;
		entry.FrameGraphIndex	= -1;
		entry.FGI_FrameStamp	= -1;
		entry.Handle			= handle;
		entry.Format			= newEntry.Format;
		entry.Flags				= desc.backBuffer ? TF_BackBuffer : 0;
		entry.dimension			= desc.Dimensions;
		entry.arraySize			= desc.arraySize;
		entry.tileMappings      = { allocator };

		if (desc.Dimensions == TextureDimension::Buffer)
		{
			UAVResourceLayout layout;
			layout.format		= newEntry.Format;
			layout.elementCount	= (UINT)(desc.byteSize / 4);
			layout.stride		= 4;
			entry.extra			= layout;
		}

		Handles[handle]			= (UINT)UserEntries.push_back(entry);

		if (desc.bufferCount > 1)
			BufferedResources.push_back(handle);
	}


	/************************************************************************************************/


	void ResourceStateTable::ReleaseTexture(ResourceHandle handle, const uint64_t submissionID)
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		std::scoped_lock lock{ m };

		if (handle == InvalidHandle)
			return;

		const auto UserIdx	= Handles[handle];

		if (UserIdx == -1)
			return;

		auto& UserEntry		= UserEntries[UserIdx];
		const auto ResIdx	= UserEntry.ResourceIdx;
		auto& resource		= Resources[ResIdx];

		for (auto res : resource.Resources)
		{
			if (res)
				delayRelease.push_back({ res, submissionID });
		}

		const auto TempHandle	= UserEntries.back().Handle;
		UserEntry				= UserEntries.back();
		Handles[TempHandle]		= UserIdx;

		const auto temp			= Resources[UserEntry.ResourceIdx];
		Resources[ResIdx]		= Resources.back();
		UserEntries[Handles[resource.owner]].ResourceIdx = ResIdx;

		UserEntries.pop_back();
		Resources.pop_back();

		Handles.RemoveHandle(handle);
	}


	/************************************************************************************************/


	void ResourceStateTable::_ReleaseTextureForceRelease(ResourceHandle handle)
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto  UserIdx		= Handles[handle];
		auto& UserEntry		= UserEntries[UserIdx];
		const auto ResIdx	= UserEntry.ResourceIdx;
		auto& resource		= Resources[ResIdx];

		for (auto res : resource.Resources)
			res->Release();

		const auto TempHandle	= UserEntries.back().Handle;
		UserEntry				= UserEntries.back();
		Handles[TempHandle]		= UserIdx;

		const auto temp			= Resources[UserEntry.ResourceIdx];
		Resources[ResIdx]		= Resources.back();
		UserEntries[Handles[resource.owner]].ResourceIdx = ResIdx;

		UserEntries.pop_back();
		Resources.pop_back();

		Handles.RemoveHandle(handle);
	}


	/************************************************************************************************/


	Texture2D ResourceStateTable::operator[](ResourceHandle handle)
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		const auto Idx		    = Handles[handle];
		const auto resource     = Resources[UserEntries[Idx].ResourceIdx];

		const auto Res		    = resource.GetAsset();
		const auto WH			= resource.WH;
		const auto Format		= resource.Format;
		const auto mipCount	    = resource.mipCount;

		return { Res, WH, mipCount, Format };
	}


	/************************************************************************************************/


	void ResourceStateTable::SetLayout(ResourceHandle handle, DeviceLayout layout)
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		auto ResIdx  = UserEntries[UserIdx].ResourceIdx;

		Resources[ResIdx].SetLayout(layout);
	}


	/************************************************************************************************/


	void ResourceStateTable::SetDebug(ResourceHandle handle, const char* string)
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx                    = Handles[handle];
		UserEntries[UserIdx].userString = string;
	}


	/************************************************************************************************/


	void ResourceStateTable::SetBufferedIdx(ResourceHandle handle, uint32_t idx)
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		auto Residx  = UserEntries[UserIdx].ResourceIdx;

		Resources[Residx].CurrentResource = idx;
	}


	/************************************************************************************************/


	void ResourceStateTable::SetDebugName(ResourceHandle handle, const char* str)
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx    = Handles[handle];

		if (UserIdx == -1)
			return;

		auto resIdx     = UserEntries[UserIdx].ResourceIdx;
		auto& resource  = Resources[resIdx];

		UserEntries[UserIdx].userString = str;

		for (auto& res : resource.Resources)
			if (res)
				SETDEBUGNAME(res, str);
	}


	/************************************************************************************************/


	size_t ResourceStateTable::GetFrameGraphIndex(ResourceHandle handle, size_t FrameID) const
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx	= Handles[handle];
		auto FrameStamp	= UserEntries[UserIdx].FGI_FrameStamp;

		if (FrameStamp < int(FrameID))
			return INVALIDHANDLE;

		return UserEntries[UserIdx].FrameGraphIndex;
	}


	/************************************************************************************************/


	void ResourceStateTable::SetFrameGraphIndex(ResourceHandle handle, size_t FrameID, size_t Index)
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		UserEntries[UserIdx].FGI_FrameStamp		= FrameID;
		UserEntries[UserIdx].FrameGraphIndex	= (UINT)Index;
	}


	/************************************************************************************************/


	DXGI_FORMAT ResourceStateTable::GetFormat(ResourceHandle handle) const
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		return UserEntries[UserIdx].Format;
	}


	/************************************************************************************************/


	TextureDimension ResourceStateTable::GetDimension(ResourceHandle handle) const
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		return UserEntries[UserIdx].dimension;
	}


	/************************************************************************************************/


	size_t ResourceStateTable::GetArraySize(ResourceHandle handle) const
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		return UserEntries[UserIdx].arraySize;
	}


	/************************************************************************************************/


	uint8_t ResourceStateTable::GetMIPCount(ResourceHandle handle) const
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		return Resources[UserEntries[UserIdx].ResourceIdx].mipCount;
	}


	/************************************************************************************************/


	void ResourceStateTable::SetWH(ResourceHandle handle, uint2 WH)
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx									= Handles[handle];
		Resources[UserEntries[UserIdx].ResourceIdx].WH	= WH;
	}


	/************************************************************************************************/


	void ResourceStateTable::SetExtra(ResourceHandle handle, GPUResourceExtra_t extra)
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		UserEntries[UserIdx].extra = extra;
	}


	/************************************************************************************************/


	GPUResourceExtra_t  ResourceStateTable::GetExtra(ResourceHandle handle) const
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		return UserEntries[UserIdx].extra;
	}


	/************************************************************************************************/


	const char* ResourceStateTable::GetDebug(ResourceHandle handle)
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		return UserEntries[UserIdx].userString;
	}


	/************************************************************************************************/


	uint2 ResourceStateTable::GetWH(ResourceHandle handle) const
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		return Resources[UserEntries[UserIdx].ResourceIdx].WH;
	}


	/************************************************************************************************/


	uint3 ResourceStateTable::GetXYZ(ResourceHandle handle) const
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx	= Handles[handle];
		auto& user		= UserEntries[UserIdx];
		auto& resource	= Resources[UserEntries[UserIdx].ResourceIdx];

		return { resource.WH, user.arraySize };
	}


	/************************************************************************************************/


	void ResourceStateTable::UpdateTileMappings(ResourceHandle handle, const TileMapping* begin, const TileMapping* end, iAllocator& temp)
	{
		if (handle == InvalidHandle)
			return;

		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto itr		= begin;
		auto UserIdx	= Handles[handle];
		auto& mappings	= UserEntries[UserIdx].tileMappings;

		Vector<TileMapping, 256> newElements{ temp };

		std::sort(mappings.begin(), mappings.end(),
			[](const auto& lhs, const auto& rhs)
			{
				return lhs.sortingID() < rhs.sortingID();
			});

		std::for_each(begin, end,
			[&](const TileMapping& updatedTile)
			{
				auto pred = [](const TileMapping& lhs, const TileMapping& rhs)
				{
					return lhs.tileID < rhs.tileID;
				};

				auto res = std::lower_bound(
					mappings.begin(),
					mappings.end(),
					updatedTile,
					pred);

				if (res != mappings.end() && res->tileID == updatedTile.tileID)
					res->state = updatedTile.state;
				else
					newElements.push_back(updatedTile);
			});

		mappings.reserve(mappings.size() + newElements.size());

		for (auto& e : newElements)
			mappings.push_back(e);
	}


	/************************************************************************************************/


	void ResourceStateTable::SubmitTileUpdates(ID3D12CommandQueue* queue, RenderSystem& renderSystem, iAllocator* allocator_temp)
	{
		Vector<D3D12_TILED_RESOURCE_COORDINATE>	coordinates	{ allocator_temp };
		Vector<D3D12_TILE_REGION_SIZE>			regionSize	{ allocator_temp };
		Vector<D3D12_TILE_RANGE_FLAGS>			tile_flags	{ allocator_temp };
		Vector<UINT>							heapOffsets	{ allocator_temp };
		Vector<UINT>							tileCounts	{ allocator_temp };

		for (auto& userEntry : UserEntries)
		{
			coordinates.size();
			regionSize.size();

			ID3D12Resource*	resource	= nullptr;
			ID3D12Heap*		heap		= nullptr;

			for (const auto& mapping: userEntry.tileMappings)
			{
				auto desc = resource->GetDesc();
				
				if (auto mappingHeap = renderSystem.GetDeviceResource(mapping.heap); heap != mappingHeap)
				{
					if (!coordinates.size())
						__debugbreak();

					if (mappingHeap)
						queue->UpdateTileMappings(
							resource,
							(UINT)coordinates.size(),
							coordinates.data(),
							regionSize.data(),
							heap,
							(UINT)tile_flags.size(),
							tile_flags.data(),
							heapOffsets.data(),
							tileCounts.data(),
							D3D12_TILE_MAPPING_FLAG_NONE);

					coordinates.clear();
					regionSize.clear();
					tile_flags.clear();
					heapOffsets.clear();
					tileCounts.clear();
				}
				else
				{
					coordinates.push_back(
						D3D12_TILED_RESOURCE_COORDINATE{
							mapping.tileID.GetTileX(),
							mapping.tileID.GetTileY(),
							0,
							(UINT)mapping.tileID.GetMipLevel()
						});

					regionSize.push_back(
						D3D12_TILE_REGION_SIZE{
							1,
							true,
							1,
							1,
							1,
						});

					tile_flags.push_back(D3D12_TILE_RANGE_FLAGS::D3D12_TILE_RANGE_FLAG_NONE);
					heapOffsets.push_back(mapping.heapOffset);
					tileCounts.push_back(1);
				}
			}
		}
	}


	/************************************************************************************************/


	const TileMapList& ResourceStateTable::GetTileMappings(const ResourceHandle handle) const
	{
		auto UserIdx = Handles[handle];
		return UserEntries[UserIdx].tileMappings;
	}


	/************************************************************************************************/


	TileMapList& ResourceStateTable::_GetTileMappings(const ResourceHandle handle)
	{
		auto UserIdx = Handles[handle];
		return UserEntries[UserIdx].tileMappings;
	}


	/************************************************************************************************/


	void ResourceStateTable::MarkRTUsed(ResourceHandle Handle)
	{
		auto UserIdx = Handles[Handle];
		UserEntries[UserIdx].Flags |= TF_INUSE;
	}


	/************************************************************************************************/


	void ResourceStateTable::LockUntil(size_t FrameID)
	{
		for (auto bufferedResource : BufferedResources)
		{
			const auto idx  = Handles[bufferedResource];
			auto& UserEntry = UserEntries[idx];
			auto Flags      = UserEntry.Flags;

			if (Flags & TF_INUSE && !(Flags & TF_BackBuffer))
			{
				Resources[UserEntry.ResourceIdx].SetFrameLock(FrameID);
				Resources[UserEntry.ResourceIdx].IncreaseIdx();
			}
		}
	}


	/************************************************************************************************/


	void ResourceStateTable::FreeDelayedResources(ThreadManager& threads, const uint64_t completed)
	{
		if (delayRelease.size() < 32)
			return;

		ProfileFunction();

		Vector<ID3D12Resource*> freeList = { delayRelease.Allocator };

		for (auto& [resource, submissionID] : delayRelease)
		{
			if (completed > submissionID)
			{
				freeList.push_back(resource);
				resource = nullptr;
			}
		}

		if (freeList.size())
		{
			if(false)
			{
				auto& workItem = FlexKit::CreateWorkItem(
					[freeList = std::move(freeList)](auto& threadLocalAllocator)
					{
						for (auto res : freeList)
						{
#if USING(AFTERMATH)
							if (res)
								GFSDK_Aftermath_DX12_UnregisterResource(res);
#endif

							res->Release();
						}
					}, delayRelease.Allocator);

				threads.AddBackgroundWork(workItem);
			}
			else
			{
				for (auto& res : freeList)
					res->Release();
			}
		}

		delayRelease.erase(
			std::remove_if(
				delayRelease.begin(),
				delayRelease.end(),
				[&](auto& res) { return res.resource == nullptr; }),

		delayRelease.end());
	}


	/************************************************************************************************/


	bool ResourceStateTable::FreeDelayedResourcesIncrementally(const uint64_t completed)
	{
		ProfileFunction();

		for (auto& release : delayRelease)
		{
			auto&& [resource, submissionID] = release;
			if (completed > submissionID)
			{
				resource->Release();
				delayRelease.remove_unstable(&release);
				return true;
			}
		}
		return false;
	}


	/************************************************************************************************/


	void ResourceStateTable::ResourceEntry::Release()
	{
		for (auto& R : Resources)
		{
			if (R)
				R->Release();

			R = nullptr;
		}
	}


	/************************************************************************************************/


	DeviceLayout ResourceStateTable::GetLayout(ResourceHandle Handle) const
	{
		auto    Idx			    = Handles[Handle];
		auto    ResourceIdx	    = UserEntries[Idx].ResourceIdx;
		const   auto& resources = Resources[ResourceIdx];

		return resources.layouts[resources.CurrentResource];
	}


	/************************************************************************************************/


	ID3D12Resource* ResourceStateTable::GetResource(ResourceHandle Handle, ID3D12Device* device) const
	{
		if (Handle >= Handles.size())
			return nullptr;

		auto  idx			= Handles[Handle];
		auto  resourceIdx	= UserEntries[idx].ResourceIdx;
		auto& resource		= Resources[resourceIdx];
		auto& resources		= resource.Resources;
		auto  currentIdx	= resource.CurrentResource;

		return resources[currentIdx];
	}


	/************************************************************************************************/


	std::span<ID3D12Resource*>	ResourceStateTable::GetResources(ResourceHandle Handle)
	{
		if (Handle >= Handles.size())
			return {};

		auto  idx			= Handles[Handle];
		auto  resourceIdx	= UserEntries[idx].ResourceIdx;
		auto& resource		= Resources[resourceIdx];

		return {
			resource.Resources + 0,
			resource.Resources + resource.ResourceCount
		};
	}


	/************************************************************************************************/


	size_t ResourceStateTable::GetResourceSize(ResourceHandle Handle) const
	{
		auto  Idx			= Handles[Handle];
		return UserEntries[Idx].resourceSize;
	}


	/************************************************************************************************/


	uint2 ResourceStateTable::GetHeapOffset(ResourceHandle Handle, uint subResourceIdx) const
	{
		auto Idx = Handles[Handle];

		return Resources[UserEntries[Idx].resourceSize].heapRange[subResourceIdx];
	}


	/************************************************************************************************/


	ResourceHandle ResourceStateTable::FindResourceHandle(ID3D12Resource* deviceResource) const
	{
		auto res = std::find_if(
			Resources.begin(), Resources.end(),
			[&](auto res)
			{
				for (size_t I = 0; I < res.ResourceCount; I++)
					if (res.Resources[I] == deviceResource)
						return true;

				return false;
			});

		if (res != Resources.end())
			return res->owner;
		else
		{
			auto res = std::find_if(
				delayRelease.begin(), delayRelease.end(),
				[&](auto res)
				{
					if (res.resource == deviceResource)
						return true;

					return false;
				});

			if (res != delayRelease.end())
				__debugbreak();
		}

		return InvalidHandle;
	}


	/************************************************************************************************/


	void ResourceStateTable::ReplaceResources(ResourceHandle handle, ID3D12Resource** begin, size_t size)
	{
		auto  Idx                   = Handles[handle];
		auto  ResourceIdx           = UserEntries[Idx].ResourceIdx;
		const auto resourceCount    = Resources[ResourceIdx].ResourceCount;
		auto& resources             = Resources[ResourceIdx].Resources;

		for (size_t I = 0; I < Min(resourceCount, size); ++I)
			resources[I] = begin[I];
	}


	/************************************************************************************************/


	VertexResourceBuffer RenderSystem::_CreateVertexBufferDeviceResource(const size_t ResourceSize, bool GPUResident)
	{
		D3D12_RESOURCE_DESC   Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(ResourceSize);
		Resource_DESC.Alignment          = 0;
		Resource_DESC.DepthOrArraySize   = 1;
		Resource_DESC.Dimension          = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		Resource_DESC.Layout             = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Resource_DESC.Width              = ResourceSize;
		Resource_DESC.Height             = 1;
		Resource_DESC.Format             = DXGI_FORMAT_UNKNOWN;
		Resource_DESC.SampleDesc.Count   = 1;
		Resource_DESC.SampleDesc.Quality = 0;
		Resource_DESC.Flags              = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;

		D3D12_HEAP_PROPERTIES HEAP_Props = {};
		HEAP_Props.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type                  = GPUResident ? D3D12_HEAP_TYPE_DEFAULT : D3D12_HEAP_TYPE_UPLOAD;
		HEAP_Props.MemoryPoolPreference  = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask      = 0;
		HEAP_Props.VisibleNodeMask       = 0;

		FrameBufferedResource NewResource;
		NewResource.BufferCount          = BufferCount;

		auto InitialState = GPUResident ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_GENERIC_READ;

		ID3D12Resource* Resource = nullptr;
		HRESULT HR = pDevice->CreateCommittedResource(
			&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
			&Resource_DESC, InitialState, nullptr,
			IID_PPV_ARGS(&Resource));

		SETDEBUGNAME(Resource, __func__);

		return Resource;
	}


	/************************************************************************************************/


	ConstantBuffer RenderSystem::_CreateConstantBufferResource(RenderSystem* RS, ConstantBuffer_desc* desc)
	{
		D3D12_RESOURCE_DESC   Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(desc->InitialSize);
		Resource_DESC.Alignment				= 0;
		Resource_DESC.DepthOrArraySize		= 1;
		Resource_DESC.Dimension				= D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		Resource_DESC.Layout				= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Resource_DESC.Width					= desc->InitialSize;
		Resource_DESC.Height				= 1;
		Resource_DESC.Format				= DXGI_FORMAT_UNKNOWN;
		Resource_DESC.SampleDesc.Count		= 1;
		Resource_DESC.SampleDesc.Quality	= 0;
		Resource_DESC.Flags					= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

		D3D12_HEAP_PROPERTIES HEAP_Props ={};
		HEAP_Props.CPUPageProperty	     = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type				     = D3D12_HEAP_TYPE_DEFAULT;
		HEAP_Props.MemoryPoolPreference  = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask	     = 0;
		HEAP_Props.VisibleNodeMask		 = 0;

		size_t BufferCount = RS->BufferCount;
		FrameBufferedResource NewResource;
		NewResource.BufferCount = BufferCount;

		for(size_t I = 0; I < BufferCount; ++I)
		{
			ID3D12Resource* Resource = nullptr;
			HRESULT HR = RS->pDevice->CreateCommittedResource(
							&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, 
							&Resource_DESC, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr, 
							IID_PPV_ARGS(&Resource));

			CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE CONSTANT BUFFER"));
			NewResource.Resources[I] = Resource;

			SETDEBUGNAME(Resource, __func__);
		}

		return NewResource;
	}


	/************************************************************************************************/


	RenderSystem::AllocationResult	RenderSystem::_AllocateDescriptorRange(const size_t size)
	{
		uint64_t completed = directFence->GetCompletedValue();

		auto res = descriptorHeapAllocator.Alloc(size, completed);

		if (res)
			return res.value();
		else
			return std::unexpected{ RenderSystem::DescriptorRangeAllocationError::OutOfSpace };
	}


	/************************************************************************************************/


	void RenderSystem::_ReleaseDescriptorRange(DescriptorRange range, uint64_t lock)
	{
		descriptorHeapAllocator.Release(range, lock, directFence->GetCompletedValue());
	}


	/************************************************************************************************/


	void RenderSystem::_PushDelayReleasedResource(ID3D12Resource* resource, CopyContextHandle uploadQueue)
	{
		if (uploadQueue != InvalidHandle)
			copyEngine.Push_Temporary(resource, uploadQueue);
		else
			FreeList_CopyQueue.push_back({ resource, copyEngine.counter });
	}


	/************************************************************************************************/


	void RenderSystem::_ForceReleaseTexture(ResourceHandle handle)
	{
		Textures._ReleaseTextureForceRelease(handle);
	}


	void RenderSystem::_ReleaseDelayedResources()
	{
		//while (Textures.FreeDelayedResourcesIncrementally(directFence->GetCompletedValue()));
		Textures.FreeDelayedResources(threads, directFence->GetCompletedValue());
	}


	/************************************************************************************************/


	RenderSystem::VidMemoryStates RenderSystem::_GetVidMemStats()
	{
		DXGI_QUERY_VIDEO_MEMORY_INFO VideoMemInfo = {0};
		if(pDXGIAdapter)
			pDXGIAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &VideoMemInfo);

		return { VideoMemInfo.CurrentUsage, VideoMemInfo.Budget };
	}


	/************************************************************************************************/


	[[nodiscard]] ID3D12DescriptorHeap* RenderSystem::_CreateShaderVisibleHeap(const size_t numDescriptors)
	{
		ID3D12DescriptorHeap* heap;

		HRESULT HR;
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapdesc;
		descriptorHeapdesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		descriptorHeapdesc.NumDescriptors	= (uint32_t)numDescriptors;
		descriptorHeapdesc.NodeMask			= 0u;
		descriptorHeapdesc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		HR									= pDevice->CreateDescriptorHeap(&descriptorHeapdesc, IID_PPV_ARGS(&heap));
		FK_ASSERT(HR, "FAILED TO CREATE DESCRIPTOR HEAP");

		return heap;
	}


	/************************************************************************************************/


	ID3D12QueryHeap* RenderSystem::_GetQueryResource(QueryHandle Handle)
	{
		return Queries.GetDeviceObject(Handle);
	}



	/************************************************************************************************/


	CopyContext& RenderSystem::_GetCopyContext(CopyContextHandle handle)
	{
		if (handle == InvalidHandle)
		{
			if (ImmediateUpload == InvalidHandle)
				ImmediateUpload = OpenUploadQueue();

			handle = ImmediateUpload;
		}

		return copyEngine[handle];
	}


	/************************************************************************************************/


	void RenderSystem::_OnCrash()
	{
		static std::mutex m;

		if (!m.try_lock())
		{
			DebugBreak();
			return;
		}

		EXITSCOPE(m.unlock());

		auto reason = pDevice->GetDeviceRemovedReason();

		switch (reason)
		{
		case DXGI_ERROR_DEVICE_HUNG:
			FK_LOG_ERROR("DXGI_ERROR_DEVICE_HUNG");				break;
		case DXGI_ERROR_DEVICE_REMOVED:
			FK_LOG_ERROR("DXGI_ERROR_DEVICE_REMOVED");			break;
		case DXGI_ERROR_DEVICE_RESET:
			FK_LOG_ERROR("DXGI_ERROR_DEVICE_RESET");			break;
		case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
			FK_LOG_ERROR("DXGI_ERROR_DRIVER_INTERNAL_ERROR");	break;
		case DXGI_ERROR_INVALID_CALL:
			FK_LOG_ERROR("DXGI_ERROR_INVALID_CALL");			break;
		case S_OK:
			FK_LOG_ERROR("???? S_OK ????");						break;
		}

#if USING(AFTERMATH)
		{
			/*
			// Decode the crash dump to a JSON string.
			// Step 1: Generate the JSON and get the size.
			uint32_t jsonSize = 0;
			GFSDK_Aftermath_GpuCrashDump_GenerateJSON(
				decoder,
				GFSDK_Aftermath_GpuCrashDumpDecoderFlags_ALL_INFO,
				GFSDK_Aftermath_GpuCrashDumpFormatterFlags_NONE,
				RenderSystem::OnShaderDebugInfoLookup,
				RenderSystem::OnShaderLookup,
				RenderSystem::OnShaderInstructionsLookup,
				RenderSystem::OnShaderSourceDebugInfoLookup,
				this,
				&jsonSize);

			// Step 2: Allocate a buffer and fetch the generated JSON.
			std::vector<char> json(jsonSize);
			GFSDK_Aftermath_GpuCrashDump_GetJSON(
				decoder,
				uint32_t(json.size()),
				json.data());

			// Write the the crash dump data as JSON to a file.
			const std::string jsonFileName = crashDumpFileName + ".json";
			std::ofstream jsonFile(jsonFileName, std::ios::out | std::ios::binary);
			if (jsonFile)
			{
				jsonFile.write(json.data(), json.size());
				jsonFile.close();
			}
			std::lock_guard lock{ crashM };
			WriteGpuCrashDumpToFile(gpuCrashDrump, gpuCrashDumpSize);
			*/

			GFSDK_Aftermath_Device_Status           device_status;


			std::vector<GFSDK_Aftermath_ContextHandle> context_handles;
			std::vector<GFSDK_Aftermath_ContextData> context_crash_info{ Contexts.size() };

			for (auto& context : Contexts) {
				context_handles.push_back(context.AFTERMATH_context);
			}

			GFSDK_Aftermath_GetData(context_handles.size(), context_handles.data(), context_crash_info.data());

			std::vector<GFSDK_Aftermath_ContextData> faultedContexts;

			for (auto& contextInfo : context_crash_info)
			{
				if (contextInfo.status == GFSDK_Aftermath_Context_Status_Invalid) {
					faultedContexts.push_back(contextInfo);
					GFSDK_Aftermath_GetContextError(&contextInfo);
				}

			}





			GFSDK_Aftermath_GetDeviceStatus(&device_status);

			Sleep(3000); // Aftermath needs a moment to do it's crash dump

			switch(device_status)
			{
			case GFSDK_Aftermath_Device_Status_Active:
				FK_LOG_ERROR("GFSDK_Aftermath_Device_Status_Active"); break;
			case GFSDK_Aftermath_Device_Status_Timeout:
				FK_LOG_ERROR("GFSDK_Aftermath_Device_Status_Timeout"); break;
			case GFSDK_Aftermath_Device_Status_OutOfMemory:
				FK_LOG_ERROR("GFSDK_Aftermath_Device_Status_OutOfMemory"); break;
			case GFSDK_Aftermath_Device_Status_PageFault:
				FK_LOG_ERROR("GFSDK_Aftermath_Device_Status_PageFault"); break;
			case GFSDK_Aftermath_Device_Status_Stopped:
				FK_LOG_ERROR("GFSDK_Aftermath_Device_Status_Stopped"); break;
			case GFSDK_Aftermath_Device_Status_Reset:
				FK_LOG_ERROR("GFSDK_Aftermath_Device_Status_Reset"); break;
			case GFSDK_Aftermath_Device_Status_Unknown:
				FK_LOG_ERROR("GFSDK_Aftermath_Device_Status_Unknown"); break;
			case GFSDK_Aftermath_Device_Status_DmaFault:
			{
				GFSDK_Aftermath_PageFaultInformation    pageFault;

				memset(&pageFault, 0, sizeof(pageFault));

				GFSDK_Aftermath_GetPageFaultInformation(&pageFault);

				if (pageFault.bHasPageFaultOccured) {
					FK_LOG_ERROR("GFSDK_Aftermath_Device_Status_DmaFault : %u", pageFault.resourceDesc.ptr_align_pAppResource);

					auto res = Textures.FindResourceHandle(pageFault.resourceDesc.pAppResource);
					if(res != InvalidHandle)
					{
						auto debugString = Textures.GetDebug(res);
						if (debugString)
							FK_LOG_ERROR("DMA FAULT DETECTED INVOLVING RESOURCE: %s!", debugString);
						else
							FK_LOG_ERROR("DMA FAULT DETECTED!");
					}
				}
				

			}   break;
			};


			Sleep(3000); // Aftermath needs a moment to do it's crash dump

			__debugbreak();

			for (auto& context : Contexts)
				GFSDK_Aftermath_ReleaseContextHandle(context.AFTERMATH_context);


			exit(-1);
		}
#else

#if USING(ENABLEDRED)

		if (FAILED(pDevice->QueryInterface(&dred)))
			FK_LOG_ERROR("CRASH DETECTED, DRED NOT ENABLED!");
		else
			FK_LOG_ERROR("Dumping Breadcrumbs");

		D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 DredAutoBreadcrumbsOutput	= {};
		D3D12_DRED_PAGE_FAULT_OUTPUT		DredPageFaultOutput			= {};

		DebugBreak();

		std::this_thread::sleep_for(1s);

		if (auto HR = dred->GetAutoBreadcrumbsOutput1(&DredAutoBreadcrumbsOutput); FAILED(HR))
		{
			FK_LOG_ERROR("Failed to get Breadcrumbs!");
			exit(-1);
		}

		if (auto HR = dred->GetPageFaultAllocationOutput(&DredPageFaultOutput); FAILED(HR))
			FK_LOG_ERROR("Failed to get Fault Allocation Info!");

		const D3D12_AUTO_BREADCRUMB_NODE1* node = DredAutoBreadcrumbsOutput.pHeadAutoBreadcrumbNode;

		if(!node) 
			FK_LOG_ERROR("No Breadcrumbs!");

		while (node)
		{
			auto event = *node->pCommandHistory;
			if (event)
			{
				switch (event)
				{
				case D3D12_AUTO_BREADCRUMB_OP_SETMARKER:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_SETMARKER"); break;
				case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT: command list %s", node->pCommandListDebugNameA); break;
				case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_ENDEVENT"); break;
				case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED"); break;
				case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED"); break;
				case D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT"); break;
				case D3D12_AUTO_BREADCRUMB_OP_DISPATCH:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_DISPATCH"); break;
				case D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION"); break;
				case D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION"); break;
				case D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE"); break;
				case D3D12_AUTO_BREADCRUMB_OP_COPYTILES:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_COPYTILES"); break;
				case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE"); break;
				case D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW"); break;
				case D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW"); break;
				case D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW"); break;
				case D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER"); break;
				case D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE"); break;
				case D3D12_AUTO_BREADCRUMB_OP_PRESENT:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_PRESENT"); break;
				case D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA"); break;
				case D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION"); break;
				case D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION"); break;
				case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME"); break;
				case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES"); break;
				case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT"); break;
				case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64"); break;
				case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION"); break;
				case D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE"); break;
				case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1"); break;
				case D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION"); break;
				case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2"); break;
				case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1"); break;
				case D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE"); break;
				case D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO"); break;
				case D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE"); break;
				case D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS"); break;
				case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND"); break;
				case D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND"); break;
				case D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION"); break;
				case D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP"); break;
				case D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1"); break;
				case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND"); break;
				case D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND"); break;
				case D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH: 
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH");  break;
				};
			}

			const size_t ctxCount = node->BreadcrumbContextsCount;
			for (size_t i = 0; i < ctxCount; i++)
			{
				std::wcout << "BreadCrumb: " << node->pBreadcrumbContexts[i].BreadcrumbIndex << " | " << node->pBreadcrumbContexts[i].pContextString << '\n';
			}
			node = node->pNext;
		}
#endif
		__debugbreak();
		exit(-1);
#endif
	}


	/************************************************************************************************/

#if USING(AFTERMATH)

	void RenderSystem::WriteGpuCrashDumpToFile(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize)
	{
		FK_LOG_ERROR("RenderSystem::WriteGpuCrashDumpToFile");
	}


	void RenderSystem::GpuCrashDumpCallback(const void* gpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData)
	{
		FK_LOG_ERROR("RenderSystem::GpuCrashDumpCallback");

		RenderSystem*   renderSystem = reinterpret_cast<RenderSystem*>(pUserData);

		std::vector<GFSDK_Aftermath_ContextHandle> context_handles;
		std::vector<GFSDK_Aftermath_ContextData> context_crash_info{ renderSystem->Contexts.size() };


		GFSDK_Aftermath_Device_Status           device_status;
		GFSDK_Aftermath_PageFaultInformation    pafeFault;

		for (auto& context : renderSystem->Contexts)
			context_handles.push_back(context.AFTERMATH_context);

		GFSDK_Aftermath_GetDeviceStatus(&device_status);
		GFSDK_Aftermath_GetData(context_handles.size(), context_handles.data(), context_crash_info.data());
		GFSDK_Aftermath_GetPageFaultInformation(&pafeFault);

		GFSDK_Aftermath_GpuCrashDump_Decoder decoder = {};

		auto res = GFSDK_Aftermath_GpuCrashDump_CreateDecoder(
			GFSDK_Aftermath_Version_API,
			gpuCrashDump,
			gpuCrashDumpSize,
			&decoder);

		GFSDK_Aftermath_GpuCrashDump_BaseInfo baseInfo = {};
		GFSDK_Aftermath_GpuCrashDump_GetBaseInfo(decoder, &baseInfo);

		uint32_t applicationNameLength = 0;
		GFSDK_Aftermath_GpuCrashDump_GetDescriptionSize(
			decoder,
			GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName,
			&applicationNameLength);

		std::vector<char> applicationName(applicationNameLength, '\0');

		GFSDK_Aftermath_GpuCrashDump_GetDescription(
			decoder,
			GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName,
			uint32_t(applicationName.size()),
			applicationName.data());

		static int count = 0;
		const std::string baseFileName =
			std::string("TestGame")
			+ "-"
			+ std::to_string(baseInfo.pid)
			+ "-"
			+ std::to_string(++count);

		const std::string crashDumpFileName = baseFileName + ".nv-gpudmp";
		std::ofstream dumpFile(crashDumpFileName, std::ios::out | std::ios::binary);
		if (dumpFile)
		{
			dumpFile.write((const char*)gpuCrashDump, gpuCrashDumpSize);
			dumpFile.close();
		}
	}



	void RenderSystem::ShaderDebugInfoCallback(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize, void* pUserData)
	{
		RenderSystem* renderSystem = reinterpret_cast<RenderSystem*>(pUserData);
		__debugbreak();
	}


	void RenderSystem::CrashDumpDescriptionCallback(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription, void* pUserData)
	{
		RenderSystem* renderSystem = reinterpret_cast<RenderSystem*>(pUserData);
		__debugbreak();
	}



	void RenderSystem::OnShaderDebugInfoLookup(const GFSDK_Aftermath_ShaderDebugInfoIdentifier* pIdentifier, PFN_GFSDK_Aftermath_SetData setShaderDebugInfo, void* pUserData)
	{
		__debugbreak();
	}



	void RenderSystem::OnShaderLookup(const GFSDK_Aftermath_ShaderHash* pShaderHash, PFN_GFSDK_Aftermath_SetData setShaderBinary, void* pUserData)
	{
		__debugbreak();
	}



	void RenderSystem::OnShaderInstructionsLookup(const GFSDK_Aftermath_ShaderInstructionsHash* pShaderInstructionsHash, PFN_GFSDK_Aftermath_SetData setShaderBinary, void* pUserData)
	{
		__debugbreak();
	}



	void RenderSystem::OnShaderSourceDebugInfoLookup(const GFSDK_Aftermath_ShaderDebugName* pShaderDebugName, PFN_GFSDK_Aftermath_SetData setShaderBinary, void* pUserData)
	{
		__debugbreak();
	}
#endif


	bool RenderSystem::DEBUG_AttachPIX()
	{
#if USING(PIX)
		if (pix)
			return true;

		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pix))))
		{
			FK_LOG_INFO("Pix Attached.");
			return true;
		}
#endif
		return false;
	}


	bool RenderSystem::DEBUG_BeginPixCapture()
	{
#if USING(PIX)
		if (pix)
		{
			pix->BeginCapture();
			FK_LOG_INFO("Pix capture started.");
		}

		return pix != nullptr;
#else
		return false;
#endif
	}


	bool RenderSystem::DEBUG_EndPixCapture()
	{
#if USING(PIX)
		if (pix)
		{
			pix->EndCapture();
			FK_LOG_INFO("Pix capture ended.");
		}

		return pix != nullptr;
#else
		return false;
#endif
	}


	/************************************************************************************************/


	ResourceHandle RenderSystem::_CreateDefaultTexture()
	{
		char tempBuffer[256];
		memset(tempBuffer, 0xffff, 256);

		auto upload = OpenUploadQueue();

		TextureBuffer textureBuffer{ { 1,  1 }, (std::byte*)tempBuffer, 256, 4, nullptr };

		auto defaultTexture = MoveTextureBuffersToVRAM(
			this,
			upload,
			&textureBuffer,
			1,
			1,
			DeviceFormat::R8G8B8A8_UNORM);

		SetDebugName(defaultTexture, "Default Texture");
		SubmitUploadQueues(&upload);

		SyncDirectTo(SyncUploadTicket());

		return defaultTexture;
	}


	/************************************************************************************************/


	UploadReservation RenderSystem::ReserveDirectUploadSpace(size_t size, size_t alignment) noexcept
	{
		std::scoped_lock lock{ directUploadBufferMutex };
		auto res = directUploadBuffer.Reserve(size, alignment);

		if (res)
			return res.value();
		else if(res.error() == ReserveErrors::OutOfSpace)
		{
			auto oldBuffer = directUploadBuffer.Resize(directUploadBuffer.Size * 2);

			if (oldBuffer)
			{
				FreeList_GraphicsQueue.emplace_back(
					oldBuffer,
					directSubmissionCounter.load(std::memory_order_relaxed));
			}

			return directUploadBuffer.Reserve(size, alignment).value_or(UploadReservation{});
		}

		return UploadReservation{};
	}


	/************************************************************************************************/


	const IRootSignature* RenderSystem::Library(ROOTLIBRARYSIG ID) const noexcept
	{
		switch (ID)
		{
		case ROOTLIBRARYSIG::RS2UAVs4SRVs4CBs:
			return rootLibrary.RS2UAVs4SRVs4CBs;
		case ROOTLIBRARYSIG::RS6CBVs4SRVs:
			return rootLibrary.RS6CBVs4SRVs;
		case ROOTLIBRARYSIG::RS4CBVs_SO:
			return rootLibrary.RS4CBVs_SO;
		case ROOTLIBRARYSIG::ShadingRTSig:
			return rootLibrary.ShadingRTSig;
		case ROOTLIBRARYSIG::RSDefault:
			return rootLibrary.RSDefault;
		case ROOTLIBRARYSIG::ComputeSignature:
			return rootLibrary.ComputeSignature;
		case ROOTLIBRARYSIG::ClearBuffer:
			return rootLibrary.ClearBuffer;
		default:
			FK_ASSERT(0, "Invalid ROOTLIBRARYSIG ID passed to RenderSystem::Library(ROOTLIBRARYSIG)");
		}

		std::unreachable();
	}


	/************************************************************************************************/


	RootSignature* RenderSystem::_CreateRootSignature(ID3D12RootSignature* rootSig, RootSignatureBuilder& builder)
	{
		{
			std::shared_lock lock{ rootSignatureLock };

			if (auto res = rootSignatures[(uint64_t)rootSig]; res != nullptr)
				return res->get();
		}


		auto& object			= Memory->allocate_aligned<RootSignature>(rootSig, std::move(builder.Heaps), Memory);
		auto object_ptr			= RootSignature_ptr(&object, RootSignatureDeleter{ Memory });

		std::unique_lock unique{ rootSignatureLock };
		auto rootSignatureEntry = rootSignatures.insert((uint64_t)rootSig, std::move(object_ptr));

		return &object;
	}


	/************************************************************************************************/


	RootSignature* RenderSystem::_CreateRootSignature(RootSignatureBuilder& builder, iAllocator& temp)
	{
		Vector<Vector<CD3DX12_DESCRIPTOR_RANGE, 16>, 16> DesciptorHeaps{ temp };

		static_vector<CD3DX12_ROOT_PARAMETER> Parameters;

		for (const auto& I : builder.RootEntries)
		{
			CD3DX12_ROOT_PARAMETER Param;

			switch (I.Type)
			{
			case RootSignatureEntryType::UINT:
			{
				Param.InitAsConstants(
					I.UINTConstant.size,
					I.UINTConstant.Register,
					I.UINTConstant.RegisterSpace,
					PipelineDest2ShaderVis(I.UINTConstant.Accessibility));
			}   break;
			case RootSignatureEntryType::DescriptorHeap:
			{
				const auto  HeapIdx		= I.DescriptorHeap.HeapIdx;
				const auto& HeapEntry	= builder.Heaps[HeapIdx];

				DesciptorHeaps.push_back(Vector<CD3DX12_DESCRIPTOR_RANGE>(temp));

				for (auto& H : HeapEntry.Heap.Entries)
				{
					D3D12_DESCRIPTOR_RANGE_TYPE RangeType;
					switch (H.Type)
					{
					case DescHeapEntryType::ConstantBuffer:
						RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
						break;
					case DescHeapEntryType::ShaderResource:
						RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
						break;
					case DescHeapEntryType::UAVBuffer:
						RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
						break;
					case DescHeapEntryType::HeapError:
					default:
						FK_ASSERT(false);
						break;
					}

					CD3DX12_DESCRIPTOR_RANGE Range;
					Range.Init(
						RangeType,
						H.Count, H.Register, H.Space);

					DesciptorHeaps.back().push_back(Range);
				}

				Param.InitAsDescriptorTable(
					(UINT)DesciptorHeaps.back().size(),
					DesciptorHeaps.back().begin(),
					PipelineDest2ShaderVis(I.DescriptorHeap.Accessibility));
			}	break;
			case RootSignatureEntryType::ConstantBuffer:
			{
				Param.InitAsConstantBufferView
				(I.Direct.Register,
					I.Direct.RegisterSpace,
					PipelineDest2ShaderVis(I.Direct.Accessibility));

			}	break;
			case RootSignatureEntryType::StructuredBuffer:
			{
				Param.InitAsShaderResourceView(
					I.Direct.Register,
					I.Direct.RegisterSpace,
					PipelineDest2ShaderVis(I.Direct.Accessibility));
			}	break;
			case RootSignatureEntryType::UnorderedAcess:
			{
				Param.InitAsUnorderedAccessView(
					I.Direct.Register,
					I.Direct.RegisterSpace,
					PipelineDest2ShaderVis(I.Direct.Accessibility));
			}   break;
			default:
				return nullptr;
				FK_ASSERT(false);
			}
			Parameters.push_back(Param);
		}

		ID3DBlob* SignatureBlob		= nullptr;
		ID3DBlob* ErrorBlob			= nullptr;

		CD3DX12_STATIC_SAMPLER_DESC Default(0);
		CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;

		CD3DX12_STATIC_SAMPLER_DESC	 Samplers[] = {
			CD3DX12_STATIC_SAMPLER_DESC{0, D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP},

			CD3DX12_STATIC_SAMPLER_DESC{1, D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT },
			CD3DX12_STATIC_SAMPLER_DESC{2,  D3D12_FILTER::D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
											0, 1, D3D12_COMPARISON_FUNC_LESS_EQUAL,
											D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK }
		};

		RootSignatureDesc.Init((UINT)Parameters.size(), Parameters.begin(), 1, &Default);
		RootSignatureDesc.pStaticSamplers	= builder.LocalRoot ? nullptr : Samplers;
		RootSignatureDesc.NumStaticSamplers = builder.LocalRoot ? 0 : 3;

		RootSignatureDesc.Flags |= builder.AllowIA ?
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT :
			D3D12_ROOT_SIGNATURE_FLAG_NONE;

		RootSignatureDesc.Flags |= builder.AllowSO ?
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT :
			D3D12_ROOT_SIGNATURE_FLAG_NONE;

		RootSignatureDesc.Flags |= builder.LocalRoot ?
			D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE :
			D3D12_ROOT_SIGNATURE_FLAG_NONE;

		HRESULT HR = D3D12SerializeRootSignature(
			&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			&SignatureBlob, &ErrorBlob);

		if (!SUCCEEDED(HR))
		{
			std::cout << (char*)ErrorBlob->GetBufferPointer() << '\n';
			ErrorBlob->Release();

#ifdef _DEBUG 
			FK_ASSERT(false, "Invalid Root Signature Description!");
#endif

			return nullptr;
		}

		ID3D12RootSignature* rootSignature = nullptr;
		auto CreateHR = pDevice14->CreateRootSignature(0, SignatureBlob->GetBufferPointer(), SignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

		if (FAILED(CreateHR))
		{
			FK_LOG_ERROR("RenderSystem: Failed to create root signature!");
			return nullptr;
		}


		std::shared_lock lock{ rootSignatureLock };

		if (auto res = rootSignatures[(uint64_t)rootSignature]; res != nullptr)
			return res->get();

		lock.unlock();

		auto& object			= Memory->allocate_aligned<RootSignature>(rootSignature, std::move(builder.Heaps), Memory);
		auto object_ptr			= RootSignature_ptr(&object, RootSignatureDeleter{ Memory });

		std::unique_lock unique{ rootSignatureLock };

		auto rootSignatureEntry = rootSignatures.insert((uint64_t)rootSignature, std::move(object_ptr));

		builder.Clear();

		return rootSignatureEntry->get();
	}


	/************************************************************************************************/


	RootSignature* RenderSystem::_GetRootSignature(uint64_t hashID) const
	{
		std::shared_lock lock{ const_cast<std::shared_mutex&>(rootSignatureLock) };
		auto sig = rootSignatures[hashID];

		return sig ? sig->get() : nullptr;
	}


	/************************************************************************************************/


	void RenderSystem::_ReleaseRootSignature(uint64_t hashID)
	{
		std::unique_lock lock{ rootSignatureLock };

		rootSignatures.remove(hashID);
	}


	/************************************************************************************************/


	IDirectContext& RenderSystem::GetDirectCommandList(std::optional<SyncPoint> ticket)
	{
		const uint64_t submissionId		= ticket ? ticket.value().syncCounter : ++directSubmissionCounter;

		while (true)
		{
			const uint64_t completedCounter = directFence->GetCompletedValue();

			uint64_t lowest = -1;

			for (auto& _ : Contexts)
			{
				const size_t idx = contextIdx;
				contextIdx = ++contextIdx % Contexts.size();

				Context& context = Contexts[idx];
				if (context._GetCounter() <= completedCounter)
				{
					auto range = _AllocateDescriptorRange(1024);

					if (!range.has_value())
						FK_LOG_ERROR("Failed to Allocate descriptor range!");

					return context.Reset(range.value(), submissionId, descriptorHeapAllocator.Heap());
				}
				else
					lowest = Min(lowest, context._GetCounter());
			}

			WaitFor(lowest);
		}

		std::unreachable();

		return Contexts[0];
	}


	/************************************************************************************************/


	void RenderSystem::SyncUploadTo(SyncPoint sp)
	{
		FK_LOG_9("QUEUE:DIRECT signaling: %I64 : %I64\n", sp.fence, sp.syncCounter);

		copyEngine.copyQueue->Wait(sp.fence, sp.syncCounter);
	}

	SyncPoint RenderSystem::SyncUploadPoint()
	{
		return { copyEngine.counter, copyEngine.fence };
	}

	SyncPoint RenderSystem::SyncUploadTicket()
	{

		const uint64_t counter = ++copyEngine.counter;
		copyEngine.copyQueue->Signal(copyEngine.fence, counter);

		FK_LOG_9("QUEUE:DIRECT signaling: %I64 : %I64\n", copyEngine.fence, counter);

		return { counter, copyEngine.fence };
	}

	void RenderSystem::SyncDirectTo(SyncPoint sp)
	{
		FK_LOG_9("QUEUE:DIRECT signaling: %I64 : %I64\n", sp.fence, sp.syncCounter);

		GraphicsQueue->Wait(sp.fence, sp.syncCounter);
	}

	SyncPoint RenderSystem::SyncDirectPoint()
	{
		return { directSubmissionCounter, directFence };
	}

	SyncPoint RenderSystem::SyncSubmittedDirectPoint()
	{
		return { directSubmittedCounter, directFence };
	}

	SyncPoint RenderSystem::SyncDirectTicket()
	{
		auto counter = ++directSubmissionCounter;
		GraphicsQueue->Signal(directFence, counter);

		FK_LOG_9("QUEUE:DIRECT signaling: %I64 : %I64\n", directFence, counter);

		return { counter, directFence };
	}

	void RenderSystem::SignalDirect(uint64_t value)
	{
		if (value > directSubmissionCounter)
			DebugBreak();

		FK_LOG_9("QUEUE:DIRECT signaling: %I64\n", value);

		GraphicsQueue->Signal(directFence, value);
		directSubmittedCounter = Max(value, directSubmittedCounter);
	}

	/************************************************************************************************/


	SyncPoint RenderSystem::GetSubmissionTicket(uint32_t count)
	{
		auto value = directSubmissionCounter.fetch_add(count) + count;

		return { value, directFence };
	}


	/************************************************************************************************/


	SyncPoint RenderSystem::Submit(std::span<IDirectContext*> contexts, std::optional<SyncPoint> syncOptional)
	{
		ProfileFunction();

		if (ImmediateUpload != InvalidHandle)
		{
			SubmitUploadQueues(&ImmediateUpload, 1, {});
			ImmediateUpload = InvalidHandle;

			SyncDirectTo(SyncUploadTicket());
		}

		static_vector<ID3D12CommandList*, 64> cls;

		uint64_t dispatchIdx = 0;

		for (auto context : contexts)
		{
			auto deviceContext = static_cast<Context*>(context);
			dispatchIdx = Max(deviceContext->dispatchIdx, dispatchIdx);

			cls.push_back(deviceContext->GetCommandList());
			deviceContext->Close();
		}

		if (auto sync = syncOptional.value_or(SyncPoint{}); syncOptional.has_value())
			GraphicsQueue->Wait(sync.fence, sync.syncCounter);

		try
		{
			GraphicsQueue->ExecuteCommandLists((UINT)cls.size(), cls.begin());
		}
		catch (...)
		{
			DebugBreak();
		}

		if (auto HR = GraphicsQueue->Signal(directFence, dispatchIdx); FAILED(HR))
			FK_LOG_ERROR("Failed to Signal");

		for (auto context : contexts)
			static_cast<Context*>(context)->_QueueReadBacks();

		directUploadBuffer.Last = directUploadBuffer.Position;

		FK_LOG_9("QUEUE:DIRECT Submitting. Signaling: %I64 : $I64 \n", directFence, dispatchIdx);

		return { dispatchIdx, directFence };
	}


	/************************************************************************************************/


	void RenderSystem::EndFrame()
	{
		static auto dispatchIdx = directSubmissionCounter.load(std::memory_order_relaxed);

		VertexBuffers.LockUntil(dispatchIdx);
		Textures.LockUntil(dispatchIdx);

		ReadBackTable.Update();
	}


	/************************************************************************************************/


	void RenderSystem::Signal(SyncPoint syncPoint)
	{
		if (auto HR = GraphicsQueue->Signal(syncPoint.fence, syncPoint.syncCounter); FAILED(HR))
			FK_LOG_ERROR("Failed to Signal");
	}


	/************************************************************************************************/


	void RenderSystem::SubmitUploadQueues(CopyContextHandle* handles, size_t count, std::optional<SyncPoint> syncBefore, std::optional<SyncPoint> syncAfter)
	{
		if(syncBefore)
			copyEngine.Wait(syncBefore.value());

		if(count)
			copyEngine.Submit(handles, handles + count);

		if(syncAfter)
			copyEngine.Signal(syncAfter.value());
	}


	/************************************************************************************************/


	CopyContextHandle RenderSystem::OpenUploadQueue()
	{
		return copyEngine.Open();
	}


	/************************************************************************************************/


	CopyContextHandle RenderSystem::GetImmediateCopyQueue()
	{
		if (ImmediateUpload == InvalidHandle)
			ImmediateUpload = copyEngine.Open();

		return ImmediateUpload;
	}


	/************************************************************************************************/
	

	bool CreateInputLayout(RenderSystem* RS, VertexBufferView** Buffers, size_t count, Shader* Shader, VertexBufferSet* DVB_Out)
	{
		InputDescription Input_Desc;

			// Index Counters
		size_t POS_Buffer_Counter		= 0;
		size_t INDEX_Buffer_Counter		= 0;
		size_t UV_Buffer_Counter		= 0;
		size_t WEIGHT_Buffer_Counter	= 0;
		size_t WINDICES_Buffer_Counter  = 0;
		size_t COLOR_Buffer_Counter		= 0;
		size_t Normal_Buffer_Counter	= 0;
		size_t Tangent_Buffer_Counter	= 0;
		size_t InputSlot				= 0;

		// Try and Guess the Input Layout
		size_t itr = 0;
		for( ; itr < count; itr++ )
		{
			if (Buffers[itr])
			{
				switch (Buffers[itr]->GetBufferType())
				{
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION:
				{
					switch (Buffers[itr]->GetBufferFormat())
					{
					case VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R32G32B32_FLOAT;
						InputElementDesc.InputSlot            = static_cast<UINT>(++InputSlot);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = (UINT)POS_Buffer_Counter++;
						InputElementDesc.SemanticName         = "POSITION";

						Input_Desc.push_back(InputElementDesc);
					}
					break;
					default:
						FK_ASSERT(0);
					}
					break;
				}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX:
					break;
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_COLOR:
				{
					switch (Buffers[itr]->GetBufferFormat())
					{
					case VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32A32:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R32G32B32A32_FLOAT;
						InputElementDesc.InputSlot            = static_cast<UINT>(InputSlot++);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = (UINT)COLOR_Buffer_Counter++;
						InputElementDesc.SemanticName         = "COLOR";

						Input_Desc.push_back(InputElementDesc);
					}
					break;
					default:
						FK_ASSERT(0);
					}
					break;
				}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL:
				{
					switch (Buffers[itr]->GetBufferFormat())
					{
					case VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R32G32B32_FLOAT;
						InputElementDesc.InputSlot            = static_cast<UINT>(InputSlot++);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = (UINT)Normal_Buffer_Counter++;
						InputElementDesc.SemanticName         = "NORMAL";

						Input_Desc.push_back(InputElementDesc);
					}
					break;
					default:
						FK_ASSERT(0);
					}
					break;
				}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT:
				{
					switch (Buffers[itr]->GetBufferFormat())
					{
					case VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R32G32B32_FLOAT;
						InputElementDesc.InputSlot            = static_cast<UINT>(InputSlot++);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = (UINT)Tangent_Buffer_Counter++;
						InputElementDesc.SemanticName         = "TANGENT";

						Input_Desc.push_back(InputElementDesc);
					}
					break;
					default:
						FK_ASSERT(0);
					}
					break;
				}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV:
				{
					switch (Buffers[itr]->GetBufferFormat())
					{
					case VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R32G32_FLOAT;
						InputElementDesc.InputSlot            = static_cast<UINT>(InputSlot++);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = (UINT)UV_Buffer_Counter++;
						InputElementDesc.SemanticName         = "TEXCOORD";

						Input_Desc.push_back(InputElementDesc);
					}
					break;
					default:
						FK_ASSERT(0);
					}
					UV_Buffer_Counter++;
					break;
				}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1:
				{
					switch (Buffers[itr]->GetBufferFormat())
					{
					case VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R32G32B32_FLOAT;
						InputElementDesc.InputSlot            = static_cast<UINT>(InputSlot++);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = (UINT)WEIGHT_Buffer_Counter++;
						InputElementDesc.SemanticName         = "WEIGHTS";

						Input_Desc.push_back(InputElementDesc);
					}
					break;
					default:
						FK_ASSERT(0);
					}
					WEIGHT_Buffer_Counter++;
					break;
				}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2:
				{
					switch (Buffers[itr]->GetBufferFormat())
					{
					case VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32A32:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R32G32B32A32_UINT;
						InputElementDesc.InputSlot            = static_cast<UINT>(InputSlot++);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = (UINT)WINDICES_Buffer_Counter++;
						InputElementDesc.SemanticName         = "WEIGHTINDICES";

						Input_Desc.push_back(InputElementDesc);
					}
					case VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R16G16B16A16:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R16G16B16A16_UINT;
						InputElementDesc.InputSlot            = static_cast<UINT>(InputSlot++);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = (UINT)WINDICES_Buffer_Counter++;
						InputElementDesc.SemanticName         = "WEIGHTINDICES";

						Input_Desc.push_back(InputElementDesc);
					}
					break;
					default:
						FK_ASSERT(0);
					}
					WINDICES_Buffer_Counter++;
					break;
				}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_PACKED:
				{
					D3D12_INPUT_ELEMENT_DESC InputElementDesc;
					InputElementDesc.AlignedByteOffset		= 0;
					InputElementDesc.Format					= ::DXGI_FORMAT_R32G32B32_FLOAT;
					InputElementDesc.InputSlot				= static_cast<UINT>(InputSlot++);
					InputElementDesc.InputSlotClass			= D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
					InputElementDesc.InstanceDataStepRate	= 0;

					InputElementDesc.SemanticIndex			= (UINT)POS_Buffer_Counter++;
					InputElementDesc.SemanticName			= "POSITION";
					Input_Desc.push_back(InputElementDesc);
					InputElementDesc.AlignedByteOffset		= 16;
					InputElementDesc.SemanticName			= "NORMAL";
					InputElementDesc.SemanticIndex			= (UINT)Normal_Buffer_Counter++;
					Input_Desc.push_back(InputElementDesc);
					InputElementDesc.AlignedByteOffset		= 32;
					InputElementDesc.SemanticName			= "TANGENT";
					InputElementDesc.SemanticIndex			= (UINT)Tangent_Buffer_Counter++;
					Input_Desc.push_back(InputElementDesc);
					InputElementDesc.AlignedByteOffset		= 48;
					InputElementDesc.SemanticName			= "TEXCOORD";
					InputElementDesc.SemanticIndex			= (UINT)UV_Buffer_Counter++;
					InputElementDesc.Format					= ::DXGI_FORMAT_R32G32_FLOAT;
					Input_Desc.push_back(InputElementDesc);
				}	break;
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_PACKEDANIMATION:
				{
					D3D12_INPUT_ELEMENT_DESC InputElementDesc;
					InputElementDesc.AlignedByteOffset		= 0;
					InputElementDesc.Format					= ::DXGI_FORMAT_R32G32B32_FLOAT;
					InputElementDesc.InputSlot				= static_cast<UINT>(InputSlot++);
					InputElementDesc.InputSlotClass			= D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
					InputElementDesc.InstanceDataStepRate	= 0;

					InputElementDesc.SemanticIndex			= (UINT)WEIGHT_Buffer_Counter++;
					InputElementDesc.SemanticName			= "WEIGHTS";
					Input_Desc.push_back(InputElementDesc);

					InputElementDesc.AlignedByteOffset		= 12;
					InputElementDesc.SemanticIndex			= (UINT)WINDICES_Buffer_Counter++;
					InputElementDesc.SemanticName			= "WEIGHTINDICES";
					InputElementDesc.Format					= ::DXGI_FORMAT_R16G16B16A16_UINT;
					Input_Desc.push_back(InputElementDesc);
				}	break;
				default:
					break;
				}
			}
		}

		for (size_t I= 0; I < Input_Desc.size(); ++I)
			DVB_Out->MD.InputLayout[I] = Input_Desc[I];
		
		DVB_Out->MD.InputElementCount = Input_Desc.size();

		return true;
	}
	


	/************************************************************************************************/
	

	void Release( ConstantBuffer& buffer )
	{
		buffer.Release();
	}
	

	/************************************************************************************************/
	

	void Release( Texture2D txt2d )
	{
		if (txt2d)
			txt2d->Release();
	}
	

	/************************************************************************************************/


	DescHeapPOS PushRenderTarget(RenderSystem* RS, ResourceHandle target, DescHeapPOS POS, const size_t MIPOffset)
	{
		D3D12_RENDER_TARGET_VIEW_DESC TargetDesc = {};
		const auto dimension            = RS->GetTextureDimension(target);

		switch (dimension)
		{
		case TextureDimension::Texture2D:
			TargetDesc.Format               = TextureFormat2DXGIFormat(RS->GetTextureFormat(target));
			TargetDesc.Texture2D.MipSlice   = (UINT)MIPOffset;
			TargetDesc.Texture2D.PlaneSlice = 0;
			TargetDesc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
			break;
		case TextureDimension::TextureCubeMap:
			TargetDesc.Format                           = TextureFormat2DXGIFormat(RS->GetTextureFormat(target));
			TargetDesc.Texture2DArray.FirstArraySlice   = 0;
			TargetDesc.Texture2DArray.MipSlice          = (UINT)MIPOffset;
			TargetDesc.Texture2DArray.PlaneSlice        = 0;
			TargetDesc.Texture2DArray.ArraySize         = 6;
			TargetDesc.ViewDimension                    = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			break;
		}
			

		auto resource = RS->GetDeviceResource(target);
		RS->pDevice->CreateRenderTargetView(resource, &TargetDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorRTVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushDepthStencil(RenderSystem* RS, ResourceHandle Target, DescHeapPOS POS)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
		DSVDesc.Format				= RS->GetTextureDeviceFormat(Target);
		DSVDesc.Texture2D.MipSlice	= 0;
		DSVDesc.ViewDimension		= D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2D;

		RS->pDevice->CreateDepthStencilView(RS->GetDeviceResource(Target), &DSVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorDSVSize, 1);
	}

	DescHeapPOS PushDepthStencilArray(RenderSystem* RS, ResourceHandle Target, size_t arrayOffset, size_t MipSlice, DescHeapPOS POS, size_t IN_arraySize)
	{
		const size_t arraySize = IN_arraySize == -1 ? RS->GetTextureArraySize(Target) : IN_arraySize;

		D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
		DSVDesc.Format                          = RS->GetTextureDeviceFormat(Target);
		DSVDesc.Texture2DArray.ArraySize        = (UINT)(arraySize - arrayOffset);
		DSVDesc.Texture2DArray.FirstArraySlice  = (UINT)arrayOffset;
		DSVDesc.Texture2DArray.MipSlice         = (UINT)MipSlice;
		DSVDesc.ViewDimension                   = D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2DARRAY;

		RS->pDevice->CreateDepthStencilView(RS->GetDeviceResource(Target), &DSVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorDSVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushCBToDescHeap(RenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, size_t BufferSize, size_t Offset)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC CBV_DESC = {};
		CBV_DESC.BufferLocation = Buffer ? Buffer->GetGPUVirtualAddress() + Offset : 0;
		CBV_DESC.SizeInBytes	= (UINT)BufferSize;
		RS->pDevice->CreateConstantBufferView(&CBV_DESC, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushSRVToDescHeap(RenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, size_t ElementCount, size_t Stride, D3D12_BUFFER_SRV_FLAGS Flags, size_t offset)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC desc; {
			desc.Format						= DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
			desc.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			desc.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER;
			desc.Buffer.FirstElement		= offset;
			desc.Buffer.Flags				= D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE;
			desc.Buffer.NumElements			= (UINT)(ElementCount - offset);
			desc.Buffer.StructureByteStride	= (UINT)Stride;
		}

		FK_ASSERT(desc.Buffer.StructureByteStride < 512);
		FK_ASSERT(desc.Buffer.NumElements > 0);

		RS->pDevice->CreateShaderResourceView(Buffer, &desc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushSRVNULLDescHeap(RenderSystem* RS, DescHeapPOS POS)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC desc; {
			desc.Format						= DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
			desc.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			desc.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER;
			desc.Buffer.FirstElement		= 0;
			desc.Buffer.Flags				= D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE;
			desc.Buffer.NumElements			= 1;
			desc.Buffer.StructureByteStride	= 1;
		}

		FK_ASSERT(desc.Buffer.StructureByteStride < 512);
		FK_ASSERT(desc.Buffer.NumElements > 0);

		RS->pDevice->CreateShaderResourceView(nullptr, &desc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS Push2DSRVToDescHeap(RenderSystem* RS, ID3D12Resource* Buffer, const DescHeapPOS POS, const D3D12_BUFFER_SRV_FLAGS Flags, const DXGI_FORMAT format)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC desc; {
			desc.Format                         = format;
			desc.Shader4ComponentMapping        = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			desc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipLevels		    = 1;
			desc.Texture2D.MostDetailedMip	    = 0;
			desc.Texture2D.PlaneSlice		    = 0;
			desc.Texture2D.ResourceMinLODClamp	= 0;
		}

		RS->pDevice->CreateShaderResourceView(Buffer, &desc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });
		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushTextureToDescHeap(RenderSystem* RS, Texture2D tex, DescHeapPOS POS)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc   = {}; {
			ViewDesc.Format                        = tex.Format;
			ViewDesc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			ViewDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
			ViewDesc.Texture2D.MipLevels           = !tex.mipCount ? 1 : tex.mipCount;
			ViewDesc.Texture2D.MostDetailedMip     = 0;
			ViewDesc.Texture2D.PlaneSlice          = 0;
			ViewDesc.Texture2D.ResourceMinLODClamp = 0;
		}

		RS->pDevice->CreateShaderResourceView(tex, &ViewDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushTextureToDescHeap(RenderSystem* RS, DXGI_FORMAT format, ResourceHandle handle, DescHeapPOS POS)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc = {}; {
			const auto mipCount     = RS->GetTextureMipCount(handle);
			const auto arraySize    = RS->GetTextureArraySize(handle);

			ViewDesc.Format                             = format;
			ViewDesc.Shader4ComponentMapping            = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			ViewDesc.ViewDimension                      = arraySize > 1 ? D3D12_SRV_DIMENSION_TEXTURE2DARRAY : D3D12_SRV_DIMENSION_TEXTURE2D;
			ViewDesc.Texture2DArray.MipLevels           = Max(mipCount, 1);
			ViewDesc.Texture2DArray.MostDetailedMip     = 0;
			ViewDesc.Texture2DArray.PlaneSlice          = 0;
			ViewDesc.Texture2DArray.ResourceMinLODClamp = 0;
			ViewDesc.Texture2DArray.ArraySize           = (UINT)arraySize;
		}

		auto debug = RS->GetDeviceResource(handle);
		RS->pDevice->CreateShaderResourceView(RS->GetDeviceResource(handle), &ViewDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushTextureToDescHeap(RenderSystem* RS, DXGI_FORMAT format, uint32_t highestMipLevel, ResourceHandle handle, DescHeapPOS POS)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc = {}; {
			const auto mipCount     = RS->GetTextureMipCount(handle);
			const auto arraySize    = RS->GetTextureArraySize(handle);

			ViewDesc.Format                             = format;
			ViewDesc.Shader4ComponentMapping            = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			ViewDesc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2D;
			ViewDesc.Texture2DArray.MipLevels           = Max(mipCount - highestMipLevel, 1);
			ViewDesc.Texture2DArray.MostDetailedMip     = highestMipLevel;
			ViewDesc.Texture2DArray.PlaneSlice          = 0;
			ViewDesc.Texture2DArray.ResourceMinLODClamp = 0;
			ViewDesc.Texture2DArray.ArraySize           = (UINT)arraySize;
		}

		auto debug = RS->GetDeviceResource(handle);
		RS->pDevice->CreateShaderResourceView(RS->GetDeviceResource(handle), &ViewDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 } );

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushTexture3DToDescHeap(RenderSystem* RS, DXGI_FORMAT format, uint32_t mipCount, uint32_t highestDetailMip, uint32_t minLODClamp, ResourceHandle handle, DescHeapPOS POS)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc = {}; {
			const auto mipCount     = RS->GetTextureMipCount(handle);
			const auto arraySize    = RS->GetTextureArraySize(handle);

			ViewDesc.Format                             = format;
			ViewDesc.Shader4ComponentMapping            = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			ViewDesc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE3D;
			ViewDesc.Texture3D.MipLevels                = mipCount;
			ViewDesc.Texture3D.MostDetailedMip          = highestDetailMip;
			ViewDesc.Texture3D.ResourceMinLODClamp      = (float)minLODClamp;
		}

		auto debug = RS->GetDeviceResource(handle);
		RS->pDevice->CreateShaderResourceView(RS->GetDeviceResource(handle), &ViewDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushCubeMapTextureToDescHeap(RenderSystem* RS, ResourceHandle resource, DescHeapPOS POS, DeviceFormat format)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc = {}; {
			ViewDesc.Format                          = TextureFormat2DXGIFormat(format);
			ViewDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			ViewDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
			ViewDesc.TextureCube.MipLevels           = Max(RS->GetTextureMipCount(resource), 1);
			ViewDesc.TextureCube.MostDetailedMip     = 0;
			ViewDesc.TextureCube.ResourceMinLODClamp = 0;
		}

		RS->pDevice->CreateShaderResourceView(RS->GetDeviceResource(resource), &ViewDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushUAV2DToDescHeap(RenderSystem* RS, Texture2D tex, DescHeapPOS POS)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		UAVDesc.Format               = tex.Format;
		UAVDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
		UAVDesc.Texture2D.MipSlice   = 0;
		UAVDesc.Texture2D.PlaneSlice = 0;

		RS->pDevice->CreateUnorderedAccessView(tex, nullptr, &UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });
		
		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushUAV2DToDescHeap(RenderSystem* RS, Texture2D tex, uint32_t mipLevel, DescHeapPOS POS)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		UAVDesc.Format               = tex.Format;
		UAVDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
		UAVDesc.Texture2D.MipSlice   = mipLevel;
		UAVDesc.Texture2D.PlaneSlice = 0;

		RS->pDevice->CreateUnorderedAccessView(tex, nullptr, &UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });
		
		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	DescHeapPOS PushUAV3DToDescHeap(RenderSystem* RS, Texture2D tex, uint32_t width, DescHeapPOS POS)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		UAVDesc.Format                  = tex.Format;
		UAVDesc.ViewDimension           = D3D12_UAV_DIMENSION_TEXTURE3D;
		UAVDesc.Texture3D.MipSlice      = 0;
		UAVDesc.Texture3D.FirstWSlice   = 0;
		UAVDesc.Texture3D.WSize         = width;

		RS->pDevice->CreateUnorderedAccessView(tex, nullptr, &UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });
		
		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	inline DescHeapPOS PushUAV1DToDescHeap(RenderSystem* RS, ID3D12Resource* resource, DXGI_FORMAT format, uint mip, DescHeapPOS POS)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		UAVDesc.Format						= format;
		UAVDesc.ViewDimension				= D3D12_UAV_DIMENSION_TEXTURE1D;
		UAVDesc.Texture1D.MipSlice          = 0;

		RS->pDevice->CreateUnorderedAccessView(resource, nullptr, &UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	inline DescHeapPOS PushUAVBufferToDescHeap(RenderSystem* RS, UAVBuffer buffer, DescHeapPOS POS)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		UAVDesc.Format						= buffer.format;
		UAVDesc.ViewDimension				= D3D12_UAV_DIMENSION_BUFFER;
		UAVDesc.Buffer.CounterOffsetInBytes = buffer.counterOffset;
		UAVDesc.Buffer.FirstElement			= buffer.offset;
		UAVDesc.Buffer.Flags				= buffer.typeless == true ? D3D12_BUFFER_UAV_FLAGS::D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAGS::D3D12_BUFFER_UAV_FLAG_NONE;
		UAVDesc.Buffer.NumElements			= buffer.elementCount - buffer.offset; // space for counter at beginning
		UAVDesc.Buffer.StructureByteStride	= buffer.stride;

		FK_ASSERT(UAVDesc.Buffer.StructureByteStride < 512);
		FK_ASSERT(UAVDesc.Buffer.NumElements > 0);

		RS->pDevice->CreateUnorderedAccessView(buffer.resource, nullptr, &UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	inline DescHeapPOS PushUAVBufferToDescHeap2(RenderSystem* RS, UAVBuffer buffer, ID3D12Resource* counter, DescHeapPOS POS)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		UAVDesc.Format						= buffer.format;
		UAVDesc.ViewDimension				= D3D12_UAV_DIMENSION_BUFFER;
		UAVDesc.Buffer.CounterOffsetInBytes = buffer.counterOffset;
		UAVDesc.Buffer.FirstElement         = buffer.offset;
		UAVDesc.Buffer.Flags				= buffer.typeless == true ? D3D12_BUFFER_UAV_FLAGS::D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAGS::D3D12_BUFFER_UAV_FLAG_NONE;
		UAVDesc.Buffer.NumElements			= buffer.elementCount - buffer.offset;
		UAVDesc.Buffer.StructureByteStride	= buffer.stride;

		FK_ASSERT(UAVDesc.Buffer.StructureByteStride < 512);
		FK_ASSERT(UAVDesc.Buffer.NumElements > 0);

		RS->pDevice->CreateUnorderedAccessView(buffer.resource, counter, &UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	
	/************************************************************************************************/


	DescHeapPOS PushUAVCubeMapToDescHeap(RenderSystem* RS, DXGI_FORMAT format, ID3D12Resource* resource, DescHeapPOS POS)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		UAVDesc.Format							= format;
		UAVDesc.ViewDimension					= D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		UAVDesc.Texture2DArray.MipSlice			= 0;
		UAVDesc.Texture2DArray.FirstArraySlice	= 0;
		UAVDesc.Texture2DArray.ArraySize		= 6;
		UAVDesc.Texture2DArray.PlaneSlice		= 0;

		RS->pDevice->CreateUnorderedAccessView(resource, nullptr, &UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	float2 GetPixelSize(IRenderWindow& Window)
	{
		return float2{ 1.0f, 1.0f } / Window.GetWH();
	}


	/************************************************************************************************/

	// assumes File str should be at most 256 bytes
	ResourceHandle LoadDDSTextureFromFile(char* file, RenderSystem* RS, CopyContextHandle handle, iAllocator* MemoryOut)
	{
		Texture2D tex = {};
		wchar_t	wfile[256];
		size_t	ConvertedSize = 0;
		mbstowcs_s(&ConvertedSize, wfile, file, 256);
		auto [Texture, Sucess] = LoadDDSTexture2DFromFile_2(file, MemoryOut, RS, handle);

		FK_ASSERT(Sucess != false, "Failed to Create Texture!");


		return Texture;
	}


	/************************************************************************************************/


	ResourceHandle MoveTextureBufferToVRAM(RenderSystem* RS, CopyContextHandle handle, TextureBuffer* buffer, DeviceFormat format)
	{
		auto textureHandle = RS->CreateGPUResource(GPUResourceDesc::ShaderResource(buffer->WH, format));
		RS->UploadTexture(textureHandle, handle, buffer->Buffer, buffer->Size);
		RS->SetDebugName(textureHandle, "MoveTextureBufferToVRAM");

		return textureHandle;
	}


	/************************************************************************************************/


	ResourceHandle MoveTextureBuffersToVRAM(RenderSystem* RS, CopyContextHandle handle, TextureBuffer* buffer, size_t resourceCount, DeviceFormat format)
	{
		FK_ASSERT(resourceCount < std::numeric_limits<uint8_t>::max());

		auto texture_desc = GPUResourceDesc::ShaderResource(buffer[0].WH, format, (uint8_t)resourceCount);
		texture_desc.initialLayout = DeviceLayout::Common;

		auto textureHandle = RS->CreateGPUResource(texture_desc);
		RS->UploadTexture(textureHandle, handle, buffer, resourceCount);
		RS->SetDebugName(textureHandle, "MoveTextureBuffersToVRAM");

		return textureHandle;
	}


	/************************************************************************************************/


	ResourceHandle MoveTextureBuffersToVRAM(RenderSystem* RS, CopyContextHandle handle, TextureBuffer* buffer, size_t MIPCount, size_t arrayCount, DeviceFormat format)
	{
		FK_ASSERT(MIPCount < std::numeric_limits<uint8_t>::max());

		auto textureDesc			= GPUResourceDesc::ShaderResource(buffer[0].WH, format, (uint8_t)MIPCount, arrayCount);
		textureDesc.initialLayout	= DeviceLayout::Common;

		auto textureHandle = RS->CreateGPUResource(textureDesc);
		RS->UploadTexture(textureHandle, handle, buffer, arrayCount * MIPCount);
		RS->SetDebugName(textureHandle, "MoveTextureBuffersToVRAM");

		return textureHandle;
	}


	/************************************************************************************************/

	BlendState BlendState::Blend()
	{
		return
			BlendState{
				.alphaToCoverageEnable		= false,
				.independentBlendEnable		= false,
				.renderTarget = {
					RenderTargetStateDesc{
						.blendEnable	= true,
						.srcBlend		= EBlend::SRC_ALPHA,
						.dstBlend		= EBlend::INV_SRC_ALPHA,
						.blendOp		= EBlendOP::ADD,
						.srcBlendAlpha	= EBlend::ONE,
						.dstBlendAlpha	= EBlend::ONE
					}
				}};
	}

	/************************************************************************************************/


	MemoryPoolAllocator::MemoryPoolAllocator(RenderSystem& IN_renderSystem, size_t IN_heapSize, size_t IN_blockSize, uint32_t IN_flags, iAllocator* IN_allocator) :
		renderSystem	{ IN_renderSystem },
		blockCount		{ IN_heapSize / IN_blockSize },
		blockSize		{ IN_blockSize },
		allocator		{ IN_allocator },
		allocations		{ IN_allocator },
		freeRanges		{ IN_allocator },
		heap			{ IN_renderSystem.CreateHeap(IN_heapSize, IN_flags) },
		flags			{ IN_flags }
	{
		freeRanges.push_back({ 0, (uint32_t)blockCount, Clear });

		estimatedBlocksAvailable = blockCount;
	}


	/************************************************************************************************/


	MemoryPoolAllocator::~MemoryPoolAllocator()
	{
		renderSystem.ReleaseHeap(heap);
	}


	/************************************************************************************************/


	GPUHeapAllocation MemoryPoolAllocator::GetMemory(const size_t requestBlockCount, const uint64_t frameID, const uint64_t flags)
	{
		ProfileFunction();
		FK_ASSERT(requestBlockCount < std::numeric_limits<uint32_t>::max());

		std::scoped_lock localLock{ m };
		const auto completionCount = renderSystem.directFence->GetCompletedValue();

		std::sort(
			freeRanges.begin(), freeRanges.end(),
			[](auto& lhs, auto& rhs)
			{
				const auto t1 = uint64_t(lhs.flags & 0x5) << 59 | lhs.frameID << 24 | uint64_t(lhs.blockCount);
				const auto t2 = uint64_t(rhs.flags & 0x5) << 59 | rhs.frameID << 24 | uint64_t(rhs.blockCount);

				return t1 < t2;
			});

		auto _GetMemory = [&]() -> GPUHeapAllocation {
			for (auto& range : freeRanges)
			{
				if (range.offset > blockCount)
					__debugbreak();

				if (range.blockCount >= requestBlockCount)
				{
					if  (range.flags == Clear ||
						(range.flags == Locked && range.frameID <= completionCount && range.frameID < frameID))
						//|| (!(range.flags | AllowReallocation) && range.frameID == frameID))
					{
						GPUHeapAllocation heapAllocation = {
							.offset		= range.offset * blockSize,
							.size		= requestBlockCount * blockSize,
							.overlap	= (range.priorAllocation != InvalidHandle && (range.flags & AllowReallocation) && range.frameID == frameID) ? range.priorAllocation : InvalidHandle
						};

						FK_LOG_9("Allocated Blocks %u - %u from Heap %u during %u; Completed count %u; Last Used: %u", range.offset, range.offset + requestBlockCount, heap.INDEX, frameID, completionCount, range.frameID);

						if (range.blockCount > requestBlockCount)
						{
							range.blockCount	-= (uint32_t)(requestBlockCount);
							range.offset		+= (uint32_t)(requestBlockCount);
						}
						else if (range.blockCount == requestBlockCount) {
							freeRanges.remove_unstable(&range);
						}

						estimatedBlocksAvailable -= requestBlockCount;

						return heapAllocation;
					}
				}
			}

			//exit(-10);
			return {};
		};
		auto res = _GetMemory();
		if (res)
			return res;


		Coalesce();

		return _GetMemory();
	}


	void MemoryPoolAllocator::Coalesce()
	{
		const uint64_t completedID = renderSystem.directFence->GetCompletedValue();

		FK_LOG_9("Coalesce");

		std::sort(freeRanges.begin(), freeRanges.end());

		size_t I = 0; 
		while (I + 1 < freeRanges.size())
		{
			auto& range1 = freeRanges[I];
			auto& range2 = freeRanges[I + 1];

			if ((range1.offset + range1.blockCount) == range2.offset &&
				range1.frameID < completedID && range2.frameID < completedID)
			{
				range1.blockCount += range2.blockCount;
				range1.frameID = 0;
				range1.flags = Clear;

				freeRanges.remove_stable(&range2);
			}
			else
				I++;
		}
	}

	/************************************************************************************************/


	AcquireResult MemoryPoolAllocator::Acquire(GPUResourceDesc desc, bool temporary)
	{
		ProfileFunction();

		const uint64_t frameIdx	= renderSystem.directSubmissionCounter;
		const uint64_t size		= renderSystem.GetAllocationSize(desc);

		const size_t	requestedBlockCount	= Max((size / blockSize) + ((size % blockSize == 0) ? 0 : 1), 1);
		auto			allocation			= GetMemory(requestedBlockCount, frameIdx, Clear);

		FK_LOG_9("Allocating Block with size %u", requestedBlockCount);

		if (allocation.offset / blockSize > blockCount) {
			FK_LOG_ERROR("MemoryPoolAllocator Allocated a block beyond range!");
			return { InvalidHandle, InvalidHandle };
		}

		if (!allocation)
		{
			if (estimatedBlocksAvailable > requestedBlockCount)
				FK_LOG_INFO("High Fragmentation Detected in Pool %u.", heap.INDEX);

			FK_LOG_INFO("MemoryPoolAllocator Caused Stall!; Pool %u", heap.INDEX);
			renderSystem.WaitForGPU();

			Coalesce();
			allocation = GetMemory(requestedBlockCount, frameIdx, Clear);

			if (!allocation)
			{
				FK_LOG_ERROR("MemoryPoolAllocator Ran out of memory! Pool %u.", heap.INDEX);
				return { InvalidHandle, InvalidHandle };
			}
		}

		desc.bufferCount    = 1;
		desc.allocationType = ResourceAllocationType::Placed;
		desc.placed.heap    = heap;
		desc.placed.offset  = allocation.offset;


		ResourceHandle resource = renderSystem.CreateGPUResource(desc);

		if (resource != InvalidHandle) {
			renderSystem.SetDebugName(resource, "Acquire");

			std::scoped_lock localLock{ m };

			allocations.push_back({
				(uint32_t)(allocation.offset / blockSize),
				(uint32_t)(allocation.size / blockSize),
				frameIdx,
				(uint64_t)(temporary ? Temporary : Allocated),
				resource });
		}
		else
			__debugbreak();

		return { resource, allocation.overlap };
	}


	/************************************************************************************************/


	AcquireDeferredRes MemoryPoolAllocator::AcquireDeferred(GPUResourceDesc desc, bool temporary)
	{
		ProfileFunction();

		const uint64_t frameIdx	= renderSystem.directSubmissionCounter;
		const uint64_t size		= renderSystem.GetAllocationSize(desc);

		const size_t requestedBlockCount	= Max((size / blockSize) + ((size % blockSize == 0) ? 0 : 1), 1);
		auto allocation						= GetMemory(requestedBlockCount, frameIdx, Clear);

		FK_LOG_9("Allocating Block with size %u", requestedBlockCount);

		if (allocation.offset / blockSize > blockCount) {
			FK_LOG_ERROR("MemoryPoolAllocator Allocated a block beyond range!");
			return { InvalidHandle, InvalidHandle };
		}

		if (!allocation) {
			if(estimatedBlocksAvailable > requestedBlockCount)
				FK_LOG_INFO("High Fragmentation Detected in Pool %u.", heap.INDEX);

			FK_LOG_INFO("MemoryPoolAllocator Caused Stall!; Pool %u", heap.INDEX);
			renderSystem.WaitForGPU();

			Coalesce();

			allocation = GetMemory(requestedBlockCount, frameIdx, Clear);
			if (!allocation)
			{
				FK_LOG_ERROR("MemoryPoolAllocator Ran out of memory! Pool %u.", heap.INDEX);
				return { InvalidHandle, InvalidHandle };
			}
		}

		desc.bufferCount    = 1;
		desc.allocationType = ResourceAllocationType::Placed;
		desc.placed.heap    = heap;
		desc.placed.offset  = allocation.offset;


		ResourceHandle resource = renderSystem.CreateGPUResourceHandle();

		if (resource != InvalidHandle) {
			std::scoped_lock localLock{ m };

			allocations.push_back({
				(uint32_t)(allocation.offset / blockSize),
				(uint32_t)(allocation.size / blockSize),
				frameIdx,
				(uint64_t)(temporary ? Temporary : Allocated),
				resource });
		}
		else
			__debugbreak();

		return { resource, allocation.overlap, allocation.offset, heap };
	}


	/************************************************************************************************/


	AcquireResult MemoryPoolAllocator::Recycle(ResourceHandle resourceToRecycle, GPUResourceDesc desc)
	{
		ProfileFunction();

		std::scoped_lock localLock{ m };

		const uint64_t frameIdx				= renderSystem.directSubmissionCounter;
		const uint64_t size					= renderSystem.GetAllocationSize(desc);
		const size_t requestedBlockCount	= (size / blockSize) + (size % blockSize == 0) ? 0 : 1;

		FK_ASSERT(requestedBlockCount < std::numeric_limits<uint32_t>::max());

		if (auto res = std::find_if(std::begin(freeRanges), std::end(freeRanges),
			[&](auto r) { return r.priorAllocation == resourceToRecycle; }); res != std::end(freeRanges))
		{
			if (res->flags | AllowReallocation)
			{
				auto rangeDescriptor = *res;

				if (res->blockCount == requestedBlockCount)
				{
					freeRanges.remove_unstable(res);
				}
				else if(res->blockCount > requestedBlockCount)
				{
					res->offset     += (uint32_t)requestedBlockCount;
					res->blockCount -= (uint32_t)(requestedBlockCount + 1);
				}
				else if (res->blockCount < requestedBlockCount)
					return { InvalidHandle, InvalidHandle }; // ERROR!?

				GPUHeapAllocation heapAllocation = {
						rangeDescriptor.offset * blockSize,
						requestedBlockCount * blockSize,
						(rangeDescriptor.priorAllocation != InvalidHandle &&
							rangeDescriptor.flags | AllowReallocation &&
							rangeDescriptor.frameID == frameIdx) ?
						rangeDescriptor.priorAllocation : InvalidHandle
				};
				
				desc.bufferCount    = 1;
				desc.allocationType = ResourceAllocationType::Placed;
				desc.placed.heap    = heap;
				desc.placed.offset  = heapAllocation.offset;

				ResourceHandle resource = renderSystem.CreateGPUResource(desc);
				if (resource != InvalidHandle) {
					renderSystem.SetDebugName(resource, "Acquire");

					allocations.push_back({
						(uint32_t)(rangeDescriptor.offset / blockSize),
						(uint32_t)(requestedBlockCount),
						frameIdx,
						Temporary,
						resource });
				}
				else
					__debugbreak();

				return { resource, resourceToRecycle };

			}
		}

		return { InvalidHandle, InvalidHandle };
	}


	/************************************************************************************************/


	uint32_t MemoryPoolAllocator::Flags() const
	{
		return flags;
	}


	/************************************************************************************************/


	void MemoryPoolAllocator::Release(ResourceHandle handle, uint64_t submissionID, const bool freeResourceImmedate, const bool allowImmediateReuse)
	{
		std::scoped_lock localLock{ m };

		auto res = find(allocations,
			[&](auto& e)
			{
				return (e.resource == handle);
			}
		);

		if (res != std::end(allocations))
		{
#if DEBUG
			if (res->offset > blockCount || res->offset + res->blockCount > blockCount)
				__debugbreak();

			FK_LOG_9("Releasing Blocks %u - %u from Heap %u during %u", res->offset, res->offset + res->blockCount, heap.INDEX, submissionID);
#endif

			estimatedBlocksAvailable += res->blockCount;

			freeRanges.push_back(
				{
					res->offset,
					res->blockCount,
					Locked,
					Max(submissionID, res->frameID),
					handle
				});

			allocations.remove_unstable(res);
		}
	}


	/************************************************************************************************/


	void MemoryPoolAllocator::LockRange(uint64_t begin, uint64_t end)
	{
		for (auto& range : freeRanges)
		{
			if (range.frameID >= begin)
				range.frameID = end;
		}
	}


	/************************************************************************************************/
}//	Namespace FlexKit


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
