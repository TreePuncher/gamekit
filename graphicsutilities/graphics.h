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

// TODOs
// Draw Wireframe Primitive Helper Functions
// Light Sorting for Tiled Rendering
// Shadowing

#ifdef _WIN32
#pragma once
#endif

#ifndef CGRAPHICSMANAGER_H_INCLUDED
#define CGRAPHICSMANAGER_H_INCLUDED

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
#include "..\coreutilities\mathutils.h"
#include "..\coreutilities\Transforms.h"
#include "..\coreutilities\type.h"

#include <iso646.h>
#include <algorithm>
//#include <opensubdiv/far/topologyDescriptor.h>
//#include <opensubdiv/far/primvarRefiner.h>
#include <string>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3d12shader.h>
#include <DirectXMath.h>
#include <dxgi1_5.h>

//#pragma comment(lib, "osdCPU.lib" )
//#pragma comment(lib, "osdGPU.lib" )


namespace FlexKit
{
	// Forward Declarations
	struct RenderTargetDesc;
	struct RenderWindowDesc;
	struct RenderViewDesc;
	struct StackAllocator;

	class cAny;
	class iCamera;
	class Hash;
	class InputInterface;
	class iCurve;
	class iObject;
	class PhObject;
	class PluginManager;
	class iRenderTarget;
	class iRenderWindow;
	class iSceneNode;

	struct RenderTargetDesc;
	struct RenderWindowDesc;
	struct RenderViewDesc;

	using DirectX::XMMATRIX;

	/************************************************************************************************/

#pragma warning(disable:4067) 

	FLEXKITAPI void SetDebugName(ID3D12Object* Obj, const char* cstr, size_t size);

#ifdef USING(DEBUGGRAPHICS)
#define SETDEBUGNAME(RES, ID) {const char* NAME = ID; FlexKit::SetDebugName(RES, ID, strnlen(ID, 64));}
#else
#define SETDEBUGNAME(RES, ID) 
#endif

#define SAFERELEASE(RES) if(RES) { RES->Release(); RES = nullptr; }

#define CALCULATECONSTANTBUFFERSIZE(TYPE) (sizeof(TYPE)/1024 + 1024)

	/************************************************************************************************/


	enum KEYCODES
	{
		KC_A = 'A',
		KC_B = 'B',
		KC_C = 'C',
		KC_D = 'D',
		KC_E = 'E',
		KC_F = 'F',
		KC_G = 'G',
		KC_H = 'H',
		KC_I = 'I',
		KC_J = 'J',
		KC_K = 'K',
		KC_L = 'L',
		KC_M = 'M',
		KC_N = 'N',
		KC_O = 'O',
		KC_P = 'P',
		KC_Q = 'Q',
		KC_R = 'R',
		KC_S = 'S',
		KC_T = 'T',
		KC_U = 'U',
		KC_V = 'V',
		KC_W = 'W',
		KC_X = 'X',
		KC_Y = 'Y',
		KC_Z = 'Z',

		KC_0,
		KC_1,
		KC_2,
		KC_3,
		KC_4,
		KC_5,
		KC_6,
		KC_7,
		KC_8,
		KC_9,

		KC_OTHER,

		KC_INPUTCHARACTERCOUNT = 36,

		KC_ENTER,
		KC_ESC,
		KC_LEFTSHIFT,
		KC_LEFTCTRL,
		KC_SPACE,
		KC_BACKSPACE,
		KC_TAB,
		KC_RIGHTSHIFT,
		KC_RIGHTCTRL,

		KC_COMMA,
		KC_ARROWUP,
		KC_ARROWDOWN,
		KC_ARROWLEFT,
		KC_ARROWRIGHT,

		KC_PLUS,
		KC_MINUS,

		KC_UNDERSCORE,
		KC_EQUAL,

		KC_SYMBOL,

		KC_SYMBOLS_BEGIN,

		KC_HASH,
		KC_DOLLER,

		KC_PERCENT,
		KC_CHEVRON,

		KC_EXCLAMATION,
		KC_AT,
		KC_AMPERSAND,
		KC_STAR,

		KC_LEFTPAREN,
		KC_RIGHTPAREN,

		KC_SYMBOLS_END,

		KC_DELETE,
		KC_HOME,
		KC_END,
		KC_PAGEUP,
		KC_PAGEDOWN,

		KC_F1,
		KC_F2,
		KC_F3,
		KC_F4,
		KC_F5,
		KC_F6,
		KC_F7,
		KC_F8,
		KC_F9,
		KC_F10,
		KC_F11,
		KC_F12,

		KC_NUM1,
		KC_NUM2,
		KC_NUM3,
		KC_NUM4,
		KC_NUM5,
		KC_NUM6,
		KC_NUM7,
		KC_NUM8,
		KC_NUM9,
		KC_NUM0,

		KC_MOUSELEFT,
		KC_MOUSERIGHT,
		KC_MOUSEMIDDLE,

		KC_ERROR,

		KC_TILDA = '~'
	};


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
		VERTEXBUFFER_FORMAT_UNKNOWN			= -1,
		VERTEXBUFFER_FORMAT_R8				= 1,
		VERTEXBUFFER_FORMAT_R8G8B8			= 3,
		VERTEXBUFFER_FORMAT_R8G8B8A8		= 8,
		VERTEXBUFFER_FORMAT_R16				= 2,
		VERTEXBUFFER_FORMAT_R16G16			= 4,
		VERTEXBUFFER_FORMAT_R16G16B16		= 6,
		VERTEXBUFFER_FORMAT_R16G16B16A16	= 8,
		VERTEXBUFFER_FORMAT_R32				= 4,
		VERTEXBUFFER_FORMAT_R32G32			= 8,
		VERTEXBUFFER_FORMAT_R32G32B32		= 12,
		VERTEXBUFFER_FORMAT_R32G32B32A32	= 16,
		VERTEXBUFFER_FORMAT_MATRIX			= 64,
		VERTEXBUFFER_FORMAT_COMBINED		= 32
	};


	/************************************************************************************************/


	enum PIPELINE_DESTINATION : unsigned char
	{
		PIPELINE_DEST_NONE = 0x00,// All Nodes are Initialised to NONE
		PIPELINE_DEST_IA   = 0x01,
		PIPELINE_DEST_HS   = 0x02,
		PIPELINE_DEST_GS   = 0x04,
		PIPELINE_DEST_VS   = 0x08,
		PIPELINE_DEST_PS   = 0x10,
		PIPELINE_DEST_CS   = 0x20,
		PIPELINE_DEST_OM   = 0x30,
		PIPELINE_DEST_DS   = 0x40,
		PIPELINE_DEST_ALL  = 0xFF
	};


	/************************************************************************************************/


	enum class FORMAT_2D
	{
		R16G16_UINT,
		R8G8B8A_UINT,
		R8G8B8A8_UINT,
		R8G8B8A8_UNORM,
		R8G8_UNORM,
		D24_UNORM_S8_UINT,
		R32_FLOAT,
		D32_FLOAT,
		R16G16B16A16_FLOAT,
		R32G32_FLOAT,
		R32G32B32_FLOAT,
		R32G32B32A32_FLOAT
	};
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
			Height		 = 0;
			Width		 = 0;
			Write        = true;
			Read         = true;
			RenderTarget = false;
			UAV          = false;
			MipLevels	 = 0;
			CV           = true;
			initialData	 = nullptr;

			Format	= FORMAT_2D::D24_UNORM_S8_UINT;
			FLAGS	= SPECIALFLAGS::NONE;
		}

		Texture2D_Desc(
			size_t			in_height,
			size_t			in_width,
			FORMAT_2D		in_format,
			CPUACCESSMODE	in_Mode,
			SPECIALFLAGS	in_FLAGS ) : Texture2D_Desc()
		{
			Height	     = in_height;
			Width	     = in_width;
			Format	     = in_format;
			FLAGS	     = in_FLAGS;
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
		FrameBufferedObject(){ BufferCount = MaxBufferedSize; Idx = 0; for(auto& r : Resources) r = nullptr;}
		

		FrameBufferedObject(const std::initializer_list<TY_*>& IL) : FrameBufferedObject() {
			auto IL_I = IL.begin();
			for (size_t I = 0; I < MaxBufferedSize && IL_I != IL.end(); ++I, IL_I++)
				Resources[I] = *IL_I;
		}

		size_t Idx;
		size_t BufferCount;
		TY_*   Resources[MaxBufferedSize];
		size_t _Pad;

		size_t	operator ++()					{ IncrementCounter(); return (Idx); }			// Post Increment
		size_t	operator ++(int)				{ size_t T = Idx; IncrementCounter(); return T; }// Pre Increment
		
		TY_*& operator [] (size_t Index){	return Resources[Index]; }

		//operator ID3D12Resource*()				{ return Get(); }
		TY_* operator -> ()			{ return Get(); }
		TY_* operator -> () const	{ return Get(); }
		operator bool()				{ return (Resources[0]!= nullptr); }
		size_t	size()				{ return BufferCount; }
		TY_*	Get()				{ return Resources[Idx]; }
		TY_*	Get() const			{ return Resources[Idx]; }

		void IncrementCounter() { Idx = (Idx + 1) % BufferCount; };
		void Release()
		{
			for( auto& r : Resources) 
			{
				if(r) 
					r->Release(); 
				r = nullptr;
			};
		}

		void Release_Delayed(RenderSystem* RS) 
		{ 
			for (auto& r : Resources) 
				if(r) Push_DelayedRelease(RS, r); 
		}


		void _SetDebugName(const char* _str) {
			size_t Str_len = strnlen_s(_str, 64);
			for (auto& r : Resources) {
				SetDebugName(r, _str, Str_len);
			}
		}
	};

	typedef FrameBufferedObject<ID3D12Resource>	 FrameBufferedResource;
	typedef FrameBufferedResource				 IndexBuffer;
	typedef FrameBufferedResource				 ConstantBuffer;
	typedef FrameBufferedResource				 ShaderResourceBuffer;
	typedef FrameBufferedResource				 StreamOutBuffer;
	typedef	FrameBufferedObject<ID3D12QueryHeap> QueryResource;

	typedef ID3D12Resource*						VertexResourceBuffer;

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

		VertexBufferView  operator + ( const VertexBufferView& RHS );

		VertexBufferView& operator = ( const VertexBufferView& RHS );	// Assignment Operator
		//VertexBuffer& operator = ( const VertexBuffer&& RHS );	// Movement Operator


		template<typename Ty>
		class Typed_Iteration
		{
			typedef Typed_Iteration<Ty> This_Type;
		public:
			Typed_Iteration( byte* _ptr, size_t Size ) : m_position( (Ty*)( _ptr ) ), m_size( Size ) {}

			class Typed_iterator
			{
			public:
				Typed_iterator( Ty* _ptr ) : m_position(_ptr){}
				Typed_iterator( const Typed_iterator& rhs ) : m_position( rhs.m_position ) {}

				inline void operator ++ ( )		{ m_position++;}
				inline void operator ++ ( int ) { m_position++;}
				inline void operator -- ( )		{ m_position--;}
				inline void operator -- ( int ) { m_position--;}

				inline Typed_iterator	operator =	( const Typed_iterator& rhs ) {}
				
				inline bool operator <	(const Typed_iterator& rhs) { return (this->m_position < rhs.m_position); }


				inline bool				operator == ( const Typed_iterator& rhs ) {	return  ( m_position == &*rhs ); }
				inline bool				operator != ( const Typed_iterator& rhs ) { return !( m_position == &*rhs ); }

				inline Ty&				operator * ()		  { return *m_position; }
				inline Ty				operator * ()	const { return *m_position; }

				inline const Ty&		peek_forward()	const { return *( m_position + sizeof( Ty ) ); }

			private:

				Ty*	m_position;
			};

			Ty&	operator [] ( size_t index )	{ return m_position[index]; }
			Typed_iterator begin()				{ return Typed_iterator( m_position ); }
			Typed_iterator end()				{ return Typed_iterator( m_position + m_size); }

			inline const size_t	size() { return m_size / sizeof( Ty ); }

		private:
			Ty*			m_position;
			size_t		m_size;
		};


		/************************************************************************************************/


		template< typename TY >
		inline bool Push( TY& in )
		{
			if( mBufferUsed + sizeof(TY) > mBufferSize )
				return false;

			auto size = sizeof( TY );
			if( sizeof( TY ) != static_cast<uint32_t>( mBufferFormat ) )
				mBufferinError = true;
			else if( !mBufferinError )
			{
				char* Val = (char*)&in;
				for( size_t itr = 0; itr < static_cast<uint32_t>( mBufferFormat ); itr++ )
					mBuffer[mBufferUsed++] = Val[itr];
			}
			return !mBufferinError;
		}


		/************************************************************************************************/


		template< typename TY >
		inline bool Push( TY& in, size_t bytesize )
		{
			if( mBufferUsed + bytesize > mBufferSize )
				return false;

			char* Val = (char*)&in;
			for( size_t itr = 0; itr < bytesize; itr++ )
				mBuffer.push_back( Val[itr] );

			return !mBufferinError;
		}


		/************************************************************************************************/


		template<>
		inline bool Push( float3& in )
		{
			if( mBufferUsed + static_cast<uint32_t>(mBufferFormat) > mBufferSize )
				return false;

			char* Val = (char*)&in;
			for( size_t itr = 0; itr < static_cast<uint32_t>( mBufferFormat ); itr++ )
				mBuffer[mBufferUsed++] = Val[itr];
			return !mBufferinError;
		}


		/************************************************************************************************/


		template<typename TY>
		inline bool Push( TY* in )
		{
			if( mBufferUsed + sizeof(TY) > mBufferSize )
				return false;

			if( !mBufferinError )
			{
				char Val[128]
					memcpy( Val, &in, mBufferFormat );
				for( size_t itr = 0; itr < static_cast<uint32_t>( mBufferFormat ); itr++ )
					mBuffer.push_back( Val[itr] );
			}
			return !mBufferinError;
		}


		/************************************************************************************************/


		template<typename Ty>
		inline Typed_Iteration<Ty> CreateTypedProxy()
		{
			return Typed_Iteration<Ty>( GetBuffer(), GetBufferSizeUsed() );
		}


		template<typename Ty>
		inline bool Push( vector_t<Ty> static_vector )
		{
			if( mBufferUsed + sizeof(Ty)*static_vector.size()> mBufferSize )
				return false;

			for( auto in : static_vector )
				if( !Push( in ) ) return false;

			return true;
		}


		template<typename Ty, size_t count>
		inline bool Push( static_vector<Ty, count>& static_vector )
		{
			if( mBufferUsed + sizeof(Ty)*static_vector.size()> mBufferSize )
				return false;

			for( auto in : static_vector )
				if( !Push( in ) ) return false;

			return true;
		}


		void Begin( VERTEXBUFFER_TYPE, VERTEXBUFFER_FORMAT );
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

		void SetTypeFormatSize( VERTEXBUFFER_TYPE, VERTEXBUFFER_FORMAT, size_t count );

	private:
		bool _combine( const VertexBufferView& LHS, const VertexBufferView& RHS, char* out );

		void				SetElementSize( size_t ) {}
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

	struct RenderWindow;


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


	struct Buffer;
	struct ShaderResource;
	struct Shader;
	struct Texture2D;
	struct RenderSystem;


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
		HWND						hWindow;
		IDXGISwapChain3*			SwapChain_ptr;
		ID3D12Resource*				BackBuffer[4];
		UINT						BufferIndex;
		DXGI_FORMAT					Format;

		uint2						WH; // Width-Height
		uint2						WindowCenterPosition;
		Viewport					VP;
		bool						Close;
		bool						Fullscreen;
		
		EventNotifier<>				Handler;

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
		Texture2D					Buffer	[MaxBufferedSize];
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
		EIT_TRIANGLELIST,
		EIT_TRIANGLE,
		EIT_POINT,
	};


	/************************************************************************************************/

		
	typedef	fixed_vector<ID3D12Resource*> TempResourceList;
	const static int QueueSize		= 3;
	const static int MaxThreadCount = 8;

	struct DescHeapStack
	{
		D3D12_CPU_DESCRIPTOR_HANDLE CPU_HeapPOS;
		D3D12_GPU_DESCRIPTOR_HANDLE GPU_HeapPOS;
		ID3D12DescriptorHeap*		DescHeap;
	};

	struct PerFrameUploadQueue
	{
		size_t UploadCount;
		ID3D12CommandAllocator*		UploadCLAllocator	[MaxThreadCount];
		ID3D12GraphicsCommandList*	UploadList			[MaxThreadCount];
	};

	struct PerFrameResources
	{
		TempResourceList*			TempBuffers;
		ID3D12GraphicsCommandList*	CommandLists		[MaxThreadCount];
		bool						CommandListsUsed	[MaxThreadCount];
		ID3D12CommandAllocator*		GraphicsCLAllocator	[MaxThreadCount];
		ID3D12CommandAllocator*		ComputeCLAllocator	[MaxThreadCount];
		ID3D12GraphicsCommandList*	ComputeList			[MaxThreadCount];

		DescHeapStack RTVHeap;
		DescHeapStack DSVHeap;
		DescHeapStack DescHeap;
		DescHeapStack GPUDescHeap;

		size_t						ThreadsIssued;
	};

	typedef Handle_t<32>	ConstantBufferHandle;
	struct ConstantBufferTable
	{
		ConstantBufferTable() 
		{
		}

		struct ConstantBufferUsage
		{
			size_t BufferSize;

			struct MemoryRange
			{
				size_t					Begin;
				size_t					End;
				size_t					BlockCount;
				size_t					BlockSize;
				Vector<MemoryRange*>	ChildRanges;
			}Top;
		};

		Vector<ID3D12Resource*>		ConstantBufferPools;
		Vector<ConstantBufferUsage>	ConstantBufferDesciptors;

		HandleUtilities::HandleTable<ConstantBufferHandle>	Handles;
	};

	template<size_t ENTRYCOUNT = 16>
	class DesciptorHeap
	{
		DesciptorHeap(DescHeapStack* Stack)
		{
		}
		struct Descriptor
		{
			enum class Entry_TYPE
			{
				ConstantBuffer,
				StructuredBuffer,
				TextureBuffer,
				UAVBuffer,
			}Type;

			union
			{
				ConstantBufferHandle		ConstantBuffer;
				ID3D12Resource*				Resource_ptr;
				D3D12_CPU_DESCRIPTOR_HANDLE CPU_HeapPOS;
				D3D12_GPU_DESCRIPTOR_HANDLE GPU_HeapPOS;
			};
		};
		static_vector<Descriptor, ENTRYCOUNT> Entries;
	};


	class RootSignature
	{
	};


	FLEXKITAPI struct RenderSystem
	{
		bool Initiate(Graphics_Desc* desc_in);
		void Release();

		operator RenderSystem* () { return this; }

		iAllocator*				Memory;

		ID3D12Device*			pDevice;
		ID3D12Debug*			pDebug;
		ID3D12DebugDevice*		pDebugDevice;

		PerFrameResources		FrameResources[QueueSize];
		PerFrameUploadQueue		UploadQueues[QueueSize];

		IDXGIFactory4*			pGIFactory;
		IDXGIAdapter3*			pDXGIAdapter;
		ID3D12CommandQueue*		GraphicsQueue;
		ID3D12CommandQueue*		UploadQueue;
		ID3D12CommandQueue*		ComputeQueue;

		ConstantBuffer			NullConstantBuffer; // Zero Filled Constant Buffer
		Texture2D				NullUAV; // 1x1 Zero UAV
		Texture2D				NullSRV;
		ShaderResourceBuffer	NullSRV1D;

		size_t CurrentIndex;
		size_t CurrentUploadIndex;
		size_t FenceCounter;
		size_t FenceUploadCounter;

		ID3D12Fence* Fence;
		ID3D12Fence* CopyFence;

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
		}CopyEngine;

		struct RootSigLibrary
		{
			ID3D12RootSignature* RS2UAVs4SRVs4CBs;  // 4CBVs On all Stages, 4 SRV On all Stages
			ID3D12RootSignature* RS4CBVs4SRVs;		// 4CBVs On all Stages, 4 SRV On all Stages
			ID3D12RootSignature* RS4CBVs_SO;		// Stream Out Enabled
			ID3D12RootSignature* ShadingRTSig;		// Signature For Compute Based Deferred Shading
		}Library;

		PipelineStateTable* States;

		struct FreeEntry
		{
			ID3D12Resource* Resource;
			size_t			Counter;
		};
		Vector<FreeEntry> FreeList;

		ConstantBufferTable ConstantBuffers;
	};


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

		void clear()
		{
			VertexBuffers.clear();
		}

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


	typedef fixed_vector<PointLight>		PLList;
	typedef fixed_vector<PointLightEntry>	PLBuffer;
	typedef fixed_vector<SpotLight>			SLList;
	typedef fixed_vector<SpotLightEntry>	SLBuffer;
	typedef fixed_vector<char>				LightFlags;
	typedef fixed_vector<char*>				LightIDs;


	/************************************************************************************************/


	struct PointLightBufferDesc
	{
		size_t	MaxLightCount;
	};

	struct PointLightBuffer
	{
		ID3D12Resource*				Resource;
		PLList*						Lights;
		LightFlags*					Flags;
		LightIDs*					IDs;

		PointLight&	operator []( size_t index )	{
			FK_ASSERT (!Lights->full());
			return Lights->at(index);
		}

		void push_back( PointLight PL )	{
			FK_ASSERT (!Lights->full());
			Lights->push_back(PL);
		}

		void Swap( size_t i1, size_t i2 ) {
			auto Temp = Lights->at(i1);
			Lights->at(i1) = Lights->at(i2);
			Lights->at(i2) = Temp;
		}

		size_t	size()	const {return Lights->size();}
		size_t	max()	const {return Lights->max_length();}
		void Release()	{Resource->Release();}
	};


	/************************************************************************************************/


	struct SpotLightBuffer
	{
		ID3D12Resource*				Resource;
		SLList*						Lights;
		LightFlags*					Flags;
		LightIDs*					IDs;

		SpotLight&	operator []( size_t index )
		{
			FK_ASSERT (index < Lights->size());
			return Lights->at(index);
		}

		void push_back( SpotLight PL )
		{
			FK_ASSERT (!Lights->full());
			Lights->push_back(PL);
		}

		void Swap( size_t i1, size_t i2 )
		{
			auto Temp	 = Lights->at(i1);
			Lights->at(i1) = Lights->at(i2);
			Lights->at(i2) = Temp;
		}

		size_t size() const {return Lights->size();}

		void Release(){Resource->Release();}
	};


	/************************************************************************************************/


	struct Camera
	{
		ConstantBuffer	Buffer;
		NodeHandle		Node;

		float FOV;
		float AspectRatio;
		float Near;
		float Far;
		bool  invert;
		float fStop;	// Future
		float ISO;		// Future

		float4x4 View;
		float4x4 Proj;
		float4x4 WT;	// World Transform
		float4x4 PV;	// Projection x View
		float4x4 IV;	// Inverse Transform

		struct __declspec(align(16)) BufferLayout
		{
			XMMATRIX	View;
			XMMATRIX	ViewI;
			XMMATRIX	Proj;
			XMMATRIX	PV;			//  Projection x View
			XMMATRIX	PVI;		// (Projection x View)^-1
			float4		WPOS;
			float		MinZ;
			float		MaxZ;
			float		PointLightCount;
			float		SpotLightCount;
			float		WindowWidth;
			float		WindowHeight;

			float Padding[2];

			float3  WSTopLeft;
			float3  WSTopRight;
			float3  WSBottomLeft;
			float3  WSBottomRight;

			float3  WSTopLeft_Near;
			float3  WSTopRight_Near;
			float3  WSBottomLeft_Near;
			float3  WSBottomRight_Near;

			constexpr static const size_t GetBufferSize()
			{
				//return sizeof(BufferLayout);	
				return 4096;	
			}

		};

	};


	/************************************************************************************************/


	struct SpotLightShadowCaster
	{
		Camera	C;
		size_t	LightHandle;
	};


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
	struct GeometryTable
	{
		operator GeometryTable* () { return this; }

		GeometryTable(iAllocator* memory = nullptr) : 
			FreeList(memory),
			Geometry(memory),
			GeometryIDs(memory),
			Guids(memory),
			Handles(memory, GetTypeGUID(GeometryTable)),
			Memory(memory),
			ReferenceCounts(memory)
		{}

		HandleUtilities::HandleTable<TriMeshHandle>		Handles;
		Vector<TriMesh>									Geometry;
		Vector<size_t>									ReferenceCounts;
		Vector<GUID_t>									Guids;
		Vector<const char*>								GeometryIDs;
		Vector<size_t>									FreeList;
		iAllocator*										Memory;
	};


	/************************************************************************************************/


	FLEXKITAPI void							InitiateGeometryTable	( GeometryTable* GT, iAllocator* Memory );
	FLEXKITAPI void							ReleaseGeometryTable	( GeometryTable* GT );

	FLEXKITAPI void							AddRef					( GeometryTable* GT, TriMeshHandle  TMHandle );
	FLEXKITAPI void							ReleaseMesh				( RenderSystem* RS, GeometryTable* GT, TriMeshHandle  TMHandle );

	FLEXKITAPI TriMesh*						GetMesh					( GeometryTable* GT, TriMeshHandle  TMHandle );
	FLEXKITAPI Skeleton*					GetSkeleton				( GeometryTable* GT, TriMeshHandle  TMHandle );
	FLEXKITAPI size_t						GetSkeletonGUID			( GeometryTable* GT, TriMeshHandle  TMHandle );
	FLEXKITAPI void							SetSkeleton				( GeometryTable* GT, TriMeshHandle  TMHandle, Skeleton* S );

	FLEXKITAPI Pair<TriMeshHandle, bool>	FindMesh				( GeometryTable* GT, GUID_t			guid );
	FLEXKITAPI Pair<TriMeshHandle, bool>	FindMesh				( GeometryTable* GT, const char*	ID   );
	FLEXKITAPI bool							IsMeshLoaded			( GeometryTable* GT, GUID_t			guid );
	FLEXKITAPI bool							IsSkeletonLoaded		( GeometryTable* GT, TriMeshHandle	guid );
	FLEXKITAPI bool							HasAnimationData		( GeometryTable* GT, TriMeshHandle	guid );



	/************************************************************************************************/


	struct Mesh_Description
	{

		size_t IndexBuffer;
		size_t BufferCount;

		VertexBufferView** Buffers;
	};

	FLEXKITAPI TriMeshHandle	BuildMesh	(RenderSystem* RS, GeometryTable* GT, Mesh_Description* Desc, TriMeshHandle guid);


	FLEXKITAPI FustrumPoints	GetCameraFrustumPoints	(Camera* Camera, float3 XYZ, Quaternion Q);
	FLEXKITAPI MinMax			GetCameraAABS_XZ		(FustrumPoints Points);


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

		Texture2D	Textures[16];
		bool		Loaded[16];
	};


	/************************************************************************************************/


	typedef float2 VTexcord; // virtual Texture Cord

	struct TextureEntry
	{
		size_t		OffsetIndex;
		GUID_t		ResourceID;
	};


	struct Resources;

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

		// 
		Resources*		ResourceTable;
		iAllocator*		Memory;
	};


	/************************************************************************************************/


	struct TextureVTable_Desc
	{
		uint2			PageSize;
		uint2			PageCount;
		Resources*		Resources;
		iAllocator*		Memory; // Needs to be persistent memory
	};


	/************************************************************************************************/


	struct TextureManager
	{
		Vector<TextureSet>	Textures;
		Vector<uint32_t>		FreeList;
	};


	FLEXKITAPI void UploadTextureSet	( RenderSystem* RS, TextureSet* TS, iAllocator* Memory );
	FLEXKITAPI void ReleaseTextureSet	( TextureSet* TS, iAllocator* Memory );


	struct DrawableDesc
	{
		ShaderSetHandle Material;
	};

	struct DrawableAnimationState;
	struct DrawablePoseState;
	 
	struct Drawable
	{
		// TODO: move state flags into a single byte 
		Drawable() 
		{
			DrawLast		   = false;
			Transparent		   = false;
			Posed			   = false; // Use Vertex Palette Skinning
			PoseState		   = nullptr;
			AnimationState	   = nullptr;
			Occluder		   = INVALIDMESHHANDLE;
		}
		
		NodeHandle			Node;						// 2
		TriMeshHandle		Occluder;					// 2

		//TriMesh*				Mesh;			// 8
		TriMeshHandle			MeshHandle;		// 2
		bool					DrawLast;		// 1
		bool					Transparent;	// 1
		bool					Textured;		// 1
		bool					Posed;			// 1
		bool					Dirty;			// 1
		bool					Padding;		// 1

		DrawablePoseState*		PoseState;		// 8 16
		DrawableAnimationState*	AnimationState; // 8 24
		TextureSet*				Textures;		// 8 32

		struct MaterialProperties
		{
			MaterialProperties(float4 a, float4 m) : Albedo(a), Spec(m)
			{
				Albedo.w = min(a.w, 1.0f);
				Spec.w	 = min(m.w, 1.0f);
			}
			MaterialProperties() : Albedo(1.0, 1.0f, 1.0f, 0.5f), Spec(0)
			{}
			float4		Albedo;		// Term 4 is Roughness
			float4		Spec;		// Metal Is first 4, Specular is rgb
		}MatProperties;	// 32 64

		ConstantBuffer			VConstants;

		Drawable&	SetAlbedo(float4 RGBA)		{ MatProperties.Albedo	= RGBA; return *this; }
		Drawable&	SetSpecular(float4 RGBA)	{ MatProperties.Spec	= RGBA; return *this; }
		Drawable&	SetNode(NodeHandle H)		{ Node					= H;	return *this; }

		struct VConsantsLayout
		{
			MaterialProperties	MP;
			DirectX::XMMATRIX	Transform;
		};
	};


	/************************************************************************************************/


	struct SortingField
	{
		unsigned int Posed			: 1;
		unsigned int Textured		: 1;
		unsigned int MaterialID		: 7;
		unsigned int InvertDepth	: 1;
		unsigned long long Depth	: 53;

		operator uint64_t(){return *(uint64_t*)(this + 8);}

	};

	struct PVEntry
	{
		PVEntry() {}
		PVEntry(Drawable* d) : OcclusionID(-1), D(d){}
		PVEntry(Drawable* d, size_t ID, size_t sortID) : OcclusionID(ID), D(d), SortID(sortID) {}

		size_t		SortID;
		size_t		OcclusionID;
		Drawable*	D;

		operator Drawable* () {return D;}
		operator size_t () { return SortID; }

	};

	
	typedef Vector<PVEntry> PVS;

	inline void PushPV(Drawable* e, PVS* pvs)
	{
		if (e && e->MeshHandle.to_uint() != INVALIDHANDLE)
			pvs->push_back(PVEntry( e, 0xffffffffffffffff, 0u));
	}

	FLEXKITAPI void UpdateDrawables		(RenderSystem* RS, SceneNodes* Nodes, PVS* PVS_);
	FLEXKITAPI void SortPVS				(SceneNodes* Nodes, PVS* PVS_, Camera* C);
	FLEXKITAPI void SortPVSTransparent	(SceneNodes* Nodes, PVS* PVS_, Camera* C);


	/************************************************************************************************/


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

	FLEXKITAPI OcclusionCuller	CreateOcclusionCuller	( RenderSystem* RS, size_t Count, uint2 OcclusionBufferSize, bool UseFloat = true );
	FLEXKITAPI void				OcclusionPass			( RenderSystem* RS, PVS* Set, OcclusionCuller* OC, ID3D12GraphicsCommandList* CL, GeometryTable* GT, Camera* C );


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

	// TODO: Implement this
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


	void Upload					(RenderSystem* RS, SceneNodes* Nodes, iAllocator* Temp, Camera* C);
	void BuildGeometryTable		(RenderSystem* RS, iAllocator* TempMemory, StaticMeshBatcher* Batcher);
	void CleanUpStaticBatcher	(StaticMeshBatcher* Batcher);

	
	/************************************************************************************************/

	
	inline float2 PixelToSS(size_t X, size_t Y, uint2 Dimensions) {	return { -1.0f + (float(X)	   / Dimensions[0]), 1.0f - (float(Y)	  / Dimensions[1]) }; } // Assumes screen boundaries are -1 and 1
	inline float2 PixelToSS(uint2 XY, uint2 Dimensions)			  { return { -1.0f + (float(XY[0]) / Dimensions[0]), 1.0f - (float(XY[1]) / Dimensions[1]) }; } // Assumes screen boundaries are -1 and 1

	FLEXKITAPI VertexBuffer::BuffEntry* GetBuffer( VertexBuffer*, VERTEXBUFFER_TYPE ); // return nullptr if not found
	
	FLEXKITAPI bool	InitiateRenderSystem();

	FLEXKITAPI void ReserveTempSpace	( RenderSystem* RS, size_t Size, void*& CPUMem, size_t& Offset );

	FLEXKITAPI size_t GetVidMemUsage	( RenderSystem* RS);

	FLEXKITAPI void	Release			( RenderSystem* System );
	FLEXKITAPI void	ReleaseCamera	( SceneNodes* Nodes, Camera* camera );


	FLEXKITAPI void Push_DelayedRelease			( RenderSystem* RS, ID3D12Resource* Res);
	FLEXKITAPI void Free_DelayedReleaseResources( RenderSystem* RS);


	FLEXKITAPI void AddTempBuffer	( ID3D12Resource* _ptr, RenderSystem* RS );
	FLEXKITAPI void AddTempShaderRes( ShaderResourceBuffer& ShaderResource, RenderSystem* RS );
	FLEXKITAPI void	PresentWindow	( RenderWindow* RW, RenderSystem* RS );
	FLEXKITAPI void	WaitforGPU		( RenderSystem* RS );

	FLEXKITAPI ID3D12DescriptorHeap*		GetCurrentRTVTable				( RenderSystem* RS );
	FLEXKITAPI ID3D12DescriptorHeap*		GetCurrentDescriptorTable		( RenderSystem* RS );
	FLEXKITAPI ID3D12DescriptorHeap*		GetCurrentDSVTable				( RenderSystem* RS );
	FLEXKITAPI D3D12_GPU_DESCRIPTOR_HANDLE  GetDescTableCurrentPosition_GPU	( RenderSystem* RS );
	FLEXKITAPI D3D12_GPU_DESCRIPTOR_HANDLE  GetRTVTableCurrentPosition_GPU	( RenderSystem* RS );
	FLEXKITAPI D3D12_CPU_DESCRIPTOR_HANDLE  GetRTVTableCurrentPosition_CPU	( RenderSystem* RS );
	FLEXKITAPI D3D12_CPU_DESCRIPTOR_HANDLE  GetDSVTableCurrentPosition_CPU	( RenderSystem* RS );
	FLEXKITAPI D3D12_GPU_DESCRIPTOR_HANDLE  GetDSVTableCurrentPosition_GPU	( RenderSystem* RS );

	FLEXKITAPI D3D12_GPU_DESCRIPTOR_HANDLE  GetGPUDescTableCurrentPosition_GPU (RenderSystem* RS);


	FLEXKITAPI void	ResetDescHeap		( RenderSystem* RS );
	FLEXKITAPI void	ResetRTVHeap		( RenderSystem* RS );
	FLEXKITAPI void	ResetGPUDescHeap	( RenderSystem* RS );

	typedef Pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> DescHeapPOS;

	inline DescHeapPOS IncrementHeapPOS(DescHeapPOS POS, size_t Size, size_t INC) {
		const size_t Offset = Size * INC;
		return{ POS.V1.ptr + Offset , POS.V2.ptr + Offset };
	}

	FLEXKITAPI DescHeapPOS ReserveDescHeap			( RenderSystem* RS, size_t SlotCount = 1 );
	FLEXKITAPI DescHeapPOS ReserveGPUDescHeap		( RenderSystem* RS, size_t SlotCount = 1 );

	FLEXKITAPI DescHeapPOS ReserveRTVHeap			( RenderSystem* RS, size_t SlotCount );
	FLEXKITAPI DescHeapPOS ReserveDSVHeap			( RenderSystem* RS, size_t SlotCount );

	FLEXKITAPI DescHeapPOS PushRenderTarget			( RenderSystem* RS, Texture2D* Target, DescHeapPOS POS );

	FLEXKITAPI DescHeapPOS PushCBToDescHeap			( RenderSystem* RS, ID3D12Resource* Buffer,			DescHeapPOS POS, size_t BufferSize );
	FLEXKITAPI DescHeapPOS PushSRVToDescHeap		( RenderSystem* RS, ID3D12Resource* Buffer,			DescHeapPOS POS, size_t ElementCount, size_t Stride, D3D12_BUFFER_SRV_FLAGS Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE );
	FLEXKITAPI DescHeapPOS Push2DSRVToDescHeap		( RenderSystem* RS, ID3D12Resource* Buffer,			DescHeapPOS POS, D3D12_BUFFER_SRV_FLAGS Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE );
	FLEXKITAPI DescHeapPOS PushTextureToDescHeap	( RenderSystem* RS,	Texture2D tex,					DescHeapPOS POS );
	FLEXKITAPI DescHeapPOS PushUAV2DToDescHeap		( RenderSystem* RS, Texture2D tex,					DescHeapPOS POS, DXGI_FORMAT F = DXGI_FORMAT_R8G8B8A8_UNORM );


	FLEXKITAPI void SetViewport	 ( ID3D12GraphicsCommandList* CL, Texture2D Targets, uint2 Offset = {0, 0} );
	FLEXKITAPI void SetViewports ( ID3D12GraphicsCommandList* CL, Texture2D* Targets, uint2* Offsets, UINT Count = 1u );

	FLEXKITAPI void SetScissors	( ID3D12GraphicsCommandList* CL, uint2* WH, uint2* Offsets, UINT Count );
	FLEXKITAPI void SetScissor	( ID3D12GraphicsCommandList* CL, uint2  WH, uint2  Offset = {0, 0} );

	FLEXKITAPI void	IncrementRSIndex	( RenderSystem* RS );

	FLEXKITAPI ID3D12CommandAllocator*		GetCurrentCommandAllocator	( RenderSystem* RS );
	FLEXKITAPI ID3D12GraphicsCommandList*	GetCurrentCommandList		( RenderSystem* RS );
	FLEXKITAPI PerFrameResources*			GetCurrentFrameResources	( RenderSystem* RS );
	FLEXKITAPI Texture2D					GetRenderTarget				( RenderWindow* RW );

	FLEXKITAPI ID3D12GraphicsCommandList*	GetCommandList_1 ( RenderSystem* RS );


	FLEXKITAPI void BeginSubmission			( RenderSystem* RS, RenderWindow* Window );
	
	FLEXKITAPI void Close					( static_vector<ID3D12GraphicsCommandList*> CLs );
	FLEXKITAPI void Submit					( static_vector<ID3D12CommandList*>& CLs,		 RenderSystem* RS, RenderWindow* Window);
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

	
	FLEXKITAPI bool	CreateRenderWindow	( RenderSystem*, RenderWindowDesc* desc_in, RenderWindow* );
	FLEXKITAPI bool	ResizeRenderWindow	( RenderSystem*, RenderWindow* Window, uint2 HW );
	FLEXKITAPI void	SetInputWIndow		( RenderWindow* );
	FLEXKITAPI void	UpdateInput			( void );
	FLEXKITAPI void	UpdateCamera		( RenderSystem* RS, SceneNodes* Nodes, Camera* camera, double dt);
	FLEXKITAPI void	UploadCamera		( RenderSystem* RS, SceneNodes* Nodes, Camera* camera, int PointLightCount, int SpotLightCount, double dt, uint2 HW = {1u, 1u});

	
	/************************************************************************************************/

	FLEXKITAPI void					InitiateCamera				( RenderSystem* RS, SceneNodes* Nodes, Camera* out, float AspectRatio = 1.0f, float Near = 0.01, float Far = 10000.0f, bool invert = false );
	FLEXKITAPI ConstantBuffer		CreateConstantBuffer		( RenderSystem* RS, ConstantBuffer_desc* );
	FLEXKITAPI bool					CreateDepthBuffer			( RenderSystem* RS, uint2				Dimensions,	DepthBuffer_Desc&	DepthDesc,	DepthBuffer* out, ID3D12GraphicsCommandList* CL = nullptr );
	FLEXKITAPI Texture2D			CreateDepthBufferResource	( RenderSystem* RS, Texture2D_Desc*		desc_in,	bool				Float32);
	FLEXKITAPI bool					CreateInputLayout			( RenderSystem* RS, VertexBufferView**,  size_t count, Shader*, VertexBuffer* OUT );		// Expects Index buffer in index 15
	FLEXKITAPI Texture2D			CreateTexture2D				( RenderSystem* RS, Texture2D_Desc* desc_in );
	FLEXKITAPI void					CreateVertexBuffer			( RenderSystem* RS, VertexBufferView** Buffers, size_t BufferCount, VertexBuffer& DVB_Out ); // Expects Index buffer in index 15
	FLEXKITAPI ShaderResourceBuffer CreateShaderResource		( RenderSystem* RS, const size_t ResourceSize, const char* _DebugName = "CreateShaderResource" );
	FLEXKITAPI StreamOutBuffer		CreateStreamOut				( RenderSystem* RS, const size_t ResourceSize );
	FLEXKITAPI VertexResourceBuffer CreateVertexBufferResource	( RenderSystem* RS, const size_t ResourceSize );
	FLEXKITAPI VertexBufferView*	CreateVertexBufferView		( byte*, size_t );
	FLEXKITAPI QueryResource		CreateSOQuery				( RenderSystem* RS, D3D12_QUERY_HEAP_TYPE Type = D3D12_QUERY_HEAP_TYPE_SO_STATISTICS );

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

	FLEXKITAPI void UploadResources					( RenderSystem* RS);
	FLEXKITAPI void UpdateResourceByTemp			( RenderSystem* RS, ID3D12Resource* Dest, void* Data, size_t SourceSize, size_t ByteSize = 1, D3D12_RESOURCE_STATES EndState = D3D12_RESOURCE_STATE_COMMON);
	FLEXKITAPI void UpdateResourceByTemp			( RenderSystem* RS, FrameBufferedResource* Dest, void* Data, size_t SourceSize, size_t ByteSize = 1, D3D12_RESOURCE_STATES EndState = D3D12_RESOURCE_STATE_COMMON);
	FLEXKITAPI void UpdateSubResourceByUploadQueue	( RenderSystem* RS, ID3D12Resource* Dest, SubResourceUpload_Desc* Desc, D3D12_RESOURCE_STATES EndState);

	FLEXKITAPI void ReadyUploadQueues	 ( RenderSystem* RS );
	FLEXKITAPI void SubmitUploadQueues	 ( RenderSystem* RS );
	FLEXKITAPI void ShutDownUploadQueues ( RenderSystem* RS );

	FLEXKITAPI PerFrameUploadQueue* GetCurrentUploadQueue( RenderSystem* RS );

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


	FLEXKITAPI void DelayReleaseDrawable(RenderSystem* RS, Drawable* E);

	FLEXKITAPI void	Release( ConstantBuffer&	);
	FLEXKITAPI void	Release( Texture2D			);
	FLEXKITAPI void	Release( RenderWindow*		);
	FLEXKITAPI void	Release( VertexBuffer*		);
	FLEXKITAPI void	Release( Shader*			);
	FLEXKITAPI void	Release( SpotLightBuffer*	);
	FLEXKITAPI void	Release( PointLightBuffer*	);
	FLEXKITAPI void Release( DepthBuffer*		);


	/************************************************************************************************/


	FLEXKITAPI bool			LoadAndCompileShaderFromFile	(const char* FileLoc, ShaderDesc* desc, Shader* out);
	FLEXKITAPI Shader		LoadShader						(const char* Entry, const char* ID, const char* ShaderVersion, const char* File);
	FLEXKITAPI Texture2D	LoadTextureFromFile				(char* file, RenderSystem* RS, iAllocator* Memout);
	FLEXKITAPI Texture2D	LoadTexture						(TextureBuffer* Buffer,  RenderSystem* RS, iAllocator* Memout);

	FLEXKITAPI TextureBuffer CreateTextureBuffer			(size_t Width, size_t Height, iAllocator* Memout);


	FLEXKITAPI void			FreeTexture						(Texture2D* Tex);


	/************************************************************************************************/
	

	FLEXKITAPI Texture2D GetRenderTarget(RenderWindow* in);

	struct PIXELPROCESS_DESC
	{

	};

	FLEXKITAPI void	DoPixelProcessor(RenderSystem* RS, PIXELPROCESS_DESC* DESC_in, Texture2D* out);


	struct TextEntry
	{
		float2 POS;
		float2 Size;
		float2 TopLeftUV;
		float2 BottomRightUV;
		float4 Color;
	};

	struct TextArea
	{
		char*		TextBuffer;
		size_t		BufferSize;
		uint2		BufferDimensions;
		uint2		Position;
		float2		ScreenPosition;
		float2		CharacterScale;
		uint2		ScreenWH; // Screen Width - In Pixels

		ShaderResourceBuffer Buffer;

		size_t				 CharacterCount;
		size_t				 Dirty;

		iAllocator*			 Memory;
	};

	struct TextArea_Desc
	{
		float2 POS;		// Screen Space Cord
		float2 WH;		// WH of Area Being Rendered to,		Percent of Screen
		float2 CharWH;	// Size of Characters Being Rendered,	Percent of Screen
	};

	struct FontAsset
	{
		uint2	FontSize = { 0, 0 };
		bool	Unicode	 = false;

		uint2			TextSheetDimensions;
		Texture2D		Texture;
		uint4			Padding = 0; // Ordering { Top, Left, Bottom, Right }
		iAllocator*		Memory;

		struct Glyph
		{
			float2		WH;
			float2		XY;
			float2		Offsets;
			float		Xadvance;
			uint32_t	Channel;
		}GlyphTable[256];

		size_t	KerningTableSize = 0;
		struct Kerning
		{
			char	ID[2];
			float   Offset;
		}*KerningTable;


		char	FontName[128];
		char*	FontDir; // Texture Directory
	};


	/************************************************************************************************/


	struct LineSegment
	{
		float3 A;
		float3 AColour;
		float3 B;
		float3 BColour;
	};

	typedef Vector<LineSegment> LineSegments;

	struct LineSet
	{
		LineSegments			LineSegments;
		FrameBufferedResource	GPUResource;
		size_t					ResourceSize;
	};


	/************************************************************************************************/


	FLEXKITAPI void InitiateLineSet			( RenderSystem* RS, iAllocator* Mem, LineSet* out );
	FLEXKITAPI void ClearLineSet			( LineSet* Pass	);
	FLEXKITAPI void CleanUpLineSet			( LineSet* Pass	);
	FLEXKITAPI void AddLineSegment			( LineSet* Pass, LineSegment in		);
	FLEXKITAPI void UploadLineSegments		( RenderSystem* RS, LineSet* Pass	);


	/************************************************************************************************/


	FLEXKITAPI void ClearText		( TextArea* TA );
	FLEXKITAPI void CleanUpTextArea	( TextArea* TA, iAllocator* BA, RenderSystem* RS = nullptr );
	FLEXKITAPI void PrintText		( TextArea* Area, const char* text );

	FLEXKITAPI TextArea CreateTextArea	( RenderSystem* RS, iAllocator* Mem, TextArea_Desc* D);// Setups a 2D Surface for Drawing Text into
	FLEXKITAPI void		UploadTextArea	( FontAsset* F, TextArea* TA, iAllocator* Temp, RenderSystem* RS, RenderWindow* Target);


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


	FLEXKITAPI void CreatePointLightBuffer	( RenderSystem* RS, PointLightBuffer* out, PointLightBufferDesc Desc, iAllocator* Mem );
	FLEXKITAPI void CreateSpotLightBuffer	( RenderSystem* RS, SpotLightBuffer* out, iAllocator* Memory, size_t Max = 512 );

	FLEXKITAPI void Release	( PointLightBuffer* out,	iAllocator* Memory );
	FLEXKITAPI void Release	( SpotLightBuffer* out,		iAllocator* Memory );

	FLEXKITAPI LightHandle CreateLight		( PointLightBuffer*	PL, LightDesc& in );
	FLEXKITAPI LightHandle CreateLight		( SpotLightBuffer*	SL, LightDesc& in, float3 Dir, float p );
	
	FLEXKITAPI void ReleaseLight(PointLightBuffer*	PL, LightHandle Handle);

	FLEXKITAPI void UpdateSpotLightBuffer	( RenderSystem& RS, SceneNodes* nodes, SpotLightBuffer* out, iAllocator* TempMemory );
	FLEXKITAPI void UpdatePointLightBuffer	( RenderSystem& RS, SceneNodes* nodes, PointLightBuffer* out, iAllocator* TempMemory );


	/************************************************************************************************/
	

	template<typename Ty>
	void SetLightFlag( Ty* out, LightHandle light, LightBufferFlags flag )	{out->Flags->at(light.INDEX) ^= flag;}


	/************************************************************************************************/


	FLEXKITAPI void InitiateShadowMapPass	( RenderSystem* RenderSystem, ShadowMapPass* out);
	FLEXKITAPI void CleanUpShadowPass		( ShadowMapPass* Out );


	/************************************************************************************************/

	FLEXKITAPI void RenderShadowMap				( RenderSystem* RS, PVS* _PVS, SpotLightShadowCaster* Caster, Texture2D* RenderTarget, ShadowMapPass* PSOs, GeometryTable* GT );


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
	FLEXKITAPI void CreateDrawable		( RenderSystem* RS, Drawable* e,	DrawableDesc& desc );

	FLEXKITAPI bool LoadObjMesh			( RenderSystem* RS, char* File_Loc,	Obj_Desc IN desc, TriMesh ROUT out, StackAllocator RINOUT LevelSpace, StackAllocator RINOUT TempSpace, bool DiscardBuffers );
	FLEXKITAPI void UpdateDrawable		( RenderSystem* RS, SceneNodes* Nodes, Drawable* E );


	/************************************************************************************************/


	FLEXKITAPI void ReleaseDrawable		( Drawable*	p );
	FLEXKITAPI void ReleaseTriMesh		( TriMesh*	p );

	FLEXKITAPI void DelayedReleaseTriMesh ( RenderSystem* RS, TriMesh* T );


	/************************************************************************************************/


	inline void ClearTriMeshVBVs(TriMesh* Mesh) { for (auto& buffer : Mesh->Buffers) buffer = nullptr; }


	/************************************************************************************************/

	class RenderResources
	{

	};

	// Interface
	class RenderPass
	{
	public:
		virtual void Draw(RenderSystem* RS, ID3D12CommandList* CL, RenderResources* Resources, PVS* Elements) = delete;
		virtual void Release() = delete;
	};
}

#endif