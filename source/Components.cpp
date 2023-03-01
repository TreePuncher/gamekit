#include "Assets.h"
#include "Components.h"
#include "ComponentBlobs.h"
#include <regex>

namespace FlexKit
{   /************************************************************************************************/


	StringIDHandle StringIDComponent::Create(const char* initial, size_t length)
	{
		auto handle = handles.GetNewHandle();

		StringID newID;
		newID.ID[length]	= '\0';
		newID.handle		= handle;
		strncpy_s(newID.ID, initial, Min(sizeof(StringID), length));

		handles[handle] = static_cast<index_t>(IDs.push_back(newID));

		return handle;
	}


	/************************************************************************************************/


	void StringIDComponent::Remove(StringIDHandle handle)
	{
		if (handle == InvalidHandle)
			return;

		auto lastElement = IDs.back();
		IDs[handles[handle]] = lastElement;
		IDs.pop_back();

		handles[lastElement.handle] = handles[handle];
		handles.RemoveHandle(handle);
	}


	/************************************************************************************************/


	void StringIDComponent::AddComponentView(GameObject& gameObject, ValueMap user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
	{
		IDComponentBlob blob;
		std::memcpy(&blob, buffer, sizeof(blob));

		auto temp = sizeof(blob.ID);

		const size_t IDLen = strnlen(blob.ID, sizeof(blob.ID));

		if (!gameObject.hasView(StringComponentID))
			gameObject.AddView<StringIDView>(blob.ID, IDLen);
		else
			SetStringID(gameObject, blob.ID);
	}


	/************************************************************************************************/


	void StringIDComponent::FreeComponentView(void* _ptr)
	{
		static_cast<StringIDView*>(_ptr)->Release();
	}


	/************************************************************************************************/


	bool StringPatternQuery::IsValid(const StringIDView& stringID) const noexcept
	{
		return std::regex_search(stringID.GetString(), pattern);
	}


	/************************************************************************************************/


	void GameOjectTest(iAllocator* allocator)
	{
		SampleComponent		sample1(allocator);
		SampleComponent2	sample2(allocator);
		SampleComponent3	sample3(allocator);

		GameObject go;
		go.AddView<SampleView>();
		go.AddView<SampleView2>();
		go.AddView<TestView>();

		Apply(go,
			[&](	// Function Sources
				SampleView& sample1,
				SampleView2& sample2,
				SampleView3& sample3
				)
			{	// do things with behaviors
				// JK doesn't run, not all inputs satisfied!
				sample1.DoSomething();
				sample2.DoSomething();
				sample3.DoSomething();

				FK_ASSERT(false, "Expected Failure");
			},
			[&]
			{
				FK_ASSERT(true, "SUCCESS");

				// this runs instead!
			});

		go.AddView<SampleView3>();

		Apply(go,
			[](	// Function Sources
				SampleView& sample1,
				SampleView2& sample2,
				SampleView3& sample3,
				TestView& test1)
			{	// do things with behaviors
				sample1.DoSomething();
				sample2.DoSomething();
				sample3.DoSomething();

				auto val = test1.GetData();

				FK_ASSERT(true, "SUCCESS");
			},
			[]
			{   // should not run
				FK_ASSERT(false, "Unexpected Failure");
			});
	}


	bool LoadPrefab(GameObject& gameObject, const char* assetID, iAllocator& allocator, ValueMap user_ptr)
	{
		if (auto guid = FlexKit::FindAssetGUID(assetID); guid)
			return LoadPrefab(gameObject, guid.value(), allocator, user_ptr);
		else
			return false;
	}


	bool LoadPrefab(GameObject& gameObject, uint64_t assetID, iAllocator& allocator, ValueMap userValues)
	{
		auto assetHandle    = LoadGameAsset(assetID);
		if (assetHandle == -1)
			return false;

		auto asset          = static_cast<PrefabResource*>(GetAsset(assetHandle));

		if (!asset)
			return false;


		char* buffer            = (char*)(asset + 1);
		size_t offset           = 0;
		size_t componentCount   = asset->componentCount;

		for (size_t I = 0; I < componentCount; I++)
		{
			ComponentBlock::Header component;
			memcpy(&component, buffer + offset, sizeof(component));

			if (ComponentAvailability(component.componentID) == true)
			{
				auto& componentSystem = GetComponent(component.componentID);
				componentSystem.AddComponentView(gameObject, userValues, (std::byte*)buffer + offset, component.blockSize, &allocator);
			}
			else
			{
				gameObject.Release();
				return false;
			}

			offset += component.blockSize;
		}

		return true;
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
