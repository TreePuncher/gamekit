Texture2D<float>    Input  : register(t0);
RWTexture2D<float>  Output : register(u0);

[numthreads(32, 32, 1)]
void Copy(const uint2 threadID : SV_DISPATCHTHREADID)
{
    Output[threadID] = Input.Load(uint3(threadID, 0));
}

[numthreads(32, 32, 1)]
void GenerateZLevel(const uint2 threadID : SV_DISPATCHTHREADID)
{
    const float z1 = Input.Load(uint3(threadID + uint2(0, 0), 0));
    const float z2 = Input.Load(uint3(threadID + uint2(0, 1), 0));
    const float z3 = Input.Load(uint3(threadID + uint2(1, 0), 0));
    const float z4 = Input.Load(uint3(threadID + uint2(1, 1), 0));

    const float zOut = max(max(z1, z2), max(z3, z4));

    Output[threadID] = zOut;
}