

/**********************************************************************

Copyright (c) 2014-2017 Robert May

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

#include "..\pch.h"
#include "..\buildsettings.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\intersection.h"
#include "..\graphicsutilities\MeshUtils.h"
#include "..\graphicsutilities\DDSUtilities.h"


#include "AnimationUtilities.h"
#include "graphics.h"

#include <algorithm>
#include <memory>
#include <Windows.h>
#include <windowsx.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <d3d11sdklayers.h>
#include <d3d11shader.h>
#include <fstream>
#include <string>
#include <iostream>

#include "..\Dependencies\include\DirectXTK\WICTextureLoader.h"
#include "..\Dependencies\include\DirectXTK\DDSTextureLoader.h"

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


	ConstantBufferHandle	ConstantBufferTable::CreateConstantBuffer(size_t BufferSize, bool GPUResident)
	{
		D3D12_RESOURCE_DESC   Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(BufferSize);
		Resource_DESC.Alignment				= 0;
		Resource_DESC.DepthOrArraySize		= 1;
		Resource_DESC.Dimension				= D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		Resource_DESC.Layout				= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Resource_DESC.Width					= BufferSize;
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

		size_t BufferCount		= 3;
		BufferResourceSet	NewResourceSet;

		D3D12_RESOURCE_STATES InitialState = GPUResident ? 
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON :
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ;

		for(size_t I = 0; I < BufferCount; ++I)
		{
			ID3D12Resource* Resource = nullptr;
			HRESULT HR = RS->pDevice->CreateCommittedResource(
							&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, 
							&Resource_DESC, InitialState, nullptr,
							IID_PPV_ARGS(&Resource));

			CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE CONSTANT BUFFER"));
			NewResourceSet.Resources[I] = Resource;

			SETDEBUGNAME(Resource, __func__);
		}

		uint32_t BufferIdx = uint32_t(ConstantBuffers.size());
		ConstantBuffers.push_back(NewResourceSet);

		void* Mapped_ptr = nullptr;

		if(!GPUResident)
			NewResourceSet.Resources[0]->Map(0, nullptr, &Mapped_ptr);

		auto Idx = UserBufferEntries.size();

		ConstantBufferHandle NewHandle = Handles.GetNewHandle();
		Handles[NewHandle] = Idx;

		UserConstantBuffer	NewBuffer = {
			uint32_t(BufferSize),
			BufferIdx,
			0,
			0,
			0,
			Mapped_ptr,
			Memory,
			GPUResident,
			false,
			NewHandle,
			{0, 0, 0} };

		UserBufferEntries.push_back(NewBuffer);

		return NewHandle;
	}


	/************************************************************************************************/


	ID3D12Resource* ConstantBufferTable::GetBufferResource(const ConstantBufferHandle Handle) const
	{
		size_t UserIdx		= Handles[Handle];
		uint32_t Idx		= UserBufferEntries[UserIdx].CurrentBuffer;
		uint32_t BufferIdx	= UserBufferEntries[UserIdx].BufferSet;
		return ConstantBuffers[BufferIdx].Resources[Idx];
	}


	/************************************************************************************************/


	size_t ConstantBufferTable::GetBufferOffset(const ConstantBufferHandle Handle) const
	{
		return UserBufferEntries[Handles[Handle]].Offset;
	}


	/************************************************************************************************/


	size_t ConstantBufferTable::GetBufferBeginOffset(const ConstantBufferHandle Handle) const
	{
		return UserBufferEntries[Handles[Handle]].BeginOffset;
	}


	/************************************************************************************************/


	size_t ConstantBufferTable::BeginNewBuffer(ConstantBufferHandle Handle)
	{
		size_t UserIdx = Handles[Handle];

		if (UserBufferEntries[Handles[Handle]].GPUResident)
			return false; // Cannot directly push to GPU Resident Memory

		UpdateCurrentBuffer(Handle);

		uint32_t Idx			= UserBufferEntries[UserIdx].CurrentBuffer;
		uint32_t BufferIdx		= UserBufferEntries[UserIdx].BufferSet;
		uint32_t BufferSize		= UserBufferEntries[UserIdx].BufferSize;
		uint32_t BufferOffset	= UserBufferEntries[UserIdx].Offset;

		size_t offset				 = 256 - BufferOffset % 256;
		size_t OffsetToNextAlignment = (offset == 256) ? 0 : offset;
		size_t NewOffset			 = BufferOffset + OffsetToNextAlignment;

		UserBufferEntries[UserIdx].BeginOffset	= NewOffset;
		UserBufferEntries[UserIdx].Offset		= NewOffset;

		return NewOffset;
	}


	/************************************************************************************************/


	bool ConstantBufferTable::Push(ConstantBufferHandle Handle, void* _Ptr, size_t PushSize)
	{
		size_t UserIdx = Handles[Handle];

		if (UserBufferEntries[UserIdx].GPUResident)
			return false; // Cannot directly push to GPU Resident Memory

		UpdateCurrentBuffer(Handle);

		uint32_t Idx			= UserBufferEntries[UserIdx].CurrentBuffer;
		uint32_t BufferIdx		= UserBufferEntries[UserIdx].BufferSet;
		uint32_t BufferSize		= UserBufferEntries[UserIdx].BufferSize;
		uint32_t BufferOffset	= UserBufferEntries[UserIdx].Offset;
		char*  Mapped_Ptr		= (char*)UserBufferEntries[UserIdx].Mapped_ptr;

		if (!Mapped_Ptr)
			return false;

		if (BufferSize < BufferOffset + PushSize)
			return false; // Buffer To small to accommodate Push

		UserBufferEntries[UserIdx].Offset += PushSize;
		UserBufferEntries[UserIdx].WrittenTo = true;

		if(_Ptr)
			memcpy(Mapped_Ptr + BufferOffset, _Ptr, PushSize);

		return true;
	}


	/************************************************************************************************/


	ConstantBufferTable::SubAllocation ConstantBufferTable::Reserve(ConstantBufferHandle CB, size_t reserveSize)
	{
		size_t offsetBegin = GetBufferOffset(CB);
		FK_ASSERT(Push(CB, nullptr, reserveSize));

		size_t		UserIdx		= Handles[CB];
		uint32_t	Idx			= UserBufferEntries[UserIdx].CurrentBuffer;
		void*		buffer		= UserBufferEntries[UserIdx].Mapped_ptr;

		return { static_cast<char*>(buffer), offsetBegin, reserveSize };
	}


	/************************************************************************************************/


	void ConstantBufferTable::LockUntil(size_t FrameID)
	{
		for (auto& Buffer : UserBufferEntries) 
		{	// TODO: handle WrittenTo Flags
			auto BufferIdx		= Buffer.CurrentBuffer;

			Buffer.Locks[BufferIdx] = FrameID;
		}
	}


	/************************************************************************************************/


	void ConstantBufferTable::ReleaseBuffer(ConstantBufferHandle Handle)
	{
		size_t UserIdx		= Handles[Handle];
		size_t BufferIdx	= UserBufferEntries[UserIdx].BufferSet;

		for (auto& res : ConstantBuffers[BufferIdx].Resources)
		{
			if (res)
				res->Release();

			res = nullptr;
		}
	}


	/************************************************************************************************/


	void ConstantBufferTable::UpdateCurrentBuffer(ConstantBufferHandle Handle)
	{
		size_t UserIdx	= Handles[Handle];
		size_t Idx		= UserBufferEntries[UserIdx].CurrentBuffer;

		if (UserBufferEntries[UserIdx].Locks[Idx] > RS->GetCurrentFrame())
		{
			size_t BufferIdx = UserBufferEntries[UserIdx].BufferSet;

			// Map Next Buffer
			if (!UserBufferEntries[UserIdx].GPUResident)
			{
				char*  Mapped_Ptr = nullptr;
				ConstantBuffers[BufferIdx].Resources[Idx]->Unmap(0, nullptr);	// Is a Written Range Needed?
				Idx = UserBufferEntries[UserIdx].CurrentBuffer = (Idx + 1) % 3;	// Update Buffer Idx
				auto HR = ConstantBuffers[BufferIdx].Resources[Idx]->Map(0, nullptr, (void**)&Mapped_Ptr);
				if (FAILED(HR))
				{
					int x = 0;
				}
				UserBufferEntries[UserIdx].Mapped_ptr	= Mapped_Ptr;
			}

			UserBufferEntries[UserIdx].Offset		= 0;
			UserBufferEntries[UserIdx].BeginOffset	= 0;
		}
	}


	/************************************************************************************************/


	DescriptorHeap::DescriptorHeap(RenderSystem* RS, const DesciptorHeapLayout<16>& Layout_IN, iAllocator* TempMemory) :
		FillState(TempMemory)
	{
		FK_ASSERT(TempMemory);

		const size_t EntryCount = Layout_IN.size();
		descriptorHeap	= RS->_ReserveDescHeap(EntryCount);
		Layout			= &Layout_IN;

		for (size_t I = 0; I < EntryCount; I++)
			FillState.push_back(false);
	}


	/************************************************************************************************/


	DescriptorHeap& DescriptorHeap::Init(RenderSystem* RS, const DesciptorHeapLayout<16>& Layout_IN, iAllocator* TempMemory)
	{
		FK_ASSERT(TempMemory);
		FillState = Vector<bool>(TempMemory);

		const size_t EntryCount = Layout_IN.size();
		descriptorHeap = RS->_ReserveDescHeap(EntryCount);
		Layout = &Layout_IN;

		for (size_t I = 0; I < EntryCount; I++)
			FillState.push_back(false);
		
		return *this;
	}


	/************************************************************************************************/


	DescriptorHeap& DescriptorHeap::Init(RenderSystem* RS, const DesciptorHeapLayout<16>& Layout_IN, size_t reserveCount, iAllocator* TempMemory)
	{
		FK_ASSERT(TempMemory);
		FillState = Vector<bool>(TempMemory);

		const size_t EntryCount = Layout_IN.size() * reserveCount;
		descriptorHeap = RS->_ReserveDescHeap(EntryCount);
		Layout = &Layout_IN;

		for (size_t I = 0; I < EntryCount; I++)
			FillState.push_back(false);

		return *this;
	}


	/************************************************************************************************/


	DescriptorHeap& DescriptorHeap::NullFill(RenderSystem* RS)
	{
		auto& Entries = Layout->Entries;
		for (size_t I = 0, Idx = 0; I < FillState.size(); I++)
		{
			auto& e = Entries[I];

			for (size_t II = 0; II < e.Space; II++)
			{
				if (!FillState[Idx])
				{
					switch (e.Type)
					{
					case DescHeapEntryType::ConstantBuffer:
					{
						auto POS = IncrementHeapPOS(
							descriptorHeap,
							RS->DescriptorCBVSRVUAVSize,
							Idx);

						PushCBToDescHeap(
							RS, RS->NullConstantBuffer.Get(),
							POS, 1024);
					}	break;
					case DescHeapEntryType::ShaderResource:
					{
						auto POS = IncrementHeapPOS(
							descriptorHeap,
							RS->DescriptorCBVSRVUAVSize,
							Idx);

						PushSRVToDescHeap(
							RS, RS->NullSRV1D.Get(),
							POS, 0, 16);
					}	break;
					case DescHeapEntryType::UAVBuffer:
					{
						auto POS = IncrementHeapPOS(
							descriptorHeap,
							RS->DescriptorCBVSRVUAVSize,
							Idx);

						PushUAV2DToDescHeap(
							RS, RS->NullUAV,
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


	bool DescriptorHeap::SetSRV(RenderSystem* RS, size_t idx, TextureHandle handle)
	{
		for (auto entry : Layout->Entries)
		{
			if ((entry.Register <= idx && entry.Register + entry.Count > idx) &&
				(entry.Type == DescHeapEntryType::ShaderResource))
				break;

			if (entry.Register > idx)
				return false;
		}

		PushTextureToDescHeap(
			RS, 
			RS->Textures[handle],
			IncrementHeapPOS(descriptorHeap, 
				RS->DescriptorCBVSRVUAVSize, 
				idx));

		return true;
	}


	/************************************************************************************************/


	bool DescriptorHeap::SetStructuredResource(RenderSystem* RS, size_t idx, TextureHandle handle, size_t stride)
	{
		for (auto entry : Layout->Entries)
		{
			if ((entry.Register <= idx && entry.Register + entry.Count > idx) &&
				(entry.Type == DescHeapEntryType::ShaderResource) ) 
				break;

			if (entry.Register > idx)
				return false;
		}

		auto WH = RS->GetTextureWH(handle);

		PushSRVToDescHeap(
			RS,
			RS->Textures[handle],
			IncrementHeapPOS(descriptorHeap,
				RS->DescriptorCBVSRVUAVSize,
				idx),
			WH[0],
			stride);
			

		return true;
	}


	/************************************************************************************************/


	DescriptorHeap	DescriptorHeap::GetHeapOffsetted(size_t offset, RenderSystem* RS) const
	{
		DescriptorHeap subHeap(*this);
		subHeap.descriptorHeap = IncrementHeapPOS(
										descriptorHeap,
										RS->DescriptorCBVSRVUAVSize,
										offset);

		return subHeap;
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



	void Context::AddRenderTargetBarrier(TextureHandle Handle, DeviceResourceState Before, DeviceResourceState New)
	{
		Barrier NewBarrier;
		NewBarrier.OldState				= Before;
		NewBarrier.NewState				= New;
		NewBarrier.Type					= Barrier::BT_RenderTarget;
		NewBarrier.renderTarget			= Handle;

		PendingBarriers.push_back(NewBarrier);
	}

	/************************************************************************************************/


	void Context::AddUAVBarrier(UAVResourceHandle handle, DeviceResourceState priorState, DeviceResourceState desiredState)
	{
		Barrier NewBarrier;
		NewBarrier.OldState		= priorState;
		NewBarrier.NewState		= desiredState;
		NewBarrier.Type			= Barrier::BT_UAV;
		NewBarrier.UAV			= handle;

		PendingBarriers.push_back(NewBarrier);
	}


	/************************************************************************************************/


	void Context::AddPresentBarrier(TextureHandle Handle, DeviceResourceState Before)
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


	void Context::AddShaderResourceBarrier(TextureHandle resource, DeviceResourceState Before, DeviceResourceState State)
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

		if (CurrentPipelineState == PSO)
			return;

		CurrentPipelineState = PSO;
		DeviceContext->SetPipelineState(PSO);
	}


	/************************************************************************************************/


	void Context::SetRenderTargets(const static_vector<DescHeapPOS> RTs, bool DepthStecil, DescHeapPOS DepthStencil)
	{
		DSVPOSCPU = DepthStencil;

		static_vector<D3D12_CPU_DESCRIPTOR_HANDLE> RTVHandles;

		for (auto RT : RTs)
			RTVHandles.push_back((DescHeapPOS)RT);

		DeviceContext->OMSetRenderTargets(
			RTs.size(),
			RTVHandles.begin(),
			false,
			DepthStecil ? &(D3D12_CPU_DESCRIPTOR_HANDLE)DepthStencil : nullptr);
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
	void Context::SetScissorAndViewports(static_vector<TextureHandle, 16>	RenderTargets)
	{
		static_vector<D3D12_VIEWPORT, 16>	VPs;
		static_vector<D3D12_RECT, 16>		Rects;

		for (auto RT : RenderTargets)
		{
			auto WH = renderSystem->GetRenderTargetWH(RT);
			VPs.push_back	({ 0, 0,	(FLOAT)WH[0], (FLOAT)WH[1], 0, 1 });
			Rects.push_back	({ 0,0,	(LONG)WH[0], (LONG)WH[1] });
		}

		SetViewports(VPs);
		SetScissorRects(Rects);
	}


	/************************************************************************************************/


	void Context::SetDepthStencil(Texture2D* DS)
	{
		if (DS)
		{
			DSVPOSCPU		= renderSystem->_GetDSVTableCurrentPosition_CPU(); // _Ptr to Current POS On DSV heap on CPU
			auto DSVPOS		= renderSystem->_ReserveDSVHeap(1);
			auto DSVHeap	= renderSystem->_GetCurrentRTVTable();
			PushDepthStencil(renderSystem, DS, DSVPOS);
		}

		DepthStencilEnabled = DS != nullptr;
		UpdateRTVState();
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
		DeviceContext->SetGraphicsRootConstantBufferView(idx, renderSystem->GetConstantBufferAddress((ConstantBufferHandle)CB) + (size_t)CB);
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


	void Context::SetComputeShaderResourceView(size_t idx, Texture2D&			Texture)
	{
		DeviceContext->SetComputeRootShaderResourceView(idx, Texture->GetGPUVirtualAddress());
	}

	/************************************************************************************************/


	void Context::SetComputeUnorderedAccessView(size_t idx, UAVResourceHandle& UAVResource)
	{
		auto resource = renderSystem->BufferUAVs.GetResource(UAVResource);
		DeviceContext->SetComputeRootUnorderedAccessView(idx, resource->GetGPUVirtualAddress());
	}


	/************************************************************************************************/


	void Context::BeginQuery(QueryHandle query, size_t idx)
	{
		auto resource	= renderSystem->Queries.GetResource(query);
		auto queryType	= renderSystem->Queries.GetType(query);
		DeviceContext->BeginQuery(resource, queryType, idx);
	}


	void Context::EndQuery(QueryHandle query, size_t idx)
	{
		auto resource	= renderSystem->Queries.GetResource(query);
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

		/*
		typedef struct D3D12_WRITEBUFFERIMMEDIATE_PARAMETER
		{
		D3D12_GPU_VIRTUAL_ADDRESS Dest;
		UINT32 Value;
		} 	D3D12_WRITEBUFFERIMMEDIATE_PARAMETER;
		*/

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
			auto resource	= renderSystem->GetObjectDeviceResource(handles[itr]);
			auto state		= currentStates[itr];

			if(prevResource != resource && prevState != state)
				_AddBarrier(resource, state, DeviceResourceState::DRS_Write);

			prevResource	= resource;
			prevState		= prevState;
		}

		FlushBarriers();

		for (size_t itr = 0; itr < handles.size(); ++itr)
		{
			auto resource = renderSystem->GetObjectDeviceResource(handles[itr]);

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
			auto resource	= renderSystem->GetObjectDeviceResource(handles[itr]);
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


	void Context::ClearDepthBuffer(TextureObject Texture, float ClearDepth)
	{
		UpdateResourceStates();
		DeviceContext->ClearDepthStencilView(Texture.GPUHandle, D3D12_CLEAR_FLAG_DEPTH, ClearDepth, 0, 0, nullptr);

		renderSystem->RenderTargets.MarkRTUsed(Texture.CPUHandle);
	}


	/************************************************************************************************/


	void Context::ClearRenderTarget(TextureObject Texture, float4 ClearColor)
	{
		UpdateResourceStates();
		DeviceContext->ClearRenderTargetView(Texture.GPUHandle, ClearColor, 0, nullptr);
	}


	/************************************************************************************************/


	void Context::ResolveQuery(QueryHandle query, size_t begin, size_t end, UAVResourceHandle destination, size_t destOffset)
	{
		auto res			= renderSystem->GetObjectDeviceResource(destination);
		auto type			= renderSystem->Queries.GetType(query);
		auto queryResource	= renderSystem->Queries.GetResource(query);

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
			renderSystem->GetObjectDeviceResource(args),
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


	void Context::CopyBuffer(const UploadSegment src, TextureHandle destination)
	{
		const auto destinationResource		= renderSystem->GetObjectDeviceResource(destination);
		const auto sourceResource			= renderSystem->GetUploadResource();

		DeviceContext->CopyBufferRegion(destinationResource, 0, sourceResource, src.offset, src.uploadSize);
	}


	/************************************************************************************************/


	void Context::CopyTexture2D(const UploadSegment src, TextureHandle destination, uint2 BufferSize)
	{
		const auto destinationResource		= renderSystem->GetObjectDeviceResource(destination);
		const auto sourceResource			= renderSystem->GetUploadResource();
		const auto WH						= renderSystem->GetTextureWH(destination);
		const auto format					= renderSystem->GetTextureDeviceFormat(destination);
		const auto texelSize				= renderSystem->GetTextureElementSize(destination);

		D3D12_TEXTURE_COPY_LOCATION destLocation{};
		destLocation.pResource			= destinationResource;
		destLocation.SubresourceIndex	= 0;
		destLocation.Type				= D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;


		D3D12_TEXTURE_COPY_LOCATION srcLocation{};
		srcLocation.pResource							= sourceResource;
		srcLocation.Type								= D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcLocation.PlacedFootprint.Offset				= src.offset;
		srcLocation.PlacedFootprint.Footprint.Depth		= 1;
		srcLocation.PlacedFootprint.Footprint.Format	= format;
		srcLocation.PlacedFootprint.Footprint.Height	= WH[1];
		srcLocation.PlacedFootprint.Footprint.Width		= WH[0];
		srcLocation.PlacedFootprint.Footprint.RowPitch	= BufferSize[0] * texelSize;

		DeviceContext->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, nullptr);
	}


	/************************************************************************************************/


	void Context::Clear()
	{
		PendingBarriers.Release();
		CurrentPipelineState = nullptr;
	}


	/************************************************************************************************/


	void Context::SetRTRead(TextureHandle Handle)
	{
	}


	/************************************************************************************************/


	void Context::SetRTWrite(TextureHandle Handle)
	{
	}


	/************************************************************************************************/


	void Context::SetRTFree(TextureHandle Handle)
	{
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
					auto resource		= renderSystem->RenderTargets.GetResource(handle);
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
				case Barrier::BT_UAV:
				{
					auto handle			= B.UAV;
					auto resource		= renderSystem->GetObjectDeviceResource(handle);
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
						auto SOresource			= renderSystem->GetObjectDeviceResource(handle);
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
						auto resource			= renderSystem->GetObjectDeviceResource(handle);
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
					auto resource		= renderSystem->Textures.GetResource(B.shaderResource);
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

		if (Barriers.size())
			DeviceContext->ResourceBarrier(Barriers.size(), Barriers.begin());

		Barriers.clear();
		PendingBarriers.clear();
	}


	/************************************************************************************************/


	void Context::UpdateRTVState()
	{
		DeviceContext->OMSetRenderTargets(
			RenderTargetCount,
			RenderTargetCount ?		&RTVPOSCPU : nullptr, true,
			DepthStencilEnabled ?	&DSVPOSCPU : nullptr);
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
			CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR, 
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

			DesciptorHeapLayout<16> DescriptorHeap;
			DescriptorHeap.SetParameterAsSRV(0, 0, 10);
			FK_ASSERT(DescriptorHeap.Check());

			RS->Library.RSDefault.SetParameterAsCBV				(0, 0, 0, PIPELINE_DESTINATION::PIPELINE_DEST_ALL);
			RS->Library.RSDefault.SetParameterAsCBV				(1, 1, 0, PIPELINE_DESTINATION::PIPELINE_DEST_ALL);
			RS->Library.RSDefault.SetParameterAsCBV				(2, 2, 0, PIPELINE_DESTINATION::PIPELINE_DEST_ALL);
			RS->Library.RSDefault.SetParameterAsSRV				(3, 0, 0, PIPELINE_DESTINATION::PIPELINE_DEST_VS);
			RS->Library.RSDefault.SetParameterAsDescriptorTable	(4, DescriptorHeap, -1, PIPELINE_DESTINATION::PIPELINE_DEST_PS);
			RS->Library.RSDefault.Build(RS, TempMemory);

			SETDEBUGNAME(RS->Library.RSDefault, "RSDefault");
		}
	}


	/************************************************************************************************/


	void InitiateCopyEngine(RenderSystem* RS)
	{
		D3D12_RESOURCE_DESC   Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(MEGABYTE * 16);
		D3D12_HEAP_PROPERTIES HEAP_Props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		ID3D12Resource* TempBuffer;
		HRESULT HR = RS->pDevice->CreateCommittedResource(&HEAP_Props, D3D12_HEAP_FLAG_NONE,
			&Resource_DESC, D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(&TempBuffer));

		RS->CopyEngine.Position	    = 0;
		RS->CopyEngine.Last			= 0;
		RS->CopyEngine.Size		    = MEGABYTE * 16;
		RS->CopyEngine.TempBuffer   = TempBuffer;

		SETDEBUGNAME(TempBuffer, __func__);

		CD3DX12_RANGE Range(0, 0);
		HR = TempBuffer->Map(0, &Range, (void**)&RS->CopyEngine.Buffer); CheckHR(HR, ASSERTONFAIL("FAILED TO MAP TEMP BUFFER"));
	}


	/************************************************************************************************/


	void ReserveTempSpace(RenderSystem* RS, size_t Size, void*& CPUMem, size_t& Offset)
	{
		// TODO: make thread Safe

		void* out = nullptr;
		auto& CopyEngine = RS->CopyEngine;

		size_t SizePlusOffset = Size + 512;

		auto ResizeBuffer = [&]()
		{
			AddTempBuffer(CopyEngine.TempBuffer, RS);
			CopyEngine.TempBuffer->Unmap(0, 0);

			Push_DelayedRelease(RS, CopyEngine.TempBuffer);

			size_t NewBufferSize = CopyEngine.Size;
			while (NewBufferSize < SizePlusOffset)
				NewBufferSize = NewBufferSize * 2;

			D3D12_RESOURCE_DESC   Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(NewBufferSize);
			D3D12_HEAP_PROPERTIES HEAP_Props	= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

			ID3D12Resource* TempBuffer;
			HRESULT HR = RS->pDevice->CreateCommittedResource(&HEAP_Props, D3D12_HEAP_FLAG_NONE,
				&Resource_DESC, D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr, IID_PPV_ARGS(&TempBuffer));

			RS->CopyEngine.Position   = 0;
			RS->CopyEngine.Last		  = 0;
			RS->CopyEngine.Size       = NewBufferSize;
			RS->CopyEngine.TempBuffer = TempBuffer;
			SETDEBUGNAME(TempBuffer, "TEMPORARY");

			CD3DX12_RANGE Range(0, 0);
			HR = TempBuffer->Map(0, &Range, (void**)&RS->CopyEngine.Buffer); CheckHR(HR, ASSERTONFAIL("FAILED TO MAP TEMP BUFFER"));


			return ReserveTempSpace(RS, Size, CPUMem, Offset);
		};

		// Not enough remaining Space in Buffer GOTO Beginning
		if	(CopyEngine.Position + SizePlusOffset > CopyEngine.Size)
			CopyEngine.Position = 0;

		// Buffer too Small
		if (SizePlusOffset > CopyEngine.Size)
		{
			ResizeBuffer();
			return ReserveTempSpace(RS, Size, CPUMem, Offset);
		}

		if(CopyEngine.Last <= CopyEngine.Position + SizePlusOffset)
		{	// Safe, Do Upload

			size_t AlignmentOffset = 512 - (size_t(CopyEngine.Buffer + CopyEngine.Position) % 512);
			AlignmentOffset = (AlignmentOffset == 512) ? 0 : AlignmentOffset;

			CPUMem = CopyEngine.Buffer + CopyEngine.Position + AlignmentOffset;
			Offset = CopyEngine.Position + AlignmentOffset;
			CopyEngine.Position += SizePlusOffset;
		}
		else if (CopyEngine.Last > CopyEngine.Position)
		{	// Potential Overlap condition
			if(CopyEngine.Position + SizePlusOffset  < CopyEngine.Last)
			{	// Safe, Do Upload
				size_t AlignmentOffset = 512 - (size_t(CopyEngine.Buffer + CopyEngine.Position) % 512);

				CPUMem = CopyEngine.Buffer + CopyEngine.Position + AlignmentOffset;
				Offset = CopyEngine.Position + AlignmentOffset;
				CopyEngine.Position += SizePlusOffset;
			}
			else
			{	// Resize Buffer and Try again
				ResizeBuffer();
				return ReserveTempSpace(RS, Size, CPUMem, Offset);
			}
		}
	}


	/************************************************************************************************/


	void UpdateGPUResource(RenderSystem* RS, void* Data, size_t Size, ID3D12Resource* Dest)
	{
		FK_ASSERT(Data);
		FK_ASSERT(Dest);

		auto& CopyEngine = RS->CopyEngine;
		ID3D12GraphicsCommandList* CS = RS->_GetCurrentCommandList();

		void* _ptr = nullptr;
		size_t Offset;
		ReserveTempSpace(RS, Size, _ptr, Offset);

		FK_ASSERT(_ptr, "UPLOAD ERROR!");

		memcpy(_ptr, Data, Size);
		CS->CopyBufferRegion(Dest, 0, CopyEngine.TempBuffer, Offset, Size);
	}


	/************************************************************************************************/


	void CopyEnginePostFrameUpdate(RenderSystem* RS)
	{
		auto& CopyEngine = RS->CopyEngine;
		CopyEngine.Last  = CopyEngine.Position;
	}


	/************************************************************************************************/


	bool RenderSystem::Initiate( Graphics_Desc* in)
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
		FenceCounter	   = 0;
		FenceUploadCounter = 0;
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
			FK_LOG_INFO("Failed to create A DX12 Device!");

			// Trying again with a DX11 Feature Level
			HR = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device));
			if (FAILED(HR))
			{
				FK_LOG_INFO("Failed to create A DX11 Device!");
				MessageBox(NULL, L"FAILED TO CREATE D3D12 ADAPTER! GET A NEWER COMPUTER", L"ERROR!", MB_OK);
				return false;
			}
		}

#if USING( DEBUGGRAPHICS )
		HR =  Device->QueryInterface(__uuidof(ID3D12DebugDevice), (void**)&DebugDevice);
#else
		DebugDevice = nullptr;
#endif
		
		{
			ID3D12Fence* NewFence = nullptr;
			HR = Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&NewFence));
			FK_ASSERT(FAILED(HR), "FAILED TO CREATE FENCE!");
			SETDEBUGNAME(NewFence, "GRAPHICS FENCE");

			FK_LOG_INFO("GRAPHICS FENCE CREATED: %u", NewFence);

			Fence = NewFence;
			ObjectsCreated.push_back(NewFence);
		}

		{
			ID3D12Fence* NewFence = nullptr;
			HR = Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&NewFence));
			FK_ASSERT(FAILED(HR), "FAILED TO CREATE FENCE!");
			SETDEBUGNAME(NewFence, "COPY FENCE");

			FK_LOG_INFO("GRAPHICS COPY FENCE CREATED: %u", NewFence);

			CopyFence = NewFence;
			ObjectsCreated.push_back(NewFence);
		}

		for(size_t I = 0; I < 3; ++I){
			CopyFences[I].FenceHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			CopyFences[I].FenceValue  = 0;
		}
			
		D3D12_COMMAND_QUEUE_DESC CQD		= {};
		CQD.Flags							            = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
		CQD.Type								        = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;

		D3D12_COMMAND_QUEUE_DESC UploadCQD	= {};
		UploadCQD.Flags							        = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
		UploadCQD.Type								    = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY;

		D3D12_COMMAND_QUEUE_DESC ComputeCQD = {};
		ComputeCQD.Flags								= D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
		ComputeCQD.Type									= D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE;

		IDXGIFactory5*				DXGIFactory         = nullptr;
		IDXGIAdapter4*				DXGIAdapter			= nullptr;
		

		HR = Device->CreateCommandQueue(&CQD,			IID_PPV_ARGS(&GraphicsQueue));		FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND QUEUE!");
		HR = Device->CreateCommandQueue(&UploadCQD,		IID_PPV_ARGS(&UploadQueue));		FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND QUEUE!");
		HR = Device->CreateCommandQueue(&ComputeCQD,	IID_PPV_ARGS(&ComputeQueue));		FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND QUEUE!");


		ObjectsCreated.push_back(GraphicsQueue);
		ObjectsCreated.push_back(UploadQueue);
		ObjectsCreated.push_back(ComputeQueue);

		FK_LOG_INFO("GRAPHICS COMMAND QUEUE CREATED: %u", CQD);
		FK_LOG_INFO("GRAPHICS COMPUTE QUEUE CREATED: %u", ComputeCQD);
		FK_LOG_INFO("GRAPHICS UPLOAD  QUEUE CREATED: %u", UploadCQD);

		SETDEBUGNAME(GraphicsQueue, "GRAPHICS QUEUE");
		SETDEBUGNAME(UploadQueue,	"UPLOAD QUEUE");
		SETDEBUGNAME(ComputeQueue,	"COMPUTE QUEUE");


		D3D12_DESCRIPTOR_HEAP_DESC	FrameTextureHeap_DESC = {};
		FrameTextureHeap_DESC.Flags				= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		FrameTextureHeap_DESC.NumDescriptors	= 1024 * 64;
		FrameTextureHeap_DESC.NodeMask			= 0;
		FrameTextureHeap_DESC.Type				= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

		D3D12_DESCRIPTOR_HEAP_DESC	GPUFrameTextureHeap_DESC = {};
		GPUFrameTextureHeap_DESC.Flags			= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		GPUFrameTextureHeap_DESC.NumDescriptors	= 1024 * 64;
		GPUFrameTextureHeap_DESC.NodeMask		= 0;
		GPUFrameTextureHeap_DESC.Type			= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

		D3D12_DESCRIPTOR_HEAP_DESC	RenderTargetHeap_DESC = {};
		RenderTargetHeap_DESC.Flags				= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		RenderTargetHeap_DESC.NumDescriptors	= 128;
		RenderTargetHeap_DESC.NodeMask			= 0;
		RenderTargetHeap_DESC.Type				= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

		D3D12_DESCRIPTOR_HEAP_DESC	DepthStencilHeap_DESC = {};
		DepthStencilHeap_DESC.Flags				= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		DepthStencilHeap_DESC.NumDescriptors	= 128;
		DepthStencilHeap_DESC.NodeMask			= 0;
		DepthStencilHeap_DESC.Type				= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV;


		FINALLY
			if (!InitiateComplete)
			{
				for (auto O : ObjectsCreated)
					O->Release();
			}
		FINALLYOVER;


		// Create Resources
		{
			for(size_t I = 0; I < QueueSize; ++I)
			{
				UploadQueues[I].Initiate(Device, ObjectsCreated);

				for(size_t II = 0; II < MaxThreadCount; ++II)
				{
					ID3D12GraphicsCommandList2*	CommandList			= nullptr;
					ID3D12CommandAllocator*		GraphicsAllocator	= nullptr;
					ID3D12CommandAllocator*		ComputeAllocator	= nullptr;
					ID3D12GraphicsCommandList2*	ComputeList			= nullptr;

					HR = Device->CreateCommandAllocator	(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,		IID_PPV_ARGS(&GraphicsAllocator));	FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND ALLOCATOR!");
					HR = Device->CreateCommandAllocator	(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE,		IID_PPV_ARGS(&ComputeAllocator));	FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND ALLOCATOR!");
					HR = Device->CreateCommandList		(0, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,	GraphicsAllocator,	nullptr, __uuidof(ID3D12CommandList), (void**)&CommandList);	FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND LIST!");
					HR = Device->CreateCommandList		(0, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE,	ComputeAllocator,	nullptr, __uuidof(ID3D12CommandList), (void**)&ComputeList);	FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND LIST!");

					FK_VLOG(10, "GRAPHICS COMMANDLIST DIRECT  CREATED: %u", CommandList);
					FK_VLOG(10, "GRAPHICS COMMANDLIST COMPUTE CREATED: %u", ComputeList);

					FK_ASSERT(CommandList != nullptr, "FAILED TO CREATE COMMAND LIST");

					SETDEBUGNAME(ComputeAllocator,	"COMPUTE ALLOCATOR");
					SETDEBUGNAME(GraphicsAllocator, "GRAPHICS ALLOCATOR");


					ObjectsCreated.push_back(GraphicsAllocator);
					ObjectsCreated.push_back(ComputeAllocator);

					ObjectsCreated.push_back(ComputeList);
					ObjectsCreated.push_back(CommandList);

					FK_VLOG(10, "CLOSING AND RESETING COMMAND LISTS");

					CommandList->Close();
					ComputeList->Close();

					FK_VLOG(10, "RESETING COMMAND LIST ALLOCACTORS");

					GraphicsAllocator->Reset();
					ComputeAllocator->Reset();

					FK_VLOG(10, "FINISHED RESETING COMMAND LISTS");

					FrameResources[I].TempBuffers				= TempResourceList(in->Memory);
					FrameResources[I].ComputeList[II]			= static_cast<ID3D12GraphicsCommandList3*>(ComputeList);
					FrameResources[I].CommandListsUsed[II]		= false;
					FrameResources[I].ComputeCLAllocator[II]	= ComputeAllocator;
					FrameResources[I].GraphicsCLAllocator[II]	= GraphicsAllocator;
					FrameResources[I].CommandLists[II]			= static_cast<ID3D12GraphicsCommandList3*>(CommandList);
				}

				ID3D12DescriptorHeap* SRVHeap		= nullptr;
				ID3D12DescriptorHeap* GPUSRVHeap	= nullptr;
				ID3D12DescriptorHeap* RTVHeap		= nullptr;
				ID3D12DescriptorHeap* DSVHeap		= nullptr;									

				FK_VLOG(10, "Creating Descriptor Heaps");

				HR = Device->CreateDescriptorHeap(&FrameTextureHeap_DESC,		IID_PPV_ARGS(&SRVHeap));	FK_ASSERT(FAILED(HR), "FAILED TO CREATE SRV Heap HEAP!");
				HR = Device->CreateDescriptorHeap(&GPUFrameTextureHeap_DESC,	IID_PPV_ARGS(&GPUSRVHeap));	FK_ASSERT(FAILED(HR), "FAILED TO CREATE SRV Heap HEAP!");
				HR = Device->CreateDescriptorHeap(&RenderTargetHeap_DESC,		IID_PPV_ARGS(&RTVHeap));	FK_ASSERT(FAILED(HR), "FAILED TO CREATE SRV Heap HEAP!");
				HR = Device->CreateDescriptorHeap(&DepthStencilHeap_DESC,		IID_PPV_ARGS(&DSVHeap));	FK_ASSERT(FAILED(HR), "FAILED TO CREATE SRV Heap HEAP!");

				FK_LOG_INFO("GRAPHICS FRAME TEXTURE HEAP CREATED: %u", SRVHeap);
				FK_LOG_INFO("GRAPHICS GPU FRAME TEXTURE HEAP DIRECT  CREATED: %u", GPUSRVHeap);
				FK_LOG_INFO("GRAPHICS RENDER TARGET HEAP CREATED: %u", RTVHeap);
				FK_LOG_INFO("GRAPHICS DEPTH STENCIL HEAP CREATED: %u", DSVHeap);


				SETDEBUGNAME(SRVHeap,		"RESOURCEHEAP");
				SETDEBUGNAME(GPUSRVHeap,	"GPURESOURCEHEAP");
				SETDEBUGNAME(RTVHeap,		"RENDERTARGETHEAP");
				SETDEBUGNAME(DSVHeap,		"DSVHEAP");

				UploadQueues[I].UploadCount = 0;

				FrameResources[I].DescHeap.DescHeap    = SRVHeap;
				FrameResources[I].GPUDescHeap.DescHeap = GPUSRVHeap;
				FrameResources[I].RTVHeap.DescHeap     = RTVHeap;
				FrameResources[I].DSVHeap.DescHeap	   = DSVHeap;
				FrameResources[I].ThreadsIssued        = 0;
			}

			FrameResources[0].CommandLists[0]->Reset(FrameResources[0].GraphicsCLAllocator[0], nullptr);

			HR = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&DXGIFactory));

			FK_ASSERT(FAILED(HR), "FAILED TO CREATE DXGIFactory!"  );
		}

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
			CurrentIndex			= 0;
			CurrentUploadIndex		= 0;
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
		{
			Texture2D_Desc NullUAV_Desc;
			NullUAV_Desc.Height		  = 1;
			NullUAV_Desc.Width		  = 1;
			NullUAV_Desc.Read         = false;
			NullUAV_Desc.Write        = false;
			NullUAV_Desc.MipLevels    = 1;
			NullUAV_Desc.UAV		  = true;
			NullUAV_Desc.CV			  = true;
			NullUAV_Desc.RenderTarget = true;

			NullUAV = FlexKit::CreateTexture2D(this, &NullUAV_Desc);
			SETDEBUGNAME(NullUAV, "NULL UAV");

			ObjectsCreated.push_back(NullUAV);
		}
		{
			Texture2D_Desc NullSRV_Desc;
			NullSRV_Desc.Height		  = 1;
			NullSRV_Desc.Width		  = 1;
			NullSRV_Desc.Read         = false;
			NullSRV_Desc.Write        = false;
			NullSRV_Desc.CV           = true;
			NullSRV_Desc.MipLevels    = 1;
			NullSRV_Desc.UAV		  = false;
			NullSRV_Desc.CV			  = false;
			NullSRV_Desc.RenderTarget = false;
			NullSRV_Desc.Format		  = FORMAT_2D::R32G32B32A32_FLOAT;

			NullSRV = FlexKit::CreateTexture2D(this, &NullSRV_Desc);
			SETDEBUGNAME( NullSRV, "NULL SRV");
			
			ObjectsCreated.push_back(NullSRV);
		}
		{
			NullSRV1D = CreateShaderResource(this, 1024, "NULL SRV 1D");
			ObjectsCreated.push_back(NullSRV1D[0]);
			ObjectsCreated.push_back(NullSRV1D[1]);
			ObjectsCreated.push_back(NullSRV1D[2]);
		}

		InitiateComplete = true;
		InitiateCopyEngine(this);
		
		Library.Initiate(this, in->TempMemory);

		FreeList.Allocator = in->Memory;

		ReadyUploadQueues();

		_ResetDescHeap();
		_ResetRTVHeap();
		_ResetGPUDescHeap();
		_ResetDSVHeap();

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

		FK_LOG_INFO("Releasing RenderSystem");

		while (FreeList.size())
			Free_DelayedReleaseResources(this);


		for (auto& UQ : UploadQueues)
			UQ.Release();

		for (auto& FR : FrameResources)
			FR.Release();


		PipelineStates.ReleasePSOs();

		CopyEngine.Release();

		NullConstantBuffer.Release();
		NullSRV1D.Release();

		if(NullUAV) NullUAV->Release();
		if(NullSRV) NullSRV->Release();

		if(GraphicsQueue)	GraphicsQueue->Release();
		if(UploadQueue)		UploadQueue->Release();
		if(ComputeQueue)	ComputeQueue->Release();
		if(pGIFactory)		pGIFactory->Release();
		if(pDXGIAdapter)	pDXGIAdapter->Release();
		if(pDevice)			pDevice->Release();
		if(Fence)			Fence->Release();
		if(CopyFence)		CopyFence->Release();

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
		FK_LOG_INFO("Reloading PSO!");

		PipelineStates.QueuePSOLoad(State, Memory);
	}


	/************************************************************************************************/


	size_t	RenderSystem::GetCurrentFrame()
	{
		return CurrentFrame;
	}


	/************************************************************************************************/

	
	void RenderSystem::PresentWindow(RenderWindow* Window)
	{
		//CopyEnginePostFrameUpdate(this);

		Window->SwapChain_ptr->Present(1, 0);
		Window->BufferIndex = Window->SwapChain_ptr->GetCurrentBackBufferIndex();

		Fences[CurrentIndex].FenceValue = ++FenceCounter;
		GraphicsQueue->Signal(Fence, FenceCounter);

		++CurrentFrame;
	}



	/************************************************************************************************/


	void RenderSystem::WaitforGPU()
	{
		size_t Index			= CurrentIndex;
		auto Fence				= this->Fence;
		size_t CompleteValue	= Fence->GetCompletedValue();

		if (CompleteValue < Fences[Index].FenceValue) {
			HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
			Fence->SetEventOnCompletion(Fences[Index].FenceValue, eventHandle);
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
			ReleaseTempResources();
		}
	}


	/************************************************************************************************/


	D3D12_GPU_VIRTUAL_ADDRESS RenderSystem::GetVertexBufferAddress(const VertexBufferHandle VB)
	{
		return VertexBuffers.GetResource(VB)->GetGPUVirtualAddress();
	}


	/************************************************************************************************/


	size_t RenderSystem::GetVertexBufferSize(const VertexBufferHandle VB)
	{
		return VertexBuffers.GetBufferSize(VB);
	}


	/************************************************************************************************/


	uint32_t RenderSystem::GetTag(TextureHandle handle)
	{
		return RenderTargets.GetTag(handle);
	}


	/************************************************************************************************/


	void RenderSystem::SetTag(TextureHandle handle, uint32_t Tag)
	{
		RenderTargets.SetTag(handle, Tag);
	}


	/************************************************************************************************/



	const size_t	RenderSystem::GetTextureElementSize(TextureHandle handle) const
	{
		auto Format = Textures.GetFormat(handle);
		return GetFormatElementSize(Format);
	}


	/************************************************************************************************/


	const uint2	RenderSystem::GetTextureWH(TextureHandle handle) const
	{
		return Textures.GetWH(handle);
	}


	/************************************************************************************************/


	const uint2	RenderSystem::GetRenderTargetWH(TextureHandle handle) const
	{
		return RenderTargets.GetWH(handle);
	}


	/************************************************************************************************/


	FORMAT_2D RenderSystem::GetTextureFormat(TextureHandle handle) const
	{
		return DXGIFormat2TextureFormat(Textures.GetFormat(handle));
	}


	/************************************************************************************************/


	DXGI_FORMAT RenderSystem::GetTextureDeviceFormat(TextureHandle handle) const
	{
		return Textures.GetFormat(handle);
	}


	/************************************************************************************************/


	DXGI_FORMAT RenderSystem::_GetDXGIRenderTargetFormat(TextureHandle handle) const
	{
		return RenderTargets.GetFormat(handle);
	}


	/************************************************************************************************/


	void RenderSystem::UploadTexture(TextureHandle handle, byte* buffer, size_t bufferSize)
	{
		auto resource	= _GetTextureResource(handle);
		auto wh			= GetTextureWH(handle);
		auto formatSize = GetTextureElementSize(handle); FK_ASSERT(formatSize != -1);

		/*
		struct SubResourceUpload_Desc
		{
			const void* Data; 
			size_t	Size;
			size_t	RowSize;
			size_t	RowCount;
			uint2	WH;
			size_t	SubResourceStart;
			size_t	SubResourceCount;
			size_t*	SubResourceSizes;
			size_t	ElementSize;
		};
		*/

		size_t resourceSize = bufferSize;
		size_t offset = 0;

		SubResourceUpload_Desc desc;
		desc.Data				= buffer;
		desc.RowSize			= wh[1] * formatSize;
		desc.RowCount			= wh[0];
		desc.Size				= bufferSize;
		desc.SubResourceCount	= 1;
		desc.SubResourceSizes	= &resourceSize;
		desc.SubResourceOffset	= &offset;
		desc.SubResourceStart	= 0;
		desc.WH					= wh;
		desc.ElementSize		= formatSize;

		UpdateSubResourceByUploadQueue(this, resource, &desc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}


	void RenderSystem::UploadTexture(TextureHandle handle, byte* buffer, size_t bufferSize, uint2 WH, size_t resourceCount, size_t* mipOffsets, iAllocator* temp) // Uses Upload Queue
	{
		auto resource	= _GetTextureResource(handle);
		auto format		=  GetTextureFormat(handle);
		auto formatSize =  GetTextureElementSize(handle); FK_ASSERT(formatSize != -1);

		/*
		struct SubResourceUpload_Desc
		{
			const void* Data; 
			size_t	Size;
			size_t	RowSize;
			size_t	RowCount;
			uint2	WH;
			size_t	SubResourceStart;
			size_t	SubResourceCount;
			size_t*	SubResourceSizes;
			size_t	ElementSize;
		};
		*/

		auto WHtemp = WH;
		static_vector<size_t> subResourceSizes;
		for (size_t I = 0; I < resourceCount; ++I) 
		{
			subResourceSizes.push_back(WHtemp.Product() * formatSize);
			WHtemp /= 2;
		}

		size_t zero = 0;

		SubResourceUpload_Desc desc;
		desc.Data				= buffer;
		desc.RowSize			= WH[1] * formatSize;
		desc.RowCount			= WH[0];
		desc.Size				= bufferSize;
		desc.SubResourceCount	= resourceCount;
		desc.SubResourceSizes	= subResourceSizes.begin();
		desc.SubResourceOffset  = resourceCount == 1 ? &zero : mipOffsets;
		desc.SubResourceStart	= 0;
		desc.WH					= WH;
		desc.ElementSize		= formatSize;

		UpdateSubResourceByUploadQueue(this, resource, &desc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}


	/************************************************************************************************/


	D3D12_GPU_VIRTUAL_ADDRESS RenderSystem::GetConstantBufferAddress(const ConstantBufferHandle CB)
	{
		// TODO: deal with Push Buffer Offsets
		return ConstantBuffers.GetBufferResource(CB)->GetGPUVirtualAddress();
	}


	/************************************************************************************************/


	size_t RenderSystem::GetTextureFrameGraphIndex(TextureHandle Texture)
	{
		auto FrameID = GetCurrentFrame();
		return Textures.GetFrameGraphIndex(Texture, FrameID);
	}


	/************************************************************************************************/


	void RenderSystem::SetTextureFrameGraphIndex(TextureHandle Texture, size_t Index)
	{
		auto FrameID = GetCurrentFrame();
		Textures.SetFrameGraphIndex(Texture, FrameID, Index);
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


	TextureHandle RenderSystem::CreateDepthBuffer( const uint2 WH, const bool UseFloat, const size_t bufferCount)
	{
		Texture2D_Desc Desc(WH[1], WH[0], FORMAT_2D::D32_FLOAT, CPUACCESSMODE::NONE, SPECIALFLAGS::DEPTHSTENCIL);

		ID3D12Resource* Resources[3];

		for(size_t itr = 0; itr < bufferCount; ++itr)
			Resources[itr] = CreateDepthBufferResource(this, &Desc, UseFloat);

		auto DepthBuffer = RenderTargets.AddResource(Desc, Resources, bufferCount, 1, DRS_DEPTHBUFFERWRITE, TF_RenderTarget);

		return DepthBuffer;
	}


	/************************************************************************************************/


	TextureHandle RenderSystem::CreateStructuredResource(const size_t size, const size_t elementSize)
	{
		Texture2D_Desc desc;
		desc.CV				= false;
		desc.Format			= FORMAT_2D::UNKNOWN;
		desc.Height			= 1;
		desc.Width			= size * elementSize;
		desc.initialData	= nullptr;
		desc.MipLevels		= 1;
		desc.Read			= false;
		desc.RenderTarget	= false;
		desc.UAV			= false;
		desc.Write			= false;
		desc.FLAGS			= SPECIALFLAGS::Structred1D;

		auto newTexture = FlexKit::CreateTexture2D(this, &desc);

		ID3D12Resource* Resources[] = {
			newTexture.Texture
		};

		Texture2D_Desc Desc(1, size, FORMAT_2D::UNKNOWN, CPUACCESSMODE::NONE, SPECIALFLAGS::NONE, 1);
		auto Texture = Textures.AddResource(Desc, Resources, 1, 1, DRS_ShaderResource, TF_NONE);

		return Texture;
	}


	/************************************************************************************************/


	TextureHandle RenderSystem::CreateTexture2D(const uint2 WH, const FORMAT_2D Format, const size_t mipLevels)
	{
		Texture2D_Desc desc;
		desc.CV				= false;
		desc.Format			= Format;
		desc.Height			= WH[1];
		desc.Width			= WH[0];
		desc.initialData	= nullptr;
		desc.MipLevels		= mipLevels;
		desc.Read			= false;
		desc.RenderTarget	= false;
		desc.UAV			= false;
		desc.Write			= false;

		auto newTexture = FlexKit::CreateTexture2D(this, &desc);
		
		ID3D12Resource* Resources[] = {
			newTexture.Texture
		};

		Texture2D_Desc Desc(WH[1], WH[0], Format, CPUACCESSMODE::NONE, SPECIALFLAGS::NONE, mipLevels);
		auto Texture = Textures.AddResource(Desc, Resources, 1, mipLevels, DRS_ShaderResource, TF_NONE);

		return Texture;
	}


	/************************************************************************************************/


	TextureHandle RenderSystem::CreateTexture2D(const uint2 WH, const FORMAT_2D Format, const size_t mipLevels, ID3D12Resource** Resources, size_t ResourceCount)
	{
		Texture2D_Desc Desc(WH[1], WH[0], Format, CPUACCESSMODE::NONE, SPECIALFLAGS::DEPTHSTENCIL, mipLevels);

		auto Texture = Textures.AddResource(Desc, Resources, ResourceCount, mipLevels, DRS_ShaderResource, TF_NONE);
		return Texture;
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
		Resource_DESC.Alignment          = 0;
		Resource_DESC.DepthOrArraySize   = 1;
		Resource_DESC.Dimension          = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		Resource_DESC.Layout             = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Resource_DESC.Width              = resourceSize;
		Resource_DESC.Height             = 1;
		Resource_DESC.Format             = DXGI_FORMAT_UNKNOWN;
		Resource_DESC.SampleDesc.Count   = 1;
		Resource_DESC.SampleDesc.Quality = 0;
		Resource_DESC.Flags              = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		D3D12_HEAP_PROPERTIES HEAP_Props = {};
		HEAP_Props.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type                  = D3D12_HEAP_TYPE_DEFAULT;
		HEAP_Props.MemoryPoolPreference  = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask      = 0;
		HEAP_Props.VisibleNodeMask       = 0;

		static_vector<ID3D12Resource*, 3> buffers;

		for (size_t I = 0; I < (tripleBuffer ? BufferCount : 1); ++I)
		{
			ID3D12Resource* Resource = nullptr;
			HRESULT HR = pDevice->CreateCommittedResource(
								&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
								&Resource_DESC, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
								IID_PPV_ARGS(&Resource));

			CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE SHADERRESOURCE!"));
			buffers.push_back(Resource);

			//SETDEBUGNAME(Resource, __func__);
		}

		return BufferUAVs.AddResource(buffers, resourceSize, DeviceResourceState::DRS_Write); // DRS Write assumed on first use, ignores initial state specified above!
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


	void RenderSystem::SetObjectState(TextureHandle handle, DeviceResourceState state)
	{
		Textures.SetState(handle, state);
	}


	/************************************************************************************************/


	DeviceResourceState RenderSystem::GetObjectState(const QueryHandle handle) const
	{
		return Queries.GetResourceState(handle);
	}


	/************************************************************************************************/


	DeviceResourceState RenderSystem::GetObjectState(const SOResourceHandle handle) const
	{
		return StreamOutTable.GetResourceState(handle);
	}


	/************************************************************************************************/


	DeviceResourceState RenderSystem::GetObjectState(const UAVResourceHandle	handle) const
	{
		return BufferUAVs.GetResourceState(handle);
	}


	/************************************************************************************************/


	DeviceResourceState RenderSystem::GetObjectState(const TextureHandle		handle) const
	{
		return Textures.GetState(handle);
	}


	/************************************************************************************************/


	ID3D12Resource* RenderSystem::GetObjectDeviceResource(const ConstantBufferHandle handle) const
	{
		return ConstantBuffers.GetBufferResource(handle);
	}


	/************************************************************************************************/


	ID3D12Resource* RenderSystem::GetObjectDeviceResource(const TextureHandle	handle) const
	{
		return Textures.GetResource(handle);
	}


	/************************************************************************************************/


	ID3D12Resource*	RenderSystem::GetObjectDeviceResource(const SOResourceHandle handle) const
	{
		return StreamOutTable.GetResource(handle);
	}


	/************************************************************************************************/


	ID3D12Resource*	RenderSystem::GetObjectDeviceResource(const UAVResourceHandle	handle) const
	{
		return BufferUAVs.GetResource(handle);
	}


	/************************************************************************************************/


	ID3D12Resource*	RenderSystem::GetSOCounterResource(const SOResourceHandle handle) const
	{
		return StreamOutTable.GetResourceCounter(handle);
	}



	/************************************************************************************************/


	size_t RenderSystem::GetStreamOutBufferSize(const SOResourceHandle handle) const
	{
		return StreamOutTable.GetResourceSize(handle);
	}


	/************************************************************************************************/


	ID3D12Resource* RenderSystem::GetUploadResource()
	{
		return CopyEngine.TempBuffer;
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


	void RenderSystem::ReleaseDB(TextureHandle Handle)
	{
		FK_LOG_INFO("Leaking DepthBuffer!");
		//TODO: Release DepthBuffer Properly!
	}


	/************************************************************************************************/


	void RenderSystem::ReleaseTexture(TextureHandle Handle)
	{
		Textures.ReleaseTexture(Handle);
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


	Texture2D CreateDepthBufferResource(RenderSystem* RS, Texture2D_Desc* desc_in, bool Float32)
	{
		FK_ASSERT(desc_in != nullptr);
		FK_ASSERT(RS != nullptr);

		ID3D12Resource*			Resource		= nullptr;
		D3D11_TEXTURE2D_DESC	Desc			= { 0 };
		D3D12_RESOURCE_DESC		Resource_DESC	= {};
		D3D12_HEAP_PROPERTIES	HEAP_Props		= {};
		D3D12_CLEAR_VALUE		Clear;
		Texture2D				NewTexture;

		Clear.Color[0] = 0.0f;
		Clear.Color[1] = 0.0f;
		Clear.Color[2] = 0.0f;
		Clear.Color[3] = 0.0f;

		Clear.DepthStencil.Depth	= 1.0f;
		Clear.DepthStencil.Stencil	= 0;

		if (Float32){
			Resource_DESC.Width					= desc_in->Width;
			Resource_DESC.Height				= desc_in->Height;

			Resource_DESC.Alignment				= 0;
			Resource_DESC.DepthOrArraySize		= 1;
			Resource_DESC.Dimension				= D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			Resource_DESC.Layout				= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
			Resource_DESC.Format				= DXGI_FORMAT_D32_FLOAT;
			Resource_DESC.SampleDesc.Count		= 1;
			Resource_DESC.SampleDesc.Quality	= 0;
			Resource_DESC.Flags					= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

			HEAP_Props.CPUPageProperty			= D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			HEAP_Props.Type						= D3D12_HEAP_TYPE_DEFAULT;
			HEAP_Props.MemoryPoolPreference		= D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
			HEAP_Props.CreationNodeMask			= 0;
			HEAP_Props.VisibleNodeMask			= 0;

			Clear.Format = DXGI_FORMAT_D32_FLOAT;
		}
		else{
			Resource_DESC.Alignment				= 0;
			Resource_DESC.DepthOrArraySize		= 1;
			Resource_DESC.Dimension				= D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			Resource_DESC.Layout				= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
			Resource_DESC.Width					= desc_in->Width;
			Resource_DESC.Height				= desc_in->Height;
			Resource_DESC.Format				= DXGI_FORMAT_D24_UNORM_S8_UINT;
			Resource_DESC.SampleDesc.Count		= 1;
			Resource_DESC.SampleDesc.Quality	= 0;
			Resource_DESC.Flags					= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

			HEAP_Props.CPUPageProperty	     = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			HEAP_Props.Type				     = D3D12_HEAP_TYPE_DEFAULT;
			HEAP_Props.MemoryPoolPreference  = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
			HEAP_Props.CreationNodeMask	     = 0;
			HEAP_Props.VisibleNodeMask		 = 0;

			Clear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		}


		CheckHR(RS->pDevice->CreateCommittedResource(
			&HEAP_Props,
			D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &Resource_DESC,
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&Clear, IID_PPV_ARGS(&Resource)), [&]() {});

		NewTexture.Format		= Resource_DESC.Format;
		NewTexture.Texture      = Resource;
		NewTexture.WH			= {Resource_DESC.Width,Resource_DESC.Height};

		FK_LOG_INFO("Creating Depth Buffer Resource!");

		FK_ASSERT(Resource != nullptr, "Failed Creating Depth Buffer Resource!");
		SETDEBUGNAME(Resource, __func__);

		return (NewTexture);
	}


	/************************************************************************************************/


	bool CreateDepthBuffer(RenderSystem* RS, uint2 Dimensions, DepthBuffer_Desc& DepthDesc, DepthBuffer* out, ID3D12GraphicsCommandList* CL)
	{
		// Create Depth Buffer
		Texture2D_Desc desc;
		desc.Height      = Dimensions[1];
		desc.Width       = Dimensions[0];

		D3D12_DEPTH_STENCIL_VIEW_DESC DepthStencilDesc = {};
		DepthStencilDesc.Format        =  DepthDesc.Float32 ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_D24_UNORM_S8_UINT;
		DepthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		DepthStencilDesc.Flags         = D3D12_DSV_FLAG_NONE;

		D3D12_CLEAR_VALUE DepthOptimizedClearValue    = {};
		DepthOptimizedClearValue.Format               = DXGI_FORMAT_D32_FLOAT;
		DepthOptimizedClearValue.DepthStencil.Depth   = 1.0f;
		DepthOptimizedClearValue.DepthStencil.Stencil = 0;
		
		for(size_t I = 0; I < DepthDesc.BufferCount; ++I){
			auto DB = CreateDepthBufferResource(RS, &desc, DepthDesc.Float32);
			
			if (!DB) {
				cout << "Failed To Create Depth Buffer!\n";
				FK_ASSERT(0);
			} else {
				SETDEBUGNAME(DB, "DEPTHBUFFER");
				out->Buffer[I] = DB;

#if 0
				if (CL)
					ClearDepthBuffers(RS, CL, { DB });
#endif
			}

		}

		if (CL) {
			/*
			typedef struct D3D12_RESOURCE_BARRIER
			{
				D3D12_RESOURCE_BARRIER_TYPE Type;
				D3D12_RESOURCE_BARRIER_FLAGS Flags;
				union
				{
				D3D12_RESOURCE_TRANSITION_BARRIER Transition;
				D3D12_RESOURCE_ALIASING_BARRIER Aliasing;
				D3D12_RESOURCE_UAV_BARRIER UAV;
				} ;	
			} 	D3D12_RESOURCE_BARRIER;

			typedef struct D3D12_RESOURCE_TRANSITION_BARRIER
			{
				ID3D12Resource *pResource;
				UINT Subresource;
				D3D12_RESOURCE_STATES StateBefore;
				D3D12_RESOURCE_STATES StateAfter;
			} 	D3D12_RESOURCE_TRANSITION_BARRIER;
			*/


			D3D12_RESOURCE_BARRIER Barriers[] = {
				{	D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
					D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE,
					D3D12_RESOURCE_TRANSITION_BARRIER{ out->Buffer[0], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ }
				},

				{	D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
					D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE,
					D3D12_RESOURCE_TRANSITION_BARRIER{ out->Buffer[1], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ }
				},

				{	D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
					D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE,
					D3D12_RESOURCE_TRANSITION_BARRIER{ out->Buffer[2], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ }
				},
			};

			CL->ResourceBarrier(3, Barriers);
		}
		out->Inverted	   = DepthDesc.InverseDepth;
		out->BufferCount   = DepthDesc.BufferCount;
		out->CurrentBuffer = 0;

		return true;
	}


	/************************************************************************************************/


#if 0
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferHandle(DescriptorHeaps* DH, RenderWindow* RW)
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(DH->RTVDescHeap->GetCPUDescriptorHandleForHeapStart(), RW->BufferIndex, DH->DescriptorRTVSize);
	}
#endif

	/************************************************************************************************/


	bool CreateRenderWindow( RenderSystem* RS, RenderWindowDesc* In_Desc, RenderWindow* out )
	{
		SetProcessDPIAware();


		RenderWindow NewWindow = {0};
		static size_t Window_Count = 0;

		Window_Count++;

		// Register Window Class
		auto Window = CreateWindow( L"RENDER_WINDOW", L"Render Window", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_BORDER,
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
		NewWindow.BufferIndex = 0;
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

		Texture2D_Desc Desc;
		Desc.Height	= NewWindow.VP.Height;
		Desc.Width	= NewWindow.VP.Width;
		Desc.Format = FORMAT_2D::R16G16B16A16_FLOAT;

		//CreateBackBuffer
		for (size_t I = 0; I < SwapChainDesc.BufferCount; ++I)
		{
			ID3D12Resource* buffer;
			NewSwapChain_ptr->GetBuffer( I, __uuidof(ID3D12Resource), (void**)&buffer);
			NewWindow.BackBuffer[I] = buffer;

			FK_ASSERT(buffer, "Failed to Create Back Buffer!");
			SETDEBUGNAME(buffer, "BackBuffer");

			auto Handle = RS->RenderTargets.AddResource(Desc, &buffer, 1, 1, DeviceResourceState::DRS_Present, TF_BackBuffer);
			NewWindow.RenderTargets[I] = Handle;

			RS->RenderTargets.SetTag(Handle, GetCRCGUID(BACKBUFFER));
		}

		SetActiveWindow(Window);

		NewWindow.WH[0]         = In_Desc->width;
		NewWindow.WH[1]         = In_Desc->height;
		NewWindow.Close         = false;
		NewWindow.BufferCount	= SwapChainDesc.BufferCount;
		NewWindow.Format		= SwapChainDesc.Format;
		memset(NewWindow.InputBuffer, 0, sizeof(NewWindow.InputBuffer));

		*out = NewWindow;
		return true;
	}

	

	/************************************************************************************************/


	ID3D12Resource* GetBackBufferResource(RenderWindow* Window)
	{
		return Window->BackBuffer[Window->BufferIndex];
	}


	/************************************************************************************************/


	Texture2D GetBackBufferTexture(RenderWindow* Window)
	{
		Texture2D Texture;
		Texture.Format = Window->Format;
		Texture.WH = Window->WH;
		Texture.Texture = GetBackBufferResource(Window);
		Window->BackBuffer[Window->BufferIndex];
		return Texture;
	}
	

	/************************************************************************************************/


	bool ResizeRenderWindow	( RenderSystem* RS, RenderWindow* Window, uint2 HW )
	{
		Release(Window);

		RenderWindowDesc RW_Desc = {};

		RW_Desc.fullscreen = Window->Fullscreen;
		RW_Desc.hInstance;// = Window->hWindow;
		RW_Desc.hWindow;
		RW_Desc.height	= HW[0];
		RW_Desc.width	= HW[1];
		RW_Desc.depth;
		RW_Desc.AA_Count;
		RW_Desc.AA_Quality;

		RECT POS;
		GetWindowRect( Window->hWindow, &POS );
		RW_Desc.POS_X = POS.left;
		RW_Desc.POS_Y = POS.top;

		return CreateRenderWindow( RS, &RW_Desc, Window );
	}


	/************************************************************************************************/


	void RenderSystem::ReleaseTempResources()
	{
		auto& TempBuffers = FrameResources[CurrentIndex].TempBuffers;

		for (auto Res : TempBuffers)
			Res->Release();

		TempBuffers.clear();
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


	Texture2D GetRenderTarget(RenderWindow* RW)
	{
		Texture2D Out;
		auto Res    = GetBackBufferResource(RW);

		Out.Texture = Res;
		Out.WH      = RW->WH;
		Out.Format  = RW->Format;

		return Out;
	}


	/************************************************************************************************/


	void Release( RenderWindow* Window )
	{
		Window->SwapChain_ptr->SetFullscreenState(false, nullptr);

		DestroyWindow(Window->hWindow);

		Window->SwapChain_ptr->Release();
		for(auto W : Window->BackBuffer)
		{
			if(W)
				W->Release();
			W = nullptr;
		}
	}


	/************************************************************************************************/


	DXGI_FORMAT TextureFormat2DXGIFormat(FORMAT_2D F)
	{
		switch (F)
		{
		case FlexKit::FORMAT_2D::R16_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
		case FlexKit::FORMAT_2D::R16G16_UINT:
			return DXGI_FORMAT::DXGI_FORMAT_R16G16_UINT;
		case FlexKit::FORMAT_2D::R8G8B8A8_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
		case FlexKit::FORMAT_2D::R8G8_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_R8G8_UNORM;
		case FlexKit::FORMAT_2D::R32G32B32_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
		case FlexKit::FORMAT_2D::R16G16B16A16_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		case FlexKit::FORMAT_2D::R32G32B32A32_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
		case FlexKit::FORMAT_2D::R32_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		case FlexKit::FORMAT_2D::D32_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
		case FlexKit::FORMAT_2D::BC1_TYPELESS:
			return DXGI_FORMAT::DXGI_FORMAT_BC1_TYPELESS;
		case FlexKit::FORMAT_2D::BC1_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM;
		case FlexKit::FORMAT_2D::BC1_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM_SRGB;
		case FlexKit::FORMAT_2D::BC2_TYPELESS:
			return DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM_SRGB;
		case FlexKit::FORMAT_2D::BC2_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM;
		case FlexKit::FORMAT_2D::BC2_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM_SRGB;
		case FlexKit::FORMAT_2D::BC3_TYPELESS:
			return DXGI_FORMAT::DXGI_FORMAT_BC3_TYPELESS;
		case FlexKit::FORMAT_2D::BC3_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM;
		case FlexKit::FORMAT_2D::BC3_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM_SRGB;
		case FlexKit::FORMAT_2D::BC4_TYPELESS:
			return DXGI_FORMAT::DXGI_FORMAT_BC4_TYPELESS;
		case FlexKit::FORMAT_2D::BC4_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC4_UNORM;
		case FlexKit::FORMAT_2D::BC4_SNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC4_SNORM;
		case FlexKit::FORMAT_2D::BC5_TYPELESS:
			return DXGI_FORMAT::DXGI_FORMAT_BC5_TYPELESS;
		case FlexKit::FORMAT_2D::BC5_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC5_UNORM;
		case FlexKit::FORMAT_2D::BC5_SNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC5_SNORM;
		default:
			return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UINT;
			break;
		}
	}

	
	/************************************************************************************************/


	FORMAT_2D	DXGIFormat2TextureFormat(DXGI_FORMAT F)
	{
		switch (F)
		{
		case DXGI_FORMAT::DXGI_FORMAT_R16G16_UINT:
			return FlexKit::FORMAT_2D::R16G16_UINT;
		case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM:
			return FlexKit::FORMAT_2D::R8G8B8A8_UNORM;
		case DXGI_FORMAT::DXGI_FORMAT_R8G8_UNORM:
			return FlexKit::FORMAT_2D::R8G8_UNORM;
		case DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT:
			return FlexKit::FORMAT_2D::R32G32B32_FLOAT;
		case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT:
			return FlexKit::FORMAT_2D::R16G16B16A16_FLOAT;
		case DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT:
			return FlexKit::FORMAT_2D::R32G32B32A32_FLOAT;
		case DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT:
			return FlexKit::FORMAT_2D::R32_FLOAT;
		case DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT:
			return FlexKit::FORMAT_2D::D32_FLOAT;
		case DXGI_FORMAT::DXGI_FORMAT_BC1_TYPELESS:
			return FlexKit::FORMAT_2D::BC1_TYPELESS;
		case DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM:
			return FlexKit::FORMAT_2D::BC1_UNORM;
		case DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM_SRGB:
			return FlexKit::FORMAT_2D::BC1_UNORM_SRGB;
		case DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM:
			return FlexKit::FORMAT_2D::BC2_UNORM;
		case DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM_SRGB:
			return FlexKit::FORMAT_2D::BC2_UNORM_SRGB;
		case DXGI_FORMAT::DXGI_FORMAT_BC3_TYPELESS:
			return FlexKit::FORMAT_2D::BC3_TYPELESS;
		case DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM:
			return FlexKit::FORMAT_2D::BC3_UNORM;
		case DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM_SRGB:
			return FlexKit::FORMAT_2D::BC3_UNORM_SRGB;
		case DXGI_FORMAT::DXGI_FORMAT_BC4_TYPELESS:
			return FlexKit::FORMAT_2D::BC4_TYPELESS;
		case DXGI_FORMAT::DXGI_FORMAT_BC4_UNORM:
			return FlexKit::FORMAT_2D::BC4_UNORM;
		case DXGI_FORMAT::DXGI_FORMAT_BC4_SNORM:
			return FlexKit::FORMAT_2D::BC4_SNORM;
		case DXGI_FORMAT::DXGI_FORMAT_BC5_TYPELESS:
			return FlexKit::FORMAT_2D::BC5_TYPELESS;
		case DXGI_FORMAT::DXGI_FORMAT_BC5_UNORM:
			return FlexKit::FORMAT_2D::BC5_UNORM;
		case DXGI_FORMAT::DXGI_FORMAT_BC5_SNORM:
			return FlexKit::FORMAT_2D::BC5_SNORM;
		default:
			return FlexKit::FORMAT_2D::UNKNOWN;
			break;
		}
	}


	/************************************************************************************************/

	void UpdateResourceByTemp(RenderSystem* RS, ID3D12Resource* Dest, void* Data, size_t SourceSize, size_t ByteSize, D3D12_RESOURCE_STATES EndState)
	{
		UpdateGPUResource(RS, Data, SourceSize, Dest);

		auto& CopyEngine = RS->CopyEngine;
		ID3D12GraphicsCommandList* CS = RS->_GetCurrentCommandList();

		CS->ResourceBarrier(1, 
				&CD3DX12_RESOURCE_BARRIER::Transition(Dest, D3D12_RESOURCE_STATE_COPY_DEST, EndState, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE));
	}


	/************************************************************************************************/


	FLEXKITAPI void UpdateSubResourceByUploadQueue(RenderSystem* RS, ID3D12Resource* Dest, SubResourceUpload_Desc* Desc, D3D12_RESOURCE_STATES EndState)
	{
		auto& CE					= RS->CopyEngine;
		size_t SrOffset				= 0;
		const size_t SubResCount	= Desc->SubResourceCount;
		const size_t SubResStart	= Desc->SubResourceStart;
		uint2 WH					= Desc->WH;
		size_t elementSize			= 4; // TODO: MAKE THIS DYNAMIC BASED ON TEXTURE TYPE!

		PerFrameUploadQueue& UploadQueue	= RS->_GetCurrentUploadQueue();
		ID3D12GraphicsCommandList* CS		= UploadQueue.UploadList[0];

		for (size_t I = 0; I < SubResCount + SubResStart; ++I) 
		{
			D3D12_RANGE R;
			R.Begin     = Desc->SubResourceOffset[I];
			R.End	    = Desc->SubResourceOffset[I] + Desc->SubResourceSizes[I];

			void* pData			= nullptr;
			size_t TempOffset	= 0;
			ReserveTempSpace(RS, WH[0] * WH[1] * Desc->ElementSize, pData, TempOffset);

			D3D12_PLACED_SUBRESOURCE_FOOTPRINT SubRegion;
			SubRegion.Footprint.Depth    = 1;
			SubRegion.Footprint.Format   = DXGI_FORMAT_R8G8B8A8_UNORM;
			SubRegion.Footprint.RowPitch = elementSize * WH[0];
			SubRegion.Footprint.Width    = WH[0];
			SubRegion.Footprint.Height   = WH[1];
			SubRegion.Offset             = TempOffset;

			auto Destination	= CD3DX12_TEXTURE_COPY_LOCATION(Dest);
			auto Source			= CD3DX12_TEXTURE_COPY_LOCATION(CE.TempBuffer, SubRegion);

			Destination.SubresourceIndex					= I;
			Destination.Type								= D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			Destination.pResource							= Dest;
			Destination.PlacedFootprint.Footprint.Depth		= 1;
			Destination.PlacedFootprint.Footprint.Format	= DXGI_FORMAT_R8G8B8A8_UNORM;
			Destination.PlacedFootprint.Footprint.Width		= WH[0];
			Destination.PlacedFootprint.Footprint.Height	= WH[1];
			Destination.PlacedFootprint.Footprint.RowPitch	= elementSize * WH[0];

			memcpy((char*)pData, (char*)Desc->Data + Desc->SubResourceOffset[I], WH[0] * WH[1] * elementSize);
			auto TextureDesc = Dest->GetDesc();
			CS->CopyTextureRegion(&Destination, 0, 0, 0, &Source, nullptr);
			SrOffset += Desc->SubResourceSizes[I];
			WH /= 2;
		}
	}


	/************************************************************************************************/


	void RenderSystem::UpdateResourceByUploadQueue(ID3D12Resource* Dest, void* Data, size_t Size, size_t ByteSize, D3D12_RESOURCE_STATES EndState)
	{
		FK_ASSERT(Data);
		FK_ASSERT(Dest);

		PerFrameUploadQueue& UploadQueue	= _GetCurrentUploadQueue();
		ID3D12GraphicsCommandList* CS		= UploadQueue.UploadList[0];

		void* _ptr = nullptr;
		size_t Offset = 0;
		ReserveTempSpace(this, Size, _ptr, Offset);

		FK_ASSERT(_ptr, "UPLOAD ERROR!");

		memcpy(_ptr, Data, Size);
		CS->CopyBufferRegion(Dest, 0, CopyEngine.TempBuffer, Offset, Size);
		UploadQueue.UploadCount++;
	}


	
	/************************************************************************************************/


	UploadSegment ReserveUploadBuffer(
		const size_t	uploadSize, 
		RenderSystem&	renderSystem)
	{
		size_t	Offset	= 0;
		void*	_ptr	= nullptr;

		ReserveTempSpace(renderSystem, uploadSize + 512, _ptr, Offset);
		
		size_t alignmentOffset = Offset % 512;
		Offset += alignmentOffset;

		return { .offset = Offset, .uploadSize = uploadSize + 512, .buffer = (char*)_ptr };
	}


	/************************************************************************************************/


	void MoveTexture2UploadBuffer(const UploadSegment& data, byte* source, size_t uploadSize)
	{
		memcpy(data.buffer, source, data.uploadSize > uploadSize ? uploadSize : data.uploadSize);
	}


	/************************************************************************************************/


	void UpdateResourceByTemp( RenderSystem* RS,  FrameBufferedResource* Dest, void* Data, size_t SourceSize, size_t ByteSize, D3D12_RESOURCE_STATES EndState)
	{
		Dest->IncrementCounter();
		UpdateGPUResource(RS, Data, SourceSize, Dest->Get());
		// NOT SURE IF NEEDED, RUNS CORRECTLY WITHOUT FOR THE MOMENT
		RS->_GetCurrentCommandList()->ResourceBarrier(1,
				&CD3DX12_RESOURCE_BARRIER::Transition(Dest->Get(), D3D12_RESOURCE_STATE_COPY_DEST, EndState, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE));
	}


	/************************************************************************************************/

	
	void AddTempBuffer(ID3D12Resource* _ptr, RenderSystem* RS)
	{
		auto& TempBuffers = RS->FrameResources[RS->CurrentIndex].TempBuffers;
		TempBuffers.push_back(_ptr);
	}


	/************************************************************************************************/


	void AddTempShaderRes(ShaderResourceBuffer& ShaderResource, RenderSystem* RS)
	{
		AddTempBuffer(ShaderResource[0], RS);
		AddTempBuffer(ShaderResource[1], RS);
		AddTempBuffer(ShaderResource[2], RS);
	}


	/************************************************************************************************/


	Texture2D CreateTexture2D( RenderSystem* RS, Texture2D_Desc* desc_in )
	{	
		D3D12_RESOURCE_DESC   Resource_DESC = 
			(desc_in->FLAGS & Structred1D) ? 
				CD3DX12_RESOURCE_DESC::Buffer(desc_in->Width)
			:
				CD3DX12_RESOURCE_DESC::Tex2D(
					TextureFormat2DXGIFormat(desc_in->Format), 
					desc_in->Width, 
					desc_in->Height, 
					desc_in->MipLevels);

		D3D12_RESOURCE_STATES InitialState = desc_in->RenderTarget ?
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET :
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;

		Resource_DESC.Flags	= desc_in->RenderTarget ? 
								D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : 
								D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;

		Resource_DESC.Flags |= desc_in->UAV ?
			D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS :
			D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;

		Resource_DESC.Flags |=  (desc_in->FLAGS & SPECIALFLAGS::DEPTHSTENCIL) ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : 
			D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;

		Resource_DESC.MipLevels	= desc_in->MipLevels;

		D3D12_HEAP_PROPERTIES HEAP_Props	={};
		HEAP_Props.CPUPageProperty			= D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type						= D3D12_HEAP_TYPE_DEFAULT;
		HEAP_Props.MemoryPoolPreference		= D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask			= 0;
		HEAP_Props.VisibleNodeMask			= 0;

		D3D12_CLEAR_VALUE CV;
		CV.Color[0] = 0.0f;
		CV.Color[1] = 0.0f;
		CV.Color[2] = 0.0f;
		CV.Color[3] = 0.0f;
		CV.Format	= TextureFormat2DXGIFormat(desc_in->Format);
		
		D3D12_CLEAR_VALUE* pCV = desc_in->CV ? &CV : nullptr;
		
		ID3D12Resource* NewResource = nullptr;
		HRESULT HR = RS->pDevice->CreateCommittedResource(&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, 
															&Resource_DESC, InitialState, pCV, IID_PPV_ARGS(&NewResource));

		CheckHR(HR, ASSERTONFAIL("FAILED TO COMMIT MEMORY FOR TEXTURE"));

		Texture2D NewTexture = { NewResource, 
								 { desc_in->Width, desc_in->Height }, 
								 static_cast<uint8_t>(desc_in->MipLevels),
								 TextureFormat2DXGIFormat(desc_in->Format)};

		FK_LOG_INFO("Creating Texture!");

		FK_ASSERT(NewTexture, "Failed to Create Texture!");
		SETDEBUGNAME(NewResource, __func__);

		return (NewTexture);
	}


	/************************************************************************************************/


	FrameBufferedRenderTarget CreateFrameBufferedRenderTarget(RenderSystem* RS, Texture2D_Desc* Desc) 
	{
		return {	{	CreateTexture2D(RS, Desc), CreateTexture2D(RS, Desc), CreateTexture2D(RS, Desc) },
					{	Desc->Width, Desc->Height }, 
						Desc->GetFormat() };
	}



	void UploadTextureSet(RenderSystem* RS, TextureSet* TS, iAllocator* Memory)
	{
		for (size_t I = 0; I < 16; ++I)
		{
			if (TS->TextureGuids[I]) {
				TS->Loaded[I]	= true;
				TS->Textures[I] = LoadDDSTextureFromFile(TS->TextureLocations[I].Directory, RS, Memory);
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


	void CreateVertexBuffer(RenderSystem* RS, VertexBufferView** Buffers, size_t BufferCount, VertexBuffer& DVB_Out)
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

	#if 0
				RS->UpdateResourceByTemp(
					NewBuffer, 
					Buffers[15]->GetBuffer(), 
					Buffers[0x0f]->GetBufferSizeRaw(), 1, 
					D3D12_RESOURCE_STATE_INDEX_BUFFER);
	#else
				RS->UpdateResourceByUploadQueue(
					NewBuffer, 
					Buffers[itr]->GetBuffer(),
					Buffers[itr]->GetBufferSizeRaw(), 1,
					D3D12_RESOURCE_STATE_INDEX_BUFFER);
	#endif

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
#ifdef _DEBUG
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
#endif 
#if 0
			
			UpdateResourceByTemp(RS, NewBuffer, Buffers[itr]->GetBuffer(), 
				Buffers[itr]->GetBufferSizeRaw(), 1, 
				D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

#else
				RS->UpdateResourceByUploadQueue(NewBuffer, Buffers[itr]->GetBuffer(),
					Buffers[itr]->GetBufferSizeRaw(), 1, 
					D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
#endif

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

		auto Handle = Handles.GetNewHandle();
		Handles[Handle] = UserBuffers.size();

		UserBuffers.push_back(UserVertexBuffer{
			0,
			{Buffer[0], Buffer[1], Buffer[2]},
			BufferSize,
			0, 
			Buffers[Buffer[0]].MappedPtr,
			Buffers[Buffer[0]].Resource,
			false});

		return Handle;
	}


	/************************************************************************************************/


	VertexBufferStateTable::VBufferHandle VertexBufferStateTable::CreateVertexBufferResource(size_t BufferSize, bool GPUResident, RenderSystem* RS)
	{
		ID3D12Resource*	NewResource = RS->_CreateVertexBufferDeviceResource(BufferSize, GPUResident);
		void* _ptr = nullptr;

		FK_ASSERT(NewResource, "Failed To Create VertexBuffer Resource!");

		CD3DX12_RANGE readRange(0, 0);

		if (!GPUResident)
			NewResource->Map(0, &readRange, &_ptr);

		Buffers.push_back(
			VertexBuffer{
				NewResource,
				BufferSize,
				(char*)_ptr, 
				RS->GetCurrentFrame() });

		return Buffers.size() - 1;
	}


	/************************************************************************************************/


	bool VertexBufferStateTable::PushVertex(VertexBufferHandle Handle, void* _ptr, size_t ElementSize)
	{
		auto	Idx			= Handles[Handle];
		auto&	UserBuffer  = UserBuffers[Idx];
		auto	Offset		= UserBuffers[Idx].Offset;
		auto	Mapped_Ptr	= UserBuffers[Idx].MappedPtr;
		//UserBuffer.Resource	= Buffers[UserBuffer.GetCurrentBuffer()].Resource;
#if 1
		//if (!Mapped_Ptr)
		//{
			// TODO: Get next available buffer.
			//UserBuffer.IncrementCurrentBuffer();

			//UserBuffer.Resource		= Buffers[ResourceIdx].Resource;
			
			//Buffers[ResourceIdx].Resource->Map(0, nullptr, (void**)&Mapped_Ptr);

			//UserBuffer.MappedPtr = Mapped_Ptr;

			//if (!UserBuffer.Offset)
			//	return false;
		//}

		memcpy(Mapped_Ptr + Offset, _ptr, ElementSize);
		UserBuffers[Idx].Offset += ElementSize;
		UserBuffers[Idx].WrittenTo = true;

#else
		if(!UserBuffers[Idx].WrittenTo)
			UserBuffer.IncrementCurrentBuffer();

		Buffers[ResourceIdx].Resource->Map(0, nullptr, (void**)&Mapped_Ptr);
		memcpy(Mapped_Ptr + Offset, _ptr, ElementSize);
		UserBuffers[Idx].Offset += ElementSize;
		UserBuffers[Idx].WrittenTo = true;
		Buffers[ResourceIdx].Resource->Unmap(0, nullptr);
#endif

		return true;
	}
	

	/************************************************************************************************/


	void VertexBufferStateTable::LockUntil(size_t Frame)
	{
		for (auto& UserBuffer : UserBuffers)
		{
			if (!UserBuffer.WrittenTo)
				continue;

			Buffers[UserBuffer.GetCurrentBuffer()].NextAvailableFrame = Frame;

			UserBuffer.IncrementCurrentBuffer();
			auto BufferIdx					= UserBuffer.GetCurrentBuffer();
			UserBuffer.MappedPtr			= Buffers[BufferIdx].MappedPtr;
			UserBuffer.WrittenTo			= false;
			UserBuffer.Offset				= 0;
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


	ID3D12Resource* VertexBufferStateTable::GetResource(VertexBufferHandle Handle)
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

		UserBuffers[idx].Offset		+= size;
		UserBuffers[idx].WrittenTo	 = true;

		return { mapped_Ptr, offset, size };
	}



	/************************************************************************************************/


	bool VertexBufferStateTable::CurrentlyAvailable(VertexBufferHandle Handle, size_t CurrentFrame) const
	{
		auto UserIdx	= Handles[Handle];
		auto BufferIdx	= UserBuffers[UserIdx].GetCurrentBuffer();

		return Buffers[Handle].NextAvailableFrame <= CurrentFrame;
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


	void VertexBufferStateTable::ReleaseVertexBuffer(VertexBufferHandle Handle)
	{
		// TODO: buffer Recycling
		auto UserIdx		= Handles[Handle];

		for (size_t I = 0; I < 3; ++I)
		{
			auto ResourceIdx = UserBuffers[UserIdx].Buffers[I];

			if(Buffers[ResourceIdx].Resource)
				Buffers[ResourceIdx].Resource->Release();

			Buffers[ResourceIdx].Resource = nullptr;
			FreeBuffers.push_back({0, ResourceIdx});
		}
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

			if ( !(F & TF_BackBuffer))
				Resources[T.ResourceIdx].Release();
		}

		UserEntries.Release();
		Resources.Release();
		Handles.Clear();
	}


	/************************************************************************************************/


	TextureHandle TextureStateTable::AddResource(
		Texture2D_Desc&		Desc, 
		ID3D12Resource**	Resources_IN, 
		uint32_t			ResourceCount, 
		uint8_t				mipCount,
		DeviceResourceState InitialState,
		uint32_t			Flags_IN)
	{
		auto UserIdx     = UserEntries.size();
		auto ResourceIdx = Resources.size();
		auto Handle		 = Handles.GetNewHandle();
		Handles[Handle]  = UserIdx;

		UserEntry Entry;
		Entry.ResourceIdx		= ResourceIdx;
		Entry.FrameGraphIndex	= -1;
		Entry.FGI_FrameStamp    = -1;
		Entry.Flags				= Flags_IN;
		Entry.Handle            = Handle;
		Entry.Format			= Desc.GetFormat();

		UserEntries.push_back(Entry);

		ResourceEntry NewEntry = 
		{	
			ResourceCount,
			0,
			{ nullptr, nullptr, nullptr },
			{ 0, 0, 0 },
			{ InitialState, InitialState , InitialState },
			Desc.GetFormat(),
			mipCount,
			{ Desc.Width, Desc.Height },
		};

		for (size_t I = 0; I < ResourceCount; ++I)
			NewEntry.Resources[I] = Resources_IN[I];

		Resources.push_back(NewEntry);

		return Handle;
	}


	/************************************************************************************************/


	Texture2D TextureStateTable::operator[](TextureHandle Handle)
	{
		auto Idx		= Handles[Handle];
		auto Res		= Resources[UserEntries[Idx].ResourceIdx].GetResource();
		auto WH			= Resources[UserEntries[Idx].ResourceIdx].WH;
		auto Format		= Resources[UserEntries[Idx].ResourceIdx].Format;
		auto mipCount	= Resources[UserEntries[Idx].ResourceIdx].mipCount;
		return { Res, WH, mipCount, Format };
	}


	/************************************************************************************************/


	void TextureStateTable::SetState(TextureHandle Handle, DeviceResourceState State)
	{
		auto UserIdx = Handles[Handle];
		auto ResIdx  = UserEntries[UserIdx].ResourceIdx;

		Resources[ResIdx].SetState(State);
	}


	/************************************************************************************************/


	uint32_t TextureStateTable::GetTag(TextureHandle Handle) const
	{
		auto UserIdx = Handles[Handle];
		return UserEntries[UserIdx].Tag;
	}


	/************************************************************************************************/


	void TextureStateTable::SetTag(TextureHandle Handle, uint32_t Tag)
	{
		auto UserIdx = Handles[Handle];
		UserEntries[UserIdx].Tag = Tag;
	}


	size_t TextureStateTable::GetFrameGraphIndex(TextureHandle Handle, size_t FrameID) const
	{
		auto UserIdx	= Handles[Handle];
		auto FrameStamp	= UserEntries[UserIdx].FGI_FrameStamp;

		if (FrameStamp < int(FrameID))
			return INVALIDHANDLE;

		return UserEntries[UserIdx].FrameGraphIndex;
	}


	/************************************************************************************************/


	void TextureStateTable::SetFrameGraphIndex(TextureHandle Handle, size_t FrameID, size_t Index)
	{
		auto UserIdx = Handles[Handle];
		UserEntries[UserIdx].FGI_FrameStamp		= FrameID;
		UserEntries[UserIdx].FrameGraphIndex	= Index;
	}


	/************************************************************************************************/


	DXGI_FORMAT TextureStateTable::GetFormat(TextureHandle handle) const
	{
		auto UserIdx = Handles[handle];
		return UserEntries[UserIdx].Format;
	}


	/************************************************************************************************/


	uint2 TextureStateTable::GetWH(TextureHandle Handle) const
	{
		auto UserIdx = Handles[Handle];
		return Resources[UserEntries[UserIdx].ResourceIdx].WH;
	}


	/************************************************************************************************/


	void TextureStateTable::MarkRTUsed(TextureHandle Handle)
	{
		auto UserIdx = Handles[Handle];
		UserEntries[UserIdx].Flags |= TF_INUSE;
	}


	/************************************************************************************************/


	void TextureStateTable::LockUntil(size_t FrameID)
	{
		for (size_t I = 0; I < UserEntries.size(); ++I)
		{
			auto& UserEntry = UserEntries[I];
			auto Flags = UserEntry.Flags;

			if (Flags & TF_INUSE && !(Flags & TF_BackBuffer))
			{
				Resources[UserEntry.ResourceIdx].SetFrameLock(FrameID);
				Resources[UserEntry.ResourceIdx].IncreaseIdx();
			}
		}
	}
	

	/************************************************************************************************/


	void TextureStateTable::ReleaseTexture(TextureHandle Handle)
	{
		auto UserIdx		= Handles[Handle];
		auto& UserEntry		= UserEntries[UserIdx];
		const auto ResIdx	= UserEntry.ResourceIdx;
		auto& Resource		= Resources[ResIdx];

		Resource.Release();

		auto TempHandle		= UserEntries.back().Handle;
		UserEntry			= UserEntries.back();

		Resource				= Resources[UserEntry.ResourceIdx];
		UserEntry.ResourceIdx	= ResIdx;
		Handles[TempHandle]		= UserIdx;

		UserEntries.pop_back();
		Resources.pop_back();

		Handles.RemoveHandle(Handle);
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


	DeviceResourceState TextureStateTable::GetState(TextureHandle Handle) const
	{
		auto Idx			= Handles[Handle];
		auto ResourceIdx	= UserEntries[Idx].ResourceIdx;

		return Resources[ResourceIdx].States[Resources[ResourceIdx].CurrentResource];
	}


	/************************************************************************************************/


	ID3D12Resource*		TextureStateTable::GetResource(TextureHandle Handle) const
	{
		auto Idx			= Handles[Handle];
		auto ResourceIdx	= UserEntries[Idx].ResourceIdx;

		return Resources[ResourceIdx].Resources[Resources[ResourceIdx].CurrentResource];
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

		TriMeshHandle triMesh = LoadTriMeshIntoTable(rs, guid);

		return triMesh;
	}


	/************************************************************************************************/


	TriMeshHandle GetMesh(RenderSystem* rs, const char* meshID)
	{
		auto [mesh, result] = FindMesh(meshID);

		if(result)
			return mesh;

		return LoadTriMeshIntoTable(rs, meshID);
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


	//TextureHandle RenderSystem::_AddBackBuffer(Texture2D_Desc& Desc, ID3D12Resource* Res, uint32_t Tag)
	//{
	//}


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


	DescHeapPOS RenderSystem::_ReserveDescHeap(size_t SlotCount)
	{	// Make This Atomic
		auto FrameResources = _GetCurrentFrameResources();
		auto CPU = FrameResources->DescHeap.CPU_HeapPOS;
		auto GPU = FrameResources->DescHeap.GPU_HeapPOS;
		FrameResources->DescHeap.CPU_HeapPOS.ptr = FrameResources->DescHeap.CPU_HeapPOS.ptr + DescriptorCBVSRVUAVSize * SlotCount;
		FrameResources->DescHeap.GPU_HeapPOS.ptr = FrameResources->DescHeap.GPU_HeapPOS.ptr + DescriptorCBVSRVUAVSize * SlotCount;

		return { CPU , GPU };
	}



	/************************************************************************************************/


	DescHeapPOS RenderSystem::_ReserveGPUDescHeap(size_t SlotCount)
	{
		auto FrameResources = _GetCurrentFrameResources();
		auto CPU = FrameResources->GPUDescHeap.CPU_HeapPOS;
		auto GPU = FrameResources->GPUDescHeap.GPU_HeapPOS;
		FrameResources->GPUDescHeap.CPU_HeapPOS.ptr = FrameResources->GPUDescHeap.CPU_HeapPOS.ptr + DescriptorCBVSRVUAVSize * SlotCount;
		FrameResources->GPUDescHeap.GPU_HeapPOS.ptr = FrameResources->GPUDescHeap.GPU_HeapPOS.ptr + DescriptorCBVSRVUAVSize * SlotCount;

		return{ CPU , GPU };
	}

	/************************************************************************************************/


	DescHeapPOS RenderSystem::_ReserveRTVHeap(size_t SlotCount)
	{
		auto FrameResources = _GetCurrentFrameResources();
		auto CPU = FrameResources->RTVHeap.CPU_HeapPOS;
		auto GPU = FrameResources->RTVHeap.GPU_HeapPOS;
		FrameResources->RTVHeap.CPU_HeapPOS.ptr = FrameResources->RTVHeap.CPU_HeapPOS.ptr + DescriptorRTVSize * SlotCount;
		FrameResources->RTVHeap.GPU_HeapPOS.ptr = FrameResources->RTVHeap.GPU_HeapPOS.ptr + DescriptorRTVSize * SlotCount;

		return { CPU , GPU };
	}


	/************************************************************************************************/


	DescHeapPOS RenderSystem::_ReserveDSVHeap(size_t SlotCount)
	{
		auto FrameResources = _GetCurrentFrameResources();
		auto CPU = FrameResources->DSVHeap.CPU_HeapPOS;
		auto GPU = FrameResources->DSVHeap.GPU_HeapPOS;
		FrameResources->DSVHeap.CPU_HeapPOS.ptr = FrameResources->DSVHeap.CPU_HeapPOS.ptr + DescriptorDSVSize * SlotCount;
		FrameResources->DSVHeap.GPU_HeapPOS.ptr = FrameResources->DSVHeap.GPU_HeapPOS.ptr + DescriptorDSVSize * SlotCount;

		return{ CPU , GPU };
	}


	/************************************************************************************************/


	ID3D12Resource* RenderSystem::_GetTextureResource(TextureHandle handle)
	{
		return Textures.GetResource(handle);
	}


	/************************************************************************************************/



	void RenderSystem::_ResetDescHeap()
	{
		auto FrameResources = _GetCurrentFrameResources();
		FrameResources->DescHeap.GPU_HeapPOS = FrameResources->DescHeap.DescHeap->GetGPUDescriptorHandleForHeapStart();
		FrameResources->DescHeap.CPU_HeapPOS = FrameResources->DescHeap.DescHeap->GetCPUDescriptorHandleForHeapStart();
	}


	/************************************************************************************************/


	void RenderSystem::_ResetRTVHeap()
	{
		auto FrameResources = _GetCurrentFrameResources();
		FrameResources->RTVHeap.GPU_HeapPOS = FrameResources->RTVHeap.DescHeap->GetGPUDescriptorHandleForHeapStart();
		FrameResources->RTVHeap.CPU_HeapPOS = FrameResources->RTVHeap.DescHeap->GetCPUDescriptorHandleForHeapStart();
	}


	/************************************************************************************************/


	void RenderSystem::_ResetGPUDescHeap()
	{
		auto FrameResources = _GetCurrentFrameResources();
		FrameResources->GPUDescHeap.GPU_HeapPOS = FrameResources->GPUDescHeap.DescHeap->GetGPUDescriptorHandleForHeapStart();
		FrameResources->GPUDescHeap.CPU_HeapPOS = FrameResources->GPUDescHeap.DescHeap->GetCPUDescriptorHandleForHeapStart();
	}


	/************************************************************************************************/


	void RenderSystem::_ResetDSVHeap()
	{
		auto FrameResources = _GetCurrentFrameResources();
		FrameResources->DSVHeap.GPU_HeapPOS = FrameResources->DSVHeap.DescHeap->GetGPUDescriptorHandleForHeapStart();
		FrameResources->DSVHeap.CPU_HeapPOS = FrameResources->DSVHeap.DescHeap->GetCPUDescriptorHandleForHeapStart();
	}


	/************************************************************************************************/


	size_t RenderSystem::_GetVidMemUsage()
	{
		DXGI_QUERY_VIDEO_MEMORY_INFO VideoMemInfo;
		pDXGIAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &VideoMemInfo);
		return VideoMemInfo.CurrentUsage;
	}


	/************************************************************************************************/


	ID3D12DescriptorHeap* RenderSystem::_GetCurrentRTVTable()
	{
		return _GetCurrentFrameResources()->RTVHeap.DescHeap;
	}

	ID3D12DescriptorHeap* RenderSystem::_GetCurrentDescriptorTable()
	{
		return _GetCurrentFrameResources()->DescHeap.DescHeap;
	}

	ID3D12DescriptorHeap* RenderSystem::_GetCurrentDSVTable()
	{
		return _GetCurrentFrameResources()->DSVHeap.DescHeap;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE RenderSystem::_GetDescTableCurrentPosition_GPU()
	{
		return _GetCurrentFrameResources()->DescHeap.GPU_HeapPOS;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE RenderSystem::_GetRTVTableCurrentPosition_GPU()
	{
		return _GetCurrentFrameResources()->RTVHeap.GPU_HeapPOS;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE RenderSystem::_GetRTVTableCurrentPosition_CPU()
	{
		return _GetCurrentFrameResources()->RTVHeap.CPU_HeapPOS;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE RenderSystem::_GetDSVTableCurrentPosition_CPU()
	{
		return _GetCurrentFrameResources()->DSVHeap.CPU_HeapPOS;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE RenderSystem::_GetDSVTableCurrentPosition_GPU()
	{
		return _GetCurrentFrameResources()->DSVHeap.GPU_HeapPOS;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE RenderSystem::_GetGPUDescTableCurrentPosition_GPU()
	{
		return _GetCurrentFrameResources()->GPUDescHeap.GPU_HeapPOS;
	}


	/************************************************************************************************/


	ID3D12QueryHeap* RenderSystem::_GetQueryResource(QueryHandle Handle)
	{
		return Queries.GetResource(Handle);
	}


	/************************************************************************************************/


	ID3D12CommandAllocator*	 RenderSystem::_GetCurrentCommandAllocator()
	{
		return FrameResources[CurrentIndex].GraphicsCLAllocator[0];
	}

	/************************************************************************************************/


	ID3D12GraphicsCommandList3*	RenderSystem::_GetCurrentCommandList()
	{
		return FrameResources[CurrentIndex].CommandLists[0];
	}


	/************************************************************************************************/


	PerFrameResources* RenderSystem::_GetCurrentFrameResources()
	{
		return FrameResources + CurrentIndex;
	}

	/************************************************************************************************/


	ID3D12GraphicsCommandList3*	RenderSystem::_GetCommandList_1()
	{
		_GetCurrentFrameResources()->CommandListsUsed[1] = true;

		return FrameResources[CurrentIndex].CommandLists[1];
	}


	/************************************************************************************************/


	PerFrameUploadQueue& RenderSystem::_GetCurrentUploadQueue()
	{
		return UploadQueues[CurrentUploadIndex];
	}


	/************************************************************************************************/


	void Close(static_vector<ID3D12GraphicsCommandList*> CLs) {
		//HRESULT HR;
		for (auto CL : CLs)
			FK_ASSERT(SUCCEEDED(CL->Close()));
	}

	/************************************************************************************************/



	void RenderSystem::BeginSubmission(RenderWindow* Window)
	{
		_IncrementRSIndex();
		size_t Index = CurrentIndex;
		WaitforGPU();

		HRESULT HR;
		HR = _GetCurrentCommandAllocator()->Reset();									FK_ASSERT(SUCCEEDED(HR));
		HR = _GetCurrentCommandList()->Reset(_GetCurrentCommandAllocator(), nullptr);	FK_ASSERT(SUCCEEDED(HR));

		for (auto& b : _GetCurrentFrameResources()->CommandListsUsed)
			b = false;

		_GetCurrentFrameResources()->CommandListsUsed[0] = true;

		_ResetRTVHeap();
		_ResetDescHeap();
		_ResetGPUDescHeap();
		_ResetDSVHeap();

		auto SRVs = _GetCurrentDescriptorTable();
		_GetCurrentCommandList()->SetDescriptorHeaps(1, &SRVs);
	}


	/************************************************************************************************/


	void RenderSystem::Submit(static_vector<ID3D12CommandList*>& CLs)
	{
		auto Val = ++FenceCounter;
		Fences[CurrentIndex].FenceValue = Val;

		GraphicsQueue->ExecuteCommandLists(CLs.size(), CLs.begin());
		GraphicsQueue->Signal(Fence, Val);

		VertexBuffers.LockUntil(GetCurrentFrame() + 2);
		ConstantBuffers.LockUntil(GetCurrentFrame() + 2);
		RenderTargets.LockUntil(GetCurrentFrame() + 2);
	}


	/************************************************************************************************/


	void RenderSystem::WaitForUploadQueue()
	{
		auto CompletedValue      = CopyFence->GetCompletedValue();
		size_t Index	         = CurrentUploadIndex;
		auto Fence		         = CopyFence;

		if (CompletedValue < CopyFences[Index].FenceValue) {
			HANDLE eventHandle   = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
			Fence->SetEventOnCompletion(CopyFences[Index].FenceValue, eventHandle);
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}


	/************************************************************************************************/


	void RenderSystem::ReadyUploadQueues()
	{
		_GetCurrentUploadQueue().Reset();
	}


	/************************************************************************************************/


	void RenderSystem::SubmitUploadQueues()
	{
		auto& CurrentUploadQueue	= _GetCurrentUploadQueue();
		auto Fence					= CopyFence;

		if (CurrentUploadQueue.UploadCount) {
			WaitForUploadQueue();

			auto HR			= CurrentUploadQueue.UploadList[0]->Close();
			size_t Value	= CopyFences[CurrentUploadIndex].FenceValue = ++FenceUploadCounter;

			UploadQueue->ExecuteCommandLists(1, (ID3D12CommandList**)CurrentUploadQueue.UploadList);
			UploadQueue->Signal(Fence, Value);

			CurrentUploadIndex = (CurrentUploadIndex + 1) % 3;

			ReadyUploadQueues();
		}
	}


	/************************************************************************************************/


	void RenderSystem::ShutDownUploadQueues()
	{
		auto& UploadQueue	= _GetCurrentUploadQueue();

		WaitForUploadQueue();
		CurrentUploadIndex = (CurrentUploadIndex + 1) % 3;
		WaitForUploadQueue();
		CurrentUploadIndex = (CurrentUploadIndex + 1) % 3;
		WaitForUploadQueue();
		CurrentUploadIndex = (CurrentUploadIndex + 1) % 3;

		UploadQueue.Close();
		UploadQueue.Reset();
	}



	/************************************************************************************************/


	void RenderSystem::UploadResources()
	{
		ID3D12CommandList* CommandLists[1] = { _GetCurrentCommandList() };
		auto CL = _GetCurrentCommandList();// ->Close();
		CL->Close();
		GraphicsQueue->ExecuteCommandLists(1, CommandLists);
	}


	/************************************************************************************************/


	ShaderResourceBuffer CreateShaderResource(RenderSystem* RS, const size_t ResourceSize, const char* _DebugName)
	{
		D3D12_RESOURCE_DESC   Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(ResourceSize);
		Resource_DESC.Alignment				= 0;
		Resource_DESC.DepthOrArraySize		= 1;
		Resource_DESC.Dimension				= D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		Resource_DESC.Layout				= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Resource_DESC.Width					= ResourceSize;
		Resource_DESC.Height				= 1;
		Resource_DESC.Format				= DXGI_FORMAT_UNKNOWN;
		Resource_DESC.SampleDesc.Count		= 1;
		Resource_DESC.SampleDesc.Quality	= 0;
		Resource_DESC.Flags					= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		
		D3D12_HEAP_PROPERTIES HEAP_Props = {};
		HEAP_Props.CPUPageProperty	     = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type				     = D3D12_HEAP_TYPE_DEFAULT;
		HEAP_Props.MemoryPoolPreference  = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask	     = 0;
		HEAP_Props.VisibleNodeMask		 = 0;

		size_t BufferCount = RS->BufferCount;
		FrameBufferedResource NewResource;
		NewResource.BufferCount = BufferCount;

		for (size_t I = 0; I < BufferCount; ++I)
		{
			ID3D12Resource* Resource = nullptr;
			HRESULT HR = RS->pDevice->CreateCommittedResource(
							&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, 
							&Resource_DESC, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr, 
							IID_PPV_ARGS(&Resource));

			CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE SHADERRESOURCE!"));
			NewResource.Resources[I] = Resource;

			SETDEBUGNAME(Resource, _DebugName);
		}
		return NewResource;
	}
	

	/************************************************************************************************/


	StreamOutBuffer CreateStreamOut(RenderSystem* RS, const size_t ResourceSize) {
		D3D12_RESOURCE_DESC Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(ResourceSize);
		Resource_DESC.Alignment          = 0;
		Resource_DESC.DepthOrArraySize   = 1;
		Resource_DESC.Dimension          = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		Resource_DESC.Layout             = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Resource_DESC.Width              = ResourceSize;
		Resource_DESC.Height             = 1;
		Resource_DESC.Format             = DXGI_FORMAT_UNKNOWN;
		Resource_DESC.SampleDesc.Count   = 1;
		Resource_DESC.SampleDesc.Quality = 0;
		Resource_DESC.Flags              = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		D3D12_HEAP_PROPERTIES HEAP_Props = {};
		HEAP_Props.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type                  = D3D12_HEAP_TYPE_DEFAULT;
		HEAP_Props.MemoryPoolPreference  = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask      = 0;
		HEAP_Props.VisibleNodeMask       = 0;

		size_t BufferCount = RS->BufferCount;
		FrameBufferedResource NewResource;
		NewResource.BufferCount = BufferCount;

		for (size_t I = 0; I < BufferCount; ++I)
		{
			ID3D12Resource* Resource = nullptr;
			HRESULT HR = RS->pDevice->CreateCommittedResource(
								&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
								&Resource_DESC, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr,
								IID_PPV_ARGS(&Resource));

			CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE SHADERRESOURCE!"));
			NewResource.Resources[I] = Resource;

			SETDEBUGNAME(Resource, __func__);
		}
		return NewResource;
	}


	/************************************************************************************************/
	

	bool CreateInputLayout(RenderSystem* RS, VertexBufferView** Buffers, size_t count, Shader* Shader, VertexBuffer* DVB_Out)
	{
#ifdef _DEBUG
		//FK_ASSERT(Shader);
		//FK_ASSERT(Shader->Blob);
#endif
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
	

	void Release( VertexBuffer* VertexBuffer )
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
	

	void Release(ID3D11View* View)
	{
		if (View)
			View->Release();
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


	void Release(SpotLightList* SLB)
	{
	}


	/************************************************************************************************/
	

	void Release(PointLightList* SLB)
	{
	}
	

	/************************************************************************************************/
	

	VertexBuffer::BuffEntry* GetBuffer( VertexBuffer* VB, VERTEXBUFFER_TYPE type ) // return Nullptr if not found
	{	
		for( auto& Buff : VB->VertexBuffers )
		{	
			if( Buff.Type == type )
			return &Buff;
		}
		return nullptr;
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
			FK_LOG_INFO("LoadingShader - %s - \n", Entry);
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



	DescHeapPOS PushRenderTarget(RenderSystem* RS, Texture2D* Target, DescHeapPOS POS)
	{
		D3D12_RENDER_TARGET_VIEW_DESC TargetDesc = {};
		TargetDesc.Format				= Target->Format;
		TargetDesc.Texture2D.MipSlice	= 0;
		TargetDesc.Texture2D.PlaneSlice = 0;
		TargetDesc.ViewDimension		= D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;

		RS->pDevice->CreateRenderTargetView(*Target, &TargetDesc, POS);

		return IncrementHeapPOS(POS, RS->DescriptorRTVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushRenderTarget(RenderSystem* RS, TextureHandle target, DescHeapPOS POS)
	{
		D3D12_RENDER_TARGET_VIEW_DESC TargetDesc = {};
		TargetDesc.Format				= RS->_GetDXGIRenderTargetFormat(target);
		TargetDesc.Texture2D.MipSlice	= 0;
		TargetDesc.Texture2D.PlaneSlice = 0;
		TargetDesc.ViewDimension		= D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;

		auto resource = RS->GetObjectDeviceResource(target);
		RS->pDevice->CreateRenderTargetView(resource, &TargetDesc, POS);

		return IncrementHeapPOS(POS, RS->DescriptorRTVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushDepthStencil(RenderSystem* RS, Texture2D* Target, DescHeapPOS POS)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
		DSVDesc.Format				= Target->Format;
		DSVDesc.Texture2D.MipSlice	= 0;
		DSVDesc.ViewDimension		= D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2D;

		RS->pDevice->CreateDepthStencilView(*Target, &DSVDesc, POS);

		return IncrementHeapPOS(POS, RS->DescriptorDSVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushCBToDescHeap(RenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, size_t BufferSize)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC CBV_DESC = {};
		CBV_DESC.BufferLocation = Buffer->GetGPUVirtualAddress();
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
			ViewDesc.Texture2D.MipLevels           = -1;//tex.mipCount;
			ViewDesc.Texture2D.MostDetailedMip     = 0;
			ViewDesc.Texture2D.PlaneSlice          = 0;
			ViewDesc.Texture2D.ResourceMinLODClamp = 0;
		}

		RS->pDevice->CreateShaderResourceView(tex, &ViewDesc, POS);

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushUAV2DToDescHeap(RenderSystem* RS, Texture2D tex, DescHeapPOS POS, DXGI_FORMAT F)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		UAVDesc.Format               = F;
		UAVDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
		UAVDesc.Texture2D.MipSlice   = 0;
		UAVDesc.Texture2D.PlaneSlice = 0;

		RS->pDevice->CreateUnorderedAccessView(tex, nullptr, &UAVDesc, POS);
		
		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	void SetViewport(ID3D12GraphicsCommandList* CL, Texture2D Target, uint2 Offsets) {
		SetViewports(CL, &Target, &Offsets);
	}


	/************************************************************************************************/


	void SetScissors(ID3D12GraphicsCommandList* CL, uint2* WH, uint2* Offsets, UINT Count)
	{
		D3D12_RECT Rects[16];
		for (size_t I = 0; I < Count; ++I) {
			Rects[I].bottom = Offsets[I][1] + WH[I][1];
			Rects[I].left	= Offsets[I][0];
			Rects[I].right	= Offsets[I][0] + WH[I][0];
			Rects[I].top	= Offsets[I][1];
		}

		CL->RSSetScissorRects(Count, Rects);
	}


	/************************************************************************************************/


	void SetScissor(ID3D12GraphicsCommandList* CL, uint2 WH, uint2 Offset) {
		SetScissors(CL, &WH, &Offset, 1);
	}


	/************************************************************************************************/


	void SetViewports(ID3D12GraphicsCommandList* CL, Texture2D* Targets, uint2* Offsets, UINT Count)
	{
		D3D12_VIEWPORT VPs[16];
		for (size_t I = 0; I < Count; ++I) {
			VPs[I].Height	= Targets[0].WH[1];
			VPs[I].Width	= Targets[0].WH[0];
			VPs[I].MaxDepth = 1.0f;
			VPs[I].MinDepth = 0.0f;
			VPs[I].TopLeftX = Offsets[I][0];
			VPs[I].TopLeftY = Offsets[I][1];
		}

		CL->RSSetViewports(Count, VPs);
	}


	/************************************************************************************************/


	void CloseAndSubmit(static_vector<ID3D12GraphicsCommandList*> CLs, RenderSystem* RS, RenderWindow* Window)
	{
		CD3DX12_RESOURCE_BARRIER Barrier[] = {
			CD3DX12_RESOURCE_BARRIER::Transition(GetRenderTarget(Window),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT, -1,
			D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE) };

		RS->_GetCurrentCommandList()->ResourceBarrier(1, Barrier);

		Close(CLs);
		
		RS->Submit(CLs._PTR_Cast<ID3D12CommandList>());
	}


	/************************************************************************************************/
	

	void ClearBackBuffer(RenderSystem* RS, ID3D12GraphicsCommandList* CL, RenderWindow* RW, float4 ClearColor){
		auto RTVPOSCPU		= RS->_GetRTVTableCurrentPosition_CPU(); // _Ptr to Current POS On RTV heap on CPU
		auto RTVPOS			= RS->_ReserveRTVHeap(1);
		auto RTVHeap		= RS->_GetCurrentRTVTable();
		
		PushRenderTarget(RS, &GetRenderTarget(RW), RTVPOS);

		CL->ClearRenderTargetView(RTVPOSCPU, ClearColor, 0, nullptr);
	}


	/************************************************************************************************/


	void SetDepthBuffersRead(RenderSystem* RS, ID3D12GraphicsCommandList* CL, static_vector<Texture2D> DB )
	{
		static_vector<D3D12_RESOURCE_BARRIER>	Barriers;

		for (size_t I = 0; I < DB.size(); ++I)
		{
			/*
			typedef struct D3D12_RESOURCE_TRANSITION_BARRIER
			{
			ID3D12Resource *pResource;
			UINT Subresource;
			D3D12_RESOURCE_STATES StateBefore;
			D3D12_RESOURCE_STATES StateAfter;
			} 	D3D12_RESOURCE_TRANSITION_BARRIER;

			typedef struct D3D12_RESOURCE_BARRIER
			{
			D3D12_RESOURCE_BARRIER_TYPE Type;
			D3D12_RESOURCE_BARRIER_FLAGS Flags;
			union
			{
			D3D12_RESOURCE_TRANSITION_BARRIER Transition;
			D3D12_RESOURCE_ALIASING_BARRIER Aliasing;
			D3D12_RESOURCE_UAV_BARRIER UAV;
			} 	;
			} 	D3D12_RESOURCE_BARRIER;
			*/


			D3D12_RESOURCE_TRANSITION_BARRIER Transition = {
				DB[I], 0,
				D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE,
				D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ,
			};


			D3D12_RESOURCE_BARRIER Barrier =
			{
				D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE,
				Transition,
			};

			Barriers.push_back(Barrier);
		}
		CL->ResourceBarrier(Barriers.size(), Barriers.begin());
	}


	/************************************************************************************************/


	void SetDepthBuffersWrite(RenderSystem* RS, ID3D12GraphicsCommandList* CL, static_vector<Texture2D> DB ) 
	{
		static_vector<D3D12_RESOURCE_BARRIER>	Barriers;

		for(auto& buffer : DB)
		{
			/*
				typedef struct D3D12_RESOURCE_TRANSITION_BARRIER
				{
				ID3D12Resource *pResource;
				UINT Subresource;
				D3D12_RESOURCE_STATES StateBefore;
				D3D12_RESOURCE_STATES StateAfter;
				} 	D3D12_RESOURCE_TRANSITION_BARRIER;

				typedef struct D3D12_RESOURCE_BARRIER
				{
				D3D12_RESOURCE_BARRIER_TYPE Type;
				D3D12_RESOURCE_BARRIER_FLAGS Flags;
				union
				{
				D3D12_RESOURCE_TRANSITION_BARRIER Transition;
				D3D12_RESOURCE_ALIASING_BARRIER Aliasing;
				D3D12_RESOURCE_UAV_BARRIER UAV;
				} 	;
				} 	D3D12_RESOURCE_BARRIER;
			*/


			D3D12_RESOURCE_TRANSITION_BARRIER Transition = {
				buffer, 0, 
				D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ, 
				D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE,
			};


			D3D12_RESOURCE_BARRIER Barrier =
			{
				D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE,
				Transition,
			};

			Barriers.push_back(Barrier);
		}
		CL->ResourceBarrier(Barriers.size(), Barriers.begin());
	}
	

	/************************************************************************************************/


	void ClearDepthBuffers(RenderSystem* RS, ID3D12GraphicsCommandList* CL, static_vector<Texture2D> DB, const float ClearValue[], const int Stencil[], const size_t count)
	{
		for (size_t I = 0; I < count; ++I) {
			auto DSVPOS		= RS->_GetDSVTableCurrentPosition_CPU();
			auto POS		= RS->_ReserveDSVHeap(1);

			PushDepthStencil(RS, &DB[I], POS);
			
			CL->ClearDepthStencilView(DSVPOS, D3D12_CLEAR_FLAG_DEPTH, ClearValue[I], Stencil[I], 0, nullptr);
		}
	}


	void CreatePointLightList(PointLightList* out, PointLightListDesc Desc, iAllocator* Memory )
	{
		FK_ASSERT( Desc.MaxLightCount > 0, "INVALID PARAMS" );

		out->Lights	= PLList		(Memory);
		out->Flags	= LightFlags	(Memory);
		out->IDs	= LightIDs		(Memory);
	}


	TriMeshHandle BuildMesh(RenderSystem* RS, Mesh_Description* Desc, GUID_t guid)
	{
		TriMesh* Mesh			= nullptr;
		TriMeshHandle Handle	= InvalidHandle_t;

		if (!GeometryTable.FreeList.size())
		{
			auto Index		= GeometryTable.Geometry.size();
			Handle			= GeometryTable.Handles.GetNewHandle();

			GeometryTable.Geometry.push_back(TriMesh());
			GeometryTable.GeometryIDs.push_back(nullptr);
			GeometryTable.Guids.push_back(guid);
			GeometryTable.ReferenceCounts.push_back(1);

			Mesh			= &GeometryTable.Geometry.back();
			memset(Mesh, 0, sizeof(TriMesh));

			Mesh->Memory	= Desc->memory;
		}
		else
		{
			auto Index	= GeometryTable.FreeList.back();
			GeometryTable.FreeList.pop_back();

			Handle		= GeometryTable.Handles.GetNewHandle();

			GeometryTable.Handles[Handle]			= Index;
			GeometryTable.GeometryIDs[Index]		= nullptr;
			GeometryTable.Guids[Index]			= guid;
			GeometryTable.ReferenceCounts[Index]	= 1;

			Mesh = &GeometryTable.Geometry[Index];
		}

		for( auto& V : Mesh->Buffers)
			V = nullptr;

		size_t End = Desc->BufferCount;
		for (size_t I = 0; I < End; ++I)
			Mesh->Buffers[I] = Desc->Buffers[I];

		CreateVertexBuffer	(RS, Mesh->Buffers, Desc->BufferCount, Mesh->VertexBuffer);

		return Handle;
	}

	/*
	struct PointLight {
		float3 K;
		float I, R;

		NodeHandle Position;
	};
	*/
	

	LightHandle CreateLight(PointLightList* PL, LightDesc& in)
	{
		return LightHandle(0);
		auto HandleIndex = PL->Lights.size();
		PL->Lights.push_back({in.K, in.I, in.R, in.Hndl});

		LightHandle Handle;
		Handle.INDEX = HandleIndex;

		return Handle;
	}


	/************************************************************************************************/
	/*
	struct SpotLight {
		float3 K;
		float3 Direction;
		float I, R;

		NodeHandle Position;
	};
	*/
	

	LightHandle CreateLight(SpotLightList* SL, LightDesc& in, float3 Dir, float p)
	{
		auto HandleIndex = SL->size();
		SL->push_back({in.K, Dir, in.I, in.R, float(pi/4.0f), in.Hndl});

		LightHandle Handle;
		Handle.INDEX = HandleIndex;

		SetLightFlag(SL, Handle, LightBufferFlags::Dirty);
		return Handle;
	}


	/************************************************************************************************/


	void CreateSpotLightList(SpotLightList* out, iAllocator* Memory, size_t Max)
	{
		out->Lights		= SLList		(Memory);
		out->Flags		= LightFlags	(Memory);
		out->IDs		= LightIDs		(Memory);
	}


	/************************************************************************************************/


	void ReleaseLight(PointLightList*	PL, LightHandle Handle)
	{
		PL->Lights.at(Handle).I		= 0.0f;
		PL->Lights.at(Handle).R		= 0.0f;
		PL->Flags.at(Handle)		= LightBufferFlags::Unused;
	}


	/************************************************************************************************/


	void CreatePlaneMesh(RenderSystem* RS, TriMesh* out, StackAllocator* mem, PlaneDesc desc)
	{	// Change this to be allocated from level Memory
		out->Buffers[0]		= FlexKit::CreateVertexBufferView((byte*)mem->malloc(512), 512);
		out->Buffers[15]	= FlexKit::CreateVertexBufferView((byte*)mem->malloc(512), 512);

		out->Buffers[0]->Begin
			( FlexKit::VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION
			, FlexKit::VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32 );

		float l = desc.r;
		// Push Vertices
		{
			auto Buffer = out->Buffers[0];
			static_vector<float3,8> temp =
			{
				{ l, 0,  l },
				{-l, 0,  l },
				{ l, 0, -l },
				{-l, 0, -l }
			};

			for (auto I : temp)
				Buffer->Push(I);
		}	out->Buffers[0]->End();
	
		out->Buffers[15]->Begin
			( FlexKit::VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX 
			, FlexKit::VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32 );
		// Push Indices
		{
			auto& Buffer = out->Buffers[15];
			static_vector<uint32_t, 6> temp =
			{
				0, 1, 2,
				3, 2, 1
			};
			for (auto I : temp)
				Buffer->Push(I);
			out->IndexCount = temp.size();
		}	out->Buffers[15]->End();

		FlexKit::CreateVertexBuffer	(RS, out->Buffers, 1, out->VertexBuffer);
		FlexKit::CreateInputLayout	(RS, out->Buffers, 1, &desc.VertexShader, &out->VertexBuffer);
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

		TL.push_back(s_TokenValue::Empty());
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

		if (!FlexKit::MeshUtilityFunctions::BuildVertexBuffer(TL, out_buffer, out_indexes, LevelSpace, TempSpace))
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


		out.Buffers[00] = FlexKit::CreateVertexBufferView(VertexBuffer, VertexBufferSize);
		out.Buffers[01] = FlexKit::CreateVertexBufferView(NormalBuffer, NormalBufferSize);
		out.Buffers[15] = FlexKit::CreateVertexBufferView(IndexBuffer,  IndexBufferSize);

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
			out.Buffers[2] = FlexKit::CreateVertexBufferView(TexcordBuffer, TexcordBufferSize);
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
		
			out.Buffers[3] = FlexKit::CreateVertexBufferView(TangentBuffer, TangentBufferSize);
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

		FlexKit::CreateVertexBuffer		(RS, out.Buffers, 2 + desc.LoadUVs + desc.GenerateTangents,           out.VertexBuffer);
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

	/*
	bool GenerateTangents(Buffer* __restrict Normals_in, Buffer* __restrict Indices_in, Buffer* __restrict  out)
	{
		// Set Clear Tangents
		for (auto V : out_buffer)
			out.Buffers[3]->Push(float3{ 0.0f, 0.0f, 0.0f });

		auto IndexBuffer = reinterpret_cast<uint32_t*>(out.Buffers[15]->GetBuffer());
		auto VBuffer = reinterpret_cast<float3*>	  (out.Buffers[0]->GetBuffer());
		auto NBuffer = reinterpret_cast<float3*>	  (out.Buffers[1]->GetBuffer());
		auto TexCoords = reinterpret_cast<float2*>	  (out.Buffers[2]->GetBuffer());
		auto Tangents = reinterpret_cast<float3*>	  (out.Buffers[3]->GetBuffer());

		for (auto itr = 0; itr < out.Buffers[0]->GetBufferSizeUsed(); itr += 3)
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
			(*Normals) = Normals->normalize();
			Normals++;
		}
	}
	*/


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


	QueryResource CreateSOQuery(RenderSystem* RS, D3D12_QUERY_HEAP_TYPE Type)
	{
		QueryResource NewQuery;

		for (size_t itr = 0; itr < MaxBufferedSize; ++itr)
		{
			ID3D12QueryHeap* Query = nullptr;
			D3D12_QUERY_HEAP_DESC QHeap_Desc;
			QHeap_Desc.Count = 3;
			QHeap_Desc.NodeMask = 0;
			QHeap_Desc.Type = Type;

			HRESULT HR = RS->pDevice->CreateQueryHeap(&QHeap_Desc, IID_PPV_ARGS(&Query));
			CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE QUERY HEAP!"));
			SETDEBUGNAME(Query, "SO STREAM OUT QUERY");

			NewQuery[itr] = Query;
		}

		return NewQuery;
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


	float2 GetPixelSize(RenderWindow* Window)
	{
		auto WH = Window->WH;
		return float2{ 1.0f, 1.0f } / WH;
	}

	/************************************************************************************************/


	void HideSystemCursor(RenderWindow* Window)
	{
		ShowCursor(false);
	}


	/************************************************************************************************/


	void LoadStaticMeshBatcherShaders(FlexKit::RenderSystem* RS, StaticMeshBatcher* Batcher)
	{
	}


	/************************************************************************************************/


	void InitiateStaticMeshBatcher(FlexKit::RenderSystem* RS, iAllocator* Memory, StaticMeshBatcher* out)
	{
		for (auto G : out->Geometry)
			G = nullptr;

		for (auto& I : out->InstanceCount)
			I = 0;

		out->NormalBuffer     = nullptr;
		out->IndexBuffer      = nullptr;
		out->TangentBuffer    = nullptr;
		out->VertexBuffer     = nullptr;
		out->GTBuffer         = nullptr;

		out->Instances			= nullptr;
		out->TransformsBuffer	= nullptr;

		LoadStaticMeshBatcherShaders(RS, out);

		out->TotalInstanceCount   = 0;
		out->MaxVertexPerInstance = 0;
	}


	/************************************************************************************************/


	void CleanUpStaticBatcher(StaticMeshBatcher* Batcher)
	{
		if(Batcher->NormalBuffer)		Batcher->NormalBuffer->Release();
		if(Batcher->IndexBuffer)		Batcher->IndexBuffer->Release();
		if(Batcher->TangentBuffer)		Batcher->TangentBuffer->Release();
		if(Batcher->VertexBuffer)		Batcher->VertexBuffer->Release();
		if(Batcher->GTBuffer)			Batcher->GTBuffer->Release();
		if(Batcher->Instances)			Batcher->Instances->Release();
		if(Batcher->TransformsBuffer)	Batcher->TransformsBuffer->Release();

		Batcher->TotalInstanceCount = 0;
		Batcher->MaxVertexPerInstance = 0;
	}


	/************************************************************************************************/


	void Upload(RenderSystem* RS, iAllocator* Temp, Camera* C)
	{
		/*
		bool BufferDirty = false;
		for (auto I = 0; I < TotalInstanceCount; ++I){
			if (DirtyFlags[I] & DIRTY_ChangedMesh)
			{
				BufferDirty = BufferDirty | true;
				auto entry = GeometryTable[GeometryIndex[I]];
				InstanceInformation[I].StartIndexLocation		= entry.IndexOffset;
				InstanceInformation[I].IndexCountPerInstance	= entry.VertexCount;
				InstanceInformation[I].BaseVertexLocation		= entry.VertexOffset;
				DirtyFlags[I] = DirtyFlags[I] ^ DIRTY_ChangedMesh;
			}
		}
		*/

		/*
		if (BufferDirty)
			MapWriteDiscard(RS, (char*)InstanceInformation, sizeof(InstanceInformation), Instances);
		BufferDirty = false;

		for (auto I = 0; I < TotalInstanceCount; ++I){
			if (DirtyFlags[I] & DIRTY_Transform)
			{
				BufferDirty = BufferDirty | true;
				FlexKit::GetTransform(nodes, NodeHandles[I], &Transforms[I]);
				Transforms[I] = DirectX::XMMatrixTranspose(Transforms[I]);
				DirtyFlags[I] = DirtyFlags[I] ^ DIRTY_Transform;
			}
		}
		*/

		//if (BufferDirty)
		//	MapWriteDiscard(RS, (char*)Transforms, sizeof(DirectX::XMMATRIX) * MAXINSTANCES, TransformsBuffer);
	}


	/************************************************************************************************/


	void BuildGeometryTable(FlexKit::RenderSystem* RS, iAllocator* TempMemory, StaticMeshBatcher* Batcher)
	{
		// Create And Upload to Buffer
		auto CreateBuffer = [](size_t BufferSize, char* Buffer, RenderSystem* RS) -> ID3D12Resource*
		{
			HRESULT	HR = ERROR;
			D3D12_RESOURCE_DESC Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(0);
			Resource_DESC.Alignment           = 0;
			Resource_DESC.DepthOrArraySize    = 1;
			Resource_DESC.Dimension           = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
			Resource_DESC.Layout              = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			Resource_DESC.Width               = 0;
			Resource_DESC.Height              = 1;
			Resource_DESC.Format              = DXGI_FORMAT_UNKNOWN;
			Resource_DESC.SampleDesc.Count    = 1;
			Resource_DESC.SampleDesc.Quality  = 0;
			Resource_DESC.Flags               = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;

			D3D12_HEAP_PROPERTIES HEAP_Props  = {};
			HEAP_Props.CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			HEAP_Props.Type                   = D3D12_HEAP_TYPE_DEFAULT;
			HEAP_Props.MemoryPoolPreference   = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
			HEAP_Props.CreationNodeMask       = 0;
			HEAP_Props.VisibleNodeMask        = 0;

			ID3D12Resource* NewBuffer = nullptr;
			HR = RS->pDevice->CreateCommittedResource(	&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, 
														&Resource_DESC, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&NewBuffer));

			CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE STATIC MESH BUFFER!"));

			SETDEBUGNAME(NewBuffer, __func__);

			UpdateResourceByTemp(RS, NewBuffer, Buffer, BufferSize, 1, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
			return NewBuffer;
		};

		using DirectX::XMMATRIX;
		{	// Create Vertex Buffer
			size_t	BufferSize = 0;

			char*	Vertices = nullptr;
			for (auto G : Batcher->Geometry)
			{
				if (G) {
					for (auto B : G->Buffers)
					{
						if (B) {
							if (B->GetBufferType() == VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION)
							{
								BufferSize += B->GetBufferSizeRaw();
								break;
							}
						}
					}
				}
				else break;
			}

			Vertices = (char*)TempMemory->_aligned_malloc(BufferSize, 16); FK_ASSERT(Vertices);

			if (BufferSize)
			{
				size_t offset = 0;
				size_t GE = 0;
				for (auto G : Batcher->Geometry)
				{
					if (G) {
						for (auto B : G->Buffers)
						{
							if (B) {
								if (B->GetBufferType() == VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION)
								{
									memcpy(Vertices + offset, B->GetBuffer(), B->GetBufferSizeRaw());
									Batcher->GeometryTable[GE++].VertexOffset += offset / sizeof(float[3]);
									offset += B->GetBufferSizeRaw();

									break;
								}
							}
						}
					}
					else break;
				}
			}

			Batcher->VertexBuffer = CreateBuffer(BufferSize, Vertices, RS);
		}
		{	// Create Normals Buffer
			size_t	BufferSize = 0;
			char*	Vertices = nullptr;
			for (auto G : Batcher->Geometry)
			{
				if (G){
					for (auto B : G->Buffers)
					{
						if (B){
							if (B->GetBufferType() == VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL)
							{
								BufferSize += B->GetBufferSizeRaw();
								break;
							}
						}
					}
				}
				else break;
			}

			Vertices = (char*)TempMemory->_aligned_malloc(BufferSize, 16); FK_ASSERT(Vertices);

			if (BufferSize)
			{
				size_t offset = 0;
				for (auto G : Batcher->Geometry)
				{
					if (G){
						for (auto B : G->Buffers)
						{
							if (B){
								if (B->GetBufferType() == VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL)
								{
									memcpy(Vertices + offset, B->GetBuffer(), B->GetBufferSizeRaw());
									offset += B->GetBufferSizeRaw();
									break;
								}
							}
						}
					}
					else break;
				}

				Batcher->NormalBuffer = CreateBuffer(BufferSize, Vertices, RS);
			}

		}
		{	// Create Tangent Buffer
			size_t	BufferSize = 0;
			char*	Vertices = nullptr;
			for (auto G : Batcher->Geometry)
			{
				if (G){
					for (auto B : G->Buffers)
					{
						if (B){
							if (B->GetBufferType() == VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT)
							{
								BufferSize += B->GetBufferSizeRaw();
								break;
							}
						}
					}
				}
				else break;
			}

			Vertices = (char*)TempMemory->_aligned_malloc(BufferSize, 16); FK_ASSERT(Vertices);

			if (BufferSize)
			{
				size_t offset = 0;
				for (auto G : Batcher->Geometry)
				{
					if (G){
						for (auto B : G->Buffers)
						{
							if (B){
								if (B->GetBufferType() == VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT)
								{
									memcpy(Vertices + offset, B->GetBuffer(), B->GetBufferSizeRaw());
									offset += B->GetBufferSizeRaw();
									break;
								}
							}
						}
					}
					else break;
				}
				Batcher->TangentBuffer = CreateBuffer(BufferSize, Vertices, RS);
			}
		}
		{	// Create Index Buffer
			size_t	BufferSize	= 0;
			byte*	Indices		= nullptr;

			for (auto G : Batcher->Geometry)
			{
				if (G){
					for (auto B : G->Buffers)
					{
						if (B){
							if (B->GetBufferType() == VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX)
							{
								BufferSize += B->GetBufferSizeRaw();
								break;
							}
						}
					}
				}
				else break;
			}

			Indices = (byte*)TempMemory->malloc(BufferSize); FK_ASSERT(Indices);

			if (BufferSize)
			{
				size_t GE		= 0;
				size_t offset	= 0;

				for (auto G : Batcher->Geometry)
				{
					if (G){
						for (auto B : G->Buffers)
						{
							if (B){
								if (B->GetBufferType() == VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX)
								{
									memcpy(Indices + offset, B->GetBuffer(), B->GetBufferSizeRaw());
									Batcher->GeometryTable[GE].	IndexOffset += offset/4;
									Batcher->GeometryTable[GE++].VertexCount += G->IndexCount;
										
									offset += B->GetBufferSizeRaw();

									if ( G->IndexCount > Batcher->MaxVertexPerInstance)
										Batcher->MaxVertexPerInstance = G->IndexCount;
									break;
								}
							}
						}
					}
					else break;
				}

				Batcher->IndexBuffer = CreateBuffer(BufferSize, (char*)Indices, RS);
			}
#if USING(DEBUGGRAPHICS)
			else printf("Warning !! Index Buffer Not Found\n");
#endif
		}
	}


	/************************************************************************************************/

	/*
	StaticMeshBatcher::SceneObjectHandle StaticMeshBatcher::CreateDrawable(NodeHandle node, size_t gi){
		size_t index = TotalInstanceCount++;
		InstanceCount[gi]++;
		SceneObjectHandle hndl = ObjectTable.GetNewHandle();
		ObjectTable[hndl] = index;

		GeometryIndex[index] = gi;
		NodeHandles[index] = node;
		DirtyFlags[index] = DIRTY_Transform | DIRTY_ChangedMesh;

		return hndl;
	}
	*/

	/************************************************************************************************/


	//void StaticMeshBatcher::PrintFrameStats(){
	//	for (auto I = 0; I < TriMeshCount; ++I)
	//		printf("Tris per Instance Type: %u\nInstances: %u\nTotal Tris in Batch count: %u \n\n\n\n", GeometryTable[I].VertexCount, uint32_t(InstanceCount[I]), uint32_t(GeometryTable[I].VertexCount * InstanceCount[I]));

	//}


	/************************************************************************************************/


	void Draw(FlexKit::RenderSystem* RS, StaticMeshBatcher* Batcher)
	{
		/*

		SetShader(RS->ContextState, M->GetShader(VPShader));
		SetShader(RS->ContextState, M->GetShader(PShader));

		UINT OFFSETS[] = { 0, 0, 0 };
		UINT STRIDES[] = { 12, 12, 12 };

		
		ID3D11ShaderResourceView* SRVs[] =
		{
			TransformSRV,
			GTSRV,
			IndexSRV,
			VertexSRV,
			NormalSRV,
			TangentSRV
		};
		
		ID3D11Buffer* VertexBuffers[] = 
		{
			VertexBuffer, 
			NormalBuffer, 
			TangentBuffer
		};

		RS->ContextState->IASetInputLayout(IL);
		RS->ContextState->IASetVertexBuffers(0, 3, VertexBuffers, STRIDES, OFFSETS);
		RS->ContextState->VSSetConstantBuffers(0, 1, &C->Buffer);
		RS->ContextState->VSSetShaderResources(0, 1, SRVs);
		RS->ContextState->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT::DXGI_FORMAT_R32_UINT, 0);
		RS->ContextState->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		RS->ContextState->DrawInstanced(
			MaxVertexPerInstance,
			InstanceCount,
			0,
			0);
		*/
		/*
		RS->ContextState->DrawIndexed(
				GeometryTable[0].VertexCount,
				0, 
				0);
		
		for (auto I = 0; I < TriMeshCount; ++I)
		{
			RS->ContextState->DrawIndexedInstanced(
				GeometryTable[I].VertexCount,
				InstanceCount[I],
				GeometryTable[I].IndexOffset,
				GeometryTable[I].VertexOffset,
				0);
		}
		//RS->ContextState->DrawIndexedInstancedIndirect();
		ID3D11ShaderResourceView* NULLSRVs[] =
		{
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr
		};
		//RS->ContextState->VSSetShaderResources(0, 6, NULLSRVs);
		*/
	}


	/************************************************************************************************/
	
	
	void InitiateShadowMapPass(RenderSystem* RenderSystem, ShadowMapPass* Out)
	{
		Shader VertexShader_Animated;
		Shader VertexShader_Static;
		Shader PixelShader;

		// Load Shader
		{
			VertexShader_Animated	= LoadShader("VMainVertexPallet_ShadowMapping",	"ShadowMapping", "vs_5_0", "assets\\vshader.hlsl");
			VertexShader_Static		= LoadShader("VMain_ShadowMapping",				"ShadowMapping", "vs_5_0", "assets\\vshader.hlsl");
			PixelShader				= LoadShader("PMain_ShadowMapping",				"ShadowMapping", "ps_5_0", "assets\\pshader.hlsl");
		}

		// Create Pipelines State
		{
			D3D12_RASTERIZER_DESC		Rast_Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

			D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
			Depth_Desc.DepthFunc					= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
			//Depth_Desc.DepthFunc					= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			Depth_Desc.DepthEnable					= true;
			
			D3D12_INPUT_ELEMENT_DESC InputElements[5] = {
				{ "POSITION",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "WEIGHTS",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,	3, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "WEIGHTINDICES",	0, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT,  4, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {};{
				PSO_Desc.pRootSignature			= RenderSystem->Library.RS6CBVs4SRVs;
				PSO_Desc.VS						= VertexShader_Static;
				PSO_Desc.PS						= PixelShader;
				PSO_Desc.RasterizerState		= Rast_Desc;
				PSO_Desc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
				PSO_Desc.SampleMask				= UINT_MAX;
				PSO_Desc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
				PSO_Desc.NumRenderTargets		= 0;
				PSO_Desc.SampleDesc.Count		= 1;
				PSO_Desc.SampleDesc.Quality		= 0;
				PSO_Desc.DSVFormat				= DXGI_FORMAT_D32_FLOAT;
				PSO_Desc.InputLayout			= { InputElements, 1 };
				PSO_Desc.DepthStencilState		= Depth_Desc;
			}

			ID3D12PipelineState* PSO = nullptr;
			HRESULT HR = RenderSystem->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
			CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE SHADOW MAPPING PIPELINE STATE OBJECT!"));

			Out->PSO_Static = PSO;

			PSO_Desc.VS = VertexShader_Animated;
			PSO_Desc.InputLayout = { InputElements, 3 };

			HR = RenderSystem->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
			CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE SHADOW MAPPING PIPELINE STATE OBJECT!"));

			Out->PSO_Animated = PSO;
		}

		Release(&VertexShader_Animated);
		Release(&VertexShader_Static);
		Release(&PixelShader);
	}


	/************************************************************************************************/


	void CleanUpShadowPass(ShadowMapPass* Out)
	{
		Out->PSO_Animated->Release();
		Out->PSO_Static->Release();
	}


	/************************************************************************************************/


	/*
	void RenderShadowMap(RenderSystem* RS, PVS* _PVS, SpotLightShadowCaster* Caster, Texture2D* RenderTarget, ShadowMapPass* PSOs, GeometryTable* GT)
	{
		auto CL             = RS->_GetCurrentCommandList();
		auto FrameResources = RS->_GetCurrentFrameResources();
		auto DescPOSGPU     = RS->_GetDescTableCurrentPosition_GPU(); // _Ptr to Beginning of Heap On GPU
		auto DescPOS        = RS->_ReserveDescHeap(6);

		auto DSVPOSCPU      = RS->_GetDSVTableCurrentPosition_CPU(); // _Ptr to Current POS On RTV heap on CPU
		auto DSVPOS         = RS->_ReserveDSVHeap(6);

		PushDepthStencil(RS, RenderTarget, DSVPOS);

		CL->SetGraphicsRootSignature(RS->Library.RS4CBVs4SRVs);
		CL->OMSetRenderTargets(0, nullptr, true, &DSVPOSCPU);
		CL->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//CL->SetGraphicsRootConstantBufferView(1, Caster->C.Buffer->GetGPUVirtualAddress());

		auto itr = _PVS->begin();
		auto end = _PVS->end();

		for (; itr != end; ++itr)
		{
			Drawable* E = (*itr).D;
			
			if (!E->Posed)
				CL->SetPipelineState(PSOs->PSO_Static);
			else
				CL->SetPipelineState(PSOs->PSO_Animated);

			//if (DrawInfo.OcclusionID != -1)
			//	CL->SetPredication()

			//CL->SetGraphicsRootConstantBufferView(2, E->VConstants->GetGPUVirtualAddress());

			auto CurrentMesh  = GetMesh(GT, E->MeshHandle);
			size_t IBIndex	  = CurrentMesh->VertexBuffer.MD.IndexBuffer_Index;
			size_t IndexCount = CurrentMesh->IndexCount;

			D3D12_INDEX_BUFFER_VIEW		IndexView;
			IndexView.BufferLocation = GetBuffer(CurrentMesh, IBIndex)->GetGPUVirtualAddress();
			IndexView.Format		 = DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
			IndexView.SizeInBytes	 = IndexCount * 32;

			static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;
			FK_ASSERT(AddVertexBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION, CurrentMesh, VBViews));

			if (E->Posed) {
				FK_ASSERT(AddVertexBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1, CurrentMesh, VBViews));
				FK_ASSERT(AddVertexBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2, CurrentMesh, VBViews));
			}

			CL->IASetIndexBuffer	(&IndexView);
			CL->IASetVertexBuffers	(0, VBViews.size(), VBViews.begin());
			CL->DrawIndexedInstanced(IndexCount, 1, 0, 0, 0);
		}

		D3D12_RESOURCE_TRANSITION_BARRIER Transition = {
			*RenderTarget, 0,
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ,
		};


		D3D12_RESOURCE_BARRIER Barrier[] = {
			{
				D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE,
				Transition
			}
		};
		CL->ResourceBarrier(1, Barrier);

	}
	*/

	/************************************************************************************************/

	// assumes File str should be at most 256 bytes
	TextureHandle LoadDDSTextureFromFile(char* file, RenderSystem* RS, iAllocator* MemoryOut)
	{
		Texture2D tex = {};
		wchar_t	wfile[256];
		size_t	ConvertedSize = 0;
		mbstowcs_s(&ConvertedSize, wfile, file, 256);
		auto [Texture, Sucess] = LoadDDSTexture2DFromFile_2(file, MemoryOut, RS);

		FK_ASSERT(Sucess != false, "Failed to Create Texture!");


		return Texture;
	}


	/************************************************************************************************/


	TextureHandle MoveTextureBufferToVRAM(RenderSystem* RS, TextureBuffer* buffer,  iAllocator* tempMemory)
	{
		auto textureHandle = RS->CreateTexture2D(buffer->WH, FORMAT_2D::R8G8B8A8_UNORM);
		RS->UploadTexture(textureHandle, buffer->Buffer, buffer->Size);

		return textureHandle;
	}


	/************************************************************************************************/


	TextureHandle MoveTextureBuffersToVRAM(RenderSystem* RS, TextureBuffer* buffer, size_t MIPCount, iAllocator* tempMemory)
	{
		size_t bufferSize = 0;
		for (size_t itr = 0; itr < MIPCount; ++itr)
			bufferSize += buffer[itr].Size;

		byte* textureBuffer = (byte*)tempMemory->malloc(bufferSize);

		static_vector<size_t> MIPOffsets;
		size_t offset = 0;
		for (size_t itr = 0; itr < MIPCount; ++itr) 
		{
			memcpy(textureBuffer + offset, buffer[itr].Buffer, buffer[itr].Size);
			MIPOffsets.push_back(offset);
			offset += buffer[itr].Size;
		}


		auto textureHandle = RS->CreateTexture2D(buffer[0].WH, FORMAT_2D::R8G8B8A8_UINT, MIPCount);
		RS->UploadTexture(textureHandle, textureBuffer, bufferSize, buffer->WH, MIPCount, MIPOffsets.begin(), tempMemory);

		return textureHandle;
	}


	/************************************************************************************************/


	Texture2D	LoadTexture(TextureBuffer* Buffer, RenderSystem* RS, iAllocator* Memout)
	{
		Texture2D_Desc TextureDesc;
		TextureDesc.MipLevels    = 1;
		TextureDesc.RenderTarget = false;
		TextureDesc.CV           = false;
		TextureDesc.FLAGS        = FlexKit::SPECIALFLAGS::NONE;
		TextureDesc.Format       = FORMAT_2D::R8G8B8A8_UNORM;
		TextureDesc.Width        = Buffer->WH[0];
		TextureDesc.Height       = Buffer->WH[1];
		TextureDesc.UAV          = false;
		TextureDesc.Write        = false;
		TextureDesc.initialData  = Buffer->Buffer;
		
		size_t ResourceSizes[] = { Buffer->Size };

		auto temp = CreateTexture2D(RS, &TextureDesc);
		FlexKit::SubResourceUpload_Desc Desc = {};
		Desc.Data                            = Buffer->Buffer;
		Desc.RowCount                        = Buffer->WH[1];
		Desc.RowSize                         = Buffer->WH[0] * 4;
		Desc.Size	                         = Buffer->Size;
		Desc.WH	                             = Buffer->WH;
		Desc.SubResourceCount                = 1;
		Desc.SubResourceSizes                = ResourceSizes;
		Desc.SubResourceStart                = 0;

		UpdateSubResourceByUploadQueue(RS, temp, &Desc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		SETDEBUGNAME(temp, "TEST TEXTURE! LOADED FROM BMP");
		return temp;
	}


	/************************************************************************************************/


	FLEXKITAPI TextureBuffer CreateTextureBuffer(size_t Width, size_t Height, iAllocator* Memory)
	{
		TextureBuffer Out;
		auto Mem		= Memory->malloc(Width * Height * 4);
		Out.Buffer      = (byte*)Mem;
		Out.ElementSize = 4;
		Out.Memory      = Memory;
		Out.Size        = Width * Height * 4;
		Out.WH          = {Width, Height};

		return Out;
	}


	/************************************************************************************************/


	void FreeTexture(Texture2D* Tex){
		if(!Tex) return;
		if(Tex->Texture)Tex->Texture->Release();
	}



	/************************************************************************************************/


	void SetFullscreenRect(ID3D12GraphicsCommandList* CL, RenderWindow* TargetWindow, UINT I)
	{
		D3D12_RECT		RECT = D3D12_RECT();
		D3D12_VIEWPORT	VP	 = D3D12_VIEWPORT();

		RECT.right  = (UINT)TargetWindow->WH[0];
		RECT.bottom = (UINT)TargetWindow->WH[1];
		VP.Height   = (UINT)TargetWindow->WH[1];
		VP.Width    = (UINT)TargetWindow->WH[0];
		VP.MaxDepth = 1;
		VP.MinDepth = 0;
		VP.TopLeftX = 0;
		VP.TopLeftY = 0;

		CL->RSSetViewports(1, &VP);
		CL->RSSetScissorRects(1, &RECT);
	}


	/************************************************************************************************/


	TriMeshHandle CreateCube(RenderSystem* RS, iAllocator* memory, float R, GUID_t MeshID)
	{
		VertexBufferView* Views[3];

		// Index Buffer
		{
			auto bufferSize = sizeof(uint32_t) * 128;
			Views[0] = CreateVertexBufferView((byte*)memory->_aligned_malloc(bufferSize), bufferSize);
			Views[0]->Begin(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32);

			for(uint32_t I = 0; I < 36; ++I)
				Views[0]->Push(I);

			FK_ASSERT( Views[0]->End() );
		}

		// Vertex Buffer
		{
			Views[1] = CreateVertexBufferView((byte*)memory->_aligned_malloc(4096), 4096);
			Views[1]->Begin(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32);

			float3 TopFarLeft	= { -R,  R, -R };
			float3 TopFarRight  = { -R,  R, -R };
			float3 TopNearLeft  = { -R,  R,  R };
			float3 TopNearRight = {  R,  R,  R };

			float3 BottomFarLeft   = { -R,  -R, -R };
			float3 BottomFarRight  = { -R,  -R, -R };
			float3 BottomNearLeft  = { -R,  -R,  R };
			float3 BottomNearRight = {  R,  -R,  R };

			// Top Plane
			Views[1]->Push(TopFarLeft); 
			Views[1]->Push(TopFarRight);
			Views[1]->Push(TopNearRight);

			Views[1]->Push(TopNearRight);
			Views[1]->Push(TopNearLeft);
			Views[1]->Push(TopFarLeft);


			// Bottom Plane
			Views[1]->Push(BottomFarLeft);
			Views[1]->Push(BottomNearRight);
			Views[1]->Push(BottomFarRight);

			Views[1]->Push(BottomNearRight);
			Views[1]->Push(BottomFarLeft);
			Views[1]->Push(BottomNearLeft);

			// Left Plane
			Views[1]->Push(TopFarLeft);
			Views[1]->Push(TopNearLeft);
			Views[1]->Push(BottomNearLeft);

			Views[1]->Push(TopFarLeft);
			Views[1]->Push(BottomNearLeft);
			Views[1]->Push(BottomFarLeft);
		
			// Right Plane
			Views[1]->Push(TopFarRight);
			Views[1]->Push(BottomNearRight);
			Views[1]->Push(TopNearRight);

			Views[1]->Push(TopFarRight);
			Views[1]->Push(BottomFarRight);
			Views[1]->Push(TopNearRight);

			// Near Plane
			Views[1]->Push(TopNearLeft);
			Views[1]->Push(TopNearRight);
			Views[1]->Push(BottomNearRight);

			Views[1]->Push(TopNearLeft);
			Views[1]->Push(BottomNearRight);
			Views[1]->Push(BottomNearLeft);

			// FarPlane
			Views[1]->Push(TopFarLeft);
			Views[1]->Push(TopFarRight);
			Views[1]->Push(BottomFarRight);
		
			Views[1]->Push(TopFarLeft);
			Views[1]->Push(BottomFarRight);
			Views[1]->Push(BottomFarLeft);

			FK_ASSERT( Views[1]->End() );
		}
		// Normal Buffer
		{
			Views[2] = CreateVertexBufferView((byte*)memory->_aligned_malloc(4096), 4096);
			Views[2]->Begin(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32);

			float3 TopPlane		= {  0,  1,  0 };
			float3 BottomPlane  = {  0, -1,  0 };
			float3 LeftPlane	= { -1,  0,  0 };
			float3 RightPlane	= {  1,  0,  1 };
			float3 NearPlane	= {  0,  0,  1 };
			float3 FarPlane		= {  0,  0, -1 };

			// Top Plane
			Views[2]->Push(TopPlane);
			Views[2]->Push(TopPlane);
			Views[2]->Push(TopPlane);

			Views[2]->Push(TopPlane);
			Views[2]->Push(TopPlane);
			Views[2]->Push(TopPlane);

			// Bottom Plane
			Views[2]->Push(BottomPlane);
			Views[2]->Push(BottomPlane);
			Views[2]->Push(BottomPlane);

			Views[2]->Push(BottomPlane);
			Views[2]->Push(BottomPlane);
			Views[2]->Push(BottomPlane);

			// Left Plane
			Views[2]->Push(LeftPlane);
			Views[2]->Push(LeftPlane);
			Views[2]->Push(LeftPlane);

			Views[2]->Push(LeftPlane);
			Views[2]->Push(LeftPlane);
			Views[2]->Push(LeftPlane);

			// Right Plane
			Views[2]->Push(RightPlane);
			Views[2]->Push(RightPlane);
			Views[2]->Push(RightPlane);

			Views[2]->Push(RightPlane);
			Views[2]->Push(RightPlane);
			Views[2]->Push(RightPlane);

			// Near Plane
			Views[2]->Push(NearPlane);
			Views[2]->Push(NearPlane);
			Views[2]->Push(NearPlane);

			Views[2]->Push(NearPlane);
			Views[2]->Push(NearPlane);
			Views[2]->Push(NearPlane);

			// FarPlane
			Views[2]->Push(FarPlane);
			Views[2]->Push(FarPlane);
			Views[2]->Push(FarPlane);

			Views[2]->Push(FarPlane);
			Views[2]->Push(FarPlane);
			Views[2]->Push(FarPlane);

			FK_ASSERT( Views[2]->End() );
		}

		Mesh_Description Desc;
		Desc.Buffers		= Views;
		Desc.BufferCount	= 3;
		Desc.IndexBuffer	= 0;
		Desc.memory			= memory;

		auto meshHandle		= BuildMesh(RS, &Desc, MeshID);
		auto triMesh		= GetMeshResource(meshHandle);

		triMesh->Info.max		= float3{  R,  R,  R };
		triMesh->Info.min		= float3{ -R, -R, -R };
		triMesh->Info.Offset	= float3{  0,  0,  0 };
		triMesh->Info.r			= triMesh->Info.max.magnitude();
		triMesh->BS				= float4(0, 0, 0, R);
		triMesh->IndexCount		= 36;

		for(auto V : Views)
			memory->_aligned_free(V);

		return meshHandle;
	}


	/************************************************************************************************/
}//	Namespace FlexKit
