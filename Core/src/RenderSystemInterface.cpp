#include "BuildSettings.hpp"
#include "RenderSystemInterface.hpp"

#include <ios>

#include "TextureUtilities.hpp"
#include <new>

namespace FlexKit
{
	PipelineInterfaceBuilder::PipelineInterfaceBuilder(iAllocator& allocator)
	{
	    
	}

    PipelineInterfaceBuilder::~PipelineInterfaceBuilder()
	{
	    
	}

	void PipelineInterfaceBuilder::Release()
	{
	    
	}

	bool PipelineInterfaceBuilder::SetParameterAsUINT(size_t Index, uint32_t size, uint32_t cbRegister, uint32_t registerSpace, PIPELINE AccessableStages)
	{
		return false;
	}

	bool PipelineInterfaceBuilder::SetParameterAsDescriptorTable(
		size_t index, const DescriptorHeapLayout& layout, size_t unused, PIPELINE accessableStages)
	{
		return false;
	}

	bool PipelineInterfaceBuilder::SetParameterAsCBV(
		size_t Index, size_t Register, size_t RegisterSpace,
		PIPELINE AccessableStages)
	{
		return false;
	}

	bool PipelineInterfaceBuilder::SetParameterAsUAV(
		size_t Index, size_t Register, size_t RegisterSpace,
		PIPELINE AccessableStages)
	{
		return false;
	}

	bool PipelineInterfaceBuilder::SetParameterAsSRV(
		size_t Index, size_t Register, size_t RegisterSpace,
		PIPELINE AccessableStages)
	{
		return false;
	}

	void PipelineInterfaceBuilder::Clear()
	{
	}

	IPipelineInterface* PipelineInterfaceBuilder::Build(iAllocator& TempMemory)
	{
		return nullptr;
	}

	IPipelineInterface* PipelineInterfaceBuilder::LoadSignatureFromFile(const char* dir, const char* entry, iAllocator& temp)
	{
		return nullptr;
	}

	IPipelineInterface* PipelineInterfaceBuilder::LoadSignatureFromBlob(void* _ptr, size_t size, iAllocator& temp)
	{
		return nullptr;
	}


	class DescriptorHeapImpl;


	PipelineBuilder::PipelineBuilder(IRenderSystem& renderSystem, iAllocator& allocator)
	{
		FK_ASSERT(renderSystem.CreatePipelineBuilder(implSpace, 128, allocator) == true, "Failed to create implementation of PipelineBuilder!");
	}

	PipelineBuilder::~PipelineBuilder()
	{
		GetImpl().Release();
	}

	IPipelineBuilder& PipelineBuilder::AddRootSignature(const IPipelineInterface* rootSig)
	{
		auto& impl = GetImpl();

		impl.AddRootSignature(rootSig);
		return impl;
	}

	IPipelineBuilder& PipelineBuilder::AddShaderLibrary(const char* file, const ShaderOptions& options)
	{
		auto& impl = GetImpl();

		impl.AddShaderLibrary(file, options);
		return impl;
	}

	IPipelineBuilder& PipelineBuilder::AddShaderLibrary(GUID_t guid)
	{
		auto& impl = GetImpl();

		impl.AddShaderLibrary(guid);
		return impl;
	}

	IPipelineBuilder& PipelineBuilder::AddComputeShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		auto& impl = GetImpl();

		impl.AddComputeShader(entryPoint, file, options);
		return impl;
	}

	IPipelineBuilder& PipelineBuilder::AddComputeShader(GUID_t guid)
	{
		auto& impl = GetImpl();

		impl.AddComputeShader(guid);
		return impl;
	}

	IPipelineBuilder& PipelineBuilder::AddWorkGraph(const WorkGraph_Desc& desc)
	{
		auto& impl = GetImpl();

		impl.AddWorkGraph(desc);
		return impl;
	}

	IPipelineBuilder& PipelineBuilder::AddVertexShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		auto& impl = GetImpl();

		impl.AddVertexShader(entryPoint, file, options);
		return impl;
	}

	IPipelineBuilder& PipelineBuilder::AddVertexShader(GUID_t guid)
	{
		auto& impl = GetImpl();

		impl.AddVertexShader(guid);
		return impl;
	}

	IPipelineBuilder& PipelineBuilder::AddDomainShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		auto& impl = GetImpl();

		impl.AddDomainShader(entryPoint, file, options);
		return impl;
	}

	IPipelineBuilder& PipelineBuilder::AddDomainShader(GUID_t guid)
	{
		auto& impl = GetImpl();

		impl.AddDomainShader(guid);
		return impl;
	}

	IPipelineBuilder& PipelineBuilder::AddHullShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		auto& impl = GetImpl();

		impl.AddHullShader(entryPoint, file, options);
		return impl;
	}

	IPipelineBuilder& PipelineBuilder::AddHullShader(GUID_t guid)
	{
		auto& impl = GetImpl();

		impl.AddHullShader(guid);
		return impl;
	}

	IPipelineBuilder& PipelineBuilder::AddGeometryShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		auto& impl = GetImpl();

		impl.AddGeometryShader(entryPoint, file, options);
		return impl;
	}

	IPipelineBuilder& PipelineBuilder::AddGeometryShader(GUID_t guid)
	{
		auto& impl = GetImpl();

		impl.AddGeometryShader(guid);
		return impl;
	}

	IPipelineBuilder& PipelineBuilder::AddAmplificationShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		auto& impl = GetImpl();

		impl.AddGeometryShader(entryPoint, file, options);
		return impl;
	}

	IPipelineBuilder& PipelineBuilder::AddAmplificationShader(GUID_t guid)
	{
		auto& impl = GetImpl();

		impl.AddGeometryShader(guid);
		return impl;
	}

	IPipelineBuilder& PipelineBuilder::AddMeshShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		auto& impl = GetImpl();

		impl.AddMeshShader(entryPoint, file, options);
		return impl;
	}

	IPipelineBuilder& PipelineBuilder::AddMeshShader(GUID_t guid)
	{
		auto& impl = GetImpl();

		impl.AddMeshShader(guid);
		return impl;
	}

	IPipelineBuilder& PipelineBuilder::AddPixelShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		auto& impl = GetImpl();

		impl.AddPixelShader(entryPoint, file, options);
		return impl;

	}

	IPipelineBuilder& PipelineBuilder::AddPixelShader(GUID_t guid)
	{
		auto& impl = GetImpl();

		impl.AddPixelShader(guid);
		return impl;
	}

	IPipelineBuilder& PipelineBuilder::SetDebugName(const char* name)
	{
		auto& impl = GetImpl();

		impl.SetDebugName(name);
		return impl;
	}

	IPipelineBuilder& PipelineBuilder::AddInputLayout(const InputLayoutState& state)
	{
		auto& impl = GetImpl();

		impl.AddInputLayout(state);
		return impl;
	}

	IPipelineBuilder& PipelineBuilder::AddInputTopology(const ETopology topology)
	{
		auto& impl = GetImpl();

		impl.AddInputTopology(topology);
		return impl;
	}

	IPipelineBuilder& PipelineBuilder::AddDepthStencilState(const DepthStencilState& state)
	{
		auto& impl = GetImpl();

		impl.AddDepthStencilState(state);
		return impl;
	}

	IPipelineBuilder& PipelineBuilder::AddRasterizerState(const RasterizerState& state)
	{
		auto& impl = GetImpl();
		impl.AddRasterizerState(state);
		return impl;

	}

	IPipelineBuilder& PipelineBuilder::AddRenderTargetState(const RenderTargetState& state)
	{
		auto& impl = GetImpl();

		impl.AddRenderTargetState(state);
		return impl;

	}

	IPipelineBuilder& PipelineBuilder::AddDepthStencilFormat(const DeviceFormat format)
	{
		auto& impl = GetImpl();

		impl.AddDepthStencilFormat(format);
		return impl;

	}

	IPipelineBuilder& PipelineBuilder::AddBlendState(const BlendState& state)
	{
		auto& impl = GetImpl();

		impl.AddBlendState(state);
		return impl;
	}

	LoadPipelineStateRes PipelineBuilder::Build(IRenderSystem& renderSystem, iAllocator& tempAllocator)
	{
		auto& impl = GetImpl();
		return impl.Build(renderSystem, tempAllocator);
	}

	LoadPipelineStateRes PipelineBuilder::BuildStream(IRenderSystem& renderSystem, void* buffer, const size_t size)
	{
		auto& impl = GetImpl();
		return impl.BuildStream(renderSystem, buffer, size);
	}

	IPipelineBuilder& PipelineBuilder::GetImpl()
	{
		return *std::launder<IPipelineBuilder>((IPipelineBuilder*)implSpace);
	}


	/************************************************************************************************/


	DescriptorSet::DescriptorSet(IContext& ctx, const DescriptorHeapLayout& layout_IN, iAllocator& tempMemory)
	{
		auto& instance = IRenderSystem::GetInstance();
		instance.CreateDescriptorSet(internal, sizeof(internal));

		auto& impl = GetImpl();
		impl.Init(ctx, layout_IN, tempMemory);
	}


	DescriptorSet& DescriptorSet::operator = (const DescriptorSet&)
	{
		return *this;
	}

	// moveable
	DescriptorSet::DescriptorSet(DescriptorSet&& rhs)
	{
		auto& instance = IRenderSystem::GetInstance();
		instance.CreateDescriptorSet(internal, sizeof(internal));
	}


	DescriptorSet& DescriptorSet::operator = (DescriptorSet&&)
	{
		auto& impl = GetImpl();
		return *this;
	}


	IDescriptorHeap& DescriptorSet::Init(IContext& ctx, const DescriptorHeapLayout& layout_IN, iAllocator& tempMemory)
	{
		auto& impl = GetImpl();
	    impl.Init(ctx, layout_IN, *tempMemory);

		return impl;
	}


	IDescriptorHeap& DescriptorSet::Init(IContext& ctx, const DescriptorHeapLayout& layout_IN, const size_t reserveCount, iAllocator& tempMemory)
	{
		auto& impl = GetImpl();
		impl.Init(ctx, layout_IN, reserveCount, tempMemory);

		return impl;
	}


	IDescriptorHeap& DescriptorSet::Init2(IContext& ctx, const DescriptorHeapLayout& layout_IN, const size_t reserveCount, iAllocator& tempMemory)
	{
		auto& impl = GetImpl();
		impl.Init2(ctx, layout_IN, reserveCount, tempMemory);

		return impl;
	}


	IDescriptorHeap& DescriptorSet::NullFill(IContext& ctx, const size_t end)
	{
		auto& impl = GetImpl();
		impl.NullFill(ctx, end);

		return impl;
	}


	IDescriptorHeap& DescriptorSet::SetCBV(IContext& ctx, size_t idx, const ConstantBufferDataSet& constants)
	{
		auto& impl = GetImpl();
		impl.SetCBV(ctx, idx, constants);

		return impl;
	}


	IDescriptorHeap& DescriptorSet::SetCBV(IContext& ctx, size_t idx, ConstantBufferHandle cbHandle, size_t offset, size_t bufferSize)
	{
		auto& impl = GetImpl();
		impl.SetCBV(ctx, idx, cbHandle, offset, bufferSize);

		return impl;
	}


	IDescriptorHeap& DescriptorSet::SetCBV(IContext& ctx, size_t idx, ResourceHandle handle, size_t offset, size_t bufferSize)
	{
		auto& impl = GetImpl();
		impl.SetCBV(ctx, idx, handle, offset, bufferSize);

		return impl;
	}


	IDescriptorHeap& DescriptorSet::SetSRV(IContext& ctx, size_t idx, ResourceHandle handle)
	{
		auto& impl = GetImpl();
		impl.SetSRV(ctx, idx, handle);

		return impl;
	}


	IDescriptorHeap& DescriptorSet::SetSRV(IContext& ctx, size_t idx, ResourceHandle handle, DeviceFormat format)
	{
		auto& impl = GetImpl();
		impl.SetSRV(ctx, idx, handle, format);

		return impl;
	}


	IDescriptorHeap& DescriptorSet::SetSRV(IContext& ctx, size_t idx, ResourceHandle handle, uint mipOffset, DeviceFormat format)
	{
		auto& impl = GetImpl();
		impl.SetSRV(ctx, idx, handle, mipOffset, format);

		return impl;
	}


	IDescriptorHeap& DescriptorSet::SetSRVArray(IContext& ctx, size_t idx, ResourceHandle handle, DeviceFormat format)
	{
		auto& impl = GetImpl();
		impl.SetSRVArray(ctx, idx, handle, format);

		return impl;
	}


	IDescriptorHeap& DescriptorSet::SetSRV3D(IContext& ctx, size_t idx, ResourceHandle handle)
	{
		auto& impl = GetImpl();
		impl.SetSRV3D(ctx, idx, handle);

		return impl;
	}


	IDescriptorHeap& DescriptorSet::SetSRVCubemap(IContext& ctx, size_t idx, ResourceHandle handle)
	{
		auto& impl = GetImpl();
		impl.SetSRVCubemap(ctx, idx, handle);

		return impl;
	}


	IDescriptorHeap& DescriptorSet::SetSRVCubemap(IContext& ctx, size_t idx, ResourceHandle handle, DeviceFormat format)
	{
		auto& impl = GetImpl();
		impl.SetSRVCubemap(ctx, idx, handle, format);

		return impl;
	}


	IDescriptorHeap& DescriptorSet::SetUAVBuffer(IContext& ctx, size_t idx, ResourceHandle handle, size_t offset)
	{
		auto& impl = GetImpl();
		impl.SetUAVBuffer(ctx, idx, handle, offset);

		return impl;
	}


	IDescriptorHeap& DescriptorSet::SetUAVTexture(IContext& ctx, size_t idx, ResourceHandle handle)
	{
		auto& impl = GetImpl();
		impl.SetUAVTexture(ctx, idx, handle);

		return impl;
	}


	IDescriptorHeap& DescriptorSet::SetUAVTexture(IContext& ctx, size_t idx, ResourceHandle handle, DeviceFormat format)
	{
		auto& impl = GetImpl();
		impl.SetUAVTexture(ctx, idx, handle, format);

		return impl;
	}


	IDescriptorHeap& DescriptorSet::SetUAVTexture(IContext& ctx, size_t idx, size_t mipLevel, ResourceHandle handle, DeviceFormat format)
	{
		auto& impl = GetImpl();
		impl.SetUAVTexture(ctx, idx, mipLevel, handle, format);

		return impl;
	}


	IDescriptorHeap& DescriptorSet::SetUAVCubemap(IContext& ctx, size_t idx, ResourceHandle handle)
	{
		auto& impl = GetImpl();
		impl.SetUAVCubemap(ctx, idx, handle);

		return impl;
	}


	IDescriptorHeap& DescriptorSet::SetUAVTexture3D(IContext& ctx, size_t idx, ResourceHandle handle, DeviceFormat format)
	{
		auto& impl = GetImpl();
		impl.SetUAVTexture3D(ctx, idx, handle, format);

		return impl;
	}


	IDescriptorHeap& DescriptorSet::SetUAVStructured(IContext& ctx, size_t idx, ResourceHandle handle, size_t stride, size_t offset)
	{
		auto& impl = GetImpl();
		impl.SetUAVStructured(ctx, idx, handle, stride, offset);

		return impl;
	}


	IDescriptorHeap& DescriptorSet::SetUAVStructured(IContext& ctx, size_t idx, ResourceHandle resource, ResourceHandle counter, size_t stride, size_t offset)
	{
		auto& impl = GetImpl();
		impl.SetUAVStructured(ctx, idx, resource, counter, stride, offset);

		return impl;
	}


	IDescriptorHeap& DescriptorSet::SetStructuredResource(IContext& ctx, size_t idx, ResourceHandle handle, size_t stride, size_t offset)
	{
		auto& impl = GetImpl();
	    impl.SetStructuredResource(ctx, idx, handle, stride, offset);

		return impl;
	}


	DescriptorSet	DescriptorSet::GetHeapOffsetted(size_t offset, IContext& ctx) const
	{
		DescriptorSet out;

		return out;
	}


	IDescriptorHeap& DescriptorSet::GetImpl() noexcept
	{
		return *std::launder<IDescriptorHeap>((IDescriptorHeap*)internal);
	}	

	const IDescriptorHeap& DescriptorSet::GetImpl() const noexcept
	{
		return *std::launder<const IDescriptorHeap>((const IDescriptorHeap*)internal);
	}


	/************************************************************************************************/


	void MoveBuffer2UploadBuffer(const UploadReservation& data, const std::byte* source, const size_t uploadSize)
	{
		memcpy(data.buffer, source, data.size > uploadSize ? uploadSize : data.size);
	}


	ResourceHandle MoveTextureBufferToVRAM(IRenderSystem& RS, CopyContextHandle copyHandle, TextureBuffer* buffer, DeviceFormat format)
	{
		auto textureHandle = RS.CreateGPUResource(GPUResourceDesc::ShaderResource(buffer->WH, format));
		RS.UploadTexture(textureHandle, copyHandle, buffer->Buffer, buffer->Size);
		RS.SetDebugName(textureHandle, "MoveTextureBufferToVRAM");

		return textureHandle;
	}


	ResourceHandle MoveTextureBuffersToVRAM(IRenderSystem& RS, CopyContextHandle copyHandle, TextureBuffer* buffer, size_t MIPCount, size_t resourceCount, DeviceFormat format)
	{
		FK_ASSERT(resourceCount < std::numeric_limits<uint8_t>::max());

		auto texture_desc = GPUResourceDesc::ShaderResource(buffer[0].WH, format, (uint8_t)resourceCount);
		texture_desc.initialLayout = DeviceLayout::Common;

		auto textureHandle = RS.CreateGPUResource(texture_desc);
		RS.UploadTexture(textureHandle, copyHandle, buffer, resourceCount);
		RS.SetDebugName(textureHandle, "MoveTextureBuffersToVRAM");

		return textureHandle;
	}


	ResourceHandle MoveTextureBuffersToVRAM(IRenderSystem& RS, CopyContextHandle copyHandle, TextureBuffer* buffer, size_t resourceCount, DeviceFormat format)
	{
		FK_ASSERT(resourceCount < std::numeric_limits<uint8_t>::max());

		auto texture_desc = GPUResourceDesc::ShaderResource(buffer[0].WH, format, (uint8_t)resourceCount);
		texture_desc.initialLayout = DeviceLayout::Common;

		auto textureHandle = RS.CreateGPUResource(texture_desc);
		RS.UploadTexture(textureHandle, copyHandle, buffer, resourceCount);
		RS.SetDebugName(textureHandle, "MoveTextureBuffersToVRAM");

		return textureHandle;
	}

	ResourceHandle MoveBufferToDevice(IRenderSystem& RS, const char* buffer, const size_t byteSize, CopyContextHandle copyCtx)
	{
		FK_ASSERT(byteSize < std::numeric_limits<uint32_t>::max());

		auto bufferResource = RS.CreateGPUResource(GPUResourceDesc::StructuredResource((uint32_t)byteSize));
		UploadReservation upload = RS.ReserveUploadBuffer(byteSize, copyCtx);
		MoveBuffer2UploadBuffer(upload, (const std::byte*)buffer, byteSize);

		auto deviceResource = RS.GetDeviceResource(bufferResource);
		auto& ctx = RS.GetCopyContext(copyCtx);

		ctx.CopyBuffer(bufferResource, 0, upload);

		return bufferResource;
	}


	ResourceHandle LoadTexture(IRenderSystem& RS, TextureBuffer* Buffer, CopyContextHandle handle, iAllocator* Memout, DeviceFormat format)
	{
		GPUResourceDesc GPUResourceDesc = GPUResourceDesc::ShaderResource(Buffer->WH, format);
		GPUResourceDesc.initial = Buffer->Buffer;
		GPUResourceDesc.initialLayout = DeviceLayout::Common;


		auto texture = RS.CreateGPUResource(GPUResourceDesc);
		SubResourceUpload_Desc desc = {};
		desc.buffers = Buffer;
		desc.subResourceCount = 1;
		desc.subResourceStart = 0;
		desc.format = format;

		UpdateSubResourceByUploadQueue(
			RS,
			handle,
			texture,
			&desc);

		RS.SetDebugName(texture, "LOADTEXTURE");

		return texture;
	}


	void UpdateSubResourceByUploadQueue(IRenderSystem& RS, CopyContextHandle uploadHandle, ResourceHandle destinationResource, SubResourceUpload_Desc* desc)
	{
		auto& copyCtx = RS.GetCopyContext(uploadHandle);

		for (size_t I = 0; I < desc->subResourceCount; ++I)
		{
			const auto region = copyCtx.Reserve(desc->buffers[I].Size, 512);

			memcpy(
				(char*)region.buffer,
				(char*)desc->buffers[I].Buffer,
				desc->buffers[I].Size);

			copyCtx.CopyTextureRegion(
				destinationResource,
				I,
				{ 0, 0, 0 },
				region,
				desc->buffers[I].WH);
		}
	}


	/************************************************************************************************/


	UniqueResourceHandle::UniqueResourceHandle(ResourceHandle IN_handle) :
		handle{ IN_handle } {}


	UniqueResourceHandle::UniqueResourceHandle(UniqueResourceHandle&& IN_handle)
	{
		handle = IN_handle;
		IN_handle.handle = InvalidHandle;
	}


	UniqueResourceHandle& UniqueResourceHandle::operator = (UniqueResourceHandle&& rhs)
	{
		handle = rhs;

		return *this;
	}


	UniqueResourceHandle::~UniqueResourceHandle()
	{
		if (handle)
			IRenderSystem::GetInstance().ReleaseResource(handle);
	}


	ResourceHandle UniqueResourceHandle::Get() const noexcept
	{
		return handle;
	}


	UniqueResourceHandle::operator ResourceHandle () const noexcept
	{
	    return Get();
	}


	UniqueResourceHandle::operator bool() const noexcept
	{
		return handle;
	}


}	/************************************************************************************************/

/**********************************************************************

Copyright (c) 2015 - 2025 Robert May

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
