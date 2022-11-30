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

StructuredBuffer<ControlPoint>		styleBuffer : register(t0);
StructuredBuffer<ControlPoint>		input : register(t1);
RWStructuredBuffer<ControlPoint>	output : register(u0);


cbuffer Constants : register(b0)
{
	float dT;
	float T;
	uint  strandCount;
	uint  strandLength;
}

[numthreads(1024, 1, 1)]
void ApplyForces(const uint3 dispatchID : SV_DispatchThreadID)
{	// Apply Forces
	if (dispatchID.x >= strandCount * strandLength)
		return;
	
	const uint offset = dispatchID.x * strandLength;
	ControlPoint CP = input[dispatchID.x];

	CP.pos += min((float3(20.0f * cos(T * 3.14159f), 20.0f * sin(T * 3.14159f), -4.0f * cos(T * 3.14159f)) + CP.v), float3(CP.l, CP.l, CP.l)) * CP.w * dT; // (float3(30.0f * cos(T * 3), 20.0f * sin(T / 2.0f), 0.0f * cos(T * 5)) + CP.v)* CP.w* dT;

	output[dispatchID.x] = CP;
}


[numthreads(1024, 1, 1)]
void ApplyShapeConstraints(const uint3 dispatchID : SV_DispatchThreadID)
{	// Length Constraint
	if (dispatchID.x >= strandCount * strandLength)
		return;
	
	const float weightStep	= 1.0f / float(strandLength);
	ControlPoint CP			= input[dispatchID.x];
	
	const float S_g		= ((1.0f - weightStep * (dispatchID.x % strandLength)) * dT * 30.0f);
	const float3 P0_i	= styleBuffer[dispatchID.x].pos;
	const float3 P_i	= CP.pos;
	
	CP.pos += S_g * (P0_i - P_i);
	
	output[dispatchID.x] = CP;
}


[numthreads(1024, 1, 1)]
void ApplyEdgeLengthContraints(uint3 dispatchID : SV_DispatchThreadID)
{
	if (dispatchID.x < strandCount)
	{
		const uint		offset		= dispatchID.x * strandLength;
		float3			previousPos	= float3(0, 0, 0);
		float3			previousV	= float3(0, 0, 0);

		for (uint I = 0; I < strandLength && I < 6000; I++)
		{
			ControlPoint	CP = input[offset + I];

			if (I == 0)
			{
				output[I + offset] = CP;
				previousPos = CP.pos;
				continue;
			}

			const float3 v	= CP.pos - previousPos;
			const float3 n	= normalize(v);
			
			const float		vl		= length(v);
			const float3	x_0		= CP.pos;
			const float3	dx		= n * CP.w * (CP.l - vl);
			const float3	p_i		= x_0 + dx;

			const ControlPoint stylePoint = styleBuffer[offset + I];
			
			CP.pos	= p_i;

			output[I + offset] = CP;
			previousPos = CP.pos;
		}
	}
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

