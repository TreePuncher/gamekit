[[fk::BeginRootSignatureDef(id=globalsig)]]
[[fk::DescriptorSet(SRVBuffer(num=1))]]

[[fk::BeginRootSignatureDef(id=localsig)]]

typedef BuiltInTriangleIntersectionAttributes Attributes;

struct RNGState
{
	uint a;
};

struct [raypayload] Payload
{
	float4 color : read(caller, anyhit, closesthit, miss) : write(caller, anyhit, closesthit, miss);
    RNGState rng : read(caller, anyhit, closesthit, miss) : write(caller, anyhit, closesthit, miss);
    uint c: read(caller, anyhit, closesthit, miss) : write(caller, anyhit, closesthit, miss);
};


struct Reservoir
{
	uint sample;
	float w;
	uint m;
};

float Hash(in float x)
{
	uint2 n = uint(x) * uint2(1597334673U, 3812015801U);
	uint q = (n.x ^ n.y) * 1597334673U;
	return float(q) * (1.0 / float(0xFFFFFFFFU));
}

float FRand(inout RNGState rng) // (0 -> 1)
{
    // Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs"
	uint x = rng.a;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	rng.a = x;

	uint Mantissa = rng.a >> 9;

	return asfloat(asuint(1.0f) | Mantissa) - 1.0f;
}

int Rand(inout RNGState rng) // (0 -> 1)
{
    // Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs"
	uint x = rng.a;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	rng.a = x;

	return rng.a;
}

#define SEED_SCALE 1000000
float RandUniform(float seed)
{
	return Hash(seed);
}

float rand_1_05(in float2 uv)
{
	float2 noise = (frac(sin(dot(uv, float2(12.9898, 78.233) * 2.0)) * 43758.5453));
	return abs(noise.x + noise.y) * 0.5;
}

float RandFloatInRange(const float lowerBound, const float upperBound, float2 r)
{
	return lowerBound + (rand_1_05(r) % (upperBound - lowerBound));
}

float RandUniform(float2 r)
{
	return RandFloatInRange(0, 1.0f, r);
}

#define PI 3.1415926535897932

float2 VectorToSphere(float3 XYZ)
{
	const bool t1 = abs(XYZ.x) > 0.0f;
	const bool t2 = (abs(XYZ.z) > 0.0f);

	const float theta	= acos(XYZ.y);
	const float phi =
		(t1 ? atan(XYZ.z / XYZ.x) :
		(t2 ? sign(XYZ.z) * PI / 2.0f : 0.0f));

	return float2(theta, phi);
}

float3 SphereToVector(float2 UV)
{
	float x = sin(UV.x) * cos(UV.y);
    float y = cos(UV.x);
	float z = sin(UV.x) * sin(UV.y);

	return float3(x, y, z);
}

float3 AxisAngle(float3 e, float theta, float3 v)
{
	return (cos(theta) * v + sin(theta) * (cross(e, v)) + (1.0f - cos(theta)) * dot(v, e) * e) * theta;
}

bool IsNaN(float x)
{
	return (asuint(x) & 0x7fffffff) > 0x7f800000;
}

float3 random_cosine_direction(inout RNGState rng)
{
	float r1 = FRand(rng);
	float r2 = FRand(rng);
	float phi = 2.0f * PI * r2;
 
	return SphereToVector(float2(r1, phi));
}

RWTexture2D<float4>				rgba_out	: register(u0);
RaytracingAccelerationStructure tlas		: register(t1);

/************************************************************************************************/

LocalRootSignature LightInterface =
{
	"RootConstants(num32BitConstants = 16, b0, space = 3)"
};

cbuffer lightConstants : register(b0, space3)
{
	float4 rgbi;
};


[shader("anyhit")]
void anyhit_light(inout Payload payload, in Attributes attr)
{
	payload.color = float4(1, 0, 0, 0);
}

[shader("closesthit")]
void closesthit_light(inout Payload payload, in Attributes attr)
{
	const float t = RayTCurrent();
	const float3 l = -WorldRayDirection();
	const float a = 1.0f / saturate(t * t);

	payload.color = float4(1, 1, 1, 1) * a;
}

TriangleHitGroup LightMaterial =
{
	"anyhit_light",
    "closesthit_light"
};

SubobjectToExportsAssociation DefaultLightInterfaceAssociation =
{
	"LightInterface",
    "anyhit_light;closesthit_light"
};


/************************************************************************************************/


LocalRootSignature DefaultMaterialInterface =
{
	"RootConstants(num32BitConstants = 16, b0, space = 2),"
    "SRV(t0, space = 2),"	// index
    "SRV(t1, space = 2),"	// position
    "SRV(t2, space = 2)"	// normal
};

cbuffer materialConstants : register(b0, space2)
{
	float4 diffuse;
};

StructuredBuffer<uint>		indexBuffer		: register(t0, space2);
StructuredBuffer<float3>	positionBuffer	: register(t1, space2);
StructuredBuffer<float3>	normalBuffer	: register(t2, space2);

[shader("anyhit")]
void anyhit_material(inout Payload payload, in Attributes attr)
{
	payload.color = float4(attr.barycentrics, 0, 0);
}

[shader("closesthit")]
void closesthit_material(inout Payload out_payload, in Attributes attr)
{
	if (out_payload.c > 1)
		return;

	const float t = RayTCurrent();
	const float a = lerp(0, 1, t * t) / (t * t);

	if (a < 0.05)
		return;

	const uint idxBegin = PrimitiveIndex() * 3;
	const uint i0 = indexBuffer[idxBegin + 0];
	const uint i1 = indexBuffer[idxBegin + 1];
	const uint i2 = indexBuffer[idxBegin + 2];

	const float3 v0 = positionBuffer[i0];
	const float3 v1 = positionBuffer[i1];
	const float3 v2 = positionBuffer[i2];

	const float3 n0 = normalBuffer[i0];

	const float3 pos_w =
	        v1 * attr.barycentrics.x +
	        v2 * attr.barycentrics.y +
	        v0 * (1.0f - (attr.barycentrics.x + attr.barycentrics.y));

	const float3 n_w	= n0;
	const float3 t_w	= normalize(v0 - v1); // tangent
	const float3 b_w	= normalize(cross(t_w, n_w)); // bitTangent
	float3x3 tbn = float3x3(t_w, b_w, n_w);

	const float3 r	= -WorldRayDirection();

	Payload payload;
	payload.color	= float4(0, 0, 0, 0);
	payload.rng.a	= Rand(out_payload.rng);
	payload.c		= out_payload.c + 1;

	float3 color = float3(1, 1, 1);
    if (dot(n_w, float3(0, -1, 0)) > 0.9f)
		color = float3(0, 1, 0);;
    if (dot(n_w, float3(0, 0, -1)) > 0.9f)
		color = float3(1, 0, 0);
	else if (dot(n_w, float3(0, 0,  1)) > 0.9f)
		color = float3(0, 0, 1);

	float4 c = float4(0, 0, 0, 0);

    const int numSamples = 5;
	for (int i = 0; i < numSamples; i++)
	{
		const float3 V_cw = random_cosine_direction(out_payload.rng);
		float3 sample_dir = mul(V_cw.xzy, tbn);
		
		RayDesc myRay;
		myRay.Origin	= pos_w;
		myRay.TMin		= 0.00001f;
		myRay.TMax		= 10.0f;
		myRay.Direction = sample_dir;

		payload.color = float4(0, 0, 0, 0);

		TraceRay(
            tlas,
            0,
            0x00000003,
            0,
            1,
            0,
            myRay,
            payload);

		c += payload.color * float4(color, 0.0f)  / (numSamples * PI);
	}

	out_payload.color = c * saturate(dot(n_w, r));

}

TriangleHitGroup DefaultMaterial =
{
	"anyhit_material",
    "closesthit_material"
};

SubobjectToExportsAssociation DefaultMaterialInterfaceAssociation =
{
	"DefaultMaterialInterface",
    "DefaultMaterial"
};


/************************************************************************************************/


LocalRootSignature MissInterface =
{
	"RootConstants(num32BitConstants = 16, b0, space = 1)"
};

cbuffer localconstants : register(b0, space1)
{
    float4 rgb;
};


[shader("miss")]
void miss_main(inout Payload payload)
{
	payload.color = float4(1, 1, 1, 0);
}

SubobjectToExportsAssociation MissInterfaceAssociation =
{
	"MissInterface",
    "miss_main"
};


/************************************************************************************************/


void UpdateReservoir(inout Reservoir r, uint E_i, float W_i, inout RNGState rng)
{
	r.m++;

	r.w += W_i;
	const float w_r = FRand(rng);
    if (w_r < W_i / r.w)
		r.sample = E_i;
}

GlobalRootSignature GlobalInterface =
{
	"RootConstants(num32BitConstants = 16, b0, space = 0),"  
    "DescriptorTable(UAV(u0, numDescriptors=1), SRV(t1, numDescriptors=1))"
};


//RWStructuredBuffer<Reservoir>	reservoirBuffer	: register(u1);
//RWStructuredBuffer<RNGState>	RNGStates		: register(u2);

cbuffer raygenerationconstants : register(b0, space4294967040)
{
	float4x4	PVI;
	float3		cameraPos;
	uint		seed;
};

[shader("raygeneration")]
void raygen_main()
{
	const uint3 id			= DispatchRaysIndex();
	const float2 pixelWH	= 1.0f / float2(DispatchRaysDimensions().xy);
	const float2 UV			= float2(id.xy) / float2(DispatchRaysDimensions().xy) + pixelWH * 0.5f;
	
	float3 topLeft		= mul(PVI, float4(-1.0f, 1.0f, 1.0f, 1.0f)).xyz;
	float3 topRight		= mul(PVI, float4( 1.0f, 1.0f, 1.0f, 1.0f)).xyz;
	float3 btmLeft		= mul(PVI, float4(-1.0f,-1.0f, 1.0f, 1.0f)).xyz;
	float3 btmRight		= mul(PVI, float4( 1.0f, -1.0f, 1.0f, 1.0f)).xyz;

	RayDesc myRay;
	myRay.Origin	= cameraPos;
	myRay.TMin		= 0;
	myRay.TMax		= 20;
	myRay.Direction = normalize(lerp(
	                    lerp(topLeft, topRight, UV.x),
	                    lerp(btmLeft, btmRight, UV.x), UV.y));

	Payload payload;
	payload.color	= float4(0.0f, 0.0f, 0.0f, 0.0f);
	payload.rng.a	= asuint(asfloat(id.x * id.y)) ^ seed;
	payload.c		= 0;

	TraceRay(
        tlas,
        0,
        0x00000003,
        0,
        1,
        0,
        myRay,
        payload);

	rgba_out[id.xy] = payload.color;
}

