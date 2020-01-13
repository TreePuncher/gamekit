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

float3 SampleCosWeighted(in float U1, in float U2)
{
    float r = sqrt(U1);
    float theta = PI * 2.0 * U2;

    float x = r * cos(theta);
    float y = r * sin(theta);

    return float3(x, y, sqrt(max(0.0, 1.0 - U1)));
}

float CantorPairing(in float a, in float b)
{
    return 0.5 * (a + b) * (a + b + 1.0) + b;
}

#define UI0 1597334673U
#define UI1 3812015801U
#define UI2 uint2(UI0, UI1)
#define UI3 uint3(UI0, UI1, 2798796415U)
#define UIF (1.0 / float(0xffffffffU))


float Hash(in float2 p)
{
    uint2 q = uint2(int2(p)) * UI2;
    uint n = (q.x ^ q.y) * UI0;
    return float(n) * UIF;
}

float Hash(in uint q)
{
    uint2 n = q * UI2;
    q = (n.x ^ n.y) * UI0;
    return float(q) * UIF;
}


float Rand(inout uint seed)
{
    return Hash( seed++ );
}

static const uint SAMPLE_COUNT = 64;


float HaltonSequence(in float i, in float b)
{
    float f = 1.0;
    float r = 0.0;

    while (i > 0.0)
    {
        f = f / b;
        r = r + (f * fmod(i, b));
        i = floor(i / b);
    }

    return r;
}

static const float2 INV_TAN2 = float2(0.1591, 0.3183);
float2 SampleSphericalMap(in float3 w)
{
    float U = 0.5 + atan2(w.x, -w.z) / PI_2;
    float V = acos(w.y / length(w)) / PI;
    return float2(U, V);
    /*
    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    uv *= INV_TAN2;
    uv += 0.5;
    return uv;*/
}

float4 texture2CubeMapDiffuse_PS(Vertex IN) : SV_TARGET
{
    float3 localPos = IN.worldPos;
    
    // the sample direction equals the hemisphere's orientation 
    float3 normal = normalize(localPos);


    float3 irradiance = float3(0.0, 0.0, 0.0);
    float3 up    = float3(0.0, 1.0, 0.0);
    float3 right = cross(up, normal);
    up           = cross(normal, right);

    float sampleDelta = 0.025;
    float nrSamples = 0.0; 
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            // spherical to cartesian (in tangent space)
            float3 tangentSample = float3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal; 
            float2 sampleUV = SampleSphericalMap(sampleVec);
            irradiance += SourceMap.Sample(bilinear, sampleUV).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }

    irradiance = PI * irradiance * (1.0 / float(nrSamples));

    return float4(irradiance, 1);
    /*
    float3 pos = IN.pos;
    float3 wi = normalize(IN.worldPos);
    float2 p = (pos.xy * 0.5) + 0.5;

    float3 N = wi;
    float3 T = normalize(cross(N, N.zxy));
    float3 B = normalize(cross(N, T));
    float3x3 TBN = float3x3(T, B, N);

    float W = 512;
    float seed = Hash(p.y * W * W + p.x * W) * W * W * SAMPLE_COUNT * 2;

    float3 irradiance = float3(0,0,0);

    for (uint i = 0; i < SAMPLE_COUNT; ++i)
    {
        float index = seed + float(i * 2);
        float U1 = Hash(index);
        float U2 = Hash(index + 1);

        float3 ws = SampleCosWeighted(U1, U2);
        float3 sampleDir = mul(TBN, ws);
        float2 sampleUV = SampleSphericalMap(sampleDir);

        float3 sampleIrradiance = SourceMap.Sample(bilinear, sampleUV);

        irradiance += sampleIrradiance;
    }

    irradiance /= SAMPLE_COUNT;
*/
}


float4 texture2CubeMapGGX_PS(Vertex IN) : SV_TARGET
{
	float3 localPos = IN.worldPos;
    
    // the sample direction equals the hemisphere's orientation 
    float3 normal = normalize(localPos);

	float2 UV = SampleSphericalMap(normal);
    return SourceMap.Sample(bilinear, UV);
    //return float4(0.5, 0.5, 0.5, 0.5);
}
