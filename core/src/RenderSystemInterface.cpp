#include "BuildSettings.hpp"
#include "RenderSystemInterface.hpp"
#include "TextureUtilities.hpp"
#include <new>

namespace FlexKit
{
	class DescriptorHeapImpl;

	PipelineBuilder::PipelineBuilder(IRenderSystem& renderSystem, iAllocator& allocator)
	{
		FK_ASSERT(renderSystem.CreatePipelineBuilder(implSpace, 128) == true, "Failed to create implementation of PipelineBuilder!");
	}

	PipelineBuilder::~PipelineBuilder()
	{
		GetImpl().Release();
	}

	IPipelineBuilderImpl& PipelineBuilder::AddRootSignature(const IRootSignature* rootSig)
	{
		auto& impl = GetImpl();

		impl.AddRootSignature(rootSig);
		return impl;
	}

	IPipelineBuilderImpl& PipelineBuilder::AddShaderLibrary(const char* file, const ShaderOptions& options)
	{
		auto& impl = GetImpl();

		impl.AddShaderLibrary(file, options);
		return impl;
	}

	IPipelineBuilderImpl& PipelineBuilder::AddComputeShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		auto& impl = GetImpl();

		impl.AddComputeShader(entryPoint, file, options);
		return impl;
	}

	IPipelineBuilderImpl& PipelineBuilder::AddWorkGraph(const WorkGraph_Desc& desc)
	{
		auto& impl = GetImpl();

		impl.AddWorkGraph(desc);
		return impl;
	}

	IPipelineBuilderImpl& PipelineBuilder::AddVertexShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		auto& impl = GetImpl();

		impl.AddVertexShader(entryPoint, file, options);
		return impl;
	}

	IPipelineBuilderImpl& PipelineBuilder::AddDomainShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		auto& impl = GetImpl();

		impl.AddDomainShader(entryPoint, file, options);
		return impl;
	}

	IPipelineBuilderImpl& PipelineBuilder::AddHullShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		auto& impl = GetImpl();

		impl.AddHullShader(entryPoint, file, options);
		return impl;
	}

	IPipelineBuilderImpl& PipelineBuilder::AddGeometryShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		auto& impl = GetImpl();

		impl.AddGeometryShader(entryPoint, file, options);
		return impl;
	}

	IPipelineBuilderImpl& PipelineBuilder::AddAmplificationShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		auto& impl = GetImpl();

		impl.AddGeometryShader(entryPoint, file, options);
		return impl;
	}

	IPipelineBuilderImpl& PipelineBuilder::AddMeshShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		auto& impl = GetImpl();

		impl.AddMeshShader(entryPoint, file, options);
		return impl;
	}

	IPipelineBuilderImpl& PipelineBuilder::AddPixelShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		auto& impl = GetImpl();

		impl.AddPixelShader(entryPoint, file, options);
		return impl;

	}

	IPipelineBuilderImpl& PipelineBuilder::SetDebugName(const char* name)
	{
		auto& impl = GetImpl();

		impl.SetDebugName(name);
		return impl;
	}

	IPipelineBuilderImpl& PipelineBuilder::AddInputLayout(const InputLayoutState& state)
	{
		auto& impl = GetImpl();

		impl.AddInputLayout(state);
		return impl;
	}

	IPipelineBuilderImpl& PipelineBuilder::AddInputTopology(const ETopology topology)
	{
		auto& impl = GetImpl();

		impl.AddInputTopology(topology);
		return impl;
	}

	IPipelineBuilderImpl& PipelineBuilder::AddDepthStencilState(const DepthStencilState& state)
	{
		auto& impl = GetImpl();

		impl.AddDepthStencilState(state);
		return impl;
	}

	IPipelineBuilderImpl& PipelineBuilder::AddRasterizerState(const RasterizerState& state)
	{
		auto& impl = GetImpl();
		impl.AddRasterizerState(state);
		return impl;

	}

	IPipelineBuilderImpl& PipelineBuilder::AddRenderTargetState(const RenderTargetState& state)
	{
		auto& impl = GetImpl();

		impl.AddRenderTargetState(state);
		return impl;

	}

	IPipelineBuilderImpl& PipelineBuilder::AddDepthStencilFormat(const DeviceFormat format)
	{
		auto& impl = GetImpl();

		impl.AddDepthStencilFormat(format);
		return impl;

	}

	IPipelineBuilderImpl& PipelineBuilder::AddBlendState(const BlendState& state)
	{
		auto& impl = GetImpl();

		impl.AddBlendState(state);
		return impl;
	}

	LoadPipelineStateRes PipelineBuilder::Build(IRenderSystem& renderSystem)
	{
		return {};
	}

	LoadPipelineStateRes PipelineBuilder::BuildStream(IRenderSystem& renderSystem, void* buffer, const size_t size)
	{
		return {};
	}

	IPipelineBuilderImpl& PipelineBuilder::GetImpl()
	{
		return *std::launder<IPipelineBuilderImpl>((IPipelineBuilderImpl*)implSpace);
	}


	/************************************************************************************************/


	DescriptorHeap::DescriptorHeap(IContext&, const DesciptorHeapLayout& Layout_IN, iAllocator* TempMemory)
	{
	}


	DescriptorHeap& DescriptorHeap::operator = (const DescriptorHeap&)
	{
		return *this;
	}

	// moveable
	DescriptorHeap::DescriptorHeap(DescriptorHeap&& rhs)
	{
	}


	DescriptorHeap& DescriptorHeap::operator = (DescriptorHeap&&)
	{
		return *this;
	}


	DescriptorHeap& DescriptorHeap::Init(IContext& ctx, const DesciptorHeapLayout& Layout_IN, iAllocator* TempMemory)
	{
		return *this;
	}


	DescriptorHeap& DescriptorHeap::Init(IContext& ctx, const DesciptorHeapLayout& Layout_IN, const size_t reserveCount, iAllocator* TempMemory)
	{
		return *this;
	}


	DescriptorHeap& DescriptorHeap::Init2(IContext& ctx, const DesciptorHeapLayout& Layout_IN, const size_t reserveCount, iAllocator* TempMemory)
	{
		return *this;
	}


	DescriptorHeap& DescriptorHeap::NullFill(IContext& ctx, const size_t end)
	{
		return *this;
	}


	DescriptorHeap& DescriptorHeap::SetCBV(IContext& ctx, size_t idx, const ConstantBufferDataSet& constants)
	{
		return *this;
	}


	DescriptorHeap& DescriptorHeap::SetCBV(IContext& ctx, size_t idx, ConstantBufferHandle, size_t offset, size_t bufferSize)
	{
		return *this;
	}


	DescriptorHeap& DescriptorHeap::SetCBV(IContext& ctx, size_t idx, ResourceHandle, size_t offset, size_t bufferSize)
	{
		return *this;
	}


	DescriptorHeap& DescriptorHeap::SetSRV(IContext& ctx, size_t idx, ResourceHandle)
	{
		return *this;
	}


	DescriptorHeap& DescriptorHeap::SetSRV(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format)
	{
		return *this;
	}


	DescriptorHeap& DescriptorHeap::SetSRV(IContext& ctx, size_t idx, ResourceHandle, uint MipOffset, DeviceFormat format)
	{
		return *this;
	}


	DescriptorHeap& DescriptorHeap::SetSRVArray(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format)
	{
		return *this;
	}


	DescriptorHeap& DescriptorHeap::SetSRV3D(IContext& ctx, size_t idx, ResourceHandle)
	{
		return *this;
	}


	DescriptorHeap& DescriptorHeap::SetSRVCubemap(IContext& ctx, size_t idx, ResourceHandle	Handle)
	{
		return *this;
	}


	DescriptorHeap& DescriptorHeap::SetSRVCubemap(IContext& ctx, size_t idx, ResourceHandle Handle, DeviceFormat format)
	{
		return *this;
	}


	DescriptorHeap& DescriptorHeap::SetUAVBuffer(IContext& ctx, size_t idx, ResourceHandle, size_t offset)
	{
		return *this;
	}


	DescriptorHeap& DescriptorHeap::SetUAVTexture(IContext& ctx, size_t idx, ResourceHandle)
	{
		return *this;
	}


	DescriptorHeap& DescriptorHeap::SetUAVTexture(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format)
	{
		return *this;
	}


	DescriptorHeap& DescriptorHeap::SetUAVTexture(IContext& ctx, size_t idx, size_t mipLevel, ResourceHandle, DeviceFormat format)
	{
		return *this;
	}


	DescriptorHeap& DescriptorHeap::SetUAVCubemap(IContext& ctx, size_t idx, ResourceHandle handle)
	{
		return *this;
	}


	DescriptorHeap& DescriptorHeap::SetUAVTexture3D(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format)
	{
		return *this;
	}


	DescriptorHeap& DescriptorHeap::SetUAVStructured(IContext& ctx, size_t idx, ResourceHandle, size_t stride, size_t offset)
	{
		return *this;
	}


	DescriptorHeap& DescriptorHeap::SetUAVStructured(IContext& ctx, size_t idx, ResourceHandle resource, ResourceHandle counter, size_t stride, size_t Offset)
	{
		return *this;
	}


	DescriptorHeap& DescriptorHeap::SetStructuredResource(IContext& ctx, size_t idx, ResourceHandle, size_t stride, size_t offset)
	{
		return *this;
	}


	static DescriptorHeap& GetImpl() noexcept
	{
	    
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
				desc->buffers[I].WH,
				desc->format);
		}
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
