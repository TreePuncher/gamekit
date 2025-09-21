#include "dxDescriptorHeaps.hpp"
#include "dxContext.hpp"
#include "dxRenderSystem.hpp"
#include <PushBuffers.hpp>


namespace dx_Internal
{
	using namespace FlexKit;
	using FlexKit::IContext;

	DescriptorHeapImpl::DescriptorHeapImpl(IContext& ictx, const DesciptorHeapLayout& Layout_IN, iAllocator& TempMemory) :
		FillState(TempMemory)
	{
		auto& ctx = static_cast<dxDirectContext&>(ictx);

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


	void DescriptorHeapImpl::Init(IContext& ictx, const DesciptorHeapLayout& Layout_IN, iAllocator& TempMemory)
	{
		auto& ctx = static_cast<dxDirectContext&>(ictx);

		FK_ASSERT(TempMemory);
		FillState = Vector<bool>(TempMemory);

		const size_t EntryCount	= Layout_IN.size();
		descriptorHeap			= ctx._ReserveSRV(EntryCount).value();
		Layout					= &Layout_IN;

		for (size_t I = 0; I < EntryCount; I++)
			FillState.push_back(false);
	}


	/************************************************************************************************/


	void DescriptorHeapImpl::Init(IContext& ictx, const DesciptorHeapLayout& Layout_IN, const size_t reserveCount, iAllocator& TempMemory)
	{
		auto& ctx = static_cast<dxDirectContext&>(ictx);

		FK_ASSERT(TempMemory);
		FillState = Vector<bool>(TempMemory);

		const size_t EntryCount = Layout_IN.size() * reserveCount;
		descriptorHeap = ctx._ReserveSRV(EntryCount).value();
		Layout = &Layout_IN;

		for (size_t I = 0; I < EntryCount; I++)
			FillState.push_back(false);
	}


	/************************************************************************************************/


	void DescriptorHeapImpl::Init2(IContext& ictx, const DesciptorHeapLayout& Layout_IN, const size_t reserveCount, iAllocator& TempMemory)
	{
		auto& ctx = static_cast<dxDirectContext&>(ictx);

		FK_ASSERT(TempMemory);
		FillState = Vector<bool>(TempMemory, reserveCount);

		descriptorHeap = ctx._ReserveSRV(reserveCount).value();
		Layout = &Layout_IN;

		for (size_t I = 0; I < reserveCount; I++)
			FillState.push_back(false);
	}



	/************************************************************************************************/


	void DescriptorHeapImpl::NullFill(IContext& ictx, const size_t end)
	{
		auto& ctx = static_cast<dxDirectContext&>(ictx);

		auto& Entries = Layout->entries;
		for (size_t I = 0, Idx = 0; I < Entries.size(); I++)
		{
			auto& e = Entries[I];
			//
			for (size_t II = 0; II < e.Count + e.Space; II++)
			{
				if (I + II > end)
					return;

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
	}


	/************************************************************************************************/


	void DescriptorHeapImpl::SetSRV(IContext& ictx, size_t idx, ResourceHandle handle)
	{
		auto& ctx = static_cast<dxDirectContext&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetSRV(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return;
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
	}


	/************************************************************************************************/


	void DescriptorHeapImpl::SetSRVCubemap(IContext& ictx, size_t idx, ResourceHandle	handle)
	{
		dxDirectContext& ctx = static_cast<dxDirectContext&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetSRVCubemap(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return;
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
	}


	/************************************************************************************************/


	void DescriptorHeapImpl::SetSRVCubemap(IContext& ictx, size_t idx, ResourceHandle	handle, DeviceFormat format)
	{
		auto& ctx = static_cast<dxDirectContext&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetSRVCubemap(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return;
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
	}



	/************************************************************************************************/


	void DescriptorHeapImpl::SetSRV(IContext& ictx, size_t idx, ResourceHandle handle, DeviceFormat format)
	{
		dxDirectContext& ctx = static_cast<dxDirectContext&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetSRV(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return;
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
	}


	/************************************************************************************************/


	void DescriptorHeapImpl::SetSRV(IContext& ictx, size_t idx, ResourceHandle handle, uint MipOffset, DeviceFormat format)
	{
		auto& ctx = static_cast<dxDirectContext&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetSRV(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return;
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
	}


	/************************************************************************************************/


	void DescriptorHeapImpl::SetSRVArray(IContext& ictx, size_t idx, ResourceHandle handle, DeviceFormat format)
	{
		dxDirectContext& ctx = static_cast<dxDirectContext&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetSRV(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return;
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
	}


	/************************************************************************************************/


	void DescriptorHeapImpl::SetSRV3D(IContext& ictx, size_t idx, ResourceHandle handle)
	{
		auto& ctx = static_cast<dxDirectContext&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetSRV3D(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return;
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
	}


	/************************************************************************************************/


	void DescriptorHeapImpl::SetCBV(IContext& ictx, size_t idx, const ConstantBufferDataSet& constants)
	{
		dxDirectContext& ctx = static_cast<dxDirectContext&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::ConstantBuffer, idx))
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetCBV(%u, %u, %u): Failed to set descriptor!", idx, constants.Handle().to_uint(), constants.Offset());
			return;
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
	}


	/************************************************************************************************/


	void DescriptorHeapImpl::SetCBV(IContext& ictx, size_t idx, ConstantBufferHandle handle, size_t offset, size_t bufferSize)
	{
		auto& ctx = static_cast<dxDirectContext&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::ConstantBuffer, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetCBV(%u, %u, %u): Failed to set descriptor!", idx, handle, offset);
			return;
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
	}


	/************************************************************************************************/


	void DescriptorHeapImpl::SetCBV(IContext& ictx, size_t idx, ResourceHandle	handle, size_t offset, size_t bufferSize)
	{
		dxDirectContext& ctx = static_cast<dxDirectContext&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::ConstantBuffer, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetCBV(%u, %u, %u): Failed to set descriptor!", idx, handle.to_uint(), offset);
			return;
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

		return;
	}


	/************************************************************************************************/


	void DescriptorHeapImpl::SetUAVBuffer(IContext& ictx, size_t idx, ResourceHandle handle, size_t offset)
	{
		auto& ctx = static_cast<dxDirectContext&>(ictx);

		FK_ASSERT(idx < std::numeric_limits<uint32_t>::max());
		FK_ASSERT(offset < std::numeric_limits<uint32_t>::max());

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetUAVBuffer(%u, %u, %u): Failed to set descriptor!", idx, handle.to_uint(), offset);
			return;
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

		return;
	}
	

	/************************************************************************************************/


	void DescriptorHeapImpl::SetUAVTexture(IContext& ictx, size_t idx, ResourceHandle handle)
	{
		dxDirectContext& ctx = static_cast<dxDirectContext&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetUAVTexture(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return;
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
	}


	/************************************************************************************************/


	void DescriptorHeapImpl::SetUAVTexture(IContext& ictx, size_t idx, ResourceHandle handle, DeviceFormat format)
	{
		auto& ctx = static_cast<dxDirectContext&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetUAVTexture(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return;
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
	}


	/************************************************************************************************/


	void DescriptorHeapImpl::SetUAVTexture(IContext& ictx, size_t idx, size_t mipLevel, ResourceHandle handle, DeviceFormat format)
	{
		dxDirectContext& ctx = static_cast<dxDirectContext&>(ictx);

		FK_ASSERT(idx < std::numeric_limits<uint32_t>::max());
		FK_ASSERT(mipLevel < std::numeric_limits<uint32_t>::max());

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetUAVTexture(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return;
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
	}


	/************************************************************************************************/


	void DescriptorHeapImpl::SetUAVCubemap(IContext& ictx, size_t idx, ResourceHandle	handle)
	{
		auto& ctx = static_cast<dxDirectContext&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetUAVCubemap(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return;
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
	}


	/************************************************************************************************/


	void DescriptorHeapImpl::SetUAVTexture3D(IContext& ictx, size_t idx, ResourceHandle handle, DeviceFormat format)
	{
		dxDirectContext& ctx = static_cast<dxDirectContext&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetUAVTexture3D(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return;
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
	}


	/************************************************************************************************/


	void DescriptorHeapImpl::SetUAVStructured(IContext& ictx, size_t idx, ResourceHandle handle, size_t stride, size_t offset)
	{
		auto& ctx = static_cast<dxDirectContext&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetUAVStructured(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return;
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
	}


	
	/************************************************************************************************/


	void DescriptorHeapImpl::SetUAVStructured(
		IContext&		ictx,
		size_t			idx,
		ResourceHandle	resource,
		ResourceHandle	counter,
		size_t			stride,
		size_t			counterOffset)
	{
		auto& ctx = static_cast<dxDirectContext&>(ictx);

		FK_ASSERT(idx < std::numeric_limits<uint32_t>::max());
		FK_ASSERT(stride < std::numeric_limits<uint32_t>::max());
		FK_ASSERT(counterOffset < std::numeric_limits<uint32_t>::max());

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx) || resource == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeap::SetUAVStructured(%u, %u): Failed to set descriptor!", idx, resource.to_uint());
			return;
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
	}


	/************************************************************************************************/


    void DescriptorHeapImpl::SetStructuredResource(IContext& ictx, size_t idx, ResourceHandle handle, size_t stride, size_t offset)
	{
		auto& ctx = static_cast<dxDirectContext&>(ictx);

#if USING(DEBUGGRAPHICS)
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx) || handle == InvalidHandle)
		{
			FK_LOG_ERROR("DescriptorHeapImpl::SetStructuredResource(%u, %u): Failed to set descriptor!", idx, handle.to_uint());
			return;
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
		}
	}


	/************************************************************************************************/


	DescriptorHeap DescriptorHeapImpl::GetHeapOffsetted(size_t offset, IContext& ictx) const
	{
		auto& ctx = static_cast<dxDirectContext&>(ictx);

		DescriptorHeap subHeap = Clone();
		DescriptorHeapImpl& impl = GetImpl(subHeap);

		impl.descriptorHeap = IncrementHeapPOS(
										descriptorHeap,
										ctx.renderSystem->DescriptorCBVSRVUAVSize,
										offset);

		return subHeap;
	}


	DescriptorHeapImpl& DescriptorHeapImpl::GetImpl(DescriptorHeap& abstractHeap) noexcept
	{
		return *std::launder<DescriptorHeapImpl>((DescriptorHeapImpl*)abstractHeap.internal);
	}


	const DescriptorHeapImpl& DescriptorHeapImpl::GetImpl(const DescriptorHeap& abstractHeap) noexcept
	{
		return *std::launder<const DescriptorHeapImpl>((const DescriptorHeapImpl*)abstractHeap.internal);
	}

	//void DescriptorHeap::Mirror(const DescriptorHeap& rhs);
	//DescriptorHeap DescriptorHeap::Clone() const;

	DescriptorHeapImpl::operator DescriptorRange() const noexcept
	{
		return {
			.begin		= descriptorHeap,
			.size		= static_cast<uint32_t>(FillState.size()),
			.stride		= static_cast<uint32_t>(RenderSystem::_GetInstance().DescriptorCBVSRVUAVSize)
		};
	}


	bool DescriptorHeapImpl::CheckType(const DesciptorHeapLayout& layout, DescHeapEntryType type, size_t idx)
	{
		size_t entryIdx = 0;
		for (HeapDescriptor entry : layout.entries)
		{
			if ((entry.Type == type)	&& 
				(entryIdx <= idx)		&&
				(entryIdx + entry.Space + entry.Count > idx))
				return true;

			entryIdx += entry.Count + entry.Space;
		}

		return false;
	}


}
