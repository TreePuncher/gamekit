#pragma once

#include <Application.h>
#include <Win32Graphics.h>
#include <filesystem>
#include <expected>

class DescriptorHeapAllocator
{
public:
	DescriptorHeapAllocator(FlexKit::RenderSystem& IN_renderSystem, const size_t numDescCount, FlexKit::iAllocator* IN_allocator) :
		descHeap		{ IN_renderSystem._CreateShaderVisibleHeap(numDescCount) },
		root			{ .begin = 0, .end = numDescCount },
		freeList		{ IN_allocator },
		allocator		{ IN_allocator },
		renderSystem	{ &IN_renderSystem }
	{
		freeList.push_back(&root);
	}

	~DescriptorHeapAllocator()
	{
		if (descHeap)
		{
			descHeap->Release();
			descHeap = nullptr;
		}
	}

	
	struct Node
	{
		size_t begin;
		size_t end;
		size_t lockUntil;

		size_t BlockCount() const noexcept { return end - begin; }

		size_t FreeCount() const noexcept
		{
			if (left && right)
				return left->FreeCount() + right->FreeCount();
			if (free)
				return BlockCount();
			else
				return 0;
		}

		std::pair<size_t, size_t> SplitSizes()
		{
			const auto numBlocks = BlockCount();
			return { 3 * numBlocks >> 2, numBlocks >> 2 };
		}

		void Split(FlexKit::iAllocator* allocator)
		{
			auto lhs = &allocator->allocate<Node>();
			auto rhs = &allocator->allocate<Node>();
			const auto numBlocks = BlockCount();

			lhs->begin	= begin;
			lhs->end	= begin + 3 * (numBlocks >> 2);
			lhs->parent = this;

			rhs->begin	= begin + 3 * (numBlocks >> 2);
			rhs->end	= end;
			rhs->parent = this;

			left = lhs;
			right = rhs;
		}

		void Collapse(FlexKit::iAllocator* allocator)
		{
			allocator->release(left);
			allocator->release(right);

			left	= nullptr;
			right	= nullptr;
			free	= true;
		}

		bool free		= true;
		Node* left		= nullptr;
		Node* right		= nullptr;
		Node* parent	= nullptr;
	};

	size_t Alloc(size_t size)
	{
		for (auto& freeNode : freeList)
		{
			if (freeNode->BlockCount() > size)
			{
				auto node			= freeNode;
				auto potentialSplit	= node->SplitSizes();

				freeList.erase(std::remove(freeList.begin(), freeList.end(), freeNode), freeList.end());

				while(std::get<0>(potentialSplit) >= size || std::get<1>(potentialSplit) >= size)
				{
					node->Split(allocator);
					auto lhs = node->left;
					auto rhs = node->right;

					if (rhs->BlockCount() >= size)
					{
						node = rhs;
						freeList.push_back(lhs);
					}
					else
					{
						node = lhs;
						freeList.push_back(rhs);
					}

					potentialSplit = node->SplitSizes();
				}

				node->free = false;
				return node->begin;
			}
		}

		return -1;
	}

	Node* LocateNode(size_t offset)
	{
		auto node = &root;

		while (node->left && node->right)
		{
			if (node->left->begin <= offset && offset < node->left->end)
				node = node->left;
			else if (node->right->begin <= offset && offset < node->right->end)
				node = node->right;
		}

		if (!node->left && !node->right)
			return node;
		else
			return nullptr;
	}

	void Release(size_t offset)
	{
		auto node = LocateNode(offset);
		node->free = true;

		while (node->parent)
		{
			if (node->parent->left->free && node->parent->right->free)
			{
				auto n = node->parent->left != node ? node->parent->left : node->parent->right;

				freeList.erase(std::remove(freeList.begin(), freeList.end(), n), freeList.end());

				node = node->parent;

				node->Collapse(allocator);
			}
		}

		node->lockUntil = renderSystem->GetCurrentFrame() + 4;
		freeList.push_back(node);
	}


	Node					root;
	FlexKit::Vector<Node*>	freeList;
	FlexKit::iAllocator*	allocator;
	FlexKit::RenderSystem*	renderSystem;
	ID3D12DescriptorHeap*	descHeap;
};

class DescriptorAllocatorTest : public FlexKit::FrameworkState
{
public:
	DescriptorAllocatorTest(FlexKit::GameFramework& IN_framework);
	~DescriptorAllocatorTest() final;

	FlexKit::UpdateTask* Update(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT) final;
	FlexKit::UpdateTask* Draw(FlexKit::UpdateTask* update, FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT, FlexKit::FrameGraph& frameGraph) final;

	void PostDrawUpdate(FlexKit::EngineCore&, double dT) final;
	bool EventHandler(FlexKit::Event evt) final;

	DescriptorHeapAllocator			descHeapAllocator;
	FlexKit::MemoryPoolAllocator	gpuAllocator;
	FlexKit::CameraComponent		cameras;
	FlexKit::SceneNodeComponent		sceneNodes;

	bool							pause = false;
	bool							debugVis = false;

	FlexKit::RootSignature			sortingRootSignature;
	FlexKit::Win32RenderWindow		renderWindow;

	FlexKit::ConstantBufferHandle	constantBuffer;

	FlexKit::RunOnceQueue<void(FlexKit::UpdateDispatcher&, FlexKit::FrameGraph&)>	runOnceQueue;
};
