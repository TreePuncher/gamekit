#pragma once
#include <directx/d3d12.h>
#include <RenderSystemInterface.hpp>

namespace dx_Internal
{
	typedef FlexKit::FrameBufferedObject<ID3D12Resource>	FrameBufferedResource;
	typedef	FlexKit::FrameBufferedObject<ID3D12QueryHeap>	QueryResource;
	typedef FrameBufferedResource							IndexBuffer;
	typedef FrameBufferedResource							ConstantBuffer;
	typedef FrameBufferedResource							ShaderResourceBuffer;
	typedef FrameBufferedResource							StreamOutBuffer;
}
