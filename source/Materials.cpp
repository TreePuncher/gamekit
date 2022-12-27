#include "Materials.h"
#include "Scene.h"
#include "TextureStreamingUtilities.h"

namespace FlexKit
{	/************************************************************************************************/


	MaterialComponentData MaterialComponent::operator [](const MaterialHandle handle) const
	{
		if(handle == InvalidHandle)
			return { 0, InvalidHandle, InvalidHandle, {}, {}, {}, {} };

		return materials[handles[handle]];
	}


	/************************************************************************************************/


	MaterialHandle MaterialComponent::CreateMaterial(MaterialHandle IN_parent)
	{
		std::scoped_lock lock{ m };

		const auto handle		= handles.GetNewHandle();
		const auto materialIdx = (index_t)materials.push_back({ (uint32_t)0, handle, IN_parent, {}, {}, {} });

		if(IN_parent != InvalidHandle)
			AddRef(IN_parent);

		handles[handle] = materialIdx;

		return handle;
	}


	/************************************************************************************************/


	void MaterialComponent::AddRef(MaterialHandle material) noexcept
	{
		if(material != InvalidHandle)
			std::atomic_ref(materials[handles[material]].refCount).fetch_add(1, std::memory_order_acq_rel);
	}


	/************************************************************************************************/


	void MaterialComponent::AddSubMaterial(MaterialHandle material, MaterialHandle subMaterial)
	{
		materials[handles[material]].SubMaterials.push_back(subMaterial);
	}


	/************************************************************************************************/


	MaterialTextureEntry* MaterialComponent::_AddTextureAsset(GUID_t textureAsset, ReadContext& readContext, const bool loadLowest)
	{
		const auto [MIPCount, DDSTextureWH, format] = GetDDSInfo(textureAsset, readContext);
		const auto resourceStrID					= GetResourceStringID(textureAsset);

		if (DDSTextureWH.Product() == 0)
			return nullptr;

		auto textureResource = renderSystem.CreateGPUResource(
			GPUResourceDesc::ShaderResource(
				DDSTextureWH,
				format,
				MIPCount,
				1,
				ResourceAllocationType::Tiled));

		renderSystem.SetDebugName(textureResource, "Virtual Texture");

		streamEngine.BindAsset(textureAsset, textureResource);

		if(loadLowest)
			streamEngine.LoadLowestLevel(textureResource, renderSystem.GetImmediateCopyQueue());

		const auto idx = textures.push_back({ 1, textureResource, textureAsset });

		return textures.begin() + idx;
	}


	/************************************************************************************************/


	void MaterialComponent::_ReleaseTexture(MaterialTextureEntry* entry)
	{
		auto res = std::atomic_ref(entry->refCount).fetch_sub(1, std::memory_order_acq_rel);

		if (res == 1)
		{
			renderSystem.ReleaseResource(entry->texture);
			textures.remove_unstable(entry);
		}
	}


	/************************************************************************************************/


	MaterialTextureEntry* MaterialComponent::_FindTextureAsset(ResourceHandle resourceHandle)
	{
		return std::ranges::find_if(
			textures,
			[&](auto element)
			{
				return element.texture == resourceHandle;
			}
		);
	}

	MaterialTextureEntry* MaterialComponent::_FindTextureAsset(GUID_t textureAsset)
	{
		return std::ranges::find_if(
			textures,
			[&](auto element)
			{
				return	element.assetID != 0xfffffffffffffff &&
						element.assetID == textureAsset;
			}
		);
	}


	/************************************************************************************************/


	void MaterialComponent::ReleaseMaterial(MaterialHandle material)
	{
		const auto idx = handles[material];
		std::atomic_ref refCount(materials[idx].refCount);

		if(refCount.load(std::memory_order_acquire) > 0)
			refCount.fetch_sub(1, std::memory_order_acq_rel);

		if (refCount.load(std::memory_order_acquire) == 0)
		{
			auto& material_ref	= materials[idx];
			auto& textures		= material_ref.Textures;
			auto parent			= material_ref.parent;

			FK_ASSERT(handles[material_ref.handle] == idx);

			if (parent != InvalidHandle)
				ReleaseMaterial(parent);

			for (auto& texture : textures)
				ReleaseTexture(texture);

			if (material_ref.textureDescriptors.size)
				renderSystem._ReleaseDescriptorRange(material_ref.textureDescriptors, material_ref.lastUsed);

			if (materials.size() > 1 && idx != materials.size() - 1)
			{
				materials[idx] = materials.back();
				handles[materials[idx].handle] = idx;
			}

			materials.pop_back();

			if (materials.size())
			{
				handles[materials[idx].handle] = idx;
				handles.RemoveHandle(material);
			}
		}
	}


	/************************************************************************************************/


	void MaterialComponent::ReleaseTexture(ResourceHandle texture)
	{
		auto res = _FindTextureAsset(texture);

		if (res != std::end(textures))
			_ReleaseTexture(res);
	}


	/************************************************************************************************/


	MaterialHandle MaterialComponent::CloneMaterial(MaterialHandle sourceMaterial)
	{
		const auto clone = (index_t)materials.push_back(materials[handles[sourceMaterial]]);

		auto& material = materials[clone];
		material.refCount = 0;

		if (material.parent != InvalidHandle)
			AddRef(material.parent);

		const auto handle = handles.GetNewHandle();
		handles[handle] = clone;

		return handle;
	}


	/************************************************************************************************/


	void MaterialComponent::PushTexture(MaterialHandle material, GUID_t textureAsset, ReadContext& readContext, const bool loadLowest)
	{
		auto res = _FindTextureAsset(textureAsset);

		if (res == std::end(textures))
		{
			auto assets = _AddTextureAsset(textureAsset, readContext, loadLowest);
			materials[handles[material]].Textures.push_back(assets->texture);
		}
		else
		{
			std::atomic_ref(res->refCount).fetch_add(1, std::memory_order_acq_rel);
			materials[handles[material]].Textures.push_back(res->texture);
		}
	}


	/************************************************************************************************/


	void MaterialComponent::PushTexture(MaterialHandle material, ResourceHandle texture)
	{
		auto res = _FindTextureAsset(texture);

		if (res == std::end(textures))
		{
			textures.push_back({ 1, texture, 0xfffffffffffffff });
			materials[handles[material]].Textures.push_back(texture);
		}
		else
		{
			std::atomic_ref(res->refCount).fetch_add(1, std::memory_order_acq_rel);
			materials[handles[material]].Textures.push_back(texture);
		}
	}


	/************************************************************************************************/


	void MaterialComponent::RemoveTexture(MaterialHandle material, GUID_t guid)
	{
		auto& material_ref = materials[handles[material]];

		auto res = _FindTextureAsset(guid);

		if (auto res2 = std::ranges::find(material_ref.Textures, res->texture); res2 != material_ref.Textures.end())
		{
			material_ref.Textures.remove_stable(res2);
			_ReleaseTexture(res);
		}
	}


	/************************************************************************************************/


	void MaterialComponent::RemoveTexture(MaterialHandle material, ResourceHandle resource)
	{
		auto& material_ref = materials[handles[material]];

		if (auto res = std::ranges::find(material_ref.Textures, resource); res != material_ref.Textures.end())
		{
			material_ref.Textures.remove_stable(res);
			ReleaseTexture(resource);
		}
	}


	/************************************************************************************************/


	void MaterialComponent::RemoveTextureAt(MaterialHandle material, int idx)
	{
		auto& material_ref = materials[handles[material]];
		const auto t = material_ref.Textures[idx];

		material_ref.Textures.remove_stable(material_ref.Textures.begin() + idx);
		ReleaseTexture(t);
	}


	/************************************************************************************************/


	void MaterialComponent::InsertTexture(MaterialHandle material, GUID_t guid, int I, ReadContext& readContext, const bool loadLowest)
	{
		auto asset = _FindTextureAsset(guid);

		if (asset == textures.end())
			asset = _AddTextureAsset(guid, readContext, loadLowest);
		else
			std::atomic_ref(asset->refCount).fetch_add(1, std::memory_order_acq_rel);

		auto& textures = materials[handles[material]].Textures;

		textures.insert(textures.begin() + I, asset->texture);
	}


	/************************************************************************************************/


	void MaterialComponent::InsertTexture(MaterialHandle material, ResourceHandle resource, int I)
	{
		auto asset = _FindTextureAsset(resource);

		if (asset == textures.end())
			return;

		std::atomic_ref(asset->refCount).fetch_add(1, std::memory_order_acq_rel);

		auto& textures = materials[handles[material]].Textures;

		textures.insert(textures.begin() + I, asset->texture);
	}


	/************************************************************************************************/


	MaterialComponent::MaterialView::MaterialView(GameObject& gameObject, MaterialHandle IN_handle) noexcept :
		handle{ IN_handle }
	{
		GetComponent().AddRef(handle);

		SetMaterialHandle(gameObject, handle);
	}

	MaterialComponent::MaterialView::MaterialView(GameObject& gameObject) noexcept : handle{ GetComponent().CreateMaterial() }
	{
		GetComponent().AddRef(handle);

		SetMaterialHandle(gameObject, handle);
	}

	/************************************************************************************************/


	MaterialComponent::MaterialView::~MaterialView()
	{
		GetComponent().ReleaseMaterial(handle);
	}


	/************************************************************************************************/


	MaterialComponentData MaterialComponent::MaterialView::GetData() const
	{
		return GetComponent()[handle];
	}


	/************************************************************************************************/


	bool MaterialComponent::MaterialView::Shared() const
	{
		const auto& material		= GetComponent()[handle];
		const auto& refCount_ref	= material.refCount;
		return std::atomic_ref(refCount_ref).load(std::memory_order_acquire) > 1;
	}


	/************************************************************************************************/


	void MaterialComponent::MaterialView::Add2Pass(const PassHandle ID)
	{
		GetComponent().Add2Pass(handle, ID);
	}


	/************************************************************************************************/


	Vector<PassHandle, 16, uint8_t> MaterialComponent::MaterialView::GetPasses() const
	{
		return GetComponent().GetPasses(handle);
	}


	/************************************************************************************************/


	void MaterialComponent::MaterialView::SetTextureCount(size_t size)
	{
		if (Shared())
		{
			auto newHandle = GetComponent().CloneMaterial(handle);
			GetComponent().ReleaseMaterial(handle);

			handle = newHandle;
		}

		ReadContext rdCtx{};
	}


	void MaterialComponent::MaterialView::PushTexture(GUID_t textureAsset, bool LoadLowest)
	{
		if (Shared())
		{
			auto newHandle = GetComponent().CloneMaterial(handle);
			GetComponent().ReleaseMaterial(handle);

			handle = newHandle;
		}

		ReadContext rdCtx{};
		GetComponent().PushTexture(handle, textureAsset, rdCtx, LoadLowest);
	}


	/************************************************************************************************/


	void MaterialComponent::MaterialView::PushTexture(ResourceHandle resource)
	{
		if (Shared())
		{
			auto newHandle = GetComponent().CloneMaterial(handle);
			GetComponent().ReleaseMaterial(handle);

			handle = newHandle;
		}

		ReadContext rdCtx{};
		GetComponent().PushTexture(handle, 0xfffffffffffffff, rdCtx);
	}


	/************************************************************************************************/


	void MaterialComponent::MaterialView::InsertTexture(GUID_t asset, int idx, ReadContext& readContext, const bool loadLowest)
	{
		if (Shared())
		{
			auto newHandle = GetComponent().CloneMaterial(handle);
			GetComponent().ReleaseMaterial(handle);

			handle = newHandle;
		}

		GetComponent().InsertTexture(handle, asset, idx, readContext, loadLowest);
	}


	/************************************************************************************************/


	void MaterialComponent::MaterialView::RemoveTextureAt(int idx)
	{
		if (Shared())
		{
			auto newHandle = GetComponent().CloneMaterial(handle);
			GetComponent().ReleaseMaterial(handle);

			handle = newHandle;
		}

		GetComponent().RemoveTextureAt(handle, idx);
	}


	/************************************************************************************************/


	void MaterialComponent::MaterialView::InsertTexture(ResourceHandle resource, int idx)
	{
		auto& materials = GetComponent();

		if (Shared())
		{
			auto newHandle = materials.CloneMaterial(handle);
			materials.ReleaseMaterial(handle);

			handle = newHandle;
		}

		materials.InsertTexture(handle, resource, idx);
	}


	/************************************************************************************************/


	void MaterialComponent::MaterialView::RemoveTexture(GUID_t asset)
	{
		auto& materials = GetComponent();

		auto res = std::ranges::find_if(
			GetComponent().textures,
			[&](auto& texture)
			{
				return texture.assetID == asset;
			});

		if (materials.textures.end() == res)
			return;

		if (Shared())
		{
			auto newHandle = materials.CloneMaterial(handle);
			materials.ReleaseMaterial(handle);

			handle = newHandle;
		}

		materials.RemoveTexture(handle, res->texture);
		materials.ReleaseTexture(res->texture);
	}


	/************************************************************************************************/


	void MaterialComponent::MaterialView::RemoveTexture(ResourceHandle resource)
	{
		auto& materials = GetComponent();

		if (Shared())
		{
			auto newHandle = materials.CloneMaterial(handle);
			materials.ReleaseMaterial(handle);

			handle = newHandle;
		}

		materials.ReleaseTexture(resource);
		materials.RemoveTexture(handle, resource);
	}


	/************************************************************************************************/


	bool MaterialComponent::MaterialView::HasSubMaterials() const
	{
		return !GetComponent()[handle].SubMaterials.empty();
	}


	/************************************************************************************************/


	MaterialHandle MaterialComponent::MaterialView::CreateSubMaterial()
	{
		auto& materials = GetComponent();

		if (Shared())
		{
			auto newHandle = GetComponent().CloneMaterial(handle);
			GetComponent().ReleaseMaterial(handle);

			handle = newHandle;
		}

		auto subMaterial = materials.CreateMaterial();
		materials.AddSubMaterial(handle, subMaterial);

		return subMaterial;
	}


	/************************************************************************************************/


	void MaterialComponent::AddComponentView(GameObject& gameObject, ValueMap userValues, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
	{
		if (gameObject.hasView(MaterialComponentID))
			return;

		MaterialComponentBlob materialBlob;
		memcpy(&materialBlob, buffer, Min(sizeof(materialBlob), bufferSize));

		const static auto parentMaterial = [&]() {
			auto newMaterial = CreateMaterial();

			Add2Pass(newMaterial, PassHandle{ GetCRCGUID(PBR_CLUSTERED_DEFERRED) });
			Add2Pass(newMaterial, PassHandle{ GetCRCGUID(VXGI_STATIC) });
			Add2Pass(newMaterial, PassHandle{ GetCRCGUID(SHADOWMAPPASS) });

			SetProperty(newMaterial, GetCRCGUID(PBR_ALBEDO),	float4(0.5f, 0.5f, 0.5f, 0.3f));
			SetProperty(newMaterial, GetCRCGUID(PBR_SPECULAR),	float4(0.9f, 0.9f, 0.9f, 0.0f));

			return newMaterial;
		}();

		auto newMaterial = CreateMaterial(parentMaterial);

		auto rdCtx = ReadContext{};
		if (materialBlob.materials.size() > 1)
		{
			for (auto& subMaterial : materialBlob.materials)
			{
				auto newSubMaterial = CreateMaterial(newMaterial);
				AddSubMaterial(newMaterial, newSubMaterial);

				for (auto& texture : subMaterial.textures)
					PushTexture(newSubMaterial, texture, rdCtx, true);
			}
		}
		else if (materialBlob.materials.size() == 1)
		{
			for (auto& texture : materialBlob.materials[0].textures)
				PushTexture(newMaterial, texture, rdCtx, true);
		}

		gameObject.AddView<MaterialView>(newMaterial);
		SetMaterialHandle(gameObject, newMaterial);

		UpdateTextureDescriptors(newMaterial);
	}


	/************************************************************************************************/


	void MaterialComponent::Add2Pass(MaterialHandle& material, const PassHandle ID)
	{
		if (std::atomic_ref(materials[handles[material]].refCount).load(std::memory_order_acquire) > 1)
		{
			auto newHandle = GetComponent().CloneMaterial(material);
			GetComponent().ReleaseMaterial(material);

			material = newHandle;
		}

		auto& passes = materials[handles[material]].Passes;
		passes.emplace_back(ID);

		if (auto res = std::find(activePasses.begin(), activePasses.end(), ID); res == activePasses.end())
			activePasses.push_back(ID);
	}


	/************************************************************************************************/


	DescriptorRange	MaterialComponent::GetTextureDescriptors(MaterialHandle material)
	{
		if (material == InvalidHandle)
			return {};

		auto& materialData = materials[handles[material]];

		if (materialData.textureDescriptors.size == 0)
			UpdateTextureDescriptors(material);

		const uint64_t current = renderSystem.graphicsSubmissionCounter;

		if(materialData.lastUsed < current)
			materialData.lastUsed = current;

		return materialData.textureDescriptors;
	}


	/************************************************************************************************/


	void MaterialComponent::UpdateTextureDescriptors(MaterialHandle material)
	{
		if (material == InvalidHandle)
			return;

		auto& materialData = materials[handles[material]];

		if (materialData.textureDescriptors.size)
			renderSystem._ReleaseDescriptorRange(materialData.textureDescriptors, renderSystem.graphicsSubmissionCounter);

		auto textureCount		= materialData.Textures.size();

		if (textureCount == 0)
			return;

		auto res				= renderSystem._AllocateDescriptorRange(textureCount);
		if (!res.has_value())
			DebugBreak();

		auto& descriptorRange	= res.value();

		FK_ASSERT(res.has_value() == true, "Failed to allocate Descriptor Heap Range");

		for (size_t I = 0; I < textureCount; I++)
		{
			auto resource	= materialData.Textures[I];
			auto format		= renderSystem.GetTextureFormat(resource);
			auto dxFormat	= FlexKit::TextureFormat2DXGIFormat(format);
			PushTextureToDescHeap(renderSystem, dxFormat, resource, descriptorRange[I]);
		}

		materialData.textureDescriptors = descriptorRange;
	}


	/************************************************************************************************/


	Vector<PassHandle, 16, uint8_t> MaterialComponent::GetPasses(MaterialHandle material) const
	{
		Vector<PassHandle, 16, uint8_t> out{ &allocator };

		if (material == InvalidHandle)
			return out;

		auto& materialData = materials[handles[material]];

		if (materialData.parent != InvalidHandle)
			out = GetPasses(materialData.parent);

		if (materialData.Passes.size())
			out += materialData.Passes;

		return out;
	}


	/************************************************************************************************/


	Vector<PassHandle> MaterialComponent::GetActivePasses(iAllocator& tempAllocator) const
	{
		Vector<PassHandle> passes{ &tempAllocator };
		passes = activePasses;

		return passes;
	}


	/************************************************************************************************/


	void SetMaterialHandle(GameObject& go, MaterialHandle material) noexcept
	{
		Apply(
			go,
			[&](BrushView& brush)
			{
				brush.GetBrush().material = material;
			});
	}


	/************************************************************************************************/


	MaterialHandle GetMaterialHandle(GameObject& go) noexcept
	{
		return Apply(
			go,
			[&](MaterialView& material)
			{
				return material.handle;
			},
			[]() -> MaterialHandle
			{
				return InvalidHandle;
			});
	}


}   /************************************************************************************************/


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

