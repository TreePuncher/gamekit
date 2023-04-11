#include "PCH.h"
#include "EditorTextureResources.h"

#ifdef _DEBUG
#pragma comment(lib, "CMP_Core_MDd.lib")
#else
#ifdef NDEBUG
#pragma comment(lib, "CMP_Core_MD.lib")
#endif
#endif

namespace FlexKit
{   /************************************************************************************************/


	inline FlexKit::DeviceFormat FormatStringToDeviceFormat(const std::string& format)
	{
		static std::map<std::string, FlexKit::DeviceFormat> formatMap = {
			{ "RGBA8_UNORM",	DeviceFormat::R8G8B8A8_UNORM		},
			{ "RGBA16_FLOAT",	DeviceFormat::R16G16B16A16_FLOAT	},
			{ "RGBA32_FLOAT",	DeviceFormat::R32G32B32A32_FLOAT	},
			{ "BC3",			DeviceFormat::BC3_UNORM				},
			{ "BC5",			DeviceFormat::BC5_UNORM				},
			{ "BC7",			DeviceFormat::BC7_UNORM				},

			{ "DXT3",			DeviceFormat::BC3_UNORM				},
			{ "DXT5",			DeviceFormat::BC5_UNORM				},
			{ "DXT7",			DeviceFormat::BC7_UNORM				},
		};

		return formatMap[format];
	}


	/************************************************************************************************/

	Blob CubeMapFace::CreateBlob() const
	{
		size_t currentOffset = sizeof(size_t) + sizeof(size_t) * mipLevels.size() * 2;
		const size_t mipCount = mipLevels.size();

		Blob offsets;
		Blob sizes;
		Blob Body;

		for (auto& mipLevel : mipLevels)
		{
			offsets			+= Blob{ currentOffset };
			sizes			+= Blob{ mipLevel.Size };
			currentOffset	+= mipLevel.Size;

			Blob textureBuffer;
			textureBuffer.resize(mipLevel.BufferSize());
			memcpy(textureBuffer.data(), mipLevel.Buffer, mipLevel.Size);

			Body += textureBuffer;
		}

		return Blob{ mipCount } +offsets + sizes + Body;
	}


	/************************************************************************************************/


	ResourceBlob CubeMapTexture_IMPL::CreateBlob() const
	{
		struct Header
		{
			size_t			ResourceSize;
			EResourceType	Type = EResourceType::EResource_CubeMapTexture;
			GUID_t			GUID;
			size_t			Pad;

			size_t			Width = 0;
			size_t			Height = 0;
			size_t			MipCount;

			size_t			Format;

			size_t offsets[6] = { 0 };
		} header;

		size_t currentOffset = sizeof(Header);

		Blob TextureBlob;
		for (size_t I = 0; I < 6; I++)
		{
			auto blob = Faces[I].CreateBlob();
			header.offsets[I] = currentOffset;

			currentOffset += blob.size();
			TextureBlob += blob;
		}

		header.GUID			= GUID;
		header.ResourceSize	= sizeof(Header) + TextureBlob.size();
		header.Height		= Height;
		header.Width		= Width;
		header.MipCount		= Faces[0].mipLevels.size();
		header.Format		= (size_t)FormatStringToDeviceFormat(format);

		auto [_ptr, size] = (Blob{ header } + TextureBlob).Release();

		ResourceBlob out;
		out.buffer			= (char*)_ptr;
		out.bufferSize		= size;
		out.GUID			= GUID;
		out.ID				= ID;
		out.resourceType	= EResourceType::EResource_CubeMapTexture;

		return out;
	}


	/************************************************************************************************/


	inline std::shared_ptr<iResource> CreateCubeMapResource(std::shared_ptr<TextureCubeMap_MetaData> metaData)
	{
		auto resource		= std::make_shared<CubeMapTexture>();
		resource->SetResourceGUID(metaData->Guid);
		resource->SetResourceID(metaData->ID);

		auto& impl		= resource->Object();
		impl->format	= metaData->format;

		for (auto& face : impl->Faces)
			face.mipLevels.resize(metaData->mipLevels.size());

		for (auto& mipLevel : metaData->mipLevels)
		{
			auto cubeMap = std::static_pointer_cast<TextureCubeMapMipLevel_MetaData>(mipLevel);

			for (size_t itr = 0; itr < 6; ++itr)
				impl->Faces[itr].mipLevels[cubeMap->level] = FlexKit::LoadHDR(cubeMap->TextureFiles[itr].c_str(), 1)[0];
		}

		impl->Height	= impl->Faces[0].mipLevels[0].WH[0];
		impl->Width		= impl->Faces[0].mipLevels[0].WH[1];

		return resource;
	}


	/************************************************************************************************/


	void CompressLevel(DDSTextureFormat format, uint32_t channelCount, uint2 WH, char* compressed, char* buffer)
	{
		const size_t rowOffset = WH[0] * channelCount;


		const auto threadCount = std::thread::hardware_concurrency() / 2;

		const size_t X_blockCount	= Max(WH[0] / 4, 1);
		const size_t Y_blockCount	= Max(WH[1] / 4, 1);
		const size_t rowPitch		= X_blockCount * 16;

		auto compress = [&](size_t begin_y, size_t end_y)
		{
			void* BC7Options;
			CreateOptionsBC7(&BC7Options);
			SetQualityBC7(BC7Options, 0.05f);


			switch (format)
			{
			case DDSTextureFormat::DXT7:
			{
				switch (channelCount)
				{
				case 3:
				{
					unsigned char   inputBlock[16 * 4];
					CMP_BYTE        outputBlock[16];

					memset(inputBlock, 0xff, sizeof(inputBlock));

					for (size_t I = 3; I < 64; I += 4)
						inputBlock[I] = 0xff;

					if (WH[0] < 4 && WH[1] < 4)
					{
						for (size_t I = 0; I < 64; I += 4)
							for(size_t J = 0; J < 3; J++)
								inputBlock[J + I] = buffer[J];

						const auto endX = WH[0];
						const auto endY = WH[1];

						for (size_t Y = 0; Y < endY; Y++)
							for (size_t X = 0; X < endX; X++)
								memcpy(inputBlock + 4 * X + 16 * Y, buffer + 6 * Y + 3 * X, 3);

						auto res = CompressBlockBC7(inputBlock, 16, outputBlock, BC7Options);
						memcpy(compressed, outputBlock, sizeof(outputBlock));
					}
					else
					{
						for (size_t Y = begin_y; Y < end_y; Y++)
						{
							for (size_t X = 0; X < X_blockCount; X++)
								{
									const size_t inputOffset  = (X_blockCount * Y * 4 + X) * 12;
									const size_t outputOffset = rowPitch * Y + X * 16;

									unsigned char RGB[4][12];

									for(size_t J = 0; J < 4; J++)
									{
										memcpy(RGB[J], buffer + inputOffset + rowOffset * J, 12);

										inputBlock[0 + 16 * J]  = RGB[J][0];
										inputBlock[1 + 16 * J]  = RGB[J][1];
										inputBlock[2 + 16 * J]  = RGB[J][2];

										inputBlock[4 + 16 * J]  = RGB[J][3];
										inputBlock[5 + 16 * J]  = RGB[J][4];
										inputBlock[6 + 16 * J]  = RGB[J][5];

										inputBlock[8 + 16 * J]  = RGB[J][6];
										inputBlock[9 + 16 * J]  = RGB[J][7];
										inputBlock[10 + 16 * J] = RGB[J][8];

										inputBlock[12 + 16 * J] = RGB[J][9];
										inputBlock[13 + 16 * J] = RGB[J][10];
										inputBlock[14 + 16 * J] = RGB[J][11];
									}

									auto res = CompressBlockBC7(inputBlock, 16, outputBlock, BC7Options);

									memcpy(compressed + outputOffset, outputBlock, sizeof(outputBlock));
								}
							}
					}
				}   break;
				case 4:
				{
					for (size_t Y = 0; Y < Y_blockCount; Y++)
					{
						for (size_t X = 0; X < X_blockCount; X++)
						{
							const size_t outputOffset = rowPitch * Y + X * 16;

							unsigned char   inputBlock[16][4];
							CMP_BYTE        outputBlock[16];

							size_t offset = (X_blockCount * Y * 4 + X) * 16;
							memcpy(inputBlock[0],  buffer + offset + rowOffset * 0, sizeof(uint8_t) * 4);
							memcpy(inputBlock[4],  buffer + offset + rowOffset * 1, sizeof(uint8_t) * 4);
							memcpy(inputBlock[8],  buffer + offset + rowOffset * 2, sizeof(uint8_t) * 4);
							memcpy(inputBlock[12], buffer + offset + rowOffset * 3, sizeof(uint8_t) * 4);

							auto res = CompressBlockBC7((unsigned char*)inputBlock, 16, outputBlock, BC7Options);
							memcpy(compressed + outputOffset, outputBlock, sizeof(outputBlock));
						}
					}
				}
				default:
					break;
				}
			}   break;
			}

		};


		if (Y_blockCount * X_blockCount > 1)
		{
			const size_t blockSize = (size_t)std::ceilf((float)Y_blockCount / (float)threadCount);
			std::vector<std::jthread> threads;
			threads.reserve(threadCount);

			for (size_t I = 0; I < threadCount; I++)
			{
				threads.push_back(
					std::jthread([&, BlockID = I]()
						{
							compress(
								FlexKit::Min((BlockID + 0) * blockSize, Y_blockCount),
								FlexKit::Min((BlockID + 1) * blockSize, Y_blockCount));

							fmt::print("Thread: {} Completed\n", BlockID);
						}));
			}

			for (auto& thread : threads)
				thread.join();

			threads.clear();
		}
		else
			compress(0, 1);



	}


	/************************************************************************************************/


	ResourceBlob TextureResource_IMPL::CreateBlob() const
	{
		if(!cachedBuffer || dirtyFlag)
		{
			uint32_t mipCount = std::ceilf(std::log2(FlexKit::Max(WH[0], WH[1]))) + 1;

			CMP_MipSet dstMipSet = { 0 };

			size_t outputSize = WH.Product();

			std::vector<uint32_t> offsets;
			offsets.resize(15);
			offsets[0] = 0;

			for (int I = 1; I < mipCount; I++)
			{
				static const size_t alignement  = 64 * 1024;

				const auto MIPWH			= (WH / (1 << I));
				const size_t X				= MIPWH[0] / 4;
				const size_t Y				= MIPWH[1] / 4;
				const auto rowPitch			= FlexKit::Max(X * 16, 256);
				const size_t MipLevelSize	= Max(rowPitch * Y, 16);

				offsets[I] = outputSize;
				outputSize += MipLevelSize;
			}

			char*			compressed	= (char*)malloc(outputSize);
			const auto		format		= FormatStringToFormatID(targetFormat);
			const char*		buffer		= (char*)MIPlevels[0].buffer;

			std::vector<TextureBuffer> mipLevels;
			mipLevels.push_back(FlexKit::TextureBuffer{ WH, (FlexKit::byte*)buffer, channelCount });

			fmt::print("Building Mip Levels\n");

			switch (channelCount)
			{
			case 3:
				for (size_t I = 1; I < mipCount; I++)
					mipLevels.emplace_back(FlexKit::BuildMipMap<Vect<3, uint8_t>, false>(mipLevels[I - 1], &FlexKit::SystemAllocator, AverageSampler<FlexKit::TextureBufferView<Vect<3, uint8_t>>>));
				break;
			case 4:
				for (size_t I = 1; I < mipCount; I++)
					mipLevels.emplace_back(FlexKit::BuildMipMap<Vect<4, uint8_t>, false>(mipLevels[I - 1], &FlexKit::SystemAllocator, AverageSampler<FlexKit::TextureBufferView<Vect<4, uint8_t>>>));
				break;
			}

			fmt::print("Compressing\n");

			for (size_t I = 0; I < mipCount; I++)
			{
				CompressLevel(format, channelCount, mipLevels[I].WH, compressed + offsets[I], (char*)mipLevels[I].Buffer);

				fmt::print("MipLevel:{} Completed\n", I);
			}

			fmt::print("Compression Completed\n");

			const_cast<std::vector<uint32_t>&>(this->offsets)	= offsets;
			const_cast<void*&>(cachedBuffer)					= compressed;
			const_cast<size_t&>(cachedBufferSize)				= outputSize;
			const_cast<size_t&>(exportedMIPCount)				= mipCount;
			const_cast<bool&>(dirtyFlag)						= true;
		}


		TextureResourceBlob headerData;

		headerData.format		= FormatStringToDeviceFormat(targetFormat);
		headerData.ResourceSize	= sizeof(headerData) + cachedBufferSize;
		headerData.GUID			= assetHandle;
		headerData.WH			= WH;
		headerData.mipLevels	= exportedMIPCount;

		memcpy(headerData.mipOffsets, offsets.data(), sizeof(headerData.mipOffsets));

		Blob header { headerData };
		Blob body   = { (const char*)cachedBuffer, cachedBufferSize };

		strncpy(headerData.ID, ID.c_str(), Min(sizeof(headerData.ID), ID.size()));

		auto [data, size]	= (header + body).Release();

		ResourceBlob out;
		out.buffer			= (char*)data;
		out.bufferSize		= size;
		out.GUID			= assetHandle;
		out.ID				= ID;
		out.resourceType	= EResourceType::EResource_Texture;

		std::cout << "_DEBUG: " __FUNCTION__ << " : Size : "		<< out.bufferSize << "\n";
		std::cout << "_DEBUG: " __FUNCTION__ << " : MipCount : "	<< exportedMIPCount << "\n";

		return out;
	}


	/************************************************************************************************/


	bool CompressionCallback(CMP_FLOAT fProgress, CMP_DWORD_PTR pUser1, CMP_DWORD_PTR pUser2) {
		(pUser1);
		(pUser2);

		std::printf("\rCompression progress = %2.0f  ", fProgress);

		return 0;
	}

	_TextureMipLevelResource CreateMIPMapResource(const std::string& string)
	{
		return {};
	}


	/************************************************************************************************/


	std::shared_ptr<iResource> CreateTextureResource(float* imageBuffer, size_t imageBufferSize, uint2 WH, uint8_t channelCount, const std::string& name, const std::string& formatString)
	{
		auto resource = std::make_shared<TextureResource>();
		auto& texture = resource->Object();

		texture->channelCount	= channelCount;
		texture->WH				= WH;
		texture->targetFormat	= formatString;

		texture->MIPlevels.push_back(
			TextureResource_IMPL::MIPLevel{
				.buffer			= (char*)imageBuffer,
				.bufferSize		= imageBufferSize,
			});

		resource->SetResourceID(name);
		resource->SetResourceGUID(rand());

		return resource;
	}


	/************************************************************************************************/


	inline std::shared_ptr<iResource> CreateTextureResource(std::shared_ptr<Texture_MetaData> metaData)
	{
		std::printf("building texture resource\n");

		auto format = FormatStringToFormatID(metaData->format);

		CMP_MipSet mipSet;
		memset(&mipSet, 0, sizeof(CMP_MipSet));

		if (auto res = CMP_LoadTexture(metaData->file.c_str(), &mipSet); res != CMP_OK)
		{
			fmt::print("Error {}: Loading source file!\n", res);

			if(!metaData->file.empty())
				fmt::print("Invalid directory string!\n");
			else
				fmt::print("File: {} not found!\n", metaData->file);

			return {};
		}

		CMP_INT nMinSize = CMP_CalcMinMipSize(mipSet.m_nHeight, mipSet.m_nWidth, mipSet.m_nMaxMipLevels);
		
		if (auto res = CMP_GenerateMIPLevels(&mipSet, nMinSize); res != CMP_OK)
		{
			std::printf("Error %d: Failed to create Mip Levels!\n", res);
			return {};
		}

		auto resource = std::make_shared<TextureResource_IMPL>();

		for (int I = 0, offset = 0; I < mipSet.m_nMipLevels; I++)
		{
			CMP_MipSet dstMipSet = { 0 };
			
			CMP_MipLevel* mipLevel = nullptr;
			CMP_GetMipLevel(&mipLevel, &mipSet, I, 0);

			auto buffer = malloc(mipLevel->m_dwLinearSize);
			memcpy(buffer, mipLevel->m_pbData, mipLevel->m_dwLinearSize);

			resource->MIPlevels.push_back(TextureResource_IMPL::MIPLevel{
				.buffer			= buffer,
				.bufferSize		= mipLevel->m_dwLinearSize });
		}

		resource->ID			= metaData->stringID;
		resource->WH			= { mipSet.m_nWidth, mipSet.m_nHeight };
		resource->assetHandle	= metaData->assetID;
		resource->targetFormat	= metaData->format;

		CMP_FreeMipSet(&mipSet);

		return resource;
	}


	/************************************************************************************************/


	std::shared_ptr<iResource>  CreateTextureResource(FlexKit::TextureBuffer& textureBuffer, std::string formatStr)
	{
		if (textureBuffer.Buffer == nullptr)
		{
			std::cout << "CreateTextureResource failed. Invalid Parameter!\n";
			return {};
		}

		auto resource		= std::make_unique<TextureResource>();
		const uint64_t ID	= rand();

		resource->SetResourceGUID(ID);
		resource->SetResourceID(std::format("{}", ID));

		auto& texture	= resource->Object();
		void* buffer = malloc(textureBuffer.BufferSize());
		memcpy(buffer, textureBuffer.Buffer, textureBuffer.BufferSize());

		texture->WH	= textureBuffer.WH;

		texture->MIPlevels.push_back(
			TextureResource_IMPL::MIPLevel{
				.buffer		= buffer,
				.bufferSize	= textureBuffer.BufferSize()
			});

		texture->channelCount	= 3;
		texture->targetFormat	= formatStr;

		return resource;
	}


	/************************************************************************************************/


	std::shared_ptr<iResource>  CreateCompressedTextureResource(FlexKit::TextureBuffer& textureBuffer, std::string formatStr)
	{
		FK_ASSERT(0);
		return {};
	}


}    /************************************************************************************************/


/**********************************************************************

Copyright (c) 2015 - 2020 Robert May

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
