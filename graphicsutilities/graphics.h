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


	enum class VERTEXBUFFER_TYPE
	{
		VERTEXBUFFER_TYPE_COLOR,
		VERTEXBUFFER_TYPE_NORMAL,
		VERTEXBUFFER_TYPE_TANGENT,
		VERTEXBUFFER_TYPE_UV,
		VERTEXBUFFER_TYPE_POSITION,
		VERTEXBUFFER_TYPE_USERTYPE,
		VERTEXBUFFER_TYPE_USERTYPE2,
		VERTEXBUFFER_TYPE_USERTYPE3,
		VERTEXBUFFER_TYPE_USERTYPE4,
		VERTEXBUFFER_TYPE_COMBINED,
		VERTEXBUFFER_TYPE_PACKED,
		VERTEXBUFFER_TYPE_PACKEDANIMATION,
		VERTEXBUFFER_TYPE_INDEX,
		VERTEXBUFFER_TYPE_ANIMATION1,
		VERTEXBUFFER_TYPE_ANIMATION2,
		VERTEXBUFFER_TYPE_ANIMATION3,
		VERTEXBUFFER_TYPE_ANIMATION4,
		VERTEXBUFFER_TYPE_ERROR
	};


	/************************************************************************************************/


	enum class VERTEXBUFFER_FORMAT
	{
		VERTEXBUFFER_FORMAT_UNKNOWN = -1,
		VERTEXBUFFER_FORMAT_R8 = 1,
		VERTEXBUFFER_FORMAT_R8G8B8 = 3,
		VERTEXBUFFER_FORMAT_R8G8B8A8 = 8,
		VERTEXBUFFER_FORMAT_R16 = 2,
		VERTEXBUFFER_FORMAT_R16G16 = 4,
		VERTEXBUFFER_FORMAT_R16G16B16 = 6,
		VERTEXBUFFER_FORMAT_R16G16B16A16 = 8,
		VERTEXBUFFER_FORMAT_R32 = 4,
		VERTEXBUFFER_FORMAT_R32G32 = 8,
		VERTEXBUFFER_FORMAT_R32G32B32 = 12,
		VERTEXBUFFER_FORMAT_R32G32B32A32 = 16,
		VERTEXBUFFER_FORMAT_MATRIX = 64,
		VERTEXBUFFER_FORMAT_COMBINED = 32
	};


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

	typedef Handle_t<32, GetTypeGUID(ConstantBuffer)>						ConstantBufferHandle;
	typedef Handle_t<32, GetTypeGUID(VertexBuffer)>							VertexBufferHandle;
	typedef Handle_t<32, GetTypeGUID(TextureHandle)>						TextureHandle;
	typedef Handle_t<32, GetTypeGUID(QueryBuffer)>							QueryBufferHandle;
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


	class FLEXKITAPI VertexBufferView
	{
	public:
		VertexBufferView();
		VertexBufferView(byte* _ptr, size_t size);
		~VertexBufferView();

		VertexBufferView  operator + (const VertexBufferView& RHS);

		VertexBufferView& operator = (const VertexBufferView& RHS);	// Assignment Operator
		//VertexBuffer& operator = ( const VertexBuffer&& RHS );	// Movement Operator


		template<typename Ty>
		class Typed_Iteration
		{
			typedef Typed_Iteration<Ty> This_Type;
		public:
			Typed_Iteration(byte* _ptr, size_t Size) : m_position((Ty*)(_ptr)), m_size(Size) {}

			class Typed_iterator
			{
			public:
				Typed_iterator(Ty* _ptr) : m_position(_ptr) {}
				Typed_iterator(const Typed_iterator& rhs) : m_position(rhs.m_position) {}

				inline void operator ++ () { m_position++; }
				inline void operator ++ (int) { m_position++; }
				inline void operator -- () { m_position--; }
				inline void operator -- (int) { m_position--; }

				inline Typed_iterator	operator =	(const Typed_iterator& rhs) {}

				inline bool operator <	(const Typed_iterator& rhs) { return (this->m_position < rhs.m_position); }


				inline bool				operator == (const Typed_iterator& rhs) { return  (m_position == &*rhs); }
				inline bool				operator != (const Typed_iterator& rhs) { return !(m_position == &*rhs); }

				inline Ty&				operator * () { return *m_position; }
				inline Ty				operator * ()	const { return *m_position; }

				inline const Ty&		peek_forward()	const { return *(m_position + sizeof(Ty)); }

			private:

				Ty*	m_position;
			};

			Ty&	operator [] (size_t index) { return m_position[index]; }
			Typed_iterator begin() { return Typed_iterator(m_position); }
			Typed_iterator end() { return Typed_iterator(m_position + m_size); }

			inline const size_t	size() { return m_size / sizeof(Ty); }

		private:
			Ty*			m_position;
			size_t		m_size;
		};


		/************************************************************************************************/


		template< typename TY >
		inline bool Push(TY& in)
		{
			if (mBufferUsed + sizeof(TY) > mBufferSize)
				return false;

			auto size = sizeof(TY);
			if (sizeof(TY) != static_cast<uint32_t>(mBufferFormat))
				mBufferinError = true;
			else if (!mBufferinError)
			{
				char* Val = (char*)&in;
				for (size_t itr = 0; itr < static_cast<uint32_t>(mBufferFormat); itr++)
					mBuffer[mBufferUsed++] = Val[itr];
			}
			return !mBufferinError;
		}


		/************************************************************************************************/


		template< typename TY >
		inline bool Push(TY& in, size_t bytesize)
		{
			if (mBufferUsed + bytesize > mBufferSize)
				return false;

			char* Val = (char*)&in;
			for (size_t itr = 0; itr < bytesize; itr++)
				mBuffer.push_back(Val[itr]);

			return !mBufferinError;
		}


		/************************************************************************************************/


		template<>
		inline bool Push(float3& in)
		{
			if (mBufferUsed + static_cast<uint32_t>(mBufferFormat) > mBufferSize)
				return false;

			char* Val = (char*)&in;
			for (size_t itr = 0; itr < static_cast<uint32_t>(mBufferFormat); itr++)
				mBuffer[mBufferUsed++] = Val[itr];
			return !mBufferinError;
		}


		/************************************************************************************************/


		template<typename TY>
		inline bool Push(TY* in)
		{
			if (mBufferUsed + sizeof(TY) > mBufferSize)
				return false;

			if (!mBufferinError)
			{
				char Val[128];
				memcpy(Val, &in, mBufferFormat);

				for (size_t itr = 0; itr < static_cast<uint32_t>(mBufferFormat); itr++)
					mBuffer.push_back(Val[itr]);
			}
			return !mBufferinError;
		}


		/************************************************************************************************/


		template<typename Ty>
		inline Typed_Iteration<Ty> CreateTypedProxy()
		{
			return Typed_Iteration<Ty>(GetBuffer(), GetBufferSizeUsed());
		}


		template<typename Ty>
		inline bool Push(vector_t<Ty> static_vector)
		{
			if (mBufferUsed + sizeof(Ty)*static_vector.size() > mBufferSize)
				return false;

			for (auto in : static_vector)
				if (!Push(in)) return false;

			return true;
		}


		template<typename Ty, size_t count>
		inline bool Push(static_vector<Ty, count>& static_vector)
		{
			if (mBufferUsed + sizeof(Ty)*static_vector.size() > mBufferSize)
				return false;

			for (auto in : static_vector)
				if (!Push(in)) return false;

			return true;
		}


		void Begin(VERTEXBUFFER_TYPE, VERTEXBUFFER_FORMAT);
		bool End();

		bool				LoadBuffer();
		bool				UnloadBuffer();

		byte*				GetBuffer()			const;
		size_t				GetElementSize()	const;
		size_t				GetBufferSize()		const;
		size_t				GetBufferSizeUsed()	const;
		size_t				GetBufferSizeRaw()	const;
		VERTEXBUFFER_FORMAT	GetBufferFormat()	const;
		VERTEXBUFFER_TYPE	GetBufferType()		const;

		void SetTypeFormatSize(VERTEXBUFFER_TYPE, VERTEXBUFFER_FORMAT, size_t count);

	private:
		bool _combine(const VertexBufferView& LHS, const VertexBufferView& RHS, char* out);

		void				SetElementSize(size_t) {}
		byte*				mBuffer;
		size_t				mBufferSize;
		size_t				mBufferUsed;
		size_t				mBufferElementSize;
		VERTEXBUFFER_FORMAT	mBufferFormat;
		VERTEXBUFFER_TYPE	mBufferType;
		bool				mBufferinError;
		bool				mBufferLock;
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
		SHADER_TYPE_Vertex
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


	struct TextureBuffer
	{
		byte*		Buffer;
		uint2		WH;
		size_t		Size;
		size_t		ElementSize;
		iAllocator* Memory;

		void Release()
		{
			Memory->_aligned_free(Buffer);
		}
	};


	template<typename TY>
	struct TextureBufferView
	{
		TextureBufferView(TextureBuffer* Buffer) : Texture(Buffer) {}

		TY& operator [](uint2 XY)
		{
			TY* buffer = (TY*)Texture->Buffer;
			return buffer[Texture->WH[1] * XY[1] + XY[0]];
		}

		TextureBuffer* Texture;
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
		ID3D12GraphicsCommandList*	CommandLists		[MaxThreadCount];
		ID3D12CommandAllocator*		GraphicsCLAllocator	[MaxThreadCount];
		ID3D12CommandAllocator*		ComputeCLAllocator	[MaxThreadCount];
		ID3D12GraphicsCommandList*	ComputeList			[MaxThreadCount];
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
				out += (e.Space > 0) ? e.Space : 1;

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
		DRS_VERTEXBUFFER	 = 0x000C, // Implied Read
		DRS_CONSTANTBUFFER	 = 0x000C, // Implied Read
		DRS_DEPTHBUFFER		 = 0x0030,
		DRS_DEPTHBUFFERREAD  = 0x0030 | DRS_Read,
		DRS_DEPTHBUFFERWRITE = 0x0030 | DRS_Write,

		DRS_PREDICATE		 = 0x0015, // Implied Read
		DRS_INDIRECTARGS	 = 0x0019, // Implied Read
		DRS_UNKNOWN			 = 0x0020, 
		DRS_ERROR			 = 0xFFFF 
	};


	/************************************************************************************************/


	inline D3D12_RESOURCE_STATES DRS2D3DState(DeviceResourceState State)
	{
		switch (State)
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
		case DeviceResourceState::DRS_Free:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
		default:
			FK_ASSERT(0);
		}
		return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
	}


	/************************************************************************************************/


	FLEXKITAPI class DesciptorHeap
	{
	public:
		DesciptorHeap() {}
		DesciptorHeap(RenderSystem* RS, const DesciptorHeapLayout<16>& Layout_IN, iAllocator* TempMemory);

		void Init		(RenderSystem* RS, const DesciptorHeapLayout<16>& Layout_IN, iAllocator* TempMemory);
		void NullFill	(RenderSystem* RS);

		bool SetSRV(RenderSystem* RS, size_t Index, TextureHandle Handle);

		operator D3D12_GPU_DESCRIPTOR_HANDLE () { return DescriptorHeap; }

	private:
		DescHeapPOS						DescriptorHeap;
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


		bool AllowIA	= true;

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
		VertexBufferHandle	VertexBuffer;
		UINT				Stride;
		UINT				Offset = 0;
	};

	typedef static_vector<VertexBufferEntry, 16>	VertexBufferList;
	typedef static_vector<DescHeapPOS, 16>			RenderTargetList;

	FLEXKITAPI class Context
	{
	public:
		Context(
				ID3D12GraphicsCommandList* Context_IN = nullptr, 
				RenderSystem* RS_IN = nullptr, 
				iAllocator* TempMemory = nullptr) :
			CurrentRootSignature	(nullptr),
			DeviceContext			(Context_IN),
			PendingBarriers			(TempMemory),
			RS						(RS_IN),
			Memory					(TempMemory),
			RenderTargetCount		(0),
			DepthStencilEnabled		(false)
			{}
			
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


		void AddRenderTargetBarrier	(TextureHandle Handle, DeviceResourceState Before, DeviceResourceState State = DeviceResourceState::DRS_RenderTarget);
		void AddPresentBarrier		(TextureHandle Handle, DeviceResourceState Before);

		void ClearDepthBuffer		(TextureObject Texture, float ClearDepth = 0.0f); // Assumes full-screen Clear
		void ClearRenderTarget		(TextureObject Texture, float4 ClearColor = float4(0.0f)); // Assumes full-screen Clear

		void SetRootSignature		(RootSignature& RS);
		void SetPipelineState		(ID3D12PipelineState* PSO);

		void SetRenderTargets		(static_vector<Texture2D*,16>	RTs);
		void SetRenderTargets		(const static_vector<DescHeapPOS, 16>	RTs, bool DepthStecil, DescHeapPOS DepthStencil = DescHeapPOS());
		//void SetRenderTargets		(const static_vector<TextureObject>		RTs, bool DepthStecil, DescHeapPOS DepthStencil = DescHeapPOS());

		void SetViewports			(static_vector<D3D12_VIEWPORT, 16>	VPs);
		void SetScissorRects		(static_vector<D3D12_RECT, 16>		Rects);

		void SetScissorAndViewports	(static_vector<TextureHandle, 16>	RenderTargets);

		void SetDepthStencil		(Texture2D* DS);
		void SetPrimitiveTopology	(EInputTopology Topology);

		void SetGraphicsConstantBufferView	(size_t idx, const ConstantBufferHandle CB, size_t Offset = 0);
		void SetGraphicsConstantBufferView	(size_t idx, const ConstantBuffer& CB);
		void SetGraphicsDescriptorTable		(size_t idx, DesciptorHeap& DH);
		void SetGraphicsShaderResourceView	(size_t idx, FrameBufferedResource* Resource, size_t Count, size_t ElementSize);
		void SetGraphicsShaderResourceView	(size_t idx, Texture2D& Texture);


		void AddIndexBuffer			(TriMesh* Mesh);
		void AddVertexBuffers		(TriMesh* Mesh, static_vector<VERTEXBUFFER_TYPE, 16> Buffers, VertexBufferList* InstanceBuffers = nullptr);
		void SetVertexBuffers		(VertexBufferList& List);

		void Draw					(size_t VertexCount, size_t BaseVertex = 0);
		void DrawIndexed			(size_t IndexCount, size_t IndexOffet = 0, size_t BaseVertex = 0);
		void DrawIndexedInstanced	(size_t IndexCount, size_t IndexOffet = 0, size_t BaseVertex = 0, size_t InstanceCount = 1, size_t InstanceOffset = 0);
		void Clear					();

		void FlushBarriers();

		void SetPredicate(bool Enable, QueryBufferHandle Handle = {}, size_t = 0);

		void SetRTRead	(TextureHandle Handle);
		void SetRTWrite	(TextureHandle Handle);
		void SetRTFree	(TextureHandle Handle);

		// Not Yet Implemented
		void SetUAVRead();
		void SetUAVWrite();
		void SetUAVFree();

		ID3D12GraphicsCommandList*	GetCommandList() { return DeviceContext; }

	private:

		struct Barrier
		{
			Barrier() {}
			Barrier(const Barrier& rhs) 
			{
				Type		= rhs.Type;
				Resource	= rhs.Resource;
				NewState	= rhs.NewState;
				OldState	= rhs.OldState;
			}

			DeviceResourceState OldState;
			DeviceResourceState NewState;

			enum BarrierType
			{
				BT_RenderTarget,
				BT_ConstantBuffer,
				BT_UAV,
				BT_VertexBuffer,
			}Type;

			union 
			{
				TextureHandle		RenderTarget;
				ID3D12Resource*		Resource;
			};
		};

		void UpdateResourceStates();
		void UpdateRTVState();

		ID3D12GraphicsCommandList*	DeviceContext;
		RootSignature*				CurrentRootSignature;
		ID3D12PipelineState*		CurrentPipelineState;
		RenderSystem*				RS;

		D3D12_CPU_DESCRIPTOR_HANDLE RTVPOSCPU;
		D3D12_CPU_DESCRIPTOR_HANDLE DSVPOSCPU;

		size_t	RenderTargetCount;
		bool	DepthStencilEnabled;

		static_vector<TextureHandle, 16>		RenderTargets;
		static_vector<D3D12_VIEWPORT, 16>		Viewports;
		static_vector<DesciptorHeap*>			DesciptorHeaps;
		static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;
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

		VertexBufferHandle	CreateVertexBuffer	(size_t BufferSize, bool GPUResident, RenderSystem* RS); // Creates Using Placed Resource
		bool				PushVertex			(VertexBufferHandle Handle, void* _ptr, size_t ElementSize);

		void				LockUntil	(size_t Frame);// Locks all in use Buffers until given Frame
		void				Reset		(VertexBufferHandle Handle);

		ID3D12Resource*		GetResource						(VertexBufferHandle Handle);
		size_t				GetCurrentVertexBufferOffset	(VertexBufferHandle Handle) const;
		size_t				GetBufferSize					(VertexBufferHandle Handle) const;


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

		ID3D12Resource*			GetBufferResource		(ConstantBufferHandle Handle);
		size_t					GetBufferOffset			(ConstantBufferHandle Handle);
		size_t					GetBufferBeginOffset	(ConstantBufferHandle Handle);

		size_t					BeginNewBuffer	(ConstantBufferHandle Handle);
		bool					Push			(ConstantBufferHandle Handle, void* _Ptr, size_t PushSize);
		
		SubBufferHandle			Reserve					(ConstantBufferHandle Handle, size_t Size);
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
			Users{Memory},
			Resources{Memory},
			RS{RS_in}
		{
		}


		~QueryTable()
		{
		}


		QueryBufferHandle	CreateQueryBuffer	(size_t Count, QueryType type);
		void				LockUntil			(size_t FrameID);


		void				SetUsed		(QueryBufferHandle Handle)
		{
			Users[Handle].Used = true;
		}


		ID3D12Resource*		GetResource	(QueryBufferHandle Handle)
		{
			auto& Res = Resources[Users[Handle].ResourceIdx];

			return Res.Resources[Res.CurrentResource];
		}


		struct UserEntry
		{
			size_t ResourceIdx;
			size_t ResourceSize;
			size_t CurrentOffset;
			bool   Used;
		};


		struct ResourceEntry
		{
			ID3D12Resource* Resources[3];
			size_t			ResourceLocks[3];
			size_t			CurrentResource;
			size_t			Padding[1];
		};

		RenderSystem*			RS;
		Vector<UserEntry>		Users;
		Vector<ResourceEntry>	Resources;
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

	FLEXKITAPI class TextureStateTable
	{
	public:
		TextureStateTable(iAllocator* memory) :
			Handles		{memory},
			UserEntries	{memory},
			Resources	{memory}
		{
		}


		~TextureStateTable()
		{
			Release();
		}

		void Release();

		// No Copy
		TextureStateTable				(const TextureStateTable&) = delete;
		TextureStateTable& operator =	(const TextureStateTable&) = delete;

		Texture2D		operator[]		(TextureHandle Handle);

		TextureHandle	AddResource		(Texture2D_Desc& Desc, ID3D12Resource** Resource, uint32_t ResourceCount, DeviceResourceState InitialState, uint32_t Flags_IN = 0);
		void			SetState		(TextureHandle Handle, DeviceResourceState State);

		uint2			GetWH(TextureHandle Handle);
		size_t			GetFrameGraphIndex(TextureHandle Texture, size_t FrameID);
		void			SetFrameGraphIndex(TextureHandle Texture, size_t FrameID, size_t Index);

		uint32_t		GetTag(TextureHandle Handle);
		void			SetTag(TextureHandle Handle, uint32_t Tag);


		void			MarkRTUsed		(TextureHandle Handle);

		DeviceResourceState GetState	(TextureHandle Handle);
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
			uint2				WH;
		};
			
		Vector<UserEntry>								UserEntries;
		Vector<ResourceEntry>							Resources;
		HandleUtilities::HandleTable<TextureHandle, 32>	Handles;
	};


	/************************************************************************************************/


	// Basic Draw States
	ID3D12PipelineState* CreateDrawTriStatePSO(RenderSystem* RS);
	ID3D12PipelineState* CreateDrawLineStatePSO(RenderSystem* RS);
	ID3D12PipelineState* CreateDraw2StatePSO(RenderSystem* RS);


	/************************************************************************************************/


	FLEXKITAPI class RenderSystem
	{
	public:
		RenderSystem(iAllocator* Memory_IN) :
			Memory			(Memory_IN),
			Library			(Memory_IN),
			Queries			(Memory_IN, this),
			RenderTargets	(Memory_IN),
			Textures		(Memory_IN),
			VertexBuffers	(Memory_IN),
			ConstantBuffers	(Memory_IN, this),
			PipelineStates	(Memory_IN, this)
		{
			pDevice                = nullptr;
			pDebug                 = nullptr;
			pDebugDevice           = nullptr;

			pGIFactory		       = nullptr;
			pDXGIAdapter	       = nullptr;
			GraphicsQueue	       = nullptr;
			UploadQueue		       = nullptr;
			ComputeQueue	       = nullptr;

			ID3D12Fence* Fence	   = nullptr;
			ID3D12Fence* CopyFence = nullptr;

			CurrentIndex			= 0;
			CurrentUploadIndex		= 0;
			FenceCounter			= 0;
			FenceUploadCounter		= 0;

			BufferCount             = 0;
			DescriptorRTVSize       = 0;
			DescriptorDSVSize       = 0;
			DescriptorCBVSRVUAVSize = 0;

			PipelineStates.RegisterPSOLoader(EPIPELINESTATES::DRAW_PSO,			CreateDrawTriStatePSO);
			PipelineStates.RegisterPSOLoader(EPIPELINESTATES::DRAW_LINE_PSO,	CreateDrawLineStatePSO);
			PipelineStates.RegisterPSOLoader(EPIPELINESTATES::DRAW_LINE3D_PSO,	CreateDraw2StatePSO);
		}
		
		~RenderSystem()
		{
			Release();
		}

		RenderSystem				(const RenderSystem&) = delete;
		RenderSystem& operator =	(const RenderSystem&) = delete;

		bool	Initiate(Graphics_Desc* desc_in);
		void	Release();

		// Pipeline State
		ID3D12PipelineState*	GetPSO				(EPIPELINESTATES StateID);
		void					RegisterPSOLoader	(EPIPELINESTATES State, LOADSTATE_FN Loader);
		void					QueuePSOLoad		(EPIPELINESTATES State);

		size_t					GetCurrentFrame();

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

		uint2		GetRenderTargetWH(TextureHandle Handle);

		// Resource Creation and Destruction
		ConstantBufferHandle	CreateConstantBuffer	(size_t BufferSize, bool GPUResident = true);
		VertexBufferHandle		CreateVertexBuffer		(size_t BufferSize, bool GPUResident = true);
		TextureHandle			CreateDepthBuffer		(uint2 WH, bool UseFloat = false);
		TextureHandle			CreateTexture2D			(uint2 WH, FORMAT_2D Format, size_t MipLevels = 0);
		TextureHandle			CreateTexture2D			(uint2 WH, FORMAT_2D Format, size_t MipLevels = 0, ID3D12Resource** Resources = nullptr, size_t ResourceCount = 1);
		QueryBufferHandle		CreateOcclusionBuffer	(size_t Size);

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

		void _ResetDescHeap();
		void _ResetRTVHeap();
		void _ResetGPUDescHeap();
		void _ResetDSVHeap();

		ID3D12DescriptorHeap*		_GetCurrentRTVTable();
		ID3D12DescriptorHeap*		_GetCurrentDescriptorTable();
		ID3D12DescriptorHeap*		_GetCurrentDSVTable();
		D3D12_GPU_DESCRIPTOR_HANDLE	_GetDescTableCurrentPosition_GPU();
		D3D12_GPU_DESCRIPTOR_HANDLE	_GetRTVTableCurrentPosition_GPU();
		D3D12_CPU_DESCRIPTOR_HANDLE	_GetRTVTableCurrentPosition_CPU();
		D3D12_CPU_DESCRIPTOR_HANDLE	_GetDSVTableCurrentPosition_CPU();
		D3D12_GPU_DESCRIPTOR_HANDLE	_GetDSVTableCurrentPosition_GPU();
		D3D12_GPU_DESCRIPTOR_HANDLE	_GetGPUDescTableCurrentPosition_GPU();

		ID3D12Resource*				_GetQueryResource(QueryBufferHandle Handle);

		ID3D12CommandAllocator*		_GetCurrentCommandAllocator();
		ID3D12GraphicsCommandList*	_GetCurrentCommandList();
		PerFrameResources*			_GetCurrentFrameResources();
		ID3D12GraphicsCommandList*	_GetCommandList_1();
		PerFrameUploadQueue&		_GetCurrentUploadQueue();

		void						_IncrementRSIndex() { CurrentIndex = (CurrentIndex + 1) % 3; }

		operator RenderSystem* () { return this; }

		ID3D12Device*			pDevice;
		ID3D12CommandQueue*		GraphicsQueue;
		ID3D12CommandQueue*		UploadQueue;
		ID3D12CommandQueue*		ComputeQueue;

		size_t CurrentFrame;
 		size_t CurrentIndex;
		size_t CurrentUploadIndex;
		size_t FenceCounter;
		size_t FenceUploadCounter;

		ID3D12Fence* Fence;
		ID3D12Fence* CopyFence;

		PerFrameResources		FrameResources	[QueueSize];
		PerFrameUploadQueue		UploadQueues	[QueueSize];

		IDXGIFactory5*			pGIFactory;
		IDXGIAdapter4*			pDXGIAdapter;

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

		size_t BufferCount;
		size_t DescriptorRTVSize;
		size_t DescriptorDSVSize;
		size_t DescriptorCBVSRVUAVSize;

		struct
		{
			size_t AASamples;
			size_t AAQuality;
		}Settings;

		struct
		{
			ID3D12Resource* TempBuffer;
			size_t			Position;
			size_t			Last;
			size_t			Size;
			char*			Buffer;

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
				RS4CBVs4SRVs		(Memory),
				RS2UAVs4SRVs4CBs	(Memory) {}

			void Initiate(RenderSystem* RS);

			RootSignature RS2UAVs4SRVs4CBs; // 4CBVs On all Stages, 4 SRV On all Stages
			RootSignature RS4CBVs4SRVs;		// 4CBVs On all Stages, 4 SRV On all Stages
			RootSignature RS4CBVs_SO;		// Stream Out Enabled
			RootSignature ShadingRTSig;		// Signature For Compute Based Deferred Shading
			RootSignature RSDefault;		// Default Signature for Rasting
		}Library;

		ConstantBufferTable		ConstantBuffers;
		QueryTable				Queries;

		VertexBufferStateTable	VertexBuffers;
		TextureStateTable		RenderTargets;
		TextureStateTable		Textures;
		PipelineStateTable		PipelineStates;

		struct FreeEntry
		{
			ID3D12Resource* Resource;
			size_t			Counter;
		};

		Vector<FreeEntry>	FreeList;


		ID3D12Debug*			pDebug;
		ID3D12DebugDevice*		pDebugDevice;
		iAllocator*				Memory;
	};


	FLEXKITAPI DescHeapPOS PushRenderTarget			(RenderSystem* RS, Texture2D* Target, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushDepthStencil			(RenderSystem* RS, Texture2D* Target, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushCBToDescHeap			(RenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, size_t BufferSize);
	FLEXKITAPI DescHeapPOS PushSRVToDescHeap		(RenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, size_t ElementCount, size_t Stride, D3D12_BUFFER_SRV_FLAGS Flags = D3D12_BUFFER_SRV_FLAG_NONE);
	FLEXKITAPI DescHeapPOS Push2DSRVToDescHeap		(RenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, D3D12_BUFFER_SRV_FLAGS = D3D12_BUFFER_SRV_FLAG_NONE);
	FLEXKITAPI DescHeapPOS PushTextureToDescHeap	(RenderSystem* RS, Texture2D tex, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushUAV2DToDescHeap		(RenderSystem* RS, Texture2D tex, DescHeapPOS POS, DXGI_FORMAT F = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);


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
		Shader() {
			Blob = nullptr;
		}

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

		ID3DBlob*			Blob;
		SHADER_TYPE			Type;
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


	struct SkinDeformer
	{
		const char*	BoneNames;
		size_t		BoneCount;
	};

	union BoundingVolume
	{
		BoundingVolume() {}

		AABB			OBB;
		BoundingSphere	BS;
	};
	

	struct Vertex 
	{
		void Clear() { xyz ={0.0f, 0.0f, 0.0f}; }
	
		void AddWithWeight(Vertex const &src, float weight) 
		{
			xyz.x += weight * src.xyz.x;
			xyz.y += weight * src.xyz.y;
			xyz.z += weight * src.xyz.z;
		}

		float3 xyz;
	};


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

		enum AnimationData
		{
			EAD_None	= 0,
			EAD_Skin	= 1,
			EAD_Vertex	= 2
		};

		enum Occlusion_Volume
		{
			EOV_ORIENTEDBOUNDINGBOX,
			EOV_ORIENTEDSPHERE,
		};

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


	typedef FlexKit::Handle_t<16> TriMeshHandle;
	const TriMeshHandle INVALIDMESHHANDLE = TriMeshHandle(-1);

	struct 
	{
		HandleUtilities::HandleTable<TriMeshHandle>		Handles;
		Vector<TriMesh>									Geometry;
		Vector<size_t>									ReferenceCounts;
		Vector<GUID_t>									Guids;
		Vector<const char*>								GeometryIDs;
		Vector<size_t>									FreeList;
		iAllocator*										Memory;
	}GeometryTable;


	/************************************************************************************************/

	FLEXKITAPI void							InitiateGeometryTable	(iAllocator* memory = nullptr);
	FLEXKITAPI void							ReleaseGeometryTable	();

	FLEXKITAPI void							AddRef					( TriMeshHandle  TMHandle );
	FLEXKITAPI void							ReleaseMesh				( RenderSystem* RS, TriMeshHandle  TMHandle );

	FLEXKITAPI TriMesh*						GetMesh					( TriMeshHandle  TMHandle );
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
		size_t IndexBuffer;
		size_t BufferCount;

		VertexBufferView** Buffers;
	};

	FLEXKITAPI TriMeshHandle	BuildMesh	(RenderSystem* RS, Mesh_Description* Desc, TriMeshHandle guid);


	/************************************************************************************************/


	inline ID3D12Resource* GetBuffer(TriMesh* Mesh, size_t Buffer)	{
		return Mesh->VertexBuffer[Buffer];
	}


	inline ID3D12Resource* FindBuffer(TriMesh* Mesh, VERTEXBUFFER_TYPE Type) {
		ID3D12Resource* Buffer = nullptr;
		auto& VertexBuffers = Mesh->VertexBuffer.VertexBuffers;
		auto RES = find(VertexBuffers, [Type](auto& V) -> bool {return V.Type == Type;});

		if (RES != VertexBuffers.end())
			Buffer = RES->Buffer;

		return Buffer;
	}


	inline VertexBuffer::BuffEntry* FindBufferEntry(TriMesh* Mesh, VERTEXBUFFER_TYPE Type) {
		ID3D12Resource* Buffer = nullptr;
		auto& VertexBuffers = Mesh->VertexBuffer.VertexBuffers;
		auto RES = find(VertexBuffers, [Type](auto& V) -> bool {return V.Type == Type;});

		if (RES != VertexBuffers.end())
			Buffer = RES->Buffer;

		return (VertexBuffer::BuffEntry*)RES; // Remove Const of result
	}


	inline bool AddVertexBuffer(VERTEXBUFFER_TYPE Type, TriMesh* Mesh, static_vector<D3D12_VERTEX_BUFFER_VIEW>& out) {
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


	FLEXKITAPI struct OcclusionCuller
	{
		/*
		OcclusionCuller(OcclusionCuller& rhs) : 
			Head(rhs.Head), 
			Max(rhs.Max), 
			Heap(rhs.Heap),
			Predicates(rhs.Predicates) {}
		*/

		size_t					Head;
		size_t					Max;
		size_t					Idx;

		ID3D12QueryHeap*		Heap[3];
		FrameBufferedResource	Predicates;		
		DepthBuffer				OcclusionBuffer;
		uint2					HW;

		ID3D12PipelineState*	PSO;

		ID3D12Resource* Get();
		size_t			GetNext();

		void Clear();
		void Increment();
		void Release();

	};

	/************************************************************************************************/


	//FLEXKITAPI OcclusionCuller	CreateOcclusionCuller	( RenderSystem* RS, size_t Count, uint2 OcclusionBufferSize, bool UseFloat = true );
	//FLEXKITAPI void				OcclusionPass			( RenderSystem* RS, PVS* Set, OcclusionCuller* OC, ID3D12GraphicsCommandList* CL, GeometryTable* GT, Camera* C );


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

	FLEXKITAPI size_t GetVidMemUsage	( RenderSystem* RS);

	FLEXKITAPI void	Release						( RenderSystem* System );
	FLEXKITAPI void Push_DelayedRelease			( RenderSystem* RS, ID3D12Resource* Res);
	FLEXKITAPI void Free_DelayedReleaseResources( RenderSystem* RS);


	FLEXKITAPI void AddTempBuffer	( ID3D12Resource* _ptr, RenderSystem* RS );
	FLEXKITAPI void AddTempShaderRes( ShaderResourceBuffer& ShaderResource, RenderSystem* RS );


	inline DescHeapPOS IncrementHeapPOS(DescHeapPOS POS, size_t Size, size_t INC) {
		const size_t Offset = Size * INC;
		return{ POS.V1.ptr + Offset , POS.V2.ptr + Offset };
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
		size_t* SubResourceSizes;
	};

	FLEXKITAPI void UpdateResourceByTemp			( RenderSystem* RS, ID3D12Resource* Dest, void* Data, size_t SourceSize, size_t ByteSize = 1, D3D12_RESOURCE_STATES EndState = D3D12_RESOURCE_STATE_COMMON);
	FLEXKITAPI void UpdateResourceByTemp			( RenderSystem* RS, FrameBufferedResource* Dest, void* Data, size_t SourceSize, size_t ByteSize = 1, D3D12_RESOURCE_STATES EndState = D3D12_RESOURCE_STATE_COMMON);
	FLEXKITAPI void UpdateSubResourceByUploadQueue	( RenderSystem* RS, ID3D12Resource* Dest, SubResourceUpload_Desc* Desc, D3D12_RESOURCE_STATES EndState);



	/************************************************************************************************/


	template<typename Ty>
	void CreateBufferView(size_t size, Ty* memory, VertexBufferView*& View, VERTEXBUFFER_TYPE T, VERTEXBUFFER_FORMAT F)
	{
		size_t VertexBufferSize = size * (size_t)F + sizeof(VertexBufferView);
		View = FlexKit::CreateVertexBufferView((byte*)memory->_aligned_malloc(VertexBufferSize), VertexBufferSize);
		View->Begin(T, F);
	}


	/************************************************************************************************/


	template<typename Ty_Container, typename FetchFN, typename TranslateFN>
	bool FillBufferView(Ty_Container* Container, size_t vertexCount, VertexBufferView* out, TranslateFN Translate, FetchFN Fetch)
	{
		for (size_t itr = 0; itr < vertexCount; ++itr) {
			auto temp = Translate(Fetch(itr, Container));
			out->Push(temp);
		}

		return out->End();
	}


	/************************************************************************************************/


	template<typename Ty_Container, typename FetchFN, typename TranslateFN, typename ProcessFN>
	void ProcessBufferView(Ty_Container* Container, size_t vertexCount, TranslateFN Translate, FetchFN Fetch, ProcessFN Process)
	{
		for (size_t itr = 0; itr < vertexCount; ++itr) {
			auto Vert = Translate(Fetch(itr, Container));
			Process(Vert);
		}
	}


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


	typedef FlexKit::Handle_t<16>	LightHandle;

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


	struct CubeDesc
	{
		float r;

		FlexKit::Shader VertexShader;
	};


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
	

	FLEXKITAPI void CreateCubeMesh		( RenderSystem* RS, TriMesh* r,		StackAllocator* mem, CubeDesc& desc );
	FLEXKITAPI void CreatePlaneMesh		( RenderSystem* RS, TriMesh* out,	StackAllocator* mem, PlaneDesc desc );

	FLEXKITAPI bool LoadObjMesh			( RenderSystem* RS, char* File_Loc,	Obj_Desc IN desc, TriMesh ROUT out, StackAllocator RINOUT LevelSpace, StackAllocator RINOUT TempSpace, bool DiscardBuffers );


	/************************************************************************************************/


	FLEXKITAPI void ReleaseTriMesh			( TriMesh*	p );
	FLEXKITAPI void DelayedReleaseTriMesh	( RenderSystem* RS, TriMesh* T );


	/************************************************************************************************/


	inline void ClearTriMeshVBVs(TriMesh* Mesh) { for (auto& buffer : Mesh->Buffers) buffer = nullptr; }


}	/************************************************************************************************/

#endif