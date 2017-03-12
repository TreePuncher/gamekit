
#include "DDSUtilities.h"

#include <d3d12.h>
#include <d3dx12.h>
#include <Windows.h>

namespace FlexKit
{
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
		if (hdr->Size != sizeof(DDS_HEADER) ||
			hdr->ddspf.Size != sizeof(DDS_PIXELFORMAT))
			return E_FAIL;

		// Check for DX10 extension
		bool bDXT10Header = false;
		if ((hdr->ddspf.Flags & DDS_FOURCC) &&
			(MAKEFOURCC('D', 'X', '1', '0') == hdr->ddspf.FourCC))
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

	static HRESULT CreateD3DResources12(
		RenderSystem* RS,
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
				SETDEBUGNAME(*texture, "TEXTURE");

				const UINT num2DSubresources = texDesc.DepthOrArraySize * texDesc.MipLevels;
				const UINT64 uploadBufferSize = GetRequiredIntermediateSize(*texture, 0, num2DSubresources);

				ID3D12Resource* textureUploadHeap = nullptr;
				ID3D12GraphicsCommandList* cmdList = GetCurrentUploadQueue(RS)->UploadList[0];
				GetCurrentUploadQueue(RS)->UploadCount++;


				hr = RS->pDevice->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
					D3D12_HEAP_FLAG_NONE,
					&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&textureUploadHeap));

				AddTempBuffer(textureUploadHeap, RS);

				if (FAILED(hr))
				{
					texture = nullptr;
					return hr;
				}
				else
				{
					//cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(*texture,
					//	D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));

					// Use Heap-allocating UpdateSubresources implementation for variable number of subresources (which is the case for textures).
					UpdateSubresources(cmdList, *texture, textureUploadHeap, 0, 0, num2DSubresources, initData);

					//cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(*texture,
					//	D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
				}
			}
		} break;
		}

		return hr;
	}


	/************************************************************************************************/

	static HRESULT CreateTextureFromDDS12(
		RenderSystem* RS,
		const DDS_HEADER* header,
		const uint8_t* bitData,
		size_t bitSize,
		size_t maxsize,
		bool forceSRGB,
		ID3D12Resource** texture,
		DXGI_FORMAT*	 formatOut = nullptr)
	{
		HRESULT hr = S_OK;

		UINT width  = header->Width;
		UINT height = header->Height;
		UINT depth  = header->Depth;

		uint32_t resDim    = D3D12_RESOURCE_DIMENSION_UNKNOWN;
		UINT arraySize     = 1;
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
		bool isCubeMap     = false;

		size_t mipCount = header->MipMapCount;
		if (0 == mipCount) mipCount = 1;

		if ((header->ddspf.Flags & DDS_FOURCC) && (MAKEFOURCC('D', 'X', '1', '0') == header->ddspf.FourCC))
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
				if ((header->Flags & DDS_HEIGHT) && height != 1)
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
				if (!(header->Flags& DDS_HEADER_FLAGS_VOLUME))
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

			if (header->Flags & DDS_HEADER_FLAGS_VOLUME)
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
		std::unique_ptr<D3D12_SUBRESOURCE_DATA[]> initData(
			new (std::nothrow) D3D12_SUBRESOURCE_DATA[mipCount * arraySize]
		);

		if (!initData)
			return E_OUTOFMEMORY;

		size_t skipMip = 0;
		size_t twidth  = 0;
		size_t theight = 0;
		size_t tdepth  = 0;

		hr = FillInitData12(
			width, height, depth, mipCount, arraySize, format, maxsize, bitSize, bitData,
			twidth, theight, tdepth, skipMip, initData.get()
		);

		if (SUCCEEDED(hr))
			hr = CreateD3DResources12(
				RS,
				resDim, twidth, theight, tdepth,
				mipCount - skipMip,
				arraySize,
				format,
				false, // forceSRGB
				isCubeMap,
				initData.get(),
				texture);
		

		if(*formatOut) *formatOut = format;

		return hr;
	}

	
	/************************************************************************************************/


	bool CreateDDSTextureFromFile12(
		RenderSystem*				RS,
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
			*alphaMode = DDS_ALPHA_MODE_UNKNOWN;
		
		if (!RS || !szFileName)
			return false;

		DDS_HEADER* header	= nullptr;
		uint8_t*	bitData = nullptr;
		size_t		bitSize = 0;
		uint8_t*	ddsData = 0;
		HRESULT hr = LoadTextureDataFromFile(szFileName, &ddsData, &header, &bitData, &bitSize);

		if (FAILED(hr))
			return false;

		hr = CreateTextureFromDDS12(RS, header,
			bitData, bitSize, maxsize, false, texture, FormatOut);

		if(WH) *WH					= {header->Width, header->Height};
		if (MIPLevels) *MIPLevels	=  header->MipMapCount;
		if (SUCCEEDED(hr)) if (alphaMode) *alphaMode = GetAlphaMode(header);
		return SUCCEEDED(hr);
	}


	/************************************************************************************************/


	DDSTexture2DLoad_RES LoadDDSTexture2DFromFile(const char* File, iAllocator* Memory, RenderSystem* RS)
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

		auto res = CreateDDSTextureFromFile12(RS, wstr, &Texture, 4096, &AlphaMode, &WH, &MipLevels, &Format);

		DDSTexture2D* TextureOut = &Memory->allocate_aligned<DDSTexture2D, 0x10>();
		TextureOut->Alpha		 = AlphaMode;
		TextureOut->Texture		 = Texture;
		TextureOut->WH			 = WH;
		TextureOut->MipMapLevels = MipLevels;
		TextureOut->Format		 = Format;
		SetDebugName(TextureOut->Texture, File, strnlen(File, 256));

		return {TextureOut, res};
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