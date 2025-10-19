#pragma once
#include "BuildSettings.hpp"
#include "Containers.hpp"
#include "MathUtilities.hpp"
#include "ResourceHandles.hpp"

#include <expected>

namespace FlexKit
{
	struct iAllocator;
	struct IPipelineInterface;
	struct TriMesh;

	template<size_t ID>
	struct TaggedVoidPtr
	{
		TaggedVoidPtr() = default;
		TaggedVoidPtr(auto IN_ptr) : _ptr{ IN_ptr } {}
		TaggedVoidPtr(nullptr_t) : _ptr{ nullptr } {}
		TaggedVoidPtr(const TaggedVoidPtr&) = default;

		TaggedVoidPtr& operator = (auto IN_ptr) { _ptr = IN_ptr; }
		TaggedVoidPtr& operator = (TaggedVoidPtr&) = default;

		bool operator == (auto rhs) const noexcept { return rhs == _ptr; }
		bool operator != (auto rhs) const noexcept { return rhs != _ptr; }

		operator bool() { return _ptr != nullptr; }

		template<typename TY>
		TY* As() noexcept
		{
		    return reinterpret_cast<TY*>(_ptr);
		}

		template<typename TY>
		TY* As() const noexcept
		{
			return reinterpret_cast<TY*>(_ptr);
		}

		void* _ptr;
	};


	using DeviceResource_ptr		= TaggedVoidPtr<GetCRCGUID(Resource)>; 
	using DeviceFence_ptr			= TaggedVoidPtr<GetCRCGUID(Fence)>; 
	using DeviceHeap_ptr			= TaggedVoidPtr<GetCRCGUID(Heap)>;
	using DevicePipelineState_ptr	= TaggedVoidPtr<GetCRCGUID(PipelineState)>;
	using DeviceRootSignature_ptr	= TaggedVoidPtr<GetCRCGUID(RootSignature)>;


	enum class ELineAliasMode
	{
		ALIASED					= 0,
		ALPHA_ANTIALIASED		= 1,
		QUADRILATERAL_WIDE		= 2,
		QUADRILATERAL_NARROW	= 3
	};

	enum class EFillMode
	{
		WIREFRAME = 2,
		SOLID = 3,
		POINT = 4
	};


	enum class ECullMode
	{
		NONE	= 1,
		FRONT	= 2,
		BACK	= 3
	};


	enum class EComparison : uint32_t
	{
		NONE			= 0,
		NEVER			= 1,
		LESS			= 2,
		EQUAL			= 3,
		LESS_EQUAL		= 4,
		GREATER			= 5,
		NOT_EQUAL		= 6,
		GREATER_EQUAL	= 7,
		ALWAYS			= 8,
		COUNT
	};


	enum class EInputClassification
	{
		PerVertex,
		PerInstance
	};


	enum class EDepthWriteMask : uint32_t
	{
		Zero	= 0,
		All		= 1
	};


	enum class EStencilOP : uint32_t
	{
		KEEP		= 1,
		ZERO		= 2,
		REPLACE		= 3,
		INCR_SAT	= 4,
		DECR_SAT	= 5,
		INVERT		= 6,
		INCR		= 7,
		DECR		= 8
	};


	enum class EBlend : uint32_t
	{
		ZERO				= 1,
		ONE					= 2,
		SRC_COLOR			= 3,
		INV_SRC_COLOR		= 4,
		SRC_ALPHA			= 5,
		INV_SRC_ALPHA		= 6,
		DEST_ALPHA			= 7,
		INV_DEST_ALPHA		= 8,
		DEST_COLOR			= 9,
		INV_DEST_COLOR		= 10,
		SRC_ALPHA_SAT		= 11,
		BLEND_FACTOR		= 14,
		INV_BLEND_FACTOR	= 15,
		SRC1_COLOR			= 16,
		INV_SRC1_COLOR		= 17,
		SRC1_ALPHA			= 18,
		INV_SRC1_ALPHA		= 19,
		ALPHA_FACTOR		= 20,
		INV_ALPHA_FACTOR	= 21
	};


	enum class EBlendOP : uint32_t
	{
		ADD				= 1,
		SUBTRACT		= 2,
		REV_SUBTRACT	= 3,
		MIN				= 4,
		MAX				= 5
	};


	enum class ELogicOP : uint32_t
	{
		CLEAR			= 0,
		SET				= 1,
		COPY			= 2,
		COPY_INVERTED	= 3,
		NOOP			= 4,
		INVERT			= 5,
		AND				= 6,
		NAND			= 7,
		OR				= 8,
		NOR				= 9,
		XOR				= 10,
		EQUIV			= 11,
		AND_REVERSE		= 12,
		AND_INVERTED	= 13,
		OR_REVERSE		= 14,
		OR_INVERTED		= 15
	};


	enum class DeviceFormat
	{
		R8_UINT,
		R16_FLOAT,
		R16_UINT,
		R16_SINT,
		R16_UNORM,
		R16_SNORM,
		R16G16_UINT,
		R32_UINT,
		R32_INT,
		R32G32_UINT,
		R32G32_INT,
		R8G8B8A_UINT,
		R8G8B8A8_UINT,
		R8G8B8A8_SINT,
		R8G8B8A8_UNORM,
		R8G8B8A8_UNORM_SRGB,
		R16G16B16A16_UNORM,
		R16G16B16A16_UINT,
		R8G8_UNORM,
		D24_UNORM_S8_UINT,
		R32_FLOAT,
		D32_FLOAT,

		R10G10B10A2_UNORM,
		R10G10B10A2_UINT,

		R16G16_FLOAT,
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
		BC7_UNORM,
		BC7_SNORM,


		R32G32B32_INT,
		R32G32B32_UINT,
		R32G32B32A32_UINT,

		UNKNOWN
	};


	enum EInputPrimitive
	{
		INPUTPRIMITIVEPOINTLIST			= 1,
		INPUTPRIMITIVELINELIST			= 2,

		INPUTPRIMITIVETRIANGLELIST		= 4,
		INPUTPRIMITIVETRIANGLESTRIP		= 5,

		INPUTPRIMITIVELINELIST_ADJ		= 10,
		INPUTPRIMITIVETRIANGLELIST_ADJ	= 12,
		INPUTPRIMITIVETRIANGLESTRIP_ADJ	= 13,


		INPUTPRIMITIVEPATCH_CP_1		= 33,
		INPUTPRIMITIVEPATCH_CP_2		= 34,
		INPUTPRIMITIVEPATCH_CP_3		= 35,
		INPUTPRIMITIVEPATCH_CP_4		= 36,
		INPUTPRIMITIVEPATCH_CP_5		= 37,
		INPUTPRIMITIVEPATCH_CP_6		= 38,
		INPUTPRIMITIVEPATCH_CP_7		= 39,
		INPUTPRIMITIVEPATCH_CP_8		= 40,
		INPUTPRIMITIVEPATCH_CP_9		= 41,
		INPUTPRIMITIVEPATCH_CP_10		= 42,
		INPUTPRIMITIVEPATCH_CP_11		= 43,
		INPUTPRIMITIVEPATCH_CP_12		= 44,
		INPUTPRIMITIVEPATCH_CP_13		= 45,
		INPUTPRIMITIVEPATCH_CP_14		= 46,
		INPUTPRIMITIVEPATCH_CP_15		= 47,
		INPUTPRIMITIVEPATCH_CP_16		= 48,
		INPUTPRIMITIVEPATCH_CP_17		= 49,
		INPUTPRIMITIVEPATCH_CP_18		= 50,
		INPUTPRIMITIVEPATCH_CP_19		= 51,
		INPUTPRIMITIVEPATCH_CP_20		= 52,
		INPUTPRIMITIVEPATCH_CP_21		= 53,
		INPUTPRIMITIVEPATCH_CP_22		= 54,
		INPUTPRIMITIVEPATCH_CP_23		= 55,
		INPUTPRIMITIVEPATCH_CP_24		= 56,
		INPUTPRIMITIVEPATCH_CP_25		= 57,
		INPUTPRIMITIVEPATCH_CP_26		= 58,
		INPUTPRIMITIVEPATCH_CP_27		= 59,
		INPUTPRIMITIVEPATCH_CP_28		= 60,
		INPUTPRIMITIVEPATCH_CP_29		= 61,
		INPUTPRIMITIVEPATCH_CP_30		= 62,
		INPUTPRIMITIVEPATCH_CP_31		= 63,
		INPUTPRIMITIVEPATCH_CP_32		= 64,
	};


	enum PIPELINE : unsigned char
	{
		PIPELINE_DEST_NONE		= 0x00,
		PIPELINE_DEST_IA		= 0x01,
		PIPELINE_DEST_HS		= 0x02,
		PIPELINE_DEST_GS		= 0x04,
		PIPELINE_DEST_VS		= 0x08,
		PIPELINE_DEST_PS		= 0x10,
		PIPELINE_DEST_CS		= 0x20,
		PIPELINE_DEST_OM		= 0x30,
		PIPELINE_DEST_DS		= 0x40,
		PIPELINE_DEST_AS		= 0x50,
		PIPELINE_DEST_MS		= 0x60,
		PIPELINE_DEST_RT		= 0x70,
		PIPELINE_DEST_AS_BUILD	= 0x80,

		PIPELINE_DEST_ALL = 0xFF
	};

	enum class ETopology
	{
		EIT_LINE			= 2,
		EIT_TRIANGLE		= 3,
		EIT_POINT			= 1,
		EIT_PATCH			= 4,
	};


	enum class EColorWriteEnable : uint8_t
	{
		RED		= 1,
		GREEN	= 2,
		BLUE	= 4,
		ALPHA	= 8,
		ALL		= (RED | GREEN | BLUE | ALPHA)
	};


	struct DepthStencilOP
	{
		EStencilOP	stencilFailOp		= EStencilOP::KEEP;
		EStencilOP	stencilDepthFailOp	= EStencilOP::KEEP;
		EStencilOP	stencilPassOp		= EStencilOP::KEEP;
		EComparison	stencilFunc			= EComparison::ALWAYS;
	};


	enum class DescHeapEntryType : uint32_t
	{
		ConstantBuffer,
		ShaderResource,			// DX12
		ShaderResourceImage,	// VK
		ShaderResourceBuffer,	// VK
		UAV,					// DX12
		UAVBuffer,				// VK
		UAVImage,				// VK
		HeapError
	};


	enum class RootSignatureEntryType : uint32_t
	{
		DescriptorHeap,
		ConstantBuffer,
		StructuredBuffer,
		UnorderedAcess,
		UINT,
		Error
	};


	enum class SHADER_TYPE : uint8_t
	{
		Compute,
		Domain,
		Geometry,
		Hull,
		Pixel,
		Vertex,
		Unknown
	};

	
	enum class PredicateOp
	{
		EqualZero,
		NotEqualZero
	};


	enum DeviceAccessState : uint16_t
	{
		DASReadFlag					= 0x0001,
		DASWriteFlag				= 0x0002,

		DASRetired					= 0x1000,
		DASPresent					= 0x1005,
		DASRenderTarget				= 0x0006,
		DASPixelShaderResource		= 0x0009,
		DASUAV						= 0x000A,
		DASSTREAMOUT				= 0x000B,
		DASVERTEXBUFFER				= 0x000C,
		DASCONSTANTBUFFER			= 0x000C,
		DASDEPTHBUFFER				= 0x0030,
		DASDEPTHBUFFERREAD			= 0x0030 | DASReadFlag,
		DASDEPTHBUFFERWRITE			= 0x0030 | DASWriteFlag,

		DASACCELERATIONSTRUCTURE_WRITE	= 0x00D0 | DASWriteFlag,
		DASACCELERATIONSTRUCTURE_READ	= 0x00D0 | DASReadFlag,

		DASPREDICATE				= 0x0010 | DASReadFlag,
		DASINDIRECTARGS				= 0x0020 | DASReadFlag,

		DASNonPixelShaderResource	= 0x0040 | DASWriteFlag,

		DASCopyDest					= 0x0050 | DASWriteFlag,
		DASCopySrc					= 0x0050 | DASReadFlag,

		DASINDEXBUFFER				= 0x0060 | DASReadFlag,

		DASGenericRead				= 0x0070 | DASReadFlag,
		DASCommon					= 0x0080 | DASReadFlag,

		DASShadingRateSrc			= 0x00A0 | DASReadFlag,
		DASShadingRateDst			= 0x00A0 | DASWriteFlag,

		DASDecodeWrite				= 0x00B0 | DASWriteFlag,

		DASProcessRead				= 0x00E0 | DASReadFlag,
		DASProcessWrite				= 0x00E0 | DASWriteFlag,

		DASEncodeRead				= 0x0100 | DASReadFlag,
		DASEncodeWrite				= 0x0100 | DASWriteFlag,

		DASResolveRead				= 0x0200 | DASReadFlag,
		DASResolveWrite				= 0x0200 | DASWriteFlag,

		DASNOACCESS					= 0xF000,
		DASUNKNOWN					= 0x0300,
		DASERROR					= 0xFFFF,
	};



	enum DeviceSyncPoint : uint32_t
	{
		Sync_None,

		Sync_IA					= 0x01 << 0,
		Sync_VertexShader		= 0x01 << 1,
		Sync_HullShader			= 0x01 << 2,
		Sync_DomainShader		= 0x01 << 3,
		Sync_GeometryShader		= 0x01 << 4,
		Sync_PixelShader		= 0x01 << 5,
		Sync_DepthStencil		= 0x01 << 6,
		Sync_RenderTarget		= 0x01 << 7,
		Sync_Raytracing			= 0x01 << 8,
		Sync_Copy				= 0x01 << 9,
		Sync_Resolve			= 0x01 << 10,
		Sync_ExecuteIndirect	= 0x01 << 11,
		Sync_Predication		= 0x01 << 12,
		Sync_Compute			= 0x01 << 13,

		Sync_EmitRaytracingAccelerationStructurePostBuildInfo	= 0x01 << 14,
		Sync_BuildRaytracingAccelerationStructure				= 0x01 << 15,
		Sync_CopyRaytracingAccelerationStructure				= 0x01 << 16,

	    Sync_VideoDecode										= 0x01 << 17,
		Sync_VideoProcess										= 0x01 << 18,
		Sync_VideoEncode										= 0x01 << 19,

		Sync_ClearUAV											= 0x01 << 20,

		Sync_Mesh			= 0x01 << 21,
		Sync_Amplification	= 0x01 << 22,

		Sync_Draw				= Sync_IA | Sync_VertexShader | Sync_HullShader  | Sync_DomainShader | Sync_PixelShader | Sync_DepthStencil | Sync_RenderTarget,
	    Sync_All_Shading		= Sync_VertexShader | Sync_HullShader | Sync_DomainShader | Sync_PixelShader | Sync_Compute,
		Sync_NonPixelShading	= Sync_VertexShader | Sync_HullShader | Sync_DomainShader | Sync_Compute,

		Sync_Unknown,
		Sync_All	= 0xffffffff,

	};

	
	enum class DeviceLayout : uint32_t
	{
		Common,
		Present,
		GenericRead,
		RenderTarget,
		UnorderedAccess,
		DepthStencilWrite,
		DepthStencilRead,
		ShaderResource,
		CopySrc,
		CopyDst,
		ResolveSrc,
		ResolveDst,
		ShadingRateSrc,
		VideoDecodeRead,
		DecodeWrite,
		ProcessRead,
		ProcessWrite,
		EncodeRead,
		EncodeWrite,
		DirectQueueCommon,
		DirectQueueGenericRead,
		DirectQueueUnorderedAccess,
		DirectQueueShaderResource,
		DirectQueueCopySrc,
		DirectQueueCopyDst,
		ComputeQueueCommon,
		ComputeQueueGenericRead,
		ComputeQueueUnorderedAccess,
		ComputeQueueShaderResource,
		ComputeQueueCopySrc,
		ComputeQueueCopyDst,
		VideoQueueCommon,
		Undefined,

		Unknown = 0xffffffff,
	};


	enum class BufferResourceFlags : uint8_t
	{
		UAV_Resource,
		Byte_Buffer,
		TripleBuffer,
	};


	enum class BufferDimension : uint8_t
	{
		Resource_1D,
		Resource_2D,
		Resource_3D,
		ByteBuffer,
	};

	enum struct ResourceHeapTier
	{
		HeapTier1,
		HeapTier2,
	};


	namespace DeviceHeapFlags
	{
		enum DeviceHeapFlagEnums : uint32_t
		{
			NONE			= 0,
			RenderTarget	= 1,
			UAVBuffer		= 2,
			UAVTextures		= 4,
			ALL				= 0xff
		};
	}


	enum class ResourceAllocationType : uint8_t
	{
		Committed,
		Placed,
		Tiled
	};


	enum class ResourceType : uint8_t
	{
		RenderTarget,
		DepthTarget,
		UnorderedAccess,
		UnorderedAccessRenderTarget,
		ShaderResource,
		RayTracingStructure,
	};


	namespace ResourceFlags
	{
		enum ResourceFlags
		{
			NONE			= 0x00,
			INUSE			= 0x01,
			RenderTarget	= 0x02,
			SwapChain		= 0x04,
			DepthBuffer		= 0x08,
		};
	}

	enum class TextureDimension : uint8_t
	{
		Buffer,
		Texture1D,
		Texture2D,
		Texture2DArray,
		Texture3D,
		TextureCubeMap,
		Unknown,
	};


	enum class QueryType : uint8_t
	{
		OcclusionQuery,
		BinaryOcclusionQuery,
		PipelineStats,
		TimeStats,
	};

	
	enum class IndirectLayoutEntryType : uint8_t
	{
		DrawCall,
		DrawIndexedCall,
		DispatchCall,
		DispatchMesh,
		DispatchRays,
		UpdateVBBindings,
		RootDescriptorUINT,
		UNKNOWN,
	};


	enum class ReserveErrors
	{
		Success,
		OutOfSpace,
		Unknown
	};


	enum class TileMapState
	{
		InUse,
		Null,
		Updated
	};


	enum class VERTEXBUFFER_FORMAT
	{
		UNKNOWN			= -1,
		R8				= 1,
		R8G8B8			= 3,
		R8G8B8A8		= 8,
		R16				= 2,
		R16G16			= 4,
		R16G16B16		= 6,
		R16G16B16A16	= 8,
		R32				= 4,
		R32G32			= 8,
		R32G32B32		= 12,
		R32G32B32A32	= 16,
		MATRIX			= 64,
		COMBINED		= 32
	};


	/************************************************************************************************/


	enum class VERTEXBUFFER_TYPE
	{
		COLOR,
		NORMAL,
		TANGENT,
		UV,
		POSITION,
		USERTYPE,
		USERTYPE2,
		USERTYPE3,
		USERTYPE4,
		COMBINED,
		PACKED,
		PACKEDANIMATION,
		INDEX,
		ANIMATION1,
		ANIMATION2,
		ANIMATION3,
		ANIMATION4,
		MORPHTARGETPOS,
		MORPHTARGETNORMAL,
		MORPHTARGETTANGENT,

		UNKNOWN
	};


	enum class ROOTLIBRARYSIG : uint32_t
	{
		RS2UAVs4SRVs4CBs,
		RS6CBVs4SRVs,
		RS4CBVs_SO,
		ShadingRTSig,
		RSDefault,
		ComputeSignature,
		ClearBuffer,
		COUNT
	};


	/************************************************************************************************/


	struct TileID_t
	{
		union {
			uint32_t bytes;

			struct
			{
				unsigned int ytile		: 12;
				unsigned int xtile		: 12;
				unsigned int mipLevel	: 7;
				unsigned int packed		: 1;
			}   segments;
		};

		uint32_t GetTileX() const
		{
			return (bytes >> 12) & 0xfff;
		}

		uint32_t GetTileY() const
		{
			return bytes & 0xfff;
		}

		uint32_t GetMipLevel() const
		{
			static_assert(sizeof(segments) == sizeof(uint32_t));
			return segments.mipLevel;
		}

		bool valid() const
		{
			return segments.xtile < 128 && segments.ytile < 128 && segments.mipLevel <= 14;
		}

		bool packed() const
		{
		   return (bytes >> 31) & 0x01;
		}

		bool operator == (const TileID_t& rhs) const
		{
			return bytes == rhs.bytes;
		}

		operator uint3 () const
		{
			return {
				GetTileX(),
				GetTileY(),
				(uint32_t)GetMipLevel()
			};
		}

		operator uint32_t() const
		{
			return bytes;
		}
	};


	inline TileID_t CreatePackedID()
	{
		TileID_t id = { 0 };
		id.segments.packed = 1;
		return id;
	}


	struct TileMapping
	{
		TileID_t			tileID;
		DeviceHeapHandle	heap;
		TileMapState		state;
		uint32_t			heapOffset;

		uint64_t sortingID() const
		{
			return (uint64_t)heap.to_uint() << 32 | tileID;
		};
	};

	inline TileID_t CreateTileID(uint32_t x, uint32_t y, uint32_t mipLevel)
	{
		return TileID_t{ (mipLevel & 0x7f) << 24 | (x & 0x0f) << 12 | y };
	}


	using TileMapList = Vector<TileMapping>;


	/************************************************************************************************/


	enum SubmitCopyFlags
	{
		SYNC_Graphics   = 0x01,
		SYNC_Compute    = 0x02
	};

	struct PipelineStateLibraryDesc
	{

	};


	struct PipelineStateLibrary
	{
	};


	/************************************************************************************************/


	struct AccelerationStructureDesc
	{
	};


	struct BLAS_PreBuildInfo
	{
		size_t BLAS_byteSize;
		size_t scratchPad_byteSize;
		size_t update_byteSize;
	};


	/************************************************************************************************/


	struct LoadPipelineStateRes
	{
		DevicePipelineState_ptr		pipelineState;
		const IPipelineInterface*	pipelineInterface;
	};

	using LOADSTATE_FN = FlexKit::TypeErasedCallable<LoadPipelineStateRes(struct IRenderSystem&, iAllocator&), 32>;

	struct AvailableFeatures
	{
		enum struct Raytracing
		{
			RT_FeatureLevel_NOTAVAILABLE,
			RT_FeatureLevel_1,
			RT_FeatureLevel_1_1,
		} RT_Level = Raytracing::RT_FeatureLevel_NOTAVAILABLE;

		enum ConservativeRasterization
		{
			ConservativeRast_AVAILABLE,
			ConservativeRast_NOTAVAILABLE,
		} conservativeRast = ConservativeRast_NOTAVAILABLE;

		enum struct HLSLCompiler
		{
			HLSL_CompilerEnabled,
			HLSL_CompilerDisabled
		} Compiler = HLSLCompiler::HLSL_CompilerDisabled;

		enum WorkGraphs
		{
			WorkGraphs_NOTAVAILABLE,
			WorkGraphs_AVAILABLE,
		} workGraph = WorkGraphs_AVAILABLE;

		enum IndirectLevel
		{
			IndirectLevel_1,
			IndirectLevel_1_1,
		} indirectLevel = IndirectLevel_1;

		ResourceHeapTier resourceHeapTier;
	};

	
	struct ShaderOptions
	{
		bool enable16BitTypes	= false;
		bool hlsl2021			= false;
		bool enableDebug		= false;
		bool loadRootSignature	= false;
	};

	enum class BarrierType
	{
		Global,
		Texture,
		Buffer,
		Unknown,
	};


	struct BarrierSubResourceRange
	{
	};


	struct Barrier
	{
		Barrier() {}

		DeviceAccessState accessBefore	= DeviceAccessState::DASUNKNOWN;
		DeviceAccessState accessAfter	= DeviceAccessState::DASUNKNOWN;

		DeviceSyncPoint	src	= DeviceSyncPoint::Sync_Unknown;
		DeviceSyncPoint	dst	= DeviceSyncPoint::Sync_Unknown;

		BarrierType type = BarrierType::Unknown;

		ResourceHandle resource;

		union
		{
			struct
			{
				DeviceLayout layoutBefore;
				DeviceLayout layoutAfter;

				uint32_t				flags;
				BarrierSubResourceRange	range;
			} texture;

			struct
			{
				uint64_t rangeBegin	= 0;
				uint64_t rangeEnd	= UINT64_MAX;
			} buffer;
		};

		uint32_t subResource = -1;
	};


	struct CopyBarrier
	{
		ResourceHandle			handle;

		union {
			DeviceResource_ptr	_ptr;
		};

		enum class ResourceType
		{
			PTR,
			HNDL
		} type;

		DeviceAccessState beforeState;
		DeviceAccessState afterState;
	};


	enum class DeviceVendor
	{
		AMD,
		NVIDIA,
		INTEL,
		UNKNOWN
	};


	/************************************************************************************************/


	struct Shader
	{
		Shader() = default;


		Shader(char* IN_buffer, size_t IN_bufferSize, iAllocator* IN_allocator) :
			buffer		{ (char*)IN_allocator->malloc(IN_bufferSize) },
			bufferSize	{ IN_bufferSize },
			allocator	{ IN_allocator	}
		{
			memcpy(buffer, IN_buffer, bufferSize);
		}


		Shader(const Shader& rhs)
		{
			buffer = (char*)rhs.allocator->malloc(rhs.bufferSize);

			memcpy(buffer, rhs.buffer, rhs.bufferSize);
			bufferSize = rhs.bufferSize;
		}


		Shader(Shader&& rhs)
		{
			allocator	= rhs.allocator;
			buffer		= rhs.buffer;
			bufferSize	= rhs.bufferSize;

			rhs.allocator	= nullptr;
			rhs.buffer		= nullptr;
			rhs.bufferSize	= 0;
		}


		~Shader()
		{
			if (allocator)
			{
				allocator->free(buffer);

				allocator   = nullptr;
				buffer      = nullptr;
				bufferSize = 0;
			}

		}


		operator bool() const { return ( buffer != nullptr ); }


		Shader& operator = (const Shader& rhs)
		{
			buffer = (char*)rhs.allocator->malloc(rhs.bufferSize);

			memcpy(buffer, rhs.buffer, rhs.bufferSize);
			bufferSize = rhs.bufferSize;

			return *this;
		}


		Shader& operator = (Shader&& rhs)
		{
			buffer        = rhs.buffer;
			bufferSize    = rhs.bufferSize;
			allocator     = rhs.allocator;

			rhs.buffer     = nullptr;
			rhs.bufferSize = 0;
			rhs.allocator  = nullptr;

			return *this;
		}


		char*   buffer = nullptr;
		size_t  bufferSize = 0;

		iAllocator* allocator = nullptr;
	};


	/************************************************************************************************/


	struct DepthStencilView_Options
	{
		size_t ArraySliceOffset = 0;
		size_t MipOffset = 0;

		ResourceHandle depthStencil;

		size_t arraySize = -1;
	};


	struct DispatchDesc
	{
		DeviceAddressRangeStride     callableShaderTable;
		DeviceAddressRangeStride     hitGroupTable;
		DeviceAddressRangeStride     missTable;
		DeviceAddressRange           rayGenerationRecord;
	};


	struct Graphics_Desc
	{
		iAllocator*		Memory;
		iAllocator*		TempMemory;
		uint32_t		SlaveThreadCount;
		bool			Fullscreen						= false;
		bool			DX_DebugMode					= false;
		bool			DX_GPUvalidation				= false;
		bool			DX_SynchronizedQueueValidation	= false;
	};


	struct ShaderDesc
	{
		char			ID[128];
		char			entry[128];
		char			IncludeFile[128];
		char			shaderVersion[16];
		SHADER_TYPE		ShaderType;
	};


	struct InputElement
	{
		const char*				name				= 0;
		uint8_t					index				= 0;
		DeviceFormat			format				= DeviceFormat::UNKNOWN;
		uint16_t				slot				= 0;
		uint16_t				alignedByteOffset	= 0;
		EInputClassification	inputSlotClass		= EInputClassification::PerVertex;
		uint16_t				instanceStepRate	= 0;
	};


	struct InputLayoutState
	{
		InputElement	inputs[16];
		uint8_t			count = 0;
	};


	struct RasterizerState
	{
		EFillMode		fill						= EFillMode::SOLID;
		ECullMode		CullMode					= ECullMode::BACK;
		bool			frontCounterClockWise		= false;
		bool			depthClipEnable				= true;
		bool			multisampleEnable			= false;
		bool			conservativeRasterEnable	= false;
		ELineAliasMode	antialiasedLineMode			= ELineAliasMode::ALIASED;
		float			depthBias					= 0.0f;
		float			depthBiasClamp				= 0.0f;
		float			slopeScaledDepthBias		= 0.0f;
		uint32_t		forcedSampleCount			= 0;

		static RasterizerState Default() { return {}; }
	};


	struct DepthStencilState
	{
		bool				depthEnable = false;
		EDepthWriteMask		depthWriteMask = EDepthWriteMask::All;
		EComparison			depthFunc = EComparison::LESS;
		bool				stencilEnable = false;
		uint8_t				stencilReadMask = 0xff;
		uint8_t				stencilWriteMask = 0xff;

		DepthStencilOP	frontFace = {};
		DepthStencilOP	backFace = {};
	};


	struct RenderTargetStateDesc
	{
		uint32_t			blendEnable				= false;
		uint32_t			logicOpEnable			= false;
		EBlend				srcBlend				= EBlend::ONE;
		EBlend				dstBlend				= EBlend::ZERO;
		EBlendOP			blendOp					= EBlendOP::ADD;
		EBlend				srcBlendAlpha			= EBlend::ONE;
		EBlend				dstBlendAlpha			= EBlend::ZERO;
		EBlendOP			blendOpAlpha			= EBlendOP::ADD;
		ELogicOP			logicOp					= ELogicOP::NOOP;
		EColorWriteEnable	renderTargetWriteMask	= EColorWriteEnable::ALL;
	};


	struct ConstantBuffer_desc
	{
		bool	Structured;
		size_t  StructureSize;

		void*	pInital;
		size_t	InitialSize;
	};


	struct RenderTargetDesc
	{
		uint32_t		height;
		uint32_t		width;
		uint32_t		mip_Levels;
		std::byte*		usr;
	};


	/************************************************************************************************/


	struct VBView
	{
		DevicePointer	buffer;
		uint32_t		size;
		uint32_t		stride;
	};


	struct VertexBufferEntry
	{
		VertexBufferHandle	VertexBuffer	= InvalidHandle;
		uint32_t			Stride			= 0;
		uint32_t			Offset			= 0;
	};

	typedef static_vector<VertexBufferEntry, 16>	VertexBufferList;
	typedef static_vector<ResourceHandle, 16>       RenderTargetList;


	struct VertexBufferResource
	{
		ResourceHandle		resource = InvalidHandle;
		uint32_t			stride = 0;
		uint32_t			offset = 0;
	};


	/************************************************************************************************/


	class IndirectDrawDescription
	{
	public:
		struct Constant
		{
			uint32_t rootParameterIdx;
			uint32_t destinationOffset;
			uint32_t numValues;
		};

		IndirectDrawDescription(IndirectLayoutEntryType IN_type = IndirectLayoutEntryType::UNKNOWN) : type{ IN_type } {}
		IndirectDrawDescription(Constant IN_constant) : type{ IndirectLayoutEntryType::RootDescriptorUINT }, description{ IN_constant } {}

		IndirectLayoutEntryType type;

		union
		{
			Constant constantValue;
		} description = {};
	};

	/************************************************************************************************/


	struct BlendState
	{
		bool					alphaToCoverageEnable	= false;
		bool					independentBlendEnable	= false;
		RenderTargetStateDesc	renderTarget[8];

		static BlendState Default()
		{
			FK_ASSERT(false);
		    return {};
		}

		static BlendState Blend()
		{
			return
			    BlendState{
				    .alphaToCoverageEnable		= false,
				    .independentBlendEnable		= false,
				    .renderTarget = {
					    RenderTargetStateDesc{
						    .blendEnable	= true,
						    .srcBlend		= EBlend::SRC_ALPHA,
						    .dstBlend		= EBlend::INV_SRC_ALPHA,
						    .blendOp		= EBlendOP::ADD,
						    .srcBlendAlpha	= EBlend::ONE,
						    .dstBlendAlpha	= EBlend::ONE
					    }
				    }};
		}
	};


	struct RenderTargetState
	{
		uint8_t			targetCount = 0;
		DeviceFormat	targetFormats[16];
	};


	struct WorkGraph_Desc
	{
		const char* programName = nullptr;
		uint32_t	nodeCount	= 0;
		uint32_t	flags		= 0;
	};


	struct Rect
	{
		uint32_t left;
		uint32_t top;
		uint32_t right;
		uint32_t bottom;
	};

	struct Viewport
	{
		float  X, Y, Height, Width;
		float  Min, Max;
	};


	using ReadBackEventHandler = TypeErasedCallable<void(ReadBackResourceHandle), 64>;

	struct MappedReadBackBuffer
	{
		void*					buffer;
		const size_t			bufferSize;
		ReadBackResourceHandle	handle;
	};


	struct PackedResourceTileInfo
	{
		uint32_t startingLevel;
		uint32_t endingLevel;
		uint32_t startingTileIndex;
	};

	
	struct UploadReservation
	{
		DeviceResource_ptr		resource		= nullptr;
		size_t					size			= 0;
		size_t					offset			= 0;
		char*					buffer			= 0;

		operator bool() const { return size > 0; }
	};


	struct SubAllocation
	{
		char*	data;
		size_t	offsetBegin;
		size_t	reserveSize;
	};


	/************************************************************************************************/


	struct DepthStencilValue
	{
		float	depth;
		uint8_t stencil;
	};


	struct ClearValue
	{
		DeviceFormat format;
		union
		{
			float4				color;
			DepthStencilValue	depthStencilValue;
		};
	};


    struct SubResourceUpload_Desc
	{
		struct TextureBuffer* buffers;

		size_t	        subResourceStart;
		size_t	        subResourceCount;

		DeviceFormat       format;
	};

	struct GPUResourceDesc
	{
		ResourceType			type;
		TextureDimension		Dimensions		= TextureDimension::Texture2D;
		ResourceAllocationType	allocationType	= ResourceAllocationType::Committed;
		DeviceFormat			format;
		DeviceLayout			initialLayout	= DeviceLayout::Common;

		// Dimensions
		uint2					WH;
		uint8_t					arraySize		= 1;
		uint8_t					bufferCount		= 1;
		uint8_t					mipLevels		= 1;

		bool					swapChain		= false;
		bool					PreCreated		= false;
		bool					denyShaderUsage = false;

		std::optional<ClearValue>	clearValue;

		union
		{
			struct
			{
				void*				_ptr;
				void**				array_ptr;
				DeviceResource_ptr*	resources;
				std::byte*			initial;
			};
		};

		struct
		{
			size_t				offset;
			DeviceHeapHandle	heap;
			DeviceAccessState	initialState;
			DeviceHeap_ptr		customHeap;
		} placed;


		size_t	byteSize;
		bool	rayTraceStructure = false;


		static GPUResourceDesc RenderTarget(uint2 IN_WH, DeviceFormat IN_format, const ResourceAllocationType allocationType = ResourceAllocationType::Committed)
		{
			return GPUResourceDesc{
				.type			= ResourceType::RenderTarget,
				.Dimensions		= TextureDimension::Texture2D,
				.allocationType = allocationType,
				.format			= IN_format,
				.WH				= IN_WH,
				.bufferCount	= 3,
				.mipLevels		= 1,

				.clearValue		= { ClearValue{
										.format = IN_format,
										.color{ 0.0f, 0.0f, 0.0f, 0.0f }}}
			};
		}


		static GPUResourceDesc DepthTarget(uint2 IN_WH, DeviceFormat IN_format, const uint8_t arraySize = 1, const ResourceAllocationType allocationType = ResourceAllocationType::Committed)
		{
			return GPUResourceDesc{
				.type			= ResourceType::DepthTarget,
				.Dimensions		= TextureDimension::Texture2D,
				.allocationType = allocationType,
				.format			= IN_format,
				.initialLayout	= DeviceLayout::DepthStencilWrite,

				.WH				= IN_WH,
				.arraySize		= arraySize,
				.bufferCount	= 3,
				.mipLevels		= 1,

				.clearValue		= { ClearValue{
										.format		= IN_format,
										.depthStencilValue{ .depth = 1.0f, .stencil = 0xff }}}
			};
		}


		static GPUResourceDesc ShaderResource(uint2 IN_WH, DeviceFormat IN_format, uint8_t mipCount = 1, size_t arraySize = 1, const ResourceAllocationType allocationType = ResourceAllocationType::Committed)
		{
			return GPUResourceDesc{
				.type			= ResourceType::ShaderResource,
				.Dimensions		= TextureDimension::Texture2D,
				.allocationType = allocationType,
				.format			= IN_format,

				.WH				= IN_WH,
				.arraySize		= (uint8_t)arraySize,
				.bufferCount	= 1,
				.mipLevels		= mipCount,
			};
		}

		static GPUResourceDesc ShaderResource3D(uint3 IN_XYZ, DeviceFormat IN_format, uint8_t mipCount = 1, const ResourceAllocationType allocationType = ResourceAllocationType::Committed)
		{
			return GPUResourceDesc{
				.type			= ResourceType::ShaderResource,
				.Dimensions		= TextureDimension::Texture3D,
				.allocationType = allocationType,
				.format			= IN_format,

				.WH				= { IN_XYZ[0], IN_XYZ[1] },
				.arraySize		= (uint8_t)IN_XYZ[2], 
				.bufferCount	= 1,
				.mipLevels		= mipCount,
			};
		}


		static GPUResourceDesc StructuredResource(const uint32_t bufferSize)
		{
			return GPUResourceDesc{
				.type			= ResourceType::ShaderResource,
				.Dimensions		= TextureDimension::Buffer,
				.allocationType = ResourceAllocationType::Committed,
				.format			= DeviceFormat::UNKNOWN,
				.initialLayout	= DeviceLayout::Undefined,

				.WH				= { bufferSize, 1 },
				.arraySize		= 1,
				.bufferCount	= 1,
				.mipLevels		= 1,
			};	
		}


		static GPUResourceDesc RayTracingStructure(const size_t bufferSize)
		{
			GPUResourceDesc desc{
				.type			= ResourceType::RayTracingStructure,
				.Dimensions		= TextureDimension::Buffer, // dimensions
				.allocationType = ResourceAllocationType::Committed,
				.format			= DeviceFormat::UNKNOWN,
				.initialLayout	= DeviceLayout::Undefined,

				.WH				= uint2{ (uint32_t)bufferSize, 1 },
				.bufferCount	= 1,
				.mipLevels		= 1,
			};

			desc.rayTraceStructure = true;

			return desc;
		}

		static GPUResourceDesc UAVResource(const size_t bufferSize, DeviceFormat IN_format = DeviceFormat::UNKNOWN, bool renderTarget = false, uint32_t bufferCount = 1)
		{
			return GPUResourceDesc{
				.type			= ResourceType::UnorderedAccess,
				.Dimensions		= TextureDimension::Buffer,
				.allocationType = ResourceAllocationType::Committed,
				.format			= IN_format,
				.initialLayout	= DeviceLayout::Undefined,


				.WH				= uint2{ (uint32_t)bufferSize, 1 },
				.bufferCount	= (uint8_t)bufferCount,
				.mipLevels		= 1,
			};	
		}

		static GPUResourceDesc UAVResource2(const size_t bufferSize, DeviceFormat IN_format = DeviceFormat::R32_UINT, bool renderTarget = false, uint32_t bufferCount = 1)
		{
			return GPUResourceDesc{
				.type			= ResourceType::UnorderedAccess,
				.Dimensions		= TextureDimension::Texture1D,
				.allocationType = ResourceAllocationType::Committed,
				.format			= IN_format,

				.WH				= uint2{ (uint32_t)bufferSize, 1 },
				.bufferCount	= (uint8_t)bufferCount,
				.mipLevels		= 1,
			};
		}


		static GPUResourceDesc UAVTexture(const uint2 IN_WH, const DeviceFormat IN_format, bool renderTarget = false, uint32_t mipCount = 1, uint32_t bufferCount = 1)
		{
			return GPUResourceDesc{
				.type			= renderTarget ? ResourceType::UnorderedAccessRenderTarget : ResourceType::UnorderedAccess,
				.Dimensions		= TextureDimension::Texture2D,
				.allocationType = ResourceAllocationType::Committed,
				.format			= IN_format,

				.WH				= IN_WH,
				.bufferCount	= (uint8_t)bufferCount,
				.mipLevels		= (uint8_t)mipCount,

				.clearValue		= renderTarget ? std::optional<ClearValue>{ ClearValue{
									.format = IN_format,
									.color{ 0.0f, 0.0f, 0.0f, 0.0f }} }
								:	std::optional<ClearValue>{},
			};
		}


		static GPUResourceDesc UAVTexture3D(const uint3 IN_XYZ, const DeviceFormat IN_format, bool renderTarget = false, uint32_t mipCount = 1, uint32_t bufferCount = 1)
		{
			return {
				.type			= ResourceType::UnorderedAccess,
				.Dimensions		= TextureDimension::Texture3D,
				.allocationType = ResourceAllocationType::Committed,
				.format			= IN_format,

				.WH				= { IN_XYZ[0], IN_XYZ[1] },
				.arraySize		= (uint8_t)IN_XYZ[2],
				.bufferCount	= (uint8_t)bufferCount,
				.mipLevels		= (uint8_t)mipCount,
			};
		}

		static GPUResourceDesc SwapChain(uint2 WH, DeviceFormat format, DeviceResource_ptr* sources, const uint8_t resourceCount)
		{
			GPUResourceDesc desc = {
				.type			= ResourceType::UnorderedAccess,
				.Dimensions		= TextureDimension::Texture2D,
				.allocationType = ResourceAllocationType::Committed,
				.format			= format,
				.initialLayout	= DeviceLayout::Present,


				.WH				= WH,
				.bufferCount	= resourceCount,
				.mipLevels		= 1,

				.swapChain		= true,
				.PreCreated		= true,
			};

			desc.resources = sources;

			return desc;
		}


		static GPUResourceDesc DDS(uint2 WH, DeviceFormat format, uint8_t mipCount, TextureDimension dimensions, const ResourceAllocationType allocationType = ResourceAllocationType::Committed)
		{
			 return {
				.type			= ResourceType::ShaderResource,
				.Dimensions		= dimensions,
				.allocationType = ResourceAllocationType::Committed,
				.format			= format,

				.WH				= WH,
				.mipLevels		= mipCount,
			};
		}


		static GPUResourceDesc BuildFromMemory(const GPUResourceDesc& format, DeviceResource_ptr* sources, const uint32_t resourceCount)
		{
			GPUResourceDesc desc	= format;
			desc.PreCreated			= true;
			desc.resources			= sources;
			desc.bufferCount		= resourceCount;

			return desc;
		}


		static GPUResourceDesc CubeMap(uint2 WH, DeviceFormat format, uint8_t mipCount, bool renderTarget, const ResourceAllocationType allocationType = ResourceAllocationType::Committed)
		{
			 return {
				.type			= renderTarget ? ResourceType::RenderTarget : ResourceType::ShaderResource,
				.Dimensions		= TextureDimension::TextureCubeMap,
				.allocationType = allocationType,
				.format			= format,

				.WH				= WH,
				.arraySize		= 6,
				.mipLevels		= mipCount,
			};
		}

		static GPUResourceDesc CubeMapUAV(uint2 WH, DeviceFormat format, uint8_t mipCount, bool renderTarget = false, const ResourceAllocationType allocationType = ResourceAllocationType::Committed)
		{
			 return {
				.type			= renderTarget ? ResourceType::UnorderedAccessRenderTarget : ResourceType::UnorderedAccess,
				.Dimensions		= TextureDimension::TextureCubeMap,
				.allocationType = allocationType,
				.format			= format,

				.WH				= WH,
				.arraySize		= 6,
				.mipLevels		= mipCount,
				.clearValue		= ClearValue{
									.format = format,
									.color = { 0.0f, 0.0f, 0.0f, 0.0f } }
			};
		}
	};


	/************************************************************************************************/


	struct GPUHeapAllocation
	{
		size_t offset = 0;
		size_t size = 0;

		ResourceHandle overlap = InvalidHandle;

		operator bool() { return size > 0; }
	};

	struct AcquireResult
	{
		ResourceHandle resource;
		ResourceHandle overlap;
	};

	struct AcquireDeferredRes
	{
		ResourceHandle      resource = InvalidHandle;
		ResourceHandle      overlap = InvalidHandle;

		size_t              offset = 0;
		DeviceHeapHandle    heap = InvalidHandle;
	};

	struct PoolAllocatorInterface
	{
		virtual ~PoolAllocatorInterface() {};

		virtual AcquireResult		Acquire(GPUResourceDesc desc, bool temporary = false) = 0;
		virtual AcquireDeferredRes	AcquireDeferred(GPUResourceDesc desc, bool temporary = false) = 0;

		virtual AcquireResult		Recycle(ResourceHandle resource, GPUResourceDesc desc) = 0;
		virtual void				Release(ResourceHandle handle, uint64_t submissionID, const bool freeResourceImmedate = true, const bool allowImmediateReuse = true) = 0;

		virtual void				LockRange(uint64_t begin, uint64_t end) = 0;

		virtual uint32_t Flags() const = 0;
	};

	constexpr size_t DefaultBlockSize = 64 * KILOBYTE;


	/************************************************************************************************/


	struct SyncPoint
	{
		uint64_t		syncCounter = 0;
		DeviceFence_ptr	fence = nullptr;

		operator bool() const noexcept { return fence != nullptr; }
	};

	
	/************************************************************************************************/


	struct HeapDescriptor
	{
		uint32_t				registerIdx = -1;
		uint32_t				count	= 0;
		uint32_t				space	= 0;
		DescHeapEntryType		type	= DescHeapEntryType::HeapError;
	};


	/************************************************************************************************/


	class DesciptorHeapLayout
	{
	public:
		DesciptorHeapLayout() {}
		DesciptorHeapLayout(iAllocator& allocator) : entries{ allocator } {}

		DesciptorHeapLayout(const DesciptorHeapLayout& RHS)
		{
			entries = RHS.entries;

#ifdef _DEBUG
			Check();
#endif
		}


		bool SetParameterAsCBV(
			uint32_t Index, uint32_t BaseRegister, uint32_t RegisterCount, uint32_t RegisterSpace = 0)
		{
			HeapDescriptor Desc;
			Desc.registerIdx	= BaseRegister;
			Desc.space			= RegisterSpace;
			Desc.type			= DescHeapEntryType::ConstantBuffer;
			Desc.count			= RegisterCount;

			if (entries.size() <= Index)
			    entries.resize(Index + 1);

		    entries[Index] = Desc;

			return true;
		}


		bool SetParameterAsSRV(
			uint32_t Index, uint32_t BaseRegister, uint32_t RegisterCount, uint32_t RegisterSpace = 0)
		{
			HeapDescriptor Desc;
			Desc.registerIdx	= uint32_t(BaseRegister);
			Desc.space			= uint32_t(RegisterSpace);
			Desc.type			= DescHeapEntryType::ShaderResource;
			Desc.count			= RegisterCount;

			if (entries.size() <= Index)
				entries.resize(Index + 1);

			entries[Index] = Desc;

			return true;
		}

		bool SetParameterAsSRVImage(
			uint32_t Index, uint32_t BaseRegister, uint32_t RegisterCount, uint32_t RegisterSpace = 0)
		{
			HeapDescriptor Desc;
			Desc.registerIdx	= uint32_t(BaseRegister);
			Desc.space			= uint32_t(RegisterSpace);
			Desc.type			= DescHeapEntryType::ShaderResourceImage;
			Desc.count			= RegisterCount;

			if (entries.size() <= Index)
				entries.resize(Index + 1);

			entries[Index] = Desc;

			return true;
		}


		bool SetParameterAsShaderUAV(
			uint32_t Index, uint32_t BaseRegister, uint32_t RegisterCount, uint32_t RegisterSpace = 0)
		{
			HeapDescriptor Desc;
			Desc.registerIdx	= BaseRegister;
			Desc.space			= RegisterSpace;
			Desc.count			= RegisterCount;
			Desc.type			= DescHeapEntryType::UAVBuffer;

			if (entries.size() <= Index)
				entries.resize(Index + 1);

			entries[Index] = Desc;

			return true;
		}


		bool Check()
		{
			return (!IsXInSet(DescHeapEntryType::HeapError, entries, [](auto a, auto b) -> bool
				{ return a == b.type; }));
		}


		const size_t size() const
		{
			size_t out = 0;
			for (auto& e : entries)
				out += e.count + e.space;

			FK_ASSERT(out);

			return out;
		}

		static constexpr size_t EntryCount = 4;
		Vector<HeapDescriptor, EntryCount> entries;
	};


	/************************************************************************************************/


	struct IRenderWindow
	{
		virtual ~IRenderWindow() {};

		virtual ResourceHandle      GetBackBuffer() const = 0;
		virtual uint2               GetWH() const = 0;

		virtual bool                Present(const uint32_t syncInternal = 0, const uint32_t flags = 0) = 0;
		virtual void                Resize(const uint2 WH) = 0;

		virtual void				Release() = 0;

		operator ResourceHandle () { return GetBackBuffer(); }

		float2  GetPixelSize() const	{ return float2{ 1.0f, 1.0f } / GetWH(); }
		float   GetAspectRatio() const	{ const auto WH = GetWH(); return float(WH[0]) / float(WH[1]); }
	};


	/************************************************************************************************/

	struct IIndirectLayout
	{
	};

	struct IndirectLayout
	{
		IndirectLayout() noexcept
		{
			FK_ASSERT(false);
		}


		~IndirectLayout() noexcept
		{
			FK_ASSERT(false);
		}


		IndirectLayout(const IndirectLayout& rhs)
		{
			FK_ASSERT(false);
		}


		IndirectLayout& operator =	(const IndirectLayout& rhs) noexcept
		{
			FK_ASSERT(false);
			return (*this);
		}

		operator bool() noexcept
		{
			FK_ASSERT(false);
		    return false;
		}

		std::byte internal[64];
	};


	/************************************************************************************************/


	struct RootSignatureHeapEntry
	{
		size_t					idx;
		DesciptorHeapLayout		Heap;
	};


	struct RootSignatureBuilderImpl
	{
	};


	struct PipelineInterfaceBuilder : NoCopy, NoMove
	{
		PipelineInterfaceBuilder(iAllocator& allocator);
		~PipelineInterfaceBuilder();

		void Release();

		bool SetParameterAsUINT(size_t Index, uint32_t size, uint32_t cbRegister, uint32_t registerSpace, PIPELINE AccessableStages = PIPELINE::PIPELINE_DEST_ALL);

		bool SetParameterAsDescriptorTable(
			size_t index, const DesciptorHeapLayout& layout, size_t unused = -1, PIPELINE accessableStages = PIPELINE::PIPELINE_DEST_ALL);

		bool SetParameterAsCBV(
			size_t Index, size_t Register, size_t RegisterSpace = 0,
			PIPELINE AccessableStages = PIPELINE::PIPELINE_DEST_ALL);

		bool SetParameterAsUAV(
			size_t Index, size_t Register, size_t RegisterSpace = 0,
			PIPELINE AccessableStages = PIPELINE::PIPELINE_DEST_ALL);

		bool SetParameterAsSRV(
			size_t Index, size_t Register, size_t RegisterSpace = 0,
			PIPELINE AccessableStages = PIPELINE::PIPELINE_DEST_ALL);

		void Clear();

		bool AllowIA = false;

		[[nodiscard]] IPipelineInterface* Build(iAllocator& TempMemory);
		[[nodiscard]] IPipelineInterface* LoadSignatureFromFile(const char* dir, const char* entry, iAllocator& temp);
		[[nodiscard]] IPipelineInterface* LoadSignatureFromBlob(void* _ptr, size_t size, iAllocator& temp);
	};


	/************************************************************************************************/


	struct IPipelineBuilderImpl : NoCopy
	{
		virtual ~IPipelineBuilderImpl() {}
		virtual void Release() = 0;

		virtual IPipelineBuilderImpl& AddRootSignature	(const IPipelineInterface* rootSig) = 0;

		virtual IPipelineBuilderImpl& AddShaderLibrary	(const char* file, const ShaderOptions& options = {}) = 0;
		virtual IPipelineBuilderImpl& AddComputeShader	(const char* entryPoint, const char* file, const ShaderOptions& options = {}) = 0;
		virtual IPipelineBuilderImpl& AddWorkGraph		(const WorkGraph_Desc& desc = {}) = 0;

		virtual IPipelineBuilderImpl& AddVertexShader	(const char* entryPoint, const char* file, const ShaderOptions& options = {}) = 0;
		virtual IPipelineBuilderImpl& AddDomainShader	(const char* entryPoint, const char* file, const ShaderOptions& options = {}) = 0;
		virtual IPipelineBuilderImpl& AddHullShader		(const char* entryPoint, const char* file, const ShaderOptions& options = {}) = 0;
		virtual IPipelineBuilderImpl& AddGeometryShader	(const char* entryPoint, const char* file, const ShaderOptions& options = {}) = 0;

		virtual IPipelineBuilderImpl& AddAmplificationShader	(const char* entryPoint, const char* file, const ShaderOptions& options = {}) = 0;
		virtual IPipelineBuilderImpl& AddMeshShader				(const char* entryPoint, const char* file, const ShaderOptions& options = {}) = 0;

		virtual IPipelineBuilderImpl& AddPixelShader			(const char* entryPoint, const char* file, const ShaderOptions& options = {}) = 0;
		virtual IPipelineBuilderImpl& AddPixelShader			(const char* entryPoint, const Shader&) = 0;

		virtual IPipelineBuilderImpl& SetDebugName(const char* name) = 0;

		virtual IPipelineBuilderImpl& AddInputLayout		(const InputLayoutState&	state = {}) = 0;
		virtual IPipelineBuilderImpl& AddInputTopology		(const ETopology			topology) = 0;
		virtual IPipelineBuilderImpl& AddDepthStencilState	(const DepthStencilState&	state = {})= 0;
		virtual IPipelineBuilderImpl& AddRasterizerState	(const RasterizerState&		state = {})= 0;
		virtual IPipelineBuilderImpl& AddRenderTargetState	(const RenderTargetState&	state = {})= 0;
		virtual IPipelineBuilderImpl& AddDepthStencilFormat	(const DeviceFormat			format = DeviceFormat::D24_UNORM_S8_UINT) = 0;
		virtual IPipelineBuilderImpl& AddBlendState			(const BlendState&			state = {}) = 0;

		virtual LoadPipelineStateRes Build(IRenderSystem& renderSystem, iAllocator& tempAllocator) = 0;
		virtual LoadPipelineStateRes BuildStream(IRenderSystem& renderSystem, void* buffer, const size_t size) = 0;
	};


	struct PipelineBuilder
	{
	    PipelineBuilder(IRenderSystem&, iAllocator& allocator);
		~PipelineBuilder();

		IPipelineBuilderImpl& AddRootSignature	(const IPipelineInterface* rootSig);

		IPipelineBuilderImpl& AddShaderLibrary	(const char* file, const ShaderOptions& options = {});
		IPipelineBuilderImpl& AddComputeShader	(const char* entryPoint, const char* file, const ShaderOptions& options = {});
		IPipelineBuilderImpl& AddWorkGraph		(const WorkGraph_Desc& desc = {});

		IPipelineBuilderImpl& AddVertexShader	(const char* entryPoint, const char* file, const ShaderOptions& options = {});
		IPipelineBuilderImpl& AddDomainShader	(const char* entryPoint, const char* file, const ShaderOptions& options = {});
		IPipelineBuilderImpl& AddHullShader		(const char* entryPoint, const char* file, const ShaderOptions& options = {});
		IPipelineBuilderImpl& AddGeometryShader	(const char* entryPoint, const char* file, const ShaderOptions& options = {});

		IPipelineBuilderImpl& AddAmplificationShader(const char* entryPoint, const char* file, const ShaderOptions& options = {});
		IPipelineBuilderImpl& AddMeshShader			(const char* entryPoint, const char* file, const ShaderOptions& options = {});

		IPipelineBuilderImpl& AddPixelShader		(const char* entryPoint, const char* file, const ShaderOptions& options = {});
		IPipelineBuilderImpl& AddPixelShader		(Shader);

		IPipelineBuilderImpl& SetDebugName			(const char* name) ;

		IPipelineBuilderImpl& AddInputLayout		(const InputLayoutState&	state = {});
		IPipelineBuilderImpl& AddInputTopology		(const ETopology			topology);
		IPipelineBuilderImpl& AddDepthStencilState	(const DepthStencilState&	state = {});
		IPipelineBuilderImpl& AddRasterizerState	(const RasterizerState&		state = {});
		IPipelineBuilderImpl& AddRenderTargetState	(const RenderTargetState&	state = {});
		IPipelineBuilderImpl& AddDepthStencilFormat	(const DeviceFormat			format = DeviceFormat::D24_UNORM_S8_UINT);
		IPipelineBuilderImpl& AddBlendState			(const BlendState&			state = {});

		LoadPipelineStateRes Build(IRenderSystem& renderSystem, iAllocator& tempAllocator);
		LoadPipelineStateRes BuildStream(IRenderSystem& renderSystem, void* buffer, const size_t size);


	private:
		IPipelineBuilderImpl& GetImpl();
		std::byte implSpace[128];
	};


	struct IContext
	{
		virtual ~IContext() {}

		virtual struct IRenderSystem& GetRenderSystem() noexcept = 0;
	};


#if _DEBUG & WIN32
#define IDIRECTCONTEXTDEBUGBODY { FK_ASSERT(false, "NOT IMPLEMENTED: " __FUNCSIG__); };
#else
#define IDIRECTCONTEXTDEBUGBODY = 0;
#endif

	struct IDirectContext : public IContext
	{
		virtual void SetDebugName(const char* debugStr) noexcept IDIRECTCONTEXTDEBUGBODY;
		virtual void FlushBarriers() noexcept IDIRECTCONTEXTDEBUGBODY;

		virtual void CreateAS(const AccelerationStructureDesc&, const TriMesh&) IDIRECTCONTEXTDEBUGBODY;
		virtual void BuildBLAS(struct IVertexBufferSet& bufferSet, ResourceHandle destination, ResourceHandle scratchSpace) IDIRECTCONTEXTDEBUGBODY;

		virtual void DiscardResource(ResourceHandle resource) IDIRECTCONTEXTDEBUGBODY;

		virtual void AddAliasingBarrier			(ResourceHandle before, ResourceHandle after) IDIRECTCONTEXTDEBUGBODY;
		virtual void AddUAVBarrier				(ResourceHandle Handle = InvalidHandle, uint32_t subresource = -1, DeviceLayout layout = DeviceLayout::Unknown, DeviceSyncPoint src = Sync_All, DeviceSyncPoint dst = Sync_All) IDIRECTCONTEXTDEBUGBODY;
		virtual void AddPresentBarrier			(ResourceHandle Handle,	DeviceAccessState Before) IDIRECTCONTEXTDEBUGBODY;
		virtual void AddStreamOutBarrier		(SOResourceHandle,		DeviceAccessState Before, DeviceAccessState State) IDIRECTCONTEXTDEBUGBODY;
		virtual void AddCopyResourceBarrier		(ResourceHandle Handle, DeviceAccessState Before, DeviceAccessState State) IDIRECTCONTEXTDEBUGBODY;

		virtual void AddGlobalBarrier			(ResourceHandle resource, DeviceAccessState accessBefore, DeviceAccessState accessAfter, DeviceSyncPoint syncBefore, DeviceSyncPoint syncAfter) IDIRECTCONTEXTDEBUGBODY;
		virtual void AddTextureBarrier			(ResourceHandle Handle, DeviceAccessState, DeviceAccessState, DeviceLayout, DeviceLayout, DeviceSyncPoint, DeviceSyncPoint, BarrierSubResourceRange range = {}) IDIRECTCONTEXTDEBUGBODY;
		virtual void AddBufferBarrier			(ResourceHandle Handle, DeviceAccessState, DeviceAccessState, DeviceSyncPoint, DeviceSyncPoint) IDIRECTCONTEXTDEBUGBODY;
		virtual void AddBarriers				(std::span<const Barrier> barriers) IDIRECTCONTEXTDEBUGBODY;

		virtual void ClearDepthBuffer			(ResourceHandle Texture, float ClearDepth = 0.0f) IDIRECTCONTEXTDEBUGBODY;
		virtual void ClearRenderTarget			(ResourceHandle Texture, float4 ClearColor = float4(0.0f)) IDIRECTCONTEXTDEBUGBODY;
		virtual void ClearUAVTextureFloat		(ResourceHandle UAV, float4 clearColor = float4(0, 0, 0, 0)) IDIRECTCONTEXTDEBUGBODY;
		virtual void ClearUAVTextureUint		(ResourceHandle UAV, uint4 clearColor = uint4{ 0, 0, 0, 0 }) IDIRECTCONTEXTDEBUGBODY;
		virtual void ClearUAV					(ResourceHandle UAV, uint4 clearColor = uint4{ 0, 0, 0, 0 }) IDIRECTCONTEXTDEBUGBODY;
		virtual void ClearUAVBuffer				(ResourceHandle UAV, uint4 clearColor = uint4{ 0, 0, 0, 0 }) IDIRECTCONTEXTDEBUGBODY;
		virtual void ClearUAVBufferRange		(ResourceHandle UAV, uint begin, uint end, uint4 clearColor = uint4{ 0, 0, 0, 0 }) IDIRECTCONTEXTDEBUGBODY;

		virtual void SetRootSignature			(RootSigHandle) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetRootSignature			(const IPipelineInterface*) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetComputeRootSignature	(RootSigHandle) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetComputeRootSignature	(const IPipelineInterface*) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetPipelineState			(const struct IPipelineState* const PSO) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetComputePipelineState	(const PSOHandle, iAllocator& temp) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetGraphicsPipelineState	(const PSOHandle, iAllocator& temp) IDIRECTCONTEXTDEBUGBODY;

		virtual void SetRenderTargets			(const static_vector<ResourceHandle> RTs, bool DepthStecil = false, ResourceHandle DepthStencil = InvalidHandle, const size_t MIPMapOffset = 0) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetRenderTargets2			(const static_vector<ResourceHandle> RTs, const size_t MIPMapOffset, const DepthStencilView_Options DSV) IDIRECTCONTEXTDEBUGBODY;

#if 0
		virtual void SetViewports				(static_vector<D3D12_VIEWPORT, 16>	VPs) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetViewports				(std::span<const D3D12_VIEWPORT>	VPs) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetScissorRects			(static_vector<D3D12_RECT, 16>		Rects) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetScissorRects			(std::span<const D3D12_RECT>		Rects) IDIRECTCONTEXTDEBUGBODY;
#endif

		virtual void SetScissorAndViewports		(static_vector<ResourceHandle, 16>	RenderTargets) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetScissorAndViewports2	(static_vector<ResourceHandle, 16>	RenderTargets, const size_t MIPMapOffset = 0) IDIRECTCONTEXTDEBUGBODY;

		virtual void QueueReadBack				(ReadBackResourceHandle readBack) IDIRECTCONTEXTDEBUGBODY;
		virtual void QueueReadBack				(ReadBackResourceHandle readBack, ReadBackEventHandler callback) IDIRECTCONTEXTDEBUGBODY;

		virtual void SetDepthStencil			(ResourceHandle DS) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetInputPrimitive			(EInputPrimitive primitive) IDIRECTCONTEXTDEBUGBODY;

		virtual void SetGraphicsConstantValue	(size_t idx, size_t valueCount, const void* data_ptr, size_t offset = 0) IDIRECTCONTEXTDEBUGBODY;

		virtual void NullGraphicsConstantBufferView	(size_t idx) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetGraphicsConstantBufferView	(size_t idx, const ConstantBufferHandle CB, size_t Offset = 0) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetGraphicsConstantBufferView	(size_t idx, const struct ConstantBufferDataSet& CB) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetGraphicsConstantBufferView	(size_t idx, DevicePointer) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetGraphicsDescriptorTable		(size_t idx, const struct DescriptorHeap& DH) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetGraphicsDescriptorTable		(size_t idx, const DescriptorRange& range) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetGraphicsShaderResourceView	(size_t idx, ResourceHandle resource, size_t offset = 0) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetGraphicsUnorderedAccessView (size_t idx, ResourceHandle resource, size_t offset = 0) IDIRECTCONTEXTDEBUGBODY;


		virtual void SetComputeDescriptorTable		(size_t idx) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetComputeDescriptorTable		(size_t idx, const struct DescriptorHeap& DH) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetComputeDescriptorTable		(size_t idx, const DescriptorRange& range) IDIRECTCONTEXTDEBUGBODY;

		virtual void SetComputeConstantBufferView	(size_t idx, const ConstantBufferHandle, size_t offset) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetComputeConstantBufferView	(size_t idx, const struct ConstantBufferDataSet& CB) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetComputeConstantBufferView	(size_t idx, ResourceHandle, size_t offset = 0, size_t bufferSize = 256) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetComputeConstantBufferView	(size_t idx, DevicePointer) IDIRECTCONTEXTDEBUGBODY;

		virtual void SetComputeShaderResourceView	(size_t idx, ResourceHandle resource, size_t offset = 0) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetComputeUnorderedAccessView	(size_t idx, ResourceHandle resource, size_t offset = 0) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetComputeConstantValue		(size_t idx, size_t valueCount, const void* data_ptr, size_t offset = 0) IDIRECTCONTEXTDEBUGBODY;


		virtual void BeginQuery	(QueryHandle query, size_t idx) IDIRECTCONTEXTDEBUGBODY;
		virtual void EndQuery	(QueryHandle query, size_t idx) IDIRECTCONTEXTDEBUGBODY;

		virtual void TimeStamp(QueryHandle query, size_t idx) IDIRECTCONTEXTDEBUGBODY;

		virtual void SetMarker_DEBUG(const char* str) IDIRECTCONTEXTDEBUGBODY;

		virtual void BeginEvent_DEBUG(const char* str) IDIRECTCONTEXTDEBUGBODY;
		virtual void EndEvent_DEBUG() IDIRECTCONTEXTDEBUGBODY;

		virtual void CopyResource(ResourceHandle dest, ResourceHandle src) IDIRECTCONTEXTDEBUGBODY;


		virtual void CopyBufferRegion(
			ResourceHandle	destination,
			ResourceHandle	source,
			size_t			size,
			size_t			destinationOffset = 0,
			size_t			sourceOffset = 0) IDIRECTCONTEXTDEBUGBODY;


		virtual void CopyBufferRegion(
			ResourceHandle		destination,
			DeviceResource_ptr	source,
			size_t				size,
			size_t				destinationOffset = 0,
			size_t				sourceOffset = 0) IDIRECTCONTEXTDEBUGBODY;


		virtual void CopyBufferRegion(
			DeviceResource_ptr	destination,
			ResourceHandle		source,
			size_t				size,
			size_t				destinationOffset = 0,
			size_t				sourceOffset = 0) IDIRECTCONTEXTDEBUGBODY;


		virtual void CopyBufferRegion(
			DeviceResource_ptr	destination,
			DeviceResource_ptr	source,
			size_t				size,
			size_t				destinationOffset = 0,
			size_t				sourceOffset = 0) IDIRECTCONTEXTDEBUGBODY;


	    virtual void CopyTextureRegion(
			ResourceHandle		destination,
			size_t				subResourceIdx,
			uint3				XYZ,
			UploadReservation	source,
			uint2				wh) IDIRECTCONTEXTDEBUGBODY;


		virtual void CopyTile(
			ResourceHandle			dest,
			const uint3				destTile,
			const size_t			tileOffset,
			const UploadReservation src) IDIRECTCONTEXTDEBUGBODY;


		virtual void ImmediateWrite(
			static_vector<ResourceHandle>		handles,
			static_vector<size_t>				value,
			static_vector<DeviceAccessState>	currentStates,
			static_vector<DeviceAccessState>	finalStates) IDIRECTCONTEXTDEBUGBODY;

		virtual void AddIndexBuffer			(TriMesh* Mesh, uint32_t lod = 0) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetIndexBuffer			(VertexBufferEntry buffer, DeviceFormat format = DeviceFormat::R32_UINT) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetIndexBuffer			(ResourceHandle, DeviceFormat format = DeviceFormat::R32_UINT) IDIRECTCONTEXTDEBUGBODY;

		virtual void AddVertexBuffers		(TriMesh* Mesh, uint32_t lod, const std::initializer_list<VERTEXBUFFER_TYPE>& buffers, VertexBufferList* InstanceBuffers = nullptr) IDIRECTCONTEXTDEBUGBODY;
		virtual void AddVertexBuffers		(TriMesh* Mesh, uint32_t lod, const std::span<const VERTEXBUFFER_TYPE> buffers, VertexBufferList* InstanceBuffers = nullptr) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetVertexBuffers		(const std::initializer_list<VertexBufferEntry>&	span) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetVertexBuffers		(const std::span<const VertexBufferEntry>			span) IDIRECTCONTEXTDEBUGBODY;

		virtual void SetVertexBuffers		(const std::initializer_list<VertexBufferResource>&	span) IDIRECTCONTEXTDEBUGBODY;
		virtual void SetVertexBuffers		(const std::span<const VertexBufferResource>		span) IDIRECTCONTEXTDEBUGBODY;

		virtual void SetVertexBuffers2		(const std::span<const VBView> views, uint32_t offset = 0) IDIRECTCONTEXTDEBUGBODY;

		virtual void Draw					(const size_t VertexCount, const size_t BaseVertex = 0, const size_t baseIndex = 0) IDIRECTCONTEXTDEBUGBODY;
		virtual void DrawInstanced			(const size_t VertexCount, const size_t BaseVertex = 0, const size_t instanceCount = 0, size_t instanceOffset = 0) IDIRECTCONTEXTDEBUGBODY;
		virtual void DrawIndexed			(const size_t IndexCount, const size_t IndexOffet = 0, const size_t BaseVertex = 0) IDIRECTCONTEXTDEBUGBODY;
		virtual void DrawIndexedInstanced	(const size_t IndexCount, const size_t IndexOffet = 0, const size_t BaseVertex = 0, const size_t InstanceCount = 1, const size_t InstanceOffset = 0) IDIRECTCONTEXTDEBUGBODY;
		virtual void Clear					() IDIRECTCONTEXTDEBUGBODY;

		virtual void ResolveQuery			(QueryHandle query, size_t begin, size_t end, ResourceHandle destination, size_t destOffset) IDIRECTCONTEXTDEBUGBODY;
		virtual void ResolveQuery			(QueryHandle query, size_t begin, size_t end, DeviceResource_ptr destination, size_t destOffset) IDIRECTCONTEXTDEBUGBODY;

		virtual void ExecuteIndirect		(ResourceHandle args, const IndirectLayout& layout, size_t argumentBufferOffset = 0, size_t executionCount = 1) IDIRECTCONTEXTDEBUGBODY;
		virtual void Dispatch				(const uint3) IDIRECTCONTEXTDEBUGBODY;
		virtual void Dispatch				(const IPipelineState* const PSO, const uint3 xyz) IDIRECTCONTEXTDEBUGBODY;
		virtual void DispatchRays			(const uint3, const DispatchDesc desc) IDIRECTCONTEXTDEBUGBODY;
		virtual void DispatchMesh			(const uint3) IDIRECTCONTEXTDEBUGBODY;

		virtual void SetPredicate(bool Enable, ResourceHandle Handle = InvalidHandle, size_t = 0, PredicateOp op = PredicateOp::EqualZero) IDIRECTCONTEXTDEBUGBODY;

		virtual void CopyBuffer		(const UploadReservation src, const ResourceHandle destination, const size_t destOffset = 0) IDIRECTCONTEXTDEBUGBODY;
		virtual void CopyTexture2D	(const UploadReservation src, const ResourceHandle destination, const uint2 BufferSize) IDIRECTCONTEXTDEBUGBODY;

		virtual  void SetRTRead		(ResourceHandle Handle) IDIRECTCONTEXTDEBUGBODY;
		virtual  void SetRTWrite	(ResourceHandle Handle) IDIRECTCONTEXTDEBUGBODY;
		virtual  void SetRTFree		(ResourceHandle Handle) IDIRECTCONTEXTDEBUGBODY;

		virtual void Close() IDIRECTCONTEXTDEBUGBODY;

		virtual void SetViewports				(std::span<const Viewport>	VPs)	IDIRECTCONTEXTDEBUGBODY;
		virtual void SetScissorRects			(std::span<const Rect>		rects)	IDIRECTCONTEXTDEBUGBODY;

		virtual UploadReservation	ReserveDirectUploadSpace(size_t size, size_t alignment = 256)
		{
#ifdef WIN32
			FK_ASSERT(false, "NOT IMPLEMENTED: " __FUNCSIG__);
#else
			FK_ASSERT(false);
#endif

		    return {};
		}
	};


	/************************************************************************************************/


	struct ICopyContext : public IContext
	{
		virtual ~ICopyContext() {}
		virtual void                Barrier(ResourceHandle destination, DeviceAccessState before, DeviceAccessState after) = 0;

	    virtual UploadReservation	Reserve(size_t byteSize, uint32_t alignment = 256) { return {}; }
		virtual void				CopyBuffer(ResourceHandle source, size_t dstOffset, UploadReservation) {}
		virtual void                CopyBuffer(GPURange dest, void* source_ptr, uint64_t size) {}
		virtual void                CopyBuffer(ResourceHandle destination, const size_t destinationOffset, ResourceHandle source, const size_t sourceOffset, const size_t copySize) {}
		virtual void                CopyTextureRegion(ResourceHandle, size_t subResourceIdx, uint3 XYZ, UploadReservation source, uint2 WH) {}
		virtual void                CopyTile(ResourceHandle dest, const uint3 destTile, const size_t tileOffset, const UploadReservation src) {}

		virtual bool				IsSubResourceTiled(ResourceHandle Resource, const size_t level) const { return false; }
	};


	/************************************************************************************************/


	struct IDescriptorHeap
	{
		virtual ~IDescriptorHeap() {}

	    virtual IDescriptorHeap& operator = (IDescriptorHeap&&) = 0;

		virtual void Init(IContext& ctx, const DesciptorHeapLayout& Layout_IN, iAllocator& TempMemory) = 0;
		virtual void Init(IContext& ctx, const DesciptorHeapLayout& Layout_IN, const size_t reserveCount, iAllocator& TempMemory) = 0;
		virtual void Init2(IContext& ctx, const DesciptorHeapLayout& Layout_IN, const size_t reserveCount, iAllocator& TempMemory) = 0;
		virtual void NullFill(IContext& ctx, const size_t end = -1) = 0;

		virtual void SetCBV(IContext& ctx, size_t idx, const ConstantBufferDataSet& constants) = 0;
		virtual void SetCBV(IContext& ctx, size_t idx, ConstantBufferHandle, size_t offset, size_t bufferSize) = 0;
		virtual void SetCBV(IContext& ctx, size_t idx, ResourceHandle, size_t offset, size_t bufferSize) = 0;

		virtual void SetSRV(IContext& ctx, size_t idx, ResourceHandle) = 0;
		virtual void SetSRV(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format) = 0;
		virtual void SetSRV(IContext& ctx, size_t idx, ResourceHandle, uint MipOffset, DeviceFormat format) = 0;
		virtual void SetSRVArray(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format) = 0;

		virtual void SetSRV3D(IContext& ctx, size_t idx, ResourceHandle) = 0;

		virtual void SetSRVCubemap(IContext& ctx, size_t idx, ResourceHandle Handle) = 0;
		virtual void SetSRVCubemap(IContext& ctx, size_t idx, ResourceHandle Handle, DeviceFormat format) = 0;

		virtual void SetUAVBuffer(IContext& ctx, size_t idx, ResourceHandle, size_t   offset = 0) = 0;

		virtual void SetUAVTexture(IContext& ctx, size_t idx, ResourceHandle) = 0;

		virtual void SetUAVTexture(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format) = 0;
		virtual void SetUAVTexture(IContext& ctx, size_t idx, size_t mipLevel, ResourceHandle, DeviceFormat format) = 0;

		virtual void SetUAVCubemap(IContext& ctx, size_t idx, ResourceHandle handle) = 0;

		virtual void SetUAVTexture3D(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format) = 0;

		virtual void SetUAVStructured(IContext& ctx, size_t idx, ResourceHandle, size_t stride, size_t offset = 0) = 0;
		virtual void SetUAVStructured(IContext& ctx, size_t idx, ResourceHandle resource, ResourceHandle counter, size_t stride, size_t Offset) = 0;

		virtual void SetStructuredResource(IContext& ctx, size_t idx, ResourceHandle, size_t stride = 4, size_t offset = 0) = 0; //

		virtual DevicePointer	GetGPUDescriptorHandle	() const = 0;
		virtual DescriptorHeap	GetHeapOffsetted(size_t offset, IContext& ctx) const = 0;
	};


	struct DescriptorHeap
	{
		DescriptorHeap() = default;
		DescriptorHeap(IContext&, const DesciptorHeapLayout& Layout_IN, iAllocator* TempMemory);

		DescriptorHeap& operator = (const DescriptorHeap&);

		// moveable
		DescriptorHeap(DescriptorHeap&& rhs);
		DescriptorHeap& operator = (DescriptorHeap&&);

		DescriptorHeap& Init(IContext& ctx, const DesciptorHeapLayout& Layout_IN, iAllocator* TempMemory);
		DescriptorHeap& Init(IContext& ctx, const DesciptorHeapLayout& Layout_IN, const size_t reserveCount, iAllocator* TempMemory);
		DescriptorHeap& Init2(IContext& ctx, const DesciptorHeapLayout& Layout_IN, const size_t reserveCount, iAllocator* TempMemory); // for variable size heap layouts
		DescriptorHeap& NullFill(IContext& ctx, const size_t end = -1);

		DescriptorHeap& SetCBV(IContext& ctx, size_t idx, const ConstantBufferDataSet& constants);
		DescriptorHeap& SetCBV(IContext& ctx, size_t idx, ConstantBufferHandle, size_t offset, size_t bufferSize);
		DescriptorHeap& SetCBV(IContext& ctx, size_t idx, ResourceHandle, size_t offset, size_t bufferSize);

		DescriptorHeap& SetSRV(IContext& ctx, size_t idx, ResourceHandle);
		DescriptorHeap& SetSRV(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format);
		DescriptorHeap& SetSRV(IContext& ctx, size_t idx, ResourceHandle, uint MipOffset, DeviceFormat format);
		DescriptorHeap& SetSRVArray(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format);

		DescriptorHeap& SetSRV3D(IContext& ctx, size_t idx, ResourceHandle);

		DescriptorHeap& SetSRVCubemap(IContext& ctx, size_t idx, ResourceHandle		Handle);
		DescriptorHeap& SetSRVCubemap(IContext& ctx, size_t idx, ResourceHandle		Handle, DeviceFormat format);

		DescriptorHeap& SetUAVBuffer(IContext& ctx, size_t idx, ResourceHandle, size_t   offset = 0);

		DescriptorHeap& SetUAVTexture(IContext& ctx, size_t idx, ResourceHandle);

		DescriptorHeap& SetUAVTexture(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format);
		DescriptorHeap& SetUAVTexture(IContext& ctx, size_t idx, size_t mipLevel, ResourceHandle, DeviceFormat format);

		DescriptorHeap& SetUAVCubemap(IContext& ctx, size_t idx, ResourceHandle handle);

		DescriptorHeap& SetUAVTexture3D(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format);

		DescriptorHeap& SetUAVStructured(IContext& ctx, size_t idx, ResourceHandle, size_t stride, size_t offset = 0);
		DescriptorHeap& SetUAVStructured(IContext& ctx, size_t idx, ResourceHandle resource, ResourceHandle counter, size_t stride, size_t Offset);

		DescriptorHeap& SetStructuredResource(IContext& ctx, size_t idx, ResourceHandle, size_t stride = 4, size_t offset = 0); //

		DescriptorHeap			GetHeapOffsetted(size_t offset, IContext& ctx) const;
		static IDescriptorHeap&	GetImpl(std::byte*) noexcept;

		std::byte internal[64];
	};


	/************************************************************************************************/


	struct IPipelineInterface
	{
		virtual const DesciptorHeapLayout&		GetDescHeap(uint32_t idx) const noexcept = 0;
		virtual DeviceRootSignature_ptr			GetAPIObject() const noexcept = 0;

		virtual void Release() = 0;
	};


	/************************************************************************************************/


	struct IPipelineState
	{
		virtual DevicePipelineState_ptr		GetDevicePipeState() const = 0;
		virtual const IPipelineInterface*	GetInterface() const noexcept = 0;
	};


	struct VertexBuffer
	{
		uint32_t			byteSize;
		uint32_t			byteStride;
		DeviceResource_ptr	resource;
		VERTEXBUFFER_TYPE	type;

		size_t Size() const { return byteSize / byteStride; }
	};


	struct IVertexBufferSet
	{
		virtual void						Clear() = 0;
		virtual std::optional<VertexBuffer> Find			(VERTEXBUFFER_TYPE) const = 0;
		virtual std::optional<uint32_t>		FindIdx			(VERTEXBUFFER_TYPE) const = 0;
		virtual const VertexBuffer			operator []		(uint8_t idx) const = 0;
		        const VertexBuffer			At				(uint8_t idx) const { return (*this)[idx]; }
		virtual uint8_t						GetIndexBufferIndex() const = 0;
	};


	/************************************************************************************************/


	enum class RenderSystemModes
	{
	    Normal,
	    Diagnostic,
		Debug
	};


    struct RenderSystemOptions
	{
		struct ThreadManager*	threads;
		iAllocator*				allocator;
		RenderSystemModes		modes = RenderSystemModes::Normal;
	};


	/************************************************************************************************/


	struct IRenderSystem
	{
		IRenderSystem()
		{
			instance = this;
		}

		inline static IRenderSystem* instance = nullptr;

		static IRenderSystem& GetInstance()
		{
			FK_ASSERT(instance != nullptr);
			return *instance;
		}

		virtual bool												Initiate(Graphics_Desc& desc) = 0;

		virtual void												BuildLibrary			(PSOHandle State, const PipelineStateLibraryDesc) = 0;
		virtual void												RegisterPSOLoader		(PSOHandle State, LOADSTATE_FN FN) = 0;
		virtual void												LoadPSOIfRequired		(PSOHandle State) = 0;
		virtual void												QueuePSOLoad			(PSOHandle State) = 0;

		virtual const IPipelineState*								GetPSO					(PSOHandle State, iAllocator& temp) = 0;
		virtual const IPipelineInterface* const 						GetPSORootSignature		(PSOHandle state) const = 0;
		virtual std::tuple<IPipelineState*, const IPipelineInterface*>	GetPSOAndRootSignature	(PSOHandle stateID, iAllocator& temp) const = 0;

		// Sync functions
		virtual uint64_t	GetCurrentCounter()						= 0;
		virtual SyncPoint	GetSubmissionTicket(uint32_t count = 1)	= 0; 
		virtual void		SyncUploadTo(SyncPoint)					= 0;
		virtual SyncPoint	SyncUploadPoint()						= 0;
		virtual SyncPoint	SyncUploadTicket()						= 0;

		virtual void		SyncDirectTo(SyncPoint)		= 0;
		virtual SyncPoint	SyncDirectPoint()			= 0;
		virtual SyncPoint	SyncSubmittedDirectPoint()	= 0;
		virtual SyncPoint	SyncDirectTicket()			= 0;

		virtual void			SignalDirect(uint64_t)	= 0;
		//virtual void			SignalCopy(uint64_t)	= 0;
		//virtual void			SignalCompute(uint64_t)	= 0;

		// Copy Queue
		virtual void					SubmitUploadQueues(CopyContextHandle* handle, size_t count = 1, std::optional<SyncPoint> syncBefore = {}, std::optional<SyncPoint> syncAfter = {}) = 0;
		virtual CopyContextHandle		OpenUploadQueue()		= 0;
		virtual CopyContextHandle		GetImmediateCopyQueue() = 0;

		virtual IDirectContext&	GetDirectCommandList(std::optional<SyncPoint> ticket = {}) = 0;
		virtual ICopyContext&	GetCopyContext(CopyContextHandle handle = InvalidHandle) = 0;

		virtual SyncPoint	Submit(std::span<IDirectContext*> CLs, std::optional<SyncPoint> sync = {})	= 0;

		virtual void		EndFrame()																	= 0;
		virtual void		Signal(SyncPoint)															= 0;

		virtual void		WaitForGPU()				= 0;
		virtual void		WaitFor(const uint64_t)	= 0;
		virtual void		WaitFor(const SyncPoint&)	= 0;

		// Debug
		virtual void		SetDebugName(ResourceHandle, const char*)		= 0;
		virtual void		SetDebugName(DeviceHeapHandle, const char*)	= 0;


		// Objects methods
		virtual void		SetObjectLayout(SOResourceHandle	handle, DeviceLayout state) noexcept = 0;
		virtual void		SetObjectLayout(ResourceHandle		handle, DeviceLayout state) noexcept = 0;

		// Info queries
		virtual	size_t					GetVertexBufferSize		(const VertexBufferHandle)	const noexcept = 0;
		virtual BLAS_PreBuildInfo		GetBLASPreBuildInfo		(const IVertexBufferSet&)	const noexcept = 0;

		virtual size_t					GetTextureFrameGraphIndex(ResourceHandle)			noexcept = 0;
		virtual void					SetTextureFrameGraphIndex(ResourceHandle, size_t)	noexcept = 0;

		virtual void					MarkTextureUsed			(ResourceHandle Handle) = 0;

		virtual DevicePointer			GetDevicePointer		(const ResourceHandle)			const noexcept = 0;

		virtual DeviceAddressRange		GetDeviceRange			(const ResourceHandle)			const noexcept = 0;
		virtual DeviceAddressRange		GetDeviceRange			(const ConstantBufferHandle)	const noexcept = 0;

		virtual DeviceLayout			GetObjectLayout			(const QueryHandle		handle) const noexcept = 0;
		virtual DeviceLayout			GetObjectLayout			(const SOResourceHandle	handle) const noexcept = 0;
		virtual DeviceLayout			GetObjectLayout			(const ResourceHandle	handle) const noexcept = 0;

		virtual size_t					GetResourceSize			(ConstantBufferHandle handle)	const noexcept = 0;
		virtual size_t					GetResourceSize			(ResourceHandle desc)			const noexcept = 0;

		virtual size_t					GetAllocationSize		(ResourceHandle handle) const noexcept = 0; // Includes padding and alignment
		virtual size_t					GetAllocationSize		(GPUResourceDesc desc) const noexcept = 0; // Includes padding and alignment

		virtual size_t					GetTextureElementSize	(ResourceHandle   Handle) const = 0;
		virtual uint2					GetTextureWH			(ResourceHandle   Handle) const = 0;

		virtual DeviceFormat			GetTextureFormat		(ResourceHandle Handle) const = 0;
		virtual uint8_t					GetTextureMipCount		(ResourceHandle Handle) const = 0;
		virtual uint2					GetTextureTilingWH		(ResourceHandle Handle, const uint mipLevel) const = 0;
		virtual uint2					GetHeapOffset			(ResourceHandle Handle, uint subResourceID = 0) const = 0;

		virtual TextureDimension		GetTextureDimension		(ResourceHandle handle) const = 0;
		virtual	size_t					GetTextureArraySize		(ResourceHandle handle) const = 0;

		virtual	DeviceHeap_ptr			GetDeviceResource		(const DeviceHeapHandle			handle) const = 0;
		virtual	DeviceResource_ptr		GetDeviceResource		(const ReadBackResourceHandle	handle) const = 0;
		virtual	DeviceResource_ptr		GetDeviceResource		(const ConstantBufferHandle		handle) const = 0;
		virtual	DeviceResource_ptr		GetDeviceResource		(const ResourceHandle		    handle) const = 0;
		virtual	DeviceResource_ptr		GetDeviceResource		(const SOResourceHandle			handle) const = 0;

		virtual	DeviceResource_ptr		GetSOCounterResource	(const SOResourceHandle		handle)		const = 0;
		virtual	size_t					GetStreamOutBufferSize	(const SOResourceHandle		handle)	const = 0;
		virtual size_t					GetVertexBufferOffset	(const VertexBufferHandle	handle)	const = 0;

		virtual PackedResourceTileInfo	GetPackedTileInfo(ResourceHandle)	const noexcept { return {}; }

		virtual bool				VertexBufferPush		(VertexBufferHandle, void* _ptr, size_t elementSize) = 0;
		virtual size_t				ConstantBufferAlign		(ConstantBufferHandle) = 0;

		virtual void				BackResource(ResourceHandle handle, const GPUResourceDesc& desc) noexcept = 0;

		// Resource upload
		virtual void				UploadTexture(ResourceHandle, CopyContextHandle, std::byte* buffer, size_t bufferSize) = 0; // Uses Upload Queue
		virtual void				UploadTexture(ResourceHandle handle, CopyContextHandle, struct TextureBuffer* buffer, size_t resourceCount) = 0; // Uses Upload Queue
		virtual void				UpdateResourceByUploadQueue(DeviceResource_ptr Dest, CopyContextHandle, const void* Data, size_t Size, size_t ByteSize, DeviceAccessState EndState) = 0;

		virtual ResourceHandle		LoadTexture(TextureBuffer* Buffer, CopyContextHandle handle, DeviceFormat format, iAllocator* allocator) = 0;

		virtual void				SubmitTileMappings			(std::span<ResourceHandle> resources, iAllocator* allocator) = 0;
		virtual void				UpdateTextureTileMappings	(const ResourceHandle Handle, std::span<const TileMapping>, iAllocator& temp) = 0;
		virtual const TileMapList&	GetTileMappings				(const ResourceHandle Handle) = 0;

		virtual SubAllocation		ReserveConstantBuffer	(ConstantBufferHandle CB, size_t reserveSize)	noexcept = 0;
		virtual SubAllocation		ReserveVertexBuffer		(VertexBufferHandle CB, size_t reserveSize)		noexcept = 0;
		virtual UploadReservation	ReserveDirectUploadSpace(size_t size, size_t alignment)					noexcept = 0;
		virtual UploadReservation	ReserveUploadBuffer		(const size_t uploadSize, CopyContextHandle)	noexcept = 0;

		// Shader
		virtual Shader								LoadShader(const char* entryPoint, const char* ShaderType, const char* file, const ShaderOptions& options = {}) = 0;
		virtual Shader								LoadShaderLibrary(const char* file, const ShaderOptions& options = {}) = 0;
		virtual std::expected<Shader, std::string>	LoadRootSignature(const char* file, const char* entry) = 0;

		// Creation
		[[nodiscard]] virtual std::optional<DescriptorRange>	CreateDescriptorRange(const uint32_t descriptorCount = 1) = 0;
		[[nodiscard]] virtual DeviceHeapHandle					CreateHeap(const size_t heapSize, const uint32_t flags) = 0;
		[[nodiscard]] virtual ConstantBufferHandle				CreateConstantBuffer(size_t BufferSize, bool GPUResident = true) = 0;
		[[nodiscard]] virtual VertexBufferHandle				CreateVertexBuffer(size_t BufferSize, bool GPUResident = true) = 0;
		[[nodiscard]] virtual ResourceHandle					CreateDepthBuffer(const uint2 WH, const bool UseFloat = false, size_t bufferCount = 3) = 0;
		[[nodiscard]] virtual ResourceHandle					CreateDepthBufferArray(const uint2 WH, const bool UseFloat = false, const size_t arraySize = 1, const bool buffered = true, const ResourceAllocationType = ResourceAllocationType::Committed) = 0;
		[[nodiscard]] virtual ResourceHandle					CreateGPUResource(const GPUResourceDesc& desc) = 0;
		[[nodiscard]] virtual ResourceHandle					CreateGPUResourceHandle() = 0;
		[[nodiscard]] virtual QueryHandle						CreateOcclusionBuffer(size_t Size) = 0;
		[[nodiscard]] virtual ResourceHandle					CreateUAVBufferResource(size_t bufferHandle, bool tripleBuffer = true) = 0;
		[[nodiscard]] virtual ResourceHandle					CreateUAVTextureResource(const uint2 WH, const DeviceFormat, const bool RenderTarget = false) = 0;
		[[nodiscard]] virtual SOResourceHandle					CreateStreamOutResource(size_t bufferHandle, bool tripleBuffer = true) = 0;
		[[nodiscard]] virtual QueryHandle						CreateSOQuery(size_t SOIndex, size_t count) = 0;
		[[nodiscard]] virtual QueryHandle						CreateTimeStampQuery(size_t count) = 0;
		[[nodiscard]] virtual IndirectLayout					CreateIndirectLayout(static_vector<IndirectDrawDescription> entries, iAllocator* allocator, const IPipelineInterface* signature = nullptr) { return {}; };
		[[nodiscard]] virtual ReadBackResourceHandle			CreateReadBackBuffer(const size_t bufferSize) = 0;
		[[nodiscard]] virtual bool								CreatePipelineBuilder(std::byte* _ptr, size_t bufferSize, iAllocator& tempAllocator) = 0;
	                  virtual void								CreateTextureView(ResourceHandle, DescHeapPOS) = 0;

					  virtual void						SetReadBackEvent(ReadBackResourceHandle readbackBuffer, ReadBackEventHandler&& handler) {}
	    [[nodiscard]] virtual std::pair<void*, size_t>	OpenReadBackBuffer(ReadBackResourceHandle readbackBuffer, const size_t readSize = -1) { return {nullptr, 0}; }

		virtual void CloseReadBackBuffer(ReadBackResourceHandle readbackBuffer) {}
	    virtual void FlushPendingReadBacks() {}


		virtual const IPipelineInterface*	Library(ROOTLIBRARYSIG ID) const noexcept = 0;
		virtual ResourceHandle			DefaultTexture() const noexcept { return FlexKit::InvalidHandle; }

		// Resettable resources
		virtual void ResetConstantBuffer(ConstantBufferHandle constant) = 0;
		virtual void ResetVertexBuffer(VertexBufferHandle constant) = 0;
		virtual void ResetQuery(QueryHandle handle) = 0;

		// Release
		virtual void ReleaseCB(ConstantBufferHandle) = 0;
		virtual void ReleaseVB(VertexBufferHandle) = 0;
		virtual void ReleaseResource(ResourceHandle) = 0;
		virtual void ReleaseReadBack(ReadBackResourceHandle) = 0;
		virtual void ReleaseHeap(DeviceHeapHandle) = 0;
		virtual void ReleaseQuery(QueryHandle) = 0;
		virtual void ReleaseDescriptorRange(DescriptorRange, uint64_t) = 0;
		virtual void Release() = 0;
	};


	/************************************************************************************************/

	void			MoveBuffer2UploadBuffer(const UploadReservation& data, const std::byte* source, const size_t uploadSize);

	ResourceHandle	MoveTextureBufferToVRAM(IRenderSystem& RS, CopyContextHandle, TextureBuffer* buffer, DeviceFormat format);
	ResourceHandle	MoveTextureBuffersToVRAM(IRenderSystem& RS, CopyContextHandle, TextureBuffer* buffer, size_t MIPCount, size_t arrayCount, DeviceFormat format);
	ResourceHandle	MoveTextureBuffersToVRAM(IRenderSystem& RS, CopyContextHandle, TextureBuffer* buffer, size_t MIPCount, DeviceFormat format);
	ResourceHandle	MoveBufferToDevice(IRenderSystem& RS, const char* buffer, const size_t, CopyContextHandle ctx = InvalidHandle);

	void			UpdateSubResourceByUploadQueue(IRenderSystem& RS, CopyContextHandle uploadHandle, ResourceHandle dstResource, SubResourceUpload_Desc* desc);


	/************************************************************************************************/


	inline bool CheckCompatibleLayout(const DeviceLayout currentLayout, const DeviceLayout requestedLayout)
	{
		//switch (currentLayout)
		//{
		//default:
		return currentLayout == requestedLayout;
		//}
	}


	inline bool CheckCompatibleAccessState(const DeviceLayout layout, const DeviceAccessState access)
	{
		switch (layout)
		{
		case DeviceLayout::Common:
			return (access & DASReadFlag);
		case DeviceLayout::Present:
			return (access & DASReadFlag);
		case DeviceLayout::GenericRead:
			return (access & DASReadFlag);
		case DeviceLayout::RenderTarget:
			return (access & DASRenderTarget);
		case DeviceLayout::UnorderedAccess:
			return (access & DASUAV);
		case DeviceLayout::DepthStencilWrite:
			return (access & DASDEPTHBUFFERWRITE);
		case DeviceLayout::DepthStencilRead:
			return (access & DASDEPTHBUFFERREAD);
		case DeviceLayout::ShaderResource:
			return (access & DASPixelShaderResource);
		case DeviceLayout::CopySrc:
			return (access & DASCopySrc);
		case DeviceLayout::CopyDst:
			return (access & DASCopyDest);
		case DeviceLayout::ResolveSrc:
		case DeviceLayout::ResolveDst:
			return true;
		case DeviceLayout::ShadingRateSrc:
			return (access & DASShadingRateSrc);
		case DeviceLayout::VideoDecodeRead:
			return (access & DASShadingRateSrc);
		case DeviceLayout::DecodeWrite:
		case DeviceLayout::ProcessRead:
		case DeviceLayout::ProcessWrite:
		case DeviceLayout::EncodeRead:
		case DeviceLayout::EncodeWrite:
		case DeviceLayout::DirectQueueCommon:
		case DeviceLayout::DirectQueueGenericRead:
		case DeviceLayout::DirectQueueUnorderedAccess:
		case DeviceLayout::DirectQueueShaderResource:
		case DeviceLayout::DirectQueueCopySrc:
		case DeviceLayout::DirectQueueCopyDst:
		case DeviceLayout::ComputeQueueCommon:
		case DeviceLayout::ComputeQueueGenericRead:
		case DeviceLayout::ComputeQueueUnorderedAccess:
		case DeviceLayout::ComputeQueueShaderResource:
		case DeviceLayout::ComputeQueueCopySrc:
		case DeviceLayout::ComputeQueueCopyDst:
		case DeviceLayout::VideoQueueCommon:
		default:
			return false;
		}

		std::unreachable();
	}

	inline bool IsReadAccessState(const DeviceAccessState access)
	{
		return access & DASReadFlag;
	}

	inline bool IsWriteAccessState(const DeviceAccessState access)
	{
		return access & DASWriteFlag;
	}


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

		void Release_Delayed(IRenderSystem* RS)
		{
			for (auto& r : Resources)
				if (r) Push_DelayedRelease(RS, r);
		}


		void _SetDebugName(const char* _str) {
			size_t Str_len = strnlen(_str, 64);
			for (auto& r : Resources) {
				SetDebugName(r, _str, Str_len);
			}
		}
	};

	
	/************************************************************************************************/


	template<typename TY_Signature>
	class RunOnceQueue
	{
	public:
		using Callable_TY = FlexKit::TypeErasedCallable<TY_Signature>;

		RunOnceQueue(iAllocator& allocator) : events{ &allocator } {}
		~RunOnceQueue() = default;

		RunOnceQueue(const RunOnceQueue&)   = delete;
		RunOnceQueue(RunOnceQueue&&)        = default;

		RunOnceQueue& operator = (const RunOnceQueue&)  = delete;
		RunOnceQueue& operator = (RunOnceQueue&&)       = default;


		void push_back(Callable_TY&& runOnce)
		{
			events.push_back(std::move(runOnce));
		}


		template<typename ... TY_Args>
		void Process(TY_Args&& ... args) requires std::is_invocable_v<Callable_TY, TY_Args...>
		{
			for (auto& evt : events)
				evt(std::forward<TY_Args>(args)...);

			events.clear();
		}
	private:
		Vector<Callable_TY> events;
	};


	/************************************************************************************************/
	// Mischellous Utilities


	struct LineSegment
	{
		float3 A;
		float3 AColour;
		float3 B;
		float3 BColour;
	};

	typedef Vector<LineSegment> LineSegments;

	inline float2 WStoSS(const float2 XY)		{ return (XY * float2(2, 2)) + float2(-1, -1); }
	inline float2 Position2SS(const float2 in)	{ return{ in.x * 2 - 1, in.y * -2 + 1 }; }
	inline float3 Grey(const float P)			{ float V = Min(Max(0, P), 1); return float3(V, V, V); }

	inline float2 PixelToSS(size_t X, size_t Y, uint2 Dimensions)	{ return { -1.0f + 2 * (float(X) / Dimensions[0]), 1.0f - 2 * (float(Y) / Dimensions[1]) }; } // Assumes screen boundaries are -1 and 1
	inline float2 PixelToSS(int2 XY, uint2 Dimensions)				{ return { -1.0f + 2 * (float(XY[0]) / Dimensions[0]), 1.0f - 2 * (float(XY[1]) / Dimensions[1]) }; } // Assumes screen boundaries are -1 and 1


	inline uint2 GetFormatTileSize(DeviceFormat format)
	{
		//switch (format)
		//{
		//default:
		return { 256, 256 };
		//}
	}


	/************************************************************************************************/


	struct UniqueResourceHandle : NoCopy
	{
		UniqueResourceHandle(ResourceHandle IN_handle = InvalidHandle);

		UniqueResourceHandle(UniqueResourceHandle&& IN_handle);
		UniqueResourceHandle& operator = (UniqueResourceHandle&&);

		~UniqueResourceHandle();

		operator ResourceHandle() const noexcept;
		operator bool() const noexcept;

	    ResourceHandle Get() const noexcept;
		ResourceHandle handle;
	};


}	/************************************************************************************************/


/**********************************************************************

Copyright (c) 2025 Robert May

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

