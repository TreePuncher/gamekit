#include "CBT.hlsl"

#define RS_DEBUGVIS "UAV(u0, flags = DATA_VOLATILE), UAV(u1, flags = DATA_VOLATILE), RootConstants(num32BitConstants=1, b0)"

RWStructuredBuffer<uint32_t> CBTBuffer		: register(u0);
RWStructuredBuffer<uint32_t> argumentsOut	: register(u1);

cbuffer constants : register(b)
{
	uint32_t maxDepth;
}

[RootSignature(RS_DEBUGVIS)]
[NumThreads(1, 1, 1)]
void AdaptiveUpdateGetArgs()
{
	const uint nodeCount = GetHeapValue(CBTBuffer, maxDepth, 1);
	
	argumentsOut[0] = nodeCount;
	argumentsOut[1] = nodeCount / 1024 + 1;
	argumentsOut[2] = 1;
	argumentsOut[3] = 1;
}

[RootSignature(RS_DEBUGVIS)]
[NumThreads(1, 1, 1)]
void AdaptiveDrawGetArgs()
{
	const uint heapID = GetHeapValue(CBTBuffer, maxDepth, 1);
	
	argumentsOut[0] = heapID * 3;
	argumentsOut[1] = 1;
	argumentsOut[2] = 0;
	argumentsOut[3] = 0;
}