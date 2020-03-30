#include "..\pch.h"
#include "..\buildsettings.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\intersection.h"
#include "..\graphicsutilities\MeshUtils.h"
#include "..\graphicsutilities\DDSUtilities.h"


#include "AnimationUtilities.h"
#include "graphics.h"

#ifdef CRND_HEADER_FILE_ONLY
#undef CRND_HEADER_FILE_ONLY
#endif

#include <algorithm>
#include <crn_decomp.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <d3d11sdklayers.h>
#include <d3d11shader.h>
#include <fstream>
#include <memory>
#include <string>
#include <iostream>
#include <Windows.h>
#include <windowsx.h>

#include "..\Dependencies\sdks\DirectXTK\Inc\WICTextureLoader.h"
#include "..\Dependencies\sdks\DirectXTK\inc\DDSTextureLoader.h"

#pragma warning( disable :4267 )


namespace FlexKit
{
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
	// Globals
	HWND			gWindowHandle	= 0;
	HINSTANCE		gInstance		= 0;
	RenderWindow*	gInputWindow	= nullptr;
	uint2			gLastMousePOS;

	char* DEBUGDEVICEID		= "MainContext";
	char* DEBUGSWAPCHAINID	= "MainSwapChain";


	/************************************************************************************************/


	LRESULT CALLBACK WindowProcess( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
	{
		int Param = wParam;
		auto ShiftState = GetAsyncKeyState(VK_LSHIFT) | GetAsyncKeyState(VK_RSHIFT);

		switch( message )
		{
		case WM_SIZE:
		{
			FlexKit::Event ev;
			ev.mType		  = Event::Internal;
			ev.InputSource	  = Event::E_SystemEvent;
			ev.Action		  = Event::InputAction::Resized;
			ev.mData1.mINT[0] = (lParam & 0x000000000000ffff);			// Width
			ev.mData2.mINT[0] = (lParam & 0x00000000ffff0000) >> 16;	// Heigth

			if(gInputWindow)
				gInputWindow->Handler.NotifyEvent(ev);
		}
			break;
		case WM_PAINT:
			break;

		case WM_DESTROY:
			PostQuitMessage( 0 );
			if (gInputWindow)
				gInputWindow->Close = true;
			break;
		case WM_MOUSEMOVE:
		{
			/*
			FlexKit::Event ev;
			ev.mType = Event::Input;
			ev.InputSource = Event::Mouse;
			ev.Action = Event::InputAction::Moved;

			ev.mData1.mINT[0] = GET_X_LPARAM(lParam);
			ev.mData1.mINT[1] = GET_Y_LPARAM(lParam);

			size_t itr = 0;

			ev.mData2.mINT[0] = gLastMousePOS[0] - ev.mData1.mINT[0];
			ev.mData2.mINT[1] = gLastMousePOS[1] - ev.mData1.mINT[1];

			//gLastMousePOS = {ev.mData1.mINT[0], ev.mData1.mINT[1]};

			//gInputWindow->Handler.NotifyEvent(ev);
			*/
		}	break;
		case WM_LBUTTONDOWN:
		{
			Event ev;
			ev.InputSource   = Event::Mouse;
			ev.mType		 = Event::Input;
			ev.Action		 = Event::InputAction::Pressed;

			ev.mData1.mKC[0] = KEYCODES::KC_MOUSELEFT;

			gInputWindow->Handler.NotifyEvent(ev);
		}	break;
		case WM_LBUTTONUP:
		{
			Event ev;
			ev.InputSource	= Event::Mouse;
			ev.mType		= Event::Input;
			ev.Action		= Event::InputAction::Release;

			ev.mData1.mKC[0] = KEYCODES::KC_MOUSELEFT;

			gInputWindow->Handler.NotifyEvent(ev);
		}	break;

		case WM_RBUTTONDOWN:
		{
			Event ev;
			ev.InputSource   = Event::Mouse;
			ev.mType         = Event::Input;
			ev.Action        = Event::InputAction::Pressed;
			ev.mData1.mKC[0] = KEYCODES::KC_MOUSERIGHT;

			gInputWindow->Handler.NotifyEvent(ev);
		}	break;
		case WM_RBUTTONUP:
		{
			Event ev;
			ev.InputSource   = Event::Mouse;
			ev.mType         = Event::Input;
			ev.Action        = Event::InputAction::Release;
			ev.mData1.mKC[0] = KEYCODES::KC_MOUSERIGHT;

			gInputWindow->Handler.NotifyEvent(ev);
		}	break;

		case WM_KEYUP:
		case WM_KEYDOWN:
		{
			Event ev;
			ev.mType       = Event::Input;
			ev.InputSource = Event::Keyboard;
			ev.Action      = message == WM_KEYUP ? Event::InputAction::Release : Event::InputAction::Pressed;

			switch (wParam)
			{
			case VK_F1:
				ev.mData1.mKC[0] = KC_F1;
				break;
			case VK_F2:
				ev.mData1.mKC[0] = KC_F2;
				break;
			case VK_F3:
				ev.mData1.mKC[0] = KC_F3;
				break;
			case VK_F4:
				ev.mData1.mKC[0] = KC_F4;
				break;
			case VK_F5:
				ev.mData1.mKC[0] = KC_F5;
				break;
			case VK_F6:
				ev.mData1.mKC[0] = KC_F6;
				break;
			case VK_F7:
				ev.mData1.mKC[0] = KC_F7;
				break;
			case VK_F8:
				ev.mData1.mKC[0] = KC_F8;
				break;
			case VK_F9:
				ev.mData1.mKC[0] = KC_F9;
				break;
			case VK_F10:
				ev.mData1.mKC[0] = KC_F10;
				break;
			case VK_BACK:
				ev.mData1.mKC[0] = KC_BACKSPACE;
				break;
			case VK_RETURN:
				ev.mData1.mKC[0] = KC_ENTER;
				break;
			case VK_SPACE:
				ev.mData1.mKC[0]   = KC_SPACE;
				break;
			case VK_ESCAPE:
				ev.mData1.mKC [0]  = KC_ESC;
				break;
			case VK_OEM_PLUS:
				if (!ShiftState) {
					ev.mData1.mKC[0] = KC_EQUAL;
					Param = '=';
				} else {
					ev.mData1.mKC[0] = KC_PLUS;
					Param = '+';
				}
				break;
			case VK_OEM_MINUS:
				if (!ShiftState) {
					ev.mData1.mKC[0] = KC_MINUS;
					Param = '-';
				} else {
					ev.mData1.mKC[0] = KC_UNDERSCORE;
					Param = '_';
				}
				break;
			case VK_OEM_7:
				if (!ShiftState) {
					ev.mData1.mKC[0] = KC_SYMBOL;
					Param = '\'';
				}
				else {
					ev.mData1.mKC[0] = KC_SYMBOL;
					Param = '\"';
				}	break;
			case VK_OEM_COMMA:
				if (!ShiftState) {
					ev.mData1.mKC[0] = KC_SYMBOL;
					Param = ',';
				}
				else {
					ev.mData1.mKC[0] = KC_SYMBOL;
					Param = '<';
				}	break;
			case VK_UP:
				ev.mData1.mKC[0] = KC_ARROWUP;
				break;
			case VK_DOWN:
				ev.mData1.mKC[0] = KC_ARROWDOWN;
				break;
			case VK_LEFT:
				ev.mData1.mKC[0] = KC_ARROWLEFT;
				break;
			case VK_RIGHT:
				ev.mData1.mKC[0] = KC_ARROWRIGHT;
				break;
			case VK_OEM_PERIOD:
				if (!ShiftState) {
					ev.mData1.mKC[0] = KC_SYMBOL;
					Param = '.';
				}
				else {
					ev.mData1.mKC[0] = KC_SYMBOL;
					Param = '>';
				}	break;
				// 0 - 9
			case 0x30:
			case 0x31:
			case 0x32:
			case 0x33:
			case 0x34:
			case 0x35:
			case 0x36:
			case 0x37:
			case 0x38:
			case 0x39:
				if (ShiftState) {
					switch (wParam)
					{
					case 0x30:
						ev.mData1.mKC[0] = KC_RIGHTPAREN;
						Param = ')';
						break;
					case 0x31:
						ev.mData1.mKC[0] = KC_EXCLAMATION;
						Param = '!';
						break;
					case 0x32:
						ev.mData1.mKC[0] = KC_AT;
						Param = '@';
						break;
					case 0x33:
						ev.mData1.mKC[0] = KC_HASH;
						Param = '#';
						break;
					case 0x34:
						ev.mData1.mKC[0] = KC_DOLLER;
						Param = '$';
						break;
					case 0x35:
						ev.mData1.mKC[0] = KC_PERCENT;
						Param = '%';
						break;
					case 0x36:
						ev.mData1.mKC[0] = KC_CHEVRON;
						Param = '^';
						break;
					case 0x37:
						ev.mData1.mKC[0] = KC_AMPERSAND;
						Param = '&';
						break;
					case 0x38:
						ev.mData1.mKC[0] = KC_STAR;
						Param = '*';
						break;
					case 0x39:
						ev.mData1.mKC[0] = KC_LEFTPAREN;
						Param = '(';
						break;
					default:
						break;
					}
				}
				else
				{
					ev.mData1.mKC[0]	= (FlexKit::KEYCODES)(KC_0 + wParam - 0x30);
					ev.mData1.mINT[2]	= wParam - 0x30;
				}
				break;
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
			case 'G':
			case 'H':
			case 'I':
			case 'J':
			case 'K':
			case 'L':
			case 'M':
			case 'N':
			case 'O':
			case 'P':
			case 'Q':
			case 'R':
			case 'S':
			case 'T':
			case 'U':
			case 'V':
			case 'W':
			case 'X':
			case 'Y':
			case 'Z':
				ev.mData1.mKC [0]  = wParam;
				if(!ShiftState)	Param += ('a' - 'A');
				break;
			case VK_OEM_3:
				if (ShiftState) {
					ev.mData1.mKC[0] = KC_TILDA;
				}
				break;
			default:

				break;
			}

			ev.mData2.mINT[0] = Param;
			gInputWindow->Handler.NotifyEvent(ev);
		}
		}
		return DefWindowProc( hWnd, message, wParam, lParam );
	}


	/************************************************************************************************/


	void RegisterWindowClass( HINSTANCE hinst )
	{
		// Register Window Class
		WNDCLASSEX wcex = {0};

		wcex.cbSize			= sizeof( wcex );
		wcex.style			= CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc	= &WindowProcess;
		wcex.cbClsExtra		= 0;
		wcex.cbWndExtra		= 0;
		wcex.hInstance		= hinst;
		wcex.hIcon			= LoadIcon( wcex.hInstance, IDI_APPLICATION );
		wcex.hCursor		= LoadCursor( nullptr, IDC_ARROW );
		wcex.hbrBackground	= (HBRUSH)( COLOR_WINDOW );
		wcex.lpszMenuName	= nullptr;
		wcex.lpszClassName	= L"RENDER_WINDOW";
		wcex.hIconSm		= LoadIcon( wcex.hInstance, IDI_APPLICATION );

		FK_ASSERT( RegisterClassEx( &wcex ) );
	 }


	/************************************************************************************************/


	UAVBuffer::UAVBuffer(const RenderSystem& rs, const UAVResourceHandle handle)
	{
		auto uavLayout	= rs.GetUAVBufferLayout(handle);
		resource		= rs.GetDeviceResource(handle);
		stride			= uavLayout.stride;
		elementCount	= uavLayout.elementCount;
		counterOffset	= 0;
		offset			= 0;
		format			= uavLayout.format;
	}



	/************************************************************************************************/


	ConstantBufferHandle	ConstantBufferTable::CreateConstantBuffer(size_t BufferSize, bool GPUResident)
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

		D3D12_HEAP_PROPERTIES HEAP_Props ={};
		HEAP_Props.CPUPageProperty	     = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type				     = GPUResident ? D3D12_HEAP_TYPE_DEFAULT : D3D12_HEAP_TYPE_UPLOAD;
		HEAP_Props.MemoryPoolPreference  = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask	     = 0;
		HEAP_Props.VisibleNodeMask		 = 0;

		constexpr size_t bufferCount = 3;
		ID3D12Resource*  resources[3];

		D3D12_RESOURCE_STATES InitialState = GPUResident ? 
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON :
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ;

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


		handles[handle] = buffers.push_back(buffer);

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


	size_t ConstantBufferTable::AlignNext(ConstantBufferHandle Handle)
	{
		auto& buffer = buffers[handles[Handle]];

		if (buffer.GPUResident)
			return -1; // Cannot directly push to GPU Resident Memory

		UpdateCurrentBuffer(Handle);

		const uint32_t size		   = buffer.size;
		const uint32_t offset	   = buffer.offset;

		const size_t alignOffset	= 256 - offset % 256;
		const size_t adjustedOffset = (alignOffset == 256) ? 0 : offset;
		const size_t alignedOffset  = offset + adjustedOffset;

		buffer.offset       = alignedOffset;

		return alignedOffset;
	}


	/************************************************************************************************/


    std::optional<size_t> ConstantBufferTable::Push(ConstantBufferHandle Handle, void* _Ptr, size_t pushSize)
	{
		auto& buffer = buffers[handles[Handle]];

		if (buffer.GPUResident)
			return false; // Cannot directly push to GPU Resident Memory

		const char* Debug_mapped_Ptr = (char*)buffer.mapped_ptr;


		UpdateCurrentBuffer(Handle);

		const uint32_t size     = buffer.size;
		const uint32_t offset   = buffer.offset;
		const char*  mapped_Ptr	= (char*)buffer.mapped_ptr;

		if (!mapped_Ptr)
            return {};

		if (size < offset + pushSize)
            return {}; // Buffer To small to accommodate Push

		buffer.offset += pushSize;

        if(!buffer.writeFlag)
		    buffer.writeFlag = true;

		FK_ASSERT(pushSize % 256 == 0); // size requests must be blocks of 256

		if(_Ptr)
			memcpy((void*)(mapped_Ptr + offset), _Ptr, pushSize);

        return { offset };
	}


	/************************************************************************************************/


	ConstantBufferTable::SubAllocation ConstantBufferTable::Reserve(ConstantBufferHandle CB, size_t reserveSize)
	{
		const auto res = Push(CB, nullptr, reserveSize);
		FK_ASSERT(res.has_value());

		const size_t UserIdx	= handles[CB];
		void*		 buffer		= buffers[UserIdx].mapped_ptr;

		return { static_cast<char*>(buffer), res.value_or(0), reserveSize };
	}


	/************************************************************************************************/


	void ConstantBufferTable::LockFor(uint8_t frameCount)
	{
		for (auto& buffer : buffers) 
			buffer.locks[buffer.currentRes] = frameCount;
	}


	/************************************************************************************************/


	void ConstantBufferTable::DecrementLocks()
	{
		for (auto& buffer : buffers)
			for (auto& lock : buffer.locks)
				if(lock) lock--;
	}


	/************************************************************************************************/


	void ConstantBufferTable::ReleaseBuffer(ConstantBufferHandle handle)
	{
		std::scoped_lock lock(criticalSection);

		const size_t UserIdx    = handles[handle];
		auto&  buffer           = buffers[UserIdx];

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


	void ConstantBufferTable::UpdateCurrentBuffer(ConstantBufferHandle Handle)
	{
		const size_t UserIdx	= handles[Handle];
		auto& buffer            = buffers[UserIdx];

		// Map Next Buffer
		if (!buffer.GPUResident)
		{
			char*  mapped_Ptr = nullptr;

			if(buffer.locks[buffer.currentRes])
			{
				D3D12_RANGE range{ 0, buffer.offset };
				buffer.resources[buffer.currentRes]->Unmap(0, &range);

				buffer.currentRes = (buffer.currentRes + 1) % 3;
				FK_ASSERT(buffer.locks[buffer.currentRes] == 0, "Constant buffer lock error! possibly all buffers locked!");

				auto HR = buffer.resources[buffer.currentRes]->Map(0, nullptr, (void**)&mapped_Ptr);
				FK_ASSERT(FAILED(HR), "Failed to map Constant Buffer");
				buffer.mapped_ptr   = mapped_Ptr;

				buffer.offset = 0;
			}
		}

	}


	/************************************************************************************************/


	DescriptorHeap::DescriptorHeap(Context& ctx, const DesciptorHeapLayout<16>& Layout_IN, iAllocator* TempMemory) :
		FillState(TempMemory)
	{
		FK_ASSERT(TempMemory);

		const size_t EntryCount = Layout_IN.size();
		descriptorHeap	= ctx._ReserveSRV(EntryCount);
		Layout			= &Layout_IN;

		for (size_t I = 0; I < EntryCount; I++)
			FillState.push_back(false);
	}


	/************************************************************************************************/


	DescriptorHeap::DescriptorHeap(DescriptorHeap&& rhs)
	{
		descriptorHeap  = rhs.descriptorHeap;
		FillState       = std::move(rhs.FillState);
		Layout          = rhs.Layout;

		rhs.descriptorHeap = DescHeapPOS{ 0u, 0u };
		rhs.Layout = nullptr;
	}


	/************************************************************************************************/


	DescriptorHeap& DescriptorHeap::operator = (DescriptorHeap&& rhs)
	{
		descriptorHeap      = rhs.descriptorHeap;
		FillState           = std::move(rhs.FillState);
		Layout              = rhs.Layout;

		rhs.descriptorHeap = DescHeapPOS{ 0u, 0u };
		rhs.Layout = nullptr;

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeap& DescriptorHeap::Init(Context& ctx, const DesciptorHeapLayout<16>& Layout_IN, iAllocator* TempMemory)
	{
		FK_ASSERT(TempMemory);
		FillState = Vector<bool>(TempMemory);

		const size_t EntryCount = Layout_IN.size();
		descriptorHeap = ctx._ReserveSRV(EntryCount);
		Layout = &Layout_IN;

		for (size_t I = 0; I < EntryCount; I++)
			FillState.push_back(false);

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeap& DescriptorHeap::Init(Context& ctx, const DesciptorHeapLayout<16>& Layout_IN, const size_t reserveCount, iAllocator* TempMemory)
	{
		FK_ASSERT(TempMemory);
		FillState = Vector<bool>(TempMemory);

		const size_t EntryCount = Layout_IN.size() * reserveCount;
		descriptorHeap = ctx._ReserveSRV(EntryCount);
		Layout = &Layout_IN;

		for (size_t I = 0; I < EntryCount; I++)
			FillState.push_back(false);

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeap& DescriptorHeap::Init2(Context& ctx, const DesciptorHeapLayout<16>& Layout_IN, const size_t reserveCount, iAllocator* TempMemory)
	{
		FK_ASSERT(TempMemory);
		FillState = Vector<bool>(TempMemory, reserveCount);

		descriptorHeap = ctx._ReserveSRV(reserveCount);
		Layout = &Layout_IN;

		for (size_t I = 0; I < reserveCount; I++)
			FillState.push_back(false);

		return *this;
	}



	/************************************************************************************************/


	DescriptorHeap& DescriptorHeap::NullFill(Context& ctx, const size_t end)
	{
		auto& Entries = Layout->Entries;
		for (size_t I = 0, Idx = 0; I < FillState.size(); I++)
		{
			auto& e = Entries[I];
			//
			for (size_t II = 0; II < e.Space; II++)
			{
				if (I + II > end)
					return *this;
				if (!FillState[Idx])
				{
					switch (e.Type)
					{
					case DescHeapEntryType::ConstantBuffer:
					{
						auto POS = IncrementHeapPOS(
							descriptorHeap,
							ctx.renderSystem->DescriptorCBVSRVUAVSize,
							Idx);

						PushCBToDescHeap(
							ctx.renderSystem, ctx.renderSystem->NullConstantBuffer.Get(),
							POS, 1024);
					}	break;
					case DescHeapEntryType::ShaderResource:
					{
						auto POS = IncrementHeapPOS(
							descriptorHeap,
							ctx.renderSystem->DescriptorCBVSRVUAVSize,
							Idx);

						PushSRVToDescHeap(
							ctx.renderSystem,
							ctx.renderSystem->GetDeviceResource(ctx.renderSystem->NullSRV1D),
							POS, 0, 16);
					}	break;
					case DescHeapEntryType::UAVBuffer:
					{
						auto POS = IncrementHeapPOS(
							descriptorHeap,
							ctx.renderSystem->DescriptorCBVSRVUAVSize,
							Idx);

						PushUAV2DToDescHeap(
							ctx.renderSystem,
							ctx.renderSystem->GetUAV2DTexture(ctx.renderSystem->NullUAV),
							POS);
					}	break;
					case DescHeapEntryType::HeapError:
					{
						FK_ASSERT(false, "ERROR IN HEAP LAYOUT!");
					}	break;
					default:
						break;
					}
					Idx++;
				}
			}
		}

		return *this;
	}


	/************************************************************************************************/


	bool DescriptorHeap::SetSRV(Context& ctx, size_t idx, ResourceHandle handle)
	{
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx))
			return false;


		FillState[idx] = true;

		PushTextureToDescHeap(
			ctx.renderSystem,
			ctx.renderSystem->Textures[handle],
			IncrementHeapPOS(
					descriptorHeap, 
					ctx.renderSystem->DescriptorCBVSRVUAVSize,
					idx));

		return true;
	}


	/************************************************************************************************/


	bool DescriptorHeap::SetSRVCubemap(Context& ctx, size_t idx, ResourceHandle	handle)
	{
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx))
			return false;

		FillState[idx] = true;

		PushCubeMapTextureToDescHeap(
			ctx.renderSystem,
			ctx.renderSystem->Textures[handle],
			IncrementHeapPOS(
					descriptorHeap, 
					ctx.renderSystem->DescriptorCBVSRVUAVSize, 
					idx));

		return true;
	}


	/************************************************************************************************/


	bool DescriptorHeap::SetSRV(Context& ctx, size_t idx, ResourceHandle handle, DeviceFormat format)
	{
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx))
			return false;

		FillState[idx] = true;

		auto texture    = ctx.renderSystem->Textures[handle];
		texture.Format  = TextureFormat2DXGIFormat(format);

		PushTextureToDescHeap(
			ctx.renderSystem,
			texture,
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx));

		return true;
	}



	/************************************************************************************************/

	bool DescriptorHeap::SetCBV(Context& ctx, size_t idx, ConstantBufferHandle	Handle, size_t offset, size_t bufferSize)
	{
		if (!CheckType(*Layout, DescHeapEntryType::ConstantBuffer, idx))
			return false;

		FillState[idx] = true;

		auto resource = ctx.renderSystem->GetDeviceResource(Handle);
		PushCBToDescHeap(
			ctx.renderSystem,
			resource,
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx),
			(bufferSize / 256) * 256 + 256,
			offset);

		return true;
	}


	/************************************************************************************************/


	bool DescriptorHeap::SetSRV(Context& ctx, size_t idx, UAVTextureHandle handle)
	{
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx))
			return false;

		FillState[idx] = true;

		PushTextureToDescHeap(
			ctx.renderSystem,
			ctx.renderSystem->GetUAV2DTexture(handle),
			IncrementHeapPOS(
					descriptorHeap, 
					ctx.renderSystem->DescriptorCBVSRVUAVSize, 
					idx));

		return true;
	}


	/************************************************************************************************/


	bool DescriptorHeap::SetSRV(Context& ctx, size_t idx, UAVResourceHandle handle)
	{
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx))
			return false;

		FillState[idx] = true;

		auto layout		= ctx.renderSystem->GetUAVBufferLayout(handle);
		auto resource	= ctx.renderSystem->GetDeviceResource(handle);

		PushSRVToDescHeap(
			ctx.renderSystem, 
			resource, 
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx),
			layout.elementCount, 
			layout.stride);

		return true;
	}


	/************************************************************************************************/


	bool DescriptorHeap::SetUAV(Context& ctx, size_t idx, UAVResourceHandle	Handle, size_t offset)
	{
		if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx))
			return false;

		FillState[idx] = true;

        UAVBuffer UAV{ *ctx.renderSystem, Handle };
        UAV.offset = offset;

		PushUAVBufferToDescHeap(
			ctx.renderSystem,
			UAV,
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx));

		return true;
	}


	/************************************************************************************************/


	bool DescriptorHeap::SetUAV(Context& ctx, size_t idx, UAVTextureHandle handle)
	{
		if (!CheckType(*Layout, DescHeapEntryType::UAVBuffer, idx))
			return false;

		FillState[idx] = true;

		Texture2D tex = ctx.renderSystem->GetUAV2DTexture(handle);

		PushUAV2DToDescHeap(
			ctx.renderSystem,
			tex, 
			IncrementHeapPOS(
				descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx));

		return false;
	}


	/************************************************************************************************/


	bool DescriptorHeap::SetStructuredResource(Context& ctx, size_t idx, ResourceHandle handle, size_t stride)
	{
		if (!CheckType(*Layout, DescHeapEntryType::ShaderResource, idx))
			return false;

		FillState[idx] = true;

		auto WH = ctx.renderSystem->GetTextureWH(handle);

		PushSRVToDescHeap(
			ctx.renderSystem,
			ctx.renderSystem->Textures[handle],
			IncrementHeapPOS(descriptorHeap,
				ctx.renderSystem->DescriptorCBVSRVUAVSize,
				idx),
			WH[0],
			stride);

		return true;
	}


	/************************************************************************************************/


	DescriptorHeap	DescriptorHeap::GetHeapOffsetted(size_t offset, Context& ctx) const
	{
		DescriptorHeap subHeap = Clone();
		subHeap.descriptorHeap = IncrementHeapPOS(
										descriptorHeap,
										ctx.renderSystem->DescriptorCBVSRVUAVSize,
										offset);

		return subHeap;
	}


	bool DescriptorHeap::CheckType(const DesciptorHeapLayout<>& layout, DescHeapEntryType type, size_t idx)
	{
		size_t entryIdx = 0;
		for (HeapDescriptor entry : layout.Entries)
		{
			if ((entry.Type == type)	&& 
				(entryIdx <= idx)		&&
				(entryIdx + entry.Space + entry.Count > idx))
				return true;

			entryIdx += entry.Count + entry.Space;
		}

		return false;
	}


	/************************************************************************************************/


	bool RootSignature::Build(RenderSystem* RS, iAllocator* TempMemory)
	{
		if (Signature)
			Release();

		Vector<Vector<CD3DX12_DESCRIPTOR_RANGE>> DesciptorHeaps(TempMemory);
		DesciptorHeaps.reserve(12);

		static_vector<CD3DX12_ROOT_PARAMETER> Parameters;

		for (auto& I : RootEntries)
		{
			CD3DX12_ROOT_PARAMETER Param;

			switch (I.Type)
			{
			case RootSignatureEntryType::DescriptorHeap:
			{
				auto  HeapIdx = I.DescriptorHeap.HeapIdx;
				auto& HeapEntry = Heaps[HeapIdx];

				DesciptorHeaps.push_back(Vector<CD3DX12_DESCRIPTOR_RANGE>(TempMemory));

				for (auto& H : HeapEntry.Heap.Entries)
				{
					D3D12_DESCRIPTOR_RANGE_TYPE RangeType;
					switch (H.Type)
					{
					case DescHeapEntryType::ConstantBuffer:
						RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
						break;
					case DescHeapEntryType::ShaderResource:
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

					CD3DX12_DESCRIPTOR_RANGE Range;
					Range.Init(
						RangeType,
						H.Count, H.Register, H.Space);

					DesciptorHeaps.back().push_back(Range);
				}

				auto temp = DesciptorHeaps.back().size();
				Param.InitAsDescriptorTable(
					DesciptorHeaps.back().size(), 
					DesciptorHeaps.back().begin(), 
					PipelineDest2ShaderVis(I.DescriptorHeap.Accessibility));
			}	break;
			case RootSignatureEntryType::ConstantBuffer:
			{
				Param.InitAsConstantBufferView
				(	I.ConstantBuffer.Register, 
					I.ConstantBuffer.RegisterSpace, 
					PipelineDest2ShaderVis(I.ConstantBuffer.Accessibility));

			}	break;
			case RootSignatureEntryType::StructuredBuffer:
			{
				Param.InitAsShaderResourceView(
					I.ShaderResource.Register,
					I.ShaderResource.RegisterSpace,
					PipelineDest2ShaderVis(I.ConstantBuffer.Accessibility));
			}	break;
			case RootSignatureEntryType::UnorderedAcess:
			{
				Param.InitAsUnorderedAccessView(
					I.ShaderResource.Register,
					I.ShaderResource.RegisterSpace,
					PipelineDest2ShaderVis(I.ConstantBuffer.Accessibility));
			}break;

			default:
				return false;
				FK_ASSERT(false);
			}
			Parameters.push_back(Param);
		}

		ID3DBlob* SignatureBlob = nullptr;
		ID3DBlob* ErrorBlob = nullptr;
		CD3DX12_STATIC_SAMPLER_DESC Default(0);
		CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;

		CD3DX12_STATIC_SAMPLER_DESC	 Samplers[] = {
			CD3DX12_STATIC_SAMPLER_DESC{0, D3D12_FILTER_ANISOTROPIC,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP},

			CD3DX12_STATIC_SAMPLER_DESC{1, D3D12_FILTER::D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR }
		};

		RootSignatureDesc.Init(Parameters.size(), Parameters.begin(), 1, &Default);
		RootSignatureDesc.pStaticSamplers	= Samplers;
		RootSignatureDesc.NumStaticSamplers	= 2;

		RootSignatureDesc.Flags |= AllowIA ? 
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT : 
			D3D12_ROOT_SIGNATURE_FLAG_NONE;
		
		RootSignatureDesc.Flags |= AllowSO ? 
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT :
			D3D12_ROOT_SIGNATURE_FLAG_NONE;


		HRESULT HR = D3D12SerializeRootSignature(
			&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, 
			&SignatureBlob,		&ErrorBlob);
		
		if (!SUCCEEDED(HR))
		{
			std::cout << (char*)ErrorBlob->GetBufferPointer() << '\n';
			ErrorBlob->Release();

#ifdef _DEBUG 
			FK_ASSERT(false, "Invalid Root Signature Description!");
#endif

			return false;
		}

		HR = RS->pDevice->CreateRootSignature(0, 
			SignatureBlob->GetBufferPointer(),
			SignatureBlob->GetBufferSize(), IID_PPV_ARGS(&Signature));

		FK_ASSERT(SUCCEEDED(HR));
		SETDEBUGNAME(Signature, "ShadingRTSig");
		SignatureBlob->Release(); 

		DesciptorHeaps.clear();
		return true;
	}


	/************************************************************************************************/


	Context::Context(
				RenderSystem*	renderSystem_IN,
				iAllocator*		allocator) :
			CurrentRootSignature	{ nullptr			},
			PendingBarriers			{ },
			renderSystem			{ renderSystem_IN	},
			Memory					{ allocator			},
			RenderTargetCount		{ 0					},
			DepthStencilEnabled		{ false				},
			TrackedSOBuffers		{ }
	{
		HRESULT HR;
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapdesc;
		descriptorHeapdesc.Flags            = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		descriptorHeapdesc.NumDescriptors   = 1024 * 64;
		descriptorHeapdesc.NodeMask         = 0;
		descriptorHeapdesc.Type             = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		HR = renderSystem->pDevice->CreateDescriptorHeap(&descriptorHeapdesc, IID_PPV_ARGS(&descHeapSRV));
		FK_ASSERT(HR, "FAILED TO CREATE DESCRIPTOR HEAP");

        D3D12_DESCRIPTOR_HEAP_DESC cpuDescriptorHeapdesc;
        cpuDescriptorHeapdesc.Flags            = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        cpuDescriptorHeapdesc.NumDescriptors   = 1024;
        cpuDescriptorHeapdesc.NodeMask         = 0;
        cpuDescriptorHeapdesc.Type             = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		HR = renderSystem->pDevice->CreateDescriptorHeap(&cpuDescriptorHeapdesc, IID_PPV_ARGS(&descHeapSRVLocal));
		FK_ASSERT(HR, "FAILED TO CREATE DESCRIPTOR HEAP");

		descriptorHeapdesc.Flags             = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		descriptorHeapdesc.NumDescriptors    = 128;
		descriptorHeapdesc.NodeMask          = 0;
		descriptorHeapdesc.Type              = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		HR = renderSystem->pDevice->CreateDescriptorHeap(&descriptorHeapdesc, IID_PPV_ARGS(&descHeapRTV));
		FK_ASSERT(HR, "FAILED TO CREATE DESCRIPTOR HEAP");

		descriptorHeapdesc.Flags             = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		descriptorHeapdesc.NumDescriptors    = 128;
		descriptorHeapdesc.NodeMask          = 0;
		descriptorHeapdesc.Type              = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		HR = renderSystem->pDevice->CreateDescriptorHeap(&descriptorHeapdesc, IID_PPV_ARGS(&descHeapDSV));
		FK_ASSERT(HR, "FAILED TO CREATE DESCRIPTOR HEAP");

		FK_LOG_9("GRAPHICS SRV DESCRIPTOR HEAP CREATED: %u", descHeapSRV);
		FK_LOG_9("GRAPHICS RTV DESCRIPTOR HEAP CREATED: %u", descHeapRTV);
		FK_LOG_9("GRAPHICS DSV DESCRIPTOR HEAP CREATED: %u", descHeapDSV);

		SETDEBUGNAME(descHeapSRV, "RESOURCEHEAP");
		SETDEBUGNAME(descHeapRTV, "GPURESOURCEHEAP");
		SETDEBUGNAME(descHeapDSV, "RENDERTARGETHEAP");

		HR = renderSystem->pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)); FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND ALLOCATOR!");
		HR = renderSystem->pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, __uuidof(ID3D12CommandList), (void**)&DeviceContext);	FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND LIST!");

#if USING(AFTERMATH)
        auto res = GFSDK_Aftermath_DX12_CreateContextHandle(DeviceContext, &AFTERMATH_context);
#endif

		DeviceContext->Close();
	}


	void Context::AddRenderTargetBarrier(ResourceHandle Handle, DeviceResourceState Before, DeviceResourceState New)
	{
		Barrier NewBarrier;
		NewBarrier.OldState				= Before;
		NewBarrier.NewState				= New;
		NewBarrier.Type					= Barrier::BT_RenderTarget;
		NewBarrier.renderTarget			= Handle;

		PendingBarriers.push_back(NewBarrier);
	}


	/************************************************************************************************/


	void Context::Release()
	{
		if(descHeapRTV)
			descHeapRTV->Release();
		if (descHeapSRV)
			descHeapSRV->Release();
		if (descHeapDSV)
			descHeapDSV->Release();
		if(commandAllocator)
			commandAllocator->Release();
		if(DeviceContext)
			DeviceContext->Release();

#if USING(AFTERMATH)
        GFSDK_Aftermath_ReleaseContextHandle(AFTERMATH_context);
#endif

		descHeapDSV         = nullptr;
		descHeapRTV         = nullptr;
		descHeapSRV         = nullptr;
		commandAllocator    = nullptr;
		DeviceContext       = nullptr;
	}


	/************************************************************************************************/


	void Context::AddUAVBarrier(UAVResourceHandle handle, DeviceResourceState priorState, DeviceResourceState desiredState)
	{
		auto res = find(PendingBarriers,
			[&](Barrier& rhs) -> bool
			{
				return
					rhs.Type        == Barrier::BT_UAVBuffer &&
					rhs.UAVBuffer   == handle;
			});

		if(res == PendingBarriers.end())
		{
			Barrier NewBarrier;
			NewBarrier.OldState		= priorState;
			NewBarrier.NewState		= desiredState;
			NewBarrier.Type			= Barrier::BT_UAVBuffer;
			NewBarrier.UAVBuffer	= handle;

			PendingBarriers.push_back(NewBarrier);
		}
		else if (res->OldState == desiredState)
		{  // Barrier no longer needed
			PendingBarriers.remove_unstable(res);
		}
		else
			res->NewState = desiredState; // Fuse
	}

	/************************************************************************************************/


	void Context::AddUAVBarrier(UAVTextureHandle handle, DeviceResourceState priorState, DeviceResourceState desiredState)
	{
		auto res = find(PendingBarriers,
			[&](Barrier& rhs) -> bool
			{
				return
					rhs.Type        == Barrier::BT_UAVTexture &&
					rhs.UAVTexture == handle;
			});

		if(res == PendingBarriers.end())
		{
			Barrier NewBarrier;
			NewBarrier.OldState		= priorState;
			NewBarrier.NewState		= desiredState;
			NewBarrier.Type			= Barrier::BT_UAVTexture;
			NewBarrier.UAVTexture   = handle;

			PendingBarriers.push_back(NewBarrier);
		}
		else if (res->OldState == desiredState)
		{   // Barrier no longer needed
			PendingBarriers.remove_unstable(res);
		}
		else
			res->NewState = desiredState; // Fuse
	}




	/************************************************************************************************/


	void Context::AddPresentBarrier(ResourceHandle Handle, DeviceResourceState Before)
	{
		Barrier NewBarrier;
		NewBarrier.OldState		= Before;
		NewBarrier.NewState		= DeviceResourceState::DRS_Present;
		NewBarrier.Type			= Barrier::BT_RenderTarget;
		NewBarrier.renderTarget	= Handle;

		PendingBarriers.push_back(NewBarrier);
	}


	/************************************************************************************************/


	void Context::AddStreamOutBarrier(SOResourceHandle streamOut, DeviceResourceState Before, DeviceResourceState State)
	{
		auto res = find(PendingBarriers, 
			[&](Barrier& rhs) -> bool
			{
				return
					rhs.Type		== Barrier::BT_StreamOut &&
					rhs.streamOut	== streamOut;
			});

		if (res != PendingBarriers.end()) {
			res->NewState = State;
		}
		else
		{
			Barrier NewBarrier;
			NewBarrier.OldState		= Before;
			NewBarrier.NewState		= State;
			NewBarrier.Type			= Barrier::BT_StreamOut;
			NewBarrier.streamOut	= streamOut;
			PendingBarriers.push_back(NewBarrier);
		}
	}


	/************************************************************************************************/


	void Context::AddShaderResourceBarrier(ResourceHandle resource, DeviceResourceState Before, DeviceResourceState State)
	{
		auto res = find(PendingBarriers, 
			[&](Barrier& rhs) -> bool
			{
				return
					rhs.Type			== Barrier::BT_ShaderResource &&
					rhs.shaderResource	== resource;
			});

		if (res != PendingBarriers.end()) {
			res->NewState = State;
		}
		else
		{
			Barrier NewBarrier;
			NewBarrier.OldState			= Before;
			NewBarrier.NewState			= State;
			NewBarrier.Type				= Barrier::BT_ShaderResource;
			NewBarrier.shaderResource	= resource;
			PendingBarriers.push_back(NewBarrier);
		}
	}


	/************************************************************************************************/


	void Context::AddCopyResourceBarrier(ResourceHandle resource, DeviceResourceState Before, DeviceResourceState State)
	{
		auto res = find(PendingBarriers,
			[&](Barrier& rhs) -> bool
			{
				return
					rhs.Type            == Barrier::BT_ShaderResource &&
					rhs.shaderResource  == resource;
			});

		if (res != PendingBarriers.end()) {
			res->NewState = State;
		}
		else
		{
			Barrier NewBarrier;
			NewBarrier.OldState         = Before;
			NewBarrier.NewState         = State;
			NewBarrier.Type             = Barrier::BT_ShaderResource;
			NewBarrier.shaderResource   = resource;
			PendingBarriers.push_back(NewBarrier);
		}
	}


	/************************************************************************************************/


	void Context::SetRootSignature(RootSignature& RS)
	{
		DeviceContext->SetGraphicsRootSignature(RS);
	}


	/************************************************************************************************/


	void Context::SetComputeRootSignature(RootSignature& RS)
	{
		DeviceContext->SetComputeRootSignature(RS);
	}


	/************************************************************************************************/


	void Context::SetPipelineState(ID3D12PipelineState* PSO)
	{
		FK_ASSERT(PSO);

		//if (CurrentPipelineState == PSO)
		//    return;

		CurrentPipelineState = PSO;
		DeviceContext->SetPipelineState(PSO);
	}


	/************************************************************************************************/


	void Context::SetRenderTargets(const static_vector<ResourceHandle> RTs, bool DepthStecil, ResourceHandle depthStencil, const size_t MIPMapOffset)
	{
		static_vector<D3D12_CPU_DESCRIPTOR_HANDLE> RTV_CPU_HANDLES;

		if (!MIPMapOffset)
		{
			for (auto renderTarget : RTs) {
				auto res = std::find_if(
					renderTargetViews.begin(),
					renderTargetViews.end(),
					[&](RTV_View& view)
					{
						return view.resource == renderTarget;
					});

				if (res != renderTargetViews.end())
				{
					RTV_CPU_HANDLES.push_back(res->CPU_Handle);
				}
				else
				{
					auto view = _ReserveRTV(1);
					PushRenderTarget(renderSystem, renderTarget, view);
					RTV_CPU_HANDLES.push_back(view);
					renderTargetViews.push_back({ renderTarget, view });
				}
			}
		}
		else
		{
			auto view = _ReserveRTV(RTs.size());
			
			for (auto& renderTarget : RTs)
			{
				RTV_CPU_HANDLES.push_back(view);
				view = PushRenderTarget(renderSystem, renderTarget, view, MIPMapOffset);
			}
		}

		auto DSV_CPU_HANDLE = D3D12_CPU_DESCRIPTOR_HANDLE{};

		if(DepthStecil)
		{
			if (auto res = std::find_if(
					depthStencilViews.begin(),
					depthStencilViews.end(),
					[&](RTV_View& view)
					{
						return view.resource == depthStencil;
					});
					res == depthStencilViews.end())
			{
				auto DSV = _ReserveDSV(1);
				PushDepthStencil(renderSystem, depthStencil, DSV);

				DSV_CPU_HANDLE = DSV;
			}
			else
				DSV_CPU_HANDLE = res->CPU_Handle;
		}


		DeviceContext->OMSetRenderTargets(
			RTV_CPU_HANDLES.size(),
			RTV_CPU_HANDLES.begin(),
			DepthStecil,
			DepthStecil ? &DSV_CPU_HANDLE : nullptr);
	}


	/************************************************************************************************/


	void Context::SetViewports(static_vector<D3D12_VIEWPORT, 16> VPs)
	{
		Viewports = VPs;
		DeviceContext->RSSetViewports(VPs.size(), VPs.begin());
	}


	/************************************************************************************************/


	void Context::SetScissorRects(static_vector<D3D12_RECT, 16>	Rects)
	{
		DeviceContext->RSSetScissorRects(Rects.size(), Rects.begin());
	}


	/************************************************************************************************/

	// Assumes setting each to fullscreen
	void Context::SetScissorAndViewports(static_vector<ResourceHandle, 16>	RenderTargets)
	{
		static_vector<D3D12_VIEWPORT, 16>	VPs;
		static_vector<D3D12_RECT, 16>		Rects;

		for (auto RT : RenderTargets)
		{
			auto WH = renderSystem->GetTextureWH(RT);
			VPs.push_back	({ 0, 0,	(FLOAT)WH[0], (FLOAT)WH[1], 0, 1 });
			Rects.push_back	({ 0,0,	(LONG)WH[0], (LONG)WH[1] });
		}

		SetViewports(VPs);
		SetScissorRects(Rects);
	}

	/************************************************************************************************/


	void Context::SetScissorAndViewports2(static_vector<ResourceHandle, 16>	RenderTargets, const size_t MIPMapOffset)
	{
		static_vector<D3D12_VIEWPORT, 16>	VPs;
		static_vector<D3D12_RECT, 16>		Rects;

		for (auto RT : RenderTargets)
		{
			auto WH = renderSystem->GetTextureWH(RT) / std::pow(2, MIPMapOffset);
			VPs.push_back({ 0, 0,	(FLOAT)WH[0], (FLOAT)WH[1], 0, 1 });
			Rects.push_back({ 0,0,	(LONG)WH[0], (LONG)WH[1] });
		}

		SetViewports(VPs);
		SetScissorRects(Rects);
	}


	/************************************************************************************************/


    void Context::QueueReadBack(ReadBackResourceHandle readBack)
    {
        queuedReadBacks.push_back(readBack);
    }


    /************************************************************************************************/


	void Context::SetDepthStencil(ResourceHandle DS)
	{
		if (DS != InvalidHandle_t)
		{
			auto DSV = _ReserveDSV(1);
			PushDepthStencil(renderSystem, DS, DSV);
			DeviceContext->OMSetRenderTargets(
				RenderTargetCount,
				RenderTargetCount ? &RTVPOSCPU : nullptr, true,
				DepthStencilEnabled ? &DSVPOSCPU : nullptr);
		}
		else 
			DepthStencilEnabled = false;
	}


	/************************************************************************************************/


	void Context::SetPrimitiveTopology(EInputTopology Topology)
	{
		D3D12_PRIMITIVE_TOPOLOGY D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_POINTLIST;
		switch (Topology)
		{
		case EIT_LINE:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			break;
		case EIT_TRIANGLELIST:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			break;
		case EIT_TRIANGLE:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			break;
		case EIT_POINT:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
			break;
		case EInputTopology::EIT_PATCH_CP_1:
			D3DTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
		}

		DeviceContext->IASetPrimitiveTopology(D3DTopology);
	}


	/************************************************************************************************/


	void Context::NullGraphicsConstantBufferView(size_t idx)
	{
		DeviceContext->SetGraphicsRootConstantBufferView(idx, renderSystem->NullConstantBuffer.Get()->GetGPUVirtualAddress());
	}


	/************************************************************************************************/


	void Context::SetGraphicsConstantBufferView(size_t idx, const ConstantBufferHandle CB, size_t Offset)
	{
		FK_ASSERT(!(Offset % 256), "Incorrect CB Offset!");

		DeviceContext->SetGraphicsRootConstantBufferView(idx, renderSystem->GetConstantBufferAddress(CB) + Offset);
	}


	/************************************************************************************************/


	void Context::SetGraphicsConstantBufferView(size_t idx, const ConstantBuffer& CB)
	{
		DeviceContext->SetGraphicsRootConstantBufferView(idx, CB.Get()->GetGPUVirtualAddress());
	}


	/************************************************************************************************/


	void Context::SetGraphicsConstantBufferView(size_t idx, const ConstantBufferDataSet& CB)
	{
		DeviceContext->SetGraphicsRootConstantBufferView(idx, renderSystem->GetConstantBufferAddress(CB.Handle()) + CB.Offset());
	}


	/************************************************************************************************/


	void Context::SetGraphicsDescriptorTable(size_t idx, const DescriptorHeap& DH)
	{
		DeviceContext->SetGraphicsRootDescriptorTable(idx, DH);
	}


	/************************************************************************************************/


	void Context::SetGraphicsShaderResourceView(size_t idx, FrameBufferedResource* Resource, size_t Count, size_t ElementSize)
	{
		DeviceContext->SetGraphicsRootShaderResourceView(idx, Resource->Get()->GetGPUVirtualAddress());
	}


	/************************************************************************************************/

	void Context::SetGraphicsShaderResourceView(size_t idx, Texture2D& Texture)
	{
		DeviceContext->SetGraphicsRootShaderResourceView(idx, Texture->GetGPUVirtualAddress());
	}


	/************************************************************************************************/


	void Context::SetComputeDescriptorTable(size_t idx, const DescriptorHeap& DH)
	{
		DeviceContext->SetComputeRootDescriptorTable(idx, DH);
	}


	/************************************************************************************************/


	void Context::SetComputeConstantBufferView(size_t idx, const ConstantBufferHandle CB, size_t offset)
	{
		DeviceContext->SetGraphicsRootConstantBufferView(idx, renderSystem->GetConstantBufferAddress(CB) + offset);
	}


	/************************************************************************************************/


	void Context::SetComputeConstantBufferView(size_t idx, const ConstantBufferDataSet& CB)
	{
		DeviceContext->SetComputeRootConstantBufferView(idx, renderSystem->GetConstantBufferAddress(CB.Handle()) + CB.Offset());
	}


	/************************************************************************************************/


	void Context::SetComputeShaderResourceView(size_t idx, Texture2D&			Texture)
	{
		DeviceContext->SetComputeRootShaderResourceView(idx, Texture->GetGPUVirtualAddress());
	}

	/************************************************************************************************/


	void Context::SetComputeUnorderedAccessView(size_t idx, UAVResourceHandle& UAVResource)
	{
		auto resource = renderSystem->BufferUAVs.GetAsset(UAVResource);
		DeviceContext->SetComputeRootUnorderedAccessView(idx, resource->GetGPUVirtualAddress());
	}


	/************************************************************************************************/


	void Context::BeginQuery(QueryHandle query, size_t idx)
	{
		auto resource	= renderSystem->Queries.GetAsset(query);
		auto queryType	= renderSystem->Queries.GetType(query);
		DeviceContext->BeginQuery(resource, queryType, idx);
	}


	void Context::EndQuery(QueryHandle query, size_t idx)
	{
		auto resource	= renderSystem->Queries.GetAsset(query);
		auto queryType	= renderSystem->Queries.GetType(query);
		DeviceContext->EndQuery(resource, queryType, idx);
	}


	/************************************************************************************************/


	void Context::CopyBufferRegion(
			static_vector<ID3D12Resource*>		sources,
			static_vector<size_t>				sourceOffset,
			static_vector<ID3D12Resource*>		destinations,
			static_vector<size_t>				destinationOffset,
			static_vector<size_t>				copySize,
			static_vector<DeviceResourceState>	currentStates,
			static_vector<DeviceResourceState>	finalStates)
	{
		FK_ASSERT(sources.size() == destinations.size(), "Invalid argument!");


		ID3D12Resource*		prevResource	= nullptr;

		for (size_t itr = 0; itr < sources.size(); ++itr)
		{
			auto resource	= destinations[itr];
			auto state		= currentStates[itr];

			if(prevResource != resource && state != DeviceResourceState::DRS_Write)
				_AddBarrier(resource, state, DeviceResourceState::DRS_Write);

			prevResource	= resource;
		}

		FlushBarriers();

		for (size_t itr = 0; itr < sources.size(); ++itr)
		{
			auto sourceResource			= sources[itr];
			auto destinationResource	= destinations[itr];

			DeviceContext->CopyBufferRegion(
				destinationResource,
				destinationOffset[itr],
				sourceResource,
				sourceOffset[itr],
				copySize[itr]);
		}

		DeviceResourceState prevState	= DeviceResourceState::DRS_ERROR;
		prevResource					= nullptr;

		for (size_t itr = 0; itr < destinations.size(); ++itr)
		{
			auto resource	= destinations[itr];
			auto state		= finalStates[itr];

			if (prevResource != resource && prevState != state && state != DeviceResourceState::DRS_Write)
				_AddBarrier(resource, DeviceResourceState::DRS_Write, state);

			prevResource	= resource;
			prevState		= prevState;
		}
	}


	/************************************************************************************************/


	void Context::ImmediateWrite(
		static_vector<UAVResourceHandle>		handles,
		static_vector<size_t>					value,
		static_vector<DeviceResourceState>		currentStates,
		static_vector<DeviceResourceState>		finalStates)
	{
		FK_ASSERT(handles.size() == currentStates.size(), "Invalid argument!");

		/*
		typedef struct D3D12_WRITEBUFFERIMMEDIATE_PARAMETER
		{
		D3D12_GPU_VIRTUAL_ADDRESS Dest;
		UINT32 Value;
		} 	D3D12_WRITEBUFFERIMMEDIATE_PARAMETER;
		*/

		DeviceResourceState prevState		= DeviceResourceState::DRS_ERROR;
		ID3D12Resource*		prevResource	= nullptr;

		for (size_t itr = 0; itr < handles.size(); ++itr)
		{
			auto resource	= renderSystem->GetDeviceResource(handles[itr]);
			auto state		= currentStates[itr];

			if(prevResource != resource && prevState != state)
				_AddBarrier(resource, state, DeviceResourceState::DRS_Write);

			prevResource	= resource;
			prevState		= prevState;
		}

		FlushBarriers();

		for (size_t itr = 0; itr < handles.size(); ++itr)
		{
			auto resource = renderSystem->GetDeviceResource(handles[itr]);

			D3D12_WRITEBUFFERIMMEDIATE_PARAMETER params[] = {
				{resource->GetGPUVirtualAddress() + 0, 0u },
			};

			D3D12_WRITEBUFFERIMMEDIATE_MODE modes[] = {
				D3D12_WRITEBUFFERIMMEDIATE_MODE_MARKER_OUT
			};

			DeviceContext->WriteBufferImmediate(1, params, nullptr);
		}

		prevState		= DeviceResourceState::DRS_ERROR;
		prevResource	= nullptr;

		for (size_t itr = 0; itr < handles.size(); ++itr)
		{
			auto resource	= renderSystem->GetDeviceResource(handles[itr]);
			auto state		= currentStates[itr];

			if (prevResource != resource && prevState != state)
				_AddBarrier(resource, DeviceResourceState::DRS_Write, state);

			prevResource	= resource;
			prevState		= prevState;
		}
	}


	/************************************************************************************************/


	// Requires SO resources to be in DeviceResourceState::DRS::STREAMOUTCLEAR!
	void Context::ClearSOCounters(static_vector<SOResourceHandle> handles)
	{
		/*
		typedef struct D3D12_WRITEBUFFERIMMEDIATE_PARAMETER
		{
		D3D12_GPU_VIRTUAL_ADDRESS Dest;
		UINT32 Value;
		} 	D3D12_WRITEBUFFERIMMEDIATE_PARAMETER;
		*/

		auto nullSource = renderSystem->NullConstantBuffer.Get();

		static_vector<ID3D12Resource*>		sources;
		static_vector<size_t>				sourceOffset;
		static_vector<ID3D12Resource*>		destinations;
		static_vector<size_t>				destinationOffset;
		static_vector<size_t>				copySize;
		static_vector<DeviceResourceState>	currentSOStates;
		static_vector<DeviceResourceState>	finalStates;

		for (auto& s : handles)
			sources.push_back(nullSource);

		for (auto& s : handles)
			sourceOffset.push_back(0);

		for (auto& s : handles)
			destinations.push_back(renderSystem->GetSOCounterResource(s));

		for (auto& s : handles)
			destinationOffset.push_back(0);

		for (auto& s : destinations)
			copySize.push_back(16);

		for (auto& s : handles)
			currentSOStates.push_back(DeviceResourceState::DRS_Write);

		for (auto& s : handles)
			finalStates.push_back(DeviceResourceState::DRS_Write);


		CopyBufferRegion(
			sources,			// sources
			sourceOffset,		// source offsets
			destinations,		// destinations
			destinationOffset,  // destination offsets
			copySize,			// copy sizes
			currentSOStates,	// source initial state
			finalStates);		// source final	state
	}


	/************************************************************************************************/


	void Context::CopyUInt64(
		static_vector<ID3D12Resource*>			sources,
		static_vector<DeviceResourceState>		sourceState,
		static_vector<size_t>					sourceOffsets,
		static_vector<ID3D12Resource*>			destinations,
		static_vector<DeviceResourceState>		destinationState,
		static_vector<size_t>					destinationOffset)
	{
		FK_ASSERT(sources.size()		== sourceState.size(),			"Invalid argument!");
		FK_ASSERT(sources.size()		== sourceOffsets.size(),		"Invalid argument!");
		FK_ASSERT(destinations.size()	== destinationState.size(),		"Invalid argument!");
		FK_ASSERT(destinations.size()	== destinationOffset.size(),	"Invalid argument!");
		FK_ASSERT(sources.size()		== destinations.size(),			"Invalid argument!");

		/*
		typedef struct D3D12_WRITEBUFFERIMMEDIATE_PARAMETER
		{
		D3D12_GPU_VIRTUAL_ADDRESS Dest;
		UINT32 Value;
		} 	D3D12_WRITEBUFFERIMMEDIATE_PARAMETER;
		*/

		// transition source resources
		for (size_t itr = 0; itr < sources.size(); ++itr) 
		{
			auto resource	= sources[itr];
			auto state		= sourceState[itr];
			_AddBarrier(resource, state, DeviceResourceState::DRS_Read);
		}

		for (size_t itr = 0; itr < sources.size(); ++itr)
		{
			auto resource	= destinations[itr];
			auto state		= destinationState[itr];
			_AddBarrier(resource, state, DeviceResourceState::DRS_Write);
		}

		FlushBarriers();

		for (size_t itr = 0; itr < sources.size(); ++itr)
		{
			DeviceContext->AtomicCopyBufferUINT64(
				destinations[itr],
				destinationOffset[itr], 
				sources[itr], 
				sourceOffsets[itr], 
				0, 
				nullptr, 
				nullptr);
		}

		for (size_t itr = 0; itr < sources.size(); ++itr) 
		{
			auto resource	= sources[itr];
			auto state		= sourceState[itr];
			_AddBarrier(resource, DeviceResourceState::DRS_Read, state);
		}

		for (size_t itr = 0; itr < sources.size(); ++itr)
		{
			auto resource	= destinations[itr];
			auto state		= destinationState[itr];
			_AddBarrier(resource, DeviceResourceState::DRS_Write, state);
		}
	}


	/************************************************************************************************/


	void Context::AddIndexBuffer(TriMesh* Mesh)
	{
		size_t	IBIndex		= Mesh->VertexBuffer.MD.IndexBuffer_Index;
		size_t	IndexCount	= Mesh->IndexCount;

		D3D12_INDEX_BUFFER_VIEW		IndexView;
		IndexView.BufferLocation	= GetBuffer(Mesh, IBIndex)->GetGPUVirtualAddress();
		IndexView.Format			= DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
		IndexView.SizeInBytes		= IndexCount * 4;

		DeviceContext->IASetIndexBuffer(&IndexView);
	}


	/************************************************************************************************/


	void Context::SetIndexBuffer(VertexBufferEntry buffer, DeviceFormat format)
	{
		D3D12_INDEX_BUFFER_VIEW		IndexView;
		IndexView.BufferLocation    = renderSystem->GetVertexBufferAddress(buffer.VertexBuffer) + buffer.Offset;
		IndexView.Format            = TextureFormat2DXGIFormat(format);
		IndexView.SizeInBytes       = renderSystem->GetVertexBufferSize(buffer.VertexBuffer) - buffer.Offset;

		DeviceContext->IASetIndexBuffer(&IndexView);
	}


	/************************************************************************************************/


	void Context::AddVertexBuffers(TriMesh* Mesh, static_vector<VERTEXBUFFER_TYPE, 16> Buffers, VertexBufferList* InstanceBuffers)
	{
		static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;

		for(auto& I : Buffers)
			FK_ASSERT(AddVertexBuffer(I, Mesh, VBViews));

		if (InstanceBuffers)
		{
			for (auto& IB : *InstanceBuffers)
			{
				VBViews.push_back({
					renderSystem->GetVertexBufferAddress(IB.VertexBuffer)		+ IB.Offset,
					(UINT)renderSystem->GetVertexBufferSize(IB.VertexBuffer)	- IB.Offset,
					IB.Stride });
			}
		}

		DeviceContext->IASetVertexBuffers(0, VBViews.size(), VBViews.begin());
	}


	/************************************************************************************************/

	
	void Context::SetVertexBuffers(VertexBufferList& List)
	{
		static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;
		for (auto& VB : List)
		{
			/*
			typedef struct D3D12_VERTEX_BUFFER_VIEW
			{
			D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
			UINT SizeInBytes;
			UINT StrideInBytes;
			} 	D3D12_VERTEX_BUFFER_VIEW;
			*/

			VBViews.push_back({
				renderSystem->GetVertexBufferAddress(VB.VertexBuffer) + VB.Offset,
				(UINT)renderSystem->GetVertexBufferSize(VB.VertexBuffer) - +VB.Offset,
				VB.Stride});
		}

		DeviceContext->IASetVertexBuffers(0, VBViews.size(), VBViews.begin());
	}



	/************************************************************************************************/


	void Context::SetVertexBuffers(VertexBufferList List)
	{
		static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;
		for (auto& VB : List)
		{
			/*
			typedef struct D3D12_VERTEX_BUFFER_VIEW
			{
			D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
			UINT SizeInBytes;
			UINT StrideInBytes;
			} 	D3D12_VERTEX_BUFFER_VIEW;
			*/

			VBViews.push_back({
				renderSystem->GetVertexBufferAddress(VB.VertexBuffer) + VB.Offset,
				(UINT)renderSystem->GetVertexBufferSize(VB.VertexBuffer) - VB.Offset,
				VB.Stride});
		}

		DeviceContext->IASetVertexBuffers(0, VBViews.size(), VBViews.begin());
	}


	/************************************************************************************************/


	void Context::SetVertexBuffers2(static_vector<D3D12_VERTEX_BUFFER_VIEW>	List)
	{
		DeviceContext->IASetVertexBuffers(0, List.size(), List.begin());
	}


	/************************************************************************************************/


	void Context::SetSOTargets(static_vector<D3D12_STREAM_OUTPUT_BUFFER_VIEW, 4> SOViews)
	{
		DeviceContext->SOSetTargets(0, SOViews.size(), SOViews.begin());
	}


	/************************************************************************************************/


	void Context::ClearDepthBuffer(ResourceHandle depthStencil, float ClearDepth)
	{
		UpdateResourceStates();

		auto DSV_CPU_HANDLE = D3D12_CPU_DESCRIPTOR_HANDLE{};

			if (auto res = std::find_if(
				depthStencilViews.begin(),
				depthStencilViews.end(),
				[&](RTV_View& view)
				{
					return view.resource == depthStencil;
				});
				res == depthStencilViews.end())
			{
				auto DSV = _ReserveDSV(1);
				PushDepthStencil(renderSystem, depthStencil, DSV);

				DSV_CPU_HANDLE = DSV;
			}
			else
				DSV_CPU_HANDLE = res->CPU_Handle;


		DeviceContext->ClearDepthStencilView(DSV_CPU_HANDLE, D3D12_CLEAR_FLAG_DEPTH, ClearDepth, 0, 0, nullptr);
		renderSystem->Textures.MarkRTUsed(depthStencil);
	}


	/************************************************************************************************/


	void Context::ClearRenderTarget(ResourceHandle renderTarget, float4 ClearColor)
	{
		UpdateResourceStates();

		D3D12_CPU_DESCRIPTOR_HANDLE RTV_CPU_HANDLES{ 0 };

		auto res = std::find_if(
			renderTargetViews.begin(),
			renderTargetViews.end(),
			[&](RTV_View& view)
			{
				return view.resource == renderTarget;
			});

		if (res != renderTargetViews.end())
		{
			RTV_CPU_HANDLES = res->CPU_Handle;
		}
		else
		{
			auto view = _ReserveRTV(1);
			PushRenderTarget(renderSystem, renderTarget, view);
			RTV_CPU_HANDLES = view;
			renderTargetViews.push_back({ renderTarget, view });
		}

		DeviceContext->ClearRenderTargetView(RTV_CPU_HANDLES, ClearColor, 0, nullptr);

		renderSystem->Textures.MarkRTUsed(renderTarget);
	}


    /************************************************************************************************/


    void Context::ClearUAVTexture(UAVTextureHandle UAV, float4 clearColor)
    {
        auto view       = _ReserveSRVLocal(1);
        auto resource   = renderSystem->GetDeviceResource(UAV);

        Texture2D tex = renderSystem->GetUAV2DTexture(UAV);

        PushUAV2DToDescHeap(
            renderSystem,
            tex,
            view);

        FlushBarriers();

        DeviceContext->ClearUnorderedAccessViewFloat(view, view, resource, clearColor, 0, nullptr);
    }

    /************************************************************************************************/


    void Context::ClearUAVTexture(UAVTextureHandle UAV, uint4 clearColor)
    {
        auto view = _ReserveSRVLocal(1);
        auto resource = renderSystem->GetDeviceResource(UAV);

        Texture2D tex = renderSystem->GetUAV2DTexture(UAV);

        PushUAV2DToDescHeap(
            renderSystem,
            tex,
            view);

        auto CPUHandle = view.Get<0>();
        auto GPUHandle = view.Get<1>();
        DeviceContext->ClearUnorderedAccessViewUint(GPUHandle, CPUHandle, resource, (UINT*)&clearColor, 0, nullptr);
    }


	/************************************************************************************************/


	void Context::ResolveQuery(QueryHandle query, size_t begin, size_t end, UAVResourceHandle destination, size_t destOffset)
	{
		auto res			= renderSystem->GetDeviceResource(destination);
		auto type			= renderSystem->Queries.GetType(query);
		auto queryResource	= renderSystem->Queries.GetAsset(query);

		UpdateResourceStates();
		DeviceContext->ResolveQueryData(queryResource, type, begin, end - begin, res, destOffset);
	}


	/************************************************************************************************/


	void Context::ExecuteIndirect(UAVResourceHandle args, const IndirectLayout& layout, size_t argumentBufferOffset, size_t executionCount)
	{
		UpdateResourceStates();
		DeviceContext->ExecuteIndirect(
			layout.signature, 
			min(layout.entries.size(), executionCount), 
			renderSystem->GetDeviceResource(args),
			argumentBufferOffset,
			nullptr, 
			0);
	}


	void Context::Draw(size_t VertexCount, size_t BaseVertex)
	{
		UpdateResourceStates();
		DeviceContext->DrawInstanced(VertexCount, 1, BaseVertex, 0);
	}


	void Context::DrawIndexed(size_t IndexCount, size_t IndexOffet, size_t BaseVertex)
	{
		UpdateResourceStates();
		DeviceContext->DrawIndexedInstanced(IndexCount, 1, IndexOffet, BaseVertex, 0);
	}


	void Context::DrawIndexedInstanced(
		size_t IndexCount, size_t IndexOffet, 
		size_t BaseVertex, size_t InstanceCount, 
		size_t InstanceOffset)
	{
		UpdateResourceStates();
		DeviceContext->DrawIndexedInstanced(
			IndexCount, InstanceCount, 
			IndexOffet, BaseVertex, 
			InstanceOffset);
	}


	/************************************************************************************************/


	void Context::Dispatch(uint3 xyz)
	{
		UpdateResourceStates();
		DeviceContext->Dispatch((UINT)xyz[0], (UINT)xyz[1], (UINT)xyz[2]);
	}


	/************************************************************************************************/


	void Context::FlushBarriers()
	{
		UpdateResourceStates();
	}

	/************************************************************************************************/


	void Context::SetPredicate(bool Enabled, QueryHandle Handle, size_t Offset)
	{
		if (Enabled)
			DeviceContext->SetPredication(
				reinterpret_cast<ID3D12Resource*>(renderSystem->_GetQueryResource(Handle)),
				Offset, 
				D3D12_PREDICATION_OP::D3D12_PREDICATION_OP_NOT_EQUAL_ZERO);
		else
			DeviceContext->SetPredication(nullptr, 0, D3D12_PREDICATION_OP::D3D12_PREDICATION_OP_EQUAL_ZERO);
	}


	/************************************************************************************************/


	void Context::CopyBuffer(const UploadSegment src, const UAVResourceHandle destination)
	{
		const auto  destinationResource		= renderSystem->GetDeviceResource(destination);

		UpdateResourceStates();

		DeviceContext->CopyBufferRegion(destinationResource, 0, src.resource, src.offset, src.uploadSize);
	}


	/************************************************************************************************/


	void Context::CopyBuffer(const UploadSegment src, const size_t uploadSize, const UAVResourceHandle destination)
	{
		const auto destinationResource	= renderSystem->GetDeviceResource(destination);

		UpdateResourceStates();

		DeviceContext->CopyBufferRegion(destinationResource, 0, src.resource, src.offset, uploadSize);
	}


	/************************************************************************************************/


	void Context::CopyBuffer(const UploadSegment src, const ResourceHandle destination)
	{
		const auto destinationResource	= renderSystem->GetDeviceResource(destination);
		const auto sourceResource       = src.resource;

		UpdateResourceStates();

		DeviceContext->CopyBufferRegion(destinationResource, 0, sourceResource, src.offset, src.uploadSize);
	}


	/************************************************************************************************/


	void Context::CopyTexture2D(const UploadSegment src, const ResourceHandle destination, const uint2 BufferSize)
	{
		const auto destinationResource		= renderSystem->GetDeviceResource(destination);
		const auto WH						= renderSystem->GetTextureWH(destination);
		const auto format					= renderSystem->GetTextureDeviceFormat(destination);
		const auto texelSize				= renderSystem->GetTextureElementSize(destination);

		D3D12_TEXTURE_COPY_LOCATION destLocation{};
		destLocation.pResource			= destinationResource;
		destLocation.SubresourceIndex	= 0;
		destLocation.Type				= D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;


		D3D12_TEXTURE_COPY_LOCATION srcLocation{};
		srcLocation.pResource							= src.resource;
		srcLocation.Type								= D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcLocation.PlacedFootprint.Offset				= src.offset;
		srcLocation.PlacedFootprint.Footprint.Depth		= 1;
		srcLocation.PlacedFootprint.Footprint.Format	= format;
		srcLocation.PlacedFootprint.Footprint.Height	= WH[1];
		srcLocation.PlacedFootprint.Footprint.Width		= WH[0];
		srcLocation.PlacedFootprint.Footprint.RowPitch	= BufferSize[0] * texelSize;

		UpdateResourceStates();
		DeviceContext->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, nullptr);
	}


	/************************************************************************************************/


	void Context::Clear()
	{
		PendingBarriers.clear();
		RenderTargets.clear();
		Viewports.clear();
		DesciptorHeaps.clear();
		VBViews.clear();

		TrackedSOBuffers.clear();
		PendingBarriers.clear();

		CurrentPipelineState = nullptr;

        DeviceContext->ClearState(nullptr);
	}


	/************************************************************************************************/


	void Context::SetRTRead(ResourceHandle Handle)
	{
		FK_ASSERT(0);
	}


	/************************************************************************************************/


	void Context::SetRTWrite(ResourceHandle Handle)
	{
		FK_ASSERT(0);
	}


	/************************************************************************************************/


	void Context::SetRTFree(ResourceHandle Handle)
	{
		FK_ASSERT(0);
	}


	/************************************************************************************************/


	void Context::Close(const size_t IN_counter)
	{
        counter = IN_counter;
		DeviceContext->Close();
	}


	/************************************************************************************************/


    Context& Context::Reset()
	{
		commandAllocator->Reset();
		DeviceContext->Reset(commandAllocator, nullptr);

		_ResetDSV();
		_ResetRTV();
		_ResetSRV();

		ID3D12DescriptorHeap* heaps[]{
			descHeapSRV };

		DeviceContext->SetDescriptorHeaps(sizeof(heaps) / sizeof(heaps[0]), heaps);

        return *this;
	}


	/************************************************************************************************/


	void Context::SetUAVRead() 
	{
		FK_ASSERT(0);
	}


	/************************************************************************************************/


	void Context::SetUAVWrite() 
	{
		FK_ASSERT(0);
	}


	/************************************************************************************************/


	void Context::SetUAVFree() 
	{
		FK_ASSERT(0);
	}


	/************************************************************************************************/


	void Context::_AddBarrier(ID3D12Resource* resource, DeviceResourceState currentState, DeviceResourceState newState)
	{
		Barrier barrier;
		barrier.Type		= Barrier::BT_Generic;
		barrier.resource	= resource;
		barrier.OldState	= currentState;
		barrier.NewState	= newState;

		PendingBarriers.push_back(barrier);
	}


	/************************************************************************************************/


	void Context::UpdateResourceStates()
	{
		if (!PendingBarriers.size())
			return;

		static_vector<D3D12_RESOURCE_BARRIER> Barriers;

		for (auto& B : PendingBarriers)
		{
			switch(B.Type)
			{
				case Barrier::BT_RenderTarget:
				{
					auto handle			= B.renderTarget;
					auto resource		= renderSystem->GetDeviceResource(handle);
					auto currentState	= DRS2D3DState(B.OldState);
					auto newState		= DRS2D3DState(B.NewState);

					/*
					#ifdef _DEBUG
						std::cout << "Transitioning Resource: " << Resource
							<< " From State: " << CurrentState << " To State: "
							<< NewState << "\n";
					#endif
					*/

					if (B.OldState != B.NewState)
						Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
							resource, currentState, newState));
				}	break;
				case Barrier::BT_UAVBuffer:
				{
					auto handle			= B.UAVBuffer;
					auto resource		= renderSystem->GetDeviceResource(handle);
					auto currentState	= DRS2D3DState(B.OldState);
					auto newState		= DRS2D3DState(B.NewState);

					/*
					#ifdef _DEBUG
						std::cout << "Transitioning Resource: " << Resource 
							<< " From State: " << CurrentState << " To State: " 
							<< NewState << "\n";
					#endif
					*/

					if (B.OldState != B.NewState)
						Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
							resource, currentState, newState));
				}	break;
				case Barrier::BT_UAVTexture:
				{
					auto handle			= B.UAVTexture;
					auto resource		= renderSystem->GetDeviceResource(handle);
					auto currentState	= DRS2D3DState(B.OldState);
					auto newState		= DRS2D3DState(B.NewState);

					/*
					#ifdef _DEBUG
						std::cout << "Transitioning Resource: " << Resource 
							<< " From State: " << CurrentState << " To State: " 
							<< NewState << "\n";
					#endif
					*/

					if (B.OldState != B.NewState)
						Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
							resource, currentState, newState));
				}	break;
				case Barrier::BT_ConstantBuffer:
				case Barrier::BT_VertexBuffer:
					break;
				case Barrier::BT_StreamOut:
				{

					if (DeviceResourceState::DRS_STREAMOUTCLEAR == B.OldState || DeviceResourceState::DRS_STREAMOUTCLEAR == B.NewState) {
						auto handle				= B.streamOut;
						auto SOresource			= renderSystem->GetDeviceResource(handle);
						auto resource			= renderSystem->GetSOCounterResource(handle);
						auto currentState		= DRS2D3DState((B.OldState == DeviceResourceState::DRS_VERTEXBUFFER) ? DeviceResourceState::DRS_STREAMOUT : B.OldState);
						auto newState			= DRS2D3DState((B.NewState == DeviceResourceState::DRS_VERTEXBUFFER) ? DeviceResourceState::DRS_STREAMOUT : B.NewState);

						auto currentSOState		= DRS2D3DState((B.OldState == DeviceResourceState::DRS_STREAMOUTCLEAR) ? DeviceResourceState::DRS_STREAMOUT : B.OldState);
						auto newSOState			= DRS2D3DState((B.NewState == DeviceResourceState::DRS_STREAMOUTCLEAR) ? DeviceResourceState::DRS_STREAMOUT : B.NewState);

						if (B.OldState != B.NewState)
							Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(resource,	currentState, newState));

						if(currentSOState != newSOState)
							Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(SOresource, currentSOState, newSOState));
					}
					else
					{
						auto handle				= B.streamOut;
						auto resource			= renderSystem->GetDeviceResource(handle);
						auto currentState		= DRS2D3DState(B.OldState);
						auto newState			= DRS2D3DState(B.NewState);

						if (B.OldState != B.NewState)
							Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(resource, currentState, newState));
					}
				}	break;
				case Barrier::BT_Generic:
				{
					auto resource		= B.resource;
					auto currentState	= DRS2D3DState(B.OldState);
					auto newState		= DRS2D3DState(B.NewState);

					/*
					#ifdef _DEBUG
						std::cout << "Transitioning Resource: " << Resource
							<< " From State: " << CurrentState << " To State: "
							<< NewState << "\n";
					#endif
					*/

					if (B.OldState != B.NewState)
						Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
							resource, currentState, newState));
				}	break;
				case Barrier::BT_ShaderResource:
				{
					auto resource		= renderSystem->Textures.GetAsset(B.shaderResource);
					auto currentState	= DRS2D3DState(B.OldState);
					auto newState		= DRS2D3DState(B.NewState);

					if (B.OldState != B.NewState)
						Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
							resource, currentState, newState));
				}	break;
				default:
					FK_ASSERT(0);
			};
		}


		for (auto barrier : renderSystem->PendingBarriers) {
			Barriers.push_back(barrier);
		}

		renderSystem->PendingBarriers.clear();

		if (Barriers.size())
			DeviceContext->ResourceBarrier(Barriers.size(), Barriers.begin());

		Barriers.clear();
		PendingBarriers.clear();
	}


	/************************************************************************************************/


    void Context::_QueueReadBacks()
    {
        for (const auto readBackHandle : queuedReadBacks)
        {
            auto& readBack  = renderSystem->ReadBackTable[readBackHandle];
            auto fence      = renderSystem->ReadBackTable._GetReadBackFence();

            const uint64_t counter = ++renderSystem->copyEngine.counter;

            auto HR     = fence->SetEventOnCompletion(counter, readBack.event); FK_ASSERT(SUCCEEDED(HR));
            renderSystem->GraphicsQueue->Signal(fence, counter);

            readBack.queueUntil = counter;
            readBack.queued     = true;
        }

        queuedReadBacks.clear();
    }


    /************************************************************************************************/


	DescHeapPOS Context::_ReserveDSV(size_t count)
	{
		auto currentCPU = DSV_CPU;
		auto currentGPU = DSV_GPU;
		DSV_CPU.ptr = DSV_CPU.ptr + renderSystem->DescriptorDSVSize * count;
		DSV_GPU.ptr = DSV_GPU.ptr + renderSystem->DescriptorDSVSize * count;

		return { currentCPU, currentGPU };
	}


	/************************************************************************************************/


	DescHeapPOS Context::_ReserveSRV(size_t count)
	{
		auto currentCPU = SRV_CPU;
		auto currentGPU = SRV_GPU;
		SRV_CPU.ptr = SRV_CPU.ptr + renderSystem->DescriptorCBVSRVUAVSize * count;
		SRV_GPU.ptr = SRV_GPU.ptr + renderSystem->DescriptorCBVSRVUAVSize * count;

		return { currentCPU, currentGPU };
	}


    /************************************************************************************************/


    DescHeapPOS Context::_ReserveSRVLocal(size_t count)
    {
        auto currentCPU = SRV_LOCAL_CPU;
		auto currentGPU = SRV_LOCAL_GPU;
		SRV_LOCAL_CPU.ptr = SRV_CPU.ptr + renderSystem->DescriptorCBVSRVUAVSize * count;
		SRV_LOCAL_GPU.ptr = SRV_GPU.ptr + renderSystem->DescriptorCBVSRVUAVSize * count;

		return { currentCPU, currentGPU };
    }


	/************************************************************************************************/


	DescHeapPOS Context::_ReserveRTV(size_t count)
	{
		auto currentCPU = RTV_CPU;
		auto currentGPU = RTV_GPU;
		RTV_CPU.ptr = RTV_CPU.ptr + renderSystem->DescriptorRTVSize * count;
		RTV_GPU.ptr = RTV_GPU.ptr + renderSystem->DescriptorRTVSize * count;

		return { currentCPU, currentGPU };
	}


	/************************************************************************************************/


	void Context::_ResetRTV()
	{
		RTV_CPU = descHeapRTV->GetCPUDescriptorHandleForHeapStart();
		RTV_GPU = descHeapRTV->GetGPUDescriptorHandleForHeapStart();

		renderTargetViews.clear();
	}


	/************************************************************************************************/


	void Context::_ResetDSV()
	{
		DSV_CPU = descHeapDSV->GetCPUDescriptorHandleForHeapStart();
		DSV_GPU = descHeapDSV->GetGPUDescriptorHandleForHeapStart();

		depthStencilViews.clear();
	}


	/************************************************************************************************/


	void Context::_ResetSRV()
	{
		SRV_CPU = descHeapSRV->GetCPUDescriptorHandleForHeapStart();
		SRV_GPU = descHeapSRV->GetGPUDescriptorHandleForHeapStart();

        SRV_LOCAL_CPU = descHeapSRVLocal->GetCPUDescriptorHandleForHeapStart();
        SRV_LOCAL_GPU = descHeapSRVLocal->GetGPUDescriptorHandleForHeapStart();
	}


	/************************************************************************************************/


	void RenderSystem::RootSigLibrary::Initiate(RenderSystem* RS, iAllocator* TempMemory)
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

		CD3DX12_STATIC_SAMPLER_DESC	 Samplers[] = {
			CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT, 
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_BORDER, 
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_BORDER,
											D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_BORDER),

			CD3DX12_STATIC_SAMPLER_DESC{1, D3D12_FILTER::D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR },
			CD3DX12_STATIC_SAMPLER_DESC{2, D3D12_FILTER::D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT},
		};

		{
			CD3DX12_DESCRIPTOR_RANGE ranges[2];
			ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);
			ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 4, 4);

			RS->Library.RS6CBVs4SRVs.AllowIA = true;
			DesciptorHeapLayout<2> DescriptorHeap;
			DescriptorHeap.SetParameterAsSRV(0, 0, 4);
			DescriptorHeap.SetParameterAsCBV(1, 6, 4);
			FK_ASSERT(DescriptorHeap.Check());

			RS->Library.RS6CBVs4SRVs.SetParameterAsDescriptorTable(0, DescriptorHeap, -1);
			RS->Library.RS6CBVs4SRVs.SetParameterAsCBV(1, 0, 0, PIPELINE_DEST_ALL);
			RS->Library.RS6CBVs4SRVs.SetParameterAsCBV(2, 1, 0, PIPELINE_DEST_ALL);
			RS->Library.RS6CBVs4SRVs.SetParameterAsCBV(3, 2, 0, PIPELINE_DEST_ALL);
			RS->Library.RS6CBVs4SRVs.SetParameterAsCBV(4, 3, 0, PIPELINE_DEST_ALL);
			RS->Library.RS6CBVs4SRVs.SetParameterAsCBV(5, 4, 0, PIPELINE_DEST_ALL);
			RS->Library.RS6CBVs4SRVs.SetParameterAsCBV(6, 5, 0, PIPELINE_DEST_ALL);
			RS->Library.RS6CBVs4SRVs.Build(RS, TempMemory);
			SETDEBUGNAME(RS->Library.RS6CBVs4SRVs, "RS4CBVs4SRVs");
		}
		{
			RS->Library.RS4CBVs_SO.AllowIA	= true;
			RS->Library.RS4CBVs_SO.AllowSO	= true;
			DesciptorHeapLayout<1> DescriptorHeap;
			DescriptorHeap.SetParameterAsSRV(0, 0, 8);

			RS->Library.RS4CBVs_SO.SetParameterAsCBV				(0, 0, 0, PIPELINE_DEST_ALL);
			RS->Library.RS4CBVs_SO.SetParameterAsCBV				(1, 1, 0, PIPELINE_DEST_ALL);
			RS->Library.RS4CBVs_SO.SetParameterAsCBV				(2, 2, 0, PIPELINE_DEST_ALL);
			RS->Library.RS4CBVs_SO.SetParameterAsDescriptorTable	(3, DescriptorHeap, -1);
			RS->Library.RS4CBVs_SO.SetParameterAsUAV				(4, 0, 0, PIPELINE_DEST_ALL);
			RS->Library.RS4CBVs_SO.Build(RS, TempMemory);

			SETDEBUGNAME(RS->Library.RS4CBVs_SO, "RS4CBVs_SO");
		}
		{
			RS->Library.RS2UAVs4SRVs4CBs.AllowIA = true;
			DesciptorHeapLayout<16> DescriptorHeap;
			DescriptorHeap.SetParameterAsShaderUAV	(0, 0, 4);
			DescriptorHeap.SetParameterAsSRV		(1, 0, 4);
			DescriptorHeap.SetParameterAsCBV		(2, 4, 4);
			FK_ASSERT(DescriptorHeap.Check());

			RS->Library.RS2UAVs4SRVs4CBs.SetParameterAsDescriptorTable(0, DescriptorHeap, -1);
			RS->Library.RS2UAVs4SRVs4CBs.SetParameterAsCBV(1, 0, 3, PIPELINE_DESTINATION::PIPELINE_DEST_ALL);
			RS->Library.RS2UAVs4SRVs4CBs.Build(RS, TempMemory);

			SETDEBUGNAME(RS->Library.RS2UAVs4SRVs4CBs, "RS2UAVs4SRVs4CBs");
		}
		{
			RS->Library.ShadingRTSig.AllowIA = false;

			DesciptorHeapLayout<16> DescriptorHeap;
			DescriptorHeap.SetParameterAsSRV		(0, 0, 8);
			DescriptorHeap.SetParameterAsShaderUAV	(1, 0, 1);
			DescriptorHeap.SetParameterAsCBV		(2, 0, 2);
			FK_ASSERT(DescriptorHeap.Check());

			RS->Library.ShadingRTSig.SetParameterAsDescriptorTable(0, DescriptorHeap, -1);
			RS->Library.ShadingRTSig.Build(RS, TempMemory);

			SETDEBUGNAME(RS->Library.ShadingRTSig, "ShadingRTSig");
		}
		{
			RS->Library.RSDefault.AllowIA = true;

			DesciptorHeapLayout<16> DescriptorHeapSRV;
			DescriptorHeapSRV.SetParameterAsSRV(0, 0, -1);
			FK_ASSERT(DescriptorHeapSRV.Check());

			DesciptorHeapLayout<16> DescriptorHeapUAV;
			DescriptorHeapUAV.SetParameterAsShaderUAV(0, 0, -1);
			FK_ASSERT(DescriptorHeapUAV.Check());

			RS->Library.RSDefault.SetParameterAsCBV				(0, 0, 0, PIPELINE_DESTINATION::PIPELINE_DEST_ALL);
			RS->Library.RSDefault.SetParameterAsCBV				(1, 1, 0, PIPELINE_DESTINATION::PIPELINE_DEST_ALL);
			RS->Library.RSDefault.SetParameterAsCBV				(2, 2, 0, PIPELINE_DESTINATION::PIPELINE_DEST_ALL);
			RS->Library.RSDefault.SetParameterAsDescriptorTable	(3, DescriptorHeapSRV, -1, PIPELINE_DESTINATION::PIPELINE_DEST_ALL);
			RS->Library.RSDefault.SetParameterAsDescriptorTable	(4, DescriptorHeapUAV, -1, PIPELINE_DESTINATION::PIPELINE_DEST_ALL);
			RS->Library.RSDefault.Build(RS, TempMemory);

			SETDEBUGNAME(RS->Library.RSDefault, "RSDefault");
		}
		{
			RS->Library.ComputeSignature.AllowIA = false;
			DesciptorHeapLayout<16> DescriptorHeap;
			DescriptorHeap.SetParameterAsShaderUAV(0, 0, 2, 0);
			DescriptorHeap.SetParameterAsSRV(1, 0, 4, 0);// Allow for indefinte amounts of SRV, UAVs, CBVs
			DescriptorHeap.SetParameterAsCBV(2, 0, 2, 0);
			FK_ASSERT(DescriptorHeap.Check());

			RS->Library.ComputeSignature.SetParameterAsDescriptorTable(0, DescriptorHeap, -1, PIPELINE_DEST_CS);
			RS->Library.ComputeSignature.Build(RS, TempMemory);

			SETDEBUGNAME(RS->Library.ShadingRTSig, "ComputeSignature");
		}
	}


	/************************************************************************************************/


	UploadBuffer::UploadBuffer(ID3D12Device* pDevice)  :
		parentDevice{ pDevice       },
		Size        { MEGABYTE * 16 }
	{
		D3D12_RESOURCE_DESC   Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(MEGABYTE * 16);
		D3D12_HEAP_PROPERTIES HEAP_Props    = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		HRESULT HR = pDevice->CreateCommittedResource(
			&HEAP_Props,
			D3D12_HEAP_FLAG_NONE,
			&Resource_DESC,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&deviceBuffer));

		SETDEBUGNAME(deviceBuffer, __func__);

		CD3DX12_RANGE Range(0, 0);
		HR = deviceBuffer->Map(0, &Range, (void**)&Buffer); CheckHR(HR, ASSERTONFAIL("FAILED TO MAP TEMP BUFFER"));
	}


	/************************************************************************************************/


	void UploadBuffer::Release()
	{
		if (!deviceBuffer)
			return;

		deviceBuffer->Unmap(0, nullptr);
		deviceBuffer->Release();

		Position        = 0;
		Size            = 0;
		deviceBuffer    = nullptr;
		Buffer          = nullptr;
	}


	/************************************************************************************************/


	ID3D12Resource* UploadBuffer::Resize(const size_t size)
	{
		auto previousBuffer = deviceBuffer;
		deviceBuffer->Unmap(0, 0);

		D3D12_RESOURCE_DESC   Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(size);
		D3D12_HEAP_PROPERTIES HEAP_Props	= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		ID3D12Resource* newDeviceBuffer = nullptr;

		HRESULT HR = parentDevice->CreateCommittedResource(
			&HEAP_Props,
			D3D12_HEAP_FLAG_NONE,
			&Resource_DESC,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&newDeviceBuffer));

		Position       = 0;
		Last	       = 0;
		Size           = size;
		deviceBuffer   = newDeviceBuffer;
		SETDEBUGNAME(newDeviceBuffer, "TEMPORARY");

		CD3DX12_RANGE Range(0, 0);
		HR = newDeviceBuffer->Map(0, &Range, (void**)&Buffer);   CheckHR(HR, ASSERTONFAIL("FAILED TO MAP TEMP BUFFER"));

		return previousBuffer;
	}


	/************************************************************************************************/


    void CopyContext::Barrier(ID3D12Resource* resource, const DeviceResourceState before, const DeviceResourceState after)
    {
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            resource,
            DRS2D3DState(before != DeviceResourceState::DRS_Write ? DeviceResourceState::DRS_GENERIC : before),
            DRS2D3DState(after));


        if (pendingBarriers.full())
            flushPendingBarriers();

        pendingBarriers.push_back(barrier);
    }


    /************************************************************************************************/


    void CopyContext::flushPendingBarriers()
    {
        if (pendingBarriers.empty())
            return;

        commandList->ResourceBarrier(pendingBarriers.size(), pendingBarriers.data());
        pendingBarriers.clear();
    }


    /************************************************************************************************/


	UploadReservation CopyContext::Reserve(const size_t reserveSize, const size_t reserveAlignement)
	{
		// Not enough remaining Space in Buffer GOTO Beginning if space in front of upload buffer is available
		if	(uploadBuffer.Position + reserveSize > uploadBuffer.Size && uploadBuffer.Last != 0)
			uploadBuffer.Position = 0;

		auto GetOffset = [&]() {
			auto offset = reserveAlignement - (uploadBuffer.Position & (reserveAlignement - 1));
			return (offset == reserveAlignement) ? 0 : offset;
		};

		auto ResizeBuffer = [&] {
			const auto newSize = (size_t )std::pow(2, std::floor(std::log2(reserveSize)) + 1);
			freeResources.push_back(uploadBuffer.Resize(newSize));
		};

		// Buffer too Small
        auto temp = reserveSize + GetOffset();
		if (uploadBuffer.Position + reserveSize + GetOffset() > uploadBuffer.Size)
			ResizeBuffer();

		if (uploadBuffer.Last > uploadBuffer.Position)
		{	// Potential Overlap condition
			if (uploadBuffer.Position + reserveSize + GetOffset() >= uploadBuffer.Last)
				ResizeBuffer();  // Resize Buffer and then upload

			const auto alignmentOffset  = GetOffset();
			char*           buffer      = uploadBuffer.Buffer + uploadBuffer.Position + alignmentOffset;
			const size_t    offset      = uploadBuffer.Position + alignmentOffset;

			uploadBuffer.Position += reserveSize + alignmentOffset;

			return { uploadBuffer.deviceBuffer, reserveSize, offset, buffer };
		}

		if(uploadBuffer.Last <= uploadBuffer.Position)
		{	// Safe, Do Upload
			const auto alignmentOffset = GetOffset();

			char* buffer            = uploadBuffer.Buffer + uploadBuffer.Position + alignmentOffset;
			size_t offset           = uploadBuffer.Position + alignmentOffset;
			uploadBuffer.Position  += reserveSize + alignmentOffset;

			return { uploadBuffer.deviceBuffer, reserveSize, offset, buffer };
		}

		return {};
	}


	/************************************************************************************************/


	void CopyContext::CopyBuffer(ID3D12Resource* destination, const size_t destinationOffset, UploadReservation source)
	{
        flushPendingBarriers();

		commandList->CopyBufferRegion(
			destination,
			destinationOffset,
			source.resource,
			source.offset,
			source.reserveSize);
	}


	/************************************************************************************************/


	void CopyContext::CopyBuffer(ID3D12Resource* destination, const size_t destinationOffset, ID3D12Resource* source, const size_t sourceOffset, const size_t sourceSize)
	{
        flushPendingBarriers();

		commandList->CopyBufferRegion(
			destination,
			destinationOffset,
			source,
			sourceOffset,
			sourceSize);
	}


	/************************************************************************************************/


	void CopyContext::CopyTextureRegion(
		ID3D12Resource*     destination,
		size_t              subResourceIdx,
		uint3               XYZ,
		UploadReservation   source,
		uint2               WH,
		DeviceFormat           format)
	{
        flushPendingBarriers();

		const auto      deviceFormat    = TextureFormat2DXGIFormat(format);
		const size_t    formatSize      = GetFormatElementSize(deviceFormat);

		const size_t rowPitch           = max((formatSize * WH[0]) / 256, 1) * 256;
		//size_t alignmentOffset    = rowPitch & 0x01ff;

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT SubRegion;
		SubRegion.Footprint.Depth       = 1;
		SubRegion.Footprint.Format      = deviceFormat;
		SubRegion.Footprint.RowPitch    = rowPitch;
		SubRegion.Footprint.Width       = WH[0];
		SubRegion.Footprint.Height      = WH[1];
		SubRegion.Offset                = source.offset;

		auto destinationLocation    = CD3DX12_TEXTURE_COPY_LOCATION(destination, subResourceIdx);
		auto sourceLocation         = CD3DX12_TEXTURE_COPY_LOCATION(source.resource, SubRegion);

		commandList->CopyTextureRegion(
			&destinationLocation,
			XYZ[0], XYZ[1], XYZ[2],
			&sourceLocation,
			nullptr);
	}


    /************************************************************************************************/


    void CopyContext::CopyTile(ID3D12Resource* dest, const uint3 destTile, const size_t tileOffset, const UploadReservation src)
    {
        flushPendingBarriers();

        const D3D12_TILED_RESOURCE_COORDINATE coordinate = {
            .X              = (UINT)destTile[0],
            .Y              = (UINT)destTile[1],
            .Z              = (UINT)0,
            .Subresource    = (UINT)destTile[2],
        };

        const D3D12_TILE_REGION_SIZE regionSize = {
            .NumTiles   = 1,
            .UseBox     = true,
            .Width      = 1,
            .Height     = 1,
            .Depth      = 1,
        };

        commandList->CopyTiles(
            dest,
            &coordinate,
            &regionSize,
            src.resource,
            tileOffset * 64 * KILOBYTE,
            D3D12_TILE_COPY_FLAG_LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE);
    }


    /************************************************************************************************/
    

	CopyContext& CopyEngine::operator [](CopyContextHandle handle)
	{
		return copyContexts[handle];
	}


	/************************************************************************************************/


	CopyContextHandle CopyEngine::Open()
	{
		const size_t currentIdx = idx++ % copyContexts.size();
		auto& ctx               = copyContexts[currentIdx];

		Wait(CopyContextHandle{ currentIdx });

		ctx.commandAllocator->Reset();
		ctx.commandList->Reset(ctx.commandAllocator, nullptr);

		for (auto resource : ctx.freeResources)
			resource->Release();

		return CopyContextHandle{ currentIdx };
	}


	/************************************************************************************************/


	void CopyEngine::Wait(CopyContextHandle handle)
	{
		auto& ctx = copyContexts[handle];

		if (fence->GetCompletedValue() < ctx.counter)
		{
			fence->SetEventOnCompletion(ctx.counter, ctx.eventHandle);
			WaitForSingleObject(ctx.eventHandle, INFINITY);
		}
	}


	/************************************************************************************************/


	void CopyEngine::Close(CopyContextHandle handle)
	{
		auto& ctx = copyContexts[handle];
        ctx.flushPendingBarriers();
		ctx.commandList->Close();
	}


	/************************************************************************************************/


	void CopyEngine::Submit(CopyContextHandle* begin, CopyContextHandle* end)
	{
        const size_t localCounter = ++counter;
		static_vector<ID3D12CommandList*, 64> cmdLists;

		for (auto itr = begin; itr < end; itr++)
		{
			auto& context = copyContexts[*itr];
			cmdLists.push_back(context.commandList);

            context.counter             = localCounter;
			context.uploadBuffer.Last   = context.uploadBuffer.Position;
            
			Close(*itr);
		}

		copyQueue->ExecuteCommandLists(cmdLists.size(), cmdLists);
        copyQueue->Signal(fence, counter);
	}


    /************************************************************************************************/


    void CopyEngine::Signal(ID3D12Fence* fence, const size_t counter)
    {
        copyQueue->Signal(fence, counter);
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
				WaitForSingleObject(ctx.eventHandle, INFINITY);

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
			SETDEBUGNAME(commandAllocator,  "COPY ALLOCATOR");
			FK_VLOG		(10,                "COPY COMMANDLIST CREATED: %u", copyCommandList);

			ObjectsCreated.push_back(copyCommandList);

			Success &= !FAILED(HR);

			copyCommandList->Close();
			commandAllocator->Reset();

			copyContexts.push_back({
				commandAllocator,
				copyCommandList,
				0,
				CreateEvent(nullptr, FALSE, FALSE, nullptr),
				UploadBuffer{ Device },
				Vector<ID3D12Resource*>{ allocator }});
			}

		Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

		return Success;
	}


	/************************************************************************************************/


	bool RenderSystem::Initiate(Graphics_Desc* in)
	{
#if USING( DEBUGGRAPHICS )
		gWindowHandle		= GetConsoleWindow();
		gInstance			= GetModuleHandle( 0 );
#endif

		Vector<ID3D12DeviceChild*> ObjectsCreated(in->Memory);

		RegisterWindowClass(gInstance);

		Memory			   = in->Memory;
		Settings.AAQuality = 0;
		Settings.AASamples = 1;
		UINT DeviceFlags   = 0;

		ID3D12Device*		Device;
		ID3D12Debug*		Debug;
		ID3D12DebugDevice*  DebugDevice;
		
		HRESULT HR;
		#if USING( DEBUGGRAPHICS )
		HR = D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&Debug);			FK_ASSERT(SUCCEEDED(HR));
		Debug->EnableDebugLayer();
		#else
		Debug		= nullptr;
		DebugDevice = nullptr;
		#endif	

		bool InitiateComplete = false;

		HR = D3D12CreateDevice(nullptr,	D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&Device));
		if(FAILED(HR))
		{
			FK_LOG_ERROR("Failed to create A DX12 Device!");

			// Trying again with a DX11 Feature Level
			HR = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device));
			if (FAILED(HR))
			{
				FK_LOG_ERROR("Failed to create A DX11 Device!");
				MessageBox(NULL, L"FAILED TO CREATE D3D12 ADAPTER! GET A NEWER COMPUTER", L"ERROR!", MB_OK);
				return false;
			}
		}

#if USING(AFTERMATH)
        auto res = GFSDK_Aftermath_DX12_Initialize(GFSDK_Aftermath_Version_API, GFSDK_Aftermath_FeatureFlags_Maximum, Device);
#endif

#if USING(DEBUGGRAPHICS)
		HR =  Device->QueryInterface(__uuidof(ID3D12DebugDevice), (void**)&DebugDevice);
#else
		DebugDevice = nullptr;
#endif
		
		{
			ID3D12Fence* NewFence = nullptr;
			HR = Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&NewFence));
			FK_ASSERT(FAILED(HR), "FAILED TO CREATE FENCE!");
			SETDEBUGNAME(NewFence, "GRAPHICS FENCE");

			FK_LOG_9("GRAPHICS FENCE CREATED: %u", NewFence);

			Fence = NewFence;
			ObjectsCreated.push_back(NewFence);
		}
		
		D3D12_COMMAND_QUEUE_DESC CQD		= {};
		CQD.Flags							            = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
		CQD.Type								        = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;

		D3D12_COMMAND_QUEUE_DESC ComputeCQD = {};
		ComputeCQD.Flags								= D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
		ComputeCQD.Type									= D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE;

		IDXGIFactory5*				DXGIFactory         = nullptr;
		IDXGIAdapter4*				DXGIAdapter			= nullptr;
		

		HR = Device->CreateCommandQueue(&CQD,			IID_PPV_ARGS(&GraphicsQueue));		FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND QUEUE!");
		HR = Device->CreateCommandQueue(&ComputeCQD,	IID_PPV_ARGS(&ComputeQueue));		FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND QUEUE!");


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


		// Create Resources
		HR = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&DXGIFactory));

		FK_ASSERT(FAILED(HR), "FAILED TO CREATE DXGIFactory!"  );

		FINALLY
			if (!InitiateComplete)
				DXGIFactory->Release();
		FINALLYOVER

		{
			// Copy temp resources over
			pDevice					= Device;
			pGIFactory				= DXGIFactory;
			pDXGIAdapter			= DXGIAdapter;
			pDebugDevice			= DebugDevice;
			pDebug					= Debug;
			BufferCount				= 3;
			FenceCounter			= 0;
			DescriptorRTVSize		= Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			DescriptorDSVSize		= Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			DescriptorCBVSRVUAVSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		{
			// Initiate null resources
			ConstantBuffer_desc NullBuffer_Desc;
			NullBuffer_Desc.InitialSize	= 1024;
			NullBuffer_Desc.pInital		= nullptr;
			NullBuffer_Desc.Structured	= false;

			NullConstantBuffer = _CreateConstantBufferResource(this, &NullBuffer_Desc);
			NullConstantBuffer._SetDebugName("NULL CONSTANT BUFFER");

			ObjectsCreated.push_back(NullConstantBuffer[0]);
			ObjectsCreated.push_back(NullConstantBuffer[1]);
			ObjectsCreated.push_back(NullConstantBuffer[2]);
		}

		NullUAV     = CreateUAVTextureResource({1, 1}, DeviceFormat::UNKNOWN, true);                                   SetDebugName(NullUAV,   "NULL UAV");
		NullSRV     = CreateGPUResource(GPUResourceDesc::ShaderResource({1, 1}, DeviceFormat::R32G32B32A32_FLOAT));    SetDebugName(NullSRV,   "NULL SRV");
		NullSRV1D   = CreateGPUResource(GPUResourceDesc::StructuredResource(1024));                               SetDebugName(NullSRV1D, "NULL SRV");

		heaps.Init(pDevice);
		copyEngine.Initiate(Device, (threads.GetThreadCount() + 1) * 1.5, ObjectsCreated, in->Memory);

		for (size_t I = 0; I < 3 * (1 + threads.GetThreadCount()); ++I)
			Contexts.emplace_back(this, Memory);

		InitiateComplete = true;
		
		Library.Initiate(this, in->TempMemory);
        ReadBackTable.Initiate(Device);

		FreeList.Allocator = in->Memory;

		return InitiateComplete;
	}



	/************************************************************************************************/


	void Release(DepthBuffer* DB)
	{
		for(auto R : DB->Buffer)if(R) R->Release();
	}


	/************************************************************************************************/


	void RenderSystem::Release()
	{
		if (!Memory)
			return;

        WaitforGPU();

		FK_LOG_9("Releasing RenderSystem");

		for (auto& ctx : Contexts)
			ctx.Release();

		while (FreeList.size())
			Free_DelayedReleaseResources(this);

		FreeList.Release();
		copyEngine.Release();

		for (auto& FR : Contexts)
			FR.Release();

		ConstantBuffers.Release();
		VertexBuffers.Release();
		Textures.Release();
		BufferUAVs.ReleaseAll();
		Texture2DUAVs.ReleaseAll();
		PipelineStates.ReleasePSOs();

		NullConstantBuffer.Release();

		Library.Release();

		if(GraphicsQueue)	GraphicsQueue->Release();
		if(ComputeQueue)	ComputeQueue->Release();
		if(pGIFactory)		pGIFactory->Release();
		if(pDXGIAdapter)	pDXGIAdapter->Release();
		if(pDevice)			pDevice->Release();
		if(Fence)			Fence->Release();

		FreeList.Release();

#if USING(DEBUGGRAPHICS)
		// Prints Detailed Report
		pDebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
		pDebugDevice->Release();
		pDebug->Release();
#endif

		Memory = nullptr;
	}


	/************************************************************************************************/


	ID3D12PipelineState* RenderSystem::GetPSO(PSOHandle StateID)
	{
		return PipelineStates.GetPSO(StateID);
	}


	/************************************************************************************************/


	RootSignature const * const RenderSystem::GetPSORootSignature(PSOHandle handle) const
	{
		return PipelineStates.GetPSORootSig(handle);
	}


	/************************************************************************************************/


	void RenderSystem::RegisterPSOLoader(PSOHandle State, PipelineStateDescription desc)
	{
		PipelineStates.RegisterPSOLoader(State, desc);
	}


	/************************************************************************************************/


	void RenderSystem::QueuePSOLoad(PSOHandle State)
	{
		FK_LOG_2("Reloading PSO!");

		PipelineStates.QueuePSOLoad(State, Memory);
	}


	/************************************************************************************************/


	size_t	RenderSystem::GetCurrentFrame()
	{
		return CurrentFrame;
	}


    /************************************************************************************************/


    size_t RenderSystem::GetCurrentCounter()
    {
        return FenceCounter;
    }


	/************************************************************************************************/

	
	void RenderSystem::PresentWindow(RenderWindow* Window)
	{
		auto HR = Window->SwapChain_ptr->Present(0, 0);

        if (HR == DXGI_ERROR_DEVICE_REMOVED || HR == DXGI_ERROR_DEVICE_RESET)
        {
            _OnCrash();
        }
        else
        {
        }


		Textures.SetBufferedIdx(Window->backBuffer, Window->SwapChain_ptr->GetCurrentBackBufferIndex());
		Textures.UpdateLocks();

		ConstantBuffers.DecrementLocks();

		++CurrentFrame;
	}


	/************************************************************************************************/


	void RenderSystem::WaitforGPU()
	{
        GraphicsQueue->Signal(Fence, ++FenceCounter);

		const size_t completedValue	= Fence->GetCompletedValue();
        const size_t currentCounter = FenceCounter;

		if (currentCounter > completedValue)
        {
			HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS); FK_ASSERT(eventHandle != 0);
			Fence->SetEventOnCompletion(currentCounter, eventHandle);
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}


	/************************************************************************************************/



	void RenderSystem::SetDebugName(ResourceHandle handle, const char* str)
	{
		Textures.SetDebugName(handle, str);
	}


	/************************************************************************************************/



	void RenderSystem::SetDebugName(UAVResourceHandle handle, const char* str)
	{
		auto resource = GetDeviceResource(handle);
		SETDEBUGNAME(resource, str);
	}


	/************************************************************************************************/


	void RenderSystem::SetDebugName(UAVTextureHandle handle, const char* str)
	{
		auto resource = GetDeviceResource(handle);
		SETDEBUGNAME(resource, str);
	}


	/************************************************************************************************/


	D3D12_GPU_VIRTUAL_ADDRESS RenderSystem::GetVertexBufferAddress(const VertexBufferHandle VB)
	{
		return VertexBuffers.GetAsset(VB)->GetGPUVirtualAddress();
	}


	/************************************************************************************************/


	size_t RenderSystem::GetVertexBufferSize(const VertexBufferHandle VB)
	{
		return VertexBuffers.GetBufferSize(VB);
	}


	/************************************************************************************************/


	uint32_t RenderSystem::GetTag(ResourceHandle handle)
	{
		return Textures.GetTag(handle);
	}


	/************************************************************************************************/


	void RenderSystem::SetTag(ResourceHandle handle, uint32_t Tag)
	{
		Textures.SetTag(handle, Tag);
	}


	/************************************************************************************************/


	void RenderSystem::MarkTextureUsed(ResourceHandle Handle)
	{
		Textures.MarkRTUsed(Handle);
	}


	/************************************************************************************************/



	const size_t	RenderSystem::GetTextureElementSize(ResourceHandle handle) const
	{
		auto Format = Textures.GetFormat(handle);
		return GetFormatElementSize(Format);
	}


	/************************************************************************************************/


	const uint2	RenderSystem::GetTextureWH(ResourceHandle handle) const
	{
		return Textures.GetWH(handle);
	}


	/************************************************************************************************/


	const uint2	RenderSystem::GetTextureWH(UAVTextureHandle Handle) const
	{
		return Texture2DUAVs.GetExtra(Handle).WH;
	}


	/************************************************************************************************/


	DeviceFormat RenderSystem::GetTextureFormat(ResourceHandle handle) const
	{
		return DXGIFormat2TextureFormat(Textures.GetFormat(handle));
	}


	/************************************************************************************************/


	DXGI_FORMAT RenderSystem::GetTextureDeviceFormat(ResourceHandle handle) const
	{
		return Textures.GetFormat(handle);
	}


	/************************************************************************************************/


	TextureDimension RenderSystem::GetTextureDimension(ResourceHandle handle) const
	{
		return Textures.GetDimension(handle);
	}


	/************************************************************************************************/


	size_t RenderSystem::GetTextureArraySize(ResourceHandle handle) const
	{
		return Textures.GetArraySize(handle);
	}


    /************************************************************************************************/


    uint32_t RenderSystem::GetTextureMipCount(ResourceHandle handle) const
    {
        return Textures.GetMIPCount(handle);
    }


	/************************************************************************************************/


	void RenderSystem::UploadTexture(ResourceHandle handle, CopyContextHandle queue, byte* buffer, size_t bufferSize)
	{
		auto resource	= GetDeviceResource(handle);
		auto wh			= GetTextureWH(handle);
		auto formatSize = GetTextureElementSize(handle); FK_ASSERT(formatSize != -1);

		size_t resourceSize = bufferSize;
		size_t offset       = 0;

		TextureBuffer textureBuffer{ wh, buffer, bufferSize, };

		SubResourceUpload_Desc desc;
		desc.buffers            = &textureBuffer;

		_UpdateSubResourceByUploadQueue(this, queue, resource, &desc, D3D12_RESOURCE_STATE_COMMON);
	}


	/************************************************************************************************/


	void RenderSystem::UploadTexture(ResourceHandle handle, CopyContextHandle queue, TextureBuffer* buffers, size_t resourceCount, iAllocator* temp) // Uses Upload Queue
	{
		auto resource	= GetDeviceResource(handle);
		auto format		= GetTextureFormat(handle);

		SubResourceUpload_Desc desc;
		desc.buffers            = buffers;
		desc.subResourceCount   = resourceCount;
		desc.format             = format;

		_UpdateSubResourceByUploadQueue(this, queue, resource, &desc, D3D12_RESOURCE_STATE_COMMON);
	}


	/************************************************************************************************/


	D3D12_GPU_VIRTUAL_ADDRESS RenderSystem::GetConstantBufferAddress(const ConstantBufferHandle CB)
	{
		// TODO: deal with Push Buffer Offsets
		return ConstantBuffers.GetDeviceResource(CB)->GetGPUVirtualAddress();
	}


	/************************************************************************************************/


	size_t RenderSystem::GetTextureFrameGraphIndex(ResourceHandle Texture)
	{
		auto FrameID = GetCurrentFrame();
		return Textures.GetFrameGraphIndex(Texture, FrameID);
	}


	/************************************************************************************************/


	void RenderSystem::SetTextureFrameGraphIndex(ResourceHandle Texture, size_t Index)
	{
		auto FrameID = GetCurrentFrame();
		Textures.SetFrameGraphIndex(Texture, FrameID, Index);
	}


	/************************************************************************************************/


	DeviceHeapHandle  RenderSystem::CreateHeap(const size_t heapSize, const uint32_t flags)
	{
		return heaps.CreateHeap(heapSize, flags);
	}


	/************************************************************************************************/


	ConstantBufferHandle RenderSystem::CreateConstantBuffer(size_t BufferSize, bool GPUResident)
	{
		return ConstantBuffers.CreateConstantBuffer(BufferSize, GPUResident);
	}


	/************************************************************************************************/


	VertexBufferHandle RenderSystem::CreateVertexBuffer(size_t BufferSize, bool GPUResident)
	{
		return VertexBuffers.CreateVertexBuffer(BufferSize, GPUResident, this);
	}


	/************************************************************************************************/


	ResourceHandle RenderSystem::CreateDepthBuffer( const uint2 WH, const bool UseFloat, const size_t bufferCount)
	{
		auto resource = CreateGPUResource(GPUResourceDesc::DepthTarget(WH, UseFloat ? DeviceFormat::D32_FLOAT : DeviceFormat::D24_UNORM_S8_UINT));
		SetDebugName(resource, "DepthBuffer");

		return resource;
	}


	/************************************************************************************************/


	ResourceHandle RenderSystem::CreateGPUResource(const GPUResourceDesc& desc)
	{
		if (desc.backBuffer)
		{
			return Textures.AddResource(desc, DRS_Present);
		}
		else if (desc.PreCreated)
		{
			return Textures.AddResource(desc, DRS_ShaderResource);
		}
		else
		{
			D3D12_RESOURCE_DESC   Resource_DESC = 
					(desc.Dimensions == TextureDimension::Texture1D) ?
					CD3DX12_RESOURCE_DESC::Buffer(desc.WH[0]):
					(desc.Dimensions == TextureDimension::Texture2D) ? (
					CD3DX12_RESOURCE_DESC::Tex2D(
						TextureFormat2DXGIFormat(desc.format),
						desc.WH[0],
						desc.WH[1],
						desc.arraySize,
						desc.MipLevels)
					) : (
					CD3DX12_RESOURCE_DESC::Tex2D(
						TextureFormat2DXGIFormat(desc.format),
						desc.WH[0],
						desc.WH[1],
						6,
						desc.MipLevels));


			Resource_DESC.Flags	= desc.renderTarget ?
									D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : 
									D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;

			Resource_DESC.Flags |= desc.unordered ?
				D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS :
				D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;

			Resource_DESC.Flags |=  (desc.depthTarget) ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : 
				D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;

			Resource_DESC.MipLevels	= !desc.MipLevels ? 1 : desc.MipLevels;

			D3D12_HEAP_PROPERTIES HEAP_Props	={};
			HEAP_Props.CPUPageProperty			= D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			HEAP_Props.Type						= D3D12_HEAP_TYPE_DEFAULT;
			HEAP_Props.MemoryPoolPreference		= D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
			HEAP_Props.CreationNodeMask			= 0;
			HEAP_Props.VisibleNodeMask			= 0;

			D3D12_CLEAR_VALUE CV;
			CV.Color[0] = desc.depthTarget ? 1.0f : 0.0f;
			CV.Color[1] = 0.0f;
			CV.Color[2] = 0.0f;
			CV.Color[3] = 0.0f;
			CV.Format	= TextureFormat2DXGIFormat(desc.format);
		
			auto flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES |  ( desc.unordered ? D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS : D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE);

			D3D12_CLEAR_VALUE* pCV = (desc.CVFlag | desc.renderTarget | desc.depthTarget) ? &CV : nullptr;


			D3D12_RESOURCE_STATES InitialState =
				desc.renderTarget   ? D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET :
				desc.depthTarget    ? D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE   :
				D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;

			ID3D12Resource* NewResource[3] = { nullptr, nullptr, nullptr };

			FK_ASSERT(desc.bufferCount <= 3);

			FK_LOG_9("Creating Texture!");

			const size_t end = desc.bufferCount;
			for (size_t itr = 0; itr < end; ++itr)
			{
				if (desc.Virtual)
				{
                    Resource_DESC.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;

					HRESULT HR = pDevice->CreateReservedResource(
									&Resource_DESC,
									InitialState,
									pCV,
									IID_PPV_ARGS(&NewResource[itr]));

					CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE VIRTUAL MEMORY FOR TEXTURE"));
				}
				else
				{
					HRESULT HR = pDevice->CreateCommittedResource(
									&HEAP_Props, 
									flags,
									&Resource_DESC, InitialState, pCV, IID_PPV_ARGS(&NewResource[itr]));

					CheckHR(HR, ASSERTONFAIL("FAILED TO COMMIT MEMORY FOR TEXTURE"));
				}
				FK_ASSERT(NewResource[itr], "Failed to Create Texture!");
				SETDEBUGNAME(NewResource[itr], __func__);
			}

			auto filledDesc = desc;


			auto initialState =
				filledDesc.renderTarget ? DRS_RenderTarget:
				filledDesc.depthTarget  ? DRS_DEPTHBUFFERWRITE :
                DRS_ShaderResource;

			filledDesc.resources = NewResource;
			return Textures.AddResource(filledDesc, initialState);
		}

		return InvalidHandle_t;
	}


	/************************************************************************************************/


	QueryHandle	RenderSystem::CreateOcclusionBuffer(size_t Counts)
	{
		return Queries.CreateQueryBuffer(Counts, QueryType::OcclusionQuery);
	}


	/************************************************************************************************/


	UAVResourceHandle RenderSystem::CreateUAVBufferResource(size_t resourceSize, bool tripleBuffer)
	{
		D3D12_RESOURCE_DESC Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(resourceSize);
		Resource_DESC.Alignment           = 0;
		Resource_DESC.DepthOrArraySize    = 1;
		Resource_DESC.Dimension           = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		Resource_DESC.Layout              = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Resource_DESC.Width               = resourceSize;
		Resource_DESC.Height              = 1;
		Resource_DESC.Format              = DXGI_FORMAT_UNKNOWN;
		Resource_DESC.SampleDesc.Count    = 1;
		Resource_DESC.SampleDesc.Quality  = 0;
		Resource_DESC.Flags               = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		D3D12_HEAP_PROPERTIES HEAP_Props  = {};
		HEAP_Props.CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type                   = D3D12_HEAP_TYPE_DEFAULT;
		HEAP_Props.MemoryPoolPreference   = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask       = 0;
		HEAP_Props.VisibleNodeMask        = 0;

		static_vector<ID3D12Resource*, 3> buffers;

		for (size_t I = 0; I < (tripleBuffer ? BufferCount : 1); ++I)
		{
			ID3D12Resource* Resource = nullptr;
			HRESULT HR = pDevice->CreateCommittedResource(
								&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES | D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS,
								&Resource_DESC, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
								IID_PPV_ARGS(&Resource));

			CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE UAV BUFFER RESOURCE!"));
			buffers.push_back(Resource);

			SETDEBUGNAME(Resource, __func__);
		}

		auto handle = BufferUAVs.AddResource(buffers, resourceSize, DeviceResourceState::DRS_Write); // DRS Write assumed on first use, ignores initial state specified above!

		UAVResourceLayout layout;
		layout.elementCount		= resourceSize / sizeof(uint32_t);// initial layout assume a uint buffer
		layout.format			= DXGI_FORMAT_UNKNOWN;
		layout.stride			= sizeof(uint32_t);

		BufferUAVs.SetExtra(handle, layout);

		return handle;
	}


	/************************************************************************************************/


	UAVTextureHandle RenderSystem::CreateUAVTextureResource(const uint2 WH, const DeviceFormat format, const bool RenderTarget)
	{
		D3D12_RESOURCE_DESC Resource_DESC = CD3DX12_RESOURCE_DESC::Tex2D(TextureFormat2DXGIFormat(format), WH[0], WH[1]);
		Resource_DESC.MipLevels         = 1;

		Resource_DESC.Flags              =
			D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS |
			(RenderTarget ? D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE);

		D3D12_HEAP_PROPERTIES HEAP_Props = {};
		HEAP_Props.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type                  = D3D12_HEAP_TYPE_DEFAULT;
		HEAP_Props.MemoryPoolPreference  = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask      = 0;
		HEAP_Props.VisibleNodeMask       = 0;

		static_vector<ID3D12Resource*, 3> buffers;

		ID3D12Resource* Resource = nullptr;
		HRESULT HR = pDevice->CreateCommittedResource(
							&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES | D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS,
							&Resource_DESC, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
							IID_PPV_ARGS(&Resource));

		CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE UAV TEXTURE RESOURCE!"));
		buffers.push_back(Resource);

		SETDEBUGNAME(Resource, __func__);

		auto handle = Texture2DUAVs.AddResource(buffers, 1, DeviceResourceState::DRS_Write);

		UAVTextureLayout layout;
		layout.format   = TextureFormat2DXGIFormat(format);
		layout.mipCount = 1;
		layout.WH       = WH;
		Texture2DUAVs.SetExtra(handle, layout);

		return handle;
	}


	/************************************************************************************************/


	SOResourceHandle RenderSystem::CreateStreamOutResource(size_t resourceSize, bool tripleBuffered)
	{
		D3D12_RESOURCE_DESC Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(resourceSize);
		Resource_DESC.Width              = resourceSize;
		Resource_DESC.Format             = DXGI_FORMAT_UNKNOWN;
		Resource_DESC.Flags              = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		D3D12_RESOURCE_DESC Counter_DESC = CD3DX12_RESOURCE_DESC::Buffer(resourceSize);
		Counter_DESC.Width              = 512;
		Counter_DESC.Format             = DXGI_FORMAT_UNKNOWN;
		Counter_DESC.Flags              = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		D3D12_HEAP_PROPERTIES HEAP_Props = {};
		HEAP_Props.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type                  = D3D12_HEAP_TYPE_DEFAULT;
		HEAP_Props.MemoryPoolPreference  = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;

		static_vector<ID3D12Resource*>	resources;
		static_vector<ID3D12Resource*>	counters;
		for (size_t I = 0; I < (tripleBuffered ? BufferCount : 1); ++I)
		{
			ID3D12Resource* Resource	= nullptr;
			ID3D12Resource* Counter		= nullptr;

			HRESULT HR;

			HR = pDevice->CreateCommittedResource(
								&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
								&Resource_DESC, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_STREAM_OUT, nullptr,
								IID_PPV_ARGS(&Resource));
			CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE STREAMOUT RESOURCE!"));

			HR = pDevice->CreateCommittedResource(
								&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
								&Resource_DESC, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_STREAM_OUT, nullptr,
								IID_PPV_ARGS(&Counter));

			CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE STREAMOUT RESOURCE!"));
			resources.push_back(Resource);
			counters.push_back(Counter);

			SETDEBUGNAME(Resource, "StreamOutResource" );
			SETDEBUGNAME(Counter, "StreamOutCounter" );
		}

		return StreamOutTable.AddResource(resources, counters, resourceSize, DeviceResourceState::DRS_STREAMOUT);
	}


	/************************************************************************************************/



	QueryHandle RenderSystem::CreateSOQuery(size_t SOIndex, size_t count)
	{
		return Queries.CreateSOQueryBuffer(count, SOIndex);
	}


	/************************************************************************************************/


	IndirectLayout RenderSystem::CreateIndirectLayout(static_vector<IndirectDrawDescription> entries, iAllocator* allocator)
	{
		ID3D12CommandSignature* signature = nullptr;
		
		Vector<IndirectDrawDescription>				layout{allocator};
		static_vector<D3D12_INDIRECT_ARGUMENT_DESC> signatureEntries;

		size_t entryStride = 0;

		for (size_t itr = 0; itr < entries.size(); ++itr)
		{
			switch (entries[itr].type)
			{
			case ILE_DrawCall:
			{
				D3D12_INDIRECT_ARGUMENT_DESC desc = {};
				desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

				signatureEntries.push_back(desc);
				layout.push_back(ILE_DrawCall);
				entryStride = max(entryStride, sizeof(uint32_t) * 4); // uses 4 8byte values
			}	break;
			}
		}

		D3D12_COMMAND_SIGNATURE_DESC desc;
		desc.ByteStride			= entryStride;
		desc.NumArgumentDescs	= entries.size();
		desc.pArgumentDescs		= signatureEntries.begin();
		desc.NodeMask			= 0;

		auto HR = pDevice->CreateCommandSignature(
			&desc,
			nullptr,
			IID_PPV_ARGS(&signature));

		CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE CONSTANT BUFFER"));

		return { signature, entryStride, std::move(layout) };
	}


    /************************************************************************************************/


    void RenderSystem::UpdateTileMappings(ResourceHandle* begin, ResourceHandle* end, iAllocator* allocator)
    {
        auto itr = begin;
        while(itr < end)
        {
            auto& mappings                      = Textures.GetTileMapping(*itr);
            auto deviceResource                 = GetDeviceResource(*itr);
            const auto mipCount                 = Textures.GetMIPCount(*itr);

            /*
            auto desc                           = deviceResource->GetDesc();
            const auto state                    = GetObjectState(*itr);
            ID3D12Resource* remappedResource    = nullptr; pDevice->CreateReservedResource(&desc, DRS2D3DState(state), nullptr, IID_PPV_ARGS(&remappedResource));
            
            _PushDelayReleasedResource(deviceResource);
            Textures.ReplaceResources(*itr, &remappedResource, &remappedResource + 1);
            */
            Vector<D3D12_TILED_RESOURCE_COORDINATE> coordinates { allocator };
            Vector<D3D12_TILE_REGION_SIZE>          regionSizes { allocator };
            Vector<D3D12_TILE_RANGE_FLAGS>          flags       { allocator };
            Vector<UINT>                            offsets     { allocator };
            Vector<UINT>                            tileRanges  { allocator };
            DeviceHeapHandle                        heap = InvalidHandle_t;

            uint32_t I = 0;

            while(I < mappings.size())
            {
                heap = mappings[I].heap;

                for (;mappings[I].heap == heap && I < mappings.size(); I++)
                {
                    const auto& mapping = mappings[I];
                    
                    D3D12_TILED_RESOURCE_COORDINATE coordinate;
                    coordinate.Subresource  = mapping.tileID.GetMipLevel(mipCount);
                    coordinate.X            = mapping.tileID.GetTileX();
                    coordinate.Y            = mapping.tileID.GetTileY();
                    coordinate.Z            = 0;

                    coordinates.push_back(coordinate);

                    D3D12_TILE_REGION_SIZE regionSize;
                    regionSize.Depth    = 1;
                    regionSize.Height   = 1;// tileSize[0];
                    regionSize.Width    = 1;// tileSize[1];

                    regionSize.NumTiles = 1;
                    regionSize.UseBox = false;

                    regionSizes.push_back(regionSize);

                    offsets.push_back(mapping.heapOffset);
                    flags.push_back(D3D12_TILE_RANGE_FLAGS::D3D12_TILE_RANGE_FLAG_NONE);
                    tileRanges.push_back(1);
                    UINT tileCount = 1;
                }

                I++;

                if (heap != InvalidHandle_t)
                {
                    D3D12_TILE_RANGE_FLAGS nullRangeFlag = D3D12_TILE_RANGE_FLAG_NULL;

                    copyEngine.copyQueue->UpdateTileMappings(
                        deviceResource,
                        1,
                        nullptr,
                        nullptr,
                        nullptr,
                        1,
                        &nullRangeFlag,
                        nullptr,
                        nullptr,
                        D3D12_TILE_MAPPING_FLAG_NONE );

                    copyEngine.copyQueue->UpdateTileMappings(
                        deviceResource,
                        mappings.size(),
                        coordinates.data(),
                        regionSizes.data(),
                        GetDeviceResource(heap),
                        coordinates.size(),
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
            }

            itr++;
        }
    }


    /************************************************************************************************/


    void RenderSystem::UpdateTextureTileMappings(const ResourceHandle handle, const TileMapList& tileMaps)
    {
        Textures.UpdateTileMappings(handle, tileMaps.begin(), tileMaps.end());
    }


    /************************************************************************************************/


    ReadBackResourceHandle  RenderSystem::CreateReadBackBuffer(const size_t bufferSize)
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

        return ReadBackTable.AddReadBack(bufferSize, resource);
    }


    /************************************************************************************************/


    void RenderSystem::SetReadBackEvent(ReadBackResourceHandle readbackBuffer, ReadBackEventHandler&& handler)
    {
        ReadBackTable.SetCallback(readbackBuffer, std::move(handler));
    }

    /************************************************************************************************/


	void RenderSystem::SetObjectState(SOResourceHandle handle, DeviceResourceState state)
	{
		StreamOutTable.SetResourceState(handle, state);
	}


	/************************************************************************************************/


	void RenderSystem::SetObjectState(UAVResourceHandle handle, DeviceResourceState state)
	{
		BufferUAVs.SetResourceState(handle, state);
	}

	/************************************************************************************************/


	void RenderSystem::SetObjectState(UAVTextureHandle handle, DeviceResourceState state)
	{
		Texture2DUAVs.SetResourceState(handle, state);
	}


	/************************************************************************************************/


	void RenderSystem::SetObjectState(ResourceHandle handle, DeviceResourceState state)
	{
		Textures.SetState(handle, state);
	}


	/************************************************************************************************/


	DeviceResourceState RenderSystem::GetObjectState(const QueryHandle handle) const
	{
		return Queries.GetAssetState(handle);
	}


	/************************************************************************************************/


	DeviceResourceState RenderSystem::GetObjectState(const SOResourceHandle handle) const
	{
		return StreamOutTable.GetAssetState(handle);
	}


	/************************************************************************************************/


	DeviceResourceState RenderSystem::GetObjectState(const UAVResourceHandle handle) const
	{
		return BufferUAVs.GetAssetState(handle);
	}


	/************************************************************************************************/


	DeviceResourceState RenderSystem::GetObjectState(const UAVTextureHandle handle) const
	{
		return Texture2DUAVs.GetAssetState(handle);
	}

	/************************************************************************************************/


	DeviceResourceState RenderSystem::GetObjectState(const ResourceHandle handle) const
	{
		return Textures.GetState(handle);
	}


	/************************************************************************************************/


    ID3D12Heap* RenderSystem::GetDeviceResource(const DeviceHeapHandle handle) const
    {
        return heaps.GetDeviceResource(handle);
    }


    ID3D12Resource* RenderSystem::GetDeviceResource(const ReadBackResourceHandle handle) const
    {
        return ReadBackTable.GetDeviceResource(handle);
    }


    /************************************************************************************************/


	ID3D12Resource* RenderSystem::GetDeviceResource(const ConstantBufferHandle handle) const
	{
		return ConstantBuffers.GetDeviceResource(handle);
	}


	/************************************************************************************************/


	ID3D12Resource* RenderSystem::GetDeviceResource(const ResourceHandle handle) const
	{
		return Textures.GetAsset(handle);
	}


	/************************************************************************************************/


	ID3D12Resource*	RenderSystem::GetDeviceResource(const SOResourceHandle handle) const
	{
		return StreamOutTable.GetAsset(handle);
	}


	/************************************************************************************************/


	ID3D12Resource*	RenderSystem::GetDeviceResource(const UAVResourceHandle handle) const
	{
		return BufferUAVs.GetAsset(handle);
	}


	/************************************************************************************************/


	ID3D12Resource* RenderSystem::GetDeviceResource(const UAVTextureHandle handle) const
	{
		return Texture2DUAVs.GetAsset(handle);
	}


	/************************************************************************************************/


	ID3D12Resource*	RenderSystem::GetSOCounterResource(const SOResourceHandle handle) const
	{
		return StreamOutTable.GetAssetCounter(handle);
	}



	/************************************************************************************************/


	size_t RenderSystem::GetStreamOutBufferSize(const SOResourceHandle handle) const
	{
		return StreamOutTable.GetAssetSize(handle);
	}


	/************************************************************************************************/


	UAVResourceLayout RenderSystem::GetUAVBufferLayout(const UAVResourceHandle handle) const noexcept
	{
		return  BufferUAVs.GetExtra(handle);
	}


	/************************************************************************************************/


	void RenderSystem::SetUAVBufferLayout(const UAVResourceHandle handle, const UAVResourceLayout newConfig) noexcept
	{
		BufferUAVs.SetExtra(handle, newConfig);
	}


	/************************************************************************************************/


	Texture2D RenderSystem::GetUAV2DTexture(const UAVTextureHandle handle) const
	{
		const auto layout = Texture2DUAVs.GetExtra(handle);

		Texture2D tex;
		tex.WH			= layout.WH;
		tex.mipCount	= layout.mipCount;
		tex.Texture		= GetDeviceResource(handle);
		tex.Format		= layout.format;

		return tex;
	}


	/************************************************************************************************/


	void RenderSystem::ResetQuery(QueryHandle handle)
	{
		Queries.LockUntil(handle, CurrentFrame);
	}


	/************************************************************************************************/


	void RenderSystem::ReleaseCB(ConstantBufferHandle Handle)
	{
		ConstantBuffers.ReleaseBuffer(Handle);
	}


	/************************************************************************************************/


	void RenderSystem::ReleaseVB(VertexBufferHandle Handle)
	{
		VertexBuffers.ReleaseVertexBuffer(Handle);
	}


	/************************************************************************************************/


	void RenderSystem::ReleaseTexture(ResourceHandle Handle)
	{
		Textures.ReleaseTexture(Handle);
	}


	/************************************************************************************************/


	void RenderSystem::ReleaseUAV(UAVResourceHandle handle)
	{
		BufferUAVs.ReleaseResource(handle);
	}


	/************************************************************************************************/


	void RenderSystem::ReleaseUAV(UAVTextureHandle handle)
	{
		Texture2DUAVs.ReleaseResource(handle);
	}


	/************************************************************************************************/


	void Push_DelayedRelease(RenderSystem* RS, ID3D12Resource* Res)
	{
		RS->FreeList.push_back({ Res, 3 });
	}


	/************************************************************************************************/


	void Free_DelayedReleaseResources(RenderSystem* RS)
	{
		for (auto& R : RS->FreeList)
			if (!--R.Counter)
				SAFERELEASE(R.Resource);

		RS->FreeList.erase(
			std::partition(
				RS->FreeList.begin(), 
				RS->FreeList.end(), 
				[](const auto& R) -> bool {return (R.Resource); }), 
			RS->FreeList.end());
	}


	/************************************************************************************************/


	void RenderWindow::Resize(const uint2 newWH, RenderSystem& renderSystem)
	{
		WH = newWH;

		renderSystem.WaitforGPU();
		renderSystem._ForceReleaseTexture(backBuffer);

		const auto HR = SwapChain_ptr->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
		if (FAILED(HR))
		{
			FK_ASSERT(0, "Failed to resize back buffer!");
		}

		//recreateBackBuffer
		ID3D12Resource* buffer[3];

		DXGI_SWAP_CHAIN_DESC1 desc;
		SwapChain_ptr->GetDesc1(&desc);
		desc.Height;
		desc.Width;

		const auto bufferCount = desc.BufferCount;

		for (size_t I = 0; I < bufferCount; ++I)
		{
			SwapChain_ptr->GetBuffer(I, __uuidof(ID3D12Resource), (void**)&buffer[I]);
			if (!buffer[I])
				FK_ASSERT(buffer[I], "Failed to Create Back Buffer!");

		}

		backBuffer = renderSystem.CreateGPUResource(
			GPUResourceDesc::BackBuffered(
				{ desc.Width, desc.Height },
				DeviceFormat::R16G16B16A16_FLOAT,
				buffer, 3));

		renderSystem.SetDebugName(backBuffer, "BackBuffer");
		renderSystem.Textures.SetBufferedIdx(backBuffer, SwapChain_ptr->GetCurrentBackBufferIndex());
	}


	/************************************************************************************************/


	bool CreateRenderWindow( RenderSystem* RS, RenderWindowDesc* In_Desc, RenderWindow* out )
	{
		SetProcessDPIAware();


		RenderWindow NewWindow = {0};
		static size_t Window_Count = 0;

		Window_Count++;

		// Register Window Class
		auto Window = CreateWindow( L"RENDER_WINDOW", L"Render Window", WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
							   In_Desc->POS_X,
							   In_Desc->POS_Y,
							   In_Desc->width,
							   In_Desc->height,
							   nullptr,
							   nullptr,
							   gInstance,
							   nullptr );
		
		RECT ClientRect;
		RECT WindowRect;
		GetClientRect(Window, &ClientRect);
		GetWindowRect(Window, &WindowRect);
		MoveWindow(
			Window, 
			In_Desc->POS_X, In_Desc->POS_Y, 
			WindowRect.right - WindowRect.left - ClientRect.right + In_Desc->width, 
			WindowRect.bottom - WindowRect.top - ClientRect.bottom + In_Desc->height, 
			false);

		{
			POINT cursor;
			ScreenToClient(Window, &cursor);
			POINT NewCursorPOS = {
			(LONG)( WindowRect.right - WindowRect.left - ClientRect.right + In_Desc->width ) / 2,
			(LONG)( WindowRect.bottom - WindowRect.top - ClientRect.bottom + In_Desc->height ) / 2 };

			NewWindow.WindowCenterPosition[ 0 ] = NewCursorPOS.x;
			NewWindow.WindowCenterPosition[ 1 ] = NewCursorPOS.y;
			ClientToScreen	( Window, &NewCursorPOS );
			SetCursorPos	( NewCursorPOS.x, NewCursorPOS.y );
			GetCursorPos	( &cursor );
			gLastMousePOS = { cursor.x, cursor.y };
		}

		NewWindow.hWindow	  = Window;
		NewWindow.VP.Height	  = In_Desc->height;
		NewWindow.VP.Width	  = In_Desc->width;
		NewWindow.VP.X		  = 0;
		NewWindow.VP.Y		  = 0;
		NewWindow.VP.Max	  = 1.0f;
		NewWindow.VP.Min      = 0.0f;

		// Create Swap Chain
		DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
		SwapChainDesc.Stereo			= false;
		SwapChainDesc.BufferCount		= 3;
		SwapChainDesc.Width				= In_Desc->width;
		SwapChainDesc.Height			= In_Desc->height;
		SwapChainDesc.Format			= DXGI_FORMAT_R16G16B16A16_FLOAT;
		SwapChainDesc.BufferUsage		= DXGI_USAGE_RENDER_TARGET_OUTPUT;
		SwapChainDesc.SwapEffect		= DXGI_SWAP_EFFECT_FLIP_DISCARD;
		SwapChainDesc.SampleDesc.Count	= 1;
		SwapChainDesc.Flags             = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		ShowWindow(Window, 5);

		IDXGISwapChain1* NewSwapChain_ptr = nullptr;
		HRESULT HR = RS->pGIFactory->CreateSwapChainForHwnd( 
			RS->GraphicsQueue, NewWindow.hWindow, 
			&SwapChainDesc, nullptr, nullptr,
			&NewSwapChain_ptr );



		if ( FAILED( HR ) )
		{
			cout << "Failed to Create Swap Chain!\n";
			FK_ASSERT(FAILED(HR), "FAILED TO CREATE SWAP CHAIN!");
			return false;
		}

		NewWindow.SwapChain_ptr = static_cast<IDXGISwapChain4*>(NewSwapChain_ptr);

		//CreateBackBuffer
		ID3D12Resource* buffer[3];

		for (size_t I = 0; I < SwapChainDesc.BufferCount; ++I)
		{
			NewSwapChain_ptr->GetBuffer( I, __uuidof(ID3D12Resource), (void**)&buffer[I]);
			if (!buffer[I]) {
				FK_ASSERT(buffer[I], "Failed to Create Back Buffer!");
				return false;
			}

		}

		NewWindow.backBuffer = RS->CreateGPUResource(
			GPUResourceDesc::BackBuffered(
				{ In_Desc->width, In_Desc->height },
				DeviceFormat::R16G16B16A16_FLOAT,
				buffer, 3));

		RS->SetDebugName(NewWindow.backBuffer, "BackBuffer");
		RS->Textures.SetBufferedIdx(NewWindow.backBuffer, NewWindow.SwapChain_ptr->GetCurrentBackBufferIndex());

		SetActiveWindow(Window);

		NewWindow.WH[0]         = In_Desc->width;
		NewWindow.WH[1]         = In_Desc->height;
		NewWindow.Close         = false;
		NewWindow.Format		= SwapChainDesc.Format;
		memset(NewWindow.InputBuffer, 0, sizeof(NewWindow.InputBuffer));

		*out = NewWindow;

		return true;
	}


	/************************************************************************************************/


	void SetInputWIndow(RenderWindow* window)
	{
		gInputWindow = window;
	}


	/************************************************************************************************/


	void FlexKit::UpdateInput()
	{
		MSG  msg;
		while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}


	/************************************************************************************************/


	void Release( RenderWindow* Window )
	{
		Window->SwapChain_ptr->SetFullscreenState(false, nullptr);

		DestroyWindow(Window->hWindow);

		Window->SwapChain_ptr->Release();
	}


	/************************************************************************************************/


	constexpr DXGI_FORMAT TextureFormat2DXGIFormat(DeviceFormat F) noexcept
	{
		switch (F)
		{
		case FlexKit::DeviceFormat::R16_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
		case FlexKit::DeviceFormat::R16G16_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R16G16_UINT;
		case FlexKit::DeviceFormat::R32_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
		case FlexKit::DeviceFormat::R32G32_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R32G32_UINT;
		case FlexKit::DeviceFormat::R8G8B8A8_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
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
		default:
			return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UINT;
			break;
		}
	}

	
	/************************************************************************************************/


	constexpr DeviceFormat	DXGIFormat2TextureFormat(DXGI_FORMAT F) noexcept
	{
		switch (F)
		{
		case DXGI_FORMAT::DXGI_FORMAT_R16G16_UINT:
			return FlexKit::DeviceFormat::R16G16_UINT;
		case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM:
			return FlexKit::DeviceFormat::R8G8B8A8_UNORM;
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
        case DXGI_FORMAT::DXGI_FORMAT_R32_UINT:
            return FlexKit::DeviceFormat::R32_UINT;

		default:
			return FlexKit::DeviceFormat::UNKNOWN;
			break;
		}
	}


	/************************************************************************************************/


	FLEXKITAPI void _UpdateSubResourceByUploadQueue(RenderSystem* RS, CopyContextHandle uploadHandle, ID3D12Resource* destinationResource, SubResourceUpload_Desc* Desc, D3D12_RESOURCE_STATES EndState)
	{
		auto& copyCtx   = RS->_GetCopyContext(uploadHandle);

		for (size_t I = 0; I < Desc->subResourceCount; ++I)
		{
			const auto region       = copyCtx.Reserve(Desc->buffers[I].Size, 512);

			memcpy(
				(char*)region.buffer,
				(char*)Desc->buffers[I].Buffer,
				Desc->buffers[I].Size);

			copyCtx.CopyTextureRegion(
				destinationResource,
				I,
				{ 0, 0, 0 },
				region,
				Desc->buffers[I].WH,
				Desc->format);
		}

        copyCtx.Barrier(
            destinationResource,
            DRS_Write,
            DRS_GENERIC);
	}


	/************************************************************************************************/


	void RenderSystem::UpdateResourceByUploadQueue(ID3D12Resource* Dest, CopyContextHandle uploadQueue, void* Data, size_t Size, size_t ByteSize, D3D12_RESOURCE_STATES EndState)
	{
		FK_ASSERT(Data);
		FK_ASSERT(Dest);

		auto& copyCtx = _GetCopyContext(uploadQueue);

		const auto reservedSpace = copyCtx.Reserve(Size);

		memcpy(reservedSpace.buffer, Data, Size);

		copyCtx.CopyBuffer(Dest, 0, reservedSpace);
	}


	
	/************************************************************************************************/


	// If uploadQueue is InvalidHandle, pushes temporaries to a different free list
	UploadSegment ReserveUploadBuffer(
		RenderSystem&	    renderSystem,
		const size_t	    uploadSize,
		CopyContextHandle   uploadQueue)
	{
		auto reserve = renderSystem._GetCopyContext(uploadQueue).Reserve(uploadSize, 512);

		return {
			.offset     = reserve.offset,
			.uploadSize = reserve.reserveSize,
			.buffer     = reserve.buffer,
			.resource   = reserve.resource
		};
	}


	/************************************************************************************************/


	void MoveBuffer2UploadBuffer(const UploadSegment& data, byte* source, size_t uploadSize)
	{
		memcpy(data.buffer, source, data.uploadSize > uploadSize ? uploadSize : data.uploadSize);
	}


	/************************************************************************************************/


	void UploadTextureSet(RenderSystem* RS, TextureSet* TS, iAllocator* Memory)
	{
		for (size_t I = 0; I < 16; ++I)
		{
			if (TS->TextureGuids[I]) {
				TS->Loaded[I]	= true;
				TS->Textures[I] = LoadDDSTextureFromFile(TS->TextureLocations[I].Directory, RS, RS->GetImmediateUploadQueue(), Memory);
			}
		}
	}


	/************************************************************************************************/


	void ReleaseTextureSet(TextureSet* TS, iAllocator* Memory)
	{
		for (size_t I = 0; I < 16; ++I)
		{
			if (TS->TextureGuids[I]) {
				TS->Loaded[I] = false;
				FK_ASSERT(0);
				//TS->Textures[I]->Release();
			}
		}
		Memory->free(TS);
	}


	/************************************************************************************************/


	void CreateVertexBuffer(RenderSystem* RS, CopyContextHandle handle, VertexBufferView** Buffers, size_t BufferCount, VertexBuffer& DVB_Out)
	{
		// TODO: Add Buffer Layout Structure for more complex Buffer Layouts
		// TODO: ATM only is able to make Buffers of a single Value
		InputDescription Input_Desc;

		DVB_Out.VertexBuffers.SetFull();

		// Generate Input Layout
		HRESULT	HR = ERROR;
		D3D12_RESOURCE_DESC Resource_DESC	= CD3DX12_RESOURCE_DESC::Buffer(0);
		Resource_DESC.Alignment				= 0;
		Resource_DESC.DepthOrArraySize		= 1;
		Resource_DESC.Dimension				= D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		Resource_DESC.Layout				= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Resource_DESC.Width					= 0;
		Resource_DESC.Height				= 1;
		Resource_DESC.Format				= DXGI_FORMAT_UNKNOWN;
		Resource_DESC.SampleDesc.Count		= 1;
		Resource_DESC.SampleDesc.Quality	= 0;
		Resource_DESC.Flags					= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;

		D3D12_HEAP_PROPERTIES HEAP_Props ={};
		HEAP_Props.CPUPageProperty	    = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type				    = D3D12_HEAP_TYPE_DEFAULT;
		HEAP_Props.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask	    = 0;
		HEAP_Props.VisibleNodeMask		= 0;

		for (uint32_t itr = 0; itr < BufferCount; ++itr)
		{
			if (nullptr != Buffers[itr] && Buffers[itr]->GetBufferType() == VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX)
			{
				// Create the Vertex Buffer
				FK_ASSERT(Buffers[itr]->GetBufferSizeRaw());// ERROR BUFFER EMPTY;
				Resource_DESC.Width	= Buffers[itr]->GetBufferSizeRaw();

				ID3D12Resource* NewBuffer = nullptr;

				HRESULT HR = RS->pDevice->CreateCommittedResource(
					&HEAP_Props, 
					D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, 
					&Resource_DESC, 
					D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, 
					nullptr, 
					IID_PPV_ARGS(&NewBuffer));
			
				if (FAILED(HR))
				{// TODO!
					FK_ASSERT(0);
				}

				RS->UpdateResourceByUploadQueue(
					NewBuffer, 
					handle,
					Buffers[itr]->GetBuffer(),
					Buffers[itr]->GetBufferSizeRaw(), 1,
					D3D12_RESOURCE_STATE_INDEX_BUFFER);

				SETDEBUGNAME(NewBuffer, "INDEXBUFFER");

				DVB_Out.VertexBuffers[itr].Buffer				= NewBuffer;
				DVB_Out.VertexBuffers[itr].BufferSizeInBytes	= Buffers[itr]->GetBufferSizeRaw();
				DVB_Out.VertexBuffers[itr].BufferStride			= Buffers[itr]->GetElementSize();
				DVB_Out.VertexBuffers[itr].Type					= Buffers[itr]->GetBufferType();
				DVB_Out.MD.IndexBuffer_Index					= itr;
			}
			else if (Buffers[itr] && Buffers[itr]->GetBufferSize())
			{
				ID3D12Resource* NewBuffer = nullptr;
				// Create the Vertex Buffer
				FK_ASSERT(Buffers[itr]->GetBufferSizeRaw());// ERROR BUFFER EMPTY;
				Resource_DESC.Width	= Buffers[itr]->GetBufferSizeRaw();

				HRESULT HR = RS->pDevice->CreateCommittedResource(&HEAP_Props, 
									D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &Resource_DESC, 
									D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, 
									nullptr, IID_PPV_ARGS(&NewBuffer));

				if (FAILED(HR))
				{// TODO!
					FK_ASSERT(0);
				}

				switch (Buffers[itr]->GetBufferType())
				{
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION:
					{SETDEBUGNAME(NewBuffer, "VERTEXBUFFER");break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL:
					{SETDEBUGNAME(NewBuffer, "NORMAL BUFFER"); break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT:
					{SETDEBUGNAME(NewBuffer, "TANGET BUFFER"); break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_COLOR:
					{SETDEBUGNAME(NewBuffer, "COLOUR BUFFER"); break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV:
					{SETDEBUGNAME(NewBuffer, "TEXCOORD BUFFER"); break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1:
					{SETDEBUGNAME(NewBuffer, "AnimationWeights"); break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2:
					{SETDEBUGNAME(NewBuffer, "AnimationIndices"); break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_PACKED:
					{SETDEBUGNAME(NewBuffer, "PACKED_BUFFER");break;}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ERROR:
				default:
					{SETDEBUGNAME(NewBuffer, "VERTEXBUFFER_TYPE_ERROR"); break; }
					break;
		}

				RS->UpdateResourceByUploadQueue(
					NewBuffer,
					handle,
					Buffers[itr]->GetBuffer(),
					Buffers[itr]->GetBufferSizeRaw(), 1, 
					D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

			DVB_Out.VertexBuffers[itr].Buffer				= NewBuffer;
			DVB_Out.VertexBuffers[itr].BufferStride			= Buffers[itr]->GetElementSize();
			DVB_Out.VertexBuffers[itr].BufferSizeInBytes	= Buffers[itr]->GetBufferSizeRaw();
			DVB_Out.VertexBuffers[itr].Type					= Buffers[itr]->GetBufferType();
		}
		}
	}
	

	/************************************************************************************************/

	VertexBufferHandle VertexBufferStateTable::CreateVertexBuffer(size_t BufferSize, bool GPUResident, RenderSystem* RS) // Creates Using Placed Resource
	{
		VBufferHandle Buffer[3];

		for(size_t I = 0; I < 3; ++I)
			Buffer[I] = CreateVertexBufferResource(BufferSize, GPUResident, RS);

		const auto handle   = Handles.GetNewHandle();
		Handles[handle]     = UserBuffers.size();

		UserBuffers.push_back(UserVertexBuffer{
			0,
			{ Buffer[0], Buffer[1], Buffer[2] },
			BufferSize,
			0, 
			_Map(Buffer[0]),
			Buffers[Buffer[0]].Resource,
			false,
			handle});

		return handle;
	}


	/************************************************************************************************/


	VertexBufferStateTable::VBufferHandle VertexBufferStateTable::CreateVertexBufferResource(size_t BufferSize, bool GPUResident, RenderSystem* RS)
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

			Buffers[userBuffer.GetCurrentBuffer()].lockCounter = 3;

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
		if(UserBuffer.WrittenTo)
		{	// UnMap Current Buffer
			auto  ResourceIdx = UserBuffer.GetCurrentBuffer();
			D3D12_RANGE Range = { 0, UserBuffer.Offset };
			Buffers[ResourceIdx].Resource->Unmap(0, &Range);
		}
		
		UserBuffer.WrittenTo = false;
		UserBuffer.IncrementCurrentBuffer();

		{	// Map Current Buffer
			auto	ResourceIdx		= UserBuffer.GetCurrentBuffer();
			void*	MappedPtr		= nullptr;

			D3D12_RANGE Range{ 0, 0 };
			Buffers[ResourceIdx].Resource->Map(0, &Range, &MappedPtr);

			UserBuffer.Offset		= 0;
			UserBuffer.MappedPtr	= (char*)MappedPtr;
			UserBuffer.Resource		= Buffers[ResourceIdx].Resource;
		}
	}


	/************************************************************************************************/


	ID3D12Resource* VertexBufferStateTable::GetAsset(VertexBufferHandle Handle)
	{
		return Buffers[UserBuffers[Handles[Handle]].GetCurrentBuffer()].Resource;
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


	VertexBufferStateTable::SubAllocation VertexBufferStateTable::Reserve(VertexBufferHandle Handle, size_t size) noexcept
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


	bool VertexBufferStateTable::CurrentlyAvailable(VertexBufferHandle Handle, size_t CurrentFrame) const
	{
		auto UserIdx	= Handles[Handle];
		auto BufferIdx	= UserBuffers[UserIdx].GetCurrentBuffer();

		return Buffers[Handle].lockCounter == 0;
	}


	/************************************************************************************************/


	char* VertexBufferStateTable::_Map(VBufferHandle handle)
	{
		void* ptr;
		Buffers[handle].Resource->Map(0, nullptr, &ptr);

		return (char*)ptr;
	}


	void VertexBufferStateTable::_UnMap(VBufferHandle handle, size_t range)
	{
		D3D12_RANGE writtenRange;
		writtenRange.Begin  = 0;
		writtenRange.End    = range;

		Buffers[handle].Resource->Unmap(0, &writtenRange);
	}


	/************************************************************************************************/


	void VertexBufferStateTable::Release()
	{
		for (auto& B : Buffers) {
			if(B.Resource)
				B.Resource->Release();
			B.Resource = nullptr;
		}


		Buffers.Release();
		UserBuffers.Release();
		FreeBuffers.Release();
		Handles.Release();
	}


	/************************************************************************************************/


	void VertexBufferStateTable::ReleaseVertexBuffer(VertexBufferHandle handle)
	{
		auto userIdx		= Handles[handle];
		auto& userEntry     = UserBuffers[userIdx];

		std::scoped_lock(criticalSection);

		for (const auto ResourceIdx : userEntry.Buffers)
			if(ResourceIdx != INVALIDHANDLE)
				FreeBuffers.push_back({ 3, ResourceIdx});
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


	void VertexBufferStateTable::ReleaseFree()
	{
		std::scoped_lock(criticalSection);

		for (auto freeBuffer : FreeBuffers)
		{
			if (Buffers[freeBuffer.BufferIdx].lockCounter <= 0)
				Buffers[freeBuffer.BufferIdx].Resource->Release();

			Buffers[freeBuffer.BufferIdx].lockCounter--;
		}

		std::sort(FreeBuffers.begin(), FreeBuffers.end());

		FreeBuffers.erase(
			std::remove_if(
				FreeBuffers.begin(),
				FreeBuffers.end(),
				[&](auto buffer)-> bool
				{
					return Buffers[buffer].lockCounter <= 0;
				}),
			FreeBuffers.end());
	}


	/************************************************************************************************/


	QueryHandle QueryTable::CreateQueryBuffer(size_t count, QueryType type)
	{
		D3D12_QUERY_HEAP_DESC desc;
		desc.Count			= count;
		desc.NodeMask		= 0;

		ResourceEntry newResEntry = { 0 };

		switch (type)
		{
		case QueryType::OcclusionQuery:
			desc.Type			= D3D12_QUERY_HEAP_TYPE_OCCLUSION;
			newResEntry.type	= D3D12_QUERY_TYPE_OCCLUSION;
			break;
		case QueryType::PipelineStats:
			desc.Type			= D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS;
			newResEntry.type	= D3D12_QUERY_TYPE_PIPELINE_STATISTICS;
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
		
		heapDesc.Count	= count;
		heapDesc.Type	= static_cast<D3D12_QUERY_HEAP_TYPE>(D3D12_QUERY_HEAP_TYPE_SO_STATISTICS);

		ResourceEntry newResEntry	= { 0 };
		newResEntry.currentResource = 0;
		newResEntry.type			= static_cast<D3D12_QUERY_TYPE>(D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0 + SOIndex);

		for (size_t itr = 0; itr < 3; ++itr)
		{
			newResEntry.resourceState[itr] = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
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


	void TextureStateTable::Release()
	{
		for (size_t I = 0; I < UserEntries.size(); ++I)
		{
			auto& T = UserEntries[I];
			auto F  = T.Flags;

			Resources[T.ResourceIdx].Release();
		}

		UserEntries.Release();
		Resources.Release();
		Handles.Clear();
	}


	/************************************************************************************************/


	ResourceHandle TextureStateTable::AddResource(const GPUResourceDesc& Desc, const DeviceResourceState InitialState)
	{
		auto Handle		 = Handles.GetNewHandle();

		ResourceEntry NewEntry = 
		{	
			Desc.bufferCount,
			0,
			{ nullptr, nullptr, nullptr },
			{ 0, 0, 0 },
			{ InitialState, InitialState , InitialState },
			TextureFormat2DXGIFormat(Desc.format),
			Desc.MipLevels,
			Desc.WH,
			Handle
		};

		for (size_t I = 0; I < Desc.bufferCount; ++I)
			NewEntry.Resources[I] = Desc.resources[I];

		UserEntry Entry;
		Entry.ResourceIdx		= Resources.push_back(NewEntry);
		Entry.FrameGraphIndex	= -1;
		Entry.FGI_FrameStamp    = -1;
		Entry.Handle            = Handle;
		Entry.Format			= NewEntry.Format;
		Entry.Flags             = Desc.backBuffer ? TF_BackBuffer : 0;
		Entry.dimension         = Desc.Dimensions;
		Entry.arraySize         = Desc.arraySize;
        Entry.tileMappings      = { allocator };

		Handles[Handle]         = UserEntries.push_back(Entry);

		if (Desc.buffered && Desc.bufferCount > 1)
			BufferedResources.push_back(Handle);

		return Handle;
	}
	

	/************************************************************************************************/


	void TextureStateTable::ReleaseTexture(ResourceHandle Handle)
	{
		const auto UserIdx	= Handles[Handle];
		auto& UserEntry		= UserEntries[UserIdx];
		const auto ResIdx	= UserEntry.ResourceIdx;
		auto& resource		= Resources[ResIdx];

		for(auto res : resource.Resources)
			if(res)
				delayRelease.push_back({ res, 3 });

		const auto TempHandle	= UserEntries.back().Handle;
		UserEntry			    = UserEntries.back();
		Handles[TempHandle]     = UserIdx;

		const auto temp         = Resources[UserEntry.ResourceIdx];
		Resources[ResIdx]       = Resources.back();
		UserEntries[Handles[resource.owner]].ResourceIdx = ResIdx;

		UserEntries.pop_back();
		Resources.pop_back();

		Handles.RemoveHandle(Handle);
	}


	/************************************************************************************************/


	void TextureStateTable::_ReleaseTextureForceRelease(ResourceHandle Handle)
	{
		auto  UserIdx		= Handles[Handle];
		auto& UserEntry		= UserEntries[UserIdx];
		const auto ResIdx	= UserEntry.ResourceIdx;
		auto& resource		= Resources[ResIdx];

        for (auto res : resource.Resources)
            res->Release();

		const auto TempHandle	= UserEntries.back().Handle;
		UserEntry			    = UserEntries.back();
		Handles[TempHandle]     = UserIdx;

		const auto temp         = Resources[UserEntry.ResourceIdx];
		Resources[ResIdx]       = Resources.back();
		UserEntries[Handles[resource.owner]].ResourceIdx = ResIdx;

		UserEntries.pop_back();
		Resources.pop_back();

		Handles.RemoveHandle(Handle);
	}


	/************************************************************************************************/


	Texture2D TextureStateTable::operator[](ResourceHandle Handle)
	{
		const auto Idx		    = Handles[Handle];
		const auto resource     = Resources[UserEntries[Idx].ResourceIdx];

		const auto Res		    = resource.GetAsset();
		const auto WH			= resource.WH;
		const auto Format		= resource.Format;
		const auto mipCount	    = resource.mipCount;

		return { Res, WH, mipCount, Format };
	}


	/************************************************************************************************/


	void TextureStateTable::SetState(ResourceHandle Handle, DeviceResourceState State)
	{
		auto UserIdx = Handles[Handle];
		auto ResIdx  = UserEntries[UserIdx].ResourceIdx;

		Resources[ResIdx].SetState(State);
	}


	/************************************************************************************************/


	uint32_t TextureStateTable::GetTag(ResourceHandle Handle) const
	{
		auto UserIdx = Handles[Handle];
		return UserEntries[UserIdx].Tag;
	}


	/************************************************************************************************/


	void TextureStateTable::SetTag(ResourceHandle Handle, uint32_t Tag)
	{
		auto UserIdx = Handles[Handle];
		UserEntries[UserIdx].Tag = Tag;
	}


	/************************************************************************************************/


	void TextureStateTable::SetBufferedIdx(ResourceHandle handle, uint32_t idx)
	{
		auto UserIdx = Handles[handle];
		auto Residx  = UserEntries[UserIdx].ResourceIdx;

		Resources[Residx].CurrentResource = idx;
	}


	/************************************************************************************************/


	void TextureStateTable::SetDebugName(ResourceHandle handle, const char* str)
	{
		auto UserIdx    = Handles[handle];
		auto resIdx     = UserEntries[UserIdx].ResourceIdx;
		auto& resource  = Resources[resIdx];

		for (auto& res : resource.Resources)
			if (res)
				SETDEBUGNAME(res, str);
	}


	/************************************************************************************************/


	size_t TextureStateTable::GetFrameGraphIndex(ResourceHandle Handle, size_t FrameID) const
	{
		auto UserIdx	= Handles[Handle];
		auto FrameStamp	= UserEntries[UserIdx].FGI_FrameStamp;

		if (FrameStamp < int(FrameID))
			return INVALIDHANDLE;

		return UserEntries[UserIdx].FrameGraphIndex;
	}


	/************************************************************************************************/


	void TextureStateTable::SetFrameGraphIndex(ResourceHandle Handle, size_t FrameID, size_t Index)
	{
		auto UserIdx = Handles[Handle];
		UserEntries[UserIdx].FGI_FrameStamp		= FrameID;
		UserEntries[UserIdx].FrameGraphIndex	= Index;
	}


	/************************************************************************************************/


	DXGI_FORMAT TextureStateTable::GetFormat(ResourceHandle handle) const
	{
		auto UserIdx = Handles[handle];
		return UserEntries[UserIdx].Format;
	}


	/************************************************************************************************/


	TextureDimension TextureStateTable::GetDimension(ResourceHandle handle) const
	{
		auto UserIdx = Handles[handle];
		return UserEntries[UserIdx].dimension;
	}


	/************************************************************************************************/


	size_t TextureStateTable::GetArraySize(ResourceHandle handle) const
	{
		auto UserIdx = Handles[handle];
		return UserEntries[UserIdx].arraySize;
	}


    /************************************************************************************************/


    uint32_t TextureStateTable::GetMIPCount(ResourceHandle handle) const
    {
        auto UserIdx = Handles[handle];
        return Resources[UserEntries[UserIdx].ResourceIdx].mipCount;
    }


	/************************************************************************************************/


	uint2 TextureStateTable::GetWH(ResourceHandle Handle) const
	{
		auto UserIdx = Handles[Handle];
		return Resources[UserEntries[UserIdx].ResourceIdx].WH;
	}


	/************************************************************************************************/


    void TextureStateTable::UpdateTileMappings(ResourceHandle handle, const TileMapping* begin, const TileMapping* end)
    {
        if (handle == InvalidHandle_t)
            return;

        auto itr        = begin;
        auto UserIdx    = Handles[handle];
        auto& mappings  = UserEntries[UserIdx].tileMappings;


        while (itr < end)
        {
            auto res = std::find_if(
                std::begin(mappings),
                std::end(mappings),
                [&](const TileMapping& e)
                {
                    return itr->tileID == e.tileID;
                });

            if (res != std::end(mappings))
            {
                res->state = TileMapState::Updated;
            }
            else
            {
                TileMapping mapping = *itr;
                mapping.state = TileMapState::Updated;
                mappings.push_back(mapping);
            }
            ++itr;
        }

        for (auto& mappings : mappings)
            if (mappings.state == TileMapState::InUse)
                mappings.state = TileMapState::Stale;
    }


    /************************************************************************************************/


    void TextureStateTable::SubmitTileUpdates(ID3D12CommandQueue* queue, RenderSystem& renderSystem, iAllocator* allocator_temp)
    {
        Vector<D3D12_TILED_RESOURCE_COORDINATE> coordinates { allocator_temp };
        Vector<D3D12_TILE_REGION_SIZE>          regionSize  { allocator_temp };
        Vector<D3D12_TILE_RANGE_FLAGS>          tile_flags  { allocator_temp };
        Vector<UINT>                            heapOffsets { allocator_temp };
        Vector<UINT>                            tileCounts  { allocator_temp };

        for (auto& userEntry : UserEntries)
        {
            coordinates.size();
            regionSize.size();

            ID3D12Resource* resource    = nullptr;
            ID3D12Heap*     heap        = nullptr;

            for (const auto& mapping: userEntry.tileMappings)
            {
                auto desc = resource->GetDesc();
                
                if (auto mappingHeap = renderSystem.GetDeviceResource(mapping.heap); heap != mappingHeap)
                {
                    if (mappingHeap)
                        queue->UpdateTileMappings(
                            resource,
                            coordinates.size(),
                            coordinates.data(),
                            regionSize.data(),
                            heap,
                            tile_flags.size(),
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
                            .X = mapping.tileID.GetTileX(),
                            .Y = mapping.tileID.GetTileY(),
                            .Z = 0,
                            .Subresource = (UINT)mapping.tileID.GetMipLevel(desc.MipLevels)
                        });

                    regionSize.push_back(
                        D3D12_TILE_REGION_SIZE{
                            .NumTiles = 1,
                            .UseBox = true,
                            .Width = 1,
                            .Height = 1,
                            .Depth = 1,
                        });

                    tile_flags.push_back(D3D12_TILE_RANGE_FLAGS::D3D12_TILE_RANGE_FLAG_NONE);
                    heapOffsets.push_back(mapping.heapOffset);
                    tileCounts.push_back(1);
                }
            }
        }
    }


    /************************************************************************************************/


    const TileMapList& TextureStateTable::GetTileMapping(const ResourceHandle handle) const
    {
        auto UserIdx    = Handles[handle];
        return UserEntries[UserIdx].tileMappings;
    }


    /************************************************************************************************/


	void TextureStateTable::MarkRTUsed(ResourceHandle Handle)
	{
		auto UserIdx = Handles[Handle];
		UserEntries[UserIdx].Flags |= TF_INUSE;
	}


	/************************************************************************************************/


	void TextureStateTable::LockUntil(size_t FrameID)
	{
		for (auto bufferedResource : BufferedResources)
		{
			const auto idx  = Handles[bufferedResource];
			auto& UserEntry = UserEntries[idx];
			auto Flags      = UserEntry.Flags;

			if (Flags & TF_INUSE && !(Flags & TF_BackBuffer))
			{
				Resources[UserEntry.ResourceIdx].SetFrameLock(FrameID);
				Resources[UserEntry.ResourceIdx].IncreaseIdx();
			}
		}
	}


	/************************************************************************************************/


	void TextureStateTable::UpdateLocks()
	{
		for (auto& res : delayRelease)
		{
			res.counter--;
			if (!res.counter)
				res.resource->Release();
		}

		delayRelease.erase(
			std::remove_if(
				delayRelease.begin(),
				delayRelease.end(),
				[](auto& res)
				{
					return res.counter == 0;
				}),

			delayRelease.end());
	}


	/************************************************************************************************/


	void TextureStateTable::ResourceEntry::Release()
	{
		for (auto& R : Resources)
		{
			if (R)
				R->Release();

			R = nullptr;
		}
	}


	/************************************************************************************************/


	DeviceResourceState TextureStateTable::GetState(ResourceHandle Handle) const
	{
		auto    Idx			    = Handles[Handle];
		auto    ResourceIdx	    = UserEntries[Idx].ResourceIdx;
		const   auto& resources = Resources[ResourceIdx];

		return resources.States[resources.CurrentResource];
	}


	/************************************************************************************************/


	ID3D12Resource*		TextureStateTable::GetAsset(ResourceHandle Handle) const
	{
		auto  Idx			= Handles[Handle];
		auto  ResourceIdx	= UserEntries[Idx].ResourceIdx;
		auto& resources     = Resources[ResourceIdx].Resources;
		auto  currentIdx    = Resources[ResourceIdx].CurrentResource;

		return resources[currentIdx];
	}


    /************************************************************************************************/


    void TextureStateTable::ReplaceResources(ResourceHandle handle, ID3D12Resource** begin, ID3D12Resource** end)
    {
        auto  Idx                   = Handles[handle];
        auto  ResourceIdx           = UserEntries[Idx].ResourceIdx;
        const auto resourceCount    = Resources[ResourceIdx].ResourceCount;
        auto& resources             = Resources[ResourceIdx].Resources;

        for (size_t I = 0; I < resourceCount && begin + I < end; ++I)
            resources[I] = *(begin + I);
    }


	/************************************************************************************************/


	void InitiateGeometryTable(iAllocator* Memory)
	{
		GeometryTable.Handles.Initiate(Memory);
		GeometryTable.Handle			= Vector<TriMeshHandle>(Memory);
		GeometryTable.Geometry			= Vector<TriMesh>(Memory);
		GeometryTable.ReferenceCounts	= Vector<size_t>(Memory);
		GeometryTable.Guids				= Vector<GUID_t>(Memory);
		GeometryTable.GeometryIDs		= Vector<const char*>(Memory);
		GeometryTable.FreeList			= Vector<size_t>(Memory);
		GeometryTable.Memory			= Memory;
	}


	/************************************************************************************************/


	void ReleaseGeometryTable()
	{
		for (auto G : GeometryTable.Geometry)
			ReleaseTriMesh(&G);

		GeometryTable.Geometry.Release();
		GeometryTable.ReferenceCounts.Release();
		GeometryTable.Guids.Release();
		GeometryTable.GeometryIDs.Release();
		GeometryTable.Handles.Release();
		GeometryTable.FreeList.Release();
		GeometryTable.Handle.Release();
	}


	/************************************************************************************************/


	bool IsMeshLoaded(GUID_t guid)
	{
		bool res = false;
		for (auto Entry : GeometryTable.Guids)
		{
			if (Entry == guid){
				res = true;
				break;
			}
		}

		return res;
	}


	/************************************************************************************************/


	void AddRef(TriMeshHandle TMHandle)
	{
		size_t Index = GeometryTable.Handles[TMHandle];

#ifdef _DEBUG
		if (Index != -1)
			GeometryTable.ReferenceCounts[Index]++;
#else
		GeometryTable.ReferenceCounts[Index]++;
#endif
	}


	/************************************************************************************************/


	void ReleaseMesh(RenderSystem* RS, TriMeshHandle TMHandle)
	{
		// TODO: MAKE ATOMIC
		if (GeometryTable.Handles[TMHandle] == -1)
			return;// Already Released

		size_t Index	= GeometryTable.Handles[TMHandle];
		auto Count		= --GeometryTable.ReferenceCounts[Index];

		if (Count == 0) 
		{
			auto G = GetMeshResource(TMHandle);

			DelayedReleaseTriMesh(RS, G);

			if (G->Skeleton)
				CleanUpSkeleton(G->Skeleton);

			GeometryTable.FreeList.push_back(Index);
			GeometryTable.Geometry[Index]   = TriMesh();
			GeometryTable.Handles[TMHandle] = -1;
		}
	}


	/************************************************************************************************/


	TriMeshHandle GetMesh(RenderSystem* rs, GUID_t guid)
	{
		if (IsMeshLoaded(guid))
		{
			auto [mesh, result] = FindMesh(guid);

			if(result)
				return mesh;
		}

		TriMeshHandle triMesh = LoadTriMeshIntoTable(rs, rs->GetImmediateUploadQueue(), guid);

		return triMesh;
	}


	/************************************************************************************************/


	TriMeshHandle GetMesh(RenderSystem* rs, const char* meshID)
	{
		auto [mesh, result] = FindMesh(meshID);

		if(result)
			return mesh;

		return LoadTriMeshIntoTable(rs, rs->GetImmediateUploadQueue(), meshID);
	}


	/************************************************************************************************/


	TriMesh* GetMeshResource(TriMeshHandle TMHandle){
		FK_ASSERT(TMHandle != InvalidHandle_t);

		return &GeometryTable.Geometry[GeometryTable.Handles[TMHandle]];
	}
	

	/************************************************************************************************/


	BoundingSphere GetMeshBoundingSphere(TriMeshHandle TMHandle)
	{
		auto Mesh = &GeometryTable.Geometry[GeometryTable.Handles[TMHandle]];
		return float4{ float3{0}, Mesh->Info.r };
	}


	/************************************************************************************************/


	Skeleton* GetSkeleton(TriMeshHandle TMHandle){
		return GetMeshResource(TMHandle)->Skeleton;
	}


	/************************************************************************************************/


	size_t	GetSkeletonGUID(TriMeshHandle TMHandle){
		return GetMeshResource(TMHandle)->SkeletonGUID;
	}


	/************************************************************************************************/


	void SetSkeleton(TriMeshHandle TMHandle, Skeleton* S){
		GetMeshResource(TMHandle)->Skeleton = S;
	}


	/************************************************************************************************/


	bool IsSkeletonLoaded(TriMeshHandle guid){
		return (GetMeshResource(guid)->Skeleton != nullptr);
	}


	/************************************************************************************************/


	bool HasAnimationData(TriMeshHandle RMeshHandle){
		return GetSkeleton(RMeshHandle)->Animations != nullptr;
	}



	/************************************************************************************************/


	Pair<TriMeshHandle, bool>	FindMesh(GUID_t guid)
	{
		size_t location		= 0;
		size_t HandleIndex	= 0;
		for (auto Entry : GeometryTable.Guids)
		{
			if (Entry == guid) {
				for ( auto index : GeometryTable.Handles.Indexes)
				{
					if (index == location) {
						return { TriMeshHandle(HandleIndex, GeometryTable.Handles.mType, 0x04), true };
						break;
					}
					++HandleIndex;
				}
				break;
			}
			++location;
		}

		return { InvalidHandle_t, false };
	}
	

	/************************************************************************************************/


	Pair<TriMeshHandle, bool>	FindMesh(const char* ID)
	{
		TriMeshHandle HandleOut = InvalidHandle_t;
		size_t location			= 0;
		size_t HandleIndex		= 0;

		for (auto Entry : GeometryTable.GeometryIDs)
		{
			if (!strncmp(Entry, ID, 64)) {
				for (auto index : GeometryTable.Handles.Indexes)
				{
					if (index == location) {
						HandleOut.INDEX = HandleIndex;
						break;
					}
					++HandleIndex;
				}
				break;
			}
			++location;
		}

		return{ HandleOut, 0 };
	}


	/************************************************************************************************/


	VertexResourceBuffer RenderSystem::_CreateVertexBufferDeviceResource(const size_t ResourceSize, bool GPUResident)
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


	ConstantBuffer RenderSystem::_CreateConstantBufferResource(RenderSystem* RS, ConstantBuffer_desc* desc)
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


	void RenderSystem::_PushDelayReleasedResource(ID3D12Resource* resource, CopyContextHandle uploadQueue)
	{
		if (uploadQueue != InvalidHandle_t)
			copyEngine.Push_Temporary(resource, uploadQueue);
		else
			FreeList.push_back({ resource, 30 });
	}


	/************************************************************************************************/


	void RenderSystem::_ForceReleaseTexture(ResourceHandle handle)
	{
		Textures._ReleaseTextureForceRelease(handle);
	}


	/************************************************************************************************/


	void RenderSystem::_InsertBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES currentState, D3D12_RESOURCE_STATES endState)
	{
		PendingBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(resource, currentState, endState, 0));
	}


	/************************************************************************************************/


	size_t RenderSystem::_GetVidMemUsage()
	{
		DXGI_QUERY_VIDEO_MEMORY_INFO VideoMemInfo = {0};
		if(pDXGIAdapter)
			pDXGIAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &VideoMemInfo);

		return VideoMemInfo.CurrentUsage;
	}


	/************************************************************************************************/


	ID3D12QueryHeap* RenderSystem::_GetQueryResource(QueryHandle Handle)
	{
		return Queries.GetAsset(Handle);
	}


	/************************************************************************************************/


	CopyContext& RenderSystem::_GetCopyContext(CopyContextHandle handle)
	{
		if (handle == InvalidHandle_t)
		{
			if (ImmediateUpload == InvalidHandle_t)
				ImmediateUpload = OpenUploadQueue();

			handle = ImmediateUpload;
		}

		return copyEngine[handle];
	}


    /************************************************************************************************/


    void RenderSystem::_OnCrash()
    {
#if USING(AFTERMATH)
        std::vector<GFSDK_Aftermath_ContextHandle> context_handles;
        std::vector<GFSDK_Aftermath_ContextData> context_crash_info{ Contexts.size() };

        for (auto& context : Contexts)
            context_handles.push_back(context.AFTERMATH_context);

        GFSDK_Aftermath_Device_Status           device_status;
        GFSDK_Aftermath_PageFaultInformation    pafeFault;

        GFSDK_Aftermath_GetDeviceStatus(&device_status);
        GFSDK_Aftermath_GetData(context_handles.size(), context_handles.data(), context_crash_info.data());
        GFSDK_Aftermath_GetPageFaultInformation(&pafeFault);

        for (auto& context : Contexts)
            GFSDK_Aftermath_ReleaseContextHandle(context.AFTERMATH_context);

        Sleep(3000); // Aftermath needs a moment to do it's crash dump

        __debugbreak();
        exit(-1);
#endif
    }


	/************************************************************************************************/


	Context& RenderSystem::GetCommandList()
	{
        const size_t completedCounter = Fence->GetCompletedValue();

		for(auto& _ : Contexts)
		{
			const size_t idx = contextIdx;
			contextIdx = ++contextIdx % Contexts.size();

			Context& context = Contexts[idx];
			if (context._GetCounter() <= completedCounter)
				return context.Reset();
		}

		FK_ASSERT(0, "Failed to get a free context!");

		return Contexts[0];
	}


	/************************************************************************************************/


	void RenderSystem::BeginSubmission()
	{
        const auto pendingFrame = pendingFrames[(frameIdx) % 3];

        if (pendingFrame > Fence->GetCompletedValue())
        {
            HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS); FK_ASSERT(eventHandle != 0);
            Fence->SetEventOnCompletion(pendingFrame, eventHandle);
            WaitForSingleObject(eventHandle, INFINITE);
            CloseHandle(eventHandle);
        }
	}


	/************************************************************************************************/


	void RenderSystem::Submit(Vector<Context*>& contexts)
	{
		if (ImmediateUpload != InvalidHandle_t)
		{
			SubmitUploadQueues(SYNC_Graphics, &ImmediateUpload);
			ImmediateUpload = InvalidHandle_t;
		}

        const auto counter = ++FenceCounter;

		static_vector<ID3D12CommandList*> cls;
		for (auto context : contexts)
        {
			cls.push_back(context->GetCommandList());
			context->Close(counter);
		}

		GraphicsQueue->ExecuteCommandLists(cls.size(), cls.begin());
		GraphicsQueue->Signal(Fence, counter);

        for (auto context : contexts)
            context->_QueueReadBacks();

		VertexBuffers.LockUntil(GetCurrentFrame() + 2);
		ConstantBuffers.LockFor(2);
		Textures.LockUntil(GetCurrentFrame() + 2);

        pendingFrames[frameIdx] = counter;
        frameIdx = ++frameIdx % 3;

        ReadBackTable.Update();
	}


	/************************************************************************************************/


	void RenderSystem::SubmitUploadQueues(uint32_t flags, CopyContextHandle* handles, size_t count)
	{
		copyEngine.Submit(handles, handles + count);

        if (SYNC_Graphics & flags)
        {
            auto HR = GraphicsQueue->Wait(copyEngine.fence, copyEngine.counter);  FK_ASSERT(SUCCEEDED(HR));
        }

        if (SYNC_Compute & flags)
        {
        }

	}


	/************************************************************************************************/


	CopyContextHandle RenderSystem::OpenUploadQueue()
	{
		return copyEngine.Open();
	}


	/************************************************************************************************/


	CopyContextHandle RenderSystem::GetImmediateUploadQueue()
	{
		if (ImmediateUpload == InvalidHandle_t)
			ImmediateUpload = copyEngine.Open();

		return ImmediateUpload;
	}


	/************************************************************************************************/
	

	bool CreateInputLayout(RenderSystem* RS, VertexBufferView** Buffers, size_t count, Shader* Shader, VertexBuffer* DVB_Out)
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
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION:
				{
					switch (Buffers[itr]->GetBufferFormat())
					{
					case VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R32G32B32_FLOAT;
						InputElementDesc.InputSlot            = static_cast<UINT>(++InputSlot);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = ++POS_Buffer_Counter;
						InputElementDesc.SemanticName         = "POSITION";

						Input_Desc.push_back(InputElementDesc);
					}
					break;
					default:
						FK_ASSERT(0);
					}
					break;
				}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX:
					break;
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_COLOR:
				{
					switch (Buffers[itr]->GetBufferFormat())
					{
					case VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32A32:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R32G32B32A32_FLOAT;
						InputElementDesc.InputSlot            = static_cast<UINT>(InputSlot++);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = COLOR_Buffer_Counter++;
						InputElementDesc.SemanticName         = "COLOR";

						Input_Desc.push_back(InputElementDesc);
					}
					break;
					default:
						FK_ASSERT(0);
					}
					break;
				}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL:
				{
					switch (Buffers[itr]->GetBufferFormat())
					{
					case VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R32G32B32_FLOAT;
						InputElementDesc.InputSlot            = static_cast<UINT>(InputSlot++);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = Normal_Buffer_Counter++;
						InputElementDesc.SemanticName         = "NORMAL";

						Input_Desc.push_back(InputElementDesc);
					}
					break;
					default:
						FK_ASSERT(0);
					}
					break;
				}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT:
				{
					switch (Buffers[itr]->GetBufferFormat())
					{
					case VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R32G32B32_FLOAT;
						InputElementDesc.InputSlot            = static_cast<UINT>(InputSlot++);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = Tangent_Buffer_Counter++;
						InputElementDesc.SemanticName         = "TANGENT";

						Input_Desc.push_back(InputElementDesc);
					}
					break;
					default:
						FK_ASSERT(0);
					}
					break;
				}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV:
				{
					switch (Buffers[itr]->GetBufferFormat())
					{
					case VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R32G32_FLOAT;
						InputElementDesc.InputSlot            = static_cast<UINT>(InputSlot++);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = UV_Buffer_Counter;
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
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1:
				{
					switch (Buffers[itr]->GetBufferFormat())
					{
					case VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R32G32B32_FLOAT;
						InputElementDesc.InputSlot            = static_cast<UINT>(InputSlot++);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = WEIGHT_Buffer_Counter;
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
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2:
				{
					switch (Buffers[itr]->GetBufferFormat())
					{
					case VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32A32:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R32G32B32A32_UINT;
						InputElementDesc.InputSlot            = static_cast<UINT>(InputSlot++);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = WINDICES_Buffer_Counter++;
						InputElementDesc.SemanticName         = "WEIGHTINDICES";

						Input_Desc.push_back(InputElementDesc);
					}
					case VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R16G16B16A16:
					{
						D3D12_INPUT_ELEMENT_DESC InputElementDesc;
						InputElementDesc.AlignedByteOffset    = 0;
						InputElementDesc.Format               = ::DXGI_FORMAT_R16G16B16A16_UINT;
						InputElementDesc.InputSlot            = static_cast<UINT>(InputSlot++);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = WINDICES_Buffer_Counter++;
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
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_PACKED:
				{
					D3D12_INPUT_ELEMENT_DESC InputElementDesc;
					InputElementDesc.AlignedByteOffset		= 0;
					InputElementDesc.Format					= ::DXGI_FORMAT_R32G32B32_FLOAT;
					InputElementDesc.InputSlot				= static_cast<UINT>(InputSlot++);
					InputElementDesc.InputSlotClass			= D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
					InputElementDesc.InstanceDataStepRate	= 0;

					InputElementDesc.SemanticIndex			= POS_Buffer_Counter++;
					InputElementDesc.SemanticName			= "POSITION";
					Input_Desc.push_back(InputElementDesc);
					InputElementDesc.AlignedByteOffset		= 16;
					InputElementDesc.SemanticName			= "NORMAL";
					InputElementDesc.SemanticIndex			= Normal_Buffer_Counter++;
					Input_Desc.push_back(InputElementDesc);
					InputElementDesc.AlignedByteOffset		= 32;
					InputElementDesc.SemanticName			= "TANGENT";
					InputElementDesc.SemanticIndex			= Tangent_Buffer_Counter++;
					Input_Desc.push_back(InputElementDesc);
					InputElementDesc.AlignedByteOffset		= 48;
					InputElementDesc.SemanticName			= "TEXCOORD";
					InputElementDesc.SemanticIndex			= UV_Buffer_Counter++;
					InputElementDesc.Format					= ::DXGI_FORMAT_R32G32_FLOAT;
					Input_Desc.push_back(InputElementDesc);
				}	break;
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_PACKEDANIMATION:
				{
					D3D12_INPUT_ELEMENT_DESC InputElementDesc;
					InputElementDesc.AlignedByteOffset		= 0;
					InputElementDesc.Format					= ::DXGI_FORMAT_R32G32B32_FLOAT;
					InputElementDesc.InputSlot				= static_cast<UINT>(InputSlot++);
					InputElementDesc.InputSlotClass			= D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
					InputElementDesc.InstanceDataStepRate	= 0;

					InputElementDesc.SemanticIndex			= WEIGHT_Buffer_Counter++;
					InputElementDesc.SemanticName			= "WEIGHTS";
					Input_Desc.push_back(InputElementDesc);

					InputElementDesc.AlignedByteOffset		= 12;
					InputElementDesc.SemanticIndex			= WINDICES_Buffer_Counter++;
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
	

	/************************************************************************************************/
	

	void Release(VertexBuffer* VertexBuffer)
	{	
		for (auto Buffer : VertexBuffer->VertexBuffers) {
			if (Buffer.Buffer)
				Buffer.Buffer->Release();

			Buffer.Buffer = nullptr;
			Buffer.BufferSizeInBytes = 0;
		}
	}
	

	void DelayedRelease(RenderSystem* RS, VertexBuffer* VertexBuffer)
	{
		for (auto& Buffer : VertexBuffer->VertexBuffers) {
			if (Buffer.Buffer)
				Push_DelayedRelease(RS, Buffer.Buffer);
			Buffer.Buffer				= nullptr;
			Buffer.BufferSizeInBytes	= 0;
		}
	}

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
	

	void Release( Shader* shader)
	{
		if( shader->Blob )
			shader->Blob->Release();
	}
	

	/************************************************************************************************/
	

	bool CompileShader( char* shader, size_t length, ShaderDesc* desc, Shader* out )
	{
		ID3DBlob* ShaderBlob	= nullptr;
		ID3DBlob* ErrorBlob		= nullptr;

		DWORD dwShaderFlags = 0;
#if USING( DEBUGGRAPHICS )
		dwShaderFlags |= D3DCOMPILE_PREFER_FLOW_CONTROL | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_ENABLE_STRICTNESS;
#endif
		HRESULT HR = D3DCompile(
			shader,
			length,
			nullptr, nullptr, nullptr,
			desc->entry,
			desc->shaderVersion,
			dwShaderFlags, 0,
			&ShaderBlob,
			&ErrorBlob );

		if( FAILED( HR ) )
		{
			std::cout << (char*)ErrorBlob->GetBufferPointer();
			ErrorBlob->Release();
			return false;
		}
		out->Blob = ShaderBlob;
		out->Type = desc->ShaderType;
		return SUCCEEDED(HR);
	}


	/************************************************************************************************/


	bool LoadAndCompileShaderFromFile(const char* FileLoc, ShaderDesc* desc, Shader* out )
	{
		size_t ConvertCount = 0;
		wchar_t WString[256];
		mbstowcs_s(&ConvertCount, WString, FileLoc, 128);
		ID3DBlob* NewBlob   = nullptr;
		ID3DBlob* Errors    = nullptr;
		DWORD dwShaderFlags = 0;

#if USING( DEBUGGRAPHICS )
		dwShaderFlags |= D3DCOMPILE_PREFER_FLOW_CONTROL | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_ENABLE_STRICTNESS;
#endif

		HRESULT HR = D3DCompileFromFile(WString, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, desc->entry, desc->shaderVersion, dwShaderFlags, 0, &NewBlob, &Errors);
		if (FAILED(HR))	{
			FK_LOG_ERROR((char*)Errors->GetBufferPointer());
			std::cout << (char*)Errors->GetBufferPointer();
			return false;
		}

		out->Blob = NewBlob;
		out->Type = desc->ShaderType;

		return true;
	}


	/************************************************************************************************/


	Shader LoadShader(const char* Entry, const char* ID, const char* ShaderVersion, const char* File)
	{
		Shader Shader;

		bool res = false;
		FlexKit::ShaderDesc SDesc;
		strncpy_s(SDesc.entry, Entry, 128);
		strncpy_s(SDesc.ID, ID, 128);
		strncpy_s(SDesc.shaderVersion, ShaderVersion, 16);

		do
		{
			FK_LOG_2("LoadingShader - %s - \n", Entry);
			res = LoadAndCompileShaderFromFile(File, &SDesc, &Shader);
#if USING( EDITSHADERCONTINUE )
			if (!res)
			{
				std::cout << "Failed to Compile Shader\n Press Enter to try again\n";
				char str[100];
				std::cin >> str;
			}
#else
			FK_ASSERT(res);
#endif
			if (res)
				return Shader;

		} while (!res);

		return Shader;
	}


	/************************************************************************************************/


	void DestroyShader( Shader* releaseme ){
		if(releaseme && releaseme->Blob) releaseme->Blob->Release();
	}



	/************************************************************************************************/


	DescHeapPOS PushRenderTarget(RenderSystem* RS, ResourceHandle target, DescHeapPOS POS, const size_t MIPOffset)
	{
		D3D12_RENDER_TARGET_VIEW_DESC TargetDesc = {};
		const auto dimension            = RS->GetTextureDimension(target);

		switch (dimension)
		{
		case TextureDimension::Texture2D:
			TargetDesc.Format               = TextureFormat2DXGIFormat(RS->GetTextureFormat(target));
			TargetDesc.Texture2D.MipSlice   = MIPOffset;
			TargetDesc.Texture2D.PlaneSlice = 0;
			TargetDesc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
			break;
		case TextureDimension::TextureCubeMap:
			TargetDesc.Format                           = TextureFormat2DXGIFormat(RS->GetTextureFormat(target));
			TargetDesc.Texture2DArray.FirstArraySlice   = 0;
			TargetDesc.Texture2DArray.MipSlice          = MIPOffset;
			TargetDesc.Texture2DArray.PlaneSlice        = 0;
			TargetDesc.Texture2DArray.ArraySize         = 6;
			TargetDesc.ViewDimension                    = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			break;
		}
			

		auto resource = RS->GetDeviceResource(target);
		RS->pDevice->CreateRenderTargetView(resource, &TargetDesc, POS);

		return IncrementHeapPOS(POS, RS->DescriptorRTVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushDepthStencil(RenderSystem* RS, ResourceHandle Target, DescHeapPOS POS)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
		DSVDesc.Format				= RS->GetTextureDeviceFormat(Target);
		DSVDesc.Texture2D.MipSlice	= 0;
		DSVDesc.ViewDimension		= D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2D;

		RS->MarkTextureUsed(Target);
		RS->pDevice->CreateDepthStencilView(RS->GetDeviceResource(Target), &DSVDesc, POS);

		return IncrementHeapPOS(POS, RS->DescriptorDSVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushCBToDescHeap(RenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, size_t BufferSize, size_t Offset)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC CBV_DESC = {};
		CBV_DESC.BufferLocation = Buffer->GetGPUVirtualAddress() + Offset;
		CBV_DESC.SizeInBytes	= BufferSize;
		RS->pDevice->CreateConstantBufferView(&CBV_DESC, POS);

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushSRVToDescHeap(RenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, size_t ElementCount, size_t Stride, D3D12_BUFFER_SRV_FLAGS Flags)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC Desc; {
			Desc.Format                     = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
			Desc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			Desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
			Desc.Buffer.FirstElement        = 0;
			Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE;
			Desc.Buffer.NumElements         = ElementCount;
			Desc.Buffer.StructureByteStride = Stride;
		}

		RS->pDevice->CreateShaderResourceView(Buffer, &Desc, POS);
		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS Push2DSRVToDescHeap(RenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, D3D12_BUFFER_SRV_FLAGS Flags )
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC Desc; {
			Desc.Format                         = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
			Desc.Shader4ComponentMapping        = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			Desc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE2D;
			Desc.Texture2D.MipLevels		    = 1;
			Desc.Texture2D.MostDetailedMip	    = 0;
			Desc.Texture2D.PlaneSlice		    = 0;
			Desc.Texture2D.ResourceMinLODClamp	= 0;
		}

		RS->pDevice->CreateShaderResourceView(Buffer, &Desc, POS);
		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushTextureToDescHeap(RenderSystem* RS, Texture2D tex, DescHeapPOS POS)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc = {}; {
			ViewDesc.Format                        = tex.Format;
			ViewDesc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			ViewDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
			ViewDesc.Texture2D.MipLevels           = !tex.mipCount ? 1 : tex.mipCount;
			ViewDesc.Texture2D.MostDetailedMip     = 0;
			ViewDesc.Texture2D.PlaneSlice          = 0;
			ViewDesc.Texture2D.ResourceMinLODClamp = 0;
		}

		RS->pDevice->CreateShaderResourceView(tex, &ViewDesc, POS);

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushCubeMapTextureToDescHeap(RenderSystem* RS, Texture2D tex, DescHeapPOS POS)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc = {}; {
			ViewDesc.Format                          = tex.Format;
			ViewDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			ViewDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
			ViewDesc.TextureCube.MipLevels           = !tex.mipCount ? 1 : tex.mipCount;
			ViewDesc.TextureCube.MostDetailedMip     = 0;
			ViewDesc.TextureCube.ResourceMinLODClamp = 0;
		}

		RS->pDevice->CreateShaderResourceView(tex, &ViewDesc, POS);

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushUAV2DToDescHeap(RenderSystem* RS, Texture2D tex, DescHeapPOS POS)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		UAVDesc.Format               = tex.Format;
		UAVDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
		UAVDesc.Texture2D.MipSlice   = 0;
		UAVDesc.Texture2D.PlaneSlice = 0;

		RS->pDevice->CreateUnorderedAccessView(tex, nullptr, &UAVDesc, POS);
		
		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	inline DescHeapPOS PushUAVBufferToDescHeap(RenderSystem* RS, UAVBuffer buffer, DescHeapPOS POS)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		UAVDesc.Format						= buffer.format;
		UAVDesc.ViewDimension				= D3D12_UAV_DIMENSION_BUFFER;
		UAVDesc.Buffer.CounterOffsetInBytes = buffer.counterOffset = 0;
		UAVDesc.Buffer.FirstElement			= buffer.offset;
		UAVDesc.Buffer.Flags				= buffer.typeless == true ? D3D12_BUFFER_UAV_FLAGS::D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAGS::D3D12_BUFFER_UAV_FLAG_NONE;
		UAVDesc.Buffer.NumElements			= buffer.elementCount - buffer.offset;
		UAVDesc.Buffer.StructureByteStride	= buffer.stride;

		RS->pDevice->CreateUnorderedAccessView(buffer.resource, nullptr, &UAVDesc, POS);

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	bool LoadObjMesh(RenderSystem* RS, char* File_Loc, Obj_Desc IN desc, TriMesh ROUT out, StackAllocator RINOUT LevelSpace, StackAllocator RINOUT TempSpace, bool DiscardBuffers)
	{
		using MeshUtilityFunctions::TokenList;
		using MeshUtilityFunctions::CombinedVertexBuffer;
		using MeshUtilityFunctions::IndexList;
		using namespace FlexKit::MeshUtilityFunctions;
		using namespace FlexKit::MeshUtilityFunctions::OBJ_Tools;

		// TODO: Handle Multi Threading Cases
		size_t pos = 0;
		size_t buffersize = 1024*1024*16;
		size_t size = buffersize;
		size_t line_pos = 0;

		char*	strBuffer = (char*)TempSpace.malloc(buffersize);
		char	current_line[512];

		memset(strBuffer, 0, buffersize);
		bool Loaded = FlexKit::LoadFileIntoBuffer(File_Loc, (byte*)strBuffer, size);// TODO: Make Thread Safe
		if (!Loaded)
		{
			TempSpace.clear();
			printf("Failed To Load Obj\n");
			return false;
		}
		
		TokenList				TL			{ TempSpace, 64000 };
		CombinedVertexBuffer	out_buffer	{ TempSpace, 16000 };
		IndexList				out_indexes	{ TempSpace, 64000 };

		//TL.push_back(s_TokenValue::Empty());
		LoaderState S;

		size = strlen(strBuffer);
		while( pos < size)
		{
			if( strBuffer[pos] != '\n' )
			{
				current_line[line_pos++] = strBuffer[pos];
			}
			else
			{
				size_t LineLength = line_pos;
				current_line[LineLength] = '\0';
				CStrToToken( ScrubLine( current_line, LineLength ), LineLength, TL, S );
				line_pos = 0;
			}
			pos++;
		}

		//if (!FlexKit::MeshUtilityFunctions::BuildVertexBuffer(TL, out_buffer, out_indexes, LevelSpace, TempSpace))
			return false;

		size_t VertexBufferSize = out_buffer.size()		* sizeof(float3)	+ sizeof(VertexBufferView);// pos
		size_t IndexBufferSize  = out_indexes.size()	* sizeof(uint32_t)	+ sizeof(VertexBufferView);// index
		size_t NormalBufferSize = out_buffer.size()		* sizeof(float3)	+ sizeof(VertexBufferView);// Normal

		// Optional Buffers
		size_t TexcordBufferSize	= out_buffer.size()	* sizeof(float2)	+ sizeof(VertexBufferView);// Texcoord
		size_t TangentBufferSize	= out_buffer.size()	* sizeof(float3)	+ sizeof(VertexBufferView);// Tangent
		size_t extraBufferSize		= out_buffer.size()	* sizeof(float3)	+ sizeof(VertexBufferView);// ?

		byte* VertexBuffer	= (byte*)(DiscardBuffers ? TempSpace._aligned_malloc(VertexBufferSize, 16)	: LevelSpace._aligned_malloc(VertexBufferSize, 16));// Position
		byte* IndexBuffer	= (byte*)(DiscardBuffers ? TempSpace._aligned_malloc(IndexBufferSize, 16)	: LevelSpace._aligned_malloc(IndexBufferSize, 16)); // Index
		byte* NormalBuffer	= (byte*)(DiscardBuffers ? TempSpace._aligned_malloc(NormalBufferSize, 16)	: LevelSpace._aligned_malloc(NormalBufferSize, 16));// Normal

		byte* TexcordBuffer  = (byte*)(desc.LoadUVs				? (DiscardBuffers ? TempSpace._aligned_malloc(TexcordBufferSize, 16) : LevelSpace._aligned_malloc(TexcordBufferSize, 16)) : nullptr); // UV's
		byte* TangentBuffer	 = (byte*)(desc.GenerateTangents	? (DiscardBuffers ? TempSpace._aligned_malloc(TangentBufferSize, 16) : LevelSpace._aligned_malloc(TangentBufferSize, 16)) : nullptr); // Tangents

		FK_ASSERT(false);

		//out.Buffers[00] = FlexKit::CreateVertexBufferView(VertexBuffer, VertexBufferSize);
		//out.Buffers[01] = FlexKit::CreateVertexBufferView(NormalBuffer, NormalBufferSize);
		//out.Buffers[15] = FlexKit::CreateVertexBufferView(IndexBuffer,  IndexBufferSize);

		out.Buffers[0]->Begin
			( FlexKit::VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION
			, FlexKit::VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32 );


		float3 Vmn = float3(0);
		float3 Vmx = float3(0);
		float BoundingShere = 0;

		for (auto V : out_buffer)
		{
			for (auto itr = 0; itr < 3; ++itr)
			{	// find Max
				if (V.POS[itr] > Vmx[itr]) 
					Vmx[itr] = V.POS[itr];

				// find Min
				if (V.POS[itr] < Vmn[itr]) 
					Vmn[itr] = V.POS[itr];

				if (V.POS.magnitude() > BoundingShere)
					BoundingShere = V.POS.magnitude();

			}
			out.Buffers[0]->Push(V.POS * desc.S);
		}

		out.Buffers[0]->End();

		out.Buffers[15]->Begin
			( FlexKit::VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX
			, FlexKit::VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32 );

		for (auto itr = out_indexes.begin(); itr != out_indexes.end(); )
		{
			auto I1 = *itr++;
			auto I2 = *itr++;
			auto I3 = *itr++;

			out.Buffers[15]->Push(uint32_t(I2));
			out.Buffers[15]->Push(uint32_t(I1));
			out.Buffers[15]->Push(uint32_t(I3));
		}

		out.Buffers[1]->End();

		if (S.Normals)
		{
			out.Buffers[1]->Begin
				( FlexKit::VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL
				, FlexKit::VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32 );

			for (auto V : out_buffer)
				out.Buffers[1]->Push( V.NORMAL );

			out.Buffers[1]->End();
		}
		out.IndexCount = out_indexes.size();

		if (S.UV_1 && desc.LoadUVs)
		{
			//out.Buffers[2] = FlexKit::CreateVertexBufferView(TexcordBuffer, TexcordBufferSize);
			out.Buffers[2]->Begin
				( FlexKit::VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV
				, FlexKit::VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32 );

			for (auto V : out_buffer)
				out.Buffers[2]->Push(V.TEXCOORD);

			out.Buffers[2]->End();
		}

		if (desc.LoadUVs && desc.GenerateTangents)
		{
			// TODO(RM): Lift out into Mesh Utilities 
			FK_ASSERT(TangentBuffer); // Check that output is not Null
		
			//out.Buffers[3] = FlexKit::CreateVertexBufferView(TangentBuffer, TangentBufferSize);
			out.Buffers[3]->Begin
				( FlexKit::VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT
				, FlexKit::VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32 );

			// Set Clear Tangents
			for (auto V : out_buffer)
				out.Buffers[3]->Push(float3{0.0f, 0.0f, 0.0f});

			auto IndexBuffer	= reinterpret_cast<uint32_t*>(out.Buffers[15]->GetBuffer()); 
			auto VBuffer		= reinterpret_cast<float3*>	 (out.Buffers[0]->GetBuffer());
			auto NBuffer		= reinterpret_cast<float3*>	 (out.Buffers[1]->GetBuffer()); 
			auto TexCoords		= reinterpret_cast<float2*>	 (out.Buffers[2]->GetBuffer()); 
			auto Tangents		= reinterpret_cast<float3*>	 (out.Buffers[3]->GetBuffer()); 

			for (auto itr = 0; itr < out.Buffers[0]->GetBufferSizeUsed(); itr+= 3)
			{
				float3 V0 = VBuffer[IndexBuffer[itr + 0]];
				float3 V1 = VBuffer[IndexBuffer[itr + 1]];
				float3 V2 = VBuffer[IndexBuffer[itr + 2]];

				float2 UV1 = TexCoords[IndexBuffer[itr + 0]];
				float2 UV2 = TexCoords[IndexBuffer[itr + 1]];
				float2 UV3 = TexCoords[IndexBuffer[itr + 2]];

				float3 Q1 = V1 - V0;
				float3 Q2 = V2 - V0;

				float2 ST1 = UV2 - UV1;
				float2 ST2 = UV3 - UV1;

				float r = 1.0f / ((ST1.x*ST2.y) - (ST2.x * ST1.y));

				float3 S = r * float3((ST2.y * Q1[0]) - (ST1.y * Q2[0]), (ST2.y * Q1[1]) - (ST1.y * Q2[1]), (ST2.y * Q1[2]) - (ST1.y * Q2[2]));
				float3 T = r * float3((ST1.x * Q1[0]) - (ST2.x * Q2[0]), (ST1.x * Q1[1]) - (ST2.x * Q2[1]), (ST1.x * Q1[2]) - (ST2.x * Q2[2]));

				Tangents[IndexBuffer[itr + 0]] += S;
				Tangents[IndexBuffer[itr + 1]] += S;
				Tangents[IndexBuffer[itr + 2]] += S;
			}

			// Normalise Averaged Tangents
			float3* Normals = (float3*)out.Buffers[3]->GetBuffer();
			float3* end = (float3*)(out.Buffers[3]->GetBuffer() + out.Buffers[3]->GetBufferSizeRaw());

			while (Normals < end)
			{
				(*Normals) = Normals->normal();
				Normals++;
			}
		}

		FlexKit::CreateVertexBuffer		(RS, RS->GetImmediateUploadQueue(), out.Buffers, 2 + desc.LoadUVs + desc.GenerateTangents,           out.VertexBuffer);
		if( !FlexKit::CreateInputLayout	(RS, out.Buffers, 2 + desc.LoadUVs + desc.GenerateTangents, &desc.V, &out.VertexBuffer))
			return false;

		out.Info.max = Vmx;
		out.Info.min = Vmn;

		if (DiscardBuffers) {
			//ClearTriMeshVBVs(&out);
			TempSpace.clear();
		}
		return true;
	}

	
	/************************************************************************************************/


	void ReleaseTriMesh(TriMesh* T)
	{
		if(T->Memory){
			Release(&T->VertexBuffer);
			T->Memory->free((void*)T->ID);

			for (auto& B : T->Buffers)
			{
				if(B)
					T->Memory->free(B);

				B = nullptr;
			}
		}
	}


	/************************************************************************************************/


	void DelayedReleaseTriMesh(RenderSystem* RS, TriMesh* T)
	{
		DelayedRelease(RS, &T->VertexBuffer);
		T->Memory->free((void*)T->ID);

		for (auto& B : T->Buffers)
		{
			if (B)
				T->Memory->free(B);

			B = nullptr;
		}

		T->VertexBuffer.clear();
	}


	/************************************************************************************************/


	int2 GetMousedPos(RenderWindow* Window)
	{
		auto POS = Window->WindowCenterPosition;
		POINT CENTER = { (long)POS[0], (long)POS[1] };
		POINT P;

		GetCursorPos(&P);
		ScreenToClient(Window->hWindow, &P);

		int2 dMouse = { CENTER.x - (int)P.x , CENTER.y - (int)P.y };
		return dMouse;
	}



	/************************************************************************************************/


	void SetSystemCursorToWindowCenter(RenderWindow* Window)
	{
		auto POS = Window->WindowCenterPosition;
		POINT CENTER = { (long)POS[0], (long)POS[1] };
		POINT P;

		GetCursorPos(&P);
		ScreenToClient(Window->hWindow, &P);

		ClientToScreen(Window->hWindow, &CENTER);
		SetCursorPos(CENTER.x, CENTER.y);
	}


	/************************************************************************************************/


	void ShowSystemCursor(RenderWindow* Window)
	{
		ShowCursor(true);
	}


	/************************************************************************************************/


	float2 GetPixelSize(RenderWindow& Window)
	{
		return float2{ 1.0f, 1.0f } / Window.WH;
	}

	/************************************************************************************************/


	void HideSystemCursor(RenderWindow* Window)
	{
		ShowCursor(false);
	}


	/************************************************************************************************/

	// assumes File str should be at most 256 bytes
	ResourceHandle LoadDDSTextureFromFile(char* file, RenderSystem* RS, CopyContextHandle handle, iAllocator* MemoryOut)
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


	ResourceHandle UploadDDSFromAsset(AssetHandle asset, RenderSystem* renderSystem, CopyContextHandle uploadHandle, iAllocator* temp)
	{
		crnd::crn_texture_info info;
		auto textureAvailable           = isAssetAvailable(asset);
		auto RHandle                    = LoadGameAsset(asset);
		TextureResourceBlob* resource   = reinterpret_cast<TextureResourceBlob*>(FlexKit::GetAsset(RHandle));
		auto buffer                     = resource->GetBuffer();
		const auto bufferSize           = resource->GetBufferSize();

		auto& copyCtx    = renderSystem->_GetCopyContext(uploadHandle);

		if (crnd::crnd_get_texture_info(buffer, resource->GetBufferSize(), &info))
		{
			crnd::crn_level_info levelInfo;

			if (!crnd::crnd_get_level_info(buffer, bufferSize, 0, &levelInfo))
				return InvalidHandle_t;

			auto context = crnd::crnd_unpack_begin(buffer, resource->GetBufferSize());

			const size_t rowPitch   = levelInfo.m_blocks_x * levelInfo.m_bytes_per_block;
			const size_t bufferSize = levelInfo.m_blocks_y * rowPitch;
			const auto region       = copyCtx.Reserve(bufferSize);
			void* data[1]           = { (void*)region.buffer };
			auto res                = crnd::crnd_unpack_level(context, data, bufferSize, rowPitch, 0);

			if (!res)
			{
				crnd::crnd_unpack_end(context);
				return InvalidHandle_t;
			}

			crnd::crnd_unpack_end(context);

			GPUResourceDesc desc = GPUResourceDesc::ShaderResource(
				{ info.m_width, info.m_height },
				DeviceFormat::BC3_UNORM,
				//resource->format,
				info.m_levels);

			
			auto textureHandle          = renderSystem->CreateGPUResource(desc);
			auto destinationResource    = renderSystem->GetDeviceResource(textureHandle);

			D3D12_PLACED_SUBRESOURCE_FOOTPRINT SubRegion;
			SubRegion.Footprint.Depth       = 1;
			SubRegion.Footprint.Format      = DXGI_FORMAT_BC3_UNORM;TextureFormat2DXGIFormat(resource->format);
			SubRegion.Footprint.RowPitch    = rowPitch;
			SubRegion.Footprint.Width       = info.m_width;
			SubRegion.Footprint.Height      = info.m_height;
			SubRegion.Offset                = region.offset;

			auto Destination    = CD3DX12_TEXTURE_COPY_LOCATION(destinationResource, 0);
			auto Source         = CD3DX12_TEXTURE_COPY_LOCATION(region.resource, SubRegion);

			copyCtx.commandList->CopyTextureRegion(&Destination, 0, 0, 0, &Source, nullptr);
			//renderSystem->_InsertBarrier(destinationResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);


			return textureHandle;
		}
		else
			return InvalidHandle_t;
	}


	/************************************************************************************************/


	ResourceHandle MoveTextureBufferToVRAM(RenderSystem* RS, CopyContextHandle handle, TextureBuffer* buffer, DeviceFormat format, iAllocator* tempMemory)
	{
		auto textureHandle = RS->CreateGPUResource(GPUResourceDesc::ShaderResource(buffer->WH, format));
		RS->UploadTexture(textureHandle, handle, buffer->Buffer, buffer->Size);

		return textureHandle;
	}


	/************************************************************************************************/


	ResourceHandle MoveTextureBuffersToVRAM(RenderSystem* RS, CopyContextHandle handle, TextureBuffer* buffer, size_t resourceCount, DeviceFormat format, iAllocator* tempMemory)
	{
		auto textureHandle = RS->CreateGPUResource(GPUResourceDesc::ShaderResource(buffer[0].WH, format, resourceCount));
		RS->UploadTexture(textureHandle, handle, buffer, resourceCount, tempMemory);

		return textureHandle;
	}


	/************************************************************************************************/


	ResourceHandle MoveTextureBuffersToVRAM(RenderSystem* RS, CopyContextHandle handle, TextureBuffer* buffer, size_t MIPCount, size_t arrayCount, DeviceFormat format, iAllocator* tempMemory)
	{
		auto textureHandle = RS->CreateGPUResource(GPUResourceDesc::ShaderResource(buffer[0].WH, format, MIPCount, arrayCount));
		RS->UploadTexture(textureHandle, handle, buffer, arrayCount * MIPCount, tempMemory);

		return textureHandle;
	}



	/************************************************************************************************/


	ResourceHandle LoadTexture(TextureBuffer* Buffer, CopyContextHandle handle, RenderSystem* RS, iAllocator* Memout, DeviceFormat format)
	{
		GPUResourceDesc GPUResourceDesc = GPUResourceDesc::ShaderResource(Buffer->WH, format);
		GPUResourceDesc.initial     = Buffer->Buffer;

		size_t elementSize          = GetFormatElementSize(TextureFormat2DXGIFormat(format));
		size_t ResourceSizes[]      = { Buffer->Size };

		auto texture = RS->CreateGPUResource(GPUResourceDesc);
		SubResourceUpload_Desc desc = {};
		desc.buffers                         = Buffer;
		desc.subResourceCount                = 1;
		desc.subResourceStart                = 0;
		desc.format                          = format;

		_UpdateSubResourceByUploadQueue(
			RS,
			handle,
			RS->GetDeviceResource(texture),
			&desc,
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		RS->Textures.SetState(texture, DeviceResourceState::DRS_ShaderResource);
		RS->SetDebugName(texture, "LOADTEXTURE");

		return texture;
	}


	/************************************************************************************************/
}//	Namespace FlexKit


/**********************************************************************

Copyright (c) 2014-2020 Robert May

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
