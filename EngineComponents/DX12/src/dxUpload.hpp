#pragma once
#include <directx/d3d12.h>
#include <RenderSystemInterface.hpp>

namespace dx_Internal
{
	using namespace FlexKit;

	struct dxUploadBuffer
	{
		dxUploadBuffer() = default;
		dxUploadBuffer(ID3D12Device* pDevice);

		dxUploadBuffer(dxUploadBuffer&&);
		dxUploadBuffer& operator = (dxUploadBuffer&&) noexcept;

		dxUploadBuffer(const dxUploadBuffer&) = delete;
		dxUploadBuffer& operator =	(const dxUploadBuffer&) = delete;

		~dxUploadBuffer();

		void Release();

		std::expected<UploadReservation, ReserveErrors> Reserve(const size_t size, const size_t reserveAlignement);

		ID3D12Resource* Resize(const size_t size); // Returns old resource

		ID3D12Resource* deviceBuffer = nullptr;
		size_t			position = 0;
		size_t			last = 0;
		size_t			size = 0;
		char*			buffer = nullptr;
		ID3D12Device*	parentDevice = nullptr;
	};

}
