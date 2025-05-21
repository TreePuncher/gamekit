#pragma once
#include "BuildSettings.hpp"
#include "Containers.hpp"
#include "MathUtilities.hpp"
#include "ResourceHandles.hpp"


#if USING(ENABLEDX12)
struct ID3D12Resource;
struct ID3D12Fence;
struct ID3D12Heap;
struct ID3D12PipelineState;
#endif


namespace FlexKit
{
	struct iAllocator;


#if USING(ENABLEDX12)
	using DeviceResource_ptr		= ID3D12Resource*;
	using DeviceFence_ptr			= ID3D12Fence*;
	using DeviceHeap_ptr			= ID3D12Heap*;
	using DevicePipelineState_ptr	= ID3D12PipelineState*;
#endif


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
		SOLID = 3
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
		R16G16_UINT,
		R32_UINT,
		R32G32_UINT,
		R8G8B8A_UINT,
		R8G8B8A8_UINT,
		R8G8B8A8_UNORM,
		R8G8B8A8_UNORM_SRGB,
		R16G16B16A16_UNORM,
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


	enum PIPELINE_DESTINATION : unsigned char
	{
		PIPELINE_DEST_NONE = 0x00,
		PIPELINE_DEST_IA = 0x01,
		PIPELINE_DEST_HS = 0x02,
		PIPELINE_DEST_GS = 0x04,
		PIPELINE_DEST_VS = 0x08,
		PIPELINE_DEST_PS = 0x10,
		PIPELINE_DEST_CS = 0x20,
		PIPELINE_DEST_OM = 0x30,
		PIPELINE_DEST_DS = 0x40,
		PIPELINE_DEST_AS = 0x40,
		PIPELINE_DEST_MS = 0x50,

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
		UINT,
		Error
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

	
	enum class PredicateOp
	{
		EqualZero,
		NotEqualZero
	};


	enum DeviceAccessState
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
		DASERROR					= 0xFFFF ,
		DASUNKNOWN					= 0x0040,
	};


	enum DeviceSyncPoint
	{
		Sync_None,
		Sync_Auto,
		Sync_All,
		Sync_Draw,
		Sync_Compute,

		Sync_VertexShader,
		Sync_PixelShader,
		Sync_DepthStencil,
		Sync_RenderTarget,
		Sync_Raytracing,
		Sync_Copy,
		Sync_Resolve,
		Sync_ExecuteIndirect,
		Sync_Predication,
		Sync_All_Shading,
		Sync_NonPixelShading,
		Sync_EmitRaytracingAccellerationStructurePostBuildInfo,
		Sync_VideoDecode,
		Sync_VideoProcess,
		Sync_VideoEncode,
		Sync_BuildRaytracingAccellerationStructure,
		Sync_CopyRaytracingAccellerationStructure,
		Sync_Split,

		Sync_Unknown
	};

	
	enum DeviceLayout
	{
		DeviceLayout_Common,
		DeviceLayout_Present,
		DeviceLayout_GenericRead,
		DeviceLayout_RenderTarget,
		DeviceLayout_UnorderedAccess,
		DeviceLayout_DepthStencilWrite,
		DeviceLayout_DepthStencilRead,
		DeviceLayout_ShaderResource,
		DeviceLayout_CopySrc,
		DeviceLayout_CopyDst,
		DeviceLayout_ResolveSrc,
		DeviceLayout_ResolveDst,
		DeviceLayout_ShadingRateSrc,
		DeviceLayout_VideoDecodeRead,
		DeviceLayout_DecodeWrite,
		DeviceLayout_ProcessRead,
		DeviceLayout_ProcessWrite,
		DeviceLayout_EncodeRead,
		DeviceLayout_EncodeWrite,
		DeviceLayout_DirectQueueCommon,
		DeviceLayout_DirectQueueGenericRead,
		DeviceLayout_DirectQueueUnorderedAccess,
		DeviceLayout_DirectQueueShaderResource,
		DeviceLayout_DirectQueueCopySrc,
		DeviceLayout_DirectQueueCopyDst,
		DeviceLayout_ComputeQueueCommon,
		DeviceLayout_ComputeQueueGenericRead,
		DeviceLayout_ComputeQueueUnorderedAccess,
		DeviceLayout_ComputeQueueShaderResource,
		DeviceLayout_ComputeQueueCopySrc,
		DeviceLayout_ComputeQueueCopyDst,
		DeviceLayout_VideoQueueCommon,
		DeviceLayout_Undefined,

		DeviceLayout_Unknown = 0xffffffff,
	};


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

	enum struct ResourceHeapTier
	{
		HeapTier1,
		HeapTier2,
	};


	namespace DeviceHeapFlags
	{
		enum DeviceHeapFlagEnums: uint32_t
		{
			NONE			= 0,
			RenderTarget	= 1,
			UAVBuffer		= 2,
			UAVTextures		= 4,
			ALL				= 0xff
		};
	}


	enum class ResourceAllocationType
	{
		Committed,
		Placed,
		Tiled
	};


	enum class ResourceType
	{
		RenderTarget,
		DepthTarget,
		UnorderedAccess,
		UnorderedAccessRenderTarget,
		ShaderResource,
		RayTracingStructure,
	};


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
		Unknown,
	};


	enum class QueryType
	{
		OcclusionQuery,
		BinaryOcclusionQuery,
		PipelineStats,
		TimeStats,
	};

	
	enum IndirectLayoutEntryType
	{
		ILE_DrawCall,
		ILE_DrawIndexedCall,
		ILE_DispatchCall,
		ILE_DispatchMesh,
		ILE_DispatchRays,
		ILE_UpdateVBBindings,
		ILE_RootDescriptorUINT,
		ILE_UNKNOWN,
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
				(UINT)GetMipLevel()
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
			struct Texture
			{
				DeviceLayout layoutBefore;
				DeviceLayout layoutAfter;

				uint32_t				flags;
				BarrierSubResourceRange	range;
			} texture;

			struct Buffer
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
			struct ::ID3D12Resource*	_ptr;
		};

		enum class ResourceType
		{
			PTR,
			HNDL
		} type;

		DeviceAccessState beforeState;
		DeviceAccessState afterState;
	};

	
	struct DeviceResourceRange
	{
		uint64_t gpuBegin;
		uint64_t size;
	};


	enum class DeviceVendor
	{
		AMD,
		NVIDIA,
		INTEL,
		UNKNOWN
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


	struct EInputElement
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
		EInputElement	inputs[16];
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


	struct TextureObject
	{
		TextureObject(ResourceHandle IN_texture) :
			Texture     { IN_texture    },
			UAV         { false         } {}

		ResourceHandle Texture;

		const bool UAV = false;
	};


	/************************************************************************************************/


	struct VertexBufferEntry
	{
		VertexBufferHandle	VertexBuffer	= InvalidHandle;
		UINT				Stride			= 0;
		UINT				Offset			= 0;
	};

	typedef static_vector<VertexBufferEntry, 16>	VertexBufferList;
	typedef static_vector<ResourceHandle, 16>       RenderTargetList;


	struct VertexBufferResource
	{
		ResourceHandle		resource = InvalidHandle;
		UINT				stride = 0;
		UINT				offset = 0;
	};

	class IndirectDrawDescription
	{
	public:
		struct Constant
		{
			uint32_t rootParameterIdx;
			uint32_t destinationOffset;
			uint32_t numValues;
		};

		IndirectDrawDescription(IndirectLayoutEntryType IN_type = ILE_UNKNOWN) : type{ IN_type } {}
		IndirectDrawDescription(Constant IN_constant) : type{ ILE_RootDescriptorUINT }, description{ IN_constant } {}

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

		static BlendState Default() { return {}; }
		static BlendState Blend();
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


	struct Viewport
	{
		size_t X, Y, Height, Width;
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


	struct GPUResourceDesc
	{
		ResourceType			type;
		TextureDimension		Dimensions		= TextureDimension::Texture2D;
		ResourceAllocationType	allocationType	= ResourceAllocationType::Committed;
		DeviceFormat			format;
		DeviceLayout			initialLayout	= DeviceLayout_Common;

		// Dimensions
		uint2					WH;
		uint8_t					arraySize		= 1;
		uint8_t					bufferCount		= 1;
		uint8_t					MipLevels		= 1;

		bool					backBuffer		= false;
		bool					PreCreated		= false;
		bool					denyShaderUsage = false;

		std::optional<ClearValue>	clearValue;

		union
		{
			struct
			{
				ID3D12Resource**	resources;
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
				.MipLevels		= 1,

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
				.initialLayout	= DeviceLayout_DepthStencilWrite,

				.WH				= IN_WH,
				.arraySize		= arraySize,
				.bufferCount	= 3,
				.MipLevels		= 1,

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
				.MipLevels		= mipCount,
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
				.MipLevels		= mipCount,
			};
		}


		static GPUResourceDesc StructuredResource(const uint32_t bufferSize)
		{
			return GPUResourceDesc{
				.type			= ResourceType::ShaderResource,
				.Dimensions		= TextureDimension::Buffer,
				.allocationType = ResourceAllocationType::Committed,
				.format			= DeviceFormat::UNKNOWN,
				.initialLayout	= DeviceLayout_Undefined,

				.WH				= { bufferSize, 1 },
				.arraySize		= 1,
				.bufferCount	= 1,
				.MipLevels		= 1,
			};	
		}


		static GPUResourceDesc RayTracingStructure(const size_t bufferSize)
		{
			GPUResourceDesc desc{
				.type			= ResourceType::RayTracingStructure,
				.Dimensions		= TextureDimension::Buffer, // dimensions
				.allocationType = ResourceAllocationType::Committed,
				.format			= DeviceFormat::UNKNOWN,
				.initialLayout	= DeviceLayout_Undefined,

				.WH				= uint2{ (uint32_t)bufferSize, 1 },
				.bufferCount	= 1,
				.MipLevels		= 1,
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
				.initialLayout	= DeviceLayout_Undefined,


				.WH				= uint2{ (uint32_t)bufferSize, 1 },
				.bufferCount	= (uint8_t)bufferCount,
				.MipLevels		= 1,
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
				.MipLevels		= 1,
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
				.MipLevels		= (uint8_t)mipCount,

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
				.MipLevels		= (uint8_t)mipCount,
			};
		}

		static GPUResourceDesc BackBuffered(uint2 WH, DeviceFormat format, ID3D12Resource** sources, const uint8_t resourceCount)
		{
			GPUResourceDesc desc = {
				.type			= ResourceType::UnorderedAccess,
				.Dimensions		= TextureDimension::Texture2D,
				.allocationType = ResourceAllocationType::Committed,
				.format			= format,
				.initialLayout	= DeviceLayout_Present,


				.WH				= WH,
				.bufferCount	= resourceCount,
				.MipLevels		= 1,

				.backBuffer		= true,
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
				.MipLevels		= mipCount,
			};
		}


		static GPUResourceDesc BuildFromMemory(const GPUResourceDesc& format, ID3D12Resource** sources, const uint32_t resourceCount)
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
				.MipLevels		= mipCount,
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
				.MipLevels		= mipCount,
				.clearValue		= ClearValue{
									.format = format,
									.color = { 0.0f, 0.0f, 0.0f, 0.0f } }
			};
		}
	};


	/************************************************************************************************/


	struct SyncPoint
	{
		uint64_t		syncCounter = 0;
		DeviceFence_ptr	fence = nullptr;

		operator bool() const noexcept { return fence != nullptr; }
	};


	/************************************************************************************************/


	struct IRenderWindow
	{
		virtual ~IRenderWindow() {};

		virtual ResourceHandle      GetBackBuffer() const = 0;
		virtual uint2               GetWH() const = 0;

		virtual bool                Present(const uint32_t syncInternal = 0, const uint32_t flags = 0) = 0;
		virtual void                Resize(const uint2 WH) = 0;

		//virtual IDXGISwapChain4*    _GetSwapChain() const = 0;

		operator ResourceHandle () { return GetBackBuffer(); }

		float2  GetPixelSize() const	{ return float2{ 1.0f, 1.0f } / GetWH(); }
		float   GetAspectRatio() const	{ const auto WH = GetWH(); return float(WH[0]) / float(WH[1]); }
	};


	struct IIndirectLayout
	{
	};


	struct IPipelineBuilder
	{
	};


	struct IDirectContext
	{
	};


	struct ICopyContext
	{
	};


	struct IRenderSystem
	{
		virtual SubAllocation ReserveConstantBuffer	(ConstantBufferHandle CB, size_t reserveSize)	noexcept	= 0;
		virtual SubAllocation ReserveVertexBuffer	(VertexBufferHandle CB, size_t reserveSize)		noexcept	= 0;
	};


	struct IRootSignature
	{
	};


	struct IDescriptorHeap
	{
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

		void Release_Delayed(IRenderSystem* RS)
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


	struct HeapDescriptor
	{
		uint32_t				Register = -1;
		uint32_t				Count = 0;
		uint32_t				Space = 0;
		DescHeapEntryType		Type = DescHeapEntryType::HeapError;
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
			static_assert(ENTRYCOUNT >= RHS_SIZE);

			Entries = RHS.Entries;

#ifdef _DEBUG
			Check();
#endif
		}


		bool SetParameterAsCBV(
			uint32_t Index, uint32_t BaseRegister, uint32_t RegisterCount, uint32_t RegisterSpace = 0)
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
			uint32_t Index, uint32_t BaseRegister, uint32_t RegisterCount, uint32_t RegisterSpace = 0)
		{
			HeapDescriptor Desc;
			Desc.Register	= uint32_t(BaseRegister);
			Desc.Space		= uint32_t(RegisterSpace);
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
			uint32_t Index, uint32_t BaseRegister, uint32_t RegisterCount, uint32_t RegisterSpace = 0)
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

