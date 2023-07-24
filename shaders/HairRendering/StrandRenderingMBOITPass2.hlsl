#include "MBOIT.hlsl"

#define RS1 "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),"	\
			"RootConstants(num32BitConstants = 22, b0),"		\
			"SRV(t0),"											\
			"DescriptorTable("									\
				"SRV(t1, numDescriptors = 3))"


/************************************************************************************************/


#define INFINITY ((float)(1e+300 * 1e+300))
#define M 4

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
	float4 color	: COLOR;
	float4 position : SV_POSITION;
	float  depth	: DEPTH;
};


/************************************************************************************************/


cbuffer constants : register(b0)
{
	float4x4	PV;
	float4		wrapping_zone_parameters;
	float		overestimation;
	float		moment_bias;
};


StructuredBuffer	<ControlPoint>	Input	: register(t0);
Texture2D			<float>			B0		: register(t1);
Texture2D			<float4>		Moments	: register(t2);


/************************************************************************************************/


[RootSignature(RS1)]
uint VMain(const uint ID : SV_VertexID) : PRIMITIVEID
{
	return ID;
}


/************************************************************************************************/


[maxvertexcount(16)]
void GMain(point uint primitiveID[1] : PRIMITIVEID, inout TriangleStream<StrandVertex> triangleStream)
{
	const uint id = primitiveID[0] + primitiveID[0] / (21 - 1);

	const ControlPoint controlPoints[2] =
	{
		Input[id + 0],
		Input[id + 1]
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
	vertex.color	= float4((controlPoints[0].pos) / 10.0 + 0.5f, 0.1);
	//const float a = pow((id % 21) / 21.0f, 2.0f);
	//vertex.color = float4(a * a, a * a, a * a, a);

	// Triangle 1
	vertex.position = t1 - float4(sideVec * width, 0) - float4(t, 0) * width;
	vertex.depth	= vertex.position.z;
	triangleStream.Append(vertex);

	vertex.position = t1 + float4(sideVec * width, 0) - float4(t, 0) * width;
	vertex.depth	= vertex.position.z;
	triangleStream.Append(vertex);

	vertex.position = t2 - float4(sideVec * width, 0);;
	vertex.depth	= vertex.position.z;
	triangleStream.Append(vertex);
	triangleStream.RestartStrip();

	// Triangle 2
	vertex.position = t2 - float4(sideVec * width, 0);
	vertex.depth	= vertex.position.z;
	triangleStream.Append(vertex);

	vertex.position = t1 + float4(sideVec * width, 0) - float4(t, 0) * width;
	vertex.depth	= vertex.position.z;
	triangleStream.Append(vertex);

	vertex.position = t2 + float4(sideVec * width, 0);
	vertex.depth	= vertex.position.z;
	triangleStream.Append(vertex);
	triangleStream.RestartStrip();
}


/************************************************************************************************/


float4 PS_Draw(const float4 color : COLOR, const float4 xy : SV_POSITION, const float depth : DEPTH) : SV_Target
{
	float b0		= B0[uint2(xy.xy)];
	float4 m		= Moments[uint2(xy.xy)];
	const float t	= ComputeTransmittanceAtDepthFrom4PowerMoments(b0, m.xy, m.zw, depth, moment_bias, overestimation, wrapping_zone_parameters);
	
	return float4(t * color.xyz, color.a);
}


/**********************************************************************

Copyright (c) 2014-2023 Robert May

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
