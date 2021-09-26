struct SortingDispatch
{
    uint4 constants;
    uint4 xyz;
};

RWStructuredBuffer<SortingDispatch>     args : register(u0);
RWStructuredBuffer<uint>            counters : register(u1);

[numthreads(1, 1, 1)]
void Main()
{
    const uint voxelSamples = counters[0];

    SortingDispatch dispatch;
    dispatch.constants  = uint4(voxelSamples, 0, 0, 0);
    dispatch.xyz        = uint4(ceil(voxelSamples / 1024.0f), 1, 1, 0);

    args[0] = dispatch;
}
