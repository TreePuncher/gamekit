/**********************************************************************

Copyright (c) 2014-2016 Robert May

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

// ---------------------------------------------------------------------------

cbuffer CameraConstants : register( b0 )
{
	float4x4 View;
	float4x4 Proj;
	float4x4 CameraWT;		// World Transform
	float4x4 PV;			// Projection x View
}

cbuffer LocalConstants : register( b1 )
{
	float4	 Albedo; 	// + Roughness
	float4	 Specular;	// + Metal
	float4x4 WT;
}

// ---------------------------------------------------------------------------

struct GOUT
{
	float3 WPOS 	: TEXCOORD0; 	// WorldSpace
	float4 NORMAL 	: TEXCOORD1;
	float3 TANGENT 	: TEXCOORD2;
	float4 POS    	: SV_POSITION; // ScreenSpace
};

struct VOUT
{
	float4 POS 	: SV_POSITION; // Screen Space
	float3 WPOS : POSITION;   // World Space
};

// ---------------------------------------------------------------------------

float3 CalculateNormal(float3 V0, float3 V1, float3 V2)
{
    float3 normal = cross(V2 - V0, V1 - V0);
    normal = normalize(normal);
    return normal;
}

// ---------------------------------------------------------------------------


[maxvertexcount(3)]
void GSMain( triangle VOUT gin[3], inout TriangleStream<GOUT> gout )
{
	float4 normal = float4( CalculateNormal( gin[ 0 ].WPOS, gin[ 1 ].WPOS, gin[ 2 ].WPOS ), 0);
	float3 tangent = float3(0.0f, 0.0f, 0.0f);

	GOUT v;
	v.TANGENT   = tangent;

	v.WPOS 		= gin[0].WPOS;
	v.POS 		= gin[0].POS;
	v.NORMAL 	= normal;
	gout.Append(v);
	v.WPOS 		= gin[1].WPOS;
	v.POS 		= gin[1].POS;
	v.NORMAL 	= normal;
	gout.Append(v);
	v.WPOS 		= gin[2].WPOS;
	v.POS 		= gin[2].POS;
	v.NORMAL 	= normal;
	gout.Append(v);

	gout.RestartStrip();
}

struct TextEntry
{
	float2 POS			 : TEXCOORD0;
	float2 Size			 : TEXCOORD1;
	float2 TopLeftUV	 : TEXCOORD2;
	float2 BottomRightUV : TEXCOORD3;
	float4 Color		 : TEXCOORD4;
};

