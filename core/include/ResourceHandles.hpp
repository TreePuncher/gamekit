#pragma once

#include "Handle.hpp"
#include "type.hpp"

namespace FlexKit
{
	using CameraHandle					= Handle_t<32u, GetCRCGUID(CameraHandle)>;
	using ConstantBufferHandle			= Handle_t<32u, GetTypeGUID(ConstantBuffer)>;
	using CopyContextHandle				= Handle_t<32u, GetTypeGUID(CopyContextHandle)>;
	using DeviceHeapHandle				= Handle_t<32u, GetTypeGUID(DeviceHeapHandle)>;
	using FrameResourceHandle			= Handle_t<16u, GetTypeGUID(FrameResource)>;
	using FrameGraphNodeHandle			= Handle_t<16,	GetTypeGUID(FrameGraphNode)>;
	using LayerHandle					= Handle_t<16u, GetTypeGUID(LayerHandle)>;
	using MaterialHandle				= Handle_t<16u, GetTypeGUID(Material)>;
	using NodeHandle					= Handle_t<32u, GetTypeGUID(SceneNode)>;
	using PassHandle					= Handle_t<32u, GetTypeGUID(PassHandle)>;
	using PSOHandle						= Handle_t<32u, GetTypeGUID(PSOHandle)>;
	using QueryHandle					= Handle_t<32u, GetTypeGUID(QueryBuffer)>;
	using ReadBackResourceHandle		= Handle_t<32u, GetTypeGUID(ReadBackResourceHandle)>;
	using RigidBodyHandle				= Handle_t<16u, GetTypeGUID(RigidBodyHandle)>;
	using SignalGroupHandle				= Handle_t<16u, GetTypeGUID(SignalGroupHandle)>;
	using StreamingTexture2DHandle		= Handle_t<32u, GetTypeGUID(StreamTexture2DHandle)>;
	using ShaderResourceHandle			= Handle_t<32u, GetTypeGUID(ShaderResourceHandle)>;
	using SOResourceHandle				= Handle_t<32u, GetTypeGUID(SOResourceHandle)>;
	using StaticBodyHandle				= Handle_t<16u, GetTypeGUID(StaticBodyHandle)>;
	using ResourceHandle				= Handle_t<32u, GetTypeGUID(ResourceHandle)>;
	using RootSigHandle					= Handle_t<16u, GetTypeGUID(RootSignature)>;
	using TriMeshHandle					= Handle_t<16u, GetTypeGUID(TriMesh)>;
	using VertexBufferHandle			= Handle_t<32u, GetTypeGUID(VertexBuffer)>;

	using DevicePointer					= uint64_t;
	using CPUDescriptorHandle			= Handle_t<64, GetTypeGUID(CPUDescriptorHandle), uint64_t>;
	using GPUDescriptorHandle			= Handle_t<64, GetTypeGUID(GPUDescriptorHandle), uint64_t>;
	using DescHeapPOS					= Pair<CPUDescriptorHandle, GPUDescriptorHandle>;


	struct DeviceAddressRange
	{
		uint64_t address	= 0;
		uint64_t size		= 0;
	};


	struct DeviceAddressRangeStride
	{
		uint64_t address	= 0;
		uint64_t size		= 0;
		uint64_t stride		= 0;
	};


	struct DescriptorRange
	{
		DescHeapPOS	begin;
		uint32_t	size;
		uint32_t	stride;

		DescHeapPOS operator [](size_t idx) const noexcept
		{
			return {
				CPUDescriptorHandle{ begin.V1 + idx * stride } ,
				GPUDescriptorHandle{ begin.V2 + idx * stride } };
		}

		operator DescHeapPOS() const noexcept { return begin; }
	};

	struct GPURange
	{
		DevicePointer		devicePtr;
		uint64_t			offset;
		uint32_t			size;
		ResourceHandle		resource;

		operator DevicePointer() const noexcept { return devicePtr; }
	};
}


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
