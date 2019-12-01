#ifndef XMMATHCONVERSION_H
#define XMMATHCONVERSION_H

#include "MathUtils.h"
#include <DirectXMath.h>

namespace FlexKit
{

    inline float4x4 XMMatrixToFloat4x4(const DirectX::XMMATRIX* const M)
    {
        float4x4 Mout;
        Mout = *(float4x4*)M;
        return Mout;
    }

    inline float4x4 XMMatrixToFloat4x4(const DirectX::XMMATRIX& M)
    {
        float4x4 Mout;
        Mout = *(float4x4*)&M;
        return Mout;
    }

    inline DirectX::XMMATRIX Float4x4ToXMMATIRX(float4x4* M)
    {
        DirectX::XMMATRIX Mout;
        Mout = *(DirectX::XMMATRIX*)M;
        return Mout;
    }
}

#endif
