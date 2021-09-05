struct OctTreeNode
{
    uint    nodes[8];
    uint4   volumeCord;
    uint    data;
    uint    flags;
};

RWStructuredBuffer<OctTreeNode>    octree  : register(u0);

[numthreads(1, 1, 1)]
void Init(uint3 threadID : SV_DispatchThreadID)
{
    OctTreeNode root;

    for(uint I = 0; I < 8; I++)
        root.nodes[I] = -1;

    root.volumeCord = uint4(0, 0, 0, 0);
    root.data       = -1;
    root.flags      = 1 << 2;

    octree[0] = root;
    octree.IncrementCounter();
}
