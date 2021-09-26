
struct VoxelSample
{
    uint64_t    mortonID;
    float4      POS;    // Position + ???
    float4      ColorR; // Color + roughness
};

#define GROUPSIZE   1024

struct sortingElement
{
    uint64_t    mortonID;
    uint        Idx;
};

cbuffer constants : register(b0)
{
    uint elementCount;
};

groupshared sortingElement scratchSpace[GROUPSIZE];

RWStructuredBuffer<VoxelSample> samples : register(u0);


/************************************************************************************************/


sortingElement lt_MortonCode(const sortingElement lhs, const sortingElement rhs)
{
    if (lhs.mortonID <= rhs.mortonID)
        return lhs;
    else
        return rhs;
}


/************************************************************************************************/


sortingElement gt_MortonCode(const sortingElement lhs, const sortingElement rhs)
{
    if (lhs.mortonID > rhs.mortonID)
        return lhs;
    else
        return rhs;
}

/************************************************************************************************/


void __CmpSwap(uint lhs, uint rhs, uint op)
{
    const sortingElement LValue = scratchSpace[lhs];
    const sortingElement RValue = scratchSpace[rhs];

    sortingElement V1;
    if (op == 0)
        V1 = lt_MortonCode(LValue, RValue);
    else
        V1 = gt_MortonCode(LValue, RValue);

    sortingElement V2;
    if (op == 0)
        V2 = gt_MortonCode(LValue, RValue);
    else
        V2 = lt_MortonCode(LValue, RValue);

    scratchSpace[lhs] = V1;
    scratchSpace[rhs] = V2;
}


/************************************************************************************************/


void BitonicPass(const uint localThreadID, const int I, const int J)
{
    const uint swapMask = (1 << (J + 1)) - 1;
    const uint offset = 1 << J;

    if ((localThreadID & swapMask) < offset)
    {
        const uint op = (localThreadID >> (I + 1)) & 0x01;
        __CmpSwap(localThreadID, localThreadID + offset, op);
    }

    GroupMemoryBarrierWithGroupSync();
}


/************************************************************************************************/


// Pad extra elements with -1
void LocalBitonicSort(const uint localThreadID)
{
    for (int I = 0; I < 10; I++)
        for (int J = I; J >= 0; J--)
            BitonicPass(localThreadID, I, J);
}

[numthreads(1024, 1, 1)]
void SortBlock(const uint threadID : SV_GroupIndex, uint3 globalID : SV_DispatchThreadID)
{
    // Load Block       
    sortingElement element;
    if (globalID.x < elementCount)
    {
        element.mortonID = samples[globalID.x].mortonID;
        element.Idx = globalID.x;

        scratchSpace[threadID] = element;
    }
    else
    {
        element.mortonID = -1;
        element.Idx = -1;

        scratchSpace[threadID] = element;
    }

    GroupMemoryBarrierWithGroupSync();

    LocalBitonicSort(threadID);

    GroupMemoryBarrierWithGroupSync();

    if (globalID.x < elementCount)
    {
        const VoxelSample temp = samples[scratchSpace[threadID].Idx];
        samples[globalID.x] = temp;
    }
}
