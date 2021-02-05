struct Arguments
{
    uint X;
    uint Y;
    uint Z;
    uint padding;
};

RWStructuredBuffer<Arguments>   argumentBuffer   : register(u0); // in-out
RWStructuredBuffer<uint>        counters1        : register(u1); // in-out
StructuredBuffer<uint>          counters2        : register(t0);

[numthreads(1, 1, 1)]
void CreateLightListArguents()
{
    Arguments args;
    args.X          = min(counters2[0], 1024);
    args.Y          = min(ceil(float(counters2[0]) / 1024.0f), 32);
    args.Z          = 1;
    args.padding    = 0;
    argumentBuffer[0] = args;

    counters1[0] = 0;
}

/**********************************************************************

Copyright (c) 2021 Robert May

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