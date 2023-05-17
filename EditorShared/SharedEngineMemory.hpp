#pragma once

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <AnimationComponents.h>
#include <CameraUtilities.h>
#include <Components.h>
#include <graphics.h>
#include <Materials.h>
#include <memoryutilities.h>
#include <physicsutilities.h>
#include <Serialization.hpp>
#include <TextureStreamingUtilities.h>
#include <Transforms.h>

#include <ranges>

using std::views::iota;
using std::views::zip;

using namespace boost::interprocess;


/************************************************************************************************/


namespace FlexKit
{
	struct EngineMemory;
}

struct SharedComponents
{
	SharedComponents(FlexKit::iAllocator& allocator);

	// Components
	FlexKit::SceneNodeComponent			sceneNodes;
	FlexKit::StringIDComponent			stringIDComponent;
	FlexKit::CameraComponent			cameraComponent;
	FlexKit::SceneVisibilityComponent	visibilityComponent;
	FlexKit::SkeletonComponent			skeletonComponent;
	FlexKit::AnimatorComponent			animatorComponent;
	FlexKit::LightComponent				lightComponent;
	FlexKit::ShadowMapComponent			shadowMaps;
	FlexKit::BrushComponent				brushComponent;

	FlexKit::FABRIKTargetComponent		ikTargetComponent;
	FlexKit::FABRIKComponent			ikComponent;

	FlexKit::TriggerComponent			triggers;

	void Register(); // TODO
};


/************************************************************************************************/


template<typename TY>
struct InterProcessQueue
{

	InterProcessQueue() = default;

	InterProcessQueue(FlexKit::iAllocator& allocator)
		: items{ allocator }
	{
		items.resize(10);
	}

	InterProcessQueue(const InterProcessQueue& rhs)
	{
		auto l1 = boost::interprocess::scoped_lock{ m };
		auto l2 = boost::interprocess::scoped_lock{ rhs.m };

		head	= rhs.head;
		tail	= rhs.tail;
		items	= std::move(rhs.items);

		rhs.head = 0;
		rhs.tail = 0;
	}

	InterProcessQueue(InterProcessQueue&& rhs)
	{
		auto l1 = boost::interprocess::scoped_lock{ m };
		auto l2 = boost::interprocess::scoped_lock{ rhs.m };

		head	= rhs.head;
		tail	= rhs.tail;
		items	= std::move(rhs.items);

		rhs.head = 0;
		rhs.tail = 0;
	}

	InterProcessQueue& operator = (InterProcessQueue&& rhs)
	{
		auto l1 = boost::interprocess::scoped_lock{ m };
		auto l2 = boost::interprocess::scoped_lock{ rhs.m };

		head	= rhs.head;
		tail	= rhs.tail;
		items	= std::move(rhs.items);

		rhs.head = 0;
		rhs.tail = 0;

		return *this;
	}

	InterProcessQueue& operator = (const InterProcessQueue& rhs)
	{
		auto l1 = boost::interprocess::scoped_lock{ m };
		auto l2 = boost::interprocess::scoped_lock{ rhs.m };

		head	= rhs.head;
		tail	= rhs.tail;
		items	= rhs.items;

		rhs.head = 0;
		rhs.tail = 0;

		return *this;
	}

	struct Iterator
	{
		InterProcessQueue*	queue;
		uint64_t			index;

		Iterator&	operator ++()		{ ++index; return *this; }
		Iterator	operator ++(int)	{ auto temp = *this ; ++index; return temp; }
		bool operator == (const Iterator& rhs) const noexcept { return rhs.index == index; }
		auto& operator *	() { return (*queue)[index];  }
		auto* operator ->	() { return &(*queue)[index]; }

		TY* data() { return  &(*queue)[index]; }
		operator TY* () { return data(); }
	};

	auto& operator [](uint32_t index)
	{
		return items[(head + index) % items.size()];
	}

	Iterator begin()
	{
		return { this, 0 };
	}

	Iterator end()
	{
		return { this, size() };
	}

	void push_front(const TY& e)
	{
		auto l = boost::interprocess::scoped_lock{ m };

		if ((tail - head) + 1 > items.size())
			items.resize(items.size() * 2);

		items[(tail++) % items.size()] = e;
	}

	void push_front(TY&& e)
	{
		auto l = boost::interprocess::scoped_lock{ m };

		if ((tail - head) + 1 > items.size())
			items.resize(items.size() * 2);

		items[(tail++) % items.size()] = std::move(e);
	}

	std::optional<TY> pop_back()
	{
		auto l = boost::interprocess::scoped_lock{ m };

		if (tail - head != 0)
			return std::move(items[(head++) % items.size()]);
		else
			return {};
	}

	TY remove_stable(Iterator element)
	{
		FK_ASSERT((size_t)items.data() <= (size_t)element.data() && (size_t)element.data() < (size_t)items.data() + items.ByteSize());

		if (size() > 1)
		{
			auto temp = *element;

			for (auto i = element; i < end(); i++)
			{
				if (i + 1 < end())
					*i = *(i + 1);
			}

			tail--;
			return temp;
		}
		else if (size() == 1)
			return pop_back().value();
		else
			throw std::runtime_error("InterProcessQueue removing value from empty queue!");
	}

	size_t size() const noexcept
	{
		return std::atomic_ref(tail) - std::atomic_ref(head);
	}

	uint32_t			head = 0;
	uint32_t			tail = 0;
	FlexKit::Vector<TY> items;
	boost::interprocess::interprocess_mutex m;
};


/************************************************************************************************/


class EditorPlayerState;
class ResponseInterface;

class IPCAllocator : public FlexKit::iAllocator
{
public:
	IPCAllocator(FlexKit::iAllocator* IN_allocator) :
		allocator{ IN_allocator } {}

	void* malloc(size_t s)
	{
		boost::interprocess::scoped_lock l{ m };
		return allocator->malloc(s);
	}

	void  free(void* _ptr)
	{
		boost::interprocess::scoped_lock l{ m };
		allocator->free(_ptr);
	}

	void* _aligned_malloc(size_t s, size_t A = 0x10)
	{
		boost::interprocess::scoped_lock l{ m };
		return allocator->_aligned_malloc(s, A);
	}

	void  _aligned_free(void* _ptr)
	{
		boost::interprocess::scoped_lock l{ m };
		return allocator->_aligned_free(_ptr);
	}

	void  clear(void)
	{
		boost::interprocess::scoped_lock l{ m };
		return allocator->clear();
	}

	void* malloc_Debug(size_t s, const char* MD, size_t MDSectionSize)
	{
		boost::interprocess::scoped_lock l{ m };

		return allocator->malloc_Debug(s, MD, MDSectionSize);
	}

	operator FlexKit::iAllocator* () { return this; }

private:

	FlexKit::iAllocator* allocator;
	boost::interprocess::interprocess_mutex m;
};

struct InterProcessMessage
{
	uint64_t					UUID;
	FlexKit::Vector<std::byte>	buffer;
};

struct XorShf96Generator
{
	uint32_t x = 123456789;
	uint32_t y = 362436069;
	uint32_t z = 521288629;

	// Marsaglia's xorshf generator
	uint64_t operator() () noexcept
	{	//period 2^96-1
		unsigned long t;
		x ^= x << 16;
		x ^= x >> 5;
		x ^= x << 1;

		t = x;
		x = y;
		y = z;
		z = t ^ x ^ y;

		return z;
	}
};

struct SharedEngineMemory
{
	char							blockTag[32];
	FlexKit::BlockAllocator			sharedAllocator;
	IPCAllocator					blockAllocator { sharedAllocator };

	SharedComponents*				components = nullptr;
	mapped_region					mapped;
	HWND							targetWindow;
	FlexKit::GameObject*			currentGameObject = nullptr;

	XorShf96Generator		idGenerator;

	InterProcessQueue<InterProcessMessage>					playerQueue;
	InterProcessQueue<InterProcessMessage>					editorQueue;
	FlexKit::Vector<std::unique_ptr<ResponseInterface>>		responders;



	uint64_t PushMessageToPlayer(auto&& message, uint64_t uuid)
	{
		FlexKit::SaveArchiveContext archive;

		archive& message;
		auto blob = archive.GetBlob();

		FlexKit::Vector<std::byte> outputBlob{ blockAllocator };

		outputBlob.resize(blob.buffer.size());

		memcpy(outputBlob.data(), blob.data(), outputBlob.size());

		playerQueue.push_front({ .UUID = uuid, .buffer = outputBlob});

		return uuid;
	}

	uint64_t PushMessageToEditor(auto&& message, uint64_t uuid)
	{
		FlexKit::SaveArchiveContext archive;

		archive& message;
		auto blob = archive.GetBlob();

		FlexKit::Vector<std::byte> outputBlob{ blockAllocator };

		outputBlob.resize(blob.buffer.size());

		memcpy(outputBlob.data(), blob.data(), outputBlob.size());

		editorQueue.push_front({ .UUID = uuid, .buffer = std::move(outputBlob) });

		return uuid;
	}

	uint64_t PushMessageToPlayer(auto&& message)
	{
		return PushMessageToPlayer(message, idGenerator());
	}

	uint64_t PushMessageToEditor(auto&& message)
	{
		return PushMessageToEditor(message, idGenerator());
	}


	std::optional<InterProcessMessage> GetMessageFromEditor(uint64_t uuid)
	{
		auto end = playerQueue.end();
		for (auto itr = playerQueue.begin(); itr < end; ++itr)
		{
			if (itr->UUID == uuid)
			{
				auto message = playerQueue.remove_stable(itr);
				return message;
			}
		}

		return {};
	}

	std::optional<InterProcessMessage> PollEditorMessages() noexcept
	{
		if (editorQueue.size())
			return { editorQueue.pop_back() };
		else
			return {};
	}
};


/************************************************************************************************/


SharedEngineMemory*	InitiateSharedMemory(shared_memory_object& obj);
SharedEngineMemory*	GetSharedMemory(shared_memory_object& obj, size_t offset);
void				ReleaseSharedEngineMemory(SharedEngineMemory&);


using ProcessQueue = InterProcessQueue<InterProcessMessage>;


/**********************************************************************

Copyright (c) 2015 - 2023 Robert May

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
