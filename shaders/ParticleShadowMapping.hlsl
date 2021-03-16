struct ParticleMeshInstanceDepthVS_IN
{
    float3 POS		        : POSITION;
    float4 instancePosition : INSTANCEPOS;
    float4 instanceArgs     : INSTANCEARGS; 
};

float4 ParticleMeshInstanceDepthVS(ParticleMeshInstanceDepthVS_IN input) : SV_POSITION
{
    return float4(input.POS + input.instancePosition.xyz, 0);
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
    triangle float4 vertices[3] : SV_POSITION,
    inout TriangleStream<GS_Out> renderTarget)
{
    for (uint face_ID = 0; face_ID < 6; face_ID++) // Loop over faces
    {
        for(uint triangle_ID = 0; triangle_ID < 3; triangle_ID++)
        {
            GS_Out Out;
            Out.pos                 = mul(matrices[face_ID], vertices[triangle_ID]);
            Out.arrayTargetIndex    = face_ID;

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
