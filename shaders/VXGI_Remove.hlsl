#include "VXGI_Common.hlsl"


/************************************************************************************************/


RWStructuredBuffer<OctTreeNode> octree      : register(u0);
RWStructuredBuffer<uint>        freeList    : register(u1);
RWStructuredBuffer<uint>        tempList    : register(u2);


cbuffer constants : register(b0)
{
    uint erasedCount;
    uint nodeCount;
};


/************************************************************************************************/


[numthreads(1024, 1, 1)]
void ReleaseNodes(uint3 threadID : SV_DispatchThreadID)
{
    if (threadID.x >= erasedCount)
        return;

    const uint nodeA_Idx = freeList[threadID.x];

    if (nodeA_Idx < nodeCount - erasedCount - 1)
    {   // Remove NodeA by moving NodeB from the end into NodeA
        const uint nodeB_Idx = nodeCount - threadID.x - 1;

        OctTreeNode nodeA = octree[nodeA_Idx];
        OctTreeNode nodeB = octree[nodeB_Idx];


        if (nodeA.volumeCord.w == 0 || nodeB.volumeCord.w == 0)
            return;


        if (nodeB.flags == NODE_FLAGS::CLEAR)
            return;


        // Patch NodeB parent to point to new location
        const uint siblings[8] = octree[nodeB.parent].children;
        for (uint II = 0; II < 8; II++)
        {
            if (siblings[II] == nodeB_Idx)
            {
                octree[nodeB.parent].children[II] = nodeA_Idx;
                break;
            }
        }

        octree[nodeA_Idx]       = nodeB;
        octree[nodeB_Idx].flags = NODE_FLAGS::CLEAR;

        const uint temp[8]          = { -1, -1, -1, -1, -1, -1, -1, -1 };
        octree[nodeB_Idx].children  = temp;

        if (nodeB.flags & NODE_FLAGS::BRANCH)
        {
            // Patch NodeB children
            [unroll(8)]
            for (uint I = 0; I < 8; I++)
                octree[nodeB.children[I]].parent = nodeA_Idx;
        }
    }
}


/************************************************************************************************/
