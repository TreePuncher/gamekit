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

        AABB operator + (const AABB& rhs) const noexcept;
        AABB operator +=(const AABB& rhs) noexcept;
        AABB operator +=(const float3& rhs) noexcept;

        float3 MidPoint() const noexcept;
        float3 Span() const noexcept;
        float3 Dim() const noexcept;

        AABB    Offset(const float3 offset) const noexcept;
        float   AABBArea() const noexcept;
    };


	typedef float4 BoundingSphere;


    inline AABB operator + (const AABB& lhs, const BoundingSphere& rhs) noexcept
    {
        auto lowerBound = rhs.xyz() -= rhs.w;
        auto upperBound = rhs.xyz() += rhs.w;

        AABB A;
        A.Min = lowerBound;
        A.Max = upperBound;

        return A;
    }



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
	// Intersection Distance from Origin to Plane Surface

	inline float IntesectTestPR(Ray R, Plane P) noexcept
	{
		float3 N;
		float3 w;

		N = P.n;
		w = P.o - R.O;

		return w.dot(N) / R.D.dot(N);
	}

    inline float Intesects(Ray R, Plane P) noexcept
    {
        return IntesectTestPR(R, P);
    }


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


	bool Intersects(const Frustum F, const BoundingSphere BS);


	/************************************************************************************************/


	inline bool Intersects(const Frustum frustum, const AABB aabb) noexcept
	{
		int Result = 1;

		for (int I = 0; I < 6; ++I)
		{
			float px = (frustum.Planes[I].n.x >= 0.0f) ? aabb.Min.x : aabb.Max.x;
			float py = (frustum.Planes[I].n.y >= 0.0f) ? aabb.Min.y : aabb.Max.y;
			float pz = (frustum.Planes[I].n.z >= 0.0f) ? aabb.Min.z : aabb.Max.z;

			float3 pV = float3{ px, py, pz } -frustum.Planes[I].o;
			float dP = dot(frustum.Planes[I].n, pV);

			if (dP >= 0)
				return false;
		}

		return true;
	}


    /************************************************************************************************/


    inline bool Intersects(const AABB a, const AABB b) noexcept
	{
        return  (a.Min.x <= b.Max.x && b.Min.x <= a.Max.x) &&
                (a.Min.y <= b.Max.y && b.Min.y <= a.Max.y) &&
                (a.Min.z <= b.Max.z && b.Min.z <= a.Max.z);
	}


    /************************************************************************************************/


    std::optional<float> Intersects(const Ray r, const AABB b);
    std::optional<float> Intersects(const Ray r, const BoundingSphere b);


	/************************************************************************************************/


	FLEXKITAPI inline float3 DirectionVector(float3 A, float3 B) noexcept { return float3{ B - A }.normal(); }


	/************************************************************************************************/


	Frustum GetFrustum(
		const float AspectRatio,
		const float Near,
		const float Far,
		const float FOV,
		float3 Position,
		Quaternion Q) noexcept;


	Frustum GetSubFrustum(
		const float AspectRatio,
		const float Near,
		const float Far,
		const float FOV,
		float3		Position,
		Quaternion	Q,
		float2		TopLeft,
		float2		BottomRight) noexcept;


	/************************************************************************************************/

}

