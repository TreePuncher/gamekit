struct DrawArgs
{
    uint vertexCount;
    uint instanceCount;
    uint vertexBase;
    uint instanceOffset;
};

RWStructuredBuffer<uint>        counters    : register(u0);
RWStructuredBuffer<DrawArgs>    ArgsBuffer  : register(u1);


[numthreads(1, 1, 1)]
void CreateDrawArgs(uint3 threadID : SV_DispatchThreadID)
{
    DrawArgs args;
    args.vertexCount        = counters[0];
    args.instanceCount      = 1;
    args.vertexBase         = 0;
    args.instanceOffset     = 0;

    ArgsBuffer[0] = args;
}
