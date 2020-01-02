/**********************************************************************

Copyright (c) 2014-2018 Robert May

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

// TODOs
// Draw Wireframe Primitive Helper Functions
// Light Sorting for Tiled Rendering
// Shadowing

#ifdef _WIN32
#pragma once
#endif

#ifndef GRAPHICS_H_INCLUDED
#define GRAPHICS_H_INCLUDED

//#pragma comment( lib, "DirectXTK.lib")
#pragma comment( lib, "D3D12.lib")
#pragma comment( lib, "d3dcompiler.lib")
#pragma comment( lib, "dxgi.lib")
#pragma comment( lib, "dxguid.lib")

#include "PipelineState.h"

#include "..\PCH.h"
#include "..\buildsettings.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\Events.h"
#include "..\coreutilities\Handle.h"
#include "..\coreutilities\intersection.h"
#include "..\coreutilities\Logging.h"
#include "..\coreutilities\mathutils.h"
#include "..\coreutilities\Transforms.h"
#include "..\coreutilities\type.h"
#include "..\coreutilities\KeycodesEnums.h"
#include "..\graphicsutilities\Geometry.h"
#include "..\graphicsutilities\TextureUtilities.h"


#include <iso646.h>
#include <algorithm>
//#include <opensubdiv/far/topologyDescriptor.h>
//#include <opensubdiv/far/primvarRefiner.h>
#include <string>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3d12shader.h>
#include <DirectXMath.h>
#include <dxgi1_6.h>

#include <tuple>
#include <variant>

//#pragma comment(lib, "osdCPU.lib" )
//#pragma comment(lib, "osdGPU.lib" )


namespace FlexKit
{
	// Forward Declarations
	struct Buffer;
	struct RenderTargetDesc;
	struct RenderWindowDesc;
	struct RenderViewDesc;
	struct RenderWindow;
	struct ShaderResource;
	struct Shader;
	struct Texture2D;
	struct TriMesh;

    class Context;
	class ConstantBufferDataSet;
	class RenderSystem;
	class StackAllocator;
	class VertexBufferDataSet;

	using DirectX::XMMATRIX;

	/************************************************************************************************/

#pragma warning(disable:4067) 

	FLEXKITAPI void SetDebugName(ID3D12Object* Obj, const char* cstr, size_t size);

#ifdef USING(DEBUGGRAPHICS)
#define SETDEBUGNAME(RES, ID) {const char* NAME = ID; FlexKit::SetDebugName(RES, ID, strnlen(ID, 64));}
#else
#define SETDEBUGNAME(RES, ID) 
#endif

#define SAFERELEASE(RES) if(RES) {RES->Release(); RES = nullptr;}

#define CALCULATECONSTANTBUFFERSIZE(TYPE) (sizeof(TYPE)/1024 + 1024)


	/************************************************************************************************/


	template<typename TY_>
	HRESULT CheckHR(HRESULT HR, TY_ FN)
	{
		auto res = FAILED(HR);
		if (res) FN();
		return HR;
	}


#define PRINTERRORBLOB(Blob) [&](){std::cout << Blob->GetBufferPointer();}
#define ASSERTONFAIL(ERRORMESSAGE)[&](){FK_ASSERT(0, ERRORMESSAGE);}

	/************************************************************************************************/

	constexpr size_t GetFormatElementSize(const DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
			return sizeof(int32_t) * 4;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			return sizeof(float) * 4;
		case DXGI_FORMAT_R32G32B32A32_UINT:
			return sizeof(uint32_t) * 4;
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return sizeof(int32_t) * 4;
		case DXGI_FORMAT_R32G32B32_TYPELESS:
			return sizeof(int32_t) * 4;
		case DXGI_FORMAT_R32G32B32_FLOAT:
			return sizeof(float) * 3;
		case DXGI_FORMAT_R32G32B32_UINT:
			return sizeof(float) * 3;
		case DXGI_FORMAT_R32G32B32_SINT:
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
			throw; // unimplemented argument
			return -1;
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
			return 4;
		case DXGI_FORMAT_R16G16_UINT:
			return sizeof(uint16_t[2]);
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_A8_UNORM:
		case DXGI_FORMAT_R1_UNORM:
		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
		case DXGI_FORMAT_R8G8_B8G8_UNORM:
		case DXGI_FORMAT_G8R8_G8B8_UNORM:
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_B5G6R5_UNORM:
		case DXGI_FORMAT_B5G5R5A1_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
		case DXGI_FORMAT_AYUV:
		case DXGI_FORMAT_Y410:
		case DXGI_FORMAT_Y416:
		case DXGI_FORMAT_NV12:
		case DXGI_FORMAT_P010:
		case DXGI_FORMAT_P016:
		case DXGI_FORMAT_420_OPAQUE:
		case DXGI_FORMAT_YUY2:
		case DXGI_FORMAT_Y210:
		case DXGI_FORMAT_Y216:
		case DXGI_FORMAT_NV11:
		case DXGI_FORMAT_AI44:
		case DXGI_FORMAT_IA44:
		case DXGI_FORMAT_P8:
		case DXGI_FORMAT_A8P8:
		case DXGI_FORMAT_B4G4R4A4_UNORM:
		case DXGI_FORMAT_P208:
		case DXGI_FORMAT_V208:
		case DXGI_FORMAT_V408:
		case DXGI_FORMAT_FORCE_UINT:
		case DXGI_FORMAT_UNKNOWN:
			throw; // unimplemented argument
		default:
			throw; // invalid argument 
			return -1;
			break;
		}
	}


	/************************************************************************************************/


	enum PIPELINE_DESTINATION : unsigned char
	{
		PIPELINE_DEST_NONE = 0x00,// All Nodes are Initialised to NONE
		PIPELINE_DEST_IA = 0x01,
		PIPELINE_DEST_HS = 0x02,
		PIPELINE_DEST_GS = 0x04,
		PIPELINE_DEST_VS = 0x08,
		PIPELINE_DEST_PS = 0x10,
		PIPELINE_DEST_CS = 0x20,
		PIPELINE_DEST_OM = 0x30,
		PIPELINE_DEST_DS = 0x40,
		PIPELINE_DEST_ALL = 0xFF
	};


	/************************************************************************************************/


	inline D3D12_SHADER_VISIBILITY PipelineDest2ShaderVis(PIPELINE_DESTINATION PD)
	{
		switch (PD)
		{
		case FlexKit::PIPELINE_DEST_NONE:
			return D3D12_SHADER_VISIBILITY_ALL;
		case FlexKit::PIPELINE_DEST_IA:
			return D3D12_SHADER_VISIBILITY_ALL;
		case FlexKit::PIPELINE_DEST_HS:
			return D3D12_SHADER_VISIBILITY_HULL;
		case FlexKit::PIPELINE_DEST_GS:
			return D3D12_SHADER_VISIBILITY_GEOMETRY;
		case FlexKit::PIPELINE_DEST_VS:
			return D3D12_SHADER_VISIBILITY_VERTEX;
		case FlexKit::PIPELINE_DEST_PS:
			return D3D12_SHADER_VISIBILITY_PIXEL;
		case FlexKit::PIPELINE_DEST_CS:
			return D3D12_SHADER_VISIBILITY_ALL;
		case FlexKit::PIPELINE_DEST_OM:
			return D3D12_SHADER_VISIBILITY_ALL;
		case FlexKit::PIPELINE_DEST_DS:
			return D3D12_SHADER_VISIBILITY_DOMAIN;
		case FlexKit::PIPELINE_DEST_ALL:
			return D3D12_SHADER_VISIBILITY_ALL;
		default:
			break;
		}
		return D3D12_SHADER_VISIBILITY_ALL;
	}


	/************************************************************************************************/



	DXGI_FORMAT TextureFormat2DXGIFormat(FORMAT_2D F);
	FORMAT_2D	DXGIFormat2TextureFormat(DXGI_FORMAT F);




	/************************************************************************************************/


	const static size_t MaxBufferedSize = 3;

	template<typename TY_>
	struct FrameBufferedObject
	{
		FrameBufferedObject() { BufferCount = MaxBufferedSize; Idx = 0; for (auto& r : Resources) r = nullptr; }


		FrameBufferedObject(const std::initializer_list<TY_*>& IL) : FrameBufferedObject() {
			auto IL_I = IL.begin();
			for (size_t I = 0; I < MaxBufferedSize && IL_I != IL.end(); ++I, IL_I++)
				Resources[I] = *IL_I;
		}

		size_t Idx;
		size_t BufferCount;
		TY_*   Resources[MaxBufferedSize];
		size_t _Pad;

		size_t	operator ++() { IncrementCounter(); return (Idx); }			// Post Increment
		size_t	operator ++(int) { size_t T = Idx; IncrementCounter(); return T; }// Pre Increment

		TY_*& operator [] (size_t Index) { return Resources[Index]; }

		//operator ID3D12Resource*()				{ return Get(); }
		TY_* operator -> ()				{ return Get(); }
		const TY_* operator -> () const	{ return Get(); }

		operator bool()				{ return (Resources[0] != nullptr); }
		size_t		size()			{ return BufferCount; }

		TY_*		Get()			{ return Resources[Idx]; }
		TY_*		Get() const		{ return Resources[Idx]; }

		void IncrementCounter() { Idx = (Idx + 1) % BufferCount; };
		void Release()
		{
			for (auto& r : Resources)
			{
				if (r)
					r->Release();
				r = nullptr;
			};
		}

		void Release_Delayed(RenderSystem* RS)
		{
			for (auto& r : Resources)
				if (r) Push_DelayedRelease(RS, r);
		}


		void _SetDebugName(const char* _str) {
			size_t Str_len = strnlen_s(_str, 64);
			for (auto& r : Resources) {
				SetDebugName(r, _str, Str_len);
			}
		}
	};

	// Old
	typedef FrameBufferedObject<ID3D12Resource>								FrameBufferedResource;
	typedef FrameBufferedResource											IndexBuffer;
	typedef FrameBufferedResource											ConstantBuffer;
	typedef FrameBufferedResource											ShaderResourceBuffer;
	typedef FrameBufferedResource											StreamOutBuffer;
	typedef	FrameBufferedObject<ID3D12QueryHeap>							QueryResource;

	// New
	typedef Pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE>	DescHeapPOS;

	typedef ID3D12Resource*													VertexResourceBuffer;

	/************************************************************************************************/


	struct ConstantBuffer_desc
	{
		bool	Structured;
		size_t  StructureSize;

		void*	pInital;
		size_t	InitialSize;
	};


	/************************************************************************************************/


	struct RenderTargetDesc
	{
		uint32_t		height;
		uint32_t		width;
		uint32_t		mip_Levels;
		byte*			usr;
		// Describe iRenderTarget here
	};


	struct RenderWindowDesc
	{
		bool		fullscreen;
		uint64_t	hInstance;
		uint64_t	hWindow;
		uint32_t	height;
		uint32_t	width;
		uint32_t	depth;
		uint32_t	AA_Count;
		uint32_t	AA_Quality;
		uint32_t	POS_X;
		uint32_t	POS_Y;
		char		ID[64];
	};



	/************************************************************************************************/


	enum class UPDATESTAGE
	{
		BEGIN,
		CONSTANTBUFFERSUPDATE,
		PORTALSUPDATED,
		TRANSFORMSUPDATED,
		RENDERVIEWSUPDATED,
		PRESENT
	};


	/************************************************************************************************/


	struct Graphics_Desc
	{
		iAllocator*		Memory;
		iAllocator*		TempMemory;
		bool			DebugRenderMode;
		bool			Fullscreen;
		uint32_t		SlaveThreadCount;
	};

	enum class SHADER_TYPE
	{
		SHADER_TYPE_Compute,
		SHADER_TYPE_Domain,
		SHADER_TYPE_Geometry,
		SHADER_TYPE_Hull,
		SHADER_TYPE_Pixel,
		SHADER_TYPE_Vertex,
		SHADER_TYPE_Unknown
	};

	struct ShaderDesc
	{
		char			ID[128];
		char			entry[128];
		char			IncludeFile[128];
		char			shaderVersion[16];
		SHADER_TYPE		ShaderType;
	};

	struct SODesc
	{
		D3D12_SO_DECLARATION_ENTRY*		Descs;
		size_t							Element_Count;
		size_t							SO_Count;
		size_t							Flags;
		UINT							Strides[16];
	};


	/************************************************************************************************/


	struct Viewport
	{
		size_t X, Y, Height, Width;
		float  Min, Max;
	};


	/************************************************************************************************/


	struct UAVBuffer
	{
		UAVBuffer(const RenderSystem& rs, const UAVResourceHandle handle); // auto Fills the struct

		ID3D12Resource* resource;
		uint32_t		elementCount;
		uint32_t		counterOffset	= 0;
		uint32_t		offset			= 0;
		uint32_t		stride;
		DXGI_FORMAT		format;
		bool			typeless = false;
	};


	struct Texture2D
	{
		ID3D12Resource* operator ->()
		{
			return Texture;
		}

		operator ID3D12Resource*() const { return Texture; }
		operator bool() { return Texture != nullptr; }


		ID3D12Resource*		Texture;
		uint2				WH;
		uint8_t				mipCount;
		DXGI_FORMAT			Format;
	};


	/************************************************************************************************/


	struct RenderWindow
	{
		IDXGISwapChain4*			SwapChain_ptr;
        ResourceHandle               backBuffer;
		DXGI_FORMAT					Format;
		HWND						hWindow;

		uint2						WH; // Width-Height
		uint2						WindowCenterPosition;
		Viewport					VP;
		bool						Close;
		bool						Fullscreen;

		EventNotifier<>				Handler;


		WORD	InputBuffer[128];
		size_t	InputBuffSize;
	};


	/************************************************************************************************/


	struct DepthBuffer_Desc
	{
		size_t BufferCount;
		bool InverseDepth;
		bool Float32;	// When True stores as float
	};

	struct DepthBuffer
	{
		Texture2D					Buffer[MaxBufferedSize];
		size_t						CurrentBuffer;
		size_t						BufferCount;
		bool						Inverted;
	};


	inline void IncrementCurrent(DepthBuffer* DB) {
		DB->CurrentBuffer = (++DB->CurrentBuffer) % DB->BufferCount;
	}


	inline Texture2D GetCurrent(DepthBuffer* DB) {
		return DB->Buffer[DB->CurrentBuffer];
	}


	/************************************************************************************************/


	enum EInputTopology
	{
		EIT_LINE,
		EIT_TRIANGLELIST,
		EIT_TRIANGLE,
		EIT_POINT,
		EIT_PATCH_CP_1,
	};


	/************************************************************************************************/


	typedef	Vector<ID3D12Resource*> TempResourceList;
	const static int QueueSize		= 3;

	struct DescHeapStack
	{
		void Release()
		{
			if (DescHeap) 
				DescHeap->Release();

			DescHeap	= nullptr;
			CPU_HeapPOS = { 0 };
			GPU_HeapPOS = { 0 };
		}

		D3D12_CPU_DESCRIPTOR_HANDLE CPU_HeapPOS;
		D3D12_GPU_DESCRIPTOR_HANDLE GPU_HeapPOS;
		ID3D12DescriptorHeap*		DescHeap;
	};


	struct UploadQueues
	{
        UploadQueues() = default;


		~UploadQueues()
		{
			Release();
		}


        UploadQueueHandle Open()
		{
            const size_t currentIdx = idx++ % uploadContexts.size();
            auto& ctx               = uploadContexts[currentIdx];

            Wait(UploadQueueHandle{ currentIdx });

            ctx.commandAllocator->Reset();
            ctx.commandList->Reset(ctx.commandAllocator, nullptr);

            for (auto resource : ctx.tempResources)
                resource->Release();

            return UploadQueueHandle{ currentIdx };
		}


        void Wait(UploadQueueHandle handle)
        {
            auto& ctx = uploadContexts[handle];

            if (fence->GetCompletedValue() < ctx.counter)
            {
                fence->SetEventOnCompletion(ctx.counter, ctx.eventHandle);
                WaitForSingleObject(ctx.eventHandle, INFINITY);
            }
        }

		void Close(UploadQueueHandle handle)
		{
            auto& ctx = uploadContexts[handle];

			ctx.commandList->Close();
		}


        void Push_Temporary(ID3D12Resource* resource, UploadQueueHandle handle)
        {
            uploadContexts[handle].tempResources.push_back(resource);
        }


		void Release()
		{
            if (!fence)
                return;

            for (auto& ctx : uploadContexts)
            {
                if (fence->GetCompletedValue() < ctx.counter)
                {
                    fence->SetEventOnCompletion(ctx.counter, ctx.eventHandle);
                    WaitForSingleObject(ctx.eventHandle, INFINITY);

                }

                for (auto resource : ctx.tempResources)
                    resource->Release();
            }


            fence->Release();
            fence = nullptr;

            for (auto& ctx : uploadContexts)
            {
                ctx.commandAllocator->Release();
                ctx.commandList->Release();
            }



            uploadContexts.clear();
		}


		bool Initiate(ID3D12Device* Device, size_t threadCount, Vector<ID3D12DeviceChild*>& ObjectsCreated)
		{
			bool Success = true;

			for (size_t I = 0; I < threadCount && Success; ++I)
			{
				ID3D12CommandAllocator*		NewUploadAllocator	= nullptr;
				ID3D12GraphicsCommandList2*	NewUploadList		= nullptr;

				auto HR = Device->CreateCommandAllocator(
					D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY,
					IID_PPV_ARGS(&NewUploadAllocator));

				ObjectsCreated.push_back(NewUploadAllocator);
				

#ifdef _DEBUG
				FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND ALLOCATOR!");
				Success &= !FAILED(HR);
#endif
				if (!Success)
					break;


				HR = Device->CreateCommandList(
					0,
					D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY,
					NewUploadAllocator,
					nullptr,
					__uuidof(ID3D12CommandList),
					(void**)&NewUploadList);


				ObjectsCreated.push_back(NewUploadList);
				FK_ASSERT	(FAILED(HR), "FAILED TO CREATE COMMAND LIST!");
				SETDEBUGNAME(NewUploadAllocator, "UPLOAD ALLOCATOR");
				FK_VLOG		(10, "GRAPHICS COMMANDLIST COPY CREATED: %u", NewUploadList);


				Success &= !FAILED(HR);

				NewUploadList->Close();
				NewUploadAllocator->Reset();

                uploadContexts.push_back({
                    NewUploadAllocator,
                    NewUploadList,
                    0,
                    CreateEvent(nullptr, FALSE, FALSE, nullptr) });
                }

            Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

			return Success;
		}


        struct UploadContext
        {
		    ID3D12CommandAllocator*		commandAllocator    = nullptr;
		    ID3D12GraphicsCommandList*	commandList         = nullptr;
            size_t                      counter             = 0;
            HANDLE                      eventHandle;

            Vector<ID3D12Resource*>    tempResources;
        };

        ID3D12Fence*                        fence           = nullptr;
        atomic_uint                         idx             = 0;
        atomic_uint                         counter         = 0;
        CircularBuffer<UploadContext, 64>   uploadContexts;
	};


	struct PerFrameResources
	{
        PerFrameResources(iAllocator* allocator) :
            TempBuffers         { allocator },
            CommandLists        { allocator },
            GraphicsCLAllocator { allocator },
            ComputeCLAllocator  { allocator },
            ComputeList         { allocator },
            CommandListsUsed    { allocator }{}

		~PerFrameResources()
		{
			
		}

		bool Initiate(ID3D12Device* Device, Vector<ID3D12DeviceChild*>& ObjectsCreated)
		{
		}

		void Close()
		{
		}

		void Release()
		{
			DSVHeap.Release();
			DescHeap.Release();
			GPUDescHeap.Release();
			RTVHeap.Release();

			for (auto& CL : CommandLists)
			{
				if (CL)
					CL->Release();
				CL = nullptr;
			}

			for (auto& alloc : GraphicsCLAllocator)
			{
				if (alloc)
					alloc->Release();
				alloc = nullptr;
			}

			for (auto& CL : ComputeList)
			{
				if (CL)
					CL->Release();
				CL = nullptr;
			}

			for (auto& alloc : ComputeCLAllocator)
			{
				if (alloc)
					alloc->Release();
				alloc = nullptr;
			}

			for (auto& CLUsed : CommandListsUsed)
				CLUsed = false;

			ThreadsIssued = 0;
		}


		TempResourceList			        TempBuffers;
		Vector<ID3D12GraphicsCommandList3*>	CommandLists;
		Vector<ID3D12CommandAllocator*>		GraphicsCLAllocator;
		Vector<ID3D12CommandAllocator*>		ComputeCLAllocator;
        Vector<ID3D12GraphicsCommandList3*> ComputeList;
        Vector<bool>                        CommandListsUsed;

		DescHeapStack RTVHeap;
		DescHeapStack DSVHeap;
		DescHeapStack DescHeap;
		DescHeapStack GPUDescHeap;

		size_t		ThreadsIssued;
	};


	/************************************************************************************************/


	enum class DescHeapEntryType : uint32_t
	{
		ConstantBuffer,
		ShaderResource,
		UAVBuffer,
		HeapError
	};


	enum class RootSignatureEntryType : uint32_t
	{
		DescriptorHeap,
		ConstantBuffer,
		StructuredBuffer,
		UnorderedAcess,
		Error
	};


	/************************************************************************************************/


	struct Descriptor
	{
		DescHeapEntryType Type;

		union Anonymouse
		{
			Anonymouse() {}
			ConstantBufferHandle		ConstantBuffer;
			ID3D12Resource*				Resource_ptr;
			D3D12_CPU_DESCRIPTOR_HANDLE CPU_HeapPOS;
			D3D12_GPU_DESCRIPTOR_HANDLE GPU_HeapPOS;
		}Value;
	};


	/************************************************************************************************/


	struct HeapDescriptor
	{
		uint32_t				Register	= -1;
		uint32_t				Count		= 0;
		uint32_t				Space		= 0;
		DescHeapEntryType		Type		= DescHeapEntryType::HeapError;
	};


	/************************************************************************************************/


	template<size_t ENTRYCOUNT = 16>
	class DesciptorHeapLayout
	{
	public:
		DesciptorHeapLayout() {}

		template<size_t RHS_SIZE>
		DesciptorHeapLayout(const DesciptorHeapLayout<RHS_SIZE>& RHS)
		{
			FK_ASSERT(ENTRYCOUNT >= RHS.size());

			Entries = RHS.Entries;

#ifdef _DEBUG
			Check();
#endif
		}

		bool SetParameterAsCBV(
			size_t Index, size_t BaseRegister, size_t RegisterCount, size_t RegisterSpace = 0,
			size_t BufferTag = -1)
		{
			HeapDescriptor Desc;
			Desc.Register = BaseRegister;
			Desc.Space    = RegisterSpace;
			Desc.Type     = DescHeapEntryType::ConstantBuffer;
			Desc.Count	  = RegisterCount;

			if (Entries.size() <= Index)
			{
				if (!Entries.full())
					Entries.resize(Index + 1);
				else
					return false;
			}
			Entries[Index] = Desc;

			return true;
		}


		bool SetParameterAsSRV(
			size_t Index, size_t BaseRegister, size_t RegisterCount, size_t RegisterSpace = 0,
			size_t BufferTag = -1)
		{
			HeapDescriptor Desc;
			Desc.Register	= BaseRegister;
			Desc.Space		= RegisterSpace;
			Desc.Type		= DescHeapEntryType::ShaderResource;
			Desc.Count		= RegisterCount;

			if (Entries.size() <= Index)
			{
				if (Entries.full())
					return false;

				Entries.resize(Index + 1);
			}

			Entries[Index] = Desc;

			return true;
		}


		bool SetParameterAsShaderUAV(
			size_t Index, size_t BaseRegister, size_t RegisterCount, size_t RegisterSpace = 0,
			size_t BufferTag = -1)
		{
			HeapDescriptor Desc;
			Desc.Register = BaseRegister;
			Desc.Space    = RegisterSpace;
			Desc.Count    = RegisterCount;
			Desc.Type     = DescHeapEntryType::UAVBuffer;

			if (Entries.size() <= Index)
			{
				if (Entries.full())
					return false;

				Entries.resize(Index + 1);
			}

			Entries[Index] = Desc;

			return true;
		}


		bool Check()
		{
			return (!IsXInSet(DescHeapEntryType::HeapError, Entries, [](auto a, auto b) -> bool
				{ return a == b.Type; }));
		}


		const size_t size() const
		{
			size_t out = 0;
			for (auto& e : Entries)
				out += e.Count + e.Space;

			FK_ASSERT(out);

			return out;
		}


		static_vector<HeapDescriptor, ENTRYCOUNT> Entries;
	};


	/************************************************************************************************/


	enum DeviceResourceState
	{
		DRS_Free			 = 0x0003, // Forces any type of transition
		DRS_Read			 = 0x0001,
		DRS_Retire			 = 0x1000,

		DRS_Write			 = 0x0002,
		DRS_Present			 = 0x1005, // Implied Read and Retire
		DRS_RenderTarget	 = 0x0006, // Implied write
		DRS_ShaderResource	 = 0x0009, // Implied Read
		DRS_UAV				 = 0x000A, // Implied Write
		DRS_STREAMOUT		 = 0x000B, // Implied Write
		DRS_STREAMOUTCLEAR	 = 0x000D, // Implied Write
		DRS_VERTEXBUFFER	 = 0x000C, // Implied Read
		DRS_CONSTANTBUFFER	 = 0x000C, // Implied Read
		DRS_DEPTHBUFFER		 = 0x0030,
		DRS_DEPTHBUFFERREAD  = 0x0030 | DRS_Read,
		DRS_DEPTHBUFFERWRITE = 0x0030 | DRS_Write,

		DRS_PREDICATE		 = 0x0015, // Implied Read
		DRS_INDIRECTARGS	 = 0x0019, // Implied Read
		DRS_UNKNOWN			 = 0x0020, 
		DRS_GENERIC			 = 0x0020, 
		DRS_ERROR			 = 0xFFFF 
	};


	/************************************************************************************************/


	inline D3D12_RESOURCE_STATES DRS2D3DState(DeviceResourceState state)
	{
		switch (state)
		{
		case DeviceResourceState::DRS_Present:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT;
		case DeviceResourceState::DRS_RenderTarget:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
		case DeviceResourceState::DRS_ShaderResource:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		case DeviceResourceState::DRS_UAV:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		case DeviceResourceState::DRS_Write:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		case DeviceResourceState::DRS_Read:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE;
		case DeviceResourceState::DRS_VERTEXBUFFER:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		case DeviceResourceState::DRS_Free:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
		case DeviceResourceState::DRS_UNKNOWN:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
		case DeviceResourceState::DRS_INDIRECTARGS:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		case DeviceResourceState::DRS_STREAMOUT:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_STREAM_OUT;
		case DeviceResourceState::DRS_STREAMOUTCLEAR:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
        case DeviceResourceState::DRS_DEPTHBUFFERWRITE:
            return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
        case DeviceResourceState::DRS_DEPTHBUFFERREAD:
            return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ;
		}
		return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
	}

	/************************************************************************************************/


	inline DeviceResourceState D3DState2DRS(D3D12_RESOURCE_STATES state)
	{
		switch (state)
		{
		case D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT:
			return DeviceResourceState::DRS_Present;
		case D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET:
			return DeviceResourceState::DRS_RenderTarget;
		case D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
			return DeviceResourceState::DRS_ShaderResource;
		case D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS:
			return DeviceResourceState::DRS_UAV;
		case D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST:
			return DeviceResourceState::DRS_Write;
		case D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE:
			return DeviceResourceState::DRS_Read;
		case D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER:
			return DeviceResourceState::DRS_VERTEXBUFFER;
		}

		return DeviceResourceState::DRS_GENERIC;
	}


	/************************************************************************************************/


	FLEXKITAPI class DescriptorHeap
	{
	public:
		DescriptorHeap() {}
		DescriptorHeap(Context& RS, const DesciptorHeapLayout<16>& Layout_IN, iAllocator* TempMemory);

        // moveable
        DescriptorHeap(DescriptorHeap&& rhs);
        DescriptorHeap& operator = (DescriptorHeap&&);

		DescriptorHeap& Init		(Context& ctx, const DesciptorHeapLayout<16>& Layout_IN, iAllocator* TempMemory);
		DescriptorHeap& Init		(Context& ctx, const DesciptorHeapLayout<16>& Layout_IN, const size_t reserveCount, iAllocator* TempMemory);
        DescriptorHeap& Init2       (Context& ctx, const DesciptorHeapLayout<16>& Layout_IN, const size_t reserveCount, iAllocator* TempMemory); // for variable size heap layouts
		DescriptorHeap& NullFill	(Context& ctx, const size_t end = -1);

		bool SetCBV					(Context& ctx, size_t idx, ConstantBufferHandle	Handle, size_t offset, size_t bufferSize);
		bool SetSRV					(Context& ctx, size_t idx, ResourceHandle		Handle);
		bool SetSRV					(Context& ctx, size_t idx, ResourceHandle		Handle, FORMAT_2D format);
		bool SetSRV					(Context& ctx, size_t idx, UAVTextureHandle		Handle);
		bool SetSRV					(Context& ctx, size_t idx, UAVResourceHandle	Handle);
        bool SetSRVCubemap          (Context& ctx, size_t idx, ResourceHandle		Handle);
		bool SetUAV					(Context& ctx, size_t idx, UAVResourceHandle	Handle);
		bool SetUAV					(Context& ctx, size_t idx, UAVTextureHandle		Handle);
		bool SetStructuredResource	(Context& ctx, size_t idx, ResourceHandle		Handle, size_t stride);

		operator D3D12_GPU_DESCRIPTOR_HANDLE () const { return descriptorHeap.V2; } // TODO: FIX PAIRS SO AUTO CASTING WORKS

		DescriptorHeap	GetHeapOffsetted(size_t offset, Context& ctx) const;

	private:

        // not publically copyable
        //DescriptorHeap(const DescriptorHeap&) = default;
        //DescriptorHeap& operator = (const DescriptorHeap&) = default;

        DescriptorHeap Clone() const
        {
            DescriptorHeap heap;
            heap.descriptorHeap     = descriptorHeap;
            heap.Layout             = Layout;
            heap.FillState          = FillState;

            return heap;
        }


		static bool CheckType(const DesciptorHeapLayout<>& layout, DescHeapEntryType type, size_t idx);

		DescHeapPOS						descriptorHeap;
		const DesciptorHeapLayout<>*	Layout;
		Vector<bool>					FillState;
	};


	/************************************************************************************************/


	FLEXKITAPI class RootSignature
	{
	public:
		RootSignature(iAllocator* Memory) :
			Signature(nullptr),
			Heaps(Memory)
		{
		}


		~RootSignature()
		{
			Release();
		}

		operator ID3D12RootSignature* () { return Signature; }

		void Release()
		{
			if (Signature)
				Signature->Release();

			Signature = nullptr;
			Heaps.Release();
		}


		template<size_t SIZE>
		bool SetParameterAsDescriptorTable(
			size_t Index, DesciptorHeapLayout<SIZE>& Layout, size_t Tag = -1, PIPELINE_DESTINATION AccessableStages = PIPELINE_DESTINATION::PIPELINE_DEST_ALL)
		{
			RootEntry Desc;
			Desc.Type							= RootSignatureEntryType::DescriptorHeap;
			Desc.DescriptorHeap.HeapIdx			= Heaps.size();
			Desc.DescriptorHeap.Accessibility	= AccessableStages;
			Heaps.push_back({ Index, Layout });

			if (RootEntries.size() <= Index)
			{
				if (!RootEntries.full()) {
					RootEntries.resize(Index + 1);
					Tags.resize(Index + 1);
				}
				else
					return false;
			}

			RootEntries[Index] = Desc;
			Tags[Index]        = Tag;

			return false;
		}


		bool SetParameterAsCBV(
			size_t Index, size_t Register, size_t RegisterSpace, 
			PIPELINE_DESTINATION AccessableStages, size_t Tag = -1)
		{
			RootEntry Desc;
			Desc.Type							= RootSignatureEntryType::ConstantBuffer;
			Desc.ConstantBuffer.Register		= (uint32_t)Register;
			Desc.ConstantBuffer.RegisterSpace	= (uint32_t)RegisterSpace;
			Desc.ConstantBuffer.Accessibility	= AccessableStages;


			if (RootEntries.size() <= Index)
			{
				if (!RootEntries.full())
					RootEntries.resize(Index + 1);
				else
					return false;
			}

			RootEntries[Index] = Desc;
			Tags[Index]        = Tag;

			return false;
		}


		bool SetParameterAsUAV(
			size_t Index, size_t Register, size_t RegisterSpace,
			PIPELINE_DESTINATION AccessableStages, size_t Tag = -1)
		{
			RootEntry Desc;
			Desc.Type							= RootSignatureEntryType::UnorderedAcess;
			Desc.ConstantBuffer.Register		= (uint32_t)Register;
			Desc.ConstantBuffer.RegisterSpace	= (uint32_t)RegisterSpace;
			Desc.ConstantBuffer.Accessibility	= AccessableStages;


			if (RootEntries.size() <= Index)
			{
				if (!RootEntries.full())
					RootEntries.resize(Index + 1);
				else
					return false;
			}

			RootEntries[Index] = Desc;
			Tags[Index]        = Tag;

			return false;
		}


		bool SetParameterAsSRV(
			size_t Index, size_t Register, size_t RegisterSpace = 0,
			PIPELINE_DESTINATION AccessableStages = PIPELINE_DESTINATION::PIPELINE_DEST_ALL,
			size_t Tag = -1)
		{
			RootEntry Desc;
			Desc.Type							= RootSignatureEntryType::StructuredBuffer;
			Desc.ConstantBuffer.Register		= (uint32_t)Register;
			Desc.ConstantBuffer.RegisterSpace	= (uint32_t)RegisterSpace;
			Desc.ConstantBuffer.Accessibility	= AccessableStages;

			if (RootEntries.size() < Index)
			{
				if (!RootEntries.full())
					RootEntries.resize(Index + 1);
				else
					return false;
			}

			RootEntries[Index] = Desc;
			Tags[Index]        = Tag;

			return false;
		}


		bool Build(RenderSystem* RS, iAllocator* TempMemory);

		const DesciptorHeapLayout<16>& GetDescHeap(size_t idx) const { return Heaps[idx].Heap; }


		size_t GetDesciptorTableSize(size_t idx) const
		{
			FK_ASSERT(RootEntries[idx].Type == RootSignatureEntryType::DescriptorHeap, "INVALID ARGUEMENT!");
			auto heapIdx = RootEntries[idx].DescriptorHeap.HeapIdx;

			return  Heaps[heapIdx].Heap.size();
		}


		bool AllowIA	= true;
		bool AllowSO	= false;

	private:
		struct HeapEntry
		{
			size_t					idx;
			DesciptorHeapLayout<16> Heap;
		};

		struct RootEntry
		{
			RootSignatureEntryType Type = RootSignatureEntryType::Error;
			union 
			{
				struct DH
				{
					size_t					HeapIdx;
					PIPELINE_DESTINATION	Accessibility;
				}DescriptorHeap;

				struct CB
				{
					uint32_t				Register;
					uint32_t				RegisterSpace;
					PIPELINE_DESTINATION	Accessibility;
				}ConstantBuffer;

				struct SR
				{
					uint32_t				Register;
					uint32_t				RegisterSpace;
					PIPELINE_DESTINATION	Accessibility;
				}ShaderResource;

				struct UAV
				{
					uint32_t				Register;
					uint32_t				RegisterSpace;
					PIPELINE_DESTINATION	Accessibility;
				}UnorderedAccess;
				// I thought these were going to be different but they all ended up the same
			};
		};

		ID3D12RootSignature*		Signature;
		static_vector<size_t, 12>	Tags;
		Vector<HeapEntry>			Heaps;
		static_vector<RootEntry>	RootEntries;
	};


	struct TextureObject
	{
        TextureObject(ResourceHandle IN_texture) :
            Texture     { IN_texture    },
            UAV         { false         } {}

        TextureObject(UAVTextureHandle IN_UAV) :
            UAVTexture  { IN_UAV        },
            UAV         { true          } {}

        union {
            ResourceHandle	    Texture;
            UAVTextureHandle    UAVTexture;
        };

        const bool UAV = false;
	};


	/************************************************************************************************/


	struct VertexBufferEntry
	{
		VertexBufferHandle	VertexBuffer	= InvalidHandle_t;
		UINT				Stride			= 0;
		UINT				Offset			= 0;
	};

	typedef static_vector<VertexBufferEntry, 16>	VertexBufferList;
	typedef static_vector<ResourceHandle, 16>       RenderTargetList;


	/************************************************************************************************/


	enum IndirectLayoutEntryType
	{
		ILE_DrawCall,
		ILE_UpdateVBBindings,
		ILE_UNKNOWN,
	};

	class IndirectDrawDescription
	{
	public:
		IndirectDrawDescription(IndirectLayoutEntryType IN_type = ILE_UNKNOWN) : type{IN_type} {}

		IndirectLayoutEntryType type;
	};


	FLEXKITAPI class IndirectLayout
	{
	public:
		IndirectLayout() noexcept :
			entries		{ nullptr },
			signature	{ nullptr }{}

		IndirectLayout(ID3D12CommandSignature* IN_signature, size_t IN_stride, Vector<IndirectDrawDescription>&& IN_Entries) noexcept :
			signature	{ IN_signature },
			stride		{ IN_stride },
			entries		{ std::move(IN_Entries) } {}

		~IndirectLayout() noexcept
		{
			if (signature)
				signature->Release();
		}


		IndirectLayout(const IndirectLayout& rhs) noexcept :
			signature	{ rhs.signature },
			entries		{ rhs.entries	},
			stride		{ rhs.stride	}
		{
			if(signature)
				signature->AddRef();
		}

		IndirectLayout& operator =	(const IndirectLayout& rhs) noexcept
		{
			if(rhs.signature)
				rhs.signature->AddRef();

			if (signature)
				signature->Release();

			signature	= rhs.signature;
			stride		= rhs.stride;
			entries		= rhs.entries;

			return (*this);
		}

		operator bool () noexcept { return signature != nullptr; }

		ID3D12CommandSignature*			signature	= nullptr;
		size_t							stride		= 0;
		Vector<IndirectDrawDescription>	entries;
	};


	/************************************************************************************************/


	struct UploadSegment 
	{
		size_t	offset		= 0;
		size_t	uploadSize	= 0;
		char*	buffer		= nullptr;
	};


    template<size_t I = 0, typename TY_Tuple, typename FN>
    void Tuple_For(TY_Tuple& tuple, FN fn)
    {
        constexpr size_t end = std::tuple_size_v<TY_Tuple>;

        if constexpr (I < end) {
            fn(std::get<I>(tuple));
            Tuple_For<I + 1>(tuple, fn);
        }
    }


	FLEXKITAPI class Context
	{
	public:
		Context(RenderSystem*				renderSystem_IN	= nullptr, 
				iAllocator*					allocator		= nullptr);
			
		Context(Context&& RHS)
		{
			DeviceContext			= RHS.DeviceContext;
			CurrentRootSignature	= RHS.CurrentRootSignature;
			CurrentPipelineState	= RHS.CurrentPipelineState;
			renderSystem			= RHS.renderSystem;

            RTV_CPU = RHS.RTV_CPU;
            RTV_GPU = RHS.RTV_GPU;

            SRV_CPU = RHS.SRV_CPU;
            SRV_GPU = RHS.SRV_GPU;

            DSV_CPU = RHS.DSV_CPU;
            DSV_GPU = RHS.DSV_GPU;

            descHeapRTV = RHS.descHeapRTV;
            descHeapSRV = RHS.descHeapSRV;
            descHeapDSV = RHS.descHeapDSV;

			RenderTargetCount		= RHS.RenderTargetCount;
			DepthStencilEnabled		= RHS.DepthStencilEnabled;

			Viewports				= RHS.Viewports;
			DesciptorHeaps			= RHS.DesciptorHeaps;
			VBViews					= RHS.VBViews;
			PendingBarriers			= RHS.PendingBarriers;
			Memory					= RHS.Memory;
            commandAllocator        = RHS.commandAllocator;

			// Null out old Context
            RHS.commandAllocator        = nullptr;
			RHS.DeviceContext			= nullptr;
			RHS.CurrentRootSignature	= nullptr;

			RHS.RenderTargetCount	    = 0;
			RHS.DepthStencilEnabled     = false;
			RHS.Memory				    = nullptr;

			RHS.Viewports.clear();
			RHS.DesciptorHeaps.clear();
			RHS.VBViews.clear();
			RHS.PendingBarriers.clear();

            RHS.RTV_CPU = { 0 };
            RHS.RTV_GPU = { 0 };

            RHS.SRV_CPU = { 0 };
            RHS.SRV_GPU = { 0 };

            RHS.DSV_CPU = { 0 };
            RHS.DSV_GPU = { 0 };

            RHS.descHeapRTV = nullptr;
            RHS.descHeapSRV = nullptr;
            RHS.descHeapDSV = nullptr;
		}


		Context& operator = (Context&& RHS)// Moves only
		{
			DeviceContext			= RHS.DeviceContext;
			CurrentRootSignature	= RHS.CurrentRootSignature;
			CurrentPipelineState	= RHS.CurrentPipelineState;
			renderSystem			= RHS.renderSystem;

            RTV_CPU = RHS.RTV_CPU;
            RTV_GPU = RHS.RTV_GPU;

            SRV_CPU = RHS.SRV_CPU;
            SRV_GPU = RHS.SRV_GPU;

            DSV_CPU = RHS.DSV_CPU;
            DSV_GPU = RHS.DSV_GPU;

            descHeapRTV = RHS.descHeapRTV;
            descHeapSRV = RHS.descHeapSRV;
            descHeapDSV = RHS.descHeapDSV;

			RenderTargetCount		= RHS.RenderTargetCount;
			DepthStencilEnabled		= RHS.DepthStencilEnabled;

			Viewports				= RHS.Viewports;
			DesciptorHeaps			= RHS.DesciptorHeaps;
			VBViews					= RHS.VBViews;
			PendingBarriers			= RHS.PendingBarriers;
			Memory					= RHS.Memory;
            commandAllocator        = RHS.commandAllocator;

			// Null out old Context
            RHS.commandAllocator        = nullptr;
			RHS.DeviceContext			= nullptr;
			RHS.CurrentRootSignature	= nullptr;

			RHS.RenderTargetCount	    = 0;
			RHS.DepthStencilEnabled     = false;
			RHS.Memory				    = nullptr;

			RHS.Viewports.clear();
			RHS.DesciptorHeaps.clear();
			RHS.VBViews.clear();
			RHS.PendingBarriers.clear();

            RHS.RTV_CPU = { 0 };
            RHS.RTV_GPU = { 0 };

            RHS.SRV_CPU = { 0 };
            RHS.SRV_GPU = { 0 };

            RHS.DSV_CPU = { 0 };
            RHS.DSV_GPU = { 0 };

            RHS.descHeapRTV = nullptr;
            RHS.descHeapSRV = nullptr;
            RHS.descHeapDSV = nullptr;

			return *this;
		}


		Context				(const Context& RHS) = delete;
		Context& operator = (const Context& RHS) = delete;

        void Release();

		void AddUAVBarrier			(UAVResourceHandle, DeviceResourceState, DeviceResourceState);
		void AddUAVBarrier			(UAVTextureHandle,	DeviceResourceState, DeviceResourceState);

		void AddPresentBarrier			(ResourceHandle Handle,	DeviceResourceState Before);
		void AddRenderTargetBarrier		(ResourceHandle Handle,	DeviceResourceState Before, DeviceResourceState State = DeviceResourceState::DRS_RenderTarget);
		void AddStreamOutBarrier		(SOResourceHandle,		DeviceResourceState Before, DeviceResourceState State);
		void AddShaderResourceBarrier	(ResourceHandle Handle,	DeviceResourceState Before, DeviceResourceState State);


		void ClearDepthBuffer		(ResourceHandle Texture, float ClearDepth = 0.0f); // Assumes full-screen Clear
		void ClearRenderTarget		(ResourceHandle Texture, float4 ClearColor = float4(0.0f)); // Assumes full-screen Clear

		void SetRootSignature		(RootSignature& RS);
		void SetComputeRootSignature(RootSignature& RS);
		void SetPipelineState		(ID3D12PipelineState* PSO);

		void SetRenderTargets		(const static_vector<ResourceHandle> RTs, bool DepthStecil, ResourceHandle DepthStencil = InvalidHandle_t);

		void SetViewports			(static_vector<D3D12_VIEWPORT, 16>	VPs);
		void SetScissorRects		(static_vector<D3D12_RECT, 16>		Rects);

		void SetScissorAndViewports	(static_vector<ResourceHandle, 16>	RenderTargets);

        template<typename ... ARGS>
        void SetScissorAndViewports(std::tuple<ARGS...>	RenderTargets)
        {
            static_vector<D3D12_VIEWPORT, 16>	VPs;
            static_vector<D3D12_RECT, 16>		Rects;

            Tuple_For(
                RenderTargets,
                [&](auto target)
                {
                    const auto WH = renderSystem->GetTextureWH(target);
                    VPs.push_back(  { 0,0, (FLOAT)WH[0], (FLOAT)WH[1], 0, 1 });
                    Rects.push_back({ 0,0, (LONG)WH[0], (LONG)WH[1] });
                });

            SetViewports(VPs);
            SetScissorRects(Rects);
        }


		void SetDepthStencil		(ResourceHandle DS);
		void SetPrimitiveTopology	(EInputTopology Topology);

		void NullGraphicsConstantBufferView	(size_t idx);
		void SetGraphicsConstantBufferView	(size_t idx, const ConstantBufferHandle CB, size_t Offset = 0);
		void SetGraphicsConstantBufferView	(size_t idx, const ConstantBufferDataSet& CB);
		void SetGraphicsConstantBufferView	(size_t idx, const ConstantBuffer& CB);
		void SetGraphicsDescriptorTable		(size_t idx, const DescriptorHeap& DH);
		void SetGraphicsShaderResourceView	(size_t idx, FrameBufferedResource* Resource, size_t Count, size_t ElementSize);
		void SetGraphicsShaderResourceView	(size_t idx, Texture2D& Texture);

		void SetComputeDescriptorTable		(size_t idx, const DescriptorHeap& DH);
		void SetComputeConstantBufferView	(size_t idx, const ConstantBufferHandle, size_t offset);
		void SetComputeShaderResourceView	(size_t idx, Texture2D&			Texture);
		void SetComputeUnorderedAccessView	(size_t idx, UAVResourceHandle& Texture);

		void BeginQuery	(QueryHandle query, size_t idx);
		void EndQuery	(QueryHandle query, size_t idx);

		void CopyBufferRegion(
			static_vector<ID3D12Resource*>		sources,
			static_vector<size_t>				sourceOffset,
			static_vector<ID3D12Resource*>		destinations,
			static_vector<size_t>				destinationOffset,
			static_vector<size_t>				copySize,
			static_vector<DeviceResourceState>	currentStates,
			static_vector<DeviceResourceState>	finalStates);

		void ImmediateWrite(
			static_vector<UAVResourceHandle>	handles,
			static_vector<size_t>				value,
			static_vector<DeviceResourceState>	currentStates,
			static_vector<DeviceResourceState>	finalStates);

		void ClearSOCounters(static_vector<SOResourceHandle> handles);

		void CopyUInt64(
			static_vector<ID3D12Resource*>			source,
			static_vector<DeviceResourceState>		sourceState,
			static_vector<size_t>					sourceoffsets,
			static_vector<ID3D12Resource*>			destination,
			static_vector<DeviceResourceState>		destinationState,
			static_vector<size_t>					destinationoffset);

		void AddIndexBuffer			(TriMesh* Mesh);
		void AddVertexBuffers		(TriMesh* Mesh, static_vector<VERTEXBUFFER_TYPE, 16> Buffers, VertexBufferList* InstanceBuffers = nullptr);
		void SetVertexBuffers		(VertexBufferList&	List);
		void SetVertexBuffers		(VertexBufferList	List);
		void SetVertexBuffers2		(static_vector<D3D12_VERTEX_BUFFER_VIEW>	List);

		void SetSOTargets		(static_vector<D3D12_STREAM_OUTPUT_BUFFER_VIEW, 4> SOViews);

		void Draw					(size_t VertexCount, size_t BaseVertex = 0);
		void DrawIndexed			(size_t IndexCount, size_t IndexOffet = 0, size_t BaseVertex = 0);
		void DrawIndexedInstanced	(size_t IndexCount, size_t IndexOffet = 0, size_t BaseVertex = 0, size_t InstanceCount = 1, size_t InstanceOffset = 0);
		void Clear					();

		void ResolveQuery			(QueryHandle query, size_t begin, size_t end, UAVResourceHandle destination, size_t destOffset);

		void ExecuteIndirect		(UAVResourceHandle args, const IndirectLayout& layout, size_t argumentBufferOffset = 0, size_t executionCount = 1);
		void Dispatch				(uint3);

		void FlushBarriers();

		void SetPredicate(bool Enable, QueryHandle Handle = {}, size_t = 0);

		void CopyBuffer		(const UploadSegment src, UAVResourceHandle destination);
		void CopyBuffer		(const UploadSegment src, size_t uploadSize, UAVResourceHandle destination);
		void CopyBuffer		(const UploadSegment src, ResourceHandle destination);
		void CopyTexture2D	(const UploadSegment src, ResourceHandle destination, uint2 BufferSize);

		void SetRTRead	(ResourceHandle Handle);
		void SetRTWrite	(ResourceHandle Handle);
		void SetRTFree	(ResourceHandle Handle);

        void Close();
        void Reset();
        void LockFor(uint8_t frameCount);

        bool _isLocked();
        void _DecrementLock();

		// Not Yet Implemented
		void SetUAVRead();
		void SetUAVWrite();
		void SetUAVFree();

        DescHeapPOS _ReserveDSV(size_t count);
        DescHeapPOS _ReserveSRV(size_t count);
        DescHeapPOS _ReserveRTV(size_t count);

        void        _ResetRTV();
        void        _ResetDSV();
        void        _ResetSRV();


		ID3D12GraphicsCommandList*	GetCommandList() { return DeviceContext; }

        RenderSystem* renderSystem = nullptr;

	private:

		void _AddBarrier(ID3D12Resource* resource, DeviceResourceState currentState, DeviceResourceState newState);

		struct Barrier
		{
			Barrier() {}

			/*
			Barrier(const Barrier& rhs) 
			{
				Type		= rhs.Type;
				NewState	= rhs.NewState;
				OldState	= rhs.OldState;

				memcpy(&resource, &rhs.resource, sizeof(resource));
			}
			*/

			DeviceResourceState OldState;
			DeviceResourceState NewState;

			enum BarrierType
			{
				BT_RenderTarget,
				BT_ConstantBuffer,
				BT_UAVBuffer,
				BT_UAVTexture,
				BT_QueryBuffer,
				BT_VertexBuffer,
				BT_StreamOut,
				BT_ShaderResource,
				BT_Generic,
				BT_
			}Type;

			union 
			{
				UAVResourceHandle	UAVBuffer;
				UAVTextureHandle	UAVTexture;
				ResourceHandle		renderTarget;
				ResourceHandle		shaderResource;
				SOResourceHandle	streamOut;
				ID3D12Resource*		resource;
				QueryHandle			query;
			};
		};


		void UpdateResourceStates();


        ID3D12CommandAllocator*         commandAllocator        = nullptr;
		ID3D12GraphicsCommandList3*		DeviceContext			= nullptr;

		RootSignature*					CurrentRootSignature	= nullptr;
		ID3D12PipelineState*			CurrentPipelineState	= nullptr;

        ID3D12DescriptorHeap*           descHeapRTV             = nullptr;
        ID3D12DescriptorHeap*           descHeapSRV             = nullptr;
        ID3D12DescriptorHeap*           descHeapDSV             = nullptr;

        D3D12_CPU_DESCRIPTOR_HANDLE RTV_CPU;
        D3D12_GPU_DESCRIPTOR_HANDLE RTV_GPU;

        D3D12_CPU_DESCRIPTOR_HANDLE SRV_CPU;
        D3D12_GPU_DESCRIPTOR_HANDLE SRV_GPU;

        D3D12_CPU_DESCRIPTOR_HANDLE DSV_CPU;
        D3D12_GPU_DESCRIPTOR_HANDLE DSV_GPU;

        D3D12_CPU_DESCRIPTOR_HANDLE RTVPOSCPU;
        D3D12_CPU_DESCRIPTOR_HANDLE DSVPOSCPU;

		size_t	RenderTargetCount;
		bool	DepthStencilEnabled;

		static_vector<ResourceHandle, 16>		RenderTargets;
		static_vector<D3D12_VIEWPORT, 16>		Viewports;
		static_vector<DescriptorHeap*>			DesciptorHeaps;
		static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;

		struct StreamOutResource {
			SOResourceHandle	handle;
		};

        static_vector<StreamOutResource, 128>   TrackedSOBuffers;
        static_vector<Barrier, 128>				PendingBarriers;

        uint8_t                                 lockCounter = 0;
		iAllocator*								Memory;
	};


	/************************************************************************************************/


	FLEXKITAPI class VertexBufferStateTable
	{
	public:

		VertexBufferStateTable(iAllocator* memory) :
			Buffers(memory),
			Handles(memory, GetTypeGUID(VertexBuffer)),
			FreeBuffers(memory),
			UserBuffers(memory)
		{}

		~VertexBufferStateTable()
		{
			Release();
		}

		struct SubAllocation
		{
			char*	Data;
			size_t	offsetBegin;
			size_t	reserveSize;
		};

		VertexBufferHandle	CreateVertexBuffer	(size_t BufferSize, bool GPUResident, RenderSystem* RS); // Creates Using Placed Resource
		bool				PushVertex			(VertexBufferHandle Handle, void* _ptr, size_t ElementSize);

		void				LockUntil	(size_t Frame);// Locks all in use Buffers until given Frame
		void				Reset		(VertexBufferHandle Handle);

		ID3D12Resource*		GetAsset						(VertexBufferHandle Handle);
		size_t				GetCurrentVertexBufferOffset	(VertexBufferHandle Handle) const;
		size_t				GetBufferSize					(VertexBufferHandle Handle) const;

		SubAllocation		Reserve							(VertexBufferHandle Handle, size_t size) noexcept;


		void				Release();
		void				ReleaseVertexBuffer(VertexBufferHandle Handle);
        void                ReleaseFree();

	private:
		typedef size_t VBufferHandle;

		VBufferHandle	CreateVertexBufferResource(size_t BufferSize, bool GPUResident, RenderSystem* RS); // Creates Using Placed Resource

		bool    CurrentlyAvailable(VertexBufferHandle Handle, size_t CurrentFrame) const;

        char*   _Map(VBufferHandle);
        void    _UnMap(VBufferHandle, size_t range);

		struct VertexBuffer
		{
			ID3D12Resource* Resource		= nullptr;
			size_t			ResourceSize	= 0;
			char			lockCounter;
		};


		struct UserVertexBuffer
		{
			size_t			    CurrentBuffer;
			size_t			    Buffers[3];					// Current Buffer
			size_t			    ResourceSize;				// Requested Size
			size_t			    Offset			 = 0;		// Current Head for Push Buffers
			char*			    MappedPtr		 = 0;		//
			ID3D12Resource*     Resource		 = nullptr; // To avoid some reads
			bool			    WrittenTo		 = false;
            VertexBufferHandle  handle;

			void IncrementCurrentBuffer()
			{
				CurrentBuffer = ++CurrentBuffer % 3;
			}


			size_t GetCurrentBuffer() const
			{
				return Buffers[CurrentBuffer];
			}
		};


		struct FreeVertexBuffer
		{
			size_t Size      = 0;
			size_t BufferIdx = 0;

            operator size_t () { return BufferIdx; }
		};

		Vector<VertexBuffer>		Buffers;
		Vector<UserVertexBuffer>	UserBuffers;
		Vector<FreeVertexBuffer>	FreeBuffers;

		HandleUtilities::HandleTable<VertexBufferHandle, 32>	Handles;

        std::mutex criticalSection;
	};


	/************************************************************************************************/


	FLEXKITAPI struct ConstantBufferTable
	{
		ConstantBufferTable(iAllocator* allocator, RenderSystem* IN_renderSystem) :
			handles         { allocator         },
            renderSystem    { IN_renderSystem   },
            buffers         { allocator         }
		{}

        ~ConstantBufferTable()
        {
            Release();
        }

        void Release()
        {
            for (auto& CB : buffers)
            {
                for (auto R : CB.resources)
                {
                    if (R)
                        R->Release();

                    R = nullptr;
                }
            }


            buffers.Release();
        }

		struct SubAllocation
		{
			char*	Data;
			size_t	offsetBegin;
			size_t	reserveSize;
		};

		struct UserConstantBuffer
		{
			uint32_t	size;
			uint32_t	offset;
			void*		mapped_ptr;

			bool GPUResident;
			bool writeFlag;

            ID3D12Resource*         resources[3];
			uint8_t                 currentRes;
			char                    locks[3];

            ConstantBufferHandle    handle;
		};



		ConstantBufferHandle	CreateConstantBuffer	(size_t BufferSize, bool GPUResident = false);
		void					ReleaseBuffer			(ConstantBufferHandle Handle);

		ID3D12Resource*			GetDeviceResource		(const ConstantBufferHandle Handle) const;
		size_t					GetBufferOffset			(const ConstantBufferHandle Handle) const;

		size_t					AlignNext               (ConstantBufferHandle Handle);
		bool					Push			        (ConstantBufferHandle Handle, void* _Ptr, size_t PushSize);
		
		SubAllocation			Reserve					(ConstantBufferHandle Handle, size_t Size);

		void					LockFor(uint8_t frameCount);
        void                    DecrementLocks();

	private:
		void					UpdateCurrentBuffer(ConstantBufferHandle Handle);

		RenderSystem*				renderSystem;
		Vector<UserConstantBuffer>	buffers;
        std::mutex                  criticalSection;

		HandleUtilities::HandleTable<ConstantBufferHandle>	handles;
	};


	/************************************************************************************************/


	enum class QueryType
	{
		OcclusionQuery,
		PipelineStats,
	};

	// WARNING, NOT IMPLEMENTED FULLY!
	FLEXKITAPI struct QueryTable
	{
		QueryTable(iAllocator* Memory, RenderSystem* RS_in) :
			users{Memory},
			resources{Memory},
			RS{RS_in}{}


		~QueryTable(){}


		QueryHandle			CreateQueryBuffer	(size_t Count, QueryType type);
		QueryHandle			CreateSOQueryBuffer	(size_t count, size_t SOIndex);
		void				LockUntil			(QueryHandle, size_t FrameID);


		void				SetUsed		(QueryHandle Handle)
		{
			users[Handle].used = true;
		}


		DeviceResourceState GetAssetState(const QueryHandle handle) const
		{
			return D3DState2DRS(_GetAssetState(handle));
		}

		D3D12_RESOURCE_STATES _GetAssetState(const QueryHandle handle) const
		{
			auto& res = resources[users[handle].resourceIdx];
			return res.resourceState[res.currentResource];
		}

		ID3D12QueryHeap*	GetAsset	(QueryHandle handle)
		{
			auto& res = resources[users[handle].resourceIdx];

			return res.resources[res.currentResource];
		}

		D3D12_QUERY_TYPE	GetType(QueryHandle handle)
		{
			auto& res = resources[users[handle].resourceIdx];

			return res.type;
		}


		struct UserEntry
		{
			size_t resourceIdx;
			size_t resourceSize;
			size_t currentOffset;
			bool   used;
		};


		struct ResourceEntry
		{
			ID3D12QueryHeap*		resources[3];
			D3D12_RESOURCE_STATES	resourceState[3];
			size_t					resourceLocks[3];
			size_t					currentResource;
			D3D12_QUERY_TYPE		type;
		};

		RenderSystem*			RS;
		Vector<UserEntry>		users;
		Vector<ResourceEntry>	resources;
	};


	/************************************************************************************************/


	enum TextureFlags
	{
		TF_NONE			= 0x00,
		TF_INUSE		= 0x01,
		TF_RenderTarget = 0x02,
		TF_BackBuffer	= 0x04,
		TF_DepthBuffer	= 0x08,
	};


    enum class TextureDimension
    {
        Buffer,
        Texture1D,
        Texture2D,
        Texture2DArray,
        Texture3D,
        TextureCubeMap,
    };


    D3D12_RTV_DIMENSION _Dimension2DeviceRTVDimension(TextureDimension dimension)
    {
        switch (dimension)
        {
        case TextureDimension::Buffer:
            return D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_BUFFER;
        case TextureDimension::Texture1D:
            return D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE1D;
        case TextureDimension::Texture2D:
            return D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;
        case TextureDimension::Texture2DArray:
        case TextureDimension::TextureCubeMap:
            return D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        case TextureDimension::Texture3D:
            return D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE3D;
        default:
            return D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_UNKNOWN;
        }
    }


	/************************************************************************************************/


    struct GPUResourceDesc
    {
        bool renderTarget   = false;
        bool depthTarget    = false;
        bool unordered      = false;
        bool buffered       = false;
        bool shaderResource = false;
        bool structured     = false;
        bool CVFlag         = false; // Assumed true if render target is true
        bool backBuffer     = false;
        bool PreCreated     = false;

        TextureDimension    Dimensions   = TextureDimension::Texture2D;
        uint8_t             MipLevels    = 1;
        uint8_t             bufferCount  = 0;

        uint2       WH;
        FORMAT_2D   format;

        uint8_t             arraySize  = 1;

        union
        {
            ID3D12Resource**    resources;
            byte*               initial = nullptr;
        };


        static GPUResourceDesc RenderTarget(uint2 IN_WH, FORMAT_2D IN_format)
        {
            return {
                true,   // render target flag
                false,  // depth target flag
                false,  // UAV Resource flag
                true,   // buffered flag
                false,  // shader resource flag
                false,  // structured flag
                true,   // clear value
                false,  // back buffer
                false,  // PreCreated

                TextureDimension::Texture2D, // dimensions
                1, // mip count
                3, // buffere count

                IN_WH, IN_format
            };
        }


        static GPUResourceDesc DepthTarget(uint2 IN_WH, FORMAT_2D IN_format)
        {
            return {
                false,   // render target flag
                true,   // depth target flag
                false,   // UAV Resource flag
                true,   // buffered flag
                false,  // shader resource flag
                false,  // structured flag
                true,   // clear value
                false,  // back buffer
                false,  // PreCreated

                TextureDimension::Texture2D, // dimensions
                1, // mip count
                3, // buffere count

                IN_WH, IN_format
            };
        }


        static GPUResourceDesc ShaderResource(uint2 IN_WH, FORMAT_2D IN_format, uint8_t mipCount = 1)
        {
            return {
                false, // render target flag
                false, // depth target flag
                false, // UAV Resource flag
                false, // buffered flag
                false, // shader resource flag
                false, // structured flag
                false, // clear value
                false, // back buffer
                false, // PreCreated

                TextureDimension::Texture2D,          // dimensions
                mipCount,   // mip count
                1,          // buffere count

                IN_WH, IN_format

            };
        }

        static GPUResourceDesc StructuredResource(const uint32_t bufferSize)
        {
            return {
                false,  // render target flag
                false,  // depth target flag
                true,   // UAV Resource flag
                false,  // buffered flag
                false,  // shader resource flag
                true,   // structured flag
                false,  // clear value
                false,  // back buffer
                false,  // created

                TextureDimension::Texture1D, // dimensions
                1, // mip count
                1, // buffered count

                { bufferSize, 1 }
            };
        }


        static GPUResourceDesc UAVResource(uint2 IN_WH, FORMAT_2D IN_format, bool renderTarget = false)
        {
            return {
                false,  // render target flag
                false,  // depth target flag
                true,   // UAV Resource flag
                false,  // buffered flag
                false,  // shader resource flag
                true,   // structured flag
                false,  // clear value
                false,  // back buffer
                false,  // created
                
                TextureDimension::Texture1D, // dimensions
                1, // mip count
                1, // buffered count

                IN_WH, IN_format
            };
        }


        static GPUResourceDesc BackBuffered(uint2 WH, FORMAT_2D format, ID3D12Resource** sources, const uint8_t resourceCount)
        {
            GPUResourceDesc desc = {
                true,   // render target flag
                false,  // depth target flag
                false,  // UAV Resource flag
                true,   // buffered flag
                false,  // shader resource flag
                false,  // structured flag
                true,   // clear value
                true,   // back buffer
                false,  // created

                TextureDimension::Texture2D, // dimensions
                1, // mip count
                resourceCount, // buffer count

                WH, format
            };

            desc.resources = sources;

            return desc;
        }


        static GPUResourceDesc DDS(uint2 WH, FORMAT_2D format, uint8_t mipCount, TextureDimension dimensions)
        {
             return {
                false,  // render target flag
                false,  // depth target flag
                false,  // UAV Resource flag
                false,  // buffered flag
                false,  // shader resource flag
                false,  // structured flag
                false,  // clear value
                false,  // back buffer
                false,  // created

                dimensions, // dimensions
                mipCount,   // mip count
                0,          // buffer count

                WH, format
            };
        }

        static GPUResourceDesc BuildFromMemory(GPUResourceDesc& format, ID3D12Resource** sources, const uint32_t resourceCount)
        {
            GPUResourceDesc desc    = format;
            desc.PreCreated         = true;
            desc.resources          = sources;
            desc.bufferCount        = resourceCount;

            return desc;
        }

        static GPUResourceDesc CubeMap(uint2 WH, FORMAT_2D format, uint8_t mipCount, bool renderTarget)
        {
             return {
                renderTarget,   // render target flag
                false,          // depth target flag
                false,          // UAV Resource flag
                false,          // buffered flag
                false,          // shader resource flag
                false,          // structured flag
                false,          // clear value
                false,          // back buffer
                false,          // created

                TextureDimension::TextureCubeMap,          // dimensions
                mipCount,   // mip count
                1,          // buffer count

                WH,
                format,
                6           // Array Size
            };
        }
    };


	FLEXKITAPI class TextureStateTable
	{
	public:
		TextureStateTable(iAllocator* allocator) :
			Handles		        { allocator },
			UserEntries	        { allocator },
			Resources	        { allocator },
            BufferedResources   { allocator }{}


		~TextureStateTable()
		{
			Release();
		}

		void Release();

		// No Copy
		TextureStateTable				(const TextureStateTable&) = delete;
		TextureStateTable& operator =	(const TextureStateTable&) = delete;

		Texture2D		operator[]		(ResourceHandle Handle);

		ResourceHandle	AddResource		(const GPUResourceDesc& Desc, const DeviceResourceState InitialState);
		void			SetState		(ResourceHandle Handle, DeviceResourceState State);

		uint2			GetWH(ResourceHandle Handle) const;
		size_t			GetFrameGraphIndex(ResourceHandle Texture, size_t FrameID) const;
		void			SetFrameGraphIndex(ResourceHandle Texture, size_t FrameID, size_t Index);

		DXGI_FORMAT		    GetFormat(ResourceHandle handle) const;
        TextureDimension    GetDimension(ResourceHandle) const;
        size_t              GetArraySize(ResourceHandle) const;

		uint32_t		GetTag(ResourceHandle Handle) const;
		void			SetTag(ResourceHandle Handle, uint32_t Tag);

        void            SetBufferedIdx(ResourceHandle handle, uint32_t idx);

		void			MarkRTUsed		(ResourceHandle Handle);

		DeviceResourceState GetState	(ResourceHandle Handle) const;
		ID3D12Resource*		GetAsset	(ResourceHandle Handle) const;


		void ReleaseTexture	(ResourceHandle Handle);
		void LockUntil		(size_t FrameID);

	private:

		struct UserEntry
		{
			size_t				ResourceIdx;
			int64_t				FGI_FrameStamp;
			uint32_t			FrameGraphIndex;
            uint32_t			Tag;
            uint32_t			Flags;
            uint16_t			arraySize;
			ResourceHandle		Handle;
			DXGI_FORMAT			Format;
            TextureDimension    dimension;
		};

		struct ResourceEntry
		{
			void			Release();
			void			SetState(DeviceResourceState State) { States[CurrentResource]		= State;	}
			void			SetFrameLock(size_t FrameID)		{ FrameLocks[CurrentResource]	= FrameID;	}
			ID3D12Resource* GetAsset() const					{ return Resources[CurrentResource];		}
			void			IncreaseIdx()						{ CurrentResource = ++CurrentResource % 3;	}

			size_t				ResourceCount;
			size_t				CurrentResource;
			ID3D12Resource*		Resources[3];
			size_t				FrameLocks[3];
			DeviceResourceState	States[3];
			DXGI_FORMAT			Format;
			uint8_t				mipCount;
			uint2				WH;
		};
			
		Vector<UserEntry>								UserEntries;
        Vector<ResourceEntry>							Resources;
        Vector<ResourceHandle>							BufferedResources;
		HandleUtilities::HandleTable<ResourceHandle, 32>	Handles;
	};


	/************************************************************************************************/


	template<typename TY_Handle, typename TY_Extra = int>
	FLEXKITAPI class UAVResourceTable
	{
	public:
		UAVResourceTable(iAllocator* IN_allocator) :
			resources	{ IN_allocator	},
			handles		{ IN_allocator	}{}


		struct UAVResource
		{
			static_vector<ID3D12Resource*, 3>	resources;	// assumes triple buffering
			size_t								resourceSize;
			DeviceResourceState					resourceStates[3];
			TY_Handle							resourceHandle;
			uint8_t								resourceIdx;
			TY_Extra							extraFields;
		};


		TY_Handle AddResource(static_vector<ID3D12Resource*,3> newResources, size_t resourceSize, DeviceResourceState initialState)
		{
			auto newHandle		= handles.GetNewHandle();
			handles[newHandle]	= 
					resources.push_back({
						newResources,
						resourceSize,
						{initialState, initialState, initialState},
						newHandle,
						0 });

			return newHandle;
		}


		ID3D12Resource*	GetAsset(const TY_Handle handle) const
		{
			auto& resourceEntry = resources[handles[handle]];
			return resourceEntry.resources[resourceEntry.resourceIdx];
		}


		TY_Extra GetExtra(const TY_Handle handle) const
		{
			auto& resourceEntry = resources[handles[handle]];
			return resourceEntry.extraFields;
		}

		void SetExtra(const TY_Handle handle, const TY_Extra& extra) noexcept
		{
			auto& resourceEntry = resources[handles[handle]];
			resourceEntry.extraFields = extra;
		}


		DeviceResourceState	GetAssetState(const TY_Handle handle) const
		{
			auto& resourceEntry = resources[handles[handle]];
			return resourceEntry.resourceStates[resourceEntry.resourceIdx];
		}

		void SetResourceState(TY_Handle handle, DeviceResourceState state)
		{
			auto& resourceEntry = resources[handles[handle]];
			resourceEntry.resourceStates[resourceEntry.resourceIdx] = state;
		}


		void ReleaseResource(TY_Handle handle)
		{
			size_t idx = handles[handle];
			handles.RemoveHandle(handle);

			for (auto resource : resources[idx].resources)
				resource->Release();

            if (resources.size() > 1 && resources.size() < idx)
            {
                resources[idx] = resources.back();
                handles[resources.back().resourceHandle] = idx;
            }

            resources.pop_back();
		}


		void ReleaseAll()
		{
			for (auto resourceEntry : resources) 
			{
				for (auto resource : resourceEntry.resources)
					resource->Release();
			}

			resources.clear();
			handles.Clear();
		}

		HandleUtilities::HandleTable<TY_Handle, 32>			handles;
		Vector<UAVResource>									resources;
		iAllocator*											allocator;
	};


	/************************************************************************************************/


	enum BufferResourceFlags
	{
		UAV_Resource,
		Byte_Buffer,
		TripleBuffer,
	};


	enum BufferDimension
	{
		Resource_1D,
		Resource_2D,
		Resource_3D,
		BYTEBUFFER,
	};

	struct BufferResourceDesc
	{
		union 
		{
			size_t		bufferSize	= 0;
			uint32_t	Width;
			uint32_t	WidthHeight[2];
			uint32_t	WidthHeightDepth[3];
		}resourceSize;

		float4				clearColor;
		uint64_t			flags					= 0;
		uint16_t			mipLevels				= 0;
		BufferDimension		dimensions				= BYTEBUFFER;
		FlexKit::FORMAT_2D	format;
		bool				tripleBuffer			= false;
		bool				useClearValues			= true;
		bool				floatingPointClearValue	= false;
		bool				allowUnorderedAccess	= false;
		bool				allowDepthStencil		= false;

		static BufferResourceDesc ByteBuffer(size_t bufferSize, bool allowUnorderedAccess = true)
		{
			return {};
		}
	};


	/************************************************************************************************/


	FLEXKITAPI class SOResourceTable
	{
	public:
		SOResourceTable(iAllocator* IN_allocator) :
			resources	{ IN_allocator	},
			handles		{ IN_allocator	}{}


		struct SOResource
		{
			static_vector<ID3D12Resource*, 3>	resources;			// assumes triple buffering
			static_vector<ID3D12Resource*, 3>	resourceCounters;	// assumes triple buffering
			size_t								resourceSize;
			DeviceResourceState					resourceStates[3];
			SOResourceHandle					resourceHandle;
			uint8_t								resourceIdx;
		};


		SOResourceHandle AddResource(
			static_vector<ID3D12Resource*,3>	newResources, 
			static_vector<ID3D12Resource*, 3>	counters, 
			size_t resourceSize, 
			DeviceResourceState initialState)
		{
			auto newHandle		= handles.GetNewHandle();
			handles[newHandle]	= 
                    (index_t)resources.push_back({
						newResources,
						counters,
						resourceSize,
						{initialState, initialState, initialState},
						newHandle,
                        0 });

			return newHandle;
		}


		ID3D12Resource*	GetAsset(SOResourceHandle handle) const
		{
			auto& resourceEntry = resources[handles[handle]];
			return resourceEntry.resources[resourceEntry.resourceIdx];
		}


		ID3D12Resource* GetAssetCounter(SOResourceHandle handle) const
		{
			auto& resourceEntry = resources[handles[handle]];
			return resourceEntry.resourceCounters[resourceEntry.resourceIdx];
		}


		size_t GetAssetSize(SOResourceHandle handle) const
		{
			auto& resourceEntry = resources[handles[handle]];
			return resourceEntry.resourceSize;
		}


		DeviceResourceState	GetAssetState(SOResourceHandle handle) const
		{
			auto& resourceEntry = resources[handles[handle]];
			return resourceEntry.resourceStates[resourceEntry.resourceIdx];
		}


		void SetResourceState(SOResourceHandle handle, DeviceResourceState state)
		{
			auto& resourceEntry = resources[handles[handle]];
			resourceEntry.resourceStates[resourceEntry.resourceIdx] = state;
		}


		void ReleaseResource(SOResourceHandle handle)
		{
			size_t idx = handles[handle];
			handles.RemoveHandle(handle);

			for (auto resource : resources[idx].resources)
				resource->Release();

			resources[idx] = resources.back();
			//handles[resources.back().resourceHandle] = idx;
			resources.pop_back();
		}


		void ReleaseAll()
		{
			for (auto resourceEntry : resources) 
			{
				for (auto resource : resourceEntry.resources)
					resource->Release();

				for (auto resource : resourceEntry.resourceCounters)
					resource->Release();
			}

			resources.clear();
			handles.Clear();
		}

		HandleUtilities::HandleTable<SOResourceHandle, 32>	handles;
		Vector<SOResource>									resources;
		iAllocator*											allocator;
	};


	/************************************************************************************************/


	// Basic Draw States
	// TODO: MOVE THESE OUT OF THIS HEADER!
	ID3D12PipelineState* CreateDrawTriStatePSO	( RenderSystem* RS );
	ID3D12PipelineState* CreateDrawLineStatePSO	( RenderSystem* RS );
	ID3D12PipelineState* CreateDraw2StatePSO	( RenderSystem* RS );


	/************************************************************************************************/


    class ReadBackStateTable
    {
    public:
        using FNHandler = void (*)(char* bufer, size_t bufferSize);

        ReadBackStateTable(iAllocator* allocator) :
            handles     { allocator },
            resources   { allocator } {}


        ReadBackResourceHandle AddReadBack(size_t BufferSize, ID3D12Resource* resource)
        {
            return InvalidHandle_t;
        }


        bool ReadBackReady(ReadBackResourceHandle handle)
        {
            return false;
        }


        void SetCallback(ReadBackResourceHandle handle, FNHandler readBackHandler)
        {
        }


        void QueueReadBack(size_t frameID)
        {
        }


    private:
        struct OnReadBack
        {
            void*       _Data;
            FNHandler   callback;
        };

        struct ReadBackEntry
        {
            OnReadBack onReceive;
        };

        ReadBackEntry& _GetEntry(ReadBackResourceHandle handle)
        {
            const size_t idx = handles[handle];
            return resources[idx];
        }

        enum States
        {
        };


        Vector<ReadBackEntry>                                   resources;
        HandleUtilities::HandleTable<ReadBackResourceHandle>    handles;
    };


    /************************************************************************************************/


	struct UAVResourceLayout
	{
		uint32_t	stride;
		uint32_t	elementCount;
		DXGI_FORMAT format;
	};

	struct UAVTextureLayout
	{
		uint2		WH;
		uint16_t	mipCount;
		DXGI_FORMAT format;
	};

	FLEXKITAPI class RenderSystem
	{
	public:
		RenderSystem(iAllocator* IN_allocator, ThreadManager* IN_Threads) :
			Memory			{ IN_allocator					                        },
			Library			{ IN_allocator					                        },
			Queries			{ IN_allocator, this			                        },
			Textures		{ IN_allocator					                        },
			VertexBuffers	{ IN_allocator					                        },
			ConstantBuffers	{ IN_allocator, this			                        },
			PipelineStates	{ IN_allocator, this, IN_Threads                        },
			BufferUAVs		{ IN_allocator					                        },
			Texture2DUAVs	{ IN_allocator					                        },
            PendingBarriers { IN_allocator                                          },
			StreamOutTable	{ IN_allocator					                        },
            ReadBackTable   { IN_allocator                                          },
            threads         {*IN_Threads                                            },
            Syncs           { IN_allocator, 64                                      },
            Contexts        { IN_allocator, 3 * ( 1 + IN_Threads->GetThreadCount()) } {}

		
		~RenderSystem() { Release(); }

		RenderSystem				(const RenderSystem&) = delete;
		RenderSystem& operator =	(const RenderSystem&) = delete;

		bool Initiate(Graphics_Desc* desc_in);
		void Release();

		// Pipeline State
		size_t						GetCurrentFrame();
		ID3D12PipelineState*		GetPSO				(PSOHandle StateID);
		RootSignature const * const GetPSORootSignature	(PSOHandle StateID) const;

		void RegisterPSOLoader	(PSOHandle State, PipelineStateDescription desc);
		void QueuePSOLoad		(PSOHandle State);

		void BeginSubmission	();
		void Submit				(Vector<Context*>& CLs);
		void PresentWindow		(RenderWindow* RW);

        void FlushPending();
		void WaitforGPU();

        void SetDebugName(ResourceHandle,        const char*);
        void SetDebugName(UAVResourceHandle,    const char*);
        void SetDebugName(UAVTextureHandle,     const char*);

		size_t						GetVertexBufferSize		(const VertexBufferHandle);
		D3D12_GPU_VIRTUAL_ADDRESS	GetVertexBufferAddress	(const VertexBufferHandle VB);
		D3D12_GPU_VIRTUAL_ADDRESS	GetConstantBufferAddress(const ConstantBufferHandle CB);


		size_t	GetTextureFrameGraphIndex(ResourceHandle);
		void	SetTextureFrameGraphIndex(ResourceHandle, size_t);

		uint32_t	GetTag(ResourceHandle Handle);
		void		SetTag(ResourceHandle Handle, uint32_t);

        void        MarkTextureUsed(ResourceHandle Handle);

		const size_t	GetTextureElementSize		(ResourceHandle   Handle) const;
		const uint2		GetTextureWH				(ResourceHandle   Handle) const;
		const uint2		GetTextureWH				(UAVTextureHandle Handle) const;
		FORMAT_2D		GetTextureFormat			(ResourceHandle Handle) const;
		DXGI_FORMAT		GetTextureDeviceFormat		(ResourceHandle Handle) const;

        TextureDimension GetTextureDimension(ResourceHandle handle) const;
        size_t           GetTextureArraySize(ResourceHandle handle) const;

		void			UploadTexture				(ResourceHandle, UploadQueueHandle, byte* buffer, size_t bufferSize); // Uses Upload Queue
        void            UploadTexture               (ResourceHandle handle, UploadQueueHandle, TextureBuffer* buffer, size_t resourceCount, iAllocator* temp); // Uses Upload Queue
		void			UpdateResourceByUploadQueue	(ID3D12Resource* Dest, UploadQueueHandle, void* Data, size_t Size, size_t ByteSize, D3D12_RESOURCE_STATES EndState);

		// Resource Creation and Destruction
		ConstantBufferHandle	CreateConstantBuffer			(size_t BufferSize, bool GPUResident = true);
		VertexBufferHandle		CreateVertexBuffer				(size_t BufferSize, bool GPUResident = true);
		ResourceHandle			CreateDepthBuffer				(const uint2 WH, const bool UseFloat = false, size_t bufferCount = 3);
		ResourceHandle			CreateGPUResource			    (const GPUResourceDesc& desc);
		QueryHandle				CreateOcclusionBuffer			(size_t Size);
		UAVResourceHandle		CreateUAVBufferResource			(size_t bufferHandle, bool tripleBuffer = true);
		UAVTextureHandle		CreateUAVTextureResource		(const uint2 WH, const FORMAT_2D, const bool RenderTarget = false);
		SOResourceHandle		CreateStreamOutResource			(size_t bufferHandle, bool tripleBuffer = true);
		QueryHandle				CreateSOQuery					(size_t SOIndex, size_t count);
		IndirectLayout			CreateIndirectLayout			(static_vector<IndirectDrawDescription> entries, iAllocator* allocator);
        ReadBackResourceHandle  CreateReadBackBuffer            (size_t bufferSize);


		void SetObjectState(SOResourceHandle	handle,	DeviceResourceState state);
		void SetObjectState(UAVResourceHandle	handle, DeviceResourceState state);
		void SetObjectState(UAVTextureHandle	handle, DeviceResourceState state);
		void SetObjectState(ResourceHandle		handle,	DeviceResourceState state);


		DeviceResourceState GetObjectState(const QueryHandle		handle) const;
		DeviceResourceState GetObjectState(const SOResourceHandle	handle) const;
		DeviceResourceState GetObjectState(const UAVResourceHandle	handle) const;
		DeviceResourceState GetObjectState(const UAVTextureHandle	handle) const;
		DeviceResourceState GetObjectState(const ResourceHandle		handle) const;


		ID3D12Resource*		GetObjectDeviceResource	(const ConstantBufferHandle	handle) const;
		ID3D12Resource*		GetObjectDeviceResource	(const ResourceHandle		handle) const;
		ID3D12Resource*		GetObjectDeviceResource (const SOResourceHandle		handle) const;
		ID3D12Resource*		GetObjectDeviceResource	(const UAVResourceHandle	handle) const;
		ID3D12Resource*		GetObjectDeviceResource	(const UAVTextureHandle		handle) const;

		ID3D12Resource*		GetSOCounterResource	(const SOResourceHandle handle) const;
		size_t				GetStreamOutBufferSize	(const SOResourceHandle handle) const;

		ID3D12Resource*		GetUploadResource();

		UAVResourceLayout	GetUAVBufferLayout(const UAVResourceHandle) const noexcept;
		void				SetUAVBufferLayout(const UAVResourceHandle, const UAVResourceLayout) noexcept;

		Texture2D			GetUAV2DTexture(const UAVTextureHandle ) const;

		void ResetQuery(QueryHandle handle);

		void ReleaseCB		(ConstantBufferHandle);
		void ReleaseVB		(VertexBufferHandle);
		void ReleaseTexture	(ResourceHandle);
		void ReleaseUAV		(UAVResourceHandle);
		void ReleaseUAV		(UAVTextureHandle);

		void ReleaseTempResources();


        void                SubmitUploadQueues(UploadQueueHandle* handle, size_t count = 1);
        UploadQueueHandle   GetUploadQueue();
        UploadQueueHandle   GetImmediateUploadQueue();
        Context&            GetCommandList();
		// Internal
		//ResourceHandle			_AddBackBuffer						(Texture2D_Desc& Desc, ID3D12Resource* Res, uint32_t Tag);
		static ConstantBuffer	_CreateConstantBufferResource		(RenderSystem* RS, ConstantBuffer_desc* desc);
		VertexResourceBuffer	_CreateVertexBufferDeviceResource	(const size_t ResourceSize, bool GPUResident = true);

        ID3D12GraphicsCommandList*  _GetUploadCommandList(UploadQueueHandle);
        void                        _PushDelayReleasedResource(ID3D12Resource*, UploadQueueHandle = InvalidHandle_t);

        void    _InsertBarrier(ID3D12Resource*, D3D12_RESOURCE_STATES currentState, D3D12_RESOURCE_STATES endState);

		size_t  _GetVidMemUsage();



		ID3D12QueryHeap*			_GetQueryResource(QueryHandle handle);

		void						_IncrementRSIndex() { CurrentIndex = (CurrentIndex + 1) % 3; }

		operator RenderSystem* () { return this; }

		ID3D12Device*			pDevice			= nullptr;
		ID3D12CommandQueue*		GraphicsQueue	= nullptr;
		ID3D12CommandQueue*		UploadQueue		= nullptr;
		ID3D12CommandQueue*		ComputeQueue	= nullptr;

		size_t CurrentFrame			= 0;
 		size_t CurrentIndex			= 0;
		size_t FenceCounter			= 0;

		ID3D12Fence* Fence			= nullptr;

		UploadQueues		    Uploads;
        UploadQueueHandle       ImmediateUpload = InvalidHandle_t;

		IDXGIFactory5*			pGIFactory		= nullptr;
		IDXGIAdapter4*			pDXGIAdapter	= nullptr;

		ConstantBuffer			NullConstantBuffer; // Zero Filled Constant Buffer
		UAVTextureHandle   		NullUAV; // 1x1 Zero UAV
        ResourceHandle          NullSRV;
        ResourceHandle          NullSRV1D;

        struct PendingBarrier
        {
            ID3D12Resource*         resource;
            D3D12_RESOURCE_STATES   beginState;
            D3D12_RESOURCE_STATES   endState;
        };

		struct
		{
			size_t			FenceValue;
			HANDLE			FenceHandle;
		}Fences[QueueSize];

		struct
		{
			size_t			FenceValue;
			HANDLE			FenceHandle;
		}CopyFences[QueueSize];

		size_t BufferCount				= 0;
		size_t DescriptorRTVSize		= 0;
		size_t DescriptorDSVSize		= 0;
		size_t DescriptorCBVSRVUAVSize	= 0;

		struct
		{
			size_t AASamples	= 0;
			size_t AAQuality	= 1;
		}Settings;

		struct
		{
			ID3D12Resource* TempBuffer	= nullptr;
			size_t			Position	= 0;
			size_t			Last		= 0;
			size_t			Size		= 0;
			char*			Buffer		= nullptr;

			void Release()
			{
				if (!TempBuffer)
					return;

				TempBuffer->Unmap(0, nullptr);
				TempBuffer->Release();

				TempBuffer	= 0;
				Position	= 0;
				Size		= 0;
				Buffer		= nullptr;
			}
		}CopyEngine;

		struct RootSigLibrary
		{
			RootSigLibrary(iAllocator* Memory) :
				RSDefault			{ Memory },
				ShadingRTSig		{ Memory },
				RS4CBVs_SO			{ Memory },
				RS6CBVs4SRVs		{ Memory },
				RS2UAVs4SRVs4CBs	{ Memory },
				ComputeSignature	{ Memory }{}

			void Initiate(RenderSystem* RS, iAllocator* TempMemory);

            void Release()
            {
                RS2UAVs4SRVs4CBs.Release();
                RS6CBVs4SRVs.Release();
                RS4CBVs_SO.Release();
                ShadingRTSig.Release();
                RSDefault.Release();
                ComputeSignature.Release();
            }

			RootSignature RS2UAVs4SRVs4CBs;		// 4CBVs On all Stages, 4 SRV On all Stages
			RootSignature RS6CBVs4SRVs;			// 4CBVs On all Stages, 4 SRV On all Stages
			RootSignature RS4CBVs_SO;			// Stream Out Enabled
			RootSignature ShadingRTSig;			// Signature For Compute Based Deferred Shading
			RootSignature RSDefault;			// Default Signature for Rasting
			RootSignature ComputeSignature;		// 
		}Library;

        Vector<Context>                                         Contexts;
        size_t                                                  contextIdx = 0;

		ConstantBufferTable		                                ConstantBuffers;
		QueryTable				                                Queries;
		VertexBufferStateTable									VertexBuffers;
		TextureStateTable										Textures;
		UAVResourceTable<UAVResourceHandle, UAVResourceLayout>	BufferUAVs;
		UAVResourceTable<UAVTextureHandle,	UAVTextureLayout>	Texture2DUAVs;
		SOResourceTable											StreamOutTable;
		PipelineStateTable										PipelineStates;
        ReadBackStateTable                                      ReadBackTable;

		struct FreeEntry
		{
			bool operator == (const FreeEntry& rhs)
			{
				return rhs.Resource == Resource;
			}

			ID3D12Resource* Resource	= nullptr;
			size_t			Counter		= 0;
		};

        struct UploadSyncPoint
        {
            UploadQueueHandle queue;
            size_t            waitCounter;
        };

        Vector<UploadSyncPoint>         Syncs;
        Vector<FreeEntry>	            FreeList;
        Vector<D3D12_RESOURCE_BARRIER>	PendingBarriers;

        ThreadManager&          threads;

		ID3D12Debug*			pDebug			= nullptr;
		ID3D12DebugDevice*		pDebugDevice	= nullptr;
		iAllocator*				Memory			= nullptr;

	};

	
	/************************************************************************************************/

    // INTERNAL USE ONLY!
	FLEXKITAPI UploadSegment	ReserveUploadBuffer		(const size_t uploadSize, RenderSystem& renderSystem, UploadQueueHandle = InvalidHandle_t);
	FLEXKITAPI void				MoveBuffer2UploadBuffer	(const UploadSegment& data, byte* source, size_t uploadSize);

	FLEXKITAPI DescHeapPOS PushRenderTarget			(RenderSystem* RS, const Texture2D&	target, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushRenderTarget			(RenderSystem* RS, ResourceHandle	target, DescHeapPOS POS);
    FLEXKITAPI DescHeapPOS PushRenderTarget2        (RenderSystem* RS, ResourceHandle    target, DescHeapPOS POS);


	FLEXKITAPI DescHeapPOS PushDepthStencil			    (RenderSystem* RS, ResourceHandle Target, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushCBToDescHeap			    (RenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, size_t BufferSize, size_t offset = 0);
	FLEXKITAPI DescHeapPOS PushSRVToDescHeap		    (RenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, size_t ElementCount, size_t Stride, D3D12_BUFFER_SRV_FLAGS Flags = D3D12_BUFFER_SRV_FLAG_NONE);
	FLEXKITAPI DescHeapPOS Push2DSRVToDescHeap		    (RenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, D3D12_BUFFER_SRV_FLAGS = D3D12_BUFFER_SRV_FLAG_NONE);
	FLEXKITAPI DescHeapPOS PushTextureToDescHeap	    (RenderSystem* RS, Texture2D tex, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushCubeMapTextureToDescHeap (RenderSystem* RS, Texture2D tex, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushUAV2DToDescHeap		    (RenderSystem* RS, Texture2D tex, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushUAVBufferToDescHeap	    (RenderSystem* RS, UAVBuffer buffer, DescHeapPOS POS);


	/************************************************************************************************/


	ResourceHandle MoveTextureBuffersToVRAM	(RenderSystem* RS, UploadQueueHandle, TextureBuffer* buffer, size_t MIPCount, iAllocator* tempMemory, FORMAT_2D format = FORMAT_2D::R8G8B8A8_UINT);
	ResourceHandle MoveTextureBufferToVRAM	(RenderSystem* RS, UploadQueueHandle, TextureBuffer* buffer, FORMAT_2D format, iAllocator* tempMemory);


	/************************************************************************************************/


	typedef static_vector<D3D12_INPUT_ELEMENT_DESC, 16> InputDescription;

	struct TriangleMeshMetaData
	{
		D3D12_INPUT_ELEMENT_DESC	InputLayout[16];
		size_t						InputElementCount;
		size_t						IndexBuffer_Index;
	};


	struct VertexBuffer
	{
		struct BuffEntry
		{
			ID3D12Resource*		Buffer;
			uint32_t			BufferSizeInBytes;
            uint32_t            BufferStride;
			VERTEXBUFFER_TYPE	Type;

			operator bool()	{ return Buffer != nullptr; }
			operator ID3D12Resource* ()	{ return Buffer; }
		};

		void clear() { VertexBuffers.clear(); }

		ID3D12Resource*	operator[](size_t idx)	{ return VertexBuffers[idx].Buffer; }
		static_vector<BuffEntry, 16>	VertexBuffers;
		TriangleMeshMetaData			MD;
	};


    /************************************************************************************************/


    struct TriMesh
    {
        TriMesh()
        {
            for (auto b : Buffers)
                b = nullptr;

            ID = nullptr;
            AnimationData = EAD_None;
            Memory = nullptr;
        }


        TriMesh(const TriMesh& rhs)
        {
            memcpy(this, &rhs, sizeof(TriMesh));
        }


        bool HasTangents() const
        {
            for (auto view : Buffers)
            {
                if (view && view->GetBufferType() == VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT)
                    return true;
            }

            return false;
        }

        bool HasNormals() const
        {
            for (auto view : Buffers)
            {
                if (view && view->GetBufferType() == VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL)
                    return true;
            }

            return false;
        }

        VertexBufferView* GetNormals()
        {
            for (auto view : Buffers)
            {
                if (view && view->GetBufferType() == VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL)
                    return view;
            }

            return nullptr;
        }


        VertexBufferView* GetIndices()
        {
            for (auto view : Buffers)
            {
                if (view && view->GetBufferType() == VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX)
                    return view;
            }

            return nullptr;
        }


        size_t AnimationData;
        size_t IndexCount;
        size_t TriMeshID;

        struct SubDivInfo
        {
            size_t  numVertices;
            size_t  numFaces;
            int* numVertsPerFace;
            int* IndicesPerFace;
        }*SubDiv;

        const char*                         ID;
        SkinDeformer*                       SkinTable;
        Skeleton*                           Skeleton;
        static_vector<VertexBufferView*>    Buffers;
        VertexBuffer		                VertexBuffer;

        struct RInfo
        {
            float3 Offset;
            float3 min, max;
            float  r;
        }Info;

        // Visibility Information
        AABB			AABB;
        BoundingSphere	BS;

        size_t		SkeletonGUID;
        iAllocator* Memory;
    };


    /************************************************************************************************/


	struct Shader
	{
		operator bool() {
			return ( Blob != nullptr );
		}

		operator Shader*() {
			return this;
		}
		operator D3D12_SHADER_BYTECODE()
		{
			return{ (BYTE*)Blob->GetBufferPointer(), Blob->GetBufferSize() };
		}

		Shader& operator = (const Shader& in) {
			Blob     = in.Blob;
			Type     = in.Type;

			return *this;
		}

		ID3DBlob*			Blob = nullptr;
		SHADER_TYPE			Type = SHADER_TYPE::SHADER_TYPE_Unknown;
	};


	/************************************************************************************************/


	enum VirtualTexturingParams
	{
		VTP_UNUSEDDESCRIPTORTABLE = 0,
		VTP_CameraConstants		  = 1,
		VTP_ShadingConstants	  = 2,
		VTP_EXTRACONSTANTS		  = 3,
		VTP_UAVOut				  = 4,
		VTP_COUNT,
	};


	struct ShadowMapPass
	{
		ID3D12PipelineState* PSO_Animated;
		ID3D12PipelineState* PSO_Static;
	};


	/************************************************************************************************/


	struct PointLight
	{
		float3 K;
		float I, R;

		NodeHandle Position;
	};

	struct PointLightEntry
	{
		float4 POS;// + R 
		float4 Color; // Plus Intensity
	};

	struct SpotLight
	{
		float3 K;
		float3 Direction;
		float I, R;
		float t;
		NodeHandle Position;
	};

	struct SpotLightEntry
	{
		float4 POS;			// + R 
		float4 Color;		// Plus Intensity
		float3 Direction;
	};

	struct Skeleton;

	enum LightBufferFlags
	{
		Dirty  = 0x01,
		Unused = 0x02,
		Clean  = 0x00
	};


	const size_t PLB_Stride  = sizeof(float[8]);
	const size_t SPLB_Stride = sizeof(float[12]);


	/************************************************************************************************/


	class CBPushBuffer
	{
	public:
		CBPushBuffer(ConstantBufferHandle cBuffer = InvalidHandle_t, char* IN_buffer = nullptr, size_t offsetBegin = 0, size_t reservedSize = 0) :
			CB				{ cBuffer		},
			buffer			{ IN_buffer		},
			pushBufferSize	{ reservedSize	},
			pushBufferBegin	{ offsetBegin	} {}


		CBPushBuffer(ConstantBufferHandle IN_CB, size_t reserveSize, RenderSystem& renderSystem)
		{
			auto reserverdBuffer = renderSystem.ConstantBuffers.Reserve(IN_CB, reserveSize);

			CB				= IN_CB;
			buffer			= reserverdBuffer.Data;
			pushBufferSize	= reserveSize;
			pushBufferBegin	= reserverdBuffer.offsetBegin;
		}


		CBPushBuffer(const CBPushBuffer&)							= delete;
		CBPushBuffer& operator = (const CBPushBuffer& pushBuffer)	= delete;


		CBPushBuffer(CBPushBuffer&& pushBuffer) 
		{
			CB				= pushBuffer.CB;
			buffer			= pushBuffer.buffer;
			pushBufferBegin = pushBuffer.pushBufferBegin;
			pushBufferSize	= pushBuffer.pushBufferSize;
			pushBufferUsed	= pushBuffer.pushBufferUsed;

			pushBuffer.CB				= InvalidHandle_t;
			pushBuffer.buffer			= nullptr;
			pushBuffer.pushBufferBegin	= 0;
			pushBuffer.pushBufferSize	= 0;
			pushBuffer.pushBufferUsed	= 0;
		}


		CBPushBuffer& operator = (CBPushBuffer&& pushBuffer)
		{
			CB				= pushBuffer.CB;
			buffer			= pushBuffer.buffer;
			pushBufferBegin = pushBuffer.pushBufferBegin;
			pushBufferSize	= pushBuffer.pushBufferSize;
			pushBufferUsed	= pushBuffer.pushBufferUsed;

			pushBuffer.CB				= InvalidHandle_t;
			pushBuffer.buffer			= nullptr;
			pushBuffer.pushBufferBegin	= 0;
			pushBuffer.pushBufferSize	= 0;
			pushBuffer.pushBufferUsed	= 0;

			return *this; 
		}

		operator ConstantBufferHandle () const { return CB; }


        template<typename TY>
        static constexpr size_t CalculateOffset()
        {
            return (sizeof(TY) / 256 + 1) * 256;
        }

		template<typename _TY>
		size_t Push(_TY& data)
		{
			if (pushBufferUsed + sizeof(data) > pushBufferSize)
				return -1;

			size_t offset = pushBufferUsed;
			memcpy(pushBufferBegin + buffer + pushBufferUsed, (char*)&data, sizeof(data));
            pushBufferUsed += CalculateOffset<_TY>();

			return offset + pushBufferBegin;
		}

		size_t Push(char* _ptr, size_t size)
		{
			if (pushBufferUsed + size > pushBufferSize)
				return -1;

			size_t offset = pushBufferUsed;
			memcpy(pushBufferBegin + buffer + pushBufferUsed, _ptr, size);
			pushBufferUsed += (size / 256 + 1) * 256;

			return offset + pushBufferBegin;
		}


		size_t begin() const { return pushBufferBegin; }



	private:
		ConstantBufferHandle	CB				= InvalidHandle_t;
		char*					buffer			= nullptr;
		size_t					pushBufferBegin = 0;
		size_t					pushBufferSize	= 0;
		size_t					pushBufferUsed	= 0;
	};


	class VBPushBuffer
	{
	public:
		VBPushBuffer(VertexBufferHandle vBuffer = InvalidHandle_t, char* buffer_ptr = nullptr, size_t offsetBegin = 0, size_t reservedSize = 0) noexcept :
			VB					{ vBuffer		},
			buffer				{ buffer_ptr	},
			pushBufferSize		{ reservedSize	},
			pushBufferOffset	{ offsetBegin	} {}


		VBPushBuffer(VertexBufferHandle IN_VB, size_t IN_reserveSize, RenderSystem& renderSystem)
		{
			auto reservedBuffer = renderSystem.VertexBuffers.Reserve(IN_VB, IN_reserveSize);
			VB					= IN_VB;
			pushBufferSize		= IN_reserveSize;
			buffer				= reservedBuffer.Data;
			pushBufferOffset	= reservedBuffer.offsetBegin;
		}


		VBPushBuffer(const VBPushBuffer&)						= delete;
		VBPushBuffer& operator = (VBPushBuffer& pushBuffer)		= delete;


		VBPushBuffer(VBPushBuffer&& pushBuffer)
		{
			VB				 = pushBuffer.VB;
			buffer			 = pushBuffer.buffer;
			pushBufferOffset = pushBuffer.pushBufferOffset;
			pushBufferSize	 = pushBuffer.pushBufferSize;
			pushBufferUsed	 = pushBuffer.pushBufferUsed;

			pushBuffer.VB				= InvalidHandle_t;
			pushBuffer.buffer			= nullptr;
			pushBuffer.pushBufferOffset	= 0;
			pushBuffer.pushBufferSize	= 0;
			pushBuffer.pushBufferUsed	= 0;
		}


		VBPushBuffer& operator = (VBPushBuffer&& pushBuffer) 
		{ 
			VB				 = pushBuffer.VB;
			buffer			 = pushBuffer.buffer;
			pushBufferOffset = pushBuffer.pushBufferOffset;
			pushBufferSize	 = pushBuffer.pushBufferSize;
			pushBufferUsed	 = pushBuffer.pushBufferUsed;

			pushBuffer.VB				= InvalidHandle_t;
			pushBuffer.buffer			= nullptr;
			pushBuffer.pushBufferOffset	= 0;
			pushBuffer.pushBufferSize	= 0;
			pushBuffer.pushBufferUsed	= 0;

			return *this; 
		}

		operator VertexBufferHandle () { return VB; }


		template<typename _TY>
		size_t Push(_TY& data) noexcept
		{
			if (pushBufferUsed + sizeof(data) > pushBufferSize)
				return -1;

			size_t offset = pushBufferUsed;
			memcpy(pushBufferOffset + buffer + pushBufferUsed, (char*)&data, sizeof(data));
			pushBufferUsed += sizeof(data);

			return offset;
		}


		size_t Push(char* _ptr, size_t size) noexcept
		{
			if (pushBufferUsed + size > pushBufferSize)
				return -1;

			size_t offset = pushBufferUsed;
			memcpy(pushBufferOffset + buffer + pushBufferUsed, _ptr, size);
			pushBufferUsed += std::ceil(size / 512) * 512;

			return offset;
		}


		size_t begin() { return pushBufferOffset; }


		VertexBufferHandle	VB					= InvalidHandle_t;
		char*				buffer				= 0;
		size_t				pushBufferOffset	= 0;
		size_t				pushBufferSize		= 0;
		size_t				pushBufferUsed		= 0;
	};


	/************************************************************************************************/


	CBPushBuffer Reserve(ConstantBufferHandle CB, size_t pushSize, size_t count, RenderSystem& renderSystem)
	{
		size_t reserveSize = (pushSize / 256 + 1) * 256 * count;
		auto buffer = renderSystem.ConstantBuffers.Reserve(CB, reserveSize);

		return { CB, buffer.Data, buffer.offsetBegin, reserveSize };
	}


	VBPushBuffer Reserve(VertexBufferHandle VB, size_t reserveSize, RenderSystem& renderSystem)
	{
		auto buffer = renderSystem.VertexBuffers.Reserve(VB, reserveSize);
		return { VB, buffer.Data, buffer.offsetBegin, reserveSize };
	}


	/************************************************************************************************/


	class ConstantBufferDataSet
	{
	public:
		ConstantBufferDataSet() {}

		template<typename TY>
		ConstantBufferDataSet(const TY& initialData, CBPushBuffer& buffer) :
			constantBuffer	{ buffer },
			constantsOffset	{ buffer.Push(initialData) } {}

        explicit ConstantBufferDataSet(const size_t& IN_offset, ConstantBufferHandle IN_buffer) :
            constantBuffer  { IN_buffer },
            constantsOffset { IN_offset } {}


		~ConstantBufferDataSet() = default;

		ConstantBufferDataSet(const ConstantBufferDataSet& rhs)					= default;
		ConstantBufferDataSet& operator = (const ConstantBufferDataSet& rhs)	= default;

		ConstantBufferHandle    Handle() const { return constantBuffer;	}
		size_t				    Offset() const { return constantsOffset; }
	private:
		ConstantBufferHandle	constantBuffer		= InvalidHandle_t;
		size_t					constantsOffset		= 0;
	};


	/************************************************************************************************/


    template<typename TY>
    auto CreateCBIterator(const CBPushBuffer& source)
    {
        struct Proxy
        {
            Proxy operator[](const size_t idx) const
            {
                return { offset, idx, CB };
            }

            operator ConstantBufferDataSet() const
            {
                return ConstantBufferDataSet{ offset + idx * CBPushBuffer::CalculateOffset<TY>(), CB };
            }

            const size_t                offset;
            const size_t                idx;
            const ConstantBufferHandle	CB = InvalidHandle_t;
        };

        return Proxy{ source.begin(), 0, source };
    }


    /************************************************************************************************/


	struct SET_MAP_t{

	}const SET_MAP_OP;

	struct SET_TRANSFORM_t {

	}const SET_TRANSFORM_OP;

	class VertexBufferDataSet
	{
	public:
		VertexBufferDataSet() {}

		template<typename TY>
		VertexBufferDataSet(TY& initialData, VBPushBuffer& buffer) :
			vertexBuffer	{ buffer					},
			offsetBegin		{ buffer.pushBufferOffset	}
		{
            vertexStride = sizeof(initialData[0]);

			for (auto& vertex : initialData) 
				buffer.Push(vertex);
		}

		template<typename TY, typename FN_TransformVertex>
		VertexBufferDataSet(const SET_MAP_t, TY& initialData, FN_TransformVertex& TransformVertex, VBPushBuffer& buffer) :
			vertexBuffer	{ buffer					},
			offsetBegin		{ buffer.pushBufferOffset	}
		{
			vertexStride = sizeof(decltype(TransformVertex(initialData.front())));

			for (auto& vertex : initialData) {
				auto transformedVertex = TransformVertex(vertex);
				if (buffer.Push(transformedVertex) == -1)
					throw std::exception("buffer too short to accomdate push!");
			}
		}

		template<typename TY_CONTAINER, typename FN_TransformVertex, typename TY>
		VertexBufferDataSet(const SET_TRANSFORM_t, TY_CONTAINER& initialData, FN_TransformVertex& TransformVertex, VBPushBuffer& buffer, TY) :
			vertexBuffer	{ buffer					},
			offsetBegin		{ buffer.pushBufferOffset	}
		{
			vertexStride = sizeof(TY);

			struct adapter
			{
				VBPushBuffer& buffer;

				adapter(VBPushBuffer& IN_buffer) : buffer{ IN_buffer } {}

				void append(TY vertex)
				{
					if(buffer.Push(vertex) == -1)
						throw std::exception("buffer too short to accomdate push!");
				}

			}_adapter{ buffer };

			for (auto& vertex : initialData)
				TransformVertex(vertex, _adapter);
		}

		~VertexBufferDataSet() = default;

		VertexBufferDataSet(const VertexBufferDataSet& rhs)					= default;
		VertexBufferDataSet& operator = (const VertexBufferDataSet& rhs)	= default;

		operator VertexBufferEntry () const
		{
			return { vertexBuffer, (UINT)vertexStride, (UINT)offsetBegin };
		}


		operator VertexBufferHandle const ()	{ return vertexBuffer;	}
		operator size_t const ()				{ return offsetBegin;	}

	private:
		uint64_t			offsetBegin		= 0;
		uint64_t			vertexStride	= 0;
		VertexBufferHandle	vertexBuffer	= InvalidHandle_t;
	};


	/************************************************************************************************/


	typedef Vector<PointLight>			PLList;
	typedef Vector<PointLightEntry>		PLBuffer;
	typedef Vector<SpotLight>			SLList;
	typedef Vector<SpotLightEntry>		SLBuffer;
	typedef Vector<char>				LightFlags;
	typedef Vector<char*>				LightIDs;


	/************************************************************************************************/


	struct PointLightListDesc
	{
		size_t	MaxLightCount;
	};

	struct PointLightList
	{
		PLList						Lights;
		LightFlags					Flags;
		LightIDs					IDs;

		PointLight&	operator []( size_t index )	{
			return Lights[index];
		}


		PointLight	operator [](size_t index) const {
			return Lights[index];
		}

		void push_back( PointLight PL )	{
			Lights.push_back(PL);
		}

		void Swap( size_t i1, size_t i2 ) {
			auto Temp = Lights[i1];
			Lights[i1] = Lights[i2];
			Lights[i2] = Temp;
		}

		size_t	size()	const {return Lights.size();}

		void Release()		
		{
			Lights.Release();
		}
	};


	/************************************************************************************************/


	struct SpotLightList
	{
		SLList						Lights;
		LightFlags					Flags;
		LightIDs					IDs;

		SpotLight&	operator []( size_t index )
		{
			FK_ASSERT (index < Lights.size());
			return Lights.at(index);
		}

		void push_back( SpotLight PL )
		{
			Lights.push_back(PL);
		}

		void Swap( size_t i1, size_t i2 )
		{
			auto Temp	 = Lights.at(i1);
			Lights.at(i1) = Lights.at(i2);
			Lights.at(i2) = Temp;
		}

		size_t size() const {return Lights.size();}

		void Release(){ Lights.Release();}
	};



	//struct SpotLightShadowCaster
	//{
	//	Camera	C;
	//	size_t	LightHandle;
	//};

	/************************************************************************************************/


	struct 
	{
		HandleUtilities::HandleTable<TriMeshHandle>		Handles;
		Vector<TriMesh>									Geometry;
		Vector<size_t>									ReferenceCounts;
		Vector<GUID_t>									Guids;
		Vector<const char*>								GeometryIDs;
		Vector<TriMeshHandle>							Handle;
		Vector<size_t>									FreeList;
		iAllocator*										Memory;
	}GeometryTable;


	/************************************************************************************************/


	FLEXKITAPI void							InitiateGeometryTable	(iAllocator* memory = nullptr);
	FLEXKITAPI void							ReleaseGeometryTable	();

	FLEXKITAPI void							AddRef					( TriMeshHandle  TMHandle );
	FLEXKITAPI void							ReleaseMesh				( RenderSystem* RS, TriMeshHandle  TMHandle );

	FLEXKITAPI TriMeshHandle				LoadMesh				( GUID_t TMHandle );

	FLEXKITAPI TriMeshHandle				GetMesh					( RenderSystem* rs, const char* meshID );
	FLEXKITAPI TriMeshHandle				GetMesh					( RenderSystem* rs, const char* meshID );


	FLEXKITAPI TriMesh*						GetMeshResource			( TriMeshHandle  TMHandle );
	FLEXKITAPI BoundingSphere				GetMeshBoundingSphere	( TriMeshHandle  TMHandle );

	FLEXKITAPI Skeleton*					GetSkeleton				( TriMeshHandle  TMHandle );
	FLEXKITAPI size_t						GetSkeletonGUID			( TriMeshHandle  TMHandle );
	FLEXKITAPI void							SetSkeleton				( TriMeshHandle  TMHandle, Skeleton* S );

	FLEXKITAPI Pair<TriMeshHandle, bool>	FindMesh				( GUID_t			guid );
	FLEXKITAPI Pair<TriMeshHandle, bool>	FindMesh				( const char*	ID   );
	FLEXKITAPI bool							IsMeshLoaded			( GUID_t			guid );
	FLEXKITAPI bool							IsSkeletonLoaded		( TriMeshHandle	guid );
	FLEXKITAPI bool							HasAnimationData		( TriMeshHandle	guid );


	/************************************************************************************************/


	struct Mesh_Description
	{
		size_t				IndexBuffer;
		size_t				BufferCount;

		VertexBufferView**	Buffers;
		iAllocator*			memory;
	};


	/************************************************************************************************/


	inline ID3D12Resource* GetBuffer(TriMesh* Mesh, size_t Buffer)	{ return Mesh->VertexBuffer[Buffer]; }


	inline ID3D12Resource* FindBuffer(TriMesh* Mesh, VERTEXBUFFER_TYPE Type) 
	{
		ID3D12Resource* Buffer = nullptr;
		auto& VertexBuffers = Mesh->VertexBuffer.VertexBuffers;
		auto RES = find(VertexBuffers, [Type](auto& V) -> bool {return V.Type == Type;});

		if (RES != VertexBuffers.end())
			Buffer = RES->Buffer;

		return Buffer;
	}


	/************************************************************************************************/


	inline VertexBuffer::BuffEntry* FindBufferEntry(TriMesh* Mesh, VERTEXBUFFER_TYPE Type) 
	{
		ID3D12Resource* Buffer = nullptr;
		auto& VertexBuffers = Mesh->VertexBuffer.VertexBuffers;
		auto RES = find(VertexBuffers, [Type](auto& V) -> bool {return V.Type == Type;});

		if (RES != VertexBuffers.end())
			Buffer = RES->Buffer;

		return (VertexBuffer::BuffEntry*)RES; // Remove Const of result
	}


	/************************************************************************************************/


	inline bool AddVertexBuffer(VERTEXBUFFER_TYPE Type, TriMesh* Mesh, static_vector<D3D12_VERTEX_BUFFER_VIEW>& out) 
	{
		auto* VB = FindBufferEntry(Mesh, Type);

		if (VB == Mesh->VertexBuffer.VertexBuffers.end() || VB->Buffer == nullptr) {
#ifdef _DBUG
			return false;
#else 

			D3D12_VERTEX_BUFFER_VIEW VBView;

			VBView.BufferLocation	= 0;
			VBView.SizeInBytes		= 0;
			VBView.StrideInBytes	= 0;
			out.push_back(VBView);

			return true;
#endif
		}

		D3D12_VERTEX_BUFFER_VIEW VBView;

		VBView.BufferLocation	= VB->Buffer->GetGPUVirtualAddress();
		VBView.SizeInBytes		= VB->BufferSizeInBytes;
		VBView.StrideInBytes	= VB->BufferStride;
		out.push_back(VBView);

		return true;
	}


	TriMeshHandle CreateCube(RenderSystem* RS, iAllocator* Memory, float R, GUID_t MeshID);


	/************************************************************************************************/


	typedef Handle_t<8> ShaderHandle;
	typedef Handle_t<8> ShaderSetHandle;


	struct Material
	{
		Material() :
			Colour(1.0f, 1.0f, 1.0f, 1.0f),
			Metal(1.0f, 1.0f, 1.0f, 0.0f){}


		float4	Colour;
		float4	Metal;
	};


	enum TEXTURETYPE
	{
		ETT_ALBEDO,
		ETT_ROUGHSMOOTH,
		ETT_NORMAL,
		ETT_BUMP,
		ETT_COUNT,
	};


	struct TextureSet
	{
		struct {
			char	Directory[64];
		}TextureLocations[16];

		size_t		TextureGuids[16];

		ResourceHandle	Textures[16];
		bool			Loaded[16];
	};


	/************************************************************************************************/


	typedef float2 VTexcord; // virtual Texture Cord

	struct TextureEntry
	{
		size_t		OffsetIndex;
		GUID_t		ResourceID;
	};

	struct FrameBufferedRenderTarget
	{
		FrameBufferedResource RenderTargets;

		ID3D12Resource*	GetAsset() {
			RenderTargets.Get();
		}

		void Increment() {
			RenderTargets.IncrementCounter();
		}

		uint2				WH;
		DXGI_FORMAT			Format;
	};


	//FrameBufferedRenderTarget CreateFrameBufferedRenderTarget(RenderSystem* RS, GPUResourceDesc* Desc);

	struct TextureVTable
	{
		Texture2D					TextureMemory;
		FrameBufferedRenderTarget	RenderTarget;// Read-Back Buffer

		Vector<Texture2D>			PageTables;
		Vector<TextureEntry>		TextureTable;
		Vector<TextureSet*>		TextureSets;

		struct TableEntry{
			GUID_t ResourceID;
			float2 Offset;
		}* PageTable;

		uint2 PageTableDimensions;

		// Update 
		ID3D12PipelineState*		UpdatePSO;
		ConstantBuffer				Constants;
		StreamOutBuffer				ReadBackBuffer;

		iAllocator*		Memory;
	};


	/************************************************************************************************/


	struct TextureVTable_Desc
	{
		uint2			PageSize;
		uint2			PageCount;
		iAllocator*		Memory; // Needs to be persistent memory
	};


	/************************************************************************************************/


	struct TextureManager
	{
		Vector<TextureSet>		Textures;
		Vector<uint32_t>		FreeList;
	};


	FLEXKITAPI void UploadTextureSet	( RenderSystem* RS, TextureSet* TS, iAllocator* Memory );
	FLEXKITAPI void ReleaseTextureSet	( TextureSet* TS, iAllocator* Memory );


	/************************************************************************************************/
	
	const size_t MAXINSTANCES = 1024 * 1;
	const size_t MAXTRIMESHES = 16;

	typedef FlexKit::Handle_t<16> StaticObjectHandle;


	struct FLEXKITAPI StaticScene
	{
		StaticScene() : ObjectTable(nullptr, GetTypeGUID(StaticScene)) {}
		Vector<DirectX::XMMATRIX>		Transforms		[MAXINSTANCES];
		char							GeometryIndex	[MAXINSTANCES];
		NodeHandle						NodeHandles		[MAXINSTANCES];

		HandleUtilities::HandleTable<StaticObjectHandle>	ObjectTable;
	};
	

	StaticObjectHandle CreateDrawable(StaticScene* Scene, NodeHandle node, size_t GeometryIndex = 0);

	// TODO: Implement StaticMeshBatcher
	struct FLEXKITAPI StaticMeshBatcher
	{
		typedef char DirtyFlag;

		void Initiate(FlexKit::RenderSystem* RS, iAllocator* Memory);

		inline size_t AddTriMesh(FlexKit::TriMesh* NewMesh){
			size_t itr = 0;
			for (auto& G : Geometry)
			{	
				if (!G){
					G = NewMesh;
					TriMeshCount++;
					break;
				}
				itr++;
			}
			return itr;
		}

		struct
		{
			uint32_t VertexCount;
			uint32_t VertexOffset;
			uint32_t IndexOffset;
			uint32_t Padding;
		}GeometryTable[MAXTRIMESHES];

		TriMesh* Geometry[MAXTRIMESHES];

		struct InstanceIOLayout
		{
			uint32_t IndexCountPerInstance;
			uint32_t InstanceCount;
			uint32_t StartIndexLocation;
			int32_t	 BaseVertexLocation;
			uint32_t StartInstanceLocation;
		}InstanceInformation[MAXINSTANCES];

		size_t TriMeshCount;
		size_t MaxVertexPerInstance;
		size_t InstanceCount[MAXTRIMESHES];
		size_t TotalInstanceCount;

		ID3D12Resource*			Instances;
		ID3D12Resource*			TransformsBuffer;
		FrameBufferedResource	RenderARGs;

		ID3D12Resource* NormalBuffer;
		ID3D12Resource* TangentBuffer;
		ID3D12Resource* IndexBuffer;
		ID3D12Resource* VertexBuffer;
		ID3D12Resource* UVBuffer;
		ID3D12Resource* GTBuffer;

		ID3D12CommandSignature*	CommandSignature;
		ID3D12DescriptorHeap*	SRVHeap;

		Shader VShader;
		Shader PShader;
	};


	/************************************************************************************************/


	void BuildGeometryTable		(RenderSystem* RS, iAllocator* TempMemory, StaticMeshBatcher* Batcher);
	void CleanUpStaticBatcher	(StaticMeshBatcher* Batcher);

	
	/************************************************************************************************/

	
	inline float2 PixelToSS(size_t X, size_t Y, uint2 Dimensions) {	return { -1.0f + (float(X)	   / Dimensions[0]), 1.0f - (float(Y)	  / Dimensions[1]) }; } // Assumes screen boundaries are -1 and 1
	inline float2 PixelToSS(uint2 XY, uint2 Dimensions)			  { return { -1.0f + (float(XY[0]) / Dimensions[0]), 1.0f - (float(XY[1]) / Dimensions[1]) }; } // Assumes screen boundaries are -1 and 1

	
	FLEXKITAPI void ReserveTempSpace	( RenderSystem* RS, UploadQueueHandle, size_t Size, void*& CPUMem, size_t& Offset, const size_t alignment = 512);

	FLEXKITAPI void	Release						( RenderSystem* System );
	FLEXKITAPI void Push_DelayedRelease			( RenderSystem* RS, ID3D12Resource* Res);
	FLEXKITAPI void Free_DelayedReleaseResources( RenderSystem* RS);


	FLEXKITAPI void AddTempBuffer	( ID3D12Resource* _ptr, RenderSystem* RS );
	FLEXKITAPI void AddTempShaderRes( ShaderResourceBuffer& ShaderResource, RenderSystem* RS );


	inline DescHeapPOS IncrementHeapPOS(DescHeapPOS POS, size_t Size, size_t INC) {
		const size_t Offset = Size * INC;
		return{ POS.V1.ptr + Offset, POS.V2.ptr + Offset };
	}


	FLEXKITAPI void SetViewport	 ( ID3D12GraphicsCommandList* CL, Texture2D Targets, uint2 Offset = {0, 0} );
	FLEXKITAPI void SetViewports ( ID3D12GraphicsCommandList* CL, Texture2D* Targets, uint2* Offsets, UINT Count = 1u );

	FLEXKITAPI void SetScissors	( ID3D12GraphicsCommandList* CL, uint2* WH, uint2* Offsets, UINT Count );
	FLEXKITAPI void SetScissor	( ID3D12GraphicsCommandList* CL, uint2  WH, uint2  Offset = {0, 0} );

	FLEXKITAPI Texture2D					GetRenderTarget	( RenderWindow* RW );
	FLEXKITAPI ID3D12GraphicsCommandList*	GetCommandList_1 ( RenderSystem* RS );

	FLEXKITAPI void Close					( static_vector<ID3D12GraphicsCommandList*> CLs );
	FLEXKITAPI void ClearBackBuffer			( RenderSystem* RS, ID3D12GraphicsCommandList* CL, RenderWindow* RW, float4 ClearColor );


	const float DefaultClearDepthValues_1[]	= { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, };
	const float DefaultClearDepthValues_0[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, };
	const int   DefaultClearStencilValues[]	= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


	/************************************************************************************************/

	
	FLEXKITAPI bool CreateRenderWindow	( RenderSystem*, RenderWindowDesc* desc_in, RenderWindow* );
	FLEXKITAPI bool	ResizeRenderWindow	( RenderSystem*, RenderWindow* Window, uint2 HW );
	FLEXKITAPI void	SetInputWIndow		( RenderWindow* );
	FLEXKITAPI void	UpdateInput			( void );

	
	/************************************************************************************************/
	// Depreciated API

	FLEXKITAPI void CreateVertexBuffer			( RenderSystem* RS, UploadQueueHandle handle, VertexBufferView** Buffers, size_t BufferCount, VertexBuffer& DVB_Out ); // Expects Index buffer in index 15

	struct SubResourceUpload_Desc
	{
        TextureBuffer*  buffers;

		size_t	        subResourceStart;
		size_t	        subResourceCount;

        FORMAT_2D       format;
	};

	FLEXKITAPI void UpdateResourceByTemp			(RenderSystem* RS, ID3D12Resource* Dest, void* Data, size_t SourceSize, size_t ByteSize = 1, D3D12_RESOURCE_STATES EndState = D3D12_RESOURCE_STATE_COMMON);
	FLEXKITAPI void UpdateResourceByTemp			(RenderSystem* RS, FrameBufferedResource* Dest, void* Data, size_t SourceSize, size_t ByteSize = 1, D3D12_RESOURCE_STATES EndState = D3D12_RESOURCE_STATE_COMMON);
    FLEXKITAPI void _UpdateSubResourceByUploadQueue (RenderSystem* RS, UploadQueueHandle, ID3D12Resource* Dest, SubResourceUpload_Desc* Desc, D3D12_RESOURCE_STATES EndState);


	/************************************************************************************************/


	FLEXKITAPI void	Release( ConstantBuffer&	);
	FLEXKITAPI void	Release( Texture2D			);
	FLEXKITAPI void	Release( RenderWindow*		);
	FLEXKITAPI void	Release( VertexBuffer*		);
	FLEXKITAPI void	Release( Shader*			);


	/************************************************************************************************/


	FLEXKITAPI bool				LoadAndCompileShaderFromFile	(const char* FileLoc, ShaderDesc* desc, Shader* out);
	FLEXKITAPI Shader			LoadShader						(const char* Entry, const char* ID, const char* ShaderVersion, const char* File);
	FLEXKITAPI ResourceHandle	LoadDDSTextureFromFile			(char* file, RenderSystem* RS, UploadQueueHandle, iAllocator* Memout);
	FLEXKITAPI ResourceHandle   LoadTexture						(TextureBuffer* Buffer,  RenderSystem* RS, iAllocator* Memout, FORMAT_2D format = FORMAT_2D::R8G8B8A8_UNORM);


	/************************************************************************************************/


	struct LineSegment
	{
		float3 A;
		float3 AColour;
		float3 B;
		float3 BColour;
	};

	typedef Vector<LineSegment> LineSegments;


	/************************************************************************************************/


	typedef Handle_t<16> LightHandle;

	struct LightDesc{
		NodeHandle	Hndl;
		float3		K;
		float		I;
		float		R;
	};

	
	/************************************************************************************************/


	FLEXKITAPI int2 GetMousedPos					( RenderWindow* Window );
	FLEXKITAPI void HideSystemCursor				( RenderWindow* Window );
	FLEXKITAPI void SetSystemCursorToWindowCenter	( RenderWindow* Window );
	FLEXKITAPI void ShowSystemCursor				( RenderWindow* Window );

	FLEXKITAPI float2 GetPixelSize(RenderWindow& Window);

	
	/************************************************************************************************/


	FLEXKITAPI Texture2D		GetBackBufferTexture	( RenderWindow* Window );
	FLEXKITAPI ID3D12Resource*	GetBackBufferResource	( RenderWindow* Window );


	/************************************************************************************************/


	inline float2 WStoSS		( const float2 XY )	{ return (XY * float2(2, 2)) + float2(-1, -1); }
	inline float2 Position2SS	( const float2 in )	{ return{ in.x * 2 - 1, in.y * -2 + 1 }; }
	inline float3 Grey			( const float P )	{ float V = min(max(0, P), 1); return float3(V, V, V); }


	/************************************************************************************************/


	struct PlaneDesc
	{
		PlaneDesc(float R, Shader V) : VertexShader{ V }
		{
			r = R;
		}

		float r;
		Shader	VertexShader;
	};


	struct Obj_Desc
	{
		Obj_Desc()
		{
			S = 1.0f;

			LoadUVs = false;
			GenerateTangents = false;
		}

		Obj_Desc(float scale, Shader v, bool UV = false, bool T = false) : 
			S{ scale },
			V{ v },
			LoadUVs{UV},
			GenerateTangents{T}{}


		Shader	V;// Vertex Shader
		float	S;

		bool	LoadUVs;
		bool	GenerateTangents;
	};


	/************************************************************************************************/


	FLEXKITAPI TriMeshHandle CreateCube(RenderSystem* RS, iAllocator* Memory, float R, GUID_t MeshID);


	FLEXKITAPI bool LoadObjMesh				( RenderSystem* RS, char* File_Loc,	Obj_Desc IN desc, TriMesh ROUT out, StackAllocator RINOUT LevelSpace, StackAllocator RINOUT TempSpace, bool DiscardBuffers );


	/************************************************************************************************/


	FLEXKITAPI void ReleaseTriMesh			( TriMesh*	p );
	FLEXKITAPI void DelayedReleaseTriMesh	( RenderSystem* RS, TriMesh* T );


	/************************************************************************************************/


	inline void ClearTriMeshVBVs(TriMesh* Mesh) { for (auto& buffer : Mesh->Buffers) buffer = nullptr; }



}	/************************************************************************************************/

#endif
