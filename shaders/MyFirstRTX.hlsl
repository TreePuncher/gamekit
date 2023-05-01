//#include "common.hlsl"

struct MyMassiveLoad
{
	float4 shadowFactor;
};

typedef BuiltInTriangleIntersectionAttributes MyAttributes;

struct MyParams
{
	float anotherThing;
};

[shader("anyhit")]
void anyhit_main(inout MyMassiveLoad payload, in MyAttributes attr)
{
	//float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);

	//payload.alsoMyThing = float4(1, 0, 1, 0);// RayTCurrent() / 100.0f;
	//AcceptHitAndEndSearch();

	//float3 hitLocation = ObjectRayOrigin() + ObjectRayDirection() * RayTCurrent();
}

[shader("miss")]
void miss(inout MyMassiveLoad payload)
{
	//payload.shadowFactor += float4(1, 1, 1, 1);
}

[shader("closesthit")]
void closestHit(inout MyMassiveLoad payload, in MyAttributes attr)
{
	//float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
	//payload.alsoMyThing = float4(barycentrics, 1);
	//payload.alsoMyThing = float4((ObjectRayOrigin() + ObjectRayDirection() * RayTCurrent()) / 25.0f, 1);
}

RaytracingAccelerationStructure MyAccelerationStructure : register(t0);
Texture2D<float>                depth                   : register(t1);
Texture2D<float4>               albedo                  : register(t2);
RWTexture2D<float4>             target                  : register(u1);

sampler BiLinear     : register(s0);
sampler NearestPoint : register(s1);

[shader("raygeneration")]
void RayGenerator()
{
	//const float2 UV         = (float2)DispatchRaysIndex() / (float2)DispatchRaysDimensions();
	//const float3 dir        = normalize(GetViewVector(UV));
	//const float  Z          = depth.Load(int3(DispatchRaysIndex().xy, 0));
	//const float3 positionWS = GetWorldSpacePosition(UV, Z);
	//
	//if (Z == 1.0f)
	//    return;
	//
	//RayDesc ray;
	//ray.Origin     = positionWS;
	//ray.TMin       = 0.01f;
	//ray.TMax       = 200.0f;
	//ray.Direction = normalize(float3(0, 1, 0));
	//
	//MyMassiveLoad payload;
	//payload.shadowFactor= 0.0f;
	//
	//TraceRay(
	//    MyAccelerationStructure,
	//    0, ~0, 0, 1, 0, ray,
	//    payload);
	//
	//target[uint2(DispatchRaysIndex().xy)] += float4(0.3f, 0.3f, 0.3f, 0.3f);
}
