#include "common.hlsl"

struct MyMassiveLoad
{
	float4 alsoMyThing;
};

typedef BuiltInTriangleIntersectionAttributes MyAttributes;

struct MyParams
{
	float anotherThing;
};

[shader("anyhit")]
void AnyHit(inout MyMassiveLoad payload, in MyAttributes attr)
{
	float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);

	payload.alsoMyThing = float4(1, 0, 1, 0);
	AcceptHitAndEndSearch();
}

[shader("miss")]
void Miss(inout MyMassiveLoad payload)
{
	payload.alsoMyThing = float4(0, 0, 0, 1);
}

[shader("closesthit")]
void ClosestHit(inout MyMassiveLoad payload, in MyAttributes attr)
{
	float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
	payload.alsoMyThing = float4(barycentrics, 1);
}

RaytracingAccelerationStructure	MyAccelerationStructure	: register(t0);
Texture2D<float>				depth					: register(t1);
Texture2D<float4>				albedo					: register(t2);
RWTexture2D<float4>				target					: register(u1);

sampler BiLinear		: register(s0);
sampler NearestPoint	: register(s1);

[shader("raygeneration")]
void RayGenerator()
{
	const float2 UV			= (float2)DispatchRaysIndex() / (float2)DispatchRaysDimensions();
	const float3 dir		= GetViewVector(UV);
	const float3 positionWS	= GetWorldSpacePosition(UV, 0);
	
	RayDesc ray;
	ray.Origin		= positionWS;
	ray.TMin		= 0.01f;
	ray.TMax		= 200.0f;
	ray.Direction	= dir;
	
	MyMassiveLoad payload;
	payload.alsoMyThing = 0.0f;
	
	TraceRay(
		MyAccelerationStructure,
		0, ~0, 0, 1, 0, ray,
		payload);
	
	target[uint2(DispatchRaysIndex().xy)] = payload.alsoMyThing;
}
