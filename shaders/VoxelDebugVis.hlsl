#include "common.hlsl"
#include "VXGI_common.hlsl"


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



StructuredBuffer<OctTreeNode> octree    : register(t0);

uint VolumePoint_VS(const uint vertexID : SV_VertexID) : NODEINDEX
{
    return vertexID;
}

struct PS_Input
{
    float4 color : COLOR;
    float  depth : DEPTH;
    float4 position : SV_Position;
    float  w : W_VS_Depth;
};

#define WIREFRAME 0

[maxvertexcount(36)]

#if WIREFRAME
void VoxelDebug_GS(point uint input[1] : NODEINDEX, inout LineStream<PS_Input> outputStream)
#else
void VoxelDebug_GS(point uint input[1] : NODEINDEX, inout TriangleStream<PS_Input> outputStream)
#endif
{
    const OctTreeNode       node    = octree[input[0]];
    const OctreeNodeVolume  volume  = GetVolume(node.volumeCord.xyz, node.volumeCord.w);

    const bool free         = node.flags == NODE_FLAGS::CLEAR;
    const bool leaf         = node.flags & NODE_FLAGS::LEAF;
    const bool branch       = node.flags & NODE_FLAGS::BRANCH;

    const float4 color      =
        free    ? float4(1, 0, 0, 0.01f) : float4(0, 0, 0, 0) +
        branch  ? float4(0, 1, 0, 0.01f) : float4(0, 0, 0, 0) +
        leaf    ? float4(0, 0, 1, 0.01f) : float4(0, 0, 0, 0) +
        (!free && !branch && !leaf) ? float4(0.5f, 0.25f, 0.15f, 0.5f) : float4(0, 0, 0, 0);

    const float3 edgeSpan   = volume.max.x - volume.min.x;

    const float4 temp       = mul(PV, float4(volume.min, 1));
    const float3 deviceCord = temp.xyz / temp.w;

    const uint level = 8;
    if (node.volumeCord.w < level|| node.volumeCord.w > level)
        return;

    //if (!OnScreen(deviceCord.xy))
    //    return;

#if WIREFRAME
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
#else
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

    for (uint triangleID = 0; triangleID < 12; triangleID++)
    {
        for (uint vertexID = 0; vertexID < 3; vertexID++)
        {
            PS_Input v;

            const float3 pos_WT     = (edgeSpan * (1.0f + verts[3 * triangleID + vertexID]) / 2 + volume.min);

            v.position  = mul(PV, float4(pos_WT, 1));
            v.color     = color;
            v.depth     = length(CameraPOS - pos_WT);
            v.w         = mul(View, float4(pos_WT.xyz, 1)).w;

            outputStream.Append(v);
        }

        outputStream.RestartStrip();
    }
#endif
}

struct PS_output
{
    float4 Color0       : SV_TARGET0;
    float4 Revealage    : SV_TARGET1;

    float Depth : SV_Depth;
};

float D(const float Z)
{
    return ((MinZ * MaxZ) / (Z - MaxZ)) / (MinZ - MaxZ);
}

float DepthWeight(const float Z, const float A)
{
    return A * max(0.001f, 1000 * pow(1 - D(Z), 3));
}

PS_output VoxelDebug_PS(PS_Input vertex)
{
    const float  ai = vertex.color.w;
    const float3 ci = vertex.color.xyz * ai;
    const float w   = DepthWeight(-vertex.w, ai);

    PS_output Out;
    Out.Color0      = float4(ci, ai) * w;
    Out.Revealage   = ai;
    Out.Depth       = vertex.depth / MaxZ;

    return Out;
}


/**********************************************************************

Copyright (c) 2015 - 2021 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/
