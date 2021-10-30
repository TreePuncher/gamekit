struct DispatchCall
{
    uint4 constants;
    uint4 xyz;
};

RWStructuredBuffer<DispatchCall>    args            : register(u0);
StructuredBuffer<uint>              sourceBuffer    : register(t0);


[numthreads(1, 1, 1)]
void Main()
{
    const uint targetCount = sourceBuffer[0];

    DispatchCall arguments;
    arguments.constants = uint4(targetCount, 0, 0, 0);
    arguments.xyz       = uint4(ceil(targetCount / 1024.0f), 1, 1, 0);

    args[0] = arguments;
}
