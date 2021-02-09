struct Cluster
{
    float4 MinPoint;
    float4 MaxPoint;
};

struct PointLight
{
    float4 KI;	// Color + intensity in W
    float4 PR;	// XYZ + radius in W
};

cbuffer clusterConstants : register(b0)
{
    uint clusterCount;
}

cbuffer passConstants : register( b1 )
{
    float FOV;
    uint2 WH;
}


RWStructuredBuffer<uint>    shadowMapResolutions : register(u0);

StructuredBuffer<Cluster>       Clusters    : register(t0); 
StructuredBuffer<uint>          lightList   : register(t1);
StructuredBuffer<PointLight>    PointLights : register(t2); 

#define PI 3.14159265359f

[numthreads(1024, 1, 1)]
void Clear(const uint threadID : SV_GroupIndex)
{
    shadowMapResolutions[threadID] = 0;
}

/*
uint DaMathStuffHappensHere(const uint LightIdx, const float3 C_pos, const float C_r, const float S)
{
    PointLight light   = PointLights[LightIdx];
    const float3 L_pos = light.PR.xyz;
    const float3 L_r   = light.PR.r;

    const float d      = lenth(C_pos - L_pos);
    const float alpha  = sqrt((d * d) - (L_r * L_r)) * (L_r / d);

    const float r = sqrt((2 / (alpha/(4 * PI))) / 6);

    return r;
}

float ScreenSpaceSize(const float3 C_pos, const float C_r)
{
    const float D       = lenght(C_pos);
    const float alpha   = sqrt((d * d) - (C_r * C_r)) * (C_r / d);
    const float pr      = cot(FOV / 2) * alpha / sqrt(D*D - alpha*alpha);

    return pr * WH.y;
}
*/

[numthreads(32, 1, 1)]
void ResolutionMatch(const uint threadID : SV_GroupIndex, const uint3 groupID : SV_GROUPID)
{
    return;
    const uint groupIdx = groupID.x + groupID.y * 1024;
    if(groupIdx < clusterCount)
    {
        Cluster C = Clusters[groupIdx];

        const uint list     = asuint(C.MinPoint.w);
        const uint count    = asuint(C.MaxPoint.w);
        const float3 C_pos  = (C.MinPoint + C.MaxPoint) / 2.0f;
        const float C_r     = length(C.MaxPoint - C.MinPoint) / 2.0f;

        const uint end = ceil(float(count) / 32);
        for(uint I = 0; I * 32 < end; ++I)
        {
            if(threadID < count){
                const uint idx                      = list + threadID + I * 32;
                const uint lightIdx                 = lightList[idx];
                //const uint requestedTexelDensity    = DaMathStuffHappensHere(lightIdx, C_pos, C_r);

                //InterlockedMax(shadowMapResolutions[lightIdx], requestedTexelDensity);
            }
        }

        GroupMemoryBarrierWithGroupSync();
    }
}