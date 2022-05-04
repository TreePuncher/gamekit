#include "intersection.h"

namespace FlexKit
{	/************************************************************************************************/


    AABB::Axis AABB::LongestAxis() const noexcept
    {
        Axis axis = Axis::Axis_count;
        float d = 0.0f;

        const auto span = Min - Max;

        for (size_t I = 0; I < Axis::Axis_count; I++)
        {
            const float axisLength = fabs(span[I]);

            if (d < axisLength)
            {
                d       = axisLength;
                axis    = (Axis)I;
            }
        }

        return axis;
    }


    /************************************************************************************************/


    AABB AABB::operator + (const AABB& rhs) const noexcept
    {
        AABB aabb = *this;


//#if USING(FASTMATH)
//        aabb.Min = _mm_cmple_ps(Min, rhs.Min);
//        aabb.Max = _mm_cmpge_ps(Max, rhs.Max);
//#else
        for (size_t I = 0; I < Axis::Axis_count; I++)
        {
            aabb.Min[I] = FlexKit::Min(rhs.Min[I], Min[I]);
            aabb.Max[I] = FlexKit::Max(rhs.Max[I], Max[I]);
        }
//#endif
        return aabb;
    }


    /************************************************************************************************/


    AABB AABB::operator += (const AABB& rhs) noexcept
    {
//#if USING(FASTMATH)
//        Min = _mm_cmple_ps(Min, rhs.Min);
//        Max = _mm_cmpge_ps(Max, rhs.Max);
//#else
        for (size_t I = 0; I < Axis::Axis_count; I++)
        {
            Min[I] = FlexKit::Min(rhs.Min[I], Min[I]);
            Max[I] = FlexKit::Max(rhs.Max[I], Max[I]);
        }
//#endif
        return (*this);
    }


    /************************************************************************************************/


    AABB AABB::operator += (const float3& rhs) noexcept
    {
//#if USING(FASTMATH)
//        Min = _mm_cmple_ps(Min, rhs);
//        Max = _mm_cmpge_ps(Max, rhs);
//#else
        for (size_t I = 0; I < Axis::Axis_count; I++)
        {
            Min[I] = FlexKit::Min(rhs[I], Min[I]);
            Max[I] = FlexKit::Max(rhs[I], Max[I]);
        }
//#endif
        return (*this);
    }


    /************************************************************************************************/


    float3 AABB::MidPoint() const noexcept
    {
        return (Min + Max) / 2;
    }


    /************************************************************************************************/


    float3 AABB::Span() const noexcept
    {
        return Max - Min;
    }


    /************************************************************************************************/


    float3 AABB::Dim() const noexcept
    {
        auto span = Span();
        return { abs(span.x), abs(span.y), abs(span.z) };
    }


    /************************************************************************************************/


    AABB AABB::Offset(const float3 offset) const noexcept
    {
        AABB out = *this;
        out.Min += offset;
        out.Max += offset;

        return out;
    }


    /************************************************************************************************/


    float AABB::AABBArea() const noexcept
    {
        auto span = Span();

        return span.x * span.y * span.z;
    }


    /************************************************************************************************/


    float Intersects(const Ray& R, const Plane& P) noexcept
    {
        const auto w = P.o - R.O;

        return w.dot(P.n) / R.D.dot(P.n);
    }


    /************************************************************************************************/


    bool Intersects(const Frustum frustum, const AABB aabb) noexcept
    {
        int Result = 1;

        for (int I = 0; I < 6; ++I)
        {
            float px = (frustum.Planes[I].n.x >= 0.0f) ? aabb.Min.x : aabb.Max.x;
            float py = (frustum.Planes[I].n.y >= 0.0f) ? aabb.Min.y : aabb.Max.y;
            float pz = (frustum.Planes[I].n.z >= 0.0f) ? aabb.Min.z : aabb.Max.z;

            float3 pV = float3{ px, py, pz } - frustum.Planes[I].o;
            float dP = dot(frustum.Planes[I].n, pV);

            if (dP >= 0)
                return false;
        }

        return true;
    }


    /************************************************************************************************/


    bool Intersects(const AABB a, const AABB b) noexcept
    {
        return  (a.Min.x <= b.Max.x && b.Min.x <= a.Max.x) &&
            (a.Min.y <= b.Max.y && b.Min.y <= a.Max.y) &&
            (a.Min.z <= b.Max.z && b.Min.z <= a.Max.z);
    }


    /************************************************************************************************/


    std::optional<float> Intersects(const Ray r, const AABB aabb)
    {
        const auto Normals = static_vector<float3, 3>(
            {   float3{ 1, 0, 0 },
                float3{ 0, 1, 0 },
                float3{ 0, 0, 1 } });

        float t_min = -INFINITY;
        float t_max = +INFINITY;

        const float3 P = aabb.MidPoint() - r.O;
        const float3 H = aabb.Span() / 2.0f;

        for (size_t I = 0; I < 3; ++I)
		{
			auto e = Normals[I].dot(P);
			auto f = Normals[I].dot(r.D);

			if (abs(f) > 0.00)
			{
				float t_1 = (e + H[I]) / f;
				float t_2 = (e - H[I]) / f;

				if (t_1 > t_2)// Swap
					std::swap(t_1, t_2);

                if (t_1 > t_min) t_min = t_1;
                if (t_2 < t_max) t_max = t_2;

                if (t_min > t_max)	return {};
                if (t_max < 0)		return {};
			}
            else if (-e - H[I] > 0 || -e + H[I] < 0) return {};
		}

		if (t_min > 0)
			return t_min;
		else
			return t_max;
    }


    /************************************************************************************************/


    std::optional<float> Intersects(const Ray r, const BoundingSphere bs)
    {
        const auto Origin = r.O;
		const auto R      = bs.w;
		const auto R2     = R * R;
		const auto S      = r.D.dot(r.D);
		const auto S2     = S * S;
		const auto L2     = r.D.dot(r.D);
		const auto M2     = L2 - S2;

        if (S < 0 && L2 > R2)
            return {}; // Miss

		if(M2 > R2)
            return {}; // Miss

		const auto Q    = sqrt(R2 - M2);

		return (L2 > R2) ? S - Q : S + Q;
    }


    /************************************************************************************************/


	bool Intersects(const Frustum& F, const BoundingSphere BS)
	{
		bool Bottom = false;
		bool Near	= false;
		bool Far	= false;
		bool Left	= false;
		bool Right	= false;
		bool Top	= false;
		const float3 V = BS.xyz();
		const float  r = BS.w;

		{
			const auto P = V - F.Planes[EPlane_FAR].o;
			const auto N = F.Planes[EPlane_FAR].n;

			const float NdP	= N.dot(P);
			Far			    = NdP <= r;
		}
		{
			const auto P = V - F.Planes[EPlane_NEAR].o;
			const auto N = F.Planes[EPlane_NEAR].n;

			const float NdP	= N.dot(P);
			Near		    = NdP <= r;
		}
		{
			const auto P = V - F.Planes[EPlane_TOP].o;
			const auto N = F.Planes[EPlane_TOP].n;

			const float NdP	= N.dot(P);
			Top			    = NdP <= r;
		}
		{
			const auto P = V - F.Planes[EPlane_BOTTOM].o;
			const auto N = F.Planes[EPlane_BOTTOM].n;

            const auto temp = N.magnitude();
			const float NdP	= N.dot(P);
			Bottom		    = NdP <= r;
		}
		{
			auto P = V - F.Planes[EPlane_LEFT].o;
			auto N = F.Planes[EPlane_LEFT].n;

			const float NdP	= N.dot(P);
			Left		    = NdP <= r;
		}
		{
			const auto P = V - F.Planes[EPlane_RIGHT].o;
			const auto N = F.Planes[EPlane_RIGHT].n;

			const float NdP	= N.dot(P);
			Right		    = NdP <= r;
		}

		return (Bottom & Near & Far & Left & Right & Top);
	}


	/************************************************************************************************/


    Frustum OffsetFustrum(const Frustum& f, const float3 offset)
    {
        auto out = f;

        for (auto p : out.Planes)
            p.o += offset;

        return out;
    }


    /************************************************************************************************/


	Frustum GetFrustum(
		const float AspectRatio, 
		const float FOV, 
		const float Near, 
		const float Far, 
		float3 Position, 
		Quaternion Q) noexcept
	{
        float3 FTL(0);
        float3 FTR(0);
        float3 FBL(0);
        float3 FBR(0);

        FTL.z = -Far;
        FTL.y = tan(FOV / 2) * Far;
        FTL.x = -FTL.y * AspectRatio;

        FTR = { -FTL.x,  FTL.y, FTL.z };
        FBL = { FTL.x, -FTL.y, FTL.z };
        FBR = { -FTL.x, -FTL.y, FTL.z };

        float3 NTL(0);
        float3 NTR(0);
        float3 NBL(0);
        float3 NBR(0);

        NTL.z = -Near;
        NTL.y = tan(FOV / 2) * Near;
        NTL.x = -NTL.y * AspectRatio;

        NTR = { -NTL.x,  NTL.y, NTL.z };
        NBL = {  NTL.x, -NTL.y, NTL.z };
        NBR = {  NTR.x, -NTR.y, NTR.z };

        FTL = Position + Q * FTL;
        FTR = Position + Q * FTR;
        FBL = Position + Q * FBL;
        FBR = Position + Q * FBR;

        NTL = Position + Q * NTL;
        NTR = Position + Q * NTR;
        NBL = Position + Q * NBL;
        NBR = Position + Q * NBR;

		Frustum Out;

		{
			float3 N1 = DirectionVector(FTL, FTR);
			float3 N2 = DirectionVector(FTL, FBL);

			Out.Planes[EPlane_FAR].o = (FBL + FTR) / 2;
			Out.Planes[EPlane_FAR].n = N1.cross(N2).normal();
		}
		{
			Out.Planes[EPlane_NEAR].o = (NBL + NTR) / 2;
			Out.Planes[EPlane_NEAR].n = -Out.Planes[EPlane_FAR].n;
		}
		{
			float3 N1 = DirectionVector(FTL, FTR);
			float3 N2 = DirectionVector(FTL, NTL);

			Out.Planes[EPlane_TOP].o = Position;
			Out.Planes[EPlane_TOP].n = -N1.cross(N2).normal();
		}
		{
			float3 N1 = DirectionVector(FBL, FBR);
			float3 N2 = DirectionVector(FBL, NBL);

			Out.Planes[EPlane_BOTTOM].o = Position;
			Out.Planes[EPlane_BOTTOM].n = N1.cross(N2).normal();
		}
		{
			float3 N1 = DirectionVector(FTL, FBL);
			float3 N2 = DirectionVector(FTL, NTL);

			Out.Planes[EPlane_LEFT].o = Position;
			Out.Planes[EPlane_LEFT].n = N1.cross(N2).normal();
		}
		{
			float3 N1 = DirectionVector(FBR, FTR);
			float3 N2 = DirectionVector(FTR, NTR);

			Out.Planes[EPlane_RIGHT].o = Position;
			Out.Planes[EPlane_RIGHT].n = N1.cross(N2).normal();
		}

		return Out;
	}


	/************************************************************************************************/


	Frustum GetSubFrustum(
		const float AspectRatio,
		const float FOV,
		const float Near,
		const float Far,
		float3		Position,
		Quaternion	Q,
		float2		TopLeft,
		float2		BottomRight) noexcept
	{
		float3 FTL(0);
		float3 FTR(0);
		float3 FBL(0);
		float3 FBR(0);

		float3 NTL(0);
		float3 NTR(0);
		float3 NBL(0);
		float3 NBR(0);

		{
			float3 FTL_FULL(0);
			float3 FTR_FULL(0);
			float3 FBL_FULL(0);
			float3 FBR_FULL(0);

			FTL_FULL.z = -Far;
			FTL_FULL.y = tan(FOV / 2) * Far;
			FTL_FULL.x = -FTL_FULL.y * AspectRatio;

			FTR_FULL = { -FTL_FULL.x,  FTL_FULL.y, FTL_FULL.z };
			FBL_FULL = {  FTL_FULL.x, -FTL_FULL.y, FTL_FULL.z };
			FBR_FULL = { -FTL_FULL.x, -FTL_FULL.y, FTL_FULL.z };

			float3 NTL_FULL(0);
			float3 NTR_FULL(0);
			float3 NBL_FULL(0);
			float3 NBR_FULL(0);

			NTL_FULL.z = -Near;
			NTL_FULL.y = tan(FOV / 2) * Near;
			NTL_FULL.x = NTL_FULL.y * AspectRatio;

			NTR_FULL = { -NTL_FULL.x,  NTL_FULL.y, NTL_FULL.z };
			NBL_FULL = { -NTL_FULL.x, -NTL_FULL.y, NTL_FULL.z };
			NBR_FULL = {  NTL_FULL.x, -NTL_FULL.y, NTL_FULL.z };

			FTL.x = lerp(FTL_FULL.x, FTR_FULL.x, TopLeft.x);
			FTL.y = lerp(FTL_FULL.y, FBR_FULL.y, TopLeft.y);
			FTL.z = -Far;

			FTR.x = lerp(FTL_FULL.x, FTR_FULL.x, BottomRight.x);
			FTR.y = lerp(FTL_FULL.y, FBR_FULL.y, TopLeft.y);
			FTR.z = -Far;

			NTL.x = lerp(NTL_FULL.x, NTR_FULL.x, TopLeft.x);
			NTL.y = lerp(NTL_FULL.y, NBR_FULL.y, TopLeft.y);
			NTL.z = -Near;

			NTR.x = lerp(NTL_FULL.x, NTR_FULL.x, BottomRight.x);
			NTR.y = lerp(NTL_FULL.y, NBR_FULL.y, TopLeft.y);
			NTR.z = -Near;

			FBL.x = lerp(FBL_FULL.x, FBR_FULL.x, TopLeft.x);
			FBL.y = lerp(FTL_FULL.y, FBR_FULL.y, BottomRight.y);
			FBL.z = -Far;

			FBR.x = lerp(FBL_FULL.x, FBR_FULL.x, BottomRight.x);
			FBR.y = lerp(FTL_FULL.y, FBR_FULL.y, BottomRight.y);
			FBR.z = -Far;

			NBL.x = lerp(NTL_FULL.x, NTR_FULL.x, TopLeft.x);
			NBL.y = lerp(NTL_FULL.y, NBL_FULL.y, BottomRight.y);
			NBL.z = -Near;

			NBR.x = lerp(NTL_FULL.x, NTR_FULL.x, BottomRight.x);
			NBR.y = lerp(NTR_FULL.y, NBR_FULL.y, BottomRight.y);
			NBR.z = -Near;
		}

		FTL = Position + Q * FTL;
		FTR = Position + Q * FTR;
		FBL = Position + Q * FBL;
		FBR = Position + Q * FBR;

		NTL = Position + Q * NTL;
		NTR = Position + Q * NTR;
		NBL = Position + Q * NBL;
		NBR = Position + Q * NBR;

		Frustum Out;

		{
			float3 N1 = DirectionVector(FTL, FTR);
			float3 N2 = DirectionVector(FTL, FBL);

			Out.Planes[EPlane_FAR].o = (FBL + FTR) / 2;
			Out.Planes[EPlane_FAR].n = N1.cross(N2).normal();
		}
		{
			Out.Planes[EPlane_NEAR].o = (NBL + NTR) / 2;
			Out.Planes[EPlane_NEAR].n = -Out.Planes[EPlane_FAR].n;
		}
		{
			float3 N1 = DirectionVector(FTL, FTR);
			float3 N2 = DirectionVector(FTL, NTL);

			Out.Planes[EPlane_TOP].o = ((FTL + FTR) / 2 + (NTL + NTR) / 2) / 2;
			Out.Planes[EPlane_TOP].n = -N1.cross(N2).normal();
		}
		{
			float3 N1 = DirectionVector(FBL, FBR);
			float3 N2 = DirectionVector(FBL, NBL);

			Out.Planes[EPlane_BOTTOM].o = (FBL + NBR) / 2;
			Out.Planes[EPlane_BOTTOM].n = N1.cross(N2).normal();
		}
		{
			float3 N1 = DirectionVector(FTL, FBL);
			float3 N2 = DirectionVector(FTL, NTL);

			Out.Planes[EPlane_LEFT].o = ((FTL + FBL) / 2 + (NTL + NBL) / 2) / 2;
			Out.Planes[EPlane_LEFT].n = N1.cross(N2).normal();
		}
		{
			float3 N1 = DirectionVector(FBR, FTR);
			float3 N2 = DirectionVector(FTR, NTR);

			Out.Planes[EPlane_RIGHT].o = ((FTR + FBR) / 2 + (NTR + NBR) / 2) / 2;
			Out.Planes[EPlane_RIGHT].n = N1.cross(N2).normal();
		}

		return Out;
	}
}	


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
