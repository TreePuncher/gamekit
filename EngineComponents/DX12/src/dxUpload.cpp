#include "dxUpload.hpp"
#include <directx/d3dx12.h>
#include "dxRenderSystem.hpp"

namespace dx_Internal
{


	dxUploadBuffer::dxUploadBuffer(ID3D12Device* pDevice) :
		parentDevice	{ pDevice },
		size			{ MEGABYTE * 16 }
	{
		D3D12_RESOURCE_DESC   Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(size);
		D3D12_HEAP_PROPERTIES HEAP_Props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		ID3D12Resource* temp = nullptr;

		HRESULT HR = pDevice->CreateCommittedResource(
			&HEAP_Props,
			D3D12_HEAP_FLAG_NONE,
			&Resource_DESC,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&temp));

		if (FAILED(HR))
			FK_LOG_ERROR("Failed to create upload buffer!");

		SETDEBUGNAME(deviceBuffer, __func__);
		deviceBuffer = temp;

		CD3DX12_RANGE Range(0, 0);
		HR = deviceBuffer->Map(0, &Range, (void**)&buffer); CheckHR(HR, ASSERTONFAIL("FAILED TO MAP TEMP BUFFER"));
	}


	/************************************************************************************************/


	void dxUploadBuffer::Release()
	{
		if (!deviceBuffer)
			return;

		deviceBuffer->Unmap(0, nullptr);
		deviceBuffer->Release();

		position = 0;
		size = 0;
		deviceBuffer = nullptr;
		buffer = nullptr;
	}


	/************************************************************************************************/


	dxUploadBuffer::dxUploadBuffer(dxUploadBuffer&& rhs)
	{
		Release();

		position = rhs.position;
		size = rhs.size;
		deviceBuffer = rhs.deviceBuffer;
		buffer = rhs.buffer;
		parentDevice = rhs.parentDevice;

		rhs.position = 0;
		rhs.size = 0;
		rhs.deviceBuffer = nullptr;
		rhs.buffer = nullptr;
		rhs.parentDevice = nullptr;
	}


	dxUploadBuffer& dxUploadBuffer::operator = (dxUploadBuffer&& rhs) noexcept
	{
		Release();

		position = rhs.position;
		size = rhs.size;
		deviceBuffer = rhs.deviceBuffer;
		buffer = rhs.buffer;
		parentDevice = rhs.parentDevice;

		rhs.position = 0;
		rhs.size = 0;
		rhs.deviceBuffer = nullptr;
		rhs.buffer = nullptr;
		rhs.parentDevice = nullptr;

		return *this;
	}


	dxUploadBuffer::~dxUploadBuffer()
	{
		Release();
	}


	/************************************************************************************************/


	std::expected<UploadReservation, ReserveErrors> dxUploadBuffer::Reserve(const size_t reserveSize, const size_t alignment)
	{
		// Not enough remaining Space in buffer GOTO Beginning if space in front of upload buffer is available
		if (position + reserveSize > size && last != 0)
			position = 0;

		auto GetOffset = [&]() {
			auto offset = alignment - (position & (alignment - 1));
			return (offset == alignment) ? 0 : offset;
			};

		// buffer too small
		if (position + reserveSize + GetOffset() > size)
			return std::unexpected{ ReserveErrors::OutOfSpace };

		if (last > position)
		{	// Potential Overlap condition
			if (position + reserveSize + GetOffset() >= last)
				return std::unexpected{ ReserveErrors::OutOfSpace };  // Resize buffer and then upload

			const auto alignmentOffset = GetOffset();
			char*			allocation	= buffer + position + alignmentOffset;
			const size_t	offset		= position + alignmentOffset;

			position += reserveSize + alignmentOffset;

			return UploadReservation{
				.resource = deviceBuffer,
				.size = reserveSize,
				.offset = offset,
				.buffer = allocation,
			};
		}

		if (last <= position)
		{	// Safe, Do Upload
			const auto alignmentOffset = GetOffset();

			char*	alllocation = buffer + position + alignmentOffset;
			size_t	offset		= position + alignmentOffset;
			position += reserveSize + alignmentOffset;

			return UploadReservation{
				.resource = deviceBuffer,
				.size = reserveSize,
				.offset = offset,
				.buffer = alllocation,
			};
		}

		return std::unexpected{ ReserveErrors::Unknown };
	}


	/************************************************************************************************/


	ID3D12Resource* dxUploadBuffer::Resize(const size_t newSize)
	{
		if (deviceBuffer)
			deviceBuffer->Unmap(0, 0);

		auto previousBuffer = deviceBuffer;

		D3D12_RESOURCE_DESC   Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(newSize);
		D3D12_HEAP_PROPERTIES HEAP_Props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		ID3D12Resource* newDeviceBuffer = nullptr;

		HRESULT HR = parentDevice->CreateCommittedResource(
			&HEAP_Props,
			D3D12_HEAP_FLAG_NONE,
			&Resource_DESC,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&newDeviceBuffer));

		position = 0;
		last = 0;
		size = newSize;
		deviceBuffer = newDeviceBuffer;
		SETDEBUGNAME(newDeviceBuffer, "TEMPORARY");

		CD3DX12_RANGE Range(0, 0);
		HR = newDeviceBuffer->Map(0, &Range, (void**)&buffer);   CheckHR(HR, ASSERTONFAIL("FAILED TO MAP TEMP BUFFER"));

		return previousBuffer;
	}
}
