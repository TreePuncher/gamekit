#include "VXGI_common.hlsl"

RWStructuredBuffer<OctTreeNode>    octree  : register(u0);

[numthreads(1, 1, 1)]
void Init(uint3 threadID : SV_DispatchThreadID)
{
    OctTreeNode root;

    for(uint I = 0; I < 8; I++)
        root.nodes[I] = -1;

    root.volumeCord = uint4(0, 0, 0, 0);
    root.data       = -1;
    root.flags      = LEAF;
    root.parent     = -1;
    root.pad        = -1;

    octree[0] = root;
    octree.IncrementCounter();
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
