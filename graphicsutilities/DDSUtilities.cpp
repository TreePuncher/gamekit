
#include "DDSUtilities.h"

#include <d3d12.h>
#include <d3dx12.h>
#include <Windows.h>
#include <dds.h>

namespace FlexKit
{   /************************************************************************************************/
    // Lifted the following functions from DirectTex
    // TODO: move to DTex

    size_t BitsPerPixel(DXGI_FORMAT fmt)
    {
        switch( fmt )
        {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return 128;

        case DXGI_FORMAT_R32G32B32_TYPELESS:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
            return 96;

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
        case DXGI_FORMAT_Y416:
        case DXGI_FORMAT_Y210:
        case DXGI_FORMAT_Y216:
            return 64;

        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
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
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        case DXGI_FORMAT_R8G8_B8G8_UNORM:
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DXGI_FORMAT_AYUV:
        case DXGI_FORMAT_Y410:
        case DXGI_FORMAT_YUY2:
            return 32;

        case DXGI_FORMAT_P010:
        case DXGI_FORMAT_P016:
        case DXGI_FORMAT_V408:
            return 24;

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
        case DXGI_FORMAT_B5G6R5_UNORM:
        case DXGI_FORMAT_B5G5R5A1_UNORM:
        case DXGI_FORMAT_A8P8:
        case DXGI_FORMAT_B4G4R4A4_UNORM:
        case DXGI_FORMAT_P208:
        case DXGI_FORMAT_V208:
            return 16;

        case DXGI_FORMAT_NV12:
        case DXGI_FORMAT_420_OPAQUE:
        case DXGI_FORMAT_NV11:
            return 12;

        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_A8_UNORM:
        case DXGI_FORMAT_AI44:
        case DXGI_FORMAT_IA44:
        case DXGI_FORMAT_P8:
            return 8;

        case DXGI_FORMAT_R1_UNORM:
            return 1;

        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
            return 4;

        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return 8;

        default:
            return 0;
        }
    }


    /************************************************************************************************/


    DXGI_FORMAT MakeSRGB(_In_ DXGI_FORMAT format)
    {
        switch (format)
        {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

        case DXGI_FORMAT_BC1_UNORM:
            return DXGI_FORMAT_BC1_UNORM_SRGB;

        case DXGI_FORMAT_BC2_UNORM:
            return DXGI_FORMAT_BC2_UNORM_SRGB;

        case DXGI_FORMAT_BC3_UNORM:
            return DXGI_FORMAT_BC3_UNORM_SRGB;

        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

        case DXGI_FORMAT_B8G8R8X8_UNORM:
            return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;

        case DXGI_FORMAT_BC7_UNORM:
            return DXGI_FORMAT_BC7_UNORM_SRGB;

        default:
            return format;
        }
    }


    /************************************************************************************************/


    inline bool IsDepthStencil(DXGI_FORMAT fmt)
    {
        switch (fmt)
        {
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_D16_UNORM:
            return true;

        default:
            return false;
        }
    }


	/************************************************************************************************/


	static HRESULT LoadTextureDataFromFile(_In_z_ const wchar_t* fileName,
		uint8_t** ddsData,
		DDS_HEADER** header,
		uint8_t** bitData,
		size_t* bitSize
	)
	{
		if (!header || !bitData || !bitSize)
		{
			return E_POINTER;
		}

		// open the file
			HANDLE  hFile(CreateFile2(fileName,
			GENERIC_READ,
			FILE_SHARE_READ,
			OPEN_EXISTING,
			nullptr));

		if (!hFile)
			return HRESULT_FROM_WIN32(GetLastError());

		FINALLY	CloseHandle(hFile);	FINALLYOVER

		// Get the file size
		LARGE_INTEGER FileSize = { 0 };

		FILE_STANDARD_INFO fileInfo;
		if (!GetFileInformationByHandleEx(hFile, FileStandardInfo, &fileInfo, sizeof(fileInfo)))
			return HRESULT_FROM_WIN32(GetLastError());
		
		FileSize = fileInfo.EndOfFile;

		// File is too big for 32-bit allocation, so reject read
		if (FileSize.HighPart > 0)
			return E_FAIL;
		// Need at least enough data to fill the header and magic number to be a valid DDS
		if (FileSize.LowPart < (sizeof(DDS_HEADER) + sizeof(uint32_t)))
			return E_FAIL;

		// create enough space for the file data
		*ddsData = new (std::nothrow) uint8_t[FileSize.LowPart];
		if (!*ddsData)
			return E_OUTOFMEMORY;

		// read the data in
		DWORD BytesRead = 0;
		if (!ReadFile(hFile,
			*ddsData,
			FileSize.LowPart,
			&BytesRead,
			nullptr
		))
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}

		if (BytesRead < FileSize.LowPart)
			return E_FAIL;

		// DDS files always start with the same magic number ("DDS ")
		uint32_t dwMagicNumber = *(const uint32_t*)(*ddsData);
		if (dwMagicNumber != DDS_MAGIC)
			return E_FAIL;

		auto hdr = reinterpret_cast<DDS_HEADER*>(*ddsData + sizeof(uint32_t));

		// Verify header to validate DDS file
		if (hdr->size != sizeof(DDS_HEADER) ||
			hdr->ddspf.size != sizeof(DDS_PIXELFORMAT))
			return E_FAIL;

		// Check for DX10 extension
		bool bDXT10Header = false;
		if ((hdr->ddspf.flags & DDS_FOURCC) &&
			(MAKEFOURCC('D', 'X', '1', '0') == hdr->ddspf.fourCC))
		{
			// Must be long enough for both headers and magic value
			if (FileSize.LowPart < (sizeof(DDS_HEADER) + sizeof(uint32_t) + sizeof(DDS_HEADER_DXT10)))
				return E_FAIL;

			bDXT10Header = true;
		}

		// setup the pointers in the process request
		*header = hdr;
		ptrdiff_t offset = sizeof(uint32_t) + sizeof(DDS_HEADER)
			+ (bDXT10Header ? sizeof(DDS_HEADER_DXT10) : 0);
		*bitData = *ddsData+ offset;
		*bitSize = FileSize.LowPart - offset;

		
		return S_OK;
	}


    /************************************************************************************************/


    HRESULT GetSurfaceInfo(
        _In_ size_t width,
        _In_ size_t height,
        _In_ DXGI_FORMAT fmt,
        size_t* outNumBytes,
        _Out_opt_ size_t* outRowBytes,
        _Out_opt_ size_t* outNumRows)
    {
        uint64_t numBytes = 0;
        uint64_t rowBytes = 0;
        uint64_t numRows = 0;

        bool bc = false;
        bool packed = false;
        bool planar = false;
        size_t bpe = 0;
        switch (fmt)
        {
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
            bc = true;
            bpe = 8;
            break;

        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            bc = true;
            bpe = 16;
            break;

        case DXGI_FORMAT_R8G8_B8G8_UNORM:
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
        case DXGI_FORMAT_YUY2:
            packed = true;
            bpe = 4;
            break;

        case DXGI_FORMAT_Y210:
        case DXGI_FORMAT_Y216:
            packed = true;
            bpe = 8;
            break;

        case DXGI_FORMAT_NV12:
        case DXGI_FORMAT_420_OPAQUE:
        case DXGI_FORMAT_P208:
            planar = true;
            bpe = 2;
            break;

        case DXGI_FORMAT_P010:
        case DXGI_FORMAT_P016:
            planar = true;
            bpe = 4;
            break;

        default:
            break;
        }

        if (bc)
        {
            uint64_t numBlocksWide = 0;
            if (width > 0)
            {
                numBlocksWide = std::max<uint64_t>(1u, (uint64_t(width) + 3u) / 4u);
            }
            uint64_t numBlocksHigh = 0;
            if (height > 0)
            {
                numBlocksHigh = std::max<uint64_t>(1u, (uint64_t(height) + 3u) / 4u);
            }
            rowBytes = numBlocksWide * bpe;
            numRows = numBlocksHigh;
            numBytes = rowBytes * numBlocksHigh;
        }
        else if (packed)
        {
            rowBytes = ((uint64_t(width) + 1u) >> 1)* bpe;
            numRows = uint64_t(height);
            numBytes = rowBytes * height;
        }
        else if (fmt == DXGI_FORMAT_NV11)
        {
            rowBytes = ((uint64_t(width) + 3u) >> 2) * 4u;
            numRows = uint64_t(height) * 2u; // Direct3D makes this simplifying assumption, although it is larger than the 4:1:1 data
            numBytes = rowBytes * numRows;
        }
        else if (planar)
        {
            rowBytes = ((uint64_t(width) + 1u) >> 1)* bpe;
            numBytes = (rowBytes * uint64_t(height)) + ((rowBytes * uint64_t(height) + 1u) >> 1);
            numRows = height + ((uint64_t(height) + 1u) >> 1);
        }
        else
        {
            size_t bpp = BitsPerPixel(fmt);
            if (!bpp)
                return E_INVALIDARG;

            rowBytes = (uint64_t(width) * bpp + 7u) / 8u; // round up to nearest byte
            numRows = uint64_t(height);
            numBytes = rowBytes * height;
        }

#if defined(_M_IX86) || defined(_M_ARM) || defined(_M_HYBRID_X86_ARM64)
        static_assert(sizeof(size_t) == 4, "Not a 32-bit platform!");
        if (numBytes > UINT32_MAX || rowBytes > UINT32_MAX || numRows > UINT32_MAX)
            return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
#else
        static_assert(sizeof(size_t) == 8, "Not a 64-bit platform!");
#endif

        if (outNumBytes)
        {
            *outNumBytes = static_cast<size_t>(numBytes);
        }
        if (outRowBytes)
        {
            *outRowBytes = static_cast<size_t>(rowBytes);
        }
        if (outNumRows)
        {
            *outNumRows = static_cast<size_t>(numRows);
        }

        return S_OK;
    }


    /************************************************************************************************/

    #define ISBITMASK( r,g,b,a ) ( ddpf.RBitMask == r && ddpf.GBitMask == g && ddpf.BBitMask == b && ddpf.ABitMask == a )

    DXGI_FORMAT GetDXGIFormat(const DDS_PIXELFORMAT& ddpf)
    {
        if (ddpf.flags & DDS_RGB)
        {
            // Note that sRGB formats are written using the "DX10" extended header

            switch (ddpf.RGBBitCount)
            {
            case 32:
                if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
                {
                    return DXGI_FORMAT_R8G8B8A8_UNORM;
                }

                if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
                {
                    return DXGI_FORMAT_B8G8R8A8_UNORM;
                }

                if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000))
                {
                    return DXGI_FORMAT_B8G8R8X8_UNORM;
                }

                // No DXGI format maps to ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0x00000000) aka D3DFMT_X8B8G8R8

                // Note that many common DDS reader/writers (including D3DX) swap the
                // the RED/BLUE masks for 10:10:10:2 formats. We assume
                // below that the 'backwards' header mask is being used since it is most
                // likely written by D3DX. The more robust solution is to use the 'DX10'
                // header extension and specify the DXGI_FORMAT_R10G10B10A2_UNORM format directly

                // For 'correct' writers, this should be 0x000003ff,0x000ffc00,0x3ff00000 for RGB data
                if (ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000))
                {
                    return DXGI_FORMAT_R10G10B10A2_UNORM;
                }

                // No DXGI format maps to ISBITMASK(0x000003ff,0x000ffc00,0x3ff00000,0xc0000000) aka D3DFMT_A2R10G10B10

                if (ISBITMASK(0x0000ffff, 0xffff0000, 0x00000000, 0x00000000))
                {
                    return DXGI_FORMAT_R16G16_UNORM;
                }

                if (ISBITMASK(0xffffffff, 0x00000000, 0x00000000, 0x00000000))
                {
                    // Only 32-bit color channel format in D3D9 was R32F
                    return DXGI_FORMAT_R32_FLOAT; // D3DX writes this out as a FourCC of 114
                }
                break;

            case 24:
                // No 24bpp DXGI formats aka D3DFMT_R8G8B8
                break;

            case 16:
                if (ISBITMASK(0x7c00, 0x03e0, 0x001f, 0x8000))
                {
                    return DXGI_FORMAT_B5G5R5A1_UNORM;
                }
                if (ISBITMASK(0xf800, 0x07e0, 0x001f, 0x0000))
                {
                    return DXGI_FORMAT_B5G6R5_UNORM;
                }

                // No DXGI format maps to ISBITMASK(0x7c00,0x03e0,0x001f,0x0000) aka D3DFMT_X1R5G5B5

                if (ISBITMASK(0x0f00, 0x00f0, 0x000f, 0xf000))
                {
                    return DXGI_FORMAT_B4G4R4A4_UNORM;
                }

                // No DXGI format maps to ISBITMASK(0x0f00,0x00f0,0x000f,0x0000) aka D3DFMT_X4R4G4B4

                // No 3:3:2, 3:3:2:8, or paletted DXGI formats aka D3DFMT_A8R3G3B2, D3DFMT_R3G3B2, D3DFMT_P8, D3DFMT_A8P8, etc.
                break;
            }
        }
        else if (ddpf.flags & DDS_LUMINANCE)
        {
            if (8 == ddpf.RGBBitCount)
            {
                if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x00000000))
                {
                    return DXGI_FORMAT_R8_UNORM; // D3DX10/11 writes this out as DX10 extension
                }

                // No DXGI format maps to ISBITMASK(0x0f,0x00,0x00,0xf0) aka D3DFMT_A4L4

                if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x0000ff00))
                {
                    return DXGI_FORMAT_R8G8_UNORM; // Some DDS writers assume the bitcount should be 8 instead of 16
                }
            }

            if (16 == ddpf.RGBBitCount)
            {
                if (ISBITMASK(0x0000ffff, 0x00000000, 0x00000000, 0x00000000))
                {
                    return DXGI_FORMAT_R16_UNORM; // D3DX10/11 writes this out as DX10 extension
                }
                if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x0000ff00))
                {
                    return DXGI_FORMAT_R8G8_UNORM; // D3DX10/11 writes this out as DX10 extension
                }
            }
        }
        else if (ddpf.flags & DDS_ALPHA)
        {
            if (8 == ddpf.RGBBitCount)
            {
                return DXGI_FORMAT_A8_UNORM;
            }
        }
        else if (ddpf.flags & DDS_BUMPDUDV)
        {
            if (16 == ddpf.RGBBitCount)
            {
                if (ISBITMASK(0x00ff, 0xff00, 0x0000, 0x0000))
                {
                    return DXGI_FORMAT_R8G8_SNORM; // D3DX10/11 writes this out as DX10 extension
                }
            }

            if (32 == ddpf.RGBBitCount)
            {
                if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
                {
                    return DXGI_FORMAT_R8G8B8A8_SNORM; // D3DX10/11 writes this out as DX10 extension
                }
                if (ISBITMASK(0x0000ffff, 0xffff0000, 0x00000000, 0x00000000))
                {
                    return DXGI_FORMAT_R16G16_SNORM; // D3DX10/11 writes this out as DX10 extension
                }

                // No DXGI format maps to ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000) aka D3DFMT_A2W10V10U10
            }
        }
        else if (ddpf.flags & DDS_FOURCC)
        {
            if (MAKEFOURCC('D', 'X', 'T', '1') == ddpf.fourCC)
            {
                return DXGI_FORMAT_BC1_UNORM;
            }
            if (MAKEFOURCC('D', 'X', 'T', '3') == ddpf.fourCC)
            {
                return DXGI_FORMAT_BC2_UNORM;
            }
            if (MAKEFOURCC('D', 'X', 'T', '5') == ddpf.fourCC)
            {
                return DXGI_FORMAT_BC3_UNORM;
            }

            // While pre-multiplied alpha isn't directly supported by the DXGI formats,
            // they are basically the same as these BC formats so they can be mapped
            if (MAKEFOURCC('D', 'X', 'T', '2') == ddpf.fourCC)
            {
                return DXGI_FORMAT_BC2_UNORM;
            }
            if (MAKEFOURCC('D', 'X', 'T', '4') == ddpf.fourCC)
            {
                return DXGI_FORMAT_BC3_UNORM;
            }

            if (MAKEFOURCC('A', 'T', 'I', '1') == ddpf.fourCC)
            {
                return DXGI_FORMAT_BC4_UNORM;
            }
            if (MAKEFOURCC('B', 'C', '4', 'U') == ddpf.fourCC)
            {
                return DXGI_FORMAT_BC4_UNORM;
            }
            if (MAKEFOURCC('B', 'C', '4', 'S') == ddpf.fourCC)
            {
                return DXGI_FORMAT_BC4_SNORM;
            }

            if (MAKEFOURCC('A', 'T', 'I', '2') == ddpf.fourCC)
            {
                return DXGI_FORMAT_BC5_UNORM;
            }
            if (MAKEFOURCC('B', 'C', '5', 'U') == ddpf.fourCC)
            {
                return DXGI_FORMAT_BC5_UNORM;
            }
            if (MAKEFOURCC('B', 'C', '5', 'S') == ddpf.fourCC)
            {
                return DXGI_FORMAT_BC5_SNORM;
            }

            // BC6H and BC7 are written using the "DX10" extended header

            if (MAKEFOURCC('R', 'G', 'B', 'G') == ddpf.fourCC)
            {
                return DXGI_FORMAT_R8G8_B8G8_UNORM;
            }
            if (MAKEFOURCC('G', 'R', 'G', 'B') == ddpf.fourCC)
            {
                return DXGI_FORMAT_G8R8_G8B8_UNORM;
            }

            if (MAKEFOURCC('Y', 'U', 'Y', '2') == ddpf.fourCC)
            {
                return DXGI_FORMAT_YUY2;
            }

            // Check for D3DFORMAT enums being set here
            switch (ddpf.fourCC)
            {
            case 36: // D3DFMT_A16B16G16R16
                return DXGI_FORMAT_R16G16B16A16_UNORM;

            case 110: // D3DFMT_Q16W16V16U16
                return DXGI_FORMAT_R16G16B16A16_SNORM;

            case 111: // D3DFMT_R16F
                return DXGI_FORMAT_R16_FLOAT;

            case 112: // D3DFMT_G16R16F
                return DXGI_FORMAT_R16G16_FLOAT;

            case 113: // D3DFMT_A16B16G16R16F
                return DXGI_FORMAT_R16G16B16A16_FLOAT;

            case 114: // D3DFMT_R32F
                return DXGI_FORMAT_R32_FLOAT;

            case 115: // D3DFMT_G32R32F
                return DXGI_FORMAT_R32G32_FLOAT;

            case 116: // D3DFMT_A32B32G32R32F
                return DXGI_FORMAT_R32G32B32A32_FLOAT;
            }
        }

        return DXGI_FORMAT_UNKNOWN;
    }


    /************************************************************************************************/


    inline void AdjustPlaneResource(
        _In_ DXGI_FORMAT fmt,
        _In_ size_t height,
        _In_ size_t slicePlane,
        _Inout_ D3D12_SUBRESOURCE_DATA& res)
    {
        switch (fmt)
        {
        case DXGI_FORMAT_NV12:
        case DXGI_FORMAT_P010:
        case DXGI_FORMAT_P016:
            if (!slicePlane)
            {
                // Plane 0
                res.SlicePitch = res.RowPitch * static_cast<LONG>(height);
            }
            else
            {
                // Plane 1
                res.pData = reinterpret_cast<const uint8_t*>(res.pData) + uintptr_t(res.RowPitch) * height;
                res.SlicePitch = res.RowPitch * ((static_cast<LONG>(height) + 1) >> 1);
            }
            break;

        case DXGI_FORMAT_NV11:
            if (!slicePlane)
            {
                // Plane 0
                res.SlicePitch = res.RowPitch * static_cast<LONG>(height);
            }
            else
            {
                // Plane 1
                res.pData = reinterpret_cast<const uint8_t*>(res.pData) + uintptr_t(res.RowPitch) * height;
                res.RowPitch = (res.RowPitch >> 1);
                res.SlicePitch = res.RowPitch * static_cast<LONG>(height);
            }
            break;
        }
    }


    /************************************************************************************************/


    HRESULT FillInitData(_In_ size_t width,
        _In_ size_t height,
        _In_ size_t depth,
        _In_ size_t mipCount,
        _In_ size_t arraySize,
        _In_ size_t numberOfPlanes,
        _In_ DXGI_FORMAT format,
        _In_ size_t maxsize,
        _In_ size_t bitSize,
        _In_reads_bytes_(bitSize) const uint8_t* bitData,
        _Out_ size_t& twidth,
        _Out_ size_t& theight,
        _Out_ size_t& tdepth,
        _Out_ size_t& skipMip,
        std::vector<D3D12_SUBRESOURCE_DATA>& initData)
    {
        if (!bitData)
        {
            return E_POINTER;
        }

        skipMip = 0;
        twidth = 0;
        theight = 0;
        tdepth = 0;

        size_t NumBytes = 0;
        size_t RowBytes = 0;
        const uint8_t* pEndBits = bitData + bitSize;

        initData.clear();

        for (size_t p = 0; p < numberOfPlanes; ++p)
        {
            const uint8_t* pSrcBits = bitData;

            for (size_t j = 0; j < arraySize; j++)
            {
                size_t w = width;
                size_t h = height;
                size_t d = depth;
                for (size_t i = 0; i < mipCount; i++)
                {
                    HRESULT hr = GetSurfaceInfo(w, h, format, &NumBytes, &RowBytes, nullptr);
                    if (FAILED(hr))
                        return hr;

                    if (NumBytes > UINT32_MAX || RowBytes > UINT32_MAX)
                        return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

                    if ((mipCount <= 1) || !maxsize || (w <= maxsize && h <= maxsize && d <= maxsize))
                    {
                        if (!twidth)
                        {
                            twidth = w;
                            theight = h;
                            tdepth = d;
                        }

                        D3D12_SUBRESOURCE_DATA res =
                        {
                            reinterpret_cast<const void*>(pSrcBits),
                            static_cast<LONG_PTR>(RowBytes),
                            static_cast<LONG_PTR>(NumBytes)
                        };

                        AdjustPlaneResource(format, h, p, res);

                        initData.emplace_back(res);
                    }
                    else if (!j)
                    {
                        // Count number of skipped mipmaps (first item only)
                        ++skipMip;
                    }

                    if (pSrcBits + (NumBytes*d) > pEndBits)
                    {
                        return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
                    }

                    pSrcBits += NumBytes * d;

                    w = w >> 1;
                    h = h >> 1;
                    d = d >> 1;
                    if (w == 0)
                    {
                        w = 1;
                    }
                    if (h == 0)
                    {
                        h = 1;
                    }
                    if (d == 0)
                    {
                        d = 1;
                    }
                }
            }
        }

        return initData.empty() ? E_FAIL : S_OK;
    }


	/************************************************************************************************/


	static HRESULT CreateD3DResources12(
		RenderSystem* RS,
        UploadQueueHandle handle,
		uint32_t resDim,
		size_t width,
		size_t height,
		size_t depth,
		size_t mipCount,
		size_t arraySize,
		DXGI_FORMAT format,
		bool forceSRGB,
		bool isCubeMap,
		D3D12_SUBRESOURCE_DATA* initData,
		ID3D12Resource** texture
	)
	{
		FK_ASSERT(RS, "INVALID ARGUEMENT");

		if (forceSRGB)
			format = MakeSRGB(format);

		HRESULT hr = E_FAIL;
		switch (resDim)
		{
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		{
			D3D12_RESOURCE_DESC texDesc;
			ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
			texDesc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			texDesc.Alignment          = 0;
			texDesc.Width              = width;
			texDesc.Height             = (uint32_t)height;
			texDesc.DepthOrArraySize   = (depth > 1) ? (uint16_t)depth : (uint16_t)arraySize;
			texDesc.MipLevels          = (uint16_t)mipCount;
			texDesc.Format             = format;
			texDesc.SampleDesc.Count   = 1;
			texDesc.SampleDesc.Quality = 0;
			texDesc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			texDesc.Flags              = D3D12_RESOURCE_FLAG_NONE;

			hr = RS->pDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				IID_PPV_ARGS(texture)
			);

			if (FAILED(hr))
			{
				texture = nullptr;
				return hr;
			}
			else
			{

				const UINT		num2DSubresources	= texDesc.DepthOrArraySize * texDesc.MipLevels;
				const UINT64	uploadBufferSize	= GetRequiredIntermediateSize(*texture, 0, num2DSubresources);

				ID3D12Resource* textureUploadHeap	= nullptr;
				ID3D12GraphicsCommandList* cmdList	= RS->_GetUploadCommandList(handle);


				hr = RS->pDevice->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
					D3D12_HEAP_FLAG_NONE,
					&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&textureUploadHeap));

				Push_DelayedRelease(RS, textureUploadHeap);

				SETDEBUGNAME(*texture,          "TEXTURE");
				SETDEBUGNAME(textureUploadHeap, "textureUploadHeap");

				if (FAILED(hr))
				{
					texture = nullptr;
					return hr;
				}
				else
				{
					// Use Heap-allocating UpdateSubresources implementation for variable number of subresources (which is the case for textures).
					UpdateSubresources(cmdList, *texture, textureUploadHeap, 0, 0, num2DSubresources, initData);
				}
			}
		} break;
		}

		return hr;
	}


    /************************************************************************************************/


    DDS_ALPHA_MODE GetAlphaMode(_In_ const DDS_HEADER* header)
    {
        if (header->ddspf.flags & DDS_FOURCC)
        {
            if (MAKEFOURCC('D', 'X', '1', '0') == header->ddspf.fourCC)
            {
                auto d3d10ext = reinterpret_cast<const DDS_HEADER_DXT10*>(reinterpret_cast<const uint8_t*>(header) + sizeof(DDS_HEADER));
                auto mode = static_cast<DDS_ALPHA_MODE>(d3d10ext->miscFlags2 & DirectX::DDS_MISC_FLAGS2_ALPHA_MODE_MASK);
                switch (mode)
                {
                case DirectX::DDS_ALPHA_MODE_STRAIGHT:
                case DirectX::DDS_ALPHA_MODE_PREMULTIPLIED:
                case DirectX::DDS_ALPHA_MODE_OPAQUE:
                case DirectX::DDS_ALPHA_MODE_CUSTOM:
                    return mode;

                case DirectX::DDS_ALPHA_MODE_UNKNOWN:
                default:
                    break;
                }
            }
            else if ((MAKEFOURCC('D', 'X', 'T', '2') == header->ddspf.fourCC)
                || (MAKEFOURCC('D', 'X', 'T', '4') == header->ddspf.fourCC))
            {
                return DirectX::DDS_ALPHA_MODE_PREMULTIPLIED;
            }
        }

        return DirectX::DDS_ALPHA_MODE_UNKNOWN;
    }


	/************************************************************************************************/


	static HRESULT CreateTextureFromDDS12(
		RenderSystem* RS,
        UploadQueueHandle handle,
		const DDS_HEADER* header,
		const uint8_t* bitData,
		size_t bitSize,
		size_t maxsize,
		bool forceSRGB,
		ID3D12Resource** texture,
		DXGI_FORMAT*	 formatOut = nullptr)
	{
		HRESULT hr = S_OK;

		UINT width  = header->width;
		UINT height = header->height;
		UINT depth  = header->depth;

		uint32_t resDim    = D3D12_RESOURCE_DIMENSION_UNKNOWN;
		UINT arraySize     = 1;
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
		bool isCubeMap     = false;

		size_t mipCount = header->mipMapCount;
		if (0 == mipCount) mipCount = 1;

		if ((header->ddspf.flags & DDS_FOURCC) && (MAKEFOURCC('D', 'X', '1', '0') == header->ddspf.fourCC))
		{
			auto d3d10ext = reinterpret_cast<const DDS_HEADER_DXT10*>((const char*)header + sizeof(DDS_HEADER));

			arraySize = d3d10ext->arraySize;
			if (arraySize == 0)
				return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

			switch (d3d10ext->dxgiFormat)
			{
			case DXGI_FORMAT_AI44:
			case DXGI_FORMAT_IA44:
			case DXGI_FORMAT_P8:
			case DXGI_FORMAT_A8P8:
				return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

			default:
				if (BitsPerPixel(d3d10ext->dxgiFormat) == 0)
					return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
			}

			format = d3d10ext->dxgiFormat;

			switch (d3d10ext->resourceDimension)
			{
			case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
				if ((header->flags & DDS_HEIGHT) && height != 1)
					return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
				height = depth = 1;
				break;
			case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
				if (d3d10ext->miscFlag & D3D11_RESOURCE_MISC_TEXTURECUBE)
				{
					arraySize *= 6;
					isCubeMap = true;
				}
				depth = 1;
				break;

			case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
				if (!(header->flags& DDS_HEADER_FLAGS_VOLUME))
					return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
				if (arraySize > 1)
					return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
				break;

			default:
				return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
			}

			switch (d3d10ext->resourceDimension)
			{
			case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
				resDim = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
				break;
			case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
				resDim = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
				break;
			case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
				resDim = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
				break;
			}
		}
		else
		{
			format = GetDXGIFormat(header->ddspf);

			if (format == DXGI_FORMAT_UNKNOWN)
				return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

			if (header->flags & DDS_HEADER_FLAGS_VOLUME)
			{
				resDim = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			}
			else
			{
				if (header->caps2 & DDS_CUBEMAP)
				{
					if ((header->caps2 & DDS_CUBEMAP_ALLFACES) != DDS_CUBEMAP_ALLFACES)
						return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
					arraySize = 6;
					isCubeMap = true;
				}

				depth = 1;
				resDim = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			}

			assert(BitsPerPixel(format) != 0);
		}

		// Bound sizes (for security purposes we don't trust DDS file metadata larger than the D3D 11.x hardware requirements)
		if (mipCount > D3D12_REQ_MIP_LEVELS)
		{
			return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
		}

		switch (resDim)
		{
		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
			if ((arraySize > D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION) ||
				(width > D3D12_REQ_TEXTURE1D_U_DIMENSION))
			{
				return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
			}
			break;

		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			if (isCubeMap)
			{
				// This is the right bound because we set arraySize to (NumCubes*6) above
				if ((arraySize > D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) ||
					(width > D3D12_REQ_TEXTURECUBE_DIMENSION) ||
					(height > D3D12_REQ_TEXTURECUBE_DIMENSION))
				{
					return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
				}
			}
			else if ((arraySize > D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) ||
				(width > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION) ||
				(height > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION))
			{
				return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
			}
			break;

		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			if ((arraySize > 1) ||
				(width > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION) ||
				(height > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION) ||
				(depth > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION))
			{
				return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
			}
			break;

		default:
			return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
		}

		// Create the texture
        std::vector<D3D12_SUBRESOURCE_DATA> initData{ mipCount * arraySize };

		size_t skipMip = 0;
		size_t twidth  = 0;
		size_t theight = 0;
		size_t tdepth  = 0;

		hr = FillInitData(
			width, height, depth, mipCount, arraySize, 1, format, maxsize, bitSize, bitData,
			twidth, theight, tdepth, skipMip, initData);

		if (SUCCEEDED(hr))
			hr = CreateD3DResources12(
				RS,
                handle,
				resDim, twidth, theight, tdepth,
				mipCount - skipMip,
				arraySize,
				format,
				false, // forceSRGB
				isCubeMap,
				initData.data(),
				texture);
		

		if(*formatOut) *formatOut = format;

		return hr;
	}

	
	/************************************************************************************************/


	bool CreateDDSTextureFromFile12(
		RenderSystem*				RS,
        UploadQueueHandle           handle,
		const wchar_t*				szFileName,
		ID3D12Resource**			texture,
		size_t						maxsize,
		DDS_ALPHA_MODE*				alphaMode, 
		uint2*						WH,
		uint64_t*					MIPLevels,
		DXGI_FORMAT*				FormatOut)
	{
		if (*texture)
			texture = nullptr;
		
		if (alphaMode)
			*alphaMode = DirectX::DDS_ALPHA_MODE_UNKNOWN;
		
		if (!RS || !szFileName)
			return false;

		DDS_HEADER* header	= nullptr;
		uint8_t*	bitData = nullptr;
		size_t		bitSize = 0;
		uint8_t*	ddsData = 0;
		HRESULT hr = LoadTextureDataFromFile(szFileName, &ddsData, &header, &bitData, &bitSize);

		if (FAILED(hr))
			return false;

		hr = CreateTextureFromDDS12(
            RS,
            handle,
            header,
			bitData,
            bitSize,
            maxsize,
            false,
            texture,
            FormatOut);

		if(WH) *WH					= {header->width, header->height};
		if (MIPLevels) *MIPLevels	=  header->mipMapCount;
		if (SUCCEEDED(hr)) if (alphaMode) *alphaMode = GetAlphaMode(header);
		return SUCCEEDED(hr);
	}


	/************************************************************************************************/


	DDSTexture2DLoad_RES LoadDDSTexture2DFromFile(
        const char*         File,
        iAllocator*         Memory,
        RenderSystem*       RS,
        UploadQueueHandle   handle)
	{
		size_t NewSize = 0;
		wchar_t	wstr[256];
		mbstowcs_s(&NewSize, wstr, File, 256);

		ID3D12Resource* Texture = nullptr;
		ID3D12Resource* UploadHeap = nullptr;
		DDS_ALPHA_MODE	AlphaMode;
		uint2			WH;
		uint64_t		MipLevels;
		DXGI_FORMAT		Format;

		auto res = CreateDDSTextureFromFile12(RS, handle, wstr, &Texture, 4096, &AlphaMode, &WH, &MipLevels, &Format);

		DDSTexture2D* TextureOut = &Memory->allocate_aligned<DDSTexture2D, 0x10>();
		TextureOut->Alpha		 = AlphaMode;
		TextureOut->Texture		 = Texture;
		TextureOut->WH			 = WH;
		TextureOut->MipMapLevels = MipLevels;
		TextureOut->Format		 = Format;
		SetDebugName(TextureOut->Texture, File, strnlen(File, 256));

		return {TextureOut, res};
	}


	LoadDDSTexture2DFromFile_RES LoadDDSTexture2DFromFile_2(
        const char*         File,
        iAllocator*         Memory,
        RenderSystem*       RS,
        UploadQueueHandle   handle)
	{
		size_t NewSize = 0;
		wchar_t	wstr[256];
		mbstowcs_s(&NewSize, wstr, File, 256);

		ID3D12Resource* TextureResources = nullptr;
		ID3D12Resource* UploadHeap = nullptr;
		DDS_ALPHA_MODE	AlphaMode;
		uint2			WH;
		uint64_t		MipLevels;
		DXGI_FORMAT		Format;

		auto res = CreateDDSTextureFromFile12(RS, handle, wstr, &TextureResources, 4096, &AlphaMode, &WH, &MipLevels, &Format);

        ResourceHandle Texture = RS->CreateGPUResource(
            GPUResourceDesc::BuildFromMemory(
                GPUResourceDesc::DDS(WH, DXGIFormat2TextureFormat(Format), MipLevels, TextureDimension::Texture2D),
                &TextureResources, 1));

		FK_ASSERT((size_t)Texture != INVALIDHANDLE, "TEXTURE FAILED TO CREATE!");

		return {Texture, res && (size_t)Texture != INVALIDHANDLE};
	}


	/************************************************************************************************/


	DDSTexture3DLoad_RES LoadDDSTexture3DFromFile()
	{
		return{ nullptr, false };
	}


	/************************************************************************************************/


	Texture2D LoadDDSIntoResource()
	{
		return {};
	}


	/************************************************************************************************/
}
