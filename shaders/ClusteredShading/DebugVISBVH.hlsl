cbuffer constants : register(b0)
{
    float4x4    proj;
    float4x4    view;
}

struct Vertex
{
    float3 Min : MIN;
    float3 Max : MAX;
    float4 Color : COLOR;
};

Vertex VMain(Vertex v)
{
    return v;
}

struct GOut
{
    float4 Color    : COLOR;
    float4 Position : SV_POSITION;
};

[maxvertexcount(24)]
void GMain(point Vertex input[1], inout LineStream<GOut> outStream)
{
    const Vertex BVH    = input[0];

    GOut FrontTopLeft;
    GOut FrontTopRight;
    GOut RearTopLeft;
    GOut RearTopRight;

    GOut FrontBottomLeft;
    GOut FrontBottomRight;
    GOut RearBottomLeft;
    GOut RearBottomRight;

    FrontTopLeft.Color  = BVH.Color;
    FrontTopRight.Color = BVH.Color;
    RearTopLeft.Color   = BVH.Color;
    RearTopRight.Color  = BVH.Color;

    FrontBottomLeft.Color   = BVH.Color;
    FrontBottomRight.Color  = BVH.Color;
    RearBottomLeft.Color    = BVH.Color;
    RearBottomRight.Color   = BVH.Color;

    const float3 Min = float3(BVH.Min.xyz);
    const float3 Max = float3(BVH.Max.xyz);

    FrontTopLeft.Position   = mul(proj, mul(view, float4(Min.x, Max.yz, 1)));
    FrontTopRight.Position  = mul(proj, mul(view, float4(Max.x, Max.y, Max.z, 1)));
    RearTopLeft.Position    = mul(proj, mul(view, float4(Min.x, Max.y, Min.z, 1)));
    RearTopRight.Position   = mul(proj, mul(view, float4(Max.x, Max.y, Min.z, 1)));

    FrontBottomLeft.Position    = mul(proj, mul(view, float4(Min.xy, Max.z, 1))); 
    FrontBottomRight.Position   = mul(proj, mul(view, float4(Max.x, Min.y, Max.z, 1)));
    RearBottomLeft.Position     = mul(proj, mul(view, float4(Min, 1)));  
    RearBottomRight.Position    = mul(proj, mul(view, float4(Max.x, Min.yz, 1))); 

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

float4 PMain(float4 Color : COLOR) : SV_TARGET
{
    return Color;
}