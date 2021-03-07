#ifndef RESOURCESUTILITIES_H
#define RESOURCESUTILITIES_H

/**********************************************************************

Copyright (c) 2015 - 2021 Robert May

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

#include "Common.h"
#include "MetaData.h"

#include "buildsettings.h"
#include "containers.h"
#include "memoryutilities.h"
#include "Assets.h"
#include "AnimationUtilities.h"

#include <PxPhysicsAPI.h>

#include <random>
#include <limits>

using FlexKit::iAllocator;
using FlexKit::BlockAllocator;
using FlexKit::StackAllocator;
using FlexKit::static_vector;

struct Scene;
struct EngineMemory;

using FlexKit::Vector;
using FlexKit::EResourceType;
using FlexKit::float2;
using FlexKit::float3;
using FlexKit::ID_LENGTH;
using FlexKit::Drawable;
using FlexKit::Quaternion;
using FlexKit::Pair;
using FlexKit::Resource;
using FlexKit::RenderSystem;
using FlexKit::TriMesh;

typedef FlexKit::Handle_t<16> ShaderSetHandle;


namespace FlexKit::ResourceBuilder
{   /************************************************************************************************/


    struct FileDir
    {	
        char str[256]; 
        bool Valid	= false;
    };


    /************************************************************************************************/


    size_t CreateRandomID();
    FileDir SelectFile();


    /************************************************************************************************/


    class Blob
    {
    public:

        Blob() = default;

        template<typename TY>
        Blob(const TY& IN_struct)
        {
            //static_assert(std::is_pod_v<TY>, "POD types only!");

            buffer.resize(sizeof(IN_struct));
            memcpy(data(), &IN_struct, sizeof(TY));
        }


        Blob(const char* IN_buffer, const size_t size)
        {
            buffer.resize(size);
            memcpy(data(), IN_buffer, size);
        }


        Blob operator + (const Blob& rhs_blob)
        {
            Blob out;

            out += *this;
            out += rhs_blob;

            return out;
        }

        Blob& operator += (const Blob& rhs_blob)
        {
            const size_t offset = buffer.size();

            buffer.resize(buffer.size() + rhs_blob.size());
            memcpy(buffer.data() + offset, rhs_blob, rhs_blob.size());

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


        std::byte* data()
        {
            return buffer.data();
        }

        operator const std::byte* () const
        {
            return buffer.data();
        }

        std::pair<std::byte*, const size_t> Release()
        {
            std::byte* _ptr = new std::byte[buffer.size()];

            memcpy(_ptr, buffer.data(), buffer.size());
            const size_t size = buffer.size();
            buffer.clear();

            return { _ptr, size };
        }

        std::vector<std::byte> buffer;
    };


    /************************************************************************************************/


    bool ExportGameRes(const std::string& file, const ResourceList& blobs);


    /************************************************************************************************/


    struct IDTranslation
    {
        size_t	FBXID;
        GUID_t	Guid;
    };


    typedef std::vector<IDTranslation> IDTranslationTable;


    inline GUID_t	TranslateID(size_t FBXID, IDTranslationTable& Table)
    {
        for (auto ID : Table)
            if (ID.FBXID == FBXID)
                return ID.Guid;

        return FBXID;
    }

    inline bool IDPresentInTable(size_t FBXID, IDTranslationTable& Table)
    {
        for (auto ID : Table)
            if (ID.FBXID == FBXID)
                return true;

        return false;
    }


}   /************************************************************************************************/


#endif // Include guard
