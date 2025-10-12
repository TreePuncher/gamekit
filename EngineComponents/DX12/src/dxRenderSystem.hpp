#pragma once
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

#include "PipelineState.hpp"

#include <BuildSettings.hpp>
#include <Containers.hpp>
#include <Geometry.hpp>
#include <Handle.hpp>
#include <Logging.hpp>
#include <MathUtilities.hpp>
#include <MemoryUtilities.hpp>
#include <Type.hpp>
#include <ThreadUtilities.hpp>
#include <ResourceHandles.hpp>
#include <RenderSystemInterface.hpp>

#include "dxContext.hpp"
#include "dxIndirectLayout.hpp"

#include <algorithm>
#include <string>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>
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

//#include <pix3.h>
//#include <DXProgrammableCapture.h>

#endif


struct ID3D12Device9;
struct ID3D12Debug5;
struct ID3D12DebugDevice1;


namespace dx_Internal
{   // Forward Declarations
	using namespace FlexKit;
	using FlexKit::IContext;

    /*
	struct buffer;
	struct RenderTargetDesc;
	struct RenderWindowDesc;
	struct RenderViewDesc;
	struct RenderWindow;
	struct ShaderResource;
	struct Texture2D;
	struct TriMesh;

	class ConstantBufferDataSet;
	class StackAllocator;
	class VertexBufferDataSet;
    */
	class dxDirectContext;
	class dxRenderSystem;

    /************************************************************************************************/


	struct SODesc
	{
		D3D12_SO_DECLARATION_ENTRY* Descs;
		size_t							Element_Count;
		size_t							SO_Count;
		size_t							Flags;
		UINT							Strides[16];
	};


	/************************************************************************************************/


#pragma warning(disable:4067)
FLEXKITAPI void SetDebugName(ID3D12Object* Obj, const char* cstr, size_t size);
#ifdef USING(DEBUGGRAPHICS)
#define SETDEBUGNAME(RES, ID) {const char* NAME = ID; dx_Internal::SetDebugName(RES, ID, strnlen(ID, 64));}

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


	inline D3D12_GPU_VIRTUAL_ADDRESS_RANGE DeviceAddressRangeToDX(const FlexKit::DeviceAddressRange& range)
	{
		return { range.address, range.size };
	}

	inline D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE DeviceAddressRangeStrideToDX(const DeviceAddressRangeStride& rangeStride)
	{
		return { rangeStride.address, rangeStride.size, rangeStride.stride };
	}


	/************************************************************************************************/


	inline constexpr DeviceLayout DecayLayout(DeviceLayout layout)
	{
		switch (layout)
		{
		case DeviceLayout::ShaderResource:
			return DeviceLayout::Common;
		default:
			return layout;
		}
	}


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
		case DeviceLayout::Common:
			return D3D12_BARRIER_LAYOUT_COMMON;
		case DeviceLayout::Present:
			return D3D12_BARRIER_LAYOUT_PRESENT;
		case DeviceLayout::GenericRead:
			return D3D12_BARRIER_LAYOUT_GENERIC_READ;
		case DeviceLayout::RenderTarget:
			return D3D12_BARRIER_LAYOUT_RENDER_TARGET;
		case DeviceLayout::UnorderedAccess:
			return D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;
		case DeviceLayout::DepthStencilWrite:
			return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;
		case DeviceLayout::DepthStencilRead:
			return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ;
		case DeviceLayout::ShaderResource:
			return D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
		case DeviceLayout::CopySrc:
			return D3D12_BARRIER_LAYOUT_COPY_SOURCE;
		case DeviceLayout::CopyDst:
			return D3D12_BARRIER_LAYOUT_COPY_DEST;
		case DeviceLayout::ResolveSrc:
			return D3D12_BARRIER_LAYOUT_RESOLVE_SOURCE;
		case DeviceLayout::ResolveDst:
			return D3D12_BARRIER_LAYOUT_RESOLVE_DEST;
		case DeviceLayout::ShadingRateSrc:
			return D3D12_BARRIER_LAYOUT_SHADING_RATE_SOURCE;
		case DeviceLayout::VideoDecodeRead:
			return D3D12_BARRIER_LAYOUT_VIDEO_DECODE_READ;
		case DeviceLayout::DecodeWrite:
			return D3D12_BARRIER_LAYOUT_VIDEO_DECODE_WRITE;
		case DeviceLayout::ProcessRead:
			return D3D12_BARRIER_LAYOUT_VIDEO_PROCESS_READ;
		case DeviceLayout::ProcessWrite:
			return D3D12_BARRIER_LAYOUT_VIDEO_PROCESS_WRITE;
		case DeviceLayout::EncodeRead:
			return D3D12_BARRIER_LAYOUT_VIDEO_ENCODE_READ;
		case DeviceLayout::EncodeWrite:
			return D3D12_BARRIER_LAYOUT_VIDEO_ENCODE_WRITE;
		case DeviceLayout::DirectQueueCommon:
			return D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COMMON;
		case DeviceLayout::DirectQueueGenericRead:
			return D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_GENERIC_READ;
		case DeviceLayout::DirectQueueUnorderedAccess:
			return D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS;
		case DeviceLayout::DirectQueueShaderResource:
			return D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE;
		case DeviceLayout::DirectQueueCopySrc:
			return D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_SOURCE;
		case DeviceLayout::DirectQueueCopyDst:
			return D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_DEST;
		case DeviceLayout::ComputeQueueCommon:
			return D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COMMON;
		case DeviceLayout::ComputeQueueGenericRead:
			return D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_GENERIC_READ;
		case DeviceLayout::ComputeQueueUnorderedAccess:
			return D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_UNORDERED_ACCESS;
		case DeviceLayout::ComputeQueueShaderResource:
			return D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_SHADER_RESOURCE;
		case DeviceLayout::ComputeQueueCopySrc:
			return D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COPY_SOURCE;
		case DeviceLayout::ComputeQueueCopyDst:
			return D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COPY_DEST;
		case DeviceLayout::VideoQueueCommon:
			return D3D12_BARRIER_LAYOUT_VIDEO_QUEUE_COMMON;
		case DeviceLayout::Undefined:
			return D3D12_BARRIER_LAYOUT_UNDEFINED;
		case DeviceLayout::Unknown:
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
		case Sync_EmitRaytracingAccelerationStructurePostBuildInfo:
			return D3D12_BARRIER_SYNC_EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO;
		case Sync_VideoDecode:
			return D3D12_BARRIER_SYNC_VIDEO_DECODE;
		case Sync_VideoProcess:
			return D3D12_BARRIER_SYNC_VIDEO_PROCESS;
		case Sync_VideoEncode:
			return D3D12_BARRIER_SYNC_VIDEO_ENCODE;
		case Sync_BuildRaytracingAccelerationStructure:
			return D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
		case Sync_CopyRaytracingAccelerationStructure:
			return D3D12_BARRIER_SYNC_COPY_RAYTRACING_ACCELERATION_STRUCTURE;
		case Sync_Mesh:
			return D3D12_BARRIER_SYNC_COMPUTE_SHADING;
		case Sync_Amplification:
		    return D3D12_BARRIER_SYNC_COMPUTE_SHADING;
		case Sync_Unknown:
			return D3D12_BARRIER_SYNC_NONE;
		}

		std::unreachable();
	}

	inline D3D12_BARRIER_SYNC SyncPoint2DX_Forward(const DeviceSyncPoint syncPoint)
	{
		D3D12_BARRIER_SYNC out = SyncPoint2DX(syncPoint);

		out = (syncPoint & DeviceSyncPoint::Sync_RenderTarget != 0) ? D3D12_BARRIER_SYNC_RENDER_TARGET : out;
		out = (syncPoint & DeviceSyncPoint::Sync_DepthStencil != 0) ? D3D12_BARRIER_SYNC_RENDER_TARGET : out;
		out = (syncPoint & DeviceSyncPoint::Sync_PixelShader != 0) ? D3D12_BARRIER_SYNC_PIXEL_SHADING : out;
		out = ((syncPoint & DeviceSyncPoint::Sync_VertexShader!= 0) |
		       (syncPoint & DeviceSyncPoint::Sync_HullShader != 0) | 
		       (syncPoint & DeviceSyncPoint::Sync_DomainShader != 0) | 
		       (syncPoint & DeviceSyncPoint::Sync_Mesh != 0) |
		       (syncPoint & DeviceSyncPoint::Sync_GeometryShader!= 0)) ? D3D12_BARRIER_SYNC_VERTEX_SHADING : out;

		out = (syncPoint & DeviceSyncPoint::Sync_IA != 0) ? D3D12_BARRIER_SYNC_INDEX_INPUT : out;

		return out;
	}

	inline D3D12_BARRIER_SYNC SyncPoint2DX_Backward(const DeviceSyncPoint syncPoint)
	{
		D3D12_BARRIER_SYNC out = SyncPoint2DX(syncPoint);

		out = (syncPoint & DeviceSyncPoint::Sync_IA != 0) ? D3D12_BARRIER_SYNC_INDEX_INPUT : out;
		out = (syncPoint & DeviceSyncPoint::Sync_Mesh != 0) ? D3D12_BARRIER_SYNC_COMPUTE_SHADING : out;
		out = ((syncPoint & DeviceSyncPoint::Sync_VertexShader!= 0) |
		       (syncPoint & DeviceSyncPoint::Sync_HullShader != 0) | 
		       (syncPoint & DeviceSyncPoint::Sync_DomainShader!= 0) |
		       (syncPoint & DeviceSyncPoint::Sync_Mesh != 0) |
		       (syncPoint & DeviceSyncPoint::Sync_GeometryShader!= 0)) ? D3D12_BARRIER_SYNC_VERTEX_SHADING : out;

	    out = (syncPoint & DeviceSyncPoint::Sync_PixelShader != 0) ? D3D12_BARRIER_SYNC_PIXEL_SHADING : out;
		out = (syncPoint & DeviceSyncPoint::Sync_DepthStencil != 0) ? D3D12_BARRIER_SYNC_RENDER_TARGET : out;
		out = (syncPoint & DeviceSyncPoint::Sync_RenderTarget != 0) ? D3D12_BARRIER_SYNC_RENDER_TARGET : out;

		return out;
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


	inline D3D12_SHADER_VISIBILITY PipelineDest2ShaderVis(PIPELINE PD)
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


	inline PIPELINE ShaderVis2PipelineDest(D3D12_SHADER_VISIBILITY visibility)
	{
		switch (visibility)
		{
		case D3D12_SHADER_VISIBILITY_HULL:
			return PIPELINE_DEST_HS;
		case D3D12_SHADER_VISIBILITY_GEOMETRY:
			return PIPELINE_DEST_GS;
		case  D3D12_SHADER_VISIBILITY_VERTEX:
			return PIPELINE_DEST_VS;
		case D3D12_SHADER_VISIBILITY_PIXEL:
			return PIPELINE_DEST_PS;
		case D3D12_SHADER_VISIBILITY_DOMAIN:
			return PIPELINE_DEST_DS;
		case D3D12_SHADER_VISIBILITY_AMPLIFICATION:
			return PIPELINE_DEST_AS;
		case D3D12_SHADER_VISIBILITY_MESH:
			return PIPELINE_DEST_MS;
		default:
			return FlexKit::PIPELINE_DEST_ALL;
		}

		std::unreachable();
	}


	/************************************************************************************************/


	constexpr DXGI_FORMAT   TextureFormat2DXGIFormat(DeviceFormat F) noexcept;
	DeviceFormat			DXGIFormat2TextureFormat(DXGI_FORMAT F) noexcept;


	/************************************************************************************************/


	using VertexResourceBuffer	= ID3D12Resource*;


	/************************************************************************************************/


	class DescriptorHeapAllocator
	{
	public:
		~DescriptorHeapAllocator();


		void							Initialize	(dxRenderSystem& IN_renderSystem, const size_t numDescCount, FlexKit::iAllocator* IN_allocator);

		std::optional<DescriptorRange>	Alloc_ST	(const size_t size, uint64_t completedIdx) noexcept;
		auto							Alloc		(const size_t size, uint64_t completedIdx) noexcept;

		void							Release_ST	(const DescriptorRange range, uint64_t lockIdx, uint64_t completed) noexcept;
		void							Release		(const DescriptorRange range, uint64_t lockIdx, uint64_t completed);

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
		Vector<Node*>				freeList;
		iAllocator*					allocator;
		dxRenderSystem*				renderSystem;
		size_t						descriptorSize;
		D3D12_GPU_DESCRIPTOR_HANDLE	gpuHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE	cpuHeap;
		std::mutex					mutex;
		ID3D12DescriptorHeap*		descHeap;
	};


	// Old


	/************************************************************************************************/


	struct UAVBuffer
	{
		UAVBuffer(const dxRenderSystem& rs, const ResourceHandle handle, const size_t stride = -1, const size_t offset = 0); // auto Fills the struct

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


	DevicePointer GetDevicePointer(const VertexBuffer& vb_ref) noexcept;


	typedef static_vector<D3D12_INPUT_ELEMENT_DESC, 16> InputDescription;

	struct TriangleMeshMetaData
	{
		D3D12_INPUT_ELEMENT_DESC	InputLayout[16];
		size_t						InputElementCount;
		size_t						IndexBuffer_Index;
	};


	struct VertexBufferSet : IVertexBufferSet
	{
		struct BuffEntry
		{
			ID3D12Resource*		apiResource;
			uint32_t			bufferSizeInBytes;
			uint32_t			bufferStride;
			VERTEXBUFFER_TYPE	type;

			size_t Size() const { return bufferSizeInBytes / bufferStride; }

			DevicePointer		GetDevicePointer()			const noexcept { return { apiResource->GetGPUVirtualAddress() }; }
			DeviceResource_ptr	GetDeviceResourcePointer()	const noexcept { return { apiResource }; }

			operator bool() const noexcept			{ return apiResource != nullptr; }
			operator DeviceResource_ptr const ()	{ return apiResource; }
		};


		virtual std::optional<VertexBuffer> Find(const VERTEXBUFFER_TYPE type) const final
		{
			auto res = std::find_if(
				buffers.begin(),
				buffers.end(),
				[&](auto& buffer)
				{
					return buffer.type == type;
				});

			if (res != buffers.end())
			{
				VertexBuffer out{
					.byteSize	= res->bufferSizeInBytes,
					.byteStride	= res->bufferStride,
					.resource	= res->apiResource,
					.type		= res->type
				};

				return out;
			}

			return {};
		}


		virtual uint8_t	GetIndexBufferIndex() const final
		{
			return MD.IndexBuffer_Index;
		}


		virtual const VertexBuffer operator []	(uint8_t idx) const final
		{
			auto& buffer = buffers[idx];
			return {
				.byteSize	= buffer.bufferSizeInBytes,
				.byteStride = buffer.bufferStride,
				.resource	= buffer.apiResource,
				.type		= buffer.type 
			};
		}

		virtual void Clear() final { buffers.clear(); }


		//ID3D12Resource*	operator[](size_t idx)	{ return buffers[idx].apiResource; }

		static_vector<BuffEntry, 16>	buffers;
		TriangleMeshMetaData			MD;
	};



	/************************************************************************************************/


	class CopyEngine;

	class CopyEngine
	{
	public:
		CopyEngine() = default;

		CopyContext& operator [](CopyContextHandle handle);

		CopyContextHandle Open();

		void Wait(SyncPoint syncTo);
		void Wait(CopyContextHandle handle);
		void Close(CopyContextHandle handle);

		void Submit(CopyContextHandle* begin, CopyContextHandle* end, std::optional<SyncPoint> sync = {});
		void Signal(ID3D12Fence* fence, const size_t counter);
		void Signal(SyncPoint);

		void Push_Temporary(struct ID3D12Resource* resource, CopyContextHandle handle);

		void Release();

		bool Initiate(ID3D12Device*, const size_t threadCount, Vector<ID3D12DeviceChild*>& ObjectsCreated, iAllocator*);

		ID3D12CommandQueue*					copyQueue		= nullptr;
		ID3D12Fence*						fence			= nullptr;
		std::atomic_uint					idx				= 0;
		std::atomic_uint					counter			= 0;
		CircularBuffer<CopyContext, 64>		copyContexts;
	};


	/************************************************************************************************/


	struct RootSignatureHeapEntry
	{
		size_t				idx;
		DesciptorHeapLayout	heap;
	};


	class RootSignatureBuilder
	{
	public:
		RootSignatureBuilder(iAllocator* Memory) :
			Heaps		{ Memory } {}

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

		[[nodiscard]]	RootSignature* Build(dxRenderSystem* RS, iAllocator& TempMemory);
		[[nodiscard]]	RootSignature* LoadSignatureFromFile(const char* dir, const char* entry, dxRenderSystem& renderSystem, iAllocator& temp);
		[[nodiscard]]	RootSignature* LoadSignatureFromBlob(void* _ptr, size_t size, dxRenderSystem& renderSystem, iAllocator& temp);

		bool AllowIA	= true;
		bool AllowSO	= false;
		bool LocalRoot	= false;

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
					PIPELINE	Accessibility;
				}UINTConstant;

				struct
				{
					size_t					HeapIdx;
					PIPELINE	Accessibility;
				}DescriptorHeap;

				struct
				{
					uint32_t				Register;
					uint32_t				RegisterSpace;
					PIPELINE	Accessibility;
				}Direct;
			};
		};

		Vector<RootSignatureHeapEntry>	Heaps;
		static_vector<RootEntry>		RootEntries;
	};


	/************************************************************************************************/


	class RootSignature : public IPipelineInterface
	{
	public:
		RootSignature(ID3D12RootSignature* rootSignature, Vector<RootSignatureHeapEntry>&& IN_heaps, iAllocator* IN_allocator) :
			allocator	{ IN_allocator	},
			Signature	{ rootSignature },
			Heaps		{ std::move(IN_heaps) },
			slots		{ IN_allocator } {}


		~RootSignature()
		{
			Release();
		}

		operator ID3D12RootSignature* ()	const { return Signature; }
		ID3D12RootSignature* Get_ptr()		const { return Signature; };

		void Release();

		virtual const DesciptorHeapLayout&	GetDescHeap(uint32_t idx) const noexcept final
		{
			return Heaps[idx].heap;
		}

		virtual DeviceRootSignature_ptr		GetAPIObject() const noexcept final
		{
			return Get_ptr();
		}


		size_t	GetDescriptorTableSize(size_t idx) const;

		void	SetDebugStr(const char* IN_debugStr) const
		{
#ifdef _DEBUG
			debugStr = IN_debugStr;
#endif
		}

		enum class SlotType
		{
			UINT, 
		    CBV,
			UAV,
			SRV,
			DescriptorSet
		};

		uint32_t GetIndex(uint32_t slot, RootSignature::SlotType type) const
		{
			uint32_t slotCounter = 0;
			for (auto [idx, s] : enumerate(slots))
			{
				if (s == type)
				{
					if (slotCounter == slot)
						return idx;

					slotCounter++;
				}
			}

			return -1u;
		}


		ID3D12RootSignature*			Signature = nullptr;
		iAllocator*						allocator = nullptr;
		Vector<RootSignatureHeapEntry>	Heaps;
		Vector<SlotType>				slots;

#ifdef _DEBUG
		mutable const char*				debugStr  = nullptr;
#endif
	};


	/************************************************************************************************/


	class VertexBufferStateTable
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

		VertexBufferHandle	CreateVertexBuffer	(size_t BufferSize, bool GPUResident, dxRenderSystem* RS); // Creates Using Placed Resource
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

		VBufferHandle	CreateVertexBufferResource(size_t BufferSize, bool GPUResident, dxRenderSystem* RS); // Creates Using Placed Resource

		bool    CurrentlyAvailable(VertexBufferHandle Handle, size_t CurrentFrame) const;

		char*   _Map(VBufferHandle);
		void    _UnMap(VBufferHandle, size_t range);

		struct VertexBuffer
		{
			ID3D12Resource* resource		= nullptr;
			size_t			resourceSize	= 0;
			size_t			lockCounter;
		};


		struct UserVertexBuffer
		{
			size_t			    CurrentBuffer;
			size_t			    Buffers[3];					// Current buffer
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


	struct ConstantBufferTable
	{
		ConstantBufferTable(iAllocator* allocator, dxRenderSystem* IN_renderSystem) :
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
		dxRenderSystem*				renderSystem;
		Vector<UserConstantBuffer>	buffers;
		std::mutex					criticalSection;

		HandleUtilities::HandleTable<ConstantBufferHandle>	handles;
	};


	/************************************************************************************************/

	// WARNING, NOT IMPLEMENTED FULLY!
	struct QueryTable
	{
		QueryTable(iAllocator* persistent, dxRenderSystem* RS_in) :
			users			{ persistent },
			resources		{ persistent },
			pendingFrees	{ persistent },
			RS				{ RS_in } {}


		~QueryTable()
		{
			Release();
		}


		void Release(QueryHandle query, uint64_t submissionCounter)
		{
			auto idx = users[query].resourceIdx;
			
			pendingFrees.emplace_back(submissionCounter, resources[idx].resources[0]);
			pendingFrees.emplace_back(submissionCounter, resources[idx].resources[1]);
			pendingFrees.emplace_back(submissionCounter, resources[idx].resources[2]);
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

		dxRenderSystem*			RS;
		Vector<UserEntry>		users;
		Vector<ResourceEntry>	resources;

		struct PendingFree
		{
			uint64_t			id;
			ID3D12QueryHeap*	resource;
		};

		Vector<PendingFree>		pendingFrees;
	};


	/************************************************************************************************/


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


	inline D3D12_CLEAR_VALUE ClearValue2DXClearValue(const ClearValue& CV)
	{
		D3D12_CLEAR_VALUE out;
		memcpy(&out.Color, &CV.color, sizeof(out.Color));
		out.Format = TextureFormat2DXGIFormat(CV.format);

		return out;
	}


	/************************************************************************************************/


	inline D3D12_RESOURCE_DESC1 GetD3D12ResourceDesc1(const GPUResourceDesc& desc)
	{
		const auto dxgiFormat = TextureFormat2DXGIFormat(desc.format);
		D3D12_RESOURCE_DESC1 out;

		switch (desc.Dimensions)
		{
		case TextureDimension::Buffer:
			out = CD3DX12_RESOURCE_DESC1::Buffer(desc.WH[0]);

			break;
		case TextureDimension::Texture1D:
			out = CD3DX12_RESOURCE_DESC1::Tex1D(
				dxgiFormat,
				desc.WH[0],
				desc.arraySize,
				desc.mipLevels);

			break;
		case TextureDimension::Texture2D:
			out = CD3DX12_RESOURCE_DESC1::Tex2D(
				dxgiFormat,
				desc.WH[0],
				desc.WH[1],
				desc.arraySize,
				desc.mipLevels);

			break;
		case TextureDimension::Texture3D:
			out = CD3DX12_RESOURCE_DESC1::Tex3D(
				dxgiFormat,
				desc.WH[0],
				desc.WH[1],
				desc.arraySize,
				desc.mipLevels);

			break;
		case TextureDimension::TextureCubeMap:
			out = CD3DX12_RESOURCE_DESC1::Tex2D(
				dxgiFormat,
				desc.WH[0],
				desc.WH[1],
				6,
				desc.mipLevels);
			break;
		};

		out.Flags = desc.type == ResourceType::RenderTarget ? D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		out.Flags |= desc.type == ResourceType::UnorderedAccess ? D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		out.Flags |= desc.type == ResourceType::DepthTarget ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		out.Flags |= desc.type == ResourceType::UnorderedAccessRenderTarget ? D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		out.Flags |= desc.type == ResourceType::RayTracingStructure ? D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE : D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		out.Flags |= desc.denyShaderUsage ? D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE : D3D12_RESOURCE_FLAG_NONE;

		out.MipLevels	= Min(Max(desc.mipLevels, 1), 15);

		return out;
	}


	/************************************************************************************************/


	inline D3D12_RESOURCE_DESC GetD3D12ResourceDesc(const GPUResourceDesc& desc)
	{
		const auto dxgiFormat = TextureFormat2DXGIFormat(desc.format);
		D3D12_RESOURCE_DESC out;

		switch (desc.Dimensions)
		{
		case TextureDimension::Buffer:
			out = CD3DX12_RESOURCE_DESC::Buffer(desc.WH[0]);

			break;
		case TextureDimension::Texture1D:
			out = CD3DX12_RESOURCE_DESC::Tex1D(
				dxgiFormat,
				desc.WH[0],
				desc.arraySize,
				desc.mipLevels);

			break;
		case TextureDimension::Texture2D:
			out = CD3DX12_RESOURCE_DESC::Tex2D(
				dxgiFormat,
				desc.WH[0],
				desc.WH[1],
				desc.arraySize,
				desc.mipLevels);

			break;
		case TextureDimension::Texture3D:
			out = CD3DX12_RESOURCE_DESC::Tex3D(
				dxgiFormat,
				desc.WH[0],
				desc.WH[1],
				desc.arraySize,
				desc.mipLevels);

			break;
		case TextureDimension::TextureCubeMap:
			out = CD3DX12_RESOURCE_DESC::Tex2D(
				dxgiFormat,
				desc.WH[0],
				desc.WH[1],
				6,
				desc.mipLevels);
			break;
		};

		out.Flags  = desc.type	== ResourceType::RenderTarget					? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : D3D12_RESOURCE_FLAG_NONE;
		out.Flags |= desc.type	== ResourceType::UnorderedAccess				? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
		out.Flags |= desc.type	== ResourceType::UnorderedAccessRenderTarget	? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		out.Flags |= desc.type	== ResourceType::DepthTarget					? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_NONE;

		out.MipLevels = Min(Max(desc.mipLevels, 1), 15);

		return out;
	}


	inline size_t CalculateByteSize(const GPUResourceDesc& desc)
	{
		const auto dxgiFormat = TextureFormat2DXGIFormat(desc.format);

		switch (desc.Dimensions)
		{
		case TextureDimension::Buffer:
			return desc.WH[0];
		case TextureDimension::Texture1D:
			return desc.WH[0] * GetFormatElementSize(dxgiFormat);
		case TextureDimension::Texture2D:
			return desc.WH.Product() * GetFormatElementSize(dxgiFormat);
		case TextureDimension::Texture3D:
			return desc.WH.Product() * GetFormatElementSize(dxgiFormat) * desc.arraySize;
		case TextureDimension::TextureCubeMap:
			return desc.WH.Product() * GetFormatElementSize(dxgiFormat) * 6;
		};

		return -1;
	}


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


	using GPUResourceExtra_t = std::variant<UAVResourceLayout, UAVTextureLayout>;


	class ResourceStateTable
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

		void				SetWH(ResourceHandle handle, uint2 WH);
		void				SetExtra(ResourceHandle handle, GPUResourceExtra_t);
		GPUResourceExtra_t	GetExtra(ResourceHandle handle) const;

		void				SetBufferedIdx(ResourceHandle handle, uint32_t idx);
		void				SetDebugName(ResourceHandle handle, const char* str);

		void				UpdateTileMappings(ResourceHandle handle, const TileMapping* begin, const TileMapping* end, iAllocator& temp);
		const TileMapList&	GetTileMappings(const ResourceHandle handle) const;
		TileMapList&		_GetTileMappings(const ResourceHandle handle);

		void				MarkRTUsed		(ResourceHandle Handle);

		DeviceLayout		GetLayout		(ResourceHandle Handle) const;

		ID3D12Resource*				GetResource	(ResourceHandle Handle, ID3D12Device* device) const;
		std::span<ID3D12Resource*>	GetResources(ResourceHandle Handle);

		size_t				GetResourceSize	(ResourceHandle Handle) const;
		uint2				GetHeapOffset	(ResourceHandle Handle, uint subResourceIdx) const;


		ResourceHandle		FindResourceHandle(ID3D12Resource* deviceResource) const;

		void ReplaceResources(ResourceHandle handle, ID3D12Resource** begin, size_t size);


		void ReleaseTexture	(ResourceHandle Handle, const uint64_t idx);
		void LockUntil		(size_t FrameID);

		void FreeDelayedResources(ThreadManager& thread, const uint64_t currentIdx);
		bool FreeDelayedResourcesIncrementally(const uint64_t currentIdx);
		void SubmitTileUpdates(ID3D12CommandQueue* queue, dxRenderSystem& renderSystem, iAllocator* allocator_temp);

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
			ID3D12Resource*		resource;
			uint64_t			submissionID;
		};

		Vector<UnusedResource>	delayRelease;
		iAllocator*				allocator;
	};


	/************************************************************************************************/


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
		BufferDimension		dimensions				= BufferDimension::ByteBuffer;
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


	class SOResourceTable
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


	class HeapTable
	{
	public:
		HeapTable(ID3D12Device* IN_device, iAllocator* allocator);
		~HeapTable();

		void				Release();

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


	constexpr PSOHandle CLEARBUFFERPSO = PSOHandle(GetTypeGUID(CLEARBUFFERPSO));


	class dxRenderSystem : public IRenderSystem
	{
	public:
		dxRenderSystem(iAllocator* IN_allocator, ThreadManager* IN_Threads);
		~dxRenderSystem();

		dxRenderSystem(const dxRenderSystem&) = delete;
		dxRenderSystem& operator =	(const dxRenderSystem&) = delete;

		bool Initiate(Graphics_Desc* desc_in);
		void Release();

		template<typename FN>
		void LockedResourceOperation(FN op) requires std::invocable<FN>
		{
			std::unique_lock lock{ Textures.m };

			op();
		}

		const IPipelineState*									GetPSO(PSOHandle StateID, iAllocator& temp) override;
		const IPipelineInterface* const							GetPSORootSignature(PSOHandle StateID) const override;
		std::tuple<IPipelineState*, const IPipelineInterface*>	GetPSOAndRootSignature(PSOHandle StateID, iAllocator& temp) const override;

		void BuildLibrary(PSOHandle State, const PipelineStateLibraryDesc);
		void RegisterPSOLoader(PSOHandle State, LOADSTATE_FN FN);
		virtual void LoadPSOIfRequired(PSOHandle State) final;
		virtual void QueuePSOLoad(PSOHandle State) final;

		virtual uint64_t	GetCurrentCounter();
		virtual void		SyncUploadTo(SyncPoint);
		virtual SyncPoint	SyncUploadPoint();
		virtual SyncPoint	SyncUploadTicket();


		virtual void		SyncDirectTo(SyncPoint);
		virtual SyncPoint	SyncDirectPoint();
		virtual SyncPoint	SyncSubmittedDirectPoint();
		virtual SyncPoint	SyncDirectTicket();

		virtual void		SignalDirect(uint64_t);


		virtual SyncPoint	GetSubmissionTicket(uint32_t count = 1);
		virtual SyncPoint	Submit(std::span<IDirectContext*> CLs, std::optional<SyncPoint> sync = {});
		virtual void		EndFrame();
		virtual void		Signal(SyncPoint);

		void		_UpdateSubResources(ResourceHandle handle, ID3D12Resource** resources, const size_t size);

		virtual void WaitForGPU();
		virtual void WaitFor(const uint64_t);
		virtual void WaitFor(const SyncPoint&);


		virtual void SetDebugName(ResourceHandle, const char*);
		virtual void SetDebugName(DeviceHeapHandle, const char*);


		PackedResourceTileInfo  GetPackedTileInfo(ID3D12Resource*)	const noexcept;
		PackedResourceTileInfo  GetPackedTileInfo(ResourceHandle)	const noexcept final;

		D3D12_GPU_VIRTUAL_ADDRESS	GetVertexBufferAddress(const VertexBufferHandle VB);
		D3D12_GPU_VIRTUAL_ADDRESS	GetConstantBufferAddress(const ConstantBufferHandle CB);

		virtual size_t				GetVertexBufferSize(const VertexBufferHandle) const noexcept final;
		virtual BLAS_PreBuildInfo	GetBLASPreBuildInfo(const IVertexBufferSet&) const noexcept final;

		virtual size_t			GetTextureFrameGraphIndex(ResourceHandle) noexcept final;
		virtual void			SetTextureFrameGraphIndex(ResourceHandle, size_t) noexcept final;

		virtual void			MarkTextureUsed(ResourceHandle Handle) final;

		virtual DevicePointer		GetDevicePointer(const ResourceHandle) const noexcept final;

		virtual DeviceAddressRange	GetDeviceRange(const ResourceHandle) const noexcept final;
		virtual DeviceAddressRange	GetDeviceRange(const ConstantBufferHandle) const noexcept final;

		virtual size_t	GetResourceSize(ConstantBufferHandle handle)	const noexcept final;
		virtual size_t	GetResourceSize(ResourceHandle desc)			const noexcept final;

		virtual size_t	GetAllocationSize(ResourceHandle handle) const noexcept; // Includes padding and alignment
		virtual size_t	GetAllocationSize(GPUResourceDesc desc) const noexcept; // Includes padding and alignment

		virtual size_t	GetTextureElementSize	(ResourceHandle handle) const;
		virtual uint2	GetTextureWH			(ResourceHandle handle) const;

		virtual DeviceFormat	GetTextureFormat	(ResourceHandle Handle) const;
		virtual uint8_t			GetTextureMipCount	(ResourceHandle Handle) const;
		virtual uint2			GetTextureTilingWH	(ResourceHandle Handle, const uint mipLevel) const;
		virtual uint2			GetHeapOffset		(ResourceHandle Handle, uint subResourceID = 0) const;

				DXGI_FORMAT		GetTextureDeviceFormat(ResourceHandle Handle) const;

		virtual void				SubmitTileMappings(std::span<ResourceHandle> resources, iAllocator* allocator);
		virtual void				UpdateTextureTileMappings(const ResourceHandle Handle, std::span<const TileMapping>, iAllocator& temp);
		virtual const TileMapList&	GetTileMappings(const ResourceHandle Handle);


		virtual TextureDimension	GetTextureDimension(ResourceHandle handle) const final;
		virtual	size_t				GetTextureArraySize(ResourceHandle handle) const final;

		virtual void				UploadTexture(ResourceHandle, CopyContextHandle, std::byte* buffer, size_t bufferSize) final; // Uses Upload Queue
		virtual void				UploadTexture(ResourceHandle handle, CopyContextHandle, TextureBuffer* buffer, size_t resourceCount) final; // Uses Upload Queue
		virtual void				UpdateResourceByUploadQueue(ID3D12Resource* Dest, CopyContextHandle, const void* Data, size_t Size, size_t ByteSize, DeviceAccessState EndState) final;


		virtual Shader								LoadShader			(const char* entryPoint, const char* ShaderType, const char* file, const ShaderOptions& options = {}) final;
		virtual Shader								LoadShaderLibrary	(const char* file, const ShaderOptions& options = {}) { return {}; }
		virtual std::expected<Shader, std::string>	LoadRootSignature	(const char* file, const char* entry);

		PipelineStateLibraryDesc    CreatePipelibrary();

		// Resource Creation and Destruction
		[[nodiscard]] virtual DeviceHeapHandle			CreateHeap(const size_t heapSize, const uint32_t flags);
		[[nodiscard]] virtual ConstantBufferHandle		CreateConstantBuffer(size_t BufferSize, bool GPUResident = true);
		[[nodiscard]] virtual VertexBufferHandle		CreateVertexBuffer(size_t BufferSize, bool GPUResident = true);
		[[nodiscard]] virtual ResourceHandle			CreateDepthBuffer(const uint2 WH, const bool UseFloat = false, size_t bufferCount = 3);
		[[nodiscard]] virtual ResourceHandle			CreateDepthBufferArray(const uint2 WH, const bool UseFloat = false, const size_t arraySize = 1, const bool buffered = true, const ResourceAllocationType = ResourceAllocationType::Committed);
		[[nodiscard]] virtual ResourceHandle			CreateGPUResource(const GPUResourceDesc& desc);
		[[nodiscard]] virtual ResourceHandle			CreateGPUResourceHandle();
		[[nodiscard]] virtual QueryHandle				CreateOcclusionBuffer(size_t Size);
		[[nodiscard]] virtual ResourceHandle			CreateUAVBufferResource(size_t bufferHandle, bool tripleBuffer = true);
		[[nodiscard]] virtual ResourceHandle			CreateUAVTextureResource(const uint2 WH, const DeviceFormat, const bool RenderTarget = false);
		[[nodiscard]] virtual SOResourceHandle			CreateStreamOutResource(size_t bufferHandle, bool tripleBuffer = true);
		[[nodiscard]] virtual QueryHandle				CreateSOQuery(size_t SOIndex, size_t count);
		[[nodiscard]] virtual QueryHandle				CreateTimeStampQuery(size_t count);
		[[nodiscard]]		  IndirectLayout			CreateIndirectLayout(static_vector<IndirectDrawDescription> entries, iAllocator* allocator, const IPipelineInterface* signature = nullptr);
		[[nodiscard]] virtual ReadBackResourceHandle	CreateReadBackBuffer(const size_t bufferSize);

		virtual SubAllocation		ReserveConstantBuffer(ConstantBufferHandle CB, size_t reserveSize)	noexcept final;
		virtual SubAllocation		ReserveVertexBuffer(VertexBufferHandle CB, size_t reserveSize)		noexcept final;
		virtual UploadReservation	ReserveDirectUploadSpace(size_t resourceSize, size_t alignment)		noexcept final;

		virtual const IPipelineInterface* Library(ROOTLIBRARYSIG ID) const noexcept final;

		virtual void BackResource(ResourceHandle, const GPUResourceDesc& desc) noexcept final;

		void						SetReadBackEvent(ReadBackResourceHandle readbackBuffer, ReadBackEventHandler&& handler) final;
		std::pair<void*, size_t>	OpenReadBackBuffer(ReadBackResourceHandle readbackBuffer, const size_t readSize = -1) final;
		void						CloseReadBackBuffer(ReadBackResourceHandle readbackBuffer) final;
		void						FlushPendingReadBacks() final;

		virtual void SetObjectLayout(SOResourceHandle	handle, DeviceLayout state) noexcept;
		virtual void SetObjectLayout(ResourceHandle		handle, DeviceLayout state) noexcept;

		virtual DeviceLayout	GetObjectLayout(const QueryHandle		handle) const noexcept;
		virtual DeviceLayout	GetObjectLayout(const SOResourceHandle	handle) const noexcept;
		virtual DeviceLayout	GetObjectLayout(const ResourceHandle	handle) const noexcept;
																				  

		DeviceHeap_ptr		GetDeviceResource(const DeviceHeapHandle        handle) const;
		DeviceResource_ptr	GetDeviceResource(const ReadBackResourceHandle	handle) const;
		DeviceResource_ptr	GetDeviceResource(const ConstantBufferHandle	handle) const;
		DeviceResource_ptr	GetDeviceResource(const ResourceHandle		    handle) const;
		DeviceResource_ptr	GetDeviceResource(const SOResourceHandle		handle) const;

		DeviceResource_ptr	GetSOCounterResource(const SOResourceHandle handle) const;
		size_t				GetStreamOutBufferSize(const SOResourceHandle handle) const;
		virtual size_t		GetVertexBufferOffset(const VertexBufferHandle Handle) const;

		virtual bool		VertexBufferPush(VertexBufferHandle, void* _ptr, size_t elementSize);

		virtual size_t		ConstantBufferAlign(ConstantBufferHandle) noexcept;


		UAVResourceLayout	GetUAVBufferLayout(const ResourceHandle) const noexcept;
		void				SetUAVBufferLayout(const ResourceHandle, const UAVResourceLayout) noexcept;
		size_t				GetUAVBufferSize(const ResourceHandle) const noexcept;

		size_t				GetHeapSize(const DeviceHeapHandle heap) const;

		AvailableFeatures::Raytracing	GetRTFeatureLevel() const noexcept;
		bool							RTAvailable() const noexcept;

		void ResetConstantBuffer(ConstantBufferHandle constant);
		void ResetVertexBuffer(VertexBufferHandle constant);
		void ResetQuery(QueryHandle handle);

		void ReleaseCB(ConstantBufferHandle);
		void ReleaseVB(VertexBufferHandle);
		void ReleaseResource(ResourceHandle);
		void ReleaseReadBack(ReadBackResourceHandle);
		void ReleaseHeap(DeviceHeapHandle);
		void ReleaseQuery(QueryHandle);

		void					SubmitUploadQueues(CopyContextHandle* handle, size_t count = 1, std::optional<SyncPoint> syncBefore = {}, std::optional<SyncPoint> syncAfter = {});
		CopyContextHandle		OpenUploadQueue();
		CopyContextHandle		GetImmediateCopyQueue();
		virtual IDirectContext& GetDirectCommandList(std::optional<SyncPoint> ticket = {}) final;

		// Internal
		static dxRenderSystem&	_GetInstance() { return *globalInstance; }

		static ConstantBuffer	_CreateConstantBufferResource(dxRenderSystem* RS, ConstantBuffer_desc* desc);
		VertexResourceBuffer	_CreateVertexBufferDeviceResource(const size_t ResourceSize, bool GPUResident = true);
		ResourceHandle			_CreateDefaultTexture();

		RootSignature*			_CreateRootSignature(ID3D12RootSignature* rootsig, RootSignatureBuilder& builder);
		RootSignature*			_CreateRootSignature(RootSignatureBuilder& builder, iAllocator& temp);
		RootSignature*			_GetRootSignature(uint64_t hashID) const;
		void					_ReleaseRootSignature(uint64_t hashID);

		enum class DescriptorRangeAllocationError
		{
			OutOfSpace,
			NotEnoughContiguousDescriptors
		};

		using AllocationResult = std::expected<DescriptorRange, DescriptorRangeAllocationError>;

		AllocationResult	_AllocateDescriptorRange(const size_t size);
		void				_ReleaseDescriptorRange(DescriptorRange range, uint64_t lockIdx);

		void				_PushDelayReleasedResource(ID3D12Resource*, CopyContextHandle = InvalidHandle);

		void				_ForceReleaseTexture(ResourceHandle handle);

		void				_ReleaseDelayedResources();


		struct VidMemoryStates
		{
			size_t used;
			size_t available;
		};

		VidMemoryStates _GetVidMemStats();

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

		operator dxRenderSystem* () { return this; }

		ID3D12Device1*		pDevice			= nullptr;
		ID3D12Device14*		pDevice14		= nullptr;
		ID3D12CommandQueue*	GraphicsQueue	= nullptr;
		ID3D12CommandQueue*	ComputeQueue	= nullptr;

		std::atomic_uint64_t	directSubmissionCounter		= 0;
		uint64_t				directSubmittedCounter		= 0;
		ID3D12Fence*			directFence					= nullptr;

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
			void Initiate(dxRenderSystem* RS, iAllocator& allocator, iAllocator& temp);

			const RootSignature* RS2UAVs4SRVs4CBs	= nullptr;	// 4CBVs On all Stages, 4 SRV On all Stages
			const RootSignature* RS6CBVs4SRVs		= nullptr;	// 4CBVs On all Stages, 4 SRV On all Stages
			const RootSignature* RS4CBVs_SO			= nullptr;	// Stream Out Enabled
			const RootSignature* ShadingRTSig		= nullptr;	// Signature For Compute Based Deferred Shading
			const RootSignature* RSDefault			= nullptr;	// Default Signature for Rasting
			const RootSignature* ComputeSignature	= nullptr;	//
			const RootSignature* ClearBuffer		= nullptr;
		}rootLibrary;

		Vector<dxDirectContext>				Contexts;
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

		AvailableFeatures			features;


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
		dxUploadBuffer					directUploadBuffer;

		Vector<UploadSyncPoint>			Syncs;
		Vector<FreeEntry>				FreeList_GraphicsQueue;
		Vector<FreeEntry>				FreeList_CopyQueue;


		struct RootSignatureDeleter
		{
			iAllocator* allocator;

			void operator ()(RootSignature* _ptr)
			{
				_ptr->Release();
			};
		};

		using RootSignature_ptr = std::unique_ptr<RootSignature, RootSignatureDeleter>;

		HashTable<RootSignature_ptr>	rootSignatures;

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
		std::shared_mutex		rootSignatureLock;

		inline static dxRenderSystem*	globalInstance = nullptr;
	};

	
	/************************************************************************************************/


	using ReadBackEventHandler = TypeErasedCallable<void (ReadBackResourceHandle), 64>;




	/************************************************************************************************/


	
	inline D3D12_INPUT_CLASSIFICATION ToDX(EInputClassification classifiction)
	{
		switch (classifiction)
		{
		case EInputClassification::PerVertex:
			return D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		case EInputClassification::PerInstance:
			return D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
		}

		std::unreachable();
	}


	/************************************************************************************************/


	struct PipelineBuilderImpl : IPipelineBuilderImpl
	{
		PipelineBuilderImpl(iAllocator& allocator);
		~PipelineBuilderImpl();

		PipelineBuilderImpl& AddRootSignature	(const RootSignature* rootSig);

		PipelineBuilderImpl& AddShaderLibrary	(const char* file, const ShaderOptions& options = {});
		PipelineBuilderImpl& AddComputeShader	(const char* entryPoint, const char* file, const ShaderOptions& options = {});
		PipelineBuilderImpl& AddWorkGraph		(const WorkGraph_Desc& desc = {});

		PipelineBuilderImpl& AddVertexShader	(const char* entryPoint, const char* file, const ShaderOptions& options = {});
		PipelineBuilderImpl& AddDomainShader	(const char* entryPoint, const char* file, const ShaderOptions& options = {});
		PipelineBuilderImpl& AddHullShader		(const char* entryPoint, const char* file, const ShaderOptions& options = {});
		PipelineBuilderImpl& AddGeometryShader	(const char* entryPoint, const char* file, const ShaderOptions& options = {});

		PipelineBuilderImpl& AddAmplificationShader		(const char* entryPoint, const char* file, const ShaderOptions& options = {});
		PipelineBuilderImpl& AddMeshShader				(const char* entryPoint, const char* file, const ShaderOptions& options = {});

		PipelineBuilderImpl& AddPixelShader			(const char* entryPoint, const char* file, const ShaderOptions& options = {});

		PipelineBuilderImpl& SetDebugName			(const char* name) { debugName = name; return *this; }

		PipelineBuilderImpl& AddInputLayout			(const InputLayoutState&	state = {});
		PipelineBuilderImpl& AddInputTopology		(const ETopology			topology);
		PipelineBuilderImpl& AddDepthStencilState	(const DepthStencilState&	state = {});
		PipelineBuilderImpl& AddRasterizerState		(const RasterizerState&		state = {});
		PipelineBuilderImpl& AddRenderTargetState	(const RenderTargetState&	state = {});
		PipelineBuilderImpl& AddDepthStencilFormat	(const DeviceFormat			format = DeviceFormat::D24_UNORM_S8_UINT);
		PipelineBuilderImpl& AddBlendState			(const BlendState&			state = {});

		FlexKit::LoadPipelineStateRes Build(dxRenderSystem& renderSystem);
		FlexKit::LoadPipelineStateRes BuildStream(dxRenderSystem& renderSystem, void* buffer, const size_t size);


		class PipelineBlob
		{
		public:
			PipelineBlob(iAllocator& IN_allocator) :
				buffer { IN_allocator } {}


			template<typename TY>
			PipelineBlob(const TY& IN_struct)
			{
				//static_assert(std::is_pod_v<TY>, "POD types only!");

				buffer.resize(sizeof(IN_struct));
				memcpy(data(), &IN_struct, sizeof(TY));
			}


			PipelineBlob(const char* IN_buffer, const size_t size)
			{
				buffer.resize(size);
				memcpy(data(), IN_buffer, size);
			}


			template<typename TY>
			PipelineBlob& operator += (const TY& blob)
			{
				const size_t offset = buffer.size();

				buffer.resize(buffer.size() + sizeof(blob));
				memcpy(buffer.data() + offset, &blob, sizeof(blob));

				return *this;
			}

			template<typename TY, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE TYPEID, typename TY_>
			PipelineBlob& operator += (const CD3DX12_PIPELINE_STATE_STREAM_SUBOBJECT<TY, TYPEID, TY_>& blob)
			{
				const size_t offset = buffer.size();
				const CD3DX12_PIPELINE_STATE_STREAM_SUBOBJECT<TY, TYPEID, TY_>* _ptr = std::addressof(blob);

				buffer.resize(buffer.size() + sizeof(const CD3DX12_PIPELINE_STATE_STREAM_SUBOBJECT<TY, TYPEID, TY_>));
				memcpy(buffer.data() + offset, _ptr, sizeof(const CD3DX12_PIPELINE_STATE_STREAM_SUBOBJECT<TY, TYPEID, TY_>));

				return *this;
			}

			size_t size() const
			{
				return buffer.size();
			}


			void resize(size_t newSize)
			{
				buffer.resize(newSize);
			}


			char* data()
			{
				return buffer.data();
			}


			operator const char* () const
			{
				return buffer.data();
			}


			void Clear()
			{
				buffer.clear();
			}

			void Serialize(auto& ar)
			{
				ar& buffer;
			}

			Vector<char> buffer;
		};

		const char*					debugName		= nullptr;
		const RootSignature*		rootSig			= nullptr;
		D3D12_INPUT_ELEMENT_DESC*	inputElements	= nullptr;
		bool						built			= false;
		uint64_t					hash			= 0xcbf29ce484222325;
		iAllocator*					allocator		= nullptr;

		PipelineBlob				blob;
		Vector<Shader>				shaders;
	};


	/************************************************************************************************/


	template<typename ... ARGS>
	void SetScissorAndViewports(IDirectContext& ictx, std::tuple<ARGS...>	RenderTargets)
	{
		auto& ctx = static_cast<dxDirectContext&>(ictx);

		static_vector<Viewport, 16>	VPs;
		static_vector<Rect, 16>		Rects;

		Tuple_For(
			RenderTargets,
			[&](auto target)
			{
				const auto WH = ctx.renderSystem->GetTextureWH(target);
				VPs.push_back({ 0.0f ,0.0f, (float)WH[0], (float)WH[1], 0.0f, 1.0f });
				Rects.push_back({ 0,0, WH[0], WH[1] });
			});

		ctx.SetViewports(VPs);
		ctx.SetScissorRects(Rects);
	}


	template<typename TY_RES1, typename TY_RES2>
	void CopyTexture2D(IDirectContext& ictx, TY_RES1 des, TY_RES2 src)
	{
		auto& ctx = static_cast<dxDirectContext&>(ictx);

		ctx.FlushBarriers();

		ctx.GetCommandList()->CopyResource(
			ctx.renderSystem->GetDeviceResource(des),
			ctx.renderSystem->GetDeviceResource(src));
	}


	/************************************************************************************************/


	class MemoryPoolAllocator : public PoolAllocatorInterface
	{
	public:
		MemoryPoolAllocator(dxRenderSystem&, size_t IN_heapSize, size_t IN_blockSize, uint32_t IN_flags, iAllocator* IN_allocator);
		MemoryPoolAllocator(const MemoryPoolAllocator& rhs)             = delete;
		MemoryPoolAllocator& operator =(const MemoryPoolAllocator& rhs) = delete;

		~MemoryPoolAllocator() override;

		enum NodeFlags
		{
			Clear				= 0x00,
			AllowReallocation	= 0x02,
			Locked				= 0x04,
			Temporary			= 0x08,
			Allocated			= 0x10,
		};
		
		GPUHeapAllocation   GetMemory       (const size_t requestBlockCount, const uint64_t frameID, const uint64_t flags);
		AcquireResult       Acquire         (GPUResourceDesc desc, bool temporary) final override;
		AcquireDeferredRes  AcquireDeferred (GPUResourceDesc desc, bool temporary) final override;
		AcquireResult       Recycle         (ResourceHandle resource, GPUResourceDesc desc) final override;

		uint32_t Flags() const final override;

		void Release(ResourceHandle handle, uint64_t submissionID, const bool freeResourceImmedate = true, const bool allowImmediateReuse = true) final override;
		void LockRange(uint64_t begin, uint64_t end);

		void Coalesce();

		const uint32_t      flags;
		size_t              blockCount;
		size_t              blockSize;

		DeviceHeapHandle    heap;
		dxRenderSystem&       renderSystem;

		struct MemoryRange
		{
			uint32_t        offset;
			uint32_t        blockCount;
			uint64_t        flags;
			uint64_t        frameID;
			ResourceHandle  priorAllocation = InvalidHandle;

			friend bool operator < (MemoryRange& lhs, MemoryRange& rhs)
			{
				return lhs.offset < rhs.offset;
			}
		};

		struct Allocation
		{
			uint32_t        offset;
			uint32_t        blockCount;
			size_t          frameID;
			uint64_t        flags;
			ResourceHandle  resource = InvalidHandle;

			friend bool operator < (Allocation& lhs, Allocation& rhs)
			{
				return lhs.offset < rhs.offset;
			}
		};


		size_t				estimatedBlocksAvailable = 0;
		Vector<MemoryRange> freeRanges;
		Vector<Allocation>  allocations;

		std::mutex   m;
		iAllocator*  allocator;
	};

	
	/************************************************************************************************/

	// INTERNAL USE ONLY!
	FLEXKITAPI DescHeapPOS PushRenderTarget				(dxRenderSystem* RS, ResourceHandle    target, DescHeapPOS POS, const size_t MIPOffset = 0);


	FLEXKITAPI DescHeapPOS PushDepthStencil				(dxRenderSystem* RS, ResourceHandle Target, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushDepthStencilArray		(dxRenderSystem* RS, ResourceHandle Target, size_t arrayOffset, size_t MipSlice, DescHeapPOS POS, size_t arraySize = -1);
	FLEXKITAPI DescHeapPOS PushCBToDescHeap				(dxRenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, size_t BufferSize, size_t offset = 0);
	FLEXKITAPI DescHeapPOS PushSRVToDescHeap			(dxRenderSystem* RS, ID3D12Resource* Buffer, DescHeapPOS POS, size_t ElementCount, size_t Stride, D3D12_BUFFER_SRV_FLAGS Flags = D3D12_BUFFER_SRV_FLAG_NONE, size_t offset = 0);
	FLEXKITAPI DescHeapPOS PushSRVNULLDescHeap			(dxRenderSystem* RS, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS Push2DSRVToDescHeap			(dxRenderSystem* RS, ID3D12Resource* Buffer, const DescHeapPOS POS, const D3D12_BUFFER_SRV_FLAGS = D3D12_BUFFER_SRV_FLAG_NONE, const DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);

	[[deprecated]]
	FLEXKITAPI DescHeapPOS PushTextureToDescHeap		(dxRenderSystem* RS, Texture2D tex, DescHeapPOS POS);

	FLEXKITAPI DescHeapPOS PushTextureToDescHeap		(dxRenderSystem* RS, DXGI_FORMAT format, ResourceHandle handle, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushTextureToDescHeap		(dxRenderSystem* RS, DXGI_FORMAT format, uint32_t highestMipLevel, ResourceHandle handle, DescHeapPOS POS);

	FLEXKITAPI DescHeapPOS PushTexture3DToDescHeap		(dxRenderSystem* RS, DXGI_FORMAT format, uint32_t mipCount, uint32_t highestDetailMip, uint32_t minLODClamp, ResourceHandle handle, DescHeapPOS POS);

	FLEXKITAPI DescHeapPOS PushCubeMapTextureToDescHeap	(dxRenderSystem* RS, Texture2D tex, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushCubeMapTextureToDescHeap	(dxRenderSystem* RS, ResourceHandle resource, DescHeapPOS POS, DeviceFormat format);

	FLEXKITAPI DescHeapPOS PushUAV1DToDescHeap			(dxRenderSystem* RS, ID3D12Resource* resource, DXGI_FORMAT format, uint mip, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushUAV2DToDescHeap			(dxRenderSystem* RS, Texture2D tex, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushUAV2DToDescHeap			(dxRenderSystem* RS, Texture2D tex, uint32_t mipLevel, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushUAV3DToDescHeap			(dxRenderSystem* RS, Texture2D tex, uint32_t width, DescHeapPOS POS);

	FLEXKITAPI DescHeapPOS PushUAVBufferToDescHeap		(dxRenderSystem* RS, UAVBuffer buffer, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushUAVBufferToDescHeap2		(dxRenderSystem* RS, UAVBuffer buffer, ID3D12Resource* counter, DescHeapPOS POS);
	FLEXKITAPI DescHeapPOS PushUAVCubeMapToDescHeap		(dxRenderSystem* RS, DXGI_FORMAT format, ID3D12Resource* resource, DescHeapPOS POS);



	void _UpdateSubResourceByUploadQueue(dxRenderSystem* RS, CopyContextHandle uploadHandle, ID3D12Resource* destinationResource, SubResourceUpload_Desc* desc);


	/************************************************************************************************/


	inline D3D12_SHADER_BYTECODE Shader2ByteCode(const Shader& shader)
	{
		return{ (BYTE*)shader.buffer, shader.bufferSize };
	}

	inline Shader CreateShader(ID3DBlob* blob, iAllocator* allocator = SystemAllocator)
	{
		return { (char*)blob->GetBufferPointer(), blob->GetBufferSize(), allocator };
	}

	inline Shader CreateShader(IDxcBlob* blob, iAllocator* allocator = SystemAllocator)
	{
		return { (char*)blob->GetBufferPointer(), blob->GetBufferSize(), allocator };
	}


	/************************************************************************************************/


	inline ID3D12PipelineState* LoadComputeShader(const Shader& computeShader, const RootSignature& rootSignature, dxRenderSystem& renderSystem)
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			rootSignature,
			Shader2ByteCode(computeShader)
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = renderSystem.pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

		FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

		return PSO;
	}


	/************************************************************************************************/


	struct Mesh_Description
	{
		size_t				IndexBuffer;
		size_t				BufferCount;

		VertexBufferView**	Buffers;
		iAllocator*			memory;
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


	struct TextureVTable
	{
		Texture2D					TextureMemory;
		FrameBufferedRenderTarget	RenderTarget;// Read-Back buffer

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


	FLEXKITAPI void UploadTextureSet	( dxRenderSystem* RS, TextureSet* TS, iAllocator* Memory );
	FLEXKITAPI void ReleaseTextureSet	( TextureSet* TS, iAllocator* Memory );


	/************************************************************************************************/

	

	void Release						(dxRenderSystem* System);
	void Push_DelayedRelease			(dxRenderSystem* RS, ID3D12Resource* Res);
	void Push_DelayedReleaseCopy		(dxRenderSystem* RS, ID3D12Resource* Res);
	void Free_DelayedReleaseResources	(dxRenderSystem* RS);


	void AddTempBuffer	(ID3D12Resource* _ptr, dxRenderSystem* RS);
	void AddTempShaderRes(ShaderResourceBuffer& ShaderResource, dxRenderSystem* RS);


	inline DescHeapPOS IncrementHeapPOS(const DescHeapPOS POS, const size_t size, const size_t increment) {
		const size_t offset = size * increment;
		return {
			CPUDescriptorHandle{ POS.V1	+ offset },
			GPUDescriptorHandle{ POS.V2	+ offset }  };
	}


	void Close					( static_vector<ID3D12GraphicsCommandList*> CLs );
	void ClearBackBuffer		( dxRenderSystem* RS, ID3D12GraphicsCommandList* CL, IRenderWindow* RW, float4 ClearColor );


	const float DefaultClearDepthValues_1[]	= { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, };
	const float DefaultClearDepthValues_0[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, };
	const int   DefaultClearStencilValues[]	= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	
	/************************************************************************************************/
	// Depreciated API

	void CreateVertexBuffer			( dxRenderSystem* RS, CopyContextHandle handle, VertexBufferView** Buffers, size_t BufferCount, VertexBufferSet& DVB_Out ); // Expects Index buffer in index 15


	/************************************************************************************************/


	void	Release( ConstantBuffer&	);
	void	Release( Texture2D			);
	void	Release( IRenderWindow*		);
	void	Release( VertexBuffer*		);


	/************************************************************************************************/


	FLEXKITAPI ResourceHandle	LoadDDSTextureFromFile	(char* file, dxRenderSystem* RS, CopyContextHandle, iAllocator* Memout);
	FLEXKITAPI ResourceHandle   LoadTexture				(TextureBuffer* Buffer, CopyContextHandle handle, dxRenderSystem* RS, iAllocator* Memout, DeviceFormat format = DeviceFormat::R8G8B8A8_UNORM);


	//FLEXKITAPI Shader           LoadShader_OLD                  (const char* Entry, const char* ShaderVersion, const char* File);


	/************************************************************************************************/


	FLEXKITAPI Texture2D		GetBackBufferTexture	( IRenderWindow* Window );
	FLEXKITAPI ID3D12Resource*	GetBackBufferResource	( IRenderWindow* Window );


}	/************************************************************************************************/


/**********************************************************************

Copyright (c) 2014-2025 Robert May

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
