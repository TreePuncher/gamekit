#include "common.hlsl"

struct MyMassiveLoad
{
	bool	hit;
	float4	color;
};

typedef BuiltInTriangleIntersectionAttributes MyAttributes;

struct MyParams
{
	float anotherThing;
};

[shader("anyhit")]
void AnyHit(inout MyMassiveLoad payload, in MyAttributes attr)
{
	payload.color		= float4(1, 0, 1, 0);
	payload.hit			= true;
	
	AcceptHitAndEndSearch();
}

[shader("miss")]
void Miss(inout MyMassiveLoad payload)
{
	payload.color	 = float4(1, 1, 0, 1);
}

[shader("closesthit")]
void ClosestHit1(inout MyMassiveLoad payload, in MyAttributes attr)
{
	float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
	payload.color		= float4(barycentrics, 1) * float4(1, 0, 1, 1);
	payload.hit			= true;
}

[shader("closesthit")]
void ClosestHit2(inout MyMassiveLoad payload, in MyAttributes attr)
{
	float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
	payload.color = float4(0, 1, 0, 1) * float4(barycentrics, 1);
	payload.hit = true;
}

RaytracingAccelerationStructure	accelerationStructure	: register(t0);
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
	ray.TMin		= 0.000f;
	ray.TMax		= 200.0f;
	ray.Direction	= dir;
	
	MyMassiveLoad payload;
	payload.color	= 0.0f;
	payload.hit		= false;
	
	TraceRay(
		accelerationStructure,
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
		0x01, 0, 1, 0, ray,
		payload);

	target[uint2(DispatchRaysIndex().xy)] = payload.color;
}
