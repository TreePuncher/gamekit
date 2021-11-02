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


struct PS_output
{
    float4 Color    : SV_TARGET0;
    float Depth     : SV_Depth;
};

cbuffer DebugConstants : register(b1)
{
    uint MipOffset;
}

PS_output VoxelDebug_PS(const float4 pixelPosition : SV_POSITION)
{
    const float2 UV     = float2(pixelPosition.xy / float2(1920, 1080));
    const float3 view   = GetViewVector(UV);

    Ray r;
    r.origin                = CameraPOS + float3(VOLUMESIDE_LENGTH / 2, VOLUMESIDE_LENGTH / 2, VOLUMESIDE_LENGTH / 2); // Get position relative to voxel grid
    r.dir                   = normalize(view);
    RayCastResult result    = RayCastOctree(r, octree, MipOffset);
    OctTreeNode node        = octree[result.node];

    const float3 position = r.origin + r.dir * result.distance;

    if (result.distance < 0.0f)
        discard;


    PS_output Out;
    //Out.Color = float4(position, 1);
    //Out.Color = result.flags == RAYCAST_HIT ? float4(0, 1 * result.iterations / 64.0f, 1 * result.iterations, 1) : float4(1 * result.iterations / 64.0f, 0, 0, 0);
    //Out.Color = result.flags == RAYCAST_HIT ? float4(UnPack4(node.RGBA).xyz * result.iterations / 64.0f, 1) : float4(1 * result.iterations / 64.0f, 0, 0, 0);
    Out.Color = result.flags == RAYCAST_HIT ? float4(dot((2 * UnPack4(node.RGBA).xyz - 1.0f), float3(0, 0.707, 0.707)) * UnPack4(node.RGBA).xyz * result.iterations / 64.0f, 1) : float4(1 * result.iterations / 64.0f, 0, 0, 0);
    //Out.Color = result.flags == RAYCAST_HIT ? float4(UnPack4(node.RGBA).xyz, 1) : float4(1 * result.iterations / 64.0f, 0, 0, 0);
    //Out.Color = result.flags == RAYCAST_HIT ? float4(position / 64, 1) : float4(1 * result.iterations / 64.0f, 0, 0, 0);
    //Out.Depth = result.flags == RAYCAST_HIT ? (result.distance) / 10000.0f : (result.flags == RAYCAST_MISS ? 1.0f : 1.0f);
    //Out.Color = result.flags == RAYCAST_ERROR ? float4(0, 1, 0, 1) : float4(1 * result.iterations / 64.0f, 0, 0, 0);
    Out.Depth = result.flags == RAYCAST_HIT ? 0.0f : 1.0f;

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
