/**********************************************************************

Copyright (c) 2015-2016 Robert May

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

struct TextEntry
{
	float2 POS			 : POS;
	float2 Size			 : WH;
	float2 TopLeftUV	 : UVTL;
	float2 BottomRightUV : UVBL;
	float4 Color		 : COLOR;
};

struct TextGeometry
{
	float2 POS			 : TEXCOORD0;
	float2 Size			 : TEXCOORD1;
	float2 TopLeftUV	 : TEXCOORD2;
	float2 BottomRightUV : TEXCOORD3;
	float4 Color		 : TEXCOORD4;
};

struct TextVertex
{
	float4 POS	  : SV_POSITION;
	float4 Color  : COLOR;
	float2 UV	  : TEXCOORD;
};

/************************************************************************************************/

TextGeometry VTextMain(TextEntry In)
{
	TextGeometry Out;
	Out.POS				= In.POS;
	Out.Size			= In.Size;
	Out.TopLeftUV		= In.TopLeftUV;
	Out.BottomRightUV	= In.BottomRightUV;
	Out.Color			= In.Color;

	return Out;
}

/************************************************************************************************/

[maxvertexcount(6)]
void GTextMain( point TextGeometry gin[1], inout TriangleStream<TextVertex> gout )
{
	float2 TopLEFT			= float2(gin[0].POS);
	float2 TopRight			= float2(gin[0].POS) + float2(gin[0].Size.x,	0);
	float2 BottomLEFT		= float2(gin[0].POS) + float2(0,				-gin[0].Size.y);
	float2 BottomRight		= float2(gin[0].POS) + float2(gin[0].Size.x,	-gin[0].Size.y);

	float2 TopLEFTUV		= float2(gin[0].TopLeftUV);
	float2 TopRightUV		= float2(gin[0].BottomRightUV.x, gin[0].TopLeftUV.y);
	float2 BottomLEFTUV 	= float2(gin[0].TopLeftUV.x, gin[0].BottomRightUV.y);
	float2 BottomRightUV	= float2(gin[0].BottomRightUV);

	TextVertex	v;
	v.Color = gin[0].Color;

	v.POS	= float4(TopRight, 1.0f, 1);
	v.UV	= TopRightUV;
	gout.Append(v);

	v.POS	= float4(BottomLEFT, 1.0f, 1);
	v.UV	= BottomLEFTUV;
	gout.Append(v);

	v.POS	= float4(TopLEFT, 1.0f, 1);
	v.UV	= TopLEFTUV;
	gout.Append(v);
	gout.RestartStrip();

	v.POS	= float4(BottomLEFT, 1.0f, 1);
	v.UV	= BottomLEFTUV;
	gout.Append(v);

	v.POS	= float4(TopRight, 1.0f, 1);
	v.UV	= TopRightUV;
	gout.Append(v);

	v.POS	= float4(BottomRight, 1.0f, 1);
	v.UV	= BottomRightUV;
	gout.Append(v);

	gout.RestartStrip();
}

/************************************************************************************************/

SamplerState MeshTextureSampler
{
    Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

Texture2D Text : register(t0);

float4 PTextMain(TextVertex V) : SV_TARGET
{
	float4 Color = Text.Sample(MeshTextureSampler, V.UV).a * V.Color;

	if(length(Color.xyz) < .5)
		Color.w = 0.0f;

	return Color;
}

/************************************************************************************************/