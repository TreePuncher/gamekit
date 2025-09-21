#pragma once
#include <Directx/d3d12.h>
#include <RenderSystemInterface.hpp>

namespace dx_Internal
{
    using namespace FlexKit;

	class dxIndirectLayout : public IIndirectLayout
	{
	public:
		dxIndirectLayout() noexcept :
			entries		{ nullptr },
			signature	{ nullptr } {
		}

		dxIndirectLayout(ID3D12CommandSignature* IN_signature, size_t IN_stride, Vector<IndirectDrawDescription>&& IN_Entries) noexcept :
			signature	{ IN_signature },
			stride		{ IN_stride },
			entries		{ std::move(IN_Entries) } {
		}

		~dxIndirectLayout() noexcept
		{
			if (signature)
				signature->Release();
		}

		dxIndirectLayout(const dxIndirectLayout& rhs) noexcept :
			signature	{ rhs.signature },
			entries		{ rhs.entries },
			stride		{ rhs.stride }
		{
			if (signature)
				signature->AddRef();
		}

		dxIndirectLayout& operator = (const dxIndirectLayout& rhs) noexcept
		{
			if (rhs.signature)
				rhs.signature->AddRef();

			if (signature)
				signature->Release();

			signature = rhs.signature;
			stride = rhs.stride;
			entries = rhs.entries;

			return (*this);
		}

		operator bool() noexcept { return signature != nullptr; }

		static			dxIndirectLayout& GetImpl(IndirectLayout&);
		static const	dxIndirectLayout& GetImpl(const IndirectLayout&);

		ID3D12CommandSignature*			signature = nullptr;
		size_t							stride = 0;
		Vector<IndirectDrawDescription>	entries;
	};
}
