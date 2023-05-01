RWTexture2D<float4>             target                  : register(u1);
Texture2D<float>                depth                   : register(t1);
RaytracingAccelerationStructure myAccelerationStructure : register(t0);

#include "common.hlsl"

[numthreads(32, 32, 1)]
void main(uint3 XYZ : SV_dispatchThreadID)
{
	if (XYZ.x > 1920 && XYZ.y > 1080)
		return;

	RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> q;

	RayDesc desc;

	const float2 UV         = (float2)XYZ.xy / float2(1920, 1080);
	const float3 dir        = normalize(GetViewVector(UV));
	const float3 positionWS = GetWorldSpacePosition(UV, 0);
	
	RayDesc ray;
	ray.Origin		= positionWS;
	ray.TMin		= 0.01f;
	ray.TMax		= 200.0f;
	ray.Direction	= dir;

	q.TraceRayInline(
		myAccelerationStructure,
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, // OR'd with flags above
		1,
		ray);

	q.Proceed();

	if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
	{
		target[XYZ.xy] += float4(0.0f, 1.0f, 0.0f, 0.3f);
	}
	else // COMMITTED_NOTHING
			// From template specialization,
			// COMMITTED_PROCEDURAL_PRIMITIVE can't happen.
	{
		target[XYZ.xy] += float4(1.0f, 0.0f, 0.0f, 0.3f);
	}
}
