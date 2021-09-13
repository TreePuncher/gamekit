struct DispatchArgs
{
    uint freeListCount;
    uint nodeCount;
    uint pad0;
    uint pad1;

    uint X;
    uint Y;
    uint Z;
    uint W;
};

RWStructuredBuffer<uint>            freeList            : register(u0);
RWStructuredBuffer<uint>            octree              : register(u1);
RWStructuredBuffer<DispatchArgs>    dispatchArgsBuffer  : register(u2);

[numthreads(1, 1, 1)]
void CreateRemoveArgs(uint3 threadID : SV_DispatchThreadID)
{
    DispatchArgs args;
    args.freeListCount  = freeList[0];
    args.nodeCount      = octree[0];
    args.pad0           = -1;
    args.pad1           = -1;

    args.X              = ceil(freeList[0] / 1024.0f );
    args.Y              = 1;
    args.Z              = 1;
    args.W              = 0;

    dispatchArgsBuffer[0] = args;

    DispatchArgs args2;
    args2.freeListCount  = freeList[0];
    args2.nodeCount      = octree[0];
    args2.pad0           = -1;
    args2.pad1           = -1;

    args2.X              = 1;
    args2.Y              = 1;
    args2.Z              = 1;
    args2.W              = 0;

    dispatchArgsBuffer[1] = args2;
}


/**********************************************************************

Copyright (c) 2014-2021 Robert May

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
