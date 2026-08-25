#pragma once
#include <variant>

#include "Assets.hpp"
#include "Components.hpp"
#include "MathUtilities.hpp"
#include "ResourceHandles.hpp"
#include "RuntimeComponentIDs.hpp"
#include "TextureManager.hpp"


namespace FlexKit
{   /************************************************************************************************/


	class TextureStreamingEngine;


	/************************************************************************************************/


	struct MaterialProperty
	{
		MaterialProperty() = default;

		template<typename TY>
		MaterialProperty(uint32_t IN_ID, const TY& IN_value) :
			ID		{ IN_ID },
			value	{ IN_value } {}

		using ValueVarient = std::variant<float, float2, float3, float4, uint, uint2, uint3, uint4, ResourceHandle, DescriptorRange, DevicePointer>;

		uint32_t		ID = -1;
		ValueVarient	value;
	};

	template<typename TY>
	concept MaterialValue =
		requires(TY ty)
		{
			{ MaterialProperty::ValueVarient{ ty } };
		};


	enum EMaterialFlags
	{
	    Clear			= 0,
		PropertyChanged = 0x01,
	};

	struct MaterialComponentData
	{
		uint32_t								refCount;
		MaterialHandle							handle;
		MaterialHandle							parent;
		DescriptorRange							textureDescriptors;
		uint64_t								lastUsed	= -1u;
		uint64_t								lastChanged = 0;

		Vector<PassHandle, 4, uint8_t>			passes;
		Vector<ResourceHandle, 0, uint8_t>		textures;
		Vector<GUID_t, 0, uint8_t>				textureAssets;
		Vector<MaterialHandle, 0, uint8_t>		subMaterials;
		
		Vector<uint16_t, 0, uint8_t>			propertyOffsets;
		Vector<uint32_t, 0, uint8_t>			propertyIDs;
		Vector<std::byte, 0, uint16_t>			propertyBuffer;

		bool HasTexture(uint32_t tag) const noexcept;
	};


	struct MaterialTextureEntry
	{
		mutable uint32_t	refCount;
		ResourceHandle		texture;
		GUID_t				assetID;
	};


	template<MaterialValue TY>
	std::optional<TY> GetProperty(const MaterialComponentData& material, const uint32_t ID)
	{
		const auto& propertyIDs = material.propertyIDs;
		const auto& offsets		= material.propertyOffsets;
		const auto& buffer		= material.propertyBuffer;

		if (const uint32_t* prop =
			std::find_if(
				propertyIDs.begin(), propertyIDs.end(),
				[&](const uint32_t& prop) { return prop == ID; }); prop != propertyIDs.end())
		{
			const auto idx = std::distance(propertyIDs.begin(), prop);
			const size_t byteOffset = offsets[idx];
			const size_t byteSize = (((idx + 1) == offsets.size()) ? buffer.size() : offsets[idx + 1]) - byteOffset;

			if (byteSize != sizeof(TY))
				return {};

			TY out;
			memcpy(&out, buffer.data() + byteOffset, byteSize);

			return { out };
		}
		else
			return {};
	}

	/************************************************************************************************/


	struct MaterialComponent final : public Component<MaterialComponent, MaterialComponentID>
	{
		MaterialComponent(IRenderSystem& IN_renderSystem, iAllocator* IN_allocator, ITextureManager* IN_TSE = &NullTextureManager);

		virtual ~MaterialComponent();

		void FreeComponentView(void* _ptr) final override;


		MaterialComponentData operator [](const MaterialHandle handle) const;

		const MaterialComponentData& GetRef(const MaterialHandle handle) const;
		
		MaterialHandle CreateMaterial(MaterialHandle IN_parent = InvalidHandle);

		void AddRef(MaterialHandle material) noexcept;
		void AddSubMaterial(MaterialHandle material, MaterialHandle subMaterial);

		void ReleaseMaterial(MaterialHandle material);
		void ReleaseTexture(ResourceHandle texture);

		MaterialTextureEntry*	_AddTextureAsset(GUID_t textureAsset, ReadContext& readContext, const bool loadLowest = false);
		void					_ReleaseTexture(MaterialTextureEntry* entry);
		MaterialTextureEntry*	_FindTextureAsset(ResourceHandle	resourceHandle);
		MaterialTextureEntry*	_FindTextureAsset(GUID_t			textureAsset);

		MaterialHandle  CloneMaterial(MaterialHandle sourceMaterial);

		// The Material View is a ref counted reference to a material instance.
		// On Writes to the material, if it is shared, the MaterialView does a copy on write and creates a new instance of the material.
		struct MaterialView : public ComponentView_t<MaterialComponent>
		{
			MaterialView(GameObject& gameObject, MaterialHandle IN_handle) noexcept;
			MaterialView(GameObject& gameObject) noexcept;

			void Release();

			MaterialComponentData GetData() const;

			bool Shared() const;

			void Add2Pass(const PassHandle ID);

			Vector<PassHandle, 16, uint8_t> GetPasses() const;

			void SetProperty(const uint32_t ID, auto&& value)		{ GetComponent().SetProperty(handle, ID, value); }
			void SetProperty(const uint32_t ID, const auto& value)	{ GetComponent().SetProperty(handle, ID, value); }


			template<MaterialValue TY>
			std::optional<TY> GetProperty(const uint32_t ID) const { return GetComponent().GetProperty<TY>(handle, ID); }


			void						PushTexture(GUID_t textureAsset, bool LoadLowest = false);
			void						PushTexture(ResourceHandle);

			void						InsertTexture(GUID_t, int idx, ReadContext& readContext, const bool loadLowest = false);
			void						InsertTexture(ResourceHandle, int idx);

			void						RemoveTextureAt(int idx);

			void						RemoveTexture(GUID_t);
			void						RemoveTexture(ResourceHandle);

			std::span<const GUID_t>		GetTextureAssets() const;
			DescriptorRange				GetTextureDescriptors() const;
			void						UpdateTextureDescriptors();

			bool							HasSubMaterials() const;
			std::span<const MaterialHandle>	GetSubMaterials() const;
			MaterialHandle					CreateSubMaterial();

			operator MaterialHandle () const noexcept { return handle; }

			MaterialHandle	handle;
		};

		using View = MaterialView;

		void PushTexture(MaterialHandle material, GUID_t textureAsset, ReadContext& readContext,  const bool LoadLowest = false);
		void PushTexture(MaterialHandle material, ResourceHandle texture);

		void RemoveTexture(MaterialHandle material, GUID_t);
		void RemoveTexture(MaterialHandle material, ResourceHandle);
		void RemoveTextureAt(MaterialHandle material, int idx);

		void InsertTexture(MaterialHandle material, GUID_t, int I, ReadContext& readContext, const bool loadLowest = false);
		void InsertTexture(MaterialHandle material, ResourceHandle, int I);

		void AddComponentView(GameObject& gameObject, ValueMap userValues, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) override;

		void Add2Pass(MaterialHandle& material, const PassHandle ID);

		DescriptorRange				GetTextureDescriptors	(MaterialHandle material);
		void						UpdateTextureDescriptors(MaterialHandle material);

		Vector<PassHandle, 16, uint8_t>	GetPasses(MaterialHandle material) const;
		Vector<PassHandle>				GetActivePasses(iAllocator& allocator) const;

		template<MaterialValue TY>
		void SetProperty(MaterialHandle& materialHndl, const uint32_t ID, TY&& value)
		{
			if (entries[handles[materialHndl]].refCount > 1)
			{
				auto newHandle = CloneMaterial(materialHndl);
				ReleaseMaterial(materialHndl);

				materialHndl = newHandle;
			}

			auto& material		= entries[handles[materialHndl]];
			auto& propertyIDs	= material.propertyIDs;
			auto& offsets		= material.propertyOffsets;
			auto& buffer		= material.propertyBuffer;
			
		    if (uint32_t* prop =
				std::find_if(
					propertyIDs.begin(), propertyIDs.end(),
					[&](uint32_t& prop) { return prop == ID; }); prop != propertyIDs.end())
			{
				auto idx				= std::distance(propertyIDs.begin(), prop);
				const size_t byteOffset	= offsets[idx];
				const size_t byteSize	= (((idx + 1) < offsets.size()) ? offsets[idx + 1] : buffer.size()) - byteOffset;
				memcpy(buffer.data() + byteOffset, &value, byteSize);
			}
			else
			{
				const size_t byteOffset = buffer.size();
				propertyIDs.emplace_back(ID);
				offsets.push_back(byteOffset);
				buffer.resize(buffer.size() + sizeof(value));
				memcpy(buffer.data() + byteOffset, &value, sizeof(value));
			}

			material.lastChanged = IRenderSystem::GetInstance().GetCurrentCounter();
		}


		template<MaterialValue TY>
		std::optional<TY> GetProperty(MaterialHandle handle, const uint32_t ID) const
		{
			const auto& material = entries[handles[handle]];
			return FlexKit::GetProperty<TY>(material, ID);
		}


		template<MaterialValue TY>
		TY GetPropertyOr(MaterialHandle handle, const uint32_t ID, const TY& orValue) const
		{
			return GetProperty<TY>(handle, ID).value_or(orValue);
		}

		bool HasTexture(MaterialHandle, const uint32_t id) const;


		IRenderSystem&		renderSystem;
		ITextureManager*	textureManager;

		Vector<MaterialComponentData>					entries;
		Vector<MaterialTextureEntry>					textures;
		Vector<PassHandle>								activePasses;

		HandleUtilities::HandleTable<MaterialHandle>	handles;
		std::mutex										m;
		iAllocator&										allocator;
	};


	using MaterialView = MaterialComponent::View;


	void SetMaterialHandle(GameObject& go, MaterialHandle material) noexcept;
	MaterialHandle GetMaterialHandle(GameObject& go) noexcept;

	template<MaterialValue TY>
	std::optional<TY> GetMaterialProperty(GameObject& go, const uint32_t ID) noexcept
	{
		return Apply(go, [&](MaterialView& view){ return view.GetProperty<TY>(ID); });
	}

	template<MaterialValue TY>
	std::optional<TY> GetMaterialProperty(const MaterialHandle material, const uint32_t ID) noexcept
	{
		return MaterialComponent::GetComponent().GetProperty<TY>(material, ID);
	}

	template<IsConstCharStar ... TY>
	struct MaterialQuery
	{
		using Type		= MaterialView&;
		using ValueType = MaterialView;
		static constexpr bool IsConst() { return false; }

		bool					IsValid(const MaterialView&) { return true; }
		bool					Available(const GameObject& gameObject) { return gameObject.hasView(MaterialComponentID); }
		MaterialView&			GetValue(GameObject& gameObject) { return GetView<MaterialView>(gameObject); }
	};


	struct MaterialPassQuery
	{
		MaterialPassQuery(PassHandle IN_pass) : pass{ IN_pass } {}

		PassHandle pass;

		using Type		= MaterialView&;
		using ValueType = MaterialView;
		static constexpr bool	IsConst() { return false; }

		bool					IsValid(const MaterialView& view)
		{
			auto passes = view.GetPasses();
		    return std::find(passes.begin(), passes.end(), pass) != passes.end();
		}

		bool					Available(const GameObject& gameObject) { return gameObject.hasView(MaterialComponentID); }
		MaterialView&			GetValue(GameObject& gameObject) { return GetView<MaterialView>(gameObject); }
	};

}	/************************************************************************************************/

/**********************************************************************

Copyright (c) 2015 - 2026 Robert May

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
