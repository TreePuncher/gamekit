Texture2D<float4>           source  : register(t0);
Texture2D<float>            Input   : register(t1);
RWStructuredBuffer<float>   Output  : register(u0);

float GetLuminance(float3 RGB)
{
    return 0.2126f * RGB.x + 0.7152f * RGB.y + 0.0722f * RGB.z;
}

groupshared float TempMemory[32*32];

cbuffer Constants : register(b0)
{ 
    uint2 XY;
};


[numthreads(32, 32, 1)]
void LuminanceAverage(const uint3 threadID : SV_DISPATCHTHREADID, const uint localThreadId : SV_GroupIndex, const uint3 groupID : SV_GroupID, const uint3 threadGroupID : SV_GroupThreadID)
{
    TempMemory[localThreadId] = 0.0f;

    const float XY_dim  = 8;
    float lAverage      = 0.0f;

    for (uint X = 0; X < 8; X++)
    {
        for (uint Y = 0; Y < 8; Y++)
        {
            const uint2 groupOffset = groupID.xy * uint2(512, 512) + uint2(X * 64, Y * 64);
            const uint2 XY = uint2(0, 0) + groupOffset;
            const float l1 = GetLuminance(threadID + source.Load(uint3(uint2(0, 0) + XY, 0)));
            const float l2 = GetLuminance(threadID + source.Load(uint3(uint2(0, 1) + XY, 0)));
            const float l3 = GetLuminance(threadID + source.Load(uint3(uint2(1, 0) + XY, 0)));
            const float l4 = GetLuminance(threadID + source.Load(uint3(uint2(1, 1) + XY, 0)));

            lAverage += (l1 + l2 + l3 + l4);
        }
    }

    TempMemory[localThreadId] = lAverage / 4096;
    GroupMemoryBarrierWithGroupSync();

    for (uint I = 0; I < 7; I++)
    {
        const uint groupSize = 512u >> I;
        if (localThreadId < groupSize)
        {
            const float temp =
                TempMemory[localThreadId] +
                TempMemory[localThreadId + groupSize];

            GroupMemoryBarrierWithGroupSync();
            TempMemory[localThreadId] = temp / 2;
        }
        GroupMemoryBarrierWithGroupSync();
    }

    GroupMemoryBarrierWithGroupSync();

    if(localThreadId == 0)
        Output[groupID.x + groupID.y * XY.x] = TempMemory[0];
}


[numthreads(1, 1, 1)]
void AverageLuminance(const uint2 threadID : SV_DISPATCHTHREADID)
{
    const uint size = XY.x * XY.y;

    float average = 0;
    for (uint I = 0; I < size; I++)
        average += Output[I];

    Output[0] = average / size;
}


float4 FullScreen(const uint vertexIdx : SV_VertexId) : SV_POSITION
{
    float4 vertices[] =
        {
            float4(3,  -1,  0,  1),
            float4(-1, -1,  0,  1),
            float4(-1,  3,  0,  1),
        };

    return vertices[vertexIdx];
}


float4 ToneMap(float4 position : SV_POSITION) : SV_TARGET
{
    const float exposure    = 0.5f;
    const float4 hdrColor   = source.Load(position);

    const float4 mapped     = hdrColor / (hdrColor + 1.0); // reinhardt

    //const float sceneExposure = 1.0f / Output[0];
    //const float4 mapped     = 1.0 - exp(-hdrColor * clamp(sceneExposure * 0.5f, 0.1f, 10.0f));

    return pow(mapped, 2.2f);
}
