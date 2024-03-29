#pragma once

/**********************************************************************

Copyright (c) 2014-2022 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/

#include "MathUtils.h"
#include "containers.h"
#include <optional>

namespace FlexKit
{
	/************************************************************************************************/

	typedef float4 BoundingSphere;

	/************************************************************************************************/


	// Axis Aligned Bounding Box
	static const float inf = std::numeric_limits<float>::infinity();


	struct AABB
	{
		enum Axis : size_t
		{
			X, Y, Z, Axis_count
		};

		float3 Min = { inf, inf, inf };
		float3 Max = { -inf, -inf, -inf };

		Axis LongestAxis() const noexcept;

		AABB  operator +	(const AABB& rhs) const noexcept;
		AABB& operator +=	(const AABB& rhs) noexcept;
		AABB& operator +=	(const float3& rhs) noexcept;
		AABB& operator +=	(this AABB& lhs, const BoundingSphere& rhs) noexcept;

		float3 MidPoint() const noexcept;
		float3 Span() const noexcept;
		float3 Dim() const noexcept;

		AABB    Offset(const float3 offset) const noexcept;
		float   AABBArea() const noexcept;

		operator BoundingSphere () const noexcept;
	};


	/************************************************************************************************/


	struct Plane
	{
		float3 n; // normal
		float3 o; // origin
	};


	/************************************************************************************************/


	struct Ray
	{
		float3 D;// Direction
		float3 O;// Origin

		float3 R(float t)           const noexcept { return D * t + O; }
		float3 operator * (float k) const noexcept { return O + D*k; }
	};


	typedef Vector<Ray> RaySet;


	/************************************************************************************************/


	struct Cone
	{
		float3 O;
		float3 D;
		float length;
		float theta;
	};


	/************************************************************************************************/


	enum EPlane
	{
		EPlane_FAR		= 0,
		EPlane_NEAR		= 1,
		EPlane_TOP		= 2, 
		EPlane_BOTTOM	= 3, 
		EPlane_LEFT		= 4, 
		EPlane_RIGHT	= 5,
	};


	struct Frustum
	{
		Plane Planes[6];
	};


	struct MinMax
	{
		float3 Min, Max;
	};

	Frustum OffsetFustrum(const Frustum&, const float3);


	/************************************************************************************************/


	union FustrumPoints
	{
		FustrumPoints() noexcept {}
		FustrumPoints(const FustrumPoints& rhs) noexcept
		{
			for (size_t I = 0; I < 8; ++I) {
				Points[I] = rhs.Points[I];
			}
		}

		struct {
			float3 NTL;
			float3 NTR;
			float3 NBL;
			float3 NBR;

			float3 FTL;
			float3 FTR;
			float3 FBL;
			float3 FBR;
		};

		float3 Points[8];
	};


	/************************************************************************************************/


	float                   Intersects(const Ray& R, const Plane& P) noexcept;
	bool                    Intersects(const Frustum& F, const BoundingSphere BS);
	bool                    Intersects(const Frustum frustum, const AABB aabb) noexcept;
	bool                    Intersects(const AABB a, const AABB b) noexcept;
	std::optional<float>    Intersects(const Ray r, const AABB b);
	std::optional<float>    Intersects(const Ray r, const BoundingSphere b);
	bool					Intersects(const Cone c, const AABB);


	/************************************************************************************************/


	FLEXKITAPI inline float3 DirectionVector(float3 A, float3 B) noexcept { return float3{ B - A }.normal(); }


	/************************************************************************************************/


	Frustum GetFrustum(
		const float AspectRatio,
		const float FOV,
		const float Near,
		const float Far,
		float3      Position,
		Quaternion  Q) noexcept;


	Frustum GetSubFrustum(
		const float AspectRatio,
		const float FOV,
		const float Near,
		const float Far,
		float3		Position,
		Quaternion	Q,
		float2		TopLeft,
		float2		BottomRight) noexcept;


}   /************************************************************************************************/

