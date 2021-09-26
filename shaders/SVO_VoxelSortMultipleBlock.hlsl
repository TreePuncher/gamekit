
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

cbuffer constants : register(b1)
{
    uint groupSize;
};


groupshared sortingElement scratchSpace[GROUPSIZE];

RWStructuredBuffer<VoxelSample> samples         : register(u0);
RWStructuredBuffer<VoxelSample> scratchSpace    : register(u1);


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


uint GetDiagonal(uint diagonal)
{
    return -1;
}

struct Partition
{
    uint begin;
    uint end;
};


Partition MergePathPartition(uint Diagonal, const uint A_start, const uint B_start, const uint partitionSize)
{
    uint begin  = 0;
    uint end    = (partitionSize - 1);

    while (begin < end)
    {
        uint mid = (begin + end) / 2;

        VoxelSample A = samples[mid];
        VoxelSample B = samples[Diagonal - mid];

        if (A.mortonID < B.mortonID)
            begin = mid + 1;
        else
            end = mid;
    }
}


/************************************************************************************************/


[numthreads(1024, 1, 1)]
void SortBlock(const uint threadID : SV_GroupIndex, const uint3 globalID : SV_DispatchThreadID, const uint3 groupID : SV_GroupID)
{
    const uint groupIdx = groupID.x;

    /*
    // Load
    sortingElement element;
    if (globalID.x < elementCount)
    {
        element.mortonID    = samples[threadID + (groupIdx * groupSize) * (threadID % 2)].mortonID;
        element.Idx         = threadID;

        scratchSpace[threadID] = element;
    }
    else
    {
        element.mortonID = -1;
        element.Idx = -1;

        scratchSpace[threadID] = element;
    }
    */
}
