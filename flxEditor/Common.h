/**********************************************************************

Copyright (c) 2015 - 2019 Robert May

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

#ifndef COMMON_H
#define COMMON_H

#include "MathUtils.h"
#include "MemoryUtilities.h"
#include "Assets.h"
#include "XMMathConversion.h"
#include "ResourceIDs.h"
#include "Serialization.hpp"

#include <DirectXMath.h>


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

        virtual void                Restore(Archive*, void*) {}
        virtual SerializableBase*   CloneSerializer() { return nullptr; }

        virtual ResourceBlob        CreateBlob()        const = 0;
        virtual const std::string&  GetResourceID()     const noexcept { const static std::string _temp{ "resource" };  return _temp; }
        virtual const uint64_t      GetResourceGUID()   const noexcept { return -1; }
        virtual const ResourceID_t  GetResourceTypeID() const noexcept = 0;// { return -1; }

        virtual void                SetResourceID(std::string& id) {}
    };


    using Resource_ptr = std::shared_ptr<iResource>;
    using ResourceList = std::vector<Resource_ptr>;


}   /************************************************************************************************/

#endif
