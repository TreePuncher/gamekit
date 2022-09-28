#pragma once
#include <vector>

#include "EditorResource.h"
#include "MetaData.h"
#include "TextureUtilities.h"
#include "ResourceUtilities.h"

#include "Compressonator.h"
#include <filesystem>

#include "Serialization.hpp"


namespace FlexKit
{   /************************************************************************************************/


	inline FlexKit::DeviceFormat FormatStringToDeviceFormat(const std::string& format);


	/************************************************************************************************/


	struct CubeMapFace
	{
		std::vector<FlexKit::TextureBuffer> mipLevels;

		Blob CreateBlob() const;
	};


	/************************************************************************************************/


	class CubeMapTexture : public iResource
	{
	public:
		CubeMapFace					Faces[6];
		size_t						GUID;
		std::string					ID;
		FlexKit::DeviceFormat		format;

		size_t						Width   = 0;
		size_t						Height  = 0;


		ResourceBlob CreateBlob() const override;

				ResourceID_t  GetResourceTypeID()	const noexcept override { return CubeMapResourceTypeID; }
		const	std::string&  GetResourceID()		const noexcept final { return ID; }
				uint64_t      GetResourceGUID()		const noexcept final { return GUID; }

		void SetResourceGUID(uint64_t newGUID)			noexcept final { GUID = newGUID; }
		void SetResourceID(const std::string& newID)	noexcept final { ID = newID; }
	};


	inline std::shared_ptr<iResource> CreateCubeMapResource(std::shared_ptr<TextureCubeMap_MetaData> metaData);


	/************************************************************************************************/


	inline void CreateCompressedDDSTextureResource()
	{
	}


	/************************************************************************************************/


	enum class DDSTextureFormat
	{
		DXT3,
		DXT5,
		DXT7,
		UNKNOWN
	};


	/************************************************************************************************/


	inline DDSTextureFormat FormatStringToFormatID(const std::string& ID)
	{
		if (ID == "DXT3")
			return DDSTextureFormat::DXT3;
		if (ID == "DXT5")
			return DDSTextureFormat::DXT5;
		if (ID == "DXT7")
			return DDSTextureFormat::DXT7;

		return DDSTextureFormat::UNKNOWN;
	}


	/************************************************************************************************/


	struct _TextureMipLevelResource
	{
		CMP_MipSet mipSet;
		std::string file;

		void Release()
		{
			CMP_FreeMipSet(&mipSet);
		}
	};


	/************************************************************************************************/



	class TextureResource_IMPL : public Serializable<TextureResource_IMPL, iResource, GetTypeGUID(TextureResource)>
	{
	public:
		void Serialize(auto& ar)
		{
			uint32_t version = 01;
			ar& version;

			FK_ASSERT(version == 01, "Texture resource version mismatch!");

			ar& ID;
			ar& assetHandle;
			ar& targetFormat;
			ar& WH;
			ar& MIPlevels;
			ar& channelCount;

			ar& exportedMIPCount;
			ar& offsets;
			ar& RawBuffer{ cachedBuffer, cachedBufferSize };
		}

		const	std::string&	GetResourceID()		const noexcept final { return ID; }
				uint64_t		GetResourceGUID()	const noexcept final { return assetHandle; }
				ResourceID_t	GetResourceTypeID()	const noexcept final { return TextureResourceTypeID; }

		void					SetResourceID(const std::string& IN_ID)	noexcept final { ID = IN_ID; }
		void					SetResourceGUID(uint64_t newGUID)		noexcept final { assetHandle = newGUID; }

		ResourceBlob			CreateBlob() const override;

		bool DirtyFlag()		{ return dirtyFlag; }
		void ClearDirtyFlag()	{ dirtyFlag = false; }

		std::string				ID					= "";
		GUID_t					assetHandle			= -1;
		std::string				targetFormat		= "";
		uint2					WH					= { 0, 0 };
		uint32_t				channelCount		= 0;

		size_t					exportedMIPCount	= 0;
		std::vector<uint32_t>	offsets;

		bool					dirtyFlag			= false;
		void*					cachedBuffer		= nullptr;
		size_t					cachedBufferSize	= 0;
		
		struct MIPLevel
		{
			void*	buffer		= nullptr;
			size_t	bufferSize	= 0;

			void Serialize(auto& ar)
			{
				ar& RawBuffer{ buffer, bufferSize };
			}
		};

		std::vector<MIPLevel> MIPlevels;
	};


	using TextureResource = FileObjectResource<TextureResource_IMPL, GetTypeGUID(TextureResource)>;


	/************************************************************************************************/


	_TextureMipLevelResource	CreateMIPMapResource(const std::string& string);
	std::shared_ptr<iResource>	CreateTextureResource(float* imageBuffer, size_t imageBufferSize, uint2 WH, uint8_t channelCount, const std::string& name, const std::string& formatString);
	std::shared_ptr<iResource>	CreateTextureResource(std::shared_ptr<Texture_MetaData> metaData);
	std::shared_ptr<iResource>	CreateTextureResource(FlexKit::TextureBuffer& textureBuffer, std::string format);
	std::shared_ptr<iResource>	CreateCompressedTextureResource(FlexKit::TextureBuffer& textureBuffer, std::string format);


}    /************************************************************************************************/


/**********************************************************************

Copyright (c) 2015 - 2022 Robert May

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
