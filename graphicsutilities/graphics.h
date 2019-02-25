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

	class StackAllocator;
	class RenderSystem;

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
			return sizeof(int32_t) * 4;// 1 Pad
		case DXGI_FORMAT_R32G32B32_FLOAT:
			return sizeof(float) * 3;// assuming 1 Pad
		case DXGI_FORMAT_R32G32B32_UINT:
			return sizeof(float) * 3;// assuming 1 Pad
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
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
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


	enum class FORMAT_2D
	{
		R8_UINT,
		R16_UINT,
		R16G16_UINT,
		R8G8B8A_UINT,
		R8G8B8A8_UINT,
		R8G8B8A8_UNORM,
		R16G16B16A16_UNORM,
		R8G8_UNORM,
		D24_UNORM_S8_UINT,
		R32_FLOAT,
		D32_FLOAT,
		R16G16B16A16_FLOAT,
		R32G32_FLOAT,
		R32G32B32_FLOAT,
		R32G32B32A32_FLOAT,
		BC1_TYPELESS,
		BC1_UNORM,
		BC1_UNORM_SRGB,
		BC2_TYPELESS,
		BC2_UNORM,
		BC2_UNORM_SRGB,
		BC3_TYPELESS,
		BC3_UNORM,
		BC3_UNORM_SRGB,
		BC4_TYPELESS,
		BC4_UNORM,
		BC4_SNORM,
		BC5_TYPELESS,
		BC5_UNORM,
		BC5_SNORM,
		UNKNOWN
	};

	DXGI_FORMAT TextureFormat2DXGIFormat(FORMAT_2D F);
	FORMAT_2D	DXGIFormat2TextureFormat(DXGI_FORMAT F);


	enum class CPUACCESSMODE
	{
		READ,
		WRITE,
		WRITEONCE,
		NONE = 0
	};


	enum SPECIALFLAGS
	{
		BACKBUFFER = 1,
		DEPTHSTENCIL = 2,
		NONE = 0
	};


	/************************************************************************************************/


	struct Texture2D_Desc
	{
		Texture2D_Desc()
		{
			Height       = 0;
			Width        = 0;
			Write        = true;
			Read         = true;
			RenderTarget = false;
			UAV          = false;
			MipLevels    = 0;
			CV           = true;
			initialData  = nullptr;

			Format       = FORMAT_2D::D24_UNORM_S8_UINT;
			FLAGS        = SPECIALFLAGS::NONE;
		}

		Texture2D_Desc(
			size_t			in_height,
			size_t			in_width,
			FORMAT_2D		in_format,
			CPUACCESSMODE	in_Mode,
			SPECIALFLAGS	in_FLAGS,
			size_t			in_MipLevels = 0) : Texture2D_Desc()
		{
			Height		= in_height;
			Width		= in_width;
			Format		= in_format;
			FLAGS		= in_FLAGS;
			MipLevels	= in_MipLevels;
		}

		size_t			Height, Width;
		size_t			MipLevels;
		FORMAT_2D		Format;
		bool			Write;
		bool			Read;
		bool			RenderTarget;
		bool			UAV;
		bool			CV;
		byte*			initialData;
		SPECIALFLAGS	FLAGS;

		inline DXGI_FORMAT	GetFormat()
		{
			switch (Format)
			{
			case FlexKit::FORMAT_2D::R8G8B8A_UINT:
				return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UINT;
			case FlexKit::FORMAT_2D::R8G8B8A8_UINT:
				return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UINT;
			case FlexKit::FORMAT_2D::R8G8B8A8_UNORM:
				return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
			case FlexKit::FORMAT_2D::D24_UNORM_S8_UINT:
				return DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT;
			case FlexKit::FORMAT_2D::R32_FLOAT:
				return DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
			case FlexKit::FORMAT_2D::R32G32_FLOAT:
				return DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
			case FlexKit::FORMAT_2D::R32G32B32_FLOAT:
				return DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
			case FlexKit::FORMAT_2D::R32G32B32A32_FLOAT:
				return DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
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
				break;
			}
			return DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		}
	};


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

	// BackBuffer desc
	struct BufferDesc
	{
		Texture2D_Desc BufferFormat;
		Texture2D_Desc BufferInterp;
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


	struct Texture2D
	{
		ID3D12Resource* operator ->()
		{
			return Texture;
		}

		operator ID3D12Resource*() { return Texture; }
		operator bool() { return Texture != nullptr; }


		ID3D12Resource*		Texture;
		uint2				WH;
		uint8_t				mipCount;
		DXGI_FORMAT			Format;
	};


	/************************************************************************************************/


	struct RenderWindow
	{
		UINT						BufferIndex;
		IDXGISwapChain4*			SwapChain_ptr;
		ID3D12Resource*				BackBuffer[4];
		TextureHandle				RenderTargets[4];
		DXGI_FORMAT					Format;
		HWND						hWindow;

		uint2						WH; // Width-Height
		uint2						WindowCenterPosition;
		Viewport					VP;
		bool						Close;
		bool						Fullscreen;

		EventNotifier<>				Handler;

		TextureHandle	GetBackBuffer()
		{
			return RenderTargets[BufferIndex];
		}

		WORD	InputBuffer[128];
		size_t	InputBuffSize;
		size_t	CurrentBuffer;
		size_t	BufferCount;
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


	inline TextureHandle GetCurrentBackBuffer(RenderWindow* Window) 
	{
		return Window->RenderTargets[Window->BufferIndex];
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
	const static int MaxThreadCount = 6;

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

	struct PerFrameUploadQueue
	{
		PerFrameUploadQueue()
		{
		}


		~PerFrameUploadQueue()
		{
			Release();
		}


		void Open()
		{
			UploadList[0]->Close();
		}

		void Close()
		{
			UploadList[0]->Close();
		}


		void Reset()
		{
			UploadCount = 0;

			HRESULT HR	= UploadCLAllocator[0]->Reset();						FK_ASSERT(SUCCEEDED(HR));
			HR			= UploadList[0]->Reset(UploadCLAllocator[0], nullptr);	FK_ASSERT(SUCCEEDED(HR));
		}


		void Release()
		{
			for (auto& UL : UploadList) {
				if (UL)
					UL->Release();

				UL = nullptr;
			}

			for (auto& alloc : UploadCLAllocator)
			{
				if (alloc)
					alloc->Release();

				alloc = nullptr;
			}
		}

		bool Initiate(ID3D12Device* Device, Vector<ID3D12DeviceChild*>& ObjectsCreated)
		{
			bool Success = true;

			for (size_t I = 0; I < MaxThreadCount && Success; ++I)
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

				UploadCount				= 0;
				UploadCLAllocator[I]	= NewUploadAllocator;
				UploadList[I]			= NewUploadList;
			}

			return Success;
		}

		size_t						UploadCount;
		ID3D12CommandAllocator*		UploadCLAllocator	[MaxThreadCount];
		ID3D12GraphicsCommandList*	UploadList			[MaxThreadCount];
	};


	struct PerFrameResources
	{
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


		TempResourceList			TempBuffers;
		ID3D12GraphicsCommandList3*	CommandLists		[MaxThreadCount];
		ID3D12CommandAllocator*		GraphicsCLAllocator	[MaxThreadCount];
		ID3D12CommandAllocator*		ComputeCLAllocator	[MaxThreadCount];
		ID3D12GraphicsCommandList3*	ComputeList			[MaxThreadCount];
		bool						CommandListsUsed	[MaxThreadCount];

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
		DescriptorHeap(RenderSystem* RS, const DesciptorHeapLayout<16>& Layout_IN, iAllocator* TempMemory);

		DescriptorHeap& Init		(RenderSystem* RS, const DesciptorHeapLayout<16>& Layout_IN, iAllocator* TempMemory);
		DescriptorHeap& Init		(RenderSystem* RS, const DesciptorHeapLayout<16>& Layout_IN, size_t reserveCount, iAllocator* TempMemory);
		DescriptorHeap& NullFill	(RenderSystem* RS);

		bool SetSRV(RenderSystem* RS, size_t Index, TextureHandle Handle);

		operator D3D12_GPU_DESCRIPTOR_HANDLE () const { return descriptorHeap.V2; } // TODO: FIX PAIRS SO AUTO CASTING WORKS

		DescriptorHeap	GetHeapOffsetted(size_t offset, RenderSystem* RS) const;

	private:
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
			Desc.ConstantBuffer.Register		= Register;
			Desc.ConstantBuffer.RegisterSpace	= RegisterSpace;
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
			Desc.ConstantBuffer.Register		= Register;
			Desc.ConstantBuffer.RegisterSpace	= RegisterSpace;
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
			Desc.ConstantBuffer.Register		= Register;
			Desc.ConstantBuffer.RegisterSpace	= RegisterSpace;
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
		DescHeapPOS		GPUHandle;
		TextureHandle	CPUHandle;

		operator DescHeapPOS ()		{ return GPUHandle; }
		operator TextureHandle ()	{ return CPUHandle; }
	};


	/************************************************************************************************/


	struct VertexBufferEntry
	{
		VertexBufferHandle	VertexBuffer	= InvalidHandle_t;
		UINT				Stride			= 0;
		UINT				Offset			= 0;
	};

	typedef static_vector<VertexBufferEntry, 16>	VertexBufferList;
	typedef static_vector<DescHeapPOS, 16>			RenderTargetList;


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


	// Non-copyable
	FLEXKITAPI class IndirectLayout
	{
	public:
		IndirectLayout() :
			entries{ nullptr },
			signature{ nullptr }{}

		IndirectLayout(ID3D12CommandSignature* IN_signature, size_t IN_stride, Vector<IndirectDrawDescription>&& IN_Entries) :
			signature{ IN_signature },
			stride{ IN_stride },
			entries{ std::move(IN_Entries) } {}

		~IndirectLayout()
		{
			signature->Release();
		}


		IndirectLayout(const IndirectLayout& rhs) :
			signature{ rhs.signature },
			entries{ rhs.entries },
			stride{ rhs.stride }
		{
			signature->AddRef();
		}

		IndirectLayout& operator =	(const IndirectLayout& rhs)
		{
			if(rhs.signature)
				rhs.signature->AddRef();

			if (signature)
				signature->Release();

			signature = rhs.signature;
			stride = rhs.stride;
			entries = rhs.entries;

			return (*this);
		}

		operator bool () { return signature != nullptr; }

		ID3D12CommandSignature*			signature;
		size_t							stride;
		Vector<IndirectDrawDescription>	entries;
	};


	/************************************************************************************************/


	FLEXKITAPI class Context
	{
	public:
		Context(
				ID3D12GraphicsCommandList3*	Context_IN = nullptr, 
				RenderSystem*				RS_IN = nullptr, 
				iAllocator*					TempMemory = nullptr) :
			CurrentRootSignature	(nullptr),
			DeviceContext			(Context_IN),
			PendingBarriers			(TempMemory),
			RS						(RS_IN),
			Memory					(TempMemory),
			RenderTargetCount		(0),
			DepthStencilEnabled		(false),
			TrackedSOBuffers		{TempMemory}{}
			
		Context(Context&& RHS) : 
			CurrentRootSignature	(RHS.CurrentRootSignature),
			DeviceContext			(RHS.DeviceContext),
			PendingBarriers			(std::move(RHS.PendingBarriers)),
			RS						(RHS.RS),
			Memory					(RHS.Memory)

		{
			RHS.DeviceContext = nullptr;
		}


		Context& operator = (Context&& RHS)// Moves only
		{
			DeviceContext			= RHS.DeviceContext;
			CurrentRootSignature	= RHS.CurrentRootSignature;
			CurrentPipelineState	= RHS.CurrentPipelineState;
			RS						= RHS.RS;

			RTVPOSCPU				= RHS.RTVPOSCPU;
			DSVPOSCPU				= RHS.DSVPOSCPU;

			RenderTargetCount		= RHS.RenderTargetCount;
			DepthStencilEnabled		= RHS.DepthStencilEnabled;

			Viewports				= RHS.Viewports;
			DesciptorHeaps			= RHS.DesciptorHeaps;
			VBViews					= RHS.VBViews;
			PendingBarriers			= RHS.PendingBarriers;
			Memory					= RHS.Memory;

			// Null out old Context
			RHS.DeviceContext			= nullptr;
			RHS.CurrentRootSignature	= nullptr;

			RHS.RTVPOSCPU               = {0};
			RHS.DSVPOSCPU               = {0};

			RHS.RenderTargetCount	    = 0;
			RHS.DepthStencilEnabled     = false;
			RHS.Memory				    = nullptr;

			RHS.Viewports.clear();
			RHS.DesciptorHeaps.clear();
			RHS.VBViews.clear();
			RHS.PendingBarriers.clear();

			return *this;
		}


		Context				(const Context& RHS) = delete;
		Context& operator = (const Context& RHS) = delete;

		void AddUAVBarrier			(UAVResourceHandle, DeviceResourceState, DeviceResourceState);

		void AddPresentBarrier			(TextureHandle Handle,	DeviceResourceState Before);
		void AddRenderTargetBarrier		(TextureHandle Handle,	DeviceResourceState Before, DeviceResourceState State = DeviceResourceState::DRS_RenderTarget);
		void AddStreamOutBarrier		(SOResourceHandle,		DeviceResourceState Before, DeviceResourceState State);
		void AddShaderResourceBarrier	(TextureHandle Handle,	DeviceResourceState Before, DeviceResourceState State);


		void ClearDepthBuffer		(TextureObject Texture, float ClearDepth = 0.0f); // Assumes full-screen Clear
		void ClearRenderTarget		(TextureObject Texture, float4 ClearColor = float4(0.0f)); // Assumes full-screen Clear

		void SetRootSignature		(RootSignature& RS);
		void SetComputeRootSignature(RootSignature& RS);
		void SetPipelineState		(ID3D12PipelineState* PSO);

		void SetRenderTargets		(static_vector<Texture2D*,16>	RTs);
		void SetRenderTargets		(const static_vector<DescHeapPOS, 16>	RTs, bool DepthStecil, DescHeapPOS DepthStencil = DescHeapPOS());
		//void SetRenderTargets		(const static_vector<TextureObject>		RTs, bool DepthStecil, DescHeapPOS DepthStencil = DescHeapPOS());

		void SetViewports			(static_vector<D3D12_VIEWPORT, 16>	VPs);
		void SetScissorRects		(static_vector<D3D12_RECT, 16>		Rects);

		void SetScissorAndViewports	(static_vector<TextureHandle, 16>	RenderTargets);

		void SetDepthStencil		(Texture2D* DS);
		void SetPrimitiveTopology	(EInputTopology Topology);

		void NullGraphicsConstantBufferView	(size_t idx);
		void SetGraphicsConstantBufferView	(size_t idx, const ConstantBufferHandle CB, size_t Offset = 0);
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

		void SetRTRead	(TextureHandle Handle);
		void SetRTWrite	(TextureHandle Handle);
		void SetRTFree	(TextureHandle Handle);

		// Not Yet Implemented
		void SetUAVRead();
		void SetUAVWrite();
		void SetUAVFree();

		ID3D12GraphicsCommandList*	GetCommandList() { return DeviceContext; }

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
				BT_UAV,
				BT_QueryBuffer,
				BT_VertexBuffer,
				BT_StreamOut,
				BT_ShaderResource,
				BT_Generic,
				BT_
			}Type;

			union 
			{
				UAVResourceHandle	UAV;
				TextureHandle		renderTarget;
				TextureHandle		shaderResource;
				SOResourceHandle	streamOut;
				ID3D12Resource*		resource;
				QueryHandle			query;
			};
		};


		void UpdateResourceStates();
		void UpdateRTVState();

		ID3D12GraphicsCommandList3*		DeviceContext;
		RootSignature*					CurrentRootSignature;
		ID3D12PipelineState*			CurrentPipelineState;
		RenderSystem*					RS;

		D3D12_CPU_DESCRIPTOR_HANDLE RTVPOSCPU;
		D3D12_CPU_DESCRIPTOR_HANDLE DSVPOSCPU;

		size_t	RenderTargetCount;
		bool	DepthStencilEnabled;

		static_vector<TextureHandle, 16>		RenderTargets;
		static_vector<D3D12_VIEWPORT, 16>		Viewports;
		static_vector<DescriptorHeap*>			DesciptorHeaps;
		static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;

		struct StreamOutResource {
			SOResourceHandle	handle;
		};

		Vector<StreamOutResource>				TrackedSOBuffers;
		Vector<Barrier>							PendingBarriers;
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

		ID3D12Resource*		GetResource						(VertexBufferHandle Handle);
		size_t				GetCurrentVertexBufferOffset	(VertexBufferHandle Handle) const;
		size_t				GetBufferSize					(VertexBufferHandle Handle) const;

		SubAllocation		Reserve							(VertexBufferHandle Handle, size_t size);


		void				Release();
		void				ReleaseVertexBuffer(VertexBufferHandle Handle);

	private:
		typedef size_t VBufferHandle;

		VBufferHandle	CreateVertexBufferResource(size_t BufferSize, bool GPUResident, RenderSystem* RS); // Creates Using Placed Resource

		bool CurrentlyAvailable(VertexBufferHandle Handle, size_t CurrentFrame) const;

		struct VertexBuffer
		{
			ID3D12Resource* Resource		= nullptr;
			size_t			ResourceSize	= 0;
			char*			MappedPtr		= 0;
			size_t			NextAvailableFrame; // -1 means static Buffer
		};


		struct UserVertexBuffer
		{
			size_t			CurrentBuffer;
			size_t			Buffers[3];					// Current Buffer
			size_t			ResourceSize;				// Requested Size
			size_t			Offset			 = 0;		// Current Head for Push Buffers
			char*			MappedPtr		 = 0;		//
			ID3D12Resource* Resource		 = nullptr; // To avoid some reads
			bool			WrittenTo		 = false;
			size_t			Padding[2];	

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
		};

		Vector<VertexBuffer>		Buffers;
		Vector<UserVertexBuffer>	UserBuffers;
		Vector<FreeVertexBuffer>	FreeBuffers;

		HandleUtilities::HandleTable<VertexBufferHandle, 32>	Handles;
	};


	/************************************************************************************************/

	typedef Handle_t<16> SubBufferHandle;
	
	FLEXKITAPI struct ConstantBufferTable
	{
		ConstantBufferTable(iAllocator* Memory, RenderSystem* RS_IN) :
			ConstantBuffers(Memory),
			Handles(Memory),
			FreeResourceSets(Memory),
			RS(RS_IN),
			UserBufferEntries(Memory)
		{}

		~ConstantBufferTable()
		{
			for (auto& CB : ConstantBuffers)
			{
				for (auto R : CB.Resources)
				{
					if (R)
						R->Release();

					R = nullptr;
				}
			}


			ConstantBuffers.Release();
			UserBufferEntries.Release();
			FreeResourceSets.Release();
		}

		struct SubAllocation
		{
			char*	Data;
			size_t	offsetBegin;
			size_t	reserveSize;
		};

		struct BufferResourceSet
		{
			ID3D12Resource*	Resources[3];
			size_t BufferSize;
			bool GPUResident;
		};

		struct UserConstantBuffer
		{
			uint32_t	BufferSize;
			uint32_t	BufferSet;
			uint32_t	CurrentBuffer;
			uint32_t	Offset;
			uint32_t	BeginOffset;
			void*		Mapped_ptr;

			Vector<SubAllocation> SubAllocations;
			bool GPUResident;
			bool WrittenTo;

			ConstantBufferHandle Handle;
			size_t Locks[3];

			/*
			TODO: Constant Buffer Pools
			struct MemoryRange
			{
			size_t					Begin;
			size_t					End;
			size_t					BlockCount;
			size_t					BlockSize;
			Vector<MemoryRange*>	ChildRanges;
			}Top;
			*/
		};


		ConstantBufferHandle	CreateConstantBuffer	(size_t BufferSize, bool GPUResident = false);
		void					ReleaseBuffer			(ConstantBufferHandle Handle);

		ID3D12Resource*			GetBufferResource		(const ConstantBufferHandle Handle) const;
		size_t					GetBufferOffset			(const ConstantBufferHandle Handle) const;
		size_t					GetBufferBeginOffset	(const ConstantBufferHandle Handle) const;

		size_t					BeginNewBuffer	(ConstantBufferHandle Handle);
		bool					Push			(ConstantBufferHandle Handle, void* _Ptr, size_t PushSize);
		
		SubAllocation			Reserve					(ConstantBufferHandle Handle, size_t Size);
		size_t					GetSubAllocationSize	(ConstantBufferHandle Handle, SubBufferHandle);
		void					PushSuballocation		(ConstantBufferHandle Handle, SubBufferHandle, void*, size_t PushSize);

		void					LockUntil(size_t);

	private:
		void					UpdateCurrentBuffer(ConstantBufferHandle Handle);

		RenderSystem*				RS;
		iAllocator*					Memory;
		Vector<BufferResourceSet>	ConstantBuffers;
		Vector<UserConstantBuffer>	UserBufferEntries;
		Vector<size_t>				FreeResourceSets;

		HandleUtilities::HandleTable<ConstantBufferHandle>	Handles;
	};


	/************************************************************************************************/


	enum class QueryType
	{
		OcclusionQuery,
		PipelineStats,
	};


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


		DeviceResourceState GetResourceState(const QueryHandle handle) const
		{
			return D3DState2DRS(_GetResourceState(handle));
		}

		D3D12_RESOURCE_STATES _GetResourceState(const QueryHandle handle) const
		{
			auto& res = resources[users[handle].resourceIdx];
			return res.resourceState[res.currentResource];
		}

		ID3D12QueryHeap*	GetResource	(QueryHandle handle)
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


	/************************************************************************************************/


	FLEXKITAPI class TextureStateTable
	{
	public:
		TextureStateTable(iAllocator* memory) :
			Handles		{memory},
			UserEntries	{memory},
			Resources	{memory}{}


		~TextureStateTable()
		{
			Release();
		}

		void Release();

		// No Copy
		TextureStateTable				(const TextureStateTable&) = delete;
		TextureStateTable& operator =	(const TextureStateTable&) = delete;

		Texture2D		operator[]		(TextureHandle Handle);

		TextureHandle	AddResource		(Texture2D_Desc& Desc, ID3D12Resource** Resource, uint32_t ResourceCount, uint8_t mipCount, DeviceResourceState InitialState, uint32_t Flags_IN = 0);
		void			SetState		(TextureHandle Handle, DeviceResourceState State);

		uint2			GetWH(TextureHandle Handle) const;
		size_t			GetFrameGraphIndex(TextureHandle Texture, size_t FrameID) const;
		void			SetFrameGraphIndex(TextureHandle Texture, size_t FrameID, size_t Index);

		DXGI_FORMAT		GetFormat(TextureHandle handle) const;

		uint32_t		GetTag(TextureHandle Handle) const;
		void			SetTag(TextureHandle Handle, uint32_t Tag);


		void			MarkRTUsed		(TextureHandle Handle);

		DeviceResourceState GetState	(TextureHandle Handle) const;
		ID3D12Resource*		GetResource	(TextureHandle Handle);


		void ReleaseTexture	(TextureHandle Handle);
		void LockUntil		(size_t FrameID);

	private:

		struct UserEntry
		{
			size_t				ResourceIdx;
			int64_t				FGI_FrameStamp;
			uint32_t			FrameGraphIndex;
			uint32_t			Flags;
			uint32_t			Tag;
			TextureHandle		Handle;
			DXGI_FORMAT			Format;
			uint32_t			Pad;
		};

		struct ResourceEntry
		{
			void			Release();
			void			SetState(DeviceResourceState State) { States[CurrentResource]		= State;	}
			void			SetFrameLock(size_t FrameID)		{ FrameLocks[CurrentResource]	= FrameID;	}
			ID3D12Resource* GetResource()						{ return Resources[CurrentResource];		}
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
		HandleUtilities::HandleTable<TextureHandle, 32>	Handles;
	};


	/************************************************************************************************/


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
			UAVResourceHandle					resourceHandle;
			uint8_t								resourceIdx;
		};


		UAVResourceHandle AddResource(static_vector<ID3D12Resource*,3> newResources, size_t resourceSize, DeviceResourceState initialState)
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


		ID3D12Resource*	GetResource(const UAVResourceHandle handle) const
		{
			auto& resourceEntry = resources[handles[handle]];
			return resourceEntry.resources[resourceEntry.resourceIdx];
		}


		DeviceResourceState	GetResourceState(const UAVResourceHandle handle) const
		{
			auto& resourceEntry = resources[handles[handle]];
			return resourceEntry.resourceStates[resourceEntry.resourceIdx];
		}

		void SetResourceState(UAVResourceHandle handle, DeviceResourceState state)
		{
			auto& resourceEntry = resources[handles[handle]];
			resourceEntry.resourceStates[resourceEntry.resourceIdx] = state;
		}


		void ReleaseResource(UAVResourceHandle handle)
		{
			size_t idx = handles[handle];
			handles.RemoveHandle(handle);

			for (auto resource : resources[idx].resources)
				resource->Release();

			resources[idx] = resources.back();
			handles[resources.back().resourceHandle] = idx;
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

		HandleUtilities::HandleTable<UAVResourceHandle, 32>	handles;
		Vector<UAVResource>									resources;
		iAllocator*											allocator;
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
					resources.push_back({
						newResources,
						counters,
						resourceSize,
						{initialState, initialState, initialState},
						newHandle,
						0 });

			return newHandle;
		}


		ID3D12Resource*	GetResource(SOResourceHandle handle) const
		{
			auto& resourceEntry = resources[handles[handle]];
			return resourceEntry.resources[resourceEntry.resourceIdx];
		}


		ID3D12Resource* GetResourceCounter(SOResourceHandle handle) const
		{
			auto& resourceEntry = resources[handles[handle]];
			return resourceEntry.resourceCounters[resourceEntry.resourceIdx];
		}


		size_t GetResourceSize(SOResourceHandle handle) const
		{
			auto& resourceEntry = resources[handles[handle]];
			return resourceEntry.resourceSize;
		}


		DeviceResourceState	GetResourceState(SOResourceHandle handle) const
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
	ID3D12PipelineState* CreateDrawTriStatePSO	( RenderSystem* RS );
	ID3D12PipelineState* CreateDrawLineStatePSO	( RenderSystem* RS );
	ID3D12PipelineState* CreateDraw2StatePSO	( RenderSystem* RS );


	/************************************************************************************************/


	FLEXKITAPI class RenderSystem
	{
	public:
		RenderSystem(iAllocator* Memory_IN, ThreadManager* Threads) :
			Memory			{ Memory_IN					},
			Library			{ Memory_IN					},
			Queries			{ Memory_IN, this			},
			RenderTargets	{ Memory_IN					},
			Textures		{ Memory_IN					},
			VertexBuffers	{ Memory_IN					},
			ConstantBuffers	{ Memory_IN, this			},
			PipelineStates	{ Memory_IN, this, Threads	},
			BufferUAVs		{ Memory_IN					},
			Texture2DUAVs	{ Memory_IN					},
			StreamOutTable	{ Memory_IN					}{}

		
		~RenderSystem() { Release(); }

		RenderSystem				(const RenderSystem&) = delete;
		RenderSystem& operator =	(const RenderSystem&) = delete;

		bool	Initiate(Graphics_Desc* desc_in);
		void	Release();

		// Pipeline State
		size_t						GetCurrentFrame();
		ID3D12PipelineState*		GetPSO				(PSOHandle StateID);
		RootSignature const * const GetPSORootSignature	(PSOHandle StateID) const;

		void RegisterPSOLoader	(PSOHandle State, PipelineStateDescription desc);
		void QueuePSOLoad		(PSOHandle State);

		void BeginSubmission	(RenderWindow* Window);
		void Submit				(static_vector<ID3D12CommandList*>& CLs);
		void PresentWindow		(RenderWindow* RW);
		void WaitforGPU			();

		size_t						GetVertexBufferSize		(const VertexBufferHandle);
		D3D12_GPU_VIRTUAL_ADDRESS	GetVertexBufferAddress	(const VertexBufferHandle VB);
		D3D12_GPU_VIRTUAL_ADDRESS	GetConstantBufferAddress(const ConstantBufferHandle CB);


		size_t	GetTextureFrameGraphIndex(TextureHandle);
		void	SetTextureFrameGraphIndex(TextureHandle, size_t);

		uint32_t	GetTag(TextureHandle Handle);
		void		SetTag(TextureHandle Handle, uint32_t);

		const size_t	GetTextureElementSize	(TextureHandle Handle) const;
		const uint2		GetTextureWH			(TextureHandle Handle) const;
		const uint2		GetRenderTargetWH		(TextureHandle Handle) const;
		FORMAT_2D		GetTextureFormat		(TextureHandle Handle) const;

		void			UploadTexture				(TextureHandle, byte* buffer, size_t bufferSize); // Uses Upload Queue
		void			UploadTexture				(TextureHandle handle, byte* buffer, size_t bufferSize, uint2 WH, size_t resourceCount, size_t* mipOffsets, iAllocator* temp); // Uses Upload Queue
		void			UpdateResourceByUploadQueue	(ID3D12Resource* Dest, void* Data, size_t Size, size_t ByteSize, D3D12_RESOURCE_STATES EndState);

		// Resource Creation and Destruction
		ConstantBufferHandle	CreateConstantBuffer			(size_t BufferSize, bool GPUResident = true);
		VertexBufferHandle		CreateVertexBuffer				(size_t BufferSize, bool GPUResident = true);
		TextureHandle			CreateDepthBuffer				(uint2 WH, bool UseFloat = false, size_t bufferCount = 3);
		TextureHandle			CreateTexture2D					(uint2 WH, FORMAT_2D Format, size_t MipLevels = 1);
		TextureHandle			CreateTexture2D					(uint2 WH, FORMAT_2D Format, size_t MipLevels, ID3D12Resource** Resources, size_t ResourceCount = 1);
		QueryHandle				CreateOcclusionBuffer			(size_t Size);
		UAVResourceHandle		CreateUAVBufferResource			(size_t bufferHandle, bool tripleBuffer = true);
		SOResourceHandle		CreateStreamOutResource			(size_t bufferHandle, bool tripleBuffer = true);
		QueryHandle				CreateSOQuery					(size_t SOIndex,			size_t count);
		IndirectLayout			CreateIndirectLayout			(static_vector<IndirectDrawDescription> entries, iAllocator* allocator);

		void SetObjectState(SOResourceHandle	handle,	DeviceResourceState state);
		void SetObjectState(UAVResourceHandle	handle, DeviceResourceState state);
		void SetObjectState(TextureHandle		handle,	DeviceResourceState state);

		DeviceResourceState GetObjectState			(const QueryHandle			handle) const;
		DeviceResourceState GetObjectState			(const SOResourceHandle		handle) const;
		DeviceResourceState GetObjectState			(const UAVResourceHandle	handle) const;
		DeviceResourceState GetObjectState			(const TextureHandle		handle) const;


		ID3D12Resource*		GetObjectDeviceResource	(const ConstantBufferHandle	handle) const;
		ID3D12Resource*		GetObjectDeviceResource (const SOResourceHandle		handle) const;
		ID3D12Resource*		GetObjectDeviceResource	(const UAVResourceHandle	handle) const;

		ID3D12Resource*		GetSOCounterResource	(const SOResourceHandle handle) const;
		size_t				GetStreamOutBufferSize	(const SOResourceHandle handle) const;

		void ResetQuery(QueryHandle handle);

		void ReleaseCB(ConstantBufferHandle);
		void ReleaseVB(VertexBufferHandle);
		void ReleaseDB(TextureHandle);
		void ReleaseTexture(TextureHandle Handle);
		void ReleaseTempResources();

		// Upload Queues
		void ShutDownUploadQueues();
		void WaitForUploadQueue();
		void ReadyUploadQueues();
		void UploadResources();
		void SubmitUploadQueues();


		// Internal
		//TextureHandle			_AddBackBuffer						(Texture2D_Desc& Desc, ID3D12Resource* Res, uint32_t Tag);
		static ConstantBuffer	_CreateConstantBufferResource		(RenderSystem* RS, ConstantBuffer_desc* desc);
		VertexResourceBuffer	_CreateVertexBufferDeviceResource	(const size_t ResourceSize, bool GPUResident = true);

		DescHeapPOS				_ReserveDescHeap			(size_t SlotCount = 1 );
		DescHeapPOS				_ReserveGPUDescHeap			(size_t SlotCount = 1 );

		DescHeapPOS				_ReserveRTVHeap				(size_t SlotCount );
		DescHeapPOS				_ReserveDSVHeap				(size_t SlotCount );

		ID3D12Resource*			_GetTextureResource(TextureHandle handle);

		void _ResetDescHeap();
		void _ResetRTVHeap();
		void _ResetGPUDescHeap();
		void _ResetDSVHeap();

		size_t _GetVidMemUsage();

		ID3D12DescriptorHeap*		_GetCurrentRTVTable();
		ID3D12DescriptorHeap*		_GetCurrentDescriptorTable();
		ID3D12DescriptorHeap*		_GetCurrentDSVTable();
		D3D12_GPU_DESCRIPTOR_HANDLE	_GetDescTableCurrentPosition_GPU();
		D3D12_GPU_DESCRIPTOR_HANDLE	_GetRTVTableCurrentPosition_GPU();
		D3D12_CPU_DESCRIPTOR_HANDLE	_GetRTVTableCurrentPosition_CPU();
		D3D12_CPU_DESCRIPTOR_HANDLE	_GetDSVTableCurrentPosition_CPU();
		D3D12_GPU_DESCRIPTOR_HANDLE	_GetDSVTableCurrentPosition_GPU();
		D3D12_GPU_DESCRIPTOR_HANDLE	_GetGPUDescTableCurrentPosition_GPU();

		ID3D12QueryHeap*			_GetQueryResource(QueryHandle handle);

		ID3D12CommandAllocator*		_GetCurrentCommandAllocator();
		ID3D12GraphicsCommandList3*	_GetCurrentCommandList();
		PerFrameResources*			_GetCurrentFrameResources();
		ID3D12GraphicsCommandList3*	_GetCommandList_1();
		PerFrameUploadQueue&		_GetCurrentUploadQueue();

		void						_IncrementRSIndex() { CurrentIndex = (CurrentIndex + 1) % 3; }

		operator RenderSystem* () { return this; }

		ID3D12Device*			pDevice			= nullptr;
		ID3D12CommandQueue*		GraphicsQueue	= nullptr;
		ID3D12CommandQueue*		UploadQueue		= nullptr;
		ID3D12CommandQueue*		ComputeQueue	= nullptr;

		size_t CurrentFrame			= 0;
 		size_t CurrentIndex			= 0;
		size_t CurrentUploadIndex	= 0;
		size_t FenceCounter			= 0;
		size_t FenceUploadCounter	= 0;

		ID3D12Fence* Fence			= nullptr;
		ID3D12Fence* CopyFence		= nullptr;

		PerFrameResources		FrameResources	[QueueSize];
		PerFrameUploadQueue		UploadQueues	[QueueSize];

		IDXGIFactory5*			pGIFactory		= nullptr;
		IDXGIAdapter4*			pDXGIAdapter	= nullptr;

		ConstantBuffer			NullConstantBuffer; // Zero Filled Constant Buffer
		Texture2D				NullUAV; // 1x1 Zero UAV
		Texture2D				NullSRV;
		ShaderResourceBuffer	NullSRV1D;

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
				RSDefault			(Memory),
				ShadingRTSig		(Memory),
				RS4CBVs_SO			(Memory),
				RS6CBVs4SRVs		(Memory),
				RS2UAVs4SRVs4CBs	(Memory) {}

			void Initiate(RenderSystem* RS, iAllocator* TempMemory);

			RootSignature RS2UAVs4SRVs4CBs; // 4CBVs On all Stages, 4 SRV On all Stages
			RootSignature RS6CBVs4SRVs;		// 4CBVs On all Stages, 4 SRV On all Stages
			RootSignature RS4CBVs_SO;		// Stream Out Enabled
			RootSignature ShadingRTSig;		// Signature For Compute Based Deferred Shading
			RootSignature RSDefault;		// Default Signature for Rasting
		}Library;

		ConstantBufferTable		ConstantBuffers;
		QueryTable				Queries;

		VertexBufferStateTable	VertexBuffers;
		TextureStateTable		RenderTargets;
		TextureStateTable		Textures;
		UAVResourceTable		BufferUAVs;
		UAVResourceTable		Texture2DUAVs;
		SOResourceTable			StreamOutTable;
		PipelineStateTable		PipelineStates;

		struct FreeEntry
		{
			bool operator == (const FreeEntry& rhs)
			{
				return rhs.Resource == Resource;
			}

			ID3D12Resource* Resource	= nullptr;
			size_t			Counter		= 0;
		};

		Vector<FreeEntry>	FreeList;

		ID3D12Debug*			pDebug			= nullptr;
		ID3D12DebugDevice*		pDebugDevice	= nullptr;
		iAllocator*				Memory			= nullptr;
	};

	
	/************************************************************************************************/


	FLEXKITAPI DescHeapPOS PushRenderTarget			(RenderSystem* RS, Texture2D* Target, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushDepthStencil			(RenderSystem* RS, Texture2D* Target, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushCBToDescHeap			(RenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, size_t BufferSize);
	FLEXKITAPI DescHeapPOS PushSRVToDescHeap		(RenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, size_t ElementCount, size_t Stride, D3D12_BUFFER_SRV_FLAGS Flags = D3D12_BUFFER_SRV_FLAG_NONE);
	FLEXKITAPI DescHeapPOS Push2DSRVToDescHeap		(RenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, D3D12_BUFFER_SRV_FLAGS = D3D12_BUFFER_SRV_FLAG_NONE);
	FLEXKITAPI DescHeapPOS PushTextureToDescHeap	(RenderSystem* RS, Texture2D tex, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushUAV2DToDescHeap		(RenderSystem* RS, Texture2D tex, DescHeapPOS POS, DXGI_FORMAT F = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);


	/************************************************************************************************/


	TextureHandle MoveTextureBuffersToVRAM	(RenderSystem* RS, TextureBuffer* buffer, size_t bufferCount, iAllocator* tempMemory);
	TextureHandle MoveTextureBufferToVRAM	(RenderSystem* RS, TextureBuffer* buffer, iAllocator* tempMemory);


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
			size_t				BufferSizeInBytes;
			size_t				BufferStride;
			VERTEXBUFFER_TYPE	Type;

			operator bool()	{ return Buffer != nullptr; }
			operator ID3D12Resource* ()	{ return Buffer; }
		};

		void clear() { VertexBuffers.clear(); }

		ID3D12Resource*	operator[](size_t idx)	{ return VertexBuffers[idx].Buffer; }
		static_vector<BuffEntry, 16>	VertexBuffers;
		TriangleMeshMetaData			MD;
	};
	

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

		operator ConstantBufferHandle () { return CB; }


		template<typename _TY>
		size_t Push(_TY& data)
		{
			if (pushBufferUsed + sizeof(data) > pushBufferSize)
				return -1;

			size_t offset = pushBufferUsed;
			memcpy(pushBufferBegin + buffer + pushBufferUsed, (char*)&data, sizeof(data));
			pushBufferUsed += (sizeof(data) / 256 + 1) * 256;

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


		size_t begin() { return pushBufferBegin; }

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
		VBPushBuffer(VertexBufferHandle vBuffer = InvalidHandle_t, char* buffer_ptr = nullptr, size_t offsetBegin = 0, size_t reservedSize = 0) :
			VB					{ vBuffer		},
			buffer				{ buffer_ptr	},
			pushBufferSize		{ reservedSize	},
			pushBufferOffset	{ offsetBegin	} {}


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
		};

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
		size_t Push(_TY& data)
		{
			if (pushBufferUsed + sizeof(data) > pushBufferSize)
				return -1;

			size_t offset = pushBufferUsed;
			memcpy(pushBufferOffset + buffer + pushBufferUsed, (char*)&data, sizeof(data));
			pushBufferUsed += sizeof(data);

			return offset;
		}


		size_t Push(char* _ptr, size_t size)
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


	struct TriMesh
	{
		TriMesh()
		{
			for ( auto b : Buffers )
				b = nullptr;

			ID				= nullptr;
			AnimationData	= EAD_None;
			Memory			= nullptr;
		}


		TriMesh( const TriMesh& rhs )
		{
			memcpy(this, &rhs, sizeof(TriMesh));
		}

		size_t AnimationData;
		size_t IndexCount;
		size_t TriMeshID;

		struct SubDivInfo
		{
			size_t  numVertices;
			size_t  numFaces;
			int*	numVertsPerFace;
			int*	IndicesPerFace;
		}*SubDiv;

		const char*			ID;
		SkinDeformer*		SkinTable;
		Skeleton*			Skeleton;
		VertexBufferView*	Buffers[16];
		VertexBuffer		VertexBuffer;

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
		iAllocator*	Memory;
	};


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

	FLEXKITAPI TriMeshHandle	BuildMesh	(RenderSystem* RS, Mesh_Description* Desc, TriMeshHandle guid);


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

		TextureHandle	Textures[16];
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

		ID3D12Resource*	GetResource() {
			RenderTargets.Get();
		}

		void Increment() {
			RenderTargets.IncrementCounter();
		}

		uint2				WH;
		DXGI_FORMAT			Format;
	};

	FrameBufferedRenderTarget CreateFrameBufferedRenderTarget(RenderSystem* RS, Texture2D_Desc* Desc);

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

	FLEXKITAPI VertexBuffer::BuffEntry* GetBuffer( VertexBuffer*, VERTEXBUFFER_TYPE ); // return nullptr if not found
	
	FLEXKITAPI void ReserveTempSpace	( RenderSystem* RS, size_t Size, void*& CPUMem, size_t& Offset );

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
	FLEXKITAPI void CloseAndSubmit			( static_vector<ID3D12GraphicsCommandList*> CLs, RenderSystem* RS, RenderWindow* Window);
	FLEXKITAPI void ClearBackBuffer			( RenderSystem* RS, ID3D12GraphicsCommandList* CL, RenderWindow* RW, float4 ClearColor );


	const float DefaultClearDepthValues_1[]	= { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, };
	const float DefaultClearDepthValues_0[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, };
	const int   DefaultClearStencilValues[]	= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


	FLEXKITAPI void SetDepthBuffersRead		( RenderSystem* RS, ID3D12GraphicsCommandList* CL, static_vector<Texture2D> DB );
	FLEXKITAPI void SetDepthBuffersWrite	( RenderSystem* RS, ID3D12GraphicsCommandList* CL, static_vector<Texture2D> DB );
	FLEXKITAPI void ClearDepthBuffers		( RenderSystem* RS, ID3D12GraphicsCommandList* CL, static_vector<Texture2D> DB, const float ClearValue[] = DefaultClearDepthValues_1, const int Stencil[] = DefaultClearStencilValues, const size_t count = 1);
	//FLEXKITAPI void ClearDepthBuffer		( RenderSystem* RS, ID3D12GraphicsCommandList* CL, Texture2D* DB, float ClearValue = 1.0f, int Stencil = 0);
	

	/************************************************************************************************/

	
	FLEXKITAPI bool					CreateRenderWindow			( RenderSystem*, RenderWindowDesc* desc_in, RenderWindow* );
	FLEXKITAPI bool					ResizeRenderWindow			( RenderSystem*, RenderWindow* Window, uint2 HW );
	FLEXKITAPI void					SetInputWIndow				( RenderWindow* );
	FLEXKITAPI void					UpdateInput					( void );

	
	/************************************************************************************************/
	// Depreciated API

	FLEXKITAPI bool					CreateDepthBuffer			( RenderSystem* RS, uint2				Dimensions,	DepthBuffer_Desc&	DepthDesc,	DepthBuffer* out, ID3D12GraphicsCommandList* CL = nullptr );
	FLEXKITAPI Texture2D			CreateDepthBufferResource	( RenderSystem* RS, Texture2D_Desc*		desc_in,	bool				Float32);
	FLEXKITAPI bool					CreateInputLayout			( RenderSystem* RS, VertexBufferView**,  size_t count, Shader*, VertexBuffer* OUT );		// Expects Index buffer in index 15
	FLEXKITAPI Texture2D			CreateTexture2D				( RenderSystem* RS, Texture2D_Desc* desc_in );
	FLEXKITAPI void					CreateVertexBuffer			( RenderSystem* RS, VertexBufferView** Buffers, size_t BufferCount, VertexBuffer& DVB_Out ); // Expects Index buffer in index 15
	FLEXKITAPI ShaderResourceBuffer CreateShaderResource		( RenderSystem* RS, const size_t ResourceSize, const char* _DebugName = "CreateShaderResource" );
	FLEXKITAPI StreamOutBuffer		CreateStreamOut				( RenderSystem* RS, const size_t ResourceSize );
	FLEXKITAPI VertexResourceBuffer CreateVertexBufferResource	( RenderSystem* RS, const size_t ResourceSize, bool GPUResident);
	FLEXKITAPI VertexBufferView*	CreateVertexBufferView		( byte*, size_t );
	FLEXKITAPI QueryResource		CreateSOQuery				( RenderSystem* RS, D3D12_QUERY_HEAP_TYPE Type = D3D12_QUERY_HEAP_TYPE_SO_STATISTICS );

	FLEXKITAPI TextureHandle CreateRenderTarget	(RenderSystem* RS, Texture2D_Desc& Desc, uint32_t Tag);

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
		size_t*	SubResourceOffset;
		size_t	ElementSize;
	};

	FLEXKITAPI void UpdateResourceByTemp			( RenderSystem* RS, ID3D12Resource* Dest, void* Data, size_t SourceSize, size_t ByteSize = 1, D3D12_RESOURCE_STATES EndState = D3D12_RESOURCE_STATE_COMMON);
	FLEXKITAPI void UpdateResourceByTemp			( RenderSystem* RS, FrameBufferedResource* Dest, void* Data, size_t SourceSize, size_t ByteSize = 1, D3D12_RESOURCE_STATES EndState = D3D12_RESOURCE_STATE_COMMON);
	FLEXKITAPI void UpdateSubResourceByUploadQueue	( RenderSystem* RS, ID3D12Resource* Dest, SubResourceUpload_Desc* Desc, D3D12_RESOURCE_STATES EndState);


	/************************************************************************************************/


	FLEXKITAPI void	Release( ConstantBuffer&	);
	FLEXKITAPI void	Release( Texture2D			);
	FLEXKITAPI void	Release( RenderWindow*		);
	FLEXKITAPI void	Release( VertexBuffer*		);
	FLEXKITAPI void	Release( Shader*			);
	FLEXKITAPI void	Release( SpotLightList*	);
	FLEXKITAPI void	Release( PointLightList*	);
	FLEXKITAPI void Release( DepthBuffer*		);


	/************************************************************************************************/


	FLEXKITAPI bool				LoadAndCompileShaderFromFile	(const char* FileLoc, ShaderDesc* desc, Shader* out);
	FLEXKITAPI Shader			LoadShader						(const char* Entry, const char* ID, const char* ShaderVersion, const char* File);
	FLEXKITAPI TextureHandle	LoadDDSTextureFromFile			(char* file, RenderSystem* RS, iAllocator* Memout);
	FLEXKITAPI Texture2D		LoadTexture						(TextureBuffer* Buffer,  RenderSystem* RS, iAllocator* Memout);

	FLEXKITAPI TextureBuffer CreateTextureBuffer			(size_t Width, size_t Height, iAllocator* Memout);


	FLEXKITAPI void			FreeTexture						(Texture2D* Tex);


	/************************************************************************************************/
	

	FLEXKITAPI Texture2D GetRenderTarget(RenderWindow* in);

	struct PIXELPROCESS_DESC
	{

	};


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

	FLEXKITAPI float2 GetPixelSize(RenderWindow* Window);

	
	/************************************************************************************************/


	FLEXKITAPI void CreatePointLightList	( PointLightList* out, PointLightListDesc Desc, iAllocator* Mem );
	FLEXKITAPI void CreateSpotLightList	( SpotLightList* out, iAllocator* Memory, size_t Max = 512 );

	FLEXKITAPI void Release	( PointLightList* out,	iAllocator* Memory );
	FLEXKITAPI void Release	( SpotLightList* out,		iAllocator* Memory );

	FLEXKITAPI LightHandle CreateLight		( PointLightList*	PL, LightDesc& in );
	FLEXKITAPI LightHandle CreateLight		( SpotLightList*	SL, LightDesc& in, float3 Dir, float p );
	
	FLEXKITAPI void ReleaseLight(PointLightList*	PL, LightHandle Handle);


	/************************************************************************************************/
	

	template<typename Ty>
	void SetLightFlag( Ty* out, LightHandle light, LightBufferFlags flag )	{out->Flags.at(light.INDEX) ^= flag;}


	/************************************************************************************************/


	FLEXKITAPI void InitiateShadowMapPass	( RenderSystem* RenderSystem, ShadowMapPass* out);
	FLEXKITAPI void CleanUpShadowPass		( ShadowMapPass* Out );


	/************************************************************************************************/

	//FLEXKITAPI void RenderShadowMap				( RenderSystem* RS, PVS* _PVS, SpotLightShadowCaster* Caster, Texture2D* RenderTarget, ShadowMapPass* PSOs, GeometryTable* GT );


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