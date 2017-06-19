/**********************************************************************

Copyright (c) 2014-2017 Robert May

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

#include "..\coreutilities\MathUtils.h"

namespace FlexKit
{
	/************************************************************************************************/


	struct AABB // Axis Aligned Bounding Box
	{
		float3 BottomLeft, TopRight;
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


	FLEXKITAPI bool CompareBSAgainstFrustum(Frustum* F, BoundingSphere BS);


	/************************************************************************************************/


	FLEXKITAPI inline float3 DirectionVector(float3 A, float3 B) {return float3{ B - A }.normal();}

	struct Camera;
	FLEXKITAPI Frustum GetFrustum(Camera* C, float3 Position, Quaternion Q);


	/************************************************************************************************/

}

#endif //INTERSECTION_H
