struct BVH_Node
{
    float4 MinPoint;
    float4 MaxPoint;

    uint Offset;
    uint Count; // max child count is 16

    uint Leaf;
};

cbuffer constants : register(b0)
{
    float4x4    iproj;
    float4x4    view;
    uint2       LightMapWidthHeight;
    uint        lightCount;
};

AppendStructuredBuffer<BVH_Node> BVHNodes    : register(u0); // in-out