#include "common.hlsl"
#include "VXGI_common.hlsl"


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

StructuredBuffer<OctTreeNode> octree : register(t0);

/*

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
    const bool remove       = node.flags & NODE_FLAGS::DELETE;
    const bool leaf         = node.flags & NODE_FLAGS::LEAF;
    const bool branch       = node.flags & NODE_FLAGS::BRANCH;

    if ((!branch || free))
        return;

    const float4 color      =
        remove  ? float4(1, 0, 0, 0.1f) : float4(0, 0, 0, 0) +
        branch  ? float4(1, 1, 1, 0.01f) : float4(0, 0, 0, 0) +
        leaf    ? float4(0, 0, 1, 0.01f) : float4(0, 0, 0, 0);
        //(!free && !branch && !leaf) ? float4(0.5f, 0.25f, 0.15f, 0.5f) : float4(0, 0, 0, 0);

    const float3 edgeSpan   = volume.max.x - volume.min.x;

    const float4 temp       = mul(PV, float4(volume.min, 1));
    const float3 deviceCord = temp.xyz / temp.w;

    //const uint level = 9;
    //if (node.volumeCord.w < level|| node.volumeCord.w > level)
    //    return;

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

*/


struct Ray
{
    float3 origin;
    float3 dir;
};

bool intersection(const Ray r, const OctreeNodeVolume b)
{
    const float3 invD = rcp(r.dir);

    const float3 t0s = (b.min - r.origin) * invD;
    const float3 t1s = (b.max - r.origin) * invD;

    const float3 tsmaller   = min(t0s, t1s);
    const float3 tbigger    = max(t0s, t1s);

    const float tmin = max(tsmaller[0], max(tsmaller[1], tsmaller[2]));
    const float tmax = min(tbigger[0], min(tbigger[1], tbigger[2]));

    return tmin > 0 && (tmin < tmax);
}

bool intersection(const Ray r, const OctreeNodeVolume b, out float d)
{
    const float3 invD = rcp(r.dir);

    const float3 t0s = (b.min - r.origin) * invD;
    const float3 t1s = (b.max - r.origin) * invD;

    const float3 tsmaller = min(t0s, t1s);
    const float3 tbigger = max(t0s, t1s);

    const float tmin = max(tsmaller[0], max(tsmaller[1], tsmaller[2]));
    const float tmax = min(tbigger[0], min(tbigger[1], tbigger[2]));

    d = tmin;

    return tmin > 0 && (tmin < tmax);
}

struct RayCastResult
{
    uint    node;
    uint    flags;
    float   distance;
    uint    iterations;
};

enum RAYCAST_RESULT_CODES
{
    RAYCAST_ERROR               = -1,
    RAYCAST_NODE_NOT_ALLOCATED  = -2,
    RAYCAST_HIT                 = 1,
    RAYCAST_MISS                = 2,
};


RayCastResult RayCastOctree(const Ray r, in StructuredBuffer<OctTreeNode> octree)
{
    static const uint4 childVIDOffsets[] = {
        uint4(0, 0, 0, 0),
        uint4(1, 0, 0, 0),
        uint4(0, 0, 1, 0),
        uint4(1, 0, 1, 0),

        uint4(0, 1, 0, 0),
        uint4(1, 1, 0, 0),
        uint4(0, 1, 1, 0),
        uint4(1, 1, 1, 0)
    };
            
    uint    parent          = -1;

    uint    nodeID      = 0;
    uint    childID     = -1;
    uint    flags       = octree[0].flags;
    uint    children    = octree[0].children;
    uint4   nodeCord    = uint4(0, 0, 0, 0);

    float distance = 0;

    uint node = 0;
    //[allow_uav_condition]
    for (uint i = 0; i < 200; i++)
    {
        if (flags & BRANCH)
        {   // Push child
            const uint4 childCoordinateBase = uint4(nodeCord.xyz * 2, nodeCord.w + 1);

            for (uint childIdx = (childID != -1 ? childID : 0);
                childIdx < 8;
                childIdx++)
            {                                                     
                const uint childFlags   = GetChildFlags(childIdx, flags);

                if (childFlags == CHILD_FLAGS::EMPTY)
                    continue;

                const uint4 childCoordinate         = childCoordinateBase + childVIDOffsets[childIdx];
                const OctreeNodeVolume childVolume  = GetVolume(childCoordinate.xyz, childCoordinate.w);

                if (intersection(r, childVolume, distance))
                {   // Push child
                    const uint newNode = octree[nodeID].children + childIdx;          
                    
                    parent      = nodeID;

                    nodeID      = newNode;
                    childID     = -1;
                    flags       = octree[newNode].flags;
                    children    = octree[newNode].children;
                    nodeCord    = childCoordinate;           

                    break;
                }
                else continue;
            }


            // All children miss go to next sibling       
            if(childIdx == 8)
            {
                if(parent == -1) // No parent, miss
                {
                    RayCastResult res;
                    res.node        = nodeID;
                    res.flags       = RAYCAST_RESULT_CODES::RAYCAST_MISS;
                    res.distance    = 10000;
                    res.iterations  = i;
                    return res;
                }
                else
                {
                    const uint parent_children = octree[parent].children;
                    childID     = (nodeID - parent_children) + 1; // next sibling

                    nodeID      = parent;
                    flags       = octree[nodeID].flags;
                    children    = parent_children;
                    nodeCord    = uint4(nodeCord.xyz / 2, nodeCord.w - 1);
                    parent      = octree[nodeID].parent;

                    continue;
                }
            }
        }
        else if (flags & LEAF)
        {
            RayCastResult res;
            res.node        = nodeID;
            res.flags       = RAYCAST_RESULT_CODES::RAYCAST_HIT;
            res.distance    = distance;
            res.iterations = i;

            return res;
        }
    }

    RayCastResult res;
    res.node        = -1;
    res.flags       = RAYCAST_RESULT_CODES::RAYCAST_MISS;
    res.iterations  = 32;

    return res;
}

struct PS_output
{
    float4 Color    : SV_TARGET0;
    float Depth : SV_Depth;
};

PS_output VoxelDebug_PS(const float4 pixelPosition : SV_POSITION)
{
    const float2 UV     = float2(pixelPosition.xy / float2(1920, 1080));
    const float3 view   = GetViewVector(UV);

    Ray r;
    r.origin    = CameraPOS;
    r.dir       = normalize(view);
    RayCastResult result = RayCastOctree(r, octree);

    //float3 position = r.origin + r.dir * result.distance;

    PS_output Out;
    //Out.Color = float4(position, 1);
    Out.Color = result.flags == RAYCAST_HIT ? float4(0, 1 * result.iterations / 64.0f, 0, 1) : float4(1 * result.iterations / 64.0f, 0, 0, 0);
    Out.Depth = result.flags == RAYCAST_HIT ? 0.0f : (result.flags == RAYCAST_MISS ? 1.0f : 0.0f);

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
