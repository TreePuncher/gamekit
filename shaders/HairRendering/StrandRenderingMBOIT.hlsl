#define RS1 "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),"	\
			"RootConstants(num32BitConstants = 17, b0),"		\
			"SRV(t0),"											\
			"UAV(u0, visibility = SHADER_VISIBILITY_PIXEL),"	\
			"UAV(u1, visibility = SHADER_VISIBILITY_PIXEL)"	


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
	uint		offset;
};


struct MLAB_Samples
{
	half4 samples[M];
};

struct MLAB_Depth
{
	float samples[M];
};

StructuredBuffer					<ControlPoint>	input			: register(t0);
RasterizerOrderedStructuredBuffer	<MLAB_Samples>	renderTarget	: register(u0);
RasterizerOrderedStructuredBuffer	<MLAB_Depth>	depthValues		: register(u1);


/************************************************************************************************/


[RootSignature(RS1)]
uint VMain(const uint ID : SV_VertexID) : PRIMITIVEID
{
	return ID;
}


static const float4 fullScreenTriangle[] =
{
	float4(-1, -1, 1, 1),
	float4(-1,  3, 1, 1),
	float4( 3, -1, 1, 1),
};


[RootSignature(RS1)]
float4 VS_FullScreen(const uint ID : SV_VertexID) : SV_POSITION
{
	return fullScreenTriangle[ID];
}



/************************************************************************************************/


[maxvertexcount(16)]
void GMain(point uint primitiveID[1] : PRIMITIVEID, inout TriangleStream<StrandVertex> triangleStream)
{
	const uint id = primitiveID[0] + primitiveID[0] / (21 - 1);

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


void PS_Draw(const float4 color : COLOR, const float4 xy : SV_POSITION, const float depth : DEPTH)
{
	float4 tempSample	= color;
	float tempDepth		= depth;

	float4 samples[M + 1];
	float depthSamples[M + 1];

	
	[unroll(M)]
	for (uint i = 0; i < M; i++)
	{
		samples[i]		= renderTarget[xy.x + xy.y * 1920].samples[i];
		depthSamples[i] = depthValues[xy.x + xy.y * 1920].samples[i];
	}

	samples[M]		= 0;
	depthSamples[M] = INFINITY;

	
	[unroll(M + 1)]
	for (uint i = 0; i < 5; i++)
	{
		if (tempDepth <= depthSamples[i])
		{
			const float4 s	= samples[i];
			const float4 d	= depthSamples[i];
			
			samples[i]			= tempSample;
			depthSamples[i]		= tempDepth;
			
			tempSample	= s;
			tempDepth	= d;
		}
	}

	// Compression
	tempSample =	half4(	samples[M - 1].rgb + (samples[M].rgb * samples[M - 1].w),
							samples[M - 1].w * samples[M].w);
	
	samples[M - 1]		= tempSample;
	depthSamples[M - 1]	= tempDepth;

	for (uint i = 0; i < M - 1; i++)
	{
		renderTarget[xy.x + xy.y * 1920].samples[i]	= samples[i];
		depthValues[xy.x + xy.y * 1920].samples[i]	= depthSamples[i];
	}
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
	return float4(1, 1, 1, 0);
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
