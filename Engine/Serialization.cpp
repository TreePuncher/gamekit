#include <zlib-ng.h>
#include <fmt\format.h>
#include "Serialization.hpp"
#include "Logging.h"

namespace FlexKit
{   /************************************************************************************************/


    std::optional<Blob> CompressBuffer(const RawBuffer& buffer)
    {
        fmt::print("Compressing buffer\n");

        size_t      size = zng_compressBound(buffer.size);
        uint8_t*    dest = (uint8_t*)malloc(size);

        if (auto res = zng_compress(dest, &size, (const uint8_t*)buffer._ptr, buffer.size); res != Z_OK)
            return {};

        fmt::print("Finished Compressing buffer.\n Compressed Size: {}\nDecompressed {}\nRatio {}\n", size, buffer.size, (float)size / (float)buffer.size);

        return Blob{ (const char*)dest, size };
    }


    /************************************************************************************************/


    std::optional<void*> DecompressBuffer(const RawBuffer& buffer, const size_t decompressedBufferSize)
    {
        fmt::print("Decompressing buffer\n");

        size_t      size = decompressedBufferSize;
        uint8_t*    dest = (uint8_t*)malloc(decompressedBufferSize);

        if (auto res = zng_uncompress(dest, &size, (const uint8_t*)buffer._ptr, buffer.size); res != Z_OK)
            return {};

        fmt::print("Decompressed buffer.\n Compressed Size: {}\nDecompressed {}\nRatio {}\n", buffer.size, decompressedBufferSize, (float)buffer.size / (float)decompressedBufferSize);

        return dest;
    }


}   /************************************************************************************************/



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

