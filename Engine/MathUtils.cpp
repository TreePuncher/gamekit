#ifdef _FLEXKIT
#include "buildsettings.h"
#endif

#include "MathUtils.h"
#include <bit>
#include <immintrin.h>

namespace FlexKit
{
	inline int Exp( int32_t Number, uint32_t exp )
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
		float  IP = DotProduct3(PV, QV);

		float3  Vout = (PV * Q.w) + (QV * P.w) + OP;
		float   Wout = P.w * Q.w - IP;
		
		return Quaternion{Vout, Wout};
	}


	/************************************************************************************************/


	Quaternion	MatrixToQuat(const Matrix<4, 4>& m)
	{
		float tr = m(0, 0) + m(1, 1) + m(2, 2);
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


	Matrix<4, 4> InverseFast(const Matrix<4, 4>& m) noexcept
	{
		constexpr static float f = std::bit_cast<float>(0xffffffff);
		constexpr static __m256 fmask{
							 f, f, f, 0,
							 f, f, f, 0 };

		constexpr static __m256i fIndex0{ .m256i_i32 =
				{   0, 4, 8, 12,
					1, 5, 9, 13 } };

		constexpr static __m256i fIndex1{ .m256i_i32 =
				{   2, 6, 10, 14,
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

	/************************************************************************************************/

	void printfloat2(const float2& in)
	{
		printf("{%f, %f}", in[0], in[1]);
	}

	/************************************************************************************************/

	void printfloat3(const float3& in)
	{
		printf("{%f, %f, %f}", in[0], in[1], in[2]);
	}

	/************************************************************************************************/
	
	void printfloat4(const float4& in)
	{
		printf("{%f, %f, %f, %f}", in[0], in[1], in[2], in[3]);
	}

	/************************************************************************************************/

	void printQuaternion(const Quaternion in)
	{
		printf("{%f, %f, %f, %f}", in[0], in[1], in[2], in[3]);
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
