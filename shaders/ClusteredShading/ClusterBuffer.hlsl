#define MINZ 0.1f
#define MAXZ 10000.0f
#define SharedMemorySize 32 * 32
#define NUMSLICES 24

struct Cluster
{
    float4 MinPoint;
    float4 MaxPoint;
};

cbuffer Constants : register(b0)
{
    float4x4    IProj;
    float4x4    unused;
    uint2       RenderTargetWH;
    uint2       Dim;
    uint        rowPitch;
    uint        slicePitch;
    uint        lightCount;
};

uint GetClusterIdx(const uint3 clusterID)
{
    return clusterID.x + clusterID.y * rowPitch + clusterID.z * slicePitch;
}

uint GetSliceIdx(float z)
{            
    const float numSlices   = NUMSLICES;                                      
    const float MinOverMax  = MAXZ / MINZ;
    const float LogMoM      = log(MinOverMax);

    return (log(z) * numSlices / LogMoM) - (numSlices * log(MINZ) / LogMoM);
}


float GetSliceDepth(float slice)
{
    const float numSlices = NUMSLICES;                                      
    return MINZ * pow(MAXZ / MINZ, floor(slice) / numSlices);
}


float2 UV2Clip(float2 UV) { return 2 * float2(UV.x, 1.0f - UV.y) - 1.0f; }


float4 Clip2View(float4 DC)
{
    const float4 view = mul(IProj, DC); // View space transform
    return view / view.w;               // Perspective projection
}


float3 LineIntersectionToZPlane(float3 A, float3 B, float Z)
{
    const float3 normal     = float3(0.0, 0.0, 1.0);                    // All clusters planes are aligned in the same z direction   
    const float3 a2b        =  B - A;                                   // Getting the line from the eye to the tile
    const float t           = (Z - dot(normal, A)) / dot(normal, a2b);  // Computing the intersection length for the line and the plane
    const float3 result     = A + t * a2b;                              // Computing the actual xyz position of the point along the line

    return result;
}

Cluster CreateCluster(const uint clusterID, const uint2 renderTargetWH)
{
    const uint X            = (clusterID >> 16) & 0x0f;
    const uint Y            = (clusterID >> 8)  & 0x0f;
    const uint SliceIdx     = (clusterID >> 00) & 0x0f;

    const float minZ        = GetSliceDepth(SliceIdx);
    const float maxZ        = GetSliceDepth(SliceIdx + 1);

    const uint2 TileSize    = uint2(32, 32);
    const float2 TileWHDim  = ceil(float2(renderTargetWH) / TileSize);

    const uint2 Temp = uint2(X, Y);
    const float2 XY1 = float2(Temp.x, Temp.y) / TileWHDim; // {{ -1 -> 1 }, {1 -> -1}}
    const float2 XY2 = float2(Temp.x + 1, Temp.y + 1) / TileWHDim; // {{ -1 -> 1 },  {1 -> -1}}

    const float2 Clip_Min = UV2Clip(XY1);
    const float2 Clip_Max = UV2Clip(XY2);

    const float3 VS_Min = Clip2View(float4(Clip_Min, 1, 1)); // View Vector
    const float3 VS_Max = Clip2View(float4(Clip_Max, 1, 1)); // View Vector

    const float3 center = float3(0, 0,-1);
    const float3 eye    = float3(0, 0, 0);

    const float3 MinPointNear   = LineIntersectionToZPlane(eye, VS_Min, -minZ);
    const float3 MinPointFar    = LineIntersectionToZPlane(eye, VS_Min,  -maxZ);

    const float3 MaxPointNear   = LineIntersectionToZPlane(eye, VS_Max, -minZ);
    const float3 MaxPointFar    = LineIntersectionToZPlane(eye, VS_Max, -maxZ);

    const float3 ClusterMin = min(min(MinPointNear, MinPointFar), min(MaxPointNear, MaxPointFar));
    const float3 ClusterMax = max(max(MinPointNear, MinPointFar), max(MaxPointNear, MaxPointFar));

    Cluster cluster;
    cluster.MinPoint = float4(ClusterMin, asfloat(0));
    cluster.MaxPoint = float4(ClusterMax, asfloat(clusterID));

    return cluster;
}

RWStructuredBuffer<Cluster>     clusterBuffer   : register(u0); // in

[numthreads(32, 32, 1)]
void CreateClusterBuffer(uint3 globalThreadID : SV_DISPATCHTHREADID, uint localThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
    const uint X            = globalThreadID.x;
    const uint Y            = globalThreadID.y;
    const uint slice        = globalThreadID.z;
    const uint clusterID    = (X << 16 | Y << 8 | slice);

    if(X >= Dim.x || Y >= Dim.y || slice >= 24)
        return;

    const uint      clusterIdx  = GetClusterIdx(uint3(X, Y, slice));
    const Cluster   cluster     = CreateCluster(clusterID, RenderTargetWH);

    clusterBuffer[clusterIdx] = cluster;
}