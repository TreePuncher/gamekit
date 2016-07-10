

/**********************************************************************

Copyright (c) 2014-2016 Robert May

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

#include "AnimationUtilities.h"
#include "graphics.h"

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
	/************************************************************************************************/
	// UTILITIY FUNCTIONS

	template<typename TY_>
	HRESULT CheckHR(HRESULT HR, TY_ FN)
	{
		auto res = FAILED(HR);
		if (res) FN();
		return HR;
	}

	#define PRINTERRORBLOB(Blob) [&](){std::cout << Blob->GetBufferPointer();}
	#define ASSERTONFAIL(ERRORMESSAGE)[&](){FK_ASSERT(0, ERRORMESSAGE);}

	void SetDebugName(ID3D12Object* Obj, const char* cstr, size_t size)
	{
#if USING(DEBUGGRAPHICS)
		if (!Obj)
			return;

		wchar_t WString[128];
		mbstowcs(WString, cstr, 128);
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

	#define CALCULATECONSTANTBUFFERSIZE(TYPE) (sizeof(TYPE)/1024 + 1024)


	/************************************************************************************************/


	LRESULT CALLBACK WindowProcess( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
	{
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

			if (wParam == MK_LBUTTON)
				ev.mData1.mKC[0] = KEYCODES::KC_MOUSELEFT;

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
			case VK_SPACE:
				ev.mData1.mKC[0]   = KC_SPACE;
				break;
			case VK_ESCAPE:
				ev.mData1.mKC [0]  = KC_ESC;
				break;
				//case VK_W
			case 'A':
				ev.mData1.mKC [0]  = KC_A;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'B':
				ev.mData1.mKC [0]  = KC_B;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'C':
				ev.mData1.mKC [0]  = KC_C;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'D':
				ev.mData1.mKC [0]  = KC_D;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'E':
				ev.mData1.mKC [0]  = KC_E;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'F':
				ev.mData1.mKC [0]  = KC_F;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'G':
				ev.mData1.mKC [0]  = KC_G;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'H':
				ev.mData1.mKC [0]  = KC_H;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'I':
				ev.mData1.mKC [0]  = KC_I;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'J':
				ev.mData1.mKC [0]  = KC_J;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'K':
				ev.mData1.mKC [0]  = KC_K;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'L':
				ev.mData1.mKC [0]  = KC_L;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'M':
				ev.mData1.mKC [0]  = KC_M;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'N':
				ev.mData1.mKC [0]  = KC_N;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'O':
				ev.mData1.mKC [0]  = KC_O;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'P':
				ev.mData1.mKC [0]  = KC_P;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'Q':
				ev.mData1.mKC [0]  = KC_Q;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'R':
				ev.mData1.mKC [0]  = KC_R;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'S':
				ev.mData1.mKC [0]  = KC_S;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'T':
				ev.mData1.mKC [0]  = KC_T;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'U':
				ev.mData1.mKC [0]  = KC_U;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'V':
				ev.mData1.mKC [0]  = KC_V;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'W':
				ev.mData1.mKC [0]  = KC_W;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'X':
				ev.mData1.mKC [0]  = KC_X;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'Y':
				ev.mData1.mKC [0]  = KC_Y;
				ev.mData2.mINT[0]  = wParam;
				break;
			case 'Z':
				ev.mData1.mKC [0]  = KC_Z;
				ev.mData2.mINT[0]  = wParam;
				break;
			default:

				break;
			}
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


	void CreateDescriptorHeaps(RenderSystem* RS, DescriptorHeaps* out)
	{
		ID3D12DescriptorHeap* RTVDescHeap = nullptr;
		ID3D12DescriptorHeap* DSVDescHeap = nullptr;

		D3D12_DESCRIPTOR_HEAP_DESC	DH_Desc = {};
		DH_Desc.Flags    = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		DH_Desc.Type     = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		DH_Desc.NodeMask = 0;
		
		HRESULT HR;
		DH_Desc.NumDescriptors	= 10;
		HR = RS->pDevice->CreateDescriptorHeap(&DH_Desc, IID_PPV_ARGS(&RTVDescHeap));
		FK_ASSERT(FAILED(HR), "ERROR!");

		DH_Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		DH_Desc.NumDescriptors	= 10;
		HR = RS->pDevice->CreateDescriptorHeap(&DH_Desc, IID_PPV_ARGS(&DSVDescHeap));
		FK_ASSERT(FAILED(HR), "ERROR!");

		out->DescriptorCBVSRVUAVSize = RS->DescriptorCBVSRVUAVSize;
		out->DescriptorDSVSize       = RS->DescriptorDSVSize;
		out->DescriptorRTVSize       = RS->DescriptorRTVSize;
		out->RTVDescHeap             = RTVDescHeap;
		out->DSVDescHeap             = DSVDescHeap;
	}


	/************************************************************************************************/


	void CleanUpDescriptorHeaps(DescriptorHeaps* DDH)
	{
		DDH->RTVDescHeap->Release();
		DDH->DSVDescHeap->Release();
	}


	/************************************************************************************************/


	void CreateRootSignatureLibrary(RenderSystem* out)
	{
		ID3D12Device* Device = out->pDevice;
		CD3DX12_STATIC_SAMPLER_DESC	 Default(0);
		{
			// Pixel Processor Root Sig
			CD3DX12_DESCRIPTOR_RANGE ranges[2];
			ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);
			ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 4, 4);

			CD3DX12_ROOT_PARAMETER Parameters[5];
			Parameters[0].InitAsDescriptorTable(2, ranges, D3D12_SHADER_VISIBILITY_ALL);
			Parameters[1].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
			Parameters[2].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
			Parameters[3].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL);
			Parameters[4].InitAsConstantBufferView(3, 0, D3D12_SHADER_VISIBILITY_ALL);

			ID3DBlob* Signature = nullptr;
			ID3DBlob* ErrorBlob = nullptr;
			D3D12_ROOT_SIGNATURE_DESC	RootSignatureDesc;

			CD3DX12_ROOT_SIGNATURE_DESC::Init(RootSignatureDesc, 5, Parameters, 1, &Default, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
			CheckHR(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &Signature, &ErrorBlob),
												PRINTERRORBLOB(ErrorBlob));

			ID3D12RootSignature* NewRootSig = nullptr;
			CheckHR(Device->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(), IID_PPV_ARGS(&NewRootSig)),
												ASSERTONFAIL("FAILED TO CREATE ROOT SIGNATURE"));

			out->Library.RS4CBVs4SRVs = NewRootSig;
		}
		{
			// Stream-Out
			CD3DX12_DESCRIPTOR_RANGE ranges[2];
			ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);
			ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 4, 4);

			CD3DX12_ROOT_PARAMETER Parameters[5];
			Parameters[0].InitAsDescriptorTable(2, ranges, D3D12_SHADER_VISIBILITY_ALL);
			Parameters[1].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
			Parameters[2].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
			Parameters[3].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL);
			Parameters[4].InitAsUnorderedAccessView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

			ID3DBlob* Signature = nullptr;
			ID3DBlob* ErrorBlob = nullptr;
			D3D12_ROOT_SIGNATURE_DESC	RootSignatureDesc;

			CD3DX12_ROOT_SIGNATURE_DESC::Init(RootSignatureDesc, 5, Parameters, 1, &Default, 
												D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | 
												D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT);

			CheckHR(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &Signature, &ErrorBlob),
				PRINTERRORBLOB(ErrorBlob));

			ID3D12RootSignature* NewRootSig = nullptr;
			CheckHR(Device->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(), IID_PPV_ARGS(&NewRootSig)),
				ASSERTONFAIL("FAILED TO CREATE ROOT SIGNATURE"));

			out->Library.RS4CBVs_SO = NewRootSig;
		}
		{
			// Compute Processor Root Sig
			CD3DX12_DESCRIPTOR_RANGE ranges[3];
			ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0);
			ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);
			ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 4, 4);

			CD3DX12_ROOT_PARAMETER Parameters[5];
			Parameters[0].InitAsDescriptorTable(sizeof(ranges)/sizeof(ranges[0]), ranges, D3D12_SHADER_VISIBILITY_ALL);
			Parameters[1].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
			Parameters[2].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
			Parameters[3].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL);
			Parameters[4].InitAsConstantBufferView(3, 0, D3D12_SHADER_VISIBILITY_ALL);

			ID3DBlob* Signature = nullptr;
			ID3DBlob* ErrorBlob = nullptr;
			D3D12_ROOT_SIGNATURE_DESC	RootSignatureDesc;

			CD3DX12_ROOT_SIGNATURE_DESC::Init(RootSignatureDesc, 5, Parameters, 1, &Default, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
			CheckHR(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &Signature, &ErrorBlob),
				PRINTERRORBLOB(ErrorBlob));

			ID3D12RootSignature* NewRootSig = nullptr;
			CheckHR(Device->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(), IID_PPV_ARGS(&NewRootSig)),
				ASSERTONFAIL("FAILED TO CREATE ROOT SIGNATURE"));

			out->Library.RS2UAVs4SRVs4CBs = NewRootSig;
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

		CD3DX12_RANGE Range(0, 0);
		HR = TempBuffer->Map(0, &Range, (void**)&RS->CopyEngine.Buffer); CheckHR(HR, ASSERTONFAIL("FAILED TO MAP TEMP BUFFER"));
	}


	/************************************************************************************************/


	void UpdateGPUResource(RenderSystem* RS, void* Data, size_t Size, ID3D12Resource* Dest)
	{
		FK_ASSERT(Data);
		FK_ASSERT(Dest);

		auto& CopyEngine = RS->CopyEngine;
		ID3D12GraphicsCommandList* CS = GetCurrentCommandList(RS);
		 
		// Not enough remaining Space in Buffer GOTO Beginning
		if	(CopyEngine.Position + Size > CopyEngine.Size)
			CopyEngine.Position = 0;

		if(CopyEngine.Last <= CopyEngine.Position + Size)
		{// Safe, Do Upload
			memcpy(CopyEngine.Buffer + CopyEngine.Position, Data, Size);
			CS->CopyBufferRegion(Dest, 0, CopyEngine.TempBuffer, CopyEngine.Position, Size);
			CopyEngine.Position += Size;
		}
		else if (CopyEngine.Last > CopyEngine.Position)
		{// Potential Overlap condition
			if(CopyEngine.Position + Size  < CopyEngine.Last)
			{// Safe, Do Upload
				memcpy(CopyEngine.Buffer + CopyEngine.Position, Data, Size);
				CS->CopyBufferRegion(Dest, 0, CopyEngine.TempBuffer, CopyEngine.Position, Size);
				CopyEngine.Position += Size;
			}
			else
			{// Resize Buffer and Try again
				AddTempBuffer(CopyEngine.TempBuffer, RS);

				D3D12_RESOURCE_DESC   Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(CopyEngine.Size * 2);
				D3D12_HEAP_PROPERTIES HEAP_Props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

				ID3D12Resource* TempBuffer;
				HRESULT HR = RS->pDevice->CreateCommittedResource(&HEAP_Props, D3D12_HEAP_FLAG_NONE,
					&Resource_DESC, D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr, IID_PPV_ARGS(&TempBuffer));

				RS->CopyEngine.Position   = 0;
				RS->CopyEngine.Last		  = 0;
				RS->CopyEngine.Size       = CopyEngine.Size * 2;
				RS->CopyEngine.TempBuffer = TempBuffer;
				SETDEBUGNAME(TempBuffer, "TEMPORARY");

				CD3DX12_RANGE Range(0, 0);
				HR = TempBuffer->Map(0, &Range, (void**)&RS->CopyEngine.Buffer); CheckHR(HR, ASSERTONFAIL("FAILED TO MAP TEMP BUFFER"));

				return UpdateGPUResource(RS, Data, Size, Dest);
			}
		}
	}


	/************************************************************************************************/


	void CopyEnginePostFrameUpdate(RenderSystem* RS)
	{
		auto& CopyEngine = RS->CopyEngine;
		CopyEngine.Last  = CopyEngine.Position;
	}


	/************************************************************************************************/


	void InitiateRenderSystem( Graphics_Desc* in, RenderSystem* out )
	{
#if USING( DEBUGGRAPHICS )
		gWindowHandle		= GetConsoleWindow();
		gInstance			= GetModuleHandle( 0 );
#endif

		RegisterWindowClass(gInstance);

		RenderSystem NewRenderSystem = {0};
		NewRenderSystem.Memory			   = in->Memory;
		NewRenderSystem.Settings.AAQuality = 0;
		NewRenderSystem.Settings.AASamples = 1;
		UINT DeviceFlags = 0;

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
		
		HR = D3D12CreateDevice(nullptr,	D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device));
		if( FAILED( HR ) )
		{
		#ifdef _DEBUG
			FK_ASSERT(0);
		#endif
			return;
		}

#if USING( DEBUGGRAPHICS )
		HR =  Device->QueryInterface(__uuidof(ID3D12DebugDevice), (void**)&DebugDevice);
#else
		DebugDevice = nullptr;
#endif
		
		{
			ID3D12Fence* NewFence = nullptr;
			HR = Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&NewFence);
			FK_ASSERT(FAILED(HR), "FAILED TO CREATE FENCE!");

			NewRenderSystem.Fence = NewFence;
		}

		for(size_t I = 0; I < 3; ++I){
			NewRenderSystem.Fences[I].FenceHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			NewRenderSystem.Fences[I].FenceValue  = 0;
		}
 			
		D3D12_COMMAND_QUEUE_DESC CQD ={};
		CQD.Flags							            = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
		CQD.Type								        = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;
		D3D12_COMMAND_QUEUE_DESC UploadCQD ={};
		UploadCQD.Flags							        = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
		UploadCQD.Type								    = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY;

		D3D12_COMMAND_QUEUE_DESC ComputeCQD = {};
		UploadCQD.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
		UploadCQD.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE;

		D3D12_DESCRIPTOR_HEAP_DESC	FrameTextureHeap_DESC = {};
		FrameTextureHeap_DESC.Flags				= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		FrameTextureHeap_DESC.NumDescriptors	= 16000;
		FrameTextureHeap_DESC.NodeMask			= 0;
		FrameTextureHeap_DESC.Type				= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

		D3D12_DESCRIPTOR_HEAP_DESC	RenderTargetHeap_DESC = {};
		RenderTargetHeap_DESC.Flags				= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		RenderTargetHeap_DESC.NumDescriptors	= 256;
		RenderTargetHeap_DESC.NodeMask			= 0;
		RenderTargetHeap_DESC.Type				= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

		ID3D12CommandQueue*			GraphicsQueue		= nullptr;
		ID3D12CommandQueue*			UploadQueue			= nullptr;
		ID3D12CommandQueue*			ComputeQueue		= nullptr;
		ID3D12CommandAllocator*		UploadAllocator		= nullptr;
		ID3D12CommandAllocator*		ComputeAllocator	= nullptr;
		ID3D12GraphicsCommandList*	UploadList	        = nullptr;
		ID3D12GraphicsCommandList*	ComputeList	        = nullptr;
		IDXGIFactory4*				DXGIFactory         = nullptr;

		// Create Resources
		{
			for(size_t I = 0; I < QueueSize; ++I)
			{
				for(size_t II = 0; II < MaxThreadCount; ++II)
				{
					ID3D12GraphicsCommandList*	CommandList		  = nullptr;
					ID3D12CommandAllocator*		GraphicsAllocator = nullptr;

					HR = Device->CreateCommandAllocator	(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,		__uuidof(ID3D12CommandAllocator), (void**)&GraphicsAllocator);						FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND ALLOCATOR!");
					HR = Device->CreateCommandAllocator	(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY,			__uuidof(ID3D12CommandAllocator), (void**)&UploadAllocator);						FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND ALLOCATOR!");
					HR = Device->CreateCommandAllocator	(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE,		__uuidof(ID3D12CommandAllocator), (void**)&ComputeAllocator);						FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND ALLOCATOR!");
					HR = Device->CreateCommandList		(0, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE,	ComputeAllocator,	nullptr, __uuidof(ID3D12CommandList), (void**)&ComputeList);	FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND LIST!");
					HR = Device->CreateCommandList		(0, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,	GraphicsAllocator,	nullptr, __uuidof(ID3D12CommandList), (void**)&CommandList);	FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND LIST!");
					HR = Device->CreateCommandList		(0, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY,		UploadAllocator,	nullptr, __uuidof(ID3D12CommandList), (void**)&UploadList);		FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND LIST!");

					CommandList->Close();
					GraphicsAllocator->Reset();

					NewRenderSystem.FrameResources[I].TempBuffers				= nullptr;
					NewRenderSystem.FrameResources[I].UploadList[II]			= static_cast<ID3D12GraphicsCommandList*>(UploadList);
					NewRenderSystem.FrameResources[I].ComputeList[II]			= static_cast<ID3D12GraphicsCommandList*>(ComputeList);
					NewRenderSystem.FrameResources[I].UploadCLAllocator[II]		= UploadAllocator;
					NewRenderSystem.FrameResources[I].ComputeCLAllocator[II]	= ComputeAllocator;
					NewRenderSystem.FrameResources[I].GraphicsCLAllocator[II]	= GraphicsAllocator;
					NewRenderSystem.FrameResources[I].CommandLists[II]			= static_cast<ID3D12GraphicsCommandList*>(CommandList);

				}

				ID3D12DescriptorHeap* SRVHeap = nullptr;
				HR = Device->CreateDescriptorHeap(&FrameTextureHeap_DESC, IID_PPV_ARGS(&SRVHeap));																									FK_ASSERT(FAILED(HR), "FAILED TO CREATE SRV Heap HEAP!");
				SETDEBUGNAME(SRVHeap, "TEXTUREHEAP");

				ID3D12DescriptorHeap* RTVHeap = nullptr;
				HR = Device->CreateDescriptorHeap(&RenderTargetHeap_DESC, IID_PPV_ARGS(&RTVHeap));																									FK_ASSERT(FAILED(HR), "FAILED TO CREATE SRV Heap HEAP!");
				SETDEBUGNAME(SRVHeap, "RENDERTARGETHEAP");

				NewRenderSystem.FrameResources[I].DescHeap.SRVDescHeap	= SRVHeap;
				NewRenderSystem.FrameResources[I].RTVHeap.RTVDescHeap	= RTVHeap;
				NewRenderSystem.FrameResources[I].ThreadsIssued = 0;
			}

			NewRenderSystem.FrameResources[0].CommandLists[0]->Reset(NewRenderSystem.FrameResources[0].GraphicsCLAllocator[0], nullptr);
			
			HR = Device->CreateCommandQueue		(&CQD,		 __uuidof(ID3D12CommandQueue),		(void**)&GraphicsQueue);	FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND QUEUE!");
			HR = Device->CreateCommandQueue		(&UploadCQD, __uuidof(ID3D12CommandQueue),		(void**)&UploadQueue);		FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND QUEUE!");
			HR = Device->CreateCommandQueue		(&ComputeCQD, __uuidof(ID3D12CommandQueue),		(void**)&ComputeQueue);		FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND QUEUE!");
			HR = CreateDXGIFactory(IID_PPV_ARGS(&DXGIFactory));																																FK_ASSERT(FAILED(HR), "FAILED TO CREATE DXGIFactory!");
		}
		// Copy Resources Over
		{
			NewRenderSystem.pDevice					= Device;
			NewRenderSystem.UploadQueue				= UploadQueue;
			NewRenderSystem.GraphicsQueue			= GraphicsQueue;
			NewRenderSystem.ComputeQueue			= ComputeQueue;
			NewRenderSystem.pGIFactory				= DXGIFactory;
			NewRenderSystem.pDebugDevice			= DebugDevice;
			NewRenderSystem.pDebug					= Debug;
			NewRenderSystem.BufferCount				= 3;
			NewRenderSystem.CurrentIndex			= 0;
			NewRenderSystem.FenceCounter			= 0;
			NewRenderSystem.DescriptorRTVSize		= Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			NewRenderSystem.DescriptorDSVSize		= Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			NewRenderSystem.DescriptorCBVSRVUAVSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		*out = NewRenderSystem;
		CreateDescriptorHeaps(out, &out->DefaultDescriptorHeaps);
		CreateRootSignatureLibrary(out);

		InitiateCopyEngine(out);
	}



	/************************************************************************************************/


	void Destroy(DepthBuffer* DB)
	{
		for(auto R : DB->Buffer)if(R) R->Release();
	}


	/************************************************************************************************/


	void CleanUp(RenderSystem* System)
	{
		for(auto FR : System->FrameResources)
		{
			FR.DescHeap.SRVDescHeap->Release();
			FR.RTVHeap.RTVDescHeap->Release();

			for (auto CL : FR.CommandLists)
				if(CL)CL->Release();
		
			for (auto alloc : FR.GraphicsCLAllocator)
				if (alloc)alloc->Release();

			for (auto CL : FR.ComputeList)
				if (CL)CL->Release();
			
			for (auto alloc : FR.ComputeCLAllocator)
				if (alloc)alloc->Release();

			for (auto CL : FR.UploadList)
				if (CL)CL->Release();

			for (auto alloc : FR.UploadCLAllocator)
				if (alloc)alloc->Release();

			if(FR.TempBuffers)
				System->Memory->_aligned_free(FR.TempBuffers);
		}

		System->GraphicsQueue->Release();
		System->UploadQueue->Release();
		System->ComputeQueue->Release();
		System->Library.RS4CBVs4SRVs->Release();
		System->Library.RS4CBVs_SO->Release();
		System->Library.RS2UAVs4SRVs4CBs->Release();
		System->pGIFactory->Release();
		System->pDevice->Release();
		System->DefaultDescriptorHeaps.DSVDescHeap->Release();
		System->DefaultDescriptorHeaps.RTVDescHeap->Release();
		System->CopyEngine.TempBuffer->Release();
		System->Fence->Release();

#if USING(DEBUGGRAPHICS)
		// Prints Detailed Report
		System->pDebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
		System->pDebugDevice->Release();
		System->pDebug->Release();
#endif
	}


	/************************************************************************************************/


	Texture2D CreateDepthBufferResource(RenderSystem* RS, Tex2DDesc* desc_in, bool Float32)
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

		Clear.DepthStencil.Depth = 0.0f;
		Clear.DepthStencil.Stencil = 0;

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

			HEAP_Props.CPUPageProperty	    = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			HEAP_Props.Type				    = D3D12_HEAP_TYPE_DEFAULT;
			HEAP_Props.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
			HEAP_Props.CreationNodeMask	    = 0;
			HEAP_Props.VisibleNodeMask		= 0;

			Clear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		}


		CheckHR(RS->pDevice->CreateCommittedResource(
			&HEAP_Props,
			D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &Resource_DESC,
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON,
			&Clear, IID_PPV_ARGS(&Resource)), [&]() {});

		NewTexture.Format		= Resource_DESC.Format;
		NewTexture.Texture      = Resource;

		return (NewTexture);
	}


	/************************************************************************************************/


	void CreateDepthBuffer(RenderSystem* RS, uint2 Dimensions, DepthBuffer_Desc& DepthDesc, DepthBuffer* out)
	{
		
		// Create Depth Buffer
		FlexKit::Tex2DDesc desc;
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
		
		CD3DX12_CPU_DESCRIPTOR_HANDLE DepthBufferHandle	= CD3DX12_CPU_DESCRIPTOR_HANDLE (RS->DefaultDescriptorHeaps.DSVDescHeap->GetCPUDescriptorHandleForHeapStart());

		for(size_t I = 0; I < DepthDesc.BufferCount; ++I){
			auto DB = FlexKit::CreateDepthBufferResource(RS, &desc, DepthDesc.Float32);
			if (!DB)
				FK_ASSERT(0);

			RS->pDevice->CreateDepthStencilView(DB.Texture, &DepthStencilDesc, DepthBufferHandle);
			out->Buffer[I]	= DB;
			out->View[I]	= DepthBufferHandle;
			DepthBufferHandle.Offset(RS->DescriptorDSVSize);
		}

		out->Inverted	   = DepthDesc.InverseDepth;
		out->BufferCount   = DepthDesc.BufferCount;
		out->CurrentBuffer = 0;
	}


	/************************************************************************************************/


	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferHandle(DescriptorHeaps* DH, RenderWindow* RW)
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(DH->RTVDescHeap->GetCPUDescriptorHandleForHeapStart(), RW->BufferIndex, DH->DescriptorRTVSize);
	}


	/************************************************************************************************/


	bool CreateRenderWindow( RenderSystem* RS, RenderWindowDesc* In_Desc, RenderWindow* out )
	{
		SetProcessDPIAware();

		RenderWindow NewWindow = {0};
		static size_t Window_Count = 0;

		Window_Count++;

		// Register Window Class
		auto Window = CreateWindow( L"RENDER_WINDOW", L"Render Window", WS_OVERLAPPEDWINDOW,
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
		MoveWindow(Window, In_Desc->POS_X, In_Desc->POS_Y, WindowRect.right - WindowRect.left - ClientRect.right + In_Desc->width, WindowRect.bottom - WindowRect.top - ClientRect.bottom + In_Desc->height, false);

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
			GetCursorPos	(&cursor);
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
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount		= 2;
		swapChainDesc.Width				= In_Desc->width;
		swapChainDesc.Height			= In_Desc->height;
		swapChainDesc.Format			= DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage		= DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect		= DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		swapChainDesc.SampleDesc.Count	= 1;
		ShowWindow(Window, 5);

		IDXGISwapChain1* NewSwapChain_ptr = nullptr;
		HRESULT HR = RS->pGIFactory->CreateSwapChainForHwnd( RS->GraphicsQueue, NewWindow.hWindow, &swapChainDesc, nullptr, nullptr, &NewSwapChain_ptr );
		if ( FAILED( HR ) )
		{
			FK_ASSERT(SUCCEEDED(HR), "FAILED TO CREATE SWAP CHAIN!");
			return false;
		}

		NewWindow.SwapChain_ptr = static_cast<IDXGISwapChain3*>(NewSwapChain_ptr);

		//CreateBackBuffer
		CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHeapHandle(RS->DefaultDescriptorHeaps.RTVDescHeap->GetCPUDescriptorHandleForHeapStart());

		for (size_t I = 0; I < swapChainDesc.BufferCount; ++I)
		{
			ID3D12Resource* buffer;
			NewSwapChain_ptr->GetBuffer( I, __uuidof(ID3D12Resource), (void**)&buffer);
			NewWindow.BackBuffer[I] = buffer;

			RS->pDevice->CreateRenderTargetView(buffer, nullptr, RTVHeapHandle);
			NewWindow.View[I] = RTVHeapHandle;
			RTVHeapHandle.Offset(1, RS->DefaultDescriptorHeaps.DescriptorRTVSize);

			SETDEBUGNAME(buffer, "BackBuffer");
		}

		SetActiveWindow(Window);

		NewWindow.WH[0]         = In_Desc->width;
		NewWindow.WH[1]         = In_Desc->height;
		NewWindow.Close         = false;
		NewWindow.BufferCount	= 2;
		NewWindow.Format		= swapChainDesc.Format;
		memset(NewWindow.InputBuffer, 0, sizeof(NewWindow.InputBuffer));

		*out = NewWindow;
		return true;
	}

	
	/************************************************************************************************/


	D3D12_CPU_DESCRIPTOR_HANDLE	GetBackBufferView(RenderWindow* Window)
	{
		auto index = Window->SwapChain_ptr->GetCurrentBackBufferIndex();
		return Window->View[Window->BufferIndex];
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
		Destroy(Window);

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


	void ReleaseTempResources(RenderSystem* RS)
	{
		auto TempBuffer = RS->FrameResources[RS->CurrentIndex].TempBuffers;
		if (TempBuffer)	{
			for (auto r : *TempBuffer)
				r->Release();

			TempBuffer->clear();
		}
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


	void PresentWindow( RenderWindow* Window, RenderSystem* RS )									
	{ 
		CopyEnginePostFrameUpdate(RS);

		Window->SwapChain_ptr->Present(1, 0);
		Window->BufferIndex = Window->SwapChain_ptr->GetCurrentBackBufferIndex();

		RS->Fences[RS->CurrentIndex].FenceValue = ++RS->FenceCounter;
		RS->GraphicsQueue->Signal(RS->Fence, RS->FenceCounter);
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


	void Destroy( RenderWindow* Window )
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


	VertexBufferView::VertexBufferView()
	{

		mBufferinError     = true;
		mBufferFormat      = VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_UNKNOWN;
		mBufferType        = VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ERROR;
		mBufferElementSize = 0;
		mBuffer			   = nullptr;
		mBufferUsed		   = 0;
		mBufferSize        = 0;
	}


	/************************************************************************************************/


	VertexBufferView::VertexBufferView( byte* _ptr, size_t size )
	{
		mBufferinError     = true;
		mBufferFormat      = VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_UNKNOWN;
		mBufferType        = VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ERROR;
		mBufferElementSize = 0;
		mBuffer			   = _ptr;
		mBufferUsed		   = 0;
		mBufferSize        = size;
		mBufferLock        = false;
	}


	/************************************************************************************************/


	VertexBufferView::~VertexBufferView( void )
	{}


	/************************************************************************************************/


	VertexBufferView VertexBufferView::operator + ( const VertexBufferView& RHS )
	{	//TODO: THIS FUNCTION
		FK_ASSERT(0); 
		//if( RHS.GetBufferSize() != GetBufferSize() )
		//	FK_ASSERT( 0 );
		//if( RHS.GetBufferType() != VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX ) // Un-Combinable
		//	FK_ASSERT( 0 );

		//VertexBufferView combined;
		//combined.Begin( VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_COMBINED, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_COMBINED );
		//combined.SetElementSize( mBufferElementSize + RHS.GetElementSize() );
		////combined._combine( *this, RHS );
		//
		//return combined;
		return VertexBufferView(nullptr, 0);
	}


	/************************************************************************************************/


	VertexBufferView& VertexBufferView::operator = ( const VertexBufferView& RHS )
	{
		if( &RHS != this )
		{
			mBufferFormat      = RHS.GetBufferFormat();
			mBufferType        = RHS.GetBufferType();
			mBufferElementSize = RHS.GetElementSize();
			mBufferinError     = false;
			memcpy( &mBuffer[0], RHS.GetBuffer(), mBufferUsed );
		}
		return *this;
	}


	/************************************************************************************************/


	void VertexBufferView::Begin( VERTEXBUFFER_TYPE Type, VERTEXBUFFER_FORMAT Format )
	{
		mBufferinError         = false;
		mBufferType            = Type;
		mBufferFormat          = Format;
		mBufferUsed			   = 0;
		mBufferElementSize     = static_cast<uint32_t>( Format );
	}


	/************************************************************************************************/


	bool VertexBufferView::End()
	{
		return !mBufferinError;
	}


	/************************************************************************************************/


	bool VertexBufferView::UnloadBuffer()
	{
		mBufferUsed = 0;
		return true;
	}


	/************************************************************************************************/


	byte* VertexBufferView::GetBuffer() const
	{
		return mBuffer;
	}


	/************************************************************************************************/


	size_t VertexBufferView::GetElementSize() const
	{
		return mBufferElementSize;
	}


	/************************************************************************************************/


	size_t VertexBufferView::GetBufferSize() const // Size of elements
	{
		return mBufferSize / static_cast<uint32_t>( mBufferFormat );
	}


	/************************************************************************************************/


	size_t VertexBufferView::GetBufferSizeUsed() const
	{
		return mBufferUsed / static_cast<uint32_t>( mBufferFormat );
	}


	/************************************************************************************************/


	size_t VertexBufferView::GetBufferSizeRaw() const // Size of buffer in bytes
	{
		return mBufferUsed;
	}


	/************************************************************************************************/
	

	VERTEXBUFFER_FORMAT VertexBufferView::GetBufferFormat() const
	{
		return mBufferFormat;
	}
	

	/************************************************************************************************/
	

	VERTEXBUFFER_TYPE VertexBufferView::GetBufferType() const
	{
		return mBufferType;
	}
	

	/************************************************************************************************/


	void VertexBufferView::SetTypeFormatSize(VERTEXBUFFER_TYPE Type, VERTEXBUFFER_FORMAT Format, size_t count)
	{
		mBufferinError         = false;
		mBufferType            = Type;
		mBufferFormat          = Format;
		mBufferElementSize     = static_cast<uint32_t>( Format );
		mBufferUsed			   = mBufferElementSize * count;
	}


	/************************************************************************************************/
	

	FLEXKITAPI VertexBufferView* CreateVertexBufferView( byte* Memory, size_t BufferLength )
	{
		return new (Memory - sizeof( VertexBufferView ) + BufferLength) VertexBufferView( Memory, BufferLength - sizeof(VertexBufferView));
	}
	

	/************************************************************************************************/
	

	DXGI_FORMAT TextureFormat2DXGIFormat(FORMAT_2D F)
	{
		switch (F)
		{
		case FlexKit::FORMAT_2D::R8G8B8A8_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
			break;
		case FlexKit::FORMAT_2D::R32G32B32_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
			break;
		case FlexKit::FORMAT_2D::R32G32B32A32_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
		case FlexKit::FORMAT_2D::R32_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		case FlexKit::FORMAT_2D::D32_FLOAT:
			return DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
		default:
			return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UINT;
			break;
		}
	}

	
	/************************************************************************************************/

	void UpdateResourceByTemp(RenderSystem* RS, ID3D12Resource* Dest, void* Data, size_t SourceSize, size_t ByteSize, D3D12_RESOURCE_STATES EndState)
	{
		UpdateGPUResource(RS, Data, SourceSize, Dest);
		// NOT SURE IF NEEDED, RUNS CORRECTLY WITHOUT FOR THE MOMENT
		//RS->CommandList->ResourceBarrier(1, 
		//		&CD3DX12_RESOURCE_BARRIER::Transition(Dest, D3D12_RESOURCE_STATE_COPY_DEST, EndState, -1,
		//		D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE));
	}

	void UpdateResourceByTemp( RenderSystem* RS,  FrameBufferedResource* Dest, void* Data, size_t SourceSize, size_t ByteSize, D3D12_RESOURCE_STATES EndState)
	{
		Dest->IncrementCounter();
		UpdateGPUResource(RS, Data, SourceSize, Dest->Get());
		// NOT SURE IF NEEDED, RUNS CORRECTLY WITHOUT FOR THE MOMENT
		//RS->CommandList->ResourceBarrier(1, 
		//		&CD3DX12_RESOURCE_BARRIER::Transition(Dest, D3D12_RESOURCE_STATE_COPY_DEST, EndState, -1,
		//		D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE));
	}


	/************************************************************************************************/

	
	void AddTempBuffer(ID3D12Resource* _ptr, RenderSystem* RS)
	{
		auto& TempBuffers = RS->FrameResources[RS->CurrentIndex].TempBuffers;

		if (!TempBuffers)
			TempBuffers = &TempResourceList::Create_Aligned(128, RS->Memory);

		if (TempBuffers->full())
			TempBuffers = ExpandFixedBuffer(RS->Memory, TempBuffers);

		TempBuffers->push_back(_ptr);
	}


	/************************************************************************************************/


	Texture2D CreateTexture2D( RenderSystem* RS, Tex2DDesc* desc_in )
	{	
		D3D12_RESOURCE_DESC   Resource_DESC = CD3DX12_RESOURCE_DESC::Tex2D(TextureFormat2DXGIFormat(desc_in->Format), 
																		   desc_in->Width, desc_in->Height, 1);

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
															&Resource_DESC, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, pCV, IID_PPV_ARGS(&NewResource));

		CheckHR(HR, ASSERTONFAIL("FAILED TO COMMIT MEMORY FOR TEXTURE"));

		Texture2D NewTexture = {NewResource, 
								{desc_in->Height, desc_in->Height}, 
								TextureFormat2DXGIFormat(desc_in->Format)};

		return (NewTexture);
	}


	/************************************************************************************************/


	void UploadTextureSet(RenderSystem* RS, TextureSet* TS, iAllocator* Memory)
	{
		for (size_t I = 0; I < 16; ++I)
		{
			if (TS->TextureGuids[I]) {
				TS->Loaded[I] = true;
				TS->Textures[I] = LoadTextureFromFile(TS->TextureLocations[I].Directory, RS, Memory);
			}
		}
	}


	/************************************************************************************************/


	void ReleaseTextureSet(RenderSystem* RS, TextureSet* TS, iAllocator* Memory)
	{
		for (size_t I = 0; I < 16; ++I)
		{
			if (TS->TextureGuids[I]) {
				TS->Loaded[I] = false;
				TS->Textures[I]->Release();
			}
		}
		Memory->free(TS);
	}


	/************************************************************************************************/


	void UploadResources(RenderSystem* RS)
	{
		ID3D12CommandList* CommandLists[] = {GetCurrentCommandList(RS)};
		GetCurrentCommandList(RS)->Close();
		RS->GraphicsQueue->ExecuteCommandLists(1, CommandLists);
	}


	/************************************************************************************************/


	void CreateVertexBuffer( RenderSystem* RS, VertexBufferView** Buffers, size_t BufferCount, VertexBuffer& DVB_Out )
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
			if (Buffers[itr] && Buffers[itr]->GetBufferSize())
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
					break;
				default:
					break;
		}
#endif 
			
			UpdateResourceByTemp(RS, NewBuffer, Buffers[itr]->GetBuffer(), 
				Buffers[itr]->GetBufferSizeRaw(), 1, 
				D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

			DVB_Out.VertexBuffers[itr].Buffer				= NewBuffer;
			DVB_Out.VertexBuffers[itr].BufferStride			= Buffers[itr]->GetElementSize();
			DVB_Out.VertexBuffers[itr].BufferSizeInBytes	= Buffers[itr]->GetBufferSizeRaw();
			DVB_Out.VertexBuffers[itr].Type					= Buffers[itr]->GetBufferType();
		}
		}
		if (nullptr != Buffers[0x0f] && Buffers[0x0f]->GetBufferType() == VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX)
		{
			// Create the Vertex Buffer
			FK_ASSERT(Buffers[0x0f]->GetBufferSizeRaw());// ERROR BUFFER EMPTY;
			Resource_DESC.Width	= Buffers[0x0f]->GetBufferSizeRaw();

			ID3D12Resource* NewBuffer = nullptr;

			HRESULT HR = RS->pDevice->CreateCommittedResource(
				&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, 
				&Resource_DESC, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, 
				nullptr, IID_PPV_ARGS(&NewBuffer));
			
			if (FAILED(HR))
			{// TODO!
				FK_ASSERT(0);
			}

			UpdateResourceByTemp(RS, NewBuffer, Buffers[15]->GetBuffer(), 
				Buffers[0x0f]->GetBufferSizeRaw(), 1, 
				D3D12_RESOURCE_STATE_INDEX_BUFFER);

			SETDEBUGNAME(NewBuffer, "INDEXBUFFER");

			DVB_Out.VertexBuffers[0x0f].Buffer				= NewBuffer;
			DVB_Out.VertexBuffers[0x0f].BufferSizeInBytes	= Buffers[0x0f]->GetBufferSizeRaw();
			DVB_Out.VertexBuffers[0x0f].BufferStride		= Buffers[0x0f]->GetElementSize();
			DVB_Out.VertexBuffers[0x0f].Type				= Buffers[0x0f]->GetBufferType();
			DVB_Out.MD.IndexBuffer_Index					= 0x0f;
		}
	}
	

	/************************************************************************************************/


	void InitiateGeometryTable(GeometryTable* GT, iAllocator* Memory)
	{
		GT->Memory						= Memory;
		GT->Geometry.Allocator          = Memory;
		GT->ReferenceCounts.Allocator	= Memory;
		GT->Guids.Allocator				= Memory;
		GT->GeometryIDs.Allocator		= Memory;
		GT->Handles.FreeList.Allocator  = Memory;
		GT->Handles.Indexes.Allocator   = Memory;
		GT->Handles.Clear();
	}


	/************************************************************************************************/


	void FreeGeometryTable(GeometryTable* GT)
	{
		GT->Geometry.Release();
		GT->ReferenceCounts.Release();
		GT->Guids.Release();
		GT->GeometryIDs.Release();
		GT->Handles.Release();
	}


	/************************************************************************************************/


	bool IsMeshLoaded(GeometryTable* GT, GUID_t guid)
	{
		bool res = false;
		for (auto Entry : GT->Guids)
		{
			if (Entry == guid){
				res = true;
				break;
			}
		}

		return res;
	}


	/************************************************************************************************/


	void AddRef(GeometryTable* GT, TriMeshHandle TMHandle)
	{
		size_t Index = GT->Handles[TMHandle];

#ifdef _DEBUG
		if (Index != -1)
			GT->ReferenceCounts[Index]++;
#else
		GT->ReferenceCounts[Index]++;
#endif

	}


	/************************************************************************************************/


	void ReleaseMesh(GeometryTable* GT, TriMeshHandle TMHandle)
	{
		// TODO: MAKE ATOMIC
		auto Count = --GT->ReferenceCounts[GT->Handles[TMHandle]];

		if (Count == 0) {
			auto G = GetMesh(GT, TMHandle);

			CleanUpTriMesh(G);
			if (G->Skeleton)
				CleanUpSkeleton(G->Skeleton);

			GT->Memory->free(const_cast<char*>(G->ID));
			GT->Memory->free(G);
		}
	}


	/************************************************************************************************/


	TriMesh* GetMesh(GeometryTable* GT, TriMeshHandle TMHandle){
		return GT->Geometry[GT->Handles[TMHandle]];
	}
	

	/************************************************************************************************/


	Skeleton* GetSkeleton(GeometryTable* GT, TriMeshHandle TMHandle){
		return GetMesh(GT, TMHandle)->Skeleton;
	}


	/************************************************************************************************/


	size_t	GetSkeletonGUID(GeometryTable* GT, TriMeshHandle TMHandle){
		return GetMesh(GT, TMHandle)->SkeletonGUID;
	}


	/************************************************************************************************/


	void SetSkeleton(GeometryTable* GT, TriMeshHandle TMHandle, Skeleton* S){
		GetMesh(GT, TMHandle)->Skeleton = S;
	}


	/************************************************************************************************/


	bool IsSkeletonLoaded(GeometryTable* GT, TriMeshHandle guid){
		return (GetMesh(GT, guid)->Skeleton != nullptr);
	}


	/************************************************************************************************/


	bool IsAnimationsLoaded(GeometryTable* GT, TriMeshHandle RMeshHandle){
		return GetSkeleton(GT, RMeshHandle)->Animations != nullptr;
	}



	/************************************************************************************************/


	Pair<TriMeshHandle, bool>	FindMesh(GeometryTable* GT, GUID_t guid)
	{
		TriMeshHandle HandleOut;
		size_t location		= 0;
		size_t HandleIndex	= 0;
		for (auto Entry : GT->Guids)
		{
			if (Entry == guid) {
				for ( auto index : GT->Handles.Indexes)
				{
					if (index == location) {
						HandleOut = TriMeshHandle(HandleIndex, GT->Handles.mType, 0x04);
						break;
					}
					++HandleIndex;
				}
				break;
			}
			++location;
		}

		return { HandleOut, 0 };
	}
	

	/************************************************************************************************/


	Pair<TriMeshHandle, bool>	FindMesh(GeometryTable* GT, const char* ID)
	{
		TriMeshHandle HandleOut = INVALIDMESHHANDLE;
		size_t location		= 0;
		size_t HandleIndex	= 0;
		for (auto Entry : GT->GeometryIDs)
		{
			if (!strncmp(Entry, ID, 64)) {
				for (auto index : GT->Handles.Indexes)
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


	ConstantBuffer CreateConstantBuffer(RenderSystem* RS, ConstantBuffer_desc* desc)
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
		}

		return NewResource;
	}


	/************************************************************************************************/


	VertexResourceBuffer CreateVertexBufferResource(RenderSystem* RS, size_t ResourceSize)
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
		HEAP_Props.Type                  = D3D12_HEAP_TYPE_DEFAULT;
		HEAP_Props.MemoryPoolPreference  = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask      = 0;
		HEAP_Props.VisibleNodeMask       = 0;

		size_t BufferCount               = RS->BufferCount;
		FrameBufferedResource NewResource;
		NewResource.BufferCount          = BufferCount;

		ID3D12Resource* Resource = nullptr;
		HRESULT HR = RS->pDevice->CreateCommittedResource(
			&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
			&Resource_DESC, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr,
			IID_PPV_ARGS(&Resource));

		return Resource;
	}


	/************************************************************************************************/


	ShaderResourceBuffer CreateShaderResource(RenderSystem* RS, size_t ResourceSize)
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
		}
		return NewResource;
	}
	

	/************************************************************************************************/


	StreamOutBuffer CreateStreamOut(RenderSystem* RS, size_t ResourceSize) {
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
		}
		return NewResource;
	}


	/************************************************************************************************/
	

	bool CreateInputLayout(RenderSystem* RS, VertexBufferView** Buffers, size_t count, Shader* Shader, VertexBuffer* DVB_Out)
	{
#ifdef _DEBUG
		FK_ASSERT(Shader);
		FK_ASSERT(Shader->Blob);
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
	

	void Destroy( VertexBuffer* VertexBuffer )
	{	
		for( auto Buffer : VertexBuffer->VertexBuffers )
			if( Buffer.Buffer )Buffer.Buffer->Release();
	}
	

	/************************************************************************************************/
	

	void Destroy( ConstantBuffer buffer )
	{
		buffer.Release();
	}
	

	/************************************************************************************************/
	

	void Destroy(ID3D11View* View)
	{
		if (View)
			View->Release();
	}
	

	/************************************************************************************************/
	

	void Destroy( Texture2D txt2d )
	{
		if (txt2d)
			txt2d->Release();
	}
	

	/************************************************************************************************/
	

	void Destroy( Shader* shader)
	{
		if( shader->Blob )
			shader->Blob->Release();
	}
	
	/************************************************************************************************/


	void Destroy(SpotLightBuffer* SLB)
	{
		SLB->Resource->Release();
	}


	/************************************************************************************************/
	

	void Destroy(PointLightBuffer* SLB)
	{
		SLB->Resource->Release();
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
		wchar_t WString[256];
		mbstowcs(WString, FileLoc, 128);
		ID3DBlob* NewBlob   = nullptr;
		ID3DBlob* Errors    = nullptr;
		DWORD dwShaderFlags = 0;

#if USING( DEBUGGRAPHICS )
		dwShaderFlags |= D3DCOMPILE_PREFER_FLOW_CONTROL | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_ENABLE_STRICTNESS;
#endif

		HRESULT HR = D3DCompileFromFile(WString, nullptr, nullptr, desc->entry, desc->shaderVersion, dwShaderFlags, 0, &NewBlob, &Errors);
		if (FAILED(HR))	{
			printf((char*)Errors->GetBufferPointer());
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
		strcpy(SDesc.entry, Entry);
		strcpy(SDesc.ID, ID);
		strcpy(SDesc.shaderVersion, ShaderVersion);

		do
		{
			printf("LoadingShader - %s - \n", Entry);
			res = LoadAndCompileShaderFromFile(File, &SDesc, &Shader);
#if USING( EDITSHADERCONTINUE )
			if (!res)
			{
				std::cout << "Failed to Load\n Press Enter to try again\n";
				char str[100];
				std::cin >> str;
			}
#else
			FK_ASSERT(res);
#endif
		} while (!res);
		return Shader;
	}


	/************************************************************************************************/


	void DestroyShader( Shader* releaseme ){
		if(releaseme && releaseme->Blob) releaseme->Blob->Release();
	}


	/************************************************************************************************/


	size_t CalculateNodeBufferSize(size_t BufferSize)
	{
		size_t PerNodeFootPrint = sizeof(LT_Entry) + sizeof(WT_Entry) + sizeof(Node) + sizeof(uint16_t);
		return (BufferSize - sizeof(SceneNodes)) / PerNodeFootPrint;
	}

	
	/************************************************************************************************/


	void InitiateSceneNodeBuffer(SceneNodes* Nodes, byte* pmem, size_t MemSize)
	{
		size_t NodeFootPrint = sizeof(SceneNodes::BOILERPLATE);
		size_t NodeMax = (MemSize - sizeof(SceneNodes)) / NodeFootPrint  - 0x20;

		Nodes->Nodes = (Node*)pmem;
		{
			const size_t aligment = 0x10;
			auto Memory = pmem + NodeMax;
			size_t alignoffset = (size_t)Memory % aligment;
			if(alignoffset)
				Memory += aligment - alignoffset;	// 16 Byte byte Align
			Nodes->LT = (LT_Entry*)(Memory);
			int c = 0; // Debug Point
		}
		{
			const size_t aligment = 0x40;
			char* Memory = (char*)Nodes->LT + NodeMax;
			size_t alignoffset = (size_t)Memory % aligment; // Cache Align
			if(alignoffset)
				Memory += aligment - alignoffset;
			Nodes->WT = (WT_Entry*)(Memory);
			int c = 0; // Debug Point
		}
		{
			Nodes->Flags = (char*)(Nodes->WT + NodeMax);
			for (size_t I = 0; I < NodeMax; ++I)
				Nodes->Flags[I] = SceneNodes::FREE;

			int c = 0; // Debug Point
		}
		{
			Nodes->Indexes = (uint16_t*)(Nodes->Flags + NodeMax);
			for (size_t I = 0; I < NodeMax; ++I)
				Nodes->Indexes[I] = 0xffff;

			int c = 0; // Debug Point
		}

		for (size_t I = 0; I < NodeMax; ++I)
		{
			Nodes->LT[I].R = DirectX::XMQuaternionIdentity();
			Nodes->LT[I].S = DirectX::XMVectorSet(1, 1, 1, 1);
			Nodes->LT[I].T = DirectX::XMVectorSet(0, 0, 0, 0);
		}

		for (size_t I = 0; I < NodeMax; ++I)
			Nodes->WT[I].m4x4 = DirectX::XMMatrixIdentity();

		Nodes->used		= 0;
		Nodes->max		= NodeMax;
	}


	/************************************************************************************************/


	void PushAllChildren(SceneNodes* Nodes, size_t CurrentNode, fixed_vector<size_t>& Out)
	{
		for (size_t I = 1; I < Nodes->used; ++I)
		{
			auto ParentIndex = _SNHandleToIndex(Nodes, Nodes->Nodes[I].Parent);
			if(ParentIndex == CurrentNode)
			{
				Out.push_back(I);
				PushAllChildren(Nodes, I, Out);
			}
		}
	}

	void SwapNodeEntryies(SceneNodes* Nodes, size_t LHS, size_t RHS)
	{
		LT_Entry	Local_Temp = Nodes->LT[LHS];
		WT_Entry	World_Temp = Nodes->WT[LHS];
		Node		Node_Temp  = Nodes->Nodes[LHS];
		char		Flags_Temp = Nodes->Flags[LHS];

		Nodes->LT[LHS]		= Nodes->LT		[RHS];
		Nodes->WT[LHS]		= Nodes->WT		[RHS];
		Nodes->Nodes[LHS]	= Nodes->Nodes	[RHS];
		Nodes->Flags[LHS]	= Nodes->Flags	[RHS];

		Nodes->LT[RHS]		= Local_Temp;
		Nodes->WT[RHS]		= World_Temp;
		Nodes->Nodes[RHS]	= Node_Temp;
		Nodes->Flags[RHS]	= Flags_Temp;
	}

	size_t FindFreeIndex(SceneNodes* Nodes)
	{
		size_t FreeIndex = -1;
		for (size_t I = Nodes->used - 1; I >= 0; ++I)
		{
			if (Nodes->Flags[I] & SceneNodes::FREE) {
				FreeIndex = I;
				break;
			}
		}
		return FreeIndex;
	}

	void SortNodes(SceneNodes* Nodes, StackAllocator* Temp)
	{
		#ifdef _DEBUG
			std::cout << "Node Usage Before\n";
			for (size_t I = 0; I < Nodes->used; ++I)
				if (Nodes->Flags[I] & SceneNodes::FREE)
					std::cout << "Node: " << I << " Unused\n";
				else
					std::cout << "Node: " << I << " Used\n";
		#endif

		// Find Order
		if (Nodes->used > 1)
		{
			size_t NewLength = Nodes->used;
			fixed_vector<size_t>& Out = fixed_vector<size_t>::Create(Nodes->used - 1, Temp);
			for (size_t I = 1; I < Nodes->used; ++I)// First Node Is Always Root
			{
				if (Nodes->Flags[I] & SceneNodes::FREE) {
					size_t II = I + 1;
					for (; II < Nodes->used; ++II)
						if (!(Nodes->Flags[II] & SceneNodes::FREE))
							break;
					
					SwapNodeEntryies(Nodes, I, II);
					Nodes->Indexes[I] = I;
					Nodes->Indexes[II]= II;
					NewLength--;
					int x = 0;
				}
				if(Nodes->Nodes[I].Parent == NodeHandle(-1))
					continue;

				size_t ParentIndex = _SNHandleToIndex(Nodes, Nodes->Nodes[I].Parent);
				if (ParentIndex > I)
				{					
					SwapNodeEntryies(Nodes, ParentIndex, I);
					Nodes->Indexes[ParentIndex] = I;
					Nodes->Indexes[I]			= ParentIndex;
				}
			}
			Nodes->used = NewLength + 1;
#ifdef _DEBUG
			std::cout << "Node Usage After\n";
			for (size_t I = 0; I < Nodes->used; ++I)
				if (Nodes->Flags[I] & SceneNodes::FREE)
					std::cout << "Node: " << I << " Unused\n";
				else
					std::cout << "Node: " << I << " Used\n";
#endif
		}
	}


	/************************************************************************************************/

	// TODO: Search an optional Free List
	NodeHandle GetNewNode(SceneNodes* Nodes )
	{
		if (Nodes->max < Nodes->used)
			FK_ASSERT(0);

		auto HandleIndex	= 0;
		auto NodeIndex		= 0;

		{
			size_t itr = 0;
			size_t end = Nodes->max;
			for (; itr < end; ++itr)
			{
				if (Nodes->Indexes[itr] == 0xffff)
					break;
			}
			NodeIndex = itr;

			itr = 0;
			for (; itr < end; ++itr)
			{
				if (Nodes->Flags[itr] & SceneNodes::FREE) break;
			}

			HandleIndex = itr;
		}

		Nodes->Flags[NodeIndex] = SceneNodes::DIRTY;
		auto node = NodeHandle(HandleIndex);

		Nodes->Indexes[HandleIndex] = NodeIndex;
		Nodes->Nodes[node.INDEX].TH	= node;
		Nodes->used++;

		return node;
	}


	/************************************************************************************************/


	void SwapNodes(SceneNodes* Nodes, NodeHandle lhs, NodeHandle rhs)
	{
		auto lhs_Index = _SNHandleToIndex(Nodes, lhs);
		auto rhs_Index = _SNHandleToIndex(Nodes, rhs);
		SwapNodeEntryies(Nodes, lhs_Index, rhs_Index);

		_SNSetHandleIndex(Nodes, lhs, rhs_Index);
		_SNSetHandleIndex(Nodes, rhs, lhs_Index);
	}

	
	/************************************************************************************************/


	void FreeHandle(SceneNodes* Nodes, NodeHandle handle)
	{
		Nodes->Flags[_SNHandleToIndex(Nodes, handle)] = SceneNodes::FREE;
		Nodes->Nodes[_SNHandleToIndex(Nodes, handle)].Parent = NodeHandle(-1);
		_SNSetHandleIndex(Nodes, handle, -1);
	}

	
	/************************************************************************************************/


	NodeHandle GetParentNode(SceneNodes* Nodes, NodeHandle node )
	{
		return Nodes->Nodes[Nodes->Indexes[node.INDEX]].Parent;
	}


	/************************************************************************************************/


	void SetParentNode(SceneNodes* Nodes, NodeHandle parent, NodeHandle node)
	{
		auto ParentIndex	= _SNHandleToIndex(Nodes, parent);
		auto ChildIndex		= _SNHandleToIndex(Nodes, node);

		if (ChildIndex < ParentIndex)
			SwapNodes(Nodes, parent, node);

		Nodes->Nodes[Nodes->Indexes[node.INDEX]].Parent = parent;
		SetFlag(Nodes, node, SceneNodes::DIRTY);
	}


	/************************************************************************************************/


	float3 GetPositionW(SceneNodes* Nodes, NodeHandle node)
	{
		DirectX::XMMATRIX wt;
		GetWT(Nodes, node, &wt);
		return float3( wt.r[0].m128_f32[3], wt.r[1].m128_f32[3], wt.r[2].m128_f32[3] );
	}


	/************************************************************************************************/


	void SetPositionW(SceneNodes* Nodes, NodeHandle node, float3 in) // Sets Position in World Space
	{
#if 0
		using DirectX::XMQuaternionConjugate;
		using DirectX::XMQuaternionMultiply;
		using DirectX::XMVectorSubtract;

		// Set New Local Position
		LT_Entry Local;
		WT_Entry Parent;

		// Gather
		GetLocal(Nodes, node, &Local);
		GetWT(Nodes, GetParentNode(Nodes, node), &Parent);

		// Calculate
		Local.T = XMVectorSubtract( Parent.World.T, in.pfloats );
		SetLocal(Nodes, node, &Local);
#else
		DirectX::XMMATRIX wt;
		GetWT(Nodes, node, &wt);
		auto tmp =  DirectX::XMMatrixInverse(nullptr, wt) * DirectX::XMMatrixTranslation(in[0], in[1], in[2] );
		float3 lPosition = float3( tmp.r[3].m128_f32[0], tmp.r[3].m128_f32[1], tmp.r[3].m128_f32[2] );

		wt.r[0].m128_f32[3] = in.x;
		wt.r[1].m128_f32[3] = in.y;
		wt.r[2].m128_f32[3] = in.z;

		SetWT(Nodes, node, &wt);
		// Set New Local Position
		LT_Entry Local(GetLocal(Nodes, node));

		Local.T.m128_f32[0] = lPosition[0];
		Local.T.m128_f32[1] = lPosition[1];
		Local.T.m128_f32[2] = lPosition[2];

		SetLocal(Nodes, node, &Local);
		SetFlag(Nodes, node, SceneNodes::DIRTY);
#endif
	}


	/************************************************************************************************/


	void SetPositionL(SceneNodes* Nodes, NodeHandle Node, float3 V)
	{
		auto Local = GetLocal(Nodes, Node);
		Local.T = V;
		SetLocal(Nodes, Node, &Local);
		SetFlag(Nodes, Node, SceneNodes::DIRTY);
	}


	/************************************************************************************************/


	float3 LocalToGlobal(SceneNodes* Nodes, NodeHandle Node, float3 POS)
	{
		float4x4 WT; GetWT(Nodes, Node, &WT);
		return (WT * float4(POS, 1)).xyz();
	}


	/************************************************************************************************/


	LT_Entry GetLocal(SceneNodes* Nodes, NodeHandle Node){ 
		return Nodes->LT[_SNHandleToIndex(Nodes, Node)]; 
	}


	/************************************************************************************************/


	float3 GetLocalScale(SceneNodes* Nodes, NodeHandle Node)
	{
		float3 L_s = GetLocal(Nodes, Node).S;
		return L_s;
	}


	/************************************************************************************************/


	void GetWT(SceneNodes* Nodes, NodeHandle node, DirectX::XMMATRIX* __restrict out)
	{
		auto index		= _SNHandleToIndex(Nodes, node);
#if 0
		auto WorldQRS	= Nodes->WT[index];
		DirectX::XMMATRIX wt = DirectX::XMMatrixTranspose(DirectX::XMMatrixRotationQuaternion(WorldQRS.World.R) * DirectX::XMMatrixTranslationFromVector(WorldQRS.World.T)); // Seperate this
		*out = wt;
#else
		*out = Nodes->WT[index].m4x4;
#endif
	}


	/************************************************************************************************/


	void GetWT(SceneNodes* Nodes, NodeHandle node, float4x4* __restrict out)
	{
		auto index		= _SNHandleToIndex(Nodes, node);
#if 0
		auto WorldQRS	= Nodes->WT[index];
		DirectX::XMMATRIX wt = DirectX::XMMatrixTranspose(DirectX::XMMatrixRotationQuaternion(WorldQRS.World.R) * DirectX::XMMatrixTranslationFromVector(WorldQRS.World.T)); // Seperate this
		*out = wt;
#else
		*out = XMMatrixToFloat4x4(&Nodes->WT[index].m4x4).Transpose();
#endif
	}


	/************************************************************************************************/
	
	
	void GetWT(SceneNodes* Nodes, NodeHandle node, WT_Entry* __restrict out )
	{
		*out = Nodes->WT[_SNHandleToIndex(Nodes, node)];
	}


	/************************************************************************************************/


	void SetWT(SceneNodes* Nodes, NodeHandle node, DirectX::XMMATRIX* __restrict In)
	{
		Nodes->WT[_SNHandleToIndex(Nodes, node)].m4x4 = *In;
		SetFlag(Nodes, node, SceneNodes::DIRTY);
	}


	/************************************************************************************************/


	void SetLocal(SceneNodes* Nodes, NodeHandle node, LT_Entry* __restrict In)
	{
		Nodes->LT[_SNHandleToIndex(Nodes, node)] = *In;
		SetFlag(Nodes, node, SceneNodes::StateFlags::DIRTY);
	}


	/************************************************************************************************/


	bool GetFlag(SceneNodes* Nodes, NodeHandle Node, size_t m)
	{
		auto index = _SNHandleToIndex(Nodes, Node);
		auto F = Nodes->Flags[index];
		return (F & m) != 0;
	}


	/************************************************************************************************/


	void GetOrientation(SceneNodes* Nodes, NodeHandle node, Quaternion* Out)
	{
		DirectX::XMMATRIX WT;
		GetWT(Nodes, node, &WT);

		DirectX::XMVECTOR q = DirectX::XMQuaternionRotationMatrix(WT);

		Out->floats = q;
	}


	/************************************************************************************************/


	void SetOrientation(SceneNodes* Nodes, NodeHandle node, Quaternion& in)
	{
		DirectX::XMMATRIX wt;
		LT_Entry Local(GetLocal(Nodes, node));
		GetWT(Nodes, FlexKit::GetParentNode(Nodes, node), &wt);

		auto tmp2 = FlexKit::Matrix2Quat(FlexKit::XMMatrixToFloat4x4(&DirectX::XMMatrixTranspose(wt)));
		tmp2 = tmp2.Inverse();

		Local.R = DirectX::XMQuaternionMultiply(in, tmp2);
		SetLocal(Nodes, node, &Local);
		SetFlag(Nodes, node, SceneNodes::DIRTY);
	}

	
	/************************************************************************************************/


	void SetOrientationL(SceneNodes* Nodes, NodeHandle node, Quaternion& in)
	{
		LT_Entry Local(GetLocal(Nodes, node));
		Local.R = in;
		SetLocal(Nodes, node, &Local);
		SetFlag(Nodes, node, SceneNodes::DIRTY);
	}


	/************************************************************************************************/


	bool UpdateTransforms(SceneNodes* Nodes)
	{
		using DirectX::XMMatrixIdentity;
		using DirectX::XMMatrixMultiply;
		using DirectX::XMMatrixTranspose;
		using DirectX::XMMatrixTranslationFromVector;
		using DirectX::XMMatrixScalingFromVector;
		using DirectX::XMMatrixRotationQuaternion;

		Nodes->WT[0].SetToIdentity();// Making sure root is Identity 
#if 0
		for (size_t itr = 1; itr < Nodes->used; ++itr)// Skip Root
		{
			if ( Nodes->Flags[itr]												| SceneNodes::DIRTY || 
				 Nodes->Flags[_SNHandleToIndex(Nodes, Nodes->Nodes[itr].Parent)]| SceneNodes::DIRTY )
			{
				Nodes->Flags[itr] = Nodes->Flags[itr] | SceneNodes::DIRTY; // To cause Further children to update
				bool			  EnableScale = Nodes->Nodes[itr].Scaleflag;
				FlexKit::WT_Entry Parent;
				FlexKit::WT_Entry WOut;
				FlexKit::LT_Entry LIn;

				// Gather
				GetWT(Nodes, Nodes->Nodes[itr].Parent, &Parent);

				WOut = Nodes->WT[itr];
				LIn  = Nodes->LT[itr];

				// Calculate
				WOut.World.R = DirectX::XMQuaternionMultiply(LIn.R, Parent.World.R);
				WOut.World.S = DirectX::XMVectorMultiply	(LIn.S, Parent.World.S);

				LIn.T.m128_f32[3] = 0;// Should Always Be Zero
				//if(EnableScale)	WOut.World.T = DirectX::XMVectorMultiply(Parent.World.S, LIn.T);
				WOut.World.T = DirectX::XMQuaternionMultiply(DirectX::XMQuaternionConjugate(Parent.World.R), LIn.T);
				WOut.World.T = DirectX::XMQuaternionMultiply(WOut.World.T, Parent.World.R);
				WOut.World.T = DirectX::XMVectorAdd			(WOut.World.T, Parent.World.T);

				// Write Results
				Nodes->WT[itr] = WOut;
			}
		}
#else
		for (size_t itr = 1; itr < Nodes->used; ++itr)
			if (Nodes->Flags[itr] | SceneNodes::UPDATED && !(Nodes->Flags[itr] | SceneNodes::DIRTY))
				Nodes->Flags[itr] ^= SceneNodes::CLEAR;

		size_t Unused_Nodes = 0;;
		for (size_t itr = 1; itr < Nodes->used; ++itr)
		{
			if (!(Nodes->Flags[itr] & SceneNodes::FREE)  && 
				 (Nodes->Flags[itr] | SceneNodes::DIRTY) ||
				  Nodes->Flags[_SNHandleToIndex(Nodes, Nodes->Nodes[itr].Parent)] | SceneNodes::UPDATED)
			{
				Nodes->Flags[itr] ^= SceneNodes::DIRTY;		
				Nodes->Flags[itr] |= SceneNodes::UPDATED; // Propagates Dirty Status of Panret to Children to update

				DirectX::XMMATRIX LT = XMMatrixIdentity();
				LT_Entry TRS = GetLocal(Nodes, Nodes->Nodes[itr].TH);

				bool sf = (Nodes->Flags[itr] & SceneNodes::StateFlags::SCALE) != 0;
				LT =(	XMMatrixRotationQuaternion(TRS.R) *
						XMMatrixScalingFromVector(sf ? TRS.S : float3(1.0f, 1.0f, 1.0f).pfloats)) *
						XMMatrixTranslationFromVector(TRS.T);

				auto ParentIndex = _SNHandleToIndex(Nodes, Nodes->Nodes[itr].Parent);
				auto PT = Nodes->WT[ParentIndex].m4x4;
				auto WT = XMMatrixTranspose(XMMatrixMultiply(LT, XMMatrixTranspose(PT)));

				auto temp	= Nodes->WT[itr].m4x4;
				Nodes->WT[itr].m4x4 = WT;
			}

			Unused_Nodes += (Nodes->Flags[itr] & SceneNodes::FREE);
		}
#endif

		Nodes->WT[0].SetToIdentity();// Making sure root is Identity 
		return ((float(Unused_Nodes) / float(Nodes->used)) > 0.25f);
	}


	/************************************************************************************************/


	NodeHandle ZeroNode(SceneNodes* Nodes, NodeHandle node)
	{
		FlexKit::LT_Entry LT;
		LT.S = DirectX::XMVectorSet(1, 1, 1, 1);
		LT.R = DirectX::XMQuaternionIdentity();
		LT.T = DirectX::XMVectorZero();
		FlexKit::SetLocal(Nodes, node, &LT);

		DirectX::XMMATRIX WT;
		WT = DirectX::XMMatrixIdentity();
		FlexKit::SetWT(Nodes, node, &WT);

		Nodes->Nodes[_SNHandleToIndex(Nodes, node)].Scaleflag = false;
		return node;
	}


	/************************************************************************************************/


	NodeHandle	GetZeroedNode(SceneNodes* nodes)
	{
		return ZeroNode(nodes, GetNewNode(nodes));
	}


	/************************************************************************************************/


	void SetFlag(SceneNodes* Nodes, NodeHandle node, SceneNodes::StateFlags f)
	{
		auto index = _SNHandleToIndex(Nodes, node);
		Nodes->Flags[index] = Nodes->Flags[index] | f;
	}


	/************************************************************************************************/


	void Scale(SceneNodes* Nodes, NodeHandle node, float3 XYZ)
	{
		LT_Entry Local(GetLocal(Nodes, node));

		Local.S.m128_f32[0] *= XYZ.pfloats.m128_f32[0];
		Local.S.m128_f32[1] *= XYZ.pfloats.m128_f32[1];
		Local.S.m128_f32[2] *= XYZ.pfloats.m128_f32[2];

		SetLocal(Nodes, node, &Local);
	}


	/************************************************************************************************/


	void TranslateLocal(SceneNodes* Nodes, NodeHandle node, float3 XYZ)
	{
		LT_Entry Local(GetLocal(Nodes, node));

		Local.T.m128_f32[0] += XYZ.pfloats.m128_f32[0];
		Local.T.m128_f32[1] += XYZ.pfloats.m128_f32[1];
		Local.T.m128_f32[2] += XYZ.pfloats.m128_f32[2];

		SetLocal(Nodes, node, &Local);
	}


	/************************************************************************************************/


	void TranslateWorld(SceneNodes* Nodes, NodeHandle Node, float3 XYZ)
	{
		WT_Entry WT;
		GetWT(Nodes, GetParentNode(Nodes, Node), &WT);

		auto MI = DirectX::XMMatrixInverse(nullptr, WT.m4x4);
		auto V = DirectX::XMVector4Transform(XYZ.pfloats, MI);
		TranslateLocal(Nodes, Node, float3(V));
	}


	/************************************************************************************************/


	void SceneNodeCtr::Rotate(Quaternion Q)
	{
		LT_Entry Local(GetLocal(SceneNodes, Node));
		DirectX::XMVECTOR R = DirectX::XMQuaternionMultiply(Local.R, Q.floats);
		Local.R = R;

		SetLocal(SceneNodes, Node, &Local);
	}


	/************************************************************************************************/


	void Yaw(FlexKit::SceneNodes* Nodes, NodeHandle Node, float r)
	{
		DirectX::XMVECTOR rot;
		rot.m128_f32[0] = 0;
		rot.m128_f32[1] = std::sin(r / 2);
		rot.m128_f32[2] = 0;
		rot.m128_f32[3] = std::cos(r / 2);

		FlexKit::LT_Entry Local(FlexKit::GetLocal(Nodes, Node));

		Local.R = DirectX::XMQuaternionMultiply(rot, Local.R);

		FlexKit::SetLocal(Nodes, Node, &Local);
	}


	/************************************************************************************************/


	void Pitch(FlexKit::SceneNodes* Nodes, NodeHandle Node, float r)
	{
		DirectX::XMVECTOR rot;
		rot.m128_f32[0] = std::sin(r / 2);;
		rot.m128_f32[1] = 0;
		rot.m128_f32[2] = 0;
		rot.m128_f32[3] = std::cos(r / 2);

		FlexKit::LT_Entry Local(FlexKit::GetLocal(Nodes, Node));

		Local.R = DirectX::XMQuaternionMultiply(rot, Local.R);

		FlexKit::SetLocal(Nodes, Node, &Local);
	}


	/************************************************************************************************/


	void Roll(FlexKit::SceneNodes* Nodes, NodeHandle Node, float r)
	{
		DirectX::XMVECTOR rot;
		rot.m128_f32[0] = 0;
		rot.m128_f32[1] = 0;
		rot.m128_f32[2] = std::sin(r / 2);
		rot.m128_f32[3] = std::cos(r / 2);

		FlexKit::LT_Entry Local(FlexKit::GetLocal(Nodes, Node));

		Local.R = DirectX::XMQuaternionMultiply(rot, Local.R);

		FlexKit::SetLocal(Nodes, Node, &Local);
	}


	/************************************************************************************************/


	void ResetDescHeap(RenderSystem* RS)
	{
		auto FrameResources = GetCurrentFrameResources(RS);
		FrameResources->DescHeap.GPU_HeapPOS = FrameResources->DescHeap.SRVDescHeap->GetGPUDescriptorHandleForHeapStart();
		FrameResources->DescHeap.CPU_HeapPOS = FrameResources->DescHeap.SRVDescHeap->GetCPUDescriptorHandleForHeapStart();
	}


	/************************************************************************************************/


	void ResetRTVHeap(RenderSystem* RS)
	{
		auto FrameResources = GetCurrentFrameResources(RS);
		FrameResources->RTVHeap.GPU_HeapPOS = FrameResources->RTVHeap.RTVDescHeap->GetGPUDescriptorHandleForHeapStart();
		FrameResources->RTVHeap.CPU_HeapPOS = FrameResources->RTVHeap.RTVDescHeap->GetCPUDescriptorHandleForHeapStart();
	}


	/************************************************************************************************/


	DescHeapPOS ReserveDescHeap(RenderSystem* RS, size_t SlotCount)
	{	// Make This Atomic
		auto FrameResources						 = GetCurrentFrameResources(RS);
		auto CPU								 = FrameResources->DescHeap.CPU_HeapPOS;
		auto GPU								 = FrameResources->DescHeap.GPU_HeapPOS;
		FrameResources->DescHeap.CPU_HeapPOS.ptr = FrameResources->DescHeap.CPU_HeapPOS.ptr + RS->DescriptorCBVSRVUAVSize * SlotCount;
		FrameResources->DescHeap.GPU_HeapPOS.ptr = FrameResources->DescHeap.GPU_HeapPOS.ptr + RS->DescriptorCBVSRVUAVSize * SlotCount;

		return { CPU , GPU };
	}


	/************************************************************************************************/


	DescHeapPOS ReserveRTVHeap(RenderSystem* RS, size_t SlotCount)
	{
		auto FrameResources						 = GetCurrentFrameResources(RS);
		auto CPU								 = FrameResources->RTVHeap.CPU_HeapPOS;
		auto GPU								 = FrameResources->RTVHeap.GPU_HeapPOS;
		FrameResources->RTVHeap.CPU_HeapPOS.ptr = FrameResources->RTVHeap.CPU_HeapPOS.ptr + RS->DescriptorRTVSize * SlotCount;
		FrameResources->RTVHeap.GPU_HeapPOS.ptr = FrameResources->RTVHeap.GPU_HeapPOS.ptr + RS->DescriptorRTVSize * SlotCount;

		return { CPU , GPU };
	}


	/************************************************************************************************/

	ID3D12DescriptorHeap* GetCurrentRTVTable(RenderSystem* RS)
	{
		return GetCurrentFrameResources(RS)->RTVHeap.RTVDescHeap;
	}

	ID3D12DescriptorHeap* GetCurrentDescriptorTable(RenderSystem* RS)
	{
		return GetCurrentFrameResources(RS)->DescHeap.SRVDescHeap;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetDescTableCurrentPosition_GPU(RenderSystem* RS)
	{
		return GetCurrentFrameResources(RS)->DescHeap.GPU_HeapPOS;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetRTVTableCurrentPosition_GPU(RenderSystem* RS)
	{
		return GetCurrentFrameResources(RS)->RTVHeap.GPU_HeapPOS;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVTableCurrentPosition_CPU(RenderSystem* RS)
	{
		return GetCurrentFrameResources(RS)->RTVHeap.CPU_HeapPOS;
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


	DescHeapPOS PushCBToDescHeap(RenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, size_t BufferSize)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC CBV_DESC = {};
		CBV_DESC.BufferLocation = Buffer->GetGPUVirtualAddress();
		CBV_DESC.SizeInBytes	= BufferSize;
		RS->pDevice->CreateConstantBufferView(&CBV_DESC, POS);

		return IncrementHeapPOS(POS, RS->DescriptorCBVSRVUAVSize, 1);
	}


	/************************************************************************************************/


	DescHeapPOS PushSRVBufferToDescHeap(RenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, size_t ElementCount, size_t Stride, D3D12_BUFFER_SRV_FLAGS Flags)
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


	DescHeapPOS PushTextureToDescHeap(RenderSystem* RS, Texture2D tex, DescHeapPOS POS)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc = {}; {
			ViewDesc.Format                        = tex.Format;
			ViewDesc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			ViewDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
			ViewDesc.Texture2D.MipLevels           = 1;
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
	
	
	void InitiateShadowMapPass(RenderSystem* RenderSystem, ShadowMapPass* Out)
	{
		Shader VertexShader_Animated;
		Shader VertexShader_Static;

		// Load Shader
		{
			VertexShader_Animated	= LoadShader("VMainVertexPallet_ShadowMapping",	"ShadowMapping", "vs_5_0", "assets\\vshader.hlsl");
			VertexShader_Static		= LoadShader("VMain_ShadowMapping",				"ShadowMapping", "vs_5_0", "assets\\vshader.hlsl");
		}

		// Create Pipelines State
		{
			D3D12_RASTERIZER_DESC			Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
				D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
				Depth_Desc.DepthFunc					= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
				Depth_Desc.DepthEnable					= true;


			D3D12_INPUT_ELEMENT_DESC InputElements[5] = {
				{ "POSITION",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "WEIGHTS",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,	3, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "WEIGHTINDICES",	0, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT,  4, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {};{
				PSO_Desc.pRootSignature			= RenderSystem->Library.RS4CBVs4SRVs;
				PSO_Desc.VS						= VertexShader_Static;
				PSO_Desc.RasterizerState		= Rast_Desc;
				PSO_Desc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
				PSO_Desc.SampleMask				= UINT_MAX;
				PSO_Desc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
				PSO_Desc.NumRenderTargets		= 0;
				PSO_Desc.SampleDesc.Count		= 1;
				PSO_Desc.SampleDesc.Quality		= 0;
				PSO_Desc.DSVFormat				= DXGI_FORMAT_D32_FLOAT;
				PSO_Desc.InputLayout			={ InputElements, 1 };
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
	}


	void CleanUpShadowPass(ShadowMapPass* Out)
	{
		Out->PSO_Animated->Release();
		Out->PSO_Static->Release();
	}


	/************************************************************************************************/


	void InitiateDeferredPass(RenderSystem* RS, DeferredPassDesc* GBdesc, DeferredPass* out)
	{

		{
			// Create GBuffers
			for(size_t I = 0; I < 3; ++I)
			{
				Tex2DDesc  desc;
				desc.Height = GBdesc->RenderWindow->WH[1];
				desc.Width  = GBdesc->RenderWindow->WH[0];
				desc.Read         = false;
				desc.Write        = false;
				desc.CV           = true;
				desc.RenderTarget = true;
				desc.MipLevels    = 1;

				{
					{	// Alebdo Buffer
						desc.Format = FORMAT_2D::R8G8B8A8_UNORM;
						out->GBuffers[I].ColorTex = CreateTexture2D(RS, &desc);
						FK_ASSERT(out->GBuffers[I].ColorTex);
						SETDEBUGNAME(out->GBuffers[I].ColorTex.Texture, "Albedo Buffer");
					}
					{
						// Create Specular Buffer
						desc.Format = FORMAT_2D::R8G8B8A8_UNORM;
						out->GBuffers[I].SpecularTex = CreateTexture2D(RS, &desc);
						FK_ASSERT(out->GBuffers[I].SpecularTex);
						SETDEBUGNAME(out->GBuffers[I].SpecularTex.Texture, "Specular Buffer");
					}
					{
						// Create Normal Buffer
						desc.Format = FORMAT_2D::R32G32B32A32_FLOAT;
						out->GBuffers[I].NormalTex = CreateTexture2D(RS, &desc);
						FK_ASSERT(out->GBuffers[I].NormalTex);
						SETDEBUGNAME(out->GBuffers[I].NormalTex.Texture, "Normal Buffer");
					}
					{
						desc.Format = FORMAT_2D::R32G32B32A32_FLOAT;
						out->GBuffers[I].PositionTex = CreateTexture2D(RS, &desc);
						FK_ASSERT(out->GBuffers[I].PositionTex);
						SETDEBUGNAME(out->GBuffers[I].PositionTex.Texture, "WorldCord Texture");
					}
					{ // Create Output Byffer
						desc.Format			= FlexKit::FORMAT_2D::R8G8B8A8_UNORM;
						desc.UAV			= true;
						desc.CV				= true;
						desc.RenderTarget	= true;

						out->GBuffers[I].OutputBuffer = CreateTexture2D(RS, &desc);
						FK_ASSERT(out->GBuffers[I].OutputBuffer);
						SETDEBUGNAME(out->GBuffers[I].OutputBuffer.Texture, "Output Buffer");
					}
				}
			}

			out->Shading.Shade = LoadShader("cmain", "DeferredShader", "cs_5_0", "assets\\cshader.hlsl");

			// Create Constant Buffers
			{
				FlexKit::GBufferConstantsLayout initial;
				initial.DLightCount = 0;
				initial.PLightCount = 0;
				initial.SLightCount = 0;
				initial.Width	    = GBdesc->RenderWindow->WH[0];
				initial.Height	    = GBdesc->RenderWindow->WH[1];
				initial.SLightCount = 0;

				FlexKit::ConstantBuffer_desc CB_Desc;
				CB_Desc.InitialSize		= sizeof(initial);
				CB_Desc.pInital			= &initial;
				CB_Desc.Structured		= false;
				CB_Desc.StructureSize	= 0;

				auto NewConstantBuffer		 = CreateConstantBuffer(RS, &CB_Desc);
				out->Shading.ShaderConstants = NewConstantBuffer;
				NewConstantBuffer._SetDebugName("GBufferPass Constants");
			}

			D3D12_DESCRIPTOR_HEAP_DESC	Heap_Desc;
			Heap_Desc.Flags				= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			Heap_Desc.NodeMask			= 0;
			Heap_Desc.NumDescriptors	= 20;
			Heap_Desc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			
			D3D12_SHADER_RESOURCE_VIEW_DESC SRV_DESC;
			SRV_DESC.Format                        = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
			SRV_DESC.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			SRV_DESC.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
			SRV_DESC.Texture2D.MipLevels           = 1;
			SRV_DESC.Texture2D.MostDetailedMip     = 0;
			SRV_DESC.Texture2D.PlaneSlice          = 0;
			SRV_DESC.Texture2D.ResourceMinLODClamp = 0;

			D3D12_UNORDERED_ACCESS_VIEW_DESC	UAV_DESC = {};
			UAV_DESC.Format               = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
			UAV_DESC.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
			UAV_DESC.Texture2D.MipSlice   = 0;
			UAV_DESC.Texture2D.PlaneSlice = 0;

			for (size_t I = 0; I < 3; ++I)
				out->GBuffers[I].DepthBuffer = GBdesc->DepthBuffer->View[I];

			// Shading Signature Setup
			CD3DX12_DESCRIPTOR_RANGE ranges[3];
			{
				ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 0);
				ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
				ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2, 0);
			}

			CD3DX12_ROOT_PARAMETER Parameters[1];
			Parameters[DSRP_DescriptorTable].InitAsDescriptorTable(3, ranges, D3D12_SHADER_VISIBILITY_ALL);

			ID3DBlob* Signature = nullptr;
			ID3DBlob* ErrorBlob = nullptr;
			CD3DX12_STATIC_SAMPLER_DESC Default(0);
			CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;

			RootSignatureDesc.Init(1, Parameters, 1, &Default);
			HRESULT HR = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &Signature, &ErrorBlob);
			FK_ASSERT(SUCCEEDED(HR));

			ID3D12RootSignature* RootSig = nullptr;
			HR = RS->pDevice->CreateRootSignature(0, Signature->GetBufferPointer(),
			Signature->GetBufferSize(), IID_PPV_ARGS(&RootSig));

			FK_ASSERT(SUCCEEDED(HR));
			Signature->Release();

			D3D12_COMPUTE_PIPELINE_STATE_DESC CPSODesc{};
			ID3D12PipelineState* ShadingPSO = nullptr;

			CPSODesc.pRootSignature		= RootSig;
			CPSODesc.CS					= CD3DX12_SHADER_BYTECODE(out->Shading.Shade.Blob);
			CPSODesc.Flags				= D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;

			HR = RS->pDevice->CreateComputePipelineState(&CPSODesc, IID_PPV_ARGS(&ShadingPSO));
			FK_ASSERT(SUCCEEDED(HR));

			out->Shading.ShadingRTSig = RootSig;
			out->Shading.ShadingPSO   = ShadingPSO;
		}
		// GBuffer Fill Signature Setup
		{
			// LoadShaders
			out->Filling.NormalMesh		= LoadShader( "V2Main",				"V2Main",				"vs_5_0", "assets\\vshader.hlsl" );
			out->Filling.AnimatedMesh	= LoadShader( "VMainVertexPallet",	"VMainVertexPallet",	"vs_5_0", "assets\\vshader.hlsl");
			out->Filling.NoTexture		= LoadShader( "PMain",				"PShader",				"ps_5_0", "assets\\pshader.hlsl");
			out->Filling.Textured		= LoadShader( "PMain_TEXTURED",		"PMain_TEXTURED",		"ps_5_0", "assets\\pshader.hlsl");
	
			// Setup Pipeline State
			{
				CD3DX12_DESCRIPTOR_RANGE ranges[1];	{
					ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 0);
				}

				CD3DX12_ROOT_PARAMETER Parameters[DFRP_COUNT];{
					Parameters[DFRP_CameraConstants].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
					Parameters[DFRP_ShadingConstants].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
					Parameters[DFRP_AnimationResources].InitAsShaderResourceView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
					Parameters[DFRP_Textures].InitAsDescriptorTable(1, ranges, D3D12_SHADER_VISIBILITY_PIXEL);
				}

				ID3DBlob* SignatureDescBlob                    = nullptr;
				ID3DBlob* ErrorBlob		                       = nullptr;
				D3D12_ROOT_SIGNATURE_DESC	 Desc              = {};
				CD3DX12_ROOT_SIGNATURE_DESC  RootSignatureDesc = {};
				CD3DX12_STATIC_SAMPLER_DESC	 Default(0);

				RootSignatureDesc.Init(RootSignatureDesc, DFRP_COUNT, Parameters, 1, &Default, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
				HRESULT HR = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &SignatureDescBlob, &ErrorBlob);

				FK_ASSERT(SUCCEEDED(HR));

				ID3D12RootSignature* RootSig = nullptr;
				HR = RS->pDevice->CreateRootSignature(0, SignatureDescBlob->GetBufferPointer(),
													  SignatureDescBlob->GetBufferSize(), IID_PPV_ARGS(&RootSig));

				SignatureDescBlob->Release();

				FK_ASSERT(SUCCEEDED(HR));
				out->Filling.FillRTSig = RootSig;

				D3D12_INPUT_ELEMENT_DESC InputElements[5] = {	
					{"POSITION",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
					{"TEXCOORD",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	    1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
					{"NORMAL",			0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
					{"WEIGHTS",			0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,	3, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
					{"WEIGHTINDICES",	0, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT,  4, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				};

				D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
				D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
				Depth_Desc.DepthFunc					= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
				Depth_Desc.DepthEnable					= true;

				D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {};{
					PSO_Desc.pRootSignature			= RootSig;
					PSO_Desc.VS						= out->Filling.NormalMesh;
					PSO_Desc.PS						= out->Filling.NoTexture;
					PSO_Desc.RasterizerState		= Rast_Desc;
					PSO_Desc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
					PSO_Desc.SampleMask				= UINT_MAX;
					PSO_Desc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
					PSO_Desc.NumRenderTargets		= 4;
					PSO_Desc.RTVFormats[0]			= DXGI_FORMAT_R8G8B8A8_UNORM;
					PSO_Desc.RTVFormats[1]			= DXGI_FORMAT_R8G8B8A8_UNORM;
					PSO_Desc.RTVFormats[2]			= DXGI_FORMAT_R32G32B32A32_FLOAT;
					PSO_Desc.RTVFormats[3]			= DXGI_FORMAT_R32G32B32A32_FLOAT;
					PSO_Desc.SampleDesc.Count		= 1;
					PSO_Desc.SampleDesc.Quality		= 0;
					PSO_Desc.DSVFormat				= DXGI_FORMAT_D32_FLOAT;
					PSO_Desc.InputLayout			={ InputElements, 3 };
					PSO_Desc.DepthStencilState		= Depth_Desc;
				}

				ID3D12PipelineState* PSO = nullptr;
				HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
				FK_ASSERT(SUCCEEDED(HR));
				out->Filling.PSO = PSO;

				PSO_Desc.PS = { (BYTE*)out->Filling.Textured.Blob->GetBufferPointer(), out->Filling.Textured.Blob->GetBufferSize() };
				HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
				FK_ASSERT(SUCCEEDED(HR));
				out->Filling.PSOTextured = PSO;

				PSO_Desc.VS = { (BYTE*)out->Filling.AnimatedMesh.Blob->GetBufferPointer(), out->Filling.AnimatedMesh.Blob->GetBufferSize() };
				PSO_Desc.PS = { (BYTE*)out->Filling.NoTexture.Blob->GetBufferPointer(), out->Filling.NoTexture.Blob->GetBufferSize() };
				PSO_Desc.InputLayout = { InputElements, 5 };
				HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
				FK_ASSERT(SUCCEEDED(HR));
				out->Filling.PSOAnimated = PSO;

				PSO_Desc.PS = { (BYTE*)out->Filling.Textured.Blob->GetBufferPointer(), out->Filling.Textured.Blob->GetBufferSize() };
				PSO_Desc.InputLayout = { InputElements, 5 };
				HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
				FK_ASSERT(SUCCEEDED(HR));
				out->Filling.PSOAnimatedTextured = PSO;
			}
		}
	}


	/************************************************************************************************/

	void IncrementRSIndex(RenderSystem* RS){ RS->CurrentIndex = (RS->CurrentIndex + 1) % 3;}

	void BeginPass(RenderSystem* RS, RenderWindow* Window)
	{
		IncrementRSIndex(RS);
		size_t Index = RS->CurrentIndex;
		WaitforGPU(RS);

		HRESULT HR;
		HR = GetCurrentCommandAllocator(RS)->Reset();									FK_ASSERT(SUCCEEDED(HR));
		HR = GetCurrentCommandList(RS)->Reset(GetCurrentCommandAllocator(RS), nullptr);	FK_ASSERT(SUCCEEDED(HR));

		ResetRTVHeap(RS);
		ResetDescHeap(RS);

		GetCurrentCommandList(RS)->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetBackBufferResource(Window),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	}


	/************************************************************************************************/


	void EndPass(ID3D12GraphicsCommandList* CL, RenderSystem* RS, RenderWindow* Window)
	{
		CD3DX12_RESOURCE_BARRIER Barrier[] = {
			CD3DX12_RESOURCE_BARRIER::Transition(GetRenderTarget(Window),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT, -1,
			D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE) };

		GetCurrentCommandList(RS)->ResourceBarrier(1, Barrier);

		HRESULT HR;
		HR = CL->Close();	FK_ASSERT(SUCCEEDED(HR));

		ID3D12CommandList* CommandLists[10];
		CommandLists[0] = CL;

		RS->GraphicsQueue->ExecuteCommandLists(1, CommandLists);
	}


	/************************************************************************************************/


	void ClearBackBuffer(RenderSystem* RS, ID3D12GraphicsCommandList* CL, RenderWindow* RW, float4 ClearColor){
		CL->ClearRenderTargetView(GetCurrentBackBufferHandle(&RS->DefaultDescriptorHeaps, RW), ClearColor, 0, nullptr);
	}


	/************************************************************************************************/


	void ClearDepthBuffer(RenderSystem* RS, ID3D12GraphicsCommandList* CL, DepthBuffer* DB, float ClearValue, int Stencil, size_t Index){
		CL->ClearDepthStencilView(DB->View[Index],
			D3D12_CLEAR_FLAG_DEPTH, ClearValue, Stencil, 0, nullptr);
	}


	/************************************************************************************************/


	ID3D12CommandAllocator*		GetCurrentCommandAllocator(RenderSystem* RS){
		return RS->FrameResources[RS->CurrentIndex].GraphicsCLAllocator[0]; 
	}

	ID3D12GraphicsCommandList*	GetCurrentCommandList(RenderSystem* RS){
		return RS->FrameResources[RS->CurrentIndex].CommandLists[0];
	}

	PerFrameResources*			GetCurrentFrameResources(RenderSystem* RS){
		return RS->FrameResources + RS->CurrentIndex;
	}


	/************************************************************************************************/


	void WaitforGPU(RenderSystem* RS)
	{
		size_t Index = RS->CurrentIndex;
		auto Fence = RS->Fence;
		size_t CompleteValue = Fence->GetCompletedValue();
		
		if (RS->Fences[Index].FenceValue != 0 && CompleteValue < RS->Fences[Index].FenceValue){
			HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
			Fence->SetEventOnCompletion(RS->Fences[Index].FenceValue, eventHandle);
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
			ReleaseTempResources(RS);
		}
	}

	
	/************************************************************************************************/


	void UploadDeferredPassConstants(RenderSystem* RS, DeferredPass_Parameters* in, float4 A, DeferredPass* Pass)
	{
		GBufferConstantsLayout Update = {};
		Update.PLightCount  = in->PointLightCount;
		Update.SLightCount  = in->SpotLightCount;
		Update.AmbientLight = A;

		UpdateResourceByTemp(RS, &Pass->Shading.ShaderConstants, &Update, sizeof(Update), 1, 
							D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	}


	/************************************************************************************************/

	void
	ShadeDeferredPass(
		PVS* _PVS, DeferredPass* Pass, Texture2D Target,
		RenderSystem* RS, const Camera* C,
		const PointLightBuffer* PLB, const SpotLightBuffer* SPLB)
	{
		auto CL = GetCurrentCommandList(RS);
		auto FrameResources  = GetCurrentFrameResources(RS);
		auto BufferIndex	 = Pass->CurrentBuffer;
		auto& CurrentGBuffer = Pass->GBuffers[BufferIndex];
		Pass->CurrentBuffer = ++Pass->CurrentBuffer % 3;

		{
			CD3DX12_RESOURCE_BARRIER Barrier1[] = {
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.ColorTex,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.SpecularTex,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.NormalTex,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.PositionTex,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.OutputBuffer,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE) };

			CL->ResourceBarrier(4, Barrier1);
		}

		auto DescTable = GetDescTableCurrentPosition_GPU(RS);
		auto TablePOS  = ReserveDescHeap(RS, 8);
		// The Max is to quiet a error if a no Lights are passed
		TablePOS = PushSRVBufferToDescHeap	(RS, PLB->Resource, TablePOS,  max(PLB->size(), 1),  PLB_Stride);	
		TablePOS = PushSRVBufferToDescHeap	(RS, SPLB->Resource, TablePOS, max(SPLB->size(), 1), SPLB_Stride);

		TablePOS = PushTextureToDescHeap	(RS, CurrentGBuffer.ColorTex,		TablePOS);
		TablePOS = PushTextureToDescHeap	(RS, CurrentGBuffer.SpecularTex,	TablePOS);
		TablePOS = PushTextureToDescHeap	(RS, CurrentGBuffer.NormalTex,		TablePOS);
		TablePOS = PushTextureToDescHeap	(RS, CurrentGBuffer.PositionTex,	TablePOS);
		TablePOS = PushUAV2DToDescHeap		(RS, CurrentGBuffer.OutputBuffer,	TablePOS);

		TablePOS = PushCBToDescHeap			(RS, Pass->Shading.ShaderConstants.Get(), TablePOS, CALCULATECONSTANTBUFFERSIZE(GBufferConstantsLayout));
		TablePOS = PushCBToDescHeap			(RS, C->Buffer.Get(), TablePOS,						CALCULATECONSTANTBUFFERSIZE(GBufferConstantsLayout));

		ID3D12DescriptorHeap* Heaps[] = { FrameResources->DescHeap.SRVDescHeap };
		CL->SetDescriptorHeaps(1, Heaps);

		// Do Shading Here
		CL->SetPipelineState(Pass->Shading.ShadingPSO);
		CL->SetComputeRootSignature(Pass->Shading.ShadingRTSig);
		CL->SetComputeRootDescriptorTable(DSRP_DescriptorTable, DescTable);
		CL->Dispatch(Target.WH[0] / 20, Target.WH[1] / 20, 1);

		{
			CD3DX12_RESOURCE_BARRIER Barrier2[] = {
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.ColorTex,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.SpecularTex,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.NormalTex,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET, -1, D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.PositionTex,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.OutputBuffer,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_COPY_SOURCE, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
				CD3DX12_RESOURCE_BARRIER::Transition(Target,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_COPY_DEST,	-1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE) };

			CL->ResourceBarrier(6, Barrier2);
		}

		CL->CopyResource(Target, CurrentGBuffer.OutputBuffer);

		{
			CD3DX12_RESOURCE_BARRIER Barrier3[] = {
				CD3DX12_RESOURCE_BARRIER::Transition(CurrentGBuffer.OutputBuffer,
				D3D12_RESOURCE_STATE_COPY_SOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
				CD3DX12_RESOURCE_BARRIER::Transition(Target,
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_RENDER_TARGET, -1,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE) };

			CL->ResourceBarrier(2, Barrier3);
		}

	}

	void 
	DoDeferredPass(
		PVS* _PVS,	DeferredPass* Pass,	Texture2D Target, 
		RenderSystem* RS,		const Camera* C, 
		TextureManager* TM, 	GeometryTable*	GT)
	{
		auto CL					= GetCurrentCommandList				(RS);
		auto FrameResources		= GetCurrentFrameResources			(RS);
		auto DescPOSGPU			= GetDescTableCurrentPosition_GPU	(RS); // _Ptr to Beginning of Heap On GPU
		auto DescPOS			= ReserveDescHeap					(RS, 6);
		auto DescriptorHeap		= GetCurrentDescriptorTable			(RS);

		auto RTVPOSCPU			= GetRTVTableCurrentPosition_CPU	(RS); // _Ptr to Current POS On RTV heap on CPU
		auto RTVPOS				= ReserveRTVHeap					(RS, 6);
		auto RTVHeap			= GetCurrentRTVTable				(RS);

		CL->SetDescriptorHeaps	(1, &DescriptorHeap);

		size_t BufferIndex		 = Pass->CurrentBuffer;

		{	// Clear Targets
			float4	ClearValue = {0.0f, 0.0f, 0.0f, 0.0f};
			ClearGBuffer(RS, Pass, ClearValue, BufferIndex);
		}

		{	// Setup State
			D3D12_RECT		RECT = D3D12_RECT();
			D3D12_VIEWPORT	VP	 = D3D12_VIEWPORT();

			RECT.right  = (UINT)Target.WH[0];
			RECT.bottom = (UINT)Target.WH[1];
			VP.Height   = (UINT)Target.WH[1];
			VP.Width    = (UINT)Target.WH[0];
			VP.MaxDepth = 1;
			VP.MinDepth = 0;
			VP.TopLeftX = 0;
			VP.TopLeftY = 0;

			D3D12_VIEWPORT	VPs[]	= { VP, VP, VP, VP, };
			D3D12_RECT		RECTs[] = { RECT, RECT, RECT, RECT,};

			{
				RTVPOS = PushRenderTarget(RS, &Pass->GBuffers[BufferIndex].ColorTex,	RTVPOS);
				RTVPOS = PushRenderTarget(RS, &Pass->GBuffers[BufferIndex].SpecularTex, RTVPOS);
				RTVPOS = PushRenderTarget(RS, &Pass->GBuffers[BufferIndex].NormalTex,	RTVPOS);
				RTVPOS = PushRenderTarget(RS, &Pass->GBuffers[BufferIndex].PositionTex,	RTVPOS);
			}

			CL->SetGraphicsRootSignature			(Pass->Filling.FillRTSig);
			CL->SetPipelineState					(Pass->Filling.PSO);
			CL->OMSetRenderTargets					(4, &RTVPOSCPU, true, &Pass->GBuffers[BufferIndex].DepthBuffer);
			CL->SetGraphicsRootConstantBufferView	(0, C->Buffer->GetGPUVirtualAddress());
			CL->RSSetViewports						(4, VPs);
			CL->RSSetScissorRects					(4, RECTs);
			CL->IASetPrimitiveTopology				(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		}
		
		TriMeshHandle	LastHandle	= INVALIDMESHHANDLE;
		TriMesh*		Mesh		= nullptr;
		size_t			IndexCount  = 0;

		auto itr = _PVS->begin();
		auto end = _PVS->end();

		for (;itr != end;++itr)
		{
			Drawable* E = *itr;

			if (E->Posed || E->Textured)
				break;

			TriMeshHandle CurrentHandle = E->MeshHandle;
			if (CurrentHandle == INVALIDMESHHANDLE)
				continue;

			if(CurrentHandle != LastHandle)
			{
				TriMesh* CurrentMesh	= GetMesh(GT, E->MeshHandle);

				IndexCount			= CurrentMesh->IndexCount;
				size_t IBIndex		= CurrentMesh->VertexBuffer.MD.IndexBuffer_Index;

				D3D12_INDEX_BUFFER_VIEW		IndexView;
				IndexView.BufferLocation	= GetBuffer(CurrentMesh, IBIndex)->GetGPUVirtualAddress();
				IndexView.Format			= DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
				IndexView.SizeInBytes		= IndexCount * 32;

				static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;
				FK_ASSERT(AddVertexBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION, CurrentMesh, VBViews));
				FK_ASSERT(AddVertexBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,		 CurrentMesh, VBViews));
				FK_ASSERT(AddVertexBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,	 CurrentMesh, VBViews));
			
				CL->IASetIndexBuffer(&IndexView);
				CL->IASetVertexBuffers(0, VBViews.size(), VBViews.begin() );
				LastHandle = CurrentHandle;
			}

			CL->SetGraphicsRootConstantBufferView(DFRP_ShadingConstants, E->VConstants->GetGPUVirtualAddress());
			CL->DrawIndexedInstanced(IndexCount, 1, 0, 0, 0);
		}

		if(itr != end)
			CL->SetPipelineState(Pass->Filling.PSOAnimated);

		D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
		Desc.ViewDimension	     = D3D12_SRV_DIMENSION_BUFFER;
		Desc.Buffer.FirstElement = 0;
		Desc.Buffer.Flags        = D3D12_BUFFER_SRV_FLAG_NONE;

		for (; itr != end; ++itr)
		{
			Drawable* E = *itr;
			TriMesh* CurrentMesh	= GetMesh(GT, E->MeshHandle);
			auto AnimationState		= E->PoseState;
			
			if (!E->Posed || E->Textured)
				break;
			
			if(!AnimationState || !AnimationState->Resource)
				continue;

			Desc.Buffer.NumElements			= AnimationState->JointCount;
			Desc.Buffer.StructureByteStride = sizeof(float4x4) * 2;

			TriMeshHandle CurrentHandle = E->MeshHandle;
			if (CurrentHandle == INVALIDMESHHANDLE)
				continue;

			if (CurrentHandle != LastHandle)
			{
				size_t IBIndex = CurrentMesh->VertexBuffer.MD.IndexBuffer_Index;
				IndexCount = CurrentMesh->IndexCount;

				D3D12_INDEX_BUFFER_VIEW	IndexView;
				IndexView.BufferLocation = GetBuffer(CurrentMesh, IBIndex)->GetGPUVirtualAddress(),
					IndexView.SizeInBytes = IndexCount * 32;

				IndexView.Format = DXGI_FORMAT::DXGI_FORMAT_R32_UINT;

				static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;
				{	FK_ASSERT(AddVertexBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,	CurrentMesh, VBViews));
					FK_ASSERT(AddVertexBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,			CurrentMesh, VBViews));
					FK_ASSERT(AddVertexBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,		CurrentMesh, VBViews));
					FK_ASSERT(AddVertexBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1,	CurrentMesh, VBViews));
					FK_ASSERT(AddVertexBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2,	CurrentMesh, VBViews));
				}

				CL->IASetIndexBuffer(&IndexView);
				CL->IASetVertexBuffers(0, VBViews.size(), VBViews.begin());
				LastHandle = CurrentHandle;
			}

			CL->SetGraphicsRootConstantBufferView(DFRP_ShadingConstants, E->VConstants->GetGPUVirtualAddress());
			CL->SetGraphicsRootShaderResourceView(DFRP_AnimationResources, AnimationState->Resource->GetGPUVirtualAddress());
			CL->DrawIndexedInstanced(IndexCount, 1, 0, 0, 0);
		}

		if (itr != end)
			CL->SetPipelineState(Pass->Filling.PSOTextured);

		TextureSet*	LastTextureSet = nullptr;

		for (; itr != end; ++itr)
		{
			Drawable* E = *itr;

			if (E->Posed)
				break;

			TriMeshHandle CurrentHandle		= E->MeshHandle;

			if (CurrentHandle == INVALIDMESHHANDLE)
				continue;

			TriMesh*	CurrentMesh			= GetMesh(GT, CurrentHandle);
			TextureSet* CurrentTextureSet	= E->Textures;

			size_t IBIndex	= CurrentMesh->VertexBuffer.MD.IndexBuffer_Index;
			IndexCount		= CurrentMesh->IndexCount;

			
			if (CurrentTextureSet == nullptr)
				continue;

			if(LastTextureSet != CurrentTextureSet)
			{
				LastTextureSet	= CurrentTextureSet;
				auto Textures	= GetDescTableCurrentPosition_GPU(RS);
				auto TablePOS	= ReserveDescHeap(RS, 2);

				TablePOS		= PushTextureToDescHeap(RS, E->Textures->Textures[0], TablePOS);
				TablePOS		= PushTextureToDescHeap(RS, E->Textures->Textures[1], TablePOS);
				CL->SetGraphicsRootDescriptorTable(DFRP_Textures, Textures);
			}

			if(CurrentHandle != LastHandle)
			{
				D3D12_INDEX_BUFFER_VIEW	IndexView;{
					IndexView.BufferLocation = GetBuffer(CurrentMesh, IBIndex)->GetGPUVirtualAddress();
					IndexView.Format         = DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
					IndexView.SizeInBytes    = IndexCount * 32;
				}

				LastHandle = CurrentHandle;
				
				static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;
				FK_ASSERT(AddVertexBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,	CurrentMesh, VBViews));
				FK_ASSERT(AddVertexBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,			CurrentMesh, VBViews));
				FK_ASSERT(AddVertexBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,		CurrentMesh, VBViews));

				CL->SetGraphicsRootConstantBufferView	(DFRP_ShadingConstants, E->VConstants->GetGPUVirtualAddress());
				CL->IASetIndexBuffer(&IndexView);
				CL->IASetVertexBuffers(0, VBViews.size(), VBViews.begin());
			}

			CL->SetGraphicsRootConstantBufferView(DFRP_ShadingConstants, E->VConstants->GetGPUVirtualAddress());
			CL->DrawIndexedInstanced(IndexCount, 1, 0, 0, 0);
		}
	}


	/************************************************************************************************/


	void InitiateForwardPass( FlexKit::RenderSystem* RS, ForwardPass_DESC* FBdesc, ForwardPass* out )
	{
		// Load Shaders
		out->VShader = LoadShader("VMain", "Forward", "vs_5_0", "assets\\ForwardPassVShader.hlsl");
		out->PShader = LoadShader("PMain", "Forward", "ps_5_0", "assets\\ForwardPassPShader.hlsl");

		ID3DBlob* SignatureDescBlob	= nullptr;
		ID3DBlob* ErrorBlob			= nullptr;

		CD3DX12_DESCRIPTOR_RANGE ConstantsBuffer[2];
		CD3DX12_DESCRIPTOR_RANGE ShaderResources[1];
		CD3DX12_ROOT_PARAMETER	 Parameters[5];

		ConstantsBuffer[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
		ConstantsBuffer[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
		ShaderResources[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		Parameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		Parameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		Parameters[2].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
		Parameters[3].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
		Parameters[4].InitAsShaderResourceView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc = CD3DX12_ROOT_SIGNATURE_DESC(CD3DX12_DEFAULT());
		RootSignatureDesc.Init(RootSignatureDesc, 5, Parameters, 0, nullptr, rootSignatureFlags);
		HRESULT HR = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &SignatureDescBlob, &ErrorBlob );

		if (FAILED(HR)){
			printf((char*)ErrorBlob->GetBufferPointer());
			assert(0);
		}

		if (ErrorBlob)
			ErrorBlob->Release();

		ID3DBlob* SignatureBlob = nullptr;
		HR = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &SignatureBlob, &ErrorBlob);

		ID3D12RootSignature* RootSig = nullptr;
		HR = RS->pDevice->CreateRootSignature(0, SignatureDescBlob->GetBufferPointer(), 
												 SignatureDescBlob->GetBufferSize(), IID_PPV_ARGS(&RootSig));

		SignatureDescBlob->Release();
		SignatureBlob->Release();

		ID3D12DescriptorHeap*		CBDescHeap = nullptr;
		D3D12_DESCRIPTOR_HEAP_DESC	CBDescHeapDesc = {};

		CBDescHeapDesc.NumDescriptors = 1;
		CBDescHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		CBDescHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		CBDescHeapDesc.NodeMask       = 0;

		RS->pDevice->CreateDescriptorHeap(&CBDescHeapDesc, IID_PPV_ARGS(&CBDescHeap));
		out->RenderTarget	= FBdesc->OutputTarget;
		out->DepthTarget	= FBdesc->DepthBuffer;
		out->CBDescHeap		= CBDescHeap;
		out->PassRTSig		= RootSig;

		SETDEBUGNAME(CBDescHeap, "ForwardRenderCBHeap");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = 
		{
			{"POSITION", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	  1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"NORMAL",	 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};

		D3D12_RASTERIZER_DESC		Rast_Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); {
		}

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);{
			Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
			//Depth_Desc.DepthEnable	= false;
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {};
		PSO_Desc.pRootSignature                      = RootSig;
		PSO_Desc.VS                                  = out->VShader;
		PSO_Desc.PS                                  = out->PShader;
		PSO_Desc.RasterizerState                     = Rast_Desc;
		PSO_Desc.BlendState		                     = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		PSO_Desc.SampleMask		                     = UINT_MAX;
		PSO_Desc.PrimitiveTopologyType               = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		PSO_Desc.NumRenderTargets	                 = 1;
		PSO_Desc.RTVFormats[0]		                 = out->RenderTarget->Format;
		PSO_Desc.SampleDesc.Count	                 = 1;
		PSO_Desc.SampleDesc.Quality	                 = 0;
		PSO_Desc.DSVFormat			                 = out->DepthTarget->Buffer[out->DepthTarget->CurrentBuffer].Format;
		PSO_Desc.InputLayout		                 = {InputElements, 3};
		PSO_Desc.DepthStencilState					 = Depth_Desc;

		ID3D12PipelineState* PSO = nullptr;
		HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		if (FAILED(HR))
		{	// Failed
			FK_ASSERT(0);
		}

		HR = RS->pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&out->CommandAllocator));							FK_ASSERT(SUCCEEDED(HR));
		HR = RS->pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, out->CommandAllocator, PSO, IID_PPV_ARGS(&out->CommandList));	FK_ASSERT(SUCCEEDED(HR));

		out->CommandList->Close();

		out->PSO = PSO;
	}


	/************************************************************************************************/


	void CleanupForwardPass(ForwardPass* FP)
	{
		FP->CommandList->Release();
		FP->CommandAllocator->Release();
		FP->PSO->Release();
		FP->PassRTSig->Release();
		FP->CBDescHeap->Release();

		FP->VShader.Blob->Release();
		FP->PShader.Blob->Release();
	}


	/************************************************************************************************/


	void DoForwardPass(PVS* _PVS, ForwardPass* Pass, RenderSystem* RS, Camera* C, float4& ClearColor, PointLightBuffer* PLB, GeometryTable* GT)
	{
		auto CL = GetCurrentCommandList(RS);
		if(!_PVS->size())
			return;

		{	// Setup State
			CL->SetPipelineState(Pass->PSO);
			CL->SetGraphicsRootSignature(Pass->PassRTSig);
			CL->OMSetRenderTargets(1, &GetBackBufferView(Pass->RenderTarget), true, &RS->DefaultDescriptorHeaps.DSVDescHeap->GetCPUDescriptorHandleForHeapStart());
			SetWindowRect(CL, Pass->RenderTarget);
		}

		CL->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		CL->SetGraphicsRootShaderResourceView(4, PLB->Resource->GetGPUVirtualAddress());

		for(auto& PV : *_PVS)
		{
			auto E                  = (Drawable*)PV;
			TriMesh* CurrentMesh	= GetMesh(GT, E->MeshHandle);
			size_t IBIndex          =CurrentMesh->VertexBuffer.MD.IndexBuffer_Index;
			size_t IndexCount       =CurrentMesh->IndexCount;

			D3D12_INDEX_BUFFER_VIEW		IndexView;
			IndexView.BufferLocation	= CurrentMesh->VertexBuffer.VertexBuffers[IBIndex].Buffer->GetGPUVirtualAddress();
			IndexView.Format			= DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
			IndexView.SizeInBytes		= IndexCount * 32;

			CL->SetGraphicsRootConstantBufferView(0, C->Buffer->GetGPUVirtualAddress());
			CL->SetGraphicsRootConstantBufferView(1, E->VConstants->GetGPUVirtualAddress()); 
			CL->IASetIndexBuffer(&IndexView);

			static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;
			D3D12_VERTEX_BUFFER_VIEW VBView;
			for (size_t I = 0; I < CurrentMesh->VertexBuffer.VertexBuffers.size(); ++I)
			{
				if (!CurrentMesh->VertexBuffer.VertexBuffers[I].Buffer)
					continue;

				auto& VB = CurrentMesh->VertexBuffer.VertexBuffers[I];
				VBView.BufferLocation	= VB.Buffer->GetGPUVirtualAddress();
				VBView.SizeInBytes		= VB.BufferSizeInBytes;
				VBView.StrideInBytes	= VB.BufferStride;
				VBViews.push_back(VBView);
			}

			CL->IASetVertexBuffers(0, VBViews.size(), VBViews.begin() );
			CL->DrawIndexedInstanced(IndexCount, 1, 0, 0, 0);
		}

		GetCurrentCommandList(RS)->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetBackBufferResource(Pass->RenderTarget),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	}


	/************************************************************************************************/
	
	
	void CleanupDeferredPass(DeferredPass* gb)
	{
		// GBuffer
		for(size_t I = 0; I < 3; ++I)
		{
			gb->GBuffers[I].ColorTex->Release();
			gb->GBuffers[I].SpecularTex->Release();
			gb->GBuffers[I].NormalTex->Release();
			gb->GBuffers[I].PositionTex->Release();
			gb->GBuffers[I].OutputBuffer->Release();
		}

		gb->Shading.ShaderConstants.Release();
		gb->Shading.ShadingPSO->Release();
		gb->Shading.ShadingRTSig->Release();
		gb->Shading.Shade.Blob->Release();

		gb->Filling.PSO->Release();
		gb->Filling.PSOTextured->Release();
		gb->Filling.PSOAnimated->Release();
		gb->Filling.PSOAnimatedTextured->Release();
		gb->Filling.FillRTSig->Release();
		gb->Filling.NormalMesh.Blob->Release();
		gb->Filling.AnimatedMesh.Blob->Release();
		gb->Filling.NoTexture.Blob->Release();
		gb->Filling.Textured.Blob->Release();
	}
	

	/************************************************************************************************/
	

	void ClearGBuffer(RenderSystem* RS, DeferredPass* Pass, const float4& Clear, size_t Index)
	{
		auto CL			= GetCurrentCommandList(RS);
		auto RTVPOSCPU	= GetRTVTableCurrentPosition_CPU(RS); // _Ptr to Current POS On RTV heap on CPU
		auto RTVPOS		= ReserveRTVHeap(RS, 6);
		auto RTVHeap	= GetCurrentRTVTable(RS);

		size_t BufferIndex = Pass->CurrentBuffer;

		auto ClearTarget = [&](DescHeapPOS POS, Texture2D& Texture) {
			RTVPOS = PushRenderTarget(RS, &Texture, POS);
			CL->ClearRenderTargetView(POS, Clear, 0, nullptr);
		};
		
		ClearTarget(RTVPOS, Pass->GBuffers[BufferIndex].ColorTex);
		ClearTarget(RTVPOS, Pass->GBuffers[BufferIndex].SpecularTex);
		ClearTarget(RTVPOS, Pass->GBuffers[BufferIndex].NormalTex);
		ClearTarget(RTVPOS, Pass->GBuffers[BufferIndex].PositionTex);
		ClearTarget(RTVPOS, Pass->GBuffers[BufferIndex].OutputBuffer);
	}
	

	/************************************************************************************************/


	void CleanUpCamera(SceneNodes* Nodes, Camera* camera)
	{
		if(camera->Buffer)
			FlexKit::Destroy(camera->Buffer);

		FlexKit::FreeHandle(Nodes, camera->Node);
	}


	/************************************************************************************************/


	DirectX::XMMATRIX CreatePerspective(Camera* camera)
	{
		DirectX::XMMATRIX InvertPersepective(DirectX::XMMatrixIdentity());
		if (camera->invert)
		{
			InvertPersepective.r[2].m128_f32[2] = -1;
			InvertPersepective.r[2].m128_f32[3] = 1;
			InvertPersepective.r[3].m128_f32[2] = 1;
		}

		DirectX::XMMATRIX proj = InvertPersepective * XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovRH( camera->FOV, camera->AspectRatio, 0.01f, camera->Far ));

		return XMMatrixTranspose(proj);
	}


	/************************************************************************************************/


	void InitiateCamera(RenderSystem* RS, SceneNodes* Nodes, Camera* out, float AspectRatio, float Near, float Far, bool invert)
	{
		FlexKit::NodeHandle Node = FlexKit::GetNewNode(Nodes);
		FlexKit::ZeroNode(Nodes, Node);

		out->Node        = Node;
		out->FOV         = FlexKit::pi / 6;
		out->Near        = Near;
		out->Far         = Far;
		out->AspectRatio = AspectRatio;
		out->invert		 = invert;

		DirectX::XMMATRIX WT;
		FlexKit::GetWT(Nodes, Node, &WT);

		Camera::BufferLayout InitialData;
		InitialData.Proj = CreatePerspective(out);
		InitialData.View = DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, WT));
		InitialData.PV   = DirectX::XMMatrixMultiply(InitialData.Proj, InitialData.View);
		InitialData.WT   = WT;

		FlexKit::ConstantBuffer_desc desc;
		desc.InitialSize   = 2048;
		desc.pInital       = &InitialData;
		desc.Structured    = false;
		desc.StructureSize = 0;

		auto NewBuffer = FlexKit::CreateConstantBuffer( RS, &desc);
		if (!NewBuffer)
			FK_ASSERT(0); // Failed to initialise Constant Buffer

		char* DebugName = "CAMERA BUFFER";
		NewBuffer._SetDebugName(DebugName);

		out->Buffer = NewBuffer;
	}


	/************************************************************************************************/


	void UpdateCamera(RenderSystem* RS, SceneNodes* nodes, Camera* camera, double dt)
	{
		using DirectX::XMMATRIX;
		using DirectX::XMMatrixTranspose;
		
		XMMATRIX View;
		XMMATRIX WT;
		XMMATRIX PV;
		XMMATRIX IV;
		XMMATRIX Proj;

		FlexKit::GetWT(nodes, camera->Node, &WT);
		View = DirectX::XMMatrixInverse(nullptr, WT);

		Proj = CreatePerspective(camera);
		View = XMMatrixTranspose(View);
		PV	 = XMMatrixTranspose(XMMatrixTranspose(Proj)* View);
		IV	 = XMMatrixTranspose(XMMatrixInverse(nullptr, XMMatrixTranspose(Proj)* View));
		WT	 = WT;
		
		camera->WT	 = XMMatrixToFloat4x4(&WT);
		camera->View = XMMatrixToFloat4x4(&View);
		camera->PV	 = XMMatrixToFloat4x4(&PV);
		camera->Proj = XMMatrixToFloat4x4(&Proj);
	}


	/************************************************************************************************/


	void UploadCamera(RenderSystem* RS, SceneNodes* Nodes, Camera* camera, int PointLightCount, int SpotLightCount, double dt)
	{
		using DirectX::XMMATRIX;
		using DirectX::XMMatrixTranspose;
		using DirectX::XMMatrixInverse;

		DirectX::XMMATRIX WT   = Float4x4ToXMMATIRX(&camera->WT);
		DirectX::XMMATRIX View = DirectX::XMMatrixInverse(nullptr, WT);

		Camera::BufferLayout NewData;
		NewData.Proj            = Float4x4ToXMMATIRX(&camera->Proj);
		NewData.View            = Float4x4ToXMMATIRX(&camera->View);
		NewData.PV              = XMMatrixTranspose(XMMatrixTranspose(NewData.Proj) * View);
		NewData.IV              = XMMatrixTranspose(XMMatrixInverse(nullptr, XMMatrixTranspose(NewData.Proj)* View));
		NewData.WT              = XMMatrixTranspose(WT);

		NewData.MinZ            = camera->Near;
		NewData.MaxZ            = camera->Far;

		NewData.WPOS[0]         = WT.r[0].m128_f32[3];
		NewData.WPOS[1]         = WT.r[1].m128_f32[3];
		NewData.WPOS[2]         = WT.r[2].m128_f32[3];
		NewData.WPOS[3]         = 0;
		NewData.PointLightCount = PointLightCount;
		NewData.SpotLightCount  = SpotLightCount;

		UpdateResourceByTemp(RS, &camera->Buffer, &NewData, sizeof(Camera::BufferLayout), 1,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	}


	/************************************************************************************************/


	void UpdatePointLightBuffer( RenderSystem& RS, SceneNodes* nodes, PointLightBuffer* out, iAllocator* TempMemory )
	{
		size_t count = out->Lights->size();
		if (!count)
			return;

		PointLightEntry* PLs = (PointLightEntry*)TempMemory->_aligned_malloc(sizeof(PointLightEntry) * out->Lights->size());

		for ( size_t itr = 0; itr < count; ++itr )
		{
			auto LightEntry = out->Lights->at(itr);
			PLs[itr].POS	= float4( GetPositionW( nodes, LightEntry.Position ), LightEntry.R );
			PLs[itr].Color	= float4( LightEntry.K, LightEntry.I );
		}

		UpdateResourceByTemp(RS, out->Resource, (char*)PLs, sizeof(PointLightEntry) * out->Lights->size() );
	}


	/************************************************************************************************/


	void UpdateSpotLightBuffer( RenderSystem& RS, SceneNodes* nodes, SpotLightBuffer* out, iAllocator* TempMemory )
	{
		size_t count = out->Lights->size();
		if (!count)
			return;

		SpotLightEntry* SLs = (SpotLightEntry*)TempMemory->_aligned_malloc(sizeof(SpotLightEntry) * out->Lights->size());

		for(size_t itr = 0; itr < count; ++itr)
		{
			auto& L = out->Lights->at(itr);
			if (out->Flags->at(itr) | LightBufferFlags::Dirty)
			{
				SLs[itr].POS	= float4(GetPositionW(nodes, L.Position), L.R);
				SLs[itr].Color	= float4(L.K, L.I);

				Quaternion Q;
				GetOrientation(nodes, L.Position, &Q);
				SLs[itr].Direction = Q *  L.Direction;
				itr++;
			}
		}
		UpdateResourceByTemp(RS, out->Resource, (char*)SLs, sizeof(SpotLightEntry) * out->Lights->size());
	}


	/************************************************************************************************/


	void CreatePointLightBuffer( RenderSystem* RS, PointLightBuffer* out, PointLightBufferDesc Desc, iAllocator* Memory )
	{
		FK_ASSERT( Desc.MaxLightCount > 0, "INVALID PARAMS" );

		out->Lights	= &fixed_vector<PointLight>::Create_Aligned(Desc.MaxLightCount, Memory, 0x10);
		out->Flags	= &LightFlags::Create_Aligned(Desc.MaxLightCount, Memory, 0x10);
		out->IDs	= &LightIDs::Create_Aligned(Desc.MaxLightCount, Memory, 0x10);

		D3D12_RESOURCE_DESC Resource_DESC	= CD3DX12_RESOURCE_DESC::Buffer(0);
		Resource_DESC.Alignment				= 0;
		Resource_DESC.DepthOrArraySize		= 1;
		Resource_DESC.Dimension				= D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		Resource_DESC.Layout				= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Resource_DESC.Width					= sizeof(PointLightEntry) * Desc.MaxLightCount;
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

		ID3D12Resource* NewBuffer = nullptr;
		HRESULT HR = RS->pDevice->CreateCommittedResource(&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &Resource_DESC, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&NewBuffer));
		out->Resource = NewBuffer;

		SETDEBUGNAME(NewBuffer, "POINTLIGHTBUFFER");

		for ( size_t itr = 0; itr < Desc.MaxLightCount; itr++ )
			out->IDs->at(itr) = nullptr;
	}


	void CleanUp(PointLightBuffer* out, iAllocator* Memory)
	{
		if(out->Resource) out->Resource->Release();

		Memory->_aligned_free(out->Lights);
		Memory->_aligned_free(out->Flags);
		Memory->_aligned_free(out->IDs);

		out->Resource = nullptr;
		out->Lights   = nullptr;
		out->Flags    = nullptr;
		out->IDs      = nullptr;
	}


	void CleanUp(SpotLightBuffer* out, iAllocator* Memory)
	{
		if(out->Resource) out->Resource->Release();

		Memory->_aligned_free(out->Lights);
		Memory->_aligned_free(out->Flags);
		Memory->_aligned_free(out->IDs);

		out->Resource = nullptr;
		out->Lights   = nullptr;
		out->Flags    = nullptr;
		out->IDs      = nullptr;
	}


	/************************************************************************************************/
	/*
	struct PointLight {
		float3 K;
		float I, R;

		NodeHandle Position;
	};
	*/
	

	LightHandle CreateLight(PointLightBuffer* PL, LightDesc& in)
	{
		return Handle(0);
		auto HandleIndex = PL->Lights->size();
		PL->Lights->push_back({in.K, in.I, in.R, in.Hndl});

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
	

	LightHandle CreateLight(SpotLightBuffer* SL, LightDesc& in, float3 Dir, float p)
	{
		auto HandleIndex = SL->size();
		SL->push_back({in.K, Dir, in.I, in.R, in.Hndl});

		LightHandle Handle;
		Handle.INDEX = HandleIndex;

		SetLightFlag(SL, Handle, LightBufferFlags::Dirty);
		return Handle;
	}


	/************************************************************************************************/


	void CreateSpotLightBuffer(RenderSystem* RS, SpotLightBuffer* out, iAllocator* Memory, size_t Max)
	{
		out->Lights		= &SLList::		Create_Aligned(Max, Memory, 0x10);
		out->Flags		= &LightFlags::	Create_Aligned(Max, Memory, 0x10);
		out->IDs		= &LightIDs::	Create_Aligned(Max, Memory, 0x10);
		
		D3D12_RESOURCE_DESC Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(0);
		Resource_DESC.Alignment           = 0;
		Resource_DESC.DepthOrArraySize    = 1;
		Resource_DESC.Dimension           = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		Resource_DESC.Layout              = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Resource_DESC.Width               = sizeof(SpotLightEntry) * Max;
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
		HRESULT HR = RS->pDevice->CreateCommittedResource(&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &Resource_DESC, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&NewBuffer));
		out->Resource = NewBuffer;

		SETDEBUGNAME(NewBuffer, "SpotLightBuffer");

		for (size_t itr = 0; itr < Max; itr++)
			out->IDs->at(itr) = nullptr;

		for (auto& F : *out->Flags)
			F = LightBufferFlags::Dirty;
	}

	
	/************************************************************************************************/
	
	
	ShaderTable::ShaderTable() 
		: ShaderSetHndls(FlexKit::GetTypeID<ShaderSetHandle>(), nullptr)
		, ShaderHandles(FlexKit::GetTypeID<ShaderHandle>(), nullptr)
	{

	}


	/************************************************************************************************/
	

	float4	ShaderTable::GetAlbedo(ShaderSetHandle hndl)
	{
		return Albedo[ShaderSetHndls[hndl]];
	}

	
	/************************************************************************************************/


	float4	ShaderTable::GetAlbedo(ShaderSetHandle hndl) const
	{
		return Albedo[ShaderSetHndls[hndl]];
	}

	
	/************************************************************************************************/
	

	void	ShaderTable::SetAlbedo(ShaderSetHandle hndl, float4 A)
	{
		Albedo[ShaderSetHndls[hndl]] = A;
	}

	
	/************************************************************************************************/
	

	float4	ShaderTable::GetMetal(ShaderSetHandle hndl)
	{
		return Metal[ShaderSetHndls[hndl]];
	}

	float4	ShaderTable::GetMetal(ShaderSetHandle hndl) const
	{
		return Metal[ShaderSetHndls[hndl]];
	}


	/************************************************************************************************/
	

	void ShaderTable::SetMetal(ShaderSetHandle hndl, float4 M)
	{
		Metal[ShaderSetHndls[hndl]] = M;
	}

	
	/************************************************************************************************/
	
	
	ShaderHandle ShaderTable::AddShader(Shader S)
	{
		size_t index = Shaders.size();
		Shaders.push_back(S);
		auto hndl = ShaderHandles.GetNewHandle();
		ShaderHandles[hndl] = index;
		Shaders[index] = S;
		return hndl;
	}

	
	/************************************************************************************************/
	

	Shader ShaderTable::GetShader(ShaderHandle hndl)
	{
		return Shaders[ShaderHandles[hndl]];
	}

	void ShaderTable::SetShader(ShaderHandle hndl, Shader S)
	{
		Shaders[ShaderHandles[hndl]] = S;
	}
	
	
	/************************************************************************************************/


	void	ShaderTable::FreeShader(ShaderHandle hndl)
	{
		ShaderHandles.RemoveHandle(hndl);
	}


	/************************************************************************************************/
	

	Shader	ShaderTable::GetPixelShader(ShaderSetHandle hndl)
	{
		if (previousShaderSetHandle != hndl)
		{
			previousShaderSetHandle = hndl;
			previousShaderSet = SSetMappings[ShaderSetHndls[hndl]];
		}
		return Shaders[ShaderHandles[SSets[previousShaderSet].PShader]];
	}


	/************************************************************************************************/
	

	Shader	ShaderTable::GetVertexShader(ShaderSetHandle hndl)
	{
		if (previousShaderSetHandle != hndl)
		{
			previousShaderSetHandle = hndl;
			previousShaderSet = SSetMappings[ShaderSetHndls[hndl]];
		}
		return Shaders[ShaderHandles[SSets[previousShaderSet].VShader]];
	}


	/************************************************************************************************/


	Shader ShaderTable::GetVertexShader_Animated( ShaderSetHandle hndl )
	{
		if (previousShaderSetHandle != hndl)
		{
			previousShaderSetHandle = hndl;
			previousShaderSet = SSetMappings[ShaderSetHndls[hndl]];
		}
		return Shaders[ShaderHandles[SSets[previousShaderSet].VShader_Animated]];
	}

	
	/************************************************************************************************/
	
	
	Shader	ShaderTable::GetGeometryShader(ShaderSetHandle hndl)
	{
		if (previousShaderSetHandle != hndl)
		{
			previousShaderSetHandle = hndl;
			previousShaderSet = SSetMappings[ShaderSetHndls[hndl]];
		}
		return Shaders[ShaderHandles[SSets[previousShaderSet].GShader]];
	}


	/************************************************************************************************/
	
	
	size_t	ShaderTable::RegisterShaderSet(ShaderSet& in)
	{
		size_t index = SSets.size();
		SSets.push_back(in);

		return index;
	}


	/************************************************************************************************/


	void ShaderTable::SetSSet(ShaderSetHandle hndl, size_t I)
	{
		SSetMappings[ShaderSetHndls[hndl]] = I;
	}


	/************************************************************************************************/


	ShaderSetHandle	ShaderTable::GetNewShaderSet()
	{
		size_t I = Albedo.size();
		Albedo.push_back({1.0f, 1.0f, 1.0f, 0.8f});
		Metal.push_back({1.0f, 1.0f, 1.0f, 1.0f});
		SSetMappings.push_back(0);
		DirtyFlags.push_back(0);

		ShaderSetHandle newHndl = ShaderSetHndls.GetNewHandle();
		ShaderSetHndls[newHndl] = I;

		return newHndl;
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


	void CreateCubeMesh(RenderSystem* RS, TriMesh* out, StackAllocator* mem, CubeDesc& desc)
	{
		// Change this to be allocated from level Memory
		out->Buffers[0]		= FlexKit::CreateVertexBufferView((byte*)mem->malloc(512), 512);
		out->Buffers[15]	= FlexKit::CreateVertexBufferView((byte*)mem->malloc(512), 512); // Index is always in 0x0f

		out->Buffers[0]->Begin
			( FlexKit::VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION
			, FlexKit::VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32 );

		float l = desc.r;
		// Push Vertices
		{
			auto Buffer = out->Buffers[0];
			static_vector<float3,8> temp =
			{
				{-l,  l, -l },
				{ l,  l, -l }, 
				{ l,  l,  l },  
				{-l,  l,  l },  
				{-l, -l, -l },
				{ l, -l, -l },
				{ l, -l,  l }, 
				{-l, -l,  l }
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
			static_vector<uint32_t, 36> temp =
			{
				0, 1, 3,
				3, 1, 2,
				4, 5, 0,
				0, 5, 1,
				7, 4, 3,
				3, 4, 0,
				5, 6, 1,
				1, 6, 2,
				6, 7, 2,
				2, 7, 3,
				5, 4, 6,
				6, 4, 7
			};
			for (auto I : temp)
				Buffer->Push(I);
			out->IndexCount = temp.size();
		}	out->Buffers[15]->End();

		out->Info.max ={ l, l, l };
		out->Info.min ={ l, l, l };
		FlexKit::CreateVertexBuffer	(RS, out->Buffers, 1, out->VertexBuffer);
		FlexKit::CreateInputLayout	(RS, out->Buffers, 1,&desc.VertexShader, &out->VertexBuffer);
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
		
		TokenList&				TL			= TokenList::Create_Aligned(64000, TempSpace);
		CombinedVertexBuffer&	out_buffer	= CombinedVertexBuffer::Create_Aligned(64000, TempSpace);
		IndexList&				out_indexes	= IndexList::Create_Aligned(64000, TempSpace);

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


	void UpdateDrawable(RenderSystem* RS, SceneNodes* Nodes, Drawable* E)
	{
		if ( E->Dirty || ( E->Visable && E->VConstants && GetFlag(Nodes, E->Node, SceneNodes::UPDATED))){
			DirectX::XMMATRIX WT;
			FlexKit::GetWT( Nodes, E->Node, &WT );

			Drawable::VConsantsLayout	NewData;

			NewData.MP.Albedo	= E->MatProperties.Albedo;
			NewData.MP.Spec		= E->MatProperties.Spec;
			NewData.Transform	= DirectX::XMMatrixTranspose( WT );

			UpdateResourceByTemp(RS, &E->VConstants, &NewData, sizeof(NewData), 1, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
			E->Dirty = false;
		}
	}



	/************************************************************************************************/


	void UpdateDrawables( RenderSystem* RS, SceneNodes* Nodes, PVS* PVS_ ){
		for ( auto v : *PVS_ ) {
			auto E = ( Drawable* )v;
			UpdateDrawable( RS, Nodes, v.V2 );
		}
	}


	/************************************************************************************************/


	void SortPVS( SceneNodes* Nodes, PVS* PVS_, Camera* C)
	{
		if(!PVS_->size())
			return;

		auto CP = FlexKit::GetPositionW( Nodes, C->Node );
		for( auto& v : *PVS_ )
		{
			auto E = ( (Drawable*)v );
			auto P = FlexKit::GetPositionW( Nodes, E->Node );
			SortingField D;
			D.Posed		  = E->Posed;
			D.InvertDepth = false;
			D.Textured	  = E->Textured;
			D.Depth		  = abs( float3( CP - P ).magnitudesquared() * 10000 );
			v.GetByType<size_t>() = *(size_t*)&D;
		}

		std::sort( PVS_->begin().I, PVS_->end().I, []( auto& R, auto& L ) -> bool
		{
			return ( (size_t)R < (size_t)L );
		} );
	}


	/************************************************************************************************/


	void SortPVSTransparent( SceneNodes* Nodes, PVS* PVS_, Camera* C)
	{
		if(!PVS_->size())
			return;

		auto CP = FlexKit::GetPositionW( Nodes, C->Node );
		for( auto& v : *PVS_ )
		{
			auto E = ( (Drawable*)v );
			auto P = FlexKit::GetPositionW( Nodes, E->Node );
			float D = float3( CP - P ).magnitudesquared() * ( E->DrawLast ? -1.0 : 1.0 );
			v.GetByType<size_t>() = D;
		}

		std::sort( PVS_->begin().I, PVS_->end().I, []( auto& R, auto& L ) -> bool
		{
			return ( (size_t)R > (size_t)L );
		} );
	}
	

	/************************************************************************************************/


	void CreateDrawable( RenderSystem* RS, Drawable* e, DrawableDesc& desc )
	{
		DirectX::XMMATRIX WT;
		WT = DirectX::XMMatrixIdentity();

		Drawable::VConsantsLayout	VC;
		VC.Transform = DirectX::XMMatrixTranspose(WT);
		VC.MP.Albedo = {1.0f, 1.0f, 1.0f, 0.75f};
		VC.MP.Spec	 = {1.0f, 1.0f, 1.0f, 0.75f};

		e->MatProperties.Albedo = VC.MP.Albedo;
		e->MatProperties.Spec	= VC.MP.Spec;

		FlexKit::ConstantBuffer_desc CDesc;
		CDesc.InitialSize = 1024;
		CDesc.pInital     = &VC;
		CDesc.Structured  = false;

		e->MeshHandle				  = INVALIDMESHHANDLE;
		e->Occluder                   = INVALIDMESHHANDLE;
		e->Visable                    = true; 
		e->DrawLast					  = false;
		e->Transparent				  = false;
		e->AnimationState			  = nullptr;
		e->PoseState				  = false;
		e->Posed					  = false;
		e->VConstants = FlexKit::CreateConstantBuffer(RS, &CDesc);
	}


	/************************************************************************************************/


	void CleanUpDrawable(Drawable* E)
	{
		if (E)
			Destroy(E->VConstants);

		if (E->PoseState)
			Destroy(E->PoseState);
	}
	

	/************************************************************************************************/


	void CleanUpTriMesh(TriMesh* T)
	{
		FlexKit::Destroy(&T->VertexBuffer);
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


	void Upload(RenderSystem* RS, SceneNodes* nodes, iAllocator* Temp, Camera* C)
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
				FlexKit::GetWT(nodes, NodeHandles[I], &Transforms[I]);
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


	void UpdateGBufferConstants(RenderSystem* RS, DeferredPass* gb, size_t PLightCount, size_t SLightCount)
	{
		FlexKit::GBufferConstantsLayout constants;
		constants.DLightCount = 0;
		constants.PLightCount = PLightCount;
		constants.SLightCount = SLightCount;
		UpdateResourceByTemp(RS, &gb->Shading.ShaderConstants, &constants, sizeof(constants), 1, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	}


	/************************************************************************************************/


	void RenderShadowMap(RenderSystem* RS, PVS* _PVS, SpotLightShadowCaster* Caster, Texture2D* RenderTarget, ShadowMapPass* PSOs, GeometryTable* GT)
	{
		auto CL             = GetCurrentCommandList(RS);
		auto FrameResources = GetCurrentFrameResources(RS);
		auto DescPOSGPU     = GetDescTableCurrentPosition_GPU(RS); // _Ptr to Beginning of Heap On GPU
		auto DescPOS        = ReserveDescHeap(RS, 6);
		auto DescriptorHeap = GetCurrentDescriptorTable(RS);

		auto RTVPOSCPU      = GetRTVTableCurrentPosition_CPU(RS); // _Ptr to Current POS On RTV heap on CPU
		auto RTVPOS         = ReserveRTVHeap(RS, 6);
		auto RTVHeap        = GetCurrentRTVTable(RS);

		CL->SetGraphicsRootSignature(RS->Library.RS4CBVs4SRVs);

		auto itr = _PVS->begin();
		auto end = _PVS->end();

		for (; itr != end; ++itr)
		{
			Drawable* E = *itr;
			if (E->Posed)
				CL->SetPipelineState(PSOs->PSO_Animated);
			else
				CL->SetPipelineState(PSOs->PSO_Static);

			E->VConstants;
		}
	}


	/************************************************************************************************/

	// assumes File str should be at most 256 bytes
	Texture2D LoadTextureFromFile(char* file, RenderSystem* RS, iAllocator* MemoryOut)
	{
		Texture2D tex = {};
		wchar_t	wfile[256];
		size_t	ConvertedSize = 0;
		mbstowcs_s(&ConvertedSize, wfile, file, 256);
		auto RESULT = LoadDDSTexture2DFromFile(file, MemoryOut, RS);

		FK_ASSERT(RESULT, "Failed to Create Texture!");
		DDSTexture2D* Texture = RESULT;
		tex.Texture = Texture->Texture;
		tex.WH		= Texture->WH;
		tex.Format	= Texture->Format;

		MemoryOut->free(Texture);

		return tex;
	}


	/************************************************************************************************/


	void FreeTexture(Texture2D* Tex){
		if(!Tex) return;
		if(Tex->Texture)Tex->Texture->Release();
	}



	/************************************************************************************************/

	float2 WStoSS(float2 XY) {
		return (XY * float2( 2, 2)) + float2( -1, -1);
	}

	void PushRect(GUIRender* RG, Draw_RECT Rect) 
	{
		RG->DrawCalls.push_back({ DRAWCALLTYPE::DCT_2DRECT , 0 });
		Draw_RECTPoint P;
		P.Color = F4MUL(Rect.Color, Rect.Color);

		//
		P.V  = WStoSS({ Rect.TRight.x, Rect.BLeft.y });
		P.UV = { 1, 1 };
		RG->Rects.push_back(P);// Bottom Right
		P.V  = WStoSS(Rect.BLeft);
		P.UV = { 0, 1 };
		RG->Rects.push_back(P);// Bottom Left
		P.V  = WStoSS(Rect.TRight);
		P.UV = { 1, 0 };
		RG->Rects.push_back(P);// Top Right
		//

		//
		P.V  = WStoSS(Rect.TRight);
		P.UV = { 1, 0 };
		RG->Rects.push_back(P);// Top Right
		P.V  = WStoSS(Rect.BLeft);
		P.UV = { 0, 1 };
		RG->Rects.push_back(P);// Bottom Left
		P.V  = WStoSS({ Rect.BLeft.x, Rect.TRight.y });
		P.UV = { 0, 0 };
		RG->Rects.push_back(P);// Top Left
		//
	}


	/************************************************************************************************/


	void PushRect(GUIRender* RG, Draw_RECT_CLIPPED Rect)
	{
		RG->DrawCalls.push_back({ DRAWCALLTYPE::DCT_2DRECT_CLIPPED , RG->ClipAreas.size() });
		RG->ClipAreas.push_back({ Rect.CLIPAREA_BLEFT , Rect.CLIPAREA_TRIGHT });

		Draw_RECTPoint P;
		P.Color = F4MUL(Rect.Color, Rect.Color);

		//
		P.V = WStoSS({ Rect.TRight.x, Rect.BLeft.y });
		P.UV = { 1, 1 };
		RG->Rects.push_back(P);// Bottom Right
		P.V = WStoSS(Rect.BLeft);
		P.UV = { 0, 1 };
		RG->Rects.push_back(P);// Bottom Left
		P.V = WStoSS(Rect.TRight);
		P.UV = { 1, 0 };
		RG->Rects.push_back(P);// Top Right
	    //

		//
		P.V = WStoSS(Rect.TRight);
		P.UV = { 1, 0 };
		RG->Rects.push_back(P);// Top Right
		P.V = WStoSS(Rect.BLeft);
		P.UV = { 0, 1 };
		RG->Rects.push_back(P);// Bottom Left
		P.V = WStoSS({ Rect.BLeft.x, Rect.TRight.y });
		P.UV = { 0, 0 };
		RG->Rects.push_back(P);// Top Left
		//
	}


	/************************************************************************************************/


	void PushRect(GUIRender* RG, Draw_TEXTURED_RECT Rect)
	{
		RG->DrawCalls.push_back		({ DRAWCALLTYPE::DCT_2DRECT_TEXTURED , RG->TexturedRects.size() });
		RG->TexturedRects.push_back	({ Rect.TextureHandle, Rect.BLeft, Rect.TRight });

		Draw_RECTPoint P;
		P.Color = F4MUL(Rect.Color, Rect.Color);

		//
		P.V = WStoSS({ Rect.TRight.x, Rect.BLeft.y });
		P.UV = { 1, 1 };
		RG->Rects.push_back(P);// Bottom Right
		P.V = WStoSS(Rect.BLeft);
		P.UV = { 0, 1 };
		RG->Rects.push_back(P);// Bottom Left
		P.V = WStoSS(Rect.TRight);
		P.UV = { 1, 0 };
		RG->Rects.push_back(P);// Top Right
		//

		//
		P.V = WStoSS(Rect.TRight);
		P.UV = { 1, 0 };
		RG->Rects.push_back(P);// Top Right
		P.V = WStoSS(Rect.BLeft);
		P.UV = { 0, 1 };
		RG->Rects.push_back(P);// Bottom Left
		P.V = WStoSS({ Rect.BLeft.x, Rect.TRight.y });
		P.UV = { 0, 0 };
		RG->Rects.push_back(P);// Top Left
		//
	}


	/************************************************************************************************/


	void PushText(GUIRender* RG, Draw_TEXT Text)
	{
		RG->DrawCalls.push_back({ DRAWCALLTYPE::DCT_Text, RG->Text.size() });
		RG->Text.push_back(Text);
		/*
		Draw_RECTPoint P;
		P.Color = F4MUL(Text.Color, Text.Color);

		//
		P.V = WStoSS({ Text.CLIPAREA_TRIGHT.x, Text.CLIPAREA_BLEFT.y });
		P.UV = { 1, 1 };
		RG->Rects.push_back(P);// Bottom Right
		P.V = WStoSS(Text.CLIPAREA_BLEFT);
		P.UV = { 0, 1 };
		RG->Rects.push_back(P);// Bottom Left
		P.V = WStoSS(Text.CLIPAREA_TRIGHT);
		P.UV = { 1, 0 };
		RG->Rects.push_back(P);// Top Right
		//

		//
		P.V = WStoSS(Text.CLIPAREA_TRIGHT);
		P.UV = { 1, 0 };
		RG->Rects.push_back(P);// Top Right
		P.V = WStoSS(Text.CLIPAREA_BLEFT);
		P.UV = { 0, 1 };
		RG->Rects.push_back(P);// Bottom Left
		P.V = WStoSS({ Text.CLIPAREA_BLEFT.x, Text.CLIPAREA_TRIGHT.y });
		P.UV = { 0, 0 };
		RG->Rects.push_back(P);// Top Left
		//
		*/
	}


	/************************************************************************************************/


	void PushLineSet(GUIRender* RG, Draw_LineSet_3D LineSet)
	{
		RG->DrawCalls.push_back({DCT_LINES3D, RG->DrawLines3D.size()});
		RG->DrawLines3D.push_back(LineSet);
	}


	/************************************************************************************************/


	void InitiateDrawGUI(RenderSystem* RS, GUIRender* RG, iAllocator* Memory) 
	{
		for (size_t I = 0; I < DRAWCALLTYPE::DCT_COUNT; ++I)
			RG->DrawStates[I] = nullptr;

		Shader DrawRectVShader;
		Shader DrawRectPShader;
		Shader DrawRectPSTexturedShader;
		Shader DrawLineVShader;
		Shader DrawLinePShader;

		DrawRectVShader				= LoadShader("DrawRect_VS", "DrawRect_VS", "vs_5_0", "assets\\vshader.hlsl");
		DrawRectPShader				= LoadShader("DrawRect", "DrawRect", "ps_5_0", "assets\\pshader.hlsl");
		DrawRectPSTexturedShader	= LoadShader("DrawRectTextured", "DrawRectTextured", "ps_5_0", "assets\\pshader.hlsl");
		DrawLineVShader				= LoadShader("VSegmentPassthrough", "VSegmentPassthrough", "vs_5_0", "assets\\vshader.hlsl");
		DrawLinePShader				= LoadShader("DrawLine", "DrawLine", "ps_5_0", "assets\\pshader.hlsl");

		{
			// Draw Rect State Objects
			auto RootSig = RS->Library.RS4CBVs4SRVs;

			/*
			typedef struct D3D12_INPUT_ELEMENT_DESC
			{
				LPCSTR						SemanticName;
				UINT						SemanticIndex;
				DXGI_FORMAT					Format;
				UINT						InputSlot;
				UINT						AlignedByteOffset;
				D3D12_INPUT_CLASSIFICATION	InputSlotClass;
				UINT						InstanceDataStepRate;
			} 	D3D12_INPUT_ELEMENT_DESC;
			*/

			D3D12_INPUT_ELEMENT_DESC InputElements[] = {
				{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 0, 8,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			D3D12_RASTERIZER_DESC		Rast_Desc  = CD3DX12_RASTERIZER_DESC	(D3D12_DEFAULT);
			D3D12_DEPTH_STENCIL_DESC	Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC	(D3D12_DEFAULT);
			Depth_Desc.DepthFunc		= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
			Depth_Desc.DepthEnable		= false;

			D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
				PSO_Desc.pRootSignature        = RootSig;
				PSO_Desc.VS                    = DrawRectVShader;
				PSO_Desc.RasterizerState       = Rast_Desc;
				PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
				PSO_Desc.SampleMask            = UINT_MAX;
				PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
				PSO_Desc.NumRenderTargets      = 1;
				PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM;
				PSO_Desc.SampleDesc.Count      = 1;
				PSO_Desc.SampleDesc.Quality    = 0;
				PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
				PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
				PSO_Desc.DepthStencilState     = Depth_Desc;
			}

			ID3D12PipelineState* PSO = nullptr;
			HRESULT HR;
				
			PSO_Desc.PS = DrawRectPShader;
			HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
			FK_ASSERT(SUCCEEDED(HR));
			RG->DrawStates[DCT_2DRECT]			= PSO;
			RG->DrawStates[DCT_2DRECT_CLIPPED]	= PSO; PSO->AddRef();

			PSO_Desc.PS = DrawRectPSTexturedShader;
			HR  = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
			FK_ASSERT(SUCCEEDED(HR));
			RG->DrawStates[DCT_2DRECT_TEXTURED] = PSO;
		}

		{
			// Draw Line State Objects
			/*
			typedef struct D3D12_INPUT_ELEMENT_DESC
			{
				LPCSTR						SemanticName;
				UINT						SemanticIndex;
				DXGI_FORMAT					Format;
				UINT						InputSlot;
				UINT						AlignedByteOffset;
				D3D12_INPUT_CLASSIFICATION	InputSlotClass;
				UINT						InstanceDataStepRate;
			} 	D3D12_INPUT_ELEMENT_DESC;
			*/

			D3D12_INPUT_ELEMENT_DESC InputElements[2] = {
				{ "POSITION",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOUR",			0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
			Depth_Desc.DepthFunc		= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
			Depth_Desc.DepthEnable		= false;

			D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
				PSO_Desc.pRootSignature        = RS->Library.RS4CBVs4SRVs;;
				PSO_Desc.VS                    = DrawLineVShader;
				PSO_Desc.PS                    = DrawLinePShader;
				PSO_Desc.RasterizerState       = Rast_Desc;
				PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
				PSO_Desc.SampleMask            = UINT_MAX;
				PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
				PSO_Desc.NumRenderTargets      = 1;
				PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM;
				PSO_Desc.SampleDesc.Count      = 1;
				PSO_Desc.SampleDesc.Quality    = 0;
				PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
				PSO_Desc.InputLayout           = { InputElements, 2 };
				PSO_Desc.DepthStencilState     = Depth_Desc;
			}

			ID3D12PipelineState* PSO = nullptr;
			HRESULT HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
			FK_ASSERT(SUCCEEDED(HR));
			RG->DrawStates[DCT_LINES3D] = PSO;
		}

		// Create/Load Text Rendering State
		{
			Shader DrawTextVShader = LoadShader("VTextMain", "TextPassThrough", "vs_5_0", "assets\\TextRendering.hlsl");
			Shader DrawTextGShader = LoadShader("GTextMain", "GTextMain",		"gs_5_0", "assets\\TextRendering.hlsl");
			Shader DrawTextPShader = LoadShader("PTextMain", "TextShading",		"ps_5_0", "assets\\TextRendering.hlsl");


			HRESULT HR;
			ID3D12PipelineState* PSO = nullptr;

			/*
			struct TextEntry
			{
				float2 POS;
				float2 Size;
				float2 TopLeftUV;
				float2 BottomRightUV;
				float4 Color;
			};
			*/

			D3D12_INPUT_ELEMENT_DESC InputElements[5] =	{
				{ "POS",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,			0, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{ "WH",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,			0, 8,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{ "UVTL",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,			0, 16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{ "UVBL",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,			0, 24, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{ "COLOR",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,		0, 32, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			};

			D3D12_RASTERIZER_DESC Rast_Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);{
			}

			D3D12_DEPTH_STENCIL_DESC Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);{
				Depth_Desc.DepthEnable	 = false;
				Depth_Desc.StencilEnable = false;
			}

			D3D12_RENDER_TARGET_BLEND_DESC TransparencyBlend_Desc;{
				TransparencyBlend_Desc.BlendEnable           = true;
				TransparencyBlend_Desc.LogicOpEnable         = false;
				TransparencyBlend_Desc.SrcBlend              = D3D12_BLEND_SRC_ALPHA;
				TransparencyBlend_Desc.DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
				TransparencyBlend_Desc.BlendOp               = D3D12_BLEND_OP_ADD;
				TransparencyBlend_Desc.SrcBlendAlpha         = D3D12_BLEND_ZERO;
				TransparencyBlend_Desc.DestBlendAlpha        = D3D12_BLEND_ZERO;
				TransparencyBlend_Desc.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
				TransparencyBlend_Desc.LogicOp               = D3D12_LOGIC_OP_NOOP;
				TransparencyBlend_Desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			}

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSO_Desc = {};{
				PSO_Desc.pRootSignature             = RS->Library.RS4CBVs4SRVs;
				PSO_Desc.VS                         = DrawTextVShader;
				PSO_Desc.GS                         = DrawTextGShader;
				PSO_Desc.PS                         = DrawTextPShader;
				PSO_Desc.RasterizerState            = Rast_Desc;
				PSO_Desc.BlendState                 = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
				PSO_Desc.SampleMask                 = UINT_MAX;
				PSO_Desc.PrimitiveTopologyType      = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
				PSO_Desc.NumRenderTargets           = 1;
				PSO_Desc.RTVFormats[0]              = DXGI_FORMAT_R8G8B8A8_UNORM;
				PSO_Desc.BlendState.RenderTarget[0] = TransparencyBlend_Desc;
				PSO_Desc.SampleDesc.Count           = 1;
				PSO_Desc.SampleDesc.Quality         = 0;
				PSO_Desc.DSVFormat                  = DXGI_FORMAT_D32_FLOAT;
				PSO_Desc.InputLayout                = { InputElements, 5 };
				PSO_Desc.DepthStencilState          = Depth_Desc;
			}

			HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
			FlexKit::CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE PIPELINE STATE OBJECT"));

			Destroy(&DrawTextVShader);
			Destroy(&DrawTextGShader);
			Destroy(&DrawTextPShader);

			RG->DrawStates[DCT_Text] = PSO;
		}

		ConstantBuffer_desc	Desc;
		Desc.InitialSize = KILOBYTE * 32;
		Desc.pInital	 = nullptr;
		Desc.Structured  = false;

		RG->RectBuffer			= CreateConstantBuffer(RS, &Desc);

		RG->Rects.Allocator			= Memory;
		RG->DrawCalls.Allocator		= Memory;
		RG->Text.Allocator			= Memory;
		RG->TexturedRects.Allocator = Memory;
		RG->ClipAreas.Allocator		= Memory;
		RG->DrawLines3D.Allocator	= Memory;

		Destroy(&DrawRectVShader);
		Destroy(&DrawRectPShader);
		Destroy(&DrawRectPSTexturedShader);
		Destroy(&DrawLineVShader);
		Destroy(&DrawLinePShader);
	}


	/************************************************************************************************/


	void DrawGUI(RenderSystem* RS, ID3D12GraphicsCommandList* CL, GUIRender* GUIStack, Texture2D RenderTarget)
	{
		size_t			RectOffset		= 0;
		size_t			TexturePosition = 0;
		DRAWCALLTYPE	PrevState		= DCT_COUNT;

		FINALLY
		{
			GUIStack->ClipAreas.Release();
			GUIStack->Rects.Release();
			GUIStack->TexturedRects.Release();
			GUIStack->Text.Release();
			GUIStack->DrawLines3D.Release();

			GUIStack->DrawCalls.Release();
		}
		FINALLYOVER

		if (!GUIStack->DrawCalls.size())
			return;

		/*
		typedef struct D3D12_VERTEX_BUFFER_VIEW
		{
			D3D12_GPU_VIRTUAL_ADDRESS	BufferLocation;
			UINT						SizeInBytes;
			UINT						StrideInBytes;
		} 	D3D12_VERTEX_BUFFER_VIEW;
		*/
		
		auto Resources		= GetCurrentFrameResources(RS);
		auto RTVHeap		= Resources->RTVHeap.RTVDescHeap;
		auto DescriptorHeap = GetCurrentDescriptorTable(RS);
		auto RTVs			= GetRTVTableCurrentPosition_CPU(RS);
		auto RTVPOS			= ReserveRTVHeap(RS, 1);

		PushRenderTarget(RS, &RenderTarget, RTVPOS);

		D3D12_VERTEX_BUFFER_VIEW View[] = {
			{ GUIStack->RectBuffer->GetGPUVirtualAddress(),			(UINT)(sizeof(Draw_RECT)  * GUIStack->Rects.size()),			(UINT)sizeof(Draw_RECT) },
		};

		D3D12_VIEWPORT VP;
		VP.Width  = RenderTarget.WH[0];
		VP.Height = RenderTarget.WH[1];
		VP.MaxDepth = 1.0f;
		VP.MaxDepth = 0.0f;
		VP.TopLeftX = 0;
		VP.TopLeftY = 0;


		CL->SetGraphicsRootSignature(RS->Library.RS4CBVs4SRVs);
		CL->OMSetRenderTargets(1, &RTVs, true, &RS->DefaultDescriptorHeaps.DSVDescHeap->GetCPUDescriptorHandleForHeapStart());
		CL->RSSetViewports(1, &VP);

		for (auto D : GUIStack->DrawCalls)
		{
			bool StateChanged = true;
			if (PrevState != D.Type) {
				CL->SetPipelineState(GUIStack->DrawStates[D.Type]);
				StateChanged = true;
			}
			else
				StateChanged = false;

			switch (D.Type)
			{
			case DRAWCALLTYPE::DCT_2DRECT_CLIPPED: {
				/*
				LONG left;
				LONG top;
				LONG right;
				LONG bottom;
				*/

				auto ClipArea	= GUIStack->ClipAreas[D.Index];
				auto WH			= RenderTarget.WH;
				
				D3D12_RECT Rects[] = { {
						(LONG)(WH[0] * ClipArea.CLIPAREA_BLEFT[0]),
						(LONG)(WH[1] - WH[1] * ClipArea.CLIPAREA_TRIGHT[1]),
						(LONG)(WH[0] * ClipArea.CLIPAREA_TRIGHT[0]),
						(LONG)(WH[1] - WH[1] * ClipArea.CLIPAREA_BLEFT[1]),
					}
				};

				CL->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				CL->RSSetScissorRects	(1, Rects);
				CL->IASetVertexBuffers	(0, 1, View + 0);
				CL->DrawInstanced		(6, 1, RectOffset, 0);
				RectOffset += 6;
			}	break;
			case DRAWCALLTYPE::DCT_2DRECT:{
				auto WH = RenderTarget.WH;
				D3D12_RECT Rects[] = { {
						0,
						0,
						(LONG)WH[0],
						(LONG)WH[1],
					}
				};

				CL->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				CL->RSSetScissorRects	(1, Rects);
				CL->IASetVertexBuffers	(0, 1, View + 0);
				CL->DrawInstanced		(6, 1, RectOffset, 0);
				RectOffset+= 6;

			}	break;
			case DRAWCALLTYPE::DCT_2DRECT_TEXTURED: {
				auto WH		= RenderTarget.WH;
				auto BLeft	= GUIStack->TexturedRects[D.Index].CLIPAREA_BLEFT;
				auto TRight	= GUIStack->TexturedRects[D.Index].CLIPAREA_TRIGHT;

				D3D12_RECT Rects[] = { {
						(LONG)(WH[0] * BLeft[0]),
						(LONG)(WH[1] - WH[1] * TRight[1]),
						(LONG)(WH[0] * TRight[0]),
						(LONG)(WH[1] - WH[1] * BLeft[1]),
					}
				};

				auto DescPOS  = ReserveDescHeap(RS, 8);
				PushTextureToDescHeap(RS, *GUIStack->TexturedRects[TexturePosition].Texture, DescPOS);

				CL->IASetPrimitiveTopology			(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				CL->SetGraphicsRootDescriptorTable	(0, DescPOS);
				CL->IASetVertexBuffers				(0, 1, View + 0);
				CL->DrawInstanced					(6, 1, RectOffset, 0);
	
				RectOffset+= 6;
				TexturePosition++;
			}	break;
			case DRAWCALLTYPE::DCT_LINES3D: {
				LineSet* LineDrawCall = GUIStack->DrawLines3D[D.Index].Lines;
				Camera* C = GUIStack->DrawLines3D[D.Index].C;
				
				D3D12_VERTEX_BUFFER_VIEW Views[] = {
					{		  LineDrawCall->GPUResource->GetGPUVirtualAddress() ,
						(UINT)LineDrawCall->LineSegments.size() * sizeof(LineSegment),
						(UINT)sizeof(float3) * 2
					},
				};

				CL->IASetVertexBuffers					(0, 1, Views);
				CL->IASetPrimitiveTopology				(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_LINELIST);
				CL->SetGraphicsRootConstantBufferView	(1,  C->Buffer.Get()->GetGPUVirtualAddress());
				CL->DrawInstanced						(2 * LineDrawCall->LineSegments.size(), 1, 0, 0);
			}	break;
			case DRAWCALLTYPE::DCT_Text: {
				{
					auto T = GUIStack->Text[D.Index];
					auto Font = T.Font;
					auto Text = T.Text;

					if(Text->CharacterCount){
						D3D12_RECT RECT = D3D12_RECT();
						RECT.right	= (UINT)RenderTarget.WH[0];
						RECT.bottom = (UINT)RenderTarget.WH[1];

						D3D12_SHADER_RESOURCE_VIEW_DESC SRV_DESC = {};
						SRV_DESC.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
						SRV_DESC.Texture2D.MipLevels           = 1;
						SRV_DESC.Texture2D.MostDetailedMip     = 0;
						SRV_DESC.Texture2D.PlaneSlice          = 0;
						SRV_DESC.Texture2D.ResourceMinLODClamp = 0;
						SRV_DESC.Shader4ComponentMapping	   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; 

						D3D12_VERTEX_BUFFER_VIEW VBuffers[] = {
							{ Text->Buffer.Get()->GetGPUVirtualAddress(), UINT(sizeof(TextEntry) * Text->CharacterCount), sizeof(TextEntry) },
						};
			
						auto TextureView = ReserveDescHeap(RS, 8);
						RS->pDevice->CreateShaderResourceView(Font->Texture, &SRV_DESC, TextureView);

						ID3D12DescriptorHeap* Heaps[] = { GetCurrentDescriptorTable(RS) };

						CL->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
						CL->IASetVertexBuffers				(0, 1, VBuffers);
						CL->SetDescriptorHeaps				(1, Heaps);
						CL->SetGraphicsRootDescriptorTable	(0, TextureView);
						CL->OMSetBlendFactor				(float4(1.0f, 1.0f, 1.0f, 1.0f));
						CL->RSSetScissorRects				(1, &RECT);

						CL->DrawInstanced					(Text->CharacterCount, 1, 0, 0);
					}
				}
			}	break;
			//case DRAWCALLTYPE::DCT_3DMesh:
				//CL->IASetVertexBuffers(0, 1, View + 1);
				//CL->DrawInstanced(4, 1, 4 * TexturedRectOffset, 0);
			//break;
			default:
				break;
			}

			PrevState = D.Type;
		}
	}


	/************************************************************************************************/

	
	void UploadGUI(RenderSystem* RS, GUIRender* RG, iAllocator* TempMemory, RenderWindow* TargetWindow)
	{
		if(RG->Rects.size())
			UpdateResourceByTemp(RS, &RG->RectBuffer, RG->Rects.begin(), RG->Rects.size() * sizeof(Draw_RECT), 1, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		for (auto& I : RG->Text)
			UploadTextArea(I.Font, I.Text, TempMemory, RS, TargetWindow);
	}


	/************************************************************************************************/


	void CleanUpDrawGUI(GUIRender* RG) {
		RG->Rects.Release();
		RG->TexturedRects.Release();
		RG->ClipAreas.Release();

		if(RG->RectBuffer)			RG->RectBuffer.Release();

		for (size_t I = 0; I < DRAWCALLTYPE::DCT_COUNT; ++I)
			if (RG->DrawStates[I])	RG->DrawStates[I]->Release();
	}


	/************************************************************************************************/


	void UploadLineSegments(RenderSystem* RS, LineSet* Pass)
	{
		if(Pass->LineSegments.size())
		{
			UpdateResourceByTemp(
					RS, &Pass->GPUResource, 
					Pass->LineSegments.begin(), 
					Pass->LineSegments.size() * sizeof(LineSegment), 1, 
					D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		}
	}


	/************************************************************************************************/


	void Clear(GUIRender* RG)
	{
		RG->DrawCalls.clear();
		RG->Rects.clear();
		RG->TexturedRects.clear();
	}


	/************************************************************************************************/


	void InitiateLineSet(RenderSystem* RS, iAllocator* Mem, LineSet* out)
	{
		out->LineSegments.Release();
		out->LineSegments.Allocator = Mem;

		out->GPUResource = CreateShaderResource(RS, KILOBYTE * 16);
		out->GPUResource._SetDebugName("LINE SEGMENTS");
	}


	/************************************************************************************************/


	void ClearLineSet(LineSet* Set)
	{
		Set->LineSegments.clear();
	}


	/************************************************************************************************/


	void CleanUpLineSet(LineSet* pass)
	{
		pass->LineSegments.Release();
		pass->GPUResource.Release();
	}


	/************************************************************************************************/


	void AddLineSegment(LineSet* Pass, LineSegment in)
	{
		Pass->LineSegments.push_back(in);
	}


	/************************************************************************************************/


	void SetWindowRect(ID3D12GraphicsCommandList* CL, RenderWindow* TargetWindow, UINT I)
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


	void ClearText(TextArea* TA)
	{
		memset(TA->TextBuffer, '\0', TA->BufferSize);
		TA->Position ={ 0, 0 };
		TA->Dirty = 0;
	}


	/************************************************************************************************/


	void CleanUpTextArea(TextArea* TA, FlexKit::iAllocator* BA)
	{
		BA->free(TA->TextBuffer);
		if (TA->Buffer) TA->Buffer.Release();
	}


	/************************************************************************************************/


	void UploadTextArea(FontAsset* F, TextArea* TA, iAllocator* Temp, RenderSystem* RS, RenderWindow* Target)
	{
		if (TA->Dirty)
		{
			TA->Dirty = 0;
			size_t Size =  TA->BufferDimensions.Product();
			auto& NewTextBuffer = FlexKit::fixed_vector<TextEntry>::Create_Aligned(Size, Temp, 0x10);

			uint2 I = { 0, 0 };
			size_t CharactersFound	= 0;
			float AspectRatio		= Target->WH[0] / Target->WH[1];
			float TextScale			= 0.5f;

			float2 PositionOffset	= float2(float(TA->Position[0]) / Target->WH[0], -float(TA->Position[1]) / Target->WH[1]);
			float2 StartPOS			= WStoSS(TA->ScreenPosition);
			float2 Scale			= float2( 1.0f, 1.0f ) / F->TextSheetDimensions;
			float2 Normlizer		= float2(1.0f/AspectRatio, 1.0f);
			float2 CharacterScale	= { TextScale, TextScale };

			while (I[1] <= TA->Position[1])
			{
				while (I[0] <= TA->BufferDimensions[0])
				{
					char C = *(TA->TextBuffer + I[0] + I[1] * TA->BufferDimensions[0]);
					if(C == '\n' || C == '\0')
						break;
					
					switch (C)
					{
					case ' ':
						break;
					default:
						{
							auto G		= F->GlyphTable[C];
							float2 WH   = G.WH * Scale;
							float2 XY   = G.XY * Scale;
							float2 UVTL = XY;
							float2 UVBR = XY + WH;
							float2 CPOS = StartPOS + ((WH * I) * float2(1.0f, -1.0f) * CharacterScale * Normlizer);

							TextEntry Entry;
							Entry.POS			= CPOS;
							Entry.Color			= { 1, 1, 1, 1 };
							Entry.BottomRightUV	= UVBR;
							Entry.TopLeftUV		= UVTL;
							Entry.Size			= WH * CharacterScale;
							NewTextBuffer.push_back(Entry);
							CharactersFound++;
						}	break;
					}

					++I[0];
				}

				I[0] = 0;
				++I[1];
			}
			TA->CharacterCount = CharactersFound;
			
			UpdateResourceByTemp(RS, &TA->Buffer, (void*)NewTextBuffer.begin(), sizeof(TextEntry) * NewTextBuffer.size(), 1, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		}
	}


	/************************************************************************************************/


	void PrintText(TextArea* Area, const char* text)
	{
		size_t strLength = strlen(text);
		Area->Dirty = true;

		for (size_t I = 0; I < strLength; ++I)
		{
			char C = text[I];
			if (Area->Position[0] >= Area->BufferDimensions[0])
			{
				Area->Position[0] = 0;
				Area->Position[1]++;
			}
			if (C == '\n')
			{
				Area->Position[0] = 0;
				Area->Position[1]++;
			}
			else if (C == '\0')
			{
				return;
			}
			else if (Area->Position[0] < Area->BufferDimensions[0]) 
			{
				if (!(Area->Position[0] < Area->BufferDimensions[0]))
				{
					Area->Position[1]++;
					Area->Position[0] = 0;

					if (Area->Position[1] >= Area->BufferDimensions[1])
						return;
				}
				if (Area->Position[1] < Area->BufferDimensions[1])
					Area->TextBuffer[Area->Position[0]++ + Area->Position[1] * Area->BufferDimensions[0]] = C;
			}
		}
	}


	/************************************************************************************************/


	TextArea CreateTextArea(RenderSystem* RS, iAllocator* Mem, TextArea_Desc* D)// Setups a 2D Surface for Drawing Text into
	{
		size_t C = D->WH.x/D->TextWH.x;
		size_t R = D->WH.y/D->TextWH.y;

		char* TextBuffer = (char*)Mem->malloc(C * R);
		memset(TextBuffer, '\0', C * R);

		HRESULT	HR = ERROR;
		D3D12_RESOURCE_DESC Resource_DESC	= CD3DX12_RESOURCE_DESC::Buffer(C * R);
		Resource_DESC.Height			= 1;
		Resource_DESC.Width				= C * R;
		Resource_DESC.DepthOrArraySize	= 1;

		D3D12_HEAP_PROPERTIES HEAP_Props = {};
		HEAP_Props.CPUPageProperty	     = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type				     = D3D12_HEAP_TYPE_DEFAULT;
		HEAP_Props.MemoryPoolPreference  = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask	     = 0;
		HEAP_Props.VisibleNodeMask		 = 0;

		auto NewResource = FlexKit::CreateShaderResource(RS, C * R * sizeof(TextEntry));
		NewResource._SetDebugName("TEXTOBJECT");

		return{ TextBuffer, C * R, {C, R},  {0, 0}, {0.0f, 1.0f}, NewResource, 0, false, Mem };
	}


}//	Namespace FlexKit
