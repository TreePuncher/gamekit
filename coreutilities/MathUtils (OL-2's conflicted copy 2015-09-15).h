/**********************************************************************

Copyright (c) 2015 Robert May

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

#ifdef _WIN32
#pragma once
#endif

#ifndef cMyMath_H_INCLUDED
#define cMyMath_H_INCLUDED

#pragma warning ( disable : 4244 )

// Includes
#include "..\buildsettings.h"
#include "containers.h"
#include <math.h>
#include <stdint.h>
#include <string>
#include <emmintrin.h>
#include <xmmintrin.h>
#include <initializer_list>

using namespace std;

namespace FlexKit
{
	static const double	pi = 3.141592653589793;
	static const unsigned int Matrix_Size = 16;

	static const size_t X = 0;
	static const size_t Y = 1;
	static const size_t Z = 2;

	template< typename Ty >
	static Ty DegreetoRad( Ty deg )
	{
		return (Ty)(deg * pi ) /180;
	}

	typedef float FxDef(float);
	FLEXKITAPI float newtonmethod( FxDef fX, FxDef fprimeX, float initial, unsigned int iterations );

	union float2
	{
	public:
		float2() : x(0), y(0) {}
		float2( float X, float Y )
		{
			x = X;
			y = Y;
		}

		inline bool operator == ( const float2& rhs ) const
		{
			if( rhs.x == x )
				if( rhs.y == y )
					return true;
			return false;
		}

		inline float& operator[] ( size_t i )
		{	
		#ifdef _DEBUG
			FK_ASSERT( i < 2 );
		#endif
			return i ? y : x;
		}

		inline float operator[] ( size_t i ) const
		{
#ifdef _DEBUG
			FK_ASSERT( i < 2 );
#endif
			return i ? y : x;
		}

		inline float2 operator+  ( const float2& a )	const { return float2( this->x + a.x, this->y + a.y );			}
		inline float2 operator+  ( const float a )		const { return float2( x + a, y + a );							}
		inline float2 operator-  ( const float2& a )	const { return float2( x - a.x, y - a.y );						}
		inline float2 operator - ( const float a )		const { return float2( this->x - a, this->y - a );				}
		inline float2 operator*  ( const float2& a )	const { return float2( this->x * a.x, this->y * a.y );			}
		inline float2 operator*  ( const float a )		const { return float2( this->x * a, this->y * a );				}
		inline float2 operator +=( const float2& a )	const { return float2( this->x + a.x, this->y + a.y );			}
		inline float2 operator / ( const float2& a )	const { return float2( this->x / a.x, this->y / a.y );			}
		inline float2 operator / ( const float a )		const { return float2( this->x / a, this->y / a );				}
		inline float2 operator % ( const float2& a )	const { return float2( std::fmodf(x, y ), std::fmod( x, y ) );	}

		inline void Add( const float2& lhs, const float2& rhs )
		{
			x = lhs.x + rhs.x;
			y = lhs.y + rhs.y;
		}

		struct
		{
			float x, y;
		};

		float XY[2];

	};

	inline float DotProduct(const __m128 lhs, const __m128 rhs)
	{
#if 0//USING(FASTMATH)
		__m128 res = _mm_dp_ps(lhs, rhs, 0x0E);
		return res.m128_f32[0];
#else
		return ( lhs.m128_f32[0] * rhs.m128_f32[0] ) + ( lhs.m128_f32[1] * rhs.m128_f32[1] ) + ( lhs.m128_f32[2] * rhs.m128_f32[2] );
#endif
	}


	inline void CrossProductSlow(const __m128 lhs, const __m128 rhs, float* __restrict out)
	{
		out[0] = ( lhs.m128_f32[1] * rhs.m128_f32[2] ) - ( lhs.m128_f32[2] * rhs.m128_f32[1] );
		out[1] = ( lhs.m128_f32[2] * rhs.m128_f32[0] ) - ( lhs.m128_f32[0] * rhs.m128_f32[2] );
		out[2] = ( lhs.m128_f32[0] * rhs.m128_f32[1] ) - ( lhs.m128_f32[1] * rhs.m128_f32[0] );

		
	}
	

	inline __m128 CrossProduct( __m128& a, __m128& b )
	{
#if USING(FASTMATH)
		__m128 temp1 = _mm_mul_ps(_mm_shuffle_ps(a, a, 0x01 | 0x02 << 2 | 0x00 << 4 | 0x00 << 6), _mm_shuffle_ps(b, b, 0x02 | 0x00 << 2 | 0x01 << 4 | 0x00 << 6));
		__m128 temp2 = _mm_mul_ps(_mm_shuffle_ps(a, a, 0x02 | 0x00 << 2 | 0x01 << 4 | 0x00 << 6), _mm_shuffle_ps(b, b, 0x01 | 0x02 << 2 | 0x00 << 4 | 0x00 << 6));
		return _mm_sub_ps(temp1, temp2);;
#else
		__m128 out;
		CrossProductSlow(a, b, out.m128_f32);
		return out;
#endif
	}

	union Quaternion;
	FLEXKITAPI Quaternion	GrassManProduct( Quaternion& lhs, Quaternion& rhs );

	__declspec( align( 16 ) ) union float3
	{
	public:
		float3() {}

		template<class TY>
		float3(std::initializer_list<TY> il)
		{
			__m128 V = _mm_set1_ps(0.0f);
			auto n = il.begin();
			for( size_t itr = 0; n != il.end() && itr < 4; ++itr, ++n )
				V.m128_f32[itr] = *n;

			pfloats = V;
		}

#if USING( FASTMATH )

		inline float3 ( float val )						{ pfloats = _mm_set_ps1(val);					}
		inline float3 ( float X, float Y, float Z )		{ pfloats = _mm_set_ps(0.0f, Z, Y, X);			}
		inline float3 ( const float2 in )				{ pfloats = _mm_set_ps(in.x, in.y, 0.0f, 0.0f);	}
		inline float3 ( const float3& a )				{ _mm_store_ps(pfloats.m128_f32, a.pfloats);	}
		inline float3 ( const __m128& in )				{ _mm_store_ps(pfloats.m128_f32, in);			}

#else

		inline float3 ( float val )						{ x = y = z = val;								}
		inline float3 ( float X, float Y, float Z )		{ x = X, y = Y, z = Z;							}
		inline float3 ( const float3& a )				{ x = a.x, y = a.y, z = a.z;					}
		inline float3 ( const __m128& in )				{ pfloats = in;									}
		inline float3 ( const float2 in )				{ x = in.x, y = in.y;							}

#endif

		inline float3 xy() { return float3(x, y, 0.0f); }
		inline float3 xz() { return float3(x, 0.0f, z); }
		inline float3 yz() { return float3(0.0f, y, z); }

		
		inline float3( std::initializer_list<float> il )
		{
			auto n = il.begin();
			for (size_t itr = 0; n != il.end() && itr < 3; ++itr, ++n)
				pfloats.m128_f32[itr] = *n;
		}

		
		inline float& operator[] ( const size_t index )		  { return pfloats.m128_f32[index]; }
		inline float operator[]  ( const size_t index )	const { return pfloats.m128_f32[index]; }
		// Operator Overloads
		inline float3 operator + ( const float& rhs )	const { return float3( x + rhs, y + rhs, z + rhs ); }
		inline float3 operator + ( const float3& rhs )	const { return float3( x + rhs.x, y + rhs.y, z + rhs.z ); }


		inline float3& operator += ( const float3& rhs )
		{
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
			return *this;
		}


		inline float3 operator - ( const float rhs ) const
		{
#if USING(FASTMATH)
			return _mm_sub_ps(pfloats, _mm_set1_ps(rhs));
#else
			return float3( x - rhs, y - rhs, z - rhs );
#endif
		}

		
		inline float3& operator -= ( const float3& rhs )
		{
#if USING(FASTMATH)
			_mm_store_ps(pfloats.m128_f32, _mm_sub_ps(pfloats, rhs.pfloats));
#else
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;
			return *this;
#endif
		}

		
		inline bool operator == ( const float3& rhs ) const
		{
			if( rhs.x == x )
				if( rhs.y == y )
					if( rhs.z == z )
						return true;
			return false;
		}

		
		inline float3 operator - ( const float3& a ) const
		{
#if USING(FASTMATH)
			return _mm_sub_ps(pfloats, a);
#else
			return float3( x - a.x, y - a.y, z - a.z );
#endif
		}

		
		inline float3 operator *	( const float3& a )		const { return float3( x * a.x, y * a.y, z * a.z );	}
		inline float3 operator *	( const float a )		const {	return float3( x * a, y * a, z * a );		}
		
		inline float3& operator *=	( const float3& a )		
		{
			x *= a.x;
			y *= a.y;
			z *= a.z;
			return *this;
		}

		inline float3& operator *= ( float a )
		{
			x *= a;
			y *= a;
			z *= a;
			return *this;
		}

		inline float3 operator / ( const float& a ) const
		{
			return float3( x / a, y / a, z / a );
		}

		inline float3& Scale( float S )
		{
#if USING(FASTMATH)
			pfloats = _mm_mul_ps(pfloats, _mm_set1_ps(S));
#else
			(*this)[0] *= S;
			(*this)[1] *= S;
			(*this)[2] *= S;
			return *this;
#endif
		}

		inline float3 inverse()
		{
			return float3( -x, -y, -z );
		}

		// Identities
		inline float3 cross( float3& rhs ) 
		{
			return CrossProduct( pfloats, rhs.pfloats );
		}
		 

		inline float3 distance( float3 &b )
		{
			return b.magnitude() - magnitude();
		}

		//inline float dot( const float3 &b ) {
		//	return ( x * b.x )+( y * b.y )+( z * b.z );
		//}

		inline float dot( const float3 &b ) const 
		{
			return DotProduct(pfloats, b.pfloats);
		}


		// Slow due to the use of a square root
		inline float magnitude() const 
		{
#if USING(FASTMATH)
			__m128 r = _mm_mul_ps(pfloats, pfloats);
			r = _mm_hadd_ps(r, r);
			r = _mm_hadd_ps(r, r);
			return _mm_mul_ps(_mm_rsqrt_ps(r), _mm_set1_ps(r.m128_f32[0])).m128_f32[0];
#else
			return ::std::sqrt( ( x*x ) + ( y*y ) + ( z*z ) );
#endif
		}


		inline float magnitudesquared() const 
		{
#if USING(FASTMATH)
			__m128 r = _mm_mul_ps(pfloats, pfloats);
			r = _mm_hadd_ps(r, r);
			r = _mm_hadd_ps(r, r);
			return r.m128_f32[0];
#else		
			return ( x*x ) + ( y*y ) + ( z*z );
#endif
		}

		// Slow uses Square roots
		inline float3 normalize() 
		{
#if USING(FASTMATH)
			__m128 sq = _mm_mul_ps(pfloats, pfloats);
			sq = _mm_hadd_ps(sq, sq);
			sq = _mm_hadd_ps(sq, sq);
			return _mm_mul_ps(_mm_rsqrt_ps(sq), pfloats);
#else
			float3 temp = *this;
			return temp / magnitude();
#endif
		}


		operator __m128 () const	 {return pfloats;}
		inline float* toFloat3_ptr() { return reinterpret_cast<float*>( &pfloats ); }


		struct
		{
			float x, y, z, PAD;
		};

		__m128	pfloats;

		private:
		static float3 SetVector( float in )
		{
			float3 V;
			V.x = in;
			V.y = in;
			V.z = in;
			return V;
		}
	};

	inline float3 operator* ( float s, float3 V )
	{
#if USING(FASTMATH)
		return _mm_mul_ps(V, _mm_set1_ps(s));
#else
		return V*s;
#endif
	}

	inline float3 RotateVectorAxisAngle( float3 N, float a, float3 V ) { return V*cos(a) + (V.dot(N) * N * (1-cos(a)) + (N.cross(V)*sin(a)));	}

	union float4
	{
	public:
		float4() {}

		inline float4( float r ) 
		{
#if USING(FASTMATH)
			pFloats = _mm_set1_ps(r);
#else
			x = r;
			y = r;
			z = r;
			w = r;
#endif
		}

		inline float4( float X, float Y, float Z, float W  ) 
		{
#if USING(FASTMATH)
			pFloats = _mm_set_ps(W, Z, Y, X);
#else
			x = X;
			y = Y;
			z = Z;
			w = W;
#endif
		}

		inline float4( const float3& V, float W  ) 
		{
			x = V[0];
			y = V[1];
			z = V[2];
			w = W;
		}

		inline float4( __m128 in ) : pFloats(in){}

		inline float4( const float2& V1, const float2& V2  ) 
		{
			x = V1[0];
			y = V1[1];
			z = V2[0];
			w = V2[1];
		}

		inline float4( std::initializer_list<float> il )
		{
			auto n = il.begin();
			for (size_t itr = 0; n != il.end() && itr < 4; ++itr, ++n)
				pFloats.m128_f32[itr] = *n;
		}

		inline float& operator[] ( const size_t index )			{ return pFloats.m128_f32[index]; }
		inline float operator[]  ( const size_t index )	const	{ return pFloats.m128_f32[index]; }
		inline operator __m128	 ()						const	{ return pFloats;} 

		inline float4 operator+ ( const float4& a ) const 
		{
#if USING(FASTMATH)
			return _mm_add_ps(pFloats, a.pFloats);
#else
			return float4(	x + a.x, 
							y + a.y, 
							z + a.z, 
							w + a.w );
#endif
		}

		inline float4 operator+ ( const float a ) const
		{
#if USING(FASTMATH)
			return _mm_add_ps(pFloats, _mm_set1_ps(a));
#else
			return float4(	x + a, 
							y + a, 
							z + a, 
							w + a );
#endif
		}

		inline float4 operator- ( const float4& a ) const
		{
#if USING(FASTMATH)
			return _mm_sub_ps(pFloats, a);
#else
			return float4(	x - a.x, 
							y - a.y, 
							z - a.z, 
							w - a.w );
#endif
		}

		inline float4 operator- ( const float a ) const
		{
#if USING(FASTMATH)
			return _mm_sub_ps(pFloats, _mm_set1_ps(a));
#else
			return float4(	x - a, 
							y - a, 
							z - a, 
							w - a );
#endif
		}

		inline float4 operator* ( const float4 a ) const
		{
#if USING(FASTMATH)
			return _mm_mul_ps(pFloats, a);
#else
			return float4(	x * a.x, 
							y * a.y, 
							z * a.z, 
							w * a.w );
#endif
		}

		inline float4 operator* ( const float a ) const
		{
#if USING(FASTMATH)
			return _mm_mul_ps(pFloats, _mm_set1_ps(a));
#else
			return float4(	x * a, 
							y * a, 
							z * a, 
							w * a );
#endif
		}

		inline float4 operator += ( const float4& a )
		{
#if USING(FASTMATH)
			_mm_store_ps(pFloats.m128_f32, _mm_mul_ps(pFloats, a));
#else
			return float4(	x + a.x, 
							y + a.y, 
							z + a.z, 
							w + a.w );
#endif
			return pFloats;
		}

		inline float4 operator / ( const float4& a ) const
		{
#if USING(FASTMATH)
			return _mm_div_ps(pFloats, a);
#else
			return float4(	x / a.x, 
							y / a.y, 
							z / a.z, 
							w / a.w );
#endif
		}

		inline float4 operator / ( const float a ) const
		{
#if USING(FASTMATH)
			return _mm_div_ps(pFloats, _mm_set1_ps(a));
#else
			return float4(	x / a, 
							y / a, 
							z / a, 
							w / a );
#endif
		}

		inline float4 operator % ( const float4& a ) const
		{
			return float4(	std::fmodf( x, a.x ), 
							std::fmodf( y, a.y ),
							std::fmodf( z, a.z ),
							std::fmodf( w, a.w ) );
		}

		struct
		{
			float x, y, z, w;
		};

		__m128 pFloats;
	};

	__declspec( align( 16 ) ) union Quaternion
	{
	public:
		inline Quaternion() {}
		
		inline explicit Quaternion( const float3& vector, float scaler )
		{
#if USING(FASTMATH)
			floats = _mm_set_ps(scaler, vector[2], vector[1],vector[0]);
#else
			x = vector[0];
			y = vector[1];
			z = vector[2];
			w = scaler;
#endif
		}

		inline explicit Quaternion( float X, float Y, float Z, float W )
		{
#if USING(FASTMATH)
			floats = _mm_set_ps(W, Z, Y, X);
#else
			x = X;
			y = Y;
			z = Z;
			w = W;
#endif
		}

		inline Quaternion( std::initializer_list<float> il )
		{
			auto n = il.begin();
			for (size_t itr = 0; n != il.end() && itr < 4; ++itr, ++n)
				floats.m128_f32[itr] = *n;
		}

		inline Quaternion( __m128 in )
		{
			floats = in;
		}

		inline Quaternion( float* in )
		{
			memcpy( floats.m128_f32, in, sizeof( floats ) );
		}

		inline Quaternion( const Quaternion& in ) : 
			floats( in.floats )
		{
		}

		inline explicit Quaternion( float dX, float dY, float dZ ) // Degrees to Quat
		{
			FlexKit::Quaternion X, Y, Z;
			X.Zero();
			Y.Zero();
			Z.Zero();

			float dX1_2 = dX/2;
			float dY1_2 = dY/2;
			float dZ1_2 = dZ/2;
			X.x = std::sin( dX1_2 );
			X.w = std::cos( dX1_2 );
			Y.y = std::sin( dY1_2 );
			Y.w = std::cos( dY1_2 );
			Z.z = std::sin( dZ1_2 );
			Z.w = std::cos( dZ1_2 );

			( *this ) = X.normalize() * Y.normalize() * Z.normalize();
			*this = normalize();
		}

		inline Quaternion operator * ( Quaternion& q )
		{
			return GrassManProduct(*this, q);
		}

		inline float3 operator * ( float3& V )
		{
#if 1
			Quaternion Q = *this;
			auto res = Q * Quaternion(V, 0) * Q.Inverse();

#else
			auto v = this->XYZ() * -1;
			auto vXV = v.cross(V);
			auto test = v.cross(vXV);
			auto ret = float3(V + (vXV * (2 * w)) + (v.cross(vXV) * 2));
#endif
			return res.V();
		}

		inline Quaternion operator * ( float scaler )
		{
			__m128 r = floats; 
			__m128 s = _mm_set1_ps(scaler); 
			return _mm_mul_ps(r, s);
		}

		inline Quaternion& operator *= ( Quaternion& rhs )
		{
			return (*this)*rhs;
		}

		inline Quaternion& operator = ( Quaternion& rhs )
		{
			floats = rhs.floats;
			return (*this);
		}

		inline Quaternion& operator = ( __m128 rhs )
		{
			floats = rhs;
			return (*this);
		}

		inline float& operator [] ( const size_t index )
		{
			return floats.m128_f32[index];
		}

		inline float operator [] ( const size_t index ) const
		{
			return floats.m128_f32[index];
		}

		inline operator float* ()
		{
			return floats.m128_f32;
		}

		inline operator const float* () const
		{
			return floats.m128_f32;
		}

		inline operator __m128 ()
		{
			return floats;
		}

		inline operator const __m128 () const
		{
			return floats;
		}

		template< typename Ty_2 >
		inline Quaternion& operator = ( const float4& rhs )
		{
			x = rhs.x;
			y = rhs.y;
			z = rhs.z;
			w = rhs.w;

			return (*this);
		}

		inline Quaternion Conjugate() 
		{
			Quaternion Conjugate;
#if USING(FASTMATH)
			Conjugate = _mm_mul_ps(_mm_set_ps(1, -1, -1, -1), floats);
#else
			Conjugate.x = -x;
			Conjugate.y = -y;
			Conjugate.z = -z;
			Conjugate.w = w;
#endif
			return Conjugate;
		}

		inline Quaternion Inverse() 
		{
			return  Conjugate();
		}
		inline float3 XYZ() const	{return float3(x, y, z);}
		inline float3 V() const		{return float3(x, y, z);}

		inline float Magnitude()
		{
#if USING( FASTMATH )
			__m128 q2 = _mm_mul_ps(floats, floats);
			q2 = _mm_hadd_ps(q2, q2);
			q2 = _mm_hadd_ps(q2, q2);
			return q2.m128_f32[0];
#else
			return x * x + y * y + z * z + w * w;
#endif
		}

		inline Quaternion normalize() 
		{
			float mag2 = Magnitude();
			if( mag2 != 0 && ( fabs( mag2 - 1.0f ) > .00001f ) )
			{
#if USING(FASTMATH)

				__m128 rsq = _mm_rsqrt_ps(_mm_set1_ps(mag2));
				floats = _mm_mul_ps(rsq, floats);
#else
				float mag  = sqrt( mag2 );
				w = w / mag;
				x = x / mag;
				y = y / mag;
				z = z / mag;
#endif
			}
			return *this;
		}

		inline void Zero() 
		{
#if USING(FASTMATH)
			floats = _mm_set1_ps(0);
#else
			x = 0;
			y = 0;
			z = 0;
			w = 0;
#endif
		}

		static Quaternion Identity()
		{
			Quaternion Q(0, 0, 0, 1);
			return Q;
		}

		struct
		{
			float x, y, z, w;
		};

		 __m128	floats;
	};

	enum XYZ
	{
		x = 0,
		y = 0,
		z = 0
	};

	template< unsigned int SIZE, typename TY = float >
	class Vect
	{
		typedef Vect<SIZE, TY> THISTYPE;
	public:
		Vect()
		{
		}

		Vect( TY n )
		{
			for( auto e : Vector )
				e = n;
		}
		template<typename TY_2>
		Vect( Vect<SIZE, TY_2> in)
		{
			for (size_t I=0; I<SIZE; ++I)
				Vector[I] = in[I];
		}

		Vect( std::initializer_list<TY> il )
		{
			size_t itr = 0;
			for( auto n : il )
			{
				Vector[itr++] = n;
				if( itr > SIZE )
					return;
			}
		}

		template< typename TY_i >
		Vect<SIZE, TY>	operator *( TY_i scaler )
		{
			Vect<SIZE, TY>	out;
			size_t i = 0;
			for( auto element : Vector )
			{
				out[i] = element * scaler;
				++i;
			}
			return out;
		}

		template< typename TY_i >
		Vect<SIZE, TY>	operator *( Vect<SIZE, TY_i> rhs )
		{
			Vect<SIZE, TY>	out;
			for (auto i = 0; i < SIZE; ++i)
				out[i]= Vector[i] * rhs[i];
			return out;
		}

		template< typename TY_i >
		Vect<3, TY>	Cross( Vect<3, TY_i> rhs )
		{
			Vect<SIZE, TY> out;
			for( size_t i = 0; i < SIZE; ++i )
				out[i] = ( Vector[(1+i)%SIZE] * rhs[(2+i)%SIZE] ) - ( rhs[(1+i)%SIZE] * Vector[(2+i)%SIZE] );
			return out;
		}

		template< typename TY_i >
		TY Dot( const Vect<SIZE, TY_i>& rhs ) const
		{
			TY dotproduct = 0;
			for( size_t i = 0; i < SIZE; ++i )
				dotproduct += rhs[i] * Vector[i];

			return dotproduct;
		}

		template< typename TY_i >
		TY Dot( const Vect<SIZE, TY_i>* rhs_ptr )
		{
			auto& rhs = *rhs_ptr;
			Vect<SIZE> products;
			for( size_t i = 0; i < SIZE; ++i )
				products[i] += rhs[i] * Vector[i];

			return products.Sum();
		}

		TY Norm( unsigned int exp = 2 )
		{
			TY sum = 0;
			for( auto element : Vector )
			{
				TY product = 0;
				for( size_t i = 0; i < SIZE; i++ )
					product *= element;
				sum += product;	
			}

			return pow( sum, 1/SIZE );
		}

		TY Magnitude()
		{
			TY sum = 0;
			for( auto element : Vector )
			{
				sum += element * element;	
			}

			return sqrt( sum );
		}

		TY& operator []( size_t index )
		{
			return Vector[index];
		}

		const TY& operator []( size_t index ) const
		{
			return Vector[index];
		}

		TY Sum() const
		{
			TY sum = 0;
			for( auto element : Vector )
				sum += element;
			return sum;
		}

		template<typename TY_2>
		THISTYPE operator + (const Vect<SIZE, TY_2>& in)
		{
			THISTYPE temp = *this;
			for (auto I = 0; I < SIZE; ++I)
				temp[I] += in[I];
			
			return temp;
		}

		template<typename TY_2>
		THISTYPE& operator += (const Vect<SIZE, TY_2>& in)
		{
			for (auto I = 0; I < SIZE; ++I)
				Vector[I] += in[I];
			
			return *this;
		}

		template<typename TY_2>
		THISTYPE operator - (const Vect<SIZE, TY_2>& in)
		{
			THISTYPE temp = *this;
			for (auto I = 0; I < SIZE; ++I)
				temp[I] -= in[I];
			
			return temp;
		}

		template<typename TY_2>
		THISTYPE& operator -= ( const Vect<SIZE, TY_2>& in )
		{
			for (auto I = 0; I < SIZE; ++I)
				Vector[I] -= in[I];
			
			return *this;
		}

		template<typename TY_2>
		THISTYPE& operator = (const Vect<SIZE, TY_2>& in)
		{
			for (auto I = 0; I < SIZE; ++I)
				Vector[I] = in[I];
			
			return *this;
		}

		template<typename TY_2>
		THISTYPE& operator = ( const std::initializer_list<TY_2>& il )
		{
			size_t itr = 0;
			for( auto n : il )
			{
				Vector[itr++] = n;
				if( itr > SIZE )
					break;
			}
			return *this;
		}

		static THISTYPE Zero()
		{
			THISTYPE zero;
			for( auto element : zero.Vector )
				element = 0;

			return zero;
		}

		TY Vector[SIZE];
	};

	typedef Vect<2> Vect2;
	typedef Vect<3> Vect3;
	typedef Vect<4> Vect4;
	
	typedef Vect<3, double> double3;
	typedef Vect<4, double> double4;
	
	typedef Vect<2, size_t> uint2;
	typedef Vect<3, size_t> uint3;
	typedef Vect<4, size_t> uint4;

	typedef Vect<2, int> int2;
	typedef Vect<3, int> int3;
	typedef Vect<4, int> int4;

	// Row Major
	template< const int ROW, const int COL, typename Ty = float >
	class __declspec( align( 64 ) ) Matrix
	{
	public:
		Matrix()
		{
			for( float* col : matrix )
				for( size_t i = 0; i < ROW; i++ )
					col[i] = 0;
		}

		template< const int RHS_COL >
		Matrix<ROW, RHS_COL>	operator*( const Matrix<ROW, RHS_COL>& rhs )
		{
			static_assert( ROW == RHS_COL, "ROW AND RHS COLS DO NOT MATCH" );
			Matrix<ROW, RHS_COL> out;
			auto transposed = rhs.Transpose();

			for( size_t i = 0; i < ROW; ++i )
			{
				const auto v = *((Vect<4>*)matrix[i]);
				for( size_t i2 = 0; i2 < COL; ++i2 )
				{
					const auto v2 = *((Vect<4>*)transposed[i2]);
					out[i][i2] = v.Dot( v2 );
				}
			}
			return out;
		}

		float* operator[] ( size_t i )
		{
			return matrix[i];
		}

		const float* operator[] ( size_t i ) const
		{
			return matrix[i];
		}

		static Matrix<ROW, COL> Identity()
		{
			Matrix<ROW, COL> m;
			for( size_t i = 0; i < ROW; i++ )
				m[i][i] = 1;
			return m;
		}

		Matrix<ROW, COL> Transpose() const
		{
			Matrix<ROW, COL> m_transposed;

			for( size_t col = 0; col < COL; ++col )
				for( size_t row = 0; row < ROW; ++row )
					m_transposed[col][row] = matrix[row][col];

			return m_transposed;
		}

	private:
		Ty matrix[COL][ROW];
	};

	typedef FlexKit::Matrix<4,4> float4x4;

	FLEXKITAPI inline float CalcMatrixTrace( float in[Matrix_Size] )
	{
		float sum = 0.0f;
		for( size_t itr = 0; itr < Matrix_Size; itr++ )
			for( size_t itr_2 = 0;itr_2 < 3; itr_2+= 4, itr + itr_2 )
				sum += in[itr];
		return sum;
	}

	template<typename TY>
	inline TY Lerp(TY& A, TY& B, float t)
	{
		return A(1.0f - t) + t*B;
	}

	namespace Conversion
	{
		template< typename Ty >
		static float4 toVector3( Ty& Convert )
		{
			return Vect4( Convert.x, Convert.y, Convert.z );
		}

		template< typename Ty >
		inline Ty FromVector3Conversion( const Vect4& Convert )
		{
			Ty Convertto;
			Convertto.x = Convert.x;
			Convertto.y = Convert.y;
			Convertto.z = Convert.z;
			return Convertto;
		}

		inline float4 MatrixToVect4( float in[16] )
		{
			FK_ASSERT(0);
			float trace = CalcMatrixTrace( in );
			return float4();
		}
	}

	FLEXKITAPI int			Exp( int32_t Number, uint32_t exp );
	FLEXKITAPI Quaternion	MatrixToQuat( Matrix<4,4>& );
	FLEXKITAPI void			NumberToString( int32_t n, string& _Dest );
	FLEXKITAPI int			Testing();

	FLEXKITAPI void printfloat2(const float2& in);
	FLEXKITAPI void printfloat3(const float3& in);
	FLEXKITAPI void printfloat4(const float4& in);
}

#endif