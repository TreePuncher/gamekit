#ifndef XMMATHCONVERSION_H
#define XMMATHCONVERSION_H

#include "MathUtils.h"
#include <DirectXMath.h>

namespace FlexKit
{

    inline float4x4 XMMatrixToFloat4x4(const DirectX::XMMATRIX* const M)
    {
        float4x4 Mout;
        memcpy(&Mout, M, sizeof(Mout));
        return Mout;
    }

    inline float4x4 XMMatrixToFloat4x4(const DirectX::XMMATRIX& M)
    {
        float4x4 Mout;
        memcpy(&Mout, &M, sizeof(Mout));
        return Mout;
    }

    inline DirectX::XMMATRIX Float4x4ToXMMATIRX(const float4x4* const M)
    {
        DirectX::XMMATRIX Mout;
        memcpy(&Mout, M, sizeof(Mout));
        return Mout;
    }

    inline DirectX::XMMATRIX Float4x4ToXMMATIRX(const float4x4& M)
    {
        DirectX::XMMATRIX Mout;
        memcpy(&Mout, &M, sizeof(Mout));
        return Mout;
    }
}

#endif
