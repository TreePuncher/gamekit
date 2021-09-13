struct DispatchArgs
{
    uint X;
    uint Y;
    uint Z;
    uint W;
};

RWStructuredBuffer<uint>            counters            : register(u0);
RWStructuredBuffer<DispatchArgs>    dispatchArgsBuffer  : register(u1);

[numthreads(1, 1, 1)]
void CreateIndirectArgs(uint3 threadID : SV_DispatchThreadID)
{
    DispatchArgs args;
    args.X = ceil( counters[0] / 1024.0f );
    args.Y = 1;
    args.Z = 1;
    args.W = 0;

    dispatchArgsBuffer[0] = args;
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
