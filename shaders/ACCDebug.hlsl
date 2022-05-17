
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

struct GS_Input
{
    float3 pos : POSITION;
    float3 c : COLOR;
};


GS_Input VS_Main(float3 pos : POSITION, float3 c : COLOR)
{
    GS_Input output;
    output.pos = pos;
    output.c = c;

    return output;
}


float4 VS2_Main(float3 pos : POSITION) : SV_POSITION
{
    const float4 pos_WS = mul(WT, float4(pos, 1));
    const float4 pos_DC = mul(PV, pos_WS);

    return pos_DC;
}


struct OutputVert
{
    uint color : COLOR;
    float4 pos : SV_POSITION;
};


[maxvertexcount(6)]
void GS_Main(
    point GS_Input                      IN[1],
    inout TriangleStream<OutputVert>    triangleStream)
{
    const float4 pos    = float4(IN[0].pos, 1);
    const float4 pos_WS = mul(WT, pos);
    const float4 pos_DC = mul(PV, pos);

    float r = 0.03f;

    OutputVert v1, v2, v3, v4;

    v1.pos = pos_DC + float4(-r / AspectRatio, -r, -0.01f, 0);
    v2.pos = pos_DC + float4( r / AspectRatio, -r, -0.01f, 0);
    v3.pos = pos_DC + float4( r / AspectRatio,  r, -0.01f, 0);
    v4.pos = pos_DC + float4(-r / AspectRatio,  r, -0.01f, 0);

    v1.color = IN[0].c[1];
    v2.color = IN[0].c[1];
    v3.color = IN[0].c[1];
    v4.color = IN[0].c[1];

    triangleStream.Append(v1);
    triangleStream.Append(v2);
    triangleStream.Append(v3);
    triangleStream.RestartStrip();

    v1.color = IN[0].c[0];
    v2.color = IN[0].c[0];
    v3.color = IN[0].c[0];
    v4.color = IN[0].c[0];

    triangleStream.Append(v1);
    triangleStream.Append(v3);
    triangleStream.Append(v4);
    triangleStream.RestartStrip();
}
 

float4 PS_Main(const uint color : COLOR) : SV_TARGET
{
    float3 colors[] =
    {
        float3(1, 0, 0),                // 0:Red            
        float3(0, 1, 0),                // 1:Green          
        float3(0, 0, 1),                // 2:Blue
        float3(1, 0, 1),                // 3:Purple
        float3(1, 1, 1),                // 4:White
        float3(0.0f, 1.0f, 1.0f),       // 5: Cyan
        float3(0.0f, 0.0f, 0.0f),       // 6: Black
    };

    return float4(colors[color % 7], 1.0f);
}


float4 PS2_Main() : SV_TARGET
{
    return float4(0.0f, 0.0f, 0.0f, 1.0f);
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
