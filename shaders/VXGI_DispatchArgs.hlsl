struct DispatchArgs
{
    uint X;
    uint Y;
    uint Z;
    uint W;
};

RWStructuredBuffer<uint>            counters            : register(u0);
RWStructuredBuffer<DispatchArgs>    dispatchArgsBuffer  : register(u1);

[numthreads(1, 1, 1)]
void CreateIndirectArgs(uint3 threadID : SV_DispatchThreadID)
{
    DispatchArgs args;
    args.X = ceil( counters[0] / 1024.0f );
    args.Y = 1;
    args.Z = 1;
    args.W = 0;

    dispatchArgsBuffer[0] = args;
}
