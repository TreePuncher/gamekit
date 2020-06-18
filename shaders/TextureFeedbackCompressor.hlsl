
cbuffer LocalConstants : register(b1)
{

}

// XY - SIZE
// Z  - ID
// W  - PADDING
cbuffer PassConstants : register(b2)
{
    uint2 bufferSize;
}

globallycoherent    RWByteAddressBuffer UAVCounters     : register(u0);
                    RWByteAddressBuffer UAVBuffer       : register(u1);
                    RWByteAddressBuffer UAVOffsets      : register(u2);
                    RWTexture2D<uint>   PPLinkedList    : register(u3);
                    RWByteAddressBuffer UAVOutput       : register(u4);
                    RWTexture2D<uint>   PPTemp          : register(u5);

[numthreads(8, 16, 1)]
void CompressTiles(const uint3 threadID : SV_DispatchThreadID)
{
    uint offset1 = PPLinkedList[threadID.xy * 2 + uint2(0, 0)];
    uint offset2 = PPLinkedList[threadID.xy * 2 + uint2(0, 1)];
    uint offset3 = PPLinkedList[threadID.xy * 2 + uint2(1, 0)];
    uint offset4 = PPLinkedList[threadID.xy * 2 + uint2(1, 1)];
    uint nodeCount = 0;
    uint prevOutputOffset = -1;

    for(uint I = 0; I < 1; I++)
    {
        if( (offset1 == offset2) && 
            (offset2 == offset3) && 
            (offset3 == offset4) &&
             offset1 ==  -1) 
        {
            PPTemp[threadID.xy] = -1;
            return;
        }

        uint3 nodes[4] = { 
            offset1 != -1 ? UAVBuffer.Load3(offset1) : uint3(-1, -1, -1), 
            offset2 != -1 ? UAVBuffer.Load3(offset2) : uint3(-1, -1, -1),
            offset3 != -1 ? UAVBuffer.Load3(offset3) : uint3(-1, -1, -1),
            offset4 != -1 ? UAVBuffer.Load3(offset4) : uint3(-1, -1, -1)};

        offset1 = nodes[0].z;
        offset2 = nodes[1].z;
        offset3 = nodes[2].z;
        offset4 = nodes[3].z;

        for(uint J = 0; J < 4; J++)
        {
            for(uint K = 0; K < 4; K++)
            {
                if(J == K)
                    continue;

                if(nodes[J].x == nodes[K].x && nodes[J].y == nodes[K].y)
                    nodes[K] = uint3(-1, -1, -1);
            }
        }
        
        uint nodeCount = 4;
        for(uint J = 0; J < 4; J++)
            if(nodes[J].x == -1)
                nodeCount--;

        uint outputOffset = 0;
        UAVCounters.InterlockedAdd(0, nodeCount * 12, outputOffset);

        uint outputLocalOffset = 0;
        uint prev = prevOutputOffset;
        for(uint J = 0; J < 4; J++)
        {
            if(nodes[J].x != -1)
            {
                UAVOutput.Store3(outputOffset + outputLocalOffset, uint3(nodes[J].xy, prev));
                prev = outputOffset + outputLocalOffset;
                outputLocalOffset += 12;
            }
        }

        prevOutputOffset = outputOffset;
    }

    PPTemp[threadID.xy] = prevOutputOffset;
}