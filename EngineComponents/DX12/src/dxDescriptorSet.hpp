#pragma once
#include <directx/d3d12.h>
#include <RenderSystemInterface.hpp>

namespace dx_Internal
{
    using namespace FlexKit;
	using FlexKit::IContext;

	class dxDescriptorSet : public IDescriptorHeap
	{
	public:
		dxDescriptorSet() = default;
		dxDescriptorSet(IContext& ctx, const DescriptorHeapLayout& Layout_IN, iAllocator& TempMemory);

		// moveable
		dxDescriptorSet(dxDescriptorSet&& rhs);
		dxDescriptorSet& operator = (dxDescriptorSet&&);

		void Init(IContext& ctx, const DescriptorHeapLayout& Layout_IN, iAllocator& TempMemory);
		void Init(IContext& ctx, const DescriptorHeapLayout& Layout_IN, const size_t reserveCount, iAllocator& TempMemory);
		void Init2(IContext& ctx, const DescriptorHeapLayout& Layout_IN, const size_t reserveCount, iAllocator& TempMemory); // for variable size heap v
		void NullFill(IContext& ctx, const size_t end = -1);

		void SetCBV(IContext& ctx, size_t idx, const ConstantBufferDataSet& constants);
		void SetCBV(IContext& ctx, size_t idx, ConstantBufferHandle, size_t offset, size_t bufferSize);
		void SetCBV(IContext& ctx, size_t idx, ResourceHandle, size_t offset, size_t bufferSize);

		void SetSRV(IContext& ctx, size_t idx, ResourceHandle);
		void SetSRV(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format);
		void SetSRV(IContext& ctx, size_t idx, ResourceHandle, uint MipOffset, DeviceFormat format);
		void SetSRVArray(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format);

		void SetSRV3D(IContext& ctx, size_t idx, ResourceHandle);

		void SetSRVCubemap(IContext& ctx, size_t idx, ResourceHandle		Handle);
		void SetSRVCubemap(IContext& ctx, size_t idx, ResourceHandle		Handle, DeviceFormat format);

		void SetUAVBuffer(IContext& ctx, size_t idx, ResourceHandle, size_t   offset = 0);

		void SetUAVTexture(IContext& ctx, size_t idx, ResourceHandle);

		void SetUAVTexture(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format);
		void SetUAVTexture(IContext& ctx, size_t idx, size_t mipLevel, ResourceHandle, DeviceFormat format);

		void SetUAVCubemap(IContext& ctx, size_t idx, ResourceHandle handle);

		void SetUAVTexture3D(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format);

		void SetUAVStructured(IContext& ctx, size_t idx, ResourceHandle, size_t stride, size_t offset = 0);
		void SetUAVStructured(IContext& ctx, size_t idx, ResourceHandle resource, ResourceHandle counter, size_t stride, size_t Offset);

		void SetStructuredResource(IContext& ctx, size_t idx, ResourceHandle, size_t stride = 4, size_t offset = 0); //

		operator D3D12_GPU_DESCRIPTOR_HANDLE	() const { return { descriptorHeap.V2 }; } // TODO: FIX PAIRS SO AUTO CASTING WORKS
		operator GPUDescriptorHandle			() const { return descriptorHeap.V2; }

		DescriptorSet	GetHeapOffsetted(size_t offset, IContext& ctx) const;

		void Mirror(const DescriptorSet& rhs);
		operator DescriptorRange() const noexcept;

		static			dxDescriptorSet& GetImpl(DescriptorSet&) noexcept;
		static const	dxDescriptorSet& GetImpl(const DescriptorSet&) noexcept;
	private:

		DescriptorSet Clone() const { return {}; }


		static bool CheckType(const DescriptorHeapLayout& layout, DescHeapEntryType type, size_t idx);

		DescHeapPOS						descriptorHeap;
		const DescriptorHeapLayout* Layout;
		Vector<bool>					FillState;
	};
}
