#include "common.hlsl"

struct RayPayload
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
void AnyHit(inout RayPayload payload, in MyAttributes attr)
{
	payload.color		= float4(1, 0, 1, 0);
	payload.hit			= true;
	
	AcceptHitAndEndSearch();
}

[shader("miss")]
void Miss(inout RayPayload payload)
{
	payload.color	 = float4(1, 1, 0, 1);
}


StructuredBuffer<float3>	points		: register(t0, space1);
StructuredBuffer<uint32_t>	indices		: register(t1, space1);

cbuffer hitArguments : register(b1)
{
	float4	color;
	uint	index;
	uint	unused0;
};

[shader("closesthit")]
void ClosestHit1(inout RayPayload payload, in MyAttributes attr)
{
	const float3 barycentrics	= float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
	const uint triangleIndex	= PrimitiveIndex();
	const float4x3 wt			= ObjectToWorld4x3();
	const float3 p1				= mul(wt, float4(points[indices[triangleIndex * 3 + 0]], 1));
	const float3 p2				= mul(wt, float4(points[indices[triangleIndex * 3 + 1]], 1));
	const float3 p3				= mul(wt, float4(points[indices[triangleIndex * 3 + 2]], 1));

	payload.color	= float4(p1 * barycentrics.x + p2 * barycentrics.y + p3 * barycentrics.z, 1);
	payload.hit		= true;
}

[shader("closesthit")]
void ClosestHit2(inout RayPayload payload, in MyAttributes attr)
{
	float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
	payload.color		= color * float4(barycentrics, 1);
	payload.hit			= true;
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
	
	RayPayload payload;
	payload.color	= 0.0f;
	payload.hit		= false;
	
	TraceRay(
		accelerationStructure,
		RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		0xff, 0, 1, 0, ray,
		payload);

	target[uint2(DispatchRaysIndex().xy)] = payload.color;
}
