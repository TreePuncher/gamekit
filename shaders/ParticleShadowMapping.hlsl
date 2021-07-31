struct ParticleMeshInstanceDepthVS_IN
{
    float3 POS		        : POSITION;
    float4 instancePosition : INSTANCEPOS;
    float4 instanceArgs     : INSTANCEARGS; 
};

float3 ParticleMeshInstanceDepthVS(ParticleMeshInstanceDepthVS_IN input) : POSITION
{
    return input.POS + input.instancePosition.xyz;
}

cbuffer PassConstants : register( b0 )
{
    float4x4 matrices[6];
};

struct GS_Out
{
    float4  pos                 : SV_POSITION;
    uint    arrayTargetIndex    : SV_RENDERTARGETARRAYINDEX;
};

[maxvertexcount(18)]
void ParticleMeshInstanceDepthGS(
    triangle float3 vertices[3] : POSITION,
    inout TriangleStream<GS_Out> renderTarget)
{
    for (uint face_ID = 0; face_ID < 6; face_ID++) // Loop over faces
    {
        uint temp[] = { 1, 0, 5, 3, 4, 2 }; // (R.M): WTF?
        //uint temp[] = { 0, 1, 2, 3, 4, 5 }; // (R.M): WTF?
        GS_Out Out;
        Out.arrayTargetIndex    = face_ID;

        for(uint vertex_ID = 0; vertex_ID < 3; vertex_ID++)
        {
            Out.pos = mul(matrices[temp[face_ID]], float4(vertices[vertex_ID], 1));
            renderTarget.Append(Out);
        }

        renderTarget.RestartStrip();
    }
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
