#define RS0 "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),"\
			"RootConstants(num32BitConstants = 16, b0),"\
			"UAV(u0, flags = DATA_STATIC)"
//			"UAV(u1, flags = DATA_STATIC)"

cbuffer Constants : register(b0)
{
	uint d;
};

struct Triangle
{
	float3 p0;
	float3 p1;
	float3 p2;
};

RWStructuredBuffer<int> CBT			: register(u0);
//RWStructuredBuffer<Triangle> triangleBuffer : register(u1);

/*
uint HeapToBitIndex(uint threadID)
{
	return 0;
}

uint BitToHeapIndex()
{
	
}

void NodeSplitting()
{
	
}
*/

#define DEPTH 4

uint Depth(uint Bit)
{
	return DEPTH - log2(Bit + 1);
}

uint ipow(uint base, uint exp)
{
	uint result = 1;
	for (;;)
	{
		if (exp & 1)
			result *= base;
		exp >>= 1;
		if (!exp)
			break;
		base *= base;
	}

	return result;
}

uint DecodeNode(in RWStructuredBuffer<int> cbt, int leafID)
{
	uint heapID	= 1;
	uint node	= cbt[ipow(2u, 5)];
	
	while (cbt[heapID] > 1)
	{               
        uint temp = cbt[heapID * 2];
		if (leafID < temp)
		{
			heapID	= heapID * 2;
		}
		else
		{
			leafID -= cbt[heapID * 2];
			heapID	= heapID * 2 + 1;
		}
	}
	
	return heapID;
}

uint FindMSB(uint heapID)
{
	return firstbithigh(heapID);
}

bool GetBitValue(uint heapID, uint bitID)
{
	return (heapID >> bitID) & 0x01;
}

float3x3 M0()
{
	return float3x3(
		1.0f, 0.0f, 0.0f, 
		0.5f, 0.0f, 0.5f,
		0.0f, 1.0f, 0.0f);
}

float3x3 M1()
{
	return float3x3(
		0.0f, 1.0f, 0.0f,
		0.5f, 0.0f, 0.5f,
		0.0f, 0.0f, 1.0f);
}

[RootSignature(RS0)]
[numthreads(32, 1, 1)]
void ComputeSumReductionCBT(const uint threadID : SV_GroupIndex)
{
	uint i = threadID + ipow(2, d);
	
	if (i < ipow(2u, d + 1u))
	{
		const uint v0 = CBT[2 * i];
		const uint v1 = CBT[2 * i + 1];
		
		CBT[i] = v0 + v1;
	}
}

[RootSignature(RS0)]
[numthreads(32, 1, 1)]
void Generate(const uint3 threadID : SV_GroupIndex)
{
	if (threadID.x < d)
	{
		//uint idx = HeapToBitIndex(threadID.x);
	}
}

float3x3 WindingMatrix(bool bitValue)
{
	float b = (float)bitValue;
	float c = 1.0f - b;
	
	float3x3 windingMatrix =
	{
		{ c, 0.0f, b },
		{ 0, 1.0f, 0 },
		{ b, 0.0f, c }
	};

	return transpose(windingMatrix);
}

float3x3 SplittingMatrix(bool v)
{
	const float b = (float) v;
	const float c = 1.0f - b;
	
	const float3x3 splitMatrix =
		float3x3(
			c,    b,    0.0f,
			0.5f, 0.0f, 0.5f,
			0.0f, c,    b );
	
	return (splitMatrix);
}

struct Vertex
{
	float3 color	: COLOR;
	float4 position : SV_Position;
};

[RootSignature(RS0)]
Vertex VTestMain(uint vID : SV_VertexID)
{
	const uint tid = (vID / 3);
	
	const float a = 9.0f / 16.0f;
	
	float3x3 t =
	{
		float3(-a * 0.9f,  0.9f, 0.1f),
		float3(-a * 0.9f, -0.9f, 0.1f),
		float3( a * 0.9f, -0.9f, 0.1f),
	};

	float3x3 m =
		float3x3(
			1.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 1.0f);

	const uint heapID	= DecodeNode(CBT, tid);
	const uint d		= FindMSB(heapID);

	for (int bitID = d - 1; bitID >= 0; --bitID)
	{
		if (GetBitValue(heapID, bitID))
			m = mul(M1(), m);
		else
			m = mul(M0(), m);
	}

	const float3x3 tri = mul(m, t);
	Vertex OUT;
	OUT.position	= float4(tri[vID % 3].xyz, 1);
	OUT.color		= (tid != 14) ? float3(1, 1, 1) : float3(1, 0, 0);
	return OUT;
}

float4 PTestMain(Vertex OUT) : SV_Target
{
	return float4(OUT.color, 1);
}
