cbuffer constants : register(b0)
{
    uint erasedCount;
    uint nodeCount;
};

RWStructuredBuffer<uint>  octree    : register(u0);
RWStructuredBuffer<uint>  temp      : register(u1);

[numthreads(1, 1, 1)]
void CreateIndirectArgs(uint3 threadID : SV_DispatchThreadID)
{
    octree[0]   = nodeCount - erasedCount;// + temp[0];
}
