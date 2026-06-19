[[fk::BeginRootSignatureDef(id=globalsig)]]
[[fk::DescriptorSet(SRVBuffer(num=1))]]

[[fk::BeginRootSignatureDef(id=localsig)]]

typedef BuiltInTriangleIntersectionAttributes Attributes;


struct [raypayload] Payload
{
	float4 color : read(caller, anyhit, closesthit, miss) : write(caller, anyhit, closesthit, miss);
    int c : read(caller, anyhit, closesthit, miss) : write(caller, anyhit, closesthit, miss);
};


[shader("anyhit")]
void anyhit_main(inout Payload payload, in Attributes attr)
{
	payload.color = float4(attr.barycentrics, 0, 0);
}


[shader("closesthit")]
void closesthit_main(inout Payload payload, in Attributes attr)
{
	payload.color = float4(attr.barycentrics, 1.0f - attr.barycentrics.x - attr.barycentrics.y, 0);
}


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

RWTexture2D<float4> rgba_out : register(u0);
RaytracingAccelerationStructure tlas : register(t1);

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
        1,
        1,
        0,
        myRay,
        payload);

	rgba_out[id.xy] = pow(float4(payload.color), 2.1f);
}

TriangleHitGroup defaultHitGroup =
{
    "anyhit_main",
    "closesthit_main"
};

LocalRootSignature MyLocalRootSignature =
{
	"RootConstants(num32BitConstants = 16, b0, space = 1)"  // Cube constants 
};

GlobalRootSignature GlobalRoot =
{
	"RootConstants(num32BitConstants = 16, b0, space = 0),"  
    "DescriptorTable(UAV(u0, numDescriptors=1), SRV(t1, numDescriptors=1))"
};

SubobjectToExportsAssociation localRootAssocation =
{
	"MyLocalRootSignature", 
    "miss_main"
};

SubobjectToExportsAssociation globalRootAssocation =
{
	"GlobalRoot", 
    "raygen_main"
};
