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
#include "MeshUtils.h"
#include "AnimationUtilities.h"
#include "ResourceIDs.h"
#include "Serialization.hpp"

#include <DirectXMath.h>


#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "Shell32.lib")

// FBX Loading
#pragma comment	(lib,	"libfbxsdk.lib")

// PhysX Cooking Deps
#ifdef _DEBUG
#pragma comment(lib,	"PhysXFoundation_64.lib"	)
#pragma comment(lib,	"PhysXCommon_64.lib"		)
#pragma comment(lib,	"PhysXCooking_64.lib"		)

#else
#pragma comment(lib,	"PhysXFoundation_64.lib"	)
#pragma comment(lib,	"PhysXCommon_64.lib"		)
#pragma comment(lib,	"PhysXCooking_64.lib"		)
#endif


namespace FlexKit
{   /************************************************************************************************/


    using FlexKit::float2;
    using FlexKit::float3;
    using FlexKit::float4;
    using FlexKit::float4x4;
    using FlexKit::uint4_16;
    using FlexKit::uint4_32;
    using FlexKit::iAllocator;

    using FlexKit::Skeleton;
    using DirectX::XMMATRIX;

    using FlexKit::MeshUtilityFunctions::IndexList;
    using FlexKit::MeshUtilityFunctions::CombinedVertexBuffer;


    /************************************************************************************************/


    inline uint32_t		FetchIndex2			(uint32_t itr, const IndexList* IL)				{ return IL->at(itr);								}
    inline float3		FetchVertexPOS		(uint32_t itr, const CombinedVertexBuffer* Buff){ return Buff->at(itr).POS;							}
    inline float3		FetchWeights		(uint32_t itr, const CombinedVertexBuffer* Buff){ return Buff->at(itr).WEIGHTS;						}
    inline float3		FetchVertexNormal	(uint32_t itr, const CombinedVertexBuffer* Buff){ return Buff->at(itr).NORMAL;						}
    inline float3		FetchVertexTangent  (uint32_t itr, const CombinedVertexBuffer* Buff){ return Buff->at(itr).TANGENT;						}
    inline float3		FetchFloat3ZERO		(uint32_t itr, const CombinedVertexBuffer* Buff){ return{ 0.0f, 0.0f, 0.0f };						}
    inline float2		FetchVertexUV		(uint32_t itr, const CombinedVertexBuffer* Buff){ auto temp = Buff->at(itr).TEXCOORD.xy(); return {temp.x, temp.y};	    }
    inline uint4_16		FetchWeightIndices	(size_t itr, const CombinedVertexBuffer* Buff)	{ return Buff->at(itr).WIndices;					}
    inline uint32_t		WriteIndex			(uint32_t in)								    { return in;										}
    inline float3		WriteVertex			(float3 in)									    { return float3(in);								}
    inline float2		WriteUV				(float2 in)									    { return in;										}
    inline uint4_16		Writeuint4			(uint4_16 in);


    /************************************************************************************************/


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

        virtual ResourceBlob        CreateBlob() = 0;
        virtual const std::string&  GetResourceID()     const { const static std::string _temp{ "resource" };  return _temp; }
        virtual const uint64_t      GetResourceGUID()   const { return -1; }
        virtual const ResourceID_t  GetResourceTypeID() const = 0;// { return -1; }
    };


    using Resource_ptr = std::shared_ptr<iResource>;
    using ResourceList = std::vector<Resource_ptr>;


}   /************************************************************************************************/

#endif
