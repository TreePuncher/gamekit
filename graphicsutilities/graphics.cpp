#include "..\pch.h"
#include "buildsettings.h"
#include "memoryutilities.h"
#include "containers.h"
#include "intersection.h"
#include "MeshUtils.h"
#include "DDSUtilities.h"
#include "ThreadUtilities.h"

#include "AnimationUtilities.h"
#include "graphics.h"

#include <algorithm>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <iostream>
#include <Windows.h>
#include <windowsx.h>

extern "C" __declspec(dllexport) DWORD  NvOptimusEnablement = 1;
extern "C" __declspec(dllexport) int    AmdPowerXpressRequestHighPerformance = 1;

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion    = 602; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath      = ".\\D3D12\\"; }

namespace FlexKit
{
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
		auto uavLayout	= rs.GetUAVBufferLayout(handle);
        auto bufferSize = rs.GetUAVBufferSize(handle);

		resource		= rs.GetDeviceResource(handle);
		stride			= IN_stride == -1 ? uavLayout.stride : IN_stride;
		elementCount	= bufferSize / stride;
		counterOffset	= 0;
		offset			= IN_offset;
		format			= uavLayout.format;
	}



	/************************************************************************************************/


	ConstantBufferHandle	ConstantBufferTable::CreateConstantBuffer(size_t BufferSize, bool GPUResident)
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

		D3D12_HEAP_PROPERTIES HEAP_Props ={};
		HEAP_Props.CPUPageProperty	     = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type				     = GPUResident ? D3D12_HEAP_TYPE_DEFAULT : D3D12_HEAP_TYPE_UPLOAD;
		HEAP_Props.MemoryPoolPreference  = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask	     = 0;
		HEAP_Props.VisibleNodeMask		 = 0;

		constexpr size_t bufferCount = 3;
		ID3D12Resource*  resources[3];

		D3D12_RESOURCE_STATES InitialState = GPUResident ? 
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON :
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ;

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


		handles[handle] = buffers.push_back(buffer);

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

		UpdateCurrentBuffer(Handle);

		const uint32_t size		   = buffer.size;
		const uint32_t offset	   = buffer.offset;

		const size_t alignOffset	= 256 - offset % 256;
		const size_t adjustedOffset = (alignOffset == 256) ? 0 : offset;
		const size_t alignedOffset  = offset + adjustedOffset;

		buffer.offset       = alignedOffset;

		return alignedOffset;
	}


	/************************************************************************************************/


	std::optional<size_t> ConstantBufferTable::Push(ConstantBufferHandle Handle, void* _Ptr, size_t pushSize)
	{
		auto& buffer = buffers[handles[Handle]];

		if (buffer.GPUResident)
			return false; // Cannot directly push to GPU Resident Memory

		const char* Debug_mapped_Ptr = (char*)buffer.mapped_ptr;


		UpdateCurrentBuffer(Handle);

		const uint32_t size     = buffer.size;
		const uint32_t offset   = buffer.offset;
		const char*  mapped_Ptr	= (char*)buffer.mapped_ptr;

		if (!mapped_Ptr)
			return {};

		if (size < offset + pushSize)
			return {}; // Buffer To small to accommodate Push

		buffer.offset += pushSize;

		if(!buffer.writeFlag)
			buffer.writeFlag = true;

		FK_ASSERT(pushSize % 256 == 0); // size requests must be blocks of 256

		if(_Ptr)
			memcpy((void*)(mapped_Ptr + offset), _Ptr, pushSize);

		return { offset };
	}


	/************************************************************************************************/


	ConstantBufferTable::SubAllocation ConstantBufferTable::Reserve(ConstantBufferHandle CB, size_t reserveSize)
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


	void ConstantBufferTable::LockFor(uint8_t frameCount)
	{
		for (auto& buffer : buffers) 
			buffer.locks[buffer.currentRes] = frameCount;
	}


	/************************************************************************************************/


	void ConstantBufferTable::DecrementLocks()
	{
		for (auto& buffer : buffers)
			for (auto& lock : buffer.locks)
				if(lock) lock--;
	}


	/************************************************************************************************/


	void ConstantBufferTable::ReleaseBuffer(ConstantBufferHandle handle)
	{
		std::scoped_lock lock(criticalSection);

		const size_t UserIdx    = handles[handle];
		auto&  buffer           = buffers[UserIdx];

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


	void ConstantBufferTable::UpdateCurrentBuffer(ConstantBufferHandle Handle)
	{
		const size_t UserIdx	= handles[Handle];
		auto& buffer            = buffers[UserIdx];

		// Map Next Buffer
		if (!buffer.GPUResident)
		{
			char*  mapped_Ptr = nullptr;

			if(buffer.locks[buffer.currentRes])
			{
				D3D12_RANGE range{ 0, buffer.offset };
				buffer.resources[buffer.currentRes]->Unmap(0, &range);

				buffer.currentRes = (buffer.currentRes + 1) % 3;
				FK_ASSERT(buffer.locks[buffer.currentRes] == 0, "Constant buffer lock error! possibly all buffers locked!");

				auto HR = buffer.resources[buffer.currentRes]->Map(0, nullptr, (void**)&mapped_Ptr);
				FK_ASSERT(FAILED(HR), "Failed to map Constant Buffer");
				buffer.mapped_ptr   = mapped_Ptr;

				buffer.offset = 0;
			}
		}

	}


	/************************************************************************************************/


	DescriptorHeap::DescriptorHeap(Context& ctx, const DesciptorHeapLayout<16>& Layout_IN, iAllocator* TempMemory) :
		FillState(TempMemory)
	{
		FK_ASSERT(TempMemory);

		const size_t EntryCount = Layout_IN.size();
		descriptorHeap	= ctx._ReserveSRV(EntryCount);
		Layout			= &Layout_IN;

		for (size_t I = 0; I < EntryCount; I++)
			FillState.push_back(false);
	}


	/************************************************************************************************/


	DescriptorHeap::DescriptorHeap(DescriptorHeap&& rhs)
	{
		descriptorHeap  = rhs.descriptorHeap;
		FillState       = std::move(rhs.FillState);
		Layout          = rhs.Layout;

		rhs.descriptorHeap = DescHeapPOS{ 0u, 0u };
		rhs.Layout = nullptr;
	}


	/************************************************************************************************/


	DescriptorHeap& DescriptorHeap::operator = (DescriptorHeap&& rhs)
	{
		descriptorHeap      = rhs.descriptorHeap;
		FillState           = std::move(rhs.FillState);
		Layout              = rhs.Layout;

		rhs.descriptorHeap = DescHeapPOS{ 0u, 0u };
		rhs.Layout = nullptr;

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeap& DescriptorHeap::Init(Context& ctx, const DesciptorHeapLayout<16>& Layout_IN, iAllocator* TempMemory)
	{
		FK_ASSERT(TempMemory);
		FillState = Vector<bool>(TempMemory);

		const size_t EntryCount = Layout_IN.size();
		descriptorHeap = ctx._ReserveSRV(EntryCount);
		Layout = &Layout_IN;

		for (size_t I = 0; I < EntryCount; I++)
			FillState.push_back(false);

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeap& DescriptorHeap::Init(Context& ctx, const DesciptorHeapLayout<16>& Layout_IN, const size_t reserveCount, iAllocator* TempMemory)
	{
		FK_ASSERT(TempMemory);
		FillState = Vector<bool>(TempMemory);

		const size_t EntryCount = Layout_IN.size() * reserveCount;
		descriptorHeap = ctx._ReserveSRV(EntryCount);
		Layout = &Layout_IN;

		for (size_t I = 0; I < EntryCount; I++)
			FillState.push_back(false);

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeap& DescriptorHeap::Init2(Context& ctx, const DesciptorHeapLayout<16>& Layout_IN, const size_t reserveCount, iAllocator* TempMemory)
	{
		FK_ASSERT(TempMemory);
		FillState = Vector<bool>(TempMemory, reserveCount);

		descriptorHeap = ctx._ReserveSRV(reserveCount);
		Layout = &Layout_IN;

		for (size_t I = 0; I < reserveCount; I++)
			FillState.push_back(false);

		return *this;
	}



	/************************************************************************************************/


	DescriptorHeap& DescriptorHeap::NullFill(Context& ctx, const size_t end)
	{
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


	bool DescriptorHeap::SetSRV(Context& ctx, size_t idx, ResourceHandle handle)
	{
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx))
			return false;


		FillState[idx] = true;

		PushTextureToDescHeap(
			ctx.renderSystem,
			ctx.renderSystem->GetTextureDeviceFormat(handle),
			handle,
			IncrementHeapPOS(
					descriptorHeap, 
					ctx.renderSystem->DescriptorCBVSRVUAVSize,
					idx));

		return true;
	}


	/************************************************************************************************/


	bool DescriptorHeap::SetSRVCubemap(Context& ctx, size_t idx, ResourceHandle	handle)
	{
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx))
			return false;

		FillState[idx] = true;

		PushCubeMapTextureToDescHeap(
			ctx.renderSystem,
			handle,
			IncrementHeapPOS(
					descriptorHeap, 
					ctx.renderSystem->DescriptorCBVSRVUAVSize, 
					idx),
			ctx.renderSystem->GetTextureFormat(handle));

		return true;
	}


	/************************************************************************************************/


	bool DescriptorHeap::SetSRVCubemap(Context& ctx, size_t idx, ResourceHandle	handle, DeviceFormat format)
	{
		if (handle == InvalidHandle || !CheckType(*Layout, DescHeapEntryType::ShaderResource, idx))
			return false;

		FillState[idx] = true;

		PushCubeMapTextureToDescHeap(
			ctx.renderSystem,
			handle,
			IncrementHeapPOS(
					descriptorHeap, 
					ctx.renderSystem->DescriptorCBVSRVUAVSize, 
					idx),
			format);

		return true;
	}



	/************************************************************************************************/


	bool DescriptorHeap::SetSRV(Context& ctx, size_t idx, ResourceHandle handle, DeviceFormat format)
	{
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx))
			return false;

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

		return true;
	}


    /************************************************************************************************/


    bool DescriptorHeap::SetSRV(Context& ctx, size_t idx, ResourceHandle handle, uint MipOffset, DeviceFormat format)
    {
        if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx))
			return false;

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

		return true;
    }


    /************************************************************************************************/


    bool DescriptorHeap::SetSRV3D(Context& ctx, size_t idx, ResourceHandle resource)
    {
        if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx))
			return false;

		FillState[idx] = true;

        const uint32_t  mipCount    = ctx.renderSystem->GetTextureMipCount(resource);
        const auto      format      = ctx.renderSystem->GetTextureFormat(resource);
		const auto      dxFormat    = TextureFormat2DXGIFormat(format);

        PushTexture3DToDescHeap(
            ctx.renderSystem,
            dxFormat,
            mipCount,
            0,
            0,
            resource,
            IncrementHeapPOS(
                descriptorHeap,
                ctx.renderSystem->DescriptorCBVSRVUAVSize,
                idx));

		return true;
    }


	/************************************************************************************************/


    bool DescriptorHeap::SetCBV(Context& ctx, size_t idx, const ConstantBufferDataSet& constants)
    {
        if (!CheckType(*Layout, DescHeapEntryType::ConstantBuffer, idx))
			return false;

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

		return true;
    }


    /************************************************************************************************/


	bool DescriptorHeap::SetCBV(Context& ctx, size_t idx, ConstantBufferHandle	Handle, size_t offset, size_t bufferSize)
	{
		if (!CheckType(*Layout, DescHeapEntryType::ConstantBuffer, idx))
			return false;

		FillState[idx] = true;

		auto resource = ctx.renderSystem->GetDeviceResource(Handle);
		PushCBToDescHeap(
			ctx.renderSystem,
			resource,
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx),
			(bufferSize / 256) * 256 + 256,
			offset);

		return true;
	}


    /************************************************************************************************/


    bool DescriptorHeap::SetCBV(Context& ctx, size_t idx, ResourceHandle	Handle, size_t offset, size_t bufferSize)
    {
        if (!CheckType(*Layout, DescHeapEntryType::ConstantBuffer, idx))
			return false;

		FillState[idx] = true;

		auto resource = ctx.renderSystem->GetDeviceResource(Handle);
		PushCBToDescHeap(
			ctx.renderSystem,
			resource,
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx),
			(bufferSize / 256) * 256 + 256,
			offset);

		return true;
    }


	/************************************************************************************************/


	bool DescriptorHeap::SetUAVBuffer(Context& ctx, size_t idx, ResourceHandle handle, size_t offset)
	{
		if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx))
			return false;

		FillState[idx] = true;

		UAVBuffer UAV{ *ctx.renderSystem, handle };
		UAV.offset = offset;

		PushUAVBufferToDescHeap(
			ctx.renderSystem,
			UAV,
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx));

		return true;
	}
    

	/************************************************************************************************/


	bool DescriptorHeap::SetUAVTexture(Context& ctx, size_t idx, ResourceHandle handle)
	{
		if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx))
			return false;

		FillState[idx] = true;

        Texture2D tex;
        tex.WH          = ctx.renderSystem->GetTextureWH(handle);
        tex.Texture     = ctx.renderSystem->GetDeviceResource(handle);
        tex.Format      = ctx.renderSystem->GetTextureDeviceFormat(handle);

		PushUAV2DToDescHeap(
			ctx.renderSystem,
			tex, 
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx));

		return true;
	}


    /************************************************************************************************/


    bool DescriptorHeap::SetUAVTexture(Context& ctx, size_t idx, ResourceHandle handle, DeviceFormat format)
    {
        if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx))
			return false;

		FillState[idx] = true;

        Texture2D tex;
        tex.WH          = ctx.renderSystem->GetTextureWH(handle);
        tex.Texture     = ctx.renderSystem->GetDeviceResource(handle);
        tex.Format      = TextureFormat2DXGIFormat(format);

		PushUAV2DToDescHeap(
			ctx.renderSystem,
			tex, 
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx));

        return true;
    }


    /************************************************************************************************/


    bool DescriptorHeap::SetUAVTexture(Context& ctx, size_t idx, size_t mipLevel, ResourceHandle handle, DeviceFormat format)
    {
        if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx))
			return false;

		FillState[idx] = true;

        Texture2D tex;
        tex.WH          = ctx.renderSystem->GetTextureWH(handle);
        tex.Texture     = ctx.renderSystem->GetDeviceResource(handle);
        tex.Format      = TextureFormat2DXGIFormat(format);

        PushUAV2DToDescHeap(
			ctx.renderSystem,
			tex,
            mipLevel,
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx));

        return true;
    }


    /************************************************************************************************/


    bool DescriptorHeap::SetUAVTexture3D(Context& ctx, size_t idx, ResourceHandle handle, DeviceFormat format)
    {
        if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx))
			return false;

		FillState[idx] = true;

        Texture2D tex;
        tex.WH          = ctx.renderSystem->GetTextureWH(handle);
        tex.Texture     = ctx.renderSystem->GetDeviceResource(handle);
        tex.Format      = TextureFormat2DXGIFormat(format);

        PushUAV3DToDescHeap(
			ctx.renderSystem,
			tex,
            tex.WH[0],
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx));

        return true;
    }


    /************************************************************************************************/


    bool DescriptorHeap::SetUAVStructured(Context& ctx, size_t idx, ResourceHandle handle, size_t stride, size_t offset)
    {
        if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx))
			return false;

		FillState[idx] = true;

        UAVBuffer uavDesc{ *ctx.renderSystem, handle, stride, offset };
        
        PushUAVBufferToDescHeap(
            ctx.renderSystem,
            uavDesc,
            IncrementHeapPOS(
                descriptorHeap,
                ctx.renderSystem->DescriptorCBVSRVUAVSize,
                idx));

		return true;
    }


    
    /************************************************************************************************/


    bool DescriptorHeap::SetUAVStructured(
        Context&            ctx,
        size_t              idx,
        ResourceHandle	    resource,
        ResourceHandle      counter,
        size_t              stride,
        size_t              counterOffset)
    {
        if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx))
            return false;

        FillState[idx] = true;

        UAVBuffer uavDesc{ *ctx.renderSystem, resource, stride, 0 };
        uavDesc.counterOffset   = counterOffset;
        uavDesc.offset          = resource == counter ? Max(4096 / stride, 1) : 0;

        PushUAVBufferToDescHeap2(
            ctx.renderSystem,
            uavDesc,
            ctx.renderSystem->GetDeviceResource(counter),
            IncrementHeapPOS(
                descriptorHeap,
                ctx.renderSystem->DescriptorCBVSRVUAVSize,
                idx));

        return true;
    }


	/************************************************************************************************/


	bool DescriptorHeap::SetStructuredResource(Context& ctx, size_t idx, ResourceHandle handle, size_t stride, size_t offset)
	{
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx))
			return false;

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

		return true;
	}


	/************************************************************************************************/


    /*
	bool DescriptorHeap::SetStructuredResource(Context& ctx, size_t idx, ResourceHandle handle, size_t stride, size_t offset)
	{
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx))
			return false;

		FillState[idx] = true;

		const auto size = ctx.renderSystem->GetUAVBufferSize(handle);
		
		PushSRVToDescHeap(
			ctx.renderSystem,
			ctx.renderSystem->GetDeviceResource(handle),
			IncrementHeapPOS(descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx),
			size / stride,
			stride,
            D3D12_BUFFER_SRV_FLAG_NONE,
            offset);

		return true;
	}
    */

	/************************************************************************************************/


	DescriptorHeap	DescriptorHeap::GetHeapOffsetted(size_t offset, Context& ctx) const
	{
		DescriptorHeap subHeap = Clone();
		subHeap.descriptorHeap = IncrementHeapPOS(
										descriptorHeap,
										ctx.renderSystem->DescriptorCBVSRVUAVSize,
										offset);

		return subHeap;
	}


	bool DescriptorHeap::CheckType(const DesciptorHeapLayout<>& layout, DescHeapEntryType type, size_t idx)
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


	bool RootSignature::Build(RenderSystem* RS, iAllocator* TempMemory)
	{
		if (Signature)
			Release();

		Vector<Vector<CD3DX12_DESCRIPTOR_RANGE>> DesciptorHeaps(TempMemory);
		DesciptorHeaps.reserve(12);

		static_vector<CD3DX12_ROOT_PARAMETER> Parameters;

		for (const auto& I : RootEntries)
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
				const auto  HeapIdx = I.DescriptorHeap.HeapIdx;
				const auto& HeapEntry = Heaps[HeapIdx];

				DesciptorHeaps.push_back(Vector<CD3DX12_DESCRIPTOR_RANGE>(TempMemory));

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

				const auto temp = DesciptorHeaps.back().size();
				Param.InitAsDescriptorTable(
					DesciptorHeaps.back().size(), 
					DesciptorHeaps.back().begin(), 
					PipelineDest2ShaderVis(I.DescriptorHeap.Accessibility));
			}	break;
			case RootSignatureEntryType::ConstantBuffer:
			{
				Param.InitAsConstantBufferView
				(	I.ConstantBuffer.Register, 
					I.ConstantBuffer.RegisterSpace, 
					PipelineDest2ShaderVis(I.ConstantBuffer.Accessibility));

			}	break;
			case RootSignatureEntryType::StructuredBuffer:
			{
				Param.InitAsShaderResourceView(
					I.ShaderResource.Register,
					I.ShaderResource.RegisterSpace,
					PipelineDest2ShaderVis(I.ConstantBuffer.Accessibility));
			}	break;
			case RootSignatureEntryType::UnorderedAcess:
			{
				Param.InitAsUnorderedAccessView(
					I.ShaderResource.Register,
					I.ShaderResource.RegisterSpace,
					PipelineDest2ShaderVis(I.ConstantBuffer.Accessibility));
			}   break;
			default:
				return false;
				FK_ASSERT(false);
			}
			Parameters.push_back(Param);
		}

		ID3DBlob* SignatureBlob = nullptr;
		ID3DBlob* ErrorBlob = nullptr;
		CD3DX12_STATIC_SAMPLER_DESC Default(0);
		CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;

		CD3DX12_STATIC_SAMPLER_DESC	 Samplers[] = {
			CD3DX12_STATIC_SAMPLER_DESC{0, D3D12_FILTER_ANISOTROPIC,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP},

			CD3DX12_STATIC_SAMPLER_DESC{1, D3D12_FILTER::D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR },
            CD3DX12_STATIC_SAMPLER_DESC{2,  D3D12_FILTER::D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
                                            D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                            D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                            D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                            0, 1, D3D12_COMPARISON_FUNC_LESS_EQUAL,
                                            D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK }
		};

		RootSignatureDesc.Init(Parameters.size(), Parameters.begin(), 1, &Default);
		RootSignatureDesc.pStaticSamplers	= Samplers;
		RootSignatureDesc.NumStaticSamplers	= 3;

		RootSignatureDesc.Flags |= AllowIA ? 
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT : 
			D3D12_ROOT_SIGNATURE_FLAG_NONE;
		
		RootSignatureDesc.Flags |= AllowSO ? 
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT :
			D3D12_ROOT_SIGNATURE_FLAG_NONE;


		HRESULT HR = D3D12SerializeRootSignature(
			&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, 
			&SignatureBlob,		&ErrorBlob);
		
		if (!SUCCEEDED(HR))
		{
			std::cout << (char*)ErrorBlob->GetBufferPointer() << '\n';
			ErrorBlob->Release();

#ifdef _DEBUG 
			FK_ASSERT(false, "Invalid Root Signature Description!");
#endif

			return false;
		}

		HR = RS->pDevice->CreateRootSignature(0, 
			SignatureBlob->GetBufferPointer(),
			SignatureBlob->GetBufferSize(), IID_PPV_ARGS(&Signature));

		FK_ASSERT(SUCCEEDED(HR));
		SETDEBUGNAME(Signature, "ShadingRTSig");
		SignatureBlob->Release(); 

		DesciptorHeaps.clear();

        Signature->AddRef();

		return true;
	}


	/************************************************************************************************/


	Context::Context(
				RenderSystem*	renderSystem_IN,
				iAllocator*		allocator) :
			CurrentRootSignature	{ nullptr			},
			PendingBarriers			{ },
			renderSystem			{ renderSystem_IN	},
			Memory					{ allocator			},
			RenderTargetCount		{ 0					},
			DepthStencilEnabled		{ false				},
			TrackedSOBuffers		{ }
	{
		HRESULT HR;
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapdesc;
		descriptorHeapdesc.Flags            = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		descriptorHeapdesc.NumDescriptors   = 1024 * 128;
		descriptorHeapdesc.NodeMask         = 0;
		descriptorHeapdesc.Type             = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		HR = renderSystem->pDevice->CreateDescriptorHeap(&descriptorHeapdesc, IID_PPV_ARGS(&descHeapSRV));
		FK_ASSERT(HR, "FAILED TO CREATE DESCRIPTOR HEAP");

		D3D12_DESCRIPTOR_HEAP_DESC cpuDescriptorHeapdesc;
		cpuDescriptorHeapdesc.Flags            = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		cpuDescriptorHeapdesc.NumDescriptors   = 1024;
		cpuDescriptorHeapdesc.NodeMask         = 0;
		cpuDescriptorHeapdesc.Type             = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		HR = renderSystem->pDevice->CreateDescriptorHeap(&cpuDescriptorHeapdesc, IID_PPV_ARGS(&descHeapSRVLocal));
		FK_ASSERT(HR, "FAILED TO CREATE DESCRIPTOR HEAP");

		descriptorHeapdesc.Flags             = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		descriptorHeapdesc.NumDescriptors    = 128;
		descriptorHeapdesc.NodeMask          = 0;
		descriptorHeapdesc.Type              = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		HR = renderSystem->pDevice->CreateDescriptorHeap(&descriptorHeapdesc, IID_PPV_ARGS(&descHeapRTV));
		FK_ASSERT(HR, "FAILED TO CREATE DESCRIPTOR HEAP");

		descriptorHeapdesc.Flags             = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		descriptorHeapdesc.NumDescriptors    = 128;
		descriptorHeapdesc.NodeMask          = 0;
		descriptorHeapdesc.Type              = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		HR = renderSystem->pDevice->CreateDescriptorHeap(&descriptorHeapdesc, IID_PPV_ARGS(&descHeapDSV));
		FK_ASSERT(HR, "FAILED TO CREATE DESCRIPTOR HEAP");

		FK_LOG_9("GRAPHICS SRV DESCRIPTOR HEAP CREATED: %u", descHeapSRV);
		FK_LOG_9("GRAPHICS RTV DESCRIPTOR HEAP CREATED: %u", descHeapRTV);
		FK_LOG_9("GRAPHICS DSV DESCRIPTOR HEAP CREATED: %u", descHeapDSV);

		SETDEBUGNAME(descHeapSRV, "RESOURCEHEAP");
		SETDEBUGNAME(descHeapRTV, "GPURESOURCEHEAP");
		SETDEBUGNAME(descHeapDSV, "RENDERTARGETHEAP");

		HR = renderSystem->pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)); FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND ALLOCATOR!");
		HR = renderSystem->pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, __uuidof(ID3D12CommandList), (void**)&DeviceContext);	FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND LIST!");


        SETDEBUGNAME(DeviceContext, "GraphicsContext");
        SETDEBUGNAME(DeviceContext, "GraphicsContextAllocator");

#if USING(AFTERMATH)
		auto res = GFSDK_Aftermath_DX12_CreateContextHandle(DeviceContext, &AFTERMATH_context);
#endif

		DeviceContext->Close();
	}


	void Context::AddRenderTargetBarrier(ResourceHandle Handle, DeviceResourceState Before, DeviceResourceState New)
	{
		Barrier NewBarrier;
		NewBarrier.OldState				= Before;
		NewBarrier.NewState				= New;
		NewBarrier.Type					= Barrier::Type::Resource;
		NewBarrier.resourceHandle   	= Handle;

		PendingBarriers.push_back(NewBarrier);
	}


	/************************************************************************************************/

    Context::Context(Context&& RHS)
	{
		DeviceContext			= RHS.DeviceContext;
		CurrentRootSignature	= RHS.CurrentRootSignature;
		CurrentPipelineState	= RHS.CurrentPipelineState;
		renderSystem			= RHS.renderSystem;

		RTV_CPU = RHS.RTV_CPU;

		SRV_CPU = RHS.SRV_CPU;
		SRV_GPU = RHS.SRV_GPU;

		DSV_CPU = RHS.DSV_CPU;

		descHeapRTV = RHS.descHeapRTV;
		descHeapSRV = RHS.descHeapSRV;
		descHeapDSV = RHS.descHeapDSV;

		RenderTargetCount		= RHS.RenderTargetCount;
		DepthStencilEnabled		= RHS.DepthStencilEnabled;

		Viewports				= RHS.Viewports;
		DesciptorHeaps			= RHS.DesciptorHeaps;
		VBViews					= RHS.VBViews;
		PendingBarriers			= RHS.PendingBarriers;
		Memory					= RHS.Memory;
		commandAllocator        = RHS.commandAllocator;


		// Null out old Context
		RHS.commandAllocator        = nullptr;
		RHS.DeviceContext			= nullptr;
		RHS.CurrentRootSignature	= nullptr;

		RHS.RenderTargetCount	    = 0;
		RHS.DepthStencilEnabled     = false;
		RHS.Memory				    = nullptr;

		RHS.Viewports.clear();
		RHS.DesciptorHeaps.clear();
		RHS.VBViews.clear();
		RHS.PendingBarriers.clear();

		RHS.RTV_CPU = { 0 };

		RHS.SRV_CPU = { 0 };
		RHS.SRV_GPU = { 0 };

		RHS.DSV_CPU = { 0 };

		RHS.descHeapRTV = nullptr;
		RHS.descHeapSRV = nullptr;
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

		RTV_CPU = RHS.RTV_CPU;

		SRV_CPU = RHS.SRV_CPU;
		SRV_GPU = RHS.SRV_GPU;

		DSV_CPU = RHS.DSV_CPU;

		descHeapRTV = RHS.descHeapRTV;
		descHeapSRV = RHS.descHeapSRV;
		descHeapDSV = RHS.descHeapDSV;

		RenderTargetCount		= RHS.RenderTargetCount;
		DepthStencilEnabled		= RHS.DepthStencilEnabled;

		Viewports				= RHS.Viewports;
		DesciptorHeaps			= RHS.DesciptorHeaps;
		VBViews					= RHS.VBViews;
		PendingBarriers			= RHS.PendingBarriers;
		Memory					= RHS.Memory;
		commandAllocator        = RHS.commandAllocator;

		// Null out old Context
		RHS.commandAllocator        = nullptr;
		RHS.DeviceContext			= nullptr;
		RHS.CurrentRootSignature	= nullptr;

		RHS.RenderTargetCount	    = 0;
		RHS.DepthStencilEnabled     = false;
		RHS.Memory				    = nullptr;

		RHS.Viewports.clear();
		RHS.DesciptorHeaps.clear();
		RHS.VBViews.clear();
		RHS.PendingBarriers.clear();

		RHS.RTV_CPU = { 0 };

		RHS.SRV_CPU = { 0 };
		RHS.SRV_GPU = { 0 };

		RHS.DSV_CPU = { 0 };

		RHS.descHeapRTV = nullptr;
		RHS.descHeapSRV = nullptr;
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

		if (descHeapSRV)
			descHeapSRV->Release();

		if (descHeapSRVLocal)
			descHeapSRVLocal->Release();

		if (descHeapDSV)
			descHeapDSV->Release();

		if(commandAllocator)
			commandAllocator->Release();

		if(DeviceContext)
			DeviceContext->Release();


		descHeapDSV          = nullptr;
		descHeapRTV          = nullptr;
		descHeapSRV          = nullptr;
		descHeapSRVLocal     = nullptr;
		commandAllocator     = nullptr;
		DeviceContext        = nullptr;
        CurrentPipelineState = nullptr;
	}


	/************************************************************************************************/


    void Context::CreateAS(const AccelerationStructureDesc& asDesc, const TriMesh&)
    {
        FK_ASSERT(0);
    }


    void Context::BuildBLAS(VertexBuffer& vertexBuffer, ResourceHandle destination, ResourceHandle scratchSpace)
    {
        auto indexBuffer    = vertexBuffer.VertexBuffers[vertexBuffer.MD.IndexBuffer_Index];
        auto positionBuffer = vertexBuffer.Find(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION);

        D3D12_RAYTRACING_GEOMETRY_DESC desc;
        desc.Type   = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        desc.Flags  = D3D12_RAYTRACING_GEOMETRY_FLAGS::D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

        desc.Triangles.Transform3x4 = 0;
        desc.Triangles.IndexFormat  = DXGI_FORMAT_R32_UINT;
        desc.Triangles.IndexBuffer  = indexBuffer.GetDevicePointer();
        desc.Triangles.IndexCount   = indexBuffer.Size();

        desc.Triangles.VertexFormat                 = DXGI_FORMAT_R32G32B32_FLOAT;
        desc.Triangles.VertexBuffer.StartAddress    = positionBuffer->GetDevicePointer();
        desc.Triangles.VertexBuffer.StrideInBytes   = positionBuffer->BufferStride;
        desc.Triangles.VertexCount                  = positionBuffer->Size();

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
	}


    /************************************************************************************************/


    void Context::AddUAVBarrier(ResourceHandle handle, uint32_t subresource)
    {
        auto res = find(PendingBarriers,
            [&](Barrier& rhs) -> bool
            {
                return
                    rhs.Type == Barrier::Type::UAV &&
                    rhs.resourceHandle == handle;
            });

        if (std::end(PendingBarriers) == res)
        {
            Barrier barrier;
            barrier.Type            = Barrier::Type::UAV;
            barrier.resourceHandle  = handle;
            barrier.subResource     = subresource;

            PendingBarriers.push_back(barrier);
        }
    }


	/************************************************************************************************/


	void Context::AddPresentBarrier(ResourceHandle Handle, DeviceResourceState Before)
	{
		Barrier NewBarrier;
		NewBarrier.OldState		    = Before;
		NewBarrier.NewState		    = DeviceResourceState::DRS_Present;
		NewBarrier.Type			    = Barrier::Type::Resource;
		NewBarrier.resourceHandle	= Handle;

		PendingBarriers.push_back(NewBarrier);
	}


	/************************************************************************************************/


	void Context::AddStreamOutBarrier(SOResourceHandle streamOut, DeviceResourceState Before, DeviceResourceState State)
	{
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
	}


	/************************************************************************************************/


	void Context::AddResourceBarrier(ResourceHandle resource, DeviceResourceState Before, DeviceResourceState State, uint32_t subResource)
	{
		auto res = find(PendingBarriers, 
			[&](Barrier& rhs) -> bool
			{
                switch (rhs.Type)
                {
                case Barrier::Type::Generic:
                    return rhs.resource_ptr == renderSystem->GetDeviceResource(resource);
                case Barrier::Type::Resource:
                    return rhs.resourceHandle == resource;
                default:
                    return false;
                }
			});

		if (res != PendingBarriers.end()) {
            if (res->OldState == State)
                PendingBarriers.remove_unstable(res);
            else
                res->NewState = State;
		}
		else
		{
			Barrier NewBarrier;
			NewBarrier.OldState			= Before;
			NewBarrier.NewState			= State;
			NewBarrier.Type				= Barrier::Type::Resource;
			NewBarrier.resourceHandle	= resource;

			PendingBarriers.push_back(NewBarrier);
		}
	}


	/************************************************************************************************/


	void Context::AddCopyResourceBarrier(ResourceHandle resource, DeviceResourceState Before, DeviceResourceState State)
	{
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
	}


	/************************************************************************************************/


	void Context::SetRootSignature(const RootSignature& RS)
	{
        CurrentRootSignature = &RS;
		DeviceContext->SetGraphicsRootSignature(RS);
	}


	/************************************************************************************************/


	void Context::SetComputeRootSignature(const RootSignature& RS)
	{
        CurrentComputeRootSignature = &RS;
		DeviceContext->SetComputeRootSignature(RS);
	}


	/************************************************************************************************/


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


	/************************************************************************************************/


	void Context::SetRenderTargets(const static_vector<ResourceHandle> RTs, bool DepthStecil, ResourceHandle depthStencil, const size_t MIPMapOffset)
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
					RTV_CPU_HANDLES.push_back(res->descriptor);
				}
				else
				{
					auto view = _ReserveRTV(1);
					PushRenderTarget(renderSystem, renderTarget, view);
					RTV_CPU_HANDLES.push_back(view);
					renderTargetViews.push_back({ renderTarget, view });
				}
			}
		}
		else
		{
			auto view = _ReserveRTV(RTs.size());
			
			for (auto& renderTarget : RTs)
			{
				RTV_CPU_HANDLES.push_back(view);
				view = PushRenderTarget(renderSystem, renderTarget, view, MIPMapOffset);
			}
		}

		auto DSV_CPU_HANDLE = D3D12_CPU_DESCRIPTOR_HANDLE{};

		if(DepthStecil)
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

				DSV_CPU_HANDLE = DSV;
			}
			else
				DSV_CPU_HANDLE = res->descriptor;
		}


		DeviceContext->OMSetRenderTargets(
			RTV_CPU_HANDLES.size(),
			RTV_CPU_HANDLES.begin(),
			DepthStecil,
			DepthStecil ? &DSV_CPU_HANDLE : nullptr);
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
					RTV_CPU_HANDLES.push_back(res->descriptor);
				}
				else
				{
					auto view = _ReserveRTV(1);
					PushRenderTarget(renderSystem, renderTarget, view);
					RTV_CPU_HANDLES.push_back(view);
					renderTargetViews.push_back({ renderTarget, view });
				}
			}
		}
		else
		{
			auto view = _ReserveRTV(RTs.size());
			
			for (auto& renderTarget : RTs)
			{
				RTV_CPU_HANDLES.push_back(view);
				view = PushRenderTarget(renderSystem, renderTarget, view, MIPMapOffset);
			}
		}
		
		auto DSV_CPU_HANDLE = D3D12_CPU_DESCRIPTOR_HANDLE{};

		const bool depthEnabled = DSV.depthStencil != InvalidHandle;

		if(depthEnabled)
		{
			auto descriptor = _GetDepthDesciptor(DSV.depthStencil);

			PushDepthStencilArray(renderSystem, DSV.depthStencil, DSV.ArraySliceOffset, DSV.MipOffset, descriptor);

			DSV_CPU_HANDLE = descriptor;
		}

		DeviceContext->OMSetRenderTargets(
			RTV_CPU_HANDLES.size(),
			RTV_CPU_HANDLES.begin(),
			depthEnabled,
			depthEnabled ? &DSV_CPU_HANDLE : nullptr);
	}


	/************************************************************************************************/


	void Context::SetViewports(static_vector<D3D12_VIEWPORT, 16> VPs)
	{
		Viewports = VPs;
		DeviceContext->RSSetViewports(VPs.size(), VPs.begin());
	}


	/************************************************************************************************/


	void Context::SetScissorRects(static_vector<D3D12_RECT, 16>	Rects)
	{
		DeviceContext->RSSetScissorRects(Rects.size(), Rects.begin());
	}


	/************************************************************************************************/

	// Assumes setting each to fullscreen
	void Context::SetScissorAndViewports(static_vector<ResourceHandle, 16>	RenderTargets)
	{
		static_vector<D3D12_VIEWPORT, 16>	VPs;
		static_vector<D3D12_RECT, 16>		Rects;

		for (auto RT : RenderTargets)
		{
			auto WH = renderSystem->GetTextureWH(RT);
			VPs.push_back	({ 0, 0,	(FLOAT)WH[0], (FLOAT)WH[1], 0, 1 });
			Rects.push_back	({ 0,0,	(LONG)WH[0], (LONG)WH[1] });
		}

		SetViewports(VPs);
		SetScissorRects(Rects);
	}

	/************************************************************************************************/


	void Context::SetScissorAndViewports2(static_vector<ResourceHandle, 16>	RenderTargets, const size_t MIPMapOffset)
	{
		static_vector<D3D12_VIEWPORT, 16>	VPs;
		static_vector<D3D12_RECT, 16>		Rects;

		for (auto RT : RenderTargets)
		{
			auto WH = renderSystem->GetTextureWH(RT) / std::pow(2, MIPMapOffset);
			VPs.push_back({ 0, 0,	(FLOAT)WH[0], (FLOAT)WH[1], 0, 1 });
			Rects.push_back({ 0,0,	(LONG)WH[0], (LONG)WH[1] });
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
				RenderTargetCount,
				RenderTargetCount ? &RTVPOSCPU : nullptr, true,
				DepthStencilEnabled ? &DSVPOSCPU : nullptr);
		}
		else 
			DepthStencilEnabled = false;
	}


	/************************************************************************************************/


	void Context::SetPrimitiveTopology(EInputTopology Topology)
	{
		D3D12_PRIMITIVE_TOPOLOGY D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_POINTLIST;
		switch (Topology)
		{
		case EIT_LINE:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			break;
		case EIT_TRIANGLELIST:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			break;
		case EIT_TRIANGLE:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			break;
		case EIT_POINT:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_1:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_2:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_3:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_4:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_5:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_6:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_7:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_8:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_9:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_10:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_11:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_12:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_13:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_14:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_15:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_16:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_17:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_18:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_19:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_20:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_21:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_22:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_23:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_24:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST;
			break;
        case EInputTopology::EIT_PATCH_CP_25:
            D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST;
            break;
        case EInputTopology::EIT_PATCH_CP_26:
            D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST;
            break;
        case EInputTopology::EIT_PATCH_CP_27:
            D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST;
            break;
        case EInputTopology::EIT_PATCH_CP_28:
            D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST;
            break;
        case EInputTopology::EIT_PATCH_CP_29:
            D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST;
            break;
        case EInputTopology::EIT_PATCH_CP_30:
            D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST;
            break;
        case EInputTopology::EIT_PATCH_CP_31:
            D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST;
            break;
        case EInputTopology::EIT_PATCH_CP_32:
            D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST;
            break;
		default:
			FK_ASSERT(0);
		}

		DeviceContext->IASetPrimitiveTopology(D3DTopology);
	}


	/************************************************************************************************/


    void Context::SetGraphicsConstantValue(size_t idx, size_t valueCount, const void* data_ptr, size_t offset)
    {
        DeviceContext->SetGraphicsRoot32BitConstants(idx, valueCount, data_ptr, offset);
    }


    /************************************************************************************************/


	void Context::NullGraphicsConstantBufferView(size_t idx)
	{
        DeviceContext->SetGraphicsRootConstantBufferView(idx, 0);
	}


	/************************************************************************************************/


	void Context::SetGraphicsConstantBufferView(size_t idx, const ConstantBufferHandle CB, size_t Offset)
	{
		FK_ASSERT(!(Offset % 256), "Incorrect CB Offset!");

		DeviceContext->SetGraphicsRootConstantBufferView(idx, renderSystem->GetConstantBufferAddress(CB) + Offset);
	}


	/************************************************************************************************/


	void Context::SetGraphicsConstantBufferView(size_t idx, const ConstantBuffer& CB)
	{
		DeviceContext->SetGraphicsRootConstantBufferView(idx, CB.Get()->GetGPUVirtualAddress());
	}


	/************************************************************************************************/


	void Context::SetGraphicsConstantBufferView(size_t idx, const ConstantBufferDataSet& CB)
	{
		DeviceContext->SetGraphicsRootConstantBufferView(idx, renderSystem->GetConstantBufferAddress(CB.Handle()) + CB.Offset());
	}


	/************************************************************************************************/


	void Context::SetGraphicsDescriptorTable(size_t idx, const DescriptorHeap& DH)
	{
		DeviceContext->SetGraphicsRootDescriptorTable(idx, DH);
	}


	/************************************************************************************************/


	void Context::SetGraphicsShaderResourceView(size_t idx, FrameBufferedResource* Resource, size_t Count, size_t ElementSize)
	{
		DeviceContext->SetGraphicsRootShaderResourceView(idx, Resource->Get()->GetGPUVirtualAddress());
	}


	/************************************************************************************************/

	void Context::SetGraphicsShaderResourceView(size_t idx, Texture2D& Texture)
	{
		DeviceContext->SetGraphicsRootShaderResourceView(idx, Texture->GetGPUVirtualAddress());
	}


    /************************************************************************************************/


    void Context::SetGraphicsShaderResourceView(size_t idx, ResourceHandle resource, size_t offset)
    {
        DeviceContext->SetGraphicsRootShaderResourceView(idx, renderSystem->GetDeviceResource(resource)->GetGPUVirtualAddress());
    }


    /************************************************************************************************/


    void Context::SetGraphicsUnorderedAccessView(size_t idx, ResourceHandle UAVresource, size_t offset)
    {
        auto resource = renderSystem->GetDeviceResource(UAVresource);

        DeviceContext->SetGraphicsRootUnorderedAccessView(idx, resource->GetGPUVirtualAddress() + offset);
    }


	/************************************************************************************************/


    void Context::SetComputeDescriptorTable(size_t idx)
    {
        DeviceContext->SetComputeRootDescriptorTable(idx, D3D12_GPU_DESCRIPTOR_HANDLE{ 0 });
    }


	void Context::SetComputeDescriptorTable(size_t idx, const DescriptorHeap& DH)
	{
		DeviceContext->SetComputeRootDescriptorTable(idx, DH);
	}


	/************************************************************************************************/


	void Context::SetComputeConstantBufferView(size_t idx, const ConstantBufferHandle CB, size_t offset)
	{
		DeviceContext->SetGraphicsRootConstantBufferView(idx, renderSystem->GetConstantBufferAddress(CB) + offset);
	}


	/************************************************************************************************/


	void Context::SetComputeConstantBufferView(size_t idx, const ConstantBufferDataSet& CB)
	{
		DeviceContext->SetComputeRootConstantBufferView(idx, renderSystem->GetConstantBufferAddress(CB.Handle()) + CB.Offset());
	}


    /************************************************************************************************/


    void Context::SetComputeConstantBufferView(size_t idx, ResourceHandle resource, size_t offset, size_t bufferSize)
    {
        auto deviceResource     = renderSystem->GetDeviceResource(resource);
        auto gpuAddress         = deviceResource->GetGPUVirtualAddress();

        DeviceContext->SetComputeRootConstantBufferView(idx, gpuAddress + offset);
    }


	/************************************************************************************************/


	void Context::SetComputeShaderResourceView(size_t idx, Texture2D& Texture)
	{
		DeviceContext->SetComputeRootShaderResourceView(idx, Texture->GetGPUVirtualAddress());
	}


    /************************************************************************************************/


    void Context::SetComputeShaderResourceView(size_t idx, ResourceHandle resource, const size_t offset)
    {
        DeviceContext->SetComputeRootShaderResourceView(idx, renderSystem->GetDeviceResource(resource)->GetGPUVirtualAddress() + offset);
    }


	/************************************************************************************************/


	void Context::SetComputeUnorderedAccessView(size_t idx, ResourceHandle UAVresource, size_t offset)
	{
		auto resource = renderSystem->GetDeviceResource(UAVresource);

		DeviceContext->SetComputeRootUnorderedAccessView(idx, resource->GetGPUVirtualAddress() + offset);
	}


    /************************************************************************************************/


    void Context::SetComputeConstantValue(size_t idx, size_t valueCount, const void* data_ptr, size_t offset)
    {
        DeviceContext->SetComputeRoot32BitConstants(idx, valueCount, data_ptr, offset);
    }


	/************************************************************************************************/


	void Context::BeginQuery(QueryHandle query, size_t idx)
	{
        if (query == InvalidHandle)
            return;

		auto resource	= renderSystem->Queries.GetAsset(query);
		auto queryType	= renderSystem->Queries.GetType(query);

		DeviceContext->BeginQuery(resource, queryType, idx);
	}


	void Context::EndQuery(QueryHandle query, size_t idx)
	{
        if (query == InvalidHandle)
            return;

		auto resource	= renderSystem->Queries.GetAsset(query);
		auto queryType	= renderSystem->Queries.GetType(query);

		DeviceContext->EndQuery(resource, queryType, idx);
	}


    void Context::TimeStamp(QueryHandle query, size_t idx)
    {
        if (query == InvalidHandle)
            return;

        auto resource   = renderSystem->Queries.GetAsset(query);
        auto queryType  = renderSystem->Queries.GetType(query);

        DeviceContext->EndQuery(resource, queryType, idx);
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


	void Context::CopyBufferRegion(
			static_vector<ID3D12Resource*>		sources,
			static_vector<size_t>				sourceOffset,
			static_vector<ID3D12Resource*>		destinations,
			static_vector<size_t>				destinationOffset,
			static_vector<size_t>				copySize,
			static_vector<DeviceResourceState>	currentStates,
			static_vector<DeviceResourceState>	finalStates)
	{
		FK_ASSERT(sources.size() == destinations.size(), "Invalid argument!");


		ID3D12Resource*		prevResource	= nullptr;

		for (size_t itr = 0; itr < sources.size(); ++itr)
		{
			auto resource	= destinations[itr];
			auto state		= currentStates[itr];

			if(prevResource != resource && state != DeviceResourceState::DRS_CopyDest)
				_AddBarrier(resource, state, DeviceResourceState::DRS_CopyDest);

			prevResource	= resource;
		}

		FlushBarriers();

		for (size_t itr = 0; itr < sources.size(); ++itr)
		{
			auto sourceResource			= sources[itr];
			auto destinationResource	= destinations[itr];

			DeviceContext->CopyBufferRegion(
				destinationResource,
				destinationOffset[itr],
				sourceResource,
				sourceOffset[itr],
				copySize[itr]);
		}

		DeviceResourceState prevState	= DeviceResourceState::DRS_ERROR;
		prevResource					= nullptr;

		for (size_t itr = 0; itr < destinations.size(); ++itr)
		{
			auto resource	= destinations[itr];
			auto state		= finalStates[itr];

			if (prevResource != resource && prevState != state && state != DeviceResourceState::DRS_CopyDest)
				_AddBarrier(resource, DeviceResourceState::DRS_CopyDest, state);

			prevResource	= resource;
            prevState       = state;
		}
	}


	/************************************************************************************************/


	void Context::ImmediateWrite(
		static_vector<ResourceHandle>		    handles,
		static_vector<size_t>					value,
		static_vector<DeviceResourceState>		currentStates,
		static_vector<DeviceResourceState>		finalStates)
	{
		FK_ASSERT(handles.size() == currentStates.size(), "Invalid argument!");

		/*
		typedef struct D3D12_WRITEBUFFERIMMEDIATE_PARAMETER
		{
		D3D12_GPU_VIRTUAL_ADDRESS Dest;
		UINT32 Value;
		} 	D3D12_WRITEBUFFERIMMEDIATE_PARAMETER;
		*/

		DeviceResourceState prevState		= DeviceResourceState::DRS_ERROR;
		ID3D12Resource*		prevResource	= nullptr;

		for (size_t itr = 0; itr < handles.size(); ++itr)
		{
			auto resource	= renderSystem->GetDeviceResource(handles[itr]);
			auto state		= currentStates[itr];

			if(prevResource != resource && prevState != state)
				_AddBarrier(resource, state, DeviceResourceState::DRS_CopyDest);

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

		prevState		= DeviceResourceState::DRS_ERROR;
		prevResource	= nullptr;

		for (size_t itr = 0; itr < handles.size(); ++itr)
		{
			auto resource	= renderSystem->GetDeviceResource(handles[itr]);
			auto state		= currentStates[itr];

			if (prevResource != resource && prevState != state)
				_AddBarrier(resource, DeviceResourceState::DRS_CopyDest, state);

			prevResource	= resource;
			prevState		= prevState;
		}
	}


	/************************************************************************************************/


	// Requires SO resources to be in DeviceResourceState::DRS::STREAMOUTCLEAR!
	void Context::ClearSOCounters(static_vector<SOResourceHandle> handles)
	{
		/*
		typedef struct D3D12_WRITEBUFFERIMMEDIATE_PARAMETER
		{
		D3D12_GPU_VIRTUAL_ADDRESS Dest;
		UINT32 Value;
		} 	D3D12_WRITEBUFFERIMMEDIATE_PARAMETER;
		*/

		static_vector<ID3D12Resource*>		sources;
		static_vector<size_t>				sourceOffset;
		static_vector<ID3D12Resource*>		destinations;
		static_vector<size_t>				destinationOffset;
		static_vector<size_t>				copySize;
		static_vector<DeviceResourceState>	currentSOStates;
		static_vector<DeviceResourceState>	finalStates;

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
			currentSOStates.push_back(DeviceResourceState::DRS_CopyDest);

		for (auto& s : handles)
			finalStates.push_back(DeviceResourceState::DRS_CopyDest);


		CopyBufferRegion(
			sources,			// sources
			sourceOffset,		// source offsets
			destinations,		// destinations
			destinationOffset,  // destination offsets
			copySize,			// copy sizes
			currentSOStates,	// source initial state
			finalStates);		// source final	state
	}


	/************************************************************************************************/


	void Context::CopyUInt64(
		static_vector<ID3D12Resource*>			sources,
		static_vector<DeviceResourceState>		sourceState,
		static_vector<size_t>					sourceOffsets,
		static_vector<ID3D12Resource*>			destinations,
		static_vector<DeviceResourceState>		destinationState,
		static_vector<size_t>					destinationOffset)
	{
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

		// transition source resources
		for (size_t itr = 0; itr < sources.size(); ++itr) 
		{
			auto resource	= sources[itr];
			auto state		= sourceState[itr];
			_AddBarrier(resource, state, DeviceResourceState::DRS_CopySrc);
		}

		for (size_t itr = 0; itr < sources.size(); ++itr)
		{
			auto resource	= destinations[itr];
			auto state		= destinationState[itr];
			_AddBarrier(resource, state, DeviceResourceState::DRS_CopyDest);
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
			_AddBarrier(resource, DeviceResourceState::DRS_CopySrc, state);
		}

		for (size_t itr = 0; itr < sources.size(); ++itr)
		{
			auto resource	= destinations[itr];
			auto state		= destinationState[itr];
			_AddBarrier(resource, DeviceResourceState::DRS_CopyDest, state);
		}
	}


	/************************************************************************************************/


	void Context::AddIndexBuffer(TriMesh* Mesh, uint32_t lod)
	{
		const size_t	IBIndex		= Mesh->lods[lod].GetIndexBufferIndex();
		const size_t	IndexCount	= Mesh->lods[lod].GetIndexCount();

		D3D12_INDEX_BUFFER_VIEW		IndexView;
		IndexView.BufferLocation	= GetBuffer(Mesh, lod, IBIndex)->GetGPUVirtualAddress();
		IndexView.Format			= DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
		IndexView.SizeInBytes		= IndexCount * 4;

		DeviceContext->IASetIndexBuffer(&IndexView);
	}


	/************************************************************************************************/


	void Context::SetIndexBuffer(VertexBufferEntry buffer, DeviceFormat format)
	{
		D3D12_INDEX_BUFFER_VIEW		IndexView;
		IndexView.BufferLocation    = renderSystem->GetVertexBufferAddress(buffer.VertexBuffer) + buffer.Offset;
		IndexView.Format            = TextureFormat2DXGIFormat(format);
		IndexView.SizeInBytes       = renderSystem->GetVertexBufferSize(buffer.VertexBuffer) - buffer.Offset;

		DeviceContext->IASetIndexBuffer(&IndexView);
	}


	/************************************************************************************************/


	void Context::AddVertexBuffers(TriMesh* Mesh, uint32_t lod, static_vector<VERTEXBUFFER_TYPE, 16> Buffers, VertexBufferList* InstanceBuffers)
	{
		static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;

		for(auto& I : Buffers)
			FK_ASSERT(AddVertexBuffer(I, Mesh, lod, VBViews));

		if (InstanceBuffers)
		{
			for (auto& IB : *InstanceBuffers)
			{
				VBViews.push_back({
					renderSystem->GetVertexBufferAddress(IB.VertexBuffer)		+ IB.Offset,
					(UINT)renderSystem->GetVertexBufferSize(IB.VertexBuffer)	- IB.Offset,
					IB.Stride });
			}
		}

		DeviceContext->IASetVertexBuffers(0, VBViews.size(), VBViews.begin());
	}


	/************************************************************************************************/

	
	void Context::SetVertexBuffers(VertexBufferList& List)
	{
		static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;
		for (auto& VB : List)
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

		DeviceContext->IASetVertexBuffers(0, VBViews.size(), VBViews.begin());
	}



	/************************************************************************************************/


	void Context::SetVertexBuffers(VertexBufferList List)
	{
		static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;
		for (auto& VB : List)
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
				(UINT)renderSystem->GetVertexBufferSize(VB.VertexBuffer) - VB.Offset,
				VB.Stride});
		}

		DeviceContext->IASetVertexBuffers(0, VBViews.size(), VBViews.begin());
	}


	/************************************************************************************************/


	void Context::SetVertexBuffers2(static_vector<D3D12_VERTEX_BUFFER_VIEW>	List)
	{
		DeviceContext->IASetVertexBuffers(0, List.size(), List.begin());
	}


	/************************************************************************************************/


	void Context::SetSOTargets(static_vector<D3D12_STREAM_OUTPUT_BUFFER_VIEW, 4> SOViews)
	{
		DeviceContext->SOSetTargets(0, SOViews.size(), SOViews.begin());
	}


	/************************************************************************************************/


	void Context::ClearDepthBuffer(ResourceHandle depthBuffer, float ClearDepth)
	{
		UpdateResourceStates();

        auto descriptor = _GetDepthDesciptor(depthBuffer);
        PushDepthStencilArray(renderSystem, depthBuffer, 0, 0, descriptor);

		DeviceContext->ClearDepthStencilView(descriptor, D3D12_CLEAR_FLAG_DEPTH, ClearDepth, 0, 0, nullptr);
		renderSystem->Textures.MarkRTUsed(depthBuffer);
	}


	/************************************************************************************************/


	void Context::ClearRenderTarget(ResourceHandle renderTarget, float4 ClearColor)
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
			RTV_CPU_HANDLES = res->descriptor;
		}
		else
		{
			auto view = _ReserveRTV(1);
			PushRenderTarget(renderSystem, renderTarget, view);
			RTV_CPU_HANDLES = view;
			renderTargetViews.push_back({ renderTarget, view });
		}

		DeviceContext->ClearRenderTargetView(RTV_CPU_HANDLES, ClearColor, 0, nullptr);

		renderSystem->Textures.MarkRTUsed(renderTarget);
	}


	/************************************************************************************************/


	void Context::ClearUAVTextureFloat(ResourceHandle UAV, float4 clearColor)
	{
		auto view       = _ReserveSRVLocal(1);
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
			view);

		FlushBarriers();

		DeviceContext->ClearUnorderedAccessViewFloat(view, view, resource, clearColor, 0, nullptr);
	}

	/************************************************************************************************/


	void Context::ClearUAVTextureUint(ResourceHandle UAV, uint4 clearColor)
	{
        auto CPUview    = _ReserveSRVLocal(1);
        auto GPUview    = _ReserveSRV(1);
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
			CPUview);

        PushUAV2DToDescHeap(
            renderSystem,
            tex,
            GPUview);

        const auto CPUHandle = CPUview.Get<0>();
		const auto GPUHandle = GPUview.Get<1>();

		FlushBarriers();

		DeviceContext->ClearUnorderedAccessViewUint(GPUHandle, CPUHandle, resource, (UINT*)&clearColor, 0, nullptr);
	}


	/************************************************************************************************/


	void Context::ClearUAV(ResourceHandle resource, uint4 clearColor)
	{
		const auto view               = _ReserveSRVLocal(1);
		const auto deviceResource     = renderSystem->GetDeviceResource(resource);
		const auto deviceFormat       = renderSystem->GetTextureDeviceFormat(resource);

        PushUAV1DToDescHeap(renderSystem, deviceResource, deviceFormat, 0, view);

		const auto CPUHandle = view.Get<0>();
		const auto GPUHandle = view.Get<1>();

		FlushBarriers();

		DeviceContext->ClearUnorderedAccessViewUint(GPUHandle, CPUHandle, deviceResource, clearColor, 0, 0);
	}


    /************************************************************************************************/


    void Context::ClearUAVBuffer(ResourceHandle UAV, uint4 clearColor)
    {
        BeginEvent_DEBUG("ClearUAVBuffer");

        UpdateResourceStates();

        auto PSO = renderSystem->GetPSO(CLEARBUFFERPSO);
        DeviceContext->SetComputeRootSignature(renderSystem->Library.ClearBuffer);
        DeviceContext->SetPipelineState(PSO);
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

        uint2 range{ begin / 16, end / 16};

        auto PSO = renderSystem->GetPSO(CLEARBUFFERPSO);
        DeviceContext->SetComputeRootSignature(renderSystem->Library.ClearBuffer);
        DeviceContext->SetPipelineState(PSO);
        DeviceContext->SetComputeRoot32BitConstants(0, 4, &clearColor, 0);
        DeviceContext->SetComputeRoot32BitConstants(0, 2, &range, 4);
        DeviceContext->SetComputeRootUnorderedAccessView(1, renderSystem->GetDeviceResource(UAV)->GetGPUVirtualAddress());

        auto resourceSize = renderSystem->GetResourceSize(UAV);
        DeviceContext->Dispatch(UINT(ceil(Min(resourceSize, end - begin)/ 1024.0f)), 1, 1);

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
		auto queryResource	= renderSystem->Queries.GetAsset(query);

		UpdateResourceStates();

		DeviceContext->ResolveQueryData(queryResource, type, begin, end - begin, res, destOffset);
	}


    /************************************************************************************************/


    void Context::ResolveQuery(QueryHandle query, size_t begin, size_t end, ID3D12Resource* destination, size_t destOffset)
    {
        if (query == InvalidHandle)
            return;

		auto type			= renderSystem->Queries.GetType(query);
		auto queryResource	= renderSystem->Queries.GetAsset(query);

		UpdateResourceStates();

		DeviceContext->ResolveQueryData(queryResource, type, begin, end - begin, destination, destOffset);
    }


	/************************************************************************************************/


	void Context::ExecuteIndirect(ResourceHandle args, const IndirectLayout& layout, size_t argumentBufferOffset, size_t executionCount)
	{
		UpdateResourceStates();

		DeviceContext->ExecuteIndirect(
			layout.signature, 
			Min(layout.entries.size(), executionCount), 
			renderSystem->GetDeviceResource(args),
			argumentBufferOffset,
			nullptr, 
			0);
	}


	void Context::Draw(const size_t VertexCount, const size_t BaseVertex, const size_t baseIndex)
	{
		UpdateResourceStates();
		DeviceContext->DrawInstanced(VertexCount, 1, BaseVertex, baseIndex);
	}


    void Context::DrawInstanced(const size_t vertexCount, const size_t baseVertex, const size_t instanceCount, const size_t instanceOffset )
    {
        UpdateResourceStates();
        DeviceContext->DrawInstanced(vertexCount, instanceCount, baseVertex, instanceOffset);
    }


	void Context::DrawIndexed(const size_t IndexCount, const size_t IndexOffet, const size_t BaseVertex)
	{
		UpdateResourceStates();
		DeviceContext->DrawIndexedInstanced(IndexCount, 1, IndexOffet, BaseVertex, 0);
	}


	void Context::DrawIndexedInstanced(
		const size_t IndexCount, const size_t IndexOffet, 
		const size_t BaseVertex, const size_t InstanceCount, 
		const size_t InstanceOffset)
	{
		UpdateResourceStates();
		DeviceContext->DrawIndexedInstanced(
			IndexCount, InstanceCount, 
			IndexOffet, BaseVertex, 
			InstanceOffset);
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

        dispatchDesc.CallableShaderTable        = desc.callableShaderTable;
        dispatchDesc.HitGroupTable              = desc.hitGroupTable;
        dispatchDesc.MissShaderTable            = desc.missTable;
        dispatchDesc.RayGenerationShaderRecord  = desc.rayGenerationRecord;

        DeviceContext->DispatchRays(&dispatchDesc);
    }


	/************************************************************************************************/


	void Context::FlushBarriers()
	{
		UpdateResourceStates();
	}

	/************************************************************************************************/


	void Context::SetPredicate(bool Enabled, QueryHandle Handle, size_t Offset)
	{
		if (Enabled)
			DeviceContext->SetPredication(
				reinterpret_cast<ID3D12Resource*>(renderSystem->_GetQueryResource(Handle)),
				Offset, 
				D3D12_PREDICATION_OP::D3D12_PREDICATION_OP_NOT_EQUAL_ZERO);
		else
			DeviceContext->SetPredication(nullptr, 0, D3D12_PREDICATION_OP::D3D12_PREDICATION_OP_EQUAL_ZERO);
	}


	/************************************************************************************************/


	void Context::CopyBuffer(const UploadSegment src, const ResourceHandle destination, const size_t destOffset)
	{
		const auto destinationResource	= renderSystem->GetDeviceResource(destination);
		const auto sourceResource       = src.resource;

		UpdateResourceStates();

		DeviceContext->CopyBufferRegion(destinationResource, destOffset, sourceResource, src.offset, src.uploadSize);
	}


	/************************************************************************************************/


	void Context::CopyTexture2D(const UploadSegment src, const ResourceHandle destination, const uint2 BufferSize)
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
		srcLocation.PlacedFootprint.Footprint.RowPitch	= BufferSize[0] * texelSize;

		UpdateResourceStates();
		DeviceContext->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, nullptr);
	}


	/************************************************************************************************/


	void Context::Clear()
	{
		PendingBarriers.clear();
		RenderTargets.clear();
		Viewports.clear();
		DesciptorHeaps.clear();
		VBViews.clear();

		TrackedSOBuffers.clear();
		PendingBarriers.clear();

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


	void Context::Close(const size_t IN_counter)
	{
		counter = IN_counter;
        if (auto HR = DeviceContext->Close(); FAILED(HR)) {
            FK_LOG_ERROR("Failed to close graphics context!");
            renderSystem->_OnCrash();
        }
	}


	/************************************************************************************************/


	Context& Context::Reset()
	{
        CurrentPipelineState = nullptr;

        if (FAILED(commandAllocator->Reset()))
            FK_LOG_ERROR("Failed to reset command allocator");

		if(FAILED(DeviceContext->Reset(commandAllocator, nullptr)))
            FK_LOG_ERROR("Failed to reset device context");

		_ResetDSV();
		_ResetRTV();
		_ResetSRV();

        TrackedSOBuffers.clear();
        PendingBarriers.clear();
        queuedBarriers.clear();
        renderTargetViews.clear();
        depthStencilViews.clear();
        queuedReadBacks.clear();

		ID3D12DescriptorHeap* heaps[]{ descHeapSRV };

		DeviceContext->SetDescriptorHeaps(sizeof(heaps) / sizeof(heaps[0]), heaps);

		return *this;
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


    void Context::BeginMarker(const char* str)
    {
    }


    /************************************************************************************************/


    void Context::EndMarker(const char* str)
    {

    }


    /************************************************************************************************/


	void Context::_AddBarrier(ID3D12Resource* resource, DeviceResourceState currentState, DeviceResourceState newState)
	{
		Barrier barrier;
		barrier.Type		    = Barrier::Type::Generic;
		barrier.resource_ptr    = resource;
		barrier.OldState	    = currentState;
		barrier.NewState	    = newState;

        if (PendingBarriers.full())
            FlushBarriers();

		PendingBarriers.push_back(barrier);
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
		if (!PendingBarriers.size())
			return;

		static_vector<D3D12_RESOURCE_BARRIER, 64> Barriers;

		for (auto& B : PendingBarriers)
		{
			switch(B.Type)
			{
				case Barrier::Type::ConstantBuffer:
				case Barrier::Type::VertexBuffer:
					break;
				case Barrier::Type::StreamOut:
				{

					if (DeviceResourceState::DRS_STREAMOUTCLEAR == B.OldState || DeviceResourceState::DRS_STREAMOUTCLEAR == B.NewState) {
						auto handle				= B.streamOut;
						auto SOresource			= renderSystem->GetDeviceResource(handle);
						auto resource			= renderSystem->GetSOCounterResource(handle);
						auto currentState		= DRS2D3DState((B.OldState == DeviceResourceState::DRS_VERTEXBUFFER) ? DeviceResourceState::DRS_STREAMOUT : B.OldState);
						auto newState			= DRS2D3DState((B.NewState == DeviceResourceState::DRS_VERTEXBUFFER) ? DeviceResourceState::DRS_STREAMOUT : B.NewState);

						auto currentSOState		= DRS2D3DState((B.OldState == DeviceResourceState::DRS_STREAMOUTCLEAR) ? DeviceResourceState::DRS_STREAMOUT : B.OldState);
						auto newSOState			= DRS2D3DState((B.NewState == DeviceResourceState::DRS_STREAMOUTCLEAR) ? DeviceResourceState::DRS_STREAMOUT : B.NewState);

						if (B.OldState != B.NewState)
							Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(resource,	currentState, newState, B.subResource));

						if(currentSOState != newSOState)
							Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(SOresource, currentSOState, newSOState, B.subResource));
					}
					else
					{
						auto handle				= B.streamOut;
						auto resource			= renderSystem->GetDeviceResource(handle);
						auto currentState		= DRS2D3DState(B.OldState);
						auto newState			= DRS2D3DState(B.NewState);

						if (B.OldState != B.NewState)
							Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(resource, currentState, newState, B.subResource));
					}
				}	break;
                case Barrier::Type::Generic:
				{
					auto resource		= B.resource_ptr;
					auto currentState	= DRS2D3DState(B.OldState);
					auto newState		= DRS2D3DState(B.NewState);

					/*
					#ifdef _DEBUG
						std::cout << "Transitioning Resource: " << Resource
							<< " From State: " << CurrentState << " To State: "
							<< NewState << "\n";
					#endif
					*/

					if (B.OldState != B.NewState)
						Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
							resource, currentState, newState, B.subResource));
				}	break;
                case Barrier::Type::Resource:
				{
					auto resource		= renderSystem->GetDeviceResource(B.resourceHandle);
					auto currentState	= DRS2D3DState(B.OldState);
					auto newState		= DRS2D3DState(B.NewState);

					if (B.OldState != B.NewState)
						Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
							resource, currentState, newState, B.subResource));
				}	break;
                case Barrier::Type::Aliasing:
				{
                    auto before = renderSystem->GetDeviceResource(B.aliasedResources[0]);
                    auto after  = renderSystem->GetDeviceResource(B.aliasedResources[1]);
					const auto aliasingBarrier = CD3DX12_RESOURCE_BARRIER::Aliasing(before, after);

					Barriers.push_back(aliasingBarrier);
				}   break;
                case Barrier::Type::UAV:
                {
                    const auto UAVBarrier = CD3DX12_RESOURCE_BARRIER::UAV(renderSystem->GetDeviceResource(B.resourceHandle));
                    Barriers.push_back(UAVBarrier);

                }   break;
				default:
					FK_ASSERT(0);
			};
		}

		if (Barriers.size())
			DeviceContext->ResourceBarrier(Barriers.size(), Barriers.begin());

		Barriers.clear();
		PendingBarriers.clear();
	}


	/************************************************************************************************/


	void Context::_QueueReadBacks()
	{
		for (const auto readBackHandle : queuedReadBacks)
		{
			auto& readBack  = renderSystem->ReadBackTable[readBackHandle];
			auto fence      = renderSystem->ReadBackTable._GetReadBackFence();

			const uint64_t counter = ++renderSystem->copyEngine.counter;

			auto HR     = fence->SetEventOnCompletion(counter, readBack.event); FK_ASSERT(SUCCEEDED(HR));
			renderSystem->GraphicsQueue->Signal(fence, counter);

			readBack.queueUntil = counter;
			readBack.queued     = true;
		}

		queuedReadBacks.clear();
	}


	/************************************************************************************************/


	DescHeapPOS Context::_ReserveDSV(size_t count)
	{
		auto currentCPU = DSV_CPU;
		DSV_CPU.ptr = DSV_CPU.ptr + renderSystem->DescriptorDSVSize * count;

		return { currentCPU, 0 };
	}


	/************************************************************************************************/


	DescHeapPOS Context::_ReserveSRV(size_t count)
	{
		auto currentCPU = SRV_CPU;
		auto currentGPU = SRV_GPU;
		SRV_CPU.ptr = SRV_CPU.ptr + renderSystem->DescriptorCBVSRVUAVSize * count;
		SRV_GPU.ptr = SRV_GPU.ptr + renderSystem->DescriptorCBVSRVUAVSize * count;

		return { currentCPU, currentGPU };
	}


	/************************************************************************************************/


	DescHeapPOS Context::_ReserveSRVLocal(size_t count)
	{
		auto currentCPU = SRV_LOCAL_CPU;
		SRV_LOCAL_CPU.ptr = SRV_LOCAL_CPU.ptr + renderSystem->DescriptorCBVSRVUAVSize * count;

		return { currentCPU, 0 };
	}


	/************************************************************************************************/


	DescHeapPOS Context::_ReserveRTV(size_t count)
	{
		auto currentCPU = RTV_CPU;
		RTV_CPU.ptr = RTV_CPU.ptr + renderSystem->DescriptorRTVSize * count;

		return { currentCPU, 0 };
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
		SRV_CPU = descHeapSRV->GetCPUDescriptorHandleForHeapStart();
		SRV_GPU = descHeapSRV->GetGPUDescriptorHandleForHeapStart();

		SRV_LOCAL_CPU = descHeapSRVLocal->GetCPUDescriptorHandleForHeapStart();
        //descHeapSRVLocal->GetGPUDescriptorHandleForHeapStart();
	}


	/************************************************************************************************/


	void RenderSystem::RootSigLibrary::Initiate(RenderSystem* RS, iAllocator* TempMemory)
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

		CD3DX12_STATIC_SAMPLER_DESC	 Samplers[] = {
			CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT, 
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP),

			CD3DX12_STATIC_SAMPLER_DESC{1, D3D12_FILTER::D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR },
			CD3DX12_STATIC_SAMPLER_DESC{2, D3D12_FILTER::D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT},
		};

		{
			CD3DX12_DESCRIPTOR_RANGE ranges[2];
			ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);
			ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 4, 4);

			RS->Library.RS6CBVs4SRVs.AllowIA = true;
			DesciptorHeapLayout<2> DescriptorHeap;
			DescriptorHeap.SetParameterAsSRV(0, 0, 6);
			DescriptorHeap.SetParameterAsCBV(1, 6, 4);
			FK_ASSERT(DescriptorHeap.Check());

			RS->Library.RS6CBVs4SRVs.SetParameterAsDescriptorTable(0, DescriptorHeap, -1);
			RS->Library.RS6CBVs4SRVs.SetParameterAsCBV(1, 0, 0, PIPELINE_DEST_ALL);
			RS->Library.RS6CBVs4SRVs.SetParameterAsCBV(2, 1, 0, PIPELINE_DEST_ALL);
			RS->Library.RS6CBVs4SRVs.SetParameterAsCBV(3, 2, 0, PIPELINE_DEST_ALL);
			RS->Library.RS6CBVs4SRVs.SetParameterAsCBV(4, 3, 0, PIPELINE_DEST_ALL);
			RS->Library.RS6CBVs4SRVs.SetParameterAsCBV(5, 4, 0, PIPELINE_DEST_ALL);
			RS->Library.RS6CBVs4SRVs.SetParameterAsCBV(6, 5, 0, PIPELINE_DEST_ALL);
			RS->Library.RS6CBVs4SRVs.Build(RS, TempMemory);
			SETDEBUGNAME(RS->Library.RS6CBVs4SRVs, "RS4CBVs4SRVs");
		}
		{
			RS->Library.RS4CBVs_SO.AllowIA	= true;
			RS->Library.RS4CBVs_SO.AllowSO	= true;
			DesciptorHeapLayout<1> DescriptorHeap;
			DescriptorHeap.SetParameterAsSRV(0, 0, 8);

			RS->Library.RS4CBVs_SO.SetParameterAsCBV				(0, 0, 0, PIPELINE_DEST_ALL);
			RS->Library.RS4CBVs_SO.SetParameterAsCBV				(1, 1, 0, PIPELINE_DEST_ALL);
			RS->Library.RS4CBVs_SO.SetParameterAsCBV				(2, 2, 0, PIPELINE_DEST_ALL);
			RS->Library.RS4CBVs_SO.SetParameterAsDescriptorTable	(3, DescriptorHeap, -1);
			RS->Library.RS4CBVs_SO.SetParameterAsUAV				(4, 0, 0, PIPELINE_DEST_ALL);
			RS->Library.RS4CBVs_SO.Build(RS, TempMemory);

			SETDEBUGNAME(RS->Library.RS4CBVs_SO, "RS4CBVs_SO");
		}
		{
			RS->Library.RS2UAVs4SRVs4CBs.AllowIA = true;
			DesciptorHeapLayout<16> DescriptorHeap;
			DescriptorHeap.SetParameterAsShaderUAV	(0, 0, 4);
			DescriptorHeap.SetParameterAsSRV		(1, 0, 4);
			DescriptorHeap.SetParameterAsCBV		(2, 4, 4);
			FK_ASSERT(DescriptorHeap.Check());

			RS->Library.RS2UAVs4SRVs4CBs.SetParameterAsDescriptorTable(0, DescriptorHeap, -1);
			RS->Library.RS2UAVs4SRVs4CBs.SetParameterAsCBV(1, 0, 3, PIPELINE_DESTINATION::PIPELINE_DEST_ALL);
			RS->Library.RS2UAVs4SRVs4CBs.Build(RS, TempMemory);

			SETDEBUGNAME(RS->Library.RS2UAVs4SRVs4CBs, "RS2UAVs4SRVs4CBs");
		}
		{
			RS->Library.ShadingRTSig.AllowIA = false;

			DesciptorHeapLayout<16> DescriptorHeap;
			DescriptorHeap.SetParameterAsSRV		(0, 0, 8);
			DescriptorHeap.SetParameterAsShaderUAV	(1, 0, 1);
			DescriptorHeap.SetParameterAsCBV		(2, 0, 2);
			FK_ASSERT(DescriptorHeap.Check());

			RS->Library.ShadingRTSig.SetParameterAsDescriptorTable(0, DescriptorHeap, -1);
			RS->Library.ShadingRTSig.Build(RS, TempMemory);

			SETDEBUGNAME(RS->Library.ShadingRTSig, "ShadingRTSig");
		}
		{
			RS->Library.RSDefault.AllowIA = true;

			DesciptorHeapLayout<16> DescriptorHeapSRV;
			DescriptorHeapSRV.SetParameterAsSRV(0, 0, -1);
			FK_ASSERT(DescriptorHeapSRV.Check());

			DesciptorHeapLayout<16> DescriptorHeapUAV;
			DescriptorHeapUAV.SetParameterAsShaderUAV(0, 0, -1);
			FK_ASSERT(DescriptorHeapUAV.Check());

			RS->Library.RSDefault.SetParameterAsCBV				(0, 0, 0, PIPELINE_DESTINATION::PIPELINE_DEST_ALL);
			RS->Library.RSDefault.SetParameterAsCBV				(1, 1, 0, PIPELINE_DESTINATION::PIPELINE_DEST_ALL);
			RS->Library.RSDefault.SetParameterAsCBV				(2, 2, 0, PIPELINE_DESTINATION::PIPELINE_DEST_ALL);
			RS->Library.RSDefault.SetParameterAsCBV				(3, 3, 0, PIPELINE_DESTINATION::PIPELINE_DEST_ALL);
			RS->Library.RSDefault.SetParameterAsDescriptorTable	(4, DescriptorHeapSRV, -1, PIPELINE_DESTINATION::PIPELINE_DEST_ALL);
			RS->Library.RSDefault.SetParameterAsDescriptorTable	(5, DescriptorHeapUAV, -1, PIPELINE_DESTINATION::PIPELINE_DEST_ALL);
			RS->Library.RSDefault.Build(RS, TempMemory);

			SETDEBUGNAME(RS->Library.RSDefault, "RSDefault");
		}
		{
			RS->Library.ComputeSignature.AllowIA = false;
			DesciptorHeapLayout<16> DescriptorHeap;
			DescriptorHeap.SetParameterAsShaderUAV(0, 0, 4, 0);
			DescriptorHeap.SetParameterAsSRV(1, 0, 4, 0);
			DescriptorHeap.SetParameterAsCBV(2, 0, 2, 0);
			FK_ASSERT(DescriptorHeap.Check());

			RS->Library.ComputeSignature.SetParameterAsDescriptorTable(0, DescriptorHeap, -1, PIPELINE_DEST_CS);
			RS->Library.ComputeSignature.Build(RS, TempMemory);

			SETDEBUGNAME(RS->Library.ShadingRTSig, "ComputeSignature");
		}
        {
            RS->Library.ClearBuffer.SetParameterAsUINT(0, 6, 0, 0, PIPELINE_DEST_CS);
            RS->Library.ClearBuffer.SetParameterAsUAV(1, 0, 0, PIPELINE_DEST_CS);
            RS->Library.ClearBuffer.Build(RS, TempMemory);

			SETDEBUGNAME(RS->Library.ClearBuffer, "ClearBuffer");
		}
	}


	/************************************************************************************************/


	UploadBuffer::UploadBuffer(ID3D12Device* pDevice)  :
		parentDevice{ pDevice       },
		Size        { MEGABYTE * 16 }
	{
		D3D12_RESOURCE_DESC   Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(MEGABYTE * 16);
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

		Position        = 0;
		Size            = 0;
		deviceBuffer    = nullptr;
		Buffer          = nullptr;
	}


	/************************************************************************************************/


	ID3D12Resource* UploadBuffer::Resize(const size_t size)
	{
		auto previousBuffer = deviceBuffer;
		deviceBuffer->Unmap(0, 0);

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

		Position       = 0;
		Last	       = 0;
		Size           = size;
		deviceBuffer   = newDeviceBuffer;
		SETDEBUGNAME(newDeviceBuffer, "TEMPORARY");

		CD3DX12_RANGE Range(0, 0);
		HR = newDeviceBuffer->Map(0, &Range, (void**)&Buffer);   CheckHR(HR, ASSERTONFAIL("FAILED TO MAP TEMP BUFFER"));

		return previousBuffer;
	}


	/************************************************************************************************/


	void CopyContext::Barrier(ID3D12Resource* resource, const DeviceResourceState before, const DeviceResourceState after)
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

		commandList->ResourceBarrier(pendingBarriers.size(), pendingBarriers.data());
		pendingBarriers.clear();
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

			return { uploadBuffer.deviceBuffer, reserveSize, offset, buffer };
		}

		if(uploadBuffer.Last <= uploadBuffer.Position)
		{	// Safe, Do Upload
			const auto alignmentOffset = GetOffset();

			char* buffer            = uploadBuffer.Buffer + uploadBuffer.Position + alignmentOffset;
			size_t offset           = uploadBuffer.Position + alignmentOffset;
			uploadBuffer.Position  += reserveSize + alignmentOffset;

			return { uploadBuffer.deviceBuffer, reserveSize, offset, buffer };
		}

		return {};
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
			source.reserveSize);
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
		ID3D12Resource*     destination,
		size_t              subResourceIdx,
		uint3               XYZ,
		UploadReservation   source,
		uint2               WH,
		DeviceFormat        format)
	{
		flushPendingBarriers();

		const auto      deviceFormat    = TextureFormat2DXGIFormat(format);
		const size_t    formatSize      = GetFormatElementSize(deviceFormat);
		const bool      BCformat        = IsDDS(format);
		const size_t rowPitch           = Max((!BCformat ? formatSize * WH[0] : formatSize * WH[0] / 4) / 256, 1) * 256;
		//size_t alignmentOffset    = rowPitch & 0x01ff;

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT SubRegion;
		SubRegion.Footprint.Depth       = 1;
		SubRegion.Footprint.Format      = deviceFormat;
		SubRegion.Footprint.RowPitch    = rowPitch;
		SubRegion.Footprint.Width       = WH[0];
		SubRegion.Footprint.Height      = WH[1];
		SubRegion.Offset                = source.offset;

		auto destinationLocation    = CD3DX12_TEXTURE_COPY_LOCATION(destination, subResourceIdx);
		auto sourceLocation         = CD3DX12_TEXTURE_COPY_LOCATION(source.resource, SubRegion);

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
		coordinate.X              = (UINT)destTile[0];
		coordinate.Y              = (UINT)destTile[1];
		coordinate.Z              = (UINT)0;
		coordinate.Subresource    = (UINT)destTile[2];
		
        D3D12_TILE_REGION_SIZE regionSize;
		regionSize.NumTiles   = 1;
		regionSize.UseBox     = false;
		regionSize.Width      = 1;
		regionSize.Height     = 1;
		regionSize.Depth      = 1;

		commandList->CopyTiles(
			dest,
			&coordinate,
			&regionSize,
			src.resource,
			src.offset,
			D3D12_TILE_COPY_FLAG_LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE);
	}


	/************************************************************************************************/


	PackedResourceTileInfo CopyContext::GetPackedTileInfo(ID3D12Resource* resource) const
	{
		ID3D12Device* device = nullptr;
		commandList->GetDevice(IID_PPV_ARGS(&device));

		UINT                        TileCount = 0;
		D3D12_PACKED_MIP_INFO       packedMipInfo;
		D3D12_TILE_SHAPE            TileShape;
		UINT                        subResourceTilingCount = 1;
		D3D12_SUBRESOURCE_TILING    subResourceTiling_Packed;

		device->GetResourceTiling(resource, &TileCount, &packedMipInfo, &TileShape, &subResourceTilingCount, 0, &subResourceTiling_Packed);

		return {
			size_t(packedMipInfo.NumStandardMips),
			size_t(packedMipInfo.NumStandardMips + packedMipInfo.NumPackedMips),
			packedMipInfo.StartTileIndexInOverallResource };
	}


	/************************************************************************************************/


	bool CopyContext::IsSubResourceTiled(ID3D12Resource* resource, const size_t level) const
	{
		ID3D12Device* device = nullptr;
		commandList->GetDevice(IID_PPV_ARGS(&device));

		UINT                        TileCount = 0;
		D3D12_PACKED_MIP_INFO       packedMipInfo;
		D3D12_TILE_SHAPE            TileShape;
		UINT                        subResourceTilingCount = 1;
		D3D12_SUBRESOURCE_TILING    subResourceTiling_Packed;

		device->GetResourceTiling(resource, &TileCount, &packedMipInfo, &TileShape, &subResourceTilingCount, level, &subResourceTiling_Packed);

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
		const size_t currentIdx = idx++ % copyContexts.size();
		auto& ctx               = copyContexts[currentIdx];

		Wait(CopyContextHandle{ currentIdx });

		if (FAILED(ctx.commandAllocator->Reset()))
			__debugbreak();

        if (FAILED(ctx.commandList->Reset(ctx.commandAllocator, nullptr)))
            __debugbreak();

		/*
		for (auto resource : ctx.freeResources)
			resource->Release();
		*/

		return CopyContextHandle{ currentIdx };
	}


	/************************************************************************************************/


	void CopyEngine::Wait(CopyContextHandle handle)
	{
		auto& ctx = copyContexts[handle];

		if (fence->GetCompletedValue() < ctx.counter)
		{
			fence->SetEventOnCompletion(ctx.counter, ctx.eventHandle);
			WaitForSingleObject(ctx.eventHandle, INFINITY);
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


    SyncPoint CopyEngine::Submit(CopyContextHandle* begin, CopyContextHandle* end, std::optional<SyncPoint> syncOpt)
	{
		const size_t localCounter = ++counter;
		static_vector<ID3D12CommandList*, 64> cmdLists;

		for (auto itr = begin; itr < end; itr++)
		{
			auto& context = copyContexts[*itr];
			cmdLists.push_back(context.commandList);

			context.counter             = localCounter;
			context.uploadBuffer.Last   = context.uploadBuffer.Position;
			
			Close(*itr);
		}

        if (auto syncPoint = syncOpt.value_or(SyncPoint{}); syncOpt.has_value())
            copyQueue->Wait(syncPoint.fence, syncPoint.syncCounter);

		copyQueue->ExecuteCommandLists(cmdLists.size(), cmdLists);
        if (const auto HR = copyQueue->Signal(fence, localCounter); FAILED(HR))
            FK_LOG_ERROR("FAILED TO SUBMIT TO COPY ENGINE!");

        return { (UINT)localCounter, fence };
	}


	/************************************************************************************************/


	void CopyEngine::Signal(ID3D12Fence* fence, const size_t counter)
	{
        if (auto HR = copyQueue->Signal(fence, counter); FAILED(HR))
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
				WaitForSingleObject(ctx.eventHandle, INFINITY);

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

			copyContexts.push_back({
				commandAllocator,
				copyCommandList,
				0,
				CreateEvent(nullptr, FALSE, FALSE, nullptr),
				UploadBuffer{ Device },
				Vector<ID3D12Resource*>{ allocator }});
			}

		Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

		return Success;
	}


	/************************************************************************************************/


    ID3D12PipelineState* CreateClearBufferPSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("Clear", "cs_6_0", R"(assets\shaders\ClearBuffer.hlsl)");

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
            RS->Library.ClearBuffer,
            computeShader
        };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

        FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

        return PSO;
    }


    /************************************************************************************************/


    RenderSystem::RenderSystem(iAllocator* IN_allocator, ThreadManager* IN_Threads) :
			Memory          { IN_allocator },
			Library         { IN_allocator },
			Queries         { IN_allocator, this },
			Textures        { IN_allocator },
			VertexBuffers   { IN_allocator },
			ConstantBuffers { IN_allocator, this },
			PipelineStates  { IN_allocator, this, IN_Threads },
			PendingBarriers { IN_allocator },
			StreamOutTable  { IN_allocator },
			ReadBackTable   { IN_allocator },
			threads         { *IN_Threads },
			Syncs           { IN_allocator, 64 },
			Contexts        { IN_allocator, 3 * (1 + IN_Threads->GetThreadCount()) },
			heaps           { pDevice, IN_allocator }{}


    RenderSystem::~RenderSystem() { Release(); }


    /************************************************************************************************/


	bool RenderSystem::Initiate(Graphics_Desc* in)
	{
		Vector<ID3D12DeviceChild*> ObjectsCreated(in->Memory);

		Memory			   = in->Memory;
		Settings.AAQuality = 0;
		Settings.AASamples = 1;
		UINT DeviceFlags   = 0;

		ID3D12Device9*		Device;
		ID3D12Debug5*		Debug;
		ID3D12DebugDevice1* DebugDevice;


#if USING(ENABLEDRED)
        ID3D12DeviceRemovedExtendedDataSettings1* dredSettings;
        if(auto HR = D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings)); FAILED(HR))
            FK_LOG_ERROR("Failed to enable Dred!");
        else
        {
            dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            dredSettings->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);

            FK_LOG_INFO("DRED enabled");
        }
#endif


        HRESULT HR;
        HR = D3D12GetDebugInterface(__uuidof(ID3D12Debug5), (void**)&Debug); FK_ASSERT(SUCCEEDED(HR));
#if USING( DEBUGGRAPHICS )
        Debug->EnableDebugLayer();
        //Debug->SetEnableSynchronizedCommandQueueValidation(true);
        Debug->SetEnableAutoName(true);

#if USING(DEBUGGPUVALIDATION)
        Debug->SetEnableGPUBasedValidation(true);
#endif
#else
        Debug = nullptr;
        DebugDevice = nullptr;
#endif	

        bool InitiateComplete = false;

		HR = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&Device));
		if(FAILED(HR))
		{
			FK_LOG_ERROR("Failed to create A DX12 Device!");

			// Trying again with a DX11 Feature Level
			HR = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device));
			if (FAILED(HR))
			{
				FK_LOG_ERROR("Failed to create A DX11 Device!");
				return false;
			}
		}

#if USING(ENABLEDRED)
        if (auto HR = Device->QueryInterface(IID_PPV_ARGS(&dred)); FAILED(HR))
            FK_LOG_ERROR("Failed to enable Dred!");
#endif

        D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
        Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));

        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
        Device->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));


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
            features.resourceHeapTier = AvailableFeatures::ResourceHeapTier::HeapTier1;
            break;
        case D3D12_RESOURCE_HEAP_TIER_2:
            features.resourceHeapTier = AvailableFeatures::ResourceHeapTier::HeapTier2;
            break;
        default:
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

#if USING(DEBUGGRAPHICS)
		HR =  Device->QueryInterface(__uuidof(ID3D12DebugDevice1), (void**)&DebugDevice);
#else
		DebugDevice = nullptr;
#endif
		
		{
			ID3D12Fence* NewFence = nullptr;
			HR = Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&NewFence));
			//FK_ASSERT(FAILED(HR), "FAILED TO CREATE FENCE!");
			SETDEBUGNAME(NewFence, "GRAPHICS FENCE");

			FK_LOG_9("GRAPHICS FENCE CREATED: %u", NewFence);

			Fence = NewFence;
			ObjectsCreated.push_back(NewFence);
		}
		
		D3D12_COMMAND_QUEUE_DESC CQD		= {};
		CQD.Flags							            = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
		CQD.Type								        = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;

		D3D12_COMMAND_QUEUE_DESC ComputeCQD = {};
		ComputeCQD.Flags								= D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
		ComputeCQD.Type									= D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE;

		IDXGIFactory5*				DXGIFactory         = nullptr;
		IDXGIAdapter4*				DXGIAdapter			= nullptr;
		

		HR = Device->CreateCommandQueue(&CQD,			IID_PPV_ARGS(&GraphicsQueue));		FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND QUEUE!");
		HR = Device->CreateCommandQueue(&ComputeCQD,	IID_PPV_ARGS(&ComputeQueue));		FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND QUEUE!");

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
		HR = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&DXGIFactory));
        DXGIFactory->EnumAdapterByLuid(Device->GetAdapterLuid(), IID_PPV_ARGS(&DXGIAdapter));

		FK_ASSERT(FAILED(HR), "FAILED TO CREATE DXGIFactory!"  );

		FINALLY
			if (!InitiateComplete)
				DXGIFactory->Release();
		FINALLYOVER

		{
			// Copy temp resources over
			pDevice					    = Device;
			pGIFactory				    = DXGIFactory;
			pDXGIAdapter			    = DXGIAdapter;
			pDebugDevice			    = DebugDevice;
			pDebug					    = Debug;
			BufferCount				    = 3;
			graphicsSubmissionCounter	= 0;
			DescriptorRTVSize		    = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			DescriptorDSVSize		    = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			DescriptorCBVSRVUAVSize     = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}


		heaps.Init(pDevice);
		copyEngine.Initiate(Device, (threads.GetThreadCount() + 1) * 1.5, ObjectsCreated, in->Memory);

		for (size_t I = 0; I < 3 * (1 + threads.GetThreadCount()); ++I)
			Contexts.emplace_back(this, Memory);

		InitiateComplete = true;
		
		Library.Initiate(this, in->TempMemory);
		ReadBackTable.Initiate(Device);

        FreeList_GraphicsQueue.Allocator    = in->Memory;
        FreeList_CopyQueue.Allocator        = in->Memory;
		DefaultTexture                      = _CreateDefaultTexture();

        RegisterPSOLoader(CLEARBUFFERPSO, { &Library.ClearBuffer, CreateClearBufferPSO });
        QueuePSOLoad(CLEARBUFFERPSO);

		SetDebugName(DefaultTexture, "Default Texture");

		return InitiateComplete;
	}


	/************************************************************************************************/


	void RenderSystem::Release()
	{
		if (!Memory)
			return;

		WaitforGPU();

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

		ConstantBuffers.Release();
		VertexBuffers.Release();
		Textures.Release();
		PipelineStates.ReleasePSOs();
		ReadBackTable.Release();
        Queries.Release();

		Library.Release();

		if(GraphicsQueue)	GraphicsQueue->Release();
		if(ComputeQueue)	ComputeQueue->Release();
		if(pGIFactory)		pGIFactory->Release();
		if(pDXGIAdapter)	pDXGIAdapter->Release();
		if(pDevice)			pDevice->Release();
		if(Fence)			Fence->Release();

#if USING(DEBUGGRAPHICS)
		// Prints Detailed Report
		pDebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
		pDebugDevice->Release();
		pDebug->Release();
#endif

		Memory = nullptr;
	}


	/************************************************************************************************/


	ID3D12PipelineState* RenderSystem::GetPSO(PSOHandle StateID)
	{
		return PipelineStates.GetPSO(StateID);
	}


	/************************************************************************************************/


	RootSignature const * const RenderSystem::GetPSORootSignature(PSOHandle handle) const
	{
		return PipelineStates.GetPSORootSig(handle);
	}


	/************************************************************************************************/


    void RenderSystem::BuildLibrary(PSOHandle State, const PipelineStateLibraryDesc desc)
    {

        FK_ASSERT(0);
        //CompileShader();
    }


    /************************************************************************************************/


	void RenderSystem::RegisterPSOLoader(PSOHandle State, PipelineStateDescription desc)
	{
		PipelineStates.RegisterPSOLoader(State, desc);
	}


	/************************************************************************************************/


	void RenderSystem::QueuePSOLoad(PSOHandle State)
	{
		FK_LOG_2("Reloading PSO!");

		PipelineStates.QueuePSOLoad(State, Memory);
	}


	/************************************************************************************************/


	size_t	RenderSystem::GetCurrentFrame()
	{
		return CurrentFrame;
	}


	/************************************************************************************************/


	size_t RenderSystem::GetCurrentCounter()
	{
		return graphicsSubmissionCounter;
	}


	/************************************************************************************************/

	
	void RenderSystem::_UpdateCounters()
	{
		Textures.UpdateLocks(threads, Fence->GetCompletedValue());

		ConstantBuffers.DecrementLocks();

		++CurrentFrame;
	}


	/************************************************************************************************/


	void RenderSystem::_UpdateSubResources(ResourceHandle handle, ID3D12Resource** resources, const size_t size)
	{
		Textures.ReplaceResources(handle, resources, size);
	}


	/************************************************************************************************/


	void RenderSystem::WaitforGPU()
	{
		GraphicsQueue->Signal(Fence, ++graphicsSubmissionCounter);

		const size_t completedValue	= Fence->GetCompletedValue();
		const size_t currentCounter = graphicsSubmissionCounter;

		if (currentCounter > completedValue)
		{
			const HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS); FK_ASSERT(eventHandle != 0);
			Fence->SetEventOnCompletion(currentCounter, eventHandle);
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}


	/************************************************************************************************/



	void RenderSystem::SetDebugName(ResourceHandle handle, const char* str)
	{
		Textures.SetDebugName(handle, str);
	}


	/************************************************************************************************/


	D3D12_GPU_VIRTUAL_ADDRESS RenderSystem::GetVertexBufferAddress(const VertexBufferHandle VB)
	{
		return VertexBuffers.GetAsset(VB)->GetGPUVirtualAddress();
	}


	/************************************************************************************************/


	size_t RenderSystem::GetVertexBufferSize(const VertexBufferHandle VB)
	{
		return VertexBuffers.GetBufferSize(VB);
	}


	/************************************************************************************************/


	void RenderSystem::MarkTextureUsed(ResourceHandle Handle)
	{
		Textures.MarkRTUsed(Handle);
	}


	/************************************************************************************************/


    const size_t RenderSystem::GetResourceSize(ConstantBufferHandle handle) const noexcept
    {
        return ConstantBuffers.GetBufferOffset(handle);
    }


    const size_t RenderSystem::GetResourceSize(ResourceHandle handle) const noexcept
    {
        return Textures.GetResourceSize(handle);
    }


    /************************************************************************************************/


	const size_t    RenderSystem::GetAllocationSize(ResourceHandle handle) const noexcept
	{
		auto resource               = GetDeviceResource(handle);
		D3D12_RESOURCE_DESC desc    = resource->GetDesc();
		auto resourceInfo           = pDevice->GetResourceAllocationInfo(0, 1, &desc);

		return resourceInfo.SizeInBytes;
	}


	const size_t    RenderSystem::GetAllocationSize(GPUResourceDesc desc) const noexcept
	{
        const D3D12_RESOURCE_DESC Resource_DESC = desc.GetD3D12ResourceDesc();
		return pDevice->GetResourceAllocationInfo(0, 1, &Resource_DESC).SizeInBytes;
	}


	/************************************************************************************************/


	const size_t	RenderSystem::GetTextureElementSize(ResourceHandle handle) const
	{
		auto Format = Textures.GetFormat(handle);
		return GetFormatElementSize(Format);
	}


	/************************************************************************************************/


	const uint2	RenderSystem::GetTextureWH(ResourceHandle handle) const
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


	void RenderSystem::UploadTexture(ResourceHandle handle, CopyContextHandle queue, byte* buffer, size_t bufferSize)
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

		_UpdateSubResourceByUploadQueue(this, queue, resource, &desc, DRS_Common);

        Textures.SetState(handle, DeviceResourceState::DRS_Common);
	}


	/************************************************************************************************/


	void RenderSystem::UploadTexture(ResourceHandle handle, CopyContextHandle queue, TextureBuffer* buffers, size_t resourceCount) // Uses Upload Queue
	{
		auto resource	= GetDeviceResource(handle);
		auto format		= GetTextureFormat(handle);

		SubResourceUpload_Desc desc;
		desc.buffers            = buffers;
		desc.subResourceCount   = resourceCount;
		desc.format             = format;

		_UpdateSubResourceByUploadQueue(this, queue, resource, &desc, DRS_Common);

        Textures.SetState(handle, DeviceResourceState::DRS_Common);
	}


	/************************************************************************************************/


	D3D12_GPU_VIRTUAL_ADDRESS RenderSystem::GetConstantBufferAddress(const ConstantBufferHandle CB)
	{
		// TODO: deal with Push Buffer Offsets
		return ConstantBuffers.GetDeviceResource(CB)->GetGPUVirtualAddress();
	}



    /************************************************************************************************/


    BLAS_PreBuildInfo RenderSystem::GetBLASPreBuildInfo(VertexBuffer& vertexBuffer)
    {
        auto& indexBuffer    = vertexBuffer.VertexBuffers[vertexBuffer.MD.IndexBuffer_Index];
        auto* positionBuffer = vertexBuffer.Find(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION);

        D3D12_RAYTRACING_GEOMETRY_DESC desc;
        desc.Type   = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        desc.Flags  = D3D12_RAYTRACING_GEOMETRY_FLAGS::D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
        desc.Triangles.IndexFormat  = DXGI_FORMAT_R32_UINT;
        desc.Triangles.IndexBuffer  = indexBuffer.GetDevicePointer();
        desc.Triangles.IndexCount   = indexBuffer.Size();

        desc.Triangles.VertexFormat                 = DXGI_FORMAT_R32G32B32_FLOAT;
        desc.Triangles.VertexBuffer.StartAddress    = positionBuffer->GetDevicePointer();
        desc.Triangles.VertexBuffer.StrideInBytes   = positionBuffer->BufferStride;
        desc.Triangles.VertexCount                  = positionBuffer->Size();


        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs;
        inputs.DescsLayout      = D3D12_ELEMENTS_LAYOUT::D3D12_ELEMENTS_LAYOUT_ARRAY;
        inputs.Type             = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        inputs.pGeometryDescs   = &desc;
        inputs.NumDescs         = 1u;
        inputs.Flags            = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
        pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

        return {
            .BLAS_byteSize          = info.ResultDataMaxSizeInBytes,
            .scratchPad_byteSize    = info.ScratchDataSizeInBytes,
            .update_byteSize        = info.UpdateScratchDataSizeInBytes
        };
    }


	/************************************************************************************************/


	size_t RenderSystem::GetTextureFrameGraphIndex(ResourceHandle Texture)
	{
		auto FrameID = GetCurrentFrame();
		return Textures.GetFrameGraphIndex(Texture, FrameID);
	}


	/************************************************************************************************/


	void RenderSystem::SetTextureFrameGraphIndex(ResourceHandle Texture, size_t Index)
	{
		auto FrameID = GetCurrentFrame();
		Textures.SetFrameGraphIndex(Texture, FrameID, Index);
	}


	/************************************************************************************************/


	DeviceHeapHandle  RenderSystem::CreateHeap(const size_t heapSize, const uint32_t flags)
	{
		return heaps.CreateHeap(heapSize, flags);
	}


	/************************************************************************************************/


	ConstantBufferHandle RenderSystem::CreateConstantBuffer(size_t BufferSize, bool GPUResident)
	{
		return ConstantBuffers.CreateConstantBuffer(BufferSize, GPUResident);
	}


	/************************************************************************************************/


	VertexBufferHandle RenderSystem::CreateVertexBuffer(size_t BufferSize, bool GPUResident)
	{
		return VertexBuffers.CreateVertexBuffer(BufferSize, GPUResident, this);
	}


	/************************************************************************************************/


	ResourceHandle RenderSystem::CreateDepthBuffer( const uint2 WH, const bool UseFloat, const size_t bufferCount)
	{
        auto resourceDesc           = GPUResourceDesc::DepthTarget(WH, UseFloat ? DeviceFormat::D32_FLOAT : DeviceFormat::D24_UNORM_S8_UINT);
        resourceDesc.bufferCount    = bufferCount;

		auto resource = CreateGPUResource(resourceDesc);
		SetDebugName(resource, "DepthBuffer");

		return resource;
	}


	/************************************************************************************************/


	ResourceHandle RenderSystem::CreateDepthBufferArray(
		const uint2                     WH,
		const bool                      UseFloat,
		const size_t                    arraySize,
		const bool                      buffered,
		const ResourceAllocationType    allocationType)
	{
		auto desc           = GPUResourceDesc::DepthTarget(WH, UseFloat ? DeviceFormat::D32_FLOAT : DeviceFormat::D24_UNORM_S8_UINT);
		desc.arraySize      = arraySize;
		desc.allocationType = allocationType;

		auto resource   = CreateGPUResource(desc);
		SetDebugName(resource, "DepthBufferArray");

		return resource;
	}


	/************************************************************************************************/


	ResourceHandle RenderSystem::CreateGPUResource(const GPUResourceDesc& desc)
	{
        ProfileFunction();

		if (desc.backBuffer)
		{
			return Textures.AddResource(desc, DRS_Present);
		}
		else if (desc.PreCreated)
		{
			return Textures.AddResource(desc, DRS_GenericRead);
		}
		else
		{
            size_t byteSize                   = desc.CalculateByteSize();
            D3D12_RESOURCE_DESC Resource_DESC = desc.GetD3D12ResourceDesc();

			D3D12_HEAP_PROPERTIES HEAP_Props	={};
			HEAP_Props.CPUPageProperty			= D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			HEAP_Props.Type						= D3D12_HEAP_TYPE_DEFAULT;
			HEAP_Props.MemoryPoolPreference		= D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
			HEAP_Props.CreationNodeMask			= 0;
			HEAP_Props.VisibleNodeMask			= 0;

			D3D12_CLEAR_VALUE CV;
			CV.Color[0] = desc.depthTarget ? 1.0f : 0.0f;
			CV.Color[1] = 0.0f;
			CV.Color[2] = 0.0f;
			CV.Color[3] = 0.0f;
			CV.Format	= TextureFormat2DXGIFormat(desc.format);
		
			auto flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES |
                (desc.unordered ? D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS : D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE);


			D3D12_CLEAR_VALUE* pCV = (desc.CVFlag | desc.renderTarget | desc.depthTarget) ? &CV : nullptr;


			D3D12_RESOURCE_STATES InitialState =
				desc.renderTarget       ? D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET :
				desc.depthTarget        ? D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE   :
                desc.rayTraceStructure  ? D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE : 
                D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;

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
                    ProfileFunctionLabeled(Tiled);

					Resource_DESC.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
                    
					HRESULT HR = pDevice->CreateReservedResource(
									&Resource_DESC,
									InitialState,
									pCV,
									IID_PPV_ARGS(&NewResource[itr]));

					CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE VIRTUAL MEMORY FOR TEXTURE"));
				}   break;
				case ResourceAllocationType::Committed:
				{
                    ProfileFunctionLabeled(Committed);

					HRESULT HR = pDevice->CreateCommittedResource(
									&HEAP_Props, 
									flags,
									&Resource_DESC, InitialState, pCV, IID_PPV_ARGS(&NewResource[itr]));

					CheckHR(HR, ASSERTONFAIL("FAILED TO COMMIT MEMORY FOR TEXTURE"));
				}   break;
				case ResourceAllocationType::Placed:
				{
                    ProfileFunctionLabeled(Placed);

					HRESULT HR = pDevice->CreatePlacedResource(
                        desc.placed.heap != InvalidHandle ? GetDeviceResource(desc.placed.heap) : desc.placed.customHeap,
						desc.placed.offset,
						&Resource_DESC,
						InitialState,
						pCV,
						IID_PPV_ARGS(&NewResource[itr]));

                    CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE PLACED RESOURCE"));

                    if (FAILED(HR))
                        _OnCrash();
				}   break;
				}
				FK_ASSERT(NewResource[itr], "Failed to Create Texture!");
				SETDEBUGNAME(NewResource[itr], __func__);
			}

			auto filledDesc = desc;

            auto initialState = filledDesc.InitialResourceState();

			filledDesc.resources    = NewResource;
            filledDesc.byteSize     = byteSize;

			return Textures.AddResource(filledDesc, initialState);
		}

		return InvalidHandle;
	}


    /************************************************************************************************/


    void RenderSystem::BackResource(ResourceHandle handle, const GPUResourceDesc& desc)
    {
        ProfileFunction();

        size_t byteSize                   = desc.CalculateByteSize();
        D3D12_RESOURCE_DESC Resource_DESC = desc.GetD3D12ResourceDesc();

		D3D12_HEAP_PROPERTIES HEAP_Props	={};
		HEAP_Props.CPUPageProperty			= D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type						= D3D12_HEAP_TYPE_DEFAULT;
		HEAP_Props.MemoryPoolPreference		= D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask			= 0;
		HEAP_Props.VisibleNodeMask			= 0;

		D3D12_CLEAR_VALUE CV;
		CV.Color[0] = desc.depthTarget ? 1.0f : 0.0f;
		CV.Color[1] = 0.0f;
		CV.Color[2] = 0.0f;
		CV.Color[3] = 0.0f;
		CV.Format	= TextureFormat2DXGIFormat(desc.format);
		
		auto flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES |
            (desc.unordered ? D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS : D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE);


		D3D12_CLEAR_VALUE* pCV = (desc.CVFlag | desc.renderTarget | desc.depthTarget) ? &CV : nullptr;


		D3D12_RESOURCE_STATES InitialState =
			desc.renderTarget       ? D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET :
			desc.depthTarget        ? D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE   :
            desc.rayTraceStructure  ? D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE : 
            D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;

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
                ProfileFunctionLabeled(Tiled);

				Resource_DESC.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
                    
				HRESULT HR = pDevice->CreateReservedResource(
								&Resource_DESC,
								InitialState,
								pCV,
								IID_PPV_ARGS(&NewResource[itr]));

				CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE VIRTUAL MEMORY FOR TEXTURE"));
			}   break;
			case ResourceAllocationType::Committed:
			{
                ProfileFunctionLabeled(Committed);

				HRESULT HR = pDevice->CreateCommittedResource(
								&HEAP_Props, 
								flags,
								&Resource_DESC, InitialState, pCV, IID_PPV_ARGS(&NewResource[itr]));

				CheckHR(HR, ASSERTONFAIL("FAILED TO COMMIT MEMORY FOR TEXTURE"));
			}   break;
			case ResourceAllocationType::Placed:
			{
                ProfileFunctionLabeled(Placed);

				HRESULT HR = pDevice->CreatePlacedResource(
                    desc.placed.heap != InvalidHandle ? GetDeviceResource(desc.placed.heap) : desc.placed.customHeap,
					desc.placed.offset,
					&Resource_DESC,
					InitialState,
					pCV,
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
        auto initialState   = filledDesc.InitialResourceState();


		filledDesc.resources    = NewResource;
        filledDesc.byteSize     = byteSize;

		Textures.SetResource(handle, filledDesc, initialState);
    }


    /************************************************************************************************/


    ResourceHandle RenderSystem::CreateGPUResourceHandle()
    {
        return Textures.GetFreeHandle();
    }


	/************************************************************************************************/


	QueryHandle	RenderSystem::CreateOcclusionBuffer(size_t Counts)
	{
		return Queries.CreateQueryBuffer(Counts, QueryType::OcclusionQuery);
	}


	/************************************************************************************************/


    ResourceHandle RenderSystem::CreateUAVBufferResource(size_t resourceSize, bool tripleBuffer)
	{
        auto desc = GPUResourceDesc::UAVResource(resourceSize);
        desc.bufferCount = tripleBuffer ? 3 : 1;

        UAVResourceLayout layout;
        layout.elementCount = resourceSize / sizeof(uint32_t);// initial layout assume a uint buffer
        layout.format       = DXGI_FORMAT_UNKNOWN;
        layout.stride       = sizeof(uint32_t);

        auto handle = CreateGPUResource(desc);
        Textures.SetExtra(handle, layout);
        SetDebugName(handle, "CreateUAVBuffer");

        return handle;
	}


	/************************************************************************************************/


    ResourceHandle RenderSystem::CreateUAVTextureResource(const uint2 WH, const DeviceFormat format, const bool renderTarget)
	{
        auto desc           = GPUResourceDesc::UAVTexture(WH, format, renderTarget);
        desc.bufferCount    = 3;

        auto UAVresource = CreateGPUResource(desc);
        SetDebugName(UAVresource, "CreateUAVBuffer");

        return UAVresource;
	}


	/************************************************************************************************/


	SOResourceHandle RenderSystem::CreateStreamOutResource(size_t resourceSize, bool tripleBuffered)
	{
		D3D12_RESOURCE_DESC Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(resourceSize);
		Resource_DESC.Width              = resourceSize;
		Resource_DESC.Format             = DXGI_FORMAT_UNKNOWN;
		Resource_DESC.Flags              = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		D3D12_RESOURCE_DESC Counter_DESC = CD3DX12_RESOURCE_DESC::Buffer(resourceSize);
		Counter_DESC.Width              = 512;
		Counter_DESC.Format             = DXGI_FORMAT_UNKNOWN;
		Counter_DESC.Flags              = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		D3D12_HEAP_PROPERTIES HEAP_Props = {};
		HEAP_Props.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type                  = D3D12_HEAP_TYPE_DEFAULT;
		HEAP_Props.MemoryPoolPreference  = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;

		static_vector<ID3D12Resource*>	resources;
		static_vector<ID3D12Resource*>	counters;
		for (size_t I = 0; I < (tripleBuffered ? BufferCount : 1); ++I)
		{
			ID3D12Resource* Resource	= nullptr;
			ID3D12Resource* Counter		= nullptr;

			HRESULT HR;

			HR = pDevice->CreateCommittedResource(
								&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
								&Resource_DESC, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_STREAM_OUT, nullptr,
								IID_PPV_ARGS(&Resource));
			CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE STREAMOUT RESOURCE!"));

			HR = pDevice->CreateCommittedResource(
								&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
								&Resource_DESC, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_STREAM_OUT, nullptr,
								IID_PPV_ARGS(&Counter));

			CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE STREAMOUT RESOURCE!"));
			resources.push_back(Resource);
			counters.push_back(Counter);

			SETDEBUGNAME(Resource, "StreamOutResource" );
			SETDEBUGNAME(Counter, "StreamOutCounter" );
		}

		return StreamOutTable.AddResource(resources, counters, resourceSize, DeviceResourceState::DRS_STREAMOUT);
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


	IndirectLayout RenderSystem::CreateIndirectLayout(static_vector<IndirectDrawDescription> entries, iAllocator* allocator, RootSignature* rootSignature)
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
				    entryStride += sizeof(uint32_t) * 4; // uses 4 8byte values
			    }	break;
                case ILE_DispatchCall:
                {
                    D3D12_INDIRECT_ARGUMENT_DESC desc = {};
                    desc.Type   = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

                    signatureEntries.push_back(desc);
                    layout.push_back(ILE_DispatchCall);
                    entryStride += sizeof(uint4); // uses 4 8byte values
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

		D3D12_COMMAND_SIGNATURE_DESC desc;
		desc.ByteStride			= entryStride;
		desc.NumArgumentDescs	= entries.size();
		desc.pArgumentDescs		= signatureEntries.begin();
		desc.NodeMask			= 0;

		auto HR = pDevice->CreateCommandSignature(
			&desc,
            rootSignature ? rootSignature->Get_ptr() : nullptr,
			IID_PPV_ARGS(&signature));

		CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE CONSTANT BUFFER"));

		return { signature, entryStride, std::move(layout) };
	}


	/************************************************************************************************/


    SyncPoint RenderSystem::UpdateTileMappings(ResourceHandle* begin, ResourceHandle* end, iAllocator* allocator)
	{
		FK_LOG_9("Updating tile mappings for frame: %u", GetCurrentFrame());

		auto itr = begin;
		while(itr < end)
		{
            FK_ASSERT(*itr >= Textures.Handles.size(), "Invalid Handle Detected");

			auto& mappings      = Textures._GetTileMappings(*itr);
			auto deviceResource = GetDeviceResource(*itr);
			const auto mipCount = Textures.GetMIPCount(*itr);

			Vector<D3D12_TILED_RESOURCE_COORDINATE> coordinates { allocator };
			Vector<D3D12_TILE_REGION_SIZE>          regionSizes { allocator };
			Vector<D3D12_TILE_RANGE_FLAGS>          flags       { allocator };
			Vector<UINT>                            offsets     { allocator };
			Vector<UINT>                            tileRanges  { allocator };
			DeviceHeapHandle                        heap = InvalidHandle;


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
					coordinate.Subresource  = mapping.tileID.GetMipLevel();
					coordinate.X            = mapping.tileID.GetTileX();
					coordinate.Y            = mapping.tileID.GetTileY();
					coordinate.Z            = 0;

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
                    coordinates.size(),
                    coordinates.data(),
                    nullptr,
                    nullptr,
                    coordinates.size(),
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

            uint32_t I = std::distance(mappings.begin(), nullEnd);
			while(I < mappings.size())
			{
				heap = mappings[I].heap;

				for (;mappings[I].heap == heap && I < mappings.size(); I++)
				{
					auto& mapping = mappings[I];

					mapping.state = TileMapState::InUse;

					const auto flag = D3D12_TILE_RANGE_FLAG_NONE;

					D3D12_TILED_RESOURCE_COORDINATE coordinate;
					coordinate.Subresource  = mapping.tileID.GetMipLevel();
					coordinate.X            = mapping.tileID.GetTileX();
					coordinate.Y            = mapping.tileID.GetTileY();
					coordinate.Z            = 0;

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
					    regionSize.Depth    = 1;
					    regionSize.Height   = 1;
					    regionSize.Width    = 1;

					    regionSize.NumTiles = 1;
					    regionSize.UseBox = false;

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
                        D3D12_TILE_RANGE_FLAGS rangeFlags = D3D12_TILE_RANGE_FLAG_NULL;
                        copyEngine.copyQueue->UpdateTileMappings(deviceResource, 1, NULL, NULL, NULL, 1, &rangeFlags, NULL, NULL, D3D12_TILE_MAPPING_FLAG_NONE);
                    }
                    else
                    {
                        copyEngine.copyQueue->UpdateTileMappings(
                            deviceResource,
                            coordinates.size(),
                            coordinates.data(),
                            regionSizes.data(),
                            GetDeviceResource(heap),
                            coordinates.size(),
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

			itr++;
		}

		FK_LOG_9("Completed file mapping update");

        /*/
        const auto counter = ++FenceCounter;
        if (auto HR = GraphicsQueue->Signal(Fence, counter); FAILED(HR))
            FK_LOG_ERROR("Failed to Signal");

        return { counter, Fence };
        */

        return {};
	}


	/************************************************************************************************/


	void RenderSystem::UpdateTextureTileMappings(const ResourceHandle handle, const TileMapList& tileMaps)
	{
		Textures.UpdateTileMappings(handle, tileMaps.begin(), tileMaps.end());
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


	void RenderSystem::SetObjectState(SOResourceHandle handle, DeviceResourceState state)
	{
		StreamOutTable.SetResourceState(handle, state);
	}


	/************************************************************************************************/


	void RenderSystem::SetObjectState(ResourceHandle handle, DeviceResourceState state)
	{
		Textures.SetState(handle, state);
	}


	/************************************************************************************************/


	DeviceResourceState RenderSystem::GetObjectState(const QueryHandle handle) const
	{
		return Queries.GetAssetState(handle);
	}


	/************************************************************************************************/


	DeviceResourceState RenderSystem::GetObjectState(const SOResourceHandle handle) const
	{
		return StreamOutTable.GetAssetState(handle);
	}


	/************************************************************************************************/


	DeviceResourceState RenderSystem::GetObjectState(const ResourceHandle handle) const
	{
		return Textures.GetState(handle);
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


	void RenderSystem::ResetQuery(QueryHandle handle)
	{
		Queries.LockUntil(handle, CurrentFrame);
	}


	/************************************************************************************************/


	void RenderSystem::ReleaseCB(ConstantBufferHandle Handle)
	{
		ConstantBuffers.ReleaseBuffer(Handle);
	}


	/************************************************************************************************/


	void RenderSystem::ReleaseVB(VertexBufferHandle Handle)
	{
		VertexBuffers.ReleaseVertexBuffer(Handle);
	}


	/************************************************************************************************/


	void RenderSystem::ReleaseResource(ResourceHandle Handle)
	{
		Textures.ReleaseTexture(Handle, graphicsSubmissionCounter);
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


	void Push_DelayedRelease(RenderSystem* RS, ID3D12Resource* Res)
	{
		RS->FreeList_GraphicsQueue.push_back({ Res, RS->graphicsSubmissionCounter });
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
            auto completed = RS->Fence->GetCompletedValue();
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
		switch (F)
		{
        case FlexKit::DeviceFormat::R16_FLOAT:
            return DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT;
		case FlexKit::DeviceFormat::R16_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
		case FlexKit::DeviceFormat::R16G16_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R16G16_UINT;
		case FlexKit::DeviceFormat::R32_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
		case FlexKit::DeviceFormat::R32G32_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R32G32_UINT;
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


	constexpr DeviceFormat	DXGIFormat2TextureFormat(DXGI_FORMAT F) noexcept
	{
		switch (F)
		{
		case DXGI_FORMAT::DXGI_FORMAT_R16G16_UINT:
			return FlexKit::DeviceFormat::R16G16_UINT;
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


	FLEXKITAPI void _UpdateSubResourceByUploadQueue(RenderSystem* RS, CopyContextHandle uploadHandle, ID3D12Resource* destinationResource, SubResourceUpload_Desc* Desc, DeviceResourceState EndState)
	{
		auto& copyCtx   = RS->_GetCopyContext(uploadHandle);

		for (size_t I = 0; I < Desc->subResourceCount; ++I)
		{
			const auto region       = copyCtx.Reserve(Desc->buffers[I].Size, 512);

			memcpy(
				(char*)region.buffer,
				(char*)Desc->buffers[I].Buffer,
				Desc->buffers[I].Size);

			copyCtx.CopyTextureRegion(
				destinationResource,
				I,
				{ 0, 0, 0 },
				region,
				Desc->buffers[I].WH,
				Desc->format);
		}

		copyCtx.Barrier(
			destinationResource,
			DRS_CopyDest,
            EndState);
	}


	/************************************************************************************************/


	void RenderSystem::UpdateResourceByUploadQueue(ID3D12Resource* Dest, CopyContextHandle uploadQueue, void* Data, size_t Size, size_t ByteSize, DeviceResourceState EndState)
	{
		FK_ASSERT(Data);
		FK_ASSERT(Dest);

		auto& copyCtx = _GetCopyContext(uploadQueue);

		const auto reservedSpace = copyCtx.Reserve(Size);

		memcpy(reservedSpace.buffer, Data, Size);

		copyCtx.CopyBuffer(Dest, 0, reservedSpace);

        copyCtx.Barrier(
            Dest,
            DRS_CopyDest,
            EndState);
	}


    /************************************************************************************************/

    struct IncludeHandler : public IDxcIncludeHandler
    {
        HRESULT STDMETHODCALLTYPE LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource) override
        {
            char fileStr[256];
            auto fileLength = wcstombs(fileStr, pFilename, 256);

            std::filesystem::path file{ fileStr };
            auto newFilePath = includePath.string() + R"(\)" + file.filename().string();

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


    Shader  RenderSystem::LoadShader(const char* entryPoint, const char* profile, const char* file, bool enable16Bit)
    {
        std::filesystem::path filePath{ file };
        auto parentPath = filePath.parent_path();

        wchar_t entryPointW[64];
        wchar_t fileW[256];
        wchar_t filenameW[256];
        wchar_t profileW[64];

        size_t fileWLength = 0;
        if(entryPoint != nullptr)
            mbstowcs(entryPointW, entryPoint, 64);

        mbstowcs(profileW, profile, 64);
        mbstowcs(fileW, file, 256);
        mbstowcs(filenameW, filePath.filename().string().c_str(), 256);


        IDxcBlobEncoding* blob;
        auto HR1 = hlslLibrary->CreateBlobFromFile(fileW, nullptr, &blob);


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

        if(enable16Bit)
            arguments.push_back(L"-enable-16bit-types");
#else

        if (enable16Bit)
            arguments.push_back(L"-enable-16bit-types");
#endif

        IDxcOperationResult* result = nullptr;
        auto HR2 = hlslCompiler->Compile(blob, filenameW, entryPoint != nullptr ? entryPointW : nullptr, profileW, arguments.data(), arguments.size(), nullptr, 0, &includeHandler, &result);

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
                FK_LOG_ERROR("%s\nFailed to Compile Shader\nEntryPoint: %s\nFile: %s\nPress Enter to try again\n", errorString, entryPoint, file);

                errors->Release();

                char str[100];
                std::cin >> str;

                HR1 = hlslLibrary->CreateBlobFromFile(fileW, nullptr, &blob);
                HR2 = hlslCompiler->Compile(blob, filenameW, entryPointW, profileW, arguments.data(), arguments.size(), nullptr, 0, &includeHandler, &result);

                result->GetStatus(&status);
            }


            auto HR = result->GetResult(&byteCodeBlob);

            wchar_t* text = (wchar_t*)byteCodeBlob->GetBufferPointer();

            Shader out{ byteCodeBlob };
            byteCodeBlob->Release();
            result->Release();

            return std::move(out);
        }
    }

	
	/************************************************************************************************/


	// If uploadQueue is InvalidHandle, pushes temporaries to a different free list
	UploadSegment ReserveUploadBuffer(
		RenderSystem&	    renderSystem,
		const size_t	    uploadSize,
		CopyContextHandle   uploadQueue)
	{
		auto reserve = renderSystem._GetCopyContext(uploadQueue).Reserve(uploadSize, 512);


		return {
			reserve.offset,
			reserve.reserveSize,
			reserve.buffer,
			reserve.resource
		};
	}


	/************************************************************************************************/


	void MoveBuffer2UploadBuffer(const UploadSegment& data, const byte* source, const size_t uploadSize)
	{
		memcpy(data.buffer, source, data.uploadSize > uploadSize ? uploadSize : data.uploadSize);
	}


	/************************************************************************************************/


	void UploadTextureSet(RenderSystem* RS, TextureSet* TS, iAllocator* Memory)
	{
		for (size_t I = 0; I < 16; ++I)
		{
			if (TS->TextureGuids[I]) {
				TS->Loaded[I]	= true;
				TS->Textures[I] = LoadDDSTextureFromFile(TS->TextureLocations[I].Directory, RS, RS->GetImmediateUploadQueue(), Memory);
			}
		}
	}


	/************************************************************************************************/


	void ReleaseTextureSet(TextureSet* TS, iAllocator* Memory)
	{
		for (size_t I = 0; I < 16; ++I)
		{
			if (TS->TextureGuids[I]) {
				TS->Loaded[I] = false;
				FK_ASSERT(0);
				//TS->Textures[I]->Release();
			}
		}
		Memory->free(TS);
	}


	/************************************************************************************************/


	void CreateVertexBuffer(RenderSystem* RS, CopyContextHandle handle, VertexBufferView** Buffers, size_t BufferCount, VertexBuffer& DVB_Out)
	{
		// TODO: Add Buffer Layout Structure for more complex Buffer Layouts
		// TODO: ATM only is able to make Buffers of a single Value
		InputDescription Input_Desc;

		DVB_Out.VertexBuffers.SetFull();

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

                Barrier b = {
                    ._ptr           = NewBuffer,
                    .type           = Barrier::ResourceType::PTR,
                    .beforeState    = DRS_Common,
                    .afterState     = DRS_INDEXBUFFER,
                };

                RS->_InsertBarrier(b);

                cctx.Barrier(NewBuffer, DRS_Common, DRS_CopyDest);

				RS->UpdateResourceByUploadQueue(
					NewBuffer, 
					handle,
					Buffers[itr]->GetBuffer(),
					Buffers[itr]->GetBufferSizeRaw(), 1,
                    DRS_Common);

				SETDEBUGNAME(NewBuffer, "INDEXBUFFER");

				DVB_Out.VertexBuffers[itr].Buffer				= NewBuffer;
				DVB_Out.VertexBuffers[itr].BufferSizeInBytes	= Buffers[itr]->GetBufferSizeRaw();
				DVB_Out.VertexBuffers[itr].BufferStride			= Buffers[itr]->GetElementSize();
				DVB_Out.VertexBuffers[itr].Type					= Buffers[itr]->GetBufferType();
				DVB_Out.MD.IndexBuffer_Index					= itr;
				DVB_Out.MD.InputElementCount                    = Buffers[itr]->GetBufferSize();
			}
			else if (Buffers[itr] && Buffers[itr]->GetBufferSize())
			{
				ID3D12Resource* NewBuffer = nullptr;
				// Create the Vertex Buffer
				FK_ASSERT(Buffers[itr]->GetBufferSizeRaw());// ERROR BUFFER EMPTY;
				Resource_DESC.Width	= Buffers[itr]->GetBufferSizeRaw();

				HRESULT HR = RS->pDevice->CreateCommittedResource(&HEAP_Props, 
									D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &Resource_DESC, 
									D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON,
									nullptr, IID_PPV_ARGS(&NewBuffer));

				if (FAILED(HR))
				{// TODO!
					FK_ASSERT(0);
				}

				switch (Buffers[itr]->GetBufferType())
				{
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION:
					{SETDEBUGNAME(NewBuffer, "VERTEXBUFFER");break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL:
					{SETDEBUGNAME(NewBuffer, "NORMAL BUFFER"); break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT:
					{SETDEBUGNAME(NewBuffer, "TANGET BUFFER"); break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_COLOR:
					{SETDEBUGNAME(NewBuffer, "COLOUR BUFFER"); break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV:
					{SETDEBUGNAME(NewBuffer, "TEXCOORD BUFFER"); break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1:
					{SETDEBUGNAME(NewBuffer, "AnimationWeights"); break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2:
					{SETDEBUGNAME(NewBuffer, "AnimationIndices"); break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_PACKED:
					{SETDEBUGNAME(NewBuffer, "PACKED_BUFFER");break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ERROR:
				default:
					{SETDEBUGNAME(NewBuffer, "VERTEXBUFFER_TYPE_ERROR"); break; }
					break;
		        }

                Barrier b = {
                    ._ptr           = NewBuffer,
                    .type           = Barrier::ResourceType::PTR,
                    .beforeState    = DRS_Common,
                    .afterState     = DRS_VERTEXBUFFER,
                };

                RS->_InsertBarrier(b);

                cctx.Barrier(NewBuffer, DRS_Common, DRS_CopyDest);

			    RS->UpdateResourceByUploadQueue(
				    NewBuffer,
				    handle,
				    Buffers[itr]->GetBuffer(),
				    Buffers[itr]->GetBufferSizeRaw(), 1, 
                    DRS_Common);

			    DVB_Out.VertexBuffers[itr].Buffer				= NewBuffer;
			    DVB_Out.VertexBuffers[itr].BufferStride			= Buffers[itr]->GetElementSize();
			    DVB_Out.VertexBuffers[itr].BufferSizeInBytes	= Buffers[itr]->GetBufferSizeRaw();
			    DVB_Out.VertexBuffers[itr].Type					= Buffers[itr]->GetBufferType();
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
		Handles[handle]     = UserBuffers.size();

		UserBuffers.push_back(UserVertexBuffer{
			0,
			{ Buffer[0], Buffer[1], Buffer[2] },
			BufferSize,
			0, 
			_Map(Buffer[0]),
			Buffers[Buffer[0]].Resource,
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

			Buffers[userBuffer.GetCurrentBuffer()].lockCounter = 3;

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
		if(UserBuffer.WrittenTo)
		{	// UnMap Current Buffer
			auto  ResourceIdx = UserBuffer.GetCurrentBuffer();
			D3D12_RANGE Range = { 0, UserBuffer.Offset };
			Buffers[ResourceIdx].Resource->Unmap(0, &Range);
		}
		
		UserBuffer.WrittenTo = false;
		UserBuffer.IncrementCurrentBuffer();

		{	// Map Current Buffer
			auto	ResourceIdx		= UserBuffer.GetCurrentBuffer();
			void*	MappedPtr		= nullptr;

			D3D12_RANGE Range{ 0, 0 };
			Buffers[ResourceIdx].Resource->Map(0, &Range, &MappedPtr);

			UserBuffer.Offset		= 0;
			UserBuffer.MappedPtr	= (char*)MappedPtr;
			UserBuffer.Resource		= Buffers[ResourceIdx].Resource;
		}
	}


	/************************************************************************************************/


	ID3D12Resource* VertexBufferStateTable::GetAsset(VertexBufferHandle Handle)
	{
		return Buffers[UserBuffers[Handles[Handle]].GetCurrentBuffer()].Resource;
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


	VertexBufferStateTable::SubAllocation VertexBufferStateTable::Reserve(VertexBufferHandle Handle, size_t size) noexcept
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


	bool VertexBufferStateTable::CurrentlyAvailable(VertexBufferHandle Handle, size_t CurrentFrame) const
	{
		auto UserIdx	= Handles[Handle];
		auto BufferIdx	= UserBuffers[UserIdx].GetCurrentBuffer();

		return Buffers[Handle].lockCounter == 0;
	}


	/************************************************************************************************/


	char* VertexBufferStateTable::_Map(VBufferHandle handle)
	{
		void* ptr;
		Buffers[handle].Resource->Map(0, nullptr, &ptr);

		return (char*)ptr;
	}


	void VertexBufferStateTable::_UnMap(VBufferHandle handle, size_t range)
	{
		D3D12_RANGE writtenRange;
		writtenRange.Begin  = 0;
		writtenRange.End    = range;

		Buffers[handle].Resource->Unmap(0, &writtenRange);
	}


	/************************************************************************************************/


	void VertexBufferStateTable::Release()
	{
		for (auto& B : Buffers) {
			if(B.Resource)
				B.Resource->Release();
			B.Resource = nullptr;
		}


		Buffers.Release();
		UserBuffers.Release();
		FreeBuffers.Release();
		Handles.Release();
	}


	/************************************************************************************************/


	void VertexBufferStateTable::ReleaseVertexBuffer(VertexBufferHandle handle)
	{
		auto userIdx		= Handles[handle];
		auto& userEntry     = UserBuffers[userIdx];

		std::scoped_lock(criticalSection);

		for (const auto ResourceIdx : userEntry.Buffers)
			if(ResourceIdx != INVALIDHANDLE)
				FreeBuffers.push_back({ 3, ResourceIdx});
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


	void VertexBufferStateTable::ReleaseFree()
	{
		std::scoped_lock(criticalSection);

		for (auto freeBuffer : FreeBuffers)
		{
			if (Buffers[freeBuffer.BufferIdx].lockCounter <= 0)
				Buffers[freeBuffer.BufferIdx].Resource->Release();

			Buffers[freeBuffer.BufferIdx].lockCounter--;
		}

		std::sort(FreeBuffers.begin(), FreeBuffers.end());

		FreeBuffers.erase(
			std::remove_if(
				FreeBuffers.begin(),
				FreeBuffers.end(),
				[&](auto buffer)-> bool
				{
					return Buffers[buffer].lockCounter <= 0;
				}),
			FreeBuffers.end());
	}


	/************************************************************************************************/


	QueryHandle QueryTable::CreateQueryBuffer(size_t count, QueryType type)
	{
		D3D12_QUERY_HEAP_DESC desc;
		desc.Count			= count;
		desc.NodeMask		= 0;

		ResourceEntry newResEntry = { 0 };

		switch (type)
		{
		case QueryType::OcclusionQuery:
			desc.Type			= D3D12_QUERY_HEAP_TYPE_OCCLUSION;
			newResEntry.type	= D3D12_QUERY_TYPE_OCCLUSION;
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
		
		heapDesc.Count	= count;
		heapDesc.Type	= static_cast<D3D12_QUERY_HEAP_TYPE>(D3D12_QUERY_HEAP_TYPE_SO_STATISTICS);

		ResourceEntry newResEntry	= { 0 };
		newResEntry.currentResource = 0;
		newResEntry.type			= static_cast<D3D12_QUERY_TYPE>(D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0 + SOIndex);

		for (size_t itr = 0; itr < 3; ++itr)
		{
			newResEntry.resourceState[itr] = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
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


	void TextureStateTable::Release()
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


    ResourceHandle TextureStateTable::GetFreeHandle()
    {
        std::scoped_lock lock{ m };

        auto newHandle = Handles.GetNewHandle();
        Handles[newHandle] = -1;

        return newHandle;
    }


    /************************************************************************************************/


	ResourceHandle TextureStateTable::AddResource(const GPUResourceDesc& Desc, const DeviceResourceState InitialState)
	{
        std::scoped_lock lock{m};

		auto Handle		 = Handles.GetNewHandle();

		ResourceEntry NewEntry = 
		{	
			Desc.bufferCount,
			0,
			{ nullptr, nullptr, nullptr },
			{ 0, 0, 0 },
			{ InitialState, InitialState , InitialState },
			TextureFormat2DXGIFormat(Desc.format),
			Desc.MipLevels,
			Desc.WH,
			Handle,
            { { Desc.placed.offset, 0 }, {}, {} }
		};

        for (size_t I = 0; I < Desc.bufferCount; ++I) {
            NewEntry.Resources[I]           = Desc.resources[I];

#if USING(AFTERMATH)
            GFSDK_Aftermath_ResourceHandle  aftermathResource;
            GFSDK_Aftermath_DX12_RegisterResource(Desc.resources[I], &aftermathResource);
            NewEntry.aftermathResource[I]   = aftermathResource;
#endif
        }

		UserEntry Entry;
		Entry.ResourceIdx		= Resources.push_back(NewEntry);
        Entry.resourceSize      = Desc.byteSize;
		Entry.FrameGraphIndex	= -1;
		Entry.FGI_FrameStamp    = -1;
		Entry.Handle            = Handle;
		Entry.Format			= NewEntry.Format;
		Entry.Flags             = Desc.backBuffer ? TF_BackBuffer : 0;
		Entry.dimension         = Desc.Dimensions;
		Entry.arraySize         = Desc.arraySize;
		Entry.tileMappings      = { allocator };

        if (Desc.Dimensions == TextureDimension::Buffer)
        {
            UAVResourceLayout layout;
            layout.format       = NewEntry.Format;
            layout.elementCount = Desc.byteSize / 4;
            layout.stride       = 4;
            Entry.extra         = layout;
        }

		Handles[Handle]         = UserEntries.push_back(Entry);

		if (Desc.buffered && Desc.bufferCount > 1)
			BufferedResources.push_back(Handle);

		return Handle;
	}


    /************************************************************************************************/


    void TextureStateTable::SetResource(ResourceHandle handle, const GPUResourceDesc& Desc, const DeviceResourceState InitialState)
    {
        std::scoped_lock lock{ m };

        ResourceEntry NewEntry = 
		{	
			Desc.bufferCount,
			0,
			{ nullptr, nullptr, nullptr },
			{ 0, 0, 0 },
			{ InitialState, InitialState , InitialState },
			TextureFormat2DXGIFormat(Desc.format),
			Desc.MipLevels,
			Desc.WH,
			handle,
            { { Desc.placed.offset, 0 }, {}, {} }
		};

        for (size_t I = 0; I < Desc.bufferCount; ++I) {
            NewEntry.Resources[I]           = Desc.resources[I];

#if USING(AFTERMATH)
            GFSDK_Aftermath_ResourceHandle  aftermathResource;
            GFSDK_Aftermath_DX12_RegisterResource(Desc.resources[I], &aftermathResource);
            NewEntry.aftermathResource[I]   = aftermathResource;
#endif
        }

		UserEntry Entry;
		Entry.ResourceIdx		= Resources.push_back(NewEntry);
        Entry.resourceSize      = Desc.byteSize;
		Entry.FrameGraphIndex	= -1;
		Entry.FGI_FrameStamp    = -1;
		Entry.Handle            = handle;
		Entry.Format			= NewEntry.Format;
		Entry.Flags             = Desc.backBuffer ? TF_BackBuffer : 0;
		Entry.dimension         = Desc.Dimensions;
		Entry.arraySize         = Desc.arraySize;
		Entry.tileMappings      = { allocator };

        if (Desc.Dimensions == TextureDimension::Buffer)
        {
            UAVResourceLayout layout;
            layout.format       = NewEntry.Format;
            layout.elementCount = Desc.byteSize / 4;
            layout.stride       = 4;
            Entry.extra         = layout;
        }

		Handles[handle]         = UserEntries.push_back(Entry);

		if (Desc.buffered && Desc.bufferCount > 1)
			BufferedResources.push_back(handle);
    }


	/************************************************************************************************/


	void TextureStateTable::ReleaseTexture(ResourceHandle handle, const int64_t idx)
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
                delayRelease.push_back({ res, idx });
        }

#if USING(AFTERMATH)
        for (auto res : resource.aftermathResource)
            if(res)
                GFSDK_Aftermath_DX12_UnregisterResource(res);
#endif

		const auto TempHandle	= UserEntries.back().Handle;
		UserEntry			    = UserEntries.back();
		Handles[TempHandle]     = UserIdx;

		const auto temp         = Resources[UserEntry.ResourceIdx];
		Resources[ResIdx]       = Resources.back();
		UserEntries[Handles[resource.owner]].ResourceIdx = ResIdx;

		UserEntries.pop_back();
		Resources.pop_back();

		Handles.RemoveHandle(handle);
	}


	/************************************************************************************************/


	void TextureStateTable::_ReleaseTextureForceRelease(ResourceHandle handle)
	{
        FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto  UserIdx		= Handles[handle];
		auto& UserEntry		= UserEntries[UserIdx];
		const auto ResIdx	= UserEntry.ResourceIdx;
		auto& resource		= Resources[ResIdx];

		for (auto res : resource.Resources)
			res->Release();

		const auto TempHandle	= UserEntries.back().Handle;
		UserEntry			    = UserEntries.back();
		Handles[TempHandle]     = UserIdx;

		const auto temp         = Resources[UserEntry.ResourceIdx];
		Resources[ResIdx]       = Resources.back();
		UserEntries[Handles[resource.owner]].ResourceIdx = ResIdx;

		UserEntries.pop_back();
		Resources.pop_back();

		Handles.RemoveHandle(handle);
	}


	/************************************************************************************************/


	Texture2D TextureStateTable::operator[](ResourceHandle handle)
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


	void TextureStateTable::SetState(ResourceHandle handle, DeviceResourceState State)
	{
        FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		auto ResIdx  = UserEntries[UserIdx].ResourceIdx;

		Resources[ResIdx].SetState(State);
	}


    /************************************************************************************************/


    void TextureStateTable::SetDebug(ResourceHandle handle, const char* string)
    {
        FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

        auto UserIdx                    = Handles[handle];
        UserEntries[UserIdx].userString = string;
    }


	/************************************************************************************************/


	void TextureStateTable::SetBufferedIdx(ResourceHandle handle, uint32_t idx)
	{
        FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		auto Residx  = UserEntries[UserIdx].ResourceIdx;

		Resources[Residx].CurrentResource = idx;
	}


	/************************************************************************************************/


	void TextureStateTable::SetDebugName(ResourceHandle handle, const char* str)
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


	size_t TextureStateTable::GetFrameGraphIndex(ResourceHandle handle, size_t FrameID) const
	{
        FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx	= Handles[handle];
		auto FrameStamp	= UserEntries[UserIdx].FGI_FrameStamp;

		if (FrameStamp < int(FrameID))
			return INVALIDHANDLE;

		return UserEntries[UserIdx].FrameGraphIndex;
	}


	/************************************************************************************************/


	void TextureStateTable::SetFrameGraphIndex(ResourceHandle handle, size_t FrameID, size_t Index)
	{
        FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		UserEntries[UserIdx].FGI_FrameStamp		= FrameID;
		UserEntries[UserIdx].FrameGraphIndex	= Index;
	}


	/************************************************************************************************/


	DXGI_FORMAT TextureStateTable::GetFormat(ResourceHandle handle) const
	{
        FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		return UserEntries[UserIdx].Format;
	}


	/************************************************************************************************/


	TextureDimension TextureStateTable::GetDimension(ResourceHandle handle) const
	{
        FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		return UserEntries[UserIdx].dimension;
	}


	/************************************************************************************************/


	size_t TextureStateTable::GetArraySize(ResourceHandle handle) const
	{
        FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		return UserEntries[UserIdx].arraySize;
	}


	/************************************************************************************************/


    uint8_t TextureStateTable::GetMIPCount(ResourceHandle handle) const
	{
        FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		return Resources[UserEntries[UserIdx].ResourceIdx].mipCount;
	}


    /************************************************************************************************/


    void TextureStateTable::SetExtra(ResourceHandle handle, GPUResourceExtra_t extra)
    {
        FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

        auto UserIdx = Handles[handle];
        UserEntries[UserIdx].extra = extra;
    }


    /************************************************************************************************/


    GPUResourceExtra_t  TextureStateTable::GetExtra(ResourceHandle handle) const
    {
        FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

        auto UserIdx = Handles[handle];
        return UserEntries[UserIdx].extra;
    }


	/************************************************************************************************/


    const char* TextureStateTable::GetDebug(ResourceHandle handle)
    {
        FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

        auto UserIdx = Handles[handle];
        return UserEntries[UserIdx].userString;
    }


    /************************************************************************************************/


	uint2 TextureStateTable::GetWH(ResourceHandle handle) const
	{
        FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

        auto UserIdx = Handles[handle];
        return Resources[UserEntries[UserIdx].ResourceIdx].WH;
	}


    /************************************************************************************************/


    uint3 TextureStateTable::GetXYZ(ResourceHandle handle) const
    {
        FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

        auto UserIdx    = Handles[handle];
        auto& user      = UserEntries[UserIdx];
        auto& resource  = Resources[UserEntries[UserIdx].ResourceIdx];

        return { resource.WH, user.arraySize };
    }


	/************************************************************************************************/


	void TextureStateTable::UpdateTileMappings(ResourceHandle handle, const TileMapping* begin, const TileMapping* end)
	{
		if (handle == InvalidHandle)
			return;

        FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto itr        = begin;
		auto UserIdx    = Handles[handle];
		auto& mappings  = UserEntries[UserIdx].tileMappings;

		TileMapList newElements{ allocator };

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

		for (auto& e : newElements)
			mappings.push_back(e);
	}


	/************************************************************************************************/


	void TextureStateTable::SubmitTileUpdates(ID3D12CommandQueue* queue, RenderSystem& renderSystem, iAllocator* allocator_temp)
	{
		Vector<D3D12_TILED_RESOURCE_COORDINATE> coordinates { allocator_temp };
		Vector<D3D12_TILE_REGION_SIZE>          regionSize  { allocator_temp };
		Vector<D3D12_TILE_RANGE_FLAGS>          tile_flags  { allocator_temp };
		Vector<UINT>                            heapOffsets { allocator_temp };
		Vector<UINT>                            tileCounts  { allocator_temp };

		for (auto& userEntry : UserEntries)
		{
			coordinates.size();
			regionSize.size();

			ID3D12Resource* resource    = nullptr;
			ID3D12Heap*     heap        = nullptr;

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
							coordinates.size(),
							coordinates.data(),
							regionSize.data(),
							heap,
							tile_flags.size(),
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


	const TileMapList& TextureStateTable::GetTileMappings(const ResourceHandle handle) const
	{
		auto UserIdx = Handles[handle];
		return UserEntries[UserIdx].tileMappings;
	}


	/************************************************************************************************/


	TileMapList& TextureStateTable::_GetTileMappings(const ResourceHandle handle)
	{
		auto UserIdx = Handles[handle];
		return UserEntries[UserIdx].tileMappings;
	}


	/************************************************************************************************/


	void TextureStateTable::MarkRTUsed(ResourceHandle Handle)
	{
		auto UserIdx = Handles[Handle];
		UserEntries[UserIdx].Flags |= TF_INUSE;
	}


	/************************************************************************************************/


	void TextureStateTable::LockUntil(size_t FrameID)
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


	void TextureStateTable::UpdateLocks(ThreadManager& threads, const int64_t currentIdx)
	{
        ProfileFunction();

        Vector<ID3D12Resource*> freeList = { delayRelease.Allocator };

		for (auto& res : delayRelease)
		{
#if _DEBUG
            if ((res.idx + 3) < currentIdx)
#else
            if ((res.idx) < currentIdx)
#endif
                freeList.push_back(res.resource);
		}

        auto& workItem = FlexKit::CreateWorkItem(
            [freeList = std::move(freeList)](auto& threadLocalAllocator)
            {
                for (auto res : freeList)
                    res->Release();
            }, delayRelease.Allocator);

        threads.AddBackgroundWork(workItem);

		delayRelease.erase(
			std::remove_if(
				delayRelease.begin(),
				delayRelease.end(),
				[&](auto& res)
				{
					return int(res.idx) < currentIdx;
				}),

        delayRelease.end());
	}


	/************************************************************************************************/


	void TextureStateTable::ResourceEntry::Release()
	{
		for (auto& R : Resources)
		{
			if (R)
				R->Release();

			R = nullptr;
		}
	}


	/************************************************************************************************/


	DeviceResourceState TextureStateTable::GetState(ResourceHandle Handle) const
	{
		auto    Idx			    = Handles[Handle];
		auto    ResourceIdx	    = UserEntries[Idx].ResourceIdx;
		const   auto& resources = Resources[ResourceIdx];

		return resources.States[resources.CurrentResource];
	}


	/************************************************************************************************/


	ID3D12Resource* TextureStateTable::GetResource(ResourceHandle Handle, ID3D12Device* device) const
	{
        if (Handle >= Handles.size())
            return nullptr;

		auto  Idx			= Handles[Handle];
		auto  ResourceIdx	= UserEntries[Idx].ResourceIdx;
		auto& resources     = Resources[ResourceIdx].Resources;
		auto  currentIdx    = Resources[ResourceIdx].CurrentResource;

		return resources[currentIdx];
	}


    /************************************************************************************************/


    size_t TextureStateTable::GetResourceSize(ResourceHandle Handle) const
    {
        auto  Idx			= Handles[Handle];
		return UserEntries[Idx].resourceSize;
    }


    /************************************************************************************************/


    uint2 TextureStateTable::GetHeapOffset(ResourceHandle Handle, uint subResourceIdx) const
    {
        auto Idx = Handles[Handle];

        return Resources[UserEntries[Idx].resourceSize].heapRange[subResourceIdx];
    }


    /************************************************************************************************/


    ResourceHandle TextureStateTable::FindResourceHandle(ID3D12Resource* deviceResource) const
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


	void TextureStateTable::ReplaceResources(ResourceHandle handle, ID3D12Resource** begin, size_t size)
	{
		auto  Idx                   = Handles[handle];
		auto  ResourceIdx           = UserEntries[Idx].ResourceIdx;
		const auto resourceCount    = Resources[ResourceIdx].ResourceCount;
		auto& resources             = Resources[ResourceIdx].Resources;

		for (size_t I = 0; I < Min(resourceCount, size); ++I)
			resources[I] = begin[I];
	}


	/************************************************************************************************/


	void InitiateGeometryTable(RenderSystem* renderSystem, iAllocator* Memory)
	{
		GeometryTable.Handles.Initiate(Memory);
		GeometryTable.Handle			= Vector<TriMeshHandle>(Memory);
		GeometryTable.Geometry			= Vector<TriMesh>(Memory);
		GeometryTable.ReferenceCounts	= Vector<size_t>(Memory);
		GeometryTable.Guids				= Vector<GUID_t>(Memory);
		GeometryTable.GeometryIDs		= Vector<const char*>(Memory);
		GeometryTable.FreeList			= Vector<size_t>(Memory);
		GeometryTable.Memory			= Memory;
        GeometryTable.renderSystem      = renderSystem;
	}


	/************************************************************************************************/


	void ReleaseGeometryTable()
	{
		for (auto G : GeometryTable.Geometry)
			ReleaseTriMesh(&G);

		GeometryTable.Geometry.Release();
		GeometryTable.ReferenceCounts.Release();
		GeometryTable.Guids.Release();
		GeometryTable.GeometryIDs.Release();
		GeometryTable.Handles.Release();
		GeometryTable.FreeList.Release();
		GeometryTable.Handle.Release();
	}


	/************************************************************************************************/


	bool IsMeshLoaded(GUID_t guid)
	{
		bool res = false;
		for (auto Entry : GeometryTable.Guids)
		{
			if (Entry == guid){
				res = true;
				break;
			}
		}

		return res;
	}


	/************************************************************************************************/


	void AddRef(TriMeshHandle TMHandle)
	{
		size_t Index = GeometryTable.Handles[TMHandle];

#ifdef _DEBUG
		if (Index != -1)
			GeometryTable.ReferenceCounts[Index]++;
#else
		GeometryTable.ReferenceCounts[Index]++;
#endif
	}


	/************************************************************************************************/


	void ReleaseMesh(TriMeshHandle TMHandle)
	{
        if (TMHandle == InvalidHandle)
            return;
		// TODO: MAKE ATOMIC
		if (GeometryTable.Handles[TMHandle] == -1)
			return;// Already Released

		size_t Index	= GeometryTable.Handles[TMHandle];
		auto Count		= --GeometryTable.ReferenceCounts[Index];

		if (Count == 0) 
		{
			auto G = GetMeshResource(TMHandle);

			DelayedReleaseTriMesh(GeometryTable.renderSystem, G);

			if (G->Skeleton)
				CleanUpSkeleton(G->Skeleton);

			GeometryTable.FreeList.push_back(Index);
			GeometryTable.Geometry[Index]   = TriMesh();
			GeometryTable.Handles[TMHandle] = -1;
		}
	}


	/************************************************************************************************/


	TriMeshHandle GetMesh(GUID_t guid, CopyContextHandle copyCtx )
	{
		if (IsMeshLoaded(guid))
		{
			auto [mesh, result] = FindMesh(guid);

			if(result)
				return mesh;
		}

		TriMeshHandle triMesh = LoadTriMeshIntoTable(
			copyCtx == InvalidHandle ? GeometryTable.renderSystem->GetImmediateUploadQueue() : copyCtx, guid);

		return triMesh;
	}


	/************************************************************************************************/


	TriMeshHandle GetMesh(const char* meshID, CopyContextHandle copyCtx )
	{
		auto [mesh, result] = FindMesh(meshID);

		if(result)
			return mesh;

		return LoadTriMeshIntoTable(copyCtx == InvalidHandle ? GeometryTable.renderSystem->GetImmediateUploadQueue() : copyCtx, meshID);
	}


	/************************************************************************************************/


	TriMesh* GetMeshResource(TriMeshHandle TMHandle)
    {
		FK_ASSERT(TMHandle != InvalidHandle);

#if USING(DEBUGGRAPHICS)
        if (GeometryTable.Handles[TMHandle] == -1)
        {
            DebugBreak();
            return nullptr;

        }
#endif
        return &GeometryTable.Geometry[GeometryTable.Handles[TMHandle]];
	}
	

	/************************************************************************************************/


	BoundingSphere GetMeshBoundingSphere(TriMeshHandle TMHandle)
	{
		auto Mesh = &GeometryTable.Geometry[GeometryTable.Handles[TMHandle]];
		return float4{ float3{0}, Mesh->Info.r };
	}


	/************************************************************************************************/


	Skeleton* GetSkeleton(TriMeshHandle TMHandle){
		return GetMeshResource(TMHandle)->Skeleton;
	}


	/************************************************************************************************/


	size_t	GetSkeletonGUID(TriMeshHandle TMHandle){
		return GetMeshResource(TMHandle)->SkeletonGUID;
	}


	/************************************************************************************************/


	void SetSkeleton(TriMeshHandle TMHandle, Skeleton* S){
		GetMeshResource(TMHandle)->Skeleton = S;
	}


	/************************************************************************************************/


	bool IsSkeletonLoaded(TriMeshHandle guid){
		return (GetMeshResource(guid)->Skeleton != nullptr);
	}


	/************************************************************************************************/


	bool HasAnimationData(TriMeshHandle RMeshHandle){
		return GetSkeleton(RMeshHandle)->Animations != nullptr;
	}



	/************************************************************************************************/


	Pair<TriMeshHandle, bool>	FindMesh(GUID_t guid)
	{
		size_t location		= 0;
		size_t HandleIndex	= 0;
		for (auto Entry : GeometryTable.Guids)
		{
			if (Entry == guid) {
				for ( auto index : GeometryTable.Handles.Indexes)
				{
					if (index == location) {
						return { TriMeshHandle(HandleIndex, GeometryTable.Handles.mType, 0x04), true };
						break;
					}
					++HandleIndex;
				}
				break;
			}
			++location;
		}

		return { InvalidHandle, false };
	}
	

	/************************************************************************************************/


	Pair<TriMeshHandle, bool>	FindMesh(const char* ID)
	{
		TriMeshHandle HandleOut = InvalidHandle;
		size_t location			= 0;
		size_t HandleIndex		= 0;

		for (auto Entry : GeometryTable.GeometryIDs)
		{
			if (!strncmp(Entry, ID, 64)) {
				for (auto index : GeometryTable.Handles.Indexes)
				{
					if (index == location) {
						HandleOut.INDEX = HandleIndex;
						break;
					}
					++HandleIndex;
				}
				break;
			}
			++location;
		}

		return{ HandleOut, 0 };
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


	/************************************************************************************************/


    void RenderSystem::_InsertBarrier(const Vector<Barrier>& vector)
	{
        std::unique_lock lock{ barrierLock };
        PendingBarriers += vector;
	}


    /************************************************************************************************/


    void RenderSystem::_InsertBarrier(Barrier b)
    {
        PendingBarriers.push_back(b);
    }


	/************************************************************************************************/


	size_t RenderSystem::_GetVidMemUsage()
	{
		DXGI_QUERY_VIDEO_MEMORY_INFO VideoMemInfo = {0};
		if(pDXGIAdapter)
			pDXGIAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &VideoMemInfo);

		return VideoMemInfo.CurrentUsage;
	}


	/************************************************************************************************/


	ID3D12QueryHeap* RenderSystem::_GetQueryResource(QueryHandle Handle)
	{
		return Queries.GetAsset(Handle);
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
        auto reason = pDevice->GetDeviceRemovedReason();

        switch (reason)
        {
        case DXGI_ERROR_DEVICE_HUNG:
            FK_LOG_ERROR("DXGI_ERROR_DEVICE_HUNG");             break;
        case DXGI_ERROR_DEVICE_REMOVED:                         
            FK_LOG_ERROR("DXGI_ERROR_DEVICE_REMOVED");          break;
        case DXGI_ERROR_DEVICE_RESET:                           
            FK_LOG_ERROR("DXGI_ERROR_DEVICE_RESET");            break;
        case DXGI_ERROR_DRIVER_INTERNAL_ERROR:                  
            FK_LOG_ERROR("DXGI_ERROR_DRIVER_INTERNAL_ERROR");   break;
        case DXGI_ERROR_INVALID_CALL:                           
            FK_LOG_ERROR("DXGI_ERROR_INVALID_CALL");            break;
        case S_OK:
            FK_LOG_ERROR("???? S_OK ????");                     break;
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

        D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 DredAutoBreadcrumbsOutput;
        D3D12_DRED_PAGE_FAULT_OUTPUT DredPageFaultOutput;

        if (auto HR = dred->GetAutoBreadcrumbsOutput1(&DredAutoBreadcrumbsOutput); FAILED(HR))
            FK_LOG_ERROR("Failed to get Breadcrumbs!");

        if (auto HR = dred->GetPageFaultAllocationOutput(&DredPageFaultOutput); FAILED(HR))
            FK_LOG_ERROR("Failed to get Fault Allocation Info!");

        auto node = DredAutoBreadcrumbsOutput.pHeadAutoBreadcrumbNode;

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
                    FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT"); break;
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

        HRESULT hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pix));

        return SUCCEEDED(hr);
#else
        return false;
#endif
    }


    bool RenderSystem::DEBUG_BeginPixCapture()
    {
#if USING(PIX)
        if (pix)
            pix->BeginCapture();

        return pix != nullptr;
#else
        return false;
#endif
    }


    bool RenderSystem::DEBUG_EndPixCapture()
    {
#if USING(PIX)
        if (pix)
            pix->EndCapture();

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

		TextureBuffer textureBuffer{ { 1,  1 }, (FlexKit::byte*)tempBuffer, 256, 4, nullptr };

		auto defaultTexture = MoveTextureBuffersToVRAM(
			this,
			upload,
			&textureBuffer,
			1,
			1,
			DeviceFormat::R8G8B8A8_UNORM);

        SetDebugName(defaultTexture, "Default Texture");
		SubmitUploadQueues(0, &upload);

		return defaultTexture;
	}


	/************************************************************************************************/


	Context& RenderSystem::GetCommandList()
	{
		const size_t completedCounter = Fence->GetCompletedValue();

		for(auto& _ : Contexts)
		{
			const size_t idx = contextIdx;
			contextIdx = ++contextIdx % Contexts.size();

			Context& context = Contexts[idx];
			if (context._GetCounter() <= completedCounter)
				return context.Reset();
		}

		FK_ASSERT(0, "Failed to get a free context!");

		return Contexts[0];
	}


	/************************************************************************************************/


	void RenderSystem::BeginSubmission()
	{
        ProfileFunction();

		const auto pendingFrame = pendingFrames[(frameIdx) % 2];

		while (pendingFrame > Fence->GetCompletedValue())
		{
			HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS); FK_ASSERT(eventHandle != 0);
			Fence->SetEventOnCompletion(pendingFrame, eventHandle);

            while (WaitForSingleObject(eventHandle, 1) == WAIT_TIMEOUT)
            {
                auto temp = Fence->GetCompletedValue();
                int x = 0;
            }

			CloseHandle(eventHandle);
		}
	}


	/************************************************************************************************/


    SyncPoint RenderSystem::Submit(Vector<Context*>& contexts, std::optional<SyncPoint> syncOptional)
	{
        ProfileFunction();

		if (ImmediateUpload != InvalidHandle)
		{
			SubmitUploadQueues(SYNC_Graphics, &ImmediateUpload);
			ImmediateUpload = InvalidHandle;
		}

		const auto counter = graphicsSubmissionCounter++;

        static_vector<ID3D12CommandList*, 64> cls;

        if (PendingBarriers.size())
        {
            std::unique_lock lock{ barrierLock };

            auto& context = GetCommandList();
            cls.push_back(context.GetCommandList());

            for (auto barrier : PendingBarriers) {
                if (barrier.type == Barrier::ResourceType::PTR)
                    context._AddBarrier(
                        barrier._ptr,
                        barrier.beforeState,
                        barrier.afterState);
                else if (barrier.type == Barrier::ResourceType::HNDL) {
                    context._AddBarrier(
                        GetDeviceResource(barrier.handle),
                        barrier.beforeState,
                        barrier.afterState);

                    SetObjectState(barrier.handle, barrier.afterState);
                }
            }

            context.FlushBarriers();
            context.Close(counter);
            PendingBarriers.clear();
        }

		for (auto context : contexts)
		{
			cls.push_back(context->GetCommandList());
			context->Close(counter);
		}

        if (auto sync = syncOptional.value_or(SyncPoint{}); syncOptional.has_value())
            GraphicsQueue->Wait(sync.fence, sync.syncCounter);

		GraphicsQueue->ExecuteCommandLists(cls.size(), cls.begin());

        if (auto HR = GraphicsQueue->Signal(Fence, counter); FAILED(HR))
            FK_LOG_ERROR("Failed to Signal");

		for (auto context : contexts)
			context->_QueueReadBacks();

		VertexBuffers.LockUntil(GetCurrentFrame() + 1);
		ConstantBuffers.LockFor(2);
		Textures.LockUntil(GetCurrentFrame() + 1);

		pendingFrames[frameIdx] = counter;
		frameIdx = ++frameIdx % 2;

		ReadBackTable.Update();

        _UpdateCounters();

        return { counter, Fence };
	}


	/************************************************************************************************/


    SyncPoint RenderSystem::SubmitUploadQueues(uint32_t flags, CopyContextHandle* handles, size_t count, std::optional<SyncPoint> sync)
	{
		auto syncPoint = copyEngine.Submit(handles, handles + count, sync);

		if (SYNC_Graphics & flags)
		{
			auto HR = GraphicsQueue->Wait(syncPoint.fence, syncPoint.syncCounter);  FK_ASSERT(SUCCEEDED(HR));
		}

		if (SYNC_Compute & flags)
		{
		}

        return syncPoint;
	}


	/************************************************************************************************/


	CopyContextHandle RenderSystem::OpenUploadQueue()
	{
		return copyEngine.Open();
	}


	/************************************************************************************************/


	CopyContextHandle RenderSystem::GetImmediateUploadQueue()
	{
		if (ImmediateUpload == InvalidHandle)
			ImmediateUpload = copyEngine.Open();

		return ImmediateUpload;
	}


	/************************************************************************************************/
	

	bool CreateInputLayout(RenderSystem* RS, VertexBufferView** Buffers, size_t count, Shader* Shader, VertexBuffer* DVB_Out)
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
						InputElementDesc.SemanticIndex        = ++POS_Buffer_Counter;
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
						InputElementDesc.SemanticIndex        = COLOR_Buffer_Counter++;
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
						InputElementDesc.SemanticIndex        = Normal_Buffer_Counter++;
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
						InputElementDesc.SemanticIndex        = Tangent_Buffer_Counter++;
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
						InputElementDesc.SemanticIndex        = UV_Buffer_Counter;
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
						InputElementDesc.SemanticIndex        = WEIGHT_Buffer_Counter;
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
						InputElementDesc.SemanticIndex        = WINDICES_Buffer_Counter++;
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
						InputElementDesc.SemanticIndex        = WINDICES_Buffer_Counter++;
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

					InputElementDesc.SemanticIndex			= POS_Buffer_Counter++;
					InputElementDesc.SemanticName			= "POSITION";
					Input_Desc.push_back(InputElementDesc);
					InputElementDesc.AlignedByteOffset		= 16;
					InputElementDesc.SemanticName			= "NORMAL";
					InputElementDesc.SemanticIndex			= Normal_Buffer_Counter++;
					Input_Desc.push_back(InputElementDesc);
					InputElementDesc.AlignedByteOffset		= 32;
					InputElementDesc.SemanticName			= "TANGENT";
					InputElementDesc.SemanticIndex			= Tangent_Buffer_Counter++;
					Input_Desc.push_back(InputElementDesc);
					InputElementDesc.AlignedByteOffset		= 48;
					InputElementDesc.SemanticName			= "TEXCOORD";
					InputElementDesc.SemanticIndex			= UV_Buffer_Counter++;
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

					InputElementDesc.SemanticIndex			= WEIGHT_Buffer_Counter++;
					InputElementDesc.SemanticName			= "WEIGHTS";
					Input_Desc.push_back(InputElementDesc);

					InputElementDesc.AlignedByteOffset		= 12;
					InputElementDesc.SemanticIndex			= WINDICES_Buffer_Counter++;
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
	

	void Release(VertexBuffer* VertexBuffer)
	{	
		for (auto Buffer : VertexBuffer->VertexBuffers) {
			if (Buffer.Buffer)
				Buffer.Buffer->Release();

			Buffer.Buffer = nullptr;
			Buffer.BufferSizeInBytes = 0;
		}
	}
	

	void DelayedRelease(RenderSystem* RS, VertexBuffer* VertexBuffer)
	{
		for (auto& Buffer : VertexBuffer->VertexBuffers) {
			if (Buffer.Buffer)
				Push_DelayedRelease(RS, Buffer.Buffer);
			Buffer.Buffer				= nullptr;
			Buffer.BufferSizeInBytes	= 0;
		}
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
	
/*
	bool LoadAndCompileShaderFromFile(const char* FileLoc, ShaderDesc* desc, Shader* out )
	{
		size_t ConvertCount = 0;
		wchar_t WString[256];
		mbstowcs_s(&ConvertCount, WString, FileLoc, 128);
		ID3DBlob* NewBlob   = nullptr;
		ID3DBlob* Errors    = nullptr;

		auto flags = D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;

#if NDEBUG
		flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

#if USING( DEBUGGRAPHICS )
		flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_ENABLE_STRICTNESS;
#endif


		HRESULT HR = D3DCompileFromFile(WString, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, desc->entry, desc->shaderVersion, flags, 0, &NewBlob, &Errors);
		if (FAILED(HR))	{
			FK_LOG_ERROR((char*)Errors->GetBufferPointer());
			return false;
		}

        *out = Shader{ NewBlob };
        NewBlob->Release();

		return true;
	}
    */

	/************************************************************************************************/


    /*
	Shader LoadShader_OLD(const char* Entry,  const char* ShaderVersion, const char* File)
	{
		Shader Shader;

		bool res = false;
		FlexKit::ShaderDesc SDesc;
		strncpy_s(SDesc.entry, Entry, 128);
		strncpy_s(SDesc.shaderVersion, ShaderVersion, 16);

		do
		{
			FK_LOG_2("LoadingShader - %s - \n", Entry);
			res = LoadAndCompileShaderFromFile(File, &SDesc, &Shader);
#if USING( EDITSHADERCONTINUE )
			if (!res)
			{
				std::cout << "Failed to Compile Shader\n Press Enter to try again\n";
				char str[100];
				std::cin >> str;
			}
#else
			FK_ASSERT(res);
#endif
			if (res)
				return Shader;

		} while (!res);

		return Shader;
	}
    */


	/************************************************************************************************/


	DescHeapPOS PushRenderTarget(RenderSystem* RS, ResourceHandle target, DescHeapPOS POS, const size_t MIPOffset)
	{
		D3D12_RENDER_TARGET_VIEW_DESC TargetDesc = {};
		const auto dimension            = RS->GetTextureDimension(target);

		switch (dimension)
		{
		case TextureDimension::Texture2D:
			TargetDesc.Format               = TextureFormat2DXGIFormat(RS->GetTextureFormat(target));
			TargetDesc.Texture2D.MipSlice   = MIPOffset;
			TargetDesc.Texture2D.PlaneSlice = 0;
			TargetDesc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
			break;
		case TextureDimension::TextureCubeMap:
			TargetDesc.Format                           = TextureFormat2DXGIFormat(RS->GetTextureFormat(target));
			TargetDesc.Texture2DArray.FirstArraySlice   = 0;
			TargetDesc.Texture2DArray.MipSlice          = MIPOffset;
			TargetDesc.Texture2DArray.PlaneSlice        = 0;
			TargetDesc.Texture2DArray.ArraySize         = 6;
			TargetDesc.ViewDimension                    = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			break;
		}
			

		auto resource = RS->GetDeviceResource(target);
		RS->pDevice->CreateRenderTargetView(resource, &TargetDesc, POS);

		return IncrementHeapPOS(POS, RS->DescriptorRTVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushDepthStencil(RenderSystem* RS, ResourceHandle Target, DescHeapPOS POS)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
		DSVDesc.Format				= RS->GetTextureDeviceFormat(Target);
		DSVDesc.Texture2D.MipSlice	= 0;
		DSVDesc.ViewDimension		= D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2D;

		RS->pDevice->CreateDepthStencilView(RS->GetDeviceResource(Target), &DSVDesc, POS);

		return IncrementHeapPOS(POS, RS->DescriptorDSVSize, 1);
	}

	DescHeapPOS PushDepthStencilArray(RenderSystem* RS, ResourceHandle Target, size_t arrayOffset, size_t MipSlice, DescHeapPOS POS)
	{
		const size_t arraySize = RS->GetTextureArraySize(Target);

		D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
		DSVDesc.Format                          = RS->GetTextureDeviceFormat(Target);
		DSVDesc.Texture2DArray.ArraySize        = arraySize - arrayOffset;
		DSVDesc.Texture2DArray.FirstArraySlice  = arrayOffset;
		DSVDesc.Texture2DArray.MipSlice         = MipSlice;
		DSVDesc.ViewDimension                   = D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2DARRAY;

		RS->pDevice->CreateDepthStencilView(RS->GetDeviceResource(Target), &DSVDesc, POS);

		return IncrementHeapPOS(POS, RS->DescriptorDSVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushCBToDescHeap(RenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, size_t BufferSize, size_t Offset)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC CBV_DESC = {};
		CBV_DESC.BufferLocation = Buffer ? Buffer->GetGPUVirtualAddress() + Offset : 0;
		CBV_DESC.SizeInBytes	= BufferSize;
		RS->pDevice->CreateConstantBufferView(&CBV_DESC, POS);

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushSRVToDescHeap(RenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, size_t ElementCount, size_t Stride, D3D12_BUFFER_SRV_FLAGS Flags, size_t offset)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC Desc; {
			Desc.Format                     = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
			Desc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			Desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
			Desc.Buffer.FirstElement        = offset;
			Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE;
			Desc.Buffer.NumElements         = ElementCount - offset;
			Desc.Buffer.StructureByteStride = Stride;
		}

        FK_ASSERT(Desc.Buffer.StructureByteStride < 512);
        FK_ASSERT(Desc.Buffer.NumElements > 0);

        RS->pDevice->CreateShaderResourceView(Buffer, &Desc, POS);

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS Push2DSRVToDescHeap(RenderSystem* RS, ID3D12Resource* Buffer, const DescHeapPOS POS, const D3D12_BUFFER_SRV_FLAGS Flags, const DXGI_FORMAT format)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC Desc; {
			Desc.Format                         = format;
			Desc.Shader4ComponentMapping        = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			Desc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE2D;
			Desc.Texture2D.MipLevels		    = 1;
			Desc.Texture2D.MostDetailedMip	    = 0;
			Desc.Texture2D.PlaneSlice		    = 0;
			Desc.Texture2D.ResourceMinLODClamp	= 0;
		}

		RS->pDevice->CreateShaderResourceView(Buffer, &Desc, POS);
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

		RS->pDevice->CreateShaderResourceView(tex, &ViewDesc, POS);

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
			ViewDesc.Texture2DArray.ArraySize           = arraySize;
		}

		auto debug = RS->GetDeviceResource(handle);
		RS->pDevice->CreateShaderResourceView(RS->GetDeviceResource(handle), &ViewDesc, POS);

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
			ViewDesc.Texture2DArray.ArraySize           = arraySize;
		}

		auto debug = RS->GetDeviceResource(handle);
		RS->pDevice->CreateShaderResourceView(RS->GetDeviceResource(handle), &ViewDesc, POS);

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
			ViewDesc.Texture3D.ResourceMinLODClamp      = minLODClamp;
		}

		auto debug = RS->GetDeviceResource(handle);
		RS->pDevice->CreateShaderResourceView(RS->GetDeviceResource(handle), &ViewDesc, POS);

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

		RS->pDevice->CreateShaderResourceView(RS->GetDeviceResource(resource), &ViewDesc, POS);

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

		RS->pDevice->CreateUnorderedAccessView(tex, nullptr, &UAVDesc, POS);
		
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

		RS->pDevice->CreateUnorderedAccessView(tex, nullptr, &UAVDesc, POS);
		
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

		RS->pDevice->CreateUnorderedAccessView(tex, nullptr, &UAVDesc, POS);
		
		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


    /************************************************************************************************/


	inline DescHeapPOS PushUAV1DToDescHeap(RenderSystem* RS, ID3D12Resource* resource, DXGI_FORMAT format, uint mip, DescHeapPOS POS)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		UAVDesc.Format						= format;
		UAVDesc.ViewDimension				= D3D12_UAV_DIMENSION_TEXTURE1D;
		UAVDesc.Texture1D.MipSlice          = 0;

		RS->pDevice->CreateUnorderedAccessView(resource, nullptr, &UAVDesc, POS);

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

		RS->pDevice->CreateUnorderedAccessView(buffer.resource, nullptr, &UAVDesc, POS);

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

		RS->pDevice->CreateUnorderedAccessView(buffer.resource, counter, &UAVDesc, POS);

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}

	/************************************************************************************************/


	bool LoadObjMesh(RenderSystem* RS, char* File_Loc, Obj_Desc IN desc, TriMesh ROUT out, StackAllocator RINOUT LevelSpace, StackAllocator RINOUT TempSpace, bool DiscardBuffers)
	{
		using MeshUtilityFunctions::TokenList;
		using MeshUtilityFunctions::CombinedVertexBuffer;
		using MeshUtilityFunctions::IndexList;
		using namespace FlexKit::MeshUtilityFunctions;
		using namespace FlexKit::MeshUtilityFunctions::OBJ_Tools;

		// TODO: Handle Multi Threading Cases
		size_t pos = 0;
		size_t buffersize = 1024*1024*16;
		size_t size = buffersize;
		size_t line_pos = 0;

		char*	strBuffer = (char*)TempSpace.malloc(buffersize);
		char	current_line[512];

		memset(strBuffer, 0, buffersize);
		bool Loaded = FlexKit::LoadFileIntoBuffer(File_Loc, (byte*)strBuffer, size);// TODO: Make Thread Safe
		if (!Loaded)
		{
			TempSpace.clear();
			printf("Failed To Load Obj\n");
			return false;
		}
		
		TokenList				TL			{ TempSpace, 64000 };
		CombinedVertexBuffer	out_buffer	{ TempSpace, 16000 };
		IndexList				out_indexes	{ TempSpace, 64000 };

		//TL.push_back(s_TokenValue::Empty());
		LoaderState S;

		size = strlen(strBuffer);
		while( pos < size)
		{
			if( strBuffer[pos] != '\n' )
			{
				current_line[line_pos++] = strBuffer[pos];
			}
			else
			{
				size_t LineLength = line_pos;
				current_line[LineLength] = '\0';
				CStrToToken( ScrubLine( current_line, LineLength ), LineLength, TL, S );
				line_pos = 0;
			}
			pos++;
		}

		//if (!FlexKit::MeshUtilityFunctions::BuildVertexBuffer(TL, out_buffer, out_indexes, LevelSpace, TempSpace))
			return false;

		size_t VertexBufferSize = out_buffer.size()		* sizeof(float3)	+ sizeof(VertexBufferView);// pos
		size_t IndexBufferSize  = out_indexes.size()	* sizeof(uint32_t)	+ sizeof(VertexBufferView);// index
		size_t NormalBufferSize = out_buffer.size()		* sizeof(float3)	+ sizeof(VertexBufferView);// Normal

		// Optional Buffers
		size_t TexcordBufferSize	= out_buffer.size()	* sizeof(float2)	+ sizeof(VertexBufferView);// Texcoord
		size_t TangentBufferSize	= out_buffer.size()	* sizeof(float3)	+ sizeof(VertexBufferView);// Tangent
		size_t extraBufferSize		= out_buffer.size()	* sizeof(float3)	+ sizeof(VertexBufferView);// ?

		byte* VertexBuffer	= (byte*)(DiscardBuffers ? TempSpace._aligned_malloc(VertexBufferSize, 16)	: LevelSpace._aligned_malloc(VertexBufferSize, 16));// Position
		byte* IndexBuffer	= (byte*)(DiscardBuffers ? TempSpace._aligned_malloc(IndexBufferSize, 16)	: LevelSpace._aligned_malloc(IndexBufferSize, 16)); // Index
		byte* NormalBuffer	= (byte*)(DiscardBuffers ? TempSpace._aligned_malloc(NormalBufferSize, 16)	: LevelSpace._aligned_malloc(NormalBufferSize, 16));// Normal

		byte* TexcordBuffer  = (byte*)(desc.LoadUVs				? (DiscardBuffers ? TempSpace._aligned_malloc(TexcordBufferSize, 16) : LevelSpace._aligned_malloc(TexcordBufferSize, 16)) : nullptr); // UV's
		byte* TangentBuffer	 = (byte*)(desc.GenerateTangents	? (DiscardBuffers ? TempSpace._aligned_malloc(TangentBufferSize, 16) : LevelSpace._aligned_malloc(TangentBufferSize, 16)) : nullptr); // Tangents

		FK_ASSERT(false);

		//out.Buffers[00] = FlexKit::CreateVertexBufferView(VertexBuffer, VertexBufferSize);
		//out.Buffers[01] = FlexKit::CreateVertexBufferView(NormalBuffer, NormalBufferSize);
		//out.Buffers[15] = FlexKit::CreateVertexBufferView(IndexBuffer,  IndexBufferSize);

        /*
		out.Buffers[0]->Begin
			( FlexKit::VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION
			, FlexKit::VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32 );
        

		float3 Vmn = float3(0);
		float3 Vmx = float3(0);
		float BoundingShere = 0;

		for (auto V : out_buffer)
		{
			for (auto itr = 0; itr < 3; ++itr)
			{	// find Max
				if (V.POS[itr] > Vmx[itr]) 
					Vmx[itr] = V.POS[itr];

				// find Min
				if (V.POS[itr] < Vmn[itr]) 
					Vmn[itr] = V.POS[itr];

				if (V.POS.magnitude() > BoundingShere)
					BoundingShere = V.POS.magnitude();

			}
			out.Buffers[0]->Push(V.POS * desc.S);
		}

		out.Buffers[0]->End();

		out.Buffers[15]->Begin
			( FlexKit::VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX
			, FlexKit::VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32 );

		for (auto itr = out_indexes.begin(); itr != out_indexes.end(); )
		{
			auto I1 = *itr++;
			auto I2 = *itr++;
			auto I3 = *itr++;

			out.Buffers[15]->Push(uint32_t(I2));
			out.Buffers[15]->Push(uint32_t(I1));
			out.Buffers[15]->Push(uint32_t(I3));
		}

		out.Buffers[1]->End();

		if (S.Normals)
		{
			out.Buffers[1]->Begin
				( FlexKit::VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL
				, FlexKit::VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32 );

			for (auto V : out_buffer)
				out.Buffers[1]->Push( V.NORMAL );

			out.Buffers[1]->End();
		}
		out.IndexCount = out_indexes.size();

		if (S.UV_1 && desc.LoadUVs)
		{
			//out.Buffers[2] = FlexKit::CreateVertexBufferView(TexcordBuffer, TexcordBufferSize);
			out.Buffers[2]->Begin
				( FlexKit::VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV
				, FlexKit::VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32 );

			for (auto V : out_buffer)
				out.Buffers[2]->Push(V.TEXCOORD);

			out.Buffers[2]->End();
		}

		if (desc.LoadUVs && desc.GenerateTangents)
		{
			// TODO(RM): Lift out into Mesh Utilities 
			FK_ASSERT(TangentBuffer); // Check that output is not Null
		
			//out.Buffers[3] = FlexKit::CreateVertexBufferView(TangentBuffer, TangentBufferSize);
			out.Buffers[3]->Begin
				( FlexKit::VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT
				, FlexKit::VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32 );

			// Set Clear Tangents
			for (auto V : out_buffer)
				out.Buffers[3]->Push(float3{0.0f, 0.0f, 0.0f});

			auto IndexBuffer	= reinterpret_cast<uint32_t*>(out.Buffers[15]->GetBuffer()); 
			auto VBuffer		= reinterpret_cast<float3*>	 (out.Buffers[0]->GetBuffer());
			auto NBuffer		= reinterpret_cast<float3*>	 (out.Buffers[1]->GetBuffer()); 
			auto TexCoords		= reinterpret_cast<float2*>	 (out.Buffers[2]->GetBuffer()); 
			auto Tangents		= reinterpret_cast<float3*>	 (out.Buffers[3]->GetBuffer()); 

			for (auto itr = 0; itr < out.Buffers[0]->GetBufferSizeUsed(); itr+= 3)
			{
				float3 V0 = VBuffer[IndexBuffer[itr + 0]];
				float3 V1 = VBuffer[IndexBuffer[itr + 1]];
				float3 V2 = VBuffer[IndexBuffer[itr + 2]];

				float2 UV1 = TexCoords[IndexBuffer[itr + 0]];
				float2 UV2 = TexCoords[IndexBuffer[itr + 1]];
				float2 UV3 = TexCoords[IndexBuffer[itr + 2]];

				float3 Q1 = V1 - V0;
				float3 Q2 = V2 - V0;

				float2 ST1 = UV2 - UV1;
				float2 ST2 = UV3 - UV1;

				float r = 1.0f / ((ST1.x*ST2.y) - (ST2.x * ST1.y));

				float3 S = r * float3((ST2.y * Q1[0]) - (ST1.y * Q2[0]), (ST2.y * Q1[1]) - (ST1.y * Q2[1]), (ST2.y * Q1[2]) - (ST1.y * Q2[2]));
				float3 T = r * float3((ST1.x * Q1[0]) - (ST2.x * Q2[0]), (ST1.x * Q1[1]) - (ST2.x * Q2[1]), (ST1.x * Q1[2]) - (ST2.x * Q2[2]));

				Tangents[IndexBuffer[itr + 0]] += S;
				Tangents[IndexBuffer[itr + 1]] += S;
				Tangents[IndexBuffer[itr + 2]] += S;
			}

			// Normalise Averaged Tangents
			float3* Normals = (float3*)out.Buffers[3]->GetBuffer();
			float3* end = (float3*)(out.Buffers[3]->GetBuffer() + out.Buffers[3]->GetBufferSizeRaw());

			while (Normals < end)
			{
				(*Normals) = Normals->normal();
				Normals++;
			}
		}

		FlexKit::CreateVertexBuffer		(RS, RS->GetImmediateUploadQueue(), out.Buffers, 2 + desc.LoadUVs + desc.GenerateTangents,           out.VertexBuffer);

		out.Info.Max = Vmx;
		out.Info.Min = Vmn;

		if (DiscardBuffers) {
			//ClearTriMeshVBVs(&out);
			TempSpace.clear();
		}
        */
		return false;
	}

	
	/************************************************************************************************/


	void ReleaseTriMesh(TriMesh* T)
	{
		if(T->Memory){
            for (auto& details : T->lods) {
                Release(&details.vertexBuffer);

                for (auto& buffer : details.buffers)
                {
                    if (buffer)
                        T->Memory->free(buffer);

                    buffer = nullptr;
                }
            }
			T->Memory->free((void*)T->ID);
		}
	}


	/************************************************************************************************/


	void DelayedReleaseTriMesh(RenderSystem* RS, TriMesh* T)
	{
        for (auto& detailLevel : T->lods)
        {
            DelayedRelease(RS, &detailLevel.vertexBuffer);

            for (auto& B : detailLevel.buffers)
            {
                if (B)
                    T->Memory->free(B);

                B = nullptr;
            }

            detailLevel.vertexBuffer.clear();
        }

		T->Memory->free((void*)T->ID);
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
		auto textureHandle = RS->CreateGPUResource(GPUResourceDesc::ShaderResource(buffer[0].WH, format, resourceCount));
		RS->UploadTexture(textureHandle, handle, buffer, resourceCount);
        RS->SetDebugName(textureHandle, "MoveTextureBuffersToVRAM");

		return textureHandle;
	}


	/************************************************************************************************/


	ResourceHandle MoveTextureBuffersToVRAM(RenderSystem* RS, CopyContextHandle handle, TextureBuffer* buffer, size_t MIPCount, size_t arrayCount, DeviceFormat format)
	{
		auto textureHandle = RS->CreateGPUResource(GPUResourceDesc::ShaderResource(buffer[0].WH, format, MIPCount, arrayCount));
		RS->UploadTexture(textureHandle, handle, buffer, arrayCount * MIPCount);
        RS->SetDebugName(textureHandle, "MoveTextureBuffersToVRAM");

		return textureHandle;
	}


    /************************************************************************************************/


    ResourceHandle MoveBufferToDevice(RenderSystem* RS, const char* buffer, const size_t byteSize, CopyContextHandle copyCtx)
    {
        auto bufferResource = RS->CreateGPUResource(GPUResourceDesc::StructuredResource(byteSize));
        UploadSegment upload = ReserveUploadBuffer(*RS, byteSize);
        MoveBuffer2UploadBuffer(upload, (const byte*)buffer, byteSize);

        auto deviceResource = RS->GetDeviceResource(bufferResource);
        auto ctx = RS->_GetCopyContext(copyCtx);

        if(auto state = RS->GetObjectState(bufferResource); state!= DRS_CopyDest)
            ctx.Barrier(deviceResource, state, DRS_CopyDest);

        ctx.CopyBuffer(deviceResource, 0, upload.resource, upload.offset, upload.uploadSize);
        ctx.Barrier(deviceResource, DRS_CopyDest, DRS_Common);

        return bufferResource;
    }


	/************************************************************************************************/


	ResourceHandle LoadTexture(TextureBuffer* Buffer, CopyContextHandle handle, RenderSystem* RS, iAllocator* Memout, DeviceFormat format)
	{
		GPUResourceDesc GPUResourceDesc = GPUResourceDesc::ShaderResource(Buffer->WH, format);
		GPUResourceDesc.initial     = Buffer->Buffer;

		size_t elementSize          = GetFormatElementSize(TextureFormat2DXGIFormat(format));
		size_t ResourceSizes[]      = { Buffer->Size };

		auto texture = RS->CreateGPUResource(GPUResourceDesc);
		SubResourceUpload_Desc desc = {};
		desc.buffers                         = Buffer;
		desc.subResourceCount                = 1;
		desc.subResourceStart                = 0;
		desc.format                          = format;

		_UpdateSubResourceByUploadQueue(
			RS,
			handle,
			RS->GetDeviceResource(texture),
			&desc,
            DRS_GenericRead);

		RS->Textures.SetState(texture, DeviceResourceState::DRS_GenericRead);
		RS->SetDebugName(texture, "LOADTEXTURE");

		return texture;
	}


    /************************************************************************************************/


    MemoryPoolAllocator::MemoryPoolAllocator(RenderSystem& IN_renderSystem, size_t IN_heapSize, size_t IN_blockSize, uint32_t IN_flags, iAllocator* IN_allocator) :
        renderSystem    { IN_renderSystem },
        blockCount      { IN_heapSize / IN_blockSize },
        blockSize       { IN_blockSize },
        allocator       { IN_allocator },
        allocations     { IN_allocator },
        freeRanges      { IN_allocator },
        heap            { IN_renderSystem.CreateHeap(IN_heapSize, IN_flags) },
        flags           { IN_flags }
    {
        freeRanges.push_back({ 0, (uint32_t)blockCount, Clear });
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
        std::scoped_lock localLock{ m };

        std::sort(freeRanges.begin(), freeRanges.end());

        auto _GetMemory = [&]() -> GPUHeapAllocation {
            for (auto& range : freeRanges)
            {
                if (range.offset > blockCount)
                    __debugbreak();

                if (range.blockCount >= requestBlockCount)
                {
                    if  (range.flags == Clear ||
                        (range.flags == Locked && range.frameID + 2 < frameID) ||
                        (range.flags | AllowReallocation && range.frameID == frameID))
                    {
                        GPUHeapAllocation heapAllocation = {
                            range.offset * blockSize,
                            requestBlockCount * blockSize,
                            (range.priorAllocation != InvalidHandle &&
                             range.flags | AllowReallocation &&
                             range.frameID == frameID) ?
                                range.priorAllocation : InvalidHandle
                        };

                        FK_LOG_9("Allocated Blocks %u - %u", range.offset, range.offset + range.blockCount);

                        if (range.blockCount > requestBlockCount)
                        {
                            range.blockCount    -= requestBlockCount;
                            range.offset        += requestBlockCount;
                        }
                        else if (range.blockCount == requestBlockCount) {
                            freeRanges.remove_unstable(&range);
                        }

                        return heapAllocation;
                    }
                }
            }

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
        const auto frameID = renderSystem.GetCurrentFrame();
        FK_LOG_9("Coalesce");

        std::sort(
            std::begin(freeRanges),
            std::end(freeRanges),
            [&](auto& lhs, auto& rhs)
            {
                return lhs.offset < rhs.offset;
            }
        );

        size_t I = 0; 
        while (I + 1 < freeRanges.size())
        {
            if (freeRanges[I].offset + freeRanges[I].blockCount == freeRanges[I + 1].offset &&
                freeRanges[I].frameID + 1 < frameID &&
                freeRanges[I + 1].frameID + 1 < frameID)
            {
                freeRanges[I].blockCount += freeRanges[I + 1].blockCount;
                freeRanges.remove_stable(freeRanges.begin() + I + 1);
            }
            else
                I++;
        }
    }

    /************************************************************************************************/


    AcquireResult MemoryPoolAllocator::Acquire(GPUResourceDesc desc, bool temporary)
    {
        ProfileFunction();

        auto frameIdx   = renderSystem.GetCurrentFrame();
        auto size       = renderSystem.GetAllocationSize(desc);

        const size_t requestedBlockCount = (size / blockSize) + ((size % blockSize == 0) ? 0 : 1);
        auto allocation = GetMemory(requestedBlockCount, frameIdx, Clear);

        if (allocation.offset / blockSize > blockCount) {
            FK_LOG_ERROR("MemoryPoolAllocator Allocated a block beyond range!");
            return { InvalidHandle, InvalidHandle };
        }

        if (!allocation) {
            FK_LOG_ERROR("MemoryPoolAllocator Ran out of memory!");
            return { InvalidHandle, InvalidHandle };
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
                (uint64_t)(temporary ? Temporary : Clear),
                resource });
        }
        else
            __debugbreak();

        return { resource, allocation.Overlap };
    }


    /************************************************************************************************/


    AcquireDeferredRes MemoryPoolAllocator::AcquireDeferred(GPUResourceDesc desc, bool temporary)
    {
        ProfileFunction();

        auto frameIdx   = renderSystem.GetCurrentFrame();
        auto size       = renderSystem.GetAllocationSize(desc);

        const size_t requestedBlockCount = (size / blockSize) + ((size % blockSize == 0) ? 0 : 1);
        auto allocation = GetMemory(requestedBlockCount, frameIdx, Clear);

        if (allocation.offset / blockSize > blockCount) {
            FK_LOG_ERROR("MemoryPoolAllocator Allocated a block beyond range!");
            return { InvalidHandle, InvalidHandle };
        }

        if (!allocation) {
            FK_LOG_ERROR("MemoryPoolAllocator Ran out of memory!");
            return { InvalidHandle, InvalidHandle };
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
                (uint64_t)(temporary ? Temporary : Clear),
                resource });
        }
        else
            __debugbreak();

        return { resource, allocation.Overlap, allocation.offset, heap };
    }


    /************************************************************************************************/


    AcquireResult MemoryPoolAllocator::Recycle(ResourceHandle resourceToRecycle, GPUResourceDesc desc)
    {
        ProfileFunction();

        std::scoped_lock localLock{ m };

        auto frameIdx                       = renderSystem.GetCurrentFrame();
        auto size                           = renderSystem.GetAllocationSize(desc);
        const size_t requestedBlockCount    = (size / blockSize) + (size % blockSize == 0) ? 0 : 1;

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
                    res->offset     += requestedBlockCount;
                    res->blockCount -= requestedBlockCount;
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


    void MemoryPoolAllocator::Release(ResourceHandle handle, const bool freeResourceImmedate, const bool allowImmediateReuse)
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
#endif


            freeRanges.push_back(
                {
                    res->offset,
                    res->blockCount,
                    uint64_t(res->flags == AllowReallocation ? AllowReallocation : Locked),
                    res->frameID,
                    handle
                });

            FK_LOG_9("Released Blocks %u - %u : flags[%u]", res->offset, res->offset + res->blockCount, flags);

            allocations.remove_unstable(res);
        }
    }


	/************************************************************************************************/
}//	Namespace FlexKit


/**********************************************************************

Copyright (c) 2014-2022 Robert May

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
