

struct OctTreeNode
{
    uint    nodes[8];
    uint4   volumeCord;
    uint    data;
    uint    parent;
    uint    flags;
    uint    pad;
};


struct OctreeNodeVolume
{
    float3 min;
    float3 max;
};

#define VOLUMESIDE_LENGTH   512
#define VOLUME_SIZE         uint3(VOLUMESIDE_LENGTH, VOLUMESIDE_LENGTH, VOLUMESIDE_LENGTH)
#define MAX_DEPTH           11
#define VOLUME_RESOLUTION   float3(1 << MAX_DEPTH, 1 << MAX_DEPTH, 1 << MAX_DEPTH)

enum NODE_FLAGS
{
    CLEAR               = 0,
    SUBDIVISION_REQUEST = 1 << 0,
    UNITIALISED         = 1 << 1,
    LEAF                = 1 << 2,
    DELETE              = 1 << 3,
    FREE                = 1 << 4,
};


float3 GetVoxelPoint(uint4 volumeID)
{
    const uint      depth = volumeID.w;
    const float     voxelSideLength = float(VOLUMESIDE_LENGTH) / float(1 << depth);
    const float3    xyz = voxelSideLength * volumeID.xyz;

    return xyz;
}

OctreeNodeVolume GetVolume(uint3 xyz, uint depth)
{
    const float     voxelSideLength = float(VOLUMESIDE_LENGTH) / float(1 << depth);
    const float3    volumeDIM       = float3(voxelSideLength, voxelSideLength, voxelSideLength);
    const float3    voxelSize       = float3(volumeDIM);

    const float3 min    = voxelSize * xyz;
    const float3 max    = min + voxelSize;

    OctreeNodeVolume outVolume;
    outVolume.min   = min;
    outVolume.max   = max;

    return outVolume;
}

bool IsInVolume(float3 voxelPoint, OctreeNodeVolume volume)
{
    return
        (   volume.min.x <= voxelPoint.x &&
            volume.min.y <= voxelPoint.y &&
            volume.min.z <= voxelPoint.z &&

            volume.max.x >= voxelPoint.x &&
            volume.max.y >= voxelPoint.y &&
            volume.max.z >= voxelPoint.z);
}

float3 VolumeCord2WS(const uint3 cord, const float3 volumePOS_ws, const float3 volumeSize)
{
    return float3(cord) / VOLUME_RESOLUTION * volumeSize + volumePOS_ws;
}

uint3 WS2VolumeCord(const float3 pos_WS, const float3 volumePOS_ws, const float3 volumeSize)
{
    const float3 UVW = ((pos_WS - volumePOS_ws) / volumeSize) * VOLUME_RESOLUTION;

    return uint3(UVW);
}

bool InsideVolume(float3 pos_WT, const float3 volumePOS_ws, const float3 volumeSize)
{
    return (
        pos_WT.x >= volumePOS_ws.x &&
        pos_WT.y >= volumePOS_ws.y &&
        pos_WT.z >= volumePOS_ws.z &&

        pos_WT.x <= volumePOS_ws.x + volumeSize.x &&
        pos_WT.y <= volumePOS_ws.y + volumeSize.y &&
        pos_WT.z <= volumePOS_ws.z + volumeSize.z );
}


bool OnScreen(float2 DC)
{
    return ((DC.x >= -1.0f && DC.x <= 1.0f) &&
        (DC.y >= -1.0f && DC.y <= 1.0f));
}


struct TraverseResult
{
    uint node;
    uint flags;
};

enum TRAVERSE_RESULT_CODES
{
    ERROR = -1,
    NODE_NOT_ALLOCATED = -2,
    NODE_FOUND
};


TraverseResult TraverseOctree(const uint4 volumeID, in RWStructuredBuffer<OctTreeNode> octree)
{
    const float3    voxelPoint = GetVoxelPoint(volumeID);
    uint            nodeID     = 0;

    for (uint depth = 0; depth <= MAX_DEPTH; depth++)
    {
        OctTreeNode             n           = octree[nodeID];
        const OctreeNodeVolume  volume      = GetVolume(n.volumeCord.xyz, n.volumeCord.w);
        const bool              volumeCheck = IsInVolume(voxelPoint, volume);

        if(volumeCheck)
        {
            if (n.volumeCord.w == volumeID.w)
            {   // Node Found
                TraverseResult res;
                res.node    = nodeID;
                res.flags   = TRAVERSE_RESULT_CODES::NODE_FOUND;

                return res;
            }
            else
            {
                if ((n.flags & NODE_FLAGS::LEAF) && volumeCheck)
                {   // Closest node is leaf, node not allocated
                    TraverseResult res;
                    res.node    = nodeID;
                    res.flags   = TRAVERSE_RESULT_CODES::NODE_NOT_ALLOCATED;

                    return res;
                }
                else
                {   // traverse to child
                    const uint4 childCords = uint4(n.volumeCord.xyz * 2, depth + 1);

                    const uint4 childVIDOffsets[] = {
                        uint4(0, 0, 0, 0),
                        uint4(1, 0, 0, 0),
                        uint4(0, 0, 1, 0),
                        uint4(1, 0, 1, 0),

                        uint4(0, 1, 0, 0),
                        uint4(1, 1, 0, 0),
                        uint4(0, 1, 1, 0),
                        uint4(1, 1, 1, 0)
                    };

                    for (uint childID = 0; childID < 8; childID++)
                    {
                        const uint4 childVID            = childCords + childVIDOffsets[childID];
                        const OctreeNodeVolume volume   = GetVolume(childVID.xyz, childVID.w);

                        if (IsInVolume(voxelPoint, volume))
                        {
                            nodeID = n.nodes[childID];
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            TraverseResult res;
            res.node    = -1;
            res.flags   = TRAVERSE_RESULT_CODES::ERROR;

            return res;
        }
    }

    TraverseResult res;
    res.node = -1;
    res.flags = TRAVERSE_RESULT_CODES::ERROR;

    return res;
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
