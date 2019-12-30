#define PI 3.14159265359
#define PI_2 (2.0 * PI)

void texture2CubeMap_VS() {}

Texture2D<float4> SourceMap : register(t0);
SamplerState      bilinear  : register(s0);

struct Vertex
{
    float4 pos              : SV_POSITION;
    float3 worldPos         : POSITION;
    uint   arrayTargetIndex : SV_RenderTargetArrayIndex;
};

float2 CalculatePointUV(in float3 w)
{
	w = normalize(w);
	float U = 0.5 + atan2(w.x, -w.z) / PI_2;
	float V = acos(w.y / length(w)) / PI;
	return float2(U, V);
}

// Vertex order is important, it maps to cube map face vertex order
static const float3 planeVerts[6] =
{
    float3(-1,  1, 1),
    float3( 1,  1, 1),
    float3(-1, -1, 1),
    float3(-1, -1, 1),
    float3( 1,  1, 1),
    float3( 1, -1, 1),
};


// Assumes clockwise winding order
static const float3 cubeVerts[36] =
{
	// Face 0 : +X
    float3(1,  1,  1),
    float3(1,  1, -1),
    float3(1, -1,  1),

    float3(1, -1,  1),
    float3(1,  1, -1),
    float3(1, -1, -1),
	
	// Face 1 : -X
    float3(-1,  1, -1),
    float3(-1,  1,  1),
    float3(-1, -1, -1),
    
	float3(-1, -1, -1),
    float3(-1,  1,  1),
    float3(-1, -1,  1),
	
	// Face 2 : +Y
    float3(-1, 1, -1),
    float3( 1, 1, -1),
    float3(-1, 1,  1),
    
	float3(-1, 1,  1),
    float3( 1, 1, -1),
    float3(1, 1,  1),

	// Face 3 : -Y
    float3(-1, -1,  1),
    float3( 1, -1,  1),
    float3(-1, -1, -1),
    
	float3(-1, -1, -1),
    float3( 1, -1,  1),
    float3( 1, -1, -1),

	// Face 4 : +Z
    float3(-1, 1, 1),
    float3(1,  1, 1),
    float3(-1, -1, 1),
    
    float3(-1, -1, 1),
    float3( 1,  1, 1),
    float3( 1, -1, 1),

	// Face 5 : -Z
    float3( 1,  1, -1),
    float3(-1,  1, -1),
    float3( 1, -1, -1),
    
    float3( 1, -1, -1),
    float3(-1,  1, -1),
    float3(-1, -1, -1),
};

struct LULNothing
{

};

[maxvertexcount(36)]
void texture2CubeMap_GS(
    point LULNothing vertices[1],
    inout TriangleStream<Vertex> renderTarget)
{
    Vertex vertex;

    //uint face_ID = 0;
    for (uint face_ID = 0; face_ID < 6; face_ID++) // Loop over faces
    {
        vertex.arrayTargetIndex = face_ID;

        vertex.pos = float4(planeVerts[0], 1);
        vertex.worldPos = cubeVerts[0 + face_ID * 6];
        renderTarget.Append(vertex);
        vertex.pos = float4(planeVerts[1], 1);
        vertex.worldPos = cubeVerts[1 + face_ID * 6];
        renderTarget.Append(vertex);

        vertex.pos = float4(planeVerts[2], 1);
        vertex.worldPos = cubeVerts[2 + face_ID * 6];
        renderTarget.Append(vertex);
        renderTarget.RestartStrip();

        vertex.pos = float4(planeVerts[3], 1);
        vertex.worldPos  = cubeVerts[3 + face_ID * 6];
        renderTarget.Append(vertex);
        vertex.pos = float4(planeVerts[4], 1);
        vertex.worldPos  = cubeVerts[4 + face_ID * 6];
        renderTarget.Append(vertex);

        vertex.pos = float4(planeVerts[5], 1);
        vertex.worldPos = cubeVerts[5 + face_ID * 6];
        renderTarget.Append(vertex);
        renderTarget.RestartStrip();
    }
}

float4 texture2CubeMap_PS(Vertex IN) : SV_TARGET
{
	float2 UV = CalculatePointUV(IN.worldPos);
	return SourceMap.Sample(bilinear, UV);
}
