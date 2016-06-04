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

#include "..\PCH.h"
#include "..\buildsettings.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\Events.h"
#include "..\coreutilities\Handle.h"
#include "..\coreutilities\mathutils.h"
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


	/************************************************************************************************/


	FLEXKITAPI void SetDebugName(ID3D12Object* Obj, const char* cstr, size_t size);

#ifdef _DEBUG
#define SETDEBUGNAME(RES, ID) {const char* NAME = ID; FlexKit::SetDebugName(RES, ID, strnlen(ID, 64));}
#else
#define SETDEBUGNAME(RES, ID) 
#endif


	/************************************************************************************************/


	enum KEYCODES
	{
		KC_ERROR,

		KC_A,
		KC_B,
		KC_C,
		KC_D,
		KC_E,
		KC_F,
		KC_G,
		KC_H,
		KC_I,
		KC_J,
		KC_K,
		KC_L,
		KC_M,
		KC_N,
		KC_O,
		KC_P,
		KC_Q,
		KC_R,
		KC_S,
		KC_T,
		KC_U,
		KC_V,
		KC_W,
		KC_X,
		KC_Y,
		KC_Z,

		KC_ESC,
		KC_LEFTSHIFT,
		KC_LEFTCTRL,
		KC_SPACE,
		KC_TAB,
		KC_RIGHTSHIFT,
		KC_RIGHTCTRL,

		KC_DELETE,
		KC_HOME,
		KC_END,
		KC_PAGEUP,
		KC_PAGEDOWN,

		KC_1,
		KC_2,
		KC_3,
		KC_4,
		KC_5,
		KC_6,
		KC_7,
		KC_8,
		KC_9,
		KC_0,

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
		KC_MOUSEMIDDLE
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
		VERTEXBUFFER_FORMAT_R16G16			= 4	,
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
		R8G8B8A_UINT,
		R8G8B8A8_UINT,
		R8G8B8A8_UNORM,
		D24_UNORM_S8_UINT,
		R32_FLOAT,
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
	enum class SPECIALFLAGS
	{
		BACKBUFFER = 1,
		DEPTHSTENCIL = 2,
		NONE = 0
	};


	/************************************************************************************************/


	struct Tex2DDesc
	{
		Tex2DDesc()
		{
			Height		 = 0;
			Width		 = 0;
			Write        = true;
			Read         = true;
			RenderTarget = false;
			UAV          = false;
			CV           = true;
			Format	= FORMAT_2D::D24_UNORM_S8_UINT;
			FLAGS	= SPECIALFLAGS::NONE;
		}

		Tex2DDesc(
			size_t			in_height,
			size_t			in_width,
			FORMAT_2D		in_format,
			CPUACCESSMODE	in_Mode,
			SPECIALFLAGS	in_FLAGS ) : Tex2DDesc()
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
		}
	};


	/************************************************************************************************/


	const static size_t MaxBufferedSize = 3;

	template<typename TY_>
	struct FrameBufferedObject
	{
		FrameBufferedObject(){ BufferCount = MaxBufferedSize; Idx = 0; for(auto& r : Resources) r = nullptr;}

		size_t			Idx;
		size_t			BufferCount;
		TY_*			Resources[MaxBufferedSize];

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
		void Release(){for( auto& r : Resources) {if(r) r->Release(); r = nullptr;};}

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
		int Dest; // PIPELINE_DESTINATION
		enum UDPATEFREQ
		{
			PERFRAME,
			MULTIFRAME,
			ONCE
		} Freq;

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

				inline void operator ++ ( )		{ m_position += sizeof( Ty ); }
				inline void operator ++ ( int ) { m_position += sizeof( Ty ); }
				inline void operator -- ( )		{ m_position -= sizeof( Ty ); }
				inline void operator -- ( int ) { m_position -= sizeof( Ty ); }

				inline Typed_iterator	operator =	( const Typed_iterator& rhs ) {}
				
				inline bool				operator == ( const Typed_iterator& rhs ) {	return  ( m_position == &*rhs ); }
				inline bool				operator != ( const Typed_iterator& rhs ) { return !( m_position == &*rhs ); }

				inline Ty&				operator * ()	const { return *m_position; }

				inline const Ty&		peek_forward()	const { return *( m_position + sizeof( Ty ) ); }

			private:

				Ty*	m_position;
			};

			Ty&	operator [] ( size_t index )	{ return m_position[index]; }
			Typed_iterator begin()				{ return Typed_iterator(  m_position ); }
			Typed_iterator end()				{ return Typed_iterator( &m_position[m_size]); }

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
		Tex2DDesc BufferFormat;
		Tex2DDesc BufferInterp;
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
		D3D12_CPU_DESCRIPTOR_HANDLE View[4];
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
		D3D12_CPU_DESCRIPTOR_HANDLE	View	[MaxBufferedSize];
		size_t						CurrentBuffer;
		size_t						BufferCount;
		bool						Inverted;
	};


	/************************************************************************************************/


	enum EInputTopology
	{
		EIT_TRIANGLELIST,
		EIT_TRIANGLE,
		EIT_POINT,
	};


	/************************************************************************************************/

		
	struct DescriptorHeaps
	{
		ID3D12DescriptorHeap*	RTVDescHeap;
		ID3D12DescriptorHeap*	DSVDescHeap;

		size_t					DescriptorRTVSize;
		size_t					DescriptorDSVSize;
		size_t					DescriptorCBVSRVUAVSize;
	};


	/************************************************************************************************/

	typedef	fixed_vector<ID3D12Resource*> TempResourceList;
	const static int QueueSize		= 3;
	const static int MaxThreadCount = 4;

	struct PerFrameResources
	{
		TempResourceList*			TempBuffers;
		ID3D12GraphicsCommandList*	CommandLists[MaxThreadCount];
		ID3D12CommandAllocator*		GraphicsCLAllocator[MaxThreadCount];
		ID3D12CommandAllocator*		UploadCLAllocator[MaxThreadCount];
		ID3D12CommandAllocator*		ComputeCLAllocator[MaxThreadCount];
		ID3D12GraphicsCommandList*	UploadList[MaxThreadCount];
		ID3D12GraphicsCommandList*	ComputeList[MaxThreadCount];
		ID3D12DescriptorHeap*		SRVDescHeap; // Texture List

		D3D12_CPU_DESCRIPTOR_HANDLE CPU_HeapPOS;
		D3D12_GPU_DESCRIPTOR_HANDLE GPU_HeapPOS;

		size_t						ThreadsIssued;
	};

	struct RenderSystem
	{
		iAllocator*				Memory;

		// TODO: Add Asset Manager _ptr Here

		ID3D12Device*		pDevice;
		ID3D12Debug*		pDebug;
		ID3D12DebugDevice*	pDebugDevice;

		PerFrameResources FrameResources[QueueSize];

		IDXGIFactory4*		pGIFactory;
		ID3D12CommandQueue*	GraphicsQueue;
		ID3D12CommandQueue*	UploadQueue;
		ID3D12CommandQueue*	ComputeQueue;

		size_t CurrentIndex;
		size_t FenceCounter;

		ID3D12Fence* Fence;

		struct
		{
			size_t			FenceValue;
			HANDLE			FenceHandle;
		}Fences[QueueSize];

		size_t BufferCount;
		size_t DescriptorRTVSize;
		size_t DescriptorDSVSize;
		size_t DescriptorCBVSRVUAVSize;

		DescriptorHeaps	DefaultDescriptorHeaps;

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
			ID3D12RootSignature* RS4CBVs4SRVs;// 4CBVs On all Stages, 4 SRV On all Stages
			ID3D12RootSignature* RS4CBVs_SO;// Stream Out Enabled
		}Library;

		operator RenderSystem* ( ) { return this; }
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

		Shader& operator = (const Shader& in) {
			Blob     = in.Blob;
			Type     = in.Type;

			return *this;
		}

		ID3DBlob*			Blob;
		SHADER_TYPE			Type;
	};


	const	size_t							NodeHandleSize = 16;
	typedef Handle_t<NodeHandleSize>		NodeHandle;
	typedef Handle_t<16>					TextureSetHandle;
	typedef static_vector<NodeHandle, 32>	ChildrenVector;


	struct Node
	{
		NodeHandle	TH;
		NodeHandle	Parent;
		NodeHandle	ChildrenList;
		bool		Scaleflag;// Calculates Scale only when set to on, Off By default
	};

	
	__declspec(align(16))  struct LT_Entry
	{
		DirectX::XMVECTOR T;
		DirectX::XMVECTOR R;
		DirectX::XMVECTOR S;
		DirectX::XMVECTOR Padding;

		static LT_Entry Zero()
		{
			LT_Entry zero;
			zero.T = DirectX::XMVectorZero();
			zero.R = DirectX::XMQuaternionIdentity();
			zero.S = DirectX::XMVectorSet(1, 1, 1, 0);

			return zero;
		}
	};


	__declspec(align(16))  struct WT_Entry
	{
		//LT_Entry			World;
		DirectX::XMMATRIX	m4x4;// Cached

		void SetToIdentity()	
		{	
			m4x4  = DirectX::XMMatrixIdentity(); 
			//World = LT_Entry::Zero();
		}	
	};


	/************************************************************************************************/

	
	inline float4x4 XMMatrixToFloat4x4(DirectX::XMMATRIX* M)
	{
		float4x4 Mout;
		Mout = *(float4x4*)M;
		return Mout;
	}


	inline DirectX::XMMATRIX Float4x4ToXMMATIRX(float4x4* M)
	{
		DirectX::XMMATRIX Mout;
		Mout = *(DirectX::XMMATRIX*)M;
		return Mout;
	}


	/************************************************************************************************/


	struct SceneNodes
	{
		enum StateFlags : char
		{
			CLEAR   = 0x00,
			DIRTY   = 0x01,
			FREE    = 0x02,
			SCALE   = 0x04,
			UPDATED = 0x08
		};

		operator SceneNodes* () { return this; }

		size_t used;
		size_t max;

		Node*			Nodes;
		LT_Entry*		LT;
		WT_Entry*		WT;
		char*			Flags;

		uint16_t*		Indexes;
		ChildrenVector* Children;


		#pragma pack(push, 1)
		struct BOILERPLATE
		{
			Node		N;
			LT_Entry	LT;
			WT_Entry	WT;
			uint16_t	I;
			char		State;
		};
		#pragma pack(pop)
	};

	
	/************************************************************************************************/


	struct DeferredPass_Parameters
	{
		float4x4 InverseZ;
		uint64_t PointLightCount;
		uint64_t SpotLightCount;
	};


	struct DeferredPassDesc
	{
		DepthBuffer*	DepthBuffer;
		RenderWindow*	RenderWindow;
		byte* _ptr;
	};


	struct GBufferConstantsLayout
	{
		uint32_t DLightCount;
		uint32_t PLightCount;
		uint32_t SLightCount;
		uint32_t Height;
		uint32_t Width;
		float4	 AmbientLight;
		//char padding[1019];
	};


	enum DeferredShadingRootParam
	{
		DSRP_DescriptorTable  = 0,
		DSRP_CameraConstants  = 1,
		DSRP_ShadingConstants = 2,
		DSRP_COUNT,
	};


	enum DeferredFillingRootParam
	{
		DFRP_CameraConstants	= 0,
		DFRP_ShadingConstants	= 1,
		DFRP_AnimationResources = 2,
		DFRP_COUNT,
	};


	struct DeferredPass
	{
		// GBuffer
		struct GBuffer
		{
			D3D12_CPU_DESCRIPTOR_HANDLE	DepthBuffer;
			ID3D12DescriptorHeap* RTVDescHeap;

			Texture2D ColorTex;
			Texture2D SpecularTex;
			Texture2D NormalTex;
			Texture2D PositionTex;
			Texture2D OutputBuffer;
		}GBuffers[3];
		size_t CurrentBuffer;

		// Shading
		struct
		{
			ID3D12PipelineState*	ShadingPSO;
			ConstantBuffer			ShaderConstants;
			ID3D12RootSignature*	ShadingRTSig;
			Shader					Shade;		// Compute Shader
		}Shading;

		// GBuffer Filling
		struct
		{
			ID3D12PipelineState*	PSO;
			ID3D12PipelineState*	PSOAnimated;
			ID3D12RootSignature*	FillRTSig;
			Shader					NormalMesh;
			Shader					AnimatedMesh;
			Shader					NoTexture;
		}Filling;
	};


	/************************************************************************************************/


	struct ForwardPass
	{
		RenderWindow*				RenderTarget;
		DepthBuffer*				DepthTarget;
		ID3D12GraphicsCommandList*	CommandList;
		ID3D12CommandAllocator*		CommandAllocator;
		ID3D12PipelineState*		PSO;
		ID3D12RootSignature*		PassRTSig;
		ID3D12DescriptorHeap*		CBDescHeap;

		Shader VShader;
		Shader PShader;
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
		Dirty = 0x01,
		Clean = 0x00
	};


	const size_t PLB_Stride  = sizeof(float[8]);
	const size_t SPLB_Stride = sizeof(float[8]);


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
		void CleanUp()	{Resource->Release();}
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

		void CleanUp(){Resource->Release();}
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
			DirectX::XMMATRIX View;
			DirectX::XMMATRIX Proj;
			DirectX::XMMATRIX WT;			// World Transform
			DirectX::XMMATRIX PV;			// Projection x View
			DirectX::XMMATRIX IV;			// Inverse Transform
			float3			  WPOS;
			float			  MinZ;
			float			  MaxZ;
			uint32_t			PointLightCount;
			uint32_t			SpotLightCount;
			static const size_t GetBufferSize(){return 1024;}
		};

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

		struct BoundingBox
		{
			float3 TopLeft, BottomRight;
		}OBB;

		struct BoundingSphere
		{
			float r;
		}BoundingSphere;
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
			float3 min, max;
			float  r;
		}Info;

		Occlusion_Volume EOV_type;
		BoundingVolume	 Bounding;

		size_t SkeletonGUID;
	};


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
		if (VB == Mesh->VertexBuffer.VertexBuffers.end())
			return false;

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
		ETT_ROUGHNESS,
		ETT_NORMAL,
		ETT_BUMB,
		ETT_COUNT,
	};

	struct TextureSet
	{
		size_t	Textures[16];
	};


	struct TextureManager
	{
		DynArray<TextureSet>	Textures;
		DynArray<uint32_t>		FreeList;
	};



	class FLEXKITAPI ShaderTable
	{
	public:
		ShaderTable();

		typedef char DirtFlag;

		float4	GetAlbedo	(ShaderSetHandle hndl);
		float4	GetMetal	(ShaderSetHandle hndl);

		float4	GetAlbedo	(ShaderSetHandle hndl)	const;
		float4	GetMetal	(ShaderSetHandle hndl)	const;

		void	SetAlbedo	(ShaderSetHandle hndl, float4);
		void	SetMetal	(ShaderSetHandle hndl, float4);


		ShaderHandle	AddShader	 (Shader);
		Shader			GetShader	 (ShaderHandle hndl);
		void			SetShader	 (ShaderHandle hndl, Shader);
		void			FreeShader	 (ShaderHandle hndl);

		DirtFlag		GetDirtyFlag (ShaderSetHandle hndl);

		Shader			GetPixelShader			 (ShaderSetHandle hndl);
		Shader			GetVertexShader			 (ShaderSetHandle hndl);
		Shader			GetVertexShader_Animated (ShaderSetHandle hndl);
		Shader			GetGeometryShader		 (ShaderSetHandle hndl);

		ShaderSetHandle	GetNewShaderSet();
		ShaderHandle	GetNewShaderHandle();
		ShaderSetHandle	GetDefaultMaterial()		  { return DefaultMaterial;}

		void SetDefaultMaterial(ShaderSetHandle mhndl) { DefaultMaterial = mhndl; }

		struct ShaderSet
		{
			ShaderSet()
			{
				inUse = false;
			}

			ShaderHandle PShader;
			ShaderHandle GShader;
			ShaderHandle HShader;
			ShaderHandle VShader;
			ShaderHandle DShader;
			ShaderHandle VShader_Animated;
			bool inUse;
		};

		size_t	RegisterShaderSet(ShaderSet& in);
		void	SetSSet(ShaderSetHandle, size_t);

	private:
		ShaderSetHandle	previousShaderSetHandle;
		ShaderSetHandle	DefaultMaterial; // Used as a fallback option

		size_t			previousShaderSet;

		static_vector<float4,	128> Albedo;
		static_vector<float4,	128> Metal;
		static_vector<char,		128> SSetMappings;
		static_vector<Shader,	128> Shaders;
		static_vector<DirtFlag, 128> DirtyFlags;
		static_vector<ShaderSet, 32> SSets;

		FlexKit::HandleUtilities::HandleTable<ShaderSetHandle>	ShaderSetHndls;
		FlexKit::HandleUtilities::HandleTable<ShaderHandle>		ShaderHandles;
	};

	struct DrawableDesc
	{
		ShaderSetHandle Material;
	};

	struct DrawableAnimationState;
	struct DrawablePoseState;

	struct Drawable
	{
		Drawable() 
		{
			Visable                    = true; 
			OverrideMaterialProperties = false;
			Occluder                   = nullptr;
			DrawLast				   = false;
			Transparent				   = false;
			Posed					   = false; // Use Vertex Palette Skinning
			PoseState				   = nullptr;
			AnimationState			   = nullptr;
		}
		
		TriMesh*				Mesh;
		TriMesh*				Occluder;
		DrawablePoseState*		PoseState;
		DrawableAnimationState*	AnimationState;
		ConstantBuffer			VConstants;	// 32 Byte Lines
		NodeHandle				Node;

		Drawable&	SetAlbedo	(float4 RGBA ){ OverrideMaterialProperties = true; MatProperties.Albedo = RGBA; return *this; }
		Drawable&	SetSpecular	(float4 RGBA ){ OverrideMaterialProperties = true; MatProperties.Spec   = RGBA; return *this; }
		Drawable&	SetNode		(NodeHandle H){ Node = H; return *this; }

		ShaderSetHandle		Material;
		TextureSetHandle	Textures;
		bool				Visable;
		bool				OverrideMaterialProperties;
		bool				DrawLast;
		bool				Transparent;
		bool				Posed;
		byte				Dirty;

		char Padding_1[21];

		struct MaterialProperties
		{
			MaterialProperties(float4 a, float4 m) : Albedo(a), Spec(m)
			{
				Albedo.w	= min(a.w, 1.0f);
				Spec.w		= min(m.w, 1.0f);
			}
			MaterialProperties() : Albedo(1.0, 1.0f, 1.0f, 0.5f), Spec(0)
			{}
			FlexKit::float4		Albedo;		// Term 4 is Roughness
			FlexKit::float4		Spec;		// Metal Is first 4, Specular is rgb
		}MatProperties;

		char Padding_2[32];

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
		unsigned int MaterialID		: 7;
		unsigned int InvertDepth	: 1;
		uint64_t	 Depth			: 55;
	};

	typedef Pair<size_t, Drawable*> PV;
	typedef static_vector<PV, 16000> PVS;

	inline void PushPV(Drawable* e, PVS* pvs)
	{
	#ifdef _DEBUG
		if (e)
	#endif

		if (e->Visable && e->Mesh)
			pvs->push_back({ 0, e });
	}

	FLEXKITAPI void UpdateDrawables		(RenderSystem* RS, SceneNodes* Nodes, FlexKit::ShaderTable* M, PVS* PVS_);
	FLEXKITAPI void SortPVS				(SceneNodes* Nodes, PVS* PVS_, Camera* C);
	FLEXKITAPI void SortPVSTransparent	(SceneNodes* Nodes, PVS* PVS_, Camera* C);

	/************************************************************************************************/


	struct FLEXKITAPI StaticMeshBatcher
	{
		const static size_t MAXINSTANCES = 1024 * 1;
		const static size_t MAXTRIMESHES = 16;

		StaticMeshBatcher() : ObjectTable(GetTypeID<StaticMeshBatcher>()) {}

		typedef FlexKit::Handle_t<16> SceneObjectHandle;
		typedef char DirtyFlag;

		enum DirtyStates : char
		{
			CLEAN				= 0x02,
			DIRTY_Transform		= 0x01,
			DIRTY_ChangedMesh	= 0x02
		};
		
		void Initiate(FlexKit::RenderSystem* RS, ShaderHandle StaticMeshBatcher, ShaderHandle pshade);
		void CleanUp();

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

		void Update(SceneNodes* Nodes)
		{
		}

		void PrintFrameStats();
		void Upload				( RenderSystem* RS, SceneNodes* Nodes, iAllocator* Temp, Camera* C );
		void BuildGeometryTable ( FlexKit::RenderSystem* RS, FlexKit::ShaderTable* M, StackAllocator* TempMemory );
		
		SceneObjectHandle CreateDrawable( NodeHandle node, size_t GeometryIndex = 0 );

		void Draw( FlexKit::RenderSystem* RS, FlexKit::ShaderTable* M, Camera* C );

		inline void SetDirtyFlag( DirtyFlag Flag, SceneObjectHandle hndl ){
			auto CFlag = DirtyFlags[ObjectTable[hndl]];
			CFlag |= Flag;
		}


		struct
		{
			uint32_t VertexCount;
			uint32_t VertexOffset;
			uint32_t IndexOffset;
		}GeometryTable[MAXTRIMESHES];

		TriMesh* Geometry[MAXTRIMESHES];

		struct InstanceIOLayout
		{
			uint32_t IndexCountPerInstance;
			uint32_t InstanceCount;
			uint32_t StartIndexLocation;
			int32_t	 BaseVertexLocation;
			uint32_t StartInstanceLocation;
		}								InstanceInformation	[MAXINSTANCES];
		DynArray<DirectX::XMMATRIX>		Transforms			[MAXINSTANCES];
		char							GeometryIndex		[MAXINSTANCES];
		DirtyFlag						DirtyFlags			[MAXINSTANCES];
		NodeHandle						NodeHandles			[MAXINSTANCES];

		HandleUtilities::HandleTable<SceneObjectHandle>	ObjectTable;
		size_t											TriMeshCount;
		size_t											MaxVertexPerInstance;
		size_t											InstanceCount[MAXTRIMESHES];
		size_t											TotalInstanceCount;

		ID3D12Resource*				Instances;
		FrameBufferedResource		TransformsBuffer;
		FrameBufferedResource		RenderARGs;

		ID3D12Resource* NormalBuffer;
		ID3D12Resource* TangentBuffer;
		ID3D12Resource* IndexBuffer;
		ID3D12Resource* VertexBuffer;
		ID3D12Resource* UVBuffer;
		ID3D12Resource* GTBuffer;

		ID3D12DescriptorHeap*	SRVHeap;
		Shader VShader;
		Shader PShader;
	};

	
	/************************************************************************************************/

	
	inline float2 PixelToSS(size_t X, size_t Y, uint2 Dimensions) {	return { -1.0f + (float(X) / Dimensions[x]), 1.0f -  (float(Y) / Dimensions[y])	}; } // Assumes screen boundaries are -1 and 1

	FLEXKITAPI VertexBuffer::BuffEntry* GetBuffer( VertexBuffer*, VERTEXBUFFER_TYPE ); // return nullptr if not found
	
	FLEXKITAPI void	InitiateCamera		( RenderSystem* RS, SceneNodes* Nodes, Camera* out, float AspectRatio = 1.0f, float Near = 0.01, float Far = 10000.0f, bool invert = false );
	FLEXKITAPI void	InitiateRenderSystem( Graphics_Desc* desc_in, RenderSystem* );

	FLEXKITAPI void	CleanUp			( RenderSystem* System );
	FLEXKITAPI void	CleanUpCamera	( SceneNodes* Nodes, Camera* camera );

	
	/************************************************************************************************/

	
	FLEXKITAPI bool	CreateRenderWindow	( RenderSystem*, RenderWindowDesc* desc_in, RenderWindow* );
	FLEXKITAPI bool	ResizeRenderWindow	( RenderSystem*, RenderWindow* Window, uint2 HW );
	FLEXKITAPI void	SetInputWIndow		( RenderWindow* );
	FLEXKITAPI void	UpdateInput			( void );
	FLEXKITAPI void	UpdateCamera		( RenderSystem* RS, SceneNodes* Nodes, Camera* camera, double dt);
	FLEXKITAPI void	UploadCamera		( RenderSystem* RS, SceneNodes* Nodes, Camera* camera, int PointLightCount, int SpotLightCount, double dt);

	
	/************************************************************************************************/

	FLEXKITAPI void AddTempBuffer		  ( ID3D12Resource* _ptr, RenderSystem* RS);
	FLEXKITAPI void	PresentWindow		  ( RenderWindow* RW, RenderSystem* RS );
	FLEXKITAPI void	WaitforGPU			  ( RenderSystem* RS );

	
	/************************************************************************************************/


	FLEXKITAPI ConstantBuffer		CreateConstantBuffer		( RenderSystem* RS, ConstantBuffer_desc* );
	FLEXKITAPI void					CreateDepthBuffer			( RenderSystem* RS, uint2 Dimensions, DepthBuffer_Desc& DepthDesc, DepthBuffer* out );
	FLEXKITAPI Texture2D			CreateDepthBufferResource	( RenderSystem* RS, Tex2DDesc* desc_in, bool Float32);
	FLEXKITAPI bool					CreateInputLayout			( RenderSystem* RS, VertexBufferView**,  size_t count, Shader*, VertexBuffer* OUT );		// Expects Index buffer in index 15
	FLEXKITAPI Texture2D			CreateTexture2D				( RenderSystem* RS, Tex2DDesc* desc_in );
	FLEXKITAPI void					CreateVertexBuffer			( RenderSystem* RS, VertexBufferView** Buffers, size_t BufferCount, VertexBuffer& DVB_Out ); // Expects Index buffer in index 15
	FLEXKITAPI ShaderResourceBuffer CreateShaderResource		( RenderSystem* RS, size_t ResourceSize );
	FLEXKITAPI StreamOutBuffer		CreateStreamOut				( RenderSystem* RS, size_t ResourceSize );
	FLEXKITAPI VertexResourceBuffer CreateVertexBufferResource	( RenderSystem* RS, size_t ResourceSize );
	FLEXKITAPI VertexBufferView*	CreateVertexBufferView		( byte*, size_t );
	FLEXKITAPI QueryResource		CreateSOQuery(RenderSystem* RS, D3D12_QUERY_HEAP_TYPE Type = D3D12_QUERY_HEAP_TYPE_SO_STATISTICS);


	FLEXKITAPI void UploadResources(RenderSystem* RS);
	FLEXKITAPI void UpdateResourceByTemp(RenderSystem* RS, ID3D12Resource* Dest, void* Data, size_t SourceSize, size_t ByteSize = 1, D3D12_RESOURCE_STATES EndState = D3D12_RESOURCE_STATE_COMMON);
	FLEXKITAPI void UpdateResourceByTemp(RenderSystem* RS, FrameBufferedResource* Dest, void* Data, size_t SourceSize, size_t ByteSize = 1, D3D12_RESOURCE_STATES EndState = D3D12_RESOURCE_STATE_COMMON);


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
		for (size_t itr = 0; itr < vertexCount; ++itr)
		{
			auto temp = Translate(Fetch(itr, Container));
			out->Push(temp);
		}

		return out->End();
	}


	/************************************************************************************************/


	FLEXKITAPI void	Destroy( ConstantBuffer		);
	FLEXKITAPI void	Destroy( Texture2D			);
	FLEXKITAPI void	Destroy( RenderWindow*		);
	FLEXKITAPI void	Destroy( VertexBuffer*		);
	FLEXKITAPI void	Destroy( Shader*			);
	FLEXKITAPI void	Destroy( SpotLightBuffer*	);
	FLEXKITAPI void	Destroy( PointLightBuffer*	);
	FLEXKITAPI void Destroy( DepthBuffer*		);


	/************************************************************************************************/


	FLEXKITAPI bool	LoadComputeShaderFromFile			( RenderSystem* RS, char* FileLoc, ShaderDesc*, Shader* OUT );
	FLEXKITAPI bool	LoadComputeShaderFromString			( RenderSystem* RS, char*, size_t, ShaderDesc*, Shader* OUT );
	FLEXKITAPI bool	LoadPixelShaderFromFile				( RenderSystem* RS, char* FileLoc, ShaderDesc*, Shader* OUT );
	FLEXKITAPI bool	LoadPixelShaderFromString			( RenderSystem* RS, char*, size_t, ShaderDesc*, Shader* OUT );
	FLEXKITAPI bool	LoadGeometryShaderFromFile			( RenderSystem* RS, char* FileLoc, ShaderDesc*, Shader* OUT );
	FLEXKITAPI bool	LoadGeometryWithSOShaderFromFile	( RenderSystem* RS, char* FileLoc, ShaderDesc* desc, SODesc* SOD,  Shader* OUT);
	FLEXKITAPI bool	LoadGeometryShaderFromString		( RenderSystem* RS, char*, size_t, ShaderDesc*, Shader* OUT );
	FLEXKITAPI bool	LoadGeometryShaderWithSOFromString	( RenderSystem* RS, char* str, size_t strlen, ShaderDesc* desc, SODesc* SOD, Shader* out);

	FLEXKITAPI bool	LoadVertexShaderFromFile	( RenderSystem* RS, char* FileLoc, ShaderDesc*, Shader* OUT );
	FLEXKITAPI bool	LoadVertexShaderFromString	( RenderSystem* RS, char*, size_t, ShaderDesc*, Shader* OUT );
	

	FLEXKITAPI Texture2D LoadTextureFromFile(char* file, RenderSystem* RS, iAllocator* Memout);
	FLEXKITAPI void FreeTexture(Texture2D* Tex);

	/************************************************************************************************/
	
	FLEXKITAPI Texture2D GetRenderTarget(RenderWindow* in);

	struct PIXELPROCESS_DESC
	{

	};

	FLEXKITAPI void	DoPixelProcessor(RenderSystem* RS, PIXELPROCESS_DESC* DESC_in, Texture2D* out);

	/************************************************************************************************/
	

	inline size_t	_SNHandleToIndex	( SceneNodes* Nodes, NodeHandle Node)				{ return Nodes->Indexes[Node.INDEX];	}
	inline void		_SNSetHandleIndex	( SceneNodes* Nodes, NodeHandle Node, size_t index)	{ Nodes->Indexes[Node.INDEX] = index;	}

	FLEXKITAPI void			InitiateSceneNodeBuffer		( SceneNodes* Nodes, byte* pmem, size_t );
	FLEXKITAPI void			SortNodes					( SceneNodes* Nodes, StackAllocator* Temp );
	FLEXKITAPI void			FreeHandle					( SceneNodes* Nodes, NodeHandle Node );

	inline	   LT_Entry		GetLocal					( SceneNodes* Nodes, NodeHandle Node)											{return Nodes->LT[_SNHandleToIndex(Nodes, Node)];}
	FLEXKITAPI void			GetWT						( SceneNodes* Nodes, NodeHandle Node,	DirectX::XMMATRIX* __restrict out );
	FLEXKITAPI void			GetWT						( SceneNodes* Nodes, NodeHandle Node,	WT_Entry* __restrict out );		
	FLEXKITAPI void			GetWT						( SceneNodes* Nodes, NodeHandle node,	float4x4* __restrict out );
	FLEXKITAPI void			GetOrientation				( SceneNodes* Nodes, NodeHandle Node,	Quaternion* Out );
	FLEXKITAPI float3		GetPositionW				( SceneNodes* Nodes, NodeHandle Node );
	FLEXKITAPI NodeHandle	GetNewNode					( SceneNodes* Nodes );
	FLEXKITAPI NodeHandle	GetZeroedNode				( SceneNodes* Nodes );
	FLEXKITAPI bool			GetFlag						( SceneNodes* Nodes, NodeHandle Node,	size_t f );

	FLEXKITAPI void			SetFlag						( SceneNodes* Nodes, NodeHandle Node,	SceneNodes::StateFlags f );
	FLEXKITAPI void			SetLocal					( SceneNodes* Nodes, NodeHandle Node,	LT_Entry* __restrict In );
	FLEXKITAPI void			SetOrientation				( SceneNodes* Nodes, NodeHandle Node,	Quaternion& In );	// Sets World Orientation
	FLEXKITAPI void			SetOrientationL				( SceneNodes* Nodes, NodeHandle Node,	Quaternion& In );	// Sets World Orientation
	FLEXKITAPI void			SetParentNode				( SceneNodes* Nodes, NodeHandle Parent, NodeHandle Node );
	FLEXKITAPI void			SetPositionW				( SceneNodes* Nodes, NodeHandle Node,	float3 in );
	FLEXKITAPI void			SetPositionL				( SceneNodes* Nodes, NodeHandle Node,	float3 in );
	FLEXKITAPI void			SetWT						( SceneNodes* Nodes, NodeHandle Node,	DirectX::XMMATRIX* __restrict in  );				// Set World Transform
	
	FLEXKITAPI void			Scale						( SceneNodes* Nodes, NodeHandle Node,	float3 In );
	FLEXKITAPI void			TranslateLocal				( SceneNodes* Nodes, NodeHandle Node,	float3 In );
	FLEXKITAPI void			TranslateWorld				( SceneNodes* Nodes, NodeHandle Node,	float3 In );
	FLEXKITAPI bool			UpdateTransforms			( SceneNodes* Nodes );
	FLEXKITAPI NodeHandle	ZeroNode					( SceneNodes* Nodes, NodeHandle Node );

	FLEXKITAPI inline void Yaw							( SceneNodes* Nodes, NodeHandle Node,	float r );
	FLEXKITAPI inline void Roll							( SceneNodes* Nodes, NodeHandle Node,	float r );
	FLEXKITAPI inline void Pitch						( SceneNodes* Nodes, NodeHandle Node,	float r );

	/************************************************************************************************/


	struct FLEXKITAPI SceneNodeCtr
	{
		inline void WTranslate	(float3 XYZ){TranslateWorld(SceneNodes, Node, XYZ);}
		inline void LTranslate	(float3 XYZ){TranslateLocal(SceneNodes, Node, XYZ);}
		void		Rotate		(Quaternion Q);

		NodeHandle	Node;
		SceneNodes*	SceneNodes;
	};

	enum class DRAWCALLTYPE
	{
		DCT_2DRECT,
		DCT_2DRECTTEXTURED,
		DCT_3DMesh,
		DCT_3DMeshAnimated,
		DCT_TextBox,
	};

	struct Draw_RECT
	{
		float2		TLeft;
		float2		BRight;
		uint32_t	ZOrder;
	};

	struct Draw_Textured_RECT
	{
		float2			TLeft;
		float2			BRight;
		float2			TLeft_UVOffset;
		float2			BRight_UVOffset;
		uint32_t		ZOrder;
		ResourceHandle	TextureHandle;
	};

	struct GUIRender
	{
		DynArray<Draw_RECT>				Rects;
		DynArray<Draw_Textured_RECT>	TexturedRects;

		FrameBufferedResource	RectBuffer;
		ID3D12DescriptorHeap*	Textures;
		ID3D12PipelineState*	DrawRectState;
	};


	/************************************************************************************************/


	typedef FlexKit::Handle	LightHandle;

	struct LightDesc{
		NodeHandle	Hndl;
		float3		K;
		float		I;
		float		R;
	};

	
	/************************************************************************************************/


	FLEXKITAPI int2 GetMousedPos(RenderWindow* Window);
	FLEXKITAPI void HideSystemCursor(RenderWindow* Window);
	FLEXKITAPI void SetSystemCursorToWindowCenter(RenderWindow* Window);
	FLEXKITAPI void ShowSystemCursor(RenderWindow* Window);

	
	/************************************************************************************************/

	FLEXKITAPI void PushRect( GUIRender* RG, Draw_RECT );
	FLEXKITAPI void PushRect( GUIRender* RG, Draw_Textured_RECT );
	
	FLEXKITAPI void InitiateRenderGUI( RenderSystem* RS, GUIRender* RG);
	FLEXKITAPI void CleanUpRenderGUI( RenderSystem* RS, GUIRender* RG);

	FLEXKITAPI void Clear	 ( GUIRender* RG );
	FLEXKITAPI void DrawRects( RenderSystem* RS, GUIRender* RG, RenderWindow* Target);// TODO: RENDER TARGET VERSION

	FLEXKITAPI void CreatePointLightBuffer	( RenderSystem* RS, PointLightBuffer* out, PointLightBufferDesc Desc, iAllocator* Mem );
	FLEXKITAPI void CreateSpotLightBuffer	( RenderSystem* RS, SpotLightBuffer* out, iAllocator* Memory, size_t Max = 512 );

	FLEXKITAPI void CleanUp	( PointLightBuffer* out,	iAllocator* Memory );
	FLEXKITAPI void CleanUp	( SpotLightBuffer* out,		iAllocator* Memory );

	FLEXKITAPI LightHandle CreateLight		( PointLightBuffer*	PL, LightDesc& in );
	FLEXKITAPI LightHandle CreateLight		( SpotLightBuffer*	SL, LightDesc& in, float3 Dir, float p );
	
	FLEXKITAPI void UpdateSpotLightBuffer	( RenderSystem& RS, SceneNodes* nodes, SpotLightBuffer* out, iAllocator* TempMemory );
	FLEXKITAPI void UpdatePointLightBuffer	( RenderSystem& RS, SceneNodes* nodes, PointLightBuffer* out, iAllocator* TempMemory );


	/************************************************************************************************/
	
	template<typename Ty>
	void SetLightFlag( Ty* out, LightHandle light, LightBufferFlags flag )	{out->Flags->at(light) ^= flag;}


	/************************************************************************************************/


	FLEXKITAPI void InitiateDeferredPass	( FlexKit::RenderSystem* RenderSystem, DeferredPassDesc* GBdesc, DeferredPass* out );
	FLEXKITAPI void DoDeferredPass			( PVS* _PVS, DeferredPass* Pass, Texture2D Target, RenderSystem* RS, const Camera* C, const float4& ClearColor, const PointLightBuffer* PLB, const SpotLightBuffer* SPLB, TextureManager* TM);
	FLEXKITAPI void ShadeDeferredPass		( PVS* _PVS, DeferredPass* Pass, Texture2D Target, RenderSystem* RS, const Camera* C, const float4& ClearColor, const PointLightBuffer* PLB, const SpotLightBuffer* SPLB);
	FLEXKITAPI void CleanupDeferredPass		( DeferredPass* gb );
	FLEXKITAPI void ClearGBuffer			( RenderSystem* RS, DeferredPass* gb, const float4& ClearColor, size_t Idx );
	FLEXKITAPI void UpdateGBufferConstants	( RenderSystem* RS, DeferredPass* gb, size_t PLightCount, size_t SLightCount );


	/************************************************************************************************/
	
	struct ForwardPass_DESC
	{
		DepthBuffer*	DepthBuffer;
		RenderWindow*	OutputTarget;
	};

	FLEXKITAPI ID3D12CommandAllocator*		GetCurrentCommandAllocator	( RenderSystem* RS );
	FLEXKITAPI ID3D12GraphicsCommandList*	GetCurrentCommandList		( RenderSystem* RS );
	FLEXKITAPI PerFrameResources*			GetCurrentFrameResources	( RenderSystem* RS );
	FLEXKITAPI Texture2D					GetRenderTarget				( RenderWindow* RW );

	FLEXKITAPI void BeginPass				( RenderSystem* RS, RenderWindow* Window );
	FLEXKITAPI void EndPass					( ID3D12GraphicsCommandList* CL, RenderSystem* RS, RenderWindow* Window);

	FLEXKITAPI void ClearBackBuffer			( RenderSystem* RS, ID3D12GraphicsCommandList* CL, RenderWindow* RW, float4 ClearColor );
	FLEXKITAPI void ClearDepthBuffer		( RenderSystem* RS, ID3D12GraphicsCommandList* CL, DepthBuffer* RW, float ClearValue = 1.0f, int Stencil = 0 );
	
	FLEXKITAPI void InitiateForwardPass		( RenderSystem* RenderSystem, ForwardPass_DESC* GBdesc, ForwardPass* out );
	FLEXKITAPI void DoForwardPass			( PVS* _PVS, ForwardPass* Pass, RenderSystem* RS, Camera* C, float4& ClearColor, PointLightBuffer* PLB );
	FLEXKITAPI void CleanupForwardPass		( ForwardPass* FP );

	typedef Pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> DHeapPOS;

	inline DHeapPOS IncrementHeapPOS(DHeapPOS POS, size_t Size, size_t INC)
	{
		size_t Offset = Size * INC;
		return {POS.V1.ptr + Offset , POS.V2.ptr + Offset};
	}

	FLEXKITAPI void		ResetDHeap			( RenderSystem* RS);
	FLEXKITAPI DHeapPOS	ReserveDHeap		( RenderSystem* RS, size_t SlotCount = 1);
	FLEXKITAPI DHeapPOS PushCBToDHeap		( RenderSystem* RS, ID3D12Resource* Buffer,			DHeapPOS POS, size_t BufferSize);
	FLEXKITAPI DHeapPOS PushSRVBufferToDHeap( RenderSystem* RS, ID3D12Resource* Buffer,			DHeapPOS POS, size_t ElementCount, size_t Stride, D3D12_BUFFER_SRV_FLAGS Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE);
	FLEXKITAPI DHeapPOS PushTextureToDHeap	( ID3D12GraphicsCommandList* CL, RenderSystem* RS,	DHeapPOS POS, Texture2D tex);
	FLEXKITAPI DHeapPOS PushUAV2DToDHeap	( RenderSystem* RS, Texture2D tex,					DHeapPOS POS, DXGI_FORMAT F = DXGI_FORMAT_R8G8B8A8_UNORM);
	FLEXKITAPI DHeapPOS PushAndSetSRV		( RenderSystem* RS, ID3D12GraphicsCommandList* CL,  ID3D12Resource* Buffer, DHeapPOS POS, size_t ElementCount, size_t Stride, UINT RootIndex, D3D12_BUFFER_SRV_FLAGS F = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE);

	FLEXKITAPI void		SetWindowRect			( ID3D12GraphicsCommandList* CL, RenderWindow* TargetWindow, UINT I = 0u);
	
	
	/************************************************************************************************/


	struct LineSegment
	{
		float3 A, B;
	};

	typedef DynArray<LineSegment> LineSegements;

	struct Line3DPass
	{
		ID3D12PipelineState*	PSO;
		LineSegements			LineSegments;
		FrameBufferedResource	GPUResource;

		Shader PShader;
		Shader VShader;
	};


	FLEXKITAPI void InitiateSegmentPass	(RenderSystem* RS, iAllocator* Mem, Line3DPass* out);
	FLEXKITAPI void CleanUpLineDrawPass	(Line3DPass* out);
	FLEXKITAPI void AddLineSegment		(Line3DPass* Pass, LineSegment in);
	FLEXKITAPI void UploadLineSegments	(RenderSystem* RS, Line3DPass* Pass);
	FLEXKITAPI void Draw3DLineSegments	(RenderSystem* RS, Line3DPass* Pass, Camera* Camera, RenderWindow* TargetWindow);
	

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
		FlexKit::Shader	VertexShader;
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
	FLEXKITAPI void CreateDrawable		( RenderSystem* RS, Drawable* e,		DrawableDesc&		desc );

	FLEXKITAPI bool LoadObjMesh			( RenderSystem* RS, char* File_Loc,	Obj_Desc IN desc, TriMesh ROUT out, StackAllocator RINOUT LevelSpace, StackAllocator RINOUT TempSpace, bool DiscardBuffers );
	FLEXKITAPI void UpdateDrawable		( RenderSystem* RS, SceneNodes* Nodes, const ShaderTable* M, Drawable* E );


	/************************************************************************************************/


	FLEXKITAPI void CleanUpDrawable		( Drawable*	p );
	FLEXKITAPI void CleanUpTriMesh		( TriMesh*	p );


	/************************************************************************************************/


	inline void ClearTriMeshVBVs(TriMesh* Mesh){ for (auto& buffer : Mesh->Buffers) buffer = nullptr; }


	/************************************************************************************************/
}

namespace TextUtilities
{
	using FlexKit::float2;
	using FlexKit::float3;
	using FlexKit::float4;
	using FlexKit::iAllocator;
	using FlexKit::RenderSystem;
	using FlexKit::RenderWindow;
	using FlexKit::StackAllocator;
	using FlexKit::Pair;
	using FlexKit::ShaderResourceBuffer;
	using FlexKit::Texture2D;

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
		char*				 TextBuffer;
		size_t				 BufferSize;
		FlexKit::uint2		 BufferDimensions;
		FlexKit::uint2		 Position;

		ShaderResourceBuffer Buffer;
	
		size_t				 CharacterCount;
		size_t				 Dirty;
	};

	struct TextRender
	{
		ID3D12PipelineState*	PSO;
		ID3D12DescriptorHeap*	Textures;

		FlexKit::Shader	VShader;
		FlexKit::Shader	GShader;
		FlexKit::Shader	PShader;
	};

	struct TextArea_Desc
	{
		float2 POS;	// Screen Space Cord
		float2 WH;
		float2 TextWH;	// Text Size
	};

	struct FontAsset
	{
		size_t	FontSize = 0;;
		bool	Unicode = false;

		FlexKit::uint2			TextSheetDimensions;
		FlexKit::Texture2D		Text;
		FlexKit::uint4			Padding = 0; // Ordering { Top, Left, Bottom, Right }
		FlexKit::iAllocator*	Memory;
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
		char*	FontDir;// Texture Directory
	};

	FLEXKITAPI void DrawTextArea			(TextRender* TR, FontAsset* F, TextArea* TA, iAllocator* Temp, RenderSystem* RS, FlexKit::RenderWindow* Out);
	FLEXKITAPI void ClearText				(TextArea* TA);
	FLEXKITAPI void CleanUpTextArea			(TextArea* TA, iAllocator* BA);
	FLEXKITAPI void PrintText				(TextArea* Area, const char* text);

	FLEXKITAPI TextArea CreateTextObject	(RenderSystem* RS, iAllocator* Mem, TextArea_Desc* D);// Setups a 2D Surface for Drawing Text into
	FLEXKITAPI void		InitiateTextRender	(RenderSystem* RS, TextRender*	out);
	FLEXKITAPI void		CleanUpTextRender	(TextRender* out);
}

#endif