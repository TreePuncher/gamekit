#pragma once
#include "ResourceHandles.h"
#include "MathUtils.h"
#include "MemoryUtilities.h"
#include "Assets.h"
#include "XMMathConversion.h"
#include "ResourceIDs.h"
#include "Serialization.hpp"

#include <DirectXMath/DirectXMath.h>

#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "Shell32.lib")


namespace FlexKit
{   /************************************************************************************************/


	class iResource;

	struct ResourceBlob
	{
		ResourceBlob() = default;

		~ResourceBlob()
		{
			if(buffer)
				free(buffer);
		}

		// No Copy
		ResourceBlob			 (ResourceBlob& rhs)		= delete;
		ResourceBlob& operator = (const ResourceBlob& rhs)	= delete;

		// Allow Moves
		ResourceBlob(ResourceBlob&& rhs)
		{
			buffer			= rhs.buffer;
			bufferSize		= rhs.bufferSize;

			GUID			= rhs.GUID;
			resourceType	= rhs.resourceType;
			ID				= std::move(rhs.ID);

			rhs.buffer		= nullptr;
			rhs.bufferSize	= 0;
		}

		ResourceBlob& operator =(ResourceBlob&& rhs)
		{
			buffer			= rhs.buffer;
			bufferSize		= rhs.bufferSize;

			GUID			= rhs.GUID;
			resourceType	= rhs.resourceType;
			ID				= std::move(rhs.ID);

			rhs.buffer		= nullptr;
			rhs.bufferSize	= 0;

			return *this;
		}

		size_t						GUID		= INVALIDHANDLE;
		std::string					ID;
		FlexKit::EResourceType		resourceType;

		char*			buffer		= nullptr;
		size_t			bufferSize	= 0;
	};


	/************************************************************************************************/


	class iResource : public FlexKit::SerializableInterface<GetTypeGUID(iResource)>
	{
	public:

		virtual ResourceBlob		CreateBlob()		const = 0;
		virtual const std::string&	GetResourceID()		const noexcept = 0;
		virtual uint64_t			GetResourceGUID()	const noexcept { return -1; }
		virtual ResourceID_t		GetResourceTypeID()	const noexcept = 0;// { return -1; }

		virtual void				SetResourceID   (const std::string& id) noexcept = 0;
		virtual void				SetResourceGUID (uint64_t) noexcept = 0;
	};


	using Resource_ptr = std::shared_ptr<iResource>;
	using ResourceList = std::vector<Resource_ptr>;


	inline std::string projectResourceDir = R"(Objects\)";

	inline std::string&	GetProjectResourceDir()							{ return projectResourceDir; }
	inline void			SetProjectResourceDir(const std::string& dir)	{ projectResourceDir = dir; }


	/************************************************************************************************/

	template<typename T>
	concept DirtyFlagged = requires(T t)
	{
		{ t.DirtyFlag() } -> std::same_as<bool>;
		{ t.ClearDirtyFlag() };
	};

	template<typename TY, uint64_t ObjectID> requires (std::is_base_of_v<iResource, TY>)
	class FileObjectResource : public Serializable<FileObjectResource<TY, ObjectID>, iResource, ObjectID>
	{
	public:
		FileObjectResource(TY* in_resource = nullptr) :
			resource	{ in_resource } {}

		auto& Object()
		{
			if (!resource)
			{
				auto dir	= std::format("{}/{}", GetProjectResourceDir(), assetHandle);
				auto F		= fopen(dir.c_str(), "rb");

				const_cast<std::unique_ptr<TY>&>(resource) = std::make_unique<TY>();

				if (!F || ferror(F)) // Object never saved before
				{
					if(F)
						fclose(F);

					return resource; 
				}

				FlexKit::LoadFileArchiveContext ctx{ F };

				ctx& *resource.get();

				fclose(F);
			}

			return resource;
		}

		const	std::string&	GetResourceID()		const noexcept final { return ID; }
				uint64_t		GetResourceGUID()	const noexcept final { return assetHandle; }
				ResourceID_t	GetResourceTypeID()	const noexcept final { return ObjectID; }

		void SetResourceID(const std::string& IN_ID)	noexcept final
		{
			ID = IN_ID;
		}

		void SetResourceGUID(uint64_t newGUID)		noexcept final
		{
			assetHandle = newGUID;
		}

		template<DirtyFlagged TY>
		static consteval bool HasDirtyFlagFunction()
		{
			return true;
		}

		template<typename TY> requires !DirtyFlagged<TY>
		static consteval bool HasDirtyFlagFunction()
		{
			return false;
		}

		ResourceBlob CreateBlob() const override
		{
			if (!resource)
			{
				auto dir	= std::format("{}/{}", GetProjectResourceDir(), assetHandle);
				auto F		= fopen(dir.c_str(), "rb");

				if (!F || !ferror(F)) // Object never saved before
				{
					FlexKit::LoadFileArchiveContext ctx{ F };
					const_cast<std::unique_ptr<TY>&>(resource) = std::make_unique<TY>();

					ctx& *resource.get();
				}
				else if(F) fclose(F);
			}

			if (resource->GetResourceGUID() != assetHandle)
				resource->SetResourceGUID(assetHandle);

			if (resource->GetResourceID() != ID)
				resource->SetResourceID(ID);

			auto blob = resource->CreateBlob();

			return blob;
		}

		bool GetDirtyState()
		{
			if constexpr (HasDirtyFlagFunction<TY>())
			{
				if (resource)
					return resource->DirtyFlag() || dirtyFlag;
				else
					return dirtyFlag;
			}
			else return dirtyFlag;
		}

		void ClearDirtyState()
		{
			dirtyFlag = false;

			if constexpr (HasDirtyFlagFunction<TY>())
				if(resource ) resource->ClearDirtyFlag();
		}

		void Save()
		{
			if (resource && GetDirtyState())
			{
				if (resource->GetResourceGUID() != assetHandle)
					resource->SetResourceGUID(assetHandle);

				if (resource->GetResourceID() != ID)
					resource->SetResourceID(ID);

				auto dir = std::format("{}/{}", GetProjectResourceDir(), assetHandle);
				auto F = fopen(dir.c_str(), "wb");

				if (F || !ferror(F))
				{
					FlexKit::SaveArchiveContext ctx{};

					ctx&* resource.get();

					FlexKit::WriteBlob(ctx.GetBlob(), F);
				}

				if (F) fclose(F);

				ClearDirtyState();
			}
		}

		void Serialize(auto& ar)
		{
			ar& ID;
			ar& assetHandle;

			if constexpr (!ar.Loading())
				Save();
			else
				ClearDirtyState();
		}

		std::string				ID				= "";
		GUID_t					assetHandle		= -1;
		bool					dirtyFlag		= true;
		std::unique_ptr<TY>		resource;
	};


}	/************************************************************************************************/



/**********************************************************************

Copyright (c) 2022 Robert May

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
