Texture2D<float3>   source : register(t0);
Texture2D<float>    Input  : register(t1);
RWTexture2D<float>  Output : register(u0);

float GetLuminance(float3 RGB)
{
    return 0.2126f * RGB.x + 0.7152f * RGB.y + 0.0722f * RGB.z;
}

[numthreads(32, 32, 1)]
void InitialLevel(const uint2 threadID : SV_DISPATCHTHREADID)
{
    const float l1 = GetLuminance(source.Load(uint3(threadID + uint2(0, 0), 0)));
    const float l2 = GetLuminance(source.Load(uint3(threadID + uint2(0, 1), 0)));
    const float l3 = GetLuminance(source.Load(uint3(threadID + uint2(1, 0), 0)));
    const float l4 = GetLuminance(source.Load(uint3(threadID + uint2(1, 1), 0)));

    const float lAverage = (l1 + l2 + l3 + l4) / 4.0f;

    Output[threadID] = lAverage;
}

[numthreads(32, 32, 1)]
void CreateLevel(const uint2 threadID : SV_DISPATCHTHREADID)
{
    const float l1 = Input.Load(uint3(threadID + uint2(0, 0), 0));
    const float l2 = Input.Load(uint3(threadID + uint2(0, 1), 0));
    const float l3 = Input.Load(uint3(threadID + uint2(1, 0), 0));
    const float l4 = Input.Load(uint3(threadID + uint2(1, 1), 0));

    const float lAverage = (l1 + l2 + l3 + l4) / 4.0f;

    Output[threadID] = lAverage;
}
