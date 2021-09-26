#include "VXGI_Common.hlsl"


/************************************************************************************************/


globallycoherent RWStructuredBuffer<OctTreeNode> StaleOctree : register(u0);
RWStructuredBuffer<OctTreeNode> FreshOctree : register(u2);

cbuffer constants : register(b0)
{
    uint erasedCount;
    uint nodeCount;
};


/************************************************************************************************/


[numthreads(1024, 1, 1)]
void ReleaseNodes(const uint3 threadID : SV_DispatchThreadID)
{
    if (threadID.x == 0)
    {
        StaleOctree[0].newNode = 0;
    }
    else if (threadID.x < nodeCount)
    {
        const uint flags = StaleOctree[threadID.x].flags;

        if (flags != CLEAR)
        {
            const uint newIdx = FreshOctree.IncrementCounter();
            StaleOctree[threadID.x].newNode = newIdx;
        }
        else
            StaleOctree[threadID.x].newNode = -1;
    }
}


/************************************************************************************************/


[numthreads(1024, 1, 1)]
void TransferNodes(uint3 threadID : SV_DispatchThreadID)
{
    if (threadID.x > nodeCount)
        return;

    OctTreeNode node    = StaleOctree[threadID.x];
    const uint newIdx   = node.newNode;


    if (node.flags == CLEAR)
        return;


    if (node.parent != -1)
    {
        const uint newParentIdx = StaleOctree[node.parent].newNode;
        node.parent             = newParentIdx;
    }
    if (node.flags & BRANCH)
    {
        [unroll(8)]
        for (uint II = 0; II < 8; II++)
        {
            const uint oldIdx = node.children[II];

            if(oldIdx != -1)
                node.children[II] = StaleOctree[oldIdx].newNode;
        }
    }

    FreshOctree[newIdx] = node;
}


/************************************************************************************************/
