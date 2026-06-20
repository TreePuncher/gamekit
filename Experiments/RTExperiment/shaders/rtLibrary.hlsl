[[fk::BeginRootSignatureDef(id=globalsig)]]
[[fk::DescriptorSet(SRVBuffer(num=1))]]

[[fk::BeginRootSignatureDef(id=localsig)]]

typedef BuiltInTriangleIntersectionAttributes Attributes;


struct [raypayload] Payload
{
	float4 color : read(caller, anyhit, closesthit, miss) : write(caller, anyhit, closesthit, miss);
    int c : read(caller, anyhit, closesthit, miss) : write(caller, anyhit, closesthit, miss);
};


struct Reservoir
{
	uint sample;
	float w;
	uint m;
};

struct RNGState
{
	uint a;
};

float FRand(inout RNGState rng) // (0 -> 1)
{
    // Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs"
	uint x = rng.a;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	rng.a = x;

	return 1.0f / float(x % (2 ^ 22 - 1));
}


/************************************************************************************************/

LocalRootSignature LightInterface =
{
	"RootConstants(num32BitConstants = 16, b0, space = 3),"
    "SRV(t0, space = 3)"
};

cbuffer lightConstants : register(b0, space3)
{
	float4 rgbi;
};


[shader("anyhit")]
void anyhit_light(inout Payload payload, in Attributes attr)
{
	payload.color = rgbi;
}

[shader("closesthit")]
void closesthit_light(inout Payload payload, in Attributes attr)
{
	payload.color = float4(1, 1, 1, 0);
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

cbuffer lightConstants : register(b0, space2)
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
void closesthit_material(inout Payload payload, in Attributes attr)
{
	const uint idxBegin = PrimitiveIndex() * 3;
	const uint i0 = indexBuffer[idxBegin + 1];
	const uint i1 = indexBuffer[idxBegin + 2];
	const uint i2 = indexBuffer[idxBegin + 0];

	const float3 v0 = positionBuffer[i0];
	const float3 v1 = positionBuffer[i1];
	const float3 v2 = positionBuffer[i2];

	const float3 n0 = normalBuffer[i0] / 2.0f + 0.5f;
	const float3 n1 = normalBuffer[i1] / 2.0f + 0.5f;
	const float3 n2 = normalBuffer[i2] / 2.0f + 0.5f;

	payload.color =
	    float4(
	        v0 * attr.barycentrics.x +
	        v1 * attr.barycentrics.y +
	        v2 * (1.0f - (attr.barycentrics.x + attr.barycentrics.y)),
	        0.0f);
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
	payload.c += 1;
	payload.color = rgb;
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

RWTexture2D<float4>				rgba_out		: register(u0);
RaytracingAccelerationStructure tlas			: register(t1);
//RWStructuredBuffer<Reservoir>	reservoirBuffer	: register(u1);
//RWStructuredBuffer<RNGState>	RNGStates		: register(u2);

cbuffer raygenerationconstants : register(b0, space4294967040)
{
	float4x4	PVI;
	float3		cameraPos;
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
	payload.color	= float4(float2(id.xy) / DispatchRaysDimensions().xy, 0, 0);
	payload.c		= 0;

	TraceRay(
        tlas,
        0,
        0x000000ff,
        0,
        1,
        0,
        myRay,
        payload);

	rgba_out[id.xy] = payload.color; //pow(float4(payload.color), 2.1f);
}

