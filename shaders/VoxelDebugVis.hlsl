#include "common.hlsl"


/*
float4 FullScreenQuad_VS(const uint vertexID : SV_VertexID) : SV_POSITION
{
    float4 verts[] = {
        float4(-1,  1, 1, 1),
        float4( 1, -1, 1, 1),
        float4(-1, -1, 0, 1),

        float4(-1,  1, 1, 1),
        float4( 1,  1, 1, 1),
        float4( 1, -1, 1, 1),
    };

    return verts[vertexID];
}


int4 VolumePoint_VS(const uint vertexID : SV_VertexID) : POINT_POSITION
{
    const uint3 volumeIdx = uint3(vertexID % 128, (vertexID % (128 * 128)) / 128, vertexID / (128 * 128));
    return int4( volumeIdx, 0);
}


bool InVolume(const float3 position, const float3 volumePosition, const float3 volumeSize)
{
    return
        ((position.x >= volumePosition.x) && (position.y >= volumePosition.y) && (position.z >= volumePosition.z)) &
        (   (position.x <= volumePosition.x + volumeSize.x) &&
            (position.y <= volumePosition.y + volumeSize.y) &&
            (position.z <= volumePosition.z + volumeSize.z));
}


float4 SampleVoxel(const float3 position, const float3 volumePosition, const float3 volumeSize)
{
    const float3 sampleUVW = (position - volumePosition) / volumeSize;
    const uint3  sampleIdx = float3(128, 128, 128) * sampleUVW;

    return volume[sampleIdx];
}
*/

struct OctTreeNode
{
    uint    nodes[8];
    uint4   volumeCord;
    uint    data;
    uint    flags;
    uint    pad[2];
};

struct OctreeNodeVolume
{
    float3 min;
    float3 max;
};

#define VOLUME_SIZE         uint3(128, 128, 128)
#define VOLUME_RESOLUTION   float3(128, 128, 128)
#define MAX_DEPTH           8

OctreeNodeVolume GetVolume(uint4 volumeID)
{
    const uint      depth      = volumeID.w;
    const uint      sideLength = 128 >> depth;
    const uint3     volumeDIM  = uint3(sideLength, sideLength, sideLength);
    const float3    voxelSize  = float3(volumeDIM);

    const float3 min = voxelSize * volumeID;
    const float3 max = min + voxelSize;

    OctreeNodeVolume outVolume;
    outVolume.min = min;
    outVolume.max = max;

    return outVolume;
}

StructuredBuffer<OctTreeNode> octree    : register(t0);

uint VolumePoint_VS(const uint vertexID : SV_VertexID) : NODEINDEX
{
    return vertexID;
}

struct PS_Input
{
    float3 color : COLOR;
    float  depth : DEPTH;
    float4 position : SV_Position;
};

[maxvertexcount(36)]
void VoxelDebug_GS(point uint input[1] : NODEINDEX, inout LineStream<PS_Input> outputStream)
{
    const OctTreeNode       node    = octree[input[0]];
    const OctreeNodeVolume  volume  = GetVolume(node.volumeCord);

    const bool leaf = node.volumeCord.w == 7;

    const float3 color = leaf ? float3(0, 1, 0) : float3(1, 0, 0);

    if (!leaf)
        return;

    PS_Input A, B;
    A.color     = color;
    A.depth     = length(CameraPOS - volume.min);
    A.position  = mul(PV, float4(volume.min, 1));
    B.color     = color;
    B.depth     = length(CameraPOS - float3(volume.min.x, volume.max.y, volume.min.z));
    B.position  = mul(PV, float4(volume.min.x, volume.max.y, volume.min.z, 1));

    outputStream.Append(A);
    outputStream.Append(B);

    outputStream.RestartStrip();

    A.position  = mul(PV, float4(volume.min, 1));
    B.position  = mul(PV, float4(volume.max.x, volume.min.y, volume.min.z, 1));
    A.depth     = length(CameraPOS - float3(volume.min));
    B.depth     = length(CameraPOS - float3(volume.max.x, volume.min.y, volume.min.z));



    outputStream.Append(A);
    outputStream.Append(B);

    outputStream.RestartStrip();

    A.position  = mul(PV, float4(volume.min, 1));
    B.position  = mul(PV, float4(volume.min.x, volume.min.y, volume.max.z, 1));
    A.depth     = length(CameraPOS - float3(volume.min));
    B.depth     = length(CameraPOS - float3(volume.min.x, volume.min.y, volume.max.z));

    outputStream.Append(A);
    outputStream.Append(B);

    outputStream.RestartStrip();

    A.position  = mul(PV, float4(volume.max, 1));
    B.position  = mul(PV, float4(volume.max.x, volume.min.y, volume.max.z, 1));
    A.depth     = length(CameraPOS - float3(volume.max));
    B.depth     = length(CameraPOS - float3(volume.max.x, volume.min.y, volume.max.z));

    outputStream.Append(A);
    outputStream.Append(B);

    outputStream.RestartStrip();

    A.position  = mul(PV, float4(volume.max, 1));
    B.position  = mul(PV, float4(volume.min.x, volume.max.y, volume.max.z, 1));
    A.depth     = length(CameraPOS - float3(volume.max));
    B.depth     = length(CameraPOS - float3(volume.min.x, volume.max.y, volume.max.z));

    outputStream.Append(A);
    outputStream.Append(B);

    outputStream.RestartStrip();

    A.position  = mul(PV, float4(volume.max, 1));
    B.position  = mul(PV, float4(volume.max.x, volume.max.y, volume.min.z, 1));
    A.depth     = length(CameraPOS - float3(volume.max));
    B.depth     = length(CameraPOS - float3(volume.max.x, volume.max.y, volume.min.z));

    outputStream.Append(A);
    outputStream.Append(B);

    outputStream.RestartStrip();

    A.position  = mul(PV, float4(volume.max.x, volume.max.y, volume.min.z, 1));
    B.position  = mul(PV, float4(volume.max.x, volume.min.y, volume.min.z, 1));
    A.depth     = length(CameraPOS - float3(volume.max.x, volume.max.y, volume.min.z));
    B.depth     = length(CameraPOS - float3(volume.max.x, volume.min.y, volume.min.z));

    outputStream.Append(A);
    outputStream.Append(B);

    outputStream.RestartStrip();

    A.position  = mul(PV, float4(volume.max.x, volume.max.y, volume.min.z, 1));
    B.position  = mul(PV, float4(volume.min.x, volume.max.y, volume.min.z, 1));
    A.depth     = length(CameraPOS - float3(volume.max.x, volume.max.y, volume.min.z));
    B.depth     = length(CameraPOS - float3(volume.min.x, volume.max.y, volume.min.z));

    outputStream.Append(A);
    outputStream.Append(B);

    outputStream.RestartStrip();

    A.position  = mul(PV, float4(volume.min.x, volume.max.y, volume.min.z, 1));
    B.position  = mul(PV, float4(volume.min.x, volume.max.y, volume.max.z, 1));
    A.depth     = length(CameraPOS - float3(volume.min.x, volume.max.y, volume.min.z));
    B.depth     = length(CameraPOS - float3(volume.min.x, volume.max.y, volume.max.z));

    outputStream.Append(A);
    outputStream.Append(B);

    outputStream.RestartStrip();

    A.position  = mul(PV, float4(volume.min.x, volume.max.y, volume.max.z, 1));
    B.position  = mul(PV, float4(volume.min.x, volume.min.y, volume.max.z, 1));
    A.depth     = length(CameraPOS - float3(volume.min.x, volume.max.y, volume.max.z));
    B.depth     = length(CameraPOS - float3(volume.min.x, volume.min.y, volume.max.z));

    outputStream.Append(A);
    outputStream.Append(B);

    outputStream.RestartStrip();

    A.position  = mul(PV, float4(volume.max.x, volume.min.y, volume.max.z, 1));
    B.position  = mul(PV, float4(volume.max.x, volume.min.y, volume.min.z, 1));
    A.depth     = length(CameraPOS - float3(volume.max.x, volume.min.y, volume.max.z));
    B.depth     = length(CameraPOS - float3(volume.max.x, volume.min.y, volume.min.z));

    outputStream.Append(A);
    outputStream.Append(B);

    outputStream.RestartStrip();

    A.position  = mul(PV, float4(volume.max.x, volume.min.y, volume.max.z, 1));
    B.position  = mul(PV, float4(volume.min.x, volume.min.y, volume.max.z, 1));
    A.depth     = length(CameraPOS - float3(volume.max.x, volume.min.y, volume.max.z));
    B.depth     = length(CameraPOS - float3(volume.min.x, volume.min.y, volume.max.z));

    outputStream.Append(A);
    outputStream.Append(B);

    outputStream.RestartStrip();

    /*
    const float3 verts[] = {
        float3(-1,  1, 1),
        float3( 1, -1, 1),
        float3(-1, -1, 1),

        float3(-1,  1, 1),
        float3( 1,  1, 1),
        float3( 1, -1, 1),

        float3( 1,  1, -1),
        float3(-1,  1, -1),
        float3( 1, -1, -1),

        float3( 1, -1, -1),
        float3(-1,  1, -1),
        float3(-1, -1, -1),

        float3(1, -1,  1),
        float3(1,  1, -1),
        float3(1, -1, -1),

        float3(1, -1,  1),
        float3(1,  1,  1),
        float3(1,  1, -1),

        float3(-1,  1, -1),
        float3(-1, -1,  1),
        float3(-1, -1, -1),

        float3(-1,  1,  1),
        float3(-1, -1,  1),
        float3(-1,  1, -1),

        float3( 1, 1, -1),
        float3(-1, 1,  1),
        float3(-1, 1, -1),

        float3( 1, 1,  1),
        float3(-1, 1,  1),
        float3( 1, 1, -1),

        float3(-1, -1,  1),
        float3( 1, -1, -1),
        float3(-1, -1, -1),

        float3(-1, -1,  1),
        float3( 1, -1,  1),
        float3( 1, -1, -1),
    };

    const float4 volumeSample = volume[input[0].xyz];

    if (volumeSample.w == 0.0f)
        return;

    for (uint triangleID = 0; triangleID < 12; triangleID++)
    {
        for (uint vertexID = 0; vertexID < 3; vertexID++)
        {
            PS_Input v;

            const float scaleFactor = 0.1f;
            const float3 pos_WT     = (verts[3 * triangleID + vertexID] + input[0].xyz * 2) * scaleFactor;

            v.position  = mul(PV, float4(pos_WT, 1));
            v.color     = volumeSample.xyz;
            v.depth     = length(CameraPOS - pos_WT);
            outputStream.Append(v);
        }

        outputStream.RestartStrip();
    }
    */
}

struct PS_output
{
    float4  target  : SV_TARGET;
    float   depth   : SV_DEPTH;
};

PS_output VoxelDebug_PS(PS_Input input)
{
    /*
    float4 VoxelDebug_PS(float4 pixelCord : SV_POSITION) : SV_TARGET
    uint width, height, numLevels;
    depthBuffer.GetDimensions(0, width, height, numLevels);

    const float2 UV     = pixelCord.xy / float2(width, height); // [0;1]
    const float depth   = depthBuffer.Load(int3(pixelCord.xy, 0));

    const float3 positionVS = GetViewSpacePosition(UV, depth);
    const float3 positionWS = GetWorldSpacePosition(UV, depth);

    const float3 volumePosition = float3(-32, -1, -32);
    const float3 volumeSize     = float3(64, 64, 64);

    if (InVolume(positionWS, volumePosition, volumeSize))
        return SampleVoxel(positionWS, volumePosition, volumeSize);
    else
        return float4(0, 0, 0, 0);
    */

    PS_output output;
    output.target   = float4(input.color, 1);
    output.depth    = input.depth / MaxZ;

    return output;
}
