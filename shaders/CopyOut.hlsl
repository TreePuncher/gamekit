/**********************************************************************

Copyright (c) 2015 - 2016 Robert May

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


/************************************************************************************************/


static const float PI		 = 3.14159265359f;
static const float PIInverse = 1.0/PI;


/************************************************************************************************/


// Outputs
RWTexture2D<float4>  Out    : register(u0); // Out
Texture2D<float4>    In     : register(t0); // In
Texture2D<float4>    In_2   : register(t2); // In


/************************************************************************************************/
// OUTPUT STRUCTS

void WriteOut( float4 C, uint2 PixelCord, uint2 offset ) 
{ 
    Out[uint2(4, 1) * PixelCord + uint2(0, 0)] = 1.0f;
    Out[uint2(4, 1) * PixelCord + uint2(1, 0)] = 1.0f;
    Out[uint2(4, 1) * PixelCord + uint2(2, 0)] = 1.0f;
    Out[uint2(4, 1) * PixelCord + uint2(3, 0)] = 1.0f;
}


/************************************************************************************************/

[numthreads(32, 12, 1)]
void ConvertOut(uint3 ID : SV_DispatchThreadID, uint3 TID : SV_GroupThreadID)
{
    Out[uint2(ID.x, ID.y)] = pow(In[uint2(ID.x, ID.y)] + In_2[uint2(ID.x, ID.y)], 1 / 2.1);
}


/************************************************************************************************/