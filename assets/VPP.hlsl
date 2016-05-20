/**********************************************************************

Copyright (c) 2015 Robert May

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

struct Plane
{
	float4 Normal;
	float4 Orgin;
};


cbuffer CameraConstants : register( b0 )
{
	float4x4 View;
	float4x4 Proj;
	float4x4 CameraWT;			// World Transform
	float4x4 PV;				// Projection x View
	float4x4 CameraInverse;
	float4   CameraPOS;
	uint  	 MinZ;
	uint  	 MaxZ;
	int 	 PointLightCount;
	int 	 SpotLightCount;
};


struct GE
{
	uint VertexCount;
	uint VertexOffset;
	uint IndexOffset;
	//uint Padding[1];
};


StructuredBuffer<float4x4>	Transforms		: register(t0);
/*
StructuredBuffer<GE>		GeometryTable	: register(t1); // Index Offsets
StructuredBuffer<uint>		Indices			: register(t2);
StructuredBuffer<float3>	Vertices		: register(t3);
StructuredBuffer<float3>	Normals			: register(t4);
StructuredBuffer<float3>	Tangents		: register(t5);
*/

static const float PI = 3.14159265359f;
static const float PIInverse = 1/PI;

struct VIN
{
	float3 V : POSITION;
	float3 N : NORMAL;
	float3 T : TANGENT;
	uint   instanceID : SV_InstanceID;
};

struct VOUT
{
	float3 WPOS 	: TEXCOORD0;
	float3 N 		: TEXCOORD1;
	float3 T 		: TEXCOORD2;
	float3 B 		: TEXCOORD3;
	float4 POS 	 	: SV_POSITION;
};

VOUT VMain( VIN IN )
{
	VOUT Out;

	float4x4 WT = Transforms[IN.instanceID];

	Out.WPOS 	= mul(WT, float4(IN.V, 1));
	Out.POS		= mul(Proj, mul(View, float4(Out.WPOS, 1.0)));
	Out.N 		= mul(WT, IN.N);
	Out.T 		= mul(WT, IN.T);
	Out.B 		= cross(IN.N, IN.T);
	/*

	if(IN.vertexID < IN.instanceData[0])
	{
		uint Index  = Indices	[IN.instanceData[2]  + IN.vertexID] + IN.instanceData[1];
		float3 V  	= Vertices	[Index];
		float3 N  	= Normals	[Index];
		float3 T  	= Tangents	[Index];
		float4x4 WT = Transforms[IN.instanceID];
		
		Out.WPOS 	= mul(WT, float4(V, 1));
		Out.POS		= mul(Proj, mul(View, float4(Out.WPOS, 1.0)));
		Out.N 		= mul(WT, N);
		Out.T 		= mul(WT, T);
		Out.B 		= cross(N, T);
	}
	else
	{
		Out.WPOS 	= float3(10.0f, 10.0f, 10.0f);
		Out.POS		= float4(10.0f, 10.0f, 10.0f, 10.0f);
		Out.N 		= float3(0.0f, 0.0f, 0.0f);
		Out.T 		= float3(0.0f, 0.0f, 0.0f);
		Out.B 		= float3(0.0f, 0.0f, 0.0f);
	}
	*/
	return Out;
}