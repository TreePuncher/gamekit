
/************************************************************************************************/


struct ControlPoint
{
	float3	pos;
	half	w;
	half	l;
	float3	v;
	half	angularVelocity;
	half	pad;
	float4  q;
};


/************************************************************************************************/


struct StrandVertex
{
	float3 color : COLOR;
	float4 position : SV_POSITION;
};


/************************************************************************************************/


cbuffer constants : register(b0)
{
	float4x4 PV;
};


StructuredBuffer<ControlPoint> input : register(t0);


/************************************************************************************************/


uint VMain(const uint ID : SV_VertexID) : PRIMITIVEID
{
	return ID;
}


/************************************************************************************************/


[maxvertexcount(16)]
void GMain(point uint primitiveID[1] : PRIMITIVEID, inout TriangleStream<StrandVertex> triangleStream)
{
	const uint id = primitiveID[0] + primitiveID[0] / (6 - 1);

	const ControlPoint controlPoints[2] =
	{
		input[id + 0],
		input[id + 1]
	};

	const float4 t1	= mul(PV, float4(controlPoints[0].pos, 1));
	const float4 t2	= mul(PV, float4(controlPoints[1].pos, 1));

	const float3 p1 = t1.xyz / t1.w;
	const float3 p2 = t2.xyz / t2.w;

	const float3 eye		= float3(0, 0, -1);
	const float3 t			= normalize(p2.xyz - p1.xyz);
	const float3 sideVec	= normalize(cross(t, eye));
	const float  width		= 0.05f;

	StrandVertex vertex;
	vertex.color	= float3(controlPoints[0].pos / 10.0f) + 0.5f;

	// Triangle 1
	vertex.position = t1 - float4(sideVec * width, 0) - float4(t, 0) * width;
	triangleStream.Append(vertex);

	vertex.position = t1 + float4(sideVec * width, 0) - float4(t, 0) * width;
	triangleStream.Append(vertex);

	vertex.position = t2 - float4(sideVec * width, 0);;
	triangleStream.Append(vertex);
	triangleStream.RestartStrip();

	// Triangle 2
	vertex.position = t2 - float4(sideVec * width, 0);
	triangleStream.Append(vertex);

	vertex.position = t1 + float4(sideVec * width, 0) - float4(t, 0) * width;
	triangleStream.Append(vertex);

	vertex.position = t2 + float4(sideVec * width, 0);
	triangleStream.Append(vertex);
	triangleStream.RestartStrip();
}


/************************************************************************************************/


float4 PMain(const float3 color : COLOR) : SV_TARGET
{
	return float4(color, 1);
}


/************************************************************************************************/


struct DebugVert
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
};

[maxvertexcount(6)]
void GDebug(point uint primitiveID[1] : PRIMITIVEID, inout LineStream<DebugVert> lineStream)
{
	const uint id = primitiveID[0] + primitiveID[0] / 6;

	const ControlPoint controlPoint = input[id];

	const float4 t1 = mul(PV, float4(input[0].pos, 1));
	const float3 p1 = t1.xyz / t1.w;
}


/************************************************************************************************/


float4 PDebug(float4 color : COLOR) : SV_TARGET
{
	return float4(1, 1, 1, 1);
}


/**********************************************************************

Copyright (c) 2014-2022 Robert May

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
