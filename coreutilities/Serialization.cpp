#include <zlib-ng.h>
#include "Serialization.hpp"
#include "Logging.h"

namespace FlexKit
{
    std::optional<Blob> CompressBuffer(const RawBuffer& buffer)
    {
        printf("Compressing buffer\n");

        size_t      size = zng_compressBound(buffer.size);
        uint8_t*    dest = (uint8_t*)malloc(size);

        if (auto res = zng_compress(dest, &size, (const uint8_t*)buffer._ptr, buffer.size); res != Z_OK)
            return {};

        printf("Finished Compressing buffer.\n Compressed Size: %zu\nDecompressed %zu\nRatio %f\n", size, buffer.size, (float)size / (float)buffer.size);

        return Blob{ (const char*)dest, size };
    }

    std::optional<void*> DecompressBuffer(const RawBuffer& buffer, const size_t decompressedBufferSize)
    {
        printf("Decompressing buffer\n");

        size_t      size = decompressedBufferSize;
        uint8_t*    dest = (uint8_t*)malloc(decompressedBufferSize);

        if (auto res = zng_uncompress(dest, &size, (const uint8_t*)buffer._ptr, buffer.size); res != Z_OK)
            return {};

        printf("Decompressed buffer.\n Compressed Size: %zu\nDecompressed %zu\nRatio %f\n", buffer.size, decompressedBufferSize, (float)buffer.size / (float)decompressedBufferSize);

        return dest;
    }
}
