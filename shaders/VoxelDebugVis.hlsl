#include "common.hlsl"
#include "VXGI_common.hlsl"


/************************************************************************************************/


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


struct PS_output
{
    float4 Color    : SV_TARGET0;
    float Depth     : SV_Depth;
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

    if (result.distance < 0.0f)
        discard;

    PS_output Out;
    //Out.Color = float4(position, 1);
    Out.Color = result.flags == RAYCAST_HIT ? float4(0, 1 * result.iterations / 64.0f, 1 * result.iterations, 1) : float4(1 * result.iterations / 64.0f, 0, 0, 0);
    Out.Depth = result.flags == RAYCAST_HIT ? (result.distance) / 10000.0f : (result.flags == RAYCAST_MISS ? 1.0f : 1.0f);
    //Out.Color = result.flags == RAYCAST_ERROR ? float4(0, 1, 0, 1) : float4(1 * result.iterations / 64.0f, 0, 0, 0);
    //Out.Depth = result.flags == RAYCAST_ERROR ? 0.0f : 1.0f;

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
