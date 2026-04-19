#include "vkVertexBufferSet.hpp"
#include "vkRenderSystem.hpp"

namespace VK_internal
{
	using namespace FlexKit;

	struct vkVertexBufferSet : public FlexKit::IVertexBufferSet
	{
		vkVertexBufferSet(iAllocator* IN_allocator) :
			apibufferObjects	{ IN_allocator },
			bufferDescriptions	{ IN_allocator },
			allocator			{ IN_allocator } {}

		void						Clear() final;
		std::optional<VertexBuffer> Find(VERTEXBUFFER_TYPE) const final;
		std::optional<uint32_t>		FindIdx(VERTEXBUFFER_TYPE) const final;
		const VertexBuffer			operator []		(uint8_t idx) const final;
		uint8_t						GetIndexBufferIndex() const final;

		void CreateBuffer(VERTEXBUFFER_TYPE, VERTEXBUFFER_FORMAT, size_t byteSize) final;
		void ReleaseBuffer(VERTEXBUFFER_TYPE) final;

		void Release() final;

		struct BufferDescription
		{
			VERTEXBUFFER_TYPE	type;
			VERTEXBUFFER_FORMAT	format;
			uint32_t			byteSize;
		};

		Vector<BufferAPIObject>		apibufferObjects;
		Vector<BufferDescription>	bufferDescriptions;

		iAllocator* allocator = nullptr;
	};

	IVertexBufferSet& CreateVertexBufferSet(iAllocator* allocator)
	{
		return allocator->allocate<vkVertexBufferSet>(allocator);
	}

	void vkVertexBufferSet::Clear()
	{
		auto& vkRS = static_cast<vkRenderSystem&>(IRenderSystem::GetInstance());
		for (auto& apiObjects : apibufferObjects)
		{
			vkDestroyBuffer(vkRS.device, apiObjects.buffer, nullptr);
			vkRS.memoryAllocator.Release(apiObjects.memory);
		}
	}

	std::optional<VertexBuffer> vkVertexBufferSet::Find(VERTEXBUFFER_TYPE type) const
	{
		for (auto [idx, descriptor] : enumerate(bufferDescriptions))
		{
			if (descriptor.type == type)
				return VertexBuffer{
					.byteSize	= descriptor.byteSize,
					.byteStride	= (uint32_t)descriptor.format,
					.resource	= apibufferObjects[idx].buffer,
					.type		= type
				};
		}
		return {};
	}

	std::optional<uint32_t> vkVertexBufferSet::FindIdx(VERTEXBUFFER_TYPE) const
	{
		return {};
	}

	const VertexBuffer vkVertexBufferSet::operator [] (uint8_t idx) const
	{
		auto& description = bufferDescriptions[idx];

		VertexBuffer out{
			.byteSize	= description.byteSize,			//uint32_t			
			.byteStride = (uint32_t)description.format, //uint32_t			
			.resource	= apibufferObjects[idx].buffer, //DeviceResource_ptr
			.type		= description.type				//VERTEXBUFFER_TYPE	
		};

		return out;
	}

	uint8_t vkVertexBufferSet::GetIndexBufferIndex() const
	{
		for (size_t i = 0; i < bufferDescriptions.size(); i++)
			if (bufferDescriptions[i].type == VERTEXBUFFER_TYPE::INDEX)
				return i;

		return -1;
	}

	void vkVertexBufferSet::CreateBuffer(VERTEXBUFFER_TYPE type, VERTEXBUFFER_FORMAT format, size_t byteSize)
	{
		auto& vkRS = static_cast<vkRenderSystem&>(IRenderSystem::GetInstance());
		auto res = CreateVertexBuffer(vkRS, byteSize, true);

		if (res.has_value())
		{
			auto buffer = res.value();
			apibufferObjects.push_back(buffer);
			bufferDescriptions.emplace_back(type, format, (uint32_t)byteSize);

#ifdef _DEBUG
			if(vkSetDebugUtilsObjectName)
			{
				static int n = 0;

				auto name = std::format("VertexBuffer_{}", n++);

				VkDebugUtilsObjectNameInfoEXT nameInfo{
					.sType			= VkStructureType::VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
					.pNext			= nullptr,
					.objectType		= VkObjectType::VK_OBJECT_TYPE_BUFFER,
					.objectHandle	= (uint64_t)buffer.buffer,
					.pObjectName	= name.c_str()
				};

				vkSetDebugUtilsObjectName(vkRS.device, &nameInfo);
			}
#endif
		}
	}

	void vkVertexBufferSet::ReleaseBuffer(VERTEXBUFFER_TYPE)
	{

	}

	void vkVertexBufferSet::Release()
	{
		Clear();
		allocator->release(*this);
	}
}
