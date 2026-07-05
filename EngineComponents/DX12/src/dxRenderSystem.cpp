#include "..\pch.h"
#include "BuildSettings.hpp"

#include "AnimationUtilities.hpp"
#include "Containers.hpp"
#include "DDSUtilities.hpp"
#include "dxVertexBufferSet.hpp"
#include "dxRenderSystem.hpp"
#include "dxPipelineBuilder.hpp"
#include "dxDescriptorSet.hpp"
#include "Logging.hpp"
#include "MemoryUtilities.hpp"
#include "ThreadUtilities.hpp"
#include "ShaderPreprocessor.hpp"

#include <algorithm>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <filesystem>
#include <fmt\format.h>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <iostream>
#include <ranges>
#include <Windows.h>
#include <stacktrace>

#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <directx/d3d12sdklayers.h>
#include <DirectXMath.h>


extern "C" __declspec(dllexport) DWORD  NvOptimusEnablement = 1;
extern "C" __declspec(dllexport) int    AmdPowerXpressRequestHighPerformance = 1;

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion    = 619; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath      = ".\\"; }


namespace dx_Internal
{

	using namespace FlexKit;

	using std::ranges::sort;
	using std::views::iota;
	using std::views::zip;


	/************************************************************************************************/


	void SetDebugName(ID3D12Object* Obj, const char* cstr, size_t size)
	{
#if USING(DEBUGGRAPHICS)
		if (!Obj)
			return;

		const size_t StringSize = 128;
		size_t ConvertedCount = 0;
		wchar_t WString[StringSize];
		mbstowcs_s(&ConvertedCount, WString, cstr, StringSize);
		Obj->SetName(WString);
#endif
	}


	/************************************************************************************************/


    DevicePointer GetDevicePointer(const VertexBuffer& vb_ref) noexcept
	{
		return vb_ref.resource.As<ID3D12Resource>()->GetGPUVirtualAddress();
	}


	/************************************************************************************************/


	UAVBuffer::UAVBuffer(dxRenderSystem& rs, const ResourceHandle handle, const size_t IN_stride, const size_t IN_offset)
	{
		FK_ASSERT(IN_offset < std::numeric_limits<uint32_t>::max());

		auto uavLayout	= rs.GetUAVBufferLayout(handle);
		auto bufferSize = rs.GetUAVBufferSize(handle);

		resource		= rs.GetDeviceResource(handle).As<ID3D12Resource>();
		stride			= (uint32_t)(IN_stride == -1 ? uavLayout.stride : IN_stride);
		elementCount	= (uint32_t)bufferSize / stride;
		counterOffset	= 0;
		offset			= (uint32_t)IN_offset;
		format			= uavLayout.format;
	}

	/************************************************************************************************/


	DescriptorHeapAllocator::~DescriptorHeapAllocator()
	{
		if (descHeap)
		{
			root.Release(allocator);
			freeList.clear();

			descHeap->Release();

			descHeap		= nullptr;
			allocator		= nullptr;
			renderSystem	= nullptr;
		}
	}


	/************************************************************************************************/


	std::optional<DescriptorRange> DescriptorHeapAllocator::Alloc_ST(const size_t size, uint64_t completedIdx) noexcept
	{
		auto cmp_less = [](Node* lhs, Node* rhs) { return lhs->BlockCount() < rhs->BlockCount(); };

		if (freeList.size() > 64) std::ranges::partial_sort(freeList, freeList.begin() + 32, cmp_less );
		else
			std::ranges::sort(freeList, cmp_less);

		for (auto& freeNode : freeList)
		{
			if (freeNode->BlockCount() > size && freeNode->lockUntil <= completedIdx)
			{
				auto node						= freeNode;
				auto potentialSplit				= node->SplitSizes();
				auto& [leftSplit, rightSplit]	= potentialSplit;

				freeList.erase(std::remove(freeList.begin(), freeList.end(), freeNode), freeList.end());

				while((leftSplit > size || rightSplit > size)  && (leftSplit > 0 && rightSplit > 0))
				{
					node->Split(allocator);
					auto lhs = node->left;
					auto rhs = node->right;

					if (rhs->BlockCount() == size)
					{
						node = rhs;
						freeList.push_back(lhs);
						break;
					}
					else if (lhs->BlockCount() == size)
					{
						node = lhs;
						freeList.push_back(rhs);
						break;
					}
					else if (rhs->BlockCount() > size)
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

				const auto offset = descriptorSize * node->begin;

				if (size == 0)
					DebugBreak();

				return DescriptorRange{
					.begin	= { { (uint64_t)cpuHeap.ptr + offset },
								{ (uint64_t)gpuHeap.ptr + offset } },
					.size	= (uint32_t)size,
					.stride = (uint32_t)descriptorSize,
				};
			}
		}

		DebugBreak();
		return {};
	}


	/************************************************************************************************/


	void DescriptorHeapAllocator::Initialize(dxRenderSystem& IN_renderSystem, const size_t numDescCount, FlexKit::iAllocator* IN_allocator)
	{
		descHeap		= IN_renderSystem._CreateShaderVisibleHeap(numDescCount);
		renderSystem	= &IN_renderSystem;
		root			= Node{ .begin = 0, .end = numDescCount };
		freeList		= FlexKit::Vector<Node*>{ IN_allocator };
		allocator		= IN_allocator;

		freeList.push_back(&root);

		gpuHeap = descHeap->GetGPUDescriptorHandleForHeapStart();
		cpuHeap = descHeap->GetCPUDescriptorHandleForHeapStart();

		descriptorSize = renderSystem->DescriptorCBVSRVUAVSize;
	}


	/************************************************************************************************/


	auto DescriptorHeapAllocator::Alloc(const size_t size, uint64_t completedIdx) noexcept
	{
		std::scoped_lock lock{ mutex };

		return Alloc_ST(size, completedIdx);
	}


	/************************************************************************************************/


	void DescriptorHeapAllocator::Release_ST(const DescriptorRange range, uint64_t lockIdx, uint64_t completedIdx) noexcept
	{
		auto& [cpu_ptr, gpu_ptr] = range.begin;
		auto offset1 = (gpu_ptr - gpuHeap.ptr) / descriptorSize;
		auto offset2 = (cpu_ptr - cpuHeap.ptr) / descriptorSize;

		FK_ASSERT(offset1 == offset2); // quick sanity check

		auto node = LocateNode(offset1);
		node->free		= true;
		node->lockUntil = lockIdx;

		while (node->parent)
		{
			if (node->parent->left->Collapsable(completedIdx) && node->parent->right->Collapsable(completedIdx))
			{
				auto n = node->parent->left != node ? node->parent->left : node->parent->right;

				freeList.erase(std::remove(freeList.begin(), freeList.end(), n), freeList.end());

				node = node->parent;

				node->Collapse(allocator);
			}
			else
				break;
		}

		freeList.push_back(node);
	}


	/************************************************************************************************/


	void DescriptorHeapAllocator::Release(const DescriptorRange range, uint64_t lockIdx, uint64_t completedIdx)
	{
		std::scoped_lock lock{mutex};

		Release_ST(range, lockIdx, completedIdx);
	}


	/************************************************************************************************/


	size_t DescriptorHeapAllocator::Node::FreeCount() const noexcept
	{
		if (left && right)
			return left->FreeCount() + right->FreeCount();
		if (free)
			return BlockCount();
		else
			return 0;
	}


	/************************************************************************************************/


	std::pair<size_t, size_t> DescriptorHeapAllocator::Node::SplitSizes()
	{
		const auto numBlocks = BlockCount();
		return { 3 * numBlocks >> 2, numBlocks >> 2 };
	}


	/************************************************************************************************/


	void DescriptorHeapAllocator::Node::Split(FlexKit::iAllocator* allocator)
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


	/************************************************************************************************/


	void DescriptorHeapAllocator::Node::Collapse(FlexKit::iAllocator* allocator)
	{
		allocator->release(left);
		allocator->release(right);

		left	= nullptr;
		right	= nullptr;
		free	= true;
	}


	/************************************************************************************************/


	bool DescriptorHeapAllocator::Node::Collapsable(uint64_t completed)
	{
		return (free && (lockUntil + 2) < completed);
	}


	/************************************************************************************************/


	void DescriptorHeapAllocator::Node::Release(iAllocator* allocator)
	{
		if (left)
		{
			left->Release(allocator);
			allocator->free(left);
		}

		if (right)
		{
			right->Release(allocator);
			allocator->free(right);
		}

		left	= nullptr;
		right	= nullptr;
	}

	/************************************************************************************************/


	DescriptorHeapAllocator::Node* DescriptorHeapAllocator::LocateNode(size_t offset)
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


	/************************************************************************************************/


	ConstantBufferHandle	ConstantBufferTable::CreateConstantBuffer(uint32_t BufferSize, bool GPUResident)
	{
		D3D12_RESOURCE_DESC   Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(BufferSize);
		Resource_DESC.Alignment				= 0;
		Resource_DESC.DepthOrArraySize		= 1;
		Resource_DESC.Dimension				= D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		Resource_DESC.Layout				= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Resource_DESC.Height				= 1;
		Resource_DESC.Format				= DXGI_FORMAT_UNKNOWN;
		Resource_DESC.SampleDesc.Count		= 1;
		Resource_DESC.SampleDesc.Quality	= 0;
		Resource_DESC.Flags					= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;//D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE; // Causes Graphics Debugger to crash

		D3D12_HEAP_PROPERTIES HEAP_Props	={};
		HEAP_Props.CPUPageProperty			= D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type						= GPUResident ? D3D12_HEAP_TYPE_DEFAULT : D3D12_HEAP_TYPE_UPLOAD;
		HEAP_Props.MemoryPoolPreference		= D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask			= 0;
		HEAP_Props.VisibleNodeMask			= 0;

		constexpr size_t bufferCount = 3;
		ID3D12Resource*  resources[3];

		D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;

		for(size_t I = 0; I < bufferCount; ++I)
		{
			ID3D12Resource* Resource = nullptr;
			HRESULT HR = renderSystem->pDevice->CreateCommittedResource(
							&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, 
							&Resource_DESC, InitialState, nullptr,
							IID_PPV_ARGS(&Resource));

			CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE CONSTANT BUFFER"));
			resources[I] = Resource;

			SETDEBUGNAME(Resource, __func__);
		}

		std::scoped_lock lock{ criticalSection };
		const uint32_t BufferIdx = uint32_t(buffers.size());

		void* Mapped_ptr = nullptr;

		if(!GPUResident)
			resources[0]->Map(0, nullptr, &Mapped_ptr);

		ConstantBufferHandle handle = handles.GetNewHandle();

		UserConstantBuffer buffer = {
			BufferSize,
			0,
			Mapped_ptr,

			GPUResident,
			false,

			{ 0, 0, 0 },
			0,
			handle,

			{ resources[0], resources[1], resources[2], },
		};


		handles[handle] = (uint32_t)buffers.push_back(buffer);

		return handle;
	}


	/************************************************************************************************/


	ID3D12Resource* ConstantBufferTable::GetDeviceResource(const ConstantBufferHandle handle) const
	{
		auto& buffer = buffers[handles[handle]];

		return buffer.resources[buffer.currentRes];
	}


	/************************************************************************************************/


	size_t ConstantBufferTable::GetBufferOffset(const ConstantBufferHandle handle) const
	{
		return buffers[handles[handle]].offset;
	}


	/************************************************************************************************/


	size_t ConstantBufferTable::GetBufferSize(const ConstantBufferHandle Handle) const
	{
		auto& buffer = buffers[handles[Handle]];

		return buffer.size;
	}


	/************************************************************************************************/


	std::optional<size_t> ConstantBufferTable::Push(ConstantBufferHandle handle, void* _Ptr, size_t pushSize)
	{
		auto& buffer = buffers[handles[handle]];

		if (buffer.GPUResident)
			return false; // Cannot directly push to GPU Resident Memory

		const char* Debug_mapped_Ptr = (char*)buffer.mapped_ptr;

		const uint32_t size		= (uint32_t)buffer.size;
		const uint32_t offset	= (uint32_t)buffer.offset;
		const char*  mapped_Ptr	= (char*)buffer.mapped_ptr;

		if (!mapped_Ptr)
			return {};

		if (size < offset + pushSize)
			return {}; // Buffer To small to accommodate Push

		buffer.offset += (uint32_t)pushSize;

		if(!buffer.writeFlag)
			buffer.writeFlag = true;

		FK_ASSERT(pushSize % 256 == 0); // size requests must be blocks of 256

		if(_Ptr)
			memcpy((void*)(mapped_Ptr + offset), _Ptr, pushSize);

		return { offset };
	}


	/************************************************************************************************/


	SubAllocation ConstantBufferTable::Reserve(ConstantBufferHandle CB, size_t reserveSize)
	{
		const auto res = Push(CB, nullptr, reserveSize);

		if (!res.has_value())
			DebugBreak();

		FK_ASSERT(res.has_value());

		const size_t UserIdx	= handles[CB];
		void*		 buffer		= buffers[UserIdx].mapped_ptr;

		return { static_cast<char*>(buffer), res.value_or(0), reserveSize };
	}


	/************************************************************************************************/


	void ConstantBufferTable::ReleaseBuffer(ConstantBufferHandle handle)
	{
		std::scoped_lock lock(criticalSection);

		const size_t UserIdx	= handles[handle];
		auto&  buffer			= buffers[UserIdx];

		for (auto& res : buffer.resources)
		{
			if (res)
				res->Release();

			res = nullptr;
		}

		buffer = buffers.back();

		handles[buffer.handle] = handles[handle];

		buffers.pop_back();
	}


	/************************************************************************************************/


	void ConstantBufferTable::Reset(ConstantBufferHandle Handle)
	{
		ProfileFunctionTextName(ConstantBufferTableReset);

		const size_t UserIdx	= handles[Handle];
		auto& buffer			= buffers[UserIdx];

		if (buffer.offset == 0)
			return; // unused

		buffer.locks[buffer.currentRes] = renderSystem->GetCurrentCounter();
		buffer.currentRes = (buffer.currentRes + 1) % 3;

		renderSystem->WaitFor(buffer.locks[buffer.currentRes]);

		char* mapped_Ptr	= nullptr;
		auto HR				= buffer.resources[buffer.currentRes]->Map(0, nullptr, (void**)&mapped_Ptr);
		FK_ASSERT(FAILED(HR), "Failed to map Constant Buffer");

		buffer.mapped_ptr	= mapped_Ptr;
		buffer.offset		= 0;
	}


	/************************************************************************************************/


	bool RootSignatureBuilder::SetParameterAsUINT(size_t Index, uint32_t size, PIPELINE AccessableStages)
	{
		RootEntry Desc;
		Desc.Type							= RootSignatureEntryType::UINT;
		Desc.UINTConstant.size              = size;
		Desc.UINTConstant.Register		    = Index;
		Desc.UINTConstant.RegisterSpace     = 0xffffff00;
		Desc.UINTConstant.Accessibility	    = AccessableStages;

		if (RootEntries.size() <= Index)
		{
			if (!RootEntries.full())
				RootEntries.resize(Index + 1);
			else
				return false;
		}

		RootEntries[Index]  = Desc;

		return true;
	}


	/************************************************************************************************/


    bool RootSignatureBuilder::SetParameterAsDescriptorSet(size_t index, const DescriptorSetLayout& layout, PIPELINE accessableStages)
	{
		RootEntry Desc;
		Desc.Type							= RootSignatureEntryType::DescriptorHeap;
		Desc.DescriptorHeap.HeapIdx			= Heaps.push_back({ index, layout });
		Desc.DescriptorHeap.Accessibility	= accessableStages;

		if (RootEntries.size() <= index)
		{
			if (!RootEntries.full()) {
				RootEntries.resize(index + 1);
			}
			else
				return false;
		}

		RootEntries[index] = Desc;

		return true;
	}


	/************************************************************************************************/


	bool RootSignatureBuilder::SetParameterAsCBV(size_t Index, PIPELINE AccessableStages)
	{
		RootEntry Desc;
		Desc.Type					= RootSignatureEntryType::ConstantBuffer;
		Desc.Direct.Register		= Index;
		Desc.Direct.RegisterSpace	= 0xffffff00;
		Desc.Direct.Accessibility	= AccessableStages;


		if (RootEntries.size() <= Index)
		{
			if (!RootEntries.full())
				RootEntries.resize(Index + 1);
			else
				return false;
		}

		RootEntries[Index] = Desc;

		return false;
	}


	/************************************************************************************************/


	bool RootSignatureBuilder::SetParameterAsUAV(size_t Index, PIPELINE AccessableStages)
	{
		RootEntry Desc;
		Desc.Type					= RootSignatureEntryType::UnorderedAccess;
		Desc.Direct.Register		= Index;
		Desc.Direct.RegisterSpace	= 0xffffff00;
		Desc.Direct.Accessibility	= AccessableStages;


		if (RootEntries.size() <= Index)
		{
			if (!RootEntries.full())
				RootEntries.resize(Index + 1);
			else
				return false;
		}

		RootEntries[Index] = Desc;

		return true;
	}


	/************************************************************************************************/


	bool RootSignatureBuilder::SetParameterAsSRV(
		size_t Index, PIPELINE AccessableStages)
	{
		RootEntry Desc;
		Desc.Type					= RootSignatureEntryType::StructuredBuffer;
		Desc.Direct.Register		= (uint32_t)Index;
		Desc.Direct.RegisterSpace	= (uint32_t)0xffffff00;
		Desc.Direct.Accessibility	= AccessableStages;

		if (RootEntries.size() <= Index)
		{
			if (!RootEntries.full())
				RootEntries.resize(Index + 1);
			else
				return false;
		}

		RootEntries[Index] = Desc;

		return true;
	}


	/************************************************************************************************/


	void RootSignatureBuilder::Clear()
	{
		Heaps.clear();
		RootEntries.clear();

		AllowIA = true;
		AllowSO = false;
	}


	/************************************************************************************************/


	IPipelineInterface* RootSignatureBuilder::Build(iAllocator& temp)
	{
		auto result = dxRenderSystem::_GetInstance()._CreateRootSignature(*this, temp);

		if (result)
		{
			Clear();
		}
		else
			FK_LOG_ERROR("Failed to build root signature!");

		return result;
	}


	/************************************************************************************************/


	IPipelineInterface* RootSignatureBuilder::LoadSignatureFromFile(const char* dir, const char* entry, iAllocator& temp)
	{
		auto& renderSystem = dxRenderSystem::_GetInstance();
		auto result = renderSystem.LoadRootSignature(dir, entry);
		
		if(result)
		{
			auto&& rootSignature = result.value();
			
			ID3D12VersionedRootSignatureDeserializer* deserializer;
			auto res  = D3D12CreateVersionedRootSignatureDeserializer(rootSignature.buffer, rootSignature.bufferSize, IID_PPV_ARGS(&deserializer));

			if (!deserializer)
			{
				std::string trace;

				for (const auto& frame : std::stacktrace::current())
					trace += "\t" + frame.description() + "\n";

				FK_LOG_ERROR("Failed to deserialize root signature!\nStack trace:\n%s", trace.c_str());

				return nullptr;
			}

			const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* versioned_desc;
			deserializer->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_1, &versioned_desc);
			auto desc = &versioned_desc->Desc_1_1;

			size_t parametersEnd = desc->NumParameters;
			for(size_t itr = 0; itr < parametersEnd; itr++)
			{
				switch(desc->pParameters[itr].ParameterType)
				{
					case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
					{
						auto& parameter = desc->pParameters[itr].DescriptorTable;
						
						for(auto&& [idx, range] : zip(iota(0), std::span{ parameter.pDescriptorRanges, parameter.NumDescriptorRanges}))
						{
							DescriptorSetLayout layout;
							switch(range.RangeType)
							{
							case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
							{
								layout.entries.push_back(HeapDescriptor{
										.registerIdx	= range.BaseShaderRegister,
										.count			= range.NumDescriptors,
										.type			= DescHeapEntryType::SRV,
								    });
							}	break;
							case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
							{
								layout.entries.push_back(HeapDescriptor{
								        .registerIdx	= range.BaseShaderRegister,
								        .count			= range.NumDescriptors,
								        .type			= DescHeapEntryType::UAV,
									});
							}	break;
							case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
							{
								layout.entries.push_back(HeapDescriptor{
										.registerIdx	= range.BaseShaderRegister,
										.count			= range.NumDescriptors,
										.type			= DescHeapEntryType::CBV,
									});
							}	break;
							case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER: 
							{
								FK_ASSERT(0, "Unimplemented funcionality!");
							}	break;
							}

							Heaps.emplace_back(Heaps.size(), layout);
						}
					
						SetParameterAsDescriptorSet(itr, Heaps.back().heap, ShaderVis2PipelineDest(desc->pParameters[itr].ShaderVisibility));
					}	break;
					case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
					{
						auto& parameter = desc->pParameters[itr].Constants;
						SetParameterAsUINT(itr, parameter.Num32BitValues, ShaderVis2PipelineDest(desc->pParameters[itr].ShaderVisibility));
					}	break;
					case D3D12_ROOT_PARAMETER_TYPE_CBV:
					{
						auto& parameter = desc->pParameters[itr].Descriptor;
						SetParameterAsCBV(itr, ShaderVis2PipelineDest(desc->pParameters[itr].ShaderVisibility));
					}	break;
					case D3D12_ROOT_PARAMETER_TYPE_SRV:
					{
						auto& parameter = desc->pParameters[itr].Descriptor;
						SetParameterAsSRV(itr, ShaderVis2PipelineDest(desc->pParameters[itr].ShaderVisibility));
					}	break;
					case D3D12_ROOT_PARAMETER_TYPE_UAV:
					{
						auto& parameter = desc->pParameters[itr].Descriptor;
						SetParameterAsUAV(itr, ShaderVis2PipelineDest(desc->pParameters[itr].ShaderVisibility));
					}	break;
				}
			}

			deserializer->Release();

			auto signature = renderSystem._CreateRootSignature(*this, temp);

			if (!signature)
				return nullptr;

			Clear();

			return signature;
		}
		else
			FK_LOG_ERROR("LoadSignatureFromFile failed to load signature. Reason: %s", result.error().c_str());

			return nullptr;
	}


	/************************************************************************************************/


	IPipelineInterface* RootSignatureBuilder::LoadSignatureFromBlob(void* buffer, const size_t bufferSize, iAllocator& temp)
	{
		auto& renderSystem = dxRenderSystem::_GetInstance();

		ID3D12VersionedRootSignatureDeserializer* deserializer;
		auto HR  = D3D12CreateVersionedRootSignatureDeserializer(buffer, bufferSize, IID_PPV_ARGS(&deserializer));
		if (FAILED(HR))
			return nullptr;

		const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* versioned_desc;
		HR	= deserializer->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_1, &versioned_desc);

		if (FAILED(HR))
			return nullptr;

		ID3D12RootSignature* dxRootSig = nullptr;
		HR = renderSystem.pDevice15->CreateRootSignature(0, buffer, bufferSize, IID_PPV_ARGS(&dxRootSig));

		if (FAILED(HR))
			return nullptr;

		auto desc = &versioned_desc->Desc_1_1;

		size_t parametersEnd = desc->NumParameters;
		for(size_t itr = 0; itr < parametersEnd; itr++)
		{
			switch(desc->pParameters[itr].ParameterType)
			{
				case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
				{
					auto& parameter = desc->pParameters[itr].DescriptorTable;
					DescriptorSetLayout layout{ *renderSystem.allocator };

					for(auto&& [idx, range] : zip(iota(0), std::span{ parameter.pDescriptorRanges, parameter.NumDescriptorRanges}))
					{
						switch(range.RangeType)
						{
						case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
						{
							layout.entries.push_back(HeapDescriptor{
									.registerIdx	= range.BaseShaderRegister,
									.count			= range.NumDescriptors,
									.type			= DescHeapEntryType::SRV,
								});
						}	break;
						case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
						{
							layout.entries.push_back(HeapDescriptor{
									.registerIdx	= range.BaseShaderRegister,
									.count			= range.NumDescriptors,
									.type			= DescHeapEntryType::UAV,
								});
						}	break;
						case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
						{
							layout.entries.push_back(HeapDescriptor{
								.registerIdx	= range.BaseShaderRegister,
								.count			= range.NumDescriptors,
								.type			= DescHeapEntryType::CBV,
								});
						}	break;
						case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER: 
						{
							FK_ASSERT(0, "Unimplemented funcionality!");
						}	break;
						}
					}
					
					SetParameterAsDescriptorSet(itr, layout, ShaderVis2PipelineDest(desc->pParameters[itr].ShaderVisibility));
				}	break;
				case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
				{
					auto& parameter = desc->pParameters[itr].Constants;

					RootEntry Desc;
					Desc.Type = RootSignatureEntryType::UINT;
					Desc.UINTConstant.size			= parameter.Num32BitValues;
					Desc.UINTConstant.Register		= parameter.ShaderRegister;
					Desc.UINTConstant.RegisterSpace = parameter.RegisterSpace;
					Desc.UINTConstant.Accessibility = ShaderVis2PipelineDest(desc->pParameters[itr].ShaderVisibility);
					RootEntries.push_back(Desc);
				}	break;
				case D3D12_ROOT_PARAMETER_TYPE_CBV:
				{
					auto& parameter = desc->pParameters[itr].Descriptor;
					RootEntry Desc;
					Desc.Type					= RootSignatureEntryType::ConstantBuffer;
					Desc.Direct.Register		= parameter.ShaderRegister;
					Desc.Direct.RegisterSpace	= parameter.RegisterSpace;
					Desc.Direct.Accessibility	= ShaderVis2PipelineDest(desc->pParameters[itr].ShaderVisibility);
					RootEntries.push_back(Desc);
				}	break;
				case D3D12_ROOT_PARAMETER_TYPE_SRV:
				{
					auto& parameter = desc->pParameters[itr].Descriptor;
					RootEntry Desc;
					Desc.Type					= RootSignatureEntryType::StructuredBuffer;
					Desc.Direct.Register		= parameter.ShaderRegister;
					Desc.Direct.RegisterSpace	= parameter.RegisterSpace;
					Desc.Direct.Accessibility	= ShaderVis2PipelineDest(desc->pParameters[itr].ShaderVisibility);
					RootEntries.push_back(Desc);
				}	break;
				case D3D12_ROOT_PARAMETER_TYPE_UAV:
				{
					auto& parameter = desc->pParameters[itr].Descriptor;
					RootEntry Desc;
					Desc.Type					= RootSignatureEntryType::UnorderedAccess;
					Desc.Direct.Register		= parameter.ShaderRegister;
					Desc.Direct.RegisterSpace	= parameter.RegisterSpace;
					Desc.Direct.Accessibility	= ShaderVis2PipelineDest(desc->pParameters[itr].ShaderVisibility);
					RootEntries.push_back(Desc);
				}	break;
			}
		}

		auto signature = renderSystem._CreateRootSignature(dxRootSig, *this);

		deserializer->Release();

		Clear();

		return signature;
	}


	/************************************************************************************************/


	void RootSignature::Release()
	{
		if(signature && !signature->Release())
		{
			auto* mutable_this = const_cast<RootSignature*>(this);

			mutable_this->heaps.Release();

			auto t = (uint64_t)signature;
			mutable_this->signature = nullptr;
			dxRenderSystem::_GetInstance()._ReleaseRootSignature(t);
			allocator->free(mutable_this);
		}
	}


	/************************************************************************************************/


	size_t RootSignature::GetDescriptorTableSize(size_t idx) const
	{
		FK_ASSERT(idx < heaps.size());
		return heaps[idx].heap.size();
	}


	/************************************************************************************************/


	void dxRenderSystem::RootSigLibrary::Initiate(dxRenderSystem* RS, iAllocator& allocator, iAllocator& temp)
	{
		ID3D12Device* Device = RS->pDevice;

		/*	CD3DX12_STATIC_SAMPLER_DESC(
			UINT shaderRegister,
			D3D12_FILTER filter                      = D3D12_FILTER_ANISOTROPIC,
			D3D12_TEXTURE_ADDRESS_MODE addressU      = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE addressV      = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE addressW      = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			FLOAT mipLODBias                         = 0,
			UINT maxAnisotropy                       = 16,
			D3D12_COMPARISON_FUNC comparisonFunc     = D3D12_COMPARISON_FUNC_LESS_EQUAL,
			D3D12_STATIC_BORDER_COLOR borderColor    = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
			FLOAT minLOD                             = 0.f,
			FLOAT maxLOD                             = D3D12_FLOAT32_MAX,
			D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
			UINT registerSpace                       = 0)
		{
		*/

		RootSignatureBuilder builder{ allocator };

		CD3DX12_STATIC_SAMPLER_DESC	 Samplers[] = {
			CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT, 
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP),

			CD3DX12_STATIC_SAMPLER_DESC{1, D3D12_FILTER::D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR },
			CD3DX12_STATIC_SAMPLER_DESC{2, D3D12_FILTER::D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT},
		};

		builder.SetParameterAsUINT(0, 6, PIPELINE_DEST_CS);
		builder.SetParameterAsUAV(1, PIPELINE_DEST_CS);
		ClearBuffer = (RootSignature*)builder.Build(temp);

		SETDEBUGNAME(*ClearBuffer, "ClearBuffer");
	}


	/************************************************************************************************/
	

	CopyContext& CopyEngine::operator [](CopyContextHandle handle)
	{
		return copyContexts[handle];
	}


	/************************************************************************************************/


	CopyContextHandle CopyEngine::Open()
	{
		const size_t currentIdx	= idx++ % copyContexts.size();
		auto& ctx				= copyContexts[currentIdx];

		Wait(CopyContextHandle{ currentIdx });

		if (FAILED(ctx.commandAllocator->Reset()))
			__debugbreak();

		if (FAILED(ctx.commandList->Reset(ctx.commandAllocator, nullptr)))
			__debugbreak();

		return CopyContextHandle{ currentIdx };
	}


	/************************************************************************************************/


	void CopyEngine::Wait(SyncPoint syncTo)
	{
		copyQueue->Wait(syncTo.fence.As<ID3D12Fence>(), syncTo.syncCounter);
	}


	/************************************************************************************************/


	void CopyEngine::Wait(CopyContextHandle handle)
	{
		auto& ctx = copyContexts[handle];

		if (fence->GetCompletedValue() < ctx.counter)
		{
			fence->SetEventOnCompletion(ctx.counter, ctx.eventHandle);
			WaitForSingleObject(ctx.eventHandle, 0xffffffff);
		}
	}


	/************************************************************************************************/


	void CopyEngine::Close(CopyContextHandle handle)
	{
		auto& ctx = copyContexts[handle];
		ctx.flushPendingBarriers();

		if (FAILED(ctx.commandList->Close()))
		{
			FK_LOG_ERROR("Failed to close Copy Command list!");
			__debugbreak();
		}
	}


	/************************************************************************************************/


	void CopyEngine::Submit(CopyContextHandle* begin, CopyContextHandle* end, std::optional<SyncPoint> syncOpt)
	{
		const size_t localCounter = ++counter;
		static_vector<ID3D12CommandList*, 64> cmdLists;

		for (auto itr = begin; itr < end; itr++)
		{
			auto& context = copyContexts[*itr];
			cmdLists.push_back(context.commandList);

			context.counter				= localCounter;
			context.uploadBuffer.last	= context.uploadBuffer.position;
			
			Close(*itr);
		}

		if (auto syncPoint = syncOpt.value_or(SyncPoint{}); syncOpt.has_value())
			copyQueue->Wait(syncPoint.fence.As<ID3D12Fence>(), syncPoint.syncCounter);

		copyQueue->ExecuteCommandLists((UINT)cmdLists.size(), cmdLists);

		if (const auto HR = copyQueue->Signal(fence, localCounter); FAILED(HR))
			FK_LOG_ERROR("FAILED TO SUBMIT TO COPY ENGINE!");
	}


	/************************************************************************************************/


	void CopyEngine::Signal(ID3D12Fence* fence, const size_t counter)
	{
		if (auto HR = copyQueue->Signal(fence, counter); FAILED(HR))
			FK_LOG_ERROR("Failed to Signal");
	}

	void CopyEngine::Signal(SyncPoint sync)
	{
		if (auto HR = copyQueue->Signal(sync.fence.As<ID3D12Fence>(), sync.syncCounter); FAILED(HR))
			FK_LOG_ERROR("Failed to Signal");
	}


	/************************************************************************************************/


	void CopyEngine::Push_Temporary(ID3D12Resource* resource, CopyContextHandle handle)
	{
		copyContexts[handle].freeResources.push_back(resource);
	}


	/************************************************************************************************/


	void CopyEngine::Release()
	{
		if (!fence)
			return;

		for (auto& ctx : copyContexts)
		{
			if (fence->GetCompletedValue() < ctx.counter)
			{
				fence->SetEventOnCompletion(ctx.counter, ctx.eventHandle);
				WaitForSingleObject(ctx.eventHandle, 0xffffffff);
			}

			for (auto resource : ctx.freeResources)
				resource->Release();

			ctx.uploadBuffer.Release();
		}

		fence->Release();
		fence = nullptr;

		for (auto& ctx : copyContexts)
		{
			ctx.commandAllocator->Release();
			ctx.commandList->Release();
		}

		copyContexts.clear();
		copyQueue->Release();
	}


	/************************************************************************************************/


	bool CopyEngine::Initiate(ID3D12Device* Device, const size_t threadCount, Vector<ID3D12DeviceChild*>& ObjectsCreated, iAllocator* allocator)
	{
		bool Success = true;


		D3D12_COMMAND_QUEUE_DESC copyQueue_Desc = {};
		copyQueue_Desc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
		copyQueue_Desc.Type  = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY;

		auto HR = Device->CreateCommandQueue(&copyQueue_Desc, IID_PPV_ARGS(&copyQueue));	FK_ASSERT(FAILED(HR), "FAILED TO CREATE COPY QUEUE!");

		SETDEBUGNAME(copyQueue, "UPLOAD QUEUE");

		for (size_t I = 0; I < threadCount && Success; ++I)
		{
			ID3D12CommandAllocator*		commandAllocator	= nullptr;
			ID3D12GraphicsCommandList2*	copyCommandList		= nullptr;

			auto HR = Device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY,
				IID_PPV_ARGS(&commandAllocator));

			ObjectsCreated.push_back(commandAllocator);
				

#ifdef _DEBUG
			FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND ALLOCATOR!");
			Success &= !FAILED(HR);
#endif
			if (!Success)
				break;

			ObjectsCreated.push_back(commandAllocator);


			HR = Device->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY,
				commandAllocator,
				nullptr,
				__uuidof(ID3D12CommandList),
				(void**)&copyCommandList);


			FK_ASSERT	(FAILED(HR),        "FAILED TO CREATE COMMAND LIST!");
			FK_VLOG		(10,                "COPY COMMANDLIST CREATED: %u", copyCommandList);
			SETDEBUGNAME(commandAllocator,  "COPY ALLOCATOR");
			SETDEBUGNAME(copyCommandList,   "COPY COMMAND LIST");

			ObjectsCreated.push_back(copyCommandList);

			Success &= !FAILED(HR);

			copyCommandList->Close();
			commandAllocator->Reset();

			CopyContext copyCtx;
			copyCtx.commandAllocator	= commandAllocator;
			copyCtx.commandList			= copyCommandList;
			copyCtx.eventHandle			= CreateEvent(nullptr, FALSE, FALSE, nullptr);
			copyCtx.uploadBuffer		= dxUploadBuffer{ Device };
			copyCtx.freeResources		= Vector<ID3D12Resource*>{ allocator };

			copyContexts.emplace_back(std::move(copyCtx));
			/*
			copyContexts.push_back(
				CopyContext{
					commandAllocator,
					copyCommandList,
					0,
					CreateEvent(nullptr, FALSE, FALSE, nullptr),
					dxUploadBuffer{ Device },
					Vector<ID3D12Resource*>{ allocator }});
			*/
			}

		Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

		return Success;
	}


	/************************************************************************************************/


	LoadPipelineStateRes CreateClearBufferPSO(IRenderSystem& irs, iAllocator& allocator)
	{
		auto& RS = static_cast<dxRenderSystem&>(irs);
		PipelineBuilder builder{irs, allocator};
		builder.AddComputeShader("Clear", R"(assets\shaders\ClearBuffer.hlsl)");

        return  builder.Build(irs, allocator);
	}


	/************************************************************************************************/


	HeapTable::HeapTable(ID3D12Device* IN_device, iAllocator* allocator) :
		pDevice	{ IN_device },
		handles	{ allocator },
		heaps	{ allocator } {}


	/************************************************************************************************/


	HeapTable::~HeapTable()
	{
		Release();
	}


	/************************************************************************************************/


	void HeapTable::Release()
	{
		for (auto& heap : heaps)
			heap.Release();

		heaps.clear();
		handles.Clear();
	}


	/************************************************************************************************/


	void HeapTable::Init(ResourceHeapTier IN_tier, ID3D12Device* IN_device)
	{
		tier	= IN_tier;
		pDevice = IN_device;
	}


	/************************************************************************************************/


	DeviceHeapHandle HeapTable::CreateHeap(const size_t size, const uint32_t flags)
	{
		D3D12_HEAP_PROPERTIES HEAP_Props	={};
		HEAP_Props.CPUPageProperty			= D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type						= D3D12_HEAP_TYPE_DEFAULT;
		HEAP_Props.MemoryPoolPreference		= D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask			= 0;
		HEAP_Props.VisibleNodeMask			= 0;

		D3D12_HEAP_DESC heapDesc;

		switch (tier)
		{
		case ResourceHeapTier::HeapTier1:
		{
			heapDesc = D3D12_HEAP_DESC
			{
				size,
				HEAP_Props,
				D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
				((flags == DeviceHeapFlags::NONE)			? D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES : D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE) |
				((flags &  DeviceHeapFlags::RenderTarget)	? D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES : D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE) |
				((flags &  DeviceHeapFlags::UAVBuffer)		? D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS | D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS: D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE) | 
				((flags &  DeviceHeapFlags::UAVTextures)	? D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS | D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES: D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE)
			};
		}	break;
		case ResourceHeapTier::HeapTier2:
		{
			heapDesc = D3D12_HEAP_DESC
			{
				size,
				HEAP_Props,
				D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
				D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE
			};
		}	break;
		default:
			break;
		}


		ID3D12Heap1* heap_ptr = nullptr;

		const auto HR = pDevice->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap_ptr));

#if USING(DEBUGGRAPHICS)
		auto temp = fmt::format("heap {}", heaps.size());
		SETDEBUGNAME(heap_ptr, temp.c_str());
#endif

		if (SUCCEEDED(HR))
		{
			auto localLock = std::scoped_lock{ m };

			auto handle = handles.GetNewHandle();
			handles[handle] = (index_t)heaps.push_back({ heap_ptr, handle });

			return handle;
		}
		else
		{
			FK_LOG_ERROR("Failed to create heap. Flags: %u", flags);
			return InvalidHandle;
		}
	}


	/************************************************************************************************/


	void HeapTable::ReleaseHeap(DeviceHeapHandle heap)
	{
		const auto idx = handles[heap];
		heaps[idx].Release();

		auto localLock = std::scoped_lock{ m };
		handles.RemoveHandle(heap);

		if (heaps.size() > 1)
		{
			heaps[idx] = heaps.back();
			handles[heaps[idx].handle] = idx;
		}

		heaps.pop_back();
	}


	/************************************************************************************************/


	size_t HeapTable::GetHeapSize(DeviceHeapHandle heap) const
	{
		auto desc = heaps[handles[heap]].heap->GetDesc();
		return desc.SizeInBytes;
	}


	/************************************************************************************************/


	ID3D12Heap* HeapTable::GetDeviceResource(DeviceHeapHandle handle) const
	{
		return heaps[handles[handle]].heap;
	}

	/************************************************************************************************/


	dxRenderSystem::dxRenderSystem(iAllocator* IN_allocator, ThreadManager* IN_Threads) :
			allocator		{ IN_allocator },
			Queries			{ IN_allocator, this },
			Textures		{ IN_allocator },
			VertexBuffers	{ IN_allocator },
			ConstantBuffers	{ IN_allocator, this },
			PipelineStates	{ IN_allocator, this, IN_Threads },
			ReadBackTable	{ IN_allocator },
			rootSignatures	{ IN_allocator },
			threads			{ *IN_Threads },
			Syncs			{ IN_allocator, 64 },
			Contexts		{ IN_allocator, 3 * (1 + IN_Threads->GetThreadCount()) },
			heaps			{ pDevice, IN_allocator }
	{
		if (globalInstance)
			throw std::runtime_error{"Two Render Systems created!"};

		globalInstance = this;
	}


	dxRenderSystem::~dxRenderSystem() { Release(); }


	/************************************************************************************************/


	bool dxRenderSystem::Initiate(Graphics_Desc& in)
	{
		Vector<ID3D12DeviceChild*> ObjectsCreated(in.Memory);

		allocator				= in.Memory;
		Settings.AAQuality	= 0;
		Settings.AASamples	= 1;
		UINT DeviceFlags	= 0;

		ID3D12Device10* Device = nullptr;
		ID3D12Debug1* Debug = nullptr;
		ID3D12DebugDevice* DebugDevice = nullptr;


#if USING(ENABLEDRED)
		ID3D12DeviceRemovedExtendedDataSettings1* dredSettings;
		if (auto HR = D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings)); FAILED(HR))
			FK_LOG_ERROR("Failed to enable Dred!");
		else
		{
			dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
			dredSettings->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
			dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);

			FK_LOG_INFO("DRED enabled");
		}
#endif


		if (in.DX_DebugMode && !FAILED(D3D12GetDebugInterface(__uuidof(ID3D12Debug1), (void**)&Debug)))
		{
			Debug->EnableDebugLayer();

			if (!FAILED(D3D12GetDebugInterface(__uuidof(ID3D12Debug5), (void**)&pDebug5)))
			{
				pDebug5->SetEnableAutoName(true);
				pDebug5->SetEnableGPUBasedValidation(in.DX_GPUvalidation);
				Debug->SetEnableSynchronizedCommandQueueValidation(in.DX_GPUvalidation);
			}
		}
		else
		{
			Debug		= nullptr;
			DebugDevice = nullptr;
		}

		bool InitiateComplete = false;

		if(FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&Device))))
		{
			FK_LOG_ERROR("Failed to create A DX12 Device!");

			if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&Device))))
			{
				FK_LOG_ERROR("Failed to create A DX12 Device!");
				return false;
			}
		}

#if USING(ENABLEDRED)
		if (auto HR = Device->QueryInterface(IID_PPV_ARGS(&dred)); FAILED(HR))
			FK_LOG_ERROR("Failed to enable Dred!");
#endif

		if(auto HR = Device->QueryInterface(IID_PPV_ARGS(&pDevice15)); FAILED(HR))
			FK_LOG_ERROR("Device fails feature request!");

		if(Debug)
			Device->QueryInterface(IID_PPV_ARGS(&DebugDevice));

		D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
		pDevice15->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));

		D3D12_FEATURE_DATA_D3D12_OPTIONS2 options2 = {};
		pDevice15->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS2, &options2, sizeof(options2));

		D3D12_FEATURE_DATA_D3D12_OPTIONS3 options3 = {};
		pDevice15->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS3, &options3, sizeof(options3));

		D3D12_FEATURE_DATA_D3D12_OPTIONS4 options4 = {};
		pDevice15->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS4, &options4, sizeof(options4));

		D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
		pDevice15->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));

		D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
		pDevice15->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6));

		D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
		pDevice15->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7));

		D3D12_FEATURE_DATA_D3D12_OPTIONS8 options8 = {};
		pDevice15->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS8, &options8, sizeof(options8));

		D3D12_FEATURE_DATA_D3D12_OPTIONS9 options9 = {};
		pDevice15->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS9, &options9, sizeof(options9));

		D3D12_FEATURE_DATA_D3D12_OPTIONS10 options10 = {};
		pDevice15->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS10, &options10, sizeof(options10));

		D3D12_FEATURE_DATA_D3D12_OPTIONS11 options11 = {};
		pDevice15->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS11, &options11, sizeof(options11));

		D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12;
		pDevice15->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS12, &options12, sizeof(options12));

		D3D12_FEATURE_DATA_D3D12_OPTIONS13 options13;
		pDevice15->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS13, &options13, sizeof(options13));

		D3D12_FEATURE_DATA_D3D12_OPTIONS14 options14;
		pDevice15->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS14, &options14, sizeof(options14));

		D3D12_FEATURE_DATA_D3D12_OPTIONS15 options15;
		pDevice15->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS15, &options15, sizeof(options15));

		D3D12_FEATURE_DATA_D3D12_OPTIONS16 options16;
		pDevice15->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS16, &options16, sizeof(options16));

		D3D12_FEATURE_DATA_D3D12_OPTIONS17 options17;
		pDevice15->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS17, &options17, sizeof(options17));

		D3D12_FEATURE_DATA_D3D12_OPTIONS18 options18;
		pDevice15->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS18, &options18, sizeof(options18));

		D3D12_FEATURE_DATA_D3D12_OPTIONS19 options19;
		pDevice15->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS19, &options19, sizeof(options19));

		D3D12_FEATURE_DATA_D3D12_OPTIONS20 options20;
		pDevice15->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS20, &options20, sizeof(options20));

		D3D12_FEATURE_DATA_D3D12_OPTIONS21 options21;
		pDevice15->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS21, &options21, sizeof(options21));

		D3D12_FEATURE_DATA_D3D12_OPTIONS22 options22;
		pDevice15->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS22, &options22, sizeof(options22));

		if (!options12.EnhancedBarriersSupported)
			FK_LOG_ERROR("Required Feature: 'Enhanced Barriers' not available.");

		switch (options5.RaytracingTier)
		{
		case D3D12_RAYTRACING_TIER_1_0:
			features.RT_Level = AvailableFeatures::Raytracing::RT_FeatureLevel_1;
			break;
		case D3D12_RAYTRACING_TIER_1_1:
			features.RT_Level = AvailableFeatures::Raytracing::RT_FeatureLevel_1_1;
			break;
		case D3D12_RAYTRACING_TIER_1_2:
			features.RT_Level = AvailableFeatures::Raytracing::RT_FeatureLevel_1_2;
			break;
		default:
			features.RT_Level = AvailableFeatures::Raytracing::RT_FeatureLevel_NOTAVAILABLE;
		}

		switch (options.ConservativeRasterizationTier)
		{
		case D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED:
			features.conservativeRast = AvailableFeatures::ConservativeRast_NOTAVAILABLE;
			break;
		default:
			features.conservativeRast = AvailableFeatures::ConservativeRast_AVAILABLE;
			break;
		};

		switch (options.ResourceHeapTier)
		{
		case D3D12_RESOURCE_HEAP_TIER_1:
			features.resourceHeapTier = ResourceHeapTier::HeapTier1;
			break;
		case D3D12_RESOURCE_HEAP_TIER_2:
			features.resourceHeapTier = ResourceHeapTier::HeapTier2;
			break;
		default:
			break;
		}

		switch (options21.ExecuteIndirectTier)
		{
			case D3D12_EXECUTE_INDIRECT_TIER_1_0:
				break;
			case D3D12_EXECUTE_INDIRECT_TIER_1_1:
				features.indirectLevel = AvailableFeatures::IndirectLevel_1_1;
				break;
		}

		switch (options21.WorkGraphsTier)
		{
			case D3D12_WORK_GRAPHS_TIER_NOT_SUPPORTED:
				break;
			case D3D12_WORK_GRAPHS_TIER_1_0:
				features.workGraph = AvailableFeatures::WorkGraphs_AVAILABLE;
				break;
		}


#if USING(AFTERMATH)
		auto res2 = GFSDK_Aftermath_EnableGpuCrashDumps(
			GFSDK_Aftermath_Version_API,
			GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_DX,
			GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks,   // Let the Nsight Aftermath library cache shader debug information.
			GpuCrashDumpCallback,                                               // registerIdx callback for GPU crash dumps.
			nullptr,                                                            // registerIdx callback for shader debug information.
			nullptr,                                                            // registerIdx callback for GPU crash dump description.
			this);                                                              // Set the GpuCrashTracker object as user data for the above callbacks.

		auto res = GFSDK_Aftermath_DX12_Initialize(GFSDK_Aftermath_Version_API, GFSDK_Aftermath_FeatureFlags_Maximum, Device);

		if (res2)
			FK_LOG_INFO("Aftermath enabled");
		else
			FK_LOG_INFO("Aftermath disabled");
#endif

		ID3D12Fence* NewFence = nullptr;
		FK_ASSERT(FAILED(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&NewFence))), "FAILED TO CREATE FENCE!");
		SETDEBUGNAME(NewFence, "GRAPHICS FENCE");

		FK_LOG_9("GRAPHICS FENCE CREATED: %u", NewFence);

		directFence = NewFence;
		ObjectsCreated.push_back(NewFence);
		
		D3D12_COMMAND_QUEUE_DESC CQD		= {};
		CQD.Flags										= D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
		CQD.Type										= D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;

		D3D12_COMMAND_QUEUE_DESC ComputeCQD = {};
		ComputeCQD.Flags								= D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
		ComputeCQD.Type									= D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE;

		IDXGIFactory5*				DXGIFactory			= nullptr;
		IDXGIAdapter4*				DXGIAdapter			= nullptr;
		

		FK_ASSERT(FAILED(Device->CreateCommandQueue(&CQD,			IID_PPV_ARGS(&GraphicsQueue))), "FAILED TO CREATE COMMAND QUEUE!");
		FK_ASSERT(FAILED(Device->CreateCommandQueue(&ComputeCQD,	IID_PPV_ARGS(&ComputeQueue))), "FAILED TO CREATE COMMAND QUEUE!");

		ObjectsCreated.push_back(GraphicsQueue);
		ObjectsCreated.push_back(ComputeQueue);

		FK_LOG_9("GRAPHICS COMMAND QUEUE CREATED: %u", CQD);
		FK_LOG_9("GRAPHICS COMPUTE QUEUE CREATED: %u", ComputeCQD);

		SETDEBUGNAME(GraphicsQueue, "GRAPHICS QUEUE");
		SETDEBUGNAME(ComputeQueue,	"COMPUTE QUEUE");


		FINALLY
			if (!InitiateComplete)
			{
				for (auto O : ObjectsCreated)
					O->Release();
			}
		FINALLYOVER;


		if (FAILED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&hlslUtils))))
			throw(std::runtime_error{ "Unable to create HLSL 6.x Library!" });

		if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&hlslCompiler))))
			throw(std::runtime_error{ "Unable to create HLSL 6.x Compiler!" });

		if (hlslUtils) hlslUtils->CreateDefaultIncludeHandler(&hlslIncludeHandler);

		// Create Resources
		const UINT DXGIFLAGS =
#if USING( DEBUGGRAPHICS )
			0;// DXGI_CREATE_FACTORY_DEBUG;
#else
			0;
#endif

		if (FAILED(CreateDXGIFactory2(DXGIFLAGS, IID_PPV_ARGS(&DXGIFactory))))
		{
			FK_LOG_ERROR("FAILED TO CREATE DXGIFactory!");
			return false;
		}

		DXGIFactory->EnumAdapterByLuid(Device->GetAdapterLuid(), IID_PPV_ARGS(&DXGIAdapter));

		DXGI_ADAPTER_DESC1 desc;
		DXGIAdapter->GetDesc1(&desc);

		switch (desc.VendorId)
		{
		case 0x1002:
			vendorID = DeviceVendor::AMD;
			break;
		case 0x10de:
			vendorID = DeviceVendor::NVIDIA;
			break;
		case 0x8086:
			vendorID = DeviceVendor::INTEL;
			break;
		default:
			vendorID = DeviceVendor::UNKNOWN;
		}	


		FINALLY
			if (!InitiateComplete)
				DXGIFactory->Release();
		FINALLYOVER

		// Copy temp resources over
		pDevice						= Device;
		pGIFactory					= DXGIFactory;
		pDXGIAdapter				= DXGIAdapter;
		pDebugDevice				= DebugDevice;
		pDebug						= Debug;
		BufferCount					= 3;
		directSubmissionCounter		= 0;
		DescriptorRTVSize			= Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		DescriptorDSVSize			= Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		DescriptorCBVSRVUAVSize		= Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		descriptorHeapAllocator.Initialize(*this, 1'000'000, in.Memory);
		heaps.Init(features.resourceHeapTier, pDevice);
		copyEngine.Initiate(Device, uint32_t((threads.GetThreadCount() + 1) * 1.5), ObjectsCreated, in.Memory);

		for (size_t I = 0; I < 3 * (1 + threads.GetThreadCount()); ++I)
			Contexts.emplace_back(this, allocator);

		InitiateComplete = true;
		
		rootLibrary.Initiate(this, *in.Memory, *in.TempMemory);
		ReadBackTable.Initiate(Device);

		FreeList_GraphicsQueue.Allocator	= in.Memory;
		FreeList_CopyQueue.Allocator		= in.Memory;
		DefaultTexture						= _CreateDefaultTexture();

		RegisterPSOLoader(CLEARBUFFERPSO, CreateClearBufferPSO);
		QueuePSOLoad(CLEARBUFFERPSO);

		directUploadBuffer = dxUploadBuffer(pDevice);

		SetDebugName(DefaultTexture, "Default Texture");

		return InitiateComplete;
	}


	/************************************************************************************************/


	AvailableFeatures dxRenderSystem::GetFeatures() const noexcept
	{
		AvailableFeatures features;
		return features;
	}


	/************************************************************************************************/


	void dxRenderSystem::Release()
	{
		if (!allocator)
			return;

		const size_t completedValue = directFence->GetCompletedValue();

		SyncDirectTo(SyncUploadTicket());
		WaitFor(SyncDirectTicket());

		FK_LOG_9("Releasing dxRenderSystem");

		for (auto& ctx : Contexts)
			ctx.Release();


		while (FreeList_GraphicsQueue.size() || FreeList_CopyQueue.size())
			Free_DelayedReleaseResources(this);

		FreeList_GraphicsQueue.Release();
		FreeList_CopyQueue.Release();
		copyEngine.Release();

		for (auto& FR : Contexts)
			FR.Release();

		Contexts.Release();
		ConstantBuffers.Release();
		VertexBuffers.Release();
		Textures.Release();
		PipelineStates.ReleasePSOs();
		ReadBackTable.Release();
		Queries.Release();
		directUploadBuffer.Release();
		heaps.Release();
		rootSignatures.Release();

		if(GraphicsQueue)	GraphicsQueue->Release();
		if(ComputeQueue)	ComputeQueue->Release();
		if(pGIFactory)		pGIFactory->Release();
		if(pDXGIAdapter)	pDXGIAdapter->Release();
		if(pDevice)			pDevice->Release();
		if(pDevice15)		pDevice15->Release();
		if(directFence)		directFence->Release();

#if USING(DEBUGGRAPHICS)
		// Prints Detailed Report
		if (pDebugDevice && pDebug)
		{
			pDebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
			pDebugDevice->Release();
			pDebug->Release();
		}
#endif

		auto alloctemp = allocator;
		allocator = nullptr;
		alloctemp->release(this);
	}


	/************************************************************************************************/


	const IPipelineState* dxRenderSystem::GetPSO(PSOHandle StateID, iAllocator& temp)
	{
		return PipelineStates.GetPSO(StateID, temp);
	}


	/************************************************************************************************/


	const IPipelineInterface * const dxRenderSystem::GetPSORootSignature(PSOHandle handle) const
	{
		return PipelineStates.GetPSORootSig(handle);
	}


	/************************************************************************************************/

	std::tuple<IPipelineState*, const IPipelineInterface*> dxRenderSystem::GetPSOAndRootSignature(PSOHandle handle, iAllocator& temp) const
	{
		auto object_ptr = PipelineStates.GetPSOObject(handle);
		object_ptr->WaitForLoad(temp);

		return { &object_ptr->PSO, object_ptr->pipelineInterface };
	}


	/************************************************************************************************/


	IPipelineStateLibrary* dxRenderSystem::CreateLibrary(std::span<LibrarySection> sections)
	{
		ID3D12StateObject* stateObject = nullptr;
		Blob blob;

		Vector<D3D12_STATE_SUBOBJECT>	subObjects{ allocator };
		Vector<void*>					allocations{ allocator };

		EXITSCOPE(
			for (auto allocation : allocations)
				allocator->free(allocation);
		);

		for (auto& section : sections)
		{
			std::visit(
				Overloaded{
					[&](const LibraryPipeline& pipline)
					{},

					[&](const LibraryRT& rt)
					{
						D3D12_DXIL_LIBRARY_DESC* libraryDesc = &allocator->allocate<D3D12_DXIL_LIBRARY_DESC>();
						D3D12_RAYTRACING_PIPELINE_CONFIG* pipelineConfig = &allocator->allocate<D3D12_RAYTRACING_PIPELINE_CONFIG>();
						D3D12_RAYTRACING_SHADER_CONFIG* shaderConfig = &allocator->allocate<D3D12_RAYTRACING_SHADER_CONFIG>();

						libraryDesc->DXILLibrary =
							D3D12_SHADER_BYTECODE{
								.pShaderBytecode = rt.byteCode->buffer,
								.BytecodeLength = rt.byteCode->bufferSize
						};

						D3D12_EXPORT_DESC* exports = (D3D12_EXPORT_DESC*)allocator->malloc(rt.exports.size() * sizeof(D3D12_EXPORT_DESC));
						allocations.push_back(exports);

						for (auto [idx, objExport] : enumerate(rt.exports))
						{
							auto strSize = objExport.id.size() * sizeof(wchar_t) + 2;
							wchar_t* wstr = (wchar_t*)allocator->malloc(strSize);
						    memset(wstr, 0, strSize);
							mbstowcs(wstr, objExport.id.data(), objExport.id.size() + 1);
							allocations.push_back(wstr);

							exports[idx].ExportToRename = nullptr;
							exports[idx].Flags = D3D12_EXPORT_FLAGS::D3D12_EXPORT_FLAG_NONE;
							exports[idx].Name = wstr;
						}

						libraryDesc->NumExports = rt.exports.size();
						libraryDesc->pExports = exports;

						pipelineConfig->MaxTraceRecursionDepth = rt.maxRayDepth;
						shaderConfig->MaxAttributeSizeInBytes = rt.attributesByteSize;
						shaderConfig->MaxPayloadSizeInBytes = rt.payloadSize;

						allocations.push_back(libraryDesc);
						allocations.push_back(pipelineConfig);
						allocations.push_back(shaderConfig);

						D3D12_STATE_SUBOBJECT librarySubObject{
							.Type	= D3D12_STATE_SUBOBJECT_TYPE::D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,
							.pDesc	= libraryDesc
						};

						D3D12_STATE_SUBOBJECT rayTracingPipelineConfig{
							.Type	= D3D12_STATE_SUBOBJECT_TYPE::D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG,
							.pDesc	= pipelineConfig
						};

						D3D12_STATE_SUBOBJECT shaderConfigObject{
							.Type	= D3D12_STATE_SUBOBJECT_TYPE::D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,
							.pDesc	= shaderConfig
						};

						for (auto& association : rt.associations)
						{
							D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION* object = &allocator->allocate<D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION>();
							object->NumExports = association.num;
							object->SubobjectToAssociate = association.id.data();
							object->pExports = association.associations;

						    D3D12_STATE_SUBOBJECT associationSubobject{
							    .Type	= D3D12_STATE_SUBOBJECT_TYPE::D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION,
							    .pDesc	= object
						    };

							allocations.push_back(object);
							subObjects.push_back(associationSubobject);
						}

						subObjects.push_back(rayTracingPipelineConfig);
						subObjects.push_back(shaderConfigObject);
						subObjects.push_back(librarySubObject);

						if (rt.globalInterface)
						{
							RootSignature* dxRootSig = (RootSignature*)rt.globalInterface;
							D3D12_GLOBAL_ROOT_SIGNATURE* globalRoot = rt.globalInterface ? &allocator->allocate<D3D12_GLOBAL_ROOT_SIGNATURE>() : nullptr;
							globalRoot->pGlobalRootSignature = dxRootSig->signature;

							D3D12_STATE_SUBOBJECT globalRootSig{
							    .Type	= D3D12_STATE_SUBOBJECT_TYPE::D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,
							    .pDesc	= globalRoot
							};

							allocations.push_back(globalRoot);
							subObjects.push_back(globalRootSig);
						}

						if (rt.exports.size())
						{
							for (const auto& hitGroup : rt.hitGroups)
							{
							    D3D12_HIT_GROUP_DESC* hitGroupDesc = (D3D12_HIT_GROUP_DESC*)allocator->malloc(sizeof(D3D12_HIT_GROUP_DESC));
							    allocations.push_back(hitGroupDesc);

								wchar_t* wExportID =
									hitGroup.ID.size() ? (wchar_t*)allocator->malloc(hitGroup.ID.size() * sizeof(wchar_t) + 2) : nullptr;

								wchar_t* wAnyHit =
								    hitGroup.anyHit.size() ? (wchar_t*)allocator->malloc(hitGroup.anyHit.size() * sizeof(wchar_t) + 2) : nullptr;
								
							    wchar_t* wClosestHit = 
    								hitGroup.closestHit.size() ? (wchar_t*)allocator->malloc(hitGroup.closestHit.size() * sizeof(wchar_t) + 2) : nullptr;

							    wchar_t* wIntersection	 =
									hitGroup.intersection.size() ? (wchar_t*)allocator->malloc(hitGroup.intersection.size() * sizeof(wchar_t) + 2) : nullptr;

								if (hitGroup.ID.size()) mbstowcs(wExportID, hitGroup.ID.data(), hitGroup.ID.size() + 1);
								if (hitGroup.anyHit.size()) mbstowcs(wAnyHit, hitGroup.anyHit.data(), hitGroup.anyHit.size() + 1);
								if (hitGroup.closestHit.size()) mbstowcs(wClosestHit, hitGroup.closestHit.data(), hitGroup.closestHit.size() + 1);
								if (hitGroup.intersection.size()) mbstowcs(wIntersection, hitGroup.intersection.data(), hitGroup.intersection.size() + 1);

								if (wExportID)
									allocations.push_back(wExportID);
								if (wAnyHit)
								    allocations.push_back(wAnyHit);
								if (wClosestHit)
									allocations.push_back(wClosestHit);
								if (wIntersection)
									allocations.push_back(wIntersection);

								hitGroupDesc->Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
								hitGroupDesc->HitGroupExport = wExportID;
								hitGroupDesc->AnyHitShaderImport = wAnyHit;
								hitGroupDesc->ClosestHitShaderImport = wClosestHit;
								hitGroupDesc->IntersectionShaderImport = wIntersection;

								D3D12_STATE_SUBOBJECT hitGroupExport{
								    .Type	= D3D12_STATE_SUBOBJECT_TYPE::D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,
								    .pDesc	= hitGroupDesc
								};

								subObjects.push_back(hitGroupExport);
							}
						}
					},

					[&](const LibraryExecutable& workGraph)
					{}
				},
				section);
		}

		D3D12_STATE_OBJECT_DESC descs = {
				.Type			= D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
				.NumSubobjects	= (UINT)subObjects.size(),
				.pSubobjects	= subObjects.data(),
		};

		if (auto HR = pDevice15->CreateStateObject(&descs, IID_PPV_ARGS(&stateObject)); FAILED(HR))
		{
			FK_LOG_ERROR("DX: dxRenderSystem::BuildLibrary(): Failed to build Shader Libary!");
			return nullptr;
		}

		return &allocator->allocate<dxPipelineStateLibrary>(stateObject);
	}


	/************************************************************************************************/


	void dxRenderSystem::RegisterPSOLoader(PSOHandle State, LOADSTATE_FN fn)
	{
		PipelineStates.RegisterPSOLoader(State, std::move(fn));
	}


	/************************************************************************************************/


	void dxRenderSystem::LoadPSOIfRequired(PSOHandle state)
	{
		auto obj = PipelineStates.GetPSOObject(state);

		if (obj && obj->state != dxPipelineStateObject::PSO_States::Loaded)
			QueuePSOLoad(state);
	}


	/************************************************************************************************/


	void dxRenderSystem::QueuePSOLoad(PSOHandle State)
	{
		FK_LOG_2("Reloading PSO!");

		PipelineStates.QueuePSOLoad(State, allocator);
	}


	/************************************************************************************************/


	uint64_t dxRenderSystem::GetCurrentCounter()
	{
		return directSubmissionCounter.load(std::memory_order_relaxed);
	}


	/************************************************************************************************/


	void dxRenderSystem::_UpdateSubResources(ResourceHandle handle, ID3D12Resource** resources, const size_t size)
	{
		Textures.ReplaceResources(handle, resources, size);
	}


	/************************************************************************************************/


	void dxRenderSystem::WaitForGPU()
	{
		const size_t completedValue	= directFence->GetCompletedValue();

		if (completedValue < directSubmittedCounter)
		{
			const size_t currentCounter = directSubmittedCounter;

			const HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS); FK_ASSERT(eventHandle != 0);
			directFence->SetEventOnCompletion(currentCounter, eventHandle);

			while (Textures.FreeDelayedResourcesIncrementally(directFence->GetCompletedValue()))
			{
				if (WaitForSingleObject(eventHandle, 0) == WAIT_OBJECT_0)
				{
					CloseHandle(eventHandle);
					return;
				}
			}

			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}


	/************************************************************************************************/


	void dxRenderSystem::WaitFor(const SyncPoint& sp)
	{
		WaitFor(sp.syncCounter);
	}


	/************************************************************************************************/


	void dxRenderSystem::WaitFor(const uint64_t counter)
	{
		uint32_t stallCounter = 0;
		while (true)
		{
			ProfileFunctionTextName(GPU_WAIT);
#ifdef _DEBUG
			if (stallCounter == 100)
				FK_LOG_ERROR("Stuck waiting for: %z\n", counter);
#endif

			const size_t completedValue = directFence->GetCompletedValue();
			if (completedValue < counter)
			{
				GraphicsQueue->Signal(directFence, counter);

				const HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS); FK_ASSERT(eventHandle != 0);
				directFence->SetEventOnCompletion(counter, eventHandle);

				while (Textures.FreeDelayedResourcesIncrementally(directFence->GetCompletedValue()))
				{
					if (WaitForSingleObject(eventHandle, 0) == WAIT_OBJECT_0)
					{
						CloseHandle(eventHandle);
						return;
					}
				}

				WaitForSingleObject(eventHandle, 33);
				CloseHandle(eventHandle);
			}
			else
				return;

			stallCounter++;
		}
	}


	/************************************************************************************************/


	void dxRenderSystem::SetDebugName(ResourceHandle handle, const char* str)
	{
		Textures.SetDebugName(handle, str);
	}


	/************************************************************************************************/


	void dxRenderSystem::SetDebugName(DeviceHeapHandle heap, const char* str)
	{
		SETDEBUGNAME(heaps.GetDeviceResource(heap), str);
	}


	/************************************************************************************************/


	D3D12_GPU_VIRTUAL_ADDRESS dxRenderSystem::GetVertexBufferAddress(const VertexBufferHandle VB)
	{
		return VertexBuffers.GetAsset(VB)->GetGPUVirtualAddress();
	}


	/************************************************************************************************/


	size_t dxRenderSystem::GetVertexBufferSize(const VertexBufferHandle VB) const noexcept
	{
		return VertexBuffers.GetBufferSize(VB);
	}


	/************************************************************************************************/


	void dxRenderSystem::MarkTextureUsed(ResourceHandle Handle)
	{
		Textures.MarkRTUsed(Handle);
	}


	/************************************************************************************************/

	DevicePointer dxRenderSystem::GetDevicePointer(const ResourceHandle resourceHandle) const noexcept
	{
		auto deviceResource = GetDeviceResource(resourceHandle).As<ID3D12Resource>();
		return deviceResource->GetGPUVirtualAddress();
	}


	DeviceAddressRange	dxRenderSystem::GetDeviceRange(const ResourceHandle resource) const noexcept
	{
		auto deviceResource = GetDeviceResource(resource).As<ID3D12Resource>();
		auto resourceSize	= GetResourceSize(resource);
		auto gpuAddress		= deviceResource->GetGPUVirtualAddress();

		return { gpuAddress, resourceSize };
	}


	/************************************************************************************************/


	DeviceAddressRange	dxRenderSystem::GetDeviceRange(const ConstantBufferHandle handle) const noexcept
	{
		auto				resource		= GetDeviceResource(handle).As<ID3D12Resource>();
		const size_t		resourceSize	= ConstantBuffers.GetBufferSize(handle);
		D3D12_GPU_VIRTUAL_ADDRESS ptr		= resource->GetGPUVirtualAddress();

		return DeviceAddressRange{
			.address	= ptr,
			.size		= resourceSize
		};
	}


	/************************************************************************************************/


	size_t dxRenderSystem::GetResourceSize(ConstantBufferHandle handle) const noexcept
	{
		return ConstantBuffers.GetBufferOffset(handle);
	}


	size_t dxRenderSystem::GetResourceSize(ResourceHandle handle) const noexcept
	{
		return Textures.GetResourceSize(handle);
	}


	/************************************************************************************************/


	size_t    dxRenderSystem::GetAllocationSize(ResourceHandle handle) const noexcept
	{
		auto resource				= GetDeviceResource(handle).As<ID3D12Resource>();
		D3D12_RESOURCE_DESC desc	= resource->GetDesc();
		auto resourceInfo			= pDevice->GetResourceAllocationInfo(0, 1, &desc);

		return resourceInfo.SizeInBytes;
	}


	size_t    dxRenderSystem::GetAllocationSize(GPUResourceDesc desc) const noexcept
	{
		const D3D12_RESOURCE_DESC Resource_DESC = GetD3D12ResourceDesc(desc);
		auto res = pDevice->GetResourceAllocationInfo(0, 1, &Resource_DESC);

		return res.SizeInBytes;
	}


	/************************************************************************************************/


	size_t	dxRenderSystem::GetResourceElementSize(ResourceHandle handle) const
	{
		auto Format = Textures.GetFormat(handle);
		return GetFormatElementSize(Format);
	}


	/************************************************************************************************/


	uint2	dxRenderSystem::GetResourceWH(ResourceHandle handle) const
	{
		if (handle == InvalidHandle)
			return { 0, 0 };
		else
			return Textures.GetWH(handle);
	}


	/************************************************************************************************/


	/*
	const uint2	dxRenderSystem::GetResourceWH(ResourceHandle Handle) const
	{
		return Texture2DUAVs.GetExtra(Handle).WH;
	}
	*/

	/************************************************************************************************/


	DeviceFormat dxRenderSystem::GetTextureFormat(ResourceHandle handle) const
	{
		return DXGIFormat2TextureFormat(Textures.GetFormat(handle));
	}


	/************************************************************************************************/


	DXGI_FORMAT dxRenderSystem::GetResourceDeviceFormat(ResourceHandle handle) const
	{
		return Textures.GetFormat(handle);
	}


	/************************************************************************************************/


	ResourceDimension dxRenderSystem::GetResourceDimension(ResourceHandle handle) const
	{
		return Textures.GetDimension(handle);
	}


	/************************************************************************************************/


	size_t dxRenderSystem::GetTextureArraySize(ResourceHandle handle) const
	{
		return Textures.GetArraySize(handle);
	}


	/************************************************************************************************/


	uint8_t dxRenderSystem::GetTextureMipCount(ResourceHandle handle) const
	{
		return Textures.GetMIPCount(handle);
	}


	/************************************************************************************************/


	uint2 dxRenderSystem::GetTextureTilingWH(ResourceHandle handle, const uint mipLevel) const
	{
		auto WH = Textures.GetWH(handle);
		WH[0]   = WH[0] >> mipLevel;
		WH[1]   = WH[1] >> mipLevel;

		const auto tileSize = GetFormatTileSize(GetTextureFormat(handle));

		return WH / tileSize;
	}


	/************************************************************************************************/


	uint2 dxRenderSystem::GetHeapOffset(ResourceHandle handle, uint subResourceID) const
	{
		return Textures.GetHeapOffset(handle, subResourceID);
	}


	/************************************************************************************************/


	ResourceHandle dxRenderSystem::LoadTexture(TextureBuffer* buffer, CopyContextHandle handle, DeviceFormat format, iAllocator* allocator)
	{
		auto texture = CreateGPUResource(GPUResourceDesc::ShaderResource(buffer->WH, format));
		UploadTexture(texture, handle, buffer->Buffer, buffer->BufferSize());

		return texture;
	}

	void dxRenderSystem::UploadTexture(ResourceHandle handle, CopyContextHandle queue, std::byte* buffer, size_t bufferSize)
	{
		auto resource	= GetDeviceResource(handle).As<ID3D12Resource>();
		auto wh			= GetResourceWH(handle);
		auto formatSize = GetResourceElementSize(handle); FK_ASSERT(formatSize != -1);
		auto format     = GetTextureFormat(handle);

		size_t resourceSize = bufferSize;
		size_t offset       = 0;

		TextureBuffer textureBuffer{ wh, buffer, bufferSize, formatSize, nullptr };

		SubResourceUpload_Desc desc;
		desc.buffers            = &textureBuffer;
		desc.subResourceCount   = 1;
		desc.subResourceStart   = 0;
		desc.format             = format;

		_UpdateSubResourceByUploadQueue(this, queue, resource, &desc);

		Textures.SetLayout(handle, DeviceLayout::Common);
	}


	/************************************************************************************************/


	void dxRenderSystem::UploadTexture(ResourceHandle handle, CopyContextHandle queue, TextureBuffer* buffers, size_t resourceCount) // Uses Upload Queue
	{
		auto resource	= GetDeviceResource(handle).As<ID3D12Resource>();
		auto format		= GetTextureFormat(handle);

		SubResourceUpload_Desc desc;
		desc.buffers			= buffers;
		desc.subResourceCount	= resourceCount;
		desc.format				= format;

		_UpdateSubResourceByUploadQueue(this, queue, resource, &desc);
	}


	/************************************************************************************************/


	D3D12_GPU_VIRTUAL_ADDRESS dxRenderSystem::GetConstantBufferAddress(const ConstantBufferHandle CB)
	{
		// TODO: deal with Push Buffer Offsets
		return ConstantBuffers.GetDeviceResource(CB)->GetGPUVirtualAddress();
	}


	/************************************************************************************************/


	BLAS_PreBuildInfo dxRenderSystem::GetBLASPreBuildInfo(const IVertexBufferSet& vertexBufferSet) const noexcept
	{
		uint8_t	indexBufferIdx	= vertexBufferSet.GetIndexBufferIndex();
		auto indexBuffer		= vertexBufferSet[indexBufferIdx];
		auto positionRes		= vertexBufferSet.Find(VERTEXBUFFER_TYPE::POSITION);

		if (!positionRes.has_value())
			return {};

		auto& positionBuffer = positionRes.value();

		D3D12_RAYTRACING_GEOMETRY_DESC desc;
		desc.Type   = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		desc.Flags  = D3D12_RAYTRACING_GEOMETRY_FLAGS::D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		desc.Triangles.IndexFormat  = DXGI_FORMAT_R32_UINT;
		desc.Triangles.IndexBuffer  = indexBuffer.resource.As<ID3D12Resource>()->GetGPUVirtualAddress();
		desc.Triangles.IndexCount   = (UINT)indexBuffer.Size();

		desc.Triangles.VertexFormat					= DXGI_FORMAT_R32G32B32_FLOAT;
		desc.Triangles.VertexBuffer.StartAddress	= positionBuffer.resource.As<ID3D12Resource>()->GetGPUVirtualAddress();
		desc.Triangles.VertexBuffer.StrideInBytes	= positionBuffer.byteStride;
		desc.Triangles.VertexCount					= (UINT)positionBuffer.byteSize;


		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs;
		inputs.DescsLayout      = D3D12_ELEMENTS_LAYOUT::D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.Type             = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		inputs.pGeometryDescs   = &desc;
		inputs.NumDescs         = 1u;
		inputs.Flags            = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
		pDevice15->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

		return {
			.BLAS_byteSize          = info.ResultDataMaxSizeInBytes,
			.scratchPad_byteSize    = info.ScratchDataSizeInBytes,
			.update_byteSize        = info.UpdateScratchDataSizeInBytes
		};
	}


	/************************************************************************************************/


	size_t dxRenderSystem::GetTextureFrameGraphIndex(ResourceHandle Texture) noexcept
	{
		return Textures.GetFrameGraphIndex(Texture, directSubmissionCounter);
	}


	/************************************************************************************************/


	void dxRenderSystem::SetTextureFrameGraphIndex(ResourceHandle Texture, size_t Index) noexcept
	{
		Textures.SetFrameGraphIndex(Texture, directSubmissionCounter, Index);
	}


	/************************************************************************************************/


	[[nodiscard]] bool dxRenderSystem::CreatePipelineBuilder(std::byte* _ptr, size_t bufferSize, iAllocator& tempAllocator)
	{
		FK_ASSERT(bufferSize >= sizeof(PipelineBuilderImpl));
		new(_ptr) PipelineBuilderImpl{ tempAllocator };

		return true;
	}

	[[nodiscard]] bool dxRenderSystem::CreatePipelineInterfaceBuilder(std::byte* _ptr, size_t bufferSize, iAllocator& tempAllocator)
	{
		constexpr auto size = sizeof(RootSignatureBuilder);
		FK_ASSERT(bufferSize >= size);
		new(_ptr) RootSignatureBuilder{ tempAllocator };

		return true;
	}


	/************************************************************************************************/


	[[nodiscard]] void dxRenderSystem::CreateDescriptorSet(std::byte* _ptr, size_t bufferSize)
	{
		FK_ASSERT(bufferSize >= sizeof(dxDescriptorSet));
		new(_ptr) dxDescriptorSet{};
	}


	/************************************************************************************************/


	[[nodiscard]] IVertexBufferSet& dxRenderSystem::CreateVertexBufferSet()
	{
		return allocator->allocate<dxVertexBufferSet>();
	}


	/************************************************************************************************/


	DeviceHeapHandle  dxRenderSystem::CreateHeap(const size_t heapSize, const uint32_t flags)
	{
		return heaps.CreateHeap(heapSize, flags);
	}


	/************************************************************************************************/


	ConstantBufferHandle dxRenderSystem::CreateConstantBuffer(size_t BufferSize, bool GPUResident)
	{
		return ConstantBuffers.CreateConstantBuffer((uint32_t)BufferSize, GPUResident);
	}


	/************************************************************************************************/


	VertexBufferHandle dxRenderSystem::CreateVertexBuffer(size_t BufferSize, bool GPUResident)
	{
		return VertexBuffers.CreateVertexBuffer(BufferSize, GPUResident, this);
	}


	/************************************************************************************************/


	ResourceHandle dxRenderSystem::CreateDepthBuffer( const uint2 WH, const bool UseFloat, const size_t bufferCount)
	{
		auto resourceDesc			= GPUResourceDesc::DepthTarget(WH, UseFloat ? DeviceFormat::D32_FLOAT : DeviceFormat::D24_UNORM_S8_UINT);
		resourceDesc.bufferCount	= (uint8_t)bufferCount;

		auto resource = CreateGPUResource(resourceDesc);
		SetDebugName(resource, "DepthBuffer");

		return resource;
	}


	/************************************************************************************************/


	ResourceHandle dxRenderSystem::CreateDepthBufferArray(
		const uint2						WH,
		const bool						UseFloat,
		const size_t					arraySize,
		const bool						buffered,
		const ResourceAllocationType	allocationType)
	{
		auto desc			= GPUResourceDesc::DepthTarget(WH, UseFloat ? DeviceFormat::D32_FLOAT : DeviceFormat::D24_UNORM_S8_UINT);
		desc.arraySize		= (uint8_t)arraySize;
		desc.allocationType	= allocationType;

		auto resource	= CreateGPUResource(desc);
		SetDebugName(resource, "DepthBufferArray");

		return resource;
	}


	/************************************************************************************************/


	ResourceHandle dxRenderSystem::CreateGPUResource(const GPUResourceDesc& desc)
	{
		ProfileFunction();

		if (desc.PreCreated)
		{
			return Textures.AddResource(desc, desc.initialLayout);
		}
		else
		{
			D3D12_HEAP_PROPERTIES heapProperties	={};
			heapProperties.CPUPageProperty			= D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProperties.Type						= D3D12_HEAP_TYPE_DEFAULT;
			heapProperties.MemoryPoolPreference		= D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
			heapProperties.CreationNodeMask			= 0;
			heapProperties.VisibleNodeMask			= 0;

			const auto flags	=
				D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES |
				(desc.type == ResourceType::UnorderedAccess ? D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS : D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE) |
				(desc.type == ResourceType::UnorderedAccessRenderTarget ? D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS : D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE);

			const D3D12_CLEAR_VALUE clearValue = desc.clearValue.has_value() ? ClearValue2DXClearValue(desc.clearValue.value()) : D3D12_CLEAR_VALUE{};
			
			D3D12_BARRIER_LAYOUT initialLayout = DeviceLayout2DX(desc.initialLayout);
			ID3D12Resource* NewResource[3]		= { nullptr, nullptr, nullptr };

			FK_ASSERT(desc.bufferCount <= 3);

			FK_LOG_9("Creating Resource!");

			const size_t end = desc.bufferCount;
			for (size_t itr = 0; itr < end; ++itr)
			{
				switch(desc.allocationType)
				{
				case ResourceAllocationType::Tiled:
				{
					ProfileFunctionLabeled(Tiled);
					D3D12_RESOURCE_DESC Resource_DESC = GetD3D12ResourceDesc(desc);

					Resource_DESC.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
					HRESULT HR = pDevice15->CreateReservedResource2(
									&Resource_DESC,
									initialLayout,
									desc.clearValue.has_value() ? &clearValue : nullptr,
									nullptr,
									0,
									nullptr,
									IID_PPV_ARGS(&NewResource[itr]));

					if (FAILED(HR))
					{
						FK_LOG_ERROR("FAILED TO CREATE VIRTUAL MEMORY FOR TEXTURE");
						_OnCrash();
					}
				}	break;
				case ResourceAllocationType::Committed:
				{
					ProfileFunctionLabeled(Committed);
					D3D12_RESOURCE_DESC1 Resource_DESC = GetD3D12ResourceDesc1(desc);

					auto HR = pDevice15->CreateCommittedResource3(
						&heapProperties,
						flags,
						&Resource_DESC,
						initialLayout,
						desc.clearValue.has_value() ? &clearValue : nullptr,
						nullptr, // protected sessction,
						0, // castable formats,
						nullptr,
						IID_PPV_ARGS(&NewResource[itr]));

					if (FAILED(HR))
					{
						FK_LOG_ERROR("FAILED TO COMMIT MEMORY FOR TEXTURE");
						_OnCrash();
					}
				}   break;
				case ResourceAllocationType::Placed:
				{
					//static std::mutex m;
					//std::unique_lock lock{ m };

					ProfileFunctionLabeled(Placed);
					D3D12_RESOURCE_DESC1 Resource_DESC = GetD3D12ResourceDesc1(desc);

					HRESULT HR = pDevice15->CreatePlacedResource2(
						desc.placed.heap != InvalidHandle ? GetDeviceResource(desc.placed.heap).As<ID3D12Heap>() : desc.placed.customHeap.As<ID3D12Heap>(),
						desc.placed.offset,
						&Resource_DESC,
						initialLayout,
						desc.clearValue.has_value() ? &clearValue : nullptr,
						0,
						nullptr,
						IID_PPV_ARGS(&NewResource[itr]));

					if (FAILED(HR))
					{
						FK_LOG_ERROR("FAILED TO CREATE PLACED RESOURCE");
						_OnCrash();
					}
				}   break;
				}
				FK_ASSERT(NewResource[itr], "Failed to Create Texture!");
				SETDEBUGNAME(NewResource[itr], __func__);
			}

			auto filledDesc = desc;
			filledDesc.resources	= (DeviceResource_ptr*)NewResource;
			filledDesc.byteSize		= CalculateByteSize(desc);

			return Textures.AddResource(filledDesc, filledDesc.initialLayout);
		}

		return InvalidHandle;
	}


	/************************************************************************************************/


	void dxRenderSystem::BackResource(ResourceHandle handle, const GPUResourceDesc& desc) noexcept
	{
		ProfileFunction();

		size_t byteSize						= CalculateByteSize(desc);
		D3D12_RESOURCE_DESC1 Resource_DESC	= GetD3D12ResourceDesc1(desc);

		D3D12_HEAP_PROPERTIES HEAP_Props	= {};
		HEAP_Props.CPUPageProperty			= D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type						= D3D12_HEAP_TYPE_DEFAULT;
		HEAP_Props.MemoryPoolPreference		= D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask			= 0;
		HEAP_Props.VisibleNodeMask			= 0;

		const auto flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES | (desc.type == ResourceType::UnorderedAccess ? D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS : D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE);

		const D3D12_CLEAR_VALUE clearValue = desc.clearValue.has_value() ? ClearValue2DXClearValue(desc.clearValue.value()) : D3D12_CLEAR_VALUE{};

		D3D12_BARRIER_LAYOUT initialLayout = DeviceLayout2DX(desc.initialLayout);

		ID3D12Resource* NewResource[3] = { nullptr, nullptr, nullptr };

		FK_ASSERT(desc.bufferCount <= 3);

		FK_LOG_9("Creating Texture!");


		const size_t end = desc.bufferCount;
		for (size_t itr = 0; itr < end; ++itr)
		{
			switch(desc.allocationType)
			{
			case ResourceAllocationType::Tiled:
			{
				DebugBreak();

				/*
				ProfileFunctionLabeled(Tiled);

				Resource_DESC.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
					
				HRESULT HR = pDevice->CreateReservedResource(
								&Resource_DESC,
								InitialState,
								pCV,
								IID_PPV_ARGS(&NewResource[itr]));

				CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE VIRTUAL MEMORY FOR TEXTURE"));
				*/
			}   break;
			case ResourceAllocationType::Committed:
			{
				DebugBreak();

				/*
				ProfileFunctionLabeled(Committed);

				HRESULT HR = pDevice->CreateCommittedResource(
								&HEAP_Props, 
								flags,
								&Resource_DESC, InitialState, pCV, IID_PPV_ARGS(&NewResource[itr]));

				CheckHR(HR, ASSERTONFAIL("FAILED TO COMMIT MEMORY FOR TEXTURE"));
				*/
			}   break;
			case ResourceAllocationType::Placed:
			{
				ProfileFunctionLabeled(Placed);

				HRESULT HR = pDevice15->CreatePlacedResource2(
					desc.placed.heap != InvalidHandle ? GetDeviceResource(desc.placed.heap).As<ID3D12Heap>() : desc.placed.customHeap.As<ID3D12Heap>(),
					desc.placed.offset,
					&Resource_DESC,
					initialLayout,
					desc.clearValue.has_value() ? &clearValue : nullptr,
					0,
					nullptr,
					IID_PPV_ARGS(&NewResource[itr]));

				CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE PLACED RESOURCE"));

				if (FAILED(HR))
					_OnCrash();
			}   break;
			}
			FK_ASSERT(NewResource[itr], "Failed to Create Texture!");
			SETDEBUGNAME(NewResource[itr], __func__);
		}

		auto filledDesc     = desc;

		filledDesc.resources    = (DeviceResource_ptr*)NewResource;
		filledDesc.byteSize     = byteSize;

		Textures.SetResource(handle, filledDesc, filledDesc.initialLayout);
	}


	/************************************************************************************************/


	ResourceHandle dxRenderSystem::CreateGPUResourceHandle()
	{
		return Textures.GetFreeHandle();
	}


	/************************************************************************************************/


	QueryHandle	dxRenderSystem::CreateOcclusionBuffer(size_t Counts)
	{
		return Queries.CreateQueryBuffer(Counts, QueryType::BinaryOcclusionQuery);
	}


	/************************************************************************************************/


	ResourceHandle dxRenderSystem::CreateUAVBufferResource(size_t resourceSize, bool tripleBuffer)
	{
		auto desc = GPUResourceDesc::UAVResource(resourceSize);
		desc.bufferCount = tripleBuffer ? 3 : 1;

		UAVResourceLayout layout;
		layout.elementCount	= (UINT)(resourceSize / sizeof(uint32_t));// initial layout assume a uint buffer
		layout.format		= DXGI_FORMAT_UNKNOWN;
		layout.stride		= sizeof(uint32_t);

		auto handle = CreateGPUResource(desc);
		Textures.SetExtra(handle, layout);
		SetDebugName(handle, "CreateUAVBuffer");

		return handle;
	}


	/************************************************************************************************/


	ResourceHandle dxRenderSystem::CreateUAVTextureResource(const uint2 WH, const DeviceFormat format, const bool renderTarget)
	{
		auto desc			= GPUResourceDesc::UAVTexture(WH, format, renderTarget);
		desc.bufferCount	= 3;

		auto UAVresource = CreateGPUResource(desc);
		SetDebugName(UAVresource, "CreateUAVBuffer");

		return UAVresource;
	}


	/************************************************************************************************/



	QueryHandle dxRenderSystem::CreateSOQuery(size_t SOIndex, size_t count)
	{
		return Queries.CreateSOQueryBuffer(count, SOIndex);
	}


	/************************************************************************************************/


	QueryHandle	dxRenderSystem::CreateTimeStampQuery(size_t count)
	{
		return Queries.CreateQueryBuffer(count, QueryType::TimeStats);
	}


	/************************************************************************************************/


	struct D3D12_DISPATCH_RAYS_DESC
	{
		D3D12_GPU_VIRTUAL_ADDRESS_RANGE				RayGenerationShaderRecord;
		D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE	MissShaderTable;
		D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE	HitGroupTable;
		D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE	CallableShaderTable;
		UINT Width;
		UINT Height;
		UINT Depth;
	} D3D12_DISPATCH_RAYS_DESC;


	IndirectLayout dxRenderSystem::CreateIndirectLayout(static_vector<IndirectDrawDescription> entries, iAllocator* allocator, const IPipelineInterface* irootSignatureID)
	{
		ID3D12CommandSignature* signature = nullptr;
		
		Vector<IndirectDrawDescription>				layout{allocator};
		static_vector<D3D12_INDIRECT_ARGUMENT_DESC> signatureEntries;

		size_t entryStride = 0;

		for (size_t itr = 0; itr < entries.size(); ++itr)
		{
			switch (entries[itr].type)
			{
			case IndirectLayoutEntryType::DrawCall:
				{
					D3D12_INDIRECT_ARGUMENT_DESC desc = {};
					desc.Type   = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

					signatureEntries.push_back(desc);
					layout.push_back(IndirectLayoutEntryType::DrawCall);
					entryStride += sizeof(uint32_t) * 4; // uses 4 x 4 byte values
				}	break;
				case IndirectLayoutEntryType::DrawIndexedCall:
				{
					D3D12_INDIRECT_ARGUMENT_DESC desc = {};
					desc.Type	= D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

					signatureEntries.push_back(desc);
					layout.push_back(IndirectLayoutEntryType::DrawCall);
					entryStride += sizeof(uint32_t) * 5; // uses 5 x 4 byte values
				}	break;
				case IndirectLayoutEntryType::DispatchCall:
				{
					D3D12_INDIRECT_ARGUMENT_DESC desc = {};
					desc.Type   = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

					signatureEntries.push_back(desc);
					layout.push_back(IndirectLayoutEntryType::DispatchCall);
					entryStride += sizeof(uint4); // uses 4 x 4 byte values
				}   break;
				case IndirectLayoutEntryType::DispatchMesh:
				{
					D3D12_INDIRECT_ARGUMENT_DESC desc = {};
					desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;
					signatureEntries.push_back(desc);
					layout.push_back(IndirectLayoutEntryType::DispatchCall);
					entryStride += sizeof(uint3); // uses 4 x 4byte values
				}   break;
				case IndirectLayoutEntryType::DispatchRays:
				{
					D3D12_INDIRECT_ARGUMENT_DESC desc = {};
					desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS;
					signatureEntries.push_back(desc);
					layout.push_back(IndirectLayoutEntryType::DispatchCall);
					entryStride += sizeof(D3D12_DISPATCH_RAYS_DESC);
				}   break;
				case IndirectLayoutEntryType::RootDescriptorUINT:
				{
					D3D12_INDIRECT_ARGUMENT_DESC desc = {};
					desc.Type                               = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
					desc.Constant.DestOffsetIn32BitValues   = entries[itr].description.constantValue.destinationOffset;
					desc.Constant.Num32BitValuesToSet       = entries[itr].description.constantValue.numValues;
					desc.Constant.RootParameterIndex        = entries[itr].description.constantValue.rootParameterIdx;

					signatureEntries.push_back(desc);
					layout.push_back(IndirectLayoutEntryType::RootDescriptorUINT);

					entryStride += desc.Constant.Num32BitValuesToSet * sizeof(uint32_t);
				}   break;
			}
		}

		ID3D12RootSignature* dxRootSig = nullptr;

		D3D12_COMMAND_SIGNATURE_DESC desc;
		desc.ByteStride			= (UINT)entryStride;
		desc.NumArgumentDescs	= (UINT)entries.size();
		desc.pArgumentDescs		= signatureEntries.begin();
		desc.NodeMask			= 0;

		auto rootSignatureID = static_cast<const RootSignature*>(irootSignatureID);

		auto HR = pDevice->CreateCommandSignature(
			&desc,
			rootSignatureID ? *rootSignatureID : nullptr,
			IID_PPV_ARGS(&signature));

		CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE CONSTANT BUFFER"));

		IndirectLayout out;
		new(out.internal) dxIndirectLayout{ signature, entryStride, std::move(layout) };

		return out;
	}


	/************************************************************************************************/


	void dxRenderSystem::SubmitTileMappings(std::span<ResourceHandle> resources, iAllocator* allocator)
	{
		FK_LOG_9("Updating tile mappings for frame: %u", directSubmissionCounter.load(std::memory_order_relaxed));

		for(const auto resource : resources)
		{
			FK_ASSERT(resource >= Textures.Handles.size(), "Invalid Handle Detected");

			auto& mappings		= Textures._GetTileMappings(resource);
			auto deviceResource	= GetDeviceResource(resource);
			const auto mipCount	= Textures.GetMIPCount(resource);

			Vector<D3D12_TILED_RESOURCE_COORDINATE>	coordinates	{ allocator };
			Vector<D3D12_TILE_REGION_SIZE>			regionSizes	{ allocator };
			Vector<D3D12_TILE_RANGE_FLAGS>			flags		{ allocator };
			Vector<UINT>							offsets		{ allocator };
			Vector<UINT>							tileRanges	{ allocator };
			DeviceHeapHandle						heap = InvalidHandle;


			auto nullEnd = std::partition(mappings.begin(), mappings.end(),
				[](auto& mapping)
				{
					return mapping.state == TileMapState::Null;
				});

			std::for_each(mappings.begin(), nullEnd,
				[&](auto& mapping)
				{
					const auto flag = D3D12_TILE_RANGE_FLAG_NULL;

					D3D12_TILED_RESOURCE_COORDINATE coordinate;
					coordinate.Subresource	= mapping.tileID.GetMipLevel();
					coordinate.X			= mapping.tileID.GetTileX();
					coordinate.Y			= mapping.tileID.GetTileY();
					coordinate.Z			= 0;

					if (mapping.tileID.bytes != -1)
					{
						coordinates.push_back(coordinate);
						offsets.push_back(mapping.heapOffset);
						flags.push_back(flag);
						tileRanges.push_back(1);
					}
					else
						FK_LOG_ERROR("Invalid virtual texture tileID received");
				});

			if (coordinates.size())
			{
				copyEngine.copyQueue->UpdateTileMappings(
					deviceResource.As<ID3D12Resource>(),
					(UINT)coordinates.size(),
					coordinates.data(),
					nullptr,
					nullptr,
					(UINT)coordinates.size(),
					flags.data(),
					offsets.data(),
					tileRanges.data(),
					D3D12_TILE_MAPPING_FLAG_NONE);
			}

			coordinates.clear();
			regionSizes.clear();
			flags.clear();
			offsets.clear();
			tileRanges.clear();

			uint32_t I = (uint32_t)std::distance(mappings.begin(), nullEnd);
			while(I < mappings.size())
			{
				heap = mappings[I].heap;

				for (;mappings[I].heap == heap && I < mappings.size(); I++)
				{
					auto& mapping = mappings[I];

					mapping.state = TileMapState::InUse;

					const auto flag = D3D12_TILE_RANGE_FLAG_NONE;

					D3D12_TILED_RESOURCE_COORDINATE coordinate;
					coordinate.Subresource	= mapping.tileID.GetMipLevel();
					coordinate.X			= mapping.tileID.GetTileX();
					coordinate.Y			= mapping.tileID.GetTileY();
					coordinate.Z			= 0;

#if _DEBUG
					if(flag == D3D12_TILE_RANGE_FLAG_NONE)
						FK_LOG_9("Mapping Tile { %u, %u, %u MIP } to Offset: { %u }", coordinate.X, coordinate.Y, coordinate.Subresource, mapping.heapOffset );
					else
						FK_LOG_INFO("Unmapping Tile { %u, %u, %u MIP } from Offset: { %u }", coordinate.X, coordinate.Y, coordinate.Subresource, mapping.heapOffset);
#endif

					if (mapping.tileID.bytes != -1)
					{
						coordinates.push_back(coordinate);

						D3D12_TILE_REGION_SIZE regionSize;
						regionSize.Depth	= 1;
						regionSize.Height	= 1;
						regionSize.Width	= 1;

						regionSize.NumTiles	= 1;
						regionSize.UseBox	= false;

						regionSizes.push_back(regionSize);

						offsets.push_back(mapping.heapOffset);
						flags.push_back(flag);
						tileRanges.push_back(1);
					}
					else
						FK_LOG_ERROR("Invalid virtual texture tile received");
				}

				I++;

				if (heap != InvalidHandle)
				{
					D3D12_TILE_RANGE_FLAGS nullRangeFlag = D3D12_TILE_RANGE_FLAG_NULL;

					if (!coordinates.size())
					{
						//D3D12_TILE_RANGE_FLAGS rangeFlags = D3D12_TILE_RANGE_FLAG_NULL;
						//copyEngine.copyQueue->UpdateTileMappings(deviceResource, 1, NULL, NULL, NULL, 1, &rangeFlags, NULL, NULL, D3D12_TILE_MAPPING_FLAG_NONE);
					}
					else
					{
						copyEngine.copyQueue->UpdateTileMappings(
							deviceResource.As<ID3D12Resource>(),
							(UINT)coordinates.size(),
							coordinates.data(),
							regionSizes.data(),
							GetDeviceResource(heap).As<ID3D12Heap>(),
							(UINT)coordinates.size(),
							flags.data(),
							offsets.data(),
							tileRanges.data(),
							D3D12_TILE_MAPPING_FLAG_NONE);
					}
				}

				mappings.erase(
					std::remove_if(
						std::begin(mappings),
						std::end(mappings),
						[](TileMapping& mapping)
						{
							return mapping.state == TileMapState::Null;
						}),
					std::end(mappings));

				coordinates.clear();
				regionSizes.clear();
				flags.clear();
				offsets.clear();
				tileRanges.clear();
			}
		}

		FK_LOG_9("Completed file mapping update");
	}


	/************************************************************************************************/


	void dxRenderSystem::UpdateTextureTileMappings(const ResourceHandle handle, std::span<const TileMapping> tileMaps, iAllocator& temp)
	{
		Textures.UpdateTileMappings(handle, tileMaps.data(), tileMaps.data() + tileMaps.size(), temp);
	}


	/************************************************************************************************/


	const TileMapList& dxRenderSystem::GetTileMappings(const ResourceHandle handle)
	{
		return Textures.GetTileMappings(handle);
	}


	/************************************************************************************************/


	ReadBackResourceHandle  dxRenderSystem::CreateReadBackBuffer(const size_t bufferSize)
	{
		D3D12_RESOURCE_DESC Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
		Resource_DESC.Width     = bufferSize;
		Resource_DESC.Format    = DXGI_FORMAT_UNKNOWN;
		Resource_DESC.Flags     = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;

		D3D12_HEAP_PROPERTIES HEAP_Props = {};
		HEAP_Props.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type                 = D3D12_HEAP_TYPE_READBACK;
		HEAP_Props.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;

		ID3D12Resource* resource = nullptr;

		auto HR = pDevice->CreateCommittedResource(
			&HEAP_Props,
			D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
			&Resource_DESC,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&resource));

		SETDEBUGNAME(resource, "ReadBackHeap");

		return ReadBackTable.AddReadBack(bufferSize, resource);
	}


	/************************************************************************************************/


	void dxRenderSystem::CreateTextureView(ResourceHandle resource, DescHeapPOS pos)
	{
		auto dxgiFormat = GetResourceDeviceFormat(resource);
		PushTextureToDescHeap(*this, dxgiFormat, resource, pos);
	}


	/************************************************************************************************/


	IVertexBufferSet& CreateVertexBufferSet()
	{
		FK_LOG_ERROR("DX: CreateVertexBufferSet unimplemented!");

		std::unreachable();
	}


	/************************************************************************************************/


	SubAllocation dxRenderSystem::ReserveConstantBuffer(ConstantBufferHandle CB, size_t reserveSize)	noexcept
	{
		return ConstantBuffers.Reserve(CB, reserveSize);
	}


	SubAllocation dxRenderSystem::ReserveVertexBuffer(VertexBufferHandle VB, size_t reserveSize)		noexcept
	{
		return VertexBuffers.Reserve(VB, reserveSize);
	}


	/************************************************************************************************/


	void dxRenderSystem::SetReadBackEvent(ReadBackResourceHandle readbackBuffer, ReadBackEventHandler&& handler)
	{
		ReadBackTable.SetCallback(readbackBuffer, std::move(handler));
	}


	/************************************************************************************************/


	std::pair<void*, size_t> dxRenderSystem::OpenReadBackBuffer(ReadBackResourceHandle readbackBuffer, const size_t readSize)
	{
		auto temp = ReadBackTable.OpenBufferData(readbackBuffer, readSize);
		return { temp.buffer, temp.bufferSize };
	}


	/************************************************************************************************/


	void dxRenderSystem::CloseReadBackBuffer(ReadBackResourceHandle readbackBuffer)
	{
		ReadBackTable.CloseBufferData(readbackBuffer);
	}


	/************************************************************************************************/


	void dxRenderSystem::FlushPendingReadBacks()
	{
		ReadBackTable.Update();
	}


	/************************************************************************************************/


	void dxRenderSystem::SetObjectLayout(ResourceHandle handle, DeviceLayout layout) noexcept
	{
		Textures.SetLayout(handle, layout);
	}


	/************************************************************************************************/


	DeviceLayout dxRenderSystem::GetObjectLayout(const QueryHandle handle) const noexcept
	{
		return Queries.GetLayout(handle);
	}


	/************************************************************************************************/


	DeviceLayout dxRenderSystem::GetObjectLayout(const ResourceHandle handle) const noexcept
	{
		return Textures.GetLayout(handle);
	}


	/************************************************************************************************/


	PackedResourceTileInfo dxRenderSystem::GetPackedTileInfo(ID3D12Resource* resource) const noexcept
	{
		UINT						TileCount = 0;
		D3D12_PACKED_MIP_INFO		packedMipInfo;
		D3D12_TILE_SHAPE			TileShape;
		UINT						subResourceTilingCount = 1;
		D3D12_SUBRESOURCE_TILING	subResourceTiling_Packed;

		pDevice->GetResourceTiling(resource, &TileCount, &packedMipInfo, &TileShape, &subResourceTilingCount, 0, &subResourceTiling_Packed);

		return {
			size_t(packedMipInfo.NumStandardMips),
			size_t(packedMipInfo.NumStandardMips + packedMipInfo.NumPackedMips),
			packedMipInfo.StartTileIndexInOverallResource };
	}


	/************************************************************************************************/


	PackedResourceTileInfo dxRenderSystem::GetPackedTileInfo(ResourceHandle resource)	const noexcept
	{
		UINT						tileCount = 0;
		D3D12_PACKED_MIP_INFO		packedMipInfo;
		D3D12_TILE_SHAPE			tileShape;
		UINT						subResourceTilingCount = 1;
		D3D12_SUBRESOURCE_TILING	subResourceTiling_Packed;

		pDevice->GetResourceTiling(GetDeviceResource(resource).As<ID3D12Resource>(),
			                        &tileCount, &packedMipInfo, &tileShape,
			                        &subResourceTilingCount, 0, &subResourceTiling_Packed);

		return {
			uint32_t(packedMipInfo.NumStandardMips),
			uint32_t(packedMipInfo.NumStandardMips + packedMipInfo.NumPackedMips),
			packedMipInfo.StartTileIndexInOverallResource };
	}


	/************************************************************************************************/


	DeviceHeap_ptr dxRenderSystem::GetDeviceResource(const DeviceHeapHandle handle) const
	{
		return heaps.GetDeviceResource(handle);
	}


	DeviceResource_ptr dxRenderSystem::GetDeviceResource(const ReadBackResourceHandle handle) const
	{
		return ReadBackTable.GetDeviceResource(handle);
	}


	/************************************************************************************************/


	DeviceResource_ptr dxRenderSystem::GetDeviceResource(const ConstantBufferHandle handle) const
	{
		return ConstantBuffers.GetDeviceResource(handle);
	}


	/************************************************************************************************/


	DeviceResource_ptr dxRenderSystem::GetDeviceResource(const ResourceHandle handle) const
	{
		if (handle != InvalidHandle)
			return Textures.GetResource(handle, pDevice);
		else
			return nullptr;
	}


	/************************************************************************************************/


	size_t	dxRenderSystem::GetVertexBufferOffset(const VertexBufferHandle handle) const
	{
		return VertexBuffers.GetCurrentVertexBufferOffset(handle);
	}


	/************************************************************************************************/


	bool dxRenderSystem::VertexBufferPush(VertexBufferHandle buffer, void* _ptr, size_t elementSize)
	{
		return VertexBuffers.PushVertex(buffer, _ptr, elementSize);
	}


	/************************************************************************************************/


	UAVResourceLayout dxRenderSystem::GetUAVBufferLayout(const ResourceHandle handle) const noexcept
	{
		auto extra  = Textures.GetExtra(handle);
		auto temp   = std::get_if<UAVResourceLayout>(&extra);

		return temp ? *temp : UAVResourceLayout{};
	}


	/************************************************************************************************/


	void dxRenderSystem::SetUAVBufferLayout(const ResourceHandle handle, const UAVResourceLayout newConfig) noexcept
	{
		Textures.SetExtra(handle, newConfig);
	}


	/************************************************************************************************/


	size_t dxRenderSystem::GetUAVBufferSize(const ResourceHandle handle) const noexcept
	{
		return Textures.GetResourceSize(handle);
	}


	/************************************************************************************************/


	size_t dxRenderSystem::GetHeapSize(const DeviceHeapHandle heap) const
	{
		return heaps.GetHeapSize(heap);
	}


	/************************************************************************************************/


	AvailableFeatures::Raytracing dxRenderSystem::GetRTFeatureLevel() const noexcept
	{
		return features.RT_Level;
	}


	/************************************************************************************************/


	bool dxRenderSystem::RTAvailable() const noexcept
	{
		return (features.RT_Level != AvailableFeatures::Raytracing::RT_FeatureLevel_NOTAVAILABLE);
	}


	/************************************************************************************************/


	void dxRenderSystem::ResetConstantBuffer(ConstantBufferHandle constantBuffer)
	{
		ConstantBuffers.Reset(constantBuffer);
	}


	/************************************************************************************************/


	void dxRenderSystem::ResetVertexBuffer(VertexBufferHandle constant)
	{
		VertexBuffers.Reset(constant);
	}


	/************************************************************************************************/


	void dxRenderSystem::ResetQuery(QueryHandle handle)
	{
		Queries.LockUntil(handle, directSubmissionCounter);
	}


	/************************************************************************************************/


	void dxRenderSystem::ReleaseCB(ConstantBufferHandle Handle)
	{
		ConstantBuffers.ReleaseBuffer(Handle);
	}


	/************************************************************************************************/


	void dxRenderSystem::ReleaseVB(VertexBufferHandle Handle)
	{
		VertexBuffers.ReleaseVertexBuffer(Handle, directSubmissionCounter);
	}


	/************************************************************************************************/


	void dxRenderSystem::ReleaseResource(ResourceHandle Handle)
	{
		Textures.ReleaseTexture(Handle, directSubmissionCounter);
	}


	/************************************************************************************************/


	void dxRenderSystem::ReleaseReadBack(ReadBackResourceHandle handle)
	{
		ReadBackTable.ReleaseResource(handle);
	}


	/************************************************************************************************/


	void dxRenderSystem::ReleaseHeap(DeviceHeapHandle heap)
	{
		heaps.ReleaseHeap(heap);
	}


	/************************************************************************************************/


	void dxRenderSystem::ReleaseQuery(QueryHandle q)
	{
		Queries.Release(q, directSubmissionCounter.load(std::memory_order_acquire));
	}


	/************************************************************************************************/


	void Push_DelayedRelease(dxRenderSystem* RS, ID3D12Resource* Res)
	{
		RS->FreeList_GraphicsQueue.push_back({ Res, RS->directSubmissionCounter });
	}


	/************************************************************************************************/


	void Push_DelayedReleaseCopy(dxRenderSystem* RS, ID3D12Resource* Res)
	{
		RS->FreeList_CopyQueue.push_back({ Res, RS->copyEngine.counter });
	}


	/************************************************************************************************/


	void Free_DelayedReleaseResources(dxRenderSystem* RS)
	{
		{
			auto completed = RS->directFence->GetCompletedValue();
			for (auto& R : RS->FreeList_GraphicsQueue)
				if (completed > R.Counter)
					SAFERELEASE(R.Resource);

			RS->FreeList_GraphicsQueue.erase(
				std::partition(
					RS->FreeList_GraphicsQueue.begin(),
					RS->FreeList_GraphicsQueue.end(),
					[](const auto& R) -> bool {return (R.Resource); }),
				RS->FreeList_GraphicsQueue.end());
		}

		{
			auto completed = RS->copyEngine.fence->GetCompletedValue();
			for (auto& R : RS->FreeList_CopyQueue)
				if (completed > R.Counter)
					SAFERELEASE(R.Resource);

			RS->FreeList_CopyQueue.erase(
				std::partition(
					RS->FreeList_CopyQueue.begin(),
					RS->FreeList_CopyQueue.end(),
					[](const auto& R) -> bool {return (R.Resource); }),
				RS->FreeList_CopyQueue.end());
		}
	}


	/************************************************************************************************/


	constexpr DXGI_FORMAT TextureFormat2DXGIFormat(DeviceFormat F) noexcept
	{
		;
		switch (F)
		{
		case FlexKit::DeviceFormat::R16_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT;
		case FlexKit::DeviceFormat::R16_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
		case FlexKit::DeviceFormat::R16G16_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R16G16_UINT;
		case DeviceFormat::R16G16B16A16_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT;
		case FlexKit::DeviceFormat::R32_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
		case FlexKit::DeviceFormat::R32G32_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R32G32_UINT;
		case FlexKit::DeviceFormat::R32G32_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
		case FlexKit::DeviceFormat::R32G32B32_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R32G32B32_UINT;
		case FlexKit::DeviceFormat::R32G32B32A32_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_UINT;
		case FlexKit::DeviceFormat::R8G8B8A8_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
		case FlexKit::DeviceFormat::R8G8B8A8_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case FlexKit::DeviceFormat::R8G8_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_R8G8_UNORM;
		case FlexKit::DeviceFormat::R16G16_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_R16G16_FLOAT;
		case FlexKit::DeviceFormat::R32G32B32_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
		case FlexKit::DeviceFormat::R16G16B16A16_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		case FlexKit::DeviceFormat::R32G32B32A32_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
		case FlexKit::DeviceFormat::R32_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		case FlexKit::DeviceFormat::D32_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
		case FlexKit::DeviceFormat::BC1_TYPELESS:
			return DXGI_FORMAT::DXGI_FORMAT_BC1_TYPELESS;
		case FlexKit::DeviceFormat::BC1_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM;
		case FlexKit::DeviceFormat::BC1_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM_SRGB;
		case FlexKit::DeviceFormat::BC2_TYPELESS:
			return DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM_SRGB;
		case FlexKit::DeviceFormat::BC2_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM;
		case FlexKit::DeviceFormat::BC2_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM_SRGB;
		case FlexKit::DeviceFormat::BC3_TYPELESS:
			return DXGI_FORMAT::DXGI_FORMAT_BC3_TYPELESS;
		case FlexKit::DeviceFormat::BC3_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM;
		case FlexKit::DeviceFormat::BC3_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM_SRGB;
		case FlexKit::DeviceFormat::BC4_TYPELESS:
			return DXGI_FORMAT::DXGI_FORMAT_BC4_TYPELESS;
		case FlexKit::DeviceFormat::BC4_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC4_UNORM;
		case FlexKit::DeviceFormat::BC4_SNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC4_SNORM;
		case FlexKit::DeviceFormat::BC5_TYPELESS:
			return DXGI_FORMAT::DXGI_FORMAT_BC5_TYPELESS;
		case FlexKit::DeviceFormat::BC5_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC5_UNORM;
		case FlexKit::DeviceFormat::BC5_SNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC5_SNORM;
		case FlexKit::DeviceFormat::BC7_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM;
		case FlexKit::DeviceFormat::UNKNOWN:
			return DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		default:
			return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UINT;
			break;
		}
	}

	
	/************************************************************************************************/


	DeviceFormat	DXGIFormat2TextureFormat(DXGI_FORMAT F) noexcept
	{
		switch (F)
		{
		case DXGI_FORMAT::DXGI_FORMAT_R16G16_UINT:
			return FlexKit::DeviceFormat::R16G16_UINT;
		case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT:
			return  DeviceFormat::R16G16B16A16_UINT;
		case DXGI_FORMAT::DXGI_FORMAT_R32G32_UINT:
			return FlexKit::DeviceFormat::R32G32_UINT;
		case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM:
			return FlexKit::DeviceFormat::R8G8B8A8_UNORM;
		case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return FlexKit::DeviceFormat::R8G8B8A8_UNORM_SRGB;
		case DXGI_FORMAT::DXGI_FORMAT_R8G8_UNORM:
			return FlexKit::DeviceFormat::R8G8_UNORM;
		case DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT:
			return FlexKit::DeviceFormat::R32G32B32_FLOAT;
		case DXGI_FORMAT::DXGI_FORMAT_R16G16_FLOAT:
			return FlexKit::DeviceFormat::R16G16_FLOAT;
		case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT:
			return FlexKit::DeviceFormat::R16G16B16A16_FLOAT;
		case DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT:
			return FlexKit::DeviceFormat::R32G32B32A32_FLOAT;
		case DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT:
			return FlexKit::DeviceFormat::R32_FLOAT;
		case DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT:
			return FlexKit::DeviceFormat::D32_FLOAT;
		case DXGI_FORMAT::DXGI_FORMAT_BC1_TYPELESS:
			return FlexKit::DeviceFormat::BC1_TYPELESS;
		case DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM:
			return FlexKit::DeviceFormat::BC1_UNORM;
		case DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM_SRGB:
			return FlexKit::DeviceFormat::BC1_UNORM_SRGB;
		case DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM:
			return FlexKit::DeviceFormat::BC2_UNORM;
		case DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM_SRGB:
			return FlexKit::DeviceFormat::BC2_UNORM_SRGB;
		case DXGI_FORMAT::DXGI_FORMAT_BC3_TYPELESS:
			return FlexKit::DeviceFormat::BC3_TYPELESS;
		case DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM:
			return FlexKit::DeviceFormat::BC3_UNORM;
		case DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM_SRGB:
			return FlexKit::DeviceFormat::BC3_UNORM_SRGB;
		case DXGI_FORMAT::DXGI_FORMAT_BC4_TYPELESS:
			return FlexKit::DeviceFormat::BC4_TYPELESS;
		case DXGI_FORMAT::DXGI_FORMAT_BC4_UNORM:
			return FlexKit::DeviceFormat::BC4_UNORM;
		case DXGI_FORMAT::DXGI_FORMAT_BC4_SNORM:
			return FlexKit::DeviceFormat::BC4_SNORM;
		case DXGI_FORMAT::DXGI_FORMAT_BC5_TYPELESS:
			return FlexKit::DeviceFormat::BC5_TYPELESS;
		case DXGI_FORMAT::DXGI_FORMAT_BC5_UNORM:
			return FlexKit::DeviceFormat::BC5_UNORM;
		case DXGI_FORMAT::DXGI_FORMAT_BC5_SNORM:
			return FlexKit::DeviceFormat::BC5_SNORM;
		case DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM:
			return FlexKit::DeviceFormat::BC7_UNORM;
		case DXGI_FORMAT::DXGI_FORMAT_R32_UINT:
			return FlexKit::DeviceFormat::R32_UINT;
		case DXGI_FORMAT::DXGI_FORMAT_R32G32B32_UINT:
			return FlexKit::DeviceFormat::R32G32B32_UINT;
		case DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_UINT:
			return FlexKit::DeviceFormat::R32G32B32A32_UINT;

		default:
			return FlexKit::DeviceFormat::UNKNOWN;
			break;
		}
	}


	/************************************************************************************************/


	void _UpdateSubResourceByUploadQueue(dxRenderSystem* RS, CopyContextHandle uploadHandle, ID3D12Resource* destinationResource, SubResourceUpload_Desc* desc)
	{
		auto& copyCtx = RS->GetCopyContext(uploadHandle);

		for (size_t I = 0; I < desc->subResourceCount; ++I)
		{
			const auto region	= copyCtx.Reserve(desc->buffers[I].Size, 512);

			memcpy(
				(char*)region.buffer,
				(char*)desc->buffers[I].Buffer,
				desc->buffers[I].Size);

			copyCtx.CopyTextureRegion(
				destinationResource,
				I,
				{ 0, 0, 0 },
				region,
				desc->buffers[I].WH,
				desc->format);
		}
	}


	/************************************************************************************************/


	void dxRenderSystem::UpdateResourceByUploadQueue(DeviceResource_ptr dest, CopyContextHandle uploadQueue, const void* data, size_t Size, size_t byteSize, DeviceAccessState endState)
	{
		if (nullptr == data || nullptr == dest._ptr)
			return;

		auto& copyCtx = GetCopyContext(uploadQueue);

		const auto reservedSpace = copyCtx.Reserve(Size);

		memcpy(reservedSpace.buffer, data, Size);

		copyCtx.CopyBuffer(dest.As<ID3D12Resource>(), 0, reservedSpace);
	}


	/************************************************************************************************/

	struct IncludeHandler : public IDxcIncludeHandler
	{
		HRESULT STDMETHODCALLTYPE LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource) override
		{
			char fileStr[256];
			auto fileLength = wcstombs(fileStr, pFilename, 256);

			std::filesystem::path file{ fileStr };
			auto newFilePath = includePath.string() + R"(\)" + file.string();

			wchar_t fileW[256];
			mbstowcs(fileW, newFilePath.c_str(), 256);


			return handler->LoadSource(fileW, ppIncludeSource);
		}

		HRESULT QueryInterface(const IID&, void**)
		{
			return 0;
		}

		std::filesystem::path   includePath;
		IDxcIncludeHandler*     handler;

		ULONG AddRef() { return 0; }
		ULONG Release() { return 0; }
	};


	/************************************************************************************************/


	Shader  dxRenderSystem::LoadShader(const char* entry, const char* profile, const char* file, const ShaderOptions& options)
	{
		std::filesystem::path filePath{ file };
		auto parentPath = filePath.parent_path();

		wchar_t entryW[64];
		wchar_t profileW[10];

		if (entry != nullptr)
			mbstowcs(entryW, entry, 64);

		if (entry != nullptr)
			mbstowcs(profileW, profile, 10);
		else
			return {};

		if (!std::filesystem::exists(filePath))
			return {};

		auto shaderFileSize = GetFileSize(filePath.string().c_str()) + 1;
		std::string shaderStr;
		shaderStr.resize(shaderFileSize);

		memset(shaderStr.data(), 0, shaderFileSize);
		LoadFileIntoBuffer(filePath.string().c_str(), (std::byte*)shaderStr.data(), shaderFileSize);

		DXShaderProprocessor(shaderStr, SHADER_TYPE::Unknown, *allocator);

		IDxcBlobEncoding* blob;
		if (auto HR = hlslUtils->CreateBlobFromPinned(shaderStr.data(), shaderStr.size(), DXC_CP_ACP, &blob); FAILED(HR))
			return {};

		IncludeHandler includeHandler;
		includeHandler.includePath = parentPath;
		includeHandler.handler = hlslIncludeHandler;

		IDxcCompiler2* debugCompiler = nullptr;
		hlslCompiler->QueryInterface<IDxcCompiler2>(&debugCompiler);

		static_vector<LPCWSTR> arguments;
		arguments.push_back(L"-T");
		arguments.push_back(profileW);

		if (strlen(entry))
		{
			arguments.push_back(L"-E");
			arguments.push_back(entryW);
		}

		if (options.enableDebug)
		{
			arguments.push_back(L"-Od");
			arguments.push_back(L"/Zi");
			arguments.push_back(L"-Qembed_debug");
		}
		else
		    arguments.push_back(L"-O2");

		if (options.enable16BitTypes)
			arguments.push_back(L"-enable-16bit-types");

		if (options.hlsl2021)
			arguments.push_back(L"-HV 2021");

		DxcBuffer buffer{
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
		};

		int _;

		blob->GetEncoding(&_, &buffer.Encoding);
		IDxcOperationResult* result = nullptr;

		auto HR2 = hlslCompiler->Compile(
			&buffer,
			arguments.data(),
			(UINT)arguments.size(),
			&includeHandler,
			IID_PPV_ARGS(&result));

		if (FAILED(HR2))
		{
			if (result)
				result->Release();

			return {};
		}
		else
		{
			IDxcBlob* byteCodeBlob;
			HRESULT status;
			result->GetStatus(&status);

			if (FAILED(status))
			{
				IDxcBlobEncoding* errors;
				result->GetErrorBuffer(&errors);

				auto errorString = (const char*)errors->GetBufferPointer();
				auto formattedError = std::format("{}\nFailed to compiled shader!\nFile: {}\nPress Enter to try again\n", errorString, filePath.string().c_str());
				FK_LOG_ERROR(formattedError.c_str());

				errors->Release();

				std::string line;
				std::getline(std::cin, line);

				return {};
			}

			auto HR = result->GetResult(&byteCodeBlob);

			wchar_t* text = (wchar_t*)byteCodeBlob->GetBufferPointer();

			Shader out = CreateShader(byteCodeBlob, allocator);
			byteCodeBlob->Release();
			result->Release();

			return out;
		}

		return {};
	}


	/************************************************************************************************/


	Shader dxRenderSystem::LoadShaderLibrary(const char* file, const ShaderOptions& options)
	{
		return LoadShader("", "lib_6_9", file, options);
	}


	/************************************************************************************************/


	std::expected<Shader, std::string> dxRenderSystem::LoadRootSignature(const char* file, const char* entry)
	{
		std::filesystem::path filePath{ file };
		auto parentPath = filePath.parent_path();

		wchar_t entryW[64];
		wchar_t profileW[] = L"rootsig_1_1";

		if(entry != nullptr)
			mbstowcs(entryW, entry, 64);


		if (!std::filesystem::exists(filePath))
			return std::unexpected{ "File Not Found!" };

		auto shaderFileSize = FlexKit::GetFileSize(filePath.string().c_str()) + 1;
		std::string shaderStr;
		shaderStr.resize(shaderFileSize);

		memset(shaderStr.data(), 0, shaderFileSize);
		LoadFileIntoBuffer(filePath.string().c_str(), (std::byte*)shaderStr.data(), shaderFileSize);


		DXShaderProprocessor(shaderStr, SHADER_TYPE::Unknown, *allocator);

		IDxcBlobEncoding* blob;
		if (auto HR = hlslUtils->CreateBlobFromPinned(shaderStr.data(), shaderStr.size(), DXC_CP_ACP, &blob); FAILED(HR))
			return std::unexpected{ "DX:LoadRootSignature: File To Create Blob!" };

		IncludeHandler includeHandler;
		includeHandler.includePath	= parentPath;
		includeHandler.handler		= hlslIncludeHandler;


		IDxcCompiler2* debugCompiler = nullptr;
		hlslCompiler->QueryInterface<IDxcCompiler2>(&debugCompiler);

		static_vector<LPCWSTR> arguments;
		arguments.push_back(L"/extractrootsignature");

		DxcBuffer buffer{
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
		};

		int _;

		blob->GetEncoding(&_, &buffer.Encoding);
		IDxcOperationResult* result = nullptr;

		auto HR2 = hlslCompiler->Compile(
			&buffer,
			arguments.data(),
			(UINT)arguments.size(),
			&includeHandler,
			IID_PPV_ARGS(&result));

		if (FAILED(HR2))
		{
			if (result)
				result->Release();

			return std::unexpected{ std::string{ "Failed to load file: " } + file };
		}
		else
		{
			IDxcBlob* byteCodeBlob;
			HRESULT status;
			result->GetStatus(&status);

			if (FAILED(status))
			{
				IDxcBlobEncoding* errors;
				result->GetErrorBuffer(&errors);

				auto errorString = (const char*)errors->GetBufferPointer();
				FK_LOG_ERROR("%s\nFailed to compile root signature \nFile: %s\nPress Enter to try again\n", filePath.string().c_str());

				errors->Release();

				return std::unexpected{ std::string{ errorString } };
			}

			auto HR = result->GetResult(&byteCodeBlob);

			wchar_t* text = (wchar_t*)byteCodeBlob->GetBufferPointer();

			Shader out = CreateShader(byteCodeBlob, allocator);
			byteCodeBlob->Release();
			result->Release();

			return out;
		}

		return std::unexpected{ std::string{ "Unexpected error!" } };
	}


	/************************************************************************************************/


	VertexBufferHandle VertexBufferStateTable::CreateVertexBuffer(size_t BufferSize, bool GPUResident, dxRenderSystem* RS) // Creates Using Placed Resource
	{
		VBufferHandle Buffer[3];

		for(size_t I = 0; I < 3; ++I)
			Buffer[I] = CreateVertexBufferResource(BufferSize, GPUResident, RS);

		const auto handle   = Handles.GetNewHandle();
		Handles[handle]     = (index_t)UserBuffers.size();

		UserBuffers.push_back(UserVertexBuffer{
			0,
			{ Buffer[0], Buffer[1], Buffer[2] },
			BufferSize,
			0, 
			GPUResident ? nullptr : _Map(Buffer[0]),
			Buffers[Buffer[0]].resource,
			false,
			handle});

		return handle;
	}


	/************************************************************************************************/


	VertexBufferStateTable::VBufferHandle VertexBufferStateTable::CreateVertexBufferResource(size_t BufferSize, bool GPUResident, dxRenderSystem* RS)
	{
		ID3D12Resource*	NewResource = RS->_CreateVertexBufferDeviceResource(BufferSize, GPUResident);
		void* _ptr = nullptr;

		FK_ASSERT(NewResource, "Failed To Create VertexBuffer Resource!");

		CD3DX12_RANGE readRange(0, 0);

		Buffers.push_back(
			VertexBuffer{
				NewResource,
				BufferSize,
				0 });

		return Buffers.size() - 1;
	}


	/************************************************************************************************/


	bool VertexBufferStateTable::PushVertex(VertexBufferHandle Handle, void* _ptr, size_t ElementSize)
	{
		auto	Idx			= Handles[Handle];
		auto&	UserBuffer  = UserBuffers[Idx];
		auto	Offset		= UserBuffers[Idx].Offset;
		auto	Mapped_Ptr	= UserBuffers[Idx].MappedPtr;

		memcpy(Mapped_Ptr + Offset, _ptr, ElementSize);
		UserBuffers[Idx].Offset += ElementSize;
		UserBuffers[Idx].WrittenTo = true;

		return true;
	}
	

	/************************************************************************************************/


	void VertexBufferStateTable::LockUntil(size_t Frame)
	{
		for (auto& userBuffer : UserBuffers)
		{
			if (!userBuffer.WrittenTo)
				continue;

			Buffers[userBuffer.GetCurrentBuffer()].lockCounter = Frame;

			_UnMap(userBuffer.GetCurrentBuffer(), userBuffer.Offset);

			userBuffer.IncrementCurrentBuffer();
			auto bufferIdx					= userBuffer.GetCurrentBuffer();
			userBuffer.MappedPtr			= _Map(bufferIdx);
			userBuffer.WrittenTo			= false;
			userBuffer.Offset				= 0;
		}
	}


	/************************************************************************************************/


	void VertexBufferStateTable::Reset(VertexBufferHandle Handle)
	{
		auto& UserBuffer = UserBuffers[Handles[Handle]];

		if (UserBuffer.Offset == 0)
			return; // Unused

		if(UserBuffer.WrittenTo)
		{	// UnMap Current Buffer
			auto  ResourceIdx = UserBuffer.GetCurrentBuffer();
			D3D12_RANGE Range = { 0, UserBuffer.Offset };
			Buffers[ResourceIdx].resource->Unmap(0, &Range);
		}
		
		UserBuffer.WrittenTo = false;
		UserBuffer.IncrementCurrentBuffer();

		{	// Map Current Buffer
			auto	ResourceIdx		= UserBuffer.GetCurrentBuffer();
			void*	MappedPtr		= nullptr;

			D3D12_RANGE Range{ 0, 0 };
			Buffers[ResourceIdx].resource->Map(0, &Range, &MappedPtr);

			UserBuffer.Offset		= 0;
			UserBuffer.MappedPtr	= (char*)MappedPtr;
			UserBuffer.Resource		= Buffers[ResourceIdx].resource;
		}
	}


	/************************************************************************************************/


	ID3D12Resource* VertexBufferStateTable::GetAsset(VertexBufferHandle Handle)
	{
		return Buffers[UserBuffers[Handles[Handle]].GetCurrentBuffer()].resource;
	}


	/************************************************************************************************/


	size_t VertexBufferStateTable::GetCurrentVertexBufferOffset(VertexBufferHandle Handle) const
	{
		return UserBuffers[Handles[Handle]].Offset;
	}


	/************************************************************************************************/


	size_t VertexBufferStateTable::GetBufferSize(VertexBufferHandle Handle) const
	{
		return UserBuffers[Handles[Handle]].ResourceSize;
	}


	/************************************************************************************************/


	SubAllocation VertexBufferStateTable::Reserve(VertexBufferHandle Handle, size_t size) noexcept
	{
		auto	idx			= Handles[Handle];
		auto&	userBuffer  = UserBuffers[idx];
		auto	offset		= UserBuffers[idx].Offset;
		auto	mapped_Ptr	= UserBuffers[idx].MappedPtr;

		const size_t mask = 0XFF;

		if( offset & mask)
		{
			size_t alignementOffset = mask - (offset & mask) + 1;
			offset  += alignementOffset;
			size    += alignementOffset;
		}

		UserBuffers[idx].Offset		+= size;
		UserBuffers[idx].WrittenTo	 = true;

		return { mapped_Ptr, offset, size };
	}



	/************************************************************************************************/


	bool VertexBufferStateTable::CurrentlyAvailable(VertexBufferHandle Handle, size_t completed) const
	{
		auto UserIdx	= Handles[Handle];
		auto BufferIdx	= UserBuffers[UserIdx].GetCurrentBuffer();

		return Buffers[Handle].lockCounter <= completed;
	}


	/************************************************************************************************/


	char* VertexBufferStateTable::_Map(VBufferHandle handle)
	{
		void* ptr;
		Buffers[handle].resource->Map(0, nullptr, &ptr);

		return (char*)ptr;
	}


	void VertexBufferStateTable::_UnMap(VBufferHandle handle, size_t range)
	{
		D3D12_RANGE writtenRange;
		writtenRange.Begin  = 0;
		writtenRange.End    = range;

		Buffers[handle].resource->Unmap(0, &writtenRange);
	}


	/************************************************************************************************/


	void VertexBufferStateTable::Release()
	{
		for (auto& B : Buffers) {
			if(B.resource)
				B.resource->Release();
			B.resource = nullptr;
		}


		Buffers.Release();
		UserBuffers.Release();
		FreeBuffers.Release();
		Handles.Release();
	}


	/************************************************************************************************/


	void VertexBufferStateTable::ReleaseVertexBuffer(VertexBufferHandle handle, uint64_t current)
	{
		std::scoped_lock lock{ criticalSection };

		auto userIdx		= Handles[handle];
		auto& userEntry     = UserBuffers[userIdx];

		for (const auto ResourceIdx : userEntry.Buffers)
			if(ResourceIdx != INVALIDHANDLE)
				FreeBuffers.push_back({ current, ResourceIdx});
			else
				break;

		if (UserBuffers.size() > 1)
		{
			auto& back = UserBuffers.back();
			userEntry = back;

			Handles[back.handle] = userIdx;
		}

		UserBuffers.pop_back();
	}


	/************************************************************************************************/


	void VertexBufferStateTable::ReleaseFree(uint64_t current)
	{
		std::scoped_lock lock{ criticalSection };

		for (auto freeBuffer : FreeBuffers)
		{
			if (Buffers[freeBuffer.BufferIdx].lockCounter <= current)
				Buffers[freeBuffer.BufferIdx].resource->Release();
		}

		std::sort(FreeBuffers.begin(), FreeBuffers.end());

		FreeBuffers.erase(
			std::remove_if(
				FreeBuffers.begin(),
				FreeBuffers.end(),
				[&](auto buffer)-> bool
				{
					return Buffers[buffer].lockCounter <= current;
				}),
			FreeBuffers.end());
	}


	/************************************************************************************************/


	QueryHandle QueryTable::CreateQueryBuffer(size_t count, QueryType type)
	{
		D3D12_QUERY_HEAP_DESC desc;
		desc.Count			= (UINT)count;
		desc.NodeMask		= 0;

		ResourceEntry newResEntry = { 0 };

		switch (type)
		{
		case QueryType::OcclusionQuery:
			desc.Type			= D3D12_QUERY_HEAP_TYPE_OCCLUSION;
			newResEntry.type	= D3D12_QUERY_TYPE_OCCLUSION;
			break;
		case QueryType::BinaryOcclusionQuery:
			desc.Type			= D3D12_QUERY_HEAP_TYPE_OCCLUSION;
			newResEntry.type	= D3D12_QUERY_TYPE_BINARY_OCCLUSION;
			break;
		case QueryType::PipelineStats:
			desc.Type			= D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS;
			newResEntry.type	= D3D12_QUERY_TYPE_PIPELINE_STATISTICS;
			break;
		case QueryType::TimeStats:
			desc.Type           = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
			newResEntry.type    = D3D12_QUERY_TYPE_TIMESTAMP;
			break;
		default:
			break;
		}


		for (auto& Res : newResEntry.resources)
			RS->pDevice->CreateQueryHeap(&desc, IID_PPV_ARGS(&Res));

		size_t		userIdx = users.size();
		UserEntry	newUserEntry = { 0, 0, 0, false };

		newUserEntry.resourceIdx	= resources.size();
		users.push_back(newUserEntry);
		resources.push_back(newResEntry);

		return QueryHandle(userIdx);
	}


	/************************************************************************************************/


	QueryHandle	QueryTable::CreateSOQueryBuffer(size_t count, size_t SOIndex)
	{
		FK_ASSERT(SOIndex < D3D12_QUERY_TYPE_SO_STATISTICS_STREAM3 - D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0, "invalid argument");

		D3D12_QUERY_HEAP_DESC	heapDesc = {};
		
		heapDesc.Count	= (UINT)count;
		heapDesc.Type	= static_cast<D3D12_QUERY_HEAP_TYPE>(D3D12_QUERY_HEAP_TYPE_SO_STATISTICS);

		ResourceEntry newResEntry	= { 0 };
		newResEntry.currentResource = 0;
		newResEntry.type			= static_cast<D3D12_QUERY_TYPE>(D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0 + SOIndex);

		for (size_t itr = 0; itr < 3; ++itr)
		{
			newResEntry.layouts[itr] = DeviceLayout::Common;
			newResEntry.resourceLocks[itr] = 0;
			RS->pDevice->CreateQueryHeap(
				&heapDesc,
				IID_PPV_ARGS(&newResEntry.resources[itr]));
		}

		size_t resourceIdx = resources.push_back(newResEntry);

		return QueryHandle(users.push_back(
								{	resourceIdx,
									count,
									0,
									false}));
	}


	/************************************************************************************************/


	void QueryTable::LockUntil(QueryHandle handle, size_t FrameID)
	{
		auto user = users[handle];

		if (user.used)
		{
			size_t resourceIdx			= user.resourceIdx;
			size_t currentResourceIdx	= resources[resourceIdx].currentResource;

			resources[resourceIdx].currentResource = (currentResourceIdx + 2 % 3);
		}
	}


	/************************************************************************************************/


	void ResourceStateTable::Release()
	{
		for (size_t I = 0; I < UserEntries.size(); ++I)
		{
			auto& T = UserEntries[I];
			auto F  = T.Flags;

			Resources[T.ResourceIdx].Release();
		}

		for (auto& resource : delayRelease)
			if(resource.resource)
				resource.resource->Release();

		delayRelease.Release();
		UserEntries.Release();
		Resources.Release();
		Handles.Clear();
	}


	/************************************************************************************************/


	ResourceHandle ResourceStateTable::GetFreeHandle()
	{
		std::scoped_lock lock{ m };

		auto newHandle = Handles.GetNewHandle();
		Handles[newHandle] = -1;

		return newHandle;
	}


	/************************************************************************************************/


	ResourceHandle ResourceStateTable::AddResource(const GPUResourceDesc& desc, const DeviceLayout layout)
	{
		std::scoped_lock lock{m};

		auto Handle		 = Handles.GetNewHandle();

		ResourceEntry newEntry = 
		{	
			desc.bufferCount,
			0,
			{ nullptr, nullptr, nullptr },
			{ 0, 0, 0 },
			{ layout, layout, layout },
			TextureFormat2DXGIFormat(desc.format),
			desc.mipLevels,
			desc.WH,
			Handle,
			{ { desc.placed.offset, 0 }, {}, {} }
		};

		for (size_t I = 0; I < desc.bufferCount; ++I) {
			newEntry.Resources[I] = desc.resources[I].As<ID3D12Resource>();

#if USING(AFTERMATH)
			GFSDK_Aftermath_ResourceHandle  aftermathResource;
			GFSDK_Aftermath_DX12_RegisterResource(desc.resources[I], &aftermathResource);
			NewEntry.aftermathResource[I]   = aftermathResource;
#endif
		}

		UserEntry entry;
		entry.ResourceIdx		= Resources.push_back(newEntry);
		entry.resourceSize		= desc.byteSize;
		entry.FrameGraphIndex	= -1;
		entry.FGI_FrameStamp	= -1;
		entry.Handle			= Handle;
		entry.Format			= newEntry.Format;
		entry.Flags				= desc.swapChain ? ResourceFlags::SwapChain : 0;
		entry.dimension			= desc.Dimensions;
		entry.arraySize			= desc.arraySize;
		entry.tileMappings		= { allocator };

		if (desc.Dimensions == ResourceDimension::Buffer)
		{
			UAVResourceLayout layout;
			layout.format		= newEntry.Format;
			layout.elementCount	= (UINT)(desc.byteSize / 4);
			layout.stride		= 4;
			entry.extra			= layout;
		}

		Handles[Handle]	= (UINT)UserEntries.push_back(entry);

		if (desc.bufferCount > 1)
			BufferedResources.push_back(Handle);

		return Handle;
	}


	/************************************************************************************************/


	void ResourceStateTable::SetResource(ResourceHandle handle, const GPUResourceDesc& desc, const DeviceLayout layout)
	{
		std::scoped_lock lock{ m };

		ResourceEntry newEntry = 
		{	
			desc.bufferCount,
			0,
			{ nullptr, nullptr, nullptr },
			{ 0, 0, 0 },
			{ layout, layout , layout },
			TextureFormat2DXGIFormat(desc.format),
			desc.mipLevels,
			desc.WH,
			handle,
			{ { desc.placed.offset, 0 }, {}, {} }
		};

		for (size_t I = 0; I < desc.bufferCount; ++I) {
			newEntry.Resources[I] = desc.resources[I].As<ID3D12Resource>();

#if USING(AFTERMATH)
			GFSDK_Aftermath_ResourceHandle  aftermathResource;
			GFSDK_Aftermath_DX12_RegisterResource(desc.resources[I], &aftermathResource);
			NewEntry.aftermathResource[I]   = aftermathResource;
#endif
		}

		UserEntry entry;
		entry.ResourceIdx		= Resources.push_back(newEntry);
		entry.resourceSize		= desc.byteSize;
		entry.FrameGraphIndex	= -1;
		entry.FGI_FrameStamp	= -1;
		entry.Handle			= handle;
		entry.Format			= newEntry.Format;
		entry.Flags				= desc.swapChain ? ResourceFlags::SwapChain : 0;
		entry.dimension			= desc.Dimensions;
		entry.arraySize			= desc.arraySize;
		entry.tileMappings      = { allocator };

		if (desc.Dimensions == ResourceDimension::Buffer)
		{
			UAVResourceLayout layout;
			layout.format		= newEntry.Format;
			layout.elementCount	= (UINT)(desc.byteSize / 4);
			layout.stride		= 4;
			entry.extra			= layout;
		}

		Handles[handle]			= (UINT)UserEntries.push_back(entry);

		if (desc.bufferCount > 1)
			BufferedResources.push_back(handle);
	}


	/************************************************************************************************/


	void ResourceStateTable::ReleaseTexture(ResourceHandle handle, const uint64_t submissionID)
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		std::scoped_lock lock{ m };

		if (handle == InvalidHandle)
			return;

		const auto UserIdx	= Handles[handle];

		if (UserIdx == -1)
			return;

		auto& UserEntry		= UserEntries[UserIdx];
		const auto ResIdx	= UserEntry.ResourceIdx;
		auto& resource		= Resources[ResIdx];

		for (auto res : resource.Resources)
		{
			if (res)
				delayRelease.push_back({ res, submissionID });
		}

		const auto TempHandle	= UserEntries.back().Handle;
		UserEntry				= UserEntries.back();
		Handles[TempHandle]		= UserIdx;

		const auto temp			= Resources[UserEntry.ResourceIdx];
		Resources[ResIdx]		= Resources.back();
		UserEntries[Handles[resource.owner]].ResourceIdx = ResIdx;

		UserEntries.pop_back();
		Resources.pop_back();

		Handles.RemoveHandle(handle);
	}


	/************************************************************************************************/


	void ResourceStateTable::_ReleaseTextureForceRelease(ResourceHandle handle)
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto  UserIdx		= Handles[handle];
		auto& UserEntry		= UserEntries[UserIdx];
		const auto ResIdx	= UserEntry.ResourceIdx;
		auto& resource		= Resources[ResIdx];

		for (auto res : resource.Resources)
			res->Release();

		const auto TempHandle	= UserEntries.back().Handle;
		UserEntry				= UserEntries.back();
		Handles[TempHandle]		= UserIdx;

		const auto temp			= Resources[UserEntry.ResourceIdx];
		Resources[ResIdx]		= Resources.back();
		UserEntries[Handles[resource.owner]].ResourceIdx = ResIdx;

		UserEntries.pop_back();
		Resources.pop_back();

		Handles.RemoveHandle(handle);
	}


	/************************************************************************************************/


	Texture2D ResourceStateTable::operator[](ResourceHandle handle)
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		const auto Idx		    = Handles[handle];
		const auto resource     = Resources[UserEntries[Idx].ResourceIdx];

		const auto Res		    = resource.GetAsset();
		const auto WH			= resource.WH;
		const auto Format		= resource.Format;
		const auto mipCount	    = resource.mipCount;

		return { Res, WH, mipCount, Format };
	}


	/************************************************************************************************/


	void ResourceStateTable::SetLayout(ResourceHandle handle, DeviceLayout layout)
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		auto ResIdx  = UserEntries[UserIdx].ResourceIdx;

		Resources[ResIdx].SetLayout(layout);
	}


	/************************************************************************************************/


	void ResourceStateTable::SetDebug(ResourceHandle handle, const char* string)
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx                    = Handles[handle];
		UserEntries[UserIdx].userString = string;
	}


	/************************************************************************************************/


	void ResourceStateTable::SetBufferedIdx(ResourceHandle handle, uint32_t idx)
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		auto Residx  = UserEntries[UserIdx].ResourceIdx;

		Resources[Residx].CurrentResource = idx;
	}


	/************************************************************************************************/


	void ResourceStateTable::SetDebugName(ResourceHandle handle, const char* str)
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx    = Handles[handle];

		if (UserIdx == -1)
			return;

		auto resIdx     = UserEntries[UserIdx].ResourceIdx;
		auto& resource  = Resources[resIdx];

		UserEntries[UserIdx].userString = str;

		for (auto& res : resource.Resources)
			if (res)
				SETDEBUGNAME(res, str);
	}


	/************************************************************************************************/


	size_t ResourceStateTable::GetFrameGraphIndex(ResourceHandle handle, size_t FrameID) const
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx	= Handles[handle];
		auto FrameStamp	= UserEntries[UserIdx].FGI_FrameStamp;

		if (FrameStamp < int(FrameID))
			return INVALIDHANDLE;

		return UserEntries[UserIdx].FrameGraphIndex;
	}


	/************************************************************************************************/


	void ResourceStateTable::SetFrameGraphIndex(ResourceHandle handle, size_t FrameID, size_t Index)
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		UserEntries[UserIdx].FGI_FrameStamp		= FrameID;
		UserEntries[UserIdx].FrameGraphIndex	= (UINT)Index;
	}


	/************************************************************************************************/


	DXGI_FORMAT ResourceStateTable::GetFormat(ResourceHandle handle) const
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		return UserEntries[UserIdx].Format;
	}


	/************************************************************************************************/


	ResourceDimension ResourceStateTable::GetDimension(ResourceHandle handle) const
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		return UserEntries[UserIdx].dimension;
	}


	/************************************************************************************************/


	size_t ResourceStateTable::GetArraySize(ResourceHandle handle) const
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		return UserEntries[UserIdx].arraySize;
	}


	/************************************************************************************************/


	uint8_t ResourceStateTable::GetMIPCount(ResourceHandle handle) const
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		return Resources[UserEntries[UserIdx].ResourceIdx].mipCount;
	}


	/************************************************************************************************/


	void ResourceStateTable::SetWH(ResourceHandle handle, uint2 WH)
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx									= Handles[handle];
		Resources[UserEntries[UserIdx].ResourceIdx].WH	= WH;
	}


	/************************************************************************************************/


	void ResourceStateTable::SetExtra(ResourceHandle handle, GPUResourceExtra_t extra)
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		UserEntries[UserIdx].extra = extra;
	}


	/************************************************************************************************/


	GPUResourceExtra_t  ResourceStateTable::GetExtra(ResourceHandle handle) const
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		return UserEntries[UserIdx].extra;
	}


	/************************************************************************************************/


	const char* ResourceStateTable::GetDebug(ResourceHandle handle)
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		return UserEntries[UserIdx].userString;
	}


	/************************************************************************************************/


	uint2 ResourceStateTable::GetWH(ResourceHandle handle) const
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx = Handles[handle];
		return Resources[UserEntries[UserIdx].ResourceIdx].WH;
	}


	/************************************************************************************************/


	uint3 ResourceStateTable::GetXYZ(ResourceHandle handle) const
	{
		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto UserIdx	= Handles[handle];
		auto& user		= UserEntries[UserIdx];
		auto& resource	= Resources[UserEntries[UserIdx].ResourceIdx];

		return { resource.WH, user.arraySize };
	}


	/************************************************************************************************/


	void ResourceStateTable::UpdateTileMappings(ResourceHandle handle, const TileMapping* begin, const TileMapping* end, iAllocator& temp)
	{
		if (handle == InvalidHandle)
			return;

		FK_ASSERT(handle >= Handles.size(), "Invalid Handle Detected");

		auto itr		= begin;
		auto UserIdx	= Handles[handle];
		auto& mappings	= UserEntries[UserIdx].tileMappings;

		Vector<TileMapping, 256> newElements{ temp };

		std::sort(mappings.begin(), mappings.end(),
			[](const auto& lhs, const auto& rhs)
			{
				return lhs.sortingID() < rhs.sortingID();
			});

		std::for_each(begin, end,
			[&](const TileMapping& updatedTile)
			{
				auto pred = [](const TileMapping& lhs, const TileMapping& rhs)
				{
					return lhs.tileID < rhs.tileID;
				};

				auto res = std::lower_bound(
					mappings.begin(),
					mappings.end(),
					updatedTile,
					pred);

				if (res != mappings.end() && res->tileID == updatedTile.tileID)
					res->state = updatedTile.state;
				else
					newElements.push_back(updatedTile);
			});

		mappings.reserve(mappings.size() + newElements.size());

		for (auto& e : newElements)
			mappings.push_back(e);
	}


	/************************************************************************************************/


	void ResourceStateTable::SubmitTileUpdates(ID3D12CommandQueue* queue, dxRenderSystem& renderSystem, iAllocator* allocator_temp)
	{
		Vector<D3D12_TILED_RESOURCE_COORDINATE>	coordinates	{ allocator_temp };
		Vector<D3D12_TILE_REGION_SIZE>			regionSize	{ allocator_temp };
		Vector<D3D12_TILE_RANGE_FLAGS>			tile_flags	{ allocator_temp };
		Vector<UINT>							heapOffsets	{ allocator_temp };
		Vector<UINT>							tileCounts	{ allocator_temp };

		for (auto& userEntry : UserEntries)
		{
			coordinates.size();
			regionSize.size();

			ID3D12Resource*	resource	= nullptr;
			ID3D12Heap*		heap		= nullptr;

			for (const auto& mapping: userEntry.tileMappings)
			{
				auto desc = resource->GetDesc();
				
				if (auto mappingHeap = renderSystem.GetDeviceResource(mapping.heap).As<ID3D12Heap>(); heap != mappingHeap)
				{
					if (!coordinates.size())
						__debugbreak();

					if (mappingHeap)
						queue->UpdateTileMappings(
							resource,
							(UINT)coordinates.size(),
							coordinates.data(),
							regionSize.data(),
							heap,
							(UINT)tile_flags.size(),
							tile_flags.data(),
							heapOffsets.data(),
							tileCounts.data(),
							D3D12_TILE_MAPPING_FLAG_NONE);

					coordinates.clear();
					regionSize.clear();
					tile_flags.clear();
					heapOffsets.clear();
					tileCounts.clear();
				}
				else
				{
					coordinates.push_back(
						D3D12_TILED_RESOURCE_COORDINATE{
							mapping.tileID.GetTileX(),
							mapping.tileID.GetTileY(),
							0,
							(UINT)mapping.tileID.GetMipLevel()
						});

					regionSize.push_back(
						D3D12_TILE_REGION_SIZE{
							1,
							true,
							1,
							1,
							1,
						});

					tile_flags.push_back(D3D12_TILE_RANGE_FLAGS::D3D12_TILE_RANGE_FLAG_NONE);
					heapOffsets.push_back(mapping.heapOffset);
					tileCounts.push_back(1);
				}
			}
		}
	}


	/************************************************************************************************/


	const TileMapList& ResourceStateTable::GetTileMappings(const ResourceHandle handle) const
	{
		auto UserIdx = Handles[handle];
		return UserEntries[UserIdx].tileMappings;
	}


	/************************************************************************************************/


	TileMapList& ResourceStateTable::_GetTileMappings(const ResourceHandle handle)
	{
		auto UserIdx = Handles[handle];
		return UserEntries[UserIdx].tileMappings;
	}


	/************************************************************************************************/


	void ResourceStateTable::MarkRTUsed(ResourceHandle Handle)
	{
		auto UserIdx = Handles[Handle];
		UserEntries[UserIdx].Flags |= ResourceFlags::INUSE;
	}


	/************************************************************************************************/


	void ResourceStateTable::LockUntil(size_t FrameID)
	{
		for (auto bufferedResource : BufferedResources)
		{
			const auto idx  = Handles[bufferedResource];
			auto& UserEntry = UserEntries[idx];
			auto Flags      = UserEntry.Flags;

			if (Flags & ResourceFlags::INUSE && !(Flags & ResourceFlags::SwapChain))
			{
				Resources[UserEntry.ResourceIdx].SetFrameLock(FrameID);
				Resources[UserEntry.ResourceIdx].IncreaseIdx();
			}
		}
	}


	/************************************************************************************************/


	void ResourceStateTable::FreeDelayedResources(ThreadManager& threads, const uint64_t completed)
	{
		if (delayRelease.size() < 32)
			return;

		ProfileFunction();

		Vector<ID3D12Resource*> freeList = { delayRelease.Allocator };

		for (auto& [resource, submissionID] : delayRelease)
		{
			if (completed > submissionID)
			{
				freeList.push_back(resource);
				resource = nullptr;
			}
		}

		if (freeList.size())
		{
			if(false)
			{
				auto& workItem = FlexKit::CreateWorkItem(
					[freeList = std::move(freeList)](auto& threadLocalAllocator)
					{
						for (auto res : freeList)
						{
#if USING(AFTERMATH)
							if (res)
								GFSDK_Aftermath_DX12_UnregisterResource(res);
#endif

							res->Release();
						}
					}, delayRelease.Allocator);

				threads.AddBackgroundWork(workItem);
			}
			else
			{
				for (auto& res : freeList)
					res->Release();
			}
		}

		delayRelease.erase(
			std::remove_if(
				delayRelease.begin(),
				delayRelease.end(),
				[&](auto& res) { return res.resource == nullptr; }),

		delayRelease.end());
	}


	/************************************************************************************************/


	bool ResourceStateTable::FreeDelayedResourcesIncrementally(const uint64_t completed)
	{
		ProfileFunction();

		for (auto& release : delayRelease)
		{
			auto&& [resource, submissionID] = release;
			if (completed > submissionID)
			{
				resource->Release();
				delayRelease.remove_unstable(&release);
				return true;
			}
		}
		return false;
	}


	/************************************************************************************************/


	void ResourceStateTable::ResourceEntry::Release()
	{
		for (auto& R : Resources)
		{
			if (R)
				R->Release();

			R = nullptr;
		}
	}


	/************************************************************************************************/


	DeviceLayout ResourceStateTable::GetLayout(ResourceHandle Handle) const
	{
		auto    Idx			    = Handles[Handle];
		auto    ResourceIdx	    = UserEntries[Idx].ResourceIdx;
		const   auto& resources = Resources[ResourceIdx];

		return resources.layouts[resources.CurrentResource];
	}


	/************************************************************************************************/


	ID3D12Resource* ResourceStateTable::GetResource(ResourceHandle Handle, ID3D12Device* device) const
	{
		if (Handle >= Handles.size())
			return nullptr;

		auto  idx			= Handles[Handle];
		auto  resourceIdx	= UserEntries[idx].ResourceIdx;
		auto& resource		= Resources[resourceIdx];
		auto& resources		= resource.Resources;
		auto  currentIdx	= resource.CurrentResource;

		return resources[currentIdx];
	}


	/************************************************************************************************/


	std::span<ID3D12Resource*>	ResourceStateTable::GetResources(ResourceHandle Handle)
	{
		if (Handle >= Handles.size())
			return {};

		auto  idx			= Handles[Handle];
		auto  resourceIdx	= UserEntries[idx].ResourceIdx;
		auto& resource		= Resources[resourceIdx];

		return {
			resource.Resources + 0,
			resource.Resources + resource.ResourceCount
		};
	}


	/************************************************************************************************/


	size_t ResourceStateTable::GetResourceSize(ResourceHandle Handle) const
	{
		auto  Idx			= Handles[Handle];
		return UserEntries[Idx].resourceSize;
	}


	/************************************************************************************************/


	uint2 ResourceStateTable::GetHeapOffset(ResourceHandle Handle, uint subResourceIdx) const
	{
		auto Idx = Handles[Handle];

		return Resources[UserEntries[Idx].resourceSize].heapRange[subResourceIdx];
	}


	/************************************************************************************************/


	ResourceHandle ResourceStateTable::FindResourceHandle(ID3D12Resource* deviceResource) const
	{
		auto res = std::find_if(
			Resources.begin(), Resources.end(),
			[&](auto res)
			{
				for (size_t I = 0; I < res.ResourceCount; I++)
					if (res.Resources[I] == deviceResource)
						return true;

				return false;
			});

		if (res != Resources.end())
			return res->owner;
		else
		{
			auto res = std::find_if(
				delayRelease.begin(), delayRelease.end(),
				[&](auto res)
				{
					if (res.resource == deviceResource)
						return true;

					return false;
				});

			if (res != delayRelease.end())
				__debugbreak();
		}

		return InvalidHandle;
	}


	/************************************************************************************************/


	void ResourceStateTable::ReplaceResources(ResourceHandle handle, ID3D12Resource** begin, size_t size)
	{
		auto  Idx                   = Handles[handle];
		auto  ResourceIdx           = UserEntries[Idx].ResourceIdx;
		const auto resourceCount    = Resources[ResourceIdx].ResourceCount;
		auto& resources             = Resources[ResourceIdx].Resources;

		for (size_t I = 0; I < Min(resourceCount, size); ++I)
			resources[I] = begin[I];
	}


	/************************************************************************************************/


	VertexResourceBuffer dxRenderSystem::_CreateVertexBufferDeviceResource(const size_t ResourceSize, bool GPUResident)
	{
		D3D12_RESOURCE_DESC   Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(ResourceSize);
		Resource_DESC.Alignment          = 0;
		Resource_DESC.DepthOrArraySize   = 1;
		Resource_DESC.Dimension          = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		Resource_DESC.Layout             = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Resource_DESC.Width              = ResourceSize;
		Resource_DESC.Height             = 1;
		Resource_DESC.Format             = DXGI_FORMAT_UNKNOWN;
		Resource_DESC.SampleDesc.Count   = 1;
		Resource_DESC.SampleDesc.Quality = 0;
		Resource_DESC.Flags              = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;

		D3D12_HEAP_PROPERTIES HEAP_Props = {};
		HEAP_Props.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type                  = GPUResident ? D3D12_HEAP_TYPE_DEFAULT : D3D12_HEAP_TYPE_UPLOAD;
		HEAP_Props.MemoryPoolPreference  = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask      = 0;
		HEAP_Props.VisibleNodeMask       = 0;

		FrameBufferedResource NewResource;
		NewResource.BufferCount          = BufferCount;

		auto InitialState = GPUResident ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_GENERIC_READ;

		ID3D12Resource* Resource = nullptr;
		HRESULT HR = pDevice->CreateCommittedResource(
			&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
			&Resource_DESC, InitialState, nullptr,
			IID_PPV_ARGS(&Resource));

		SETDEBUGNAME(Resource, __func__);

		return Resource;
	}


	/************************************************************************************************/


	ConstantBuffer dxRenderSystem::_CreateConstantBufferResource(dxRenderSystem* RS, ConstantBuffer_desc* desc)
	{
		D3D12_RESOURCE_DESC   Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(desc->InitialSize);
		Resource_DESC.Alignment				= 0;
		Resource_DESC.DepthOrArraySize		= 1;
		Resource_DESC.Dimension				= D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		Resource_DESC.Layout				= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Resource_DESC.Width					= desc->InitialSize;
		Resource_DESC.Height				= 1;
		Resource_DESC.Format				= DXGI_FORMAT_UNKNOWN;
		Resource_DESC.SampleDesc.Count		= 1;
		Resource_DESC.SampleDesc.Quality	= 0;
		Resource_DESC.Flags					= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

		D3D12_HEAP_PROPERTIES HEAP_Props ={};
		HEAP_Props.CPUPageProperty	     = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type				     = D3D12_HEAP_TYPE_DEFAULT;
		HEAP_Props.MemoryPoolPreference  = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask	     = 0;
		HEAP_Props.VisibleNodeMask		 = 0;

		size_t BufferCount = RS->BufferCount;
		FrameBufferedResource NewResource;
		NewResource.BufferCount = BufferCount;

		for(size_t I = 0; I < BufferCount; ++I)
		{
			ID3D12Resource* Resource = nullptr;
			HRESULT HR = RS->pDevice->CreateCommittedResource(
							&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, 
							&Resource_DESC, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr, 
							IID_PPV_ARGS(&Resource));

			CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE CONSTANT BUFFER"));
			NewResource.Resources[I] = Resource;

			SETDEBUGNAME(Resource, __func__);
		}

		return NewResource;
	}


	/************************************************************************************************/


	std::optional<DescriptorRange>	dxRenderSystem::CreateDescriptorRange(const uint32_t size)
	{
		uint64_t completed = directFence->GetCompletedValue();

		auto res = descriptorHeapAllocator.Alloc(size, completed);

		if (res)
			return res.value();
		else
			return {};// std::unexpected{ dxRenderSystem::DescriptorRangeAllocationError::OutOfSpace };
	}


	/************************************************************************************************/


	void dxRenderSystem::ReleaseDescriptorRange(DescriptorRange range, uint64_t lock)
	{
		descriptorHeapAllocator.Release(range, lock, directFence->GetCompletedValue());
	}


	/************************************************************************************************/


	void dxRenderSystem::_PushDelayReleasedResource(ID3D12Resource* resource)
	{
		dxRenderSystem::FreeEntry entry{
			.Resource	= resource,
			.Counter	= GetCurrentCounter(),
		};

	    FreeList_GraphicsQueue.push_back(entry);
	}

	void dxRenderSystem::_PushDelayReleasedResource(ID3D12Resource* resource, CopyContextHandle uploadQueue)
	{
		if (uploadQueue != InvalidHandle)
			copyEngine.Push_Temporary(resource, uploadQueue);
		else
			FreeList_CopyQueue.push_back({ resource, copyEngine.counter });
	}


	/************************************************************************************************/


	void dxRenderSystem::_ForceReleaseTexture(ResourceHandle handle)
	{
		Textures._ReleaseTextureForceRelease(handle);
	}


	void dxRenderSystem::_ReleaseDelayedResources()
	{
		//while (Textures.FreeDelayedResourcesIncrementally(directFence->GetCompletedValue()));
		Textures.FreeDelayedResources(threads, directFence->GetCompletedValue());
	}


	/************************************************************************************************/


	dxRenderSystem::VidMemoryStates dxRenderSystem::_GetVidMemStats()
	{
		DXGI_QUERY_VIDEO_MEMORY_INFO VideoMemInfo = {0};
		if(pDXGIAdapter)
			pDXGIAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &VideoMemInfo);

		return { VideoMemInfo.CurrentUsage, VideoMemInfo.Budget };
	}


	/************************************************************************************************/


	[[nodiscard]] ID3D12DescriptorHeap* dxRenderSystem::_CreateShaderVisibleHeap(const size_t numDescriptors)
	{
		ID3D12DescriptorHeap* heap;
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapdesc;
		descriptorHeapdesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		descriptorHeapdesc.NumDescriptors	= (uint32_t)numDescriptors;
		descriptorHeapdesc.NodeMask			= 0u;
		descriptorHeapdesc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		HRESULT HR							= pDevice->CreateDescriptorHeap(&descriptorHeapdesc, IID_PPV_ARGS(&heap));
		FK_ASSERT(HR, "FAILED TO CREATE DESCRIPTOR HEAP");

		return heap;
	}


	/************************************************************************************************/


	ID3D12QueryHeap* dxRenderSystem::_GetQueryResource(QueryHandle Handle)
	{
		return Queries.GetDeviceObject(Handle);
	}



	/************************************************************************************************/


	CopyContext& dxRenderSystem::GetCopyContext(CopyContextHandle handle)
	{
		if (handle == InvalidHandle)
		{
			if (ImmediateUpload == InvalidHandle)
				ImmediateUpload = OpenUploadQueue();

			handle = ImmediateUpload;
		}

		return copyEngine[handle];
	}


	/************************************************************************************************/


	void dxRenderSystem::_OnCrash()
	{
		static std::mutex m;

		if (!m.try_lock())
		{
			DebugBreak();
			return;
		}

		EXITSCOPE(m.unlock());

		auto reason = pDevice->GetDeviceRemovedReason();

		switch (reason)
		{
		case DXGI_ERROR_DEVICE_HUNG:
			FK_LOG_ERROR("DXGI_ERROR_DEVICE_HUNG");				break;
		case DXGI_ERROR_DEVICE_REMOVED:
			FK_LOG_ERROR("DXGI_ERROR_DEVICE_REMOVED");			break;
		case DXGI_ERROR_DEVICE_RESET:
			FK_LOG_ERROR("DXGI_ERROR_DEVICE_RESET");			break;
		case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
			FK_LOG_ERROR("DXGI_ERROR_DRIVER_INTERNAL_ERROR");	break;
		case DXGI_ERROR_INVALID_CALL:
			FK_LOG_ERROR("DXGI_ERROR_INVALID_CALL");			break;
		case S_OK:
			FK_LOG_ERROR("???? S_OK ????");						break;
		}

#if USING(AFTERMATH)
		{
			/*
			// Decode the crash dump to a JSON string.
			// Step 1: Generate the JSON and get the size.
			uint32_t jsonSize = 0;
			GFSDK_Aftermath_GpuCrashDump_GenerateJSON(
				decoder,
				GFSDK_Aftermath_GpuCrashDumpDecoderFlags_ALL_INFO,
				GFSDK_Aftermath_GpuCrashDumpFormatterFlags_NONE,
				dxRenderSystem::OnShaderDebugInfoLookup,
				dxRenderSystem::OnShaderLookup,
				dxRenderSystem::OnShaderInstructionsLookup,
				dxRenderSystem::OnShaderSourceDebugInfoLookup,
				this,
				&jsonSize);

			// Step 2: Allocate a buffer and fetch the generated JSON.
			std::vector<char> json(jsonSize);
			GFSDK_Aftermath_GpuCrashDump_GetJSON(
				decoder,
				uint32_t(json.size()),
				json.data());

			// Write the the crash dump data as JSON to a file.
			const std::string jsonFileName = crashDumpFileName + ".json";
			std::ofstream jsonFile(jsonFileName, std::ios::out | std::ios::binary);
			if (jsonFile)
			{
				jsonFile.write(json.data(), json.size());
				jsonFile.close();
			}
			std::lock_guard lock{ crashM };
			WriteGpuCrashDumpToFile(gpuCrashDrump, gpuCrashDumpSize);
			*/

			GFSDK_Aftermath_Device_Status           device_status;


			std::vector<GFSDK_Aftermath_ContextHandle> context_handles;
			std::vector<GFSDK_Aftermath_ContextData> context_crash_info{ Contexts.size() };

			for (auto& context : Contexts) {
				context_handles.push_back(context.AFTERMATH_context);
			}

			GFSDK_Aftermath_GetData(context_handles.size(), context_handles.data(), context_crash_info.data());

			std::vector<GFSDK_Aftermath_ContextData> faultedContexts;

			for (auto& contextInfo : context_crash_info)
			{
				if (contextInfo.status == GFSDK_Aftermath_Context_Status_Invalid) {
					faultedContexts.push_back(contextInfo);
					GFSDK_Aftermath_GetContextError(&contextInfo);
				}

			}





			GFSDK_Aftermath_GetDeviceStatus(&device_status);

			Sleep(3000); // Aftermath needs a moment to do it's crash dump

			switch(device_status)
			{
			case GFSDK_Aftermath_Device_Status_Active:
				FK_LOG_ERROR("GFSDK_Aftermath_Device_Status_Active"); break;
			case GFSDK_Aftermath_Device_Status_Timeout:
				FK_LOG_ERROR("GFSDK_Aftermath_Device_Status_Timeout"); break;
			case GFSDK_Aftermath_Device_Status_OutOfMemory:
				FK_LOG_ERROR("GFSDK_Aftermath_Device_Status_OutOfMemory"); break;
			case GFSDK_Aftermath_Device_Status_PageFault:
				FK_LOG_ERROR("GFSDK_Aftermath_Device_Status_PageFault"); break;
			case GFSDK_Aftermath_Device_Status_Stopped:
				FK_LOG_ERROR("GFSDK_Aftermath_Device_Status_Stopped"); break;
			case GFSDK_Aftermath_Device_Status_Reset:
				FK_LOG_ERROR("GFSDK_Aftermath_Device_Status_Reset"); break;
			case GFSDK_Aftermath_Device_Status_Unknown:
				FK_LOG_ERROR("GFSDK_Aftermath_Device_Status_Unknown"); break;
			case GFSDK_Aftermath_Device_Status_DmaFault:
			{
				GFSDK_Aftermath_PageFaultInformation    pageFault;

				memset(&pageFault, 0, sizeof(pageFault));

				GFSDK_Aftermath_GetPageFaultInformation(&pageFault);

				if (pageFault.bHasPageFaultOccured) {
					FK_LOG_ERROR("GFSDK_Aftermath_Device_Status_DmaFault : %u", pageFault.resourceDesc.ptr_align_pAppResource);

					auto res = Textures.FindResourceHandle(pageFault.resourceDesc.pAppResource);
					if(res != InvalidHandle)
					{
						auto debugString = Textures.GetDebug(res);
						if (debugString)
							FK_LOG_ERROR("DMA FAULT DETECTED INVOLVING RESOURCE: %s!", debugString);
						else
							FK_LOG_ERROR("DMA FAULT DETECTED!");
					}
				}
				

			}   break;
			};


			Sleep(3000); // Aftermath needs a moment to do it's crash dump

			__debugbreak();

			for (auto& context : Contexts)
				GFSDK_Aftermath_ReleaseContextHandle(context.AFTERMATH_context);


			exit(-1);
		}
#else

#if USING(ENABLEDRED)

		if (FAILED(pDevice->QueryInterface(&dred)))
			FK_LOG_ERROR("CRASH DETECTED, DRED NOT ENABLED!");
		else
			FK_LOG_ERROR("Dumping Breadcrumbs");

		D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 DredAutoBreadcrumbsOutput	= {};
		D3D12_DRED_PAGE_FAULT_OUTPUT		DredPageFaultOutput			= {};

		DebugBreak();

		std::this_thread::sleep_for(1s);

		if (auto HR = dred->GetAutoBreadcrumbsOutput1(&DredAutoBreadcrumbsOutput); FAILED(HR))
		{
			FK_LOG_ERROR("Failed to get Breadcrumbs!");
			exit(-1);
		}

		if (auto HR = dred->GetPageFaultAllocationOutput(&DredPageFaultOutput); FAILED(HR))
			FK_LOG_ERROR("Failed to get Fault Allocation Info!");

		const D3D12_AUTO_BREADCRUMB_NODE1* node = DredAutoBreadcrumbsOutput.pHeadAutoBreadcrumbNode;

		if(!node) 
			FK_LOG_ERROR("No Breadcrumbs!");

		while (node)
		{
			auto event = *node->pCommandHistory;
			if (event)
			{
				switch (event)
				{
				case D3D12_AUTO_BREADCRUMB_OP_SETMARKER:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_SETMARKER"); break;
				case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT: command list %s", node->pCommandListDebugNameA); break;
				case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_ENDEVENT"); break;
				case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED"); break;
				case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED"); break;
				case D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT"); break;
				case D3D12_AUTO_BREADCRUMB_OP_DISPATCH:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_DISPATCH"); break;
				case D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION"); break;
				case D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION"); break;
				case D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE"); break;
				case D3D12_AUTO_BREADCRUMB_OP_COPYTILES:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_COPYTILES"); break;
				case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE"); break;
				case D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW"); break;
				case D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW"); break;
				case D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW"); break;
				case D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER"); break;
				case D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE"); break;
				case D3D12_AUTO_BREADCRUMB_OP_PRESENT:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_PRESENT"); break;
				case D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA"); break;
				case D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION"); break;
				case D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION"); break;
				case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME"); break;
				case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES"); break;
				case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT"); break;
				case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64"); break;
				case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION"); break;
				case D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE"); break;
				case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1"); break;
				case D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION"); break;
				case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2"); break;
				case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1"); break;
				case D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE"); break;
				case D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO"); break;
				case D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE"); break;
				case D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS"); break;
				case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND"); break;
				case D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND"); break;
				case D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION"); break;
				case D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP"); break;
				case D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1"); break;
				case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND"); break;
				case D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND:
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND"); break;
				case D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH: 
					FK_LOG_ERROR("D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH");  break;
				};
			}

			const size_t ctxCount = node->BreadcrumbContextsCount;
			for (size_t i = 0; i < ctxCount; i++)
			{
				std::wcout << "BreadCrumb: " << node->pBreadcrumbContexts[i].BreadcrumbIndex << " | " << node->pBreadcrumbContexts[i].pContextString << '\n';
			}
			node = node->pNext;
		}
#endif
		__debugbreak();
		exit(-1);
#endif
	}


	/************************************************************************************************/

#if USING(AFTERMATH)

	void RenderSystem::WriteGpuCrashDumpToFile(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize)
	{
		FK_LOG_ERROR("dxRenderSystem::WriteGpuCrashDumpToFile");
	}


	void RenderSystem::GpuCrashDumpCallback(const void* gpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData)
	{
		FK_LOG_ERROR("dxRenderSystem::GpuCrashDumpCallback");

		RenderSystem*   renderSystem = reinterpret_cast<RenderSystem*>(pUserData);

		std::vector<GFSDK_Aftermath_ContextHandle> context_handles;
		std::vector<GFSDK_Aftermath_ContextData> context_crash_info{ renderSystem->Contexts.size() };


		GFSDK_Aftermath_Device_Status           device_status;
		GFSDK_Aftermath_PageFaultInformation    pafeFault;

		for (auto& context : renderSystem->Contexts)
			context_handles.push_back(context.AFTERMATH_context);

		GFSDK_Aftermath_GetDeviceStatus(&device_status);
		GFSDK_Aftermath_GetData(context_handles.size(), context_handles.data(), context_crash_info.data());
		GFSDK_Aftermath_GetPageFaultInformation(&pafeFault);

		GFSDK_Aftermath_GpuCrashDump_Decoder decoder = {};

		auto res = GFSDK_Aftermath_GpuCrashDump_CreateDecoder(
			GFSDK_Aftermath_Version_API,
			gpuCrashDump,
			gpuCrashDumpSize,
			&decoder);

		GFSDK_Aftermath_GpuCrashDump_BaseInfo baseInfo = {};
		GFSDK_Aftermath_GpuCrashDump_GetBaseInfo(decoder, &baseInfo);

		uint32_t applicationNameLength = 0;
		GFSDK_Aftermath_GpuCrashDump_GetDescriptionSize(
			decoder,
			GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName,
			&applicationNameLength);

		std::vector<char> applicationName(applicationNameLength, '\0');

		GFSDK_Aftermath_GpuCrashDump_GetDescription(
			decoder,
			GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName,
			uint32_t(applicationName.size()),
			applicationName.data());

		static int count = 0;
		const std::string baseFileName =
			std::string("TestGame")
			+ "-"
			+ std::to_string(baseInfo.pid)
			+ "-"
			+ std::to_string(++count);

		const std::string crashDumpFileName = baseFileName + ".nv-gpudmp";
		std::ofstream dumpFile(crashDumpFileName, std::ios::out | std::ios::binary);
		if (dumpFile)
		{
			dumpFile.write((const char*)gpuCrashDump, gpuCrashDumpSize);
			dumpFile.close();
		}
	}



	void RenderSystem::ShaderDebugInfoCallback(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize, void* pUserData)
	{
		RenderSystem* renderSystem = reinterpret_cast<RenderSystem*>(pUserData);
		__debugbreak();
	}


	void RenderSystem::CrashDumpDescriptionCallback(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription, void* pUserData)
	{
		RenderSystem* renderSystem = reinterpret_cast<RenderSystem*>(pUserData);
		__debugbreak();
	}



	void RenderSystem::OnShaderDebugInfoLookup(const GFSDK_Aftermath_ShaderDebugInfoIdentifier* pIdentifier, PFN_GFSDK_Aftermath_SetData setShaderDebugInfo, void* pUserData)
	{
		__debugbreak();
	}



	void RenderSystem::OnShaderLookup(const GFSDK_Aftermath_ShaderHash* pShaderHash, PFN_GFSDK_Aftermath_SetData setShaderBinary, void* pUserData)
	{
		__debugbreak();
	}



	void RenderSystem::OnShaderInstructionsLookup(const GFSDK_Aftermath_ShaderInstructionsHash* pShaderInstructionsHash, PFN_GFSDK_Aftermath_SetData setShaderBinary, void* pUserData)
	{
		__debugbreak();
	}



	void RenderSystem::OnShaderSourceDebugInfoLookup(const GFSDK_Aftermath_ShaderDebugName* pShaderDebugName, PFN_GFSDK_Aftermath_SetData setShaderBinary, void* pUserData)
	{
		__debugbreak();
	}
#endif


	bool dxRenderSystem::DEBUG_AttachPIX()
	{
#if USING(PIX)
		if (pix)
			return true;

		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pix))))
		{
			FK_LOG_INFO("Pix Attached.");
			return true;
		}
#endif
		return false;
	}


	bool dxRenderSystem::DEBUG_BeginPixCapture()
	{
#if USING(PIX)
		if (pix)
		{
			pix->BeginCapture();
			FK_LOG_INFO("Pix capture started.");
		}

		return pix != nullptr;
#else
		return false;
#endif
	}


	bool dxRenderSystem::DEBUG_EndPixCapture()
	{
#if USING(PIX)
		if (pix)
		{
			pix->EndCapture();
			FK_LOG_INFO("Pix capture ended.");
		}

		return pix != nullptr;
#else
		return false;
#endif
	}


	/************************************************************************************************/


	ResourceHandle dxRenderSystem::_CreateDefaultTexture()
	{
		char tempBuffer[256];
		memset(tempBuffer, 0xffff, 256);

		auto upload = OpenUploadQueue();

		TextureBuffer textureBuffer{ { 1,  1 }, (std::byte*)tempBuffer, 256, 4, nullptr };

		auto defaultTexture = MoveTextureBuffersToVRAM(
			*this,
			upload,
			&textureBuffer,
			1,
			1,
			DeviceFormat::R8G8B8A8_UNORM);

		SetDebugName(defaultTexture, "Default Texture");
		SubmitUploadQueues(&upload);

		SyncDirectTo(SyncUploadTicket());

		return defaultTexture;
	}


	/************************************************************************************************/


	UploadReservation dxRenderSystem::ReserveDirectUploadSpace(size_t size, size_t alignment) noexcept
	{
		std::scoped_lock lock{ directUploadBufferMutex };
		auto res = directUploadBuffer.Reserve(size, alignment);

		if (res)
			return res.value();
		else if(res.error() == ReserveErrors::OutOfSpace)
		{
			auto oldBuffer = directUploadBuffer.Resize(directUploadBuffer.size * 2);

			if (oldBuffer)
			{
				FreeList_GraphicsQueue.emplace_back(
					oldBuffer,
					directSubmissionCounter.load(std::memory_order_relaxed));
			}

			return directUploadBuffer.Reserve(size, alignment).value_or(UploadReservation{});
		}

		return UploadReservation{};
	}


	/************************************************************************************************/


	UploadReservation	dxRenderSystem::ReserveUploadBuffer(const size_t uploadSize, CopyContextHandle)	noexcept
	{
		return UploadReservation{};
	}


	/************************************************************************************************/


	RootSignature* dxRenderSystem::_CreateRootSignature(ID3D12RootSignature* rootSig, RootSignatureBuilder& builder)
	{
		{
			std::shared_lock lock{ rootSignatureLock };

			if (auto res = rootSignatures[(uint64_t)rootSig]; res != nullptr)
				return res->get();
		}


		auto& object			= allocator->allocate_aligned<RootSignature>(rootSig, std::move(builder.Heaps), allocator);

		if (builder.RootEntries.size())
		{
			object.slots.reserve(builder.RootEntries.size());
			for (const auto& s : builder.RootEntries)
			{
				switch (s.Type)
				{
				case RootSignatureEntryType::DescriptorHeap:
					object.slots.push_back(RootSignature::SlotType::DescriptorSet);
					break;
				case RootSignatureEntryType::ConstantBuffer:
					object.slots.push_back(RootSignature::SlotType::CBV);
					break;
				case RootSignatureEntryType::StructuredBuffer:
					object.slots.push_back(RootSignature::SlotType::SRV);
					break;
				case RootSignatureEntryType::UnorderedAccess:
					object.slots.push_back(RootSignature::SlotType::UAV);
					break;
				case RootSignatureEntryType::UINT:
					object.slots.push_back(RootSignature::SlotType::UINT);
					break;
				default:
				case RootSignatureEntryType::Error:
					throw std::runtime_error{ "DX: Invalid Slot type!" };
				    break;
				}
			}
		}

		auto object_ptr	= RootSignature_ptr(
			&object,
			RootSignatureDeleter{ allocator });

		std::unique_lock unique{ rootSignatureLock };
		auto rootSignatureEntry = rootSignatures.insert((uint64_t)rootSig, std::move(object_ptr));


		return &object;
	}


	/************************************************************************************************/


	RootSignature* dxRenderSystem::_CreateRootSignature(RootSignatureBuilder& builder, iAllocator& temp)
	{
		Vector<Vector<CD3DX12_DESCRIPTOR_RANGE1, 16>, 16> desciptorHeaps{ temp };
		Vector<RootSignature::SlotType, 32>		slots{ allocator };
		static_vector<CD3DX12_ROOT_PARAMETER1>	parameters;
		uint32_t heapIdx = 0;

		for (const auto& I : builder.RootEntries)
		{
			CD3DX12_ROOT_PARAMETER1 Param;

			switch (I.Type)
			{
			case RootSignatureEntryType::UINT:
			{
				Param.InitAsConstants(
					I.UINTConstant.size,
					I.UINTConstant.Register,
					I.UINTConstant.RegisterSpace,
					PipelineDest2ShaderVis(I.UINTConstant.Accessibility));

				slots.push_back(RootSignature::SlotType::UINT);
			}   break;
			case RootSignatureEntryType::DescriptorHeap:
			{
				const auto  HeapIdx		= I.DescriptorHeap.HeapIdx;
				const auto& HeapEntry	= builder.Heaps[HeapIdx];

				desciptorHeaps.push_back(Vector<CD3DX12_DESCRIPTOR_RANGE1>(temp));
				slots.push_back(RootSignature::SlotType::DescriptorSet);

				for (auto& H : HeapEntry.heap.entries)
				{
					D3D12_DESCRIPTOR_RANGE_TYPE RangeType;
					switch (H.type)
					{
					case DescHeapEntryType::CBV:
						RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
						break;
					case DescHeapEntryType::SRV:
						RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
						break;
					case DescHeapEntryType::UAVBuffer:
						RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
						break;
					case DescHeapEntryType::HeapError:
					default:
						FK_ASSERT(false);
						break;
					}

					CD3DX12_DESCRIPTOR_RANGE1 Range;
					Range.Init(
						RangeType,
						H.count, H.registerIdx, heapIdx);

					desciptorHeaps.back().push_back(Range);
				}

				heapIdx++;

				Param.InitAsDescriptorTable(
					(UINT)desciptorHeaps.back().size(),
					desciptorHeaps.back().begin(),
					PipelineDest2ShaderVis(I.DescriptorHeap.Accessibility));
			}	break;
			case RootSignatureEntryType::ConstantBuffer:
			{
				Param.InitAsConstantBufferView
				(I.Direct.Register,
					I.Direct.RegisterSpace,
					D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
					PipelineDest2ShaderVis(I.Direct.Accessibility));
				slots.push_back(RootSignature::SlotType::CBV);
			}	break;
			case RootSignatureEntryType::StructuredBuffer:
			{
				Param.InitAsShaderResourceView(
					I.Direct.Register,
					I.Direct.RegisterSpace,
					D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
					PipelineDest2ShaderVis(I.Direct.Accessibility));
				slots.push_back(RootSignature::SlotType::SRV);
			}	break;
			case RootSignatureEntryType::UnorderedAccess:
			{
				Param.InitAsUnorderedAccessView(
					I.Direct.Register,
					I.Direct.RegisterSpace,
					D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
					PipelineDest2ShaderVis(I.Direct.Accessibility));
				slots.push_back(RootSignature::SlotType::UAV);
			}   break;
			default:
				return nullptr;
				FK_ASSERT(false);
			}
			parameters.push_back(Param);
		}

		ID3DBlob* SignatureBlob		= nullptr;
		ID3DBlob* ErrorBlob			= nullptr;

		CD3DX12_STATIC_SAMPLER_DESC Default(0);
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;

		CD3DX12_STATIC_SAMPLER_DESC	 Samplers[] = {
			CD3DX12_STATIC_SAMPLER_DESC{0, D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP},

			CD3DX12_STATIC_SAMPLER_DESC{1, D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT },
			CD3DX12_STATIC_SAMPLER_DESC{2,  D3D12_FILTER::D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
											0, 1, D3D12_COMPARISON_FUNC_LESS_EQUAL,
											D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK }
		};

		rootSignatureDesc.Init_1_1((UINT)parameters.size(), parameters.begin(), 1, &Default);
		rootSignatureDesc.Desc_1_1.pStaticSamplers	= builder.LocalRoot ? nullptr : Samplers;
		rootSignatureDesc.Desc_1_1.NumStaticSamplers = builder.LocalRoot ? 0 : 3;

		rootSignatureDesc.Desc_1_1.Flags |= builder.AllowIA ?
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT :
			D3D12_ROOT_SIGNATURE_FLAG_NONE;

		rootSignatureDesc.Desc_1_1.Flags |= builder.AllowSO ?
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT :
			D3D12_ROOT_SIGNATURE_FLAG_NONE;

		rootSignatureDesc.Desc_1_1.Flags |= builder.LocalRoot ?
			D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE :
			D3D12_ROOT_SIGNATURE_FLAG_NONE;

		HRESULT HR = D3D12SerializeVersionedRootSignature(
			&rootSignatureDesc,
			&SignatureBlob,		&ErrorBlob);

		if (!SUCCEEDED(HR))
		{
			std::cout << (char*)ErrorBlob->GetBufferPointer() << '\n';
			ErrorBlob->Release();

#ifdef _DEBUG 
			FK_ASSERT(false, "Invalid Root interface Description!");
#endif

			return nullptr;
		}

		ID3D12RootSignature* rootSignature = nullptr;
		if (auto res = pDevice15->CreateRootSignature(0, SignatureBlob->GetBufferPointer(), SignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)); FAILED(res))
		{
			FK_LOG_ERROR("dxRenderSystem: Failed to create root signature!");
			return nullptr;
		}


		std::shared_lock lock{ rootSignatureLock };

		if (auto res = rootSignatures[(uint64_t)rootSignature]; res != nullptr)
			return res->get();

		lock.unlock();

		auto& object			= allocator->allocate_aligned<RootSignature>(rootSignature, builder.Heaps.Copy(*allocator), allocator);
		auto object_ptr			= RootSignature_ptr(&object, RootSignatureDeleter{ allocator });

		std::unique_lock unique{ rootSignatureLock };

		auto rootSignatureEntry = rootSignatures.insert((uint64_t)rootSignature, std::move(object_ptr));
		rootSignatureEntry->get()->slots = std::move(slots);
	    builder.Clear();

		return rootSignatureEntry->get();
	}


	/************************************************************************************************/


	RootSignature* dxRenderSystem::_GetRootSignature(uint64_t hashID) const
	{
		std::shared_lock lock{ const_cast<std::shared_mutex&>(rootSignatureLock) };
		auto sig = rootSignatures[hashID];

		return sig ? sig->get() : nullptr;
	}


	/************************************************************************************************/


	void dxRenderSystem::_ReleaseRootSignature(uint64_t hashID)
	{
		std::unique_lock lock{ rootSignatureLock };

		rootSignatures.remove(hashID);
	}


	/************************************************************************************************/


	IDirectContext& dxRenderSystem::GetDirectCommandList(std::optional<SyncPoint> ticket)
	{
		const uint64_t submissionId		= ticket ? ticket.value().syncCounter : ++directSubmissionCounter;

		while (true)
		{
			const uint64_t completedCounter = directFence->GetCompletedValue();

			uint64_t lowest = -1;

			for (auto& _ : Contexts)
			{
				const size_t idx = contextIdx;
				contextIdx = ++contextIdx % Contexts.size();

				dxDirectContext& context = Contexts[idx];
				if (context.GetCounter() <= completedCounter)
				{
					auto range = CreateDescriptorRange(1024);

					if (!range.has_value())
						FK_LOG_ERROR("Failed to Allocate descriptor range!");

					return context.Reset(range.value(), submissionId, descriptorHeapAllocator.Heap());
				}
				else
					lowest = Min(lowest, context.GetCounter());
			}

			WaitFor(lowest);
		}

		std::unreachable();

		return Contexts[0];
	}


	/************************************************************************************************/


	void dxRenderSystem::SyncUploadTo(SyncPoint sp)
	{
		FK_LOG_9("QUEUE:DIRECT signaling: %I64 : %I64\n", sp.fence, sp.syncCounter);

		copyEngine.copyQueue->Wait(sp.fence.As<ID3D12Fence>(), sp.syncCounter);
	}

	SyncPoint dxRenderSystem::SyncUploadPoint()
	{
		return { copyEngine.counter, copyEngine.fence };
	}

	SyncPoint dxRenderSystem::SyncUploadTicket()
	{

		const uint64_t counter = ++copyEngine.counter;
		copyEngine.copyQueue->Signal(copyEngine.fence, counter);

		FK_LOG_9("QUEUE:DIRECT signaling: %I64 : %I64\n", copyEngine.fence, counter);

		return { counter, copyEngine.fence };
	}

	void dxRenderSystem::SyncDirectTo(SyncPoint sp)
	{
		FK_LOG_9("QUEUE:DIRECT signaling: %I64 : %I64\n", sp.fence, sp.syncCounter);

		GraphicsQueue->Wait(sp.fence.As<ID3D12Fence>(), sp.syncCounter);
	}

	SyncPoint dxRenderSystem::SyncDirectPoint()
	{
		return { directSubmissionCounter, directFence };
	}

	SyncPoint dxRenderSystem::SyncSubmittedDirectPoint()
	{
		return { directSubmittedCounter, directFence };
	}

	SyncPoint dxRenderSystem::SyncDirectTicket()
	{
		auto counter = ++directSubmissionCounter;
		GraphicsQueue->Signal(directFence, counter);

		FK_LOG_9("QUEUE:DIRECT signaling: %I64 : %I64\n", directFence, counter);

		return { counter, directFence };
	}

	void dxRenderSystem::SignalDirect(uint64_t value)
	{
		if (value > directSubmissionCounter)
			DebugBreak();

		FK_LOG_9("QUEUE:DIRECT signaling: %I64\n", value);

		GraphicsQueue->Signal(directFence, value);
		directSubmittedCounter = Max(value, directSubmittedCounter);
	}

	/************************************************************************************************/


	SyncPoint dxRenderSystem::GetSubmissionTicket(uint32_t count)
	{
		auto value = directSubmissionCounter.fetch_add(count) + count;

		return { value, directFence };
	}


	/************************************************************************************************/


	SyncPoint dxRenderSystem::Submit(std::span<IDirectContext*> contexts, std::optional<SyncPoint> syncOptional)
	{
		ProfileFunction();

		if (ImmediateUpload != InvalidHandle)
		{
			SubmitUploadQueues(&ImmediateUpload, 1, {});
			ImmediateUpload = InvalidHandle;

			SyncDirectTo(SyncUploadTicket());
		}

		static_vector<ID3D12CommandList*, 64> cls;

		uint64_t dispatchIdx = 0;

		for (auto context : contexts)
		{
			auto deviceContext = static_cast<dxDirectContext*>(context);
			dispatchIdx = Max(deviceContext->GetCounter(), dispatchIdx);

			cls.push_back(deviceContext->GetCommandList());
			deviceContext->Close();
		}

		if (auto sync = syncOptional.value_or(SyncPoint{}); syncOptional.has_value())
			GraphicsQueue->Wait(sync.fence.As<ID3D12Fence>(), sync.syncCounter);

		try
		{
			GraphicsQueue->ExecuteCommandLists((UINT)cls.size(), cls.begin());
		}
		catch (...)
		{
			DebugBreak();
		}

		if (auto HR = GraphicsQueue->Signal(directFence, dispatchIdx); FAILED(HR))
			FK_LOG_ERROR("Failed to Signal");

		for (auto context : contexts)
			static_cast<dxDirectContext*>(context)->QueueReadBacks();

		directUploadBuffer.last = directUploadBuffer.position;

		FK_LOG_9("QUEUE:DIRECT Submitting. Signaling: %I64 : $I64 \n", directFence, dispatchIdx);

		return { dispatchIdx, directFence };
	}


	/************************************************************************************************/


	void dxRenderSystem::EndFrame()
	{
		static auto dispatchIdx = directSubmissionCounter.load(std::memory_order_relaxed);

		VertexBuffers.LockUntil(dispatchIdx);
		Textures.LockUntil(dispatchIdx);

		ReadBackTable.Update();
	}


	/************************************************************************************************/


	void dxRenderSystem::Signal(SyncPoint syncPoint)
	{
		if (auto HR = GraphicsQueue->Signal(syncPoint.fence.As<ID3D12Fence>(), syncPoint.syncCounter); FAILED(HR))
			FK_LOG_ERROR("Failed to Signal");
	}


	/************************************************************************************************/


	void dxRenderSystem::SubmitUploadQueues(CopyContextHandle* handles, size_t count, std::optional<SyncPoint> syncBefore, std::optional<SyncPoint> syncAfter)
	{
		if(syncBefore)
			copyEngine.Wait(syncBefore.value());

		if(count)
			copyEngine.Submit(handles, handles + count);

		if(syncAfter)
			copyEngine.Signal(syncAfter.value());
	}


	/************************************************************************************************/


	CopyContextHandle dxRenderSystem::OpenUploadQueue()
	{
		return copyEngine.Open();
	}


	/************************************************************************************************/


	CopyContextHandle dxRenderSystem::GetImmediateCopyQueue()
	{
		if (ImmediateUpload == InvalidHandle)
			ImmediateUpload = copyEngine.Open();

		return ImmediateUpload;
	}


	/************************************************************************************************/
	

	/*
	bool CreateInputLayout(dxRenderSystem* RS, VertexBufferView** Buffers, size_t count, Shader* Shader, dxVertexBufferSet* DVB_Out)
	{
		InputDescription Input_Desc;

			// Index Counters
		size_t POS_Buffer_Counter		= 0;
		size_t INDEX_Buffer_Counter		= 0;
		size_t UV_Buffer_Counter		= 0;
		size_t WEIGHT_Buffer_Counter	= 0;
		size_t WINDICES_Buffer_Counter  = 0;
		size_t COLOR_Buffer_Counter		= 0;
		size_t Normal_Buffer_Counter	= 0;
		size_t Tangent_Buffer_Counter	= 0;
		size_t InputSlot				= 0;

		// Try and Guess the Input Layout
		size_t itr = 0;
		for( ; itr < count; itr++ )
		{
			if (Buffers[itr])
			{
				switch (Buffers[itr]->GetBufferType())
				{
				case VERTEXBUFFER_TYPE::POSITION:
				{
					switch (Buffers[itr]->GetBufferFormat())
					{
					case VERTEXBUFFER_FORMAT::R32G32B32:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R32G32B32_FLOAT;
						InputElementDesc.InputSlot            = static_cast<UINT>(++InputSlot);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = (UINT)POS_Buffer_Counter++;
						InputElementDesc.SemanticName         = "POSITION";

						Input_Desc.push_back(InputElementDesc);
					}
					break;
					default:
						FK_ASSERT(0);
					}
					break;
				}
				case VERTEXBUFFER_TYPE::INDEX:
					break;
				case VERTEXBUFFER_TYPE::COLOR:
				{
					switch (Buffers[itr]->GetBufferFormat())
					{
					case VERTEXBUFFER_FORMAT::R32G32B32A32:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R32G32B32A32_FLOAT;
						InputElementDesc.InputSlot            = static_cast<UINT>(InputSlot++);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = (UINT)COLOR_Buffer_Counter++;
						InputElementDesc.SemanticName         = "COLOR";

						Input_Desc.push_back(InputElementDesc);
					}
					break;
					default:
						FK_ASSERT(0);
					}
					break;
				}
				case VERTEXBUFFER_TYPE::NORMAL:
				{
					switch (Buffers[itr]->GetBufferFormat())
					{
					case VERTEXBUFFER_FORMAT::R32G32B32:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R32G32B32_FLOAT;
						InputElementDesc.InputSlot            = static_cast<UINT>(InputSlot++);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = (UINT)Normal_Buffer_Counter++;
						InputElementDesc.SemanticName         = "NORMAL";

						Input_Desc.push_back(InputElementDesc);
					}
					break;
					default:
						FK_ASSERT(0);
					}
					break;
				}
				case VERTEXBUFFER_TYPE::TANGENT:
				{
					switch (Buffers[itr]->GetBufferFormat())
					{
					case VERTEXBUFFER_FORMAT::R32G32B32:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R32G32B32_FLOAT;
						InputElementDesc.InputSlot            = static_cast<UINT>(InputSlot++);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = (UINT)Tangent_Buffer_Counter++;
						InputElementDesc.SemanticName         = "TANGENT";

						Input_Desc.push_back(InputElementDesc);
					}
					break;
					default:
						FK_ASSERT(0);
					}
					break;
				}
				case VERTEXBUFFER_TYPE::UV:
				{
					switch (Buffers[itr]->GetBufferFormat())
					{
					case VERTEXBUFFER_FORMAT::R32G32:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R32G32_FLOAT;
						InputElementDesc.InputSlot            = static_cast<UINT>(InputSlot++);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = (UINT)UV_Buffer_Counter++;
						InputElementDesc.SemanticName         = "TEXCOORD";

						Input_Desc.push_back(InputElementDesc);
					}
					break;
					default:
						FK_ASSERT(0);
					}
					UV_Buffer_Counter++;
					break;
				}
				case VERTEXBUFFER_TYPE::ANIMATION1:
				{
					switch (Buffers[itr]->GetBufferFormat())
					{
					case VERTEXBUFFER_FORMAT::R32G32B32:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R32G32B32_FLOAT;
						InputElementDesc.InputSlot            = static_cast<UINT>(InputSlot++);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = (UINT)WEIGHT_Buffer_Counter++;
						InputElementDesc.SemanticName         = "WEIGHTS";

						Input_Desc.push_back(InputElementDesc);
					}
					break;
					default:
						FK_ASSERT(0);
					}
					WEIGHT_Buffer_Counter++;
					break;
				}
				case VERTEXBUFFER_TYPE::ANIMATION2:
				{
					switch (Buffers[itr]->GetBufferFormat())
					{
					case VERTEXBUFFER_FORMAT::R32G32B32A32:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R32G32B32A32_UINT;
						InputElementDesc.InputSlot            = static_cast<UINT>(InputSlot++);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = (UINT)WINDICES_Buffer_Counter++;
						InputElementDesc.SemanticName         = "WEIGHTINDICES";

						Input_Desc.push_back(InputElementDesc);
					}
					case VERTEXBUFFER_FORMAT::R16G16B16A16:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R16G16B16A16_UINT;
						InputElementDesc.InputSlot            = static_cast<UINT>(InputSlot++);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = (UINT)WINDICES_Buffer_Counter++;
						InputElementDesc.SemanticName         = "WEIGHTINDICES";

						Input_Desc.push_back(InputElementDesc);
					}
					break;
					default:
						FK_ASSERT(0);
					}
					WINDICES_Buffer_Counter++;
					break;
				}
				case VERTEXBUFFER_TYPE::PACKED:
				{
					D3D12_INPUT_ELEMENT_DESC InputElementDesc;
					InputElementDesc.AlignedByteOffset		= 0;
					InputElementDesc.Format					= ::DXGI_FORMAT_R32G32B32_FLOAT;
					InputElementDesc.InputSlot				= static_cast<UINT>(InputSlot++);
					InputElementDesc.InputSlotClass			= D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
					InputElementDesc.InstanceDataStepRate	= 0;

					InputElementDesc.SemanticIndex			= (UINT)POS_Buffer_Counter++;
					InputElementDesc.SemanticName			= "POSITION";
					Input_Desc.push_back(InputElementDesc);
					InputElementDesc.AlignedByteOffset		= 16;
					InputElementDesc.SemanticName			= "NORMAL";
					InputElementDesc.SemanticIndex			= (UINT)Normal_Buffer_Counter++;
					Input_Desc.push_back(InputElementDesc);
					InputElementDesc.AlignedByteOffset		= 32;
					InputElementDesc.SemanticName			= "TANGENT";
					InputElementDesc.SemanticIndex			= (UINT)Tangent_Buffer_Counter++;
					Input_Desc.push_back(InputElementDesc);
					InputElementDesc.AlignedByteOffset		= 48;
					InputElementDesc.SemanticName			= "TEXCOORD";
					InputElementDesc.SemanticIndex			= (UINT)UV_Buffer_Counter++;
					InputElementDesc.Format					= ::DXGI_FORMAT_R32G32_FLOAT;
					Input_Desc.push_back(InputElementDesc);
				}	break;
				case VERTEXBUFFER_TYPE::PACKEDANIMATION:
				{
					D3D12_INPUT_ELEMENT_DESC InputElementDesc;
					InputElementDesc.AlignedByteOffset		= 0;
					InputElementDesc.Format					= ::DXGI_FORMAT_R32G32B32_FLOAT;
					InputElementDesc.InputSlot				= static_cast<UINT>(InputSlot++);
					InputElementDesc.InputSlotClass			= D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
					InputElementDesc.InstanceDataStepRate	= 0;

					InputElementDesc.SemanticIndex			= (UINT)WEIGHT_Buffer_Counter++;
					InputElementDesc.SemanticName			= "WEIGHTS";
					Input_Desc.push_back(InputElementDesc);

					InputElementDesc.AlignedByteOffset		= 12;
					InputElementDesc.SemanticIndex			= (UINT)WINDICES_Buffer_Counter++;
					InputElementDesc.SemanticName			= "WEIGHTINDICES";
					InputElementDesc.Format					= ::DXGI_FORMAT_R16G16B16A16_UINT;
					Input_Desc.push_back(InputElementDesc);
				}	break;
				default:
					break;
				}
			}
		}

		for (size_t I= 0; I < Input_Desc.size(); ++I)
			DVB_Out->MD.InputLayout[I] = Input_Desc[I];
		
		DVB_Out->MD.InputElementCount = Input_Desc.size();

		return true;
	}
	*/
	


	/************************************************************************************************/
	

	void Release( ConstantBuffer& buffer )
	{
		buffer.Release();
	}
	

	/************************************************************************************************/
	

	void Release( Texture2D txt2d )
	{
		if (txt2d)
			txt2d->Release();
	}
	

	/************************************************************************************************/


	DescHeapPOS PushRenderTarget(dxRenderSystem* RS, ResourceHandle target, DescHeapPOS POS, const size_t MIPOffset)
	{
		D3D12_RENDER_TARGET_VIEW_DESC TargetDesc = {};
		const auto dimension            = RS->GetResourceDimension(target);

		switch (dimension)
		{
		case ResourceDimension::Texture2D:
			TargetDesc.Format               = TextureFormat2DXGIFormat(RS->GetTextureFormat(target));
			TargetDesc.Texture2D.MipSlice   = (UINT)MIPOffset;
			TargetDesc.Texture2D.PlaneSlice = 0;
			TargetDesc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
			break;
		case ResourceDimension::TextureCubeMap:
			TargetDesc.Format                           = TextureFormat2DXGIFormat(RS->GetTextureFormat(target));
			TargetDesc.Texture2DArray.FirstArraySlice   = 0;
			TargetDesc.Texture2DArray.MipSlice          = (UINT)MIPOffset;
			TargetDesc.Texture2DArray.PlaneSlice        = 0;
			TargetDesc.Texture2DArray.ArraySize         = 6;
			TargetDesc.ViewDimension                    = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			break;
		}
			

		auto resource = RS->GetDeviceResource(target);
		RS->pDevice->CreateRenderTargetView(resource.As<ID3D12Resource>(), &TargetDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorRTVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushDepthStencil(dxRenderSystem* RS, ResourceHandle target, DescHeapPOS POS)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
		DSVDesc.Format				= RS->GetResourceDeviceFormat(target);
		DSVDesc.Texture2D.MipSlice	= 0;
		DSVDesc.ViewDimension		= D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2D;

		RS->pDevice->CreateDepthStencilView(
			            RS->GetDeviceResource(target).As<ID3D12Resource>(),
			            &DSVDesc,
			            D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorDSVSize, 1);
	}

	DescHeapPOS PushDepthStencilArray(dxRenderSystem* RS, ResourceHandle Target, size_t arrayOffset, size_t MipSlice, DescHeapPOS POS, size_t IN_arraySize)
	{
		const size_t arraySize = IN_arraySize == -1 ? RS->GetTextureArraySize(Target) : IN_arraySize;

		D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
		DSVDesc.Format                          = RS->GetResourceDeviceFormat(Target);
		DSVDesc.Texture2DArray.ArraySize        = (UINT)(arraySize - arrayOffset);
		DSVDesc.Texture2DArray.FirstArraySlice  = (UINT)arrayOffset;
		DSVDesc.Texture2DArray.MipSlice         = (UINT)MipSlice;
		DSVDesc.ViewDimension                   = D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2DARRAY;

		RS->pDevice->CreateDepthStencilView(
			            RS->GetDeviceResource(Target).As<ID3D12Resource>(),
			            &DSVDesc,
			            D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorDSVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushCBToDescHeap(dxRenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, size_t BufferSize, size_t Offset)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC CBV_DESC = {};
		CBV_DESC.BufferLocation = Buffer ? Buffer->GetGPUVirtualAddress() + Offset : 0;
		CBV_DESC.SizeInBytes	= (UINT)BufferSize;
		RS->pDevice->CreateConstantBufferView(&CBV_DESC, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushSRVToDescHeap(dxRenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, size_t ElementCount, size_t Stride, D3D12_BUFFER_SRV_FLAGS Flags, size_t offset)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC desc; {
			desc.Format						= DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
			desc.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			desc.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER;
			desc.Buffer.FirstElement		= offset;
			desc.Buffer.Flags				= D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE;
			desc.Buffer.NumElements			= (UINT)(ElementCount - offset);
			desc.Buffer.StructureByteStride	= (UINT)Stride;
		}

		FK_ASSERT(desc.Buffer.StructureByteStride < 512);
		FK_ASSERT(desc.Buffer.NumElements > 0);

		RS->pDevice->CreateShaderResourceView(Buffer, &desc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushSRVNULLDescHeap(dxRenderSystem* RS, DescHeapPOS POS)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC desc; {
			desc.Format						= DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
			desc.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			desc.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER;
			desc.Buffer.FirstElement		= 0;
			desc.Buffer.Flags				= D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE;
			desc.Buffer.NumElements			= 1;
			desc.Buffer.StructureByteStride	= 1;
		}

		FK_ASSERT(desc.Buffer.StructureByteStride < 512);
		FK_ASSERT(desc.Buffer.NumElements > 0);

		RS->pDevice->CreateShaderResourceView(nullptr, &desc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS Push2DSRVToDescHeap(dxRenderSystem* RS, ID3D12Resource* Buffer, const DescHeapPOS POS, const D3D12_BUFFER_SRV_FLAGS Flags, const DXGI_FORMAT format)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC desc; {
			desc.Format                         = format;
			desc.Shader4ComponentMapping        = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			desc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipLevels		    = 1;
			desc.Texture2D.MostDetailedMip	    = 0;
			desc.Texture2D.PlaneSlice		    = 0;
			desc.Texture2D.ResourceMinLODClamp	= 0;
		}

		RS->pDevice->CreateShaderResourceView(Buffer, &desc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });
		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushTextureToDescHeap(dxRenderSystem* RS, Texture2D tex, DescHeapPOS POS)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc   = {}; {
			ViewDesc.Format                        = tex.Format;
			ViewDesc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			ViewDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
			ViewDesc.Texture2D.MipLevels           = !tex.mipCount ? 1 : tex.mipCount;
			ViewDesc.Texture2D.MostDetailedMip     = 0;
			ViewDesc.Texture2D.PlaneSlice          = 0;
			ViewDesc.Texture2D.ResourceMinLODClamp = 0;
		}

		RS->pDevice->CreateShaderResourceView(tex, &ViewDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushTextureToDescHeap(dxRenderSystem* RS, DXGI_FORMAT format, ResourceHandle handle, DescHeapPOS POS)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {}; 
		viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		viewDesc.Format = format;

		const auto mipCount		= RS->GetTextureMipCount(handle);
		const auto wh			= RS->GetResourceWH(handle);
		const auto arraySize    = RS->GetTextureArraySize(handle);
		auto dimension			= RS->GetResourceDimension(handle);
        
		switch (dimension)
		{
		case ResourceDimension::Buffer:
			viewDesc.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER;
			viewDesc.Buffer.Flags				= D3D12_BUFFER_SRV_FLAG_RAW;
			viewDesc.Buffer.FirstElement		= 0;
			viewDesc.Buffer.NumElements			= wh[0];
			viewDesc.Buffer.StructureByteStride = RS->GetResourceElementSize(handle);
			break;
		case ResourceDimension::Texture1D:
			viewDesc.ViewDimension = (arraySize <= 1) ? D3D12_SRV_DIMENSION_TEXTURE1D : D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
			viewDesc.Texture1D.MipLevels			= mipCount;
			viewDesc.Texture1D.MostDetailedMip		= 0;
			viewDesc.Texture1D.ResourceMinLODClamp	= 0;
			break;
		case ResourceDimension::Texture2D:
			viewDesc.ViewDimension = (arraySize <= 1) ? D3D12_SRV_DIMENSION_TEXTURE2D : D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			viewDesc.Texture2DArray.MipLevels			= Max(mipCount, 1);
			viewDesc.Texture2DArray.MostDetailedMip		= 0;
			viewDesc.Texture2DArray.PlaneSlice			= 0;
			viewDesc.Texture2DArray.ResourceMinLODClamp = 0;
			viewDesc.Texture2DArray.ArraySize			= (UINT)arraySize;
			break;
		case ResourceDimension::Texture3D:
			viewDesc.ViewDimension						= D3D12_SRV_DIMENSION_TEXTURE3D;
			viewDesc.Texture3D.MipLevels			= Max(mipCount, 1);
			viewDesc.Texture3D.MostDetailedMip		= 0;
			viewDesc.Texture3D.ResourceMinLODClamp	= 0;
			break;
		case ResourceDimension::Texture2DArray:
			viewDesc.ViewDimension						= D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			viewDesc.Texture2DArray.MipLevels			= Max(mipCount, 1);
			viewDesc.Texture2DArray.MostDetailedMip		= 0;
			viewDesc.Texture2DArray.PlaneSlice			= 0;
			viewDesc.Texture2DArray.ResourceMinLODClamp = 0;
			viewDesc.Texture2DArray.ArraySize			= (UINT)arraySize;
			break;
		case ResourceDimension::TextureCubeMap:
			viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			viewDesc.Texture2DArray.MipLevels			= Max(mipCount, 1);
			viewDesc.Texture2DArray.MostDetailedMip		= 0;
			viewDesc.Texture2DArray.PlaneSlice			= 0;
			viewDesc.Texture2DArray.ResourceMinLODClamp = 0;
			viewDesc.Texture2DArray.ArraySize			= (UINT)arraySize;
			break;
		case ResourceDimension::AccelerationStructure:
			viewDesc.ViewDimension								= D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
			viewDesc.RaytracingAccelerationStructure.Location	= RS->GetDevicePointer(handle);
			break;
        };

		RS->pDevice->CreateShaderResourceView(
			dimension != ResourceDimension::AccelerationStructure ? RS->GetDeviceResource(handle).As<ID3D12Resource>() : nullptr,
    		&viewDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushTextureToDescHeap(dxRenderSystem* RS, DXGI_FORMAT format, uint32_t highestMipLevel, ResourceHandle handle, DescHeapPOS POS)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc = {}; {
			const auto mipCount     = RS->GetTextureMipCount(handle);
			const auto arraySize    = RS->GetTextureArraySize(handle);

			ViewDesc.Format                             = format;
			ViewDesc.Shader4ComponentMapping            = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			ViewDesc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2D;
			ViewDesc.Texture2DArray.MipLevels           = Max(mipCount - highestMipLevel, 1);
			ViewDesc.Texture2DArray.MostDetailedMip     = highestMipLevel;
			ViewDesc.Texture2DArray.PlaneSlice          = 0;
			ViewDesc.Texture2DArray.ResourceMinLODClamp = 0;
			ViewDesc.Texture2DArray.ArraySize           = (UINT)arraySize;
		}

		auto debug = RS->GetDeviceResource(handle);
		RS->pDevice->CreateShaderResourceView(RS->GetDeviceResource(handle).As<ID3D12Resource>(), &ViewDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 } );

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushTexture3DToDescHeap(dxRenderSystem* RS, DXGI_FORMAT format, uint32_t mipCount, uint32_t highestDetailMip, uint32_t minLODClamp, ResourceHandle handle, DescHeapPOS POS)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc = {}; {
			const auto mipCount     = RS->GetTextureMipCount(handle);
			const auto arraySize    = RS->GetTextureArraySize(handle);

			ViewDesc.Format                             = format;
			ViewDesc.Shader4ComponentMapping            = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			ViewDesc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE3D;
			ViewDesc.Texture3D.MipLevels                = mipCount;
			ViewDesc.Texture3D.MostDetailedMip          = highestDetailMip;
			ViewDesc.Texture3D.ResourceMinLODClamp      = (float)minLODClamp;
		}

		auto debug = RS->GetDeviceResource(handle);
		RS->pDevice->CreateShaderResourceView(RS->GetDeviceResource(handle).As<ID3D12Resource>(), &ViewDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushCubeMapTextureToDescHeap(dxRenderSystem* RS, ResourceHandle resource, DescHeapPOS POS, DeviceFormat format)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc = {}; {
			ViewDesc.Format                          = TextureFormat2DXGIFormat(format);
			ViewDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			ViewDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
			ViewDesc.TextureCube.MipLevels           = Max(RS->GetTextureMipCount(resource), 1);
			ViewDesc.TextureCube.MostDetailedMip     = 0;
			ViewDesc.TextureCube.ResourceMinLODClamp = 0;
		}

		RS->pDevice->CreateShaderResourceView(
			            RS->GetDeviceResource(resource).As<ID3D12Resource>(),
			            &ViewDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushUAV2DToDescHeap(dxRenderSystem* RS, Texture2D tex, DescHeapPOS POS)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		UAVDesc.Format               = tex.Format;
		UAVDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
		UAVDesc.Texture2D.MipSlice   = 0;
		UAVDesc.Texture2D.PlaneSlice = 0;

		RS->pDevice->CreateUnorderedAccessView(tex, nullptr, &UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });
		
		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushUAV2DToDescHeap(dxRenderSystem* RS, Texture2D tex, uint32_t mipLevel, DescHeapPOS POS)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		UAVDesc.Format               = tex.Format;
		UAVDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
		UAVDesc.Texture2D.MipSlice   = mipLevel;
		UAVDesc.Texture2D.PlaneSlice = 0;

		RS->pDevice->CreateUnorderedAccessView(tex, nullptr, &UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });
		
		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	DescHeapPOS PushUAV3DToDescHeap(dxRenderSystem* RS, Texture2D tex, uint32_t width, DescHeapPOS POS)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		UAVDesc.Format                  = tex.Format;
		UAVDesc.ViewDimension           = D3D12_UAV_DIMENSION_TEXTURE3D;
		UAVDesc.Texture3D.MipSlice      = 0;
		UAVDesc.Texture3D.FirstWSlice   = 0;
		UAVDesc.Texture3D.WSize         = width;

		RS->pDevice->CreateUnorderedAccessView(tex, nullptr, &UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });
		
		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushUAV1DToDescHeap(dxRenderSystem* RS, ID3D12Resource* resource, DXGI_FORMAT format, uint mip, DescHeapPOS POS)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		UAVDesc.Format						= format;
		UAVDesc.ViewDimension				= D3D12_UAV_DIMENSION_TEXTURE1D;
		UAVDesc.Texture1D.MipSlice          = 0;

		RS->pDevice->CreateUnorderedAccessView(resource, nullptr, &UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushUAVBufferToDescHeap(dxRenderSystem* RS, UAVBuffer buffer, DescHeapPOS POS)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		UAVDesc.Format						= buffer.format;
		UAVDesc.ViewDimension				= D3D12_UAV_DIMENSION_BUFFER;
		UAVDesc.Buffer.CounterOffsetInBytes = buffer.counterOffset;
		UAVDesc.Buffer.FirstElement			= buffer.offset;
		UAVDesc.Buffer.Flags				= buffer.typeless == true ? D3D12_BUFFER_UAV_FLAGS::D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAGS::D3D12_BUFFER_UAV_FLAG_NONE;
		UAVDesc.Buffer.NumElements			= buffer.elementCount - buffer.offset; // space for counter at beginning
		UAVDesc.Buffer.StructureByteStride	= buffer.stride;

		FK_ASSERT(UAVDesc.Buffer.StructureByteStride < 512);
		FK_ASSERT(UAVDesc.Buffer.NumElements > 0);

		RS->pDevice->CreateUnorderedAccessView(buffer.resource, nullptr, &UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushUAVBufferToDescHeap2(dxRenderSystem* RS, UAVBuffer buffer, ID3D12Resource* counter, DescHeapPOS POS)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		UAVDesc.Format						= buffer.format;
		UAVDesc.ViewDimension				= D3D12_UAV_DIMENSION_BUFFER;
		UAVDesc.Buffer.CounterOffsetInBytes = buffer.counterOffset;
		UAVDesc.Buffer.FirstElement         = buffer.offset;
		UAVDesc.Buffer.Flags				= buffer.typeless == true ? D3D12_BUFFER_UAV_FLAGS::D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAGS::D3D12_BUFFER_UAV_FLAG_NONE;
		UAVDesc.Buffer.NumElements			= buffer.elementCount - buffer.offset;
		UAVDesc.Buffer.StructureByteStride	= buffer.stride;

		FK_ASSERT(UAVDesc.Buffer.StructureByteStride < 512);
		FK_ASSERT(UAVDesc.Buffer.NumElements > 0);

		RS->pDevice->CreateUnorderedAccessView(buffer.resource, counter, &UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	
	/************************************************************************************************/


	DescHeapPOS PushUAVCubeMapToDescHeap(dxRenderSystem* RS, DXGI_FORMAT format, ID3D12Resource* resource, DescHeapPOS POS)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		UAVDesc.Format							= format;
		UAVDesc.ViewDimension					= D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		UAVDesc.Texture2DArray.MipSlice			= 0;
		UAVDesc.Texture2DArray.FirstArraySlice	= 0;
		UAVDesc.Texture2DArray.ArraySize		= 6;
		UAVDesc.Texture2DArray.PlaneSlice		= 0;

		RS->pDevice->CreateUnorderedAccessView(resource, nullptr, &UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ POS.V1 });

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	float2 GetPixelSize(IRenderWindow& Window)
	{
		return float2{ 1.0f, 1.0f } / Window.GetWH();
	}


	/************************************************************************************************/

	// assumes File str should be at most 256 bytes
	ResourceHandle LoadDDSTextureFromFile(char* file, dxRenderSystem* RS, CopyContextHandle handle, iAllocator* MemoryOut)
	{
		Texture2D tex = {};
		wchar_t	wfile[256];
		size_t	ConvertedSize = 0;
		mbstowcs_s(&ConvertedSize, wfile, file, 256);
		auto [Texture, Sucess] = LoadDDSTexture2DFromFile_2(file, MemoryOut, RS, handle);

		FK_ASSERT(Sucess != false, "Failed to Create Texture!");


		return Texture;
	}


	/************************************************************************************************/


	ResourceHandle MoveTextureBufferToVRAM(dxRenderSystem* RS, CopyContextHandle handle, TextureBuffer* buffer, DeviceFormat format)
	{
		auto textureHandle = RS->CreateGPUResource(GPUResourceDesc::ShaderResource(buffer->WH, format));
		RS->UploadTexture(textureHandle, handle, buffer->Buffer, buffer->Size);
		RS->SetDebugName(textureHandle, "MoveTextureBufferToVRAM");

		return textureHandle;
	}


	/************************************************************************************************/


	ResourceHandle MoveTextureBuffersToVRAM(dxRenderSystem* RS, CopyContextHandle handle, TextureBuffer* buffer, size_t resourceCount, DeviceFormat format)
	{
		FK_ASSERT(resourceCount < std::numeric_limits<uint8_t>::max());

		auto texture_desc = GPUResourceDesc::ShaderResource(buffer[0].WH, format, (uint8_t)resourceCount);
		texture_desc.initialLayout = DeviceLayout::Common;

		auto textureHandle = RS->CreateGPUResource(texture_desc);
		RS->UploadTexture(textureHandle, handle, buffer, resourceCount);
		RS->SetDebugName(textureHandle, "MoveTextureBuffersToVRAM");

		return textureHandle;
	}


	/************************************************************************************************/


	ResourceHandle MoveTextureBuffersToVRAM(dxRenderSystem* RS, CopyContextHandle handle, TextureBuffer* buffer, size_t MIPCount, size_t arrayCount, DeviceFormat format)
	{
		FK_ASSERT(MIPCount < std::numeric_limits<uint8_t>::max());

		auto textureDesc			= GPUResourceDesc::ShaderResource(buffer[0].WH, format, (uint8_t)MIPCount, arrayCount);
		textureDesc.initialLayout	= DeviceLayout::Common;

		auto textureHandle = RS->CreateGPUResource(textureDesc);
		RS->UploadTexture(textureHandle, handle, buffer, arrayCount * MIPCount);
		RS->SetDebugName(textureHandle, "MoveTextureBuffersToVRAM");

		return textureHandle;
	}


	/************************************************************************************************/


	MemoryPoolAllocator::MemoryPoolAllocator(size_t IN_heapSize, size_t IN_blockSize, uint32_t IN_flags, iAllocator* IN_allocator) :
		blockCount		{ IN_heapSize / IN_blockSize },
		blockSize		{ IN_blockSize },
		allocator		{ IN_allocator },
		allocations		{ IN_allocator },
		freeRanges		{ IN_allocator },
		heap			{ dxRenderSystem::_GetInstance().CreateHeap(IN_heapSize, IN_flags) },
		flags			{ IN_flags }
	{
		freeRanges.push_back({ 0, (uint32_t)blockCount, Clear });

		estimatedBlocksAvailable = blockCount;
	}


	/************************************************************************************************/


	MemoryPoolAllocator::~MemoryPoolAllocator()
	{
		dxRenderSystem::_GetInstance().ReleaseHeap(heap);
	}


	/************************************************************************************************/


	GPUHeapAllocation MemoryPoolAllocator::GetMemory(const size_t requestBlockCount, const uint64_t frameID, const uint64_t flags)
	{
		ProfileFunction();
		FK_ASSERT(requestBlockCount < std::numeric_limits<uint32_t>::max());

		std::scoped_lock localLock{ m };
		const auto completionCount = dxRenderSystem::_GetInstance().directFence->GetCompletedValue();

		std::sort(
			freeRanges.begin(), freeRanges.end(),
			[](auto& lhs, auto& rhs)
			{
				const auto t1 = uint64_t(lhs.flags & 0x5) << 59 | lhs.frameID << 24 | uint64_t(lhs.blockCount);
				const auto t2 = uint64_t(rhs.flags & 0x5) << 59 | rhs.frameID << 24 | uint64_t(rhs.blockCount);

				return t1 < t2;
			});

		auto _GetMemory = [&]() -> GPUHeapAllocation {
			for (auto& range : freeRanges)
			{
				if (range.offset > blockCount)
					__debugbreak();

				if (range.blockCount >= requestBlockCount)
				{
					if  (range.flags == Clear ||
						(range.flags == Locked && range.frameID <= completionCount && range.frameID < frameID))
						//|| (!(range.flags | AllowReallocation) && range.frameID == frameID))
					{
						GPUHeapAllocation heapAllocation = {
							.offset		= range.offset * blockSize,
							.size		= requestBlockCount * blockSize,
							.overlap	= (range.priorAllocation != InvalidHandle && (range.flags & AllowReallocation) && range.frameID == frameID) ? range.priorAllocation : InvalidHandle
						};

						FK_LOG_9("Allocated Blocks %u - %u from heap %u during %u; Completed count %u; Last Used: %u", range.offset, range.offset + requestBlockCount, heap.INDEX, frameID, completionCount, range.frameID);

						if (range.blockCount > requestBlockCount)
						{
							range.blockCount	-= (uint32_t)(requestBlockCount);
							range.offset		+= (uint32_t)(requestBlockCount);
						}
						else if (range.blockCount == requestBlockCount) {
							freeRanges.remove_unstable(&range);
						}

						estimatedBlocksAvailable -= requestBlockCount;

						return heapAllocation;
					}
				}
			}

			//exit(-10);
			return {};
		};
		auto res = _GetMemory();
		if (res)
			return res;


		Coalesce();

		return _GetMemory();
	}


	void MemoryPoolAllocator::Coalesce()
	{
		const uint64_t completedID = dxRenderSystem::_GetInstance().directFence->GetCompletedValue();

		FK_LOG_9("Coalesce");

		std::sort(freeRanges.begin(), freeRanges.end());

		size_t I = 0; 
		while (I + 1 < freeRanges.size())
		{
			auto& range1 = freeRanges[I];
			auto& range2 = freeRanges[I + 1];

			if ((range1.offset + range1.blockCount) == range2.offset &&
				range1.frameID < completedID && range2.frameID < completedID)
			{
				range1.blockCount  += range2.blockCount;
				range1.frameID		= 0;
				range1.flags		= Clear;

				freeRanges.remove_stable(&range2);
			}
			else
				I++;
		}
	}

	/************************************************************************************************/


	AcquireResult MemoryPoolAllocator::Acquire(GPUResourceDesc desc, bool temporary)
	{
		ProfileFunction();

		const uint64_t frameIdx	= dxRenderSystem::_GetInstance().directSubmissionCounter;
		const uint64_t size		= dxRenderSystem::_GetInstance().GetAllocationSize(desc);

		const size_t	requestedBlockCount	= Max((size / blockSize) + ((size % blockSize == 0) ? 0 : 1), 1);
		auto			allocation			= GetMemory(requestedBlockCount, frameIdx, Clear);

		FK_LOG_9("Allocating Block with size %u", requestedBlockCount);

		if ((allocation.offset + allocation.size) / blockSize > blockCount) {
			FK_LOG_ERROR("MemoryPoolAllocator Allocated a block beyond range!");
			return { InvalidHandle, InvalidHandle };
		}

		if (!allocation)
		{
			if (estimatedBlocksAvailable > requestedBlockCount)
				FK_LOG_INFO("High Fragmentation Detected in Pool %u.", heap.INDEX);

			FK_LOG_INFO("MemoryPoolAllocator Caused Stall!; Pool %u", heap.INDEX);
			dxRenderSystem::_GetInstance().WaitForGPU();

			Coalesce();
			allocation = GetMemory(requestedBlockCount, frameIdx, Clear);

			if (!allocation)
			{
				FK_LOG_ERROR("MemoryPoolAllocator Ran out of memory! Pool %u.", heap.INDEX);
				return { InvalidHandle, InvalidHandle };
			}
		}

		desc.bufferCount    = 1;
		desc.allocationType = ResourceAllocationType::Placed;
		desc.placed.heap    = heap;
		desc.placed.offset  = allocation.offset;


		ResourceHandle resource = dxRenderSystem::_GetInstance().CreateGPUResource(desc);

		if (resource != InvalidHandle) {
			dxRenderSystem::_GetInstance().SetDebugName(resource, "Acquire");

			std::scoped_lock localLock{ m };

			allocations.push_back({
				(uint32_t)(allocation.offset / blockSize),
				(uint32_t)(allocation.size / blockSize),
				frameIdx,
				(uint64_t)(temporary ? Temporary : Allocated),
				resource });
		}
		else
			__debugbreak();

		return { resource, allocation.overlap };
	}


	/************************************************************************************************/


	AcquireDeferredRes MemoryPoolAllocator::AcquireDeferred(GPUResourceDesc desc, bool temporary)
	{
		ProfileFunction();

		const uint64_t frameIdx	= dxRenderSystem::_GetInstance().directSubmissionCounter;
		const uint64_t size		= dxRenderSystem::_GetInstance().GetAllocationSize(desc);

		const size_t requestedBlockCount	= Max((size / blockSize) + ((size % blockSize == 0) ? 0 : 1), 1);
		auto allocation						= GetMemory(requestedBlockCount, frameIdx, Clear);

		FK_LOG_9("Allocating Block with size %u", requestedBlockCount);

		if (allocation.offset / blockSize > blockCount) {
			FK_LOG_ERROR("MemoryPoolAllocator Allocated a block beyond range!");
			return { InvalidHandle, InvalidHandle };
		}

		if (!allocation) {
			if(estimatedBlocksAvailable > requestedBlockCount)
				FK_LOG_INFO("High Fragmentation Detected in Pool %u.", heap.INDEX);

			FK_LOG_INFO("MemoryPoolAllocator Caused Stall!; Pool %u", heap.INDEX);
			dxRenderSystem::_GetInstance().WaitForGPU();

			Coalesce();

			allocation = GetMemory(requestedBlockCount, frameIdx, Clear);
			if (!allocation)
			{
				FK_LOG_ERROR("MemoryPoolAllocator Ran out of memory! Pool %u.", heap.INDEX);
				return { InvalidHandle, InvalidHandle };
			}
		}

		desc.bufferCount    = 1;
		desc.allocationType = ResourceAllocationType::Placed;
		desc.placed.heap    = heap;
		desc.placed.offset  = allocation.offset;


		ResourceHandle resource = dxRenderSystem::_GetInstance().CreateGPUResourceHandle();

		if (resource != InvalidHandle) {
			std::scoped_lock localLock{ m };

			allocations.push_back({
				(uint32_t)(allocation.offset / blockSize),
				(uint32_t)(allocation.size / blockSize),
				frameIdx,
				(uint64_t)(temporary ? Temporary : Allocated),
				resource });
		}
		else
			__debugbreak();

		return { resource, allocation.overlap, allocation.offset, heap };
	}


	/************************************************************************************************/


	AcquireResult MemoryPoolAllocator::Recycle(ResourceHandle resourceToRecycle, GPUResourceDesc desc)
	{
		ProfileFunction();

		std::scoped_lock localLock{ m };

		const uint64_t frameIdx				= dxRenderSystem::_GetInstance().directSubmissionCounter;
		const uint64_t size					= dxRenderSystem::_GetInstance().GetAllocationSize(desc);
		const size_t requestedBlockCount	= (size / blockSize) + (size % blockSize == 0) ? 0 : 1;

		FK_ASSERT(requestedBlockCount < std::numeric_limits<uint32_t>::max());

		if (auto res = std::find_if(std::begin(freeRanges), std::end(freeRanges),
			[&](auto r) { return r.priorAllocation == resourceToRecycle; }); res != std::end(freeRanges))
		{
			if (res->flags | AllowReallocation)
			{
				auto rangeDescriptor = *res;

				if (res->blockCount == requestedBlockCount)
				{
					freeRanges.remove_unstable(res);
				}
				else if(res->blockCount > requestedBlockCount)
				{
					res->offset     += (uint32_t)requestedBlockCount;
					res->blockCount -= (uint32_t)(requestedBlockCount + 1);
				}
				else if (res->blockCount < requestedBlockCount)
					return { InvalidHandle, InvalidHandle }; // ERROR!?

				GPUHeapAllocation heapAllocation = {
						rangeDescriptor.offset * blockSize,
						requestedBlockCount * blockSize,
						(rangeDescriptor.priorAllocation != InvalidHandle &&
							rangeDescriptor.flags | AllowReallocation &&
							rangeDescriptor.frameID == frameIdx) ?
						rangeDescriptor.priorAllocation : InvalidHandle
				};
				
				desc.bufferCount    = 1;
				desc.allocationType = ResourceAllocationType::Placed;
				desc.placed.heap    = heap;
				desc.placed.offset  = heapAllocation.offset;

				ResourceHandle resource = dxRenderSystem::_GetInstance().CreateGPUResource(desc);
				if (resource != InvalidHandle) {
					dxRenderSystem::_GetInstance().SetDebugName(resource, "Acquire");

					allocations.push_back({
						(uint32_t)(rangeDescriptor.offset / blockSize),
						(uint32_t)(requestedBlockCount),
						frameIdx,
						Temporary,
						resource });
				}
				else
					__debugbreak();

				return { resource, resourceToRecycle };

			}
		}

		return { InvalidHandle, InvalidHandle };
	}


	/************************************************************************************************/


	uint32_t MemoryPoolAllocator::Flags() const
	{
		return flags;
	}


	/************************************************************************************************/


	void MemoryPoolAllocator::Release(ResourceHandle handle, uint64_t submissionID, const bool freeResourceImmedate, const bool allowImmediateReuse)
	{
		std::scoped_lock localLock{ m };

		auto res = find(allocations,
			[&](auto& e)
			{
				return (e.resource == handle);
			}
		);

		if (res != std::end(allocations))
		{
#if DEBUG
			if (res->offset > blockCount || res->offset + res->blockCount > blockCount)
				__debugbreak();

			FK_LOG_9("Releasing Blocks %u - %u from heap %u during %u", res->offset, res->offset + res->blockCount, heap.INDEX, submissionID);
#endif

			estimatedBlocksAvailable += res->blockCount;

			freeRanges.push_back(
				{
					res->offset,
					res->blockCount,
					Locked,
					Max(submissionID, res->frameID),
					handle
				});

			allocations.remove_unstable(res);
		}
	}


	/************************************************************************************************/


	void MemoryPoolAllocator::LockRange(uint64_t begin, uint64_t end)
	{
		for (auto& range : freeRanges)
		{
			if (range.frameID >= begin)
				range.frameID = end;
		}
	}


	/************************************************************************************************/
}//	Namespace FlexKit


/**********************************************************************

Copyright (c) 2014-2025 Robert May

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
