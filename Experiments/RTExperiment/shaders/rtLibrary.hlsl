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
	payload.color = float4(0, 1, 0, 0);
}


[shader("closesthit")]
void closesthit_main(inout Payload payload, in Attributes attr)
{
	payload.color = float4(1, 0, 0, 0);
}

LocalRootSignature MyLocalRootSignature =
{
	"RootConstants(num32BitConstants = 4, b1, space = 1234)"  // Cube constants 
};


cbuffer localconstants : register(b1, space1234)
{
    float4 rgb;
};


[shader("miss")]
[localrootsignature(MyLocalRootSignature)]
void miss_main(inout Payload payload)
{
	payload.c += 1;
	payload.color = rgb;
}

RWTexture2D<float4> rgba_out : register(u0);

[[fk::AccelerationStructure(binding=1, set=0, id=AccelerationStructure)]]

[shader("raygeneration")]
void raygen_main()
{
	uint3 id = DispatchRaysIndex();

	RayDesc myRay;
	myRay.Origin = float3(0, 0, 0);
	myRay.TMin = 1;
	myRay.Direction = float3(0, 1, 0);
	myRay.TMax = 10;

	Payload payload;
	payload.color = float4(float2(id.xy) / DispatchRaysDimensions().xy, 0, 0);
	payload.c = 0;

	TraceRay(
        AccelerationStructure,
        0,
        0xffffffff,
        1,
        1,
        0,
        myRay,
        payload);

	rgba_out[id.xy] = float4(payload.color.xyz, 0.0f);
}

TriangleHitGroup defaultHitGroup =
{
    "anyhit_main",
    "closesthit_main"
};

SubobjectToExportsAssociation Name =
{
	"MyLocalRootSignature", "miss_main"
};
