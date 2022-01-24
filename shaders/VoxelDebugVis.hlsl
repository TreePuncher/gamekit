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

StructuredBuffer<OctTreeNode>   octree  : register(t0);
Texture2D<float>                depth   : register(t1);
Texture2D<float4>               normals : register(t2);
Texture2D<float4>               albedo  : register(t3);

sampler BiLinear     : register(s0);
sampler NearestPoint : register(s1);

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

uint GetSliceIdx(const float z)
{
    const float numSlices   = 7;
    const float MinOverMax  = 100.0f / 0.1f;
    const float LogMoM      = log(MinOverMax);

    return (log(z) * numSlices / LogMoM) - (numSlices * log(100.0f) / LogMoM);
}


PS_output VoxelDebug_PS(const float4 pixelPosition : SV_POSITION)
{
    const float2 UV         = float2(pixelPosition.xy / float2(1920, 1080));
    const float3 view       = GetViewVector(UV);
    const float  Z          = depth.Sample(NearestPoint, UV);
    const float  D          = Z * MaxZ;
    const float3 N          = mul(ViewI, float4(normals.Sample(NearestPoint, UV).xyz, 0));
    const float3 A          = albedo.Sample(NearestPoint, UV).xyz;
    const float3 positionWS = GetWorldSpacePosition(UV, Z);


    if (UV.x > 1.0f)
        discard;

    //if (Z == 1.0f)
    //    discard;

    //if (dot(N, float3(0, 1, 0)) > 0.3f)
    //    discard;

    Ray r;
    //r.origin                = positionWS + N + float3(VOLUMESIDE_LENGTH / 2, VOLUMESIDE_LENGTH / 2, VOLUMESIDE_LENGTH / 2); // Get position relative to voxel grid
    //r.dir                   = reflect(view, N);

    r.origin    = CameraPOS.xyz + float3(VOLUMESIDE_LENGTH / 2, VOLUMESIDE_LENGTH / 2, VOLUMESIDE_LENGTH / 2);
    r.dir       = view;

    OctTreeNode     node;
    RayCastResult   result;

    static const float LODs[] = {
        1, 2, 2, 3, 4, 5, 6, 7
    };

    static const float ranges[] = {
        5, 100, 200, 400, 1000, 5000, 10000
    };


    result = RayCastOctree(r, octree, 1, 1500);

    if (result.flags != RAYCAST_HIT)
        discard;

    const float3 position = r.dir * result.distance - float3(VOLUMESIDE_LENGTH / 2, VOLUMESIDE_LENGTH / 2, VOLUMESIDE_LENGTH / 2);
    node = octree[result.node];
    if (result.flags == RAYCAST_MISS)
        discard;

    const float3 L = normalize(float3(0.0f, 1.0f, 0.0f));
    PS_output Out;
    //Out.Color = float4(position, 1);
    //Out.Color = result.flags == RAYCAST_HIT ? float4(0, 1 * result.iterations / 64.0f, 1 * result.iterations, 1) : float4(1 * result.iterations / 64.0f, 0, 0, 0);
    //Out.Color = result.flags == RAYCAST_HIT ? float4(UnPack4(node.RGBA).xyz * result.iterations / 64.0f, 1) : float4(1 * result.iterations / 64.0f, 0, 0, 0);
    //Out.Color = result.flags == RAYCAST_HIT ? float4(abs(2 * UnPack4(node.RGBA).xyz - 1.0f), 1) : float4(1 * result.iterations / 64.0f, 0, 0, 0);
    //Out.Color = float4(1 * result.iterations / 64.0f, 0, 0, 0);
    Out.Color = float4(1.0f / 125.0f, 1.0f / 125.0f, 1.0f / 125.0f, 0) * result.distance;
    //Out.Color = saturate(1.0f / pow(result.distance, 2)) * float4(0.5f, 0.5f, 0.5f, 1) * saturate(dot((2 * UnPack4(node.RGBA).xyz - 1.0f), L)) * saturate(dot(-view, N));

    //Out.Color = float4(float3(1, 1, 1) * result.iterations / 64, 1);// *max(0.0f, 1 * saturate(1.0f / pow(result.distance, 0)) * (1.0f / pow(D * MaxZ, 0)) * float4(A * saturate(dot(N, -view)) * saturate(dot(-r.dir, 2 * UnPack4(node.RGBA).xyz - 1.0f)), 0));
    //Out.Color = result.flags == RAYCAST_HIT ? float4(UnPack4(node.RGBA).xyz, 1) : float4(1 * result.iterations / 64.0f, 0, 0, 0);
    //Out.Color = result.flags == RAYCAST_HIT ? float4(position / 64, 1) : float4(1 * result.iterations / 64.0f, 0, 0, 0);
    //Out.Depth = result.flags == RAYCAST_HIT ? (result.distance + 0.001f) / 10000.0f : (result.flags == RAYCAST_MISS ? 1.0f : 1.0f);
    //Out.Color = result.flags == RAYCAST_ERROR ? float4(0, 1, 0, 1) : float4(1 * result.iterations / 64.0f, 0, 0, 0);
    //Out.Color = float4(abs(position), 1);
    //Out.Depth = result.flags == RAYCAST_HIT ? 0.0f : 1.0f;

    //Out.Color = float4(1, 1, 1, 1);
    Out.Depth = 1.0f;
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
