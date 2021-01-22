/**********************************************************************

Copyright (c) 2014-2020 Robert May

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

#ifndef INTERSECTION_H
#define INTERSECTION_H

#include "MathUtils.h"
#include "containers.h"

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

        Axis LongestAxis() const
        {
            Axis axis = Axis::Axis_count;
            float d = 0.0f;

            const auto span = Min - Max;

            for (size_t I = 0; I < Axis::Axis_count; I++)
            {
                const float axisLength = fabs(span[I]);

                if (d < axisLength)
                {
                    d = axisLength;
                    axis = (Axis)I;
                }
            }

            return axis;
        }

        AABB operator += (const AABB& rhs)
        {
            for (size_t I = 0; I < Axis::Axis_count; I++)
            {
                Min[I] = FlexKit::Min(rhs.Min[I], Min[I]);
                Max[I] = FlexKit::Max(rhs.Max[I], Max[I]);
            }

            return (*this);
        }

        float3 MidPoint() const
        {
            return (Min + Max) / 2;
        }
    };


	typedef float4 BoundingSphere;

	/************************************************************************************************/


	struct Plane
	{
		Plane(float2 D = float2(0.0, 0.0), float3 P = {0.0f, 0.0f, 0.0f}, float3 UpV = { 0.0f, 1.0f, 0.0f }) :
			Dimensions(D), 
			Position(P),
			UpVector(UpV){}

		float2		Dimensions;
		float3		Position;
		float3		UpVector;
		Quaternion	q;// Orientation
	};


	/************************************************************************************************/


	struct Ray
	{
		float3 D;// Direction
		float3 O;// Origin

		Ray(){}

		Ray(float3 origin, float3 dir)
		{
			O = origin;
			D = dir.normal();
		}

		float3 R(float t) { return D*t + O; }
		float3 operator * (float k) { return O + D*k; }
	};


	typedef Vector<Ray> RaySet;

	/************************************************************************************************/
	// Intersection Distance from Origin to Plane Surface

	inline float IntesectTestPR(Ray R, Plane P)
	{
		float3 N;
		float3 w;

		N = P.q * P.UpVector;
		w = P.Position - R.O;

		return w.dot(N) / R.D.dot(N);
	}


	/************************************************************************************************/


	struct FrustumFPlane
	{
		float3 Normal;
		float3 Orgin;
	};


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
		FrustumFPlane Planes[6];
	};


	struct MinMax
	{
		float3 Min, Max;
	};

	/************************************************************************************************/


	union FustrumPoints
	{
		FustrumPoints() {}
		FustrumPoints(const FustrumPoints& rhs)
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


	inline bool Intersects(const Frustum frustum, const AABB aabb)
	{
		int Result = 1;

		for (int I = 0; I < 6; ++I)
		{
			float px = (frustum.Planes[I].Normal.x >= 0.0f) ? aabb.Min.x : aabb.Max.x;
			float py = (frustum.Planes[I].Normal.y >= 0.0f) ? aabb.Min.y : aabb.Max.y;
			float pz = (frustum.Planes[I].Normal.z >= 0.0f) ? aabb.Min.z : aabb.Max.z;

			float3 pV = float3{ px, py, pz } -frustum.Planes[I].Orgin;
			float dP = dot(frustum.Planes[I].Normal, pV);

			if (dP >= 0)
				return false;
		}

		return true;
	}


	/************************************************************************************************/


	FLEXKITAPI inline float3 DirectionVector(float3 A, float3 B) {return float3{ B - A }.normal();}


	/************************************************************************************************/


	Frustum GetFrustum(
		const float AspectRatio,
		const float Near,
		const float Far,
		const float FOV,
		float3 Position,
		Quaternion Q);


	Frustum GetSubFrustum(
		const float AspectRatio,
		const float Near,
		const float Far,
		const float FOV,
		float3		Position,
		Quaternion	Q,
		float2		TopLeft,
		float2		BottomRight);


	/************************************************************************************************/

}

#endif //INTERSECTION_H
