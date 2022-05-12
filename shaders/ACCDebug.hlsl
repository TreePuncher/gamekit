
cbuffer CameraConstants : register(b0)
{
    float4x4 View;
    float4x4 ViewI;
    float4x4 Proj;
    float4x4 PV;				// Projection x View
    float4x4 PVI;
    float4   CameraPOS;
    float  	 MinZ;
    float  	 MaxZ;
    float    AspectRatio;
    float    FOV;

    float3   TLCorner_VS;
    float3   TRCorner_VS;

    float3   BLCorner_VS;
    float3   BRCorner_VS;
};

cbuffer LocalConstants : register(b1)
{
    float4x4 WT;
};

float4 VS_Main(float3 pos : POSITION) : POSITION
{
    return float4(pos, 1);
}

struct InputVert
{
    float4 pos : POSITION;
};

struct OutputVert
{
    float4 pos : SV_POSITION;
};


[maxvertexcount(6)]
void GS_Main(
    point InputVert                     IN[1],
    inout TriangleStream<OutputVert>    triangleStream)
{

    const float3 pos    = IN[0].pos * float3(3, 3, 3) + float3(-5, -1.5, -2);
    const float4 pos_WS = mul(WT, float4(pos, 1));
    const float4 pos_DC = mul(PV, pos_WS);

    float r = 0.03f;

    OutputVert v1, v2, v3, v4;
    v1.pos = pos_DC + float4(-r / AspectRatio, -r, -0.01f, 0);
    v2.pos = pos_DC + float4( r / AspectRatio, -r, -0.01f, 0);
    v3.pos = pos_DC + float4( r / AspectRatio,  r, -0.01f, 0);
    v4.pos = pos_DC + float4(-r / AspectRatio,  r, -0.01f, 0);

    triangleStream.Append(v1);
    triangleStream.Append(v2);
    triangleStream.Append(v3);
    triangleStream.RestartStrip();

    triangleStream.Append(v1);
    triangleStream.Append(v3);
    triangleStream.Append(v4);
    triangleStream.RestartStrip();
}


float4 PS_Main() : SV_TARGET
{
    return float4(0.5f, 0.5f, 0.5f, 1.0f);
}

/**********************************************************************

Copyright (c) 2015 - 2022 Robert May

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
