cbuffer constants : register(b0)
{
    float4x4    proj;
    float4x4    view;
    float4x4    IView;
    uint        lightCount;
};

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

struct PointLight
{
    float4 KI;	// Color + intensity in W
    float4 PR;	// XYZ + radius in W
};

uint VMain(uint ID : SV_VertexID) : PRIMITIVEID
{
    return ID;
}

StructuredBuffer<BVH_Node> BVHNodes : register(t0); // in-out
StructuredBuffer<Cluster> Clusters : register(t1); // in-out
StructuredBuffer<PointLight> PointLights : register(t2); // in-out

struct vertex
{
    float4 Color    : COLOR;
    float4 position : SV_POSITION;
};

struct primitive
{
    uint ID         : PRIMITIVEID;
};

[maxvertexcount(32)]
void GMain(
    point primitive             nodeID[1], 
	inout LineStream<vertex>    outStream)
{   
    const uint id = nodeID[0].ID;
    BVH_Node node = BVHNodes[id];

    vertex FrontTopLeft;
    vertex FrontTopRight;
    vertex RearTopLeft;
    vertex RearTopRight;

    vertex FrontBottomLeft;
    vertex FrontBottomRight;
    vertex RearBottomLeft;
    vertex RearBottomRight;

    const float4 colors[] = {
        float4(1, 0, 0, 0),
        float4(0, 1, 0, 0),
        float4(0, 0, 1, 0),
        float4(1, 1, 0, 0),
        float4(0, 1, 1, 0),
        float4(1, 0, 1, 0),
    };

    const float4 color = colors[id % 6];

    FrontTopLeft.Color  = color;
    FrontTopRight.Color = color;
    RearTopLeft.Color   = color;
    RearTopRight.Color  = color;

    FrontBottomLeft.Color   = color;
    FrontBottomRight.Color  = color;
    RearBottomLeft.Color    = color;
    RearBottomRight.Color   = color;

    const float3 Min = float3(node.MinPoint.xyz);
    const float3 Max = float3(node.MaxPoint.xyz);

    FrontTopLeft.position   = mul(proj, float4(Min.x, Max.yz, 1));
    FrontTopRight.position  = mul(proj, float4(Max.x, Max.y, Max.z, 1));
    RearTopLeft.position    = mul(proj, float4(Min.x, Max.y, Min.z, 1));
    RearTopRight.position   = mul(proj, float4(Max.x, Max.y, Min.z, 1));

    FrontBottomLeft.position    = mul(proj, float4(Min.xy, Max.z, 1)); 
    FrontBottomRight.position   = mul(proj, float4(Max.x, Min.y, Max.z, 1));
    RearBottomLeft.position     = mul(proj, float4(Min, 1));  
    RearBottomRight.position    = mul(proj, float4(Max.x, Min.yz, 1)); 

    // Front
    outStream.Append(FrontTopLeft);
    outStream.Append(FrontTopRight);
	outStream.RestartStrip();

    outStream.Append(FrontBottomLeft);
    outStream.Append(FrontBottomRight);
	outStream.RestartStrip();

    outStream.Append(FrontTopLeft);
    outStream.Append(FrontBottomLeft);
	outStream.RestartStrip();

    outStream.Append(FrontTopRight);
    outStream.Append(FrontBottomRight);
	outStream.RestartStrip();

    // Back
    outStream.Append(RearTopLeft);
    outStream.Append(RearTopRight);
	outStream.RestartStrip();

    outStream.Append(RearBottomLeft);
    outStream.Append(RearBottomRight);
	outStream.RestartStrip();

    outStream.Append(RearTopLeft);
    outStream.Append(RearBottomLeft);
	outStream.RestartStrip();

    outStream.Append(RearTopRight);
    outStream.Append(RearBottomRight);
	outStream.RestartStrip();

    // Sides
    outStream.Append(FrontTopLeft);
    outStream.Append(RearTopLeft);
	outStream.RestartStrip();

    outStream.Append(FrontTopRight);
    outStream.Append(RearTopRight);
	outStream.RestartStrip();

    outStream.Append(FrontBottomLeft);
    outStream.Append(RearBottomLeft);
	outStream.RestartStrip();

    outStream.Append(FrontBottomRight);
    outStream.Append(RearBottomRight);
	outStream.RestartStrip();
}

[maxvertexcount(32)]
void GMain2(
    point primitive             nodeID[1], 
	inout LineStream<vertex>    outStream)
{   
    const uint id = nodeID[0].ID;
    Cluster cluster = Clusters[id];
    const uint lightCount = asuint(cluster.MaxPoint.w);
    if(lightCount == 0)
        return;

    vertex FrontTopLeft;
    vertex FrontTopRight;
    vertex RearTopLeft;
    vertex RearTopRight;

    vertex FrontBottomLeft;
    vertex FrontBottomRight;
    vertex RearBottomLeft;
    vertex RearBottomRight;

    const float4 colors[] = {
        float4(1, 0, 0, 0),
        float4(0, 1, 0, 0),
        float4(0, 0, 1, 0),
        float4(1, 1, 0, 0),
        float4(0, 1, 1, 0),
        float4(1, 0, 1, 0),
    };

    const float4 color = colors[lightCount % 6];

    FrontTopLeft.Color  = color;
    FrontTopRight.Color = color;
    RearTopLeft.Color   = color;
    RearTopRight.Color  = color;

    FrontBottomLeft.Color   = color;
    FrontBottomRight.Color  = color;
    RearBottomLeft.Color    = color;
    RearBottomRight.Color   = color;

    const float3 Min = float3(cluster.MinPoint.xyz);
    const float3 Max = float3(cluster.MaxPoint.xyz);

    FrontTopLeft.position   = mul(proj, float4(Min.x, Max.yz, 1));
    FrontTopRight.position  = mul(proj, float4(Max.x, Max.y, Max.z, 1));
    RearTopLeft.position    = mul(proj, float4(Min.x, Max.y, Min.z, 1));
    RearTopRight.position   = mul(proj, float4(Max.x, Max.y, Min.z, 1));

    FrontBottomLeft.position    = mul(proj, float4(Min.xy, Max.z, 1)); 
    FrontBottomRight.position   = mul(proj, float4(Max.x, Min.y, Max.z, 1));
    RearBottomLeft.position     = mul(proj, float4(Min, 1));  
    RearBottomRight.position    = mul(proj, float4(Max.x, Min.yz, 1)); 

    // Front
    outStream.Append(FrontTopLeft);
    outStream.Append(FrontTopRight);
	outStream.RestartStrip();

    outStream.Append(FrontBottomLeft);
    outStream.Append(FrontBottomRight);
	outStream.RestartStrip();

    outStream.Append(FrontTopLeft);
    outStream.Append(FrontBottomLeft);
	outStream.RestartStrip();

    outStream.Append(FrontTopRight);
    outStream.Append(FrontBottomRight);
	outStream.RestartStrip();
    // Back
    outStream.Append(RearTopLeft);
    outStream.Append(RearTopRight);
	outStream.RestartStrip();

    outStream.Append(RearBottomLeft);
    outStream.Append(RearBottomRight);
	outStream.RestartStrip();

    outStream.Append(RearTopLeft);
    outStream.Append(RearBottomLeft);
	outStream.RestartStrip();

    outStream.Append(RearTopRight);
    outStream.Append(RearBottomRight);
	outStream.RestartStrip();
    // Sides
    outStream.Append(FrontTopLeft);
    outStream.Append(RearTopLeft);
	outStream.RestartStrip();

    outStream.Append(FrontTopRight);
    outStream.Append(RearTopRight);
	outStream.RestartStrip();

    outStream.Append(FrontBottomLeft);
    outStream.Append(RearBottomLeft);
	outStream.RestartStrip();

    outStream.Append(FrontBottomRight);
    outStream.Append(RearBottomRight);
	outStream.RestartStrip();
}

[maxvertexcount(32)]
void GMain3(
    point primitive             nodeID[1], 
	inout LineStream<vertex>    outStream)
{   
    const uint id           = nodeID[0].ID;
    PointLight pointLight   = PointLights[id];

    vertex FrontTopLeft;
    vertex FrontTopRight;
    vertex RearTopLeft;
    vertex RearTopRight;

    vertex FrontBottomLeft;
    vertex FrontBottomRight;
    vertex RearBottomLeft;
    vertex RearBottomRight;

    const float4 color = float4(1, 1, 1, 1);

    FrontTopLeft.Color  = color;
    FrontTopRight.Color = color;
    RearTopLeft.Color   = color;
    RearTopRight.Color  = color;

    FrontBottomLeft.Color   = color;
    FrontBottomRight.Color  = color;
    RearBottomLeft.Color    = color;
    RearBottomRight.Color   = color;

    const float3 Min = float3(pointLight.PR.xyz - pointLight.PR.w);
    const float3 Max = float3(pointLight.PR.xyz + pointLight.PR.w);

    FrontTopLeft.position   = mul(proj, mul(view, float4(Min.x, Max.yz, 1)));
    FrontTopRight.position  = mul(proj, mul(view, float4(Max.x, Max.y, Max.z, 1)));
    RearTopLeft.position    = mul(proj, mul(view, float4(Min.x, Max.y, Min.z, 1)));
    RearTopRight.position   = mul(proj, mul(view, float4(Max.x, Max.y, Min.z, 1)));

    FrontBottomLeft.position    = mul(proj, mul(view, float4(Min.xy, Max.z, 1))); 
    FrontBottomRight.position   = mul(proj, mul(view, float4(Max.x, Min.y, Max.z, 1)));
    RearBottomLeft.position     = mul(proj, mul(view, float4(Min, 1)));  
    RearBottomRight.position    = mul(proj, mul(view, float4(Max.x, Min.yz, 1))); 

    // Front
    outStream.Append(FrontTopLeft);
    outStream.Append(FrontTopRight);
	outStream.RestartStrip();

    outStream.Append(FrontBottomLeft);
    outStream.Append(FrontBottomRight);
	outStream.RestartStrip();

    outStream.Append(FrontTopLeft);
    outStream.Append(FrontBottomLeft);
	outStream.RestartStrip();

    outStream.Append(FrontTopRight);
    outStream.Append(FrontBottomRight);
	outStream.RestartStrip();

    // Back
    outStream.Append(RearTopLeft);
    outStream.Append(RearTopRight);
	outStream.RestartStrip();

    outStream.Append(RearBottomLeft);
    outStream.Append(RearBottomRight);
	outStream.RestartStrip();

    outStream.Append(RearTopLeft);
    outStream.Append(RearBottomLeft);
	outStream.RestartStrip();

    outStream.Append(RearTopRight);
    outStream.Append(RearBottomRight);
	outStream.RestartStrip();

    // Sides
    outStream.Append(FrontTopLeft);
    outStream.Append(RearTopLeft);
	outStream.RestartStrip();

    outStream.Append(FrontTopRight);
    outStream.Append(RearTopRight);
	outStream.RestartStrip();

    outStream.Append(FrontBottomLeft);
    outStream.Append(RearBottomLeft);
	outStream.RestartStrip();

    outStream.Append(FrontBottomRight);
    outStream.Append(RearBottomRight);
	outStream.RestartStrip();
}

float4 PMain(float4 Color  : COLOR) : SV_TARGET
{
    return float4(Color.xyz, 1);
}