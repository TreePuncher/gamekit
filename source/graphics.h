#pragma once
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

#include "PipelineState.h"

#include "..\PCH.h"
#include "buildsettings.h"
#include "containers.h"
#include "Handle.h"
#include "intersection.h"
#include "Logging.h"
#include "mathutils.h"
#include "Transforms.h"
#include "type.h"
#include "KeycodesEnums.h"
#include "ThreadUtilities.h"
#include "Geometry.h"
#include "TextureUtilities.h"

#include <algorithm>
#include <concepts>
#include <string>
#include <directx/d3d12.h>
#include <directx/d3d12sdklayers.h>
#include <DirectXMath/DirectXMath.h>
#include <dxgi1_6.h>
#include <concepts>
#include <expected>
#include <tuple>
#include <variant>
#include <optional>
#include <directx-dxc/dxcapi.h>

#if USING(AFTERMATH)

#include "..\ThirdParty\aftermath\include\GFSDK_Aftermath.h"
#include "..\ThirdParty\aftermath\include\GFSDK_Aftermath_GpuCrashDump.h"
#include "..\ThirdParty\aftermath\include\GFSDK_Aftermath_GpuCrashDumpDecoding.h"

#pragma comment( lib, "..\\ThirdParty\\aftermath\\lib\\x64\\GFSDK_Aftermath_Lib.x64.lib")

#endif

#if USING(PIX)
#define USE_PIX

#include <pix3.h>
#include <DXProgrammableCapture.h>

#endif


struct ID3D12Device9;
struct ID3D12Debug5;
struct ID3D12DebugDevice1;


namespace FlexKit
{   // Forward Declarations
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
		Sync_All,
		Sync_Draw,
		Sync_Compute,

		Sync_InputAssmebler,
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


	/************************************************************************************************/


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

	/************************************************************************************************/


	inline constexpr D3D12_RESOURCE_STATES DRS2D3DState(DeviceAccessState state)
	{
		switch (state)
		{
		case DeviceAccessState::DASPresent:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT;
		case DeviceAccessState::DASRenderTarget:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
		case DeviceAccessState::DASPixelShaderResource:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		case DeviceAccessState::DASUAV:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		case DeviceAccessState::DASCopyDest:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		case DeviceAccessState::DASCopySrc:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE;
		case DeviceAccessState::DASVERTEXBUFFER:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		case DeviceAccessState::DASINDIRECTARGS:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		case DeviceAccessState::DASSTREAMOUT:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_STREAM_OUT;
		case DeviceAccessState::DASDEPTHBUFFERWRITE:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
		case DeviceAccessState::DASDEPTHBUFFERREAD:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ;
		case DeviceAccessState::DASNonPixelShaderResource:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		case DeviceAccessState::DASINDEXBUFFER:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDEX_BUFFER;
		case DeviceAccessState::DASGenericRead:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ;
		case DeviceAccessState::DASACCELERATIONSTRUCTURE_READ:
		case DeviceAccessState::DASACCELERATIONSTRUCTURE_WRITE:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		case DeviceAccessState::DASNOACCESS:
		case DeviceAccessState::DASUNKNOWN:
			FK_ASSERT(0);
		}

		return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
	}


	/************************************************************************************************/


	inline constexpr DeviceAccessState D3DState2DRS(D3D12_RESOURCE_STATES state)
	{
		switch (state)
		{
		case D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ:
			return DeviceAccessState::DASGenericRead;
		case D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT:
			return DeviceAccessState::DASPresent;
		case D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET:
			return DeviceAccessState::DASRenderTarget;
		case D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
			return DeviceAccessState::DASPixelShaderResource;
		case D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS:
			return DeviceAccessState::DASUAV;
		case D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST:
			return DeviceAccessState::DASCopyDest;
		case D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE:
			return DeviceAccessState::DASCopySrc;
		case D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER:
			return DeviceAccessState::DASVERTEXBUFFER;
		case D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
			return DeviceAccessState::DASNonPixelShaderResource;
		case D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDEX_BUFFER:
			return DeviceAccessState::DASINDEXBUFFER;
		case D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE:
			return DeviceAccessState::DASACCELERATIONSTRUCTURE_READ;
		}

		FK_ASSERT(0);
		return DeviceAccessState::DASERROR;
	}


	/************************************************************************************************/


	inline constexpr D3D12_BARRIER_ACCESS DAS2AccessState(DeviceAccessState state)
	{
		switch (state)
		{
		case DeviceAccessState::DASPresent:
			return D3D12_BARRIER_ACCESS::D3D12_BARRIER_ACCESS_COMMON;
		case DeviceAccessState::DASRenderTarget:
			return D3D12_BARRIER_ACCESS::D3D12_BARRIER_ACCESS_RENDER_TARGET;
		case DeviceAccessState::DASPixelShaderResource:
			return D3D12_BARRIER_ACCESS::D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
		case DeviceAccessState::DASUAV:
			return D3D12_BARRIER_ACCESS::D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
		case DeviceAccessState::DASCopyDest:
			return D3D12_BARRIER_ACCESS::D3D12_BARRIER_ACCESS_COPY_DEST;
		case DeviceAccessState::DASCopySrc:
			return D3D12_BARRIER_ACCESS::D3D12_BARRIER_ACCESS_COPY_SOURCE;
		case DeviceAccessState::DASVERTEXBUFFER:
			return D3D12_BARRIER_ACCESS::D3D12_BARRIER_ACCESS_VERTEX_BUFFER;
		case DeviceAccessState::DASINDIRECTARGS:
			return D3D12_BARRIER_ACCESS::D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT;
		case DeviceAccessState::DASSTREAMOUT:
			return D3D12_BARRIER_ACCESS::D3D12_BARRIER_ACCESS_STREAM_OUTPUT;
		case DeviceAccessState::DASDEPTHBUFFERWRITE:
			return D3D12_BARRIER_ACCESS::D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE;
		case DeviceAccessState::DASDEPTHBUFFERREAD:
			return D3D12_BARRIER_ACCESS::D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ;
		case DeviceAccessState::DASNonPixelShaderResource:
			return D3D12_BARRIER_ACCESS::D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
		case DeviceAccessState::DASINDEXBUFFER:
			return D3D12_BARRIER_ACCESS::D3D12_BARRIER_ACCESS_INDEX_BUFFER;
		case DeviceAccessState::DASACCELERATIONSTRUCTURE_READ:
			return D3D12_BARRIER_ACCESS::D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ;
		case DeviceAccessState::DASACCELERATIONSTRUCTURE_WRITE:
			return D3D12_BARRIER_ACCESS::D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;
		case DeviceAccessState::DASNOACCESS:
			return D3D12_BARRIER_ACCESS::D3D12_BARRIER_ACCESS_NO_ACCESS;
		case DeviceAccessState::DASUNKNOWN:
			FK_ASSERT(0);
			return D3D12_BARRIER_ACCESS::D3D12_BARRIER_ACCESS_COMMON;
		}

		return D3D12_BARRIER_ACCESS::D3D12_BARRIER_ACCESS_COMMON;
	}


	/************************************************************************************************/


	inline constexpr D3D12_BARRIER_LAYOUT DeviceLayout2DX(const DeviceLayout layout)
	{
		switch (layout)
		{
		case DeviceLayout_Common:
			return D3D12_BARRIER_LAYOUT_COMMON;
		case DeviceLayout_Present:
			return D3D12_BARRIER_LAYOUT_PRESENT;
		case DeviceLayout_GenericRead:
			return D3D12_BARRIER_LAYOUT_GENERIC_READ;
		case DeviceLayout_RenderTarget:
			return D3D12_BARRIER_LAYOUT_RENDER_TARGET;
		case DeviceLayout_UnorderedAccess:
			return D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;
		case DeviceLayout_DepthStencilWrite:
			return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;
		case DeviceLayout_DepthStencilRead:
			return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ;
		case DeviceLayout_ShaderResource:
			return D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
		case DeviceLayout_CopySrc:
			return D3D12_BARRIER_LAYOUT_COPY_SOURCE;
		case DeviceLayout_CopyDst:
			return D3D12_BARRIER_LAYOUT_COPY_DEST;
		case DeviceLayout_ResolveSrc:
			return D3D12_BARRIER_LAYOUT_RESOLVE_SOURCE;
		case DeviceLayout_ResolveDst:
			return D3D12_BARRIER_LAYOUT_RESOLVE_DEST;
		case DeviceLayout_ShadingRateSrc:
			return D3D12_BARRIER_LAYOUT_SHADING_RATE_SOURCE;
		case DeviceLayout_VideoDecodeRead:
			return D3D12_BARRIER_LAYOUT_VIDEO_DECODE_READ;
		case DeviceLayout_DecodeWrite:
			return D3D12_BARRIER_LAYOUT_VIDEO_DECODE_WRITE;
		case DeviceLayout_ProcessRead:
			return D3D12_BARRIER_LAYOUT_VIDEO_PROCESS_READ;
		case DeviceLayout_ProcessWrite:
			return D3D12_BARRIER_LAYOUT_VIDEO_PROCESS_WRITE;
		case DeviceLayout_EncodeRead:
			return D3D12_BARRIER_LAYOUT_VIDEO_ENCODE_READ;
		case DeviceLayout_EncodeWrite:
			return D3D12_BARRIER_LAYOUT_VIDEO_ENCODE_WRITE;
		case DeviceLayout_DirectQueueCommon:
			return D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COMMON;
		case DeviceLayout_DirectQueueGenericRead:
			return D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_GENERIC_READ;
		case DeviceLayout_DirectQueueUnorderedAccess:
			return D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS;
		case DeviceLayout_DirectQueueShaderResource:
			return D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE;
		case DeviceLayout_DirectQueueCopySrc:
			return D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_SOURCE;
		case DeviceLayout_DirectQueueCopyDst:
			return D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_DEST;
		case DeviceLayout_ComputeQueueCommon:
			return D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COMMON;
		case DeviceLayout_ComputeQueueGenericRead:
			return D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_GENERIC_READ;
		case DeviceLayout_ComputeQueueUnorderedAccess:
			return D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_UNORDERED_ACCESS;
		case DeviceLayout_ComputeQueueShaderResource:
			return D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_SHADER_RESOURCE;
		case DeviceLayout_ComputeQueueCopySrc:
			return D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COPY_SOURCE;
		case DeviceLayout_ComputeQueueCopyDst:
			return D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COPY_DEST;
		case DeviceLayout_VideoQueueCommon:
			return D3D12_BARRIER_LAYOUT_VIDEO_QUEUE_COMMON;
		case DeviceLayout_Undefined:
			return D3D12_BARRIER_LAYOUT_UNDEFINED;
		case DeviceLayout_Unknown:
		default:
			DebugBreak();
		};

		std::unreachable();
	}


	/************************************************************************************************/


	inline DeviceAccessState BarrierAccess2DRS(D3D12_BARRIER_ACCESS state)
	{
		FK_ASSERT(0);
		return DeviceAccessState::DASERROR;
	}

	/************************************************************************************************/


	inline D3D12_BARRIER_SYNC SyncPoint2DX(const DeviceSyncPoint syncPoint)
	{
		switch (syncPoint)
		{
			case Sync_None:
				return D3D12_BARRIER_SYNC_NONE;
			case Sync_All:
				return D3D12_BARRIER_SYNC_ALL;
			case Sync_Draw:
				return D3D12_BARRIER_SYNC_DRAW;
			case Sync_Compute:
				return D3D12_BARRIER_SYNC_COMPUTE_SHADING;
			case Sync_InputAssmebler:
				return D3D12_BARRIER_SYNC_INPUT_ASSEMBLER;
			case Sync_VertexShader:
				return D3D12_BARRIER_SYNC_VERTEX_SHADING;
			case Sync_PixelShader:
				return D3D12_BARRIER_SYNC_PIXEL_SHADING;
			case Sync_DepthStencil:
				return D3D12_BARRIER_SYNC_DEPTH_STENCIL;
			case Sync_RenderTarget:
				return D3D12_BARRIER_SYNC_RENDER_TARGET;
			case Sync_Raytracing:
				return D3D12_BARRIER_SYNC_RAYTRACING;
			case Sync_Copy:
				return D3D12_BARRIER_SYNC_COPY;
			case Sync_Resolve:
				return D3D12_BARRIER_SYNC_RESOLVE;
			case Sync_ExecuteIndirect:
				return D3D12_BARRIER_SYNC_EXECUTE_INDIRECT;
			case Sync_Predication:
				return D3D12_BARRIER_SYNC_PREDICATION;
			case Sync_All_Shading:
				return D3D12_BARRIER_SYNC_ALL_SHADING;
			case Sync_NonPixelShading:
				return D3D12_BARRIER_SYNC_NON_PIXEL_SHADING;
			case Sync_EmitRaytracingAccellerationStructurePostBuildInfo:
				return D3D12_BARRIER_SYNC_EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO;
			case Sync_VideoDecode:
				return D3D12_BARRIER_SYNC_VIDEO_DECODE;
			case Sync_VideoProcess:
				return D3D12_BARRIER_SYNC_VIDEO_PROCESS;
			case Sync_VideoEncode:
				return D3D12_BARRIER_SYNC_VIDEO_ENCODE;
			case Sync_BuildRaytracingAccellerationStructure:
				return D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
			case Sync_CopyRaytracingAccellerationStructure:
				return D3D12_BARRIER_SYNC_RAYTRACING;
			case Sync_Split:
				return D3D12_BARRIER_SYNC_SPLIT;
			case Sync_Unknown:
				return D3D12_BARRIER_SYNC_NONE;
		}

		std::unreachable();
	}


	inline bool CheckCompatibleAccessState(const DeviceLayout layout, const DeviceAccessState access)
	{
		switch (layout)
		{
		case DeviceLayout_Common:
			return (access & DASReadFlag);
		case DeviceLayout_Present:
			return (access & DASReadFlag);
		case DeviceLayout_GenericRead:
			return (access & DASReadFlag);
		case DeviceLayout_RenderTarget:
			return (access & DASRenderTarget);
		case DeviceLayout_UnorderedAccess:
			return (access & DASUAV);
		case DeviceLayout_DepthStencilWrite:
			return (access & DASDEPTHBUFFERWRITE);
		case DeviceLayout_DepthStencilRead:
			return (access & DASDEPTHBUFFERREAD);
		case DeviceLayout_ShaderResource:
			return (access & DASPixelShaderResource);
		case DeviceLayout_CopySrc:
			return (access & DASCopySrc);
		case DeviceLayout_CopyDst:
			return (access & DASCopyDest);
		case DeviceLayout_ResolveSrc:
		case DeviceLayout_ResolveDst:
			return true;
		case DeviceLayout_ShadingRateSrc:
			return (access & DASShadingRateSrc);
		case DeviceLayout_VideoDecodeRead:
			return (access & DASShadingRateSrc);
		case DeviceLayout_DecodeWrite:
		case DeviceLayout_ProcessRead:
		case DeviceLayout_ProcessWrite:
		case DeviceLayout_EncodeRead:
		case DeviceLayout_EncodeWrite:
		case DeviceLayout_DirectQueueCommon:
		case DeviceLayout_DirectQueueGenericRead:
		case DeviceLayout_DirectQueueUnorderedAccess:
		case DeviceLayout_DirectQueueShaderResource:
		case DeviceLayout_DirectQueueCopySrc:
		case DeviceLayout_DirectQueueCopyDst:
		case DeviceLayout_ComputeQueueCommon:
		case DeviceLayout_ComputeQueueGenericRead:
		case DeviceLayout_ComputeQueueUnorderedAccess:
		case DeviceLayout_ComputeQueueShaderResource:
		case DeviceLayout_ComputeQueueCopySrc:
		case DeviceLayout_ComputeQueueCopyDst:
		case DeviceLayout_VideoQueueCommon:
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
		case DXGI_FORMAT_R32G32B32_SINT:
			return sizeof(float) * 3;
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
			return sizeof(uint16_t[4]);
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
			return sizeof(float[2]);
		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
			return 4;
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
			return 2;
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
			return 4;
		case DXGI_FORMAT_R16G16_UINT:
			return sizeof(uint16_t[2]);
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return 16;
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:  
			return sizeof(float);
		case DXGI_FORMAT_UNKNOWN:
			return 1;
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
			return 4;
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
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
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
		case DXGI_FORMAT_BC5_TYPELESS:
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
			throw; // unimplemented argument
		default:
			throw; // invalid argument 
			return -1;
			break;
		}

		std::unreachable();
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
		PIPELINE_DEST_AS = 0x40,
		PIPELINE_DEST_MS = 0x50,

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

		std::unreachable();
	}


	/************************************************************************************************/


	constexpr DXGI_FORMAT   TextureFormat2DXGIFormat(DeviceFormat F) noexcept;
	DeviceFormat			DXGIFormat2TextureFormat(DXGI_FORMAT F) noexcept;


	/************************************************************************************************/


	struct DevicePointer
	{
		D3D12_GPU_VIRTUAL_ADDRESS address = { 0 };

		DevicePointer operator + (std::integral auto offset) const
		{
			return { address + offset };
		}

		DevicePointer operator - (std::integral auto offset) const
		{
			return { address - offset };
		}

		DevicePointer operator * (std::integral auto x) const
		{
			return { address * x };
		}

		operator D3D12_GPU_VIRTUAL_ADDRESS () const
		{
			return address;
		}
	};


	struct DeviceAddressRange
	{
		D3D12_GPU_VIRTUAL_ADDRESS_RANGE range   = { 0, 0 };

		operator D3D12_GPU_VIRTUAL_ADDRESS_RANGE () const
		{
			return range;
		}
	};


	struct DeviceAddressRangeStride
	{
		D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE rangeStride = { 0, 0, 0 };

		operator D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE () const
		{
			return rangeStride;
		}
	};


	/************************************************************************************************/


	using DescHeapPOS = Pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE>;
	using VertexResourceBuffer = ID3D12Resource*;


	/************************************************************************************************/


	struct DescriptorRange
	{
		FlexKit::DescHeapPOS	begin;
		uint32_t				size;
		uint32_t				stride;

		FlexKit::DescHeapPOS operator [](size_t idx)
		{
			return { begin.V1.ptr + idx * stride,  begin.V2.ptr + idx * stride, };
		}

		operator FlexKit::DescHeapPOS () const { return begin; }
	};


	/************************************************************************************************/


	class DescriptorHeapAllocator
	{
	public:
		~DescriptorHeapAllocator();


		void							Initialize	(FlexKit::RenderSystem& IN_renderSystem, const size_t numDescCount, FlexKit::iAllocator* IN_allocator);

		std::optional<DescriptorRange>	Alloc_ST	(const size_t size, uint64_t completedIdx) noexcept;
		auto							Alloc		(const size_t size, uint64_t completedIdx) noexcept;

		void							Release_ST	(const DescriptorRange range, uint64_t lockIdx, uint64_t completed) noexcept;
		void							Release	(const DescriptorRange range, uint64_t lockIdx, uint64_t completed);

		ID3D12DescriptorHeap* Heap() { return descHeap; }
		private:

		struct Node
		{
			size_t begin;
			size_t end;
			size_t lockUntil;
			
			bool free		= true;
			Node* left		= nullptr;
			Node* right		= nullptr;
			Node* parent	= nullptr;

			void Split(FlexKit::iAllocator* allocator);
			void Collapse(FlexKit::iAllocator* allocator);
			bool Collapsable(uint64_t completed);

			void Release(iAllocator* allocator);

			std::pair<size_t, size_t> SplitSizes();
			size_t BlockCount() const noexcept { return end - begin; }
			size_t FreeCount() const noexcept;

		};

		Node* LocateNode(size_t offset);

		Node						root;
		FlexKit::Vector<Node*>		freeList;
		FlexKit::iAllocator*		allocator;
		FlexKit::RenderSystem*		renderSystem;
		size_t						descriptorSize;
		D3D12_GPU_DESCRIPTOR_HANDLE	gpuHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE	cpuHeap;
		std::mutex					mutex;
		ID3D12DescriptorHeap*		descHeap;
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
		uint32_t		SlaveThreadCount;
		bool			Fullscreen						= false;
		bool			DX_DebugMode					= false;
		bool			DX_GPUvalidation				= false;
		bool			DX_SynchronizedQueueValidation	= false;
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
		UAVBuffer(const RenderSystem& rs, const ResourceHandle handle, const size_t stride = -1, const size_t offset = 0); // auto Fills the struct

		ID3D12Resource* resource;
		uint32_t		elementCount;
		uint32_t		counterOffset	= 0;
		uint32_t		offset			= 0;
		uint32_t		stride          = 1;
		DXGI_FORMAT		format          = DXGI_FORMAT_UNKNOWN;
		bool			typeless        = false;
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

	struct IRenderWindow
	{
		virtual ~IRenderWindow() {};

		virtual ResourceHandle      GetBackBuffer() const = 0;
		virtual bool                Present(const uint32_t syncInternal = 0, const uint32_t flags = 0) = 0;
		virtual uint2               GetWH() const = 0;
		virtual void                Resize(const uint2 WH) = 0;

		virtual IDXGISwapChain4*    _GetSwapChain() const = 0;

		operator ResourceHandle () { return GetBackBuffer(); }

		float2  GetPixelSize() const { return float2{ 1.0f, 1.0f } / GetWH(); }
		float   GetAspectRatio() const { const auto WH = GetWH(); return float(WH[0]) / float(WH[1]); }
	};

	/************************************************************************************************/


	enum EInputTopology
	{
		EIT_LINE,
		EIT_TRIANGLELIST,
		EIT_TRIANGLE,
		EIT_POINT,
		EIT_PATCH_CP_1,
		EIT_PATCH_CP_2,
		EIT_PATCH_CP_3,
		EIT_PATCH_CP_4,
		EIT_PATCH_CP_5,
		EIT_PATCH_CP_6,
		EIT_PATCH_CP_7,
		EIT_PATCH_CP_8,
		EIT_PATCH_CP_9,
		EIT_PATCH_CP_10,
		EIT_PATCH_CP_11,
		EIT_PATCH_CP_12,
		EIT_PATCH_CP_13,
		EIT_PATCH_CP_14,
		EIT_PATCH_CP_15,
		EIT_PATCH_CP_16,
		EIT_PATCH_CP_17,
		EIT_PATCH_CP_18,
		EIT_PATCH_CP_19,
		EIT_PATCH_CP_20,
		EIT_PATCH_CP_21,
		EIT_PATCH_CP_22,
		EIT_PATCH_CP_23,
		EIT_PATCH_CP_24,
		EIT_PATCH_CP_25,
		EIT_PATCH_CP_26,
		EIT_PATCH_CP_27,
		EIT_PATCH_CP_28,
		EIT_PATCH_CP_29,
		EIT_PATCH_CP_30,
		EIT_PATCH_CP_31,
		EIT_PATCH_CP_32,
	};


	/************************************************************************************************/


	typedef	Vector<ID3D12Resource*> TempResourceList;
	const static int QueueSize		= 3;

	struct DescHeapStack
	{
		~DescHeapStack()
		{
			Release();
		}


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

			size_t Size() const { return BufferSizeInBytes / BufferStride; }

			DevicePointer GetDevicePointer() { return { Buffer->GetGPUVirtualAddress() }; }

			operator bool()	{ return Buffer != nullptr; }
			operator ID3D12Resource* ()	{ return Buffer; }
		};

		auto Find(const VERTEXBUFFER_TYPE type)
		{
			return std::find_if(
				VertexBuffers.begin(),
				VertexBuffers.end(),
				[&](auto& buffer)
				{
					return buffer.Type == type;
				});
		}

		void clear() { VertexBuffers.clear(); }


		ID3D12Resource*	operator[](size_t idx)	{ return VertexBuffers[idx].Buffer; }
		static_vector<BuffEntry, 16>	VertexBuffers;
		TriangleMeshMetaData			MD;
	};


	/************************************************************************************************/


	struct UploadSegment
	{
		size_t				offset		= 0;
		size_t				uploadSize	= 0;
		char*				buffer		= nullptr;
		ID3D12Resource*		resource	= nullptr;
	};


	enum class ReserveErrors
	{
		Success,
		OutOfSpace,
		Unknown
	};

	struct UploadBuffer
	{
		UploadBuffer() = default;
		UploadBuffer(ID3D12Device* pDevice);

		UploadBuffer(UploadBuffer&&);
		UploadBuffer& operator = (UploadBuffer&&) noexcept;

						UploadBuffer(const UploadBuffer&) = delete;
		UploadBuffer&	operator =	(const UploadBuffer&) = delete;

		~UploadBuffer();

		void Release();

		std::expected<UploadSegment, ReserveErrors> Reserve(const size_t size, const size_t reserveAlignement);

		ID3D12Resource* Resize(const size_t size); // Returns old resource

		ID3D12Resource* deviceBuffer	= nullptr;
		size_t			Position		= 0;
		size_t			Last			= 0;
		size_t			Size			= 0;
		char*			Buffer			= nullptr;
		ID3D12Device*	parentDevice	= nullptr;
	};


	struct UploadReservation
	{
		ID3D12Resource*	resource		= nullptr;
		const size_t	reserveSize		= 0;
		const size_t	offset			= 0;
		char*			buffer			= 0;
	};

	struct PackedResourceTileInfo
	{
		size_t startingLevel;
		size_t endingLevel;
		size_t startingTileIndex;
	};

	struct SyncPoint
	{
		uint64_t		syncCounter	= 0;
		ID3D12Fence*	fence		= nullptr;
	};


	class CopyEngine;

	class CopyContext
	{
	public:

		void                Barrier(ID3D12Resource* destination, DeviceAccessState before, DeviceAccessState after);

		UploadReservation   Reserve(const size_t reserveSize, const size_t reserveAignement = 256);
		void                CopyBuffer(ID3D12Resource* destination, const size_t destinationOffset, UploadReservation);
		void                CopyBuffer(ID3D12Resource* destination, const size_t destinationOffset, ID3D12Resource* source, const size_t sourceOffset, const size_t copySize);
		void                CopyTextureRegion(ID3D12Resource*, size_t subResourceIdx, uint3 XYZ, UploadReservation source, uint2 WH, DeviceFormat format);
		void                CopyTile(ID3D12Resource* dest, const uint3 destTile, const size_t tileOffset, const UploadReservation src);

		PackedResourceTileInfo  GetPackedTileInfo(ID3D12Resource* resource) const;
		bool                    IsSubResourceTiled(ID3D12Resource* Resource, const size_t level) const;

		void                    flushPendingBarriers();

		ID3D12GraphicsCommandList* GetAPIObject() { return commandList; }

		ID3D12CommandAllocator*		commandAllocator    = nullptr;
		ID3D12GraphicsCommandList*	commandList         = nullptr;
		size_t                      counter             = 0;
		HANDLE                      eventHandle;

		UploadBuffer                uploadBuffer;

		Vector<ID3D12Resource*>                 freeResources;
		static_vector<D3D12_RESOURCE_BARRIER>   pendingBarriers;
	};


	class CopyEngine
	{
	public:
		CopyEngine() = default;

		CopyContext& operator [](CopyContextHandle handle);

		CopyContextHandle Open();

		void Wait(SyncPoint syncTo);
		void Wait(CopyContextHandle handle);
		void Close(CopyContextHandle handle);

		void		Submit(CopyContextHandle* begin, CopyContextHandle* end, std::optional<SyncPoint> sync = {});
		void		Signal(ID3D12Fence* fence, const size_t counter);
		void		Signal(SyncPoint);

		void Push_Temporary(ID3D12Resource* resource, CopyContextHandle handle);

		void Release();

		bool Initiate(ID3D12Device*, const size_t threadCount, Vector<ID3D12DeviceChild*>& ObjectsCreated, iAllocator*);

		ID3D12CommandQueue*					copyQueue		= nullptr;
		ID3D12Fence*						fence			= nullptr;
		std::atomic_uint					idx				= 0;
		std::atomic_uint					counter			= 0;
		CircularBuffer<CopyContext, 64>		copyContexts;
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
		UINT,
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
			static_assert(ENTRYCOUNT >= RHS_SIZE);

			Entries = RHS.Entries;

#ifdef _DEBUG
			Check();
#endif
		}

		bool SetParameterAsCBV(
			uint32_t Index, uint32_t BaseRegister, uint32_t RegisterCount, uint32_t RegisterSpace = 0,
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
			uint32_t Index, uint32_t BaseRegister, uint32_t RegisterCount, uint32_t RegisterSpace = 0,
			size_t BufferTag = -1)
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
			uint32_t Index, uint32_t BaseRegister, uint32_t RegisterCount, uint32_t RegisterSpace = 0,
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


	FLEXKITAPI class DescriptorHeap
	{
	public:
		DescriptorHeap() {}
		DescriptorHeap(Context& RS, const DesciptorHeapLayout<16>& Layout_IN, iAllocator* TempMemory);

		DescriptorHeap& operator = (const DescriptorHeap&);

		// moveable
		DescriptorHeap(DescriptorHeap&& rhs);
		DescriptorHeap& operator = (DescriptorHeap&&);

		DescriptorHeap& Init		(Context& ctx, const DesciptorHeapLayout<16>& Layout_IN, iAllocator* TempMemory);
		DescriptorHeap& Init		(Context& ctx, const DesciptorHeapLayout<16>& Layout_IN, const size_t reserveCount, iAllocator* TempMemory);
		DescriptorHeap& Init2       (Context& ctx, const DesciptorHeapLayout<16>& Layout_IN, const size_t reserveCount, iAllocator* TempMemory); // for variable size heap layouts
		DescriptorHeap& NullFill	(Context& ctx, const size_t end = -1);

		bool SetCBV					(Context& ctx, size_t idx, const ConstantBufferDataSet& constants);
		bool SetCBV					(Context& ctx, size_t idx, ConstantBufferHandle, size_t offset, size_t bufferSize);
		bool SetCBV                 (Context& ctx, size_t idx, ResourceHandle, size_t offset, size_t bufferSize);

		bool SetSRV					(Context& ctx, size_t idx, ResourceHandle);
		bool SetSRV					(Context& ctx, size_t idx, ResourceHandle, DeviceFormat format);
		bool SetSRV					(Context& ctx, size_t idx, ResourceHandle, uint MipOffset, DeviceFormat format);
		bool SetSRVArray			(Context& ctx, size_t idx, ResourceHandle, DeviceFormat format);

		bool SetSRV3D               (Context& ctx, size_t idx, ResourceHandle);

		//bool SetSRV					(Context& ctx, size_t idx, ResourceHandle		Handle);
		//bool SetSRV					(Context& ctx, size_t idx, ResourceHandle	Handle);
		bool SetSRVCubemap			(Context& ctx, size_t idx, ResourceHandle		Handle);
		bool SetSRVCubemap			(Context& ctx, size_t idx, ResourceHandle		Handle, DeviceFormat format);

		bool SetUAVBuffer			(Context& ctx, size_t idx, ResourceHandle, size_t   offset = 0);

		bool SetUAVTexture			(Context& ctx, size_t idx, ResourceHandle);

		bool SetUAVTexture			(Context& ctx, size_t idx, ResourceHandle, DeviceFormat format);
		bool SetUAVTexture			(Context& ctx, size_t idx, size_t mipLevel, ResourceHandle, DeviceFormat format);

		bool SetUAVCubemap			(Context& ctx, size_t idx, ResourceHandle handle);

		bool SetUAVTexture3D		(Context& ctx, size_t idx, ResourceHandle, DeviceFormat format);

		bool SetUAVStructured		(Context& ctx, size_t idx, ResourceHandle, size_t stride, size_t offset = 0);
		bool SetUAVStructured		(Context& ctx, size_t idx, ResourceHandle resource, ResourceHandle counter, size_t stride, size_t Offset);

		bool SetStructuredResource	(Context& ctx, size_t idx, ResourceHandle, size_t stride = 4, size_t offset = 0); //

		operator D3D12_GPU_DESCRIPTOR_HANDLE () const { return descriptorHeap.V2; } // TODO: FIX PAIRS SO AUTO CASTING WORKS

		DescriptorHeap	GetHeapOffsetted(size_t offset, Context& ctx) const;

		void Mirror(const DescriptorHeap& rhs)
		{
			descriptorHeap = rhs.descriptorHeap;
		}

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
			Signature   { nullptr },
			Heaps       { Memory } {}

		~RootSignature()
		{
			Release();
		}

		operator ID3D12RootSignature* () const { return Signature; }

		ID3D12RootSignature* Get_ptr() const { return Signature; };

		void Release()
		{
			if (Signature)
				Signature->Release();

			Signature = nullptr;
			Heaps.Release();
		}


		bool SetParameterAsUINT(size_t Index, uint32_t size, uint32_t cbRegister, uint32_t registerSpace, PIPELINE_DESTINATION AccessableStages = PIPELINE_DESTINATION::PIPELINE_DEST_ALL)
		{
			RootEntry Desc;
			Desc.Type							= RootSignatureEntryType::UINT;
			Desc.UINTConstant.size              = size;
			Desc.UINTConstant.Register		    = cbRegister;
			Desc.UINTConstant.RegisterSpace     = registerSpace;
			Desc.UINTConstant.Accessibility	    = AccessableStages;

			if (RootEntries.size() <= Index)
			{
				if (!RootEntries.full())
					RootEntries.resize(Index + 1);
				else
					return false;
			}

			RootEntries[Index]  = Desc;
			Tags[Index]         = -1;

			return true;
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
			size_t Index, size_t Register, size_t RegisterSpace = 0, 
			PIPELINE_DESTINATION AccessableStages = PIPELINE_DESTINATION::PIPELINE_DEST_ALL, size_t Tag = -1)
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
			size_t Index, size_t Register, size_t RegisterSpace = 0,
			PIPELINE_DESTINATION AccessableStages = PIPELINE_DESTINATION::PIPELINE_DEST_ALL, size_t Tag = -1)
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
				struct
				{
					uint32_t				HeapIdx;
					uint32_t				size;
					uint32_t				Register;
					uint32_t				RegisterSpace;
					PIPELINE_DESTINATION	Accessibility;
				}UINTConstant;

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


	/************************************************************************************************/


	enum IndirectLayoutEntryType
	{
		ILE_DrawCall,
		ILE_DispatchCall,
		ILE_UpdateVBBindings,
		ILE_RootDescriptorUINT,
		ILE_UNKNOWN,
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

		IndirectDrawDescription(IndirectLayoutEntryType IN_type = ILE_UNKNOWN) : type{IN_type} {}
		IndirectDrawDescription(IndirectLayoutEntryType IN_type, Constant IN_constant) : type{ IN_type }, description{ IN_constant } {}

		IndirectLayoutEntryType type;


		union 
		{
			Constant constantValue;
		} description = {};
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


	template<size_t I = 0, typename TY_Tuple, typename FN>
	void Tuple_For(TY_Tuple& tuple, FN fn)
	{
		constexpr size_t end = std::tuple_size_v<TY_Tuple>;

		if constexpr (I < end) {
			fn(std::get<I>(tuple));
			Tuple_For<I + 1>(tuple, fn);
		}
	}


	struct DepthStencilView_Options
	{
		size_t ArraySliceOffset = 0;
		size_t MipOffset        = 0;

		ResourceHandle depthStencil;

		size_t arraySize	= -1;
	};


	/************************************************************************************************/


	struct AccelerationStructureDesc
	{
	};


	struct DispatchDesc
	{
		DeviceAddressRangeStride     callableShaderTable;
		DeviceAddressRangeStride     hitGroupTable;
		DeviceAddressRangeStride     missTable;
		DeviceAddressRange           rayGenerationRecord;
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
		void				ReleaseVertexBuffer(VertexBufferHandle Handle, uint64_t current);
		void                ReleaseFree(uint64_t current);

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
			size_t			lockCounter;
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

		HandleUtilities::HandleTable<VertexBufferHandle>    Handles;

		std::mutex criticalSection;
	};


	/************************************************************************************************/


	FLEXKITAPI struct ConstantBufferTable
	{
		ConstantBufferTable(iAllocator* allocator, RenderSystem* IN_renderSystem) :
			handles			{ allocator			},
			renderSystem	{ IN_renderSystem	},
			buffers			{ allocator			}
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

			uint64_t				locks[3];
			uint8_t					currentRes;
			ConstantBufferHandle	handle;

			ID3D12Resource*			resources[3];
		};



		ConstantBufferHandle	CreateConstantBuffer	(uint32_t size, bool GPUResident = false);
		void					ReleaseBuffer			(ConstantBufferHandle);
		void					Reset					(ConstantBufferHandle);

		ID3D12Resource*			GetDeviceResource		(const ConstantBufferHandle) const;
		size_t					GetBufferOffset			(const ConstantBufferHandle) const;
		size_t					GetBufferSize			(const ConstantBufferHandle) const;


		size_t					AlignNext				(ConstantBufferHandle);
		std::optional<size_t>	Push					(ConstantBufferHandle, void* ptr, size_t size);
		
		SubAllocation			Reserve					(ConstantBufferHandle, size_t size);

	private:
		RenderSystem*				renderSystem;
		Vector<UserConstantBuffer>	buffers;
		std::mutex					criticalSection;

		HandleUtilities::HandleTable<ConstantBufferHandle>	handles;
	};


	/************************************************************************************************/


	enum class QueryType
	{
		OcclusionQuery,
		PipelineStats,
		TimeStats,
	};

	// WARNING, NOT IMPLEMENTED FULLY!
	FLEXKITAPI struct QueryTable
	{
		QueryTable(iAllocator* persistent, RenderSystem* RS_in) :
			users		{ persistent },
			resources	{ persistent },
			RS			{ RS_in } {}


		~QueryTable()
		{
			Release();
		}


		void Release()
		{
			for (auto& resource : resources)
			{
				for (auto queries : resource.resources)
					queries->Release();
			}

			resources.clear();
			users.clear();
		}

		QueryHandle			CreateQueryBuffer	(size_t Count, QueryType type);
		QueryHandle			CreateSOQueryBuffer	(size_t count, size_t SOIndex);
		void				LockUntil			(QueryHandle, size_t FrameID);


		void				SetUsed		(QueryHandle Handle)
		{
			users[Handle].used = true;
		}


		DeviceLayout GetLayout(const QueryHandle handle) const
		{
			return _GetLayout(handle);
		}

		DeviceLayout _GetLayout(const QueryHandle handle) const
		{
			auto& res = resources[users[handle].resourceIdx];
			return res.layouts[res.currentResource];
		}

		ID3D12QueryHeap*	GetDeviceObject(QueryHandle handle)
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
			DeviceLayout			layouts[3];
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


	inline D3D12_RTV_DIMENSION _Dimension2DeviceRTVDimension(TextureDimension dimension)
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
		ShaderResource
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

		std::optional<D3D12_CLEAR_VALUE>	clearValue;

		union
		{
			struct
			{
				ID3D12Resource**	resources;
				byte*				initial;
			};
		};

		struct
		{
			size_t				offset;
			DeviceHeapHandle	heap;
			DeviceAccessState	initialState;
			ID3D12Heap*			customHeap;
		} placed;


		size_t	byteSize;
		bool	rayTraceStructure = false;

		size_t CalculateByteSize() const
		{
			const auto dxgiFormat = TextureFormat2DXGIFormat(format);

			switch (Dimensions)
			{
			case TextureDimension::Buffer:
				return WH[0];
			case TextureDimension::Texture1D:
				return WH[0] * GetFormatElementSize(dxgiFormat);
			case TextureDimension::Texture2D:
				return WH.Product() * GetFormatElementSize(dxgiFormat);
			case TextureDimension::Texture3D:
				return WH.Product() * GetFormatElementSize(dxgiFormat) * arraySize;
			case TextureDimension::TextureCubeMap:
				return WH.Product() * GetFormatElementSize(dxgiFormat) * 6;
			};

			return -1;
		}

		D3D12_RESOURCE_DESC GetD3D12ResourceDesc() const
		{
			const auto dxgiFormat = TextureFormat2DXGIFormat(format);
			D3D12_RESOURCE_DESC desc;

			switch (Dimensions)
			{
			case TextureDimension::Buffer:
				desc = CD3DX12_RESOURCE_DESC::Buffer(WH[0]);

				break;
			case TextureDimension::Texture1D:
				desc = CD3DX12_RESOURCE_DESC::Tex1D(
					dxgiFormat,
					WH[0],
					arraySize,
					MipLevels);

				break;
			case TextureDimension::Texture2D:
				desc = CD3DX12_RESOURCE_DESC::Tex2D(
					dxgiFormat,
					WH[0],
					WH[1],
					arraySize,
					MipLevels);

				break;
			case TextureDimension::Texture3D:
				desc = CD3DX12_RESOURCE_DESC::Tex3D(
					dxgiFormat,
					WH[0],
					WH[1],
					arraySize,
					MipLevels);

				break;
			case TextureDimension::TextureCubeMap:
				desc = CD3DX12_RESOURCE_DESC::Tex2D(
					dxgiFormat,
					WH[0],
					WH[1],
					6,
					MipLevels);
				break;
			};

			desc.Flags = type	== ResourceType::RenderTarget					? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : D3D12_RESOURCE_FLAG_NONE;
			desc.Flags |= type	== ResourceType::UnorderedAccess				? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
			desc.Flags |= type	== ResourceType::UnorderedAccessRenderTarget	? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
			desc.Flags |= type	== ResourceType::DepthTarget					? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_NONE;

			desc.MipLevels = Min(Max(MipLevels, 1), 15);

			return desc;
		}

		D3D12_RESOURCE_DESC1 GetD3D12ResourceDesc1() const
		{
			const auto dxgiFormat = TextureFormat2DXGIFormat(format);
			D3D12_RESOURCE_DESC1 desc;

			switch (Dimensions)
			{
			case TextureDimension::Buffer:
				desc = CD3DX12_RESOURCE_DESC1::Buffer(WH[0]);

				break;
			case TextureDimension::Texture1D:
				desc = CD3DX12_RESOURCE_DESC1::Tex1D(
					dxgiFormat,
					WH[0],
					arraySize,
					MipLevels);

				break;
			case TextureDimension::Texture2D:
				desc = CD3DX12_RESOURCE_DESC1::Tex2D(
					dxgiFormat,
					WH[0],
					WH[1],
					arraySize,
					MipLevels);

				break;
			case TextureDimension::Texture3D:
				desc = CD3DX12_RESOURCE_DESC1::Tex3D(
					dxgiFormat,
					WH[0],
					WH[1],
					arraySize,
					MipLevels);

				break;
			case TextureDimension::TextureCubeMap:
				desc = CD3DX12_RESOURCE_DESC1::Tex2D(
					dxgiFormat,
					WH[0],
					WH[1],
					6,
					MipLevels);
				break;
			};

			desc.Flags = type == ResourceType::RenderTarget ? D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
			desc.Flags |= type == ResourceType::UnorderedAccess ? D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
			desc.Flags |= type == ResourceType::DepthTarget ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
			desc.Flags |= type == ResourceType::UnorderedAccessRenderTarget ? D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
			desc.Flags |= denyShaderUsage ? D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE : D3D12_RESOURCE_FLAG_NONE;

			desc.MipLevels	= Min(Max(MipLevels, 1), 15);

			return desc;
		}

		static GPUResourceDesc RenderTarget(uint2 IN_WH, DeviceFormat IN_format, const ResourceAllocationType allocationType = ResourceAllocationType::Committed)
		{
			return {
				.type			= ResourceType::RenderTarget,
				.Dimensions		= TextureDimension::Texture2D,
				.allocationType = allocationType,
				.format			= IN_format,
				.WH				= IN_WH,
				.bufferCount	= 3,
				.MipLevels		= 1,

				.clearValue		= { D3D12_CLEAR_VALUE{
										.Format = TextureFormat2DXGIFormat(IN_format),
										.Color{ 0.0f, 0.0f, 0.0f, 0.0f }}}
			};
		}


		static GPUResourceDesc DepthTarget(uint2 IN_WH, DeviceFormat IN_format, const uint8_t arraySize = 1, const ResourceAllocationType allocationType = ResourceAllocationType::Committed)
		{
			return {
				.type			= ResourceType::DepthTarget,
				.Dimensions		= TextureDimension::Texture2D,
				.allocationType = allocationType,
				.format			= IN_format,
				.initialLayout	= DeviceLayout_DepthStencilWrite,

				.WH				= IN_WH,
				.arraySize		= arraySize,
				.bufferCount	= 3,
				.MipLevels		= 1,

				.clearValue		= { D3D12_CLEAR_VALUE{
										.Format = TextureFormat2DXGIFormat(IN_format),
										.DepthStencil{ .Depth = 1.0f, .Stencil = 0xff }}}
			};
		}


		static GPUResourceDesc ShaderResource(uint2 IN_WH, DeviceFormat IN_format, uint8_t mipCount = 1, size_t arraySize = 1, const ResourceAllocationType allocationType = ResourceAllocationType::Committed)
		{
			return {
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
			return {
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
			return {
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
				.type			= ResourceType::UnorderedAccess,
				.Dimensions		= TextureDimension::Buffer, // dimensions
				.allocationType = ResourceAllocationType::Committed,
				.format			= DeviceFormat::UNKNOWN,
				.initialLayout	= DeviceLayout_Undefined,

				.WH				= uint2{ (uint32_t)bufferSize, 1 },
				.bufferCount	= 3,
				.MipLevels		= 1,
			};

			desc.rayTraceStructure = true;

			return desc;
		}

		static GPUResourceDesc UAVResource(const size_t bufferSize, DeviceFormat IN_format = DeviceFormat::UNKNOWN, bool renderTarget = false)
		{
			return {
				.type			= ResourceType::UnorderedAccess,
				.Dimensions		= TextureDimension::Buffer,
				.allocationType = ResourceAllocationType::Committed,
				.format			= IN_format,
				.initialLayout	= DeviceLayout_Undefined,


				.WH				= uint2{ (uint32_t)bufferSize, 1 },
				.bufferCount	= 3,
				.MipLevels		= 1,
			};	
		}

		static GPUResourceDesc UAVResource2(const size_t bufferSize, DeviceFormat IN_format = DeviceFormat::R32_UINT, bool renderTarget = false)
		{
			return {
				.type			= ResourceType::UnorderedAccess,
				.Dimensions		= TextureDimension::Texture1D,
				.allocationType = ResourceAllocationType::Committed,
				.format			= IN_format,

				.WH				= uint2{ (uint32_t)bufferSize, 1 },
				.bufferCount	= 3,
				.MipLevels		= 1,
			};
		}


		static GPUResourceDesc UAVTexture(const uint2 IN_WH, const DeviceFormat IN_format, bool renderTarget = false, uint32_t mipCount = 1)
		{
			return {
				.type			= renderTarget ? ResourceType::UnorderedAccessRenderTarget : ResourceType::UnorderedAccess,
				.Dimensions		= TextureDimension::Texture2D,
				.allocationType = ResourceAllocationType::Committed,
				.format			= IN_format,

				.WH				= IN_WH,
				.bufferCount	= 3,
				.MipLevels		= (uint8_t)mipCount,
			};
		}


		static GPUResourceDesc UAVTexture3D(const uint3 IN_XYZ, const DeviceFormat IN_format, bool renderTarget = false, uint32_t mipCount = 1)
		{
			return {
				.type			= ResourceType::UnorderedAccess,
				.Dimensions		= TextureDimension::Texture3D,
				.allocationType = ResourceAllocationType::Committed,
				.format			= IN_format,

				.WH				= { IN_XYZ[0], IN_XYZ[1] },
				.arraySize		= (uint8_t)IN_XYZ[2],
				.bufferCount	= 1,
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

		static GPUResourceDesc CubeMapUAV(uint2 WH, DeviceFormat format, uint8_t mipCount, const ResourceAllocationType allocationType = ResourceAllocationType::Committed)
		{
			 return {
				.type			= ResourceType::UnorderedAccessRenderTarget,
				.Dimensions		= TextureDimension::TextureCubeMap,
				.allocationType = allocationType,
				.format			= format,

				.WH				= WH,
				.arraySize		= 6,
				.MipLevels		= mipCount,
				.clearValue		= D3D12_CLEAR_VALUE{ .Format = TextureFormat2DXGIFormat(format), .Color = { 0.0f, 0.0f, 0.0f, 0.0f } }
			};
		}
	};


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

	inline TileID_t CreateTileID(uint32_t x, uint32_t y, uint32_t mipLevel)
	{
		return TileID_t{ (mipLevel & 0x7f) << 24 | (x & 0x0f) << 12 | y };
	}

	inline TileID_t CreatePackedID()
	{
		TileID_t id = { 0 };
		id.segments.packed = 1;
		return id;
	}


	enum class TileMapState
	{
		InUse,
		Null,
		Updated
	};

	struct TileMapping
	{
		TileID_t			tileID;
		DeviceHeapHandle	heap;
		TileMapState		state;
		uint32_t			heapOffset;

		uint64_t sortingID() const
		{
			return heap << 32 | tileID;
		};
	};


	using TileMapList = Vector<TileMapping>;


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


	using GPUResourceExtra_t = std::variant<UAVResourceLayout, UAVTextureLayout>;


	FLEXKITAPI class ResourceStateTable
	{
	public:
		ResourceStateTable(iAllocator* IN_allocator) :
			Handles				{ IN_allocator },
			UserEntries			{ IN_allocator },
			Resources			{ IN_allocator },
			BufferedResources	{ IN_allocator },
			delayRelease		{ IN_allocator },
			allocator			{ IN_allocator } {}


		~ResourceStateTable()
		{
			Release();
		}


		void Release();


		// No Copy
		ResourceStateTable				(const ResourceStateTable&) = delete;
		ResourceStateTable& operator =	(const ResourceStateTable&) = delete;

		Texture2D			operator[]		(ResourceHandle Handle);

		ResourceHandle		GetFreeHandle();
		ResourceHandle		AddResource		(const GPUResourceDesc& Desc, const DeviceLayout InitialState);
		void				SetResource		(ResourceHandle handle, const GPUResourceDesc& Desc, const DeviceLayout InitialState);
		void				SetLayout		(ResourceHandle Handle, DeviceLayout);
		void				SetDebug		(ResourceHandle Handle, const char* string);

		const char*			GetDebug(ResourceHandle Handle);
		uint2				GetWH(ResourceHandle Handle) const;
		uint3				GetXYZ(ResourceHandle Handle) const;
		size_t				GetFrameGraphIndex(ResourceHandle Texture, size_t FrameID) const;
		void				SetFrameGraphIndex(ResourceHandle Texture, size_t FrameID, size_t Index);

		DXGI_FORMAT			GetFormat(ResourceHandle handle) const;
		TextureDimension	GetDimension(ResourceHandle) const;
		size_t				GetArraySize(ResourceHandle) const;
		uint8_t				GetMIPCount(ResourceHandle) const;
		
		void				SetExtra(ResourceHandle handle, GPUResourceExtra_t);
		GPUResourceExtra_t	GetExtra(ResourceHandle handle) const;

		void				SetBufferedIdx(ResourceHandle handle, uint32_t idx);
		void				SetDebugName(ResourceHandle handle, const char* str);

		void				UpdateTileMappings(ResourceHandle handle, const TileMapping* begin, const TileMapping* end);
		const TileMapList&	GetTileMappings(const ResourceHandle handle) const;
		TileMapList&		_GetTileMappings(const ResourceHandle handle);

		void				MarkRTUsed		(ResourceHandle Handle);

		DeviceLayout		GetLayout		(ResourceHandle Handle) const;

		ID3D12Resource*		GetResource		(ResourceHandle Handle, ID3D12Device* device) const;
		size_t				GetResourceSize	(ResourceHandle Handle) const;
		uint2				GetHeapOffset	(ResourceHandle Handle, uint subResourceIdx) const;


		ResourceHandle		FindResourceHandle(ID3D12Resource* deviceResource) const;

		void ReplaceResources(ResourceHandle handle, ID3D12Resource** begin, size_t size);


		void ReleaseTexture	(ResourceHandle Handle, const uint64_t idx);
		void LockUntil		(size_t FrameID);

		void UpdateLocks(ThreadManager& thread, const uint64_t currentIdx);
		void SubmitTileUpdates(ID3D12CommandQueue* queue, RenderSystem& renderSystem, iAllocator* allocator_temp);

		void _ReleaseTextureForceRelease(ResourceHandle Handle);

		struct UserEntry
		{
			size_t					ResourceIdx;
			size_t					resourceSize; // ByteSize
			int64_t					FGI_FrameStamp;
			uint32_t				FrameGraphIndex;
			uint32_t				Flags;
			uint16_t				arraySize;
			ResourceHandle			Handle;
			DXGI_FORMAT				Format;
			TextureDimension		dimension;
			Vector<TileMapping>		tileMappings = {};
			GPUResourceExtra_t		extra;
			const char*				userString;
		};

		struct ResourceEntry
		{
			void			Release();
			void			SetLayout(DeviceLayout layout)	{ layouts[CurrentResource]		= layout;	}
			void			SetFrameLock(size_t FrameID)	{ FrameLocks[CurrentResource]	= FrameID;	}
			ID3D12Resource* GetAsset() const				{ return Resources[CurrentResource];		}
			void			IncreaseIdx()					{ CurrentResource = ++CurrentResource % 3;	}

			size_t				ResourceCount   = 0;
			size_t				CurrentResource = 0;
			ID3D12Resource*		Resources[3];
			size_t				FrameLocks[3];
			DeviceLayout		layouts[3];
			DXGI_FORMAT			Format;
			uint8_t				mipCount;
			uint2				WH;
			ResourceHandle		owner			= InvalidHandle;
			uint2				heapRange[3]	= { { 0, 0 }, { 0, 0 }, { 0, 0 } };

#if USING(AFTERMATH)
			GFSDK_Aftermath_ResourceHandle  aftermathResource[3];
#endif
		};

			
		Vector<UserEntry>									UserEntries;
		Vector<ResourceEntry>								Resources;
		Vector<ResourceHandle>								BufferedResources;
		HandleUtilities::HandleTable<ResourceHandle>		Handles;
		std::mutex											m;

		struct UnusedResource
		{
			ID3D12Resource* resource;
			uint64_t			idx;
		};

		Vector<UnusedResource>	delayRelease;
		iAllocator*				allocator;
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
		FlexKit::DeviceFormat	format;
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
			DeviceLayout						layouts[3];
			SOResourceHandle					resourceHandle;
			uint8_t								resourceIdx;
		};


		SOResourceHandle AddResource(
			static_vector<ID3D12Resource*,3>	newResources, 
			static_vector<ID3D12Resource*, 3>	counters, 
			size_t								resourceSize, 
			DeviceLayout						initialLayout)
		{
			auto newHandle		= handles.GetNewHandle();
			handles[newHandle]	= 
					(index_t)resources.push_back({
						newResources,
						counters,
						resourceSize,
						{ initialLayout, initialLayout, initialLayout },
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


		DeviceLayout GetLayout(SOResourceHandle handle) const
		{
			auto& resourceEntry = resources[handles[handle]];
			return resourceEntry.layouts[resourceEntry.resourceIdx];
		}


		void SetLayout(SOResourceHandle handle, DeviceLayout layout)
		{
			auto& resourceEntry = resources[handles[handle]];
			resourceEntry.layouts[resourceEntry.resourceIdx] = layout;
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

		HandleUtilities::HandleTable<SOResourceHandle>	    handles;
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


	struct DeviceHeap
	{
		ID3D12Heap*			heap;
		DeviceHeapHandle	handle;

		operator ID3D12Heap* () { return heap; }

		void Release()
		{
			heap->Release();
			heap = nullptr;
		}
	};

	enum struct ResourceHeapTier
	{
		HeapTier1,
		HeapTier2,
	};

	class HeapTable
	{
	public:
		HeapTable(ID3D12Device* IN_device, iAllocator* allocator);
		~HeapTable();

		void				Init(ResourceHeapTier, ID3D12Device* IN_device);

		DeviceHeapHandle	CreateHeap(const size_t size, const uint32_t flags);
		void				ReleaseHeap(DeviceHeapHandle heap);

		size_t			GetHeapSize(DeviceHeapHandle heap) const;
		ID3D12Heap*		GetDeviceResource(DeviceHeapHandle handle) const;

	private:

		Vector<DeviceHeap>								heaps;
		HandleUtilities::HandleTable<DeviceHeapHandle>	handles;
		ID3D12Device*									pDevice;
		ResourceHeapTier								tier;
		std::mutex										m;
	};


	/************************************************************************************************/

	using ReadBackEventHandler = TypeErasedCallable<void (ReadBackResourceHandle), 64>;

	struct MappedReadBackBuffer
	{
		void*					buffer;
		const size_t			bufferSize;
		ReadBackResourceHandle	handle;
	};


	class ReadBackStateTable
	{
	public:
		ReadBackStateTable(iAllocator* allocator) :
			handles			{ allocator },
			pendingRelease	{ allocator },
			readBackBuffers	{ allocator },
			readBackFence	{ nullptr }
		{
			
		}

		~ReadBackStateTable()
		{
			Release();
		}


		ReadBackResourceHandle AddReadBack(const size_t BufferSize, ID3D12Resource* resource)
		{
			auto event		= CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
			auto handle		= handles.GetNewHandle();

			auto index		= (index_t)readBackBuffers.emplace_back(BufferSize, handle, resource);
			handles[handle]	= index;

			readBackBuffers[index].event = event;

			return handle;
		}


		void Initiate(ID3D12Device* device)
		{
			const auto HR = device->CreateFence(0, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&readBackFence));
			FK_ASSERT(SUCCEEDED(HR));
		}


		void Release()
		{
			if(readBackFence)
				readBackFence->Release();

			readBackFence = nullptr;

			for (auto& buffer : readBackBuffers)
				CloseHandle(buffer.event);

			readBackBuffers.clear();

			for (auto& pending : pendingRelease)
				pending.resource->Release();

			pendingRelease.clear();

		}

		void ReleaseResource(ReadBackResourceHandle handle)
		{
			pendingRelease.push_back(readBackBuffers[handles[handle]]);

			readBackBuffers[handles[handle]]        = std::move(readBackBuffers.back());
			handles[readBackBuffers.back().handle]  = handles[handle];

			readBackBuffers.pop_back();
		}


		void Update()
		{
			ProfileFunction();

			for (ReadBackEntry& buffer : readBackBuffers)
			{
				if (buffer.queued)
				{
					auto res = WaitForSingleObject(buffer.event, 0);

					if (res == WAIT_OBJECT_0)
					{
						buffer.queueUntil   = -1;
						buffer.queued       = false;
						buffer.onReadBack(buffer.handle);
					}
					else if (res == WAIT_FAILED)
					{
						buffer.queueUntil   = -1;
						buffer.queued       = false;

						FK_ASSERT(0, "Unknown Error");
					}
					else if (res == WAIT_TIMEOUT)
						continue;
				}
			}
		}


		auto GetDeviceResource(const ReadBackResourceHandle handle) const
		{
			return readBackBuffers[handles[handle]].resource;
		}


		auto& operator [](const ReadBackResourceHandle handle) const
		{
			return readBackBuffers[handles[handle]];
		}


		auto& operator [](const ReadBackResourceHandle handle)
		{
			return readBackBuffers[handles[handle]];
		}


		void SetCallback(ReadBackResourceHandle handle, ReadBackEventHandler&& readBackHandler)
		{
			readBackBuffers[handles[handle]].onReadBack = std::move(readBackHandler);
		}


		void QueueReadBack(const ReadBackResourceHandle handle, const size_t counter)
		{
			(*this)[handle].queueUntil = counter;
		}


		MappedReadBackBuffer OpenBufferData(const ReadBackResourceHandle handle, size_t readSize)
		{
			auto& readBack					= readBackBuffers[handles[handle]];
			void* _ptr						= nullptr;
			const size_t requestedReadSize	= Min(readBack.size, readSize);

			const D3D12_RANGE readRange = {
				0,
				requestedReadSize
			};

			readBack.resource->Map(0, &readRange, &_ptr);

			return { _ptr, requestedReadSize };
		}

		void CloseBufferData(const ReadBackResourceHandle handle)
		{
			auto& readBack = readBackBuffers[handles[handle]];

			const D3D12_RANGE writeRange = {
				0,
				0
			};

			readBack.resource->Unmap(0, &writeRange);
		}
		
		ID3D12Fence* _GetReadBackFence()
		{
			return readBackFence;
		}


		uint64_t    GetTicket()
		{
			return ++counter;
		}



	private:
		struct ReadBackEntry
		{
			size_t					size;
			ReadBackResourceHandle	handle;
			ID3D12Resource*			resource;
			size_t					queueUntil;
			ReadBackEventHandler	onReadBack;
			HANDLE					event;
			bool					queued  = false;
		};

		ReadBackEntry& _GetEntry(ReadBackResourceHandle handle)
		{
			const size_t idx = handles[handle];
			return readBackBuffers[idx];
		}

		enum States
		{
		};


		uint64_t												counter = 0;
		ID3D12Fence*											readBackFence;
		Vector<ReadBackEntry>									readBackBuffers;
		Vector<ReadBackEntry>									pendingRelease;
		HandleUtilities::HandleTable<ReadBackResourceHandle>	handles;
	};


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


	inline uint2 GetFormatTileSize(DeviceFormat format)
	{
		//switch (format)
		//{
		//default:
		return { 256, 256 };
		//}
	}

	struct BLAS_PreBuildInfo
	{
		size_t BLAS_byteSize;
		size_t scratchPad_byteSize;
		size_t update_byteSize;
	};

	constexpr PSOHandle CLEARBUFFERPSO = PSOHandle(GetTypeGUID(CLEARBUFFERPSO));

	struct AvailableFeatures
	{
		enum struct Raytracing
		{
			RT_FeatureLevel_NOTAVAILABLE,
			RT_FeatureLevel_1,
			RT_FeatureLevel_1_1,
		} RT_Level;

		enum ConservativeRasterization
		{
			ConservativeRast_AVAILABLE,
			ConservativeRast_NOTAVAILABLE,
		} conservativeRast;

		enum struct HLSLCompiler
		{
			HLSL_CompilerEnabled,
			HLSL_CompilerDisabled
		} Compiler;

		ResourceHeapTier resourceHeapTier;
	};

	struct ShaderOptions
	{
		bool enable16BitTypes	= false;
		bool hlsl2021			= false;
		bool enableDebug		= false;
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
		ResourceHandle	handle;
		ID3D12Resource*	_ptr;

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

	FLEXKITAPI class RenderSystem
	{
	public:
		RenderSystem(iAllocator* IN_allocator, ThreadManager* IN_Threads);
		~RenderSystem();

		RenderSystem(const RenderSystem&) = delete;
		RenderSystem& operator =	(const RenderSystem&) = delete;

		bool Initiate(Graphics_Desc* desc_in);
		void Release();

		template<typename FN>
		void LockedResourceOperation(FN op) requires std::invocable<FN>
		{
			std::unique_lock lock{ Textures.m };

			op();
		}

		size_t						GetCurrentCounter();
		ID3D12PipelineState*		GetPSO(PSOHandle StateID);
		const RootSignature* const	GetPSORootSignature(PSOHandle StateID) const;


		void BuildLibrary(PSOHandle State, const PipelineStateLibraryDesc);
		void RegisterPSOLoader(PSOHandle State, PipelineStateDescription desc);
		void QueuePSOLoad(PSOHandle State);


		SyncPoint	SyncWithDirect();
		SyncPoint	GetSyncDirect();
		SyncPoint	GetSubmissionTicket();
		SyncPoint	Submit(std::span<Context*> CLs, std::optional<SyncPoint> sync = {});
		void		Signal(SyncPoint);

		void		_UpdateCounters();
		void		_UpdateSubResources(ResourceHandle handle, ID3D12Resource** resources, const size_t size);

		void WaitforGPU();

		void SetDebugName(ResourceHandle, const char*);

		size_t						GetVertexBufferSize(const VertexBufferHandle);
		D3D12_GPU_VIRTUAL_ADDRESS	GetVertexBufferAddress(const VertexBufferHandle VB);
		D3D12_GPU_VIRTUAL_ADDRESS	GetConstantBufferAddress(const ConstantBufferHandle CB);
		BLAS_PreBuildInfo			GetBLASPreBuildInfo(VertexBuffer&);


		size_t	GetTextureFrameGraphIndex(ResourceHandle);
		void	SetTextureFrameGraphIndex(ResourceHandle, size_t);

		void	MarkTextureUsed(ResourceHandle Handle);

		const size_t	GetResourceSize(ConstantBufferHandle handle) const noexcept;
		const size_t	GetResourceSize(ResourceHandle desc) const noexcept;

		const size_t	GetAllocationSize(ResourceHandle handle) const noexcept; // Includes padding and alignment
		const size_t	GetAllocationSize(GPUResourceDesc desc) const noexcept; // Includes padding and alignment


		const size_t	GetTextureElementSize(ResourceHandle   Handle) const;
		const uint2		GetTextureWH(ResourceHandle   Handle) const;

		DeviceFormat	GetTextureFormat(ResourceHandle Handle) const;
		DXGI_FORMAT		GetTextureDeviceFormat(ResourceHandle Handle) const;
		uint8_t			GetTextureMipCount(ResourceHandle Handle) const;
		uint2			GetTextureTilingWH(ResourceHandle Handle, const uint mipLevel) const;
		uint2			GetHeapOffset(ResourceHandle Handle, uint subResourceID = 0) const;

		SyncPoint			UpdateTileMappings(ResourceHandle* begin, ResourceHandle* end, iAllocator* allocator);
		void				UpdateTextureTileMappings(const ResourceHandle Handle, std::span<const TileMapping>);
		const TileMapList&	GetTileMappings(const ResourceHandle Handle);


		TextureDimension	GetTextureDimension(ResourceHandle handle) const;
		size_t				GetTextureArraySize(ResourceHandle handle) const;

		void			UploadTexture(ResourceHandle, CopyContextHandle, byte* buffer, size_t bufferSize); // Uses Upload Queue
		void			UploadTexture(ResourceHandle handle, CopyContextHandle, TextureBuffer* buffer, size_t resourceCount); // Uses Upload Queue
		void			UpdateResourceByUploadQueue(ID3D12Resource* Dest, CopyContextHandle, const void* Data, size_t Size, size_t ByteSize, DeviceAccessState EndState);

		Shader  LoadShader(const char* entryPoint, const char* ShaderType, const char* file, const ShaderOptions& options = {});

		PipelineStateLibraryDesc    CreatePipelibrary();

		// Resource Creation and Destruction
		[[nodiscard]] DeviceHeapHandle			CreateHeap                  (const size_t heapSize, const uint32_t flags);
		[[nodiscard]] ConstantBufferHandle		CreateConstantBuffer		(size_t BufferSize, bool GPUResident = true);
		[[nodiscard]] VertexBufferHandle		CreateVertexBuffer			(size_t BufferSize, bool GPUResident = true);
		[[nodiscard]] ResourceHandle			CreateDepthBuffer			(const uint2 WH, const bool UseFloat = false, size_t bufferCount = 3);
		[[nodiscard]] ResourceHandle			CreateDepthBufferArray		(const uint2 WH, const bool UseFloat = false, const size_t arraySize = 1, const bool buffered = true, const ResourceAllocationType = ResourceAllocationType::Committed);
		[[nodiscard]] ResourceHandle			CreateGPUResource			(const GPUResourceDesc& desc);
		[[nodiscard]] ResourceHandle			CreateGPUResourceHandle     ();
		[[nodiscard]] QueryHandle				CreateOcclusionBuffer		(size_t Size);
		[[nodiscard]] ResourceHandle			CreateUAVBufferResource		(size_t bufferHandle, bool tripleBuffer = true);
		[[nodiscard]] ResourceHandle			CreateUAVTextureResource	(const uint2 WH, const DeviceFormat, const bool RenderTarget = false);
		[[nodiscard]] SOResourceHandle			CreateStreamOutResource		(size_t bufferHandle, bool tripleBuffer = true);
		[[nodiscard]] QueryHandle				CreateSOQuery				(size_t SOIndex, size_t count);
		[[nodiscard]] QueryHandle				CreateTimeStampQuery		(size_t count);
		[[nodiscard]] IndirectLayout			CreateIndirectLayout		(static_vector<IndirectDrawDescription> entries, iAllocator* allocator, RootSignature* signature = nullptr);
		[[nodiscard]] ReadBackResourceHandle	CreateReadBackBuffer		(const size_t bufferSize);

		void BackResource(ResourceHandle, const GPUResourceDesc& desc);

		void						SetReadBackEvent    (ReadBackResourceHandle readbackBuffer, ReadBackEventHandler&& handler);
		std::pair<void*, size_t>	OpenReadBackBuffer  (ReadBackResourceHandle readbackBuffer, const size_t readSize = -1);
		void						CloseReadBackBuffer (ReadBackResourceHandle readbackBuffer);
		void						FlushPendingReadBacks();

		void SetObjectLayout(SOResourceHandle	handle, DeviceLayout state);
		void SetObjectLayout(ResourceHandle		handle, DeviceLayout state);


		DeviceLayout GetObjectLayout(const QueryHandle		handle) const;
		DeviceLayout GetObjectLayout(const SOResourceHandle	handle) const;
		DeviceLayout GetObjectLayout(const ResourceHandle	handle) const;


		ID3D12Heap*			GetDeviceResource(const DeviceHeapHandle        handle) const;
		ID3D12Resource*		GetDeviceResource(const ReadBackResourceHandle	handle) const;
		ID3D12Resource*		GetDeviceResource(const ConstantBufferHandle	handle) const;
		ID3D12Resource*		GetDeviceResource(const ResourceHandle		    handle) const;
		ID3D12Resource*		GetDeviceResource(const SOResourceHandle		handle) const;
		ID3D12Resource*		GetSOCounterResource(const SOResourceHandle handle) const;
		size_t				GetStreamOutBufferSize(const SOResourceHandle handle) const;

		UAVResourceLayout	GetUAVBufferLayout(const ResourceHandle) const noexcept;
		void				SetUAVBufferLayout(const ResourceHandle, const UAVResourceLayout) noexcept;
		size_t				GetUAVBufferSize(const ResourceHandle) const noexcept;

		size_t				GetHeapSize(const DeviceHeapHandle heap) const;

		AvailableFeatures::Raytracing	GetRTFeatureLevel() const noexcept;
		bool							RTAvailable() const noexcept;

		void ResetConstantBuffer(ConstantBufferHandle constant);
		void ResetQuery(QueryHandle handle);

		void ReleaseCB(ConstantBufferHandle);
		void ReleaseVB(VertexBufferHandle);
		void ReleaseResource(ResourceHandle);
		void ReleaseReadBack(ReadBackResourceHandle);
		void ReleaseHeap(DeviceHeapHandle);

		void				SubmitUploadQueues(uint32_t flags, CopyContextHandle* handle, size_t count = 1, std::optional<SyncPoint> syncBefore = {}, std::optional<SyncPoint> syncAfter = {});
		CopyContextHandle	OpenUploadQueue();
		CopyContextHandle	GetImmediateCopyQueue();
		Context&			GetCommandList(std::optional<SyncPoint> ticket = {});

		// Internal
		//ResourceHandle			_AddBackBuffer						(Texture2D_Desc& Desc, ID3D12Resource* Res, uint32_t Tag);
		static ConstantBuffer	_CreateConstantBufferResource(RenderSystem* RS, ConstantBuffer_desc* desc);
		VertexResourceBuffer	_CreateVertexBufferDeviceResource(const size_t ResourceSize, bool GPUResident = true);
		ResourceHandle			_CreateDefaultTexture();
		UploadSegment			_ReserveDirectUploadSpace(size_t resourceSize, size_t alignment);

		enum class DescriptorRangeAllocationError
		{
			OutOfSpace,
			NotEnoughContiguousDescriptors
		};

		using AllocationResult = std::expected<DescriptorRange, DescriptorRangeAllocationError>;

		AllocationResult	_AllocateDescriptorRange(const size_t size);
		void				_ReleaseDescriptorRange(DescriptorRange range, uint64_t lockIdx);

		void				_PushDelayReleasedResource(ID3D12Resource*, CopyContextHandle = InvalidHandle);

		void	_ForceReleaseTexture(ResourceHandle handle);

		size_t	_GetVidMemUsage();

		[[nodiscard]] ID3D12DescriptorHeap* _CreateShaderVisibleHeap(const size_t);

		ID3D12QueryHeap*	_GetQueryResource(QueryHandle handle);
		CopyContext&		_GetCopyContext(CopyContextHandle handle = InvalidHandle);
		auto*				_GetCopyQueue() { return copyEngine.copyQueue; }

		void				_OnCrash();

		bool				DEBUG_AttachPIX();
		bool				DEBUG_BeginPixCapture();
		bool				DEBUG_EndPixCapture();

#if USING(PIX)
		IDXGraphicsAnalysis* pix = nullptr;
#endif

#if USING(AFTERMATH)
		static void GpuCrashDumpCallback(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData);
		static void ShaderDebugInfoCallback(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize, void* pUserData);
		static void CrashDumpDescriptionCallback(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription, void* pUserData);

		void WriteGpuCrashDumpToFile(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize);

		static void OnShaderDebugInfoLookup(const GFSDK_Aftermath_ShaderDebugInfoIdentifier* pIdentifier, PFN_GFSDK_Aftermath_SetData setShaderDebugInfo, void* pUserData);
		static void OnShaderLookup(const GFSDK_Aftermath_ShaderHash* pShaderHash, PFN_GFSDK_Aftermath_SetData setShaderBinary, void* pUserData);
		static void OnShaderInstructionsLookup(const GFSDK_Aftermath_ShaderInstructionsHash* pShaderInstructionsHash, PFN_GFSDK_Aftermath_SetData setShaderBinary, void* pUserData);
		static void OnShaderSourceDebugInfoLookup(const GFSDK_Aftermath_ShaderDebugName* pShaderDebugName, PFN_GFSDK_Aftermath_SetData setShaderBinary, void* pUserData);
#endif

#if USING(ENABLEDRED)
		ID3D12DeviceRemovedExtendedData1* dred;
#endif

		operator RenderSystem* () { return this; }

		ID3D12Device1*		pDevice			= nullptr;
		ID3D12Device10*		pDevice10		= nullptr;
		ID3D12CommandQueue*	GraphicsQueue	= nullptr;
		ID3D12CommandQueue*	ComputeQueue	= nullptr;

		size_t				pendingFrames[3]			= { 0, 0, 0 };
		size_t				frameIdx					= 0;
		std::atomic_uint	graphicsSubmissionCounter	= 0;
		ID3D12Fence*		Fence						= nullptr;

		CopyContextHandle	ImmediateUpload = InvalidHandle;

		IDXGIFactory2*		pGIFactory		= nullptr;
		IDXGIFactory5*		pGIFactory5		= nullptr;
		IDXGIAdapter3*		pDXGIAdapter	= nullptr;
		IDXGIAdapter4*		pDXGIAdapter4	= nullptr;

		ResourceHandle		DefaultTexture;

		size_t BufferCount = 0;
		size_t DescriptorRTVSize = 0;
		size_t DescriptorDSVSize = 0;
		size_t DescriptorCBVSRVUAVSize = 0;

		struct
		{
			size_t AASamples = 0;
			size_t AAQuality = 1;
		}Settings;

		struct RootSigLibrary
		{
			RootSigLibrary(iAllocator* allocator) :
				RSDefault			{ allocator },
				ShadingRTSig		{ allocator },
				RS4CBVs_SO			{ allocator },
				RS6CBVs4SRVs		{ allocator },
				RS2UAVs4SRVs4CBs	{ allocator },
				ComputeSignature	{ allocator },
				ClearBuffer			{ allocator } {}

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
			RootSignature ClearBuffer;
		}Library;

		Vector<Context>				Contexts;
		size_t						contextIdx = 0;

		HeapTable					heaps;
		CopyEngine					copyEngine;
		ConstantBufferTable			ConstantBuffers;
		QueryTable					Queries;
		VertexBufferStateTable		VertexBuffers;
		ResourceStateTable			Textures;
		SOResourceTable				StreamOutTable;
		PipelineStateTable			PipelineStates;
		ReadBackStateTable			ReadBackTable;
		DescriptorHeapAllocator		descriptorHeapAllocator;

		AvailableFeatures features;


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
			CopyContextHandle queue;
			size_t			waitCounter;
		};

		std::mutex						directUploadBufferMutex;
		UploadBuffer					directUploadBuffer;

		Vector<UploadSyncPoint>			Syncs;
		Vector<FreeEntry>				FreeList_GraphicsQueue;
		Vector<FreeEntry>				FreeList_CopyQueue;

		ThreadManager&			threads;

		IDxcLibrary*			hlslLibrary			= nullptr;
		IDxcCompiler*			hlslCompiler		= nullptr;
		IDxcIncludeHandler*		hlslIncludeHandler	= nullptr;

		DeviceVendor			vendorID		= DeviceVendor::UNKNOWN;
		ID3D12Debug1*			pDebug			= nullptr;
		ID3D12Debug5*			pDebug5			= nullptr;
		ID3D12DebugDevice*		pDebugDevice	= nullptr;
		ID3D12DebugDevice1*		pDebugDevice1	= nullptr;
		iAllocator*				Memory			= nullptr;

		std::mutex				crashM;
		std::mutex				barrierLock;
	};

	
	/************************************************************************************************/


	using ReadBackEventHandler = TypeErasedCallable<void (ReadBackResourceHandle), 64>;


	FLEXKITAPI class Context
	{
	public:
		Context(RenderSystem*				renderSystem_IN	= nullptr, 
				iAllocator*					allocator		= nullptr);
			
		Context(Context&& RHS);


		Context& operator = (Context&& RHS);


		Context				(const Context& RHS) = delete;
		Context& operator = (const Context& RHS) = delete;

		void Release();

		void CreateAS(const AccelerationStructureDesc&, const TriMesh& );
		void BuildBLAS(VertexBuffer& buffer, ResourceHandle destination, ResourceHandle scratchSpace);

		void DiscardResource(ResourceHandle resource);

		void AddAliasingBarrier			(ResourceHandle before, ResourceHandle after);
		void AddUAVBarrier				(ResourceHandle Handle = InvalidHandle, uint32_t subresource = -1, DeviceLayout layout = DeviceLayout::DeviceLayout_Unknown);
		void AddPresentBarrier			(ResourceHandle Handle,	DeviceAccessState Before);
		void AddRenderTargetBarrier		(ResourceHandle Handle,	DeviceAccessState Before, DeviceAccessState State = DeviceAccessState::DASRenderTarget);
		void AddStreamOutBarrier		(SOResourceHandle,		DeviceAccessState Before, DeviceAccessState State);
		void AddCopyResourceBarrier		(ResourceHandle Handle, DeviceAccessState Before, DeviceAccessState State);

		void AddGlobalBarrier			(ResourceHandle resource, DeviceAccessState accessBefore, DeviceAccessState accessAfter, DeviceSyncPoint syncBefore, DeviceSyncPoint syncAfter);
		void AddTextureBarrier			(ResourceHandle Handle, DeviceAccessState, DeviceAccessState, DeviceLayout, DeviceLayout, DeviceSyncPoint, DeviceSyncPoint, BarrierSubResourceRange range = {});
		void AddBufferBarrier			(ResourceHandle Handle, DeviceAccessState, DeviceAccessState, DeviceSyncPoint, DeviceSyncPoint);
		void AddBarriers				(std::span<const Barrier> barriers);

		void ClearDepthBuffer		(ResourceHandle Texture, float ClearDepth = 0.0f); // Assumes full-screen Clear
		void ClearRenderTarget		(ResourceHandle Texture, float4 ClearColor = float4(0.0f)); // Assumes full-screen Clear
		void ClearUAVTextureFloat	(ResourceHandle UAV, float4 clearColor = float4(0, 0, 0, 0));
		void ClearUAVTextureUint	(ResourceHandle UAV, uint4 clearColor = uint4{ 0, 0, 0, 0 });
		void ClearUAV				(ResourceHandle UAV, uint4 clearColor = uint4{ 0, 0, 0, 0 });
		void ClearUAVBuffer			(ResourceHandle UAV, uint4 clearColor = uint4{ 0, 0, 0, 0 });
		void ClearUAVBufferRange	(ResourceHandle UAV, uint begin, uint end, uint4 clearColor = uint4{ 0, 0, 0, 0 });

		void SetRootSignature			(const RootSignature& RS);
		void SetComputeRootSignature	(const RootSignature& RS);
		void SetPipelineState			(ID3D12PipelineState* PSO);

		void SetRenderTargets			(const static_vector<ResourceHandle> RTs, bool DepthStecil, ResourceHandle DepthStencil = InvalidHandle, const size_t MIPMapOffset = 0);
		void SetRenderTargets2			(const static_vector<ResourceHandle> RTs, const size_t MIPMapOffset, const DepthStencilView_Options DSV);

		void SetViewports				(static_vector<D3D12_VIEWPORT, 16>	VPs);
		void SetScissorRects			(static_vector<D3D12_RECT, 16>		Rects);

		void SetScissorAndViewports		(static_vector<ResourceHandle, 16>	RenderTargets);
		void SetScissorAndViewports2	(static_vector<ResourceHandle, 16>	RenderTargets, const size_t MIPMapOffset = 0);

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

		void QueueReadBack(ReadBackResourceHandle readBack);
		void QueueReadBack(ReadBackResourceHandle readBack, ReadBackEventHandler callback);

		void SetDepthStencil		(ResourceHandle DS);
		void SetPrimitiveTopology	(EInputTopology Topology);

		void SetGraphicsConstantValue(size_t idx, size_t valueCount, const void* data_ptr, size_t offset = 0);

		void NullGraphicsConstantBufferView	(size_t idx);
		void SetGraphicsConstantBufferView	(size_t idx, const ConstantBufferHandle CB, size_t Offset = 0);
		void SetGraphicsConstantBufferView	(size_t idx, const ConstantBufferDataSet& CB);
		void SetGraphicsConstantBufferView	(size_t idx, const ConstantBuffer& CB);
		void SetGraphicsDescriptorTable		(size_t idx, const DescriptorHeap& DH);
		void SetGraphicsDescriptorTable		(size_t idx, const DescriptorRange& range);
		void SetGraphicsShaderResourceView	(size_t idx, FrameBufferedResource* Resource, size_t Count, size_t ElementSize);
		void SetGraphicsShaderResourceView	(size_t idx, Texture2D& Texture);
		void SetGraphicsShaderResourceView	(size_t idx, ResourceHandle resource, size_t offset = 0);
		void SetGraphicsUnorderedAccessView (size_t idx, ResourceHandle resource, size_t offset = 0);


		void SetComputeDescriptorTable		(size_t idx);
		void SetComputeDescriptorTable		(size_t idx, const DescriptorHeap& DH);
		void SetComputeConstantBufferView	(size_t idx, const ConstantBufferHandle, size_t offset);
		void SetComputeConstantBufferView	(size_t idx, const ConstantBufferDataSet& CB);
		void SetComputeConstantBufferView	(size_t idx, ResourceHandle, size_t offset = 0, size_t bufferSize = 256);
		void SetComputeShaderResourceView	(size_t idx, Texture2D&		texture);
		void SetComputeShaderResourceView	(size_t idx, ResourceHandle resource, size_t offset = 0);
		void SetComputeUnorderedAccessView	(size_t idx, ResourceHandle resource, size_t offset = 0);
		void SetComputeConstantValue		(size_t idx, size_t valueCount, const void* data_ptr, size_t offset = 0);


		void BeginQuery	(QueryHandle query, size_t idx);
		void EndQuery	(QueryHandle query, size_t idx);

		void TimeStamp(QueryHandle query, size_t idx);

		void SetMarker_DEBUG(const char* str);

		void BeginEvent_DEBUG(const char* str);
		void EndEvent_DEBUG();

		void CopyResource(ResourceHandle dest, ResourceHandle src);

		void CopyBufferRegion(
			ResourceHandle	destination,
			ResourceHandle	source,
			size_t			size,
			size_t			destinationOffset	= 0,
			size_t			sourceOffset		= 0);

		void CopyBufferRegion(
			ResourceHandle	destination,
			ID3D12Resource*	source,
			size_t			size,
			size_t			destinationOffset	= 0,
			size_t			sourceOffset		= 0);

		void CopyBufferRegion(
			ID3D12Resource*	destination,
			ResourceHandle	source,
			size_t			size,
			size_t			destinationOffset	= 0,
			size_t			sourceOffset		= 0);

		void CopyBufferRegion(
			ID3D12Resource*	destination,
			ID3D12Resource* source,
			size_t			size,
			size_t			destinationOffset	= 0,
			size_t			sourceOffset		= 0);

		void CopyBufferRegion(
			static_vector<ID3D12Resource*>		sources,
			static_vector<size_t>				sourceOffset,
			static_vector<ID3D12Resource*>		destinations,
			static_vector<size_t>				destinationOffset,
			static_vector<size_t>				copySize,
			static_vector<DeviceAccessState>	currentStates,
			static_vector<DeviceAccessState>	finalStates);

		void ImmediateWrite(
			static_vector<ResourceHandle>	handles,
			static_vector<size_t>				value,
			static_vector<DeviceAccessState>	currentStates,
			static_vector<DeviceAccessState>	finalStates);

		void ClearSOCounters(static_vector<SOResourceHandle> handles);

		void CopyUInt64(
			static_vector<ID3D12Resource*>			source,
			static_vector<DeviceAccessState>		sourceState,
			static_vector<size_t>					sourceoffsets,
			static_vector<ID3D12Resource*>			destination,
			static_vector<DeviceAccessState>		destinationState,
			static_vector<size_t>					destinationoffset);

		void AddIndexBuffer			(TriMesh* Mesh, uint32_t lod = 0);
		void SetIndexBuffer			(VertexBufferEntry buffer, DeviceFormat format = DeviceFormat::R32_UINT);

		void AddVertexBuffers		(TriMesh* Mesh, uint32_t lod, const std::initializer_list<VERTEXBUFFER_TYPE>& buffers, VertexBufferList* InstanceBuffers = nullptr);
		void AddVertexBuffers		(TriMesh* Mesh, uint32_t lod, const std::span<const VERTEXBUFFER_TYPE> buffers, VertexBufferList* InstanceBuffers = nullptr);
		void SetVertexBuffers		(const std::initializer_list<VertexBufferEntry>&	list);
		void SetVertexBuffers		(const std::span<const VertexBufferEntry>			list);

		void SetVertexBuffers2		(const std::initializer_list<D3D12_VERTEX_BUFFER_VIEW>&	list);
		void SetVertexBuffers2		(const std::span<const D3D12_VERTEX_BUFFER_VIEW>		list);

		void SetSOTargets			(static_vector<D3D12_STREAM_OUTPUT_BUFFER_VIEW, 4> SOViews);

		void Draw					(const size_t VertexCount, const size_t BaseVertex = 0, const size_t baseIndex = 0);
		void DrawInstanced			(const size_t VertexCount, const size_t BaseVertex = 0, const size_t instanceCount = 0, size_t instanceOffset = 0);
		void DrawIndexed			(const size_t IndexCount, const size_t IndexOffet = 0, const size_t BaseVertex = 0);
		void DrawIndexedInstanced	(const size_t IndexCount, const size_t IndexOffet = 0, const size_t BaseVertex = 0, const size_t InstanceCount = 1, const size_t InstanceOffset = 0);
		void Clear					();

		void ResolveQuery			(QueryHandle query, size_t begin, size_t end, ResourceHandle destination, size_t destOffset);
		void ResolveQuery			(QueryHandle query, size_t begin, size_t end, ID3D12Resource* destination, size_t destOffset);

		void ExecuteIndirect		(ResourceHandle args, const IndirectLayout& layout, size_t argumentBufferOffset = 0, size_t executionCount = 1);
		void Dispatch				(const uint3);
		void Dispatch				(ID3D12PipelineState* PSO, const uint3 xyz) { SetPipelineState(PSO); Dispatch(xyz); }
		void DispatchRays			(const uint3, const DispatchDesc desc);

		void FlushBarriers();

		void SetPredicate(bool Enable, QueryHandle Handle = {}, size_t = 0);

		void CopyBuffer		(const UploadSegment src, const ResourceHandle destination, const size_t destOffset = 0);
		void CopyTexture2D	(const UploadSegment src, const ResourceHandle destination, const uint2 BufferSize);

		void CopyTexture2D(auto des, auto src)
		{
			FlushBarriers();

			DeviceContext->CopyResource(
				renderSystem->GetDeviceResource(des),
				renderSystem->GetDeviceResource(src));
		}

		void SetRTRead	(ResourceHandle Handle);
		void SetRTWrite	(ResourceHandle Handle);
		void SetRTFree	(ResourceHandle Handle);

		void		Close();
		Context&	Reset(DescriptorRange range, const size_t newDispatchIdx, ID3D12DescriptorHeap* heap);

		void SetDebugName(const char* ID)
		{
			SETDEBUGNAME(DeviceContext, ID);
		}


		UploadSegment ReserveDirectUploadSpace(size_t size, size_t alignment = 256);

		// Not Yet Implemented
		void SetUAVRead();
		void SetUAVWrite();
		void SetUAVFree();

		void _QueueReadBacks();

		std::optional<DescHeapPOS>	_ReserveSRV(size_t count);
		DescHeapPOS	_ReserveDSV(size_t count);
		DescHeapPOS	_ReserveRTV(size_t count);
		DescHeapPOS	_ReserveSRVLocal(size_t count);


		void		_ResetRTV();
		void		_ResetDSV();
		void		_ResetSRV();

		uint64_t	_GetCounter() { return dispatchIdx; }
		ID3D12GraphicsCommandList*	GetCommandList() { return DeviceContext; }

		RenderSystem* renderSystem = nullptr;

		void BeginMarker(const char* str);
		void EndMarker(const char* str);

	//private:

		DescHeapPOS _GetDepthDesciptor(ResourceHandle resource);

		void UpdateResourceStates();


		ID3D12CommandAllocator*			commandAllocator		= nullptr;
		ID3D12GraphicsCommandList7*		DeviceContext			= nullptr;

#if USING(DEBUGGRAPHICS)
		ID3D12DebugCommandList*			debugCommandList		= nullptr;
#endif

		const RootSignature*			CurrentRootSignature		= nullptr;
		const RootSignature*			CurrentComputeRootSignature	= nullptr;
		ID3D12PipelineState*			CurrentPipelineState		= nullptr;

		ID3D12DescriptorHeap*			descHeapRTV				= nullptr;
		ID3D12DescriptorHeap*			descHeapSRVLocal		= nullptr; // CPU visable only
		ID3D12DescriptorHeap*			descHeapDSV				= nullptr;


		DescriptorRange				shaderResources;
		size_t						heapUsed	= 0;
		uint64_t					dispatchIdx = 0;

		D3D12_CPU_DESCRIPTOR_HANDLE RTV_CPU;
		D3D12_CPU_DESCRIPTOR_HANDLE DSV_CPU;

		D3D12_CPU_DESCRIPTOR_HANDLE RTVPOSCPU;
		D3D12_CPU_DESCRIPTOR_HANDLE DSVPOSCPU;

		D3D12_CPU_DESCRIPTOR_HANDLE SRV_LOCAL_CPU;

		size_t	RenderTargetCount;
		bool	DepthStencilEnabled;

		
		static_vector<ResourceHandle, 16>		RenderTargets;
		static_vector<D3D12_VIEWPORT, 16>		Viewports;
		static_vector<DescriptorHeap*>			DesciptorHeaps;
		static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;

		struct StreamOutResource {
			SOResourceHandle	handle;
		};

		struct RTV_View {
			ResourceHandle  resource;
			DescHeapPOS     descriptor;
		};

		static_vector<StreamOutResource, 128>		TrackedSOBuffers;
		static_vector<Barrier, 128>					pendingBarriers; // Barriers potentially needed
		static_vector<Barrier, 128>					queuedBarriers; // Barriers required
		static_vector<RTV_View, 128>				renderTargetViews;
		static_vector<RTV_View, 128>				depthStencilViews;
		static_vector<ReadBackResourceHandle, 128>	queuedReadBacks;

		iAllocator*									Memory;

#if USING(AFTERMATH)
public:
		GFSDK_Aftermath_ContextHandle   AFTERMATH_context;
private:
#endif
	};


	/************************************************************************************************/


	template<typename ... ARGS>
	void SetScissorAndViewports(Context& ctx, std::tuple<ARGS...>	RenderTargets)
	{
		static_vector<D3D12_VIEWPORT, 16>	VPs;
		static_vector<D3D12_RECT, 16>		Rects;

		Tuple_For(
			RenderTargets,
			[&](auto target)
			{
				const auto WH = ctx.renderSystem->GetTextureWH(target);
				VPs.push_back({ 0,0, (FLOAT)WH[0], (FLOAT)WH[1], 0, 1 });
				Rects.push_back({ 0,0, (LONG)WH[0], (LONG)WH[1] });
			});

		ctx.SetViewports(VPs);
		ctx.SetScissorRects(Rects);
	}

	template<typename TY_RES1, typename TY_RES2>
	void CopyTexture2D(Context& ctx, TY_RES1 des, TY_RES2 src)
	{
		ctx.FlushBarriers();

		ctx.GetCommandList()->CopyResource(
			ctx.renderSystem->GetDeviceResource(des),
			ctx.renderSystem->GetDeviceResource(src));
	}


	/************************************************************************************************/


	struct GPUHeapAllocation
	{
		size_t offset   = 0;
		size_t size     = 0;

		ResourceHandle Overlap = InvalidHandle;

		operator bool() { return size > 0; }
	};

	struct AcquireResult
	{
		ResourceHandle resource;
		ResourceHandle overlap;
	};

	struct AcquireDeferredRes
	{
		ResourceHandle      resource    = InvalidHandle;
		ResourceHandle      overlap     = InvalidHandle;

		size_t              offset      = 0;
		DeviceHeapHandle    heap        = InvalidHandle;
	};

	struct PoolAllocatorInterface
	{
		virtual ~PoolAllocatorInterface() {};

		virtual AcquireResult		Acquire			(GPUResourceDesc desc, bool temporary = false) = 0;
		virtual AcquireDeferredRes	AcquireDeferred	(GPUResourceDesc desc, bool temporary = false) = 0;

		virtual AcquireResult		Recycle(ResourceHandle resource, GPUResourceDesc desc) = 0;
		virtual void				Release(ResourceHandle handle, const bool freeResourceImmedate = true, const bool allowImmediateReuse = true) = 0;

		virtual uint32_t Flags() const = 0;
	};

	constexpr size_t DefaultBlockSize = 64 * KILOBYTE;

	class MemoryPoolAllocator : public PoolAllocatorInterface
	{
	public:
		MemoryPoolAllocator(RenderSystem&, size_t IN_heapSize, size_t IN_blockSize, uint32_t IN_flags, iAllocator* IN_allocator);
		MemoryPoolAllocator(const MemoryPoolAllocator& rhs)             = delete;
		MemoryPoolAllocator& operator =(const MemoryPoolAllocator& rhs) = delete;

		~MemoryPoolAllocator() override;

		enum NodeFlags
		{
			Clear               = 0x00,
			AllowReallocation   = 0x2,
			Locked              = 0x4,
			Temporary           = 0x8,
		};
		
		GPUHeapAllocation   GetMemory       (const size_t requestBlockCount, const uint64_t frameID, const uint64_t flags);
		AcquireResult       Acquire         (GPUResourceDesc desc, bool temporary) final override;
		AcquireDeferredRes  AcquireDeferred (GPUResourceDesc desc, bool temporary) final override;
		AcquireResult       Recycle         (ResourceHandle resource, GPUResourceDesc desc) final override;

		uint32_t Flags() const final override;

		void Release(ResourceHandle handle, const bool freeResourceImmedate = true, const bool allowImmediateReuse = true) final override;

		void Coalesce();

		const uint32_t      flags;
		size_t              blockCount;
		size_t              blockSize;

		DeviceHeapHandle    heap;
		RenderSystem&       renderSystem;

		struct MemoryRange
		{
			uint32_t        offset;
			uint32_t        blockCount;
			uint64_t        flags;
			uint64_t        frameID;
			ResourceHandle  priorAllocation = InvalidHandle;

			friend bool operator < (MemoryRange& lhs, MemoryRange& rhs)
			{
				return lhs.blockCount < rhs.blockCount;
			}
		};

		struct Allocation
		{
			uint32_t        offset;
			uint32_t        blockCount;
			size_t          frameID;
			uint64_t        flags;
			ResourceHandle  resource = InvalidHandle;
		};

		Vector<MemoryRange> freeRanges;
		Vector<Allocation>  allocations;

		std::mutex   m;
		iAllocator*  allocator;
	};

	
	/************************************************************************************************/

	// INTERNAL USE ONLY!
	FLEXKITAPI UploadSegment		ReserveUploadBuffer		(RenderSystem& renderSystem, const size_t uploadSize, CopyContextHandle = InvalidHandle);
	FLEXKITAPI void					MoveBuffer2UploadBuffer	(const UploadSegment& data, const byte* source, const size_t uploadSize);

	FLEXKITAPI DescHeapPOS PushRenderTarget				(RenderSystem* RS, ResourceHandle    target, DescHeapPOS POS, const size_t MIPOffset = 0);


	FLEXKITAPI DescHeapPOS PushDepthStencil				(RenderSystem* RS, ResourceHandle Target, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushDepthStencilArray		(RenderSystem* RS, ResourceHandle Target, size_t arrayOffset, size_t MipSlice, DescHeapPOS POS, size_t arraySize = -1);
	FLEXKITAPI DescHeapPOS PushCBToDescHeap				(RenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, size_t BufferSize, size_t offset = 0);
	FLEXKITAPI DescHeapPOS PushSRVToDescHeap			(RenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, size_t ElementCount, size_t Stride, D3D12_BUFFER_SRV_FLAGS Flags = D3D12_BUFFER_SRV_FLAG_NONE, size_t offset = 0);
	FLEXKITAPI DescHeapPOS Push2DSRVToDescHeap			(RenderSystem* RS, ID3D12Resource* Buffer, const DescHeapPOS POS, const D3D12_BUFFER_SRV_FLAGS = D3D12_BUFFER_SRV_FLAG_NONE, const DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);

	[[deprecated]]
	FLEXKITAPI DescHeapPOS PushTextureToDescHeap		(RenderSystem* RS, Texture2D tex, DescHeapPOS POS);

	FLEXKITAPI DescHeapPOS PushTextureToDescHeap		(RenderSystem* RS, DXGI_FORMAT format, ResourceHandle handle, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushTextureToDescHeap		(RenderSystem* RS, DXGI_FORMAT format, uint32_t highestMipLevel, ResourceHandle handle, DescHeapPOS POS);

	FLEXKITAPI DescHeapPOS PushTexture3DToDescHeap		(RenderSystem* RS, DXGI_FORMAT format, uint32_t mipCount, uint32_t highestDetailMip, uint32_t minLODClamp, ResourceHandle handle, DescHeapPOS POS);

	FLEXKITAPI DescHeapPOS PushCubeMapTextureToDescHeap	(RenderSystem* RS, Texture2D tex, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushCubeMapTextureToDescHeap	(RenderSystem* RS, ResourceHandle resource, DescHeapPOS POS, DeviceFormat format);

	FLEXKITAPI DescHeapPOS PushUAV1DToDescHeap			(RenderSystem* RS, ID3D12Resource* resource, DXGI_FORMAT format, uint mip, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushUAV2DToDescHeap			(RenderSystem* RS, Texture2D tex, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushUAV2DToDescHeap			(RenderSystem* RS, Texture2D tex, uint32_t mipLevel, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushUAV3DToDescHeap			(RenderSystem* RS, Texture2D tex, uint32_t width, DescHeapPOS POS);

	FLEXKITAPI DescHeapPOS PushUAVBufferToDescHeap		(RenderSystem* RS, UAVBuffer buffer, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushUAVBufferToDescHeap2		(RenderSystem* RS, UAVBuffer buffer, ID3D12Resource* counter, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushUAVCubeMapToDescHeap		(RenderSystem* RS, DXGI_FORMAT format, ID3D12Resource* resource, DescHeapPOS POS);


	/************************************************************************************************/


	ResourceHandle MoveTextureBufferToVRAM	(RenderSystem* RS, CopyContextHandle, TextureBuffer* buffer, DeviceFormat format);
	ResourceHandle MoveTextureBuffersToVRAM	(RenderSystem* RS, CopyContextHandle, TextureBuffer* buffer, size_t MIPCount, size_t arrayCount, DeviceFormat format);
	ResourceHandle MoveTextureBuffersToVRAM	(RenderSystem* RS, CopyContextHandle, TextureBuffer* buffer, size_t MIPCount, DeviceFormat format);

	ResourceHandle MoveBufferToDevice		(RenderSystem* RS, const char* buffer, const size_t, CopyContextHandle ctx = InvalidHandle);


	/************************************************************************************************/


	struct TriMesh
	{
		TriMesh() = default;
		TriMesh(const TriMesh& rhs) = default;

		struct LOD_Runtime
		{
			LOD_Runtime() = default;

			LOD_Runtime(const LOD_Runtime& rhs) :
				buffers         { rhs.buffers       },
				lodFileOffset   { rhs.lodFileOffset },
				lodSize         { rhs.lodSize       },
				state           { rhs.state.load()  },
				subMeshes       { rhs.subMeshes     },
				vertexBuffer    { rhs.vertexBuffer  } {}

			LOD_Runtime& operator =(const LOD_Runtime& rhs)
			{
				buffers         = rhs.buffers;

				lodFileOffset   = rhs.lodFileOffset;
				lodSize         = rhs.lodSize;

				state           = rhs.state.load();
				subMeshes       = rhs.subMeshes;

				vertexBuffer    = rhs.vertexBuffer;

				return *this;
			}

			bool HasTangents() const
			{
				for (auto view : buffers)
				{
					if (view && view->GetBufferType() == VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT)
						return true;
				}

				return false;
			}

			bool HasNormals() const
			{
				for (auto view : buffers)
				{
					if (view && view->GetBufferType() == VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL)
						return true;
				}

				return false;
			}

			VertexBufferView* GetNormals()
			{
				for (auto view : buffers)
				{
					if (view && view->GetBufferType() == VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL)
						return view;
				}

				return nullptr;
			}


			VertexBufferView* GetIndices()
			{
				for (auto view : buffers)
				{
					if (view && view->GetBufferType() == VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX)
						return view;
				}

				return nullptr;
			}

			VertexBufferView* GetPoints()
			{
				for (auto view : buffers)
				{
					if (view && view->GetBufferType() == VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION)
						return view;
				}

				return nullptr;
			}

			size_t GetIndexBufferIndex() const
			{
				return vertexBuffer.MD.IndexBuffer_Index;
			}

			size_t GetIndexCount() const
			{
				return vertexBuffer.MD.InputElementCount;
			}

			size_t lodFileOffset;
			size_t lodSize;

			enum class LOD_State
			{
				Unloaded,
				Loaded,
				Loading,
				GPUResourceEvicted
			};

			static_vector<VertexBufferView*>    buffers;
			FlexKit::VertexBuffer		        vertexBuffer;
			D3D12_GPU_VIRTUAL_ADDRESS           blAS = -1; // TODO(Wrap this type)

			std::atomic<LOD_State>      state   = LOD_State::Unloaded;
			static_vector<SubMesh, 32>  subMeshes;
		};


		struct MorphTarget
		{
			ResourceHandle gpuResource;
		};


		struct MorphTargetAsset
		{
			char name[32];
			size_t offset;
			size_t size;
		};


		const uint32_t GetHighestLoadedLodIdx() const
		{
			for (uint32_t I = 0; I < lods.size(); I++)
			{
				if (lods[I].state == LOD_Runtime::LOD_State::Loaded)
					return I;
			}

			return -1;
		}

		const LOD_Runtime&  GetHighestLoadedLod() const
		{
			for (auto& lod : lods)
			{
				if (lod.state == LOD_Runtime::LOD_State::Loaded)
					return lod;
			}

			return lods.back();
		}

		LOD_Runtime& GetHighestLoadedLod()
		{
			for (auto& lod : lods)
			{
				if (lod.state == LOD_Runtime::LOD_State::Loaded)
					return lod;
			}

			return lods.back();
		}

		const uint32_t		GetLowestLodIdx()		const { return uint32_t(lods.size() - 1); }
		const LOD_Runtime&	GetLowestLoadedLod()	const { return lods.back(); }

		size_t animationData	 = EAD_None;
		size_t triMeshID		= INVALIDHANDLE;

		struct SubDivInfo
		{
			size_t  numVertices;
			size_t  numFaces;
			int* numVertsPerFace;
			int* IndicesPerFace;
		}*subDiv = nullptr;

		const char*		ID			= nullptr;
		SkinDeformer*	skinTable	= nullptr;

		GUID_t							assetHandle = INVALIDHANDLE;
		static_vector<LOD_Runtime>		lods;


		struct RInfo
		{
			float3 Offset;
			float3 Min, Max;
			float  r;
		}info;

		// Visibility Information
		AABB			AABB;
		BoundingSphere	BS;

		size_t			skeletonGUID	= INVALIDHANDLE;
		iAllocator*		allocator		= nullptr;

		static_vector<MorphTarget>		morphTargets;
		static_vector<MorphTargetAsset>	morphTargetAssets;
	};


	/************************************************************************************************/


	struct Shader
	{
		Shader() = default;

		Shader(ID3DBlob* blob, iAllocator* IN_allocator = SystemAllocator) :
			allocator{ IN_allocator }
		{
			buffer = (char*)allocator->malloc(blob->GetBufferSize());
			memcpy(buffer, blob->GetBufferPointer(), blob->GetBufferSize());
			bufferSize = blob->GetBufferSize();
		}

		Shader(IDxcBlob* blob, iAllocator* IN_allocator = SystemAllocator) :
			allocator{ IN_allocator }
		{
			buffer = (char*)allocator->malloc(blob->GetBufferSize());
			memcpy(buffer, blob->GetBufferPointer(), blob->GetBufferSize());
			bufferSize = blob->GetBufferSize();
		}

		Shader(const Shader& rhs)
		{
			buffer = (char*)rhs.allocator->malloc(rhs.bufferSize);

			memcpy(buffer, rhs.buffer, rhs.bufferSize);
			bufferSize = rhs.bufferSize;
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
		operator D3D12_SHADER_BYTECODE() const { return{ (BYTE*)buffer, bufferSize }; }

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


	inline ID3D12PipelineState* LoadComputeShader(const Shader& computeShader, const RootSignature& rootSignature, RenderSystem& renderSystem)
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			rootSignature,
			computeShader
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = renderSystem.pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

		FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

		return PSO;
	}


	/************************************************************************************************/


	struct ShadowMapPass
	{
		ID3D12PipelineState* PSO_Animated;
		ID3D12PipelineState* PSO_Static;
	};


	/************************************************************************************************/


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

	const size_t PLB_Stride  = sizeof(float[8]);
	const size_t SPLB_Stride = sizeof(float[12]);


	/************************************************************************************************/

		
	constexpr size_t AlignedSize(const size_t unalignedSize)
	{
		const auto alignment        = 256;
		const auto mask             = alignment - 1;
		const auto offset           = unalignedSize & mask;
		const auto adjustedOffset   = offset != 0 ? 256 - offset : 0;

		return unalignedSize + adjustedOffset;
	}

	template<typename TY>
	constexpr size_t AlignedSize()
	{
		return AlignedSize(sizeof(TY));
	}


	/************************************************************************************************/


	class CBPushBuffer
	{
	public:
		CBPushBuffer(ConstantBufferHandle cBuffer = InvalidHandle, char* IN_buffer = nullptr, size_t offsetBegin = 0, size_t reservedSize = 0) :
			CB				{ cBuffer		},
			buffer			{ IN_buffer		},
			pushBufferSize	{ reservedSize	},
			pushBufferBegin	{ offsetBegin	} {}


		CBPushBuffer(ConstantBufferHandle IN_CB, size_t reserveSize, RenderSystem& renderSystem)
		{
			auto reservedBuffer = renderSystem.ConstantBuffers.Reserve(IN_CB, reserveSize);

			CB				= IN_CB;
			buffer			= reservedBuffer.Data;
			pushBufferSize	= reserveSize;
			pushBufferBegin	= reservedBuffer.offsetBegin;
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

			pushBuffer.CB				= InvalidHandle;
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

			pushBuffer.CB				= InvalidHandle;
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
			return Max((sizeof(TY) / 256) * 256, 256);
		}

		static constexpr size_t CalculateOffset(const size_t size)
		{
			return Max((size / 256) * 256, 256);
		}

		template<typename _TY>
		size_t Push(const _TY& data)
		{
			constexpr size_t alignedSize = CalculateOffset<_TY>();
			if (pushBufferUsed + alignedSize > pushBufferSize)
			{
				FK_LOG_ERROR("Failed To Push Constants");
				return -1;
			}

			const size_t offset = pushBufferUsed;
			memcpy(buffer + pushBufferBegin + offset, (char*)&data, sizeof(data));
			pushBufferUsed += CalculateOffset<_TY>();

			return offset + pushBufferBegin;
		}

		size_t Push(char* _ptr, const size_t size)
		{
			if (pushBufferUsed + AlignedSize(size) > pushBufferSize)
				return -1;

			const size_t offset = pushBufferUsed;
			memcpy(buffer + pushBufferBegin + offset, _ptr, size);
			pushBufferUsed += AlignedSize(size);

			return offset + pushBufferBegin;
		}

		size_t begin() const { return pushBufferBegin; }

	private:
		ConstantBufferHandle	CB				= InvalidHandle;
		char*					buffer			= nullptr;
		size_t					pushBufferBegin = 0;
		size_t					pushBufferSize	= 0;
		size_t					pushBufferUsed	= 0;
	};


	class VBPushBuffer
	{
	public:
		VBPushBuffer(VertexBufferHandle vBuffer = InvalidHandle, char* buffer_ptr = nullptr, size_t offsetBegin = 0, size_t reservedSize = 0) noexcept :
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

			pushBuffer.VB				= InvalidHandle;
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

			pushBuffer.VB				= InvalidHandle;
			pushBuffer.buffer			= nullptr;
			pushBuffer.pushBufferOffset	= 0;
			pushBuffer.pushBufferSize	= 0;
			pushBuffer.pushBufferUsed	= 0;

			return *this; 
		}

		operator VertexBufferHandle () { return VB; }


		template<typename _TY>
		size_t Push(const _TY& data) noexcept
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
			pushBufferUsed += (size + 255) & ~255;

			return offset;
		}


		size_t begin() { return pushBufferOffset; }


		size_t GetOffset()const noexcept
		{
			return pushBufferUsed + pushBufferOffset;
		}

		VertexBufferHandle	VB					= InvalidHandle;
		char*				buffer				= 0;
		size_t				pushBufferOffset	= 0;
		size_t				pushBufferSize		= 0;
		size_t				pushBufferUsed		= 0;
	};


	/************************************************************************************************/


	inline CBPushBuffer Reserve(ConstantBufferHandle CB, size_t pushSize, size_t count, RenderSystem& renderSystem)
	{
		size_t reserveSize = (pushSize / 256 + 1) * 256 * count;
		auto buffer = renderSystem.ConstantBuffers.Reserve(CB, reserveSize);

		return { CB, buffer.Data, buffer.offsetBegin, reserveSize };
	}


	inline VBPushBuffer Reserve(VertexBufferHandle VB, size_t reserveSize, Context& ctx)
	{
		auto buffer = ctx.renderSystem->VertexBuffers.Reserve(VB, reserveSize);
		return { VB, buffer.Data, buffer.offsetBegin, reserveSize };
	}


	/************************************************************************************************/



	class ConstantBufferDataSet
	{
	public:
		ConstantBufferDataSet() {}

		template<typename TY>
		ConstantBufferDataSet(const TY& initialData, CBPushBuffer& buffer) :
			constantBuffer	{ buffer                    },
			constantsOffset	{ buffer.Push(initialData)  },
			size            { AlignedSize<TY>()         } {}

		explicit ConstantBufferDataSet(const size_t& IN_offset, ConstantBufferHandle IN_buffer, size_t IN_size = 0) :
			constantBuffer  { IN_buffer },
			constantsOffset { IN_offset },
			size            { IN_size   } {} 

		explicit ConstantBufferDataSet(char* initialData, const size_t bufferSize, CBPushBuffer& buffer) :
			constantBuffer  { buffer                                },
			constantsOffset { buffer.Push(initialData, bufferSize)  },
			size            { AlignedSize(bufferSize)               } {}

		~ConstantBufferDataSet() = default;

		ConstantBufferDataSet(const ConstantBufferDataSet& rhs)					= default;
		ConstantBufferDataSet& operator = (const ConstantBufferDataSet& rhs)	= default;

		ConstantBufferHandle    Handle() const { return constantBuffer;	}
		size_t				    Offset() const { return constantsOffset; }
		size_t                  Size() const { return size; }

	private:
		ConstantBufferHandle	constantBuffer		= InvalidHandle;
		size_t					constantsOffset		= 0;
		size_t                  size                = 0;
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
			const ConstantBufferHandle	CB = InvalidHandle;
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
		VertexBufferDataSet(const TY& initialData, VBPushBuffer& buffer) :
			vertexBuffer	{ buffer },
			offsetBegin		{ buffer.GetOffset() },
			vertexStride    { sizeof(initialData[0]) }
		{

			for (auto& vertex : initialData) 
				buffer.Push(vertex);
		}

		template<typename TY>
		VertexBufferDataSet(const TY* buffer, size_t bufferSize, VBPushBuffer& pushBuffer) :
			vertexBuffer	{ pushBuffer },
			offsetBegin		{ pushBuffer.GetOffset() }
		{
			vertexStride = sizeof(TY);

			if (pushBuffer.Push((char*)buffer, bufferSize) == -1)
				throw std::exception("buffer too short to accomdate push!");
		}

		template<typename TY, typename FN_TransformVertex>
		VertexBufferDataSet(const SET_MAP_t, const TY& initialData, const FN_TransformVertex& TransformVertex, VBPushBuffer& buffer) :
			vertexBuffer	{ buffer },
			offsetBegin		{ buffer.GetOffset() }
		{
			vertexStride = sizeof(decltype(TransformVertex(initialData[0])));

			for (const auto& vertex : initialData) {
				auto transformedVertex = TransformVertex(vertex);
				if (buffer.Push(transformedVertex) == -1)
					throw std::exception("buffer too short to accomdate push!");
			}
		}


		template<typename TY_CONTAINER, typename FN_TransformVertex>
		VertexBufferDataSet(const SET_TRANSFORM_t, const TY_CONTAINER& initialData, const FN_TransformVertex& TransformVertex, VBPushBuffer& buffer) :
			vertexBuffer	{ buffer },
			offsetBegin		{ buffer.GetOffset() }
		{
			using TY = decltype(TransformVertex(initialData.front(), buffer));
			vertexStride = sizeof(decltype(TransformVertex(initialData.front(), buffer)));

			for (const auto& vertex : initialData)
				TransformVertex(vertex, buffer);
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
		VertexBufferHandle	vertexBuffer	= InvalidHandle;
	};


	/************************************************************************************************/


	inline struct _GeometryTable
	{
		HandleUtilities::HandleTable<TriMeshHandle>		Handles;
		Vector<TriMesh>									Geometry;
		Vector<size_t>									ReferenceCounts;
		Vector<GUID_t>									Guids;
		Vector<const char*>								GeometryIDs;
		Vector<TriMeshHandle>							Handle;
		Vector<size_t>									FreeList;
		iAllocator*										allocator;
		RenderSystem*									renderSystem	= nullptr;
	}GeometryTable;


	/************************************************************************************************/


	FLEXKITAPI void							InitiateGeometryTable	(RenderSystem* renderSystem, iAllocator* memory = nullptr);
	FLEXKITAPI void							ReleaseGeometryTable	();

	FLEXKITAPI void							AddRef					( TriMeshHandle  TMHandle );
	FLEXKITAPI void							ReleaseMesh				( TriMeshHandle  TMHandle );

	FLEXKITAPI TriMeshHandle				LoadMesh				( GUID_t TMHandle );

	FLEXKITAPI TriMeshHandle				GetMesh					( GUID_t, CopyContextHandle copyCtx = InvalidHandle);
	FLEXKITAPI TriMeshHandle				GetMesh					( const char* meshID, CopyContextHandle = InvalidHandle);


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


	inline ID3D12Resource* GetBuffer(TriMesh* Mesh, size_t lod, size_t Buffer)	{ return Mesh->lods[lod].vertexBuffer[Buffer]; }


	inline ID3D12Resource* FindBuffer(TriMesh* Mesh, size_t lod, VERTEXBUFFER_TYPE Type)
	{
		ID3D12Resource* Buffer = nullptr;
		auto& VertexBuffers = Mesh->lods[lod].vertexBuffer.VertexBuffers;
		auto RES = find(VertexBuffers, [Type](auto& V) -> bool {return V.Type == Type;});

		if (RES != VertexBuffers.end())
			Buffer = RES->Buffer;

		return Buffer;
	}


	/************************************************************************************************/


	inline VertexBuffer::BuffEntry* FindBufferEntry(TriMesh* Mesh, size_t lod, VERTEXBUFFER_TYPE Type)
	{
		ID3D12Resource* Buffer = nullptr;
		auto& VertexBuffers = Mesh->lods[lod].vertexBuffer.VertexBuffers;
		auto RES = find(VertexBuffers, [Type](auto& V) -> bool {return V.Type == Type;});

		if (RES != VertexBuffers.end())
			Buffer = RES->Buffer;

		return (VertexBuffer::BuffEntry*)RES; // Remove Const of result
	}


	/************************************************************************************************/


	inline bool AddVertexBuffer(VERTEXBUFFER_TYPE Type, TriMesh* Mesh, size_t lod, static_vector<D3D12_VERTEX_BUFFER_VIEW>& out) 
	{
		auto* VB = FindBufferEntry(Mesh, lod, Type);

		if (VB == Mesh->lods[lod].vertexBuffer.VertexBuffers.end() || VB->Buffer == nullptr) {
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
	

	StaticObjectHandle CreateBrush(StaticScene* Scene, NodeHandle node, size_t GeometryIndex = 0);

	// TODO: re-implement StaticMeshBatcher
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


	FLEXKITAPI void	Release						(RenderSystem* System);
	FLEXKITAPI void Push_DelayedRelease			(RenderSystem* RS, ID3D12Resource* Res);
	FLEXKITAPI void Push_DelayedReleaseCopy     (RenderSystem* RS, ID3D12Resource* Res);
	FLEXKITAPI void Free_DelayedReleaseResources(RenderSystem* RS);


	FLEXKITAPI void AddTempBuffer	(ID3D12Resource* _ptr, RenderSystem* RS);
	FLEXKITAPI void AddTempShaderRes(ShaderResourceBuffer& ShaderResource, RenderSystem* RS);


	inline DescHeapPOS IncrementHeapPOS(const DescHeapPOS POS, const size_t size, const size_t increment) {
		const size_t offset = size * increment;
		return { POS.V1.ptr + offset, POS.V2.ptr + offset };
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
	// Depreciated API

	FLEXKITAPI void CreateVertexBuffer			( RenderSystem* RS, CopyContextHandle handle, VertexBufferView** Buffers, size_t BufferCount, VertexBuffer& DVB_Out ); // Expects Index buffer in index 15

	struct SubResourceUpload_Desc
	{
		TextureBuffer*  buffers;

		size_t	        subResourceStart;
		size_t	        subResourceCount;

		DeviceFormat       format;
	};

	FLEXKITAPI void _UpdateSubResourceByUploadQueue (RenderSystem* RS, CopyContextHandle, ID3D12Resource* Dest, SubResourceUpload_Desc* Desc);


	/************************************************************************************************/


	FLEXKITAPI void	Release( ConstantBuffer&	);
	FLEXKITAPI void	Release( Texture2D			);
	FLEXKITAPI void	Release( RenderWindow*		);
	FLEXKITAPI void	Release( VertexBuffer*		);


	/************************************************************************************************/


	FLEXKITAPI ResourceHandle	LoadDDSTextureFromFile			(char* file, RenderSystem* RS, CopyContextHandle, iAllocator* Memout);
	FLEXKITAPI ResourceHandle   LoadTexture						(TextureBuffer* Buffer,  RenderSystem* RS, iAllocator* Memout, DeviceFormat format = DeviceFormat::R8G8B8A8_UNORM);


	//FLEXKITAPI Shader           LoadShader_OLD                  (const char* Entry, const char* ShaderVersion, const char* File);


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


	FLEXKITAPI Texture2D		GetBackBufferTexture	( RenderWindow* Window );
	FLEXKITAPI ID3D12Resource*	GetBackBufferResource	( RenderWindow* Window );


	/************************************************************************************************/


	inline float2 WStoSS		( const float2 XY )	{ return (XY * float2(2, 2)) + float2(-1, -1); }
	inline float2 Position2SS	( const float2 in )	{ return{ in.x * 2 - 1, in.y * -2 + 1 }; }
	inline float3 Grey			( const float P )	{ float V = Min(Max(0, P), 1); return float3(V, V, V); }


	/************************************************************************************************/


	struct Obj_Desc
	{
		Obj_Desc()
		{
			S = 1.0f;

			LoadUVs = false;
			GenerateTangents = false;
		}

		Obj_Desc(float scale, bool UV = false, bool T = false) : 
			S{ scale },
			LoadUVs{UV},
			GenerateTangents{T}{}


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


	inline void ClearTriMeshVBVs(TriMesh* Mesh) { for (auto& lod : Mesh->lods) for (auto& buffer : lod.buffers) buffer = nullptr; }


	/************************************************************************************************/

	[[nodiscard]]
	inline auto CreateVertexBufferReserveObject(
		VertexBufferHandle	vertexBuffer,
		RenderSystem*		renderSystem,
		iAllocator*			allocator)
	{
		return MakeSynchonized(
			[=](size_t reserveSize) -> VBPushBuffer
			{
				return VBPushBuffer(vertexBuffer, reserveSize, *renderSystem);
			},
			allocator);
	}

	[[nodiscard]]
	inline auto CreateConstantBufferReserveObject(
		ConstantBufferHandle	constantBuffer,
		RenderSystem*			renderSystem,
		iAllocator*				allocator)
	{
		return MakeSynchonized(
			[=](const size_t reserveSize) -> CBPushBuffer
			{
				auto push = CBPushBuffer(constantBuffer, reserveSize, *renderSystem);
				return std::move(push);
			},
			allocator);
	}

	using ReserveVertexBufferFunction	= decltype(CreateVertexBufferReserveObject(InvalidHandle, nullptr, nullptr));
	using ReserveConstantBufferFunction	= decltype(CreateConstantBufferReserveObject(InvalidHandle, nullptr, nullptr));

	[[nodiscard]]
	inline auto CreateOnceReserveBuffer(ReserveConstantBufferFunction& reserveConstants, iAllocator* allocator)
	{
		return MakeLazyObject<CBPushBuffer>(
			allocator,
			[reserveConstants = reserveConstants](size_t reserveSize) mutable
			{
				return reserveConstants(reserveSize);
			});
	}

	using CreateOnceReserveBufferFunction = decltype(CreateOnceReserveBuffer(*((ReserveConstantBufferFunction*)nullptr), nullptr));


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


}	/************************************************************************************************/

/**********************************************************************

Copyright (c) 2014-2021 Robert May

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
