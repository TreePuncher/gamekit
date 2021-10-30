
/************************************************************************************************/


struct OctTreeNode
{
    uint parent;
    uint children;
    uint flags;
    uint padding;
    //uint RGBA[8];
};


struct OctreeNodeVolume
{
    float3 min;
    float3 max;
};


/************************************************************************************************/


#define VOLUMESIDE_LENGTH   64
#define VOLUME_SIZE         uint3(VOLUMESIDE_LENGTH, VOLUMESIDE_LENGTH, VOLUMESIDE_LENGTH)
#define MAX_DEPTH           9
#define VOLUME_RESOLUTION   float3(1 << MAX_DEPTH, 1 << MAX_DEPTH, 1 << MAX_DEPTH)


enum NODE_FLAGS
{
    CLEAR               = 0,
    SUBDIVISION_REQUEST = 1 << 0,
    LEAF                = 1 << 1,
    BRANCH              = 1 << 2,
};


enum CHILD_FLAGS
{
    EMPTY           = 0 << 0,
    FILLED          = 1 << 0,
    MASK            = 0x1
};


bool GetChildFlags(const uint childIdx, const uint flags)
{
    return (flags >> childIdx + 4) & CHILD_FLAGS::MASK;
}


/************************************************************************************************/


uint SetChildFlags(const uint childIdx, const uint childFlags, const uint flags = 0)
{
    return (childFlags & CHILD_FLAGS::MASK) << (childIdx + 4) | flags;
}


/************************************************************************************************/



uint GetChildIdxFromChildOffset(uint3 childIdx)
{
    return childIdx.x + childIdx.y * 4 + childIdx.z * 2;

    /*

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


    for (uint I = 0; I < 8; I++)
    {
        if (childIdx.x == childVIDOffsets[I].x &&
            childIdx.y == childVIDOffsets[I].y &&
            childIdx.z == childVIDOffsets[I].z)
            return I;
    }

    return -1;
    */
}


/************************************************************************************************/


float3 GetVoxelPoint(uint4 volumeID)
{
    const uint      depth           = volumeID.w;
    const float     voxelSideLength = float(VOLUMESIDE_LENGTH) / float(1 << depth);
    const float3    xyz             = voxelSideLength * volumeID.xyz;

    return xyz;
}


/************************************************************************************************/


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


/************************************************************************************************/


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


/************************************************************************************************/


float3 VolumeCord2WS(const uint3 cord, const float3 volumePOS_ws, const float3 volumeSize)
{
    return float3(cord) / VOLUME_RESOLUTION * volumeSize + volumePOS_ws;
}


/************************************************************************************************/


uint3 WS2VolumeCord(const float3 pos_WS, const float3 volumePOS_ws, const float3 volumeSize)
{
    const float3 UVW = ((pos_WS - volumePOS_ws) / volumeSize) * VOLUME_RESOLUTION;

    return uint3(UVW);
}


/************************************************************************************************/


uint3 WS2childIdx(const float3 pos_WS, const float3 volumePOS_ws, const float3 volumeSize, uint depth)
{
    const float3 UVW = ((pos_WS - volumePOS_ws) / volumeSize) * VOLUME_RESOLUTION;
    const uint3 volumeID =  uint3(
                                uint(UVW.x) >> (MAX_DEPTH - depth),
                                uint(UVW.y) >> (MAX_DEPTH - depth),
                                uint(UVW.z) >> (MAX_DEPTH - depth));

    return uint3(volumeID.x % 2, volumeID.y % 2, volumeID.z % 2);
}

/************************************************************************************************/


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


/************************************************************************************************/


bool OnScreen(float2 DC)
{
    return ((DC.x >= -1.0f && DC.x <= 1.0f) &&
        (DC.y >= -1.0f && DC.y <= 1.0f));
}


/************************************************************************************************/


struct Ray
{
    float3 origin;
    float3 dir;
};

bool Intersection(const Ray r, const OctreeNodeVolume b)
{
    const float3 invD = rcp(r.dir);

    const float3 t0s = (b.min - r.origin) * invD;
    const float3 t1s = (b.max - r.origin) * invD;

    const float3 tsmaller   = min(t0s, t1s);
    const float3 tbigger    = max(t0s, t1s);

    const float tmin = max(tsmaller[0], max(tsmaller[1], tsmaller[2]));
    const float tmax = min(tbigger[0], min(tbigger[1], tbigger[2]));

    return (tmin < tmax);
}

bool Intersection(const Ray r, const OctreeNodeVolume b, out float d)
{
    const float3 invD = rcp(r.dir);

    const float3 t0s = (b.min - r.origin) * invD;
    const float3 t1s = (b.max - r.origin) * invD;

    const float3 tsmaller = min(t0s, t1s);
    const float3 tbigger = max(t0s, t1s);

    const float tmin = max(tsmaller[0], max(tsmaller[1], tsmaller[2]));
    const float tmax = min(tbigger[0], min(tbigger[1], tbigger[2]));

    d = tmin;

    return (tmin < tmax);
}

struct VoxelRayIntersectionResult
{
    bool intersects;
    float distance;
};

VoxelRayIntersectionResult RayChildIntersection(const Ray r, const uint4 childCoordinate)
{
    const OctreeNodeVolume childVolume = GetVolume(childCoordinate.xyz, childCoordinate.w);
    float traceDistance;

    bool intersects = Intersection(r, childVolume, traceDistance);

    VoxelRayIntersectionResult result;
    result.intersects   = intersects;
    result.distance     = traceDistance;

    return result;
}

struct TraceChildrenResult
{
    float   nearestDistance;
    uint    nearestChild;
    uint4   nearestCoordinate;
};

TraceChildrenResult TraceChildren(const Ray r, const uint4 nodeCord, const uint flags, float r_min = -10000.0f)
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

    const uint4 childCoordinateBase = uint4(nodeCord.xyz * 2, nodeCord.w + 1);

    float   nearestDistance = 100000;
    uint    nearestChild    = -1;
    uint4   nearestCoordinate;

    for (uint childIdx = 0; childIdx < 8; childIdx++)
    {                                                     
        const uint childFlags   = GetChildFlags(childIdx, flags);

        if (childFlags == CHILD_FLAGS::EMPTY)
            continue;

        const uint4 childCoordinate = childCoordinateBase + childVIDOffsets[childIdx];
        const VoxelRayIntersectionResult result = RayChildIntersection(r, childCoordinate);

        if (result.intersects && result.distance > r_min && result.distance < nearestDistance)
        {
            nearestChild        = childIdx;
            nearestDistance     = result.distance;
            nearestCoordinate   = childCoordinate;
        }
        else continue;
    }

    TraceChildrenResult result;
    result.nearestDistance      = nearestDistance;
    result.nearestChild         = nearestChild;
    result.nearestCoordinate    = nearestCoordinate;

    return result;
}


/************************************************************************************************/


struct TraverseResult
{
    uint    node;
    uint    flags;
    uint    childIdx;
    uint4   voxelID;
};

enum TRAVERSE_RESULT_CODES
{
    ERROR               = -1,
    NODE_NOT_ALLOCATED  = -2,
    NODE_FOUND          = 1,
    NODE_EMPTY          = 2,
};


TraverseResult TraverseOctree(const uint4 volumeID, in RWStructuredBuffer<OctTreeNode> octree)
{
    const float3    voxelPoint = GetVoxelPoint(volumeID);
    uint            nodeID     = 0;
    uint4           volumeCord = uint4(0, 0, 0, 0);

    for (uint depth = 0; depth <= MAX_DEPTH; depth++)
    {
        OctTreeNode             n           = octree[nodeID];
        const OctreeNodeVolume  volume      = GetVolume(volumeCord.xyz, volumeCord.w);
        const bool              volumeCheck = IsInVolume(voxelPoint, volume);

        if(volumeCheck)
        {
            if (volumeCord.w == volumeID.w)
            {   // Node Found
                TraverseResult res;
                res.node    = nodeID;
                res.flags   = TRAVERSE_RESULT_CODES::NODE_FOUND;

                return res;
            }
            else
            {
                if (volumeCheck)
                {
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

                    if ((n.flags & NODE_FLAGS::LEAF))
                    {   // Closest node is leaf, node not allocated
                        TraverseResult res;
                        res.node        = nodeID;
                        res.flags       = TRAVERSE_RESULT_CODES::NODE_NOT_ALLOCATED;
                        res.voxelID     = volumeCord;
                        res.childIdx    = -1;

                        const uint4 childCords = uint4(volumeCord.xyz * 2, depth + 1);
                        for (uint childID = 0; childID < 8; childID++)
                        {
                            const uint4 childVID = childCords + childVIDOffsets[childID];
                            const OctreeNodeVolume volume = GetVolume(childVID.xyz, childVID.w);

                            if (!IsInVolume(voxelPoint, volume))
                                continue;

                            res.childIdx = childID;
                            break;
                        }

                        return res;
                    }
                    else
                    {   // traverse to child
                        const uint4 childCords = uint4(volumeCord.xyz * 2, depth + 1);

                        uint childID = 0;
                        const uint childOffset = n.children;

                        for (; childID < 8; childID++)
                        {
                            const uint childFlags = GetChildFlags(childID, n.flags);

                            const uint4 childVID = childCords + childVIDOffsets[childID];
                            const OctreeNodeVolume volume = GetVolume(childVID.xyz, childVID.w);

                            if (IsInVolume(voxelPoint, volume))
                            {
                                if (childFlags == CHILD_FLAGS::EMPTY)
                                {
                                    TraverseResult res;
                                    res.node     = nodeID;
                                    res.flags    = TRAVERSE_RESULT_CODES::NODE_EMPTY;
                                    res.childIdx = childID;

                                    return res;
                                }
                                else
                                {
                                    nodeID = childOffset + childID;
                                    volumeCord = childVID;
                                    break;
                                }
                            }
                        }

                        if (childID == 8)
                        {
                            TraverseResult res;
                            res.node = -1;
                            res.flags = TRAVERSE_RESULT_CODES::ERROR;

                            return res;
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
    res.node    = -1;
    res.flags   = TRAVERSE_RESULT_CODES::ERROR;

    return res;
}


/************************************************************************************************/


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


RayCastResult RayCast_Miss_RES(uint iterations)
{
    RayCastResult res;
    res.node        = -1;
    res.flags       = RAYCAST_RESULT_CODES::RAYCAST_MISS;
    res.distance    = -1;
    res.iterations  = iterations;

    return res;
}


RayCastResult RayCast_Hit_RES(uint nodeID, float distance, uint iterations)
{
    RayCastResult res;
    res.node        = nodeID;
    res.flags       = RAYCAST_RESULT_CODES::RAYCAST_HIT;
    res.distance    = distance;
    res.iterations  = iterations;

    return res;
}


RayCastResult RayCast_Error_RES()
{
    RayCastResult err;
    err.node        = -1;
    err.flags       = RAYCAST_ERROR;
    err.distance    = -1;
    err.iterations  = -1;

    return err;
}


struct StackVariables
{
    uint NodeID;
    uint flags;
    uint children;
};


StackVariables UnpackVars(uint4 stackFrame)
{
    StackVariables vars;
    vars.NodeID     = stackFrame.x;
    vars.flags      = stackFrame.y;
    vars.children   = stackFrame.z;

    return vars;
}


uint4 PackVars(uint nodeID, uint flags, uint children)
{
    return uint4(nodeID, flags, children, 0);
}


RayCastResult RayCastOctree(const Ray r, in StructuredBuffer<OctTreeNode> octree)
{
    uint4 stack[MAX_DEPTH + 1]; // { NodeID, Flags, children }
    float distance      = -10000;
    uint  stackIdx      = 0;
    uint4 nodeCord      = uint4(0, 0, 0, 0);

    stack[0] = PackVars(0, octree[0].flags, octree[0].children);

    [allow_uav_condition]
    for (uint i = 0; i < (MAX_DEPTH + 1) * (MAX_DEPTH + 1); i++)
    {
        const StackVariables stackFrame = UnpackVars(stack[stackIdx]);

        if (stackFrame.flags & BRANCH)
        {   // Push child
            TraceChildrenResult result = TraceChildren(r, nodeCord, stackFrame.flags, 0);

            if (result.nearestChild != -1)
            {
                const uint childNodeID      = stackFrame.children + result.nearestChild;
                const uint4 childCoordinate = result.nearestCoordinate;
                const OctTreeNode child     = octree[childNodeID];

                stack[++stackIdx] = PackVars(childNodeID, child.flags, child.children);

                distance    = result.nearestDistance;
                nodeCord    = childCoordinate; 
            }
            else
            {   // All children miss go to next sibling
                
                for(uint II = 0; II < MAX_DEPTH; II++)
                {
                    if(stackIdx == 0) // at root
                    {
                        const uint4 parentCord = uint4(0, 0, 0, 0);
                        TraceChildrenResult nextChildResult = TraceChildren(r, parentCord, stack[stackIdx].y, distance);

                        if(nextChildResult.nearestChild != -1)
                        {   // Move to next sibling
                            const uint newNodeID        = stackFrame.children + nextChildResult.nearestChild;
                            const OctTreeNode sibling   = octree[newNodeID];

                            stack[++stackIdx]   = PackVars(newNodeID, sibling.flags, sibling.children);
                            nodeCord            = nextChildResult.nearestCoordinate;
                            distance            = nextChildResult.nearestDistance;
                            break;
                        }
                        else
                            return RayCast_Miss_RES(i);
                    }
                    else
                    {
                        const StackVariables    parentFrame     = UnpackVars(stack[stackIdx - 1]);
                        const uint4             parentCord      = uint4(nodeCord.xyz / 2, nodeCord.w - 1);

                        const VoxelRayIntersectionResult result     = RayChildIntersection(r, parentCord);
                        const TraceChildrenResult nextChildResult   = TraceChildren(r, parentCord, parentFrame.flags, max(result.distance, distance));

                        if(nextChildResult.nearestChild != -1)
                        {   // Move to next sibling
                            const uint newNodeID        = parentFrame.children + nextChildResult.nearestChild;
                            const OctTreeNode sibling   = octree[newNodeID];

                            stack[stackIdx] = PackVars(newNodeID, sibling.flags, sibling.children);
                            nodeCord        = nextChildResult.nearestCoordinate;
                            distance        = max(nextChildResult.nearestDistance, distance);
                            break;
                        }
                        else
                        {   // missed all siblings, go to next parent
                            stackIdx--;
                            nodeCord = uint4(nodeCord.xyz / 2, nodeCord.w - 1);
                            distance = max(result.distance, distance);
                            continue;
                        }
                    }

                    if (II + 1 == MAX_DEPTH)
                        return RayCast_Error_RES();
                }
            }
        }
        else if (stackFrame.flags & LEAF)
            return RayCast_Hit_RES(stackFrame.NodeID, distance, i);
    }

    return RayCast_Error_RES();
}


/************************************************************************************************/


#define ComponentSize 20
#define ComponentMask (1 << ComponentSize) - 1


uint64_t CreateMortonCode64(const float3 XYZ)
{
    const uint64_t X = uint64_t(XYZ.x) & ComponentMask;
    const uint64_t Y = uint64_t(XYZ.y) & ComponentMask;
    const uint64_t Z = uint64_t(XYZ.z) & ComponentMask;

    uint64_t  mortonCode = 0;

    for (uint I = 0; I < ComponentSize; I++)
    {
        const uint64_t x_bit = X & (1 << I);
        const uint64_t y_bit = Y & (1 << I);
        const uint64_t z_bit = Z & (1 << I);

        const uint64_t  XYZ = x_bit << 2 | y_bit << 0 | z_bit << 1;

        mortonCode |= XYZ << I * 3;
    }

    return mortonCode;
}


/************************************************************************************************/


uint4 MortonID2VoxelID(uint64_t mortonID)
{
    uint4 coordinate = uint4(0, 0, 0, MAX_DEPTH);
    /*
    [unroll(10)]
    for (uint I = 0; I < 10; I++)
    {
        coordinate.x |= ((mortonID & (1 << (I + 2))) >> (I + 2)) << I;
        coordinate.y |= ((mortonID & (1 << (I + 0))) >> (I + 0)) << I;
        coordinate.z |= ((mortonID & (1 << (I + 1))) >> (I + 1)) << I;
    }
    */

    return coordinate;
}


/************************************************************************************************/


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
