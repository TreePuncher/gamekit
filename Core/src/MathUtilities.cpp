#include "BuildSettings.hpp"
#include "MathUtilities.hpp"

#include <bit>
#include <simde/x86/avx2.h>

namespace FlexKit
{
	inline int Exp(int32_t Number, uint32_t exp)
	{
		if( exp != 0) {
			int Origin = Number;
			while( exp > 1 ) {
				Number = Number * Origin;
				exp = exp - 1;
			}
		} else {
			Number = 1;
		}
		return Number;
	}


	Quaternion GrassManProduct(const Quaternion& P, const Quaternion& Q) noexcept
	{
		float3 PV = P.XYZ();
		float3 QV = Q.XYZ();

		float3 OP = PV.cross(QV);
		float  IP = PV.dot(QV);

		float3  Vout = (PV * Q.w) + (QV * P.w) + OP;
		float   Wout = P.w * Q.w - IP;
		
		return Quaternion{Vout, Wout};
	}


	/************************************************************************************************/


	Quaternion	MatrixToQuat(const float4x4& m)
	{
		float tr = m[0, 0] + m[1, 1] + m[2, 2];
		float qw, qx, qy, qz;

		if (tr > 0)
		{ 
			float S = sqrt(tr + 1.0f) * 2.0f; // S=4*qw 
			qw = 0.25f * S;
			qx = (m(2, 1) - m(1, 2)) / S;
			qy = (m(0, 2) - m(2, 0)) / S;
			qz = (m(1, 0) - m(0, 1)) / S; 
		} else if ((m(0, 0) > m(1, 1)) & (m(0, 0) > m(2, 2))) {
			float S = sqrt(1.0f + m(0, 0) - m(1, 1) - m(2, 2)) * 2.0f; // S=4*qx 
			qw = (m(2, 1) - m(1, 2)) / S;
			qx = 0.25f * S;
			qy = (m(0, 1) + m(1, 0)) / S;
			qz = (m(0, 2) + m(2, 0)) / S; 
		} else if (m(1, 1) > m(2, 2)) { 
			float S = sqrt(1.0f + m(1, 1) - m(0, 0) - m(2, 2)) * 2.0f; // S=4*qy
			qw = (m(0, 2) - m(2, 0)) / S;
			qx = (m(0, 1) + m(1, 0)) / S; 
			qy = 0.25f * S;
			qz = (m(1, 2) + m(2, 1)) / S; 
		} else { 
			float S = sqrt(1.0f + m(2, 2) - m(0, 0) - m(1, 1)) * 2.0f; // S=4*qz
			qw = (m(1, 0) - m(0, 1)) / S;
			qx = (m(0, 2) + m(2, 0)) / S;
			qy = (m(1, 2) + m(2, 1)) / S;
			qz = 0.25f * S;
		}

		return { qx, qy, qz, qw };
	}


	/************************************************************************************************/


	float saturate(float x) noexcept
	{
		return clamp(0.0f, x, 1.0f);
	}


	/************************************************************************************************/


	float3 saturate(float3 v) noexcept
	{
		float3 out = v;
		v.x = Min(Max(v.x, 0.0f), 1.0f);
		v.y = Min(Max(v.y, 0.0f), 1.0f);
		v.z = Min(Max(v.z, 0.0f), 1.0f);

		return out;
	}


	/************************************************************************************************/


	float3 TripleProduct(const float3 A, const float3 B, const float3 C) noexcept
	{
		return (B - A).cross(C - A);
	}


	/************************************************************************************************/


	float3 operator* (float s, float3 V) noexcept
	{
		return V * float3{ s };
	}

	
	/************************************************************************************************/


	float3 RotateVectorAxisAngle(float3 N, float a, float3 V) noexcept
	{
		return V * cos(a) + (V.dot(N) * N * (1 - cos(a)) + (N.cross(V) * sin(a)));
	}


	/************************************************************************************************/


	Matrix<4, 4> InverseFast(const Matrix<4, 4>& m) noexcept
	{
#ifdef WIN32
		constexpr static float f = std::bit_cast<float>(0xffffffff);
		constexpr static __m256 fmask{
							 f, f, f, 0,
							 f, f, f, 0 };

		constexpr static __m256i fIndex0{ .m256i_i32 = {
					0, 4, 8, 12,
					1, 5, 9, 13  } };

		constexpr static __m256i fIndex1{ .m256i_i32 = {
					2, 6, 10, 14,
					3, 7, 11, 15 } };

		constexpr static __m128 _x{ -1,  0, 0, 0 };
		constexpr static __m128 _y{ 0, -1, 0, 0 };
		constexpr static __m128 _z{ 0,  0,-1, 0 };
		constexpr static __m128 _w{ 0,  0, 0, 1 };

		__m256 v256m0 = _mm256_mask_i32gather_ps(_mm256_set1_ps(0), m.Data(), fIndex0, fmask, 4);
		__m256 v256m1 = _mm256_mask_i32gather_ps(_mm256_set1_ps(0), m.Data(), fIndex1, fmask, 4);

		auto temp0 = _mm256_dp_ps(v256m0, v256m0, 0xff);
		auto temp1 = _mm256_dp_ps(v256m1, v256m1, 0xff);

		auto scale0 = _mm256_shuffle_f32x4(temp0, temp0, 0x02);
		auto scale1 = _mm256_shuffle_f32x4(temp1, temp1, 0x00);

		v256m0 = _mm256_div_ps(v256m0, scale0);
		v256m1 = _mm256_div_ps(v256m1, scale1);

		__m128 v0_128 = _mm256_extractf128_ps(v256m0, 0);
		__m128 v1_128 = _mm256_extractf128_ps(v256m0, 1);
		__m128 v2_128 = _mm256_extractf128_ps(v256m1, 0);
		__m128 M_R_3  = m.vectorView.V128AtX64(3);

		auto t		= _mm_mul_ps(	_mm_dp_ps(v0_128, M_R_3, 0x77), _x);
		t			= _mm_fmadd_ps(	_mm_dp_ps(v1_128, M_R_3, 0x77), _y, t);
		t			= _mm_fmadd_ps(	_mm_dp_ps(v2_128, M_R_3, 0x77), _z, t);
		t			= _mm_add_ps(t, _w);

		v256m1 = _mm256_insertf128_ps(v256m1, t, 1);

		Matrix<4, 4> out;
		out.vectorView.V256AtX64(0) = v256m0;
		out.vectorView.V256AtX64(1) = v256m1;

		return out;
#else
		return {};
#endif
	}


	/************************************************************************************************/


	FlexKit::float4x4 Quaternion2Matrix(const Quaternion q)
	{
#ifdef WIN32
		float4x4 m;
		auto [x, y, z, w] = q.floats.m128_f32;

		const float xx = x * x;
		const float yy = y * y;
		const float zz = z * z;
		
		const float xy = x * y;
		const float xz = x * z;
		const float yz = y * z;
		const float wx = w * x;
		const float wy = w * y;
		const float wz = w * z;

		m(0,0) = 1.0f - 2.0f * yy - 2.0f * zz;
		m(1,0) = 2.0f * xy + 2.0f * wz;
		m(2,0) = 2.0f * xz - 2.0f * wy;
		m(3,0) = 0.0f;

		m(0,1) = 2.0f * xy - 2.0f * wz;
		m(1,1) = 1.0f - 2.0f * xx - 2.0f * zz;
		m(2,1) = 2.0f * yz + 2.0f * wx;
		m(3,1) = 0.0f;

		m(0,2) = 2.0f * xz + 2.0f * wy;
		m(1,2) = 2.0f * yz - 2.0f * wx;
		m(2,2) = 1.0f - 2.0f * xx - 2.0f * yy;
		m(3,2) = 0.0f;
		
		m(0,3) = 0.0f;
		m(1,3) = 0.0f;
		m(2,3) = 0.0f;
		m(3,3) = 1.0f;

		return m;
#else
		return {};
#endif
	}


	/************************************************************************************************/


	Quaternion Matrix2Quat(const float4x4& M)
	{
#if USING(FASTMATH)
		Quaternion Q
		(
			1.0f + M(0,0) - M(1,1) - M(2,2), 
			1.0f - M(0,0) + M(1,1) - M(2,2), 
			1.0f - M(0,0) - M(1,1) + M(2,2), 
			1.0f + M(0,0) + M(1,1) + M(2,2)
		);

		simde__m128 Temp1 = simde_mm_max_ps(Q, simde_mm_set1_ps(0.0f));
		Temp1 = simde_mm_sqrt_ps(Temp1);
		Temp1 = simde_mm_mul_ps(Temp1, simde_mm_set1_ps(0.5f));

		// Copy Sign
		simde__m128 Temp3	= simde_mm_set_ps(GetFirst(Temp1),		M(0,1), M(2,0), M(1, 2));
		simde__m128 Temp4	= simde_mm_set_ps(0.0f,					M(1,0), M(0,2), M(2, 1));
		simde__m128 Temp5	= simde_mm_sub_ps(Temp3, Temp4);
		simde__m128 res		= SSE_CopySign(Temp5, Temp1);

		return Quaternion{ res }.normalize();
#else 

		Quaternion Q
		{
			sqrtf( Max(1.0f + M[0][0] - M[1][1] - M[2][2], 0.0f))/2, 
			sqrtf( Max(1.0f - M[0][0] + M[1][1] - M[2][2], 0.0f))/2, 
			sqrtf( Max(1.0f - M[0][0] - M[1][1] + M[2][2], 0.0f))/2, 
			sqrtf( Max(1.0f + M[0][0] + M[1][1] + M[2][2], 0.0f))/2
		};
		return Quaternion
		{
			_copysignf(Q.x, M[1][2] - M[2][1]),
			_copysignf(Q.y, M[2][0] - M[0][2]),
			_copysignf(Q.z, M[0][1] - M[1][0]),
			Q.w
		};
#endif
	}


	/************************************************************************************************/


	float3 GetTranslation(const float4x4&)
	{
		return float3{ 0, 0, 0 };
	}


	float dot(const float3 lhs, const float3 rhs)
	{
		return DotProduct3(lhs, rhs);
	}


	float dot(const float4 lhs, const float4 rhs)
	{
		return DotProduct4(lhs, rhs);
	}


	/************************************************************************************************/


	float4x4 Vector2RotationMatrix(const float3& Forward, const float3& Up, const float3& Right)
	{
		float4x4 Out = float4x4::Identity();
		Out(0, 0) = Forward.x;
		Out(0, 1) = Forward.y;
		Out(0, 2) = Forward.z;
		Out(0, 3) = 0;

		Out(1, 0) = Up.x;
		Out(1, 1) = Up.y;
		Out(1, 2) = Up.z;
		Out(1, 3) = 0;

		Out(2, 0) = Right.x;
		Out(2, 1) = Right.y;
		Out(2, 2) = Right.z;
		Out(2, 3) = 0;

		return Out;
	}


	/************************************************************************************************/


	Quaternion Vector2Quaternion(const float3& Forward, const float3& Up, const float3& Right)
	{
		return Matrix2Quat(Vector2RotationMatrix(Forward, Up, Right));
	}



	Quaternion PointAt(float3 A, float3 B, const float3 UpV)
	{
		float3 Dir = (B - A).normal();
		Dir = { Dir.z, Dir.y, -Dir.x };
		const float3 DirXUpV = Dir.cross(UpV);

		return Vector2Quaternion(Dir, DirXUpV.cross(Dir), DirXUpV);
	}


	/************************************************************************************************/


	float4x4 PerspectiveRH(const float FOV, const float minZ, const float maxZ, const float aspectRatio)
	{
		float TwoNearZ	= minZ + minZ;
		float fRange	= maxZ / (minZ - maxZ);


		float sinFOV = std::sin(0.5f * FOV);
		float cosFOV = std::cos(0.5f * FOV);

		float height	= cosFOV / sinFOV;
		float width		= height / aspectRatio;
		float range		= maxZ / (minZ - maxZ);

		float4x4 m = float4x4::Identity();
		m(0, 0) = width;
		m(0, 1) = 0.0f;
		m(0, 2) = 0.0f;
		m(0, 3) = 0.0f;

		m(1, 0) = 0.0f;
		m(1, 1) = height;
		m(1, 2) = 0.0f;
		m(1, 3) = 0.0f;

		m(2, 0) = 0.0f;
		m(2, 1) = 0.0f;
		m(2, 2) = fRange;
		m(2, 3) = -1.0f;

		m(3, 0) = 0.0f;
		m(3, 1) = 0.0f;
		m(3, 2) = range * minZ;
		m(3, 3) = 0.0f;

		return m;
	}


	/************************************************************************************************/


	void NumberToString( int32_t n, std::string& _Dest )
	{
		std::string Tmp;
		uint32_t pwr = 9;
		uint32_t exp = 1 * Exp( 10, pwr );
		uint32_t remainder = n;

		while( pwr >= 0 ) {
			uint32_t Test = remainder/exp;
			switch( remainder/exp ) {
			case 1:
				Tmp.push_back( '1' );
				break;
			case 2:
				Tmp.push_back( '2' );
				break;
			case 3:
				Tmp.push_back( '3' );
				break;
			case 4:
				Tmp.push_back( '4' );
				break;
			case 5:
				Tmp.push_back( '5' );
				break;
			case 6:
				Tmp.push_back( '6' );
				break;
			case 7:
				Tmp.push_back( '7' );
				break;
			case 8:
				Tmp.push_back( '8' );
				break;
			case 9:
				Tmp.push_back( '9' );
				break;
			case 0:
				Tmp.push_back( '0' );
				break;
			default:
				break;
			}
			pwr--;
			exp = Exp( 10, pwr);
			if( exp != 1) {
				remainder = n%exp;
			} else {
				remainder = n%10;
			}
		}
		_Dest = Tmp;
	}


	void printfloat2(const float2& in)
	{
		printf("{%f, %f}", in[0], in[1]);
	}


	void printfloat3(const float3& in)
	{
		printf("{%f, %f, %f}", in[0], in[1], in[2]);
	}


	void printfloat4(const float4& in)
	{
		printf("{%f, %f, %f, %f}", in[0], in[1], in[2], in[3]);
	}


	void printQuaternion(const Quaternion in)
	{
		printf("{%f, %f, %f, %f}", in[0], in[1], in[2], in[3]);
	}



	std::ostream& operator << (std::ostream& stream, float2 xyz)
	{
		stream << "{ " << xyz.x << ", " << xyz.y << " }";
		return stream;
	}


	std::ostream& operator << (std::ostream& stream, float3 xyz)
	{
		stream << "{ " << xyz.x << ", " << xyz.y << ", " << xyz.z << " }";
		return stream;
	}


	std::ostream& operator << (std::ostream& stream, float4 xyz)
	{
		stream << "{ " << xyz.x << ", " << xyz.y << ", " << xyz.z << ", " << xyz.w << " }";
		return stream;
	}


	std::ostream& operator << (std::ostream& stream, Quaternion q)
	{
		stream << "{ i * " << q.x << ", j * " << q.y << ", k * " << q.z << ", " << q.w << " }";
		return stream;
	}


}	/************************************************************************************************/


/**********************************************************************

Copyright (c) 2015 - 2023 Robert May

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
