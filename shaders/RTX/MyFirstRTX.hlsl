#include "common.hlsl"

struct Light
{
	float4 KI; // Color + intensity in W
	float4 PR; // XYZ + radius in W
	float4 DS; // Direction + Spread + Type
	uint4 TypeExtra;
};

struct RayPayload
{
	bool	hit;
	float4	color;
};

typedef BuiltInTriangleIntersectionAttributes MyAttributes;

RaytracingAccelerationStructure	accelerationStructure	: register(t0);

Texture2D<float>				depth					: register(t1);
Texture2D<float4>				albedoBuffer			: register(t2);
Texture2D<float2>				normalBuffer			: register(t3);
Texture2D<float4>				MRIABuffer				: register(t4); // metallic, roughness, IOR, anisotropic
Texture2D<uint> 				lightIndexes			: register(t5);

StructuredBuffer<Light> 		lights					: register(t6);
StructuredBuffer<uint2> 		lightLists				: register(t7);
StructuredBuffer<uint> 			lightListBuffer			: register(t8);
StructuredBuffer<uint2>			lightBVH				: register(t9);

RWTexture2D<float4>				target					: register(u1);

sampler BiLinear		: register(s0);
sampler NearestPoint	: register(s1);

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
	payload.color	= float4(1, 1, 0, 1);
	payload.hit		= false;
}


StructuredBuffer<float3>	points		: register(t0, space1);
StructuredBuffer<uint32_t>	indices		: register(t1, space1);
StructuredBuffer<float3>	normals		: register(t2, space1);

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
	const float3 positionWS		= p1 * barycentrics.x + p2 * barycentrics.y + p3 * barycentrics.z;

	const float3 n1				= mul(wt, float4(normals[indices[triangleIndex * 3 + 0]], 1));
	const float3 n2				= mul(wt, float4(normals[indices[triangleIndex * 3 + 1]], 1));
	const float3 n3				= mul(wt, float4(normals[indices[triangleIndex * 3 + 2]], 1));
	const float3 n				= n1 * barycentrics.x + n2 * barycentrics.y + n3 * barycentrics.z;
	
	const Light light			= lights[0];
	const float3 Lpos			= light.PR.xyz;
	const float3 L				= normalize(Lpos - positionWS);
	const float  Ld				= length(Lpos - positionWS) + RayTCurrent();

	payload.color = float4(500 * (n / 2.0f + 0.5f) * max(0.0f, dot(L, n)) / (Ld * Ld), RayTCurrent());
	payload.hit		= true;
}

[shader("closesthit")]
void ClosestHit2(inout RayPayload payload, in MyAttributes attr)
{
	const float3 barycentrics	= float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
	const uint triangleIndex	= PrimitiveIndex();
	const float4x3 wt			= ObjectToWorld4x3();
	const float3 p1				= mul(wt, float4(points[indices[triangleIndex * 3 + 0]], 1));
	const float3 p2				= mul(wt, float4(points[indices[triangleIndex * 3 + 1]], 1));
	const float3 p3				= mul(wt, float4(points[indices[triangleIndex * 3 + 2]], 1));
	const float3 positionWS		= p1 * barycentrics.x + p2 * barycentrics.y + p3 * barycentrics.z;

	const float3 n1				= mul(wt, float4(normals[indices[triangleIndex * 3 + 0]], 1));
	const float3 n2				= mul(wt, float4(normals[indices[triangleIndex * 3 + 1]], 1));
	const float3 n3				= mul(wt, float4(normals[indices[triangleIndex * 3 + 2]], 1));
	const float3 n				= n1 * barycentrics.x + n2 * barycentrics.y + n3 * barycentrics.z;
	
	const Light light			= lights[0];
	const float3 Lpos			= light.PR.xyz;
	const float3 L				= normalize(Lpos - positionWS);
	const float  Ld				= length(Lpos - positionWS) + RayTCurrent();

	payload.color = float4(100 * (n / 2.0f + 0.5f) * max(0.0f, dot(L, n)) / (Ld * Ld), RayTCurrent());
	payload.hit		= true;
}

float2 SignNotZero(float2 v)
{
	return float2((v.x >= 0.0) ? +1.0 : -1.0, (v.y >= 0.0) ? +1.0 : -1.0);
}

float3 Decode(float2 e)
{
	float3 v = float3(e.xy, 1.0 - abs(e.x) - abs(e.y));
	if (v.z < 0)
		v.xy = (1.0 - abs(v.yx)) * SignNotZero(v.xy);

	return normalize(v);
}

float3 UnpackNormal(uint2 px)
{
	return Decode(normalBuffer.Load(uint3(px, 0)));
}

[shader("raygeneration")]
void RayGenerator()
{
	const float z = depth.Load(uint3(DispatchRaysIndex().xy, 0), 0);

	if (z == 1.0f)
		return;

	const uint lightListKey		= lightIndexes.Load(uint3(DispatchRaysIndex().xy, 0));
	const uint2 lightList		= lightLists[lightListKey];
	const uint localLightCount	= lightList.x;
	const uint localLightList	= lightList.y;

	if (localLightCount == 0)
		return;
	
	const float2	UV			= (float2)DispatchRaysIndex() / (float2)DispatchRaysDimensions();
	const float3	view_WS		= GetViewVector(UV);
	const float3	positionWS	= GetWorldSpacePosition(UV, z);
	const float3	normal_VS	= normalize(UnpackNormal(DispatchRaysIndex().xy));
	const float3	normal_WS	= normalize(mul(ViewI, normal_VS));

	const float3	albedo		= float4(normal_WS / 2 + 0.5f, 0);
	float3 color = float3(0, 0, 0);

	for (uint I = 0; I < localLightCount; I++)
	{
		const uint lightIdx = lightListBuffer[localLightList + I];
		const Light light	= lights[lightIdx];

		const float3	Lpos	= light.PR.xyz;
		const float3	L		= normalize(Lpos - positionWS);
		const float		Ld		= length(Lpos - positionWS);
		const float		Li		= 10;//lights[0].KI.w / 2;
		const float		Lr		= light.PR.w;
		const float		ld_2	= Ld * Ld;
		const float		La		= (Li / ld_2) * (1 - (pow(Ld, 10) / pow(Lr, 10)));
	
		if (dot(L, normal_WS) < 0)
			continue;

		RayDesc ray;
		ray.Origin		= positionWS + normal_WS * 0.01f;
		ray.TMin		= 0.0f;
		ray.TMax		= length(positionWS - Lpos);
		ray.Direction	= L;

		RayPayload payload;

#if 1
		TraceRay(
			accelerationStructure,
			RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
			0xff, 0, 1, 0, ray,
			payload);
		
		if(!payload.hit)
			color += float4(albedo * La * dot(normal_WS, L), 0);
#endif
#if 1
		ray.TMax		= 30;
		ray.Direction	= view_WS - normal_WS * 2.0f * dot(view_WS, normal_WS);
		
		TraceRay(
			accelerationStructure,
			RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
			0xff, 0, 1, 0, ray,
			payload);

		if(payload.hit)
			color += float4(albedo * payload.color * saturate(dot(normal_WS, ray.Direction)), 0); // (payload.color.w * payload.color.w);
#endif
	}

	if(color.x + color.y + color.z > 0)
		target[uint2(DispatchRaysIndex().xy)] = float4(color, 1);
}
