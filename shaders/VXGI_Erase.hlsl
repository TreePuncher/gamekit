#include "VXGI_Common.hlsl"
#include "Common.hlsl"

/************************************************************************************************/

Texture2D<float> depth  : register(t0);

RWStructuredBuffer<OctTreeNode> octree      : register(u0);


/************************************************************************************************/


[numthreads(1024, 1, 1)]
void MarkEraseNodes(uint3 threadID : SV_DispatchThreadID)
{
    const uint nodeIdx  = threadID.x;

    OctTreeNode node    = octree[nodeIdx];

    if (node.flags & NODE_FLAGS::DELETE)
        return;
    else if (node.flags & NODE_FLAGS::LEAF)
    {
        const float3 voxelPOS_ws = GetVoxelPoint(node.volumeCord);
        const float4 temp = mul(PV, float4(voxelPOS_ws, 1));
        const float3 deviceCord = temp.xyz / temp.w;

        uint Width, Height, _;
        depth.GetDimensions(0, Width, Height, _);

        if (temp.w > 0.01f && OnScreen(deviceCord.xy))
        {
            const float3 cord = float3(deviceCord.x / 2.0f + 0.5f, 1 - (0.5f + deviceCord.y / 2.0f), 0);

            if (cord.y <= 0.0f || cord.y >= 1.0f ||
                cord.x <= 0.0f || cord.x >= 1.0f)
                return;

            const float d = depth.Load(cord * uint3(Width, Height, 1)) * MaxZ;
            const float l = length(voxelPOS_ws - CameraPOS.xyz);

            const bool cull = d > (l + 0.05f);

            if (cull)
                //octree[threadID.x].flags = CLEAR;
                InterlockedOr(octree[threadID.x].flags, NODE_FLAGS::DELETE);
        }
    }
    else if (node.flags & NODE_FLAGS::BRANCH)
    {
        bool Keep = false;
        uint newNodeIdx = nodeIdx;

        [unroll(8)]
        for (uint I = 0; I < 8; I++)
        {
            const uint childIdx = node.children[I];

            if (childIdx == -1)
                continue;

            const uint flags = octree[childIdx].flags;

            Keep |= (flags & DELETE) == 0;
        }

        if (Keep)
        {
            node.flags = NODE_FLAGS::BRANCH;

            [unroll(8)]
            for (uint I = 0; I < 8; I++)
            {
                const uint childIdx = node.children[I];

                if (childIdx == -1)
                    continue;

                octree[childIdx].flags &= ~NODE_FLAGS::DELETE;
            }
        }
        else
        {
            [unroll(8)]
            for (uint I = 0; I < 8; I++)
            {
                const uint childIdx = node.children[I];

                if (childIdx == -1)
                    continue;

                octree[childIdx].flags = NODE_FLAGS::CLEAR;
            }

            node.flags = NODE_FLAGS::LEAF;
        }

        octree[nodeIdx] = node;
    }
}


/************************************************************************************************/
