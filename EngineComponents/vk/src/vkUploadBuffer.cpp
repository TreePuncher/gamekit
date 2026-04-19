#include "vkUploadBuffer.hpp"
#include "vkRenderSystem.hpp"

#include <RenderSystemInterface.hpp>

namespace VK_internal
{
	using namespace FlexKit;

	/************************************************************************************************/


	vkUploadBuffer::vkUploadBuffer() :
		size			{ MEGABYTE * 64 }
	{
		auto& vkRS = static_cast<vkRenderSystem&>(IRenderSystem::GetInstance());
		auto res = CreateUploadBuffer(vkRS, size);

		if (!res)
			throw std::runtime_error{ "VK: Failed to create upload buffer!" };

		auto& [newBuffer, newMemory, offset] = res.value();

		auto mapped_ptr = vkRS.MapDeviceAddress(newMemory, offset);
		deviceBuffer	= newBuffer;
		memory			= newMemory;
		buffer			= (char*)mapped_ptr;

#ifdef _DEBUG
		static int n = 0;
		auto name = std::format("UploadBuffer_{}, offset:{}", n++, offset);
		LabelBuffer(deviceBuffer, vkRS.device, name.c_str());
#endif
	}


	/************************************************************************************************/


	void vkUploadBuffer::Release()
	{
		if (!deviceBuffer)
			return;

		// TODO: Vulkanize
		//deviceBuffer->Unmap(0, nullptr);
		//deviceBuffer->Release();

		position = 0;
		size = 0;
		deviceBuffer = nullptr;
		buffer = nullptr;
	}


	/************************************************************************************************/


	vkUploadBuffer::vkUploadBuffer(vkUploadBuffer&& rhs)
	{
		Release();

		position = std::exchange(rhs.position, 0);
		size = std::exchange(rhs.size, 0);
		deviceBuffer = std::exchange(rhs.deviceBuffer, nullptr);
		memory = std::exchange(rhs.memory, nullptr);
		buffer = std::exchange(rhs.buffer, nullptr);
	}


	vkUploadBuffer& vkUploadBuffer::operator = (vkUploadBuffer&& rhs) noexcept
	{
		Release();

		position = rhs.position;
		size = rhs.size;
		deviceBuffer = rhs.deviceBuffer;
		buffer = rhs.buffer;

		rhs.position = 0;
		rhs.size = 0;
		rhs.deviceBuffer = nullptr;
		rhs.buffer = nullptr;

		return *this;
	}


	vkUploadBuffer::~vkUploadBuffer()
	{
		Release();
	}


	/************************************************************************************************/


	std::expected<UploadReservation, ReserveErrors> vkUploadBuffer::Reserve(const size_t reserveSize, const size_t alignment)
	{
		// Not enough remaining Space in buffer GOTO Beginning if space in front of upload buffer is available
		if (position + reserveSize > size && last != 0)
			position = 0;

		auto GetOffset = [&]() {
				auto offset = alignment - (position & (alignment - 1));
				return (offset == alignment) ? 0 : offset;
			};

		// buffer too Small
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

			char*	allocation	= buffer + position + alignmentOffset;
			size_t	offset		= position + alignmentOffset;
			position += reserveSize + alignmentOffset;

			return UploadReservation{
				.resource	= deviceBuffer,
				.size		= reserveSize,
				.offset		= offset,
				.buffer		= allocation,
			};
		}

		return std::unexpected{ ReserveErrors::Unknown };
	}


	/************************************************************************************************/


	VkBuffer vkUploadBuffer::Resize(const size_t newSize)
	{
		auto& vkRenderSystem = static_cast<struct vkRenderSystem&>(IRenderSystem::GetInstance());

		if (deviceBuffer)
		{
			VkMemoryUnmapInfo unmapInfo{
				.sType	= VkStructureType::VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO,
				.pNext	= nullptr,
				.flags	= 0,
				.memory = memory,
			};
			vkUnmapMemory2(vkRenderSystem.device, &unmapInfo);
		}

		auto previousBuffer = deviceBuffer;
		auto previousMemory = memory;

		auto createRes = CreateUploadBuffer(vkRenderSystem, newSize);
		if (!createRes.has_value())
			throw std::runtime_error{ "VK: Failed to resize upload buffer!" };

		auto& [newBuffer, newMemory, offset] = createRes.value();

		position = 0;
		last = 0;
		size = newSize;
		deviceBuffer = newBuffer;
		memory = newMemory;

		VkMemoryMapInfo memoryMapInfo{
			.sType	= VkStructureType::VK_STRUCTURE_TYPE_MEMORY_MAP_INFO,
			.pNext	= nullptr,
			.flags	= 0,
			.memory = newMemory,
			.offset	= 0,
			.size	= VK_WHOLE_SIZE
		};

		uint64_t cpuAddress;
		vkMapMemory2(vkRenderSystem.device, &memoryMapInfo, (void**)&cpuAddress);

		return previousBuffer;
	}
}
