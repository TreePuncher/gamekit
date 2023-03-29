#pragma once
#include <variant>

#include "Components.h"
#include "ResourceHandles.h"
#include "graphics.h"
#include "Assets.h"
#include "ComponentBlobs.h"
#include "RuntimeComponentIDs.h"

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

		using ValueVarient = std::variant<float, float2, float3, float4, uint, uint2, uint3, uint4, ResourceHandle, DescriptorRange>;

		uint32_t		ID = -1;
		ValueVarient	value;
	};

	template<typename TY>
	concept MaterialValue =
		requires(TY ty)
		{
			{ MaterialProperty::ValueVarient{ ty } };
		};

	struct MaterialComponentData
	{
		uint32_t							refCount;
		MaterialHandle						handle;
		MaterialHandle						parent;
		DescriptorRange						textureDescriptors;
		uint64_t							lastUsed = -1;

		Vector<PassHandle, 8, uint8_t>			Passes;
		Vector<MaterialProperty, 8, uint8_t>	Properties;
		Vector<ResourceHandle, 8, uint8_t>		Textures;
		Vector<MaterialHandle, 4, uint8_t>		SubMaterials;
	};


	struct MaterialTextureEntry
	{
		uint32_t		refCount;
		ResourceHandle	texture;
		GUID_t			assetID;
	};


	/************************************************************************************************/


	struct MaterialComponent : public Component<MaterialComponent, MaterialComponentID>
	{
		MaterialComponent(RenderSystem& IN_renderSystem, TextureStreamingEngine& IN_TSE, iAllocator* IN_allocator) :
			streamEngine	{ IN_TSE },
			renderSystem	{ IN_renderSystem },
			materials		{ IN_allocator },
			textures		{ IN_allocator },
			handles			{ IN_allocator },
			activePasses	{ IN_allocator },
			allocator		{ *IN_allocator }
		{
			materials.reserve(256);
		}

		void FreeComponentView(void* _ptr) { static_cast<MaterialView*>(_ptr)->Release(); }


		MaterialComponentData operator [](const MaterialHandle handle) const;
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

			void SetProperty(const uint32_t ID, auto&& value) { GetComponent().SetProperty(handle, ID, value); }


			template<MaterialValue TY>
			std::optional<TY> GetProperty(const uint32_t ID) const { return GetComponent().GetProperty(handle, ID); }


			void						SetTextureCount(size_t size);

			void						PushTexture(GUID_t textureAsset, bool LoadLowest = false);
			void						PushTexture(ResourceHandle);

			void						InsertTexture(GUID_t, int idx, ReadContext& readContext, const bool loadLowest = false);
			void						InsertTexture(ResourceHandle, int idx);

			void						RemoveTextureAt(int idx);

			void						RemoveTexture(GUID_t);
			void						RemoveTexture(ResourceHandle);

			const std::span<GUID_t>		GetTextures() const;
			DescriptorRange				GetTextureDescriptors() const;
			void						UpdateTextureDescriptors();

			bool						HasSubMaterials() const;
			std::span<MaterialHandle>	GetSubMaterials() const;
			MaterialHandle				CreateSubMaterial();

			operator MaterialHandle () const noexcept { return handle; }

			MaterialHandle	handle;
		};

		using View = MaterialView;

		void PushTexture(MaterialHandle material, GUID_t textureAsset,ReadContext& readContext, const bool LoadLowest = false);
		void PushTexture(MaterialHandle material, ResourceHandle texture);

		void RemoveTexture(MaterialHandle material, GUID_t);
		void RemoveTexture(MaterialHandle material, ResourceHandle);
		void RemoveTextureAt(MaterialHandle material, int idx);

		void InsertTexture(MaterialHandle material, GUID_t, int I, ReadContext& readContext, const bool loadLowest = false);
		void InsertTexture(MaterialHandle material, ResourceHandle, int I);

		void AddComponentView(GameObject& gameObject, ValueMap userValues, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) override;

		void Add2Pass(MaterialHandle& material, const PassHandle ID);

		void						SetTextureCount			(MaterialHandle material, size_t size);
		DescriptorRange				GetTextureDescriptors	(MaterialHandle material);
		void						UpdateTextureDescriptors(MaterialHandle material);

		Vector<PassHandle, 16, uint8_t>	GetPasses(MaterialHandle material) const;
		Vector<PassHandle>				GetActivePasses(iAllocator& allocator) const;


		void SetProperty(MaterialHandle& material, const uint32_t ID, auto&& value)
		{
			if (materials[handles[material]].refCount > 1)
			{
				auto newHandle = CloneMaterial(material);
				ReleaseMaterial(material);

				material = newHandle;
			}

			auto& properties = materials[handles[material]].Properties;

			if (MaterialProperty* prop =
					std::find_if(
						properties.begin(), properties.end(),
						[&](auto& prop) { return prop.ID == ID;}); prop != properties.end())
				prop->value = value;
			else
				properties.emplace_back(ID, value);
		}


		template<MaterialValue TY>
		std::optional<TY> GetProperty(MaterialHandle handle, const uint32_t ID) const
		{
			auto& material		= materials[handles[handle]];
			auto& properties	= material.Properties;

			if (auto res = std::find_if(
					properties.begin(), properties.end(),
					[&](const auto& prop) -> bool { return prop.ID == ID; }); res != properties.end())
			{
				if (auto* property = std::get_if<TY>(&res->value))
					return { *property };
				else
					return {};
			}
			else if (material.parent != InvalidHandle)
				return GetProperty<TY>(material.parent, ID);
			else
				return {};
		}


		RenderSystem&					renderSystem;
		TextureStreamingEngine&			streamEngine;

		Vector<MaterialComponentData>					materials;
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


}	/************************************************************************************************/

/**********************************************************************

Copyright (c) 2015 - 2022 Robert May

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
