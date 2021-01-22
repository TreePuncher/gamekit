cbuffer constants : register(b0)
{
    uint clusterCount;
}

cbuffer constants : register(b1)
{
    float4x4    IView;
    uint        rootNode;
}

struct Cluster
{
    float4 MinPoint;
    float4 MaxPoint;
};

struct BVH_Node
{
    float4 MinPoint;
    float4 MaxPoint;

    uint Offset;
    uint Count; // max child count is 16

    uint Leaf;
};

struct AABB
{
    float3 MinPoint;
    float3 MaxPoint;
};

struct PointLight
{
    float4 KI;	// Color + intensity in W
    float4 PR;	// XYZ + radius in W
};

#define NODEMAXSIZE 32

RWStructuredBuffer<Cluster>     Clusters      : register(u0); // in-out
RWStructuredBuffer<uint>        lightList     : register(u1); // in-out
RWStructuredBuffer<uint>        counters      : register(u2); // in-out
RWStructuredBuffer<BVH_Node>    BVHNodes      : register(u3);

StructuredBuffer<uint>                              LightLookup   : register(t1); 
StructuredBuffer<PointLight>                        PointLights   : register(t2); 

#define MaxLocalLightCount 128

groupshared uint counts[32];
groupshared uint localLightLists[32 * MaxLocalLightCount];


AABB GetClusterAABB(Cluster C)
{
    AABB aabb;
    aabb.MinPoint = C.MinPoint;
    aabb.MaxPoint = C.MaxPoint;

    return aabb;
}

AABB GetBVHAABB(BVH_Node n)
{
    AABB aabb;
    aabb.MinPoint = n.MinPoint;
    aabb.MaxPoint = n.MaxPoint;

    return aabb;
}

AABB GetPointLightAABB(uint lightID)
{
    PointLight pointLight = PointLights[lightID];

    AABB aabb;
    aabb.MinPoint = pointLight.PR.xyz - pointLight.PR.w;
    aabb.MinPoint = pointLight.PR.xyz + pointLight.PR.w;

    return aabb;
}


bool CompareAABBToAABB(AABB A, AABB B)
{
    return  (A.MinPoint.x <= B.MaxPoint.x && B.MinPoint.x <= A.MaxPoint.x) &&
            (A.MinPoint.y <= B.MaxPoint.y && B.MinPoint.y <= A.MaxPoint.y) &&
            (A.MinPoint.z <= B.MaxPoint.z && B.MinPoint.z <= A.MaxPoint.z);
}


bool IsLeafNode(const BVH_Node node)
{
    return node.Leaf == 1;
}


uint GetFirstChild(const BVH_Node node)
{
    return node.Offset;
}

uint GetChildCount(const BVH_Node node)
{
    return node.Count;
}

groupshared uint stack[1024];
groupshared uint stackSize;
groupshared uint parentIdx;

groupshared uint lights[256];
groupshared uint lightCount;
groupshared uint offset;

void InitStack()
{
    stackSize   = 0;
    lightCount  = 0;
    parentIdx   = rootNode;
}

void PushLight(const uint lightID)
{
    uint i;
    InterlockedAdd(lightCount, 1, i);

    lights[i] = lightID;
}

void PushNode(const uint nodeID)
{
    uint i;
    InterlockedAdd(stackSize, 1, i);
    stack[i] = nodeID;
}

uint PopNode()
{
    InterlockedAdd(stackSize, -1);

    return stack[stackSize];
}

[numthreads(NODEMAXSIZE, 1, 1)]
void CreateClustersLightLists(const uint threadID : SV_GroupIndex, const uint3 groupID : SV_GroupID)
{
    const uint groupIdx = groupID.x + groupID.y * 1024;
    if(groupIdx < clusterCount)
    {         
        Cluster localCluster    = Clusters[groupIdx];
        const AABB aabb         = GetClusterAABB(localCluster);
 
        if(threadID == 0)
        {
            InitStack();
            PushNode(rootNode);
        }
        do
        {
            const BVH_Node parent = BVHNodes[parentIdx];
            const uint childCount = GetChildCount(parent);

            if(IsLeafNode(parent))
            {
                const uint idx      = GetFirstChild(parent) + threadID;
                const uint lightID  = LightLookup[idx];
                const AABB b        = GetPointLightAABB(lightID);

                if(threadID < childCount && CompareAABBToAABB(aabb, b))
                    PushLight(lightID);
            }
            else
            { 
                const uint idx          = GetFirstChild(parent) + threadID; 
                const AABB b            = GetBVHAABB(BVHNodes[idx]);

                if(threadID < childCount && CompareAABBToAABB(aabb, b))
                    PushNode(idx);
            }

            if(threadID == 0)
                parentIdx = PopNode();
        }while(parentIdx != rootNode);

        if(threadID == 0)
        {
            InterlockedAdd(counters[0], lightCount, offset);
            
            localCluster.MaxPoint.w = asfloat(lightCount);
            localCluster.MinPoint.w = asfloat(offset);
            Clusters[groupIdx]      = localCluster;
        }

        // Move Light List to global memory
        const uint end = ceil(float(lightCount) / NODEMAXSIZE);
        for(uint I = 0; I < lightCount; I++)
        {
            const uint idx = threadID + (I * NODEMAXSIZE);
            if(idx < lightCount)
                lightList[idx + offset] = lights[idx];
        }
    }
}

/**********************************************************************

Copyright (c) 2021 Robert May

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