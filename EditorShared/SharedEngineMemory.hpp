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

	void push_front(const TY& e)
	{
		auto l = boost::interprocess::scoped_lock{ m };

		if ((tail - head) + 1 > items.size())
			items.resize(items.size() * 2);

		items[(tail++) % items.size()] = e;
	}

	std::optional<TY> pop_back()
	{
		auto l = boost::interprocess::scoped_lock{ m };

		if (tail - head != 0)
			return std::move(items[(head++) % items.size()]);
		else
			return {};
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

struct SharedEngineMemory
{
	char							blockTag[32];
	FlexKit::BlockAllocator			blockAllocator;
	SharedComponents*				components = nullptr;
	mapped_region					mapped;
	HWND							targetWindow;
	FlexKit::GameObject*			currentGameObject = nullptr;

	InterProcessQueue<FlexKit::Vector<std::byte>>	inputQueue;
	InterProcessQueue<FlexKit::Vector<std::byte>>	outputQueue;


	void PushMessageToPlayer(auto& message)
	{
		FlexKit::SaveArchiveContext archive;

		archive& message;
		auto blob = archive.GetBlob();

		FlexKit::Vector<std::byte> outputBlob{ blockAllocator };

		outputBlob.resize(blob.buffer.size());

		memcpy(outputBlob.data(), blob.data(), outputBlob.size());

		inputQueue.push_front(outputBlob);
	}

	void PushMessageToEditor(auto& message)
	{
		FlexKit::SaveArchiveContext archive;

		archive& message;
		auto blob = archive.GetBlob();

		FlexKit::Vector<std::byte> outputBlob{ blockAllocator };

		outputBlob.resize(blob.buffer.size());

		memcpy(outputBlob.data(), blob.data(), outputBlob.size());

		outputQueue.push_front(outputBlob);
	}
};


/************************************************************************************************/


SharedEngineMemory*	InitiateSharedMemory(shared_memory_object& obj);
SharedEngineMemory*	GetSharedMemory(shared_memory_object& obj, size_t offset);
void				ReleaseSharedEngineMemory(SharedEngineMemory&);



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
