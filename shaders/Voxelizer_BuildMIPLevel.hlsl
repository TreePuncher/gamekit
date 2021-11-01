#include "VXGI_common.hlsl"

cbuffer range : register(b0)
{
    uint sampleCount;
};

StructuredBuffer<uint>              dirtyNodes      : register(t0);
StructuredBuffer<uint>              parents         : register(t1);

AppendStructuredBuffer<uint>        dirtyParents    : register(u0);
RWStructuredBuffer<OctTreeNode>     octree          : register(u1);

bool FlagColor(const uint node)
{
    uint result = 0;
    InterlockedOr(
        octree[node].flags,
        COLOR,
        result);

    return !(result & COLOR);
}

uint GetParentIdx(const uint nodeIdx)
{
    return parents[(nodeIdx - 1) / 8];
}

bool FlagParent(const uint parentIdx)
{
    uint result = 0;
    InterlockedOr(
        octree[parentIdx].flags,
        MIPUPDATE_REQUEST,
        result);

    return !(result & MIPUPDATE_REQUEST);
}

[numthreads(1024, 1, 1)]
void BuildLevel(const uint3 threadID : SV_DispatchThreadID)
{
    const uint threadIdx = threadID.x;

    if (threadIdx >= sampleCount)
        return;

    const uint    nodeIdx  = dirtyNodes[threadIdx];
    OctTreeNode   node     = octree[nodeIdx];

    uint a          = 0;
    float4 color    = float4(0, 0, 0, 0);

    for (uint childIdx = 0; childIdx < 8; childIdx++)
    {
        const uint childFlags = GetChildFlags(childIdx, node.flags);

        if (childFlags == CHILD_FLAGS::EMPTY)
            continue;

        a++;

        const uint RGBA     = octree[node.children + childIdx].RGBA;
        const uint extra    = octree[node.children + childIdx].extra;

        color += UnPack4(RGBA);
    }

    node.flags ^= NODE_FLAGS::MIPUPDATE_REQUEST | NODE_FLAGS::COLOR;
    node.RGBA   = Pack4(color);

    octree[nodeIdx] = node;

    const uint parentIdx = GetParentIdx(nodeIdx);

    if (FlagParent(parentIdx))
        dirtyParents.Append(parentIdx);
}



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
