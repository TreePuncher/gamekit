#pragma once

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

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
#include <mutex>

using namespace boost::interprocess;

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

template<typename TY>
struct Queue
{
	Queue() = default;

	Queue(FlexKit::iAllocator& allocator)
		: items{ allocator }
	{
		items.resize(10);
	}

	Queue(const Queue& rhs)
	{
		std::scoped_lock lock(m, rhs.m);

		head = rhs.head;
		tail = rhs.tail;
		items = std::move(rhs.items);

		rhs.head = 0;
		rhs.tail = 0;
	}

	Queue(Queue&& rhs)
	{
		std::scoped_lock lock(m, rhs.m);

		head	= rhs.head;
		tail	= rhs.tail;
		items	= std::move(rhs.items);

		rhs.head = 0;
		rhs.tail = 0;
	}

	Queue& operator = (Queue&& rhs)
	{
		std::scoped_lock lock(m, rhs.m);

		head	= rhs.head;
		tail	= rhs.tail;
		items	= std::move(rhs.items);

		rhs.head = 0;
		rhs.tail = 0;

		return *this;
	}

	Queue& operator = (const Queue& rhs)
	{
		std::scoped_lock lock(m, rhs.m);

		head	= rhs.head;
		tail	= rhs.tail;
		items	= rhs.items;

		rhs.head = 0;
		rhs.tail = 0;

		return *this;
	}

	void push_front(const TY& e)
	{
		std::scoped_lock lock{ m };

		if ((tail - head) + 1 > items.size())
			items.resize(items.size() * 2);

		items[(tail++) % items.size()] = e;
	}

	std::optional<TY> pop_back()
	{
		std::scoped_lock lock{ m };

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
	std::mutex			m;
	FlexKit::Vector<TY> items;
};

struct MessageInterface : public FlexKit::SerializableInterface<GetTypeGUID(MessageInterface)>
{
	virtual void Do() = 0;
};

struct MessageBlob
{
	FlexKit::Vector<std::byte> buffer;
};

struct SharedEngineMemory
{
	char					blockTag[32];
	FlexKit::BlockAllocator	blockAllocator;
	SharedComponents*		components = nullptr;
	mapped_region			mapped;
	HWND					targetWindow;
	Queue<MessageBlob>		inputQueue;
	Queue<MessageBlob>		outputQueue;
};

SharedEngineMemory*	InitiateSharedMemory(shared_memory_object& obj);
SharedEngineMemory*	GetSharedMemory(shared_memory_object& obj, size_t offset);
void				ReleaseSharedEngineMemory(SharedEngineMemory&);

