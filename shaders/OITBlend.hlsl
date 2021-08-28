Texture2D<float4>   Accum   : register(t0);
Texture2D<float>    Counter : register(t1);

float4 VMain(const uint vertexID : SV_VertexID) : SV_POSITION
{
    float4 verts[] = {
        float4(-1,  1, 1, 1),
        float4( 1, -1, 1, 1),
        float4(-1, -1, 0, 1),

        float4(-1,  1, 1, 1),
        float4( 1,  1, 1, 1),
        float4( 1, -1, 1, 1),
    };

    return verts[vertexID];
}

float4 BlendMain(float4 pixelCord : SV_POSITION) : SV_TARGET
{
    const float4    accum   = Accum.Load(int3(pixelCord.xy, 0));
    const float     n       = Counter.Load(int3(pixelCord.xy, 0));

    if (n == 0.0f)
        discard;

    const float3 W_c = accum.xyz / max(accum.a, 0.00001);  // Color
    const float  W_a = pow(max(1 - accum.w / n, 0.0), n); // weighted alpha

    return float4(W_c, W_a);
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
