

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

	void SetDebugName( ID3D12Object* Obj, char* cstr, size_t size )
	{
	#if USING(DEBUGGRAPHICS)
		wchar_t WString[128];
		mbstowcs(WString, cstr, 128);
		Obj->SetName(WString);
	#endif
	}

	#ifdef _DEBUG
	#define SETDEBUGNAME(RES, ID) char* NAME = ID; SetDebugName(RES, ID, strnlen(ID, 64))
	#else
	#define SETDEBUGNAME(RES, ID) 
	#endif
	// Globals
	HWND			gWindowHandle	= 0;
	HINSTANCE		gInstance		= 0;
	RenderWindow*	gInputWindow	= nullptr;
	uint2			gLastMousePOS;

	char* DEBUGDEVICEID		= "MainContext";
	char* DEBUGSWAPCHAINID	= "MainSwapChain";


	/************************************************************************************************/


	BufferArray::BufferArray( void )
	{
		m_size = 0;
	}


	/************************************************************************************************/


	BufferArray::BufferArray( const BufferArray& in )
	{
		auto _ptr = in.GetBuffers();
		m_size = in.GetBufferSize();
		for( size_t II = 0; II < m_size; II++ )
		{
			mBuffers[II] = _ptr[II];
			mBuffers[II]->AddRef();
		}
	}


	/************************************************************************************************/


	BufferArray::~BufferArray( void )
	{
		for( uint32_t itr = 0; itr < m_size; itr++ )
			mBuffers[itr]->Release();
	}


	/************************************************************************************************/


	ID3D11Buffer* BufferArray::operator [] ( uint32_t index )
	{
		return mBuffers[index];
	}


	/************************************************************************************************/


	const BufferArray& BufferArray::operator += (const BufferArray& rhs)
	{
#ifdef _DEBUG
		// Segfault detection
		FK_ASSERT( m_size < 16 );
		FK_ASSERT( rhs.GetBufferSize() < 16 );
#endif
		for( size_t ii = m_size, iii = 0; iii < rhs.GetBufferSize() && ii < 16; ii++, iii++ )
			push_back( rhs.GetBuffers()[iii] );

		return *this;
	}


	/************************************************************************************************/


	const BufferArray&	BufferArray::operator += (ID3D11Buffer* rhs)
	{
		push_back( rhs );
		return *this;
	}


	/************************************************************************************************/


	void BufferArray::clear()
	{
		for( auto itr = 0; itr < m_size; itr++ )
			if( mBuffers[itr] ) mBuffers[itr]->Release();
		m_size = 0;
	}


	/************************************************************************************************/


	void BufferArray::push_back( ID3D11Buffer* _ptr )
	{
		_ptr->AddRef();
		mBuffers[m_size] = _ptr;
		m_size++;
	}


	/************************************************************************************************/


	void BufferArray::pop_back()
	{
		m_size--;
#ifdef _DEBUG
		FK_ASSERT( m_size < 16 );
#endif
		mBuffers[m_size]->Release();
		mBuffers[m_size] = nullptr;
	}


	/************************************************************************************************/


	uint8_t BufferArray::GetBufferSize() const
	{
		return m_size;
	}


	/************************************************************************************************/


	ID3D11Buffer*const* BufferArray::GetBuffers() const
	{
		return mBuffers;
	}


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
		case WM_KEYUP:
		case WM_KEYDOWN:
		{
			FlexKit::Event ev;
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
		ID3D12DescriptorHeap* SRVDescHeap = nullptr;
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
		DH_Desc.NumDescriptors	= 1;
		HR = RS->pDevice->CreateDescriptorHeap(&DH_Desc, IID_PPV_ARGS(&DSVDescHeap));
		FK_ASSERT(FAILED(HR), "ERROR!");

		DH_Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		HR = RS->pDevice->CreateDescriptorHeap(&DH_Desc, IID_PPV_ARGS(&SRVDescHeap));
		FK_ASSERT(FAILED(HR), "ERROR!");

		out->DescriptorCBVSRVUAVSize = RS->DescriptorCBVSRVUAVSize;
		out->DescriptorDSVSize       = RS->DescriptorDSVSize;
		out->DescriptorRTVSize       = RS->DescriptorRTVSize;
		out->SRVDescHeap             = SRVDescHeap;
		out->RTVDescHeap             = RTVDescHeap;
		out->DSVDescHeap             = DSVDescHeap;
	}


	/************************************************************************************************/


	void CleanUpDescriptorHeaps(DescriptorHeaps* DDH)
	{
		DDH->SRVDescHeap->Release();
		DDH->RTVDescHeap->Release();
		DDH->DSVDescHeap->Release();
	}


	/************************************************************************************************/


	void CreateRootSignatureLibrary(RenderSystem* out)
	{
		ID3D12Device* Device = out->pDevice;

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
			CD3DX12_STATIC_SAMPLER_DESC	Default(0);
			CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;

			RootSignatureDesc.Init(5, Parameters, 0, nullptr);
			CheckHR(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, 
												&Signature, &ErrorBlob),
												PRINTERRORBLOB(ErrorBlob));

			ID3D12RootSignature* NewDevice = nullptr;
			CheckHR(Device->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(), IID_PPV_ARGS(&NewDevice)), 
												ASSERTONFAIL("FAILED TO CREATE ROOT SIGNATURE"));
		}
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
		HR = D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&DebugDevice);	FK_ASSERT(SUCCEEDED(HR));
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

		ID3D12Fence* MainFence = nullptr;
		HR = Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&MainFence);
		FK_ASSERT(FAILED(HR), "FAILED TO CREATE FENCE!");

		NewRenderSystem.Fence = MainFence;
		NewRenderSystem.DescriptorRTVSize			= Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		NewRenderSystem.DescriptorDSVSize			= Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		NewRenderSystem.DescriptorCBVSRVUAVSize		= Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
 			
		D3D12_COMMAND_QUEUE_DESC CQD ={};
		CQD.Flags							            = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
		CQD.Type								        = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;
		D3D12_COMMAND_QUEUE_DESC UploadCQD ={};
		UploadCQD.Flags							        = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
		UploadCQD.Type								    = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY;

		D3D12_COMMAND_QUEUE_DESC ComputeCQD = {};
		UploadCQD.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
		UploadCQD.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE;

		ID3D12CommandQueue*			GraphicsQueue		= nullptr;
		ID3D12CommandQueue*			UploadQueue			= nullptr;
		ID3D12CommandQueue*			ComputeQueue		= nullptr;
		ID3D12CommandAllocator*		GraphicsAllocator	= nullptr;
		ID3D12CommandAllocator*		UploadAllocator		= nullptr;
		ID3D12CommandAllocator*		ComputeAllocator	= nullptr;
		ID3D12GraphicsCommandList*	CommandList         = nullptr;
		ID3D12GraphicsCommandList*	UploadList	        = nullptr;
		ID3D12GraphicsCommandList*	ComputeList	        = nullptr;
		IDXGIFactory4*				DXGIFactory         = nullptr;

		HR = Device->CreateCommandQueue		(&CQD,		 __uuidof(ID3D12CommandQueue),		(void**)&GraphicsQueue);																	FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND QUEUE!");
		HR = Device->CreateCommandQueue		(&UploadCQD, __uuidof(ID3D12CommandQueue),		(void**)&UploadQueue);																		FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND QUEUE!");
		HR = Device->CreateCommandQueue		(&ComputeCQD, __uuidof(ID3D12CommandQueue),		(void**)&ComputeQueue);																		FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND QUEUE!");
		HR = Device->CreateCommandAllocator	(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,	__uuidof(ID3D12CommandAllocator), (void**)&GraphicsAllocator);					FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND ALLOCATOR!");
		HR = Device->CreateCommandAllocator	(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY,		__uuidof(ID3D12CommandAllocator), (void**)&UploadAllocator);					FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND ALLOCATOR!");
		HR = Device->CreateCommandAllocator	(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE,	__uuidof(ID3D12CommandAllocator), (void**)&ComputeAllocator);					FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND ALLOCATOR!");
		HR = Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,	GraphicsAllocator,	nullptr, __uuidof(ID3D12CommandList), (void**)&CommandList);	FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND LIST!");
		HR = Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY,	UploadAllocator,	nullptr, __uuidof(ID3D12CommandList), (void**)&UploadList);		FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND LIST!");
		HR = Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE,	ComputeAllocator,	nullptr, __uuidof(ID3D12CommandList), (void**)&ComputeList);	FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND LIST!");
		HR = CreateDXGIFactory(IID_PPV_ARGS(&DXGIFactory));																																FK_ASSERT(FAILED(HR), "FAILED TO CREATE DXGIFactory!");

		NewRenderSystem.pDevice			    = Device;
		NewRenderSystem.CommandList         = static_cast<ID3D12GraphicsCommandList*>(CommandList);
		NewRenderSystem.UploadList			= static_cast<ID3D12GraphicsCommandList*>(UploadList);
		NewRenderSystem.ComputeList			= static_cast<ID3D12GraphicsCommandList*>(ComputeList);
		NewRenderSystem.GraphicsCLAllocator = GraphicsAllocator;
		NewRenderSystem.UploadCLAllocator   = UploadAllocator;
		NewRenderSystem.ComputeCLAllocator  = ComputeAllocator;
		NewRenderSystem.UploadQueue			= UploadQueue;
		NewRenderSystem.GraphicsQueue		= GraphicsQueue;
		NewRenderSystem.ComputeQueue		= ComputeQueue;
		NewRenderSystem.pGIFactory		    = DXGIFactory;
		NewRenderSystem.MeshLoadedCount     = 0;
		NewRenderSystem.FenceHandle		    = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		NewRenderSystem.FenceValue		    = 1;
		NewRenderSystem.pDebugDevice	    = DebugDevice;
		NewRenderSystem.pDebug			    = Debug;

		ID3D12Fence* NewFence = nullptr;
		HR = Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&NewFence));	FK_ASSERT(FAILED(HR), "FAILED TO CREATE FENCE!");

		*out = NewRenderSystem;
		CreateDescriptorHeaps(out, &out->DefaultDescriptorHeaps);
		CreateRootSignatureLibrary(out);

		return;
	}


	/************************************************************************************************/


	void Destroy( Context* ctx )
	{
		//ClearContext( ctx );
		//ctx->DeviceContext->Release();
	}


	/************************************************************************************************/


	void Destroy(DepthBuffer* DB)
	{
		if(DB->Buffer)		DB->Buffer->Release();
	}


	/************************************************************************************************/


	void CleanUp(RenderSystem* System)
	{
#if USING(DEBUGGRAPHICS)
		// Prints Detailed Report
		//System->pDebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
		//System->pDebugDevice->Release();
		//System->pDebug->Release();
#endif

		System->pGIFactory->Release();
		System->pDevice->Release();
		System->GraphicsCLAllocator->Release();
		System->UploadCLAllocator->Release();
		System->CommandList->Release();
		System->UploadList->Release();
		System->GraphicsQueue->Release();
		System->Fence->Release();
		System->Memory->free(System->TempBuffers);
	}


	/************************************************************************************************/


	Texture2D CreateDepthBufferResource(RenderSystem* RS, Tex2DDesc* desc_in, bool Float32)
	{
		ID3D12Resource*			Resource		= nullptr;
		D3D11_TEXTURE2D_DESC	Desc			= { 0 };
		D3D12_RESOURCE_DESC		Resource_DESC	= {};
		FlexKit::Texture2D		NewTexture;

		if (Float32)
		{
			Resource_DESC.Alignment				= 0;
			Resource_DESC.DepthOrArraySize		= 1;
			Resource_DESC.Dimension				= D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			Resource_DESC.Layout				= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
			Resource_DESC.Width					= desc_in->Width;
			Resource_DESC.Height				= desc_in->Height;
			Resource_DESC.Format				= DXGI_FORMAT_D32_FLOAT;
			Resource_DESC.SampleDesc.Count		= 1;
			Resource_DESC.SampleDesc.Quality	= 0;
			Resource_DESC.Flags					= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			D3D12_HEAP_PROPERTIES HEAP_Props ={};
			HEAP_Props.CPUPageProperty	    = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			HEAP_Props.Type				    = D3D12_HEAP_TYPE_DEFAULT;
			HEAP_Props.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
			HEAP_Props.CreationNodeMask	    = 0;
			HEAP_Props.VisibleNodeMask		= 0;

			D3D12_CLEAR_VALUE	Clear;
			Clear.Color[0] = 0.0f;
			Clear.Color[1] = 0.0f;
			Clear.Color[2] = 0.0f;
			Clear.Color[3] = 0.0f;

			Clear.DepthStencil.Depth    = 0.0f;
			Clear.DepthStencil.Stencil	= 0;
			Clear.Format                = DXGI_FORMAT_D32_FLOAT;

			HRESULT HR = RS->pDevice->CreateCommittedResource(&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &Resource_DESC, D3D12_RESOURCE_STATES ::D3D12_RESOURCE_STATE_COMMON, &Clear, IID_PPV_ARGS(&Resource));
			if (FAILED(HR))
			{
				int x  = 0;
				// Do Error Handling Here
			}
		}
		else
		{
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

			D3D12_HEAP_PROPERTIES HEAP_Props ={};
			HEAP_Props.CPUPageProperty	    = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			HEAP_Props.Type				    = D3D12_HEAP_TYPE_DEFAULT;
			HEAP_Props.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
			HEAP_Props.CreationNodeMask	    = 0;
			HEAP_Props.VisibleNodeMask		= 0;

			HRESULT HR = RS->pDevice->CreateCommittedResource(&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &Resource_DESC, D3D12_RESOURCE_STATES ::D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&Resource));
			if (FAILED(HR))
			{
				int x  = 0;
				// Do Error Handling Here
			}
		}

		NewTexture.Format		= Resource_DESC.Format;
		NewTexture.Texture      = Resource;

		return (NewTexture);
	}


	/************************************************************************************************/


	void CreateDepthBuffer(RenderSystem* RS, uint2 Dimensions, byte* InitialData, DepthBuffer_Desc& DepthDesc, DepthBuffer* out)
	{
		// Create Depth Buffer
		FlexKit::Tex2DDesc desc;
		desc.Height      = Dimensions[1];
		desc.Width       = Dimensions[0];
		desc.initialData = InitialData;

		auto DB = FlexKit::CreateDepthBufferResource(RS, &desc, DepthDesc.Float32);
		if (!DB)
			FK_ASSERT(0);

		D3D12_DEPTH_STENCIL_VIEW_DESC DepthStencilDesc = {};
		DepthStencilDesc.Format        =  DepthDesc.Float32 ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_D24_UNORM_S8_UINT;
		DepthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		DepthStencilDesc.Flags         = D3D12_DSV_FLAG_NONE;

		D3D12_CLEAR_VALUE DepthOptimizedClearValue = {};
		DepthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		DepthOptimizedClearValue.DepthStencil.Depth = 1.0f;
		DepthOptimizedClearValue.DepthStencil.Stencil = 0;

		D3D12_CPU_DESCRIPTOR_HANDLE		DepthBufferHandle	={};

		RS->pDevice->CreateDepthStencilView(DB.Texture, &DepthStencilDesc,  RS->DefaultDescriptorHeaps.DSVDescHeap->GetCPUDescriptorHandleForHeapStart() );

		out->Buffer		 = DB;
		out->Inverted	 = DepthDesc.InverseDepth;
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

#ifdef _DEBUG
			{
				char DEBUGNAME[] = "BackBuffer";
				SetDebugName(buffer, DEBUGNAME, strlen(DEBUGNAME));
			}
#endif
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
		if (RS->TempBuffers)
		{
			for (auto r : *RS->TempBuffers)
				r->Release();

			RS->TempBuffers->clear();
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


	void ClearDepthStencil(RenderSystem* RS, ID3D11DepthStencilView* _ptr, float D) // Clears to black and max Depth
	{
		RS->ContextState.DeviceContext->ClearDepthStencilView(_ptr, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, D, 0);
	}


	/************************************************************************************************/


	void ClearRenderTargetView(Context*ctx, ID3D11RenderTargetView* rtv, FlexKit::float4& k)
	{
		ctx->DeviceContext->ClearRenderTargetView( rtv, k.pFloats.m128_f32);
	}


	/************************************************************************************************/


	void PresentWindow( RenderWindow* Window, RenderSystem* RS )									
	{ 
		WaitForFrameCompletetion(RS);
		ReleaseTempResources(RS);

		HRESULT HR;
		HR = RS->GraphicsCLAllocator->Reset();							FK_ASSERT(SUCCEEDED(HR));
		HR = RS->CommandList->Reset(RS->GraphicsCLAllocator, nullptr);	FK_ASSERT(SUCCEEDED(HR));


		Window->SwapChain_ptr->Present(1, 0);
		Window->BufferIndex = Window->SwapChain_ptr->GetCurrentBackBufferIndex();
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
			break;
		default:
			return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UINT;
			break;
		}
	}

	
	/************************************************************************************************/


	void FillResourceBuffer( RenderSystem* RS, ID3D12Resource* Dest, void* Data, size_t SourceSize, size_t ByteSize = 1, D3D12_RESOURCE_STATES EndState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
	{
		D3D12_RESOURCE_DESC   Resource_DESC = CD3DX12_RESOURCE_DESC::Buffer(128);
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

		D3D12_HEAP_PROPERTIES HEAP_Props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		ID3D12Resource* TempBuffer;
		HRESULT HR = RS->pDevice->CreateCommittedResource(&HEAP_Props, D3D12_HEAP_FLAG_NONE, 
											 &CD3DX12_RESOURCE_DESC::Buffer(SourceSize), D3D12_RESOURCE_STATE_GENERIC_READ, 
											 nullptr, IID_PPV_ARGS(&TempBuffer));

		if (FAILED(HR))
		{	// TODO!
			FK_ASSERT(0, "FAILED TO COMMIT MEMORY FOR Vertex Buffer");
		}

		void* Temp = nullptr;
		D3D12_SUBRESOURCE_DATA SRD;
		SRD.pData      = Data;
		SRD.RowPitch   = SourceSize;
		SRD.SlicePitch = SRD.RowPitch;

		TempBuffer->Map(0, &CD3DX12_RANGE(0, 0), &Temp);
		memcpy(Temp, Data, SourceSize);
		TempBuffer->Unmap(0, &CD3DX12_RANGE(0, SourceSize));

		//UpdateSubresources<1>(RS->CommandList, Dest, TempBuffer, 0, 0, 1, &SRD);
		RS->CommandList->CopyResource(Dest, TempBuffer);
		RS->CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(Dest, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ, -1, D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE));
#ifdef _DEBUG
		char* TEXT = "Temporary";
		SetDebugName(TempBuffer, TEXT, strlen(TEXT));
#endif

		AddTempBuffer(TempBuffer, RS);
	}


	/************************************************************************************************/

	template<typename TY_Allocator, typename TY_>
	fixed_vector<TY_>* ExpandFixedBuffer(TY_Allocator* Memory, fixed_vector<TY_>* in)
	{
		auto NewBuffer = &TempResourceList::Create_Aligned(in->size() * 2, Memory);
		RawMove(*NewBuffer, *in);
		Memory->free(in);

		return NewBuffer;
	}
	
	void AddTempBuffer(ID3D12Resource* _ptr, RenderSystem* RS)
	{
		if (!RS->TempBuffers)
			RS->TempBuffers = &TempResourceList::Create_Aligned(128, RS->Memory);

		if (RS->TempBuffers->full())
			RS->TempBuffers = ExpandFixedBuffer(RS->Memory, RS->TempBuffers);

		RS->TempBuffers->push_back(_ptr);
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

		if (FAILED(HR))
			FK_ASSERT(0, "FAILED TO COMMIT MEMORY FOR TEXTURE");

		FlexKit::Texture2D NewTexture ={ NewResource, 
										{desc_in->Height, desc_in->Height}, 
										TextureFormat2DXGIFormat(desc_in->Format)};

		return (NewTexture);
	}


	/************************************************************************************************/


	void UploadResources(RenderSystem* RS)
	{
		ID3D12CommandList* CommandLists[] = {RS->CommandList};
		RS->CommandList->Close();
		RS->GraphicsQueue->ExecuteCommandLists(1, CommandLists);
		RS->CommandList->Reset(RS->GraphicsCLAllocator, nullptr);
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

				HRESULT HR = RS->pDevice->CreateCommittedResource(&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &Resource_DESC, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&NewBuffer));

				if (FAILED(HR))
				{// TODO!
					FK_ASSERT(0);
				}
#ifdef _DEBUG
				switch (Buffers[itr]->GetBufferType())
				{
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION:
				{
					char* TEXT = "VERTEXBUFFER";
					SetDebugName(NewBuffer, TEXT, strlen(TEXT));
					break;
				}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL:
				{
					char* TEXT = "NORMAL BUFFER";
					SetDebugName(NewBuffer, TEXT, strlen(TEXT));
					break;
				}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT:
				{
					char* TEXT = "TANGET BUFFER";
					SetDebugName(NewBuffer, TEXT, strlen(TEXT));
					break;
				}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX:
					break;
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_COLOR:
				{
					char* TEXT = "COLOUR BUFFER";
					SetDebugName(NewBuffer, TEXT, strlen(TEXT));
					break;
				}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV:
				{
					char* TEXT = "TEXCOORD BUFFER";
					SetDebugName(NewBuffer, TEXT, strlen(TEXT));
					break;
				}
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1:
				{
					char* TEXT = "AnimationData1";
					SetDebugName(NewBuffer, TEXT, strlen(TEXT));
				}	break;
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2:
				{
					char* TEXT = "AnimationData2";
					SetDebugName(NewBuffer, TEXT, strlen(TEXT));
				}	break;
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_PACKED:
				{
					char* TEXT = "PACKED_BUFFER";
					SetDebugName(NewBuffer, TEXT, strlen(TEXT));
				}	break;
				case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ERROR:
					break;
				default:
					break;
		}
#endif 
			
			FillResourceBuffer(RS, NewBuffer, Buffers[itr]->GetBuffer(), Buffers[itr]->GetBufferSizeRaw());

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

			FillResourceBuffer(RS, NewBuffer, Buffers[15]->GetBuffer(), Buffers[0x0f]->GetBufferSizeRaw(), 1, D3D12_RESOURCE_STATE_INDEX_BUFFER);

#ifdef _DEBUG
			char* TEXT = "INDEXBUFFER";
			SetDebugName(NewBuffer, TEXT, strlen(TEXT));
#endif 

			DVB_Out.VertexBuffers[0x0f].Buffer				= NewBuffer;
			DVB_Out.VertexBuffers[0x0f].BufferSizeInBytes	= Buffers[0x0f]->GetBufferSizeRaw();
			DVB_Out.VertexBuffers[0x0f].BufferStride		= Buffers[0x0f]->GetElementSize();
			DVB_Out.VertexBuffers[0x0f].Type				= Buffers[0x0f]->GetBufferType();
			DVB_Out.MD.IndexBuffer_Index					= 0x0f;
		}

		UploadResources(RS);
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
		Resource_DESC.Flags					= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;

		D3D12_HEAP_PROPERTIES HEAP_Props ={};
		HEAP_Props.CPUPageProperty	     = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type				     = D3D12_HEAP_TYPE_DEFAULT;
		HEAP_Props.MemoryPoolPreference  = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask	     = 0;
		HEAP_Props.VisibleNodeMask		 = 0;

		ID3D12Resource* NewResource = nullptr;
		HRESULT HR = RS->pDevice->CreateCommittedResource(&HEAP_Props, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &Resource_DESC, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&NewResource));
		if (FAILED(HR))
		{
			FK_ASSERT(0);
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
						InputElementDesc.InputSlot            = static_cast<UINT>(InputSlot++);
						InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						InputElementDesc.InstanceDataStepRate = 0;
						InputElementDesc.SemanticIndex        = POS_Buffer_Counter++;
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
		if(buffer)
			buffer->Release();
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
	

	bool LoadShader( char* shader, size_t length, ShaderDesc* desc, Shader* out )
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


	const size_t BufferSize = 1024 * 1024;
	FLEXKITAPI bool	LoadComputeShaderFromFile(RenderSystem* RS, char* FileLoc, ShaderDesc* desc, Shader* out)
	{
		wchar_t WString[256];
		mbstowcs(WString, FileLoc, 128);
		ID3DBlob* NewBlob = nullptr;
		ID3DBlob* Errors = nullptr;

		DWORD dwShaderFlags = 0;
#if USING( DEBUGGRAPHICS )
		dwShaderFlags |= D3DCOMPILE_PREFER_FLOW_CONTROL | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_ENABLE_STRICTNESS;
#endif

		HRESULT HR = D3DCompileFromFile(WString, nullptr, nullptr, desc->entry, desc->shaderVersion, dwShaderFlags, 0, &NewBlob, &Errors);

		if (FAILED(HR))
		{
			printf( (char*)Errors->GetBufferPointer() );
			return false;
		}

		out->Blob = NewBlob;
		out->Type = desc->ShaderType;

		return true;
	}


	/************************************************************************************************/


	FLEXKITAPI bool	LoadComputeShaderFromString(RenderSystem* RS, char* str, size_t strlen, ShaderDesc* desc, Shader* out)
	{
		FK_ASSERT(0);
		return true;
	}


	/************************************************************************************************/


	bool LoadVertexShaderFromString( RenderSystem* RS, char* str, size_t strlen, ShaderDesc* desc, Shader* out )
	{
		FK_ASSERT(0);

		return false;
	}


	/************************************************************************************************/


	bool LoadPixelShaderFromString( RenderSystem* RS, char* str, size_t strlen, ShaderDesc* desc, Shader* out )
	{
		FK_ASSERT(0);

		return false;
	}


	/************************************************************************************************/


	FLEXKITAPI bool	LoadGeometryShaderFromFile(RenderSystem* RS, char* FileLoc, ShaderDesc* desc, Shader* out)
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
		if (FAILED(HR))
		{
			return false;
		}

		out->Blob = NewBlob;
		out->Type = desc->ShaderType;

		return true;
	}


	/************************************************************************************************/


	FLEXKITAPI bool	LoadGeometryWithSOShaderFromFile(RenderSystem* RS, char* FileLoc, ShaderDesc* desc, SODesc* SOD,  Shader* out)
	{
		wchar_t WString[256];
		mbstowcs(WString, FileLoc, 128);
		ID3DBlob* NewBlob = nullptr;
		ID3DBlob* Errors = nullptr;
		DWORD dwShaderFlags = 0;

#if USING( DEBUGGRAPHICS )
		dwShaderFlags |= D3DCOMPILE_PREFER_FLOW_CONTROL | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_ENABLE_STRICTNESS;
#endif

		HRESULT HR = D3DCompileFromFile(WString, nullptr, nullptr, desc->entry, desc->shaderVersion, dwShaderFlags, 0, &NewBlob, &Errors);
		if (FAILED(HR))
		{
			return false;
		}

		out->Blob = NewBlob;
		out->Type = desc->ShaderType;

		return true;
	}


	/************************************************************************************************/


	FLEXKITAPI bool	LoadGeometryShaderWithSOFromString(RenderSystem* RS, char* str, size_t strlen, ShaderDesc* desc, SODesc* SOD, Shader* out)
	{
		FK_ASSERT(0);

		return true;
	}


	/************************************************************************************************/


	FLEXKITAPI bool	LoadGeometryShaderFromString(RenderSystem* RS, char* str, size_t strlen, ShaderDesc* desc, Shader* out)
	{
		FK_ASSERT(0);

		return true;
	}


	/************************************************************************************************/


	FLEXKITAPI bool	LoadPixelShaderFromFile(RenderSystem* RS, char* FileLoc, ShaderDesc* desc, Shader* out)
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
		if (FAILED(HR))
		{
			printf((char*)Errors->GetBufferPointer());
			return false;
		}

		out->Blob = NewBlob;
		out->Type = desc->ShaderType;

		return true;
	}


	/************************************************************************************************/


	FLEXKITAPI bool LoadVertexShaderFromFile(RenderSystem* RS, char* FileLoc, ShaderDesc* desc, Shader* out )
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
		if (FAILED(HR))
		{
			(char*)Errors->GetBufferPointer();
			return false;
		}

		out->Blob = NewBlob;
		out->Type = desc->ShaderType;

		return true;
	}


	/************************************************************************************************/


	void DestroyShader( Shader* releaseme )
	{
		releaseme->Blob->Release();
	}


	/************************************************************************************************/
	

	void UpdateState(Context* ctx)
	{
		/*
		FK_ASSERT(0);
		if (ctx->IAState & IAFlags::VBufferChanged)	ctx->DeviceContext->IASetVertexBuffers(0, ctx->VertexBuffers.GetBufferSize(), ctx->VertexBuffers.GetBuffers(), ctx->Strides.begin(), ctx->Offsets.begin());

		if (ctx->CSState & Flags::CBChanged)	ctx->DeviceContext->CSSetConstantBuffers(0, ctx->CSBuffers.GetBufferSize(), ctx->CSBuffers.GetBuffers());
		if (ctx->DSState & Flags::CBChanged)	ctx->DeviceContext->DSSetConstantBuffers(0, ctx->DSBuffers.GetBufferSize(), ctx->DSBuffers.GetBuffers());
		if (ctx->GSState & Flags::CBChanged)	ctx->DeviceContext->GSSetConstantBuffers(0, ctx->GSBuffers.GetBufferSize(), ctx->GSBuffers.GetBuffers());
		if (ctx->HSState & Flags::CBChanged)	ctx->DeviceContext->HSSetConstantBuffers(0, ctx->HSBuffers.GetBufferSize(), ctx->HSBuffers.GetBuffers());
		if (ctx->VSState & Flags::CBChanged)	ctx->DeviceContext->VSSetConstantBuffers(0, ctx->VSBuffers.GetBufferSize(), ctx->VSBuffers.GetBuffers());
		if (ctx->PSState & Flags::CBChanged)	ctx->DeviceContext->PSSetConstantBuffers(0, ctx->PSBuffers.GetBufferSize(), ctx->PSBuffers.GetBuffers());

		if (ctx->CSState & Flags::UAVChanged)	ctx->DeviceContext->CSSetUnorderedAccessViews(0, ctx->CSUAV.size(), ctx->CSUAV.begin(), ctx->CSUAVInitialCounts.begin());

		auto test = ctx->VSSRResource.size();
		if (ctx->VSState & Flags::SRChanged)	
			ctx->DeviceContext->VSSetShaderResources(0, ctx->VSSRResource.size(), ctx->VSSRResource.begin());

		ctx->IAState = Flags::StateGood;
		ctx->CSState = Flags::StateGood;
		ctx->DSState = Flags::StateGood;
		ctx->GSState = Flags::StateGood;
		ctx->HSState = Flags::StateGood;
		ctx->PSState = Flags::StateGood;
		ctx->VSState = Flags::StateGood;
		ctx->OMState = Flags::StateGood;
		*/
	}


	/************************************************************************************************/


	void ClearContext( Context* ctx )
	{
		/*
		FK_ASSERT(0);
		ctx->IAState = Flags::StateGood;
		ctx->CSState = Flags::StateGood;
		ctx->DSState = Flags::StateGood;
		ctx->GSState = Flags::StateGood;
		ctx->HSState = Flags::StateGood;
		ctx->PSState = Flags::StateGood;
		ctx->VSState = Flags::StateGood;
		ctx->OMState = Flags::StateGood;

		ctx->CSBuffers       .clear();
		ctx->DSBuffers       .clear();
		ctx->GSBuffers       .clear();
		ctx->GSSamplers      .clear();
		ctx->HSBuffers       .clear();
		ctx->HSSamplers      .clear();
		ctx->IndexBuffers    .clear();
		ctx->InputLayoutStack.clear();
		ctx->PSBuffers       .clear();
		ctx->PSResource      .clear();
		ctx->PSSamplers      .clear();
		ctx->RenderTargets   .clear();
		ctx->Viewports       .clear();
		ctx->VSBuffers       .clear();
		ctx->VSResource      .clear();
		ctx->VSSamplers      .clear();
		ctx->Offsets		 .clear();
		ctx->Strides		 .clear();
		ctx->VertexBuffers   .clear();
		ctx->VSSRResource	 .clear();

		ctx->DepthBufferView = nullptr;
		ctx->DeviceContext->Flush();
		*/
	}


	/************************************************************************************************/


	void Draw(Context& ctx, size_t count, size_t offset)
	{
		//FK_ASSERT(0);
		//UpdateState(ctx);
		//ctx->Draw(count, offset);
	}


	/************************************************************************************************/


	void DrawAuto(Context& ctx)
	{
		//FK_ASSERT(0);
		//UpdateState(ctx);
		//ctx->DrawAuto();
	}


	/************************************************************************************************/


	void DrawIndexed( Context* ctx, size_t begin, size_t end, size_t vboffset )
	{
		//FK_ASSERT(0);
		//UpdateState(ctx);
		//ctx->DeviceContext->DrawIndexed( end-begin, begin, vboffset );
	}


	/************************************************************************************************/


	void DrawIndexedInstanced(Context* ctx, size_t begin, size_t end, size_t instancecount, size_t vboffset, size_t instancestart)
	{
		//FK_ASSERT(0);
		//UpdateState(ctx);
		//ctx->DeviceContext->DrawIndexedInstanced(begin-end, instancecount, begin, vboffset, instancestart );
	}


	/************************************************************************************************/


	void AddViewport( Context* ctx, Viewport VP )
	{
		//FK_ASSERT(0);
		/*
		D3D11_VIEWPORT DVP;
		DVP.Height   = VP.Height;
		DVP.Width    = VP.Width;
		DVP.MinDepth = VP.Min;
		DVP.MaxDepth = VP.Max;
		DVP.TopLeftX = VP.X;
		DVP.TopLeftY = VP.Y;
		ctx->Viewports.push_back( DVP );
		ctx->DeviceContext->RSSetViewports( ctx->Viewports.size(), ctx->Viewports.begin());
		*/
	}


	/************************************************************************************************/


	void AddRenderTarget( Context* ctx, ID3D11RenderTargetView* VP )
	{
		//FK_ASSERT(0);
		//ctx->RenderTargets.push_back( VP );
		//ctx->DeviceContext->OMSetRenderTargets( ctx->RenderTargets.size(), ctx->RenderTargets.begin(), ctx->DepthBufferView );
	}


	/************************************************************************************************/


	void AddVertexBuffer(Context* ctx, VertexBuffer* vertexbuffs)
	{
		//FK_ASSERT(0);
		/*
		for (size_t itr = 0; itr < 15; ++itr)
		{
			if (vertexbuffs->VertexBuffers[itr].Buffer != nullptr)
			{
				ctx->VertexBuffers.push_back(vertexbuffs->VertexBuffers[itr].Buffer);
				ctx->Offsets.push_back(vertexbuffs->MD.Offsets[itr]);
				ctx->Strides.push_back(vertexbuffs->MD.Strides[itr]);
			}
		}
		ctx->IAState = ctx->IAState | IAFlags::VBufferChanged;
		*/
	}


	/************************************************************************************************/


	void PopVertexBuffer(Context* ctx, VertexBuffer* vertexbuffs)
	{
		//FK_ASSERT(0);
		/*
		for (size_t itr = 0; itr < vertexbuffs->VertexBuffers.size() && (vertexbuffs->VertexBuffers[itr].Buffer != nullptr); ++itr)
		{
			if (vertexbuffs->VertexBuffers[itr].Buffer != nullptr)
			{
				ctx->VertexBuffers.pop_back();
				ctx->Offsets.pop_back();
				ctx->Strides.pop_back();
			}
		}
		*/
	}


	/************************************************************************************************/


	void VSPushShaderResource(Context* ctx, ID3D11ShaderResourceView* View)
	{
		//FK_ASSERT(0);
		/*
		if (ctx->VSSRResource[ctx->VSBuffers.GetBufferSize()] != View )
			ctx->VSState = ctx->VSState | Flags::SRChanged;
		ctx->VSSRResource.push_back(View);
		*/
	}

	void VSPopShaderResource(Context* ctx)
	{
		//ctx->VSSRResource.pop_back();
	}

	/************************************************************************************************/


	void VSPushConstantBuffer(Context* ctx, ConstantBuffer buffer)
	{
		/*
		if (ctx->VSBuffers[ctx->VSBuffers.GetBufferSize()] != buffer )
			ctx->VSState = ctx->VSState | Flags::CBChanged;
		ctx->VSBuffers.push_back(buffer);
		*/
	}


	/************************************************************************************************/


	void VSPopConstantBuffer(Context* ctx)
	{
		//FK_ASSERT(0);
		//ctx->VSBuffers.pop_back();
	}


	/************************************************************************************************/


	void GSPushConstantBuffer(Context* ctx, ConstantBuffer buffer)
	{
		//FK_ASSERT(0);
		/*
		if (ctx->GSBuffers[ctx->GSBuffers.GetBufferSize()] != buffer )
			ctx->GSState = ctx->GSState | Flags::CBChanged;
		ctx->GSBuffers.push_back(buffer);
		*/
	}


	/************************************************************************************************/


	void GSPopConstantBuffer(Context* ctx)
	{
		//FK_ASSERT(0);
		//ctx->GSBuffers.pop_back();
	}


	/************************************************************************************************/


	void PSPushConstantBuffer(Context* ctx, ConstantBuffer buffer)
	{
		//FK_ASSERT(0);
		/*
		if (ctx->PSBuffers[ctx->PSBuffers.GetBufferSize()] != buffer )
			ctx->PSState = ctx->PSState | Flags::CBChanged;
		ctx->PSBuffers.push_back(buffer);
		*/
	}


	/************************************************************************************************/


	void PSPopConstantBuffer(Context* ctx)
	{
		//FK_ASSERT(0);
		//ctx->PSBuffers.pop_back();
	}


	/************************************************************************************************/


	void SetIndexBuffer(Context* ctx, VertexBuffer* buffer)
	{
		//FK_ASSERT(0);
		//ctx->DeviceContext->IASetIndexBuffer(buffer->VertexBuffers[buffer->MD.IndexBuffer_Index].Buffer, DXGI_FORMAT::DXGI_FORMAT_R32_UINT, 0 );
	}


	/************************************************************************************************/


	void SetInputLayout(Context* ctx, VertexBuffer* buffer)
	{
		//FK_ASSERT(0);
		//ctx->DeviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//ctx->DeviceContext->IASetInputLayout(buffer->MD.InputLayout_ptr);
	}


	/************************************************************************************************/


	void SetTopology(Context* ctx, EInputTopology Topology)
	{
		//FK_ASSERT(0);
		/*
		switch (Topology)
		{
		case FlexKit::EIT_TRIANGLELIST:
			ctx->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			break;
		case FlexKit::EIT_TRIANGLE:
			ctx->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			break;
		case FlexKit::EIT_POINT:
			ctx->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
			break;
		default:
			break;
		}
		*/
	}


	/************************************************************************************************/


	void SetShader(Context* ctx, Shader* shader)
	{
		//FK_ASSERT(0);
		/*
		switch (shader->Type)
		{
			case FlexKit::SHADER_TYPE::SHADER_TYPE_Compute:		ctx->DeviceContext->CSSetShader((ID3D11ComputeShader*)	shader->DXShader, nullptr,	0);	break;
			case FlexKit::SHADER_TYPE::SHADER_TYPE_Geometry:	ctx->DeviceContext->GSSetShader((ID3D11GeometryShader*)	shader->DXShader, nullptr,	0);	break;
			case FlexKit::SHADER_TYPE::SHADER_TYPE_Hull:		ctx->DeviceContext->HSSetShader((ID3D11HullShader*)		shader->DXShader, nullptr,	0);	break;
			case FlexKit::SHADER_TYPE::SHADER_TYPE_Domain:		ctx->DeviceContext->DSSetShader((ID3D11DomainShader*)	shader->DXShader, nullptr,	0);	break;
			case FlexKit::SHADER_TYPE::SHADER_TYPE_Pixel:		ctx->DeviceContext->PSSetShader((ID3D11PixelShader*)	shader->DXShader, nullptr,	0); break;
			case FlexKit::SHADER_TYPE::SHADER_TYPE_Vertex:		ctx->DeviceContext->VSSetShader((ID3D11VertexShader*)	shader->DXShader, nullptr,	0);	break;
		default:
			break;
		}
		*/
	}
	

	/************************************************************************************************/


	void SetShader(Context* ctx, SHADER_TYPE t) // Clears Shader
	{
		//FK_ASSERT(0);
		/*
		switch (t)
		{
			case FlexKit::SHADER_TYPE::SHADER_TYPE_Compute:			(*ctx)->CSSetShader(nullptr, nullptr, 0); break;
			case FlexKit::SHADER_TYPE::SHADER_TYPE_Domain:			(*ctx)->DSSetShader(nullptr, nullptr, 0); break;
			case FlexKit::SHADER_TYPE::SHADER_TYPE_Geometry:		(*ctx)->GSSetShader(nullptr, nullptr, 0); break;
			case FlexKit::SHADER_TYPE::SHADER_TYPE_Hull:			(*ctx)->HSSetShader(nullptr, nullptr, 0); break;
			case FlexKit::SHADER_TYPE::SHADER_TYPE_Pixel:			(*ctx)->PSSetShader(nullptr, nullptr, 0); break;
			case FlexKit::SHADER_TYPE::SHADER_TYPE_Vertex:			(*ctx)->VSSetShader(nullptr, nullptr, 0); break;
		default:
			break;
		}
		*/
	}


	/************************************************************************************************/


	void SetDepthStencil(Context* ctx, ID3D11DepthStencilView* DSV)
	{
		//FK_ASSERT(0);
		//ctx->DepthBufferView = DSV;
		//ctx->DeviceContext->OMSetRenderTargets( ctx->RenderTargets.size(), ctx->RenderTargets.begin(), DSV );
	}

	
	/************************************************************************************************/


	void SetDepthStencilState(Context* ctx, DepthBuffer* DSV)
	{
		//FK_ASSERT(0);
		//ctx->DeviceContext->OMSetDepthStencilState(DSV->DepthState, 0);
	}


	/************************************************************************************************/


	void AddOutput( Context* ctx, RenderWindow* RW )
	{
		//FK_ASSERT(0);
		/*
		ID3D11RenderTargetView* RTV = RW->RenderTarget;
		AddRenderTarget(ctx, RW->RenderTarget );
		AddViewport( ctx, RW->VP );
		ctx->DeviceContext->OMSetRenderTargets(ctx->RenderTargets.size(), ctx->RenderTargets.begin(), ctx->DepthBufferView);
		*/
	}


	/************************************************************************************************/


	void AddOutput(Context* ctx, ID3D11RenderTargetView* RTV)
	{
		//FK_ASSERT(0);
		//AddRenderTarget(ctx, RTV );
		//ctx->DeviceContext->OMSetRenderTargets(ctx->RenderTargets.size(), ctx->RenderTargets.begin(), ctx->DepthBufferView);
	}


	/************************************************************************************************/


	void PopOutput(Context* ctx)
	{
		//FK_ASSERT(0);
		//ctx->RenderTargets.pop_back();
		//ctx->DeviceContext->OMSetRenderTargets(ctx->RenderTargets.size(), ctx->RenderTargets.begin(), ctx->DepthBufferView);
	}


	/************************************************************************************************/


	void ClearOutputs(Context* ctx)
	{
		//FK_ASSERT(0);
		/*
		ID3D11RenderTargetView*	NULLTARGETS[] = 
		{
			nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
		};

		size_t t = ctx->RenderTargets.size();
		ctx->RenderTargets.clear();

		ctx->DeviceContext->RSSetViewports(0, nullptr);
		ctx->DeviceContext->OMSetRenderTargets( t, NULLTARGETS, nullptr);
		*/
	}


	/************************************************************************************************/


	void CSAddUAV(Context* ctx, ID3D11UnorderedAccessView* RTV, UINT u)
	{
		//FK_ASSERT(0);
		/*
		if (ctx->CSUAV[ctx->CSUAV.size()] == RTV)
			ctx->CSState = ctx->CSState | Flags::CBChanged;
		ctx->CSUAV.push_back(RTV);
		ctx->CSUAVInitialCounts.push_back(u);
		*/
	}


	/************************************************************************************************/


	void CSPopUAV(Context* ctx)
	{
		//FK_ASSERT(0);
		//ctx->CSUAV.pop_back();
		//ctx->CSUAVInitialCounts.pop_back();
	}



	/************************************************************************************************/


	void AddStreamOut(Context* ctx, StreamOut SO, UINT* offsets)
	{
		//FK_ASSERT(0);
		//ctx->StreamOut.push_back(SO);
		//ctx->DeviceContext->SOSetTargets(ctx->StreamOut.size(), ctx->StreamOut.begin(), offsets);
	}


	/************************************************************************************************/


	void PopStreamOut(Context* ctx)
	{
		//FK_ASSERT(0);
		//ctx->StreamOut.pop_back();
		//ctx->DeviceContext->SOSetTargets(ctx->StreamOut.size(), ctx->StreamOut.begin(), nullptr);
	}


	/************************************************************************************************/


	void MapTo( Context* ctx, Texture2D Srce, char* _ptr )
	{
		//FK_ASSERT(0);
		D3D11_MAPPED_SUBRESOURCE	SR;
		SR.DepthPitch = 0;
		SR.RowPitch = 0;
		SR.pData = _ptr;
		//ctx->DeviceContext->Map( Srce.Texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &SR );
	}


	/************************************************************************************************/


	void MapTo( Context* ctx, RenderWindow* RW, char* _ptr )
	{
		//FK_ASSERT(0);

		/*
		D3D11_MAPPED_SUBRESOURCE	SR;
		SR.DepthPitch = 0;
		SR.RowPitch   = 0;
		SR.pData      = _ptr;
		ctx->DeviceContext->Map(RW->BackBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &SR );
		*/
	}


	/************************************************************************************************/


	void MapWriteDiscard(RenderSystem* rs, char* _ptr, size_t size, ConstantBuffer CB)
	{
		FillResourceBuffer(rs, CB, _ptr, size, 1);
	}


	/************************************************************************************************/


	void Unmap(Context* ctx, ConstantBuffer Srce)
	{
		FK_ASSERT(0);
		//ctx->DeviceContext->Unmap(Srce, 0);
	}


	/************************************************************************************************/


	void Unmap( Context* ctx, Texture2D Srce)
	{
		FK_ASSERT(0);
		//ctx->DeviceContext->Unmap( Srce.Texture, 0 );
	}


	/************************************************************************************************/


	void Unmap( Context* ctx, RenderWindow* RW )
	{
		FK_ASSERT(0);

		//ctx->DeviceContext->Unmap( RW->BackBuffer, 0 );
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
			{
				if (Nodes->Flags[I] & SceneNodes::FREE)
				{
					std::cout << "Node: " << I << " Unused\n";
				}
				else
				{
					std::cout << "Node: " << I << " Used\n";
				}
			}
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
					{
						if (!(Nodes->Flags[II] & SceneNodes::FREE))
							break;
					}
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
			{
				if (Nodes->Flags[I] & SceneNodes::FREE)
				{
					std::cout << "Node: " << I << " Unused\n";
				}
				else
				{
					std::cout << "Node: " << I << " Used\n";
				}
			}
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
		Nodes->Nodes[node].TH		= node;
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

				DirectX::XMMATRIX LT = DirectX::XMMatrixIdentity();
				LT_Entry TRS = GetLocal(Nodes, Nodes->Nodes[itr].TH);

				bool sf = (Nodes->Flags[itr] & SceneNodes::StateFlags::SCALE) != 0;
				LT =(	DirectX::XMMatrixRotationQuaternion(TRS.R) *
						DirectX::XMMatrixScalingFromVector(sf ? TRS.S : float3(1.0f, 1.0f, 1.0f).pfloats)) *
						DirectX::XMMatrixTranslationFromVector(TRS.T);

				auto ParentIndex = _SNHandleToIndex(Nodes, Nodes->Nodes[itr].Parent);
				auto PT = Nodes->WT[ParentIndex].m4x4;
				auto WT = DirectX::XMMatrixTranspose(DirectX::XMMatrixMultiply(LT, DirectX::XMMatrixTranspose(PT)));

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


	void InitiateDeferredPass(FlexKit::RenderSystem* RS, DeferredPassDesc* GBdesc, DeferredPass* out)
	{
		// Create Colour Buffer
		FlexKit::Tex2DDesc  desc;
		desc.Format       = FlexKit::FORMAT_2D::R8G8B8A8_UNORM;
		desc.Height       = GBdesc->RenderWindow->WH[1];
		desc.Width	      = GBdesc->RenderWindow->WH[0];
		desc.Read	      = false;
		desc.Write	      = false;
		desc.CV			  = true;
		desc.RenderTarget = true;
		desc.MipLevels    = 1;

		{
			// Create GBuffers
			{	// Alebdo Buffer
				out->ColorTex	 = FlexKit::CreateTexture2D(RS, &desc);
				FK_ASSERT(out->ColorTex);
				SETDEBUGNAME(out->ColorTex.Texture, "Albedo Buffer");
			}
			{
				// Create Specular Buffer
				out->SpecularTex = FlexKit::CreateTexture2D(RS, &desc);
				FK_ASSERT(out->SpecularTex);
				SETDEBUGNAME(out->SpecularTex.Texture, "Specular Buffer");
			}
			{
				// Create Normal Buffer
				desc.Format = FlexKit::FORMAT_2D::R32G32B32A32_FLOAT;

				out->NormalTex = FlexKit::CreateTexture2D(RS, &desc);
				FK_ASSERT(out->NormalTex);
				SETDEBUGNAME(out->NormalTex.Texture, "Normal Buffer");
			}
			{
				desc.Format      = FlexKit::FORMAT_2D::R32G32B32A32_FLOAT;
				out->PositionTex = FlexKit::CreateTexture2D(RS, &desc);
				FK_ASSERT(out->PositionTex);
				SETDEBUGNAME(out->PositionTex.Texture, "WorldCord Texture");
			}
			{
				desc.Format			= FlexKit::FORMAT_2D::R8G8B8A8_UNORM;
				desc.UAV			= true;
				desc.CV				= false;
				desc.RenderTarget	= false;
				out->OutputBuffer	= FlexKit::CreateTexture2D(RS, &desc);
				FK_ASSERT(out->OutputBuffer);
				SETDEBUGNAME(out->OutputBuffer.Texture, "Output Buffer");
			}
			{// LoadShaders
				bool res = false;
				FlexKit::ShaderDesc SDesc;
				strcpy(SDesc.entry, "cmain");
				strcpy(SDesc.ID,	"DeferredShader");
				strcpy(SDesc.shaderVersion, "cs_5_0");
				Shader CShader;
				do
				{
					printf("LoadingShader - Compute Shader Deferred Shader -\n");
					res = FlexKit::LoadComputeShaderFromFile(RS, "assets\\cshader.hlsl", &SDesc, &CShader);
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
				out->Shading.Shade = CShader;
			}

			// Create Constant Buffers
			{
				FlexKit::GBufferConstantsLayout initial;
				initial.DLightCount          = 0;
				initial.PLightCount          = 0;
				initial.SLightCount          = 0;
				initial.Width				 = GBdesc->RenderWindow->WH[0];
				initial.Height				 = GBdesc->RenderWindow->WH[1];
				initial.SLightCount          = 0;

				FlexKit::ConstantBuffer_desc CB_Desc;
				CB_Desc.InitialSize		= sizeof(initial);
				CB_Desc.pInital			= &initial;
				CB_Desc.Structured		= false;
				CB_Desc.StructureSize	= 0;

				auto NewConstantBuffer		 = CreateConstantBuffer(RS, &CB_Desc);
				out->Shading.ShaderConstants = NewConstantBuffer;

	#ifdef _DEBUG
				{
					char DEBUGNAME[] = "GBufferPass Constants";
					SetDebugName(NewConstantBuffer, DEBUGNAME, strlen(DEBUGNAME));
				}
	#endif
			}

			// Shading Signature Setup
			CD3DX12_DESCRIPTOR_RANGE ranges[2];
			ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 0);
			ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

			CD3DX12_ROOT_PARAMETER Parameters[DSRP_COUNT];
			Parameters[DSRP_DescriptorTable].InitAsDescriptorTable(2, ranges,	D3D12_SHADER_VISIBILITY_ALL);
			Parameters[DSRP_CameraConstants].InitAsConstantBufferView(0, 0,		D3D12_SHADER_VISIBILITY_ALL);
			Parameters[DSRP_ShadingConstants].InitAsConstantBufferView(1, 0,	D3D12_SHADER_VISIBILITY_ALL);
			
			ID3DBlob* Signature	= nullptr;
			ID3DBlob* ErrorBlob	= nullptr;
			CD3DX12_STATIC_SAMPLER_DESC	 Default(0);
			CD3DX12_ROOT_SIGNATURE_DESC  RootSignatureDesc;

			RootSignatureDesc.Init(DSRP_COUNT, Parameters, 1, &Default);
			HRESULT HR = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &Signature, &ErrorBlob);
			FK_ASSERT(SUCCEEDED(HR));

			ID3D12RootSignature* RootSig = nullptr;
			HR = RS->pDevice->CreateRootSignature(0, Signature->GetBufferPointer(),
												  Signature->GetBufferSize(), IID_PPV_ARGS(&RootSig));
				
			FK_ASSERT(SUCCEEDED(HR));
			Signature->Release();

			D3D12_DESCRIPTOR_HEAP_DESC	Heap_Desc;
			Heap_Desc.Flags				= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			Heap_Desc.NodeMask			= 0;
			Heap_Desc.NumDescriptors	= 10;
			Heap_Desc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

			ID3D12DescriptorHeap* SRVHeap = nullptr;
			HR = RS->pDevice->CreateDescriptorHeap(&Heap_Desc, IID_PPV_ARGS(&SRVHeap));
			FK_ASSERT(SUCCEEDED(HR));
			
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

			CD3DX12_CPU_DESCRIPTOR_HANDLE TableEntry(SRVHeap->GetCPUDescriptorHandleForHeapStart(), 2, RS->DescriptorCBVSRVUAVSize);
			RS->pDevice->CreateShaderResourceView(out->ColorTex,	&SRV_DESC, TableEntry); TableEntry.Offset(RS->DescriptorCBVSRVUAVSize);
			RS->pDevice->CreateShaderResourceView(out->SpecularTex, &SRV_DESC, TableEntry); TableEntry.Offset(RS->DescriptorCBVSRVUAVSize);

			SRV_DESC.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
			RS->pDevice->CreateShaderResourceView(out->NormalTex,	&SRV_DESC, TableEntry); TableEntry.Offset(RS->DescriptorCBVSRVUAVSize);
			RS->pDevice->CreateShaderResourceView(out->PositionTex, &SRV_DESC, TableEntry); TableEntry.Offset(RS->DescriptorCBVSRVUAVSize);
			
			UAV_DESC.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
			RS->pDevice->CreateUnorderedAccessView(out->OutputBuffer, nullptr, &UAV_DESC, TableEntry);


			D3D12_COMPUTE_PIPELINE_STATE_DESC CPSODesc{};
			ID3D12PipelineState* ShadingPSO = nullptr;

			CPSODesc.pRootSignature		= RootSig;
			CPSODesc.CS					= CD3DX12_SHADER_BYTECODE(out->Shading.Shade.Blob);
			CPSODesc.Flags				= D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;

			HR = RS->pDevice->CreateComputePipelineState(&CPSODesc, IID_PPV_ARGS(&ShadingPSO));
			FK_ASSERT(SUCCEEDED(HR));

			out->Shading.ShadingRTSig = RootSig;
			out->Shading.ShadingPSO   = ShadingPSO;
			out->SRVDescHeap          = SRVHeap;

#ifdef _DEBUG
			{
				char DEBUGNAME[] = "GBuffer Input Heap Descriptor Head";
				SetDebugName(SRVHeap, DEBUGNAME, strlen(DEBUGNAME));
			}
#endif
		}
		// GBuffer Fill Signature Setup
		{
			// LoadShaders
			{
				bool res = false;
				FlexKit::ShaderDesc SDesc;
				strcpy(SDesc.entry, "V2Main");
				strcpy(SDesc.ID,	"DeferredShader");
				strcpy(SDesc.shaderVersion, "vs_5_0");
				Shader VShader;
				do
				{
					printf("LoadingShader - VShader Shader - Deferred\n");
					res = FlexKit::LoadComputeShaderFromFile(RS, "assets\\vshader.hlsl", &SDesc, &VShader);
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
				out->Filling.NormalMesh = VShader;
				} while (!res);
			}
			{
				bool res = false;
				FlexKit::ShaderDesc SDesc;
				strcpy(SDesc.entry, "VMainVertexPallet");
				strcpy(SDesc.ID,	"DeferredShader");
				strcpy(SDesc.shaderVersion, "vs_5_0");
				Shader VShader;
				do
				{
					printf("LoadingShader - VShader Animated Shader - Deferred\n");
					res = FlexKit::LoadComputeShaderFromFile(RS, "assets\\vshader.hlsl", &SDesc, &VShader);
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
				out->Filling.AnimatedMesh = VShader;
				} while (!res);
			}
			{
				bool res = false;
				FlexKit::ShaderDesc SDesc;
				strcpy(SDesc.entry, "PMain");
				strcpy(SDesc.ID,	"DeferredShader");
				strcpy(SDesc.shaderVersion, "ps_5_0");
				Shader VShader;
				do
				{
					printf("LoadingShader - PShader No Texture Shader - Deferred\n");
					res = FlexKit::LoadComputeShaderFromFile(RS, "assets\\pshader.hlsl", &SDesc, &VShader);
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
				out->Filling.NoTexture = VShader;
				} while (!res);
			}
			// Setup Pipeline State
			{
				CD3DX12_ROOT_PARAMETER Parameters[3];
				Parameters[0].InitAsConstantBufferView	(0, 0, D3D12_SHADER_VISIBILITY_ALL);
				Parameters[1].InitAsConstantBufferView	(1, 0, D3D12_SHADER_VISIBILITY_ALL);
				Parameters[2].InitAsShaderResourceView	(0, 0, D3D12_SHADER_VISIBILITY_ALL);

				ID3DBlob* SignatureDescBlob                    = nullptr;
				ID3DBlob* ErrorBlob		                       = nullptr;
				D3D12_ROOT_SIGNATURE_DESC	 Desc              ={};
				CD3DX12_ROOT_SIGNATURE_DESC  RootSignatureDesc ={};
				CD3DX12_STATIC_SAMPLER_DESC	 Default(0);

				RootSignatureDesc.Init(RootSignatureDesc, 3, Parameters, 1, &Default, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
				HRESULT HR = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &SignatureDescBlob, &ErrorBlob);

				FK_ASSERT(SUCCEEDED(HR));
				SignatureDescBlob->Release();

				ID3D12RootSignature* RootSig = nullptr;
				HR = RS->pDevice->CreateRootSignature(0, SignatureDescBlob->GetBufferPointer(),
													  SignatureDescBlob->GetBufferSize(), IID_PPV_ARGS(&RootSig));

				FK_ASSERT(SUCCEEDED(HR));
				out->Filling.FillRTSig = RootSig;

				D3D12_INPUT_ELEMENT_DESC InputElements[5] =
				{
					{"POSITION",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
					{"TEXCOORD",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
					{"NORMAL",			0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
					{"WEIGHTS",			0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
					{"WEIGHTINDICES",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_UINT,  4, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				};

				D3D12_RASTERIZER_DESC		Rast_Desc  = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
				D3D12_DEPTH_STENCIL_DESC	Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
				Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;

				D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc ={};
				PSO_Desc.pRootSignature                      = RootSig;
				PSO_Desc.VS                                  ={ (BYTE*)out->Filling.NormalMesh.Blob->GetBufferPointer(), out->Filling.NormalMesh.Blob->GetBufferSize() };
				PSO_Desc.PS                                  ={ (BYTE*)out->Filling.NoTexture.Blob->GetBufferPointer(), out->Filling.NoTexture.Blob->GetBufferSize() };
				PSO_Desc.RasterizerState                     = Rast_Desc;
				PSO_Desc.BlendState		                     = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
				PSO_Desc.SampleMask		                     = UINT_MAX;
				PSO_Desc.PrimitiveTopologyType               = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
				PSO_Desc.NumRenderTargets	                 = 4;
				PSO_Desc.RTVFormats[0]		                 = DXGI_FORMAT_R8G8B8A8_UNORM;
				PSO_Desc.RTVFormats[1]		                 = DXGI_FORMAT_R8G8B8A8_UNORM;
				PSO_Desc.RTVFormats[2]		                 = DXGI_FORMAT_R32G32B32A32_FLOAT;
				PSO_Desc.RTVFormats[3]		                 = DXGI_FORMAT_R32G32B32A32_FLOAT;
				PSO_Desc.SampleDesc.Count	                 = 1;
				PSO_Desc.SampleDesc.Quality	                 = 0;
				PSO_Desc.DSVFormat			                 = DXGI_FORMAT_D32_FLOAT;
				PSO_Desc.InputLayout		                 ={ InputElements, 3 };
				PSO_Desc.DepthStencilState					 = Depth_Desc;

				D3D12_DESCRIPTOR_HEAP_DESC	Heap_Desc;
				Heap_Desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				Heap_Desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				Heap_Desc.NumDescriptors = 5;
				Heap_Desc.NodeMask		 = 0;

				ID3D12DescriptorHeap* RTVHeap = nullptr;
				HR = RS->pDevice->CreateDescriptorHeap(&Heap_Desc, IID_PPV_ARGS(&RTVHeap));

				FK_ASSERT(SUCCEEDED(HR));
				out->RTVDescHeap = RTVHeap;

				D3D12_RENDER_TARGET_VIEW_DESC	RTV_Desc;
				RTV_Desc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
				RTV_Desc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
				RTV_Desc.Texture2D.MipSlice   = 0;
				RTV_Desc.Texture2D.PlaneSlice = 0;
				
				CD3DX12_CPU_DESCRIPTOR_HANDLE RenderTarget(RTVHeap->GetCPUDescriptorHandleForHeapStart());
				RS->pDevice->CreateRenderTargetView(out->ColorTex, &RTV_Desc,	 RenderTarget);	RenderTarget.Offset(RS->DescriptorRTVSize);
				RS->pDevice->CreateRenderTargetView(out->SpecularTex, &RTV_Desc, RenderTarget); RenderTarget.Offset(RS->DescriptorRTVSize);

				RTV_Desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				RS->pDevice->CreateRenderTargetView(out->NormalTex, &RTV_Desc,	 RenderTarget);	RenderTarget.Offset(RS->DescriptorRTVSize);
				RS->pDevice->CreateRenderTargetView(out->PositionTex, &RTV_Desc, RenderTarget); RenderTarget.Offset(RS->DescriptorRTVSize);

				ID3D12PipelineState* PSO = nullptr;
				HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
				FK_ASSERT(SUCCEEDED(HR));
				out->Filling.PSO = PSO;

				PSO_Desc.VS          ={ (BYTE*)out->Filling.AnimatedMesh.Blob->GetBufferPointer(), out->Filling.AnimatedMesh.Blob->GetBufferSize() };
				PSO_Desc.InputLayout ={ InputElements, 5 };
				HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
				FK_ASSERT(SUCCEEDED(HR));
				out->Filling.PSOAnimated = PSO;

				SETDEBUGNAME(RTVHeap, "GBuffer RenderTarget Descriptor Head");
			}
		}
	}


	/************************************************************************************************/


	void BeginPass(ID3D12GraphicsCommandList* CL, RenderWindow* Window)
	{
		CL->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetBackBufferResource(Window),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	}


	/************************************************************************************************/


	void EndPass(ID3D12GraphicsCommandList* CL, RenderSystem* RS)
	{
		HRESULT HR;
		HR = CL->Close();	FK_ASSERT(SUCCEEDED(HR));

		ID3D12CommandList* CommandLists[10];
		CommandLists[0] = CL;

		RS->GraphicsQueue->ExecuteCommandLists(1, CommandLists);
	}


	/************************************************************************************************/


	void ClearBackBuffer(RenderSystem* RS, ID3D12GraphicsCommandList* CL, RenderWindow* RW, float4 ClearColor)
	{
		CL->ClearRenderTargetView(GetCurrentBackBufferHandle(&RS->DefaultDescriptorHeaps, RW), ClearColor, 0, nullptr);
	}


	/************************************************************************************************/


	void ClearDepthBuffer(RenderSystem* RS, ID3D12GraphicsCommandList* CL, DepthBuffer* DB, float ClearValue, int Stencil)
	{
		CL->ClearDepthStencilView(RS->DefaultDescriptorHeaps.DSVDescHeap->GetCPUDescriptorHandleForHeapStart(),
			D3D12_CLEAR_FLAG_DEPTH, ClearValue, Stencil, 0, nullptr);
	}


	/************************************************************************************************/


	ID3D12GraphicsCommandList*	GetCurrentCommandList(RenderSystem* RS){return RS->CommandList;}


	/************************************************************************************************/


	void WaitforGPU(RenderSystem* RS, size_t FenceValue)
	{
		RS->GraphicsQueue->Signal(RS->Fence, FenceValue);
		RS->Fence->SetEventOnCompletion(FenceValue, RS->FenceHandle);

		WaitForSingleObjectEx(RS->FenceHandle, INFINITE, FALSE);
	}

	
	/************************************************************************************************/


	void DoDeferredPass(PVS* _PVS, DeferredPass* Pass, Texture2D Target, RenderSystem* RS, Camera* C, float4& ClearColor, PointLightBuffer* PLB)
	{
		auto CL = RS->CommandList;
		{	// Clear Targets
			CD3DX12_CPU_DESCRIPTOR_HANDLE RT(Pass->RTVDescHeap->GetCPUDescriptorHandleForHeapStart());
			CL->ClearRenderTargetView(RT, ClearColor, 0, nullptr);
			RT.Offset(RS->DescriptorCBVSRVUAVSize);
			CL->ClearRenderTargetView(RT, ClearColor, 0, nullptr);
			RT.Offset(RS->DescriptorCBVSRVUAVSize);
			CL->ClearRenderTargetView(RT, ClearColor, 0, nullptr);
			RT.Offset(RS->DescriptorCBVSRVUAVSize);
			CL->ClearRenderTargetView(RT, ClearColor, 0, nullptr);
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
			VP.TopLeftX = -1;
			VP.TopLeftY = -1;

			D3D12_VIEWPORT	VPs[]	= { VP, VP, VP, VP, };
			D3D12_RECT		RECTs[] = { RECT, RECT, RECT, RECT,};

			CL->SetGraphicsRootSignature(Pass->Filling.FillRTSig);
			CL->SetPipelineState(Pass->Filling.PSO);
			CL->SetGraphicsRootConstantBufferView(0, C->Buffer->GetGPUVirtualAddress());
			CL->OMSetRenderTargets(4, &Pass->RTVDescHeap->GetCPUDescriptorHandleForHeapStart(), true, &RS->DefaultDescriptorHeaps.DSVDescHeap->GetCPUDescriptorHandleForHeapStart());

			CL->RSSetViewports(4, VPs);
			CL->RSSetScissorRects(4, RECTs);
			CL->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		}
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
			UAVDesc.Format               = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
			UAVDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
			UAVDesc.Texture2D.MipSlice   = 0;
			UAVDesc.Texture2D.PlaneSlice = 0;

			CD3DX12_CPU_DESCRIPTOR_HANDLE UAVPosition(Pass->SRVDescHeap->GetCPUDescriptorHandleForHeapStart(), 6, RS->DescriptorRTVSize);
			RS->pDevice->CreateUnorderedAccessView(Pass->OutputBuffer, nullptr, &UAVDesc, UAVPosition);
		}
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC SRV_DESC;
			SRV_DESC.Format                        = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
			SRV_DESC.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			SRV_DESC.ViewDimension                 = D3D12_SRV_DIMENSION_BUFFER;
			SRV_DESC.Buffer.FirstElement		   = 0;
			SRV_DESC.Buffer.Flags				   = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE;
			SRV_DESC.Buffer.NumElements			   = 512;
			SRV_DESC.Buffer.StructureByteStride	   = sizeof(float[8]);

			CD3DX12_CPU_DESCRIPTOR_HANDLE SRView(Pass->SRVDescHeap->GetCPUDescriptorHandleForHeapStart());
			RS->pDevice->CreateShaderResourceView(PLB->Resource, &SRV_DESC, SRView);	SRView.Offset(RS->DescriptorCBVSRVUAVSize);
		}

		for (Entity* E : *_PVS)
		{
			TriMesh* CurrentMesh = E->Mesh;
			size_t IBIndex = CurrentMesh->VertexBuffer.MD.IndexBuffer_Index;
			size_t ICount  = CurrentMesh->IndexCount;
			CL->SetGraphicsRootConstantBufferView(1, E->VConstants->GetGPUVirtualAddress());

			D3D12_INDEX_BUFFER_VIEW		IndexView;
			IndexView.BufferLocation	= GetBuffer(CurrentMesh, IBIndex)->GetGPUVirtualAddress();
			IndexView.Format			= DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
			IndexView.SizeInBytes		= ICount * 32;

			static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;
			FK_ASSERT(AddVertexBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,	CurrentMesh, VBViews));
			FK_ASSERT(AddVertexBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,			CurrentMesh, VBViews));
			FK_ASSERT(AddVertexBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,		CurrentMesh, VBViews));
			
			CL->SetGraphicsRootConstantBufferView(1, E->VConstants->GetGPUVirtualAddress()); 
			CL->IASetIndexBuffer(&IndexView);
			CL->IASetVertexBuffers(0, VBViews.size(), VBViews.begin() );
			CL->DrawIndexedInstanced(ICount, 1, 0, 0, 0);
		}

		CD3DX12_RESOURCE_BARRIER Barrier1[] ={
			CD3DX12_RESOURCE_BARRIER::Transition(Pass->ColorTex,	D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, -1, D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
			CD3DX12_RESOURCE_BARRIER::Transition(Pass->SpecularTex, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, -1, D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
			CD3DX12_RESOURCE_BARRIER::Transition(Pass->NormalTex,	D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, -1, D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
			CD3DX12_RESOURCE_BARRIER::Transition(Pass->PositionTex, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, -1, D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
			CD3DX12_RESOURCE_BARRIER::Transition(Pass->OutputBuffer,D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,			-1, D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE) };
		CL->ResourceBarrier(4, Barrier1);

		// Do Shading Here
		CL->SetDescriptorHeaps				(1, &Pass->SRVDescHeap);
		CL->SetPipelineState				(Pass->Shading.ShadingPSO);
		CL->SetComputeRootSignature			(Pass->Shading.ShadingRTSig);
		CL->SetComputeRootDescriptorTable	(DSRP_DescriptorTable,	Pass->SRVDescHeap->GetGPUDescriptorHandleForHeapStart());
		CL->SetComputeRootConstantBufferView(DSRP_CameraConstants,	Pass->Shading.ShaderConstants->GetGPUVirtualAddress());
		CL->SetComputeRootConstantBufferView(DSRP_ShadingConstants, Pass->Shading.ShaderConstants->GetGPUVirtualAddress());
		CL->Dispatch(Target.WH[0]/20, Target.WH[1] / 20, 1);

		CD3DX12_RESOURCE_BARRIER Barrier2[] ={
			CD3DX12_RESOURCE_BARRIER::Transition(Pass->ColorTex,		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, -1, D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
			CD3DX12_RESOURCE_BARRIER::Transition(Pass->SpecularTex,		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, -1, D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
			CD3DX12_RESOURCE_BARRIER::Transition(Pass->NormalTex,		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, -1, D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
			CD3DX12_RESOURCE_BARRIER::Transition(Pass->PositionTex,		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, -1, D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
			CD3DX12_RESOURCE_BARRIER::Transition(Pass->OutputBuffer,	D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE,			-1, D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
			CD3DX12_RESOURCE_BARRIER::Transition(Target,				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST,					-1, D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE)	};
		CL->ResourceBarrier(6, Barrier2);

		CL->CopyResource(Target, Pass->OutputBuffer);

		CD3DX12_RESOURCE_BARRIER Barrier3[] = {
			CD3DX12_RESOURCE_BARRIER::Transition(Pass->OutputBuffer,	D3D12_RESOURCE_STATE_COPY_SOURCE,	D3D12_RESOURCE_STATE_UNORDERED_ACCESS,	-1, D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
			CD3DX12_RESOURCE_BARRIER::Transition(Target,				D3D12_RESOURCE_STATE_COPY_DEST,		D3D12_RESOURCE_STATE_PRESENT,			-1, D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE),
		};

		CL->ResourceBarrier(2, Barrier3);
	}


	/************************************************************************************************/


	void InitiateForwardPass( FlexKit::RenderSystem* RS, ForwardPass_DESC* FBdesc, ForwardPass* out )
	{
		{
			FlexKit::ShaderDesc SDesc;
			strcpy(SDesc.entry, "VMain");
			strcpy(SDesc.ID,	"Forward");
			strcpy(SDesc.shaderVersion, "vs_5_0");
			Shader VShader;

			bool res = false;
			do
			{
				printf("LoadingShader - Compute Shader Deferred Shader -\n");
				res = FlexKit::LoadVertexShaderFromFile(RS, "assets\\ForwardPassVShader.hlsl", &SDesc, &VShader);
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
			out->VShader = VShader;
		}
		{
			FlexKit::ShaderDesc SDesc;
			strcpy(SDesc.entry, "PMain");
			strcpy(SDesc.ID,	"Forward");
			strcpy(SDesc.shaderVersion, "ps_5_0");
			Shader PShader;

			bool res = false;
			do
			{
				printf("LoadingShader - Compute Shader Deferred Shader -\n");
				res = FlexKit::LoadPixelShaderFromFile(RS, "assets\\ForwardPassPShader.hlsl", &SDesc, &PShader);
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
			out->PShader = PShader;
		}

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

		if (FAILED(HR))
		{
			printf((char*)ErrorBlob->GetBufferPointer());
			int x = 0;
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

		char* TEXT = "ForwardRenderCBHeap";
		SetDebugName(CBDescHeap, TEXT, strlen(TEXT));

		D3D12_INPUT_ELEMENT_DESC InputElements[] = 
		{
			{"POSITION", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	  1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"NORMAL",	 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};

		D3D12_RASTERIZER_DESC		Rast_Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		//Depth_Desc.DepthEnable	= false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {};
		PSO_Desc.pRootSignature                      = RootSig;
		PSO_Desc.VS                                  = {(BYTE*)out->VShader.Blob->GetBufferPointer(), out->VShader.Blob->GetBufferSize()};
		PSO_Desc.PS                                  = {(BYTE*)out->PShader.Blob->GetBufferPointer(), out->PShader.Blob->GetBufferSize()};
		PSO_Desc.RasterizerState                     = Rast_Desc;
		PSO_Desc.BlendState		                     = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		PSO_Desc.SampleMask		                     = UINT_MAX;
		PSO_Desc.PrimitiveTopologyType               = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		PSO_Desc.NumRenderTargets	                 = 1;
		PSO_Desc.RTVFormats[0]		                 = out->RenderTarget->Format;
		PSO_Desc.SampleDesc.Count	                 = 1;
		PSO_Desc.SampleDesc.Quality	                 = 0;
		PSO_Desc.DSVFormat			                 = out->DepthTarget->Buffer.Format;
		PSO_Desc.InputLayout		                 ={ InputElements, 3 };
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
		FP->CBDescHeap->Release();
		FP->PassRTSig->Release();
		FP->PShader.Blob->Release();
		FP->VShader.Blob->Release();
		FP->PSO->Release();
	}


	/************************************************************************************************/


	void WaitForFrameCompletetion(RenderSystem* RS)
	{
		size_t FenceValue		= RS->FenceValue;
		auto HR					= RS->GraphicsQueue->Signal(RS->Fence, FenceValue);FK_ASSERT(SUCCEEDED(HR));
		size_t CompleteValue	= RS->Fence->GetCompletedValue();
		RS->FenceValue++;

		if (CompleteValue < FenceValue)
		{
			auto HR = RS->Fence->SetEventOnCompletion(FenceValue, RS->FenceHandle); 
			FK_ASSERT(SUCCEEDED(HR));
			WaitForSingleObject(RS->FenceHandle, INFINITE);
		}
	}


	/************************************************************************************************/


	void DoForwardPass(PVS* _PVS, ForwardPass* Pass, RenderSystem* RS, Camera* C, float4& ClearColor, PointLightBuffer* PLB)
	{
		auto CL = RS->CommandList;

		{	// Setup State
			D3D12_RECT		RECT = D3D12_RECT();
			D3D12_VIEWPORT	VP	 = D3D12_VIEWPORT();

			RECT.right  = (UINT)Pass->RenderTarget->WH[0];
			RECT.bottom = (UINT)Pass->RenderTarget->WH[1];
			VP.Height   = (UINT)Pass->RenderTarget->WH[1];
			VP.Width    = (UINT)Pass->RenderTarget->WH[0];
			VP.MaxDepth = 1;
			VP.MinDepth = 0;
			VP.TopLeftX = -1;
			VP.TopLeftY = -1;

			CL->SetPipelineState(Pass->PSO);

			CL->SetGraphicsRootSignature(Pass->PassRTSig);
			CL->OMSetRenderTargets(1, &GetBackBufferView(Pass->RenderTarget), true, &RS->DefaultDescriptorHeaps.DSVDescHeap->GetCPUDescriptorHandleForHeapStart());

			CL->RSSetViewports(1, &VP);
			CL->RSSetScissorRects(1, &RECT);
		}

		CL->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		CL->SetGraphicsRootShaderResourceView(4, PLB->Resource->GetGPUVirtualAddress());
		for(auto& PV : *_PVS)
		{
			auto E                  = (Entity*)PV;
			size_t IBIndex          = E->Mesh->VertexBuffer.MD.IndexBuffer_Index;
			size_t IndexCount       = E->Mesh->IndexCount;


			D3D12_INDEX_BUFFER_VIEW		IndexView;
			IndexView.BufferLocation	= E->Mesh->VertexBuffer.VertexBuffers[IBIndex].Buffer->GetGPUVirtualAddress();
			IndexView.Format			= DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
			IndexView.SizeInBytes		= IndexCount * 32;

			CL->SetGraphicsRootConstantBufferView(0, C->Buffer->GetGPUVirtualAddress());
			CL->SetGraphicsRootConstantBufferView(1, E->VConstants->GetGPUVirtualAddress()); 
			CL->IASetIndexBuffer(&IndexView);

			static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;
			D3D12_VERTEX_BUFFER_VIEW VBView;
			for (size_t I = 0; I < E->Mesh->VertexBuffer.VertexBuffers.size(); ++I)
			{
				if (!E->Mesh->VertexBuffer.VertexBuffers[I].Buffer)
					continue;

				auto& VB = E->Mesh->VertexBuffer.VertexBuffers[I];
				VBView.BufferLocation	= VB.Buffer->GetGPUVirtualAddress();
				VBView.SizeInBytes		= VB.BufferSizeInBytes;
				VBView.StrideInBytes	= VB.BufferStride;
				VBViews.push_back(VBView);
			}

			CL->IASetVertexBuffers(0, VBViews.size(), VBViews.begin() );
			CL->DrawIndexedInstanced(IndexCount, 1, 0, 0, 0);
		}

		RS->CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetBackBufferResource(Pass->RenderTarget), 
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	}


	/************************************************************************************************/
	
	
	void CleanupDeferredPass(DeferredPass* gb)
	{
	}
	

	/************************************************************************************************/
	

	void ClearGBuffer(RenderSystem* RS, DeferredPass* gb)
	{
		/*
		FK_ASSERT(0);
		ClearRenderTargetView(*RS, gb->BufferView,	float4{0, 0, 0, 0});
		ClearRenderTargetView(*RS, gb->Colour,		float4{0, 0, 0, 0});
		ClearRenderTargetView(*RS, gb->Specular,	float4{0, 0, 0, 0});
		ClearRenderTargetView(*RS, gb->Normal,		float4{0, 0, 0, 0});
		ClearRenderTargetView(*RS, gb->Position,	float4{0, 0, 0, 0});
		*/
	}
	

	/************************************************************************************************/


	void CleanupCamera(SceneNodes* Nodes, Camera* camera)
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
		desc.Dest          = FlexKit::PIPELINE_DEST_VS | FlexKit::PIPELINE_DEST_PS;
		desc.Freq          = FlexKit::ConstantBuffer_desc::PERFRAME;
		desc.InitialSize   = 2048;
		desc.pInital       = &InitialData;
		desc.Structured    = false;
		desc.StructureSize = 0;

		auto NewBuffer = FlexKit::CreateConstantBuffer( RS, &desc);
		if (!NewBuffer)
			FK_ASSERT(0); // Failed to initialise Constant Buffer

		char* DebugName = "CAMERA BUFFER";
		SetDebugName(NewBuffer, DebugName, strlen(DebugName));

		out->Buffer = NewBuffer;
	}


	/************************************************************************************************/


	void UpdateCamera(RenderSystem* RS, SceneNodes* nodes, Camera* camera, int Pointlightcount, int SpotLightCount, double dt)
	{
		DirectX::XMMATRIX View;
		DirectX::XMMATRIX WT;

		FlexKit::GetWT(nodes, camera->Node, &WT);
		View = DirectX::XMMatrixInverse(nullptr, WT);

		Camera::BufferLayout NewData;
		NewData.Proj = CreatePerspective(camera);
		NewData.View = DirectX::XMMatrixTranspose(View);
		NewData.PV	 = DirectX::XMMatrixTranspose(DirectX::XMMatrixTranspose(NewData.Proj)* View);
		NewData.IV	 = DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, DirectX::XMMatrixTranspose(NewData.Proj)* View));
		NewData.WT	 = DirectX::XMMatrixTranspose(WT);
		
		NewData.MinZ = camera->Near;
		NewData.MaxZ = camera->Far;
		
		NewData.WPOS[0]				= WT.r[0].m128_f32[3];
		NewData.WPOS[1]				= WT.r[1].m128_f32[3];
		NewData.WPOS[2]				= WT.r[2].m128_f32[3];
		NewData.WPOS[3]				= 0;
		NewData.PointLightCount		= Pointlightcount;
		NewData.SpotLightCount		= SpotLightCount;

		FlexKit::MapWriteDiscard(RS, (char*)&NewData, sizeof(Camera::BufferLayout), camera->Buffer);

		camera->WT	 = XMMatrixToFloat4x4(&WT);
		camera->View = XMMatrixToFloat4x4(&View);
		camera->PV	 = XMMatrixToFloat4x4(&NewData.PV);
		camera->Proj = XMMatrixToFloat4x4(&NewData.Proj);
	}


	/************************************************************************************************/


	void UpdatePointLightBuffer( RenderSystem& RS, SceneNodes* nodes, PointLightBuffer* out, StackAllocator* TempMemory )
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
			itr++;
		}

		FlexKit::MapWriteDiscard(RS, (char*)PLs, sizeof(PointLightEntry) * out->Lights->size(), out->Resource);
	}


	/************************************************************************************************/


	void UpdateSpotLightBuffer( RenderSystem& RS, SceneNodes* nodes, SpotLightBuffer* out, StackAllocator* TempMemory )
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
				SLs[itr].Direction		= Q * float3{ 0, 0, 1 };
				SLs[itr].Direction[3]	= 1;
				itr++;
			}
		}

		MapWriteDiscard(RS, (char*)SLs, sizeof(SpotLightEntry) * out->Lights->size(), out->Resource);
	}


	/************************************************************************************************/


	void CreatePointLightBuffer( RenderSystem* RS, PointLightBuffer* out, PointLightBufferDesc Desc, BlockAllocator* Memory )
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


	void CleanUp(PointLightBuffer* out, BlockAllocator* Memory)
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


	void CleanUp(SpotLightBuffer* out, BlockAllocator* Memory)
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
		return Handle(0);
		auto HandleIndex = SL->size();
		SL->push_back({in.K, Dir, in.I, in.R, in.Hndl});

		LightHandle Handle;
		Handle.INDEX = HandleIndex;

		SetLightFlag(SL, Handle, LightBufferFlags::Dirty);
		return Handle;
	}


	/************************************************************************************************/


	void CreateSpotLightBuffer(RenderSystem* RS, SpotLightBuffer* out, BlockAllocator* Memory, size_t Max)
	{
		return;
		out->Lights		= &SLList::		Create_Aligned(Max, Memory, 0x10);
		out->Flags		= &LightFlags::	Create_Aligned(Max, Memory, 0x10);
		out->IDs		= &LightIDs::	Create_Aligned(Max, Memory, 0x10);
		
		D3D11_BUFFER_DESC BD;
		BD.BindFlags			= D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
		BD.ByteWidth			= sizeof(PointLightEntry) * 64;
		BD.CPUAccessFlags		= D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
		BD.Usage				= D3D11_USAGE::D3D11_USAGE_DYNAMIC;
		BD.MiscFlags			= D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		BD.StructureByteStride	= sizeof(PointLightEntry);
	
		D3D11_SUBRESOURCE_DATA SR;
		SR.pSysMem          = out->Lights->begin();
		SR.SysMemPitch      = 0;
		SR.SysMemSlicePitch = 0;

		FK_ASSERT(0);
		//HRESULT HR = RS->pDevice->CreateBuffer(&BD, &SR, &out->PBuffer);
		//if (FAILED(HR))
		//	FK_ASSERT(0);

		D3D11_SHADER_RESOURCE_VIEW_DESC RSD;
		RSD.Format					= DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		RSD.ViewDimension			= D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_BUFFER;
		RSD.Buffer.ElementOffset	= 0;
		RSD.Buffer.ElementWidth		= BD.ByteWidth/BD.StructureByteStride;

		FK_ASSERT(0);
		//HR = RS->pDevice->CreateShaderResourceView(out->PBuffer, &RSD, &out->SRV);
		//if (FAILED(HR))
		//	FK_ASSERT(0);

		for (auto& F : *out->Flags)
			F = LightBufferFlags::Dirty;
	}

	
	/************************************************************************************************/
	
	
	ShaderTable::ShaderTable() 
		: ShaderSetHndls(FlexKit::GetTypeID<ShaderSetHandle>())
		, ShaderHandles(FlexKit::GetTypeID<ShaderHandle>())
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
		
		TokenList&				TL			= TokenList::Create_Aligned(64000, &TempSpace);
		CombinedVertexBuffer&	out_buffer	= CombinedVertexBuffer::Create_Aligned(64000, &TempSpace);
		IndexList&				out_indexes	= IndexList::Create_Aligned(64000, &TempSpace);

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
		out.TriMeshID = RS->MeshLoadedCount++;// TODO: Make Thread Safe
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


	void UpdateEntity(RenderSystem* RS, SceneNodes* Nodes, const ShaderTable* M, Entity* E)
	{
		if ( E->Dirty | ( E->Visable && E->VConstants && GetFlag(Nodes, E->Node, SceneNodes::UPDATED)))
		{
			DirectX::XMMATRIX WT;
			FlexKit::GetWT( Nodes, E->Node, &WT );

			Entity::VConsantsLayout	NewData;

			NewData.MP.Albedo = E->OverrideMaterialProperties ? E->MatProperties.Albedo : M->GetAlbedo( E->Material );
			NewData.MP.Spec = E->OverrideMaterialProperties ? E->MatProperties.Spec : M->GetMetal( E->Material );
			NewData.Transform = DirectX::XMMatrixTranspose( WT );

			FlexKit::MapWriteDiscard( RS, ( char* )&NewData, sizeof( NewData ), E->VConstants );
			E->Dirty = false;
		}
	}



	/************************************************************************************************/


	void UpdateEntities( RenderSystem* RS, SceneNodes* Nodes, FlexKit::ShaderTable* M, PVS* PVS_ )
	{
		for ( auto v : *PVS_ ) {
			auto E = ( Entity* )v;
			UpdateEntity( RS, Nodes, M, v.V2 );
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
			auto E = ( (Entity*)v );
			auto P = FlexKit::GetPositionW( Nodes, E->Node );
			float D = float3( CP - P ).magnitudesquared() * ( E->DrawLast ? -1.0 : 1.0 );
			v.GetByType<size_t>() = D;
		}

		std::sort( PVS_->begin().I, PVS_->end().I, []( auto& R, auto& L ) -> bool
		{
			return ( (size_t)R < (size_t)L );
		} );
	}

	void SortPVSTransparent( SceneNodes* Nodes, PVS* PVS_, Camera* C)
	{
		if(!PVS_->size())
			return;

		auto CP = FlexKit::GetPositionW( Nodes, C->Node );
		for( auto& v : *PVS_ )
		{
			auto E = ( (Entity*)v );
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


	void CreateEntity( RenderSystem* RS, Entity* e, EntityDesc& desc )
	{
		e->Material = desc.Material;
		
		DirectX::XMMATRIX WT;
		WT = DirectX::XMMatrixIdentity();

		Entity::VConsantsLayout	VC;
		VC.Transform = DirectX::XMMatrixTranspose(WT);
		VC.MP.Albedo = {1.0f, 1.0f, 1.0f, 0.75f};
		VC.MP.Spec	 = {1.0f, 1.0f, 1.0f, 0.75f};

		e->MatProperties.Albedo = VC.MP.Albedo;
		e->MatProperties.Spec	= VC.MP.Spec;

		FlexKit::ConstantBuffer_desc CDesc;
		CDesc.InitialSize = 1024;
		CDesc.pInital     = &VC;
		CDesc.Dest        = FlexKit::PIPELINE_DEST_VS;
		CDesc.Freq        = FlexKit::ConstantBuffer_desc::PERFRAME;
		CDesc.Structured  = false;

		e->Mesh						  = nullptr;
		e->Visable                    = true; 
		e->OverrideMaterialProperties = false;
		e->Occluder                   = nullptr;
		e->DrawLast					  = false;
		e->Transparent				  = false;
		e->AnimationState			  = nullptr;
		e->PoseState				  = false;
		e->Posed					  = false;
		e->VConstants = FlexKit::CreateConstantBuffer(RS, &CDesc);
	}


	/************************************************************************************************/


	void CleanUpEntity(Entity* E)
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


	void StaticMeshBatcher::Initiate(FlexKit::RenderSystem* RS, ShaderHandle StaticMeshBatcher, ShaderHandle pshade)
	{
		for (auto G : Geometry)
			G = nullptr;

		NormalBuffer     = nullptr;
		IndexBuffer      = nullptr;
		TangentBuffer    = nullptr;
		VertexBuffer     = nullptr;
		GTBuffer         = nullptr;

		NormalSRV        = nullptr;
		IndexSRV         = nullptr;
		TangentSRV       = nullptr;
		VertexSRV        = nullptr;
		GTSRV            = nullptr;

		IL               = nullptr;
		Instances        = nullptr;
		TransformsBuffer = nullptr;
		TransformSRV     = nullptr;
		VPShader         = StaticMeshBatcher;
		PShader          = pshade;

		for ( auto& I : InstanceCount)
			I = 0;

		TotalInstanceCount   = 0;
		MaxVertexPerInstance = 0;
	}


	/************************************************************************************************/


	void StaticMeshBatcher::CleanUp()
	{
		if (NormalBuffer)		NormalBuffer->Release();
		if (TangentBuffer)		TangentBuffer->Release();
		if (IndexBuffer)		IndexBuffer->Release();
		if (VertexBuffer)		VertexBuffer->Release();
		if (GTBuffer)			GTBuffer->Release();

		if (NormalSRV)			NormalSRV->Release();
		if (TangentSRV)			TangentSRV->Release();
		if (IndexSRV)			IndexSRV->Release();
		if (VertexSRV)			VertexSRV->Release();
		if (GTSRV)				GTSRV->Release();

		if (IL)					IL->Release();
		if (Instances)			Instances->Release();
		if (TransformSRV)		TransformSRV->Release();
		if (TransformsBuffer)	TransformsBuffer->Release();

		NormalBuffer     = nullptr;
		IndexBuffer      = nullptr;
		TangentBuffer    = nullptr;
		VertexBuffer     = nullptr;
		GTBuffer         = nullptr;

		NormalSRV        = nullptr;
		IndexSRV         = nullptr;
		TangentSRV       = nullptr;
		VertexSRV        = nullptr;
		GTSRV            = nullptr;

		IL               = nullptr;
		Instances        = nullptr;
		TransformsBuffer = nullptr;
		TransformSRV     = nullptr;

		VPShader         = ShaderHandle();
		PShader          = ShaderHandle();

		for ( auto& I : InstanceCount)
			I = 0;

		TotalInstanceCount   = 0;
		MaxVertexPerInstance = 0;
	}


	/************************************************************************************************/


	void StaticMeshBatcher::Update_PreDraw(RenderSystem* RS, SceneNodes* nodes)
	{
		FK_ASSERT(0);

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


	void StaticMeshBatcher::BuildGeometryTable(FlexKit::RenderSystem* RS, FlexKit::ShaderTable* M, StackAllocator* TempMemory)
	{
		using DirectX::XMMATRIX;
		{	// Create Vertex Buffer
			size_t	BufferSize = 0;

			char*	Vertices = nullptr;
			for (auto G : Geometry)
			{
				if (G){
					for (auto B : G->Buffers)
					{
						if (B){
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
				for (auto G : Geometry)
				{
					if (G){
						for (auto B : G->Buffers)
						{
							if (B){
								if (B->GetBufferType() == VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION)
								{
									memcpy(Vertices + offset, B->GetBuffer(), B->GetBufferSizeRaw());
									GeometryTable[GE++].VertexOffset += offset/sizeof(float[3]);
									offset += B->GetBufferSizeRaw();

									break;
								}
							}
						}
					}
					else break;
				}

				D3D11_BUFFER_DESC BD;
				BD.BindFlags            = D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER;
				BD.ByteWidth            = BufferSize;
				BD.CPUAccessFlags       = 0;
				BD.Usage                = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
				BD.MiscFlags            = 0;
				BD.StructureByteStride  = 12;

				D3D11_SUBRESOURCE_DATA	SR;
				SR.pSysMem              = Vertices;
				FK_ASSERT(0);
				//if (FAILED(RS->pDevice->CreateBuffer(&BD, &SR, &VertexBuffer))) FK_ASSERT(0);
			}
		}
		{	// Create Normals Buffer
			size_t	BufferSize = 0;
			char*	Vertices = nullptr;
			for (auto G : Geometry)
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
				for (auto G : Geometry)
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

				D3D11_BUFFER_DESC BD;
				BD.BindFlags            = D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER;
				BD.ByteWidth            = BufferSize;
				BD.CPUAccessFlags       = 0;
				BD.Usage                = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
				BD.MiscFlags            = 0;
				BD.StructureByteStride  = sizeof(float[3]);

				D3D11_SUBRESOURCE_DATA	SR;
				SR.pSysMem              = Vertices;
				FK_ASSERT(0);
				//if (FAILED(RS->pDevice->CreateBuffer(&BD, &SR, &NormalBuffer))) FK_ASSERT(0);
			}
		}
		{	// Create Tangent Buffer
			size_t	BufferSize = 0;
			char*	Vertices = nullptr;
			for (auto G : Geometry)
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
				for (auto G : Geometry)
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

				D3D11_BUFFER_DESC BD;
				BD.BindFlags            = D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER;
				BD.ByteWidth            = BufferSize;
				BD.CPUAccessFlags       = 0;
				BD.Usage                = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
				BD.MiscFlags            = 0;
				BD.StructureByteStride  = sizeof(float[3]);

				D3D11_SUBRESOURCE_DATA	SR;
				SR.pSysMem              = Vertices;
				FK_ASSERT(0);
				//if (FAILED(RS->pDevice->CreateBuffer(&BD, &SR, &TangentBuffer))) FK_ASSERT(0);
			}
		}
		{	// Create Index Buffer
			size_t	BufferSize = 0;
			byte*	Indices = nullptr;

			for (auto G : Geometry)
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
				size_t GE = 0;
				size_t offset = 0;

				for (auto G : Geometry)
				{
					if (G){
						for (auto B : G->Buffers)
						{
							if (B){
								if (B->GetBufferType() == VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX)
								{
									memcpy(Indices + offset, B->GetBuffer(), B->GetBufferSizeRaw());
									GeometryTable[GE].	IndexOffset += offset/4;
									GeometryTable[GE++].VertexCount += G->IndexCount;
										
									offset += B->GetBufferSizeRaw();

									if ( G->IndexCount > MaxVertexPerInstance)
										MaxVertexPerInstance = G->IndexCount;
									break;
								}
							}
						}
					}
					else break;
				}

				D3D11_BUFFER_DESC BD;
				BD.BindFlags            = D3D11_BIND_INDEX_BUFFER;
				BD.ByteWidth            = BufferSize;
				BD.CPUAccessFlags       = 0;
				BD.Usage                = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
				BD.MiscFlags            = 0;
				BD.StructureByteStride  = sizeof(uint32_t);

				D3D11_SUBRESOURCE_DATA	SR;
				SR.pSysMem              = Indices;

				FK_ASSERT(0);
				//if (FAILED(RS->pDevice->CreateBuffer(&BD, &SR, &IndexBuffer))) FK_ASSERT(0);
			}
#if USING(DEBUGGRAPHICS)
			else printf("Warning !! Index Buffer Not Found\n");
#endif
		}
		{
			D3D11_BUFFER_DESC BD;
			BD.BindFlags            = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
			BD.ByteWidth            = sizeof(GeometryTable);
			BD.CPUAccessFlags       = 0;
			BD.Usage                = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
			BD.MiscFlags            = D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			BD.StructureByteStride  = sizeof(uint32_t[3]);

			D3D11_SUBRESOURCE_DATA	SR;
			SR.pSysMem              = GeometryTable;

			FK_ASSERT(0);
			//if (FAILED(RS->pDevice->CreateBuffer(&BD, &SR, &GTBuffer))) FK_ASSERT(0);

			D3D11_SHADER_RESOURCE_VIEW_DESC SD;
			SD.Format               = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
			SD.ViewDimension        = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_BUFFER;
			SD.Buffer.ElementOffset = 0;
			SD.Buffer.ElementWidth  = BD.ByteWidth / BD.StructureByteStride;

			FK_ASSERT(0);
			//if (FAILED(RS->pDevice->CreateShaderResourceView(GTBuffer, &SD, &GTSRV))) FK_ASSERT(0);
		}
		{
			D3D11_BUFFER_DESC BD;
			BD.BindFlags            = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
			BD.ByteWidth            = sizeof(XMMATRIX) * MAXINSTANCES;
			BD.CPUAccessFlags       = D3D11_CPU_ACCESS_WRITE;
			BD.Usage                = D3D11_USAGE::D3D11_USAGE_DYNAMIC;
			BD.MiscFlags            = D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			BD.StructureByteStride  = sizeof(XMMATRIX);

			FK_ASSERT(0);
			//if (FAILED(RS->pDevice->CreateBuffer(&BD, nullptr, &TransformsBuffer))) FK_ASSERT(0);

			D3D11_SHADER_RESOURCE_VIEW_DESC SD;
			SD.Format               = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
			SD.ViewDimension        = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_BUFFER;
			SD.Buffer.ElementOffset = 0;
			SD.Buffer.ElementWidth  = BD.ByteWidth / BD.StructureByteStride;

			FK_ASSERT(0);
			//if (FAILED(RS->pDevice->CreateShaderResourceView(TransformsBuffer, &SD, &TransformSRV))) FK_ASSERT(0);
		}
		{
			D3D11_BUFFER_DESC BD;
			BD.BindFlags            = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
			BD.ByteWidth            = sizeof(InstanceIOLayout) * MAXINSTANCES;
			BD.CPUAccessFlags       = D3D11_CPU_ACCESS_WRITE;
			BD.Usage                = D3D11_USAGE::D3D11_USAGE_DYNAMIC;
			BD.MiscFlags            = D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
			BD.StructureByteStride  = sizeof(InstanceIOLayout);

			D3D11_SUBRESOURCE_DATA	SR;
			SR.pSysMem = InstanceInformation;

			FK_ASSERT(0);
			//if (FAILED(RS->pDevice->CreateBuffer(&BD, &SR, &Instances))) FK_ASSERT(0);

			/*
			D3D11_UNORDERED_ACCESS_VIEW_DESC SD;
			SD.Format               = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
			SD.ViewDimension        = D3D11_UAV_DIMENSION::D3D11_UAV_DIMENSION_BUFFER;
			SD.Buffer.FirstElement	= 0;
			SD.Buffer.NumElements	= BD.ByteWidth / BD.StructureByteStride;
			SD.Buffer.Flags			= 0;

			if (FAILED(RS->pDevice->CreateUnorderedAccessView(TransformsBuffer, &SD, &RenderARGs))) FK_ASSERT(0);
			*/

			D3D11_INPUT_ELEMENT_DESC InputLayout[] =
			{ {
					"POSITION", 0,
					DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
					0,
					0,
					D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
					0
				},
				{
					"NORMAL", 0,
					DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
					1,
					0,
					D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
					0
				},
				{
					"TANGENT", 0,
					DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
					2,
					0,
					D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
					0
				}};

			Shader VShader = M->GetShader(VPShader);
			FK_ASSERT(0);
			//if (FAILED(RS->pDevice->CreateInputLayout(InputLayout, 3, VShader.Blob->GetBufferPointer(), VShader.Blob->GetBufferSize(), &IL))) FK_ASSERT(0);
		}
	}


	/************************************************************************************************/


	StaticMeshBatcher::SceneObjectHandle StaticMeshBatcher::CreateEntity(NodeHandle node, size_t gi)
	{
		size_t index = TotalInstanceCount++;
		InstanceCount[gi]++;
		SceneObjectHandle hndl = ObjectTable.GetNewHandle();
		ObjectTable[hndl] = index;

		GeometryIndex[index] = gi;
		NodeHandles[index] = node;
		DirtyFlags[index] = DIRTY_Transform | DIRTY_ChangedMesh;

		return hndl;
	}
	

	/************************************************************************************************/


	void StaticMeshBatcher::PrintFrameStats()
	{
		for (auto I = 0; I < TriMeshCount; ++I)
			printf("Tris per Instance Type: %u\nInstances: %u\nTotal Tris in Batch count: %u \n\n\n\n", GeometryTable[I].VertexCount, uint32_t(InstanceCount[I]), uint32_t(GeometryTable[I].VertexCount * InstanceCount[I]));

	}


	/************************************************************************************************/


	void StaticMeshBatcher::Draw(FlexKit::RenderSystem* RS, FlexKit::ShaderTable* M, Camera* C)
	{
		FK_ASSERT(0);
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
				*/
		
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
	}


	/************************************************************************************************/


	void UpdateGBufferConstants(RenderSystem* RS, DeferredPass* gb, size_t PLightCount, size_t SLightCount)
	{
		FlexKit::GBufferConstantsLayout constants;
		constants.DLightCount = 0;
		constants.PLightCount = PLightCount;
		constants.SLightCount = SLightCount;
		FlexKit::MapWriteDiscard(RS, (char*)&constants, sizeof(constants), gb->Shading.ShaderConstants);
	}


	/************************************************************************************************/


	void ShadeGBuffer(RenderSystem* RS, DeferredPass* gb, Camera* cam, PointLightBuffer* PL, SpotLightBuffer* SL, FlexKit::DepthBuffer* DBuff,FlexKit::RenderWindow* out)
	{
	}


	/************************************************************************************************/

	// assumes File str should be at least 256 bytes
	Texture2D LoadTextureFromFile(char* file, RenderSystem* RS)
	{
		Texture2D tex = {};
		wchar_t	wfile[256];
		size_t	ConvertedSize = 0;
		mbstowcs_s(&ConvertedSize, wfile, file, 256);

		FK_ASSERT(0);
		//HRESULT HR = DirectX::CreateDDSTextureFromFile(RS->pDevice, RS->ContextState.DeviceContext, wfile, (ID3D11Resource**)&tex.Texture, &tex.View, 1024);
		//FK_ASSERT(SUCCEEDED(HR), "Failed to Create Texture!");

		return tex;
	}


	/************************************************************************************************/


	void FreeTexture(Texture2D* Tex)
	{
		//Tex->Texture->Release();
		//if(Tex->Texture) Tex->Texture->Release();
		//if(Tex->View)	 Tex->View->Release();
	}


	/************************************************************************************************/
}

namespace FontUtilities
{
	/************************************************************************************************/


	void CleanUpTextArea(TextArea* TA, FlexKit::BlockAllocator* BA)
	{
		BA->free(TA->TextBuffer);
		TA->Buffer->Release();
	}

	void ClearText(TextArea* TA)
	{
		memset(TA->TextBuffer, '\0', TA->BufferSize);
		TA->Position ={ 0, 0 };
		TA->Dirty = 0;
	}


	/************************************************************************************************/


	void DrawTextArea(FontUtilities::FontAsset* F, TextArea* TA, StackAllocator* Temp, RenderSystem* RS, FlexKit::Context* Ctx, FlexKit::ShaderTable* ST, FlexKit::RenderWindow* Out)
	{
		using FontUtilities::TextEntry;
		using FlexKit::uint2;

		if (TA->Dirty)
		{
			TA->Dirty = 0;
			size_t Size =  TA->BufferDimensions.Product();
			auto& NewTextBuffer = FlexKit::fixed_vector<TextEntry>::Create_Aligned(Size, Temp, 0x10);

			uint2 I = { 0, 0 };
			size_t CharactersFound = 0;
			float AspectRatio = Out->WH[0]/ Out->WH[1];
			float TextScale = 0.5f;

			float2 PositionOffset	= float2(float(TA->Position[0]) / Out->WH[0], -float(TA->Position[1]) / Out->WH[1]);
			float2 StartPOS			= float2{ -1.0f, 1.0f };
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
							auto G	= F->GlyphTable[C];
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
			MapWriteDiscard(RS, (char*)NewTextBuffer.begin().I, sizeof(TextEntry) * NewTextBuffer.size(), TA->Buffer);
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
				Area->Position[I] = '\n';
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

	
	void InitiateTextRender(FlexKit::RenderSystem* RS, TextRender*	out)
	{
		using FlexKit::Shader;
		using FlexKit::ShaderDesc;
		{
			FlexKit::ShaderDesc SDesc;
			strcpy(SDesc.entry, "VTextMain");
			strcpy(SDesc.ID,	"TextPassThrough");
			strcpy(SDesc.shaderVersion, "vs_5_0");
			Shader VShader;

			bool res = false;
			do
			{
				printf("LoadingShader - Compute Shader Deferred Shader -\n");
				res = FlexKit::LoadComputeShaderFromFile(RS, "assets\\TextRendering.hlsl", &SDesc, &VShader);
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
			out->VShader = VShader;
		}
		{
			FlexKit::ShaderDesc SDesc;
			strcpy_s(SDesc.entry, "GTextMain");
			strcpy_s(SDesc.ID,	"TextGeometrySubmission");
			strcpy_s(SDesc.shaderVersion, "gs_5_0");
			Shader PShader;

			bool res = false;
			do
			{
				printf("LoadingShader - Compute Shader Deferred Shader -\n");
				res = FlexKit::LoadComputeShaderFromFile(RS, "assets\\TextRendering.hlsl", &SDesc, &PShader);
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
			out->PShader = PShader;
		}
		{
			FlexKit::ShaderDesc SDesc;
			strcpy_s(SDesc.entry, "PTextMain");
			strcpy_s(SDesc.ID,	"TextShading");
			strcpy_s(SDesc.shaderVersion, "ps_5_0");
			Shader PShader;

			bool res = false;
			do
			{
				printf("LoadingShader - Compute Shader Deferred Shader -\n");
				res = FlexKit::LoadComputeShaderFromFile(RS, "assets\\TextRendering.hlsl", &SDesc, &PShader);
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
			out->PShader = PShader;
		}
		// Create Root Signature
		{
			ID3DBlob* SignatureDescBlob                   = nullptr;
			ID3DBlob* ErrorBlob		                      = nullptr;

			CD3DX12_ROOT_PARAMETER Parameters[1];
			Parameters[3].InitAsShaderResourceView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

			D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS		 |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS	 | 
				D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS	 |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
		}
		// Create 
	}


	/************************************************************************************************/

	TextArea CreateTextObject(FlexKit::RenderSystem* RS, FlexKit::BlockAllocator* Mem, TextArea_Desc* D, FlexKit::ShaderTable* ST)// Setups a 2D Surface for Drawing Text into
	{
		size_t C = D->WH.x/D->TextWH.x;
		size_t R = D->WH.y/D->TextWH.y;

		char* TextBuffer = (char*)Mem->malloc(C * R);
		memset(TextBuffer, '\0', C * R);

		ID3D12Resource* Buffer = nullptr;

		return{ TextBuffer, C * R, {C, R},  {0, 0}, Buffer, 0, false};
	}

}