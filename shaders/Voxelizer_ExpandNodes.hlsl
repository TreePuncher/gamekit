#include "VXGI_common.hlsl"

/************************************************************************************************/


cbuffer range : register(b0)
{
    uint sampleCount;
};

struct VoxelSample
{
    uint64_t    mortonID;
    float4      POS;    // Position + ???
    float4      ColorR; // Color + roughness
};


StructuredBuffer<uint>          dirtyNodes      : register(t0);
RWStructuredBuffer<OctTreeNode> octree          : register(u0);
RWStructuredBuffer<uint>        octreeCounter   : register(u1);
RWStructuredBuffer<uint>        parentLinkage   : register(u2);

uint GetChildrenIdx()
{
    uint idx = 0;
    InterlockedAdd(octreeCounter[0], 8, idx);

    return idx;
}

[numthreads(1024, 1, 1)]
void ExpandNodes(const uint3 threadID : SV_DispatchThreadID)
{
    const uint threadIdx = threadID.x;

    if (threadIdx >= sampleCount)
        return;

    const uint nodeIdx      = dirtyNodes[threadIdx];
    const uint childrenIdx  = GetChildrenIdx();

    OctTreeNode blankNode;
    blankNode.children  = -1;
    blankNode.flags     = LEAF;
    blankNode.RGBA      = 0xdeadbeef;
    blankNode.NORMALX   = 0xdeadbeef;

    for (uint I = 0; I < 8; I++)
        octree[childrenIdx + I] = blankNode;

    parentLinkage[(childrenIdx - 1) / 8] = nodeIdx;

    OctTreeNode node      = octree[nodeIdx];
    node.children         = childrenIdx;
    node.flags            ^= SUBDIVISION_REQUEST | LEAF | BRANCH;

    octree[nodeIdx] = node;
}


/************************************************************************************************/


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
